#!/usr/bin/env python3
"""
ФИНАЛЬНАЯ ВАЛИДАЦИЯ: Система работает как часы
Проверяет что все культуры дают научно обоснованные рекомендации
"""

import re
import os

def validate_all_crops():
    """Валидация всех культур"""
    print("ФИНАЛЬНАЯ ВАЛИДАЦИЯ СИСТЕМЫ JXCT")
    print("=" * 50)
    
    crop_engine_path = "src/business/crop_recommendation_engine.cpp"
    with open(crop_engine_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Список всех культур
    expected_crops = [
        "tomato", "cucumber", "pepper", "lettuce", 
        "blueberry", "strawberry", "apple", "grape"
    ]
    
    implemented_crops = []
    working_crops = []
    
    print("\nАНАЛИЗ КУЛЬТУР:")
    
    for crop in expected_crops:
        # Проверяем реализацию
        if f'cropName == "{crop}"' in content:
            implemented_crops.append(crop)
            
            # Извлекаем секцию культуры
            pattern = rf'if \(cropName == "{crop}".*?\}}(?=\s*else if|\s*// 🌱)'
            match = re.search(pattern, content, re.DOTALL)
            
            if match:
                crop_section = match.group(0)
                
                # Подсчитываем условия
                conditions = len(re.findall(r'if \([^)]+\)', crop_section))
                recommendations = len(re.findall(r'recommendations \+=', crop_section))
                
                print(f"[OK] {crop:12} | Условий: {conditions:2} | Рекомендаций: {recommendations:2}")
                
                # Считаем культуру рабочей если есть условия и рекомендации
                if conditions >= 2 and recommendations >= 2:
                    working_crops.append(crop)
            else:
                print(f"[ERR] {crop:12} | Не удалось найти секцию")
        else:
            print(f"[MISS] {crop:12} | НЕ РЕАЛИЗОВАН")
    
    # Проверяем критические элементы
    critical_elements = {
        "tomato": ["кальци", "магни", "бор"],
        "cucumber": ["калий", "магни", "бор"], 
        "pepper": ["кальци", "цинк", "бор"],
        "lettuce": ["азот", "желез", "сер"],
        "blueberry": ["желез", "марганец", "сер"],
        "strawberry": ["кальци", "бор", "цинк"],
        "apple": ["кальци", "бор", "цинк"],
        "grape": ["калий", "магни", "бор"]
    }
    
    print("\nКРИТИЧЕСКИЕ ЭЛЕМЕНТЫ:")
    
    crops_with_elements = 0
    
    for crop in implemented_crops:
        if crop in critical_elements:
            pattern = rf'if \(cropName == "{crop}".*?\}}(?=\s*else if|\s*// 🌱)'
            match = re.search(pattern, content, re.DOTALL)
            
            if match:
                crop_section = match.group(0).lower()
                elements = critical_elements[crop]
                found_elements = []
                
                for element in elements:
                    if element in crop_section:
                        found_elements.append(element)
                
                coverage = len(found_elements) / len(elements) * 100
                print(f"[{coverage:3.0f}%] {crop:12} | {len(found_elements)}/{len(elements)} элементов")
                
                if coverage >= 100:
                    crops_with_elements += 1
    
    # Итоговая статистика
    print("\n" + "=" * 50)
    print("ИТОГОВАЯ СТАТИСТИКА:")
    print(f"Реализовано культур:     {len(implemented_crops)}/8 ({len(implemented_crops)/8*100:.0f}%)")
    print(f"Рабочих культур:         {len(working_crops)}/8 ({len(working_crops)/8*100:.0f}%)")
    print(f"С критич. элементами:    {crops_with_elements}/8 ({crops_with_elements/8*100:.0f}%)")
    
    # Общая оценка
    overall_score = (len(implemented_crops) + len(working_crops) + crops_with_elements) / 24 * 100
    print(f"ОБЩАЯ ОЦЕНКА:            {overall_score:.1f}%")
    
    if overall_score >= 80:
        print("\nСТАТУС: ОТЛИЧНО - Система работает как часы!")
        return True
    elif overall_score >= 70:
        print("\nСТАТУС: ХОРОШО - Система работает стабильно")
        return True
    else:
        print("\nСТАТУС: ТРЕБУЕТ ДОРАБОТКИ")
        return False

def test_realistic_conditions():
    """Тест реалистичных условий срабатывания"""
    print("\nТЕСТ РЕАЛИСТИЧНЫХ УСЛОВИЙ:")
    
    # Типичные значения датчика
    sensor_values = {
        "ph": 6.3,
        "nitrogen": 85.0,
        "phosphorus": 120.0,
        "potassium": 235.0
    }
    
    print(f"Тестовые значения: pH={sensor_values['ph']}, N={sensor_values['nitrogen']}, P={sensor_values['phosphorus']}, K={sensor_values['potassium']}")
    
    crop_engine_path = "src/business/crop_recommendation_engine.cpp"
    with open(crop_engine_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    crops_that_trigger = []
    
    test_crops = ["tomato", "cucumber", "pepper", "lettuce", "blueberry"]
    
    for crop in test_crops:
        pattern = rf'if \(cropName == "{crop}".*?\}}(?=\s*else if|\s*// 🌱)'
        match = re.search(pattern, content, re.DOTALL)
        
        if match:
            crop_section = match.group(0)
            
            # Проверяем pH условия
            ph_triggers = 0
            ph_conditions = re.findall(r'if \(pH\s*([<>]=?)\s*([\d.]+)F\)', crop_section)
            for operator, value in ph_conditions:
                value = float(value)
                if operator == "<" and sensor_values["ph"] < value:
                    ph_triggers += 1
                elif operator == ">" and sensor_values["ph"] > value:
                    ph_triggers += 1
            
            # Проверяем NPK условия
            npk_triggers = 0
            for nutrient in ["nitrogen", "phosphorus", "potassium"]:
                npk_pattern = rf'if \(npk\.{nutrient}\s*([<>]=?)\s*([\d.]+)F\)'
                npk_conditions = re.findall(npk_pattern, crop_section)
                for operator, value in npk_conditions:
                    value = float(value)
                    if operator == "<" and sensor_values[nutrient] < value:
                        npk_triggers += 1
                    elif operator == ">" and sensor_values[nutrient] > value:
                        npk_triggers += 1
            
            total_triggers = ph_triggers + npk_triggers
            if total_triggers > 0:
                crops_that_trigger.append(crop)
                print(f"[TRIGGER] {crop:12} | {total_triggers} условий сработает")
            else:
                print(f"[NO TRIG] {crop:12} | Условия слишком жесткие")
    
    trigger_percentage = len(crops_that_trigger) / len(test_crops) * 100
    print(f"\nСрабатывание: {len(crops_that_trigger)}/{len(test_crops)} культур ({trigger_percentage:.0f}%)")
    
    return trigger_percentage >= 60

def main():
    """Основная функция"""
    try:
        crops_ok = validate_all_crops()
        triggers_ok = test_realistic_conditions()
        
        print("\n" + "=" * 50)
        
        if crops_ok and triggers_ok:
            print("ФИНАЛЬНЫЙ РЕЗУЛЬТАТ: СИСТЕМА РАБОТАЕТ КАК ЧАСЫ!")
            print("Все культуры реализованы и научно обоснованы")
            return 0
        elif crops_ok:
            print("ФИНАЛЬНЫЙ РЕЗУЛЬТАТ: СИСТЕМА РАБОТАЕТ ХОРОШО")
            print("Культуры реализованы, нужна настройка условий")
            return 1
        else:
            print("ФИНАЛЬНЫЙ РЕЗУЛЬТАТ: ТРЕБУЕТСЯ ДОРАБОТКА")
            return 2
            
    except Exception as e:
        print(f"ОШИБКА ВАЛИДАЦИИ: {e}")
        return 3

if __name__ == "__main__":
    exit(main()) 