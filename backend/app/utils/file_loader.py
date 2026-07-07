from pathlib import Path


def ensure_parent_dir(target_path: str | Path) -> Path:
    path = Path(target_path)
    path.parent.mkdir(parents=True, exist_ok=True)
    return path
