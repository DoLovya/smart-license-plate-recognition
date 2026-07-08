import argparse
import csv
import random
import shutil
import struct
from dataclasses import dataclass
from pathlib import Path

IMAGE_SUFFIXES = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}
PROVINCES = [
    "皖", "沪", "津", "渝", "冀", "晋", "蒙", "辽", "吉", "黑", "苏", "浙",
    "京", "闽", "赣", "鲁", "豫", "鄂", "湘", "粤", "桂", "琼", "川", "贵",
    "云", "藏", "陕", "甘", "青", "宁", "新", "警", "学", "O",
]
ALPHABETS = [
    "A", "B", "C", "D", "E", "F", "G", "H", "J", "K", "L", "M",
    "N", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "O",
]
ADS = [
    "A", "B", "C", "D", "E", "F", "G", "H", "J", "K", "L", "M",
    "N", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "O",
]


@dataclass(frozen=True)
class CcpdAnnotation:
    area_ratio: str
    tilt_horizontal: int
    tilt_vertical: int
    bbox: tuple[int, int, int, int]
    corners: tuple[tuple[int, int], tuple[int, int], tuple[int, int], tuple[int, int]]
    plate_indices: tuple[int, ...]
    plate_text: str


def parse_point(encoded: str) -> tuple[int, int]:
    x_text, y_text = encoded.split("&")
    return int(x_text), int(y_text)


def decode_plate_text(indices: tuple[int, ...]) -> str:
    if len(indices) < 3:
        raise ValueError("plate indices are incomplete")

    province_index, alphabet_index, *ad_indices = indices

    try:
        province = PROVINCES[province_index]
        alphabet = ALPHABETS[alphabet_index]
        suffix = "".join(ADS[index] for index in ad_indices)
    except IndexError as exc:
        raise ValueError(f"plate indices out of range: {indices}") from exc

    return f"{province}{alphabet}{suffix}"


def parse_ccpd_filename(image_path: Path) -> CcpdAnnotation:
    parts = image_path.stem.split("-")
    if len(parts) < 5:
        raise ValueError(f"invalid CCPD filename: {image_path.name}")

    area_ratio = parts[0]
    tilt_horizontal, tilt_vertical = (int(value) for value in parts[1].split("_"))

    left_top = parse_point(parts[2].split("_")[0])
    right_bottom = parse_point(parts[2].split("_")[1])
    x1, y1 = left_top
    x2, y2 = right_bottom

    corners = tuple(parse_point(value) for value in parts[3].split("_"))
    if len(corners) != 4:
        raise ValueError(f"invalid corner count in filename: {image_path.name}")

    plate_indices = tuple(int(value) for value in parts[4].split("_"))
    plate_text = decode_plate_text(plate_indices)

    return CcpdAnnotation(
        area_ratio=area_ratio,
        tilt_horizontal=tilt_horizontal,
        tilt_vertical=tilt_vertical,
        bbox=(min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2)),
        corners=corners,
        plate_indices=plate_indices,
        plate_text=plate_text,
    )


def clamp_bbox(
    bbox: tuple[int, int, int, int], width: int, height: int
) -> tuple[int, int, int, int]:
    x1, y1, x2, y2 = bbox
    x1 = min(max(x1, 0), width - 1)
    y1 = min(max(y1, 0), height - 1)
    x2 = min(max(x2, 0), width - 1)
    y2 = min(max(y2, 0), height - 1)

    if x2 <= x1 or y2 <= y1:
        raise ValueError(f"invalid bbox after clamp: {(x1, y1, x2, y2)}")

    return x1, y1, x2, y2


def bbox_to_yolo(
    bbox: tuple[int, int, int, int], width: int, height: int
) -> tuple[float, float, float, float]:
    x1, y1, x2, y2 = clamp_bbox(bbox, width, height)
    center_x = ((x1 + x2) / 2.0) / width
    center_y = ((y1 + y2) / 2.0) / height
    box_width = (x2 - x1) / width
    box_height = (y2 - y1) / height
    return center_x, center_y, box_width, box_height


def list_images(root: Path) -> list[Path]:
    return sorted(
        path
        for path in root.rglob("*")
        if path.is_file() and path.suffix.lower() in IMAGE_SUFFIXES
    )


def load_image_size_from_jpeg(image_path: Path) -> tuple[int, int]:
    with image_path.open("rb") as file:
        if file.read(2) != b"\xff\xd8":
            raise ValueError(f"invalid JPEG header: {image_path}")

        while True:
            marker_prefix = file.read(1)
            while marker_prefix == b"\xff":
                marker_type = file.read(1)
                if marker_type and marker_type != b"\xff":
                    break
                marker_prefix = file.read(1)

            if not marker_type:
                raise ValueError(f"failed to locate JPEG frame header: {image_path}")

            if marker_type in {b"\xd8", b"\xd9"}:
                continue

            segment_length_bytes = file.read(2)
            if len(segment_length_bytes) != 2:
                raise ValueError(f"truncated JPEG segment: {image_path}")

            segment_length = struct.unpack(">H", segment_length_bytes)[0]
            if segment_length < 2:
                raise ValueError(f"invalid JPEG segment length: {image_path}")

            if marker_type in {
                b"\xc0",
                b"\xc1",
                b"\xc2",
                b"\xc3",
                b"\xc5",
                b"\xc6",
                b"\xc7",
                b"\xc9",
                b"\xca",
                b"\xcb",
                b"\xcd",
                b"\xce",
                b"\xcf",
            }:
                segment = file.read(segment_length - 2)
                if len(segment) < 5:
                    raise ValueError(f"truncated JPEG frame segment: {image_path}")
                height, width = struct.unpack(">HH", segment[1:5])
                return width, height

            file.seek(segment_length - 2, 1)


def load_image_size_from_png(image_path: Path) -> tuple[int, int]:
    with image_path.open("rb") as file:
        header = file.read(24)
    if len(header) < 24 or header[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"invalid PNG header: {image_path}")
    width, height = struct.unpack(">II", header[16:24])
    return width, height


def load_image_size(image_path: Path) -> tuple[int, int]:
    try:
        import cv2
    except ImportError as exc:
        cv2 = None

    if cv2 is not None:
        image = cv2.imread(str(image_path))
        if image is not None:
            height, width = image.shape[:2]
            return width, height

    suffix = image_path.suffix.lower()
    if suffix in {".jpg", ".jpeg"}:
        return load_image_size_from_jpeg(image_path)
    if suffix == ".png":
        return load_image_size_from_png(image_path)

    raise RuntimeError(
        f"cannot read image size for {image_path.suffix}; install opencv-python or use JPEG/PNG images"
    )


def detect_existing_splits(source_dir: Path) -> dict[str, list[Path]]:
    if source_dir.name in {"train", "val", "test"}:
        return {source_dir.name: list_images(source_dir)}

    split_dirs = {}
    for split in ("train", "val", "test"):
        split_dir = source_dir / split
        if split_dir.is_dir():
            split_dirs[split] = list_images(split_dir)

    if split_dirs:
        return split_dirs

    return {"train": list_images(source_dir)}


def build_split_map(
    split_files: dict[str, list[Path]], val_ratio: float, seed: int
) -> dict[str, list[Path]]:
    normalized = {split: sorted(files) for split, files in split_files.items()}

    if "val" in normalized or val_ratio <= 0 or not normalized.get("train"):
        return normalized

    train_files = normalized["train"][:]
    rng = random.Random(seed)
    rng.shuffle(train_files)

    val_count = max(1, int(len(train_files) * val_ratio))
    val_set = set(train_files[:val_count])

    normalized["train"] = [path for path in normalized["train"] if path not in val_set]
    normalized["val"] = sorted(val_set)
    return normalized


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def transfer_image(source: Path, target: Path, image_mode: str) -> None:
    if target.exists():
        target.unlink()

    if image_mode == "copy":
        shutil.copy2(source, target)
        return

    if image_mode == "symlink":
        target.symlink_to(source.resolve())
        return

    raise ValueError(f"unsupported image mode: {image_mode}")


def write_label_file(
    label_path: Path, class_id: int, yolo_bbox: tuple[float, float, float, float]
) -> None:
    cx, cy, width, height = yolo_bbox
    label_path.write_text(
        f"{class_id} {cx:.6f} {cy:.6f} {width:.6f} {height:.6f}\n",
        encoding="utf-8",
    )


def write_dataset_yaml(output_dir: Path, splits: dict[str, list[Path]], class_name: str) -> None:
    lines = [f"path: {output_dir.resolve()}"]

    for split in ("train", "val", "test"):
        if splits.get(split):
            lines.append(f"{split}: {split}")

    lines.append("names:")
    lines.append(f"  0: {class_name}")
    (output_dir / "dataset.yaml").write_text("\n".join(lines) + "\n", encoding="utf-8")


def convert_dataset(
    source_dir: Path,
    output_dir: Path,
    class_id: int,
    class_name: str,
    image_mode: str,
    val_ratio: float,
    seed: int,
) -> tuple[int, int]:
    split_files = build_split_map(detect_existing_splits(source_dir), val_ratio=val_ratio, seed=seed)

    ensure_dir(output_dir)
    manifest_path = output_dir / "manifest.csv"
    converted_count = 0
    skipped_count = 0

    with manifest_path.open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow(
            [
                "split",
                "image_name",
                "source_image",
                "target_image",
                "label_file",
                "plate_text",
                "bbox_xyxy",
                "corners",
                "area_ratio",
                "tilt_horizontal",
                "tilt_vertical",
            ]
        )

        for split, image_paths in split_files.items():
            image_dir = output_dir / split
            label_dir = output_dir / f"{split}_labels"
            ensure_dir(image_dir)
            ensure_dir(label_dir)

            for image_path in image_paths:
                try:
                    annotation = parse_ccpd_filename(image_path)
                    width, height = load_image_size(image_path)
                    yolo_bbox = bbox_to_yolo(annotation.bbox, width=width, height=height)
                except ValueError as exc:
                    skipped_count += 1
                    print(f"[skip] {image_path.name}: {exc}")
                    continue

                target_image_path = image_dir / image_path.name
                label_path = label_dir / f"{image_path.stem}.txt"

                transfer_image(image_path, target_image_path, image_mode=image_mode)
                write_label_file(label_path, class_id=class_id, yolo_bbox=yolo_bbox)

                writer.writerow(
                    [
                        split,
                        image_path.name,
                        str(image_path.resolve()),
                        str(target_image_path.absolute()),
                        str(label_path.absolute()),
                        annotation.plate_text,
                        ",".join(str(value) for value in annotation.bbox),
                        ";".join(f"{x},{y}" for x, y in annotation.corners),
                        annotation.area_ratio,
                        annotation.tilt_horizontal,
                        annotation.tilt_vertical,
                    ]
                )
                converted_count += 1

    write_dataset_yaml(output_dir, split_files, class_name=class_name)
    return converted_count, skipped_count


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Convert CCPD dataset to YOLO detection labels.")
    parser.add_argument(
        "--source",
        type=Path,
        default=Path("data/raw/CCPD2020/ccpd_green"),
        help="CCPD dataset root. Supports train/val/test subdirectories or a flat directory.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("data/processed/ccpd_green_yolo"),
        help="Output directory. Generates flat folders like train/, train_labels/, test/, test_labels/.",
    )
    parser.add_argument("--class-id", type=int, default=0, help="YOLO class id for license plate.")
    parser.add_argument("--class-name", default="license_plate", help="YOLO class name.")
    parser.add_argument(
        "--image-mode",
        choices=("copy", "symlink"),
        default="copy",
        help="How to materialize output images.",
    )
    parser.add_argument(
        "--val-ratio",
        type=float,
        default=0.1,
        help="When source has no val split, carve val from train with this ratio.",
    )
    parser.add_argument("--seed", type=int, default=42, help="Random seed for train/val split.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    converted_count, skipped_count = convert_dataset(
        source_dir=args.source,
        output_dir=args.output,
        class_id=args.class_id,
        class_name=args.class_name,
        image_mode=args.image_mode,
        val_ratio=args.val_ratio,
        seed=args.seed,
    )
    print(
        f"Converted {converted_count} images to YOLO format at {args.output}. "
        f"Skipped {skipped_count} files."
    )


if __name__ == "__main__":
    main()
