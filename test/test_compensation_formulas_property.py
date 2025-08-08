#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Property-based тесты инвариантов для научных компенсаций (EC, pH, NPK)
Оптимизировано для Windows (кодировка stdout защищена).
"""

import math
import sys
from hypothesis import given, strategies as st

# Защита stdout для Windows
if hasattr(sys.stdout, 'encoding') and sys.stdout.encoding and sys.stdout.encoding.lower() not in ['utf-8', 'utf8']:
    try:
        sys.stdout = open(sys.stdout.fileno(), mode='w', encoding='utf-8', buffering=1)
    except Exception:
        pass


@given(
    ec_raw=st.floats(min_value=0.0, max_value=10000.0, allow_nan=False, allow_infinity=False),
    temp=st.floats(min_value=-10.0, max_value=60.0, allow_nan=False, allow_infinity=False),
)
def test_ec_monotonic_vs_25c(ec_raw: float, temp: float) -> None:
    # EC_comp = EC_raw × (1 + 0.021 × (T - 25))
    factor = 1.0 + 0.021 * (temp - 25.0)
    ec_comp = ec_raw * factor

    if temp > 25.0:
        assert ec_comp >= ec_raw
    elif temp < 25.0:
        assert ec_comp <= ec_raw
    else:
        assert abs(ec_comp - ec_raw) <= 1e-6


@given(
    ph_raw=st.floats(min_value=0.0, max_value=14.0, allow_nan=False, allow_infinity=False),
    temp=st.floats(min_value=-10.0, max_value=60.0, allow_nan=False, allow_infinity=False),
)
def test_ph_decreases_with_temperature(ph_raw: float, temp: float) -> None:
    # pH_comp = pH_raw - 0.003 × (T - 25)
    ph_comp = ph_raw - 0.003 * (temp - 25.0)
    if temp > 25.0:
        assert ph_comp <= ph_raw
    elif temp < 25.0:
        assert ph_comp >= ph_raw
    else:
        assert abs(ph_comp - ph_raw) <= 1e-6


@given(
    n_raw=st.floats(min_value=0.0, max_value=5000.0, allow_nan=False, allow_infinity=False),
    p_raw=st.floats(min_value=0.0, max_value=5000.0, allow_nan=False, allow_infinity=False),
    k_raw=st.floats(min_value=0.0, max_value=5000.0, allow_nan=False, allow_infinity=False),
    temp=st.floats(min_value=-10.0, max_value=60.0, allow_nan=False, allow_infinity=False),
    humidity=st.floats(min_value=0.0, max_value=100.0, allow_nan=False, allow_infinity=False),
)
def test_npk_monotonicity(n_raw: float, p_raw: float, k_raw: float, temp: float, humidity: float) -> None:
    # Delgado et al. (2020)
    delta_N, delta_P, delta_K = 0.0038, 0.0049, 0.0029
    eps_N, eps_P, eps_K = 0.009, 0.007, 0.011

    tN = math.exp(delta_N * (temp - 20.0))
    tP = math.exp(delta_P * (temp - 20.0))
    tK = math.exp(delta_K * (temp - 20.0))

    mN = 1.0 + eps_N * (humidity - 30.0)
    mP = 1.0 + eps_P * (humidity - 30.0)
    mK = 1.0 + eps_K * (humidity - 30.0)

    n_comp = n_raw * tN * mN
    p_comp = p_raw * tP * mP
    k_comp = k_raw * tK * mK

    # По температуре: при T > 20 ожидаем рост относительно фиксированного влажностного фактора
    baseN = n_raw * mN
    baseP = p_raw * mP
    baseK = k_raw * mK
    if temp > 20.0:
        assert n_comp >= baseN
        assert p_comp >= baseP
        assert k_comp >= baseK
    elif temp < 20.0:
        assert n_comp <= baseN
        assert p_comp <= baseP
        assert k_comp <= baseK

    # По влажности: при θ > 30 ожидаем рост относительно фиксированного температурного фактора
    if humidity > 30.0:
        assert n_comp >= n_raw * tN
        assert p_comp >= p_raw * tP
        assert k_comp >= k_raw * tK
    elif humidity < 30.0:
        assert n_comp <= n_raw * tN
        assert p_comp <= p_raw * tP
        assert k_comp <= k_raw * tK


