from unittest.mock import AsyncMock
from pathlib import Path

import pytest
from fastapi.testclient import TestClient

import backend.app as app_module
from backend.calibration import (
    build_sensor_status,
    light_percent,
    soil_moisture_percent,
)
from backend.gemini_service import GeminiEmptyResponseError, GeminiServiceError
from backend.schemas import GeminiPlantAnalysis


client = TestClient(app_module.app)
JPEG_BYTES = b"\xff\xd8\xff\xd9"
GEMINI_RESULT = GeminiPlantAnalysis(
    image_observation="Die sichtbaren Blätter wirken überwiegend aufrecht.",
    plant_message="Meine Blätter halten sich tapfer. Ich genieße gerade das Licht!",
)


def post_analyze(**params: int):
    return client.post(
        "/analyze",
        params=params,
        content=JPEG_BYTES,
        headers={"Content-Type": "image/jpeg"},
    )


@pytest.fixture
def mocked_gemini(monkeypatch: pytest.MonkeyPatch) -> AsyncMock:
    mock = AsyncMock(return_value=GEMINI_RESULT)
    monkeypatch.setattr(app_module, "analyze_with_gemini", mock)
    monkeypatch.setattr(app_module, "GEMINI_ENABLED", True)
    monkeypatch.setattr(app_module, "GEMINI_API_KEY", "test-key")
    monkeypatch.setattr(app_module, "GEMINI_MODEL", "test-model")
    return mock


@pytest.fixture(autouse=True)
def isolated_debug_dir(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> Path:
    monkeypatch.setattr(app_module, "DEBUG_DIR", tmp_path)
    monkeypatch.setattr(app_module, "SAVE_DEBUG_IMAGE", True)
    return tmp_path


def test_health() -> None:
    response = client.get("/health")

    assert response.status_code == 200
    assert response.json() == {"status": "ok"}


def test_soil_percentage_calibration() -> None:
    assert soil_moisture_percent(471) == 100
    assert soil_moisture_percent(2447) == 0
    assert soil_moisture_percent(1459) == 50


def test_light_percentage_calibration() -> None:
    assert light_percent(580) == 0
    assert light_percent(4095) == 100
    assert light_percent(2540) == 56


def test_percentages_are_clamped_to_zero_and_one_hundred() -> None:
    assert soil_moisture_percent(0) == 100
    assert soil_moisture_percent(4095) == 0
    assert light_percent(0) == 0
    assert light_percent(4095) == 100


def test_soil_sensor_direction_is_reversed() -> None:
    assert soil_moisture_percent(600) > soil_moisture_percent(2200)


def test_dry_and_sufficiently_moist_soil_status() -> None:
    assert build_sensor_status(2447, 2000, 2050)["soil"]["status"] == "dry"
    assert (
        build_sensor_status(471, 2000, 2050)["soil"]["status"]
        == "sufficiently_moist"
    )


def test_dark_and_bright_light_status() -> None:
    assert build_sensor_status(1000, 580, 2050)["light"]["status"] == "dark"
    assert build_sensor_status(1000, 4095, 2050)["light"]["status"] == "bright"


def test_valid_jpeg_without_gemini_is_saved_and_returns_200(
    monkeypatch: pytest.MonkeyPatch,
    isolated_debug_dir: Path,
) -> None:
    gemini_mock = AsyncMock()
    monkeypatch.setattr(app_module, "analyze_with_gemini", gemini_mock)
    monkeypatch.setattr(app_module, "GEMINI_ENABLED", False)
    monkeypatch.setattr(app_module, "GEMINI_API_KEY", "")

    response = post_analyze(soil_raw=1660, light_raw=2540, temp_raw=2050)

    assert response.status_code == 200
    assert response.json() == {
        "status": "received",
        "gemini_used": False,
        "image_bytes": len(JPEG_BYTES),
        "sensor_status": {
            "soil": {"raw": 1660, "percent": 40, "status": "sufficiently_moist"},
            "light": {"raw": 2540, "percent": 56, "status": "bright"},
            "temperature": {"raw": 2050, "interpretation": "adc_raw_only"},
        },
        "message": "Bild und Sensorwerte erfolgreich empfangen",
    }
    assert (isolated_debug_dir / "last_received.jpg").read_bytes() == JPEG_BYTES
    gemini_mock.assert_not_awaited()


def test_analyze_returns_structured_gemini_response(
    mocked_gemini: AsyncMock,
) -> None:
    response = post_analyze(soil_raw=1459, light_raw=2540, temp_raw=2050)

    assert response.status_code == 200
    payload = response.json()
    assert payload["status"] == "ok"
    assert payload["external_ai_used"] is True
    assert payload["sensor_status"] == {
        "soil": {"raw": 1459, "percent": 50, "status": "sufficiently_moist"},
        "light": {"raw": 2540, "percent": 56, "status": "bright"},
        "temperature": {"raw": 2050, "interpretation": "adc_raw_only"},
    }
    assert payload["image_observation"] == GEMINI_RESULT.image_observation
    assert payload["plant_message"] == GEMINI_RESULT.plant_message
    assert payload["model"] == "test-model"
    mocked_gemini.assert_awaited_once()


@pytest.mark.parametrize("value", [0, 4095])
def test_adc_boundaries_are_accepted(value: int, mocked_gemini: AsyncMock) -> None:
    response = post_analyze(soil_raw=value, light_raw=value, temp_raw=value)

    assert response.status_code == 200


@pytest.mark.parametrize("parameter", ["soil_raw", "light_raw", "temp_raw"])
@pytest.mark.parametrize("value", [-1, 4096])
def test_adc_values_outside_range_are_rejected(
    parameter: str, value: int, mocked_gemini: AsyncMock
) -> None:
    params = {"soil_raw": 2000, "light_raw": 2000, "temp_raw": 2000}
    params[parameter] = value

    response = post_analyze(**params)

    assert response.status_code == 422
    mocked_gemini.assert_not_awaited()


def test_non_jpeg_content_type_is_rejected(mocked_gemini: AsyncMock) -> None:
    response = client.post(
        "/analyze?soil_raw=2000&light_raw=2000&temp_raw=2000",
        content=JPEG_BYTES,
        headers={"Content-Type": "application/octet-stream"},
    )

    assert response.status_code == 415
    mocked_gemini.assert_not_awaited()


def test_invalid_jpeg_bytes_are_rejected(
    mocked_gemini: AsyncMock,
    isolated_debug_dir: Path,
) -> None:
    response = client.post(
        "/analyze?soil_raw=2000&light_raw=2000&temp_raw=2000",
        content=b"not-a-jpeg",
        headers={"Content-Type": "image/jpeg"},
    )

    assert response.status_code == 400
    mocked_gemini.assert_not_awaited()
    assert (
        isolated_debug_dir / "last_received_invalid.bin"
    ).read_bytes() == b"not-a-jpeg"


def test_missing_api_key_is_reported_safely(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(app_module, "GEMINI_ENABLED", True)
    monkeypatch.setattr(app_module, "GEMINI_API_KEY", "")

    response = post_analyze(soil_raw=1000, light_raw=2000, temp_raw=2050)

    assert response.status_code == 503
    assert response.json() == {"detail": "Gemini API key is not configured"}


@pytest.mark.parametrize(
    ("error", "expected_detail"),
    [
        (
            GeminiEmptyResponseError("empty"),
            "Gemini returned no usable plant analysis",
        ),
        (GeminiServiceError("secret upstream detail"), "Gemini plant analysis failed"),
    ],
)
def test_gemini_errors_are_reported_without_internal_details(
    error: Exception,
    expected_detail: str,
    mocked_gemini: AsyncMock,
) -> None:
    mocked_gemini.side_effect = error

    response = post_analyze(soil_raw=1000, light_raw=2000, temp_raw=2050)

    assert response.status_code == 502
    assert response.json() == {"detail": expected_detail}
    assert "secret upstream detail" not in response.text
