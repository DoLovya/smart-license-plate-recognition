from algorithms.detector.detector import PlateDetector
from algorithms.pipelines.plate_pipeline import LicensePlatePipeline
from algorithms.recognizer.recognizer import PlateRecognizer


def test_pipeline_returns_expected_keys() -> None:
    pipeline = LicensePlatePipeline(
        detector=PlateDetector("algorithms/weights/plate_detector.onnx"),
        recognizer=PlateRecognizer("algorithms/weights/plate_recognizer.onnx"),
    )

    result = pipeline.run("data/raw/demo.jpg")

    assert "plate_text" in result
    assert "detections" in result
