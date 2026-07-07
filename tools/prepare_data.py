from pathlib import Path


def main() -> None:
    for relative_dir in ("data/raw", "data/interim", "data/processed", "data/exports"):
        Path(relative_dir).mkdir(parents=True, exist_ok=True)

    print("Data directories are ready.")


if __name__ == "__main__":
    main()
