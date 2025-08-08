#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import requests
import pytest


def get_base_url() -> str:
    # JXCT_DEVICE_IP can be set to override device IP, default kept for local runs
    ip = os.environ.get("JXCT_DEVICE_IP", "192.168.2.74")
    return f"http://{ip}"


@pytest.fixture(scope="session")
def device_base_url():
    return get_base_url()


def ensure_device_or_skip(base_url: str, timeout: float = 1.5) -> None:
    try:
        requests.get(f"{base_url}/health", timeout=timeout)
    except Exception:
        pytest.skip("ESP32 недоступен (ожидаемо для CI/без железа)")


