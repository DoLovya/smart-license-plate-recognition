from dataclasses import dataclass
from pathlib import Path


@dataclass
class DetectionResult:
    bbox: tuple[int, int, int, int]
    confidence: float


class PlateDetector:
    def __init__(self, weight_path: str | Path, confidence_threshold: float = 0.3) -> None:
        self.weight_path = Path(weight_path)
        self.confidence_threshold = confidence_threshold

    def predict(self, image_path: str | Path) -> list[DetectionResult]:
        _ = Path(image_path)
        return [DetectionResult(bbox=(0, 0, 0, 0), confidence=0.0)]
