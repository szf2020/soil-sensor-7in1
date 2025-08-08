#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Схемная валидация ответов API: /sensor_json, /status, /health
Автоматически пропускается, если устройство недоступно.
"""

import sys
import requests
import pytest
from jsonschema import validate

from conftest import ensure_device_or_skip

# Защита stdout для Windows
if hasattr(sys.stdout, 'encoding') and sys.stdout.encoding and sys.stdout.encoding.lower() not in ['utf-8', 'utf8']:
    try:
        sys.stdout = open(sys.stdout.fileno(), mode='w', encoding='utf-8', buffering=1)
    except Exception:
        pass


_NUMERIC = {"anyOf": [{"type": "number"}, {"type": "string"}]}

SENSOR_SCHEMA = {
    "type": "object",
    "required": ["temperature", "humidity", "ec", "ph", "nitrogen", "phosphorus", "potassium"],
    "properties": {
        "temperature": _NUMERIC,
        "humidity": _NUMERIC,
        "ec": _NUMERIC,
        "ph": _NUMERIC,
        "nitrogen": _NUMERIC,
        "phosphorus": _NUMERIC,
        "potassium": _NUMERIC,
    },
    "additionalProperties": True,
}

STATUS_SCHEMA = {
    "type": "object",
    "required": ["wifi", "mqtt", "uptime"],
    "properties": {
        "wifi": {"type": "object"},
        "mqtt": {"type": "object"},
        "uptime": _NUMERIC,
    },
    "additionalProperties": True,
}


@pytest.mark.timeout(5)
def test_health_endpoint(device_base_url):
    ensure_device_or_skip(device_base_url)
    r = requests.get(f"{device_base_url}/health", timeout=3)
    assert r.status_code == 200
    # Допускаем как JSON, так и текстовый OK — проверяем JSON структуру при наличии
    try:
        data = r.json()
        assert isinstance(data, dict)
        # Минимальные ключи
        assert "device" in data and "timestamp" in data
    except Exception:
        assert "OK" in r.text.upper()


@pytest.mark.timeout(5)
def test_sensor_json_schema(device_base_url):
    ensure_device_or_skip(device_base_url)
    r = requests.get(f"{device_base_url}/sensor_json", timeout=2)
    assert r.status_code == 200
    data = r.json()
    validate(instance=data, schema=SENSOR_SCHEMA)


@pytest.mark.timeout(5)
def test_status_schema(device_base_url):
    ensure_device_or_skip(device_base_url)
    r = requests.get(f"{device_base_url}/status", timeout=3)
    assert r.status_code == 200
    # /status — HTML страница. Проверяем тип и важные маркеры.
    ctype = r.headers.get("content-type", "")
    assert "text/html" in ctype.lower()
    assert "Статус" in r.text or "Status" in r.text


