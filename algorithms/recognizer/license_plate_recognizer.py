from __future__ import annotations

from pathlib import Path
import sys
from typing import Any

try:
    from algorithms.recognizer.recognition_result import RecognitionResult
except ModuleNotFoundError:
    project_root = Path(__file__).resolve().parents[2]
    if str(project_root) not in sys.path:
        sys.path.insert(0, str(project_root))
    from algorithms.recognizer.recognition_result import RecognitionResult


DEFAULT_LANG = "ch"
DEFAULT_REC_MODEL_DIR = (
    Path(__file__).resolve().parents[1]
    / "weights"
    / "plate-models"
    / "recognizer"
    / "paddleocr"
    / "pp-ocrv6-small"
    / "v1.0.0"
)


class LicensePlateRecognizer:
    def __init__(
        self,
        lang: str = DEFAULT_LANG,
        rec_model_dir: str | Path | None = None,
        cls_model_dir: str | Path | None = None,
        **ocr_kwargs: Any,
    ):
        resolved_rec_model_dir = Path(rec_model_dir) if rec_model_dir is not None else DEFAULT_REC_MODEL_DIR
        if not resolved_rec_model_dir.exists():
            raise FileNotFoundError(f"未找到本地识别模型目录: {resolved_rec_model_dir}")

        resolved_cls_model_dir = Path(cls_model_dir) if cls_model_dir is not None else None
        if resolved_cls_model_dir is not None and not resolved_cls_model_dir.exists():
            raise FileNotFoundError(f"未找到本地方向分类模型目录: {resolved_cls_model_dir}")

        self.lang = lang
        self.rec_model_dir = resolved_rec_model_dir
        self.cls_model_dir = resolved_cls_model_dir
        self.use_angle_cls = resolved_cls_model_dir is not None

        init_kwargs = dict(ocr_kwargs)
        init_kwargs.update(
            {
                "use_angle_cls": self.use_angle_cls,
                "lang": lang,
                "det": False,
                "rec_model_dir": str(resolved_rec_model_dir),
            }
        )
        if resolved_cls_model_dir is not None:
            init_kwargs["cls_model_dir"] = str(resolved_cls_model_dir)

        self.ocr = PaddleOCR(**init_kwargs)

    def recognize_raw(self, image: str):
        return self.ocr.ocr(image, cls=self.use_angle_cls)

    def recognize(self, image: str, image_id: str | None = None) -> list[RecognitionResult]:
        raw_results = self.recognize_raw(image)
        if not raw_results or not raw_results[0]:
            return [RecognitionResult(image_id=image_id)]

        license_name, confidence = raw_results[0][0][1]
        normalized_text = license_name.replace("·", "")

        return [
            RecognitionResult(
                image_id=image_id,
                text=normalized_text,
                confidence=float(confidence),
                raw={"result": raw_results},
            )
        ]

if __name__ == "__main__":
    script_dir = Path(__file__).resolve().parent

    recognizer = LicensePlateRecognizer()
    result = recognizer.recognize(str(script_dir / "test.png"))[0]
    print(result.text, result.confidence)
