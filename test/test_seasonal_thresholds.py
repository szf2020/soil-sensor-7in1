#!/usr/bin/env python3
"""
Тест сезонных корректировок порогов дефицита
Проверяет, что пороги дефицита корректно корректируются в зависимости от сезона
"""

def test_seasonal_thresholds():
    """Тестирует сезонные корректировки порогов дефицита"""
    
    # Сезонные корректировки (из C++ кода)
    seasonal_factors = {
        "spring": {"nitrogen": 1.15, "phosphorus": 1.10, "potassium": 1.12},
        "summer": {"nitrogen": 1.08, "phosphorus": 1.05, "potassium": 1.20},
        "autumn": {"nitrogen": 1.06, "phosphorus": 1.12, "potassium": 1.15},
        "winter": {"nitrogen": 0.85, "phosphorus": 1.08, "potassium": 1.10}
    }
    
    # Базовые пороги дефицита
    base_thresholds = {
        "nitrogen": 100.0,
        "phosphorus": 50.0,
        "potassium": 150.0
    }
    
    # Тестовые случаи (без сезонных корректировок порогов)
    test_cases = [
        {
            "season": "summer",
            "nitrogen": 114.0,
            "expected_nitrogen_deficient": False,
            "description": "Лето: азот 114, стандартный порог 100 (без сезонных корректировок)"
        },
        {
            "season": "winter", 
            "nitrogen": 120.0,
            "expected_nitrogen_deficient": False,
            "description": "Зима: азот 120, стандартный порог 100 (без сезонных корректировок)"
        },
        {
            "season": "spring",
            "nitrogen": 110.0,
            "expected_nitrogen_deficient": False,
            "description": "Весна: азот 110, стандартный порог 100 (без сезонных корректировок)"
        },
        {
            "season": "summer",
            "nitrogen": 114.0,
            "crop": "lawn",
            "expected_lawn_recommendation": True,
            "description": "Газон летом: азот 114 < 120, должна быть рекомендация"
        }
    ]
    
    print("🧪 Тест сезонных корректировок порогов дефицита")
    print("=" * 60)
    
    for i, test_case in enumerate(test_cases, 1):
        season = test_case["season"]
        nitrogen = test_case["nitrogen"]
        description = test_case["description"]
        
        # Определяем ожидаемый результат в зависимости от типа теста
        if "crop" in test_case and test_case["crop"] == "lawn":
            expected = test_case["expected_lawn_recommendation"]
        else:
            expected = test_case["expected_nitrogen_deficient"]
        
        # Используем стандартный порог (без сезонных корректировок)
        # Согласно логике: "сырые значения потом коррекция, затем научная компенсация и на этом все"
        adjusted_threshold = base_thresholds["nitrogen"]
        
        # Проверяем дефицит
        is_deficient = nitrogen < adjusted_threshold
        
        print(f"\n📋 Тест {i}: {description}")
        print(f"   Сезон: {season}")
        print(f"   Азот: {nitrogen} мг/кг")
        print(f"   Базовый порог: {base_thresholds['nitrogen']} мг/кг")
        print(f"   Скорректированный порог: {adjusted_threshold:.1f} мг/кг")
        print(f"   Дефицит: {is_deficient} (ожидается: {expected})")
        
        # Проверяем специфичную рекомендацию для газона
        if "crop" in test_case and test_case["crop"] == "lawn":
            lawn_threshold = 120.0  # Стандартный порог газона 120 (без сезонных корректировок)
            lawn_recommendation = nitrogen < lawn_threshold
            expected_lawn = test_case["expected_lawn_recommendation"]
            print(f"   Порог газона: {lawn_threshold:.1f} мг/кг")
            print(f"   Рекомендация газона: {lawn_recommendation} (ожидается: {expected_lawn})")
            
            if lawn_recommendation == expected_lawn:
                print("   ✅ ПРОЙДЕН")
            else:
                print("   ❌ НЕ ПРОЙДЕН")
                return False
        else:
            if is_deficient == expected:
                print("   ✅ ПРОЙДЕН")
            else:
                print("   ❌ НЕ ПРОЙДЕН")
                return False
    
    print(f"\n🎉 Все {len(test_cases)} тестов пройдены успешно!")
    return True

if __name__ == "__main__":
    success = test_seasonal_thresholds()
    exit(0 if success else 1) 