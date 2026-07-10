from __future__ import annotations

from pathlib import Path
import sys
from tempfile import TemporaryDirectory
from typing import Any

import cv2
import yaml

try:
    from algorithms.detector.detection_result import DetectionBox, DetectionResult
    from algorithms.detector.license_plate_detector import LicensePlateDetector
    from algorithms.recognizer.license_plate_recognizer import LicensePlateRecognizer
    from algorithms.recognizer.recognition_result import RecognitionResult
except ModuleNotFoundError:
    project_root = Path(__file__).resolve().parents[3]
    if str(project_root) not in sys.path:
        sys.path.insert(0, str(project_root))
    from algorithms.detector.detection_result import DetectionBox, DetectionResult
    from algorithms.detector.license_plate_detector import LicensePlateDetector
    from algorithms.recognizer.license_plate_recognizer import LicensePlateRecognizer
    from algorithms.recognizer.recognition_result import RecognitionResult


DEFAULT_DETECTOR_WEIGHT_PATH = Path(
    "algorithms/weights/plate-models/detector/yolo/ccpd-green/v1.0.0/best.pt"
)
DEFAULT_RECOGNIZER_MODEL_NAME = "PP-OCRv6_medium_rec"


class LicensePlatePipeline:
    def __init__(
        self,
        config_path: str | Path | None = None,
        detector: LicensePlateDetector | None = None,
        recognizer: LicensePlateRecognizer | None = None,
    ):
        self.project_root = Path(__file__).resolve().parents[3]
        self.config_path = (
            Path(config_path).resolve()
            if config_path is not None
            else self.project_root / "algorithms/configs/model_config.yaml"
        )
        self.config = self._load_config(self.config_path)
        self.detector = detector or self._build_detector()
        self.recognizer = recognizer or self._build_recognizer()

    def run(self, image_path: str | Path, image_id: str | None = None) -> tuple[DetectionResult, RecognitionResult]:
        resolved_image_path = Path(image_path).resolve()
        detection_result = self.detector.detect(str(resolved_image_path), image_id=image_id)[0]
        if not detection_result.boxes:
            return detection_result, RecognitionResult(image_id=image_id)

        image = cv2.imread(str(resolved_image_path))
        if image is None:
            raise ValueError(f"无法读取图片文件: {resolved_image_path}")

        best_recognition = RecognitionResult(image_id=image_id)
        boxes_by_confidence = sorted(detection_result.boxes, key=lambda item: item.confidence, reverse=True)

        with TemporaryDirectory(prefix="slpr-crops-") as temp_dir:
            temp_dir_path = Path(temp_dir)
            for index, box in enumerate(boxes_by_confidence):
                crop = self._crop_image(image, box)
                if crop is None:
                    continue

                crop_path = temp_dir_path / f"plate_crop_{index}.png"
                if not cv2.imwrite(str(crop_path), crop):
                    continue

                recognition = self.recognizer.recognize(str(crop_path), image_id=image_id)[0]
                if recognition.text and recognition.confidence >= best_recognition.confidence:
                    best_recognition = recognition

        return detection_result, best_recognition

    def _build_detector(self) -> LicensePlateDetector:
        detector_config = self.config.get("detector", {})
        configured_weight_path = detector_config.get("weight_path")
        weight_path = (
            self._resolve_project_path(configured_weight_path)
            if configured_weight_path
            else self.project_root / DEFAULT_DETECTOR_WEIGHT_PATH
        )
        if not weight_path.exists():
            fallback_path = self.project_root / DEFAULT_DETECTOR_WEIGHT_PATH
            if fallback_path.exists():
                weight_path = fallback_path
            else:
                raise FileNotFoundError(f"未找到检测模型权重: {weight_path}")

        return LicensePlateDetector(str(weight_path))

    def _build_recognizer(self) -> LicensePlateRecognizer:
        recognizer_config = self.config.get("recognizer", {})
        runtime_config = self.config.get("runtime", {})
        model_name = recognizer_config.get("model_name", DEFAULT_RECOGNIZER_MODEL_NAME)
        batch_size = int(runtime_config.get("batch_size", 1))
        return LicensePlateRecognizer(model_name=model_name, batch_size=batch_size)

    def _load_config(self, config_path: Path) -> dict[str, Any]:
        if not config_path.exists():
            return {}

        with config_path.open("r", encoding="utf-8") as file:
            loaded = yaml.safe_load(file) or {}
        if not isinstance(loaded, dict):
            return {}
        return loaded

    def _resolve_project_path(self, path_str: str | Path) -> Path:
        path = Path(path_str)
        if path.is_absolute():
            return path
        return (self.project_root / path).resolve()

    @staticmethod
    def _crop_image(image, box: DetectionBox):
        image_height, image_width = image.shape[:2]
        x1 = max(0, min(image_width - 1, int(box.x1)))
        y1 = max(0, min(image_height - 1, int(box.y1)))
        x2 = max(0, min(image_width, int(box.x2)))
        y2 = max(0, min(image_height, int(box.y2)))

        if x2 <= x1 or y2 <= y1:
            return None

        return image[y1:y2, x1:x2]
