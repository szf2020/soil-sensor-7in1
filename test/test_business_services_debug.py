#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Тест для проверки работы бизнес-сервисов
"""

import sys
import subprocess
import re

def test_nutrient_interaction_service():
    """Тест NutrientInteractionService"""
    print("🧪 Тестируем NutrientInteractionService...")
    
    try:
        # Проверяем наличие файла
        with open('src/business/nutrient_interaction_service.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем наличие метода generateAntagonismRecommendations
        if 'generateAntagonismRecommendations' in content:
            print("✅ Метод generateAntagonismRecommendations найден")
        else:
            print("❌ Метод generateAntagonismRecommendations НЕ найден")
            return False
            
        # Проверяем наличие метода applyNutrientInteractions
        if 'applyNutrientInteractions' in content:
            print("✅ Метод applyNutrientInteractions найден")
        else:
            print("❌ Метод applyNutrientInteractions НЕ найден")
            return False
            
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_crop_recommendation_engine():
    """Тест CropRecommendationEngine"""
    print("\n🌱 Тестируем CropRecommendationEngine...")
    
    try:
        # Проверяем наличие файла
        with open('src/business/crop_recommendation_engine.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем наличие метода generateCropSpecificRecommendations
        if 'generateCropSpecificRecommendations' in content:
            print("✅ Метод generateCropSpecificRecommendations найден")
        else:
            print("❌ Метод generateCropSpecificRecommendations НЕ найден")
            return False
            
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_service_integration():
    """Тест интеграции сервисов"""
    print("\n🔗 Тестируем интеграцию сервисов...")
    
    try:
        # Проверяем business_instances.cpp
        with open('src/business_instances.cpp', 'r', encoding='utf-8') as f:
            instances_content = f.read()
            
        if 'NutrientInteractionService gNutrientInteractionService' in instances_content:
            print("✅ Глобальный экземпляр NutrientInteractionService создан")
        else:
            print("❌ Глобальный экземпляр NutrientInteractionService НЕ создан")
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

def test_compilation_with_services():
    """Тест компиляции с сервисами"""
    print("\n🔧 Тестируем компиляцию с сервисами...")
    
    try:
        # Проверяем сборку
        result = subprocess.run(['pio', 'run', '-e', 'esp32dev'], 
                              capture_output=True, text=True, timeout=120)
        
        if result.returncode == 0:
            print("✅ Проект собирается с сервисами")
            
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

def main():
    """Основная функция тестирования"""
    print("🚀 ТЕСТ БИЗНЕС-СЕРВИСОВ")
    print("=" * 60)
    
    tests_passed = 0
    total_tests = 4
    
    if test_nutrient_interaction_service():
        tests_passed += 1
        
    if test_crop_recommendation_engine():
        tests_passed += 1
        
    if test_service_integration():
        tests_passed += 1
        
    if test_compilation_with_services():
        tests_passed += 1
    
    print("\n" + "=" * 60)
    print(f"📊 РЕЗУЛЬТАТЫ: {tests_passed}/{total_tests} тестов пройдено")
    
    if tests_passed == total_tests:
        print("🎉 ВСЕ ТЕСТЫ ПРОЙДЕНЫ!")
        print("\n💡 ВОЗМОЖНЫЕ ПРИЧИНЫ ПРОБЛЕМЫ:")
        print("1. JavaScript не выполняется из-за синтаксических ошибок")
        print("2. HTML элементы не создаются правильно")
        print("3. Данные не передаются в JSON")
        print("4. Браузер блокирует выполнение JavaScript")
        return True
    else:
        print("❌ НЕКОТОРЫЕ ТЕСТЫ НЕ ПРОЙДЕНЫ!")
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1) 