#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Тест для проверки восстановления оригинальной цветовой схемы
"""

import sys

def test_original_color_functions():
    """Тест оригинальных функций цветовой схемы"""
    print("🎨 Тестируем оригинальные функции цветовой схемы...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем наличие оригинальных функций
        required_functions = [
            'function colorDelta(a,b)',
            'function colorRange(v,min,max)',
            'function applyColor(spanId,cls)'
        ]
        
        for func in required_functions:
            if func in content:
                print(f"✅ Оригинальная функция найдена: {func}")
            else:
                print(f"❌ Оригинальная функция НЕ найдена: {func}")
                return False
                
        # Проверяем логику colorDelta для рекомендаций
        if "if(diff>30)return 'red';if(diff>20)return 'orange';if(diff>10)return 'yellow'" in content:
            print("✅ Логика colorDelta для рекомендаций найдена")
        else:
            print("❌ Логика colorDelta для рекомендаций НЕ найдена")
            return False
            
        # Проверяем логику colorRange для RAW значений
        if "if(v<min||v>max)return 'red'" in content:
            print("✅ Логика colorRange для RAW значений найдена")
        else:
            print("❌ Логика colorRange для RAW значений НЕ найдена")
            return False
            
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_raw_values_coloring():
    """Тест окрашивания RAW значений"""
    print("\n📊 Тестируем окрашивание RAW значений...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем применение цветов к RAW значениям
        raw_color_patterns = [
            "applyColor('temp_raw',colorRange(tvr,limits.temp.min,limits.temp.max))",
            "applyColor('hum_raw',colorRange(hvr,limits.hum.min,limits.hum.max))",
            "applyColor('ec_raw',colorRange(evr,limits.ec.min,limits.ec.max))",
            "applyColor('ph_raw',colorRange(pvr,limits.ph.min,limits.ph.max))",
            "applyColor('n_raw',colorRange(nvr,limits.n.min,limits.n.max))",
            "applyColor('p_raw',colorRange(p2r,limits.p.min,limits.p.max))",
            "applyColor('k_raw',colorRange(kvr,limits.k.min,limits.k.max))"
        ]
        
        for pattern in raw_color_patterns:
            if pattern in content:
                param = pattern.split("'")[1].replace('_raw', '')
                print(f"✅ RAW окрашивание найдено: {param}")
            else:
                print(f"❌ RAW окрашивание НЕ найдено: {pattern}")
                return False
                
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_compensated_values_no_coloring():
    """Тест отсутствия окрашивания компенсированных значений"""
    print("\n⚪ Тестируем отсутствие окрашивания компенсированных значений...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем что компенсированные значения НЕ окрашиваются
        compensated_color_patterns = [
            "applyColor('temp', colorRange(ct,",
            "applyColor('hum', colorRange(ch,",
            "applyColor('ec', colorRange(ce,",
            "applyColor('ph', colorRange(cph,",
            "applyColor('n', colorRange(cn,",
            "applyColor('p', colorRange(cp,",
            "applyColor('k', colorRange(ck,"
        ]
        
        found_compensated_coloring = False
        for pattern in compensated_color_patterns:
            if pattern in content:
                found_compensated_coloring = True
                print(f"❌ Компенсированное значение ОКРАШИВАЕТСЯ (не должно): {pattern}")
                
        if not found_compensated_coloring:
            print("✅ Компенсированные значения НЕ окрашиваются (правильно)")
            
        # Проверяем наличие комментария
        if "Компенсированные значения БЕЗ ПОКРАСКИ (как было раньше)" in content:
            print("✅ Комментарий о БЕЗ ПОКРАСКИ найден")
        else:
            print("❌ Комментарий о БЕЗ ПОКРАСКИ НЕ найден")
            return False
            
        return not found_compensated_coloring
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_recommendations_coloring():
    """Тест окрашивания рекомендаций"""
    print("\n🎯 Тестируем окрашивание рекомендаций...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем применение цветов к рекомендациям
        rec_color_patterns = [
            "applyColor('temp_rec', colorDelta(ct, parseFloat(d.rec_temperature||0)))",
            "applyColor('hum_rec',  colorDelta(ch, parseFloat(d.rec_humidity||0)))",
            "applyColor('ec_rec',   colorDelta(ce, parseFloat(d.rec_ec||0)))",
            "applyColor('ph_rec',   colorDelta(cph,parseFloat(d.rec_ph||0)))",
            "applyColor('n_rec',    colorDelta(cn, parseFloat(d.rec_nitrogen||0)))",
            "applyColor('p_rec',    colorDelta(cp, parseFloat(d.rec_phosphorus||0)))",
            "applyColor('k_rec',    colorDelta(ck, parseFloat(d.rec_potassium||0)))"
        ]
        
        for pattern in rec_color_patterns:
            if pattern in content:
                param = pattern.split("'")[1].replace('_rec', '')
                print(f"✅ Рекомендации окрашивание найдено: {param}")
            else:
                print(f"❌ Рекомендации окрашивание НЕ найдено: {pattern}")
                return False
                
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_css_colors():
    """Тест CSS цветов"""
    print("\n🎨 Тестируем CSS цвета...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем наличие всех цветов
        required_colors = [
            ".green{color:#4CAF50}",
            ".yellow{color:#FFC107}",
            ".orange{color:#FF9800}",
            ".red{color:#F44336}",
            ".blue{color:#2196F3}"
        ]
        
        for color in required_colors:
            if color in content:
                color_name = color.split('{')[0].replace('.', '')
                print(f"✅ CSS цвет найден: {color_name}")
            else:
                print(f"❌ CSS цвет НЕ найден: {color}")
                return False
                
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def main():
    """Основная функция тестирования"""
    print("🚀 ТЕСТ ВОССТАНОВЛЕНИЯ ОРИГИНАЛЬНОЙ ЦВЕТОВОЙ СХЕМЫ")
    print("=" * 70)
    
    tests_passed = 0
    total_tests = 5
    
    if test_original_color_functions():
        tests_passed += 1
        
    if test_raw_values_coloring():
        tests_passed += 1
        
    if test_compensated_values_no_coloring():
        tests_passed += 1
        
    if test_recommendations_coloring():
        tests_passed += 1
        
    if test_css_colors():
        tests_passed += 1
    
    print("\n" + "=" * 70)
    print(f"📊 РЕЗУЛЬТАТЫ: {tests_passed}/{total_tests} тестов пройдено")
    
    if tests_passed == total_tests:
        print("🎉 ОРИГИНАЛЬНАЯ ЦВЕТОВАЯ СХЕМА ВОССТАНОВЛЕНА!")
        print("\n✅ ВОССТАНОВЛЕННАЯ СХЕМА:")
        print("🟢 Зеленый: оптимальные условия измерения")
        print("🟠 Оранжевый: неоптимальные условия (влажность <25%, температура <5°C или >40°C)")
        print("🔵 Синий: полив активен (временная невалидность)")
        print("🔴 Красный: ошибки датчика (выход за физические пределы)")
        print("\n📋 ЛОГИКА ОКРАШИВАНИЯ:")
        print("• RAW значения: относительно пределов датчика (colorRange)")
        print("• Компенсированные: БЕЗ ПОКРАСКИ")
        print("• Рекомендации: относительно компенсированных значений (colorDelta)")
        return True
    else:
        print("❌ ЦВЕТОВАЯ СХЕМА НЕ ПОЛНОСТЬЮ ВОССТАНОВЛЕНА!")
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1) 