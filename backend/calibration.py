from .config import (
    LIGHT_BRIGHT_RAW,
    LIGHT_DARK_RAW,
    LIGHT_LOW_PERCENT_THRESHOLD,
    SOIL_DRY_PERCENT_THRESHOLD,
    SOIL_DRY_RAW,
    SOIL_WET_RAW,
)


def _clamp_percent(value: float) -> int:
    return round(max(0.0, min(100.0, value)))


def soil_moisture_percent(raw: int) -> int:
    """Map low/wet ADC readings to 100% and high/dry readings to 0%."""
    value = (SOIL_DRY_RAW - raw) * 100 / (SOIL_DRY_RAW - SOIL_WET_RAW)
    return _clamp_percent(value)


def light_percent(raw: int) -> int:
    """Map low/dark ADC readings to 0% and high/bright readings to 100%."""
    value = (raw - LIGHT_DARK_RAW) * 100 / (LIGHT_BRIGHT_RAW - LIGHT_DARK_RAW)
    return _clamp_percent(value)


def build_sensor_status(
    soil_raw: int, light_raw: int, temp_raw: int
) -> dict[str, dict[str, int | str]]:
    soil_percent = soil_moisture_percent(soil_raw)
    light_percent_value = light_percent(light_raw)

    return {
        "soil": {
            "raw": soil_raw,
            "percent": soil_percent,
            "status": (
                "dry"
                if soil_percent <= SOIL_DRY_PERCENT_THRESHOLD
                else "sufficiently_moist"
            ),
        },
        "light": {
            "raw": light_raw,
            "percent": light_percent_value,
            "status": (
                "dark"
                if light_percent_value <= LIGHT_LOW_PERCENT_THRESHOLD
                else "bright"
            ),
        },
        "temperature": {
            "raw": temp_raw,
            "interpretation": "adc_raw_only",
        },
    }
