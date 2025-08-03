#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Тест логики окраски влажности RAW значений
Проверяет что влажность выше 53% окрашивается в желтый цвет
"""

import sys
import os
from pathlib import Path

# Добавляем путь к проекту
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

# Настройка кодировки для корректного отображения русских символов
if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8')
if hasattr(sys.stderr, 'reconfigure'):
    sys.stderr.reconfigure(encoding='utf-8')

def colorSensorRange(value, sensorType):
    """Симуляция функции colorSensorRange из routes_data.cpp"""
    ranges = {
        'temp': {'min': -40, 'max': 80, 'precision': '±0.5°C'},
        'hum': {'min': 0, 'max': 100, 'precision': '±3%RH'},
        'ph': {'min': 3, 'max': 9, 'precision': '±0.3pH'},
        'ec': {'min': 0, 'max': 20000, 'precision': '±2-5%'},
        'n': {'min': 0, 'max': 1999, 'precision': '2%'},
        'p': {'min': 0, 'max': 1999, 'precision': '2%'},
        'k': {'min': 0, 'max': 1999, 'precision': '2%'}
    }
    range_info = ranges[sensorType]
    if not range_info:
        return ''
    if value < range_info['min'] or value > range_info['max']:
        return 'red'
    # ОСОБАЯ ЛОГИКА ДЛЯ ВЛАЖНОСТИ: желтый выше 53%
    if sensorType == 'hum' and value > 53:
        return 'yellow'
    return 'green'

def test_humidity_coloring():
    """Тестирует логику окраски влажности"""
    print("Тестирование логики окраски влажности...")
    
    # Тест 1: Влажность ниже 0% - красный
    result = colorSensorRange(-5, 'hum')
    expected = 'red'
    print(f"Влажность -5%: {result} (ожидается: {expected})")
    assert result == expected, f"Ожидается {expected}, получено {result}"
    
    # Тест 2: Влажность 0% - зеленый
    result = colorSensorRange(0, 'hum')
    expected = 'green'
    print(f"Влажность 0%: {result} (ожидается: {expected})")
    assert result == expected, f"Ожидается {expected}, получено {result}"
    
    # Тест 3: Влажность 25% - зеленый
    result = colorSensorRange(25, 'hum')
    expected = 'green'
    print(f"Влажность 25%: {result} (ожидается: {expected})")
    assert result == expected, f"Ожидается {expected}, получено {result}"
    
    # Тест 4: Влажность 53% - зеленый (граничное значение)
    result = colorSensorRange(53, 'hum')
    expected = 'green'
    print(f"Влажность 53%: {result} (ожидается: {expected})")
    assert result == expected, f"Ожидается {expected}, получено {result}"
    
    # Тест 5: Влажность 54% - желтый (выше 53%)
    result = colorSensorRange(54, 'hum')
    expected = 'yellow'
    print(f"Влажность 54%: {result} (ожидается: {expected})")
    assert result == expected, f"Ожидается {expected}, получено {result}"
    
    # Тест 6: Влажность 75% - желтый
    result = colorSensorRange(75, 'hum')
    expected = 'yellow'
    print(f"Влажность 75%: {result} (ожидается: {expected})")
    assert result == expected, f"Ожидается {expected}, получено {result}"
    
    # Тест 7: Влажность 100% - желтый
    result = colorSensorRange(100, 'hum')
    expected = 'yellow'
    print(f"Влажность 100%: {result} (ожидается: {expected})")
    assert result == expected, f"Ожидается {expected}, получено {result}"
    
    # Тест 8: Влажность выше 100% - красный
    result = colorSensorRange(105, 'hum')
    expected = 'red'
    print(f"Влажность 105%: {result} (ожидается: {expected})")
    assert result == expected, f"Ожидается {expected}, получено {result}"
    
    print("Все тесты окраски влажности пройдены!")

def test_other_sensors():
    """Тестирует что другие датчики не имеют желтой окраски"""
    print("\nТестирование других датчиков (не должны иметь желтую окраску)...")
    
    sensors = ['temp', 'ph', 'ec', 'n', 'p', 'k']
    
    for sensor in sensors:
        # Тестируем значения в рабочем диапазоне
        if sensor == 'temp':
            result = colorSensorRange(25, sensor)
        elif sensor == 'ph':
            result = colorSensorRange(6.5, sensor)
        elif sensor == 'ec':
            result = colorSensorRange(1500, sensor)
        else:  # n, p, k
            result = colorSensorRange(200, sensor)
        
        expected = 'green'
        print(f"{sensor.upper()} в диапазоне: {result} (ожидается: {expected})")
        assert result == expected, f"Ожидается {expected}, получено {result}"
    
    print("Все тесты других датчиков пройдены!")

if __name__ == "__main__":
    test_humidity_coloring()
    test_other_sensors()
    print("\nУСПЕХ: Все тесты пройдены!") 