from algorithms.detector.detection_result import DetectionBox, DetectionResult


def test_detection_box_exposes_size() -> None:
    box = DetectionBox(x1=10.0, y1=20.0, x2=60.0, y2=50.0, confidence=0.95)

    assert box.width == 50.0
    assert box.height == 30.0


def test_detection_result_summarizes_boxes() -> None:
    result = DetectionResult(
        source="data/raw/demo.jpg",
        boxes=[
            DetectionBox(x1=1.0, y1=2.0, x2=3.0, y2=4.0, confidence=0.6),
            DetectionBox(x1=5.0, y1=6.0, x2=7.0, y2=8.0, confidence=0.9),
        ],
    )

    payload = result.to_dict()

    assert result.count == 2
    assert result.max_confidence == 0.9
    assert result.confidences == [0.6, 0.9]
    assert payload["source"] == "data/raw/demo.jpg"
    assert payload["count"] == 2
    assert payload["max_confidence"] == 0.9
    assert payload["confidences"] == [0.6, 0.9]
    assert len(payload["boxes"]) == 2
    assert payload["boxes"][0]["confidence"] == 0.6
    assert payload["boxes"][1]["confidence"] == 0.9
