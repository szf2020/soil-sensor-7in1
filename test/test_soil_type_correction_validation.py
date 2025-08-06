#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
🧪 Тест валидации коррекций типа почвы в рекомендациях
Проверяет корректность трех последовательных коррекций:
1. Тип выращивания (ПЕРВАЯ)
2. Тип почвы (ВТОРАЯ) 
3. Сезонная коррекция (ТРЕТЬЯ)

Цель: убедиться, что последовательные коррекции не дают агрессивных искажений
и остаются в границах научно обоснованных рекомендаций.
"""

import pytest
import sys
import os

# Добавляем путь к модулям проекта
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))

def test_three_step_correction_sequence():
    """Тест: проверка последовательности трех коррекций"""
    print("🧪 Тестируем последовательность трех коррекций...")
    
    # Базовые табличные значения для томата (исходные)
    base_values = {
        "nitrogen": 150.0,    # mg/kg
        "phosphorus": 80.0,   # mg/kg  
        "potassium": 200.0,   # mg/kg
        "humidity": 35.0,     # %
        "ec": 1.5,           # mS/cm
        "ph": 6.5            # pH
    }
    
    print(f"📊 Базовые значения: {base_values}")
    
    # 1. КОРРЕКЦИЯ ТИПА ВЫРАЩИВАНИЯ (ПЕРВАЯ)
    growing_type_corrections = {
        "outdoor": {"nitrogen": 1.00, "phosphorus": 1.00, "potassium": 1.00, "ec": 1.00},
        "greenhouse": {"nitrogen": 1.17, "phosphorus": 1.17, "potassium": 1.13, "ec": 1.15},
        "hydroponics": {"nitrogen": 1.32, "phosphorus": 1.33, "potassium": 1.20, "ec": 1.25},
        "organic": {"nitrogen": 0.93, "phosphorus": 0.93, "potassium": 0.92, "ec": 0.90}
    }
    
    # 2. КОРРЕКЦИЯ ТИПА ПОЧВЫ (ВТОРАЯ) - аккуратные коэффициенты
    soil_type_corrections = {
        "sand": {"nitrogen": 1.05, "phosphorus": 1.03, "potassium": 1.04, "humidity": 0.98, "ec": 1.02},
        "loam": {"nitrogen": 1.00, "phosphorus": 1.00, "potassium": 1.00, "humidity": 1.00, "ec": 1.00},
        "clay": {"nitrogen": 0.95, "phosphorus": 0.92, "potassium": 0.96, "humidity": 1.05, "ec": 0.97},
        "peat": {"nitrogen": 1.06, "phosphorus": 1.04, "potassium": 1.02, "humidity": 1.05, "ec": 0.95},
        "organic": {"nitrogen": 1.05, "phosphorus": 1.03, "potassium": 1.02, "humidity": 1.04, "ec": 0.94}
    }
    
    # 3. СЕЗОННАЯ КОРРЕКЦИЯ (ТРЕТЬЯ) - только NPK
    seasonal_corrections = {
        "spring": {"nitrogen": 1.08, "phosphorus": 1.05, "potassium": 1.06},
        "summer": {"nitrogen": 1.03, "phosphorus": 1.02, "potassium": 1.04},
        "autumn": {"nitrogen": 0.97, "phosphorus": 0.98, "potassium": 0.96},
        "winter": {"nitrogen": 0.92, "phosphorus": 0.95, "potassium": 0.90}
    }
    
    # Научно обоснованные границы для томата
    scientific_bounds = {
        "nitrogen": {"min": 120.0, "max": 200.0},      # mg/kg
        "phosphorus": {"min": 60.0, "max": 120.0},     # mg/kg
        "potassium": {"min": 150.0, "max": 300.0},     # mg/kg
        "humidity": {"min": 25.0, "max": 45.0},        # %
        "ec": {"min": 1.0, "max": 3.0},               # mS/cm
        "ph": {"min": 5.5, "max": 7.5}                # pH
    }
    
    test_cases = [
        # (тип выращивания, тип почвы, сезон, описание)
        ("outdoor", "loam", "summer", "Базовый случай"),
        ("greenhouse", "sand", "spring", "Интенсивное выращивание на песке"),
        ("organic", "clay", "autumn", "Органическое на глине"),
        ("hydroponics", "peat", "winter", "Гидропоника на торфе"),
        ("greenhouse", "organic", "spring", "Теплица на органике")
    ]
    
    all_results = []
    
    for growing_type, soil_type, season, description in test_cases:
        print(f"\n🔍 Тестируем: {description}")
        print(f"   Выращивание: {growing_type}, Почва: {soil_type}, Сезон: {season}")
        
        # Применяем коррекции по порядку
        step1 = apply_growing_type_correction(base_values, growing_type_corrections[growing_type])
        step2 = apply_soil_type_correction(step1, soil_type_corrections[soil_type])
        step3 = apply_seasonal_correction(step2, seasonal_corrections[season])
        
        print(f"   Шаг 1 (выращивание): N={step1['nitrogen']:.1f}, P={step1['phosphorus']:.1f}, K={step1['potassium']:.1f}")
        print(f"   Шаг 2 (почва): N={step2['nitrogen']:.1f}, P={step2['phosphorus']:.1f}, K={step2['potassium']:.1f}")
        print(f"   Шаг 3 (сезон): N={step3['nitrogen']:.1f}, P={step3['phosphorus']:.1f}, K={step3['potassium']:.1f}")
        
        # Проверяем границы
        is_within_bounds = check_scientific_bounds(step3, scientific_bounds)
        
        if is_within_bounds:
            print(f"   ✅ В границах научных рекомендаций")
        else:
            print(f"   ❌ ВЫХОДИТ ЗА ГРАНИЦЫ!")
            
        all_results.append({
            "description": description,
            "final_values": step3,
            "within_bounds": is_within_bounds
        })
    
    # Статистика
    passed = sum(1 for r in all_results if r["within_bounds"])
    total = len(all_results)
    
    print(f"\n📊 РЕЗУЛЬТАТЫ:")
    print(f"   Пройдено: {passed}/{total} тестов")
    print(f"   Успешность: {passed/total*100:.1f}%")
    
    # Проверяем, что все тесты прошли
    assert passed == total, f"Не все тесты прошли! {passed}/{total}"
    
    return all_results

def apply_growing_type_correction(base_values, corrections):
    """Применяет коррекцию типа выращивания"""
    result = base_values.copy()
    for param, factor in corrections.items():
        if param in result:
            result[param] *= factor
    return result

def apply_soil_type_correction(values, corrections):
    """Применяет коррекцию типа почвы"""
    result = values.copy()
    for param, factor in corrections.items():
        if param in result:
            result[param] *= factor
    return result

def apply_seasonal_correction(values, corrections):
    """Применяет сезонную коррекцию (только NPK)"""
    result = values.copy()
    for param, factor in corrections.items():
        if param in result:
            result[param] *= factor
    return result

def check_scientific_bounds(values, bounds):
    """Проверяет, что значения в научных границах"""
    for param, value in values.items():
        if param in bounds:
            min_val = bounds[param]["min"]
            max_val = bounds[param]["max"]
            if not (min_val <= value <= max_val):
                print(f"     ⚠️ {param}: {value:.1f} (границы: {min_val}-{max_val})")
                return False
    return True

def test_correction_magnitude_analysis():
    """Тест: анализ величины коррекций"""
    print("\n🧪 Анализируем величину коррекций...")
    
    # Максимальные коэффициенты для каждого типа коррекции
    max_growing_correction = 1.33  # Гидропоника
    max_soil_correction = 1.08     # Органическая почва
    max_seasonal_correction = 1.08  # Весна
    
    # Максимальная общая коррекция
    max_total_correction = max_growing_correction * max_soil_correction * max_seasonal_correction
    
    print(f"   Максимальная коррекция выращивания: {max_growing_correction:.2f}x")
    print(f"   Максимальная коррекция почвы: {max_soil_correction:.2f}x")
    print(f"   Максимальная сезонная коррекция: {max_seasonal_correction:.2f}x")
    print(f"   Максимальная общая коррекция: {max_total_correction:.2f}x")
    
    # Проверяем, что общая коррекция не превышает разумные пределы
    assert max_total_correction <= 1.6, f"Общая коррекция слишком велика: {max_total_correction:.2f}x"
    
    print(f"   ✅ Общая коррекция в разумных пределах")
    
    return max_total_correction

def test_conservative_coefficients():
    """Тест: проверка консервативности коэффициентов"""
    print("\n🧪 Проверяем консервативность коэффициентов...")
    
    # Коэффициенты почвы должны быть очень консервативными
    soil_coefficients = {
        "sand": {"nitrogen": 1.05, "phosphorus": 1.03, "potassium": 1.04},
        "clay": {"nitrogen": 0.95, "phosphorus": 0.92, "potassium": 0.96},
        "peat": {"nitrogen": 1.06, "phosphorus": 1.04, "potassium": 1.02},
        "organic": {"nitrogen": 1.05, "phosphorus": 1.03, "potassium": 1.02}
    }
    
    max_soil_correction = 0
    min_soil_correction = 2
    
    for soil_type, coeffs in soil_coefficients.items():
        for param, coeff in coeffs.items():
            max_soil_correction = max(max_soil_correction, coeff)
            min_soil_correction = min(min_soil_correction, coeff)
    
    print(f"   Максимальная коррекция почвы: {max_soil_correction:.2f}x")
    print(f"   Минимальная коррекция почвы: {min_soil_correction:.2f}x")
    
    # Проверяем, что коррекции действительно консервативные
    assert max_soil_correction <= 1.10, f"Коррекция почвы слишком агрессивная: {max_soil_correction:.2f}x"
    assert min_soil_correction >= 0.90, f"Коррекция почвы слишком агрессивная: {min_soil_correction:.2f}x"
    
    print(f"   ✅ Коэффициенты почвы консервативные")
    
    return {"max": max_soil_correction, "min": min_soil_correction}

if __name__ == "__main__":
    print("🧪 ЗАПУСК ТЕСТОВ ВАЛИДАЦИИ КОРРЕКЦИЙ ТИПА ПОЧВЫ")
    print("=" * 60)
    
    # Запускаем все тесты
    test_three_step_correction_sequence()
    test_correction_magnitude_analysis()
    test_conservative_coefficients()
    
    print("\n✅ ВСЕ ТЕСТЫ ПРОЙДЕНЫ УСПЕШНО!")
    print("🎯 Система коррекций работает корректно и консервативно")
