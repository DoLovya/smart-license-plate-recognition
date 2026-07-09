from .detection_result import DetectionBox, DetectionResult

__all__ = ["LicensePlateDetector", "DetectionBox", "DetectionResult"]


def __getattr__(name: str):
    if name == "LicensePlateDetector":
        from .LicensePlateDetector import LicensePlateDetector

        return LicensePlateDetector
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
