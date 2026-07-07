from pathlib import Path

from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    app_name: str = "Smart License Plate Recognition Backend"
    app_version: str = "0.1.0"
    api_prefix: str = "/api/v1"
    host: str = "0.0.0.0"
    port: int = 8000
    model_config_path: Path = Path("../algorithms/configs/model_config.yaml")
    export_dir: Path = Path("../data/exports")
    sqlite_url: str = "sqlite:///../data/sqlite/license_plate.db"

    model_config = SettingsConfigDict(
        env_file="../../deploy/configs/.env",
        env_file_encoding="utf-8",
        extra="ignore",
    )


settings = Settings()
