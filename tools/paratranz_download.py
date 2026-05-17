#!/usr/bin/env python3
"""Trigger Paratranz export, wait for the new artifact, then sync it locally."""

from __future__ import annotations

import io
import json
import os
import shutil
import tempfile
import time
import urllib.error
import urllib.request
import zipfile
from pathlib import Path
from typing import Any


DEFAULT_API_BASE = "https://paratranz.cn/api"
AUTH_HEADER = "Authorization"
DEFAULT_TIMEOUT_SECONDS = 300
DEFAULT_POLL_INTERVAL_SECONDS = 5


def require_env(name: str) -> str:
    value = os.environ.get(name, "").strip()
    if not value:
        raise SystemExit(f"Missing required environment variable: {name}")
    return value


def build_auth_values(token: str) -> list[str]:
    custom = os.environ.get("PARATRANZ_AUTH_VALUE", "").strip()
    if custom:
        return [custom]
    return [f"Bearer {token}"]


def request_bytes(
    url: str,
    token: str,
    *,
    method: str = "GET",
    body: bytes | None = None,
    accept: str | None = None,
) -> bytes:
    last_error: Exception | None = None

    for auth_value in build_auth_values(token):
        headers = {
            AUTH_HEADER: auth_value,
            "User-Agent": "SlashEM-Android para2github",
        }
        if accept:
            headers["Accept"] = accept
        if body is not None:
            headers["Content-Type"] = "application/json"

        request = urllib.request.Request(url, data=body, headers=headers, method=method)
        try:
            with urllib.request.urlopen(request) as response:
                return response.read()
        except urllib.error.HTTPError as exc:
            last_error = exc
            if exc.code not in (401, 403):
                body_text = exc.read().decode("utf-8", errors="replace")
                raise SystemExit(
                    f"Paratranz request failed with HTTP {exc.code}: {body_text[:400]}"
                ) from exc
        except urllib.error.URLError as exc:
            raise SystemExit(f"Paratranz request failed: {exc}") from exc

    if isinstance(last_error, urllib.error.HTTPError):
        body_text = last_error.read().decode("utf-8", errors="replace")
        raise SystemExit(
            "Paratranz authentication failed with HTTP "
            f"{last_error.code}: {body_text[:400]}"
        )
    raise SystemExit("Paratranz request failed for an unknown reason")


def request_json(
    url: str,
    token: str,
    *,
    method: str = "GET",
    payload: dict[str, Any] | None = None,
) -> dict[str, Any]:
    body = None if payload is None else json.dumps(payload).encode("utf-8")
    data = request_bytes(url, token, method=method, body=body, accept="application/json")
    try:
        parsed = json.loads(data.decode("utf-8"))
    except json.JSONDecodeError as exc:
        raise SystemExit(f"Paratranz returned invalid JSON from {url}") from exc
    if not isinstance(parsed, dict):
        raise SystemExit(f"Paratranz returned unexpected JSON shape from {url}")
    return parsed


def get_artifact(project_id: str, token: str, api_base: str) -> dict[str, Any] | None:
    url = f"{api_base.rstrip('/')}/projects/{project_id}/artifacts"
    try:
        return request_json(url, token)
    except SystemExit as exc:
        if "HTTP 404" in str(exc):
            return None
        raise


def trigger_export(project_id: str, token: str, api_base: str) -> dict[str, Any]:
    url = f"{api_base.rstrip('/')}/projects/{project_id}/artifacts"
    return request_json(url, token, method="POST", payload={})


def artifact_marker(artifact: dict[str, Any] | None) -> tuple[Any, Any]:
    if not artifact:
        return None, None
    return artifact.get("id"), artifact.get("createdAt")


def wait_for_new_artifact(
    project_id: str,
    token: str,
    api_base: str,
    previous_artifact: dict[str, Any] | None,
    timeout_seconds: int,
    poll_interval_seconds: int,
) -> dict[str, Any]:
    previous_marker = artifact_marker(previous_artifact)
    deadline = time.monotonic() + timeout_seconds

    while time.monotonic() < deadline:
        artifact = get_artifact(project_id, token, api_base)
        if artifact and artifact_marker(artifact) != previous_marker:
            return artifact
        time.sleep(poll_interval_seconds)

    raise SystemExit(
        f"Timed out after {timeout_seconds}s waiting for a new Paratranz artifact"
    )


def download_artifact_zip(project_id: str, token: str, api_base: str) -> bytes:
    url = f"{api_base.rstrip('/')}/projects/{project_id}/artifacts/download"
    return request_bytes(url, token)


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
    timeout_seconds = int(
        os.environ.get("PARATRANZ_EXPORT_TIMEOUT_SECONDS", DEFAULT_TIMEOUT_SECONDS)
    )
    poll_interval_seconds = int(
        os.environ.get(
            "PARATRANZ_EXPORT_POLL_INTERVAL_SECONDS",
            DEFAULT_POLL_INTERVAL_SECONDS,
        )
    )

    previous_artifact = get_artifact(project_id, token, api_base)
    job = trigger_export(project_id, token, api_base)
    print(
        "Triggered Paratranz export job",
        job.get("id"),
        "status",
        job.get("status"),
    )
    artifact = wait_for_new_artifact(
        project_id,
        token,
        api_base,
        previous_artifact,
        timeout_seconds,
        poll_interval_seconds,
    )
    print("Paratranz artifact ready:", artifact.get("id"), artifact.get("createdAt"))

    data = download_artifact_zip(project_id, token, api_base)

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
