#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Тест для отладки проблемы с конфигурацией культуры
"""

import sys

def test_crop_selection_html():
    """Тест HTML формы выбора культуры"""
    print("🌱 Тестируем HTML форму выбора культуры...")
    
    try:
        with open('src/web/routes_main.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем наличие голубики в HTML
        if "value='blueberry'" in content and ">Голубика</option>" in content:
            print("✅ Голубика найдена в HTML форме")
        else:
            print("❌ Голубика НЕ найдена в HTML форме")
            return False
            
        # Проверяем логику сохранения
        if 'strlcpy(config.cropId, webServer.arg("crop").c_str(), sizeof(config.cropId))' in content:
            print("✅ Логика сохранения cropId найдена")
        else:
            print("❌ Логика сохранения cropId НЕ найдена")
            return False
            
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_crop_engine_support():
    """Тест поддержки голубики в CropRecommendationEngine"""
    print("\n🫐 Тестируем поддержку голубики в CropRecommendationEngine...")
    
    try:
        with open('src/business/crop_recommendation_engine.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем конфигурацию голубики
        if 'cropConfigs["blueberry"]' in content:
            print("✅ Конфигурация голубики найдена в CropEngine")
        else:
            print("❌ Конфигурация голубики НЕ найдена в CropEngine")
            return False
            
        # Проверяем поддержку в generateCropSpecificRecommendations
        if 'cropType == "blueberry"' in content:
            print("✅ Поддержка голубики в generateCropSpecificRecommendations найдена")
        else:
            print("❌ Поддержка голубики в generateCropSpecificRecommendations НЕ найдена")
            return False
            
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_json_api_logic():
    """Тест логики JSON API"""
    print("\n📡 Тестируем логику JSON API...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем условие проверки культуры
        if 'strlen(config.cropId) > 0 && strcmp(config.cropId, "none") != 0' in content:
            print("✅ Условие проверки культуры найдено")
        else:
            print("❌ Условие проверки культуры НЕ найдено")
            return False
            
        # Проверяем вызов generateCropSpecificRecommendations
        if 'getCropEngine().generateCropSpecificRecommendations(' in content:
            print("✅ Вызов generateCropSpecificRecommendations найден")
        else:
            print("❌ Вызов generateCropSpecificRecommendations НЕ найден")
            return False
            
        # Проверяем отладочные логи
        if 'logDebugSafe("JSON API: checking crop' in content:
            print("✅ Отладочные логи добавлены")
        else:
            print("❌ Отладочные логи НЕ добавлены")
            return False
            
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_config_structure():
    """Тест структуры конфигурации"""
    print("\n⚙️ Тестируем структуру конфигурации...")
    
    try:
        # Проверяем объявление cropId в конфигурации
        with open('include/jxct_config_vars.h', 'r', encoding='utf-8') as f:
            content = f.read()
            
        if 'cropId' in content:
            print("✅ Поле cropId найдено в конфигурации")
        else:
            print("❌ Поле cropId НЕ найдено в конфигурации")
            return False
            
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла конфигурации: {e}")
        return False

def analyze_problem():
    """Анализ возможных причин проблемы"""
    print("\n🔍 АНАЛИЗ ВОЗМОЖНЫХ ПРИЧИН ПРОБЛЕМЫ:")
    print("1. ❓ Конфигурация не сохраняется после отправки формы")
    print("2. ❓ Конфигурация сбрасывается при перезагрузке")
    print("3. ❓ Условие проверки культуры работает неправильно")
    print("4. ❓ generateCropSpecificRecommendations возвращает пустую строку")
    print("5. ❓ config.cropId содержит неожиданное значение")
    
    print("\n💡 РЕКОМЕНДАЦИИ ДЛЯ ОТЛАДКИ:")
    print("• Проверьте логи ESP32 с отладочной информацией")
    print("• Убедитесь что форма отправляется с правильными параметрами")
    print("• Проверьте что saveConfig() вызывается после установки cropId")
    print("• Проверьте что config загружается правильно при старте")

def main():
    """Основная функция тестирования"""
    print("🚀 ТЕСТ ОТЛАДКИ КОНФИГУРАЦИИ КУЛЬТУРЫ")
    print("=" * 60)
    
    tests_passed = 0
    total_tests = 4
    
    if test_crop_selection_html():
        tests_passed += 1
        
    if test_crop_engine_support():
        tests_passed += 1
        
    if test_json_api_logic():
        tests_passed += 1
        
    if test_config_structure():
        tests_passed += 1
    
    print("\n" + "=" * 60)
    print(f"📊 РЕЗУЛЬТАТЫ: {tests_passed}/{total_tests} тестов пройдено")
    
    if tests_passed == total_tests:
        print("✅ ВСЕ КОМПОНЕНТЫ НА МЕСТЕ!")
        print("\n🔧 ПРОБЛЕМА СКОРЕЕ ВСЕГО В ВЫПОЛНЕНИИ:")
        print("• Конфигурация не сохраняется или не загружается")
        print("• config.cropId содержит неправильное значение")
        print("• Проверьте отладочные логи ESP32")
    else:
        print("❌ НАЙДЕНЫ ПРОБЛЕМЫ В КОМПОНЕНТАХ!")
        
    analyze_problem()
    return tests_passed == total_tests

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1) 