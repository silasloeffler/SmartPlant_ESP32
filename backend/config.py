import os

from dotenv import load_dotenv


load_dotenv()

ADC_MIN = 0
ADC_MAX = 4095


def _read_adc_threshold(name: str, default: int) -> int:
    value = int(os.getenv(name, str(default)))
    if not ADC_MIN <= value <= ADC_MAX:
        raise ValueError(f"{name} must be between {ADC_MIN} and {ADC_MAX}")
    return value


SOIL_DRY_THRESHOLD = _read_adc_threshold("SOIL_DRY_THRESHOLD", 3000)
LIGHT_LOW_THRESHOLD = _read_adc_threshold("LIGHT_LOW_THRESHOLD", 1000)
