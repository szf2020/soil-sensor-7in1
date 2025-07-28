#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Тест для проверки эндпоинта /sensor_json
"""

import json
import sys
import subprocess
import re

def test_sensor_json_compilation():
    """Тест компиляции с эндпоинтом sensor_json"""
    print("🔧 Тестируем компиляцию с эндпоинтом /sensor_json...")
    
    try:
        # Проверяем сборку
        result = subprocess.run(['pio', 'run', '-e', 'esp32dev'], 
                              capture_output=True, text=True, timeout=120)
        
        if result.returncode == 0:
            print("✅ Проект собирается успешно")
            
            # Проверяем размер прошивки
            size_match = re.search(r'Flash: \[.*?\] +(\d+\.\d+)% \(used (\d+) bytes', result.stdout)
            if size_match:
                percentage = size_match.group(1)
                bytes_used = size_match.group(2)
                print(f"📊 Размер прошивки: {percentage}% ({bytes_used} байт)")
            
            return True
        else:
            print("❌ Ошибка компиляции:")
            print(result.stderr)
            return False
            
    except subprocess.TimeoutExpired:
        print("❌ Таймаут компиляции")
        return False
    except Exception as e:
        print(f"❌ Ошибка: {e}")
        return False

def test_sensor_json_route_registration():
    """Тест регистрации маршрута /sensor_json"""
    print("\n🛣️ Тестируем регистрацию маршрута /sensor_json...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем наличие регистрации маршрута
        if 'webServer.on("/sensor_json", HTTP_GET, sendSensorJson)' in content:
            print("✅ Маршрут /sensor_json зарегистрирован")
        else:
            print("❌ Маршрут /sensor_json НЕ зарегистрирован")
            return False
            
        # Проверяем наличие функции sendSensorJson
        if 'void sendSensorJson()' in content:
            print("✅ Функция sendSensorJson найдена")
        else:
            print("❌ Функция sendSensorJson НЕ найдена")
            return False
            
        # Проверяем наличие полей в JSON
        required_fields = [
            'nutrient_interactions',
            'crop_specific_recommendations'
        ]
        
        for field in required_fields:
            if f'doc["{field}"]' in content:
                print(f"✅ Поле {field} добавляется в JSON")
            else:
                print(f"❌ Поле {field} НЕ добавляется в JSON")
                return False
                
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_javascript_fetch():
    """Тест JavaScript кода для fetch"""
    print("\n🖥️ Тестируем JavaScript код для fetch...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем наличие fetch('/sensor_json')
        if "fetch('/sensor_json')" in content:
            print("✅ JavaScript fetch('/sensor_json') найден")
        else:
            print("❌ JavaScript fetch('/sensor_json') НЕ найден")
            return False
            
        # Проверяем наличие обработки рекомендаций
        if 'nutrient-interactions' in content:
            print("✅ Обработка nutrient-interactions найдена")
        else:
            print("❌ Обработка nutrient-interactions НЕ найдена")
            return False
            
        if 'crop-specific-recommendations' in content:
            print("✅ Обработка crop-specific-recommendations найдена")
        else:
            print("❌ Обработка crop-specific-recommendations НЕ найдена")
            return False
            
        # Проверяем наличие автоматического обновления
        if 'setInterval(updateSensor' in content:
            print("✅ Автоматическое обновление настроено")
        else:
            print("❌ Автоматическое обновление НЕ настроено")
            return False
            
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_business_service_integration():
    """Тест интеграции бизнес-сервисов"""
    print("\n🏢 Тестируем интеграцию бизнес-сервисов...")
    
    try:
        # Проверяем business_instances.cpp
        with open('src/business_instances.cpp', 'r', encoding='utf-8') as f:
            instances_content = f.read()
            
        if 'NutrientInteractionService gNutrientInteractionService' in instances_content:
            print("✅ Глобальный экземпляр NutrientInteractionService найден")
        else:
            print("❌ Глобальный экземпляр NutrientInteractionService НЕ найден")
            return False
            
        # Проверяем business_services.cpp
        with open('src/business_services.cpp', 'r', encoding='utf-8') as f:
            services_content = f.read()
            
        if 'getNutrientInteractionService()' in services_content:
            print("✅ Функция getNutrientInteractionService найдена")
        else:
            print("❌ Функция getNutrientInteractionService НЕ найдена")
            return False
            
        # Проверяем include/business_services.h
        with open('include/business_services.h', 'r', encoding='utf-8') as f:
            header_content = f.read()
            
        if 'extern INutrientInteractionService& getNutrientInteractionService()' in header_content:
            print("✅ Объявление getNutrientInteractionService найдено")
        else:
            print("❌ Объявление getNutrientInteractionService НЕ найдено")
            return False
            
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файлов: {e}")
        return False

def main():
    """Основная функция тестирования"""
    print("🚀 ТЕСТ ЭНДПОИНТА /sensor_json")
    print("=" * 60)
    
    tests_passed = 0
    total_tests = 4
    
    if test_sensor_json_compilation():
        tests_passed += 1
        
    if test_sensor_json_route_registration():
        tests_passed += 1
        
    if test_javascript_fetch():
        tests_passed += 1
        
    if test_business_service_integration():
        tests_passed += 1
    
    print("\n" + "=" * 60)
    print(f"📊 РЕЗУЛЬТАТЫ: {tests_passed}/{total_tests} тестов пройдено")
    
    if tests_passed == total_tests:
        print("🎉 ВСЕ ТЕСТЫ ПРОЙДЕНЫ!")
        return True
    else:
        print("❌ НЕКОТОРЫЕ ТЕСТЫ НЕ ПРОЙДЕНЫ!")
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1) 