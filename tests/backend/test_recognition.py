from __future__ import annotations

import asyncio
from io import BytesIO
from pathlib import Path
import sys

from fastapi import HTTPException
from fastapi.testclient import TestClient
from fastapi import UploadFile

PROJECT_ROOT = Path(__file__).resolve().parents[2]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from app.api import routes
from app.main import app
from app.services.recognition_service import RecognitionService


class StubDetectionBox:
    def __init__(self, x1: int, y1: int, x2: int, y2: int, confidence: float) -> None:
        self.x1 = x1
        self.y1 = y1
        self.x2 = x2
        self.y2 = y2
        self.confidence = confidence


class StubDetectionResult:
    def __init__(self, image_id: str | None, boxes: list[StubDetectionBox]) -> None:
        self.image_id = image_id
        self.boxes = boxes


class StubRecognitionResult:
    def __init__(self, image_id: str | None, text: str, confidence: float) -> None:
        self.image_id = image_id
        self.text = text
        self.confidence = confidence


class StubPipeline:
    def __init__(self) -> None:
        self.last_image_id: str | None = None
        self.last_image_path: Path | None = None

    def run(
        self, image_path: str | Path, image_id: str | None = None
    ) -> tuple[StubDetectionResult, StubRecognitionResult]:
        self.last_image_id = image_id
        self.last_image_path = Path(image_path)
        return (
            StubDetectionResult(
                image_id=image_id,
                boxes=[
                    StubDetectionBox(
                        x1=10,
                        y1=20,
                        x2=110,
                        y2=60,
                        confidence=0.93,
                    )
                ],
            ),
            StubRecognitionResult(
                image_id=image_id,
                text="沪A12345",
                confidence=0.98,
            ),
        )


def test_recognition_service_returns_pipeline_result() -> None:
    service = RecognitionService(pipeline=StubPipeline())
    upload = UploadFile(filename="plate-input.png", file=BytesIO(b"fake-image-bytes"))

    response = asyncio.run(service.recognize(upload))

    assert response.image_id == "plate-input"
    assert response.plate_text == "沪A12345"
    assert response.confidence == 0.98
    assert len(response.boxes) == 1
    assert response.boxes[0].x1 == 10


def test_recognition_service_rejects_empty_upload() -> None:
    service = RecognitionService(pipeline=StubPipeline())
    upload = UploadFile(filename="empty.png", file=BytesIO(b""))

    try:
        asyncio.run(service.recognize(upload))
    except HTTPException as exc:
        assert exc.status_code == 400
        assert exc.detail == "上传图片为空"
    else:
        raise AssertionError("Expected HTTPException for empty upload")


def test_recognition_route_uses_pipeline_result() -> None:
    stub_pipeline = StubPipeline()
    original_pipeline = routes.recognition_service._pipeline
    routes.recognition_service._pipeline = stub_pipeline

    try:
        client = TestClient(app)
        response = client.post(
            "/api/v1/recognize",
            files={"file": ("frontend-upload.png", b"fake-image-bytes", "image/png")},
        )
    finally:
        routes.recognition_service._pipeline = original_pipeline

    assert response.status_code == 200
    payload = response.json()
    assert payload["image_id"] == "frontend-upload"
    assert payload["plate_text"] == "沪A12345"
    assert payload["confidence"] == 0.98
    assert payload["boxes"][0]["confidence"] == 0.93
    assert stub_pipeline.last_image_id == "frontend-upload"
