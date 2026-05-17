#!/usr/bin/env python3
"""Download the latest Paratranz artifact and sync it into the local paratranz directory."""

from __future__ import annotations

import io
import os
import shutil
import sys
import tempfile
import urllib.error
import urllib.request
import zipfile
from pathlib import Path


DEFAULT_API_BASE = "https://paratranz.cn/api"
AUTH_HEADER = "Authorization"
AUTH_CANDIDATES = ("Bearer {token}", "{token}")


def require_env(name: str) -> str:
    value = os.environ.get(name, "").strip()
    if not value:
        raise SystemExit(f"Missing required environment variable: {name}")
    return value


def build_auth_values(token: str) -> list[str]:
    custom = os.environ.get("PARATRANZ_AUTH_VALUE", "").strip()
    if custom:
        return [custom]
    return [candidate.format(token=token) for candidate in AUTH_CANDIDATES]


def download_artifact(project_id: str, token: str, api_base: str) -> bytes:
    url = f"{api_base.rstrip('/')}/projects/{project_id}/artifacts/download"
    last_error: Exception | None = None

    for auth_value in build_auth_values(token):
        request = urllib.request.Request(
            url,
            headers={
                AUTH_HEADER: auth_value,
                "User-Agent": "SlashEM-Android para2github",
            },
        )
        try:
            with urllib.request.urlopen(request) as response:
                return response.read()
        except urllib.error.HTTPError as exc:
            last_error = exc
            if exc.code not in (401, 403):
                body = exc.read().decode("utf-8", errors="replace")
                raise SystemExit(
                    f"Paratranz download failed with HTTP {exc.code}: {body[:400]}"
                ) from exc
        except urllib.error.URLError as exc:
            raise SystemExit(f"Paratranz download failed: {exc}") from exc

    if isinstance(last_error, urllib.error.HTTPError):
        body = last_error.read().decode("utf-8", errors="replace")
        raise SystemExit(
            "Paratranz authentication failed with HTTP "
            f"{last_error.code}: {body[:400]}"
        )
    raise SystemExit("Paratranz download failed for an unknown reason")


def extract_zip(data: bytes, destination: Path) -> Path:
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        archive.extractall(destination)

    entries = list(destination.iterdir())
    if len(entries) == 1 and entries[0].is_dir():
        direct_json_files = list(entries[0].glob("*.json"))
        if not direct_json_files:
            return entries[0]
    return destination


def replace_directory_contents(target: Path, source: Path) -> None:
    if target.exists():
        shutil.rmtree(target)
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(source, target)


def main() -> int:
    project_id = require_env("PARATRANZ_PROJECT_ID")
    token = require_env("PARATRANZ_API_TOKEN")
    api_base = os.environ.get("PARATRANZ_API_BASE_URL", DEFAULT_API_BASE).strip() or DEFAULT_API_BASE
    target_dir = Path(os.environ.get("PARATRANZ_OUTPUT_DIR", "paratranz"))

    data = download_artifact(project_id, token, api_base)

    try:
        with tempfile.TemporaryDirectory() as tmp:
            root = extract_zip(data, Path(tmp))
            replace_directory_contents(target_dir, root)
    except zipfile.BadZipFile as exc:
        raise SystemExit("Paratranz returned a non-zip artifact") from exc

    print(f"Synced Paratranz artifact into {target_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
