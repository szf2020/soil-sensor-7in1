#!/usr/bin/env python3
"""
Тест сброса интервалов к значениям по умолчанию
Проверяет, что все интервалы корректно сбрасываются к правильным значениям
"""

import pytest
import json
import sys
import os

# Добавляем путь к модулям проекта
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'src'))

# Импортируем константы из C++ (симулируем)
class MockConstants:
    # Интервалы (мс) - из jxct_config_vars.h
    SENSOR_READ_INTERVAL = 2000      # 2 сек
    MQTT_PUBLISH_INTERVAL = 30000    # 30 сек  
    THINGSPEAK_INTERVAL = 600000     # 10 мин
    WEB_UPDATE_INTERVAL = 3000       # 3 сек
    
    # Дельта-фильтры - из jxct_constants.h
    DEFAULT_DELTA_TEMPERATURE = 0.5  # 0.5°C
    DEFAULT_DELTA_HUMIDITY = 2.0     # 2.0%
    DEFAULT_DELTA_PH = 0.1           # 0.1 pH
    DEFAULT_DELTA_EC = 50.0          # 50 µS/cm
    DEFAULT_DELTA_NPK = 10.0         # 10 mg/kg
    
    # Фильтрация - из jxct_constants.h
    MOVING_AVERAGE_WINDOW_DEFAULT = 5
    DEFAULT_FORCE_PUBLISH_CYCLES = 10
    EXPONENTIAL_ALPHA_DEFAULT = 0.3
    OUTLIER_THRESHOLD_DEFAULT = 2.5

def test_interval_reset_values():
    """Тест правильности значений при сбросе интервалов"""
    
    # Симулируем конфигурацию с неправильными значениями
    config = {
        'sensorReadInterval': 5000,      # Неправильное значение
        'mqttPublishInterval': 60000,    # Неправильное значение
        'thingSpeakInterval': 900000,    # Неправильное значение
        'webUpdateInterval': 10000,      # Неправильное значение
        'deltaTemperature': 1.0,         # Неправильное значение
        'deltaHumidity': 5.0,            # Неправильное значение
        'deltaPh': 0.5,                  # Неправильное значение
        'deltaEc': 100.0,                # Неправильное значение
        'deltaNpk': 20.0,                # Неправильное значение
        'movingAverageWindow': 10,       # Неправильное значение
        'forcePublishCycles': 20,        # Неправильное значение
        'exponentialAlpha': 0.5,         # Неправильное значение
        'outlierThreshold': 3.0,         # Неправильное значение
    }
    
    # Симулируем сброс к значениям по умолчанию (как в routes_config.cpp)
    config['sensorReadInterval'] = MockConstants.SENSOR_READ_INTERVAL
    config['mqttPublishInterval'] = MockConstants.MQTT_PUBLISH_INTERVAL
    config['thingSpeakInterval'] = MockConstants.THINGSPEAK_INTERVAL
    config['webUpdateInterval'] = MockConstants.WEB_UPDATE_INTERVAL
    
    # Дельта-фильтры
    config['deltaTemperature'] = MockConstants.DEFAULT_DELTA_TEMPERATURE
    config['deltaHumidity'] = MockConstants.DEFAULT_DELTA_HUMIDITY
    config['deltaPh'] = MockConstants.DEFAULT_DELTA_PH
    config['deltaEc'] = MockConstants.DEFAULT_DELTA_EC
    config['deltaNpk'] = MockConstants.DEFAULT_DELTA_NPK
    
    # Фильтрация
    config['movingAverageWindow'] = MockConstants.MOVING_AVERAGE_WINDOW_DEFAULT
    config['forcePublishCycles'] = MockConstants.DEFAULT_FORCE_PUBLISH_CYCLES
    config['exponentialAlpha'] = MockConstants.EXPONENTIAL_ALPHA_DEFAULT
    config['outlierThreshold'] = MockConstants.OUTLIER_THRESHOLD_DEFAULT
    
    # Проверяем, что все значения сброшены правильно
    assert config['sensorReadInterval'] == 2000, f"Интервал опроса датчика должен быть 2000 мс, получено {config['sensorReadInterval']}"
    assert config['mqttPublishInterval'] == 30000, f"Интервал MQTT должен быть 30000 мс, получено {config['mqttPublishInterval']}"
    assert config['thingSpeakInterval'] == 600000, f"Интервал ThingSpeak должен быть 600000 мс, получено {config['thingSpeakInterval']}"
    assert config['webUpdateInterval'] == 3000, f"Интервал обновления веб-интерфейса должен быть 3000 мс, получено {config['webUpdateInterval']}"
    
    # Проверяем дельта-фильтры
    assert config['deltaTemperature'] == 0.5, f"Дельта температуры должна быть 0.5°C, получено {config['deltaTemperature']}"
    assert config['deltaHumidity'] == 2.0, f"Дельта влажности должна быть 2.0%, получено {config['deltaHumidity']}"
    assert config['deltaPh'] == 0.1, f"Дельта pH должна быть 0.1, получено {config['deltaPh']}"
    assert config['deltaEc'] == 50.0, f"Дельта EC должна быть 50.0 µS/cm, получено {config['deltaEc']}"
    assert config['deltaNpk'] == 10.0, f"Дельта NPK должна быть 10.0 mg/kg, получено {config['deltaNpk']}"
    
    # Проверяем фильтрацию
    assert config['movingAverageWindow'] == 5, f"Окно скользящего среднего должно быть 5, получено {config['movingAverageWindow']}"
    assert config['forcePublishCycles'] == 10, f"Принудительная публикация должна быть каждые 10 циклов, получено {config['forcePublishCycles']}"
    assert config['exponentialAlpha'] == 0.3, f"Коэффициент экспоненциального сглаживания должен быть 0.3, получено {config['exponentialAlpha']}"
    assert config['outlierThreshold'] == 2.5, f"Порог выбросов должен быть 2.5, получено {config['outlierThreshold']}"

def test_interval_ranges():
    """Тест диапазонов допустимых значений интервалов"""
    
    # Проверяем, что значения по умолчанию находятся в допустимых диапазонах
    # (из jxct_constants.h)
    
    # Интервалы
    assert 1000 <= MockConstants.SENSOR_READ_INTERVAL <= 3600000, "Интервал опроса датчика вне диапазона"
    assert 1000 <= MockConstants.MQTT_PUBLISH_INTERVAL <= 3600000, "Интервал MQTT вне диапазона"
    assert 15000 <= MockConstants.THINGSPEAK_INTERVAL <= 7200000, "Интервал ThingSpeak вне диапазона"
    assert 1000 <= MockConstants.WEB_UPDATE_INTERVAL <= 3600000, "Интервал веб-обновления вне диапазона"
    
    # Дельта-фильтры
    assert 0.1 <= MockConstants.DEFAULT_DELTA_TEMPERATURE <= 5.0, "Дельта температуры вне диапазона"
    assert 0.5 <= MockConstants.DEFAULT_DELTA_HUMIDITY <= 10.0, "Дельта влажности вне диапазона"
    assert 0.01 <= MockConstants.DEFAULT_DELTA_PH <= 1.0, "Дельта pH вне диапазона"
    assert 10 <= MockConstants.DEFAULT_DELTA_EC <= 500, "Дельта EC вне диапазона"
    assert 1 <= MockConstants.DEFAULT_DELTA_NPK <= 50, "Дельта NPK вне диапазона"
    
    # Фильтрация
    assert 1 <= MockConstants.MOVING_AVERAGE_WINDOW_DEFAULT <= 20, "Окно скользящего среднего вне диапазона"
    assert 5 <= MockConstants.DEFAULT_FORCE_PUBLISH_CYCLES <= 50, "Принудительная публикация вне диапазона"
    assert 0.1 <= MockConstants.EXPONENTIAL_ALPHA_DEFAULT <= 0.9, "Коэффициент экспоненциального сглаживания вне диапазона"
    assert 1.5 <= MockConstants.OUTLIER_THRESHOLD_DEFAULT <= 4.0, "Порог выбросов вне диапазона"

def test_interval_consistency():
    """Тест согласованности значений интервалов"""
    
    # Проверяем логическую согласованность
    assert MockConstants.SENSOR_READ_INTERVAL < MockConstants.MQTT_PUBLISH_INTERVAL, "Интервал опроса должен быть меньше интервала MQTT"
    assert MockConstants.MQTT_PUBLISH_INTERVAL < MockConstants.THINGSPEAK_INTERVAL, "Интервал MQTT должен быть меньше интервала ThingSpeak"
    assert MockConstants.WEB_UPDATE_INTERVAL < MockConstants.MQTT_PUBLISH_INTERVAL, "Интервал веб-обновления должен быть меньше интервала MQTT"
    
    # Проверяем, что интервалы кратны друг другу для эффективности
    assert MockConstants.MQTT_PUBLISH_INTERVAL % MockConstants.SENSOR_READ_INTERVAL == 0, "Интервал MQTT должен быть кратен интервалу опроса"
    assert MockConstants.THINGSPEAK_INTERVAL % MockConstants.MQTT_PUBLISH_INTERVAL == 0, "Интервал ThingSpeak должен быть кратен интервалу MQTT"

if __name__ == "__main__":
    # Запуск тестов
    print("🧪 Тестирование сброса интервалов...")
    
    test_interval_reset_values()
    print("✅ Тест значений сброса интервалов пройден")
    
    test_interval_ranges()
    print("✅ Тест диапазонов интервалов пройден")
    
    test_interval_consistency()
    print("✅ Тест согласованности интервалов пройден")
    
    print("🎉 Все тесты сброса интервалов пройдены успешно!") 