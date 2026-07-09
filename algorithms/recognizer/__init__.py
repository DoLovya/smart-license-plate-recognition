from .recognition_result import RecognitionResult

__all__ = ["LicensePlateRecognizer", "RecognitionResult"]


def __getattr__(name: str):
    if name == "LicensePlateRecognizer":
        from .license_plate_recognizer import LicensePlateRecognizer

        return LicensePlateRecognizer
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
