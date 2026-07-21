from typing import Annotated

from fastapi import Body, FastAPI, Header, HTTPException, Query, status

from .calibration import build_sensor_status
from .config import ADC_MAX, ADC_MIN, GEMINI_API_KEY, GEMINI_MODEL
from .gemini_service import (
    GeminiEmptyResponseError,
    GeminiServiceError,
    analyze_with_gemini,
)


app = FastAPI(title="SmartPlant Backend")


@app.get("/health")
async def health() -> dict[str, str]:
    return {"status": "ok"}


@app.post("/analyze")
async def analyze(
    image: Annotated[bytes, Body(media_type="image/jpeg")],
    soil_raw: Annotated[int, Query(ge=ADC_MIN, le=ADC_MAX)],
    light_raw: Annotated[int, Query(ge=ADC_MIN, le=ADC_MAX)],
    temp_raw: Annotated[int, Query(ge=ADC_MIN, le=ADC_MAX)],
    content_type: Annotated[str | None, Header()] = None,
) -> dict[str, object]:
    media_type = (content_type or "").split(";", maxsplit=1)[0].strip().lower()
    if media_type != "image/jpeg":
        raise HTTPException(
            status_code=status.HTTP_415_UNSUPPORTED_MEDIA_TYPE,
            detail="Content-Type must be image/jpeg",
        )

    if not image.startswith(b"\xff\xd8") or not image.endswith(b"\xff\xd9"):
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Request body is not a JPEG byte stream",
        )

    if not GEMINI_API_KEY:
        raise HTTPException(
            status_code=status.HTTP_503_SERVICE_UNAVAILABLE,
            detail="Gemini API key is not configured",
        )

    sensor_status = build_sensor_status(soil_raw, light_raw, temp_raw)

    try:
        gemini_result = await analyze_with_gemini(
            image=image,
            sensor_status=sensor_status,
            api_key=GEMINI_API_KEY,
            model=GEMINI_MODEL,
        )
    except GeminiEmptyResponseError as exc:
        raise HTTPException(
            status_code=status.HTTP_502_BAD_GATEWAY,
            detail="Gemini returned no usable plant analysis",
        ) from exc
    except GeminiServiceError as exc:
        raise HTTPException(
            status_code=status.HTTP_502_BAD_GATEWAY,
            detail="Gemini plant analysis failed",
        ) from exc

    return {
        "status": "ok",
        "external_ai_used": True,
        "image": {
            "content_type": "image/jpeg",
            "size_bytes": len(image),
        },
        "sensor_status": sensor_status,
        "image_observation": gemini_result.image_observation,
        "plant_message": gemini_result.plant_message,
        "model": GEMINI_MODEL,
    }
