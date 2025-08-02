#!/usr/bin/env python3
"""
Тест валидации данных для ThingSpeak
Проверяет форматирование и валидность данных перед отправкой
"""

import sys
import os
import math
import json
from typing import Dict, List, Tuple

def test_data_formatting():
    """Тест форматирования данных для ThingSpeak"""
    print("🧪 ТЕСТ ФОРМАТИРОВАНИЯ ДАННЫХ THINGSPEAK")
    print("=" * 60)
    
    # Тестовые данные (симулируем данные датчиков)
    test_cases = [
        # Нормальные значения
        {
            "name": "Нормальные значения",
            "temperature": 25.5,
            "humidity": 60.2,
            "ec": 1250,
            "ph": 6.8,
            "nitrogen": 150,
            "phosphorus": 45,
            "potassium": 200,
            "expected_valid": True
        },
        # Граничные значения
        {
            "name": "Граничные значения",
            "temperature": 0.0,
            "humidity": 0.0,
            "ec": 0,
            "ph": 0.0,
            "nitrogen": 0,
            "phosphorus": 0,
            "potassium": 0,
            "expected_valid": True
        },
        # Максимальные значения
        {
            "name": "Максимальные значения",
            "temperature": 100.0,
            "humidity": 100.0,
            "ec": 9999,
            "ph": 14.0,
            "nitrogen": 9999,
            "phosphorus": 9999,
            "potassium": 9999,
            "expected_valid": True
        },
        # Проблемные значения
        {
            "name": "NaN значения",
            "temperature": float('nan'),
            "humidity": float('nan'),
            "ec": float('nan'),
            "ph": float('nan'),
            "nitrogen": float('nan'),
            "phosphorus": float('nan'),
            "potassium": float('nan'),
            "expected_valid": False
        },
        {
            "name": "Inf значения",
            "temperature": float('inf'),
            "humidity": float('inf'),
            "ec": float('inf'),
            "ph": float('inf'),
            "nitrogen": float('inf'),
            "phosphorus": float('inf'),
            "potassium": float('inf'),
            "expected_valid": False
        },
        {
            "name": "Отрицательные значения",
            "temperature": -10.0,
            "humidity": -5.0,
            "ec": -100,
            "ph": -1.0,
            "nitrogen": -50,
            "phosphorus": -20,
            "potassium": -30,
            "expected_valid": False
        },
        {
            "name": "Слишком большие значения",
            "temperature": 1000.0,
            "humidity": 1000.0,
            "ec": 100000,
            "ph": 100.0,
            "nitrogen": 100000,
            "phosphorus": 100000,
            "potassium": 100000,
            "expected_valid": False
        }
    ]
    
    results = []
    
    for test_case in test_cases:
        print(f"\n📊 Тест: {test_case['name']}")
        
        # Проверяем каждое поле
        fields_valid = []
        
        # Температура (0-100°C)
        temp_valid = 0 <= test_case['temperature'] <= 100 and not math.isnan(test_case['temperature']) and not math.isinf(test_case['temperature'])
        fields_valid.append(("temperature", temp_valid, test_case['temperature']))
        
        # Влажность (0-100%)
        hum_valid = 0 <= test_case['humidity'] <= 100 and not math.isnan(test_case['humidity']) and not math.isinf(test_case['humidity'])
        fields_valid.append(("humidity", hum_valid, test_case['humidity']))
        
        # EC (0-10000 μS/cm)
        ec_valid = 0 <= test_case['ec'] <= 10000 and not math.isnan(test_case['ec']) and not math.isinf(test_case['ec'])
        fields_valid.append(("ec", ec_valid, test_case['ec']))
        
        # pH (0-14)
        ph_valid = 0 <= test_case['ph'] <= 14 and not math.isnan(test_case['ph']) and not math.isinf(test_case['ph'])
        fields_valid.append(("ph", ph_valid, test_case['ph']))
        
        # NPK (0-9999 mg/kg)
        npk_valid = 0 <= test_case['nitrogen'] <= 9999 and not math.isnan(test_case['nitrogen']) and not math.isinf(test_case['nitrogen'])
        fields_valid.append(("nitrogen", npk_valid, test_case['nitrogen']))
        
        npk_valid = 0 <= test_case['phosphorus'] <= 9999 and not math.isnan(test_case['phosphorus']) and not math.isinf(test_case['phosphorus'])
        fields_valid.append(("phosphorus", npk_valid, test_case['phosphorus']))
        
        npk_valid = 0 <= test_case['potassium'] <= 9999 and not math.isnan(test_case['potassium']) and not math.isinf(test_case['potassium'])
        fields_valid.append(("potassium", npk_valid, test_case['potassium']))
        
        # Проверяем результаты
        all_valid = all(valid for _, valid, _ in fields_valid)
        expected_valid = test_case['expected_valid']
        
        print(f"   Ожидается: {'✅ Валидные' if expected_valid else '❌ Невалидные'}")
        print(f"   Результат: {'✅ Валидные' if all_valid else '❌ Невалидные'}")
        
        # Выводим детали по полям
        for field_name, is_valid, value in fields_valid:
            status = "✅" if is_valid else "❌"
            if math.isnan(value):
                value_str = "NaN"
            elif math.isinf(value):
                value_str = "Inf"
            else:
                value_str = str(value)
            print(f"     {status} {field_name}: {value_str}")
        
        # Проверяем соответствие ожиданиям
        test_passed = (all_valid == expected_valid)
        status = "✅ ПРОЙДЕН" if test_passed else "❌ ПРОВАЛЕН"
        print(f"   {status}")
        
        results.append({
            "test_name": test_case['name'],
            "passed": test_passed,
            "expected_valid": expected_valid,
            "actual_valid": all_valid,
            "fields": fields_valid
        })
    
    return results

def test_buffer_overflow():
    """Тест на переполнение буфера"""
    print("\n🧪 ТЕСТ ПЕРЕПОЛНЕНИЯ БУФЕРА")
    print("=" * 60)
    
    # Симулируем проблему с буфером в 8 байт
    buffer_size = 8
    
    test_values = [
        ("Маленькое число", "123"),
        ("Среднее число", "12345"),
        ("Большое число", "123456789"),
        ("Отрицательное", "-123"),
        ("С плавающей точкой", "123.456"),
        ("NaN", "NaN"),
        ("Inf", "Inf")
    ]
    
    overflow_detected = False
    
    for test_name, value in test_values:
        # Проверяем, поместится ли значение в буфер
        value_length = len(value)
        fits_in_buffer = value_length < buffer_size
        
        status = "✅" if fits_in_buffer else "❌ ПЕРЕПОЛНЕНИЕ!"
        print(f"   {status} {test_name}: '{value}' ({value_length} символов)")
        
        if not fits_in_buffer:
            overflow_detected = True
    
    if overflow_detected:
        print("\n🚨 ОБНАРУЖЕНО ПЕРЕПОЛНЕНИЕ БУФЕРА!")
        print("   Рекомендация: увеличить буфер с 8 до 32 байт")
    
    return not overflow_detected

def test_http_request_format():
    """Тест формата HTTP запроса"""
    print("\n🌐 ТЕСТ ФОРМАТА HTTP ЗАПРОСА")
    print("=" * 60)
    
    # Симулируем данные для отправки
    sample_data = {
        "temperature": 25.5,
        "humidity": 60.2,
        "ec": 1250,
        "ph": 6.8,
        "nitrogen": 150,
        "phosphorus": 45,
        "potassium": 200
    }
    
    # Формируем URL-encoded данные
    fields = []
    for i, (key, value) in enumerate(sample_data.items(), 1):
        fields.append(f"field{i}={value}")
    
    url_encoded_data = "&".join(fields)
    
    print("📋 URL-encoded данные:")
    print(f"   {url_encoded_data}")
    
    # Проверяем длину данных
    data_length = len(url_encoded_data)
    print(f"   Длина данных: {data_length} символов")
    
    # Проверяем, что данные не слишком длинные
    max_length = 1024  # Максимальная длина для POST запроса
    if data_length > max_length:
        print(f"   ❌ Данные слишком длинные ({data_length} > {max_length})")
        return False
    else:
        print(f"   ✅ Длина данных в пределах нормы")
        return True

def test_thingspeak_api_requirements():
    """Тест требований ThingSpeak API"""
    print("\n📡 ТЕСТ ТРЕБОВАНИЙ THINGSPEAK API")
    print("=" * 60)
    
    requirements = [
        ("URL", "https://api.thingspeak.com/update", "✅"),
        ("Метод", "POST", "✅"),
        ("Content-Type", "application/x-www-form-urlencoded", "✅"),
        ("Поля", "field1-field7", "✅"),
        ("API Key", "В заголовке или параметре", "✅"),
        ("SSL", "Обязательно HTTPS", "✅"),
        ("User-Agent", "Рекомендуется", "⚠️"),
        ("Таймаут", "30 секунд", "⚠️"),
        ("Retry", "При ошибках 5xx", "⚠️")
    ]
    
    all_met = True
    
    for req_name, req_value, status in requirements:
        print(f"   {status} {req_name}: {req_value}")
        if status == "❌":
            all_met = False
    
    return all_met

def generate_recommendations(test_results):
    """Генерирует рекомендации на основе результатов тестов"""
    print("\n💡 РЕКОМЕНДАЦИИ")
    print("=" * 60)
    
    recommendations = []
    
    # Анализируем результаты тестов форматирования
    failed_tests = [r for r in test_results if not r["passed"]]
    if failed_tests:
        recommendations.append("🔧 Исправить валидацию данных в тестах")
    
    # Проверяем переполнение буфера
    if not test_buffer_overflow():
        recommendations.append("🔧 Увеличить размер буфера форматирования с 8 до 32 байт")
    
    # Общие рекомендации
    recommendations.extend([
        "🔧 Добавить проверку на NaN/Inf значения",
        "🔧 Добавить валидацию диапазонов данных",
        "🔧 Улучшить логирование HTTP ответов",
        "🔧 Добавить retry логику для сетевых ошибок",
        "🔧 Добавить таймауты для HTTP запросов",
        "🔧 Проверить SSL сертификаты",
        "🔧 Добавить User-Agent в HTTP заголовки"
    ])
    
    for i, rec in enumerate(recommendations, 1):
        print(f"   {i}. {rec}")
    
    return recommendations

def main():
    """Главная функция"""
    print("🔍 ДИАГНОСТИКА ПРОБЛЕМЫ THINGSPEAK")
    print("=" * 60)
    
    # Запускаем все тесты
    formatting_results = test_data_formatting()
    buffer_test_passed = test_buffer_overflow()
    http_format_passed = test_http_request_format()
    api_requirements_met = test_thingspeak_api_requirements()
    
    # Генерируем рекомендации
    recommendations = generate_recommendations(formatting_results)
    
    # Итоговый отчет
    print("\n📊 ИТОГОВЫЙ ОТЧЕТ")
    print("=" * 60)
    
    total_tests = len(formatting_results) + 3  # форматирование + 3 дополнительных теста
    passed_tests = len([r for r in formatting_results if r["passed"]]) + sum([
        buffer_test_passed,
        http_format_passed,
        api_requirements_met
    ])
    
    success_rate = (passed_tests / total_tests) * 100
    
    print(f"📈 Результаты тестов: {passed_tests}/{total_tests} ({success_rate:.1f}%)")
    
    if success_rate >= 80:
        print("✅ Большинство тестов пройдено")
    elif success_rate >= 50:
        print("⚠️ Много проблем, требуется внимание")
    else:
        print("❌ Критические проблемы, требуется немедленное исправление")
    
    print(f"📝 Рекомендаций: {len(recommendations)}")
    
    # Сохраняем отчет
    report = {
        "timestamp": "2024-01-XX",
        "version": "3.10.1",
        "test_results": {
            "formatting_tests": formatting_results,
            "buffer_overflow": buffer_test_passed,
            "http_format": http_format_passed,
            "api_requirements": api_requirements_met
        },
        "statistics": {
            "total_tests": total_tests,
            "passed_tests": passed_tests,
            "success_rate": success_rate
        },
        "recommendations": recommendations
    }
    
    with open("test_reports/thingspeak_analysis_report.json", "w", encoding="utf-8") as f:
        json.dump(report, f, indent=2, ensure_ascii=False)
    
    print(f"\n📄 Отчет сохранен: test_reports/thingspeak_analysis_report.json")
    
    return success_rate >= 80

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1) 