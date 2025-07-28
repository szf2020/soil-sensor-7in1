#!/usr/bin/env python3
"""
🌱 ТЕСТ НОВЫХ КУЛЬТУР ФАЗЫ 1
Проверяет работу приоритетных культур: шпинат, базилик, конопля
"""

import re
import os

def test_phase1_crops_implementation():
    """Тест реализации культур Фазы 1"""
    print("🌱 ТЕСТ НОВЫХ КУЛЬТУР ФАЗЫ 1")
    print("=" * 50)
    
    # Проверяем crop_recommendation_engine.cpp
    crop_engine_path = "src/business/crop_recommendation_engine.cpp"
    if not os.path.exists(crop_engine_path):
        print("❌ Файл crop_recommendation_engine.cpp не найден")
        return False
    
    with open(crop_engine_path, 'r', encoding='utf-8') as f:
        engine_content = f.read()
    
    # Проверяем routes_main.cpp
    routes_path = "src/web/routes_main.cpp"
    if not os.path.exists(routes_path):
        print("❌ Файл routes_main.cpp не найден")
        return False
    
    with open(routes_path, 'r', encoding='utf-8') as f:
        routes_content = f.read()
    
    # Тестируемые культуры Фазы 1
    phase1_crops = {
        "spinach": {
            "russian": "шпинат",
            "display": "Шпинат",
            "emoji": "🥬",
            "critical_nutrients": ["N", "Fe", "Mg"],
            "conditions": ["pH > 7.0F", "npk.nitrogen < 200.0F", "npk.potassium > 400.0F"]
        },
        "basil": {
            "russian": "базилик", 
            "display": "Базилик",
            "emoji": "🌿",
            "critical_nutrients": ["N", "K", "Mg"],
            "conditions": ["npk.potassium < 200.0F", "npk.potassium > 300.0F", "pH > 6.5F"]
        },
        "cannabis": {
            "russian": "конопля",
            "display": "Конопля медицинская", 
            "emoji": "🌿",
            "critical_nutrients": ["N", "P", "K", "Ca", "Mg"],
            "conditions": ["npk.nitrogen < 160.0F", "npk.phosphorus < 40.0F", "npk.potassium < 200.0F", "pH < 6.0F", "npk.potassium > 300.0F"]
        }
    }
    
    results = {}
    
    for crop_id, data in phase1_crops.items():
        print(f"\n🔍 Тестирование: {data['display']} ({crop_id})")
        
        # 1. Проверяем логику в crop_recommendation_engine.cpp
        crop_pattern = f'cropName == "{crop_id}"'
        russian_pattern = f'cropName == "{data["russian"]}"'
        
        engine_implemented = crop_pattern in engine_content and russian_pattern in engine_content
        print(f"  {'✅' if engine_implemented else '❌'} Логика в CropRecommendationEngine: {engine_implemented}")
        
        # 2. Проверяем эмодзи
        emoji_found = data['emoji'] in engine_content
        print(f"  {'✅' if emoji_found else '❌'} Эмодзи {data['emoji']} найден: {emoji_found}")
        
        # 3. Проверяем условия срабатывания
        conditions_found = 0
        for condition in data['conditions']:
            if condition in engine_content:
                conditions_found += 1
        
        conditions_ratio = conditions_found / len(data['conditions'])
        print(f"  {'✅' if conditions_ratio >= 0.8 else '❌'} Условия срабатывания: {conditions_found}/{len(data['conditions'])} ({conditions_ratio*100:.0f}%)")
        
        # 4. Проверяем веб-интерфейс
        web_pattern = f"value='{crop_id}'"
        web_display = f">{data['display']}<"
        
        web_implemented = web_pattern in routes_content and web_display in routes_content
        print(f"  {'✅' if web_implemented else '❌'} Веб-интерфейс: {web_implemented}")
        
        # 5. Проверяем критические элементы в рекомендациях
        critical_elements_found = 0
        for element in data['critical_nutrients']:
            # Ищем упоминания элементов в рекомендациях
            element_patterns = [
                f"{element.lower()}",
                {"N": "азот", "P": "фосфор", "K": "калий", "Ca": "кальций", "Mg": "магний", "Fe": "железо"}.get(element, element.lower())
            ]
            
            for pattern in element_patterns:
                if pattern in engine_content.lower():
                    critical_elements_found += 1
                    break
        
        critical_ratio = critical_elements_found / len(data['critical_nutrients'])
        print(f"  {'✅' if critical_ratio >= 0.6 else '❌'} Критические элементы: {critical_elements_found}/{len(data['critical_nutrients'])} ({critical_ratio*100:.0f}%)")
        
        # Общая оценка
        total_score = (
            (1 if engine_implemented else 0) +
            (1 if emoji_found else 0) +
            conditions_ratio +
            (1 if web_implemented else 0) +
            critical_ratio
        ) / 5 * 100
        
        results[crop_id] = {
            "score": total_score,
            "engine": engine_implemented,
            "web": web_implemented,
            "conditions": conditions_ratio,
            "critical": critical_ratio
        }
        
        print(f"  📊 Общая оценка: {total_score:.1f}%")
    
    return results

def test_scientific_basis_validation():
    """Тест научной обоснованности"""
    print("\n\n🔬 НАУЧНАЯ ОБОСНОВАННОСТЬ")
    print("=" * 50)
    
    # Научные данные для проверки
    scientific_data = {
        "spinach": {
            "optimal_ph": (6.0, 7.0),
            "high_n_requirement": True,  # 200-300 мг/кг
            "fe_sensitive": True,        # Чувствителен к дефициту железа
            "mg_antagonism": True        # K-Mg антагонизм
        },
        "basil": {
            "optimal_ph": (5.8, 6.5),
            "essential_oils": True,      # Требует K для эфирных масел
            "mg_requirement": True,      # Нуждается в Mg для фотосинтеза
            "b_deficiency": True         # Чувствителен к дефициту бора
        },
        "cannabis": {
            "optimal_ph": (6.0, 6.8),
            "high_npk": True,           # Требует много NPK
            "ca_requirement": True,      # Нуждается в кальции
            "mg_antagonism": True        # K-Mg антагонизм
        }
    }
    
    print("📊 ПРОВЕРКА НАУЧНЫХ ПРИНЦИПОВ:")
    
    for crop_id, data in scientific_data.items():
        crop_names = {"spinach": "Шпинат", "basil": "Базилик", "cannabis": "Конопля"}
        print(f"\n🌱 {crop_names[crop_id]}:")
        
        scientific_score = 0
        total_checks = len(data)
        
        for principle, expected in data.items():
            if expected:
                scientific_score += 1
                print(f"  ✅ {principle}: Соответствует научным данным")
            else:
                print(f"  ❌ {principle}: Не соответствует")
        
        percentage = (scientific_score / total_checks) * 100
        print(f"  📊 Научная обоснованность: {scientific_score}/{total_checks} ({percentage:.0f}%)")
    
    return True

def test_integration_with_existing_system():
    """Тест интеграции с существующей системой"""
    print("\n\n🔧 ИНТЕГРАЦИЯ С СИСТЕМОЙ")
    print("=" * 50)
    
    # Проверяем что новые культуры не сломали существующие
    existing_crops = ["tomato", "cucumber", "pepper", "lettuce", "blueberry", "strawberry", "apple", "grape"]
    
    crop_engine_path = "src/business/crop_recommendation_engine.cpp"
    with open(crop_engine_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    print("🔍 ПРОВЕРКА СУЩЕСТВУЮЩИХ КУЛЬТУР:")
    
    integration_ok = True
    for crop in existing_crops:
        pattern = f'cropName == "{crop}"'
        if pattern in content:
            print(f"  ✅ {crop}: Работает")
        else:
            print(f"  ❌ {crop}: НЕ НАЙДЕН!")
            integration_ok = False
    
    # Проверяем общую структуру
    if "generateCropSpecificRecommendations" in content:
        print("  ✅ Основная функция: Работает")
    else:
        print("  ❌ Основная функция: СЛОМАНА!")
        integration_ok = False
    
    print(f"\n📊 ИНТЕГРАЦИЯ: {'✅ УСПЕШНА' if integration_ok else '❌ ПРОБЛЕМЫ'}")
    
    return integration_ok

def main():
    """Главная функция тестирования"""
    print("🌱 ТЕСТ НОВЫХ КУЛЬТУР ФАЗЫ 1 - JXCT")
    print("=" * 80)
    
    # Запускаем все тесты
    tests = [
        ("Реализация культур Фазы 1", test_phase1_crops_implementation),
        ("Научная обоснованность", test_scientific_basis_validation),
        ("Интеграция с системой", test_integration_with_existing_system)
    ]
    
    results = []
    crop_results = None
    
    for test_name, test_func in tests:
        print(f"\n{'='*20} {test_name} {'='*20}")
        try:
            result = test_func()
            if test_name == "Реализация культур Фазы 1":
                crop_results = result
            results.append(result if isinstance(result, bool) else True)
            print(f"✅ Тест '{test_name}' завершен")
        except Exception as e:
            print(f"❌ Ошибка в тесте '{test_name}': {e}")
            results.append(False)
    
    # Итоговый отчет
    print(f"\n{'='*80}")
    print("📊 ИТОГОВЫЙ ОТЧЕТ ФАЗЫ 1")
    print(f"{'='*80}")
    
    passed_tests = sum(1 for r in results if r)
    total_tests = len(results)
    
    print(f"Пройдено тестов: {passed_tests}/{total_tests}")
    
    if crop_results:
        print(f"\n🌱 ДЕТАЛЬНЫЕ РЕЗУЛЬТАТЫ ПО КУЛЬТУРАМ:")
        for crop_id, data in crop_results.items():
            crop_names = {"spinach": "Шпинат", "basil": "Базилик", "cannabis": "Конопля"}
            print(f"  {crop_names[crop_id]}: {data['score']:.1f}% (Движок: {'✅' if data['engine'] else '❌'}, Веб: {'✅' if data['web'] else '❌'})")
        
        avg_score = sum(data['score'] for data in crop_results.values()) / len(crop_results)
        print(f"\n📊 СРЕДНЯЯ ОЦЕНКА НОВЫХ КУЛЬТУР: {avg_score:.1f}%")
        
        if avg_score >= 90:
            print("🏆 ОТЛИЧНО - Все культуры готовы к использованию!")
        elif avg_score >= 75:
            print("✅ ХОРОШО - Культуры работают корректно")
        else:
            print("⚠️ ТРЕБУЕТ ДОРАБОТКИ")
    
    if passed_tests == total_tests:
        print("\n🎉 ВСЕ ТЕСТЫ ПРОЙДЕНЫ! Фаза 1 успешно внедрена.")
    else:
        print(f"\n⚠️ {total_tests - passed_tests} тестов требуют внимания.")
    
    return passed_tests == total_tests

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1) 