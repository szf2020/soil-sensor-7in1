#!/usr/bin/env python3
"""
Тест JavaScript в браузере
Проверяет выполнение JavaScript функций
"""

import time
import requests
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.chrome.options import Options
from selenium.common.exceptions import TimeoutException

# Конфигурация
ESP32_IP = "192.168.2.66"
CALIBRATION_URL = f"http://{ESP32_IP}/calibration"

def setup_driver():
    """Настройка Chrome драйвера с включенными логами"""
    chrome_options = Options()
    chrome_options.add_argument("--headless")
    chrome_options.add_argument("--no-sandbox")
    chrome_options.add_argument("--disable-dev-shm-usage")
    chrome_options.add_argument("--disable-gpu")
    chrome_options.add_argument("--window-size=1920,1080")
    chrome_options.add_argument("--enable-logging")
    chrome_options.add_argument("--v=1")
    
    try:
        driver = webdriver.Chrome(options=chrome_options)
        return driver
    except Exception as e:
        print(f"❌ Ошибка создания драйвера: {e}")
        return None

def test_javascript_execution():
    """Тест выполнения JavaScript"""
    print("🔍 ТЕСТ JAVASCRIPT В БРАУЗЕРЕ")
    print("=" * 50)
    
    driver = setup_driver()
    if not driver:
        return
    
    try:
        # Загружаем страницу
        print("📄 Загрузка страницы...")
        driver.get(CALIBRATION_URL)
        time.sleep(3)
        
        # Проверяем заголовок
        title = driver.title
        print(f"   Заголовок: {title}")
        
        # Проверяем наличие элемента статуса
        status_element = driver.find_element(By.ID, "calibrationStatus")
        initial_text = status_element.text
        print(f"   Начальный статус: {initial_text}")
        
        # Выполняем JavaScript функцию loadCalibrationStatus
        print("   🔧 Выполнение loadCalibrationStatus()...")
        driver.execute_script("loadCalibrationStatus();")
        time.sleep(2)
        
        # Проверяем изменение статуса
        status_after = status_element.text
        print(f"   Статус после JS: {status_after}")
        
        if initial_text != status_after:
            print("   ✅ JavaScript работает - статус изменился")
        else:
            print("   ❌ JavaScript НЕ работает - статус не изменился")
        
        # Проверяем консоль браузера на ошибки
        print("   📋 Проверка консоли браузера...")
        logs = driver.get_log('browser')
        if logs:
            print(f"   Найдено {len(logs)} записей в консоли:")
            for log in logs[:5]:  # Показываем первые 5
                print(f"     {log['level']}: {log['message']}")
        else:
            print("   Консоль пуста")
        
        # Тестируем fetch API напрямую
        print("   🌐 Тест fetch API...")
        fetch_result = driver.execute_script("""
            return fetch('/api/calibration/status')
                .then(response => response.json())
                .then(data => {
                    console.log('Fetch result:', data);
                    return data;
                })
                .catch(err => {
                    console.error('Fetch error:', err);
                    return {error: err.message};
                });
        """)
        
        print(f"   Результат fetch: {fetch_result}")
        
        # Проверяем, есть ли ошибки CORS
        print("   🔍 Проверка CORS...")
        cors_test = driver.execute_script("""
            return new Promise((resolve) => {
                const xhr = new XMLHttpRequest();
                xhr.open('GET', '/api/calibration/status', true);
                xhr.onreadystatechange = function() {
                    if (xhr.readyState === 4) {
                        resolve({
                            status: xhr.status,
                            response: xhr.responseText,
                            headers: xhr.getAllResponseHeaders()
                        });
                    }
                };
                xhr.onerror = function() {
                    resolve({error: 'XHR error'});
                };
                xhr.send();
            });
        """)
        
        print(f"   XHR результат: {cors_test}")
        
    except Exception as e:
        print(f"   ❌ Ошибка тестирования: {e}")
    
    finally:
        driver.quit()

def test_api_directly():
    """Прямой тест API"""
    print("\n🔍 ПРЯМОЙ ТЕСТ API")
    print("=" * 30)
    
    try:
        # Тест GET запроса
        response = requests.get(f"http://{ESP32_IP}/api/calibration/status", timeout=5)
        print(f"   GET /api/calibration/status: {response.status_code}")
        print(f"   Заголовки: {dict(response.headers)}")
        print(f"   Ответ: {response.text}")
        
        # Тест POST запроса
        test_data = {
            "expected_1": 1.0,
            "measured_1": 1.1,
            "expected_2": 2.0,
            "measured_2": 2.1
        }
        
        response = requests.post(
            f"http://{ESP32_IP}/api/calibration/ec",
            json=test_data,
            headers={'Content-Type': 'application/json'},
            timeout=5
        )
        print(f"   POST /api/calibration/ec: {response.status_code}")
        print(f"   Ответ: {response.text}")
        
    except Exception as e:
        print(f"   ❌ Ошибка API: {e}")

if __name__ == "__main__":
    test_javascript_execution()
    test_api_directly()
