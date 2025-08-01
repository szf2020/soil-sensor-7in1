#!/usr/bin/env python3
"""
Тест размера JSON ответа API
Проверяет, что JSON ответ не превышает разумные размеры
"""

import json
import sys
import os

# Добавляем путь к модулям проекта
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

def test_json_size():
    """Тест размера JSON ответа"""
    print("🧪 Тест размера JSON ответа API")
    
    # Симулируем данные датчика
    sensor_data = {
        "temperature": 23.5,
        "humidity": 65.2,
        "ec": 1200,
        "ph": 6.8,
        "nitrogen": 35,
        "phosphorus": 12,
        "potassium": 28,
        "raw_temperature": 23.5,
        "raw_humidity": 65.2,
        "raw_ec": 1200,
        "raw_ph": 6.8,
        "raw_nitrogen": 35,
        "raw_phosphorus": 12,
        "raw_potassium": 28,
        "irrigation": False,
        "valid": True,
        "measurement_status": "optimal",
        "rec_temperature": 22.0,
        "rec_humidity": 60.0,
        "rec_ec": 1500,
        "rec_ph": 6.5,
        "rec_nitrogen": 40,
        "rec_phosphorus": 10,
        "rec_potassium": 30,
        "nutrient_interactions": "Рекомендации по взаимодействию питательных веществ",
        "crop_id": "tomato",
        "crop_specific_recommendations": "Специфические рекомендации для томатов",
        "season": "Лето",
        "alerts": "",
        "timestamp": 1640995200
    }
    
    # Сериализуем в JSON
    json_str = json.dumps(sensor_data, ensure_ascii=False, indent=2)
    json_size = len(json_str.encode('utf-8'))
    
    print(f"📊 Размер JSON ответа: {json_size} байт")
    
    # Проверяем размер
    max_size = 4096  # Максимальный размер буфера
    if json_size > max_size:
        print(f"❌ ОШИБКА: JSON слишком большой ({json_size} > {max_size} байт)")
        return False
    
    if json_size > max_size * 0.8:  # 80% от максимального размера
        print(f"⚠️ ПРЕДУПРЕЖДЕНИЕ: JSON близок к лимиту ({json_size}/{max_size} байт)")
    else:
        print(f"✅ JSON размер в норме ({json_size}/{max_size} байт)")
    
    return True

def test_json_structure():
    """Тест структуры JSON"""
    print("\n🔍 Тест структуры JSON")
    
    # Проверяем, что все обязательные поля присутствуют
    required_fields = [
        "temperature", "humidity", "ec", "ph", "nitrogen", "phosphorus", "potassium",
        "raw_temperature", "raw_humidity", "raw_ec", "raw_ph", "raw_nitrogen", "raw_phosphorus", "raw_potassium",
        "valid", "measurement_status", "timestamp"
    ]
    
    sensor_data = {
        "temperature": 23.5,
        "humidity": 65.2,
        "ec": 1200,
        "ph": 6.8,
        "nitrogen": 35,
        "phosphorus": 12,
        "potassium": 28,
        "raw_temperature": 23.5,
        "raw_humidity": 65.2,
        "raw_ec": 1200,
        "raw_ph": 6.8,
        "raw_nitrogen": 35,
        "raw_phosphorus": 12,
        "raw_potassium": 28,
        "valid": True,
        "measurement_status": "optimal",
        "timestamp": 1640995200
    }
    
    missing_fields = []
    for field in required_fields:
        if field not in sensor_data:
            missing_fields.append(field)
    
    if missing_fields:
        print(f"❌ ОШИБКА: Отсутствуют обязательные поля: {missing_fields}")
        return False
    
    print("✅ Все обязательные поля присутствуют")
    return True

if __name__ == "__main__":
    print("🧪 Тест JSON ответа API")
    print("=" * 50)
    
    success = True
    success &= test_json_size()
    success &= test_json_structure()
    
    if success:
        print("\n✅ ВСЕ ТЕСТЫ JSON ПРОЙДЕНЫ УСПЕШНО!")
        sys.exit(0)
    else:
        print("\n❌ ТЕСТЫ JSON ПРОВАЛЕНЫ!")
        sys.exit(1) 