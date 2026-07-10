from pydantic import BaseModel, Field


class DetectionBox(BaseModel):
    x1: int = Field(default=0)
    y1: int = Field(default=0)
    x2: int = Field(default=0)
    y2: int = Field(default=0)
    confidence: float = Field(default=0.0)


class HealthResponse(BaseModel):
    status: str
    service: str


class RecognitionResponse(BaseModel):
    image_id: str
    plate_text: str
    confidence: float
    boxes: list[DetectionBox]
    elapsed_ms: float
