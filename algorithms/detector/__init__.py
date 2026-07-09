from .detection_result import DetectionBox, DetectionResult

__all__ = ["LicensePlateDetector", "DetectionBox", "DetectionResult"]


def __getattr__(name: str):
    if name == "LicensePlateDetector":
        from .license_plate_detector import LicensePlateDetector

        return license_plate_detector
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
