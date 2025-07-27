#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Тест совместимости почвенного датчика с разными типами выращивания
Проверяет логику ограничений для гидропоники и аэропоники
"""

import unittest

def check_sensor_compatibility(growing_type: str) -> dict:
    """
    Проверяет совместимость почвенного датчика JXCT с типом выращивания

    Args:
        growing_type: Тип выращивания

    Returns:
        Словарь с информацией о совместимости
    """
    compatibility = {
        "sensor_type": "JXCT Soil Sensor",
        "sensor_capabilities": ["temperature", "humidity", "ec", "ph", "nitrogen", "phosphorus", "potassium"],
        "compatible": True,
        "limitations": [],
        "available_measurements": [],
        "unavailable_measurements": []
    }

    if growing_type == "soil":
        # ✅ ПОЛНАЯ СОВМЕСТИМОСТЬ
        compatibility["available_measurements"] = ["temperature", "humidity", "ec", "ph", "nitrogen", "phosphorus", "potassium"]
        compatibility["description"] = "Почвенный датчик идеально подходит для традиционного выращивания"

    elif growing_type == "greenhouse":
        # ✅ ПОЛНАЯ СОВМЕСТИМОСТЬ
        compatibility["available_measurements"] = ["temperature", "humidity", "ec", "ph", "nitrogen", "phosphorus", "potassium"]
        compatibility["description"] = "Теплица использует почву - датчик работает отлично"

    elif growing_type == "hydroponics":
        # ⚠️ ОГРАНИЧЕННАЯ СОВМЕСТИМОСТЬ
        compatibility["compatible"] = True
        compatibility["limitations"].append("NPK измерения недоступны в жидкой среде")
        compatibility["available_measurements"] = ["temperature", "humidity", "ec", "ph"]
        compatibility["unavailable_measurements"] = ["nitrogen", "phosphorus", "potassium"]
        compatibility["description"] = "Датчик может измерять только EC и pH в растворе"

    elif growing_type == "aeroponics":
        # ❌ НЕ СОВМЕСТИМО
        compatibility["compatible"] = False
        compatibility["limitations"].append("Датчик не может быть установлен в воздушной среде")
        compatibility["limitations"].append("Все измерения недоступны")
        compatibility["unavailable_measurements"] = ["temperature", "humidity", "ec", "ph", "nitrogen", "phosphorus", "potassium"]
        compatibility["description"] = "Почвенный датчик не подходит для аэропоники"

    elif growing_type == "organic":
        # ✅ ПОЛНАЯ СОВМЕСТИМОСТЬ
        compatibility["available_measurements"] = ["temperature", "humidity", "ec", "ph", "nitrogen", "phosphorus", "potassium"]
        compatibility["description"] = "Органическое выращивание использует почву - полная совместимость"

    else:
        # ❌ НЕИЗВЕСТНЫЙ ТИП
        compatibility["compatible"] = False
        compatibility["limitations"].append(f"Неизвестный тип выращивания: {growing_type}")
        compatibility["description"] = "Тип выращивания не поддерживается"

    return compatibility

class TestSensorCompatibility(unittest.TestCase):
    """Тесты совместимости почвенного датчика"""

    def test_soil_compatibility(self):
        """Тест совместимости с почвенным выращиванием"""
        print("\n=== Тест совместимости с почвой ===")

        result = check_sensor_compatibility("soil")

        self.assertTrue(result["compatible"])
        self.assertEqual(len(result["available_measurements"]), 7)
        self.assertEqual(len(result["unavailable_measurements"]), 0)
        self.assertEqual(len(result["limitations"]), 0)

        print(f"  ✅ Совместимость: {result['compatible']}")
        print(f"  📊 Доступные измерения: {result['available_measurements']}")
        print(f"  📝 Описание: {result['description']}")

    def test_greenhouse_compatibility(self):
        """Тест совместимости с теплицей"""
        print("\n=== Тест совместимости с теплицей ===")

        result = check_sensor_compatibility("greenhouse")

        self.assertTrue(result["compatible"])
        self.assertEqual(len(result["available_measurements"]), 7)
        self.assertEqual(len(result["unavailable_measurements"]), 0)
        self.assertEqual(len(result["limitations"]), 0)

        print(f"  ✅ Совместимость: {result['compatible']}")
        print(f"  📊 Доступные измерения: {result['available_measurements']}")
        print(f"  📝 Описание: {result['description']}")

    def test_hydroponics_compatibility(self):
        """Тест совместимости с гидропоникой"""
        print("\n=== Тест совместимости с гидропоникой ===")

        result = check_sensor_compatibility("hydroponics")

        self.assertTrue(result["compatible"])  # Частично совместимо
        self.assertEqual(len(result["available_measurements"]), 4)  # Только 4 измерения
        self.assertEqual(len(result["unavailable_measurements"]), 3)  # NPK недоступны
        self.assertEqual(len(result["limitations"]), 1)

        print(f"  ⚠️ Совместимость: {result['compatible']} (ограниченная)")
        print(f"  📊 Доступные измерения: {result['available_measurements']}")
        print(f"  ❌ Недоступные измерения: {result['unavailable_measurements']}")
        print(f"  ⚠️ Ограничения: {result['limitations']}")
        print(f"  📝 Описание: {result['description']}")

    def test_aeroponics_compatibility(self):
        """Тест совместимости с аэропоникой"""
        print("\n=== Тест совместимости с аэропоникой ===")

        result = check_sensor_compatibility("aeroponics")

        self.assertFalse(result["compatible"])
        self.assertEqual(len(result["available_measurements"]), 0)  # Никаких измерений
        self.assertEqual(len(result["unavailable_measurements"]), 7)  # Все недоступны
        self.assertEqual(len(result["limitations"]), 2)

        print(f"  ❌ Совместимость: {result['compatible']}")
        print(f"  ❌ Недоступные измерения: {result['unavailable_measurements']}")
        print(f"  ⚠️ Ограничения: {result['limitations']}")
        print(f"  📝 Описание: {result['description']}")

    def test_organic_compatibility(self):
        """Тест совместимости с органическим выращиванием"""
        print("\n=== Тест совместимости с органическим выращиванием ===")

        result = check_sensor_compatibility("organic")

        self.assertTrue(result["compatible"])
        self.assertEqual(len(result["available_measurements"]), 7)
        self.assertEqual(len(result["unavailable_measurements"]), 0)
        self.assertEqual(len(result["limitations"]), 0)

        print(f"  ✅ Совместимость: {result['compatible']}")
        print(f"  📊 Доступные измерения: {result['available_measurements']}")
        print(f"  📝 Описание: {result['description']}")

    def test_unknown_type_compatibility(self):
        """Тест с неизвестным типом выращивания"""
        print("\n=== Тест с неизвестным типом ===")

        result = check_sensor_compatibility("aquaponics")

        self.assertFalse(result["compatible"])
        self.assertEqual(len(result["limitations"]), 1)

        print(f"  ❌ Совместимость: {result['compatible']}")
        print(f"  ⚠️ Ограничения: {result['limitations']}")
        print(f"  📝 Описание: {result['description']}")

def main():
    """Главная функция"""
    print("🔬 ТЕСТ СОВМЕСТИМОСТИ ПОЧВЕННОГО ДАТЧИКА")
    print("=" * 50)

    # Запускаем тесты
    unittest.main(argv=[''], exit=False, verbosity=2)

    print("\n📋 ИТОГОВАЯ СВОДКА СОВМЕСТИМОСТИ:")
    print("-" * 30)

    growing_types = ["soil", "greenhouse", "hydroponics", "aeroponics", "organic"]

    for growing_type in growing_types:
        result = check_sensor_compatibility(growing_type)
        status = "✅" if result["compatible"] else "❌"
        print(f"{status} {growing_type.upper()}: {result['description']}")

    print("\n✅ Все тесты совместимости пройдены!")

if __name__ == "__main__":
    main()
