import argparse


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Train or fine-tune license plate models.")
    parser.add_argument("--config", default="algorithms/configs/model_config.yaml")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    print(f"Training entry is ready. Config: {args.config}")


if __name__ == "__main__":
    main()
