import pytest
from fastapi.testclient import TestClient

from backend.app import app


client = TestClient(app)
JPEG_BYTES = b"\xff\xd8\xff\xd9"


def post_analyze(**params: int):
    return client.post(
        "/analyze",
        params=params,
        content=JPEG_BYTES,
        headers={"Content-Type": "image/jpeg"},
    )


def test_health() -> None:
    response = client.get("/health")

    assert response.status_code == 200
    assert response.json() == {"status": "ok"}


def test_analyze_returns_structured_test_response() -> None:
    response = post_analyze(soil_raw=3200, light_raw=500, temp_raw=2048)

    assert response.status_code == 200
    assert response.json() == {
        "status": "ok",
        "external_ai_used": False,
        "image": {"content_type": "image/jpeg", "size_bytes": 4},
        "sensors": {
            "soil": {"raw": 3200, "status": "dry"},
            "light": {"raw": 500, "status": "low"},
            "temperature": {
                "raw": 2048,
                "interpretation": "adc_raw_only",
            },
        },
    }


@pytest.mark.parametrize("value", [0, 4095])
def test_adc_boundaries_are_accepted(value: int) -> None:
    response = post_analyze(soil_raw=value, light_raw=value, temp_raw=value)

    assert response.status_code == 200


@pytest.mark.parametrize("parameter", ["soil_raw", "light_raw", "temp_raw"])
@pytest.mark.parametrize("value", [-1, 4096])
def test_adc_values_outside_range_are_rejected(parameter: str, value: int) -> None:
    params = {"soil_raw": 2000, "light_raw": 2000, "temp_raw": 2000}
    params[parameter] = value

    response = post_analyze(**params)

    assert response.status_code == 422


def test_non_jpeg_content_type_is_rejected() -> None:
    response = client.post(
        "/analyze?soil_raw=2000&light_raw=2000&temp_raw=2000",
        content=JPEG_BYTES,
        headers={"Content-Type": "application/octet-stream"},
    )

    assert response.status_code == 415


def test_invalid_jpeg_bytes_are_rejected() -> None:
    response = client.post(
        "/analyze?soil_raw=2000&light_raw=2000&temp_raw=2000",
        content=b"not-a-jpeg",
        headers={"Content-Type": "image/jpeg"},
    )

    assert response.status_code == 400
