#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
🍅 ТЕСТ ИСПРАВЛЕНИЯ: Рекомендации для томата
Проверяет что исправленные условия работают с реальными значениями датчика
"""

import re
import os

def test_tomato_conditions_fix():
    """Тест исправленных условий для томата"""
    print("🍅 ТЕСТ: Исправленные условия для томата")
    
    crop_engine_path = "src/business/crop_recommendation_engine.cpp"
    with open(crop_engine_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Находим секцию томата
    tomato_section = re.search(
        r'if \(cropName == "tomato".*?\}(?=\s*else if|\s*// 🌱)', 
        content, 
        re.DOTALL
    )
    
    if not tomato_section:
        print("❌ Секция томата не найдена")
        return False
    
    tomato_code = tomato_section.group(0)
    print("📋 Найдена секция томата:")
    print(tomato_code[:200] + "...")
    
    # Проверяем исправленные условия
    checks = {
        "pH < 6.5F": "Расширенный диапазон pH (было 6.0, стало 6.5)",
        "npk.potassium > 200.0F": "Снижен порог калия (было 350, стало 200)", 
        "npk.nitrogen > 150.0F": "Добавлено условие для азота",
        "npk.phosphorus < 100.0F": "Добавлено условие для фосфора"
    }
    
    results = {}
    for condition, description in checks.items():
        if condition in tomato_code:
            print(f"✅ {description}")
            results[condition] = True
        else:
            print(f"❌ Отсутствует: {description}")
            results[condition] = False
    
    return all(results.values())

def simulate_real_sensor_values():
    """Симуляция реальных значений датчика"""
    print("\n📊 СИМУЛЯЦИЯ: Реальные значения датчика")
    
    # Значения из логов ESP32
    real_values = {
        "pH": 6.3,
        "nitrogen": 84.0,
        "phosphorus": 118.8,
        "potassium": 233.4
    }
    
    print("📋 Значения из реальных логов ESP32:")
    for param, value in real_values.items():
        print(f"   {param}: {value}")
    
    # Проверяем какие условия должны сработать
    print("\n🔍 Анализ условий:")
    
    conditions = [
        ("pH < 6.5F", real_values["pH"] < 6.5, "Кальций для вершинной гнили"),
        ("K > 200.0F", real_values["potassium"] > 200.0, "Магний при высоком калии"),
        ("N > 150.0F && P < 100.0F", 
         real_values["nitrogen"] > 150.0 and real_values["phosphorus"] < 100.0, 
         "Сбалансированное питание")
    ]
    
    should_trigger = []
    for condition, result, description in conditions:
        status = "✅ СРАБОТАЕТ" if result else "❌ НЕ СРАБОТАЕТ"
        print(f"   {condition}: {status} ({description})")
        if result:
            should_trigger.append(description)
    
    if should_trigger:
        print(f"\n🎯 Должны сработать рекомендации: {', '.join(should_trigger)}")
        return True
    else:
        print(f"\n⚠️  НИ ОДНО УСЛОВИЕ НЕ СРАБОТАЕТ!")
        return False

def test_expected_recommendations():
    """Тест ожидаемых рекомендаций"""
    print("\n📝 ТЕСТ: Ожидаемые рекомендации для томата")
    
    crop_engine_path = "src/business/crop_recommendation_engine.cpp"
    with open(crop_engine_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Ищем все рекомендации для томата
    tomato_recommendations = re.findall(
        r'recommendations \+= "🍅[^"]+";', 
        content
    )
    
    print(f"📋 Найдено рекомендаций для томата: {len(tomato_recommendations)}")
    for i, rec in enumerate(tomato_recommendations, 1):
        # Извлекаем текст рекомендации
        rec_text = re.search(r'"🍅([^"]+)"', rec)
        if rec_text:
            print(f"   {i}. 🍅{rec_text.group(1)}")
    
    # Проверяем что есть основные рекомендации
    expected_keywords = ["кальций", "магний", "сбалансированном"]
    found_keywords = []
    
    for keyword in expected_keywords:
        if keyword in content:
            found_keywords.append(keyword)
    
    print(f"\n🔍 Найдены ключевые слова: {found_keywords}")
    
    if len(found_keywords) >= 2:
        print("✅ Рекомендации содержат основные элементы")
        return True
    else:
        print("❌ Недостаточно рекомендаций")
        return False

def main():
    """Основная функция теста"""
    print("🍅 ТЕСТ ИСПРАВЛЕНИЯ: Рекомендации для томата")
    print("=" * 50)
    
    tests = [
        test_tomato_conditions_fix,
        simulate_real_sensor_values,
        test_expected_recommendations
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
    print(f"📊 РЕЗУЛЬТАТЫ ТЕСТА:")
    print(f"✅ Пройдено: {passed}")
    print(f"❌ Провалено: {failed}")
    
    if failed == 0:
        print(f"\n🎉 ВСЕ ТЕСТЫ ПРОЙДЕНЫ!")
        print(f"🍅 Рекомендации для томата должны работать!")
        return 0
    else:
        print(f"\n⚠️  ЕСТЬ ПРОБЛЕМЫ!")
        return 1

if __name__ == "__main__":
    exit(main()) 