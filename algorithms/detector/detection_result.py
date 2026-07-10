from __future__ import annotations

from dataclasses import asdict, dataclass, field
from typing import Any


@dataclass(slots=True)
class DetectionBox:
    x1: float
    y1: float
    x2: float
    y2: float
    confidence: float = 0.0
    class_id: int | None = None
    label: str | None = None

    @property
    def width(self) -> float:
        return max(0.0, self.x2 - self.x1)

    @property
    def height(self) -> float:
        return max(0.0, self.y2 - self.y1)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass(slots=True)
class DetectionResult:
    image_id: str | None = None
    boxes: list[DetectionBox] = field(default_factory=list)

    @property
    def count(self) -> int:
        return len(self.boxes)

    @property
    def max_confidence(self) -> float:
        if not self.boxes:
            return 0.0
        return max(box.confidence for box in self.boxes)

    @property
    def confidences(self) -> list[float]:
        return [box.confidence for box in self.boxes]

    def to_dict(self) -> dict[str, Any]:
        return {
            "image_id": self.image_id,
            "count": self.count,
            "max_confidence": self.max_confidence,
            "confidences": self.confidences,
            "boxes": [box.to_dict() for box in self.boxes],
        }
