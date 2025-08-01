#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Тест реальных формул компенсации
"""

import math

def test_real_compensation():
    """Тест реальных формул компенсации"""
    
    print("🔬 ТЕСТ РЕАЛЬНЫХ ФОРМУЛ КОМПЕНСАЦИИ")
    print("=" * 60)
    
    # Тестовые данные
    temp = 30.0  # °C
    humidity = 45.0  # %
    ec_raw = 1200.0  # µS/cm
    ph_raw = 6.8
    n_raw = 800.0  # мг/кг
    p_raw = 600.0  # мг/кг
    k_raw = 400.0  # мг/кг
    
    print(f"📊 ВХОДНЫЕ ДАННЫЕ:")
    print(f"   Температура: {temp}°C")
    print(f"   Влажность: {humidity}%")
    print(f"   EC_raw: {ec_raw} µS/cm")
    print(f"   pH_raw: {ph_raw}")
    print(f"   N_raw: {n_raw} мг/кг")
    print(f"   P_raw: {p_raw} мг/кг")
    print(f"   K_raw: {k_raw} мг/кг")
    print()
    
    # 1. EC компенсация (Rhoades et al., 1989)
    # EC_comp = EC_raw × (1 + 0.021 × (T - 25))
    temp_factor_ec = 1.0 + 0.021 * (temp - 25.0)
    ec_comp = ec_raw * temp_factor_ec
    
    print(f"⚡ EC КОМПЕНСАЦИЯ (Rhoades et al., 1989):")
    print(f"   Формула: EC_comp = EC_raw × (1 + 0.021 × (T - 25))")
    print(f"   Температурный фактор: {temp_factor_ec:.3f}")
    print(f"   EC_raw: {ec_raw} µS/cm")
    print(f"   EC_comp: {ec_comp:.1f} µS/cm")
    print(f"   Разница: {ec_comp - ec_raw:.1f} µS/cm")
    print()
    
    # 2. pH компенсация (уравнение Нернста)
    # pH_comp = pH_raw - 0.003 × (T - 25)
    temp_correction_ph = -0.003 * (temp - 25.0)
    ph_comp = ph_raw + temp_correction_ph
    
    print(f"🧪 pH КОМПЕНСАЦИЯ (уравнение Нернста):")
    print(f"   Формула: pH_comp = pH_raw - 0.003 × (T - 25)")
    print(f"   Температурная поправка: {temp_correction_ph:.3f}")
    print(f"   pH_raw: {ph_raw}")
    print(f"   pH_comp: {ph_comp:.3f}")
    print(f"   Разница: {ph_comp - ph_raw:.3f}")
    print()
    
    # 3. NPK компенсация (Delgado et al., 2020) - LOAM почва
    # Коэффициенты для LOAM:
    delta_N = 0.0038
    delta_P = 0.0049
    delta_K = 0.0029
    epsilon_N = 0.009
    epsilon_P = 0.007
    epsilon_K = 0.011
    
    # Температурная компенсация: e^(δ(T-20))
    temp_factor_N = math.exp(delta_N * (temp - 20.0))
    temp_factor_P = math.exp(delta_P * (temp - 20.0))
    temp_factor_K = math.exp(delta_K * (temp - 20.0))
    
    # Влажностная компенсация: (1 + ε(θ-30))
    moisture_factor_N = 1.0 + (epsilon_N * (humidity - 30.0))
    moisture_factor_P = 1.0 + (epsilon_P * (humidity - 30.0))
    moisture_factor_K = 1.0 + (epsilon_K * (humidity - 30.0))
    
    # Полная компенсация
    n_comp = n_raw * temp_factor_N * moisture_factor_N
    p_comp = p_raw * temp_factor_P * moisture_factor_P
    k_comp = k_raw * temp_factor_K * moisture_factor_K
    
    print(f"🌱 NPK КОМПЕНСАЦИЯ (Delgado et al., 2020) - LOAM почва:")
    print(f"   Формула: N_comp = N_raw × e^(δN(T-20)) × (1 + εN(θ-30))")
    print(f"   Коэффициенты: δN={delta_N}, δP={delta_P}, δK={delta_K}")
    print(f"   Коэффициенты: εN={epsilon_N}, εP={epsilon_P}, εK={epsilon_K}")
    print()
    print(f"   Температурные факторы:")
    print(f"     N: {temp_factor_N:.3f}")
    print(f"     P: {temp_factor_P:.3f}")
    print(f"     K: {temp_factor_K:.3f}")
    print()
    print(f"   Влажностные факторы:")
    print(f"     N: {moisture_factor_N:.3f}")
    print(f"     P: {moisture_factor_P:.3f}")
    print(f"     K: {moisture_factor_K:.3f}")
    print()
    print(f"   Результаты компенсации:")
    print(f"     N: {n_raw} → {n_comp:.1f} (разница: {n_comp - n_raw:.1f})")
    print(f"     P: {p_raw} → {p_comp:.1f} (разница: {p_comp - p_raw:.1f})")
    print(f"     K: {k_raw} → {k_comp:.1f} (разница: {k_comp - k_raw:.1f})")
    print()
    
    print("=" * 60)
    print("📋 ВЫВОДЫ:")
    print()
    print("✅ РЕАЛЬНЫЕ КОМПЕНСАЦИИ:")
    print(f"   - EC: {ec_raw} → {ec_comp:.1f} (разница: {ec_comp - ec_raw:.1f} µS/cm)")
    print(f"   - pH: {ph_raw} → {ph_comp:.3f} (разница: {ph_comp - ph_raw:.3f})")
    print(f"   - N: {n_raw} → {n_comp:.1f} (разница: {n_comp - n_raw:.1f} мг/кг)")
    print(f"   - P: {p_raw} → {p_comp:.1f} (разница: {p_comp - p_raw:.1f} мг/кг)")
    print(f"   - K: {k_raw} → {k_comp:.1f} (разница: {k_comp - k_raw:.1f} мг/кг)")
    print()
    print("❌ НЕТ КОМПЕНСАЦИИ:")
    print("   - Температура: НЕТ формулы компенсации")
    print("   - Влажность: НЕТ формулы компенсации")
    print()
    print("🚨 ПРОБЛЕМА В ВЕБ-ИНТЕРФЕЙСЕ:")
    print("   Температура и влажность показываются как 'RAW' и 'компенсированные',")
    print("   хотя компенсации НЕТ. Это вводит пользователя в заблуждение.")
    print()
    print("🔧 РЕШЕНИЕ:")
    print("   1. Убрать 'компенсированные' значения для температуры и влажности")
    print("   2. Показывать только RAW значения для температуры и влажности")
    print("   3. Оставить компенсацию только для EC, pH и NPK")

if __name__ == "__main__":
    test_real_compensation() 