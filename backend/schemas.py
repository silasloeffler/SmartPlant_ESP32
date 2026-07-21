from pydantic import BaseModel, Field


class GeminiPlantAnalysis(BaseModel):
    image_observation: str = Field(
        min_length=1,
        description="Kurze, neutrale und belegbare Beobachtung auf Deutsch.",
    )
    plant_message: str = Field(
        min_length=1,
        description="Warme Persönlichkeitsschicht aus Sicht der Pflanze auf Deutsch.",
    )
