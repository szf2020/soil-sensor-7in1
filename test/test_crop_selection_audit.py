#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
🔍 СИСТЕМНЫЙ АУДИТ: Выбор культуры в JXCT
Проверяет всю цепочку: веб-форма → сохранение → передача → обработка
"""

import re
import os
import sys

def test_web_form_crop_options():
    """Тест 1: Проверка опций культур в веб-форме"""
    print("🌱 ТЕСТ 1: Опции культур в веб-форме")
    
    routes_main_path = "src/web/routes_main.cpp"
    assert os.path.exists(routes_main_path), "Файл routes_main.cpp не найден"
    
    with open(routes_main_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Ищем все опции культур
    crop_options = re.findall(r'<option value=\'([^\']+)\'[^>]*>([^<]+)</option>', content)
    
    print(f"📋 Найдено опций культур: {len(crop_options)}")
    for value, name in crop_options:
        print(f"   {value} → {name}")
    
    # Проверяем что есть основные культуры
    crop_values = [value for value, name in crop_options]
    required_crops = ['tomato', 'cucumber', 'pepper', 'lettuce', 'blueberry']
    
    missing_crops = [crop for crop in required_crops if crop not in crop_values]
    assert not missing_crops, f"Отсутствуют культуры: {missing_crops}"
    
    print("✅ Все основные культуры присутствуют в веб-форме")

def test_crop_form_processing():
    """Тест 2: Проверка обработки формы с культурой"""
    print("\n📝 ТЕСТ 2: Обработка POST запроса с культурой")
    
    routes_main_path = "src/web/routes_main.cpp"
    with open(routes_main_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Ищем обработку параметра crop
    crop_processing = re.search(r'if \(webServer\.hasArg\("crop"\)\)\s*\{([^}]+)\}', content, re.DOTALL)
    
    assert crop_processing is not None, "Обработка параметра 'crop' не найдена"
    
    processing_code = crop_processing.group(1).strip()
    print(f"📋 Найдена обработка: {processing_code}")
    
    # Проверяем что используется strlcpy для безопасного копирования
    assert 'strlcpy(config.cropId' in processing_code, "Не используется strlcpy для config.cropId"
    
    # Проверяем что вызывается saveConfig()
    assert 'saveConfig()' in content, "saveConfig() не вызывается после обработки формы"
    
    print("✅ Обработка формы корректна")

def test_config_persistence():
    """Тест 3: Проверка сохранения/загрузки cropId"""
    print("\n💾 ТЕСТ 3: Сохранение/загрузка cropId в конфигурации")
    
    config_path = "src/config.cpp"
    with open(config_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Проверяем загрузку cropId
    load_pattern = r'preferences\.getString\("cropId",\s*config\.cropId,\s*sizeof\(config\.cropId\)\)'
    assert re.search(load_pattern, content), "Загрузка cropId из preferences не найдена"
    
    # Проверяем сохранение cropId
    save_pattern = r'preferences\.putString\("cropId",\s*config\.cropId\)'
    assert re.search(save_pattern, content), "Сохранение cropId в preferences не найдено"
    
    print("✅ Сохранение/загрузка cropId реализованы корректно")

def test_json_api_crop_handling():
    """Тест 4: Проверка передачи cropId в JSON API"""
    print("\n🔗 ТЕСТ 4: Передача cropId в JSON API")
    
    routes_data_path = "src/web/routes_data.cpp"
    with open(routes_data_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Проверяем логирование cropId
    assert 'logDebugSafe("JSON API: soilProfile=%d, soilType=%d, cropId=\'%s\' (len=%d)"' in content, "Логирование cropId в JSON API не найдено"
    
    # Проверяем условие проверки cropId
    crop_check_pattern = r'if \(strlen\(config\.cropId\) > 0 && strcmp\(config\.cropId, "none"\) != 0\)'
    assert re.search(crop_check_pattern, content), "Проверка cropId в JSON API не найдена"
    
    # Проверяем вызов generateCropSpecificRecommendations
    assert 'getCropEngine().generateCropSpecificRecommendations(' in content, "Вызов generateCropSpecificRecommendations не найден"
    
    print("✅ JSON API корректно обрабатывает cropId")

def test_crop_engine_integration():
    """Тест 5: Проверка интеграции с CropRecommendationEngine"""
    print("\n🚀 ТЕСТ 5: Интеграция с CropRecommendationEngine")
    
    crop_engine_path = "src/business/crop_recommendation_engine.cpp"
    with open(crop_engine_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Проверяем функцию generateCropSpecificRecommendations
    assert 'String CropRecommendationEngine::generateCropSpecificRecommendations(' in content, "Функция generateCropSpecificRecommendations не найдена"
    
    # Проверяем поддержку основных культур
    supported_crops = ['tomato', 'cucumber', 'pepper', 'lettuce', 'blueberry']
    missing_support = []
    
    for crop in supported_crops:
        if f'cropName == "{crop}"' not in content:
            missing_support.append(crop)
    
    assert not missing_support, f"Отсутствует поддержка культур: {missing_support}"
    
    print("✅ CropRecommendationEngine поддерживает все основные культуры")

def test_config_structure():
    """Тест 6: Проверка структуры конфигурации"""
    print("\n📊 ТЕСТ 6: Структура конфигурации")
    
    # Проверяем определение cropId в структуре
    config_files = ["include/jxct_config_vars.h", "src/config.cpp"]
    
    cropId_found = False
    for config_file in config_files:
        if os.path.exists(config_file):
            with open(config_file, 'r', encoding='utf-8') as f:
                content = f.read()
                if 'cropId' in content:
                    print(f"✅ cropId найден в {config_file}")
                    cropId_found = True
                    
                    # Проверяем размер буфера
                    size_match = re.search(r'cropId\[[^\]]+\]', content)
                    if size_match:
                        print(f"📋 Размер буфера: {size_match.group()}")
                    break
    
    assert cropId_found, "Определение cropId в структуре конфигурации не найдено"

def diagnose_potential_issues():
    """Диагностика потенциальных проблем"""
    print("\n🔍 ДИАГНОСТИКА ПОТЕНЦИАЛЬНЫХ ПРОБЛЕМ:")
    
    issues = []
    
    # Проверяем инициализацию cropId
    config_path = "src/config.cpp"
    with open(config_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Ищем инициализацию по умолчанию
    if 'strlcpy(config.cropId, "", sizeof(config.cropId))' in content:
        print("⚠️  cropId инициализируется пустой строкой при сбросе")
        issues.append("empty_init")
    
    # Проверяем размер буфера
    if 'sizeof(config.cropId)' in content:
        print("✅ Используется sizeof для безопасности буфера")
    else:
        print("⚠️  Возможны проблемы с размером буфера cropId")
        issues.append("buffer_size")
    
    # Проверяем strcmp с "none"
    routes_data_path = "src/web/routes_data.cpp"
    with open(routes_data_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    if 'strcmp(config.cropId, "none") != 0' in content:
        print('✅ Корректная проверка на "none"')
    else:
        print('⚠️  Возможны проблемы с проверкой "none"')
        issues.append("none_check")
    
    return issues

def main():
    """Основная функция аудита"""
    print("🔍 СИСТЕМНЫЙ АУДИТ: Выбор культуры в JXCT")
    print("=" * 50)
    
    tests = [
        test_web_form_crop_options,
        test_crop_form_processing,
        test_config_persistence,
        test_json_api_crop_handling,
        test_crop_engine_integration,
        test_config_structure
    ]
    
    passed = 0
    failed = 0
    
    for test in tests:
        try:
            if test():
                passed += 1
            else:
                failed += 1
        except Exception as e:
            print(f"❌ Ошибка в тесте {test.__name__}: {e}")
            failed += 1
    
    print("\n" + "=" * 50)
    print(f"📊 РЕЗУЛЬТАТЫ АУДИТА:")
    print(f"✅ Пройдено тестов: {passed}")
    print(f"❌ Провалено тестов: {failed}")
    
    # Диагностика проблем
    issues = diagnose_potential_issues()
    
    if failed > 0 or issues:
        print(f"\n⚠️  ОБНАРУЖЕНЫ ПРОБЛЕМЫ!")
        if issues:
            print(f"🔧 Потенциальные проблемы: {', '.join(issues)}")
        return 1
    else:
        print(f"\n🎉 ВСЕ ТЕСТЫ ПРОЙДЕНЫ!")
        return 0

if __name__ == "__main__":
    sys.exit(main()) 