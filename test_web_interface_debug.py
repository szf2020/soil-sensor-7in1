#!/usr/bin/env python3
"""
Диагностический скрипт для проверки веб-интерфейса калибровки
"""

import requests
import json
import time

def test_web_interface():
    base_url = "http://192.168.2.66"
    
    print("🔍 ДИАГНОСТИКА ВЕБ-ИНТЕРФЕЙСА КАЛИБРОВКИ")
    print("=" * 50)
    
    # 1. Проверяем доступность страницы калибровки
    print("1️⃣ Проверка страницы калибровки...")
    try:
        response = requests.get(f"{base_url}/calibration", timeout=5)
        print(f"   Статус: {response.status_code}")
        if response.status_code == 200:
            print("   ✅ Страница доступна")
            # Проверяем наличие JavaScript функций
            content = response.text
            if "calibrateEC()" in content:
                print("   ✅ Функция calibrateEC() найдена")
            else:
                print("   ❌ Функция calibrateEC() НЕ найдена")
                
            if "fetch('/api/calibration/ec'" in content:
                print("   ✅ URL /api/calibration/ec найден")
            else:
                print("   ❌ URL /api/calibration/ec НЕ найден")
        else:
            print("   ❌ Страница недоступна")
    except Exception as e:
        print(f"   ❌ Ошибка: {e}")
    
    # 2. Проверяем API endpoints
    print("\n2️⃣ Проверка API endpoints...")
    
    # Статус калибровки
    try:
        response = requests.get(f"{base_url}/api/calibration/status", timeout=5)
        print(f"   GET /api/calibration/status: {response.status_code}")
        if response.status_code == 200:
            data = response.json()
            print(f"   ✅ Ответ: {json.dumps(data, indent=2)}")
    except Exception as e:
        print(f"   ❌ Ошибка: {e}")
    
    # EC калибровка
    try:
        payload = {
            "expected_1": 1.41,
            "measured_1": 1.4,
            "expected_2": 12.88,
            "measured_2": 12.9
        }
        response = requests.post(f"{base_url}/api/calibration/ec", 
                               json=payload, timeout=5)
        print(f"   POST /api/calibration/ec: {response.status_code}")
        if response.status_code == 200:
            data = response.json()
            print(f"   ✅ Ответ: {json.dumps(data, indent=2)}")
    except Exception as e:
        print(f"   ❌ Ошибка: {e}")
    
    # 3. Проверяем CORS и заголовки
    print("\n3️⃣ Проверка CORS и заголовков...")
    try:
        response = requests.options(f"{base_url}/api/calibration/status", timeout=5)
        print(f"   OPTIONS запрос: {response.status_code}")
        print(f"   Заголовки: {dict(response.headers)}")
    except Exception as e:
        print(f"   ❌ Ошибка: {e}")
    
    # 4. Тестируем с реальными данными браузера
    print("\n4️⃣ Тестирование с браузерными заголовками...")
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36',
        'Accept': 'application/json, text/plain, */*',
        'Accept-Language': 'ru-RU,ru;q=0.9,en;q=0.8',
        'Content-Type': 'application/json',
        'Origin': base_url,
        'Referer': f'{base_url}/calibration'
    }
    
    try:
        payload = {
            "expected_1": 1.41,
            "measured_1": 1.4,
            "expected_2": 12.88,
            "measured_2": 12.9
        }
        response = requests.post(f"{base_url}/api/calibration/ec", 
                               json=payload, headers=headers, timeout=5)
        print(f"   POST с браузерными заголовками: {response.status_code}")
        if response.status_code == 200:
            data = response.json()
            print(f"   ✅ Ответ: {json.dumps(data, indent=2)}")
    except Exception as e:
        print(f"   ❌ Ошибка: {e}")

if __name__ == "__main__":
    test_web_interface()
