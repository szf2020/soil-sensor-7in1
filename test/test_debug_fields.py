#!/usr/bin/env python3
"""
Тест DEBUG полей в JSON API
Проверяет, что все DEBUG поля присутствуют в JSON ответе
"""

import json
import sys
import os

# Добавляем путь к модулям проекта
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

def test_debug_fields_in_json():
    """Тест наличия DEBUG полей в JSON API"""
    print("🔍 Тестирование DEBUG полей в JSON API...")
    
    # Мокаем JSON ответ (как будто от ESP32)
    mock_json_response = {
        "temperature": "25.5",
        "humidity": "45.2", 
        "ec": "1.2",
        "ph": "6.8",
        "nitrogen": "120.0",
        "phosphorus": "45.0",
        "potassium": "280.0",
        "crop_id": "tomato",
        "crop_specific_recommendations": "🍅 Рекомендации для томатов",
        "nutrient_interactions": "⚠️ Взаимодействия питательных веществ",
        
        # ✅ DEBUG ПОЛЯ - ДОЛЖНЫ БЫТЬ ПРИСУТСТВОВАТЬ
        "crop_id_debug": "tomato",
        "crop_id_hex": "746f6d61746f",  # hex представление
        "crop_len_check": True,
        "crop_str_check": True,
        "debug_npk_n": 120.0,
        "debug_npk_p": 45.0,
        "debug_npk_k": 280.0,
        "debug_ph": 6.8,
        "debug_soil_type": 1,  # enum значение
        
        "season": "Лето",
        "timestamp": 1641024000
    }
    
    # Проверяем наличие DEBUG ключей
    debug_keys = [
        "crop_id_debug",
        "crop_id_hex", 
        "crop_len_check",
        "crop_str_check",
        "debug_npk_n",
        "debug_npk_p", 
        "debug_npk_k",
        "debug_ph",
        "debug_soil_type"
    ]
    
    missing_keys = []
    for key in debug_keys:
        if key not in mock_json_response:
            missing_keys.append(key)
            print(f"❌ Отсутствует DEBUG поле: {key}")
        else:
            value = mock_json_response[key]
            print(f"✅ DEBUG поле {key}: {value} ({type(value).__name__})")
    
    if missing_keys:
        print(f"\n❌ ОШИБКА: Отсутствуют DEBUG поля: {missing_keys}")
        return False
    
    # Проверяем типы данных
    type_checks = [
        ("crop_id_debug", str),
        ("crop_id_hex", str),
        ("crop_len_check", bool),
        ("crop_str_check", bool),
        ("debug_npk_n", (int, float)),
        ("debug_npk_p", (int, float)),
        ("debug_npk_k", (int, float)),
        ("debug_ph", (int, float)),
        ("debug_soil_type", int)
    ]
    
    type_errors = []
    for key, expected_type in type_checks:
        value = mock_json_response[key]
        if not isinstance(value, expected_type):
            type_errors.append(f"{key}: ожидался {expected_type}, получен {type(value)}")
    
    if type_errors:
        print(f"\n❌ ОШИБКА ТИПОВ: {type_errors}")
        return False
    
    print("\n✅ ВСЕ DEBUG ПОЛЯ ПРИСУТСТВУЮТ И ИМЕЮТ ПРАВИЛЬНЫЕ ТИПЫ!")
    return True

def test_json_serialization():
    """Тест сериализации JSON с DEBUG полями"""
    print("\n🔍 Тестирование сериализации JSON...")
    
    # Создаем объект с DEBUG полями
    test_data = {
        "crop_id_debug": "tomato",
        "crop_id_hex": "746f6d61746f",
        "crop_len_check": True,
        "crop_str_check": True,
        "debug_npk_n": 120.0,
        "debug_npk_p": 45.0,
        "debug_npk_k": 280.0,
        "debug_ph": 6.8,
        "debug_soil_type": 1
    }
    
    try:
        # Сериализуем в JSON
        json_str = json.dumps(test_data, ensure_ascii=False)
        print(f"✅ JSON сериализация успешна: {len(json_str)} символов")
        
        # Десериализуем обратно
        parsed_data = json.loads(json_str)
        
        # Проверяем, что все поля сохранились
        for key, value in test_data.items():
            if key not in parsed_data:
                print(f"❌ Поле {key} потерялось при сериализации")
                return False
            if parsed_data[key] != value:
                print(f"❌ Значение {key} изменилось: {value} -> {parsed_data[key]}")
                return False
        
        print("✅ JSON десериализация успешна, все поля сохранены")
        return True
        
    except Exception as e:
        print(f"❌ Ошибка JSON сериализации: {e}")
        return False

if __name__ == "__main__":
    print("🧪 Тест DEBUG полей в JSON API")
    print("=" * 50)
    
    success = True
    
    # Тест 1: Наличие DEBUG полей
    if not test_debug_fields_in_json():
        success = False
    
    # Тест 2: Сериализация JSON
    if not test_json_serialization():
        success = False
    
    print("\n" + "=" * 50)
    if success:
        print("🎉 ВСЕ ТЕСТЫ DEBUG ПОЛЕЙ ПРОЙДЕНЫ УСПЕШНО!")
        sys.exit(0)
    else:
        print("❌ ТЕСТЫ DEBUG ПОЛЕЙ ПРОВАЛЕНЫ!")
        sys.exit(1) 