from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


@dataclass(slots=True)
class RecognitionResult:
    source: str | None = None
    text: str = ""
    confidence: float = 0.0
    raw: dict[str, Any] = field(default_factory=dict)

    @property
    def is_empty(self) -> bool:
        return not self.text

    def to_dict(self) -> dict[str, Any]:
        return {
            "source": self.source,
            "text": self.text,
            "confidence": self.confidence,
            "raw": self.raw,
        }
