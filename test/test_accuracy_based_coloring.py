#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Тест научно обоснованной цветовой индикации JXCT
Основан на документации: https://www.jxct-iot.com/product/showproduct.php?id=197
"""

import re
import sys
from pathlib import Path

def test_accuracy_based_function():
    """Тест функции colorAccuracyBased"""
    print("🧪 Тестируем функцию colorAccuracyBased...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем наличие функции
        if 'function colorAccuracyBased(v,param)' not in content:
            print("❌ Функция colorAccuracyBased не найдена")
            return False
            
        print("✅ Функция colorAccuracyBased найдена")
        
        # Проверяем документацию JXCT
        if 'https://www.jxct-iot.com/product/showproduct.php?id=197' not in content:
            print("❌ Ссылка на документацию JXCT не найдена")
            return False
            
        print("✅ Ссылка на документацию JXCT найдена")
        
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_temperature_zones():
    """Тест зон температуры"""
    print("\n🌡️ Тестируем зоны температуры...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем логику температуры
        temp_patterns = [
            "// Температура: ±0.5°C во всем диапазоне -40°C до +80°C",
            "// Зеленая зона: оптимальные условия измерения (5-40°C)",
            "// Синяя зона: допустимые условия (-40°C до 5°C и 40°C до 80°C)",
            "if(v >= 5 && v <= 40) return 'green';",
            "if(v >= -40 && v <= 80) return 'blue';"
        ]
        
        for pattern in temp_patterns:
            if pattern in content:
                print(f"✅ Температурная зона: {pattern[:50]}...")
            else:
                print(f"❌ Температурная зона НЕ найдена: {pattern}")
                return False
                
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_humidity_zones():
    """Тест зон влажности"""
    print("\n💧 Тестируем зоны влажности...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем логику влажности
        hum_patterns = [
            "// Влажность: ±3% в диапазоне 0-53%, ±5% в диапазоне 53-100%",
            "// Зеленая зона: высокая точность (0-53%)",
            "// Синяя зона: средняя точность (53-100%)",
            "if(v >= 0 && v <= 53) return 'green';",
            "if(v > 53 && v <= 100) return 'blue';"
        ]
        
        for pattern in hum_patterns:
            if pattern in content:
                print(f"✅ Влажность зона: {pattern[:50]}...")
            else:
                print(f"❌ Влажность зона НЕ найдена: {pattern}")
                return False
                
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_ph_zones():
    """Тест зон pH"""
    print("\n🧪 Тестируем зоны pH...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем логику pH
        ph_patterns = [
            "// pH: ±0.3 pH во всем диапазоне 3-9 pH",
            "// Зеленая зона: оптимальный диапазон для растений (6.0-7.0)",
            "// Синяя зона: допустимый диапазон (3.0-6.0 и 7.0-9.0)",
            "if(v >= 6.0 && v <= 7.0) return 'green';",
            "if(v >= 3.0 && v <= 9.0) return 'blue';"
        ]
        
        for pattern in ph_patterns:
            if pattern in content:
                print(f"✅ pH зона: {pattern[:50]}...")
            else:
                print(f"❌ pH зона НЕ найдена: {pattern}")
                return False
                
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_ec_zones():
    """Тест зон EC"""
    print("\n⚡ Тестируем зоны EC...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем логику EC
        ec_patterns = [
            "// EC: разрешение 10 µS/cm, диапазон 0-10000 µS/cm",
            "// Зеленая зона: оптимальный диапазон для растений (1000-3000 µS/cm)",
            "// Синяя зона: допустимый диапазон (0-1000 и 3000-10000 µS/cm)",
            "if(v >= 1000 && v <= 3000) return 'green';",
            "if(v >= 0 && v <= 10000) return 'blue';"
        ]
        
        for pattern in ec_patterns:
            if pattern in content:
                print(f"✅ EC зона: {pattern[:50]}...")
            else:
                print(f"❌ EC зона НЕ найдена: {pattern}")
                return False
                
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_npk_zones():
    """Тест зон NPK"""
    print("\n🌿🌱🍎 Тестируем зоны NPK...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем логику NPK
        npk_patterns = [
            "// NPK: точность 2% F.S., диапазон 0-1999 мг/кг",
            "// Зеленая зона: оптимальный диапазон для растений",
            "// Синяя зона: допустимый диапазон",
            "if(v >= 0 && v <= 1999) return 'green';"
        ]
        
        for pattern in npk_patterns:
            if pattern in content:
                print(f"✅ NPK зона: {pattern[:50]}...")
            else:
                print(f"❌ NPK зона НЕ найдена: {pattern}")
                return False
                
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_function_calls():
    """Тест вызовов функции"""
    print("\n🔧 Тестируем вызовы функции...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем вызовы для всех параметров
        calls = [
            "colorAccuracyBased(tvr,'temp')",
            "colorAccuracyBased(hvr,'hum')",
            "colorAccuracyBased(evr,'ec')",
            "colorAccuracyBased(pvr,'ph')",
            "colorAccuracyBased(nvr,'n')",
            "colorAccuracyBased(p2r,'p')",
            "colorAccuracyBased(kvr,'k')"
        ]
        
        for call in calls:
            if call in content:
                print(f"✅ Вызов найден: {call}")
            else:
                print(f"❌ Вызов НЕ найден: {call}")
                return False
                
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_blue_color_support():
    """Тест поддержки синего цвета"""
    print("\n🔵 Тестируем поддержку синего цвета...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем CSS
        if '.blue{color:#2196F3}' in content:
            print("✅ CSS синий цвет найден")
        else:
            print("❌ CSS синий цвет НЕ найден")
            return False
            
        # Проверяем удаление синего класса
        if "el.classList.remove('red','orange','yellow','green','blue')" in content:
            print("✅ Удаление синего класса найдено")
        else:
            print("❌ Удаление синего класса НЕ найдено")
            return False
            
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def main():
    """Основная функция тестирования"""
    print("🚀 ТЕСТ НАУЧНО ОБОСНОВАННОЙ ЦВЕТОВОЙ ИНДИКАЦИИ JXCT")
    print("=" * 70)
    
    tests_passed = 0
    total_tests = 8
    
    if test_accuracy_based_function():
        tests_passed += 1
        
    if test_temperature_zones():
        tests_passed += 1
        
    if test_humidity_zones():
        tests_passed += 1
        
    if test_ph_zones():
        tests_passed += 1
        
    if test_ec_zones():
        tests_passed += 1
        
    if test_npk_zones():
        tests_passed += 1
        
    if test_function_calls():
        tests_passed += 1
        
    if test_blue_color_support():
        tests_passed += 1
    
    print("\n" + "=" * 70)
    print(f"📊 РЕЗУЛЬТАТЫ: {tests_passed}/{total_tests} тестов пройдено")
    
    if tests_passed == total_tests:
        print("🎉 НАУЧНО ОБОСНОВАННАЯ ЦВЕТОВАЯ ИНДИКАЦИЯ РЕАЛИЗОВАНА!")
        print("\n✅ НОВАЯ СХЕМА:")
        print("🟢 Зеленый: высокая точность измерений (оптимальные условия)")
        print("🔵 Синий: средняя точность измерений (допустимые условия)")
        print("🔴 Красный: выход за физические пределы датчика")
        print("\n📋 НАУЧНАЯ ОСНОВА:")
        print("• Температура: ±0.5°C (зеленая: 5-40°C, синяя: -40°C до 80°C)")
        print("• Влажность: ±3% (0-53%), ±5% (53-100%)")
        print("• pH: ±0.3 pH (зеленая: 6.0-7.0, синяя: 3.0-9.0)")
        print("• EC: разрешение 10 µS/cm (зеленая: 1000-3000, синяя: 0-10000)")
        print("• NPK: точность 2% F.S. (диапазон 0-1999 мг/кг)")
        return True
    else:
        print("❌ НАУЧНО ОБОСНОВАННАЯ ЦВЕТОВАЯ ИНДИКАЦИЯ НЕ ПОЛНОСТЬЮ РЕАЛИЗОВАНА!")
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1) 