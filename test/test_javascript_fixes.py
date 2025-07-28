#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Тест для проверки исправлений JavaScript
"""

import sys
import re

def test_javascript_safety_checks():
    """Тест безопасных проверок в JavaScript"""
    print("🛡️ Тестируем безопасные проверки в JavaScript...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем безопасные проверки для d.alerts
        if '(d.alerts && d.alerts.length>0)' in content:
            print("✅ Безопасная проверка d.alerts найдена")
        else:
            print("❌ Безопасная проверка d.alerts НЕ найдена")
            return False
            
        # Проверяем безопасные проверки для parseFloat
        safe_patterns = [
            'parseFloat(d.temperature||0)',
            'parseFloat(d.humidity||0)',
            'parseFloat(d.ec||0)',
            'parseFloat(d.ph||0)',
            'parseFloat(d.nitrogen||0)',
            'parseFloat(d.phosphorus||0)',
            'parseFloat(d.potassium||0)'
        ]
        
        for pattern in safe_patterns:
            if pattern in content:
                print(f"✅ Безопасная проверка {pattern} найдена")
            else:
                print(f"❌ Безопасная проверка {pattern} НЕ найдена")
                return False
                
        # Проверяем безопасные проверки для рекомендаций
        rec_patterns = [
            'parseFloat(d.rec_temperature||0)',
            'parseFloat(d.rec_humidity||0)',
            'parseFloat(d.rec_ec||0)',
            'parseFloat(d.rec_ph||0)',
            'parseFloat(d.rec_nitrogen||0)',
            'parseFloat(d.rec_phosphorus||0)',
            'parseFloat(d.rec_potassium||0)'
        ]
        
        for pattern in rec_patterns:
            if pattern in content:
                print(f"✅ Безопасная проверка для рекомендаций {pattern} найдена")
            else:
                print(f"❌ Безопасная проверка для рекомендаций {pattern} НЕ найдена")
                return False
                
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_color_application():
    """Тест применения цветов"""
    print("\n🎨 Тестируем применение цветов...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем применение цветов к компенсированным значениям
        color_patterns = [
            "applyColor('temp', colorRange(ct, limits.temp.min, limits.temp.max))",
            "applyColor('hum',  colorRange(ch, limits.hum.min, limits.hum.max))",
            "applyColor('ec',   colorRange(ce, limits.ec.min, limits.ec.max))",
            "applyColor('ph',   colorRange(cph, limits.ph.min, limits.ph.max))",
            "applyColor('n',    colorRange(cn, limits.n.min, limits.n.max))",
            "applyColor('p',    colorRange(cp, limits.p.min, limits.p.max))",
            "applyColor('k',    colorRange(ck, limits.k.min, limits.k.max))"
        ]
        
        for pattern in color_patterns:
            if pattern in content:
                print(f"✅ Цвет для компенсированных значений найден: {pattern.split('(')[1].split(',')[0]}")
            else:
                print(f"❌ Цвет для компенсированных значений НЕ найден: {pattern}")
                return False
                
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
                print(f"✅ Цвет для рекомендаций найден: {pattern.split('(')[1].split(',')[0]}")
            else:
                print(f"❌ Цвет для рекомендаций НЕ найден: {pattern}")
                return False
                
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_recommendation_handling():
    """Тест обработки рекомендаций"""
    print("\n💡 Тестируем обработку рекомендаций...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем наличие безопасной обработки рекомендаций
        if 'typeof d.nutrient_interactions === \'string\' && d.nutrient_interactions.length > 0' in content:
            print("✅ Безопасная проверка nutrient_interactions найдена")
        else:
            print("❌ Безопасная проверка nutrient_interactions НЕ найдена")
            return False
            
        if 'typeof d.crop_specific_recommendations === \'string\' && d.crop_specific_recommendations.length > 0' in content:
            print("✅ Безопасная проверка crop_specific_recommendations найдена")
        else:
            print("❌ Безопасная проверка crop_specific_recommendations НЕ найдена")
            return False
            
        # Проверяем наличие сообщений по умолчанию
        if '✅ Антагонизмов питательных веществ не обнаружено' in content:
            print("✅ Сообщение по умолчанию для антагонизмов найдено")
        else:
            print("❌ Сообщение по умолчанию для антагонизмов НЕ найдено")
            return False
            
        if 'ℹ️ Выберите культуру для получения специфических рекомендаций' in content:
            print("✅ Сообщение по умолчанию для культур найдено")
        else:
            print("❌ Сообщение по умолчанию для культур НЕ найдено")
            return False
            
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def test_error_handling():
    """Тест обработки ошибок"""
    print("\n🚨 Тестируем обработку ошибок...")
    
    try:
        with open('src/web/routes_data.cpp', 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Проверяем catch блок
        if '}).catch(err => {' in content:
            print("✅ Catch блок найден")
        else:
            print("❌ Catch блок НЕ найден")
            return False
            
        # Проверяем безопасные проверки в catch блоке
        if 'const interactionsDiv = document.getElementById(\'nutrient-interactions\');' in content and 'if(interactionsDiv)' in content:
            print("✅ Безопасная проверка в catch блоке для interactions найдена")
        else:
            print("❌ Безопасная проверка в catch блоке для interactions НЕ найдена")
            return False
            
        if 'const cropDiv = document.getElementById(\'crop-specific-recommendations\');' in content and 'if(cropDiv)' in content:
            print("✅ Безопасная проверка в catch блоке для crop найдена")
        else:
            print("❌ Безопасная проверка в catch блоке для crop НЕ найдена")
            return False
            
        return True
        
    except Exception as e:
        print(f"❌ Ошибка чтения файла: {e}")
        return False

def main():
    """Основная функция тестирования"""
    print("🚀 ТЕСТ ИСПРАВЛЕНИЙ JAVASCRIPT")
    print("=" * 60)
    
    tests_passed = 0
    total_tests = 4
    
    if test_javascript_safety_checks():
        tests_passed += 1
        
    if test_color_application():
        tests_passed += 1
        
    if test_recommendation_handling():
        tests_passed += 1
        
    if test_error_handling():
        tests_passed += 1
    
    print("\n" + "=" * 60)
    print(f"📊 РЕЗУЛЬТАТЫ: {tests_passed}/{total_tests} тестов пройдено")
    
    if tests_passed == total_tests:
        print("🎉 ВСЕ ИСПРАВЛЕНИЯ ПРИМЕНЕНЫ!")
        print("\n💡 ИСПРАВЛЕННЫЕ ПРОБЛЕМЫ:")
        print("1. ✅ Добавлены безопасные проверки для d.alerts")
        print("2. ✅ Добавлены безопасные проверки для parseFloat")
        print("3. ✅ Восстановлены цвета для таблицы")
        print("4. ✅ Исправлена обработка рекомендаций")
        print("5. ✅ Добавлена безопасная обработка ошибок")
        return True
    else:
        print("❌ НЕКОТОРЫЕ ИСПРАВЛЕНИЯ НЕ ПРИМЕНЕНЫ!")
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1) 