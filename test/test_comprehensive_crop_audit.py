#!/usr/bin/env python3
"""
🔬 КОМПЛЕКСНЫЙ НАУЧНЫЙ АУДИТ: Все культуры в JXCT
Проверяет научную обоснованность рекомендаций для всех поддерживаемых культур
"""

import re
import os
import sys

# Научно подтвержденные данные для культур
SCIENTIFIC_CROP_DATA = {
    "tomato": {
        "name": "Томат (Solanum lycopersicum)",
        "optimal_ph": (6.0, 6.8),
        "optimal_ec": (1500, 3000),  # μS/cm
        "npk_ranges": {"N": (150, 250), "P": (50, 100), "K": (200, 400)},  # мг/кг
        "critical_nutrients": ["Ca", "Mg", "B"],
        "common_deficiencies": ["Ca (вершинная гниль)", "Mg (хлороз)", "B (деформация плодов)"],
        "sources": ["University of Florida IFAS Extension, B. Santos, 2019", "Marschner's Mineral Nutrition, 2012"]
    },
    "cucumber": {
        "name": "Огурец (Cucumis sativus)",
        "optimal_ph": (6.0, 7.0),
        "optimal_ec": (1200, 2500),
        "npk_ranges": {"N": (120, 200), "P": (40, 80), "K": (150, 300)},
        "critical_nutrients": ["K", "Mg", "B"],
        "common_deficiencies": ["K (слабое плодоношение)", "B (щелочная почва)", "Mg (антагонизм с K)"],
        "sources": ["USDA Natural Resources Conservation Service, 2020", "Horticultural Science, 2018"]
    },
    "pepper": {
        "name": "Перец (Capsicum annuum)",
        "optimal_ph": (6.0, 7.0),
        "optimal_ec": (1400, 2800),
        "npk_ranges": {"N": (100, 180), "P": (30, 70), "K": (150, 350)},
        "critical_nutrients": ["Ca", "Zn", "B"],
        "common_deficiencies": ["Zn (высокий P)", "Ca (качество плодов)", "B (цветение)"],
        "sources": ["Cornell University Cooperative Extension, 2022", "Journal of Plant Nutrition, 2020"]
    },
    "lettuce": {
        "name": "Салат (Lactuca sativa)",
        "optimal_ph": (6.0, 7.0),
        "optimal_ec": (1000, 2000),
        "npk_ranges": {"N": (80, 150), "P": (20, 50), "K": (100, 250)},
        "critical_nutrients": ["N", "Fe", "S"],
        "common_deficiencies": ["Fe (щелочная почва)", "S (высокий N)", "Mn (pH > 7)"],
        "sources": ["UC Agriculture and Natural Resources, 2018", "Plant and Soil, 2019"]
    },
    "blueberry": {
        "name": "Голубика (Vaccinium corymbosum)",
        "optimal_ph": (4.5, 5.5),
        "optimal_ec": (800, 1500),
        "npk_ranges": {"N": (60, 120), "P": (20, 40), "K": (80, 160)},
        "critical_nutrients": ["Fe", "Mn", "S"],
        "common_deficiencies": ["Fe (хлороз)", "Mn (кислая почва)", "S (подкисление)"],
        "sources": ["Michigan State University Extension, A. Schilder, 2021", "Blueberry Production Guide, 2020"]
    },
    "strawberry": {
        "name": "Клубника (Fragaria × ananassa)",
        "optimal_ph": (5.5, 6.5),
        "optimal_ec": (1000, 2000),
        "npk_ranges": {"N": (100, 150), "P": (30, 60), "K": (150, 250)},
        "critical_nutrients": ["Ca", "B", "Zn"],
        "common_deficiencies": ["Ca (качество ягод)", "B (опыление)", "Zn (рост)"],
        "sources": ["North Carolina State Extension, 2021", "Small Fruit Production Guide, 2019"]
    },
    "apple": {
        "name": "Яблоня (Malus domestica)",
        "optimal_ph": (6.0, 7.0),
        "optimal_ec": (1200, 2200),
        "npk_ranges": {"N": (120, 180), "P": (25, 50), "K": (150, 300)},
        "critical_nutrients": ["Ca", "B", "Zn"],
        "common_deficiencies": ["Ca (горькая ямчатость)", "B (деформация плодов)", "Zn (розеточность)"],
        "sources": ["Penn State Extension, 2020", "Tree Fruit Production Guide, 2021"]
    },
    "grape": {
        "name": "Виноград (Vitis vinifera)",
        "optimal_ph": (6.0, 7.5),
        "optimal_ec": (1000, 2500),
        "npk_ranges": {"N": (80, 140), "P": (20, 40), "K": (150, 300)},
        "critical_nutrients": ["K", "Mg", "B"],
        "common_deficiencies": ["K (качество ягод)", "Mg (хлороз)", "B (завязывание)"],
        "sources": ["UC Davis Viticulture, 2019", "Wine Grape Production Guide, 2020"]
    }
}

def analyze_crop_implementation():
    """Анализ реализации культур в коде"""
    print("🔬 АНАЛИЗ: Реализация культур в коде")
    
    crop_engine_path = "src/business/crop_recommendation_engine.cpp"
    with open(crop_engine_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Находим все культуры с рекомендациями
    implemented_crops = []
    
    for crop_id in SCIENTIFIC_CROP_DATA.keys():
        pattern = rf'if \(cropName == "{crop_id}".*?\}}'
        match = re.search(pattern, content, re.DOTALL)
        if match:
            implemented_crops.append(crop_id)
            print(f"✅ {crop_id}: Реализован")
        else:
            print(f"❌ {crop_id}: НЕ реализован")
    
    print(f"\n📊 Реализовано культур: {len(implemented_crops)}/{len(SCIENTIFIC_CROP_DATA)}")
    return implemented_crops

def validate_crop_conditions(crop_id, implemented_crops):
    """Валидация условий для конкретной культуры"""
    if crop_id not in implemented_crops:
        return {"status": "not_implemented", "issues": ["Культура не реализована"]}
    
    crop_engine_path = "src/business/crop_recommendation_engine.cpp"
    with open(crop_engine_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Извлекаем секцию культуры
    pattern = rf'if \(cropName == "{crop_id}".*?\}}(?=\s*else if|\s*// 🌱)'
    match = re.search(pattern, content, re.DOTALL)
    
    if not match:
        return {"status": "parse_error", "issues": ["Не удалось найти секцию культуры"]}
    
    crop_section = match.group(0)
    scientific_data = SCIENTIFIC_CROP_DATA[crop_id]
    
    issues = []
    recommendations = []
    
    # Проверяем pH условия
    ph_conditions = re.findall(r'pH\s*([<>]=?)\s*([\d.]+)F', crop_section)
    if ph_conditions:
        for operator, value in ph_conditions:
            value = float(value)
            optimal_min, optimal_max = scientific_data["optimal_ph"]
            
            if operator in ["<", "<="] and value < optimal_min:
                issues.append(f"pH порог {value} слишком низкий (научный диапазон: {optimal_min}-{optimal_max})")
            elif operator in [">", ">="] and value > optimal_max:
                issues.append(f"pH порог {value} слишком высокий (научный диапазон: {optimal_min}-{optimal_max})")
            else:
                recommendations.append(f"pH условие {operator} {value} соответствует науке")
    else:
        issues.append("Отсутствуют pH условия")
    
    # Проверяем NPK условия
    for nutrient in ["nitrogen", "phosphorus", "potassium"]:
        npk_pattern = rf'npk\.{nutrient}\s*([<>]=?)\s*([\d.]+)F'
        npk_conditions = re.findall(npk_pattern, crop_section)
        
        if npk_conditions:
            for operator, value in npk_conditions:
                value = float(value)
                nutrient_short = nutrient[0].upper()
                optimal_min, optimal_max = scientific_data["npk_ranges"][nutrient_short]
                
                # Проверяем разумность порогов
                if operator in ["<", "<="] and value < optimal_min * 0.5:
                    issues.append(f"{nutrient_short} порог {value} слишком низкий (научный: {optimal_min}-{optimal_max})")
                elif operator in [">", ">="] and value > optimal_max * 2:
                    issues.append(f"{nutrient_short} порог {value} слишком высокий (научный: {optimal_min}-{optimal_max})")
                else:
                    recommendations.append(f"{nutrient_short} условие {operator} {value} разумно")
    
    # Проверяем упоминание критических элементов
    critical_elements = scientific_data["critical_nutrients"]
    mentioned_elements = []
    
    for element in critical_elements:
        element_names = {
            "Ca": ["кальци", "Ca"],
            "Mg": ["магни", "Mg"],
            "Fe": ["желез", "Fe"],
            "B": ["бор", "B"],
            "Zn": ["цинк", "Zn"],
            "S": ["сер", "S"],
            "Mn": ["марганец", "Mn"]
        }
        
        for name in element_names.get(element, [element]):
            if name.lower() in crop_section.lower():
                mentioned_elements.append(element)
                break
    
    missing_elements = set(critical_elements) - set(mentioned_elements)
    if missing_elements:
        issues.append(f"Не упомянуты критические элементы: {', '.join(missing_elements)}")
    
    return {
        "status": "analyzed",
        "issues": issues,
        "recommendations": recommendations,
        "mentioned_elements": mentioned_elements,
        "critical_elements": critical_elements
    }

def simulate_realistic_sensor_values():
    """Генерация реалистичных значений датчика для тестирования"""
    return {
        "ph": 6.3,           # Типичное значение для большинства почв
        "nitrogen": 85.0,    # Умеренный уровень азота
        "phosphorus": 120.0, # Умеренно-высокий фосфор
        "potassium": 235.0   # Умеренно-высокий калий
    }

def test_crop_triggers(crop_id, sensor_values):
    """Тестирование срабатывания условий для культуры"""
    if crop_id not in SCIENTIFIC_CROP_DATA:
        return {"triggers": [], "should_trigger": False}
    
    crop_engine_path = "src/business/crop_recommendation_engine.cpp"
    with open(crop_engine_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Извлекаем секцию культуры
    pattern = rf'if \(cropName == "{crop_id}".*?\}}(?=\s*else if|\s*// 🌱)'
    match = re.search(pattern, content, re.DOTALL)
    
    if not match:
        return {"triggers": [], "should_trigger": False}
    
    crop_section = match.group(0)
    triggers = []
    
    # Проверяем pH условия
    ph_conditions = re.findall(r'if \(pH\s*([<>]=?)\s*([\d.]+)F\)', crop_section)
    for operator, value in ph_conditions:
        value = float(value)
        ph = sensor_values["ph"]
        
        if operator == "<" and ph < value:
            triggers.append(f"pH {ph} < {value}")
        elif operator == "<=" and ph <= value:
            triggers.append(f"pH {ph} <= {value}")
        elif operator == ">" and ph > value:
            triggers.append(f"pH {ph} > {value}")
        elif operator == ">=" and ph >= value:
            triggers.append(f"pH {ph} >= {value}")
    
    # Проверяем NPK условия
    for nutrient in ["nitrogen", "phosphorus", "potassium"]:
        npk_pattern = rf'if \(npk\.{nutrient}\s*([<>]=?)\s*([\d.]+)F\)'
        npk_conditions = re.findall(npk_pattern, crop_section)
        
        for operator, value in npk_conditions:
            value = float(value)
            npk_value = sensor_values[nutrient]
            
            if operator == "<" and npk_value < value:
                triggers.append(f"{nutrient} {npk_value} < {value}")
            elif operator == "<=" and npk_value <= value:
                triggers.append(f"{nutrient} {npk_value} <= {value}")
            elif operator == ">" and npk_value > value:
                triggers.append(f"{nutrient} {npk_value} > {value}")
            elif operator == ">=" and npk_value >= value:
                triggers.append(f"{nutrient} {npk_value} >= {value}")
    
    return {"triggers": triggers, "should_trigger": len(triggers) > 0}

def comprehensive_crop_audit():
    """Комплексный аудит всех культур"""
    print("🔬 КОМПЛЕКСНЫЙ НАУЧНЫЙ АУДИТ ВСЕХ КУЛЬТУР")
    print("=" * 60)
    
    # Анализируем реализацию
    implemented_crops = analyze_crop_implementation()
    
    # Симулируем реалистичные значения датчика
    sensor_values = simulate_realistic_sensor_values()
    print(f"\n📊 ТЕСТОВЫЕ ЗНАЧЕНИЯ ДАТЧИКА:")
    for param, value in sensor_values.items():
        print(f"   {param}: {value}")
    
    print("\n" + "=" * 60)
    
    total_score = 0
    max_score = 0
    
    for crop_id, scientific_data in SCIENTIFIC_CROP_DATA.items():
        print(f"\n🌱 АУДИТ: {scientific_data['name']}")
        print("-" * 40)
        
        # Валидация условий
        validation = validate_crop_conditions(crop_id, implemented_crops)
        max_score += 10
        
        if validation["status"] == "not_implemented":
            print("❌ СТАТУС: НЕ РЕАЛИЗОВАНО")
            print(f"⚠️  Проблемы: {', '.join(validation['issues'])}")
            continue
        elif validation["status"] == "parse_error":
            print("❌ СТАТУС: ОШИБКА ПАРСИНГА")
            print(f"⚠️  Проблемы: {', '.join(validation['issues'])}")
            continue
        
        # Подсчет баллов
        crop_score = 5  # Базовые баллы за реализацию
        
        if len(validation["issues"]) == 0:
            crop_score += 3
            print("✅ НАУЧНАЯ ВАЛИДНОСТЬ: ОТЛИЧНО")
        elif len(validation["issues"]) <= 2:
            crop_score += 1
            print("⚠️  НАУЧНАЯ ВАЛИДНОСТЬ: ТРЕБУЕТ ДОРАБОТКИ")
        else:
            print("❌ НАУЧНАЯ ВАЛИДНОСТЬ: МНОГО ПРОБЛЕМ")
        
        # Тестирование срабатывания
        trigger_test = test_crop_triggers(crop_id, sensor_values)
        if trigger_test["should_trigger"]:
            crop_score += 2
            print(f"✅ СРАБАТЫВАНИЕ: ДА ({len(trigger_test['triggers'])} условий)")
            for trigger in trigger_test["triggers"]:
                print(f"   • {trigger}")
        else:
            print("⚠️  СРАБАТЫВАНИЕ: НЕТ (условия слишком жесткие)")
        
        # Отображение проблем и рекомендаций
        if validation["issues"]:
            print("⚠️  ПРОБЛЕМЫ:")
            for issue in validation["issues"]:
                print(f"   • {issue}")
        
        if validation["recommendations"]:
            print("✅ ХОРОШО:")
            for rec in validation["recommendations"]:
                print(f"   • {rec}")
        
        # Критические элементы
        mentioned = validation.get("mentioned_elements", [])
        critical = validation.get("critical_elements", [])
        print(f"🧪 КРИТИЧЕСКИЕ ЭЛЕМЕНТЫ: {len(mentioned)}/{len(critical)} упомянуто")
        print(f"   Упомянуты: {', '.join(mentioned) if mentioned else 'нет'}")
        print(f"   Критические: {', '.join(critical)}")
        
        # Научные источники
        print(f"📚 ИСТОЧНИКИ: {len(scientific_data['sources'])}")
        for source in scientific_data['sources']:
            print(f"   • {source}")
        
        total_score += crop_score
        print(f"📊 ОЦЕНКА: {crop_score}/10")
    
    # Итоговая оценка
    print("\n" + "=" * 60)
    print("📊 ИТОГОВАЯ ОЦЕНКА СИСТЕМЫ:")
    print(f"   Общий балл: {total_score}/{max_score}")
    percentage = (total_score / max_score) * 100 if max_score > 0 else 0
    print(f"   Процент качества: {percentage:.1f}%")
    
    if percentage >= 80:
        print("🎉 ОТЛИЧНО: Система работает как часы!")
        grade = "A"
    elif percentage >= 60:
        print("⚠️  ХОРОШО: Требуются небольшие доработки")
        grade = "B"
    else:
        print("❌ ТРЕБУЕТ СЕРЬЕЗНОЙ ДОРАБОТКИ")
        grade = "C"
    
    return {
        "total_score": total_score,
        "max_score": max_score,
        "percentage": percentage,
        "grade": grade,
        "implemented_crops": len(implemented_crops),
        "total_crops": len(SCIENTIFIC_CROP_DATA)
    }

def generate_improvement_recommendations(audit_results):
    """Генерация рекомендаций по улучшению"""
    print("\n🔧 РЕКОМЕНДАЦИИ ПО УЛУЧШЕНИЮ:")
    
    if audit_results["percentage"] < 80:
        print("1. Расширить условия срабатывания для реалистичных значений датчика")
        print("2. Добавить упоминания всех критических элементов")
        print("3. Проверить научную обоснованность пороговых значений")
        print("4. Реализовать недостающие культуры")
    
    if audit_results["implemented_crops"] < audit_results["total_crops"]:
        print("5. Добавить рекомендации для всех заявленных культур")
    
    print("6. Создать автоматические тесты для всех культур")
    print("7. Обновить научные источники до актуальных версий")

def main():
    """Основная функция аудита"""
    try:
        audit_results = comprehensive_crop_audit()
        generate_improvement_recommendations(audit_results)
        
        # Возвращаем код выхода на основе качества
        if audit_results["percentage"] >= 80:
            return 0
        elif audit_results["percentage"] >= 60:
            return 1
        else:
            return 2
            
    except Exception as e:
        print(f"❌ КРИТИЧЕСКАЯ ОШИБКА АУДИТА: {e}")
        return 3

if __name__ == "__main__":
    sys.exit(main()) 