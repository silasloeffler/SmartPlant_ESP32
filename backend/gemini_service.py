from google import genai
from google.genai import types
from pydantic import ValidationError

from .prompts import SYSTEM_PROMPT, build_user_prompt
from .schemas import GeminiPlantAnalysis


class GeminiServiceError(Exception):
    """A safe, non-secret-bearing Gemini service failure."""


class GeminiEmptyResponseError(GeminiServiceError):
    """Gemini returned no usable structured content."""


async def analyze_with_gemini(
    *,
    image: bytes,
    sensor_status: dict[str, object],
    api_key: str,
    model: str,
) -> GeminiPlantAnalysis:
    try:
        async with genai.Client(api_key=api_key).aio as client:
            response = await client.models.generate_content(
                model=model,
                contents=[
                    types.Part.from_bytes(data=image, mime_type="image/jpeg"),
                    types.Part.from_text(text=build_user_prompt(sensor_status)),
                ],
                config=types.GenerateContentConfig(
                    system_instruction=SYSTEM_PROMPT,
                    response_mime_type="application/json",
                    response_schema=GeminiPlantAnalysis,
                    temperature=0.4,
                ),
            )
    except Exception as exc:
        raise GeminiServiceError("Gemini request failed") from exc

    parsed = response.parsed
    if isinstance(parsed, GeminiPlantAnalysis):
        return parsed

    try:
        if isinstance(parsed, dict):
            return GeminiPlantAnalysis.model_validate(parsed)
        if response.text and response.text.strip():
            return GeminiPlantAnalysis.model_validate_json(response.text)
    except (ValidationError, ValueError) as exc:
        raise GeminiEmptyResponseError("Gemini response was not usable") from exc

    raise GeminiEmptyResponseError("Gemini response was empty")
