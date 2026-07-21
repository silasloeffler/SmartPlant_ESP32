# SmartPlant Backend

Das Python-3.11-Backend kombiniert kalibrierte Sensorwerte mit genau einem
Gemini-Aufruf. Wie beim OpenAI-Projekt PlantTalk bleiben die neutrale,
belegbare Beobachtung und die Persönlichkeit der Pflanze klar getrennt.

## Einrichtung und Start

Unter Windows PowerShell:

```powershell
py -3.11 -m venv backend/.venv
.\backend\.venv\Scripts\Activate.ps1
python -m pip install -r backend/requirements.txt
python -m uvicorn backend.app:app --host 0.0.0.0 --port 8000 --reload
```

Unter Linux oder macOS:

```bash
python3.11 -m venv backend/.venv
source backend/.venv/bin/activate
python -m pip install -r backend/requirements.txt
python -m uvicorn backend.app:app --host 0.0.0.0 --port 8000 --reload
```

Die vorhandene `backend/.env` enthält Server-, Gemini- und
Kalibrierungseinstellungen. Sie wird nicht versioniert. Erforderlich sind:

```dotenv
APP_HOST=0.0.0.0
APP_PORT=8000
GEMINI_API_KEY=...
GEMINI_MODEL=gemini-2.5-flash
SOIL_WET_RAW=471
SOIL_DRY_RAW=2447
SOIL_DRY_PERCENT_THRESHOLD=20
LIGHT_DARK_RAW=580
LIGHT_BRIGHT_RAW=4095
LIGHT_LOW_PERCENT_THRESHOLD=15
```

## Endpunkte

`GET /health` antwortet mit `{"status":"ok"}`.

`POST /analyze` erwartet JPEG-Bytes mit `Content-Type: image/jpeg` und die
Query-Parameter `soil_raw`, `light_raw` und `temp_raw`. Alle ADC-Werte müssen
zwischen 0 und 4095 liegen.

```bash
curl -X POST "http://127.0.0.1:8000/analyze?soil_raw=1459&light_raw=2540&temp_raw=2050" \
  -H "Content-Type: image/jpeg" \
  --data-binary "@plant.jpg"
```

Die Antwort enthält `sensor_status` mit Roh- und Prozentwerten, eine sachliche
`image_observation`, die getrennte Persönlichkeitsschicht `plant_message` und
das verwendete `model`.

## Kalibrierung und Referenzmessungen

Die Bodenfeuchte wird umgekehrt linear abgebildet: Ein niedriger Rohwert steht
für nassen Boden (100 %), ein hoher für trockenen Boden (0 %). Beim Licht steht
ein hoher Rohwert für hohe Helligkeit. Werte außerhalb der Kalibrierpunkte
werden auf 0 bis 100 % begrenzt.

Vollständige Referenzmessungen:

- Licht maximal hell: 4095
- Licht vollständig dunkel: 580
- Indirektes Tageslicht: 2540
- Temperatur: ADC 2050 entsprach ungefähr 23 °C
- Boden in Wasser: 471
- Sensor in feuchter Erde beim Live-Test: `soil_raw = 1660`, entspricht mit
  der aktuellen Kalibrierung ungefähr 40 %
- Boden vollständig trocken in Luft: 2447
- Temperaturreferenz mit Miaomiaoce Thermo-Hygrometer

Die einzelne Temperaturreferenz ist keine Kalibrierkurve. `temp_raw` bleibt
ausschließlich ein ADC-Rohwert; das Backend berechnet daraus keine Celsiuszahl
und nimmt keine Temperaturbewertung vor.

## Tests

Gemini wird in den automatisierten Tests vollständig gemockt:

```bash
python -m pytest backend/tests
```
