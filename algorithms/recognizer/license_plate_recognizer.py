from __future__ import annotations

from pathlib import Path
import sys
from typing import Any

from paddleocr import TextRecognition

try:
    from algorithms.recognizer.recognition_result import RecognitionResult
except ModuleNotFoundError:
    project_root = Path(__file__).resolve().parents[2]
    if str(project_root) not in sys.path:
        sys.path.insert(0, str(project_root))
    from algorithms.recognizer.recognition_result import RecognitionResult


DEFAULT_MODEL_NAME = "PP-OCRv6_medium_rec"
DEFAULT_BATCH_SIZE = 1


class LicensePlateRecognizer:
    def __init__(
        self,
        model_name: str = DEFAULT_MODEL_NAME,
        batch_size: int = DEFAULT_BATCH_SIZE,
        **ocr_kwargs: Any,
    ):
        self.model_name = model_name
        self.batch_size = batch_size
        self.ocr = TextRecognition(model_name=model_name, **ocr_kwargs)

    def recognize_raw(self, image: str):
        return self.ocr.predict(input=image, batch_size=self.batch_size)

    def recognize(self, image: str, image_id: str | None = None) -> list[RecognitionResult]:
        raw_results = self.recognize_raw(image)
        if not raw_results:
            return [RecognitionResult(image_id=image_id)]

        text, confidence, raw_payload = self._parse_result(raw_results[0])
        normalized_text = text.replace("·", "")

        return [
            RecognitionResult(
                image_id=image_id,
                text=normalized_text,
                confidence=confidence,
                raw={"result": raw_payload},
            )
        ]

    @staticmethod
    def _parse_result(result: Any) -> tuple[str, float, Any]:
        raw_payload = LicensePlateRecognizer._result_to_raw(result)

        candidates: list[Any] = []
        if isinstance(raw_payload, dict):
            candidates.extend(
                [
                    raw_payload.get("rec_text"),
                    raw_payload.get("text"),
                    raw_payload.get("label_names"),
                ]
            )
        candidates.append(getattr(result, "rec_text", None))
        candidates.append(getattr(result, "text", None))

        text = ""
        for candidate in candidates:
            if isinstance(candidate, str) and candidate:
                text = candidate
                break
            if isinstance(candidate, list) and candidate:
                first_item = candidate[0]
                if isinstance(first_item, str):
                    text = first_item
                    break

        score_candidates: list[Any] = []
        if isinstance(raw_payload, dict):
            score_candidates.extend(
                [
                    raw_payload.get("rec_score"),
                    raw_payload.get("score"),
                    raw_payload.get("scores"),
                ]
            )
        score_candidates.append(getattr(result, "rec_score", None))
        score_candidates.append(getattr(result, "score", None))

        confidence = 0.0
        for candidate in score_candidates:
            if isinstance(candidate, (int, float)):
                confidence = float(candidate)
                break
            if isinstance(candidate, list) and candidate:
                first_item = candidate[0]
                if isinstance(first_item, (int, float)):
                    confidence = float(first_item)
                    break

        return text, confidence, raw_payload

    @staticmethod
    def _result_to_raw(result: Any) -> Any:
        if hasattr(result, "json"):
            json_method = getattr(result, "json")
            if callable(json_method):
                try:
                    return json_method()
                except TypeError:
                    pass

        if hasattr(result, "to_dict"):
            to_dict_method = getattr(result, "to_dict")
            if callable(to_dict_method):
                return to_dict_method()

        if isinstance(result, dict):
            return result

        if hasattr(result, "__dict__"):
            return dict(result.__dict__)

        return result


if __name__ == "__main__":
    script_dir = Path(__file__).resolve().parent

    recognizer = LicensePlateRecognizer()
    result = recognizer.recognize(str(script_dir / "test.png"))[0]
    print(result.text, result.confidence)
