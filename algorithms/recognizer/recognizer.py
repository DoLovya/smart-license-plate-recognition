from pathlib import Path


class PlateRecognizer:
    def __init__(self, weight_path: str | Path) -> None:
        self.weight_path = Path(weight_path)

    def predict(self, image_path: str | Path) -> tuple[str, float]:
        _ = Path(image_path)
        return "PENDING", 0.0
