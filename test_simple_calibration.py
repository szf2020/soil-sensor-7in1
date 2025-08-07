#!/usr/bin/env python3
"""
Простой тест сохранения калибровки
"""

import requests
import json

def test_simple_calibration():
    base_url = "http://192.168.2.66"
    
    print("🔍 ПРОСТОЙ ТЕСТ СОХРАНЕНИЯ КАЛИБРОВКИ")
    print("=" * 40)
    
    # 1. Проверяем статус до калибровки
    print("\n1️⃣ Статус ДО калибровки:")
    try:
        response = requests.get(f"{base_url}/api/calibration/status", timeout=5)
        if response.status_code == 200:
            status = response.json()
            print(f"EC калибровка: {status.get('ec_calibrated', 'НЕТ')}")
            print(f"Калибровка включена: {status.get('calibration_enabled', 'НЕТ')}")
        else:
            print(f"❌ Ошибка: {response.status_code}")
            return
    except Exception as e:
        print(f"❌ Ошибка подключения: {e}")
        return
    
    # 2. Выполняем EC калибровку
    print("\n2️⃣ Выполняем EC калибровку:")
    ec_data = {
        "expected_1": 1000.0,
        "measured_1": 950.0,
        "expected_2": 2000.0,
        "measured_2": 1900.0
    }
    
    try:
        response = requests.post(
            f"{base_url}/api/calibration/ec/calculate",
            json=ec_data,
            timeout=10
        )
        print(f"Статус ответа: {response.status_code}")
        if response.status_code == 200:
            result = response.json()
            print(f"Успех: {result.get('success', False)}")
            print(f"Качество: {result.get('quality', 'НЕТ')}")
        else:
            print(f"❌ Ошибка: {response.text}")
    except Exception as e:
        print(f"❌ Ошибка: {e}")
    
    # 3. Проверяем статус после калибровки
    print("\n3️⃣ Статус ПОСЛЕ калибровки:")
    try:
        response = requests.get(f"{base_url}/api/calibration/status", timeout=5)
        if response.status_code == 200:
            status = response.json()
            print(f"EC калибровка: {status.get('ec_calibrated', 'НЕТ')}")
            print(f"Калибровка включена: {status.get('calibration_enabled', 'НЕТ')}")
            
            if status.get('ec_calibrated'):
                print("✅ СОХРАНЕНИЕ РАБОТАЕТ!")
            else:
                print("❌ СОХРАНЕНИЕ НЕ РАБОТАЕТ!")
        else:
            print(f"❌ Ошибка: {response.status_code}")
    except Exception as e:
        print(f"❌ Ошибка: {e}")

if __name__ == "__main__":
    test_simple_calibration()
