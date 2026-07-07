from pathlib import Path

from algorithms.detector.detector import PlateDetector
from algorithms.recognizer.recognizer import PlateRecognizer


class LicensePlatePipeline:
    def __init__(self, detector: PlateDetector, recognizer: PlateRecognizer) -> None:
        self.detector = detector
        self.recognizer = recognizer

    def run(self, image_path: str | Path) -> dict:
        detections = self.detector.predict(image_path)
        plate_text, confidence = self.recognizer.predict(image_path)
        return {
            "image_path": str(Path(image_path)),
            "detections": [result.__dict__ for result in detections],
            "plate_text": plate_text,
            "recognition_confidence": confidence,
        }
