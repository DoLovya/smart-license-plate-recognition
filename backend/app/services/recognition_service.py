from __future__ import annotations

from pathlib import Path
from tempfile import TemporaryDirectory
from time import perf_counter
from typing import TYPE_CHECKING

from fastapi import HTTPException, UploadFile

from app.schemas.recognition import DetectionBox, RecognitionResponse

if TYPE_CHECKING:
    from app.services.license_plate_pipeline import LicensePlatePipeline


class RecognitionService:
    def __init__(self, pipeline: "LicensePlatePipeline | None" = None):
        self._pipeline = pipeline

    @property
    def pipeline(self) -> "LicensePlatePipeline":
        if self._pipeline is None:
            from app.services.license_plate_pipeline import LicensePlatePipeline

            self._pipeline = LicensePlatePipeline()
        return self._pipeline

    async def recognize(self, file: UploadFile) -> RecognitionResponse:
        started_at = perf_counter()
        uploaded_bytes = await file.read()
        if not uploaded_bytes:
            raise HTTPException(status_code=400, detail="上传图片为空")

        original_name = file.filename or "uploaded-image"
        image_id = Path(original_name).stem or "uploaded-image"
        suffix = Path(original_name).suffix or ".png"

        try:
            with TemporaryDirectory(prefix="slpr-upload-") as temp_dir:
                image_path = Path(temp_dir) / f"{image_id}{suffix}"
                image_path.write_bytes(uploaded_bytes)

                detection_result, recognition_result = self.pipeline.run(image_path, image_id=image_id)
        except ValueError as exc:
            raise HTTPException(status_code=400, detail=str(exc)) from exc
        except Exception as exc:
            raise HTTPException(status_code=500, detail=f"算法管线执行失败: {exc}") from exc

        elapsed_ms = round((perf_counter() - started_at) * 1000, 2)
        response_boxes = [
            DetectionBox(
                x1=int(box.x1),
                y1=int(box.y1),
                x2=int(box.x2),
                y2=int(box.y2),
                confidence=box.confidence,
            )
            for box in detection_result.boxes
        ]

        return RecognitionResponse(
            image_id=image_id,
            plate_text=recognition_result.text,
            confidence=recognition_result.confidence,
            boxes=response_boxes,
            elapsed_ms=elapsed_ms,
        )
