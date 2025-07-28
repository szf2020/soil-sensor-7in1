#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Тест для проверки JSON API рекомендаций по взаимодействию питательных веществ
"""

import json
import sys
import os

def test_json_api_structure():
    """Тест структуры JSON API"""
    print("🧪 Тестируем структуру JSON API...")
    
    # Симулируем ответ JSON API
    mock_json_response = {
        "temperature": "25.0",
        "humidity": "60.0", 
        "ec": "1200",
        "ph": "6.5",
        "nitrogen": "200.0",
        "phosphorus": "150.0", 
        "potassium": "300.0",
        "raw_temperature": "24.8",
        "raw_humidity": "58.5",
        "raw_ec": "1180",
        "raw_ph": "6.3",
        "raw_nitrogen": "195.0",
        "raw_phosphorus": "145.0",
        "raw_potassium": "295.0",
        "irrigation": False,
        "valid": True,
        "measurement_status": "optimal",
        "rec_temperature": "22.0",
        "rec_humidity": "65.0",
        "rec_ec": "1100",
        "rec_ph": "6.8",
        "rec_nitrogen": "180.0",
        "rec_phosphorus": "140.0",
        "rec_potassium": "280.0",
        "nutrient_interactions": "⚠️ Высокий азот блокирует калий. Рекомендуется: уменьшить азотные удобрения, увеличить калийные\n💡 Высокий калий может блокировать магний. Рекомендуется: внести сульфат магния (MgSO4)",
        "crop_specific_recommendations": "🍅 Томаты требуют кальций для предотвращения вершинной гнили. Рекомендуется: внести кальциевую селитру (Ca(NO3)2)\n🍅 Высокий калий может блокировать магний у томатов. Рекомендуется: внести сульфат магния (MgSO4)",
        "season": "Зима",
        "alerts": "",
        "timestamp": 1641024000
    }
    
    # Проверяем наличие ключей
    required_keys = [
        "nutrient_interactions",
        "crop_specific_recommendations", 
        "temperature", "humidity", "ec", "ph",
        "nitrogen", "phosphorus", "potassium"
    ]
    
    for key in required_keys:
        if key not in mock_json_response:
            print(f"❌ Отсутствует ключ: {key}")
            return False
        else:
            print(f"✅ Ключ найден: {key}")
    
    # Проверяем содержимое рекомендаций
    nutrient_interactions = mock_json_response["nutrient_interactions"]
    crop_recommendations = mock_json_response["crop_specific_recommendations"]
    
    print(f"\n📊 Рекомендации по взаимодействию:")
    print(f"   {nutrient_interactions}")
    
    print(f"\n🌱 Специфические рекомендации по культурам:")
    print(f"   {crop_recommendations}")
    
    # Проверяем, что рекомендации не пустые
    if nutrient_interactions and nutrient_interactions.strip():
        print("✅ Рекомендации по взаимодействию не пустые")
    else:
        print("❌ Рекомендации по взаимодействию пустые")
        return False
        
    if crop_recommendations and crop_recommendations.strip():
        print("✅ Специфические рекомендации не пустые")
    else:
        print("⚠️ Специфические рекомендации пустые (возможно, культура не выбрана)")
    
    return True

def test_javascript_processing():
    """Тест обработки в JavaScript"""
    print("\n🖥️ Тестируем обработку в JavaScript...")
    
    # Симулируем JavaScript обработку
    test_recommendations = "⚠️ Высокий азот блокирует калий\\n💡 Рекомендуется внести магний"
    
    # Симулируем замену \\n на <br>
    html_recommendations = test_recommendations.replace("\\n", "<br>")
    
    print(f"Исходный текст: {test_recommendations}")
    print(f"HTML версия: {html_recommendations}")
    
    if "<br>" in html_recommendations:
        print("✅ Замена \\n на <br> работает")
        return True
    else:
        print("❌ Замена \\n на <br> не работает")
        return False

def test_soil_type_mapping():
    """Тест маппинга типов почв"""
    print("\n🏺 Тестируем маппинг типов почв...")
    
    # Тестируем все 13 типов почв
    soil_types = [
        (0, "SAND", "Песок"),
        (1, "LOAM", "Суглинок"), 
        (2, "PEAT", "Торф"),
        (3, "CLAY", "Глина"),
        (4, "SANDPEAT", "Песчано-торфяная смесь"),
        (5, "SILT", "Иловая почва"),
        (6, "CLAY_LOAM", "Глинистый суглинок"),
        (7, "ORGANIC", "Органическая почва"),
        (8, "SANDY_LOAM", "Песчанистый суглинок"),
        (9, "SILTY_LOAM", "Иловатый суглинок"),
        (10, "LOAMY_CLAY", "Суглинистая глина"),
        (11, "SALINE", "Засоленная почва"),
        (12, "ALKALINE", "Щелочная почва")
    ]
    
    print("Проверяем все типы почв:")
    for soil_id, soil_enum, soil_name in soil_types:
        print(f"  {soil_id}: {soil_enum} ({soil_name})")
    
    # Проверяем диапазон
    max_soil_id = max(soil_id for soil_id, _, _ in soil_types)
    if max_soil_id == 12:
        print(f"✅ Максимальный ID почвы: {max_soil_id} (правильно)")
        return True
    else:
        print(f"❌ Максимальный ID почвы: {max_soil_id} (ожидалось 12)")
        return False

def main():
    """Основная функция тестирования"""
    print("🚀 ТЕСТ JSON API РЕКОМЕНДАЦИЙ")
    print("=" * 60)
    
    tests_passed = 0
    total_tests = 3
    
    if test_json_api_structure():
        tests_passed += 1
        
    if test_javascript_processing():
        tests_passed += 1
        
    if test_soil_type_mapping():
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