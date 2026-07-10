from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


@dataclass
class RecognitionResult:
    image_id: str | None = None
    text: str = ""
    confidence: float = 0.0
    raw: Any | None = field(default=None, repr=False)

    @property
    def is_empty(self) -> bool:
        return not self.text

    def to_dict(self) -> dict[str, Any]:
        return {
            "image_id": self.image_id,
            "text": self.text,
            "confidence": self.confidence,
        }
