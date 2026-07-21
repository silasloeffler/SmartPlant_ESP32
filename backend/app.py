from typing import Annotated

from fastapi import Body, FastAPI, Header, HTTPException, Query, status

from .config import (
    ADC_MAX,
    ADC_MIN,
    LIGHT_LOW_THRESHOLD,
    SOIL_DRY_THRESHOLD,
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
            detail="Request body is not a valid JPEG byte stream",
        )

    soil_status = "dry" if soil_raw >= SOIL_DRY_THRESHOLD else "moist"
    light_status = "low" if light_raw <= LIGHT_LOW_THRESHOLD else "sufficient"

    return {
        "status": "ok",
        "external_ai_used": False,
        "image": {
            "content_type": "image/jpeg",
            "size_bytes": len(image),
        },
        "sensors": {
            "soil": {"raw": soil_raw, "status": soil_status},
            "light": {"raw": light_raw, "status": light_status},
            "temperature": {
                "raw": temp_raw,
                "interpretation": "adc_raw_only",
            },
        },
    }
