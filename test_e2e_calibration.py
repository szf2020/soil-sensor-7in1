#!/usr/bin/env python3
"""
E2E тест калибровки с реальным браузером
Проверяет нажатие кнопок и обновление статуса
"""

import time
import json
import requests
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.chrome.options import Options
from selenium.common.exceptions import TimeoutException, NoSuchElementException

# Конфигурация
ESP32_IP = "192.168.2.66"
CALIBRATION_URL = f"http://{ESP32_IP}/calibration"
API_STATUS_URL = f"http://{ESP32_IP}/api/calibration/status"

def setup_driver():
    """Настройка Chrome драйвера"""
    chrome_options = Options()
    chrome_options.add_argument("--headless")  # Без GUI
    chrome_options.add_argument("--no-sandbox")
    chrome_options.add_argument("--disable-dev-shm-usage")
    chrome_options.add_argument("--disable-gpu")
    chrome_options.add_argument("--window-size=1920,1080")
    
    try:
        driver = webdriver.Chrome(options=chrome_options)
        return driver
    except Exception as e:
        print(f"❌ Ошибка создания драйвера: {e}")
        return None

def check_api_status():
    """Проверка API статуса калибровки"""
    try:
        response = requests.get(API_STATUS_URL, timeout=5)
        if response.status_code == 200:
            data = response.json()
            print(f"📊 API статус: {json.dumps(data, indent=2, ensure_ascii=False)}")
            return data
        else:
            print(f"❌ API ошибка: {response.status_code}")
            return None
    except Exception as e:
        print(f"❌ Ошибка API: {e}")
        return None

def test_calibration_page_load(driver):
    """Тест загрузки страницы калибровки"""
    print("\n🔍 ТЕСТ 1: Загрузка страницы калибровки")
    
    try:
        driver.get(CALIBRATION_URL)
        time.sleep(2)
        
        # Проверяем заголовок
        title = driver.title
        print(f"   Заголовок страницы: {title}")
        
        # Проверяем наличие основных элементов
        elements_to_check = [
            "calibrationStatus",
            "ecExpected1", 
            "phExpected1",
            "tempReference",
            "humidityReference",
            "npkNitrogen"
        ]
        
        for element_id in elements_to_check:
            try:
                element = driver.find_element(By.ID, element_id)
                print(f"   ✅ {element_id}: найден")
            except NoSuchElementException:
                print(f"   ❌ {element_id}: НЕ НАЙДЕН")
                return False
        
        return True
        
    except Exception as e:
        print(f"   ❌ Ошибка загрузки страницы: {e}")
        return False

def test_initial_status_display(driver):
    """Тест отображения начального статуса"""
    print("\n🔍 ТЕСТ 2: Отображение начального статуса")
    
    try:
        # Ждем загрузки статуса
        wait = WebDriverWait(driver, 10)
        status_element = wait.until(
            EC.presence_of_element_located((By.ID, "calibrationStatus"))
        )
        
        # Проверяем, что статус загрузился
        status_text = status_element.text
        print(f"   Статус на странице: {status_text}")
        
        # Проверяем, что нет "Загрузка статуса калибровок..."
        if "Загрузка статуса калибровок" in status_text:
            print("   ❌ Статус не загрузился - показывает 'Загрузка...'")
            return False
        
        # Проверяем наличие статусов для каждого датчика
        sensors = ["EC", "pH", "Температура", "Влажность", "NPK"]
        for sensor in sensors:
            if sensor in status_text:
                print(f"   ✅ Статус {sensor}: найден")
            else:
                print(f"   ⚠️ Статус {sensor}: не найден")
        
        return True
        
    except TimeoutException:
        print("   ❌ Таймаут ожидания статуса")
        return False
    except Exception as e:
        print(f"   ❌ Ошибка проверки статуса: {e}")
        return False

def test_ec_calibration_button(driver):
    """Тест кнопки калибровки EC"""
    print("\n🔍 ТЕСТ 3: Кнопка калибровки EC")
    
    try:
        # Находим поля EC калибровки
        ec_expected1 = driver.find_element(By.ID, "ecExpected1")
        ec_expected2 = driver.find_element(By.ID, "ecExpected2")
        ec_measured1 = driver.find_element(By.ID, "ecMeasured1")
        ec_measured2 = driver.find_element(By.ID, "ecMeasured2")
        
        print(f"   ✅ Поля EC калибровки: найдены")
        
        # Заполняем поля калибровки
        ec_expected1.clear()
        ec_expected1.send_keys("1.0")  # Первая точка
        print("   ✅ Заполнено поле EC Expected 1: 1.0")
        
        ec_expected2.clear()
        ec_expected2.send_keys("2.0")  # Вторая точка
        print("   ✅ Заполнено поле EC Expected 2: 2.0")
        
        ec_measured1.clear()
        ec_measured1.send_keys("1.1")  # Измеренное значение 1
        print("   ✅ Заполнено поле EC Measured 1: 1.1")
        
        ec_measured2.clear()
        ec_measured2.send_keys("2.1")  # Измеренное значение 2
        print("   ✅ Заполнено поле EC Measured 2: 2.1")
        
        # Находим кнопку калибровки EC
        calibrate_button = driver.find_element(By.XPATH, "//button[contains(text(), 'Калибровать EC')]")
        print(f"   ✅ Кнопка калибровки EC: найдена")
        
        # Запоминаем текущий статус
        status_before = driver.find_element(By.ID, "calibrationStatus").text
        print(f"   Статус до калибровки: {status_before[:100]}...")
        
        # Нажимаем кнопку
        calibrate_button.click()
        print("   ✅ Кнопка нажата")
        
        # Ждем обновления статуса
        time.sleep(3)
        
        # Проверяем изменение статуса
        status_after = driver.find_element(By.ID, "calibrationStatus").text
        print(f"   Статус после калибровки: {status_after[:100]}...")
        
        if status_before != status_after:
            print("   ✅ Статус изменился после калибровки")
            return True
        else:
            print("   ❌ Статус НЕ изменился после калибровки")
            return False
            
    except Exception as e:
        print(f"   ❌ Ошибка тестирования EC калибровки: {e}")
        return False

def test_ph_calibration_button(driver):
    """Тест кнопки калибровки pH"""
    print("\n🔍 ТЕСТ 4: Кнопка калибровки pH")
    
    try:
        # Находим поля pH калибровки
        ph_expected1 = driver.find_element(By.ID, "phExpected1")
        ph_expected2 = driver.find_element(By.ID, "phExpected2")
        ph_expected3 = driver.find_element(By.ID, "phExpected3")
        ph_measured1 = driver.find_element(By.ID, "phMeasured1")
        ph_measured2 = driver.find_element(By.ID, "phMeasured2")
        ph_measured3 = driver.find_element(By.ID, "phMeasured3")
        
        print(f"   ✅ Поля pH калибровки: найдены")
        
        # Заполняем поля калибровки (3 точки)
        ph_expected1.clear()
        ph_expected1.send_keys("4.0")  # Первая точка
        print("   ✅ Заполнено поле pH Expected 1: 4.0")
        
        ph_expected2.clear()
        ph_expected2.send_keys("7.0")  # Вторая точка
        print("   ✅ Заполнено поле pH Expected 2: 7.0")
        
        ph_expected3.clear()
        ph_expected3.send_keys("10.0")  # Третья точка
        print("   ✅ Заполнено поле pH Expected 3: 10.0")
        
        ph_measured1.clear()
        ph_measured1.send_keys("4.1")  # Измеренное значение 1
        print("   ✅ Заполнено поле pH Measured 1: 4.1")
        
        ph_measured2.clear()
        ph_measured2.send_keys("7.1")  # Измеренное значение 2
        print("   ✅ Заполнено поле pH Measured 2: 7.1")
        
        ph_measured3.clear()
        ph_measured3.send_keys("10.1")  # Измеренное значение 3
        print("   ✅ Заполнено поле pH Measured 3: 10.1")
        
        # Находим кнопку калибровки pH
        calibrate_button = driver.find_element(By.XPATH, "//button[contains(text(), 'Калибровать pH')]")
        print(f"   ✅ Кнопка калибровки pH: найдена")
        
        # Запоминаем текущий статус
        status_before = driver.find_element(By.ID, "calibrationStatus").text
        print(f"   Статус до калибровки: {status_before[:100]}...")
        
        # Нажимаем кнопку
        calibrate_button.click()
        print("   ✅ Кнопка нажата")
        
        # Ждем обновления статуса
        time.sleep(3)
        
        # Проверяем изменение статуса
        status_after = driver.find_element(By.ID, "calibrationStatus").text
        print(f"   Статус после калибровки: {status_after[:100]}...")
        
        if status_before != status_after:
            print("   ✅ Статус изменился после калибровки pH")
            return True
        else:
            print("   ❌ Статус НЕ изменился после калибровки pH")
            return False
            
    except Exception as e:
        print(f"   ❌ Ошибка тестирования pH калибровки: {e}")
        return False

def test_enable_disable_calibration(driver):
    """Тест включения/выключения калибровки"""
    print("\n🔍 ТЕСТ 5: Включение/выключение калибровки")
    
    try:
        # Находим кнопки включения/выключения
        enable_button = driver.find_element(By.XPATH, "//button[contains(text(), 'Включить калибровку')]")
        disable_button = driver.find_element(By.XPATH, "//button[contains(text(), 'Отключить калибровку')]")
        
        print("   ✅ Кнопки включения/выключения найдены")
        
        # Запоминаем начальный статус
        status_before = driver.find_element(By.ID, "calibrationStatus").text
        
        # Включаем калибровку
        enable_button.click()
        print("   ✅ Кнопка 'Включить калибровку' нажата")
        time.sleep(2)
        
        # Проверяем изменение статуса
        status_after_enable = driver.find_element(By.ID, "calibrationStatus").text
        if status_before != status_after_enable:
            print("   ✅ Статус изменился после включения")
        else:
            print("   ⚠️ Статус не изменился после включения")
        
        # Выключаем калибровку
        disable_button.click()
        print("   ✅ Кнопка 'Отключить калибровку' нажата")
        time.sleep(2)
        
        # Проверяем изменение статуса
        status_after_disable = driver.find_element(By.ID, "calibrationStatus").text
        if status_after_enable != status_after_disable:
            print("   ✅ Статус изменился после выключения")
            return True
        else:
            print("   ⚠️ Статус не изменился после выключения")
            return False
            
    except Exception as e:
        print(f"   ❌ Ошибка тестирования включения/выключения: {e}")
        return False

def main():
    """Основная функция тестирования"""
    print("🚀 E2E ТЕСТ ВЕБ-ИНТЕРФЕЙСА КАЛИБРОВКИ")
    print("=" * 50)
    
    # Проверяем API
    print("📡 Проверка API...")
    api_status = check_api_status()
    if not api_status:
        print("❌ API недоступен, тест прерван")
        return
    
    # Настраиваем драйвер
    driver = setup_driver()
    if not driver:
        print("❌ Не удалось создать драйвер браузера")
        return
    
    try:
        # Запускаем тесты
        tests = [
            test_calibration_page_load,
            test_initial_status_display,
            test_ec_calibration_button,
            test_ph_calibration_button,
            test_enable_disable_calibration
        ]
        
        passed = 0
        total = len(tests)
        
        for test in tests:
            if test(driver):
                passed += 1
        
        # Результаты
        print("\n" + "=" * 50)
        print(f"📊 РЕЗУЛЬТАТЫ E2E ТЕСТИРОВАНИЯ")
        print(f"   Пройдено: {passed}/{total}")
        print(f"   Успешность: {passed/total*100:.1f}%")
        
        if passed == total:
            print("   🎉 ВСЕ ТЕСТЫ ПРОЙДЕНЫ!")
        else:
            print("   ⚠️ ЕСТЬ ПРОБЛЕМЫ С ВЕБ-ИНТЕРФЕЙСОМ")
            
    finally:
        driver.quit()

if __name__ == "__main__":
    main()
