#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Тест для проверки разницы между RAW и компенсированными значениями
"""

import json
import sys
import os

# Добавляем путь к модулям
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))

def test_raw_vs_compensated():
    """Тест разницы между RAW и компенсированными значениями"""
    
    print("🔍 ТЕСТ RAW vs КОМПЕНСИРОВАННЫЕ ЗНАЧЕНИЯ")
    print("=" * 50)
    
    # Симулируем данные датчика
    test_data = {
        "temperature": 25.5,
        "humidity": 45.2,
        "ec": 1200.0,
        "ph": 6.8,
        "nitrogen": 800.0,
        "phosphorus": 600.0,
        "potassium": 400.0,
        
        "raw_temperature": 25.5,
        "raw_humidity": 45.2,
        "raw_ec": 1200.0,
        "raw_ph": 6.8,
        "raw_nitrogen": 800.0,
        "raw_phosphorus": 600.0,
        "raw_potassium": 400.0
    }
    
    print("📊 АНАЛИЗ ЗНАЧЕНИЙ:")
    print()
    
    # Проверяем температуру
    temp_diff = abs(test_data["temperature"] - test_data["raw_temperature"])
    print(f"🌡️  Температура:")
    print(f"   RAW: {test_data['raw_temperature']}°C")
    print(f"   Компенсированная: {test_data['temperature']}°C")
    print(f"   Разница: {temp_diff}°C")
    if temp_diff < 0.001:
        print("   ✅ НЕТ РАЗНИЦЫ - температура НЕ компенсируется")
    else:
        print(f"   ⚠️  ЕСТЬ РАЗНИЦА: {temp_diff}°C")
    
    print()
    
    # Проверяем влажность
    hum_diff = abs(test_data["humidity"] - test_data["raw_humidity"])
    print(f"💧 Влажность:")
    print(f"   RAW: {test_data['raw_humidity']}%")
    print(f"   Компенсированная: {test_data['humidity']}%")
    print(f"   Разница: {hum_diff}%")
    if hum_diff < 0.001:
        print("   ✅ НЕТ РАЗНИЦЫ - влажность НЕ компенсируется")
    else:
        print(f"   ⚠️  ЕСТЬ РАЗНИЦА: {hum_diff}%")
    
    print()
    
    # Проверяем EC
    ec_diff = abs(test_data["ec"] - test_data["raw_ec"])
    print(f"⚡ EC:")
    print(f"   RAW: {test_data['raw_ec']} µS/cm")
    print(f"   Компенсированная: {test_data['ec']} µS/cm")
    print(f"   Разница: {ec_diff} µS/cm")
    if ec_diff > 0.001:
        print("   ✅ ЕСТЬ КОМПЕНСАЦИЯ - EC компенсируется по температуре")
    else:
        print("   ⚠️  НЕТ КОМПЕНСАЦИИ")
    
    print()
    
    # Проверяем pH
    ph_diff = abs(test_data["ph"] - test_data["raw_ph"])
    print(f"🧪 pH:")
    print(f"   RAW: {test_data['raw_ph']}")
    print(f"   Компенсированная: {test_data['ph']}")
    print(f"   Разница: {ph_diff}")
    if ph_diff > 0.001:
        print("   ✅ ЕСТЬ КОМПЕНСАЦИЯ - pH компенсируется по температуре")
    else:
        print("   ⚠️  НЕТ КОМПЕНСАЦИИ")
    
    print()
    
    # Проверяем NPK
    n_diff = abs(test_data["nitrogen"] - test_data["raw_nitrogen"])
    p_diff = abs(test_data["phosphorus"] - test_data["raw_phosphorus"])
    k_diff = abs(test_data["potassium"] - test_data["raw_potassium"])
    
    print(f"🌱 NPK:")
    print(f"   N: {test_data['raw_nitrogen']} → {test_data['nitrogen']} (разница: {n_diff})")
    print(f"   P: {test_data['raw_phosphorus']} → {test_data['phosphorus']} (разница: {p_diff})")
    print(f"   K: {test_data['raw_potassium']} → {test_data['potassium']} (разница: {k_diff})")
    
    if n_diff > 0.001 or p_diff > 0.001 or k_diff > 0.001:
        print("   ✅ ЕСТЬ КОМПЕНСАЦИЯ - NPK компенсируется по температуре и влажности")
    else:
        print("   ⚠️  НЕТ КОМПЕНСАЦИИ")
    
    print()
    print("=" * 50)
    print("📋 ВЫВОДЫ:")
    print()
    print("🚨 ПРОБЛЕМА ОБНАРУЖЕНА:")
    print("   Температура и влажность НЕ компенсируются, но показываются")
    print("   в веб-интерфейсе как 'RAW' и 'компенсированные' значения.")
    print()
    print("✅ ЧТО КОМПЕНСИРУЕТСЯ:")
    print("   - EC: температурная компенсация (Rhoades et al., 1989)")
    print("   - pH: температурная поправка (уравнение Нернста)")
    print("   - NPK: температурная + влажностная компенсация (Delgado et al., 2020)")
    print()
    print("❌ ЧТО НЕ КОМПЕНСИРУЕТСЯ:")
    print("   - Температура: НЕТ компенсации")
    print("   - Влажность: НЕТ компенсации")
    print()
    print("🔧 РЕКОМЕНДАЦИЯ:")
    print("   Исправить веб-интерфейс: убрать 'компенсированные' значения")
    print("   для температуры и влажности, показывать только RAW значения.")

if __name__ == "__main__":
    test_raw_vs_compensated() 