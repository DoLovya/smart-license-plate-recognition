from __future__ import annotations

from ultralytics import YOLO
import cv2

from algorithms.detector.detection_result import DetectionBox, DetectionResult


class LicensePlateDetector:
    def __init__(self, model_path: str):
        self.model = YOLO(model=model_path, task="detect")

    def detect_raw(self, source):
        return self.model(source)

    def detect(self, source: str) -> list[DetectionResult]:
        raw_results = self.detect_raw(source)
        return [self._build_detection_result(result, source) for result in raw_results]

    def _build_detection_result(self, raw_result, source: str | None) -> DetectionResult:
        boxes = getattr(raw_result, "boxes", None)
        if boxes is None or len(boxes) == 0:
            return DetectionResult(source=source)

        xyxy_list = boxes.xyxy.tolist()
        conf_list = boxes.conf.tolist() if boxes.conf is not None else [0.0] * len(xyxy_list)
        cls_list = boxes.cls.tolist() if boxes.cls is not None else [None] * len(xyxy_list)
        names = getattr(raw_result, "names", {}) or {}

        detection_boxes: list[DetectionBox] = []
        for xyxy, confidence, class_id in zip(xyxy_list, conf_list, cls_list):
            label = None
            normalized_class_id = int(class_id) if class_id is not None else None
            if normalized_class_id is not None:
                label = names.get(normalized_class_id)

            detection_boxes.append(
                DetectionBox(
                    x1=float(xyxy[0]),
                    y1=float(xyxy[1]),
                    x2=float(xyxy[2]),
                    y2=float(xyxy[3]),
                    confidence=float(confidence),
                    class_id=normalized_class_id,
                    label=label,
                )
            )

        return DetectionResult(source=source, boxes=detection_boxes)


if __name__ == "__main__":
    detector = LicensePlateDetector("../training/runs/detect/runs/train-2/weights/best.pt")
    image_path = (
        "../../data/raw/CCPD2020/ccpd_green/test/"
        "0014128352490421455-90_90-212&467_271&489-271&489_212&489_212&467_271&467-0_0_3_30_30_25_31_32-79-4.jpg"
    )

    raw_results = detector.detect_raw(image_path)
    result = detector.detect(image_path)[0]

    print(result.to_dict())

    rendered = raw_results[0].plot()
    cv2.imshow("YOLOv8 Detection", rendered)
    cv2.waitKey(0)
