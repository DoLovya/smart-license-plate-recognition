from pathlib import Path

from tools.convert_ccpd_to_yolo import bbox_to_yolo
from tools.convert_ccpd_to_yolo import detect_existing_splits
from tools.convert_ccpd_to_yolo import parse_ccpd_filename
from tools.convert_ccpd_to_yolo import write_dataset_yaml


def test_parse_ccpd_filename_returns_expected_fields() -> None:
    image_path = Path(
        "025-95_113-154&383_386&473-386&473_177&454_154&383_363&402-0_0_22_27_27_33_16-37-15.jpg"
    )

    annotation = parse_ccpd_filename(image_path)

    assert annotation.area_ratio == "025"
    assert annotation.tilt_horizontal == 95
    assert annotation.tilt_vertical == 113
    assert annotation.bbox == (154, 383, 386, 473)
    assert annotation.plate_text == "皖AY339S"


def test_bbox_to_yolo_returns_normalized_box() -> None:
    center_x, center_y, box_width, box_height = bbox_to_yolo(
        bbox=(154, 383, 386, 473),
        width=720,
        height=1160,
    )

    assert round(center_x, 6) == 0.375
    assert round(center_y, 6) == 0.368966
    assert round(box_width, 6) == 0.322222
    assert round(box_height, 6) == 0.077586


def test_detect_existing_splits_keeps_explicit_test_dir(tmp_path: Path) -> None:
    test_dir = tmp_path / "test"
    test_dir.mkdir()
    (test_dir / "sample.jpg").write_bytes(b"fake")

    split_map = detect_existing_splits(test_dir)

    assert list(split_map.keys()) == ["test"]
    assert split_map["test"] == [test_dir / "sample.jpg"]


def test_write_dataset_yaml_uses_ultralytics_layout(tmp_path: Path) -> None:
    write_dataset_yaml(
        output_dir=tmp_path,
        splits={"train": [tmp_path / "a.jpg"], "val": [tmp_path / "b.jpg"], "test": []},
        class_name="license_plate",
    )

    yaml_text = (tmp_path / "dataset.yaml").read_text(encoding="utf-8")

    assert "train: images/train" in yaml_text
    assert "val: images/val" in yaml_text
    assert "test: images/test" not in yaml_text
