import os
from pathlib import Path

from dotenv import load_dotenv


load_dotenv(Path(__file__).with_name(".env"))

ADC_MIN = 0
ADC_MAX = 4095


def _read_int(name: str, default: int, minimum: int, maximum: int) -> int:
    try:
        value = int(os.getenv(name, str(default)))
    except ValueError as exc:
        raise ValueError(f"{name} must be an integer") from exc
    if not minimum <= value <= maximum:
        raise ValueError(f"{name} must be between {minimum} and {maximum}")
    return value


def _read_bool(name: str, default: bool) -> bool:
    value = os.getenv(name, str(default)).strip().lower()
    if value in {"1", "true", "yes", "on"}:
        return True
    if value in {"0", "false", "no", "off"}:
        return False
    raise ValueError(f"{name} must be true or false")


APP_HOST = os.getenv("APP_HOST", "0.0.0.0")
APP_PORT = _read_int("APP_PORT", 8000, 1, 65535)

GEMINI_API_KEY = os.getenv("GEMINI_API_KEY", "").strip()
GEMINI_MODEL = os.getenv("GEMINI_MODEL", "gemini-2.5-flash").strip()
GEMINI_ENABLED = _read_bool("GEMINI_ENABLED", False)
SAVE_DEBUG_IMAGE = _read_bool("SAVE_DEBUG_IMAGE", True)
if not GEMINI_MODEL:
    raise ValueError("GEMINI_MODEL must not be empty")

SOIL_WET_RAW = _read_int("SOIL_WET_RAW", 471, ADC_MIN, ADC_MAX)
SOIL_DRY_RAW = _read_int("SOIL_DRY_RAW", 2447, ADC_MIN, ADC_MAX)
SOIL_DRY_PERCENT_THRESHOLD = _read_int(
    "SOIL_DRY_PERCENT_THRESHOLD", 20, 0, 100
)

LIGHT_DARK_RAW = _read_int("LIGHT_DARK_RAW", 580, ADC_MIN, ADC_MAX)
LIGHT_BRIGHT_RAW = _read_int("LIGHT_BRIGHT_RAW", 4095, ADC_MIN, ADC_MAX)
LIGHT_LOW_PERCENT_THRESHOLD = _read_int(
    "LIGHT_LOW_PERCENT_THRESHOLD", 15, 0, 100
)

if SOIL_WET_RAW >= SOIL_DRY_RAW:
    raise ValueError("SOIL_WET_RAW must be lower than SOIL_DRY_RAW")
if LIGHT_DARK_RAW >= LIGHT_BRIGHT_RAW:
    raise ValueError("LIGHT_DARK_RAW must be lower than LIGHT_BRIGHT_RAW")
