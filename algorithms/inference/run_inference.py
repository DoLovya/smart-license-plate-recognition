import argparse

from algorithms.detector.detector import PlateDetector
from algorithms.pipelines.plate_pipeline import LicensePlatePipeline
from algorithms.recognizer.recognizer import PlateRecognizer


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run license plate inference pipeline.")
    parser.add_argument("--image", required=True, help="Input image path.")
    parser.add_argument(
        "--detector-weight",
        default="algorithms/weights/plate_detector.onnx",
        help="Detector weight file path.",
    )
    parser.add_argument(
        "--recognizer-weight",
        default="algorithms/weights/plate_recognizer.onnx",
        help="Recognizer weight file path.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    detector = PlateDetector(weight_path=args.detector_weight)
    recognizer = PlateRecognizer(weight_path=args.recognizer_weight)
    pipeline = LicensePlatePipeline(detector=detector, recognizer=recognizer)
    print(pipeline.run(args.image))


if __name__ == "__main__":
    main()
