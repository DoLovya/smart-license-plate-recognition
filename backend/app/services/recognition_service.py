from time import perf_counter

from fastapi import UploadFile

from app.schemas.recognition import DetectionBox, RecognitionResponse


class RecognitionService:
    async def recognize(self, file: UploadFile) -> RecognitionResponse:
        started_at = perf_counter()
        await file.read()

        # Placeholder for invoking the real detector + recognizer pipeline.
        elapsed_ms = round((perf_counter() - started_at) * 1000, 2)
        return RecognitionResponse(
            image_id=file.filename or "unknown",
            plate_text="PENDING",
            confidence=0.0,
            boxes=[DetectionBox()],
            elapsed_ms=elapsed_ms,
        )
