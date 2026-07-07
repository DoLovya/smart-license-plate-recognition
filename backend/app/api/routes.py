from fastapi import APIRouter, File, UploadFile

from app.schemas.recognition import HealthResponse, RecognitionResponse
from app.services.recognition_service import RecognitionService

router = APIRouter()
recognition_service = RecognitionService()


@router.get("/health", response_model=HealthResponse, tags=["system"])
async def health_check() -> HealthResponse:
    return HealthResponse(status="ok", service="backend")


@router.post("/recognize", response_model=RecognitionResponse, tags=["recognition"])
async def recognize_plate(file: UploadFile = File(...)) -> RecognitionResponse:
    return await recognition_service.recognize(file)
