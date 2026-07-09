import argparse
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parents[1]
DEFAULT_DATASET_YAML = PROJECT_ROOT / "data" / "processed" / "ccpd_green_yolo" / "dataset.yaml"
DEFAULT_PROJECT_DIR = SCRIPT_DIR / "runs"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Train a YOLOv8 license plate detector."
    )
    parser.add_argument(
        "--data",
        type=Path,
        default=DEFAULT_DATASET_YAML,
        help="Path to dataset.yaml.",
    )
    parser.add_argument(
        "--model",
        default="yolov8n.pt",
        help="YOLO model name or checkpoint path.",
    )
    parser.add_argument("--epochs", type=int, default=300, help="Training epochs.")
    parser.add_argument("--batch", type=int, default=4, help="Batch size.")
    parser.add_argument("--workers", type=int, default=4, help="Dataloader workers.")
    parser.add_argument(
        "--imgsz",
        type=int,
        default=640,
        help="Training image size.",
    )
    parser.add_argument(
        "--device",
        default="auto",
        help="Device to use. Examples: auto, cpu, 0, 0,1.",
    )
    parser.add_argument(
        "--project",
        type=Path,
        default=DEFAULT_PROJECT_DIR,
        help="Directory used by Ultralytics to store training runs.",
    )
    parser.add_argument(
        "--name",
        default="train",
        help="Run name under the project directory.",
    )
    parser.add_argument(
        "--exist-ok",
        action="store_true",
        help="Allow writing into an existing run directory.",
    )
    parser.add_argument(
        "--amp",
        dest="amp",
        action="store_true",
        help="Enable mixed precision training.",
    )
    parser.add_argument(
        "--no-amp",
        dest="amp",
        action="store_false",
        help="Disable mixed precision training.",
    )
    parser.set_defaults(amp=True)
    return parser.parse_args()


def resolve_device(device_arg: str):
    import torch

    normalized = device_arg.strip().lower()
    if normalized == "auto":
        if torch.cuda.is_available():
            return 0
        return "cpu"

    if normalized.isdigit():
        return int(normalized)

    return device_arg


def print_runtime_info(device) -> None:
    import torch

    print("=== Windows GPU 环境检测 ===")
    print("PyTorch版本:", torch.__version__)
    print("CUDA是否可用(NVIDIA显卡):", torch.cuda.is_available())
    print("CUDA版本:", torch.version.cuda if torch.cuda.is_available() else "N/A")

    if device == "cpu":
        print("❌ 当前将使用 CPU 训练")
        return

    if torch.cuda.is_available():
        if isinstance(device, int):
            print(f"✅ 当前使用 CUDA 设备: {torch.cuda.get_device_name(device)}")
        else:
            print(f"✅ 当前使用设备参数: {device}")
    else:
        print(f"⚠️ 未检测到 CUDA，继续使用设备参数: {device}")


def validate_args(args: argparse.Namespace) -> None:
    if not args.data.exists():
        raise FileNotFoundError(f"dataset.yaml 不存在: {args.data}")

    if args.epochs <= 0:
        raise ValueError("--epochs 必须大于 0")
    if args.batch <= 0:
        raise ValueError("--batch 必须大于 0")
    if args.workers < 0:
        raise ValueError("--workers 不能小于 0")
    if args.imgsz <= 0:
        raise ValueError("--imgsz 必须大于 0")


def main() -> None:
    args = parse_args()
    validate_args(args)

    from ultralytics import YOLO

    resolved_device = resolve_device(args.device)
    args.project.mkdir(parents=True, exist_ok=True)

    print_runtime_info(resolved_device)
    print("=== 训练参数 ===")
    print("数据集配置:", args.data.resolve())
    print("模型:", args.model)
    print("epochs:", args.epochs)
    print("batch:", args.batch)
    print("workers:", args.workers)
    print("imgsz:", args.imgsz)
    print("device:", resolved_device)
    print("amp:", args.amp)
    print("输出目录:", args.project.resolve())
    print("运行名称:", args.name)

    model = YOLO(args.model)
    model.train(
        data=str(args.data.resolve()),
        epochs=args.epochs,
        batch=args.batch,
        workers=args.workers,
        imgsz=args.imgsz,
        device=resolved_device,
        amp=args.amp,
        project=str(args.project.resolve()),
        name=args.name,
        exist_ok=args.exist_ok,
    )


if __name__ == "__main__":
    main()
