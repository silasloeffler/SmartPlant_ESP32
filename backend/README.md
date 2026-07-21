# SmartPlant Backend

Voraussetzung: Python 3.11

## Einrichtung und Start

Unter Windows PowerShell:

```powershell
py -3.11 -m venv backend/.venv
.\backend\.venv\Scripts\Activate.ps1
python -m pip install -r backend/requirements.txt
python -m uvicorn backend.app:app --reload
```

Unter Linux oder macOS:

```bash
python3.11 -m venv backend/.venv
source backend/.venv/bin/activate
python -m pip install -r backend/requirements.txt
python -m uvicorn backend.app:app --reload
```

Danach ist der Health-Endpunkt unter
<http://127.0.0.1:8000/health> erreichbar. Die erwartete Antwort lautet:

```json
{"status":"ok"}
```

## Bildanalyse-Testendpunkt

`POST /analyze` erwartet JPEG-Bytes mit `Content-Type: image/jpeg` sowie die
Query-Parameter `soil_raw`, `light_raw` und `temp_raw`. Alle drei Werte müssen
im ADC-Bereich von 0 bis 4095 liegen. Beispiel:

```bash
curl -X POST "http://127.0.0.1:8000/analyze?soil_raw=3200&light_raw=500&temp_raw=2048" \
  -H "Content-Type: image/jpeg" \
  --data-binary "@plant.jpg"
```

Die Statusgrenzen können in einer lokalen `.env` anhand von `.env.example`
konfiguriert werden. Der Temperaturwert wird nicht umgerechnet, sondern nur als
ADC-Rohwert zurückgegeben. Eine externe KI-API wird noch nicht verwendet.

## Tests

```bash
python -m pytest backend/tests
```
