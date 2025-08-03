#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Тест для проверки данных в веб-интерфейсе
"""

import sys
import json

# Принудительно устанавливаем stdout в utf-8 для Windows
if hasattr(sys.stdout, 'encoding') and sys.stdout.encoding and sys.stdout.encoding.lower() not in ['utf-8', 'utf8']:
    try:
        sys.stdout = open(sys.stdout.fileno(), mode='w', encoding='utf-8', buffering=1)
    except Exception:
        pass

def test_web_data():
    """Тест данных в веб-интерфейсе"""
    print("🔍 ТЕСТ ДАННЫХ В ВЕБ-ИНТЕРФЕЙСЕ")
    print("=" * 60)
    
    # Симулируем данные, которые могут прийти с веб-интерфейса
    # Это пример того, что вы видите в браузере
    web_data = {
        "temperature": 25.5,
        "humidity": 45.2,
        "ec": 1200.0,
        "ph": 6.8,
        "nitrogen": 800.0,
        "phosphorus": 600.0,
        "potassium": 400.0,
        "raw_temperature": 25.3,  # ❌ РАЗНЫЕ!
        "raw_humidity": 44.8,     # ❌ РАЗНЫЕ!
        "raw_ec": 1195.0,         # ❌ РАЗНЫЕ!
        "raw_ph": 6.7,            # ❌ РАЗНЫЕ!
        "raw_nitrogen": 795.0,    # ❌ РАЗНЫЕ!
        "raw_phosphorus": 598.0,  # ❌ РАЗНЫЕ!
        "raw_potassium": 402.0    # ❌ РАЗНЫЕ!
    }
    
    print("📊 АНАЛИЗ ДАННЫХ ВЕБ-ИНТЕРФЕЙСА:")
    print()
    
    # Проверяем температуру
    temp_diff = abs(web_data["temperature"] - web_data["raw_temperature"])
    print(f"🌡️ ТЕМПЕРАТУРА:")
    print(f"   Скомпенсированная: {web_data['temperature']}°C")
    print(f"   RAW: {web_data['raw_temperature']}°C")
    print(f"   Разница: {temp_diff}°C")
    print(f"   Статус: {'❌ РАЗНЫЕ' if temp_diff > 0.01 else '✅ ОДИНАКОВЫЕ'}")
    print()
    
    # Проверяем влажность
    hum_diff = abs(web_data["humidity"] - web_data["raw_humidity"])
    print(f"💧 ВЛАЖНОСТЬ:")
    print(f"   Скомпенсированная: {web_data['humidity']}%")
    print(f"   RAW: {web_data['raw_humidity']}%")
    print(f"   Разница: {hum_diff}%")
    print(f"   Статус: {'❌ РАЗНЫЕ' if hum_diff > 0.01 else '✅ ОДИНАКОВЫЕ'}")
    print()
    
    # Проверяем EC
    ec_diff = abs(web_data["ec"] - web_data["raw_ec"])
    print(f"⚡ EC:")
    print(f"   Скомпенсированный: {web_data['ec']} µS/cm")
    print(f"   RAW: {web_data['raw_ec']} µS/cm")
    print(f"   Разница: {ec_diff} µS/cm")
    print(f"   Статус: {'❌ РАЗНЫЕ' if ec_diff > 0.01 else '✅ ОДИНАКОВЫЕ'}")
    print()
    
    # Проверяем pH
    ph_diff = abs(web_data["ph"] - web_data["raw_ph"])
    print(f"🧪 pH:")
    print(f"   Скомпенсированный: {web_data['ph']}")
    print(f"   RAW: {web_data['raw_ph']}")
    print(f"   Разница: {ph_diff}")
    print(f"   Статус: {'❌ РАЗНЫЕ' if ph_diff > 0.001 else '✅ ОДИНАКОВЫЕ'}")
    print()
    
    # Проверяем NPK
    n_diff = abs(web_data["nitrogen"] - web_data["raw_nitrogen"])
    p_diff = abs(web_data["phosphorus"] - web_data["raw_phosphorus"])
    k_diff = abs(web_data["potassium"] - web_data["raw_potassium"])
    
    print(f"🌱 NPK:")
    print(f"   N: {web_data['raw_nitrogen']} → {web_data['nitrogen']} (разница: {n_diff})")
    print(f"   P: {web_data['raw_phosphorus']} → {web_data['phosphorus']} (разница: {p_diff})")
    print(f"   K: {web_data['raw_potassium']} → {web_data['potassium']} (разница: {k_diff})")
    print(f"   Статус: {'❌ РАЗНЫЕ' if (n_diff > 0.01 or p_diff > 0.01 or k_diff > 0.01) else '✅ ОДИНАКОВЫЕ'}")
    print()
    
    # Общий вывод
    total_diff = temp_diff + hum_diff + ec_diff + ph_diff + n_diff + p_diff + k_diff
    print("📋 ИТОГОВЫЙ АНАЛИЗ:")
    print(f"   Общая разница: {total_diff}")
    if total_diff < 0.1:
        print("   ✅ ВСЕ ДАННЫЕ ОДИНАКОВЫЕ - настройки сброшены корректно")
    else:
        print("   ❌ ЕСТЬ РАЗНИЦА - что-то применяет коррекцию")
    
    print()
    print("🔍 ВЫВОД:")
    print("   Если настройки сброшены, все значения должны быть одинаковыми.")
    print("   Если есть разница - значит где-то применяется калибровка или компенсация.")
    print()
    print("🔧 ВОЗМОЖНЫЕ ПРИЧИНЫ РАЗНИЦЫ:")
    print("   1. Калибровочные таблицы загружены в памяти")
    print("   2. Компенсация включена в настройках")
    print("   3. Проблема в порядке обработки данных")
    print("   4. Данные сохраняются в RAW ПОСЛЕ обработки")
    print()
    print("🎯 РЕКОМЕНДАЦИЯ:")
    print("   Проверьте веб-интерфейс и посмотрите, действительно ли RAW ≠ скомпенсированные")
    print("   Если да - значит где-то применяется коррекция, хотя не должна")

if __name__ == "__main__":
    test_web_data() 