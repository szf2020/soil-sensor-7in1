#!/usr/bin/env python3
"""
Тест ограничения частоты запросов к ThingSpeak
Проверяет логику защиты от спама и ограничения на 1 час при множественных ошибках
"""

import sys
import os
import time
import json
from unittest.mock import Mock, patch, MagicMock


class MockConfig:
    """Мок конфигурации для тестирования"""
    def __init__(self):
        self.flags = Mock()
        self.flags.thingSpeakEnabled = True
        self.thingSpeakInterval = 600000  # 10 минут
        self.thingSpeakApiKey = "TEST_API_KEY_123456789012345"
        self.thingSpeakChannelId = "123456789"


class MockSensorData:
    """Мок данных датчика"""
    def __init__(self):
        self.valid = True
        self.temperature = 25.5
        self.humidity = 60.0
        self.ec = 1500.0
        self.ph = 6.8
        self.nitrogen = 45.0
        self.phosphorus = 30.0
        self.potassium = 25.0


class MockWiFi:
    """Мок WiFi соединения"""
    def __init__(self):
        self.connected = True


class TestThingSpeakRateLimiting:
    """Тест ограничения частоты запросов к ThingSpeak"""
    
    def setup_method(self):
        """Настройка перед каждым тестом"""
        self.mock_config = MockConfig()
        self.mock_sensor_data = MockSensorData()
        self.mock_wifi = MockWiFi()
        
        # Сбрасываем глобальные переменные
        self.last_ts_publish = 0
        self.consecutive_fail_count = 0
        self.last_fail_time = 0
        self.current_time = 0
        
    def test_can_send_to_thingspeak_basic_checks(self):
        """Тест базовых проверок возможности отправки"""
        # Тест 1: ThingSpeak отключен
        self.mock_config.flags.thingSpeakEnabled = False
        assert not self._can_send_to_thingspeak()
        
        # Тест 2: WiFi не подключен
        self.mock_config.flags.thingSpeakEnabled = True
        self.mock_wifi.connected = False
        assert not self._can_send_to_thingspeak()
        
        # Тест 3: Данные датчика невалидны
        self.mock_wifi.connected = True
        self.mock_sensor_data.valid = False
        assert not self._can_send_to_thingspeak()
        
    def test_can_send_to_thingspeak_interval_check(self):
        """Тест проверки интервала отправки"""
        self.mock_sensor_data.valid = True
        
        # Первая отправка должна быть разрешена (last_ts_publish = 0, current_time = 0)
        assert self._can_send_to_thingspeak()
        
        # Сразу после отправки должна быть заблокирована
        self.current_time = 1000  # Устанавливаем время
        self.last_ts_publish = self.current_time
        assert not self._can_send_to_thingspeak()
        
        # Через 5 минут (меньше интервала) должна быть заблокирована
        self.current_time = 300000  # 5 минут
        assert not self._can_send_to_thingspeak()
            
        # Через 11 минут (больше интервала) должна быть разрешена
        self.current_time = 660000  # 11 минут
        assert self._can_send_to_thingspeak()
            
    def test_can_send_to_thingspeak_error_limit(self):
        """Тест ограничения при множественных ошибках"""
        self.mock_sensor_data.valid = True
        self.consecutive_fail_count = 10
        self.last_fail_time = self.current_time
        
        # Сразу после 10 ошибок должна быть заблокирована
        assert not self._can_send_to_thingspeak()
        
        # Через 30 минут (меньше часа) должна быть заблокирована
        self.current_time = 1800000  # 30 минут
        assert not self._can_send_to_thingspeak()
            
        # Через 61 минуту (больше часа) должна быть разрешена
        self.current_time = 3660000  # 61 минута
        assert self._can_send_to_thingspeak()
            
    def test_send_data_to_thingspeak_success(self):
        """Тест успешной отправки данных"""
        self.mock_sensor_data.valid = True
        
        # Мокаем успешный ответ от ThingSpeak
        result = self._send_data_to_thingspeak(success=True)
            
        assert result is True
        assert self.consecutive_fail_count == 0
        assert self.last_fail_time == 0
        
    def test_send_data_to_thingspeak_failure(self):
        """Тест неудачной отправки данных"""
        self.mock_sensor_data.valid = True
        self.current_time = 1000  # Устанавливаем время
        
        # Мокаем неудачный ответ от ThingSpeak
        result = self._send_data_to_thingspeak(success=False)
            
        assert result is False
        assert self.consecutive_fail_count == 1
        assert self.last_fail_time == 1000
        
    def test_send_data_to_thingspeak_error_limit_reached(self):
        """Тест достижения лимита ошибок"""
        self.mock_sensor_data.valid = True
        self.consecutive_fail_count = 9
        self.current_time = 2000  # Устанавливаем время
        
        # Мокаем неудачный ответ от ThingSpeak (10-я ошибка)
        result = self._send_data_to_thingspeak(success=False)
            
        assert result is False
        assert self.consecutive_fail_count == 10
        assert self.last_fail_time == 2000
        
        # Следующая попытка должна быть заблокирована
        assert not self._can_send_to_thingspeak()
        
    def test_send_data_to_thingspeak_recovery_after_success(self):
        """Тест восстановления после успешной отправки"""
        self.mock_sensor_data.valid = True
        self.consecutive_fail_count = 5
        self.last_fail_time = self.current_time
        
        # Мокаем успешный ответ от ThingSpeak
        result = self._send_data_to_thingspeak(success=True)
            
        assert result is True
        assert self.consecutive_fail_count == 0
        assert self.last_fail_time == 0
        
        # Следующая попытка должна быть разрешена
        assert self._can_send_to_thingspeak()
        
    def _can_send_to_thingspeak(self):
        """Упрощенная версия функции canSendToThingSpeak для тестирования"""
        # Проверки
        if not self.mock_config.flags.thingSpeakEnabled:
            return False
        if not self.mock_wifi.connected:
            return False
        if not self.mock_sensor_data.valid:
            return False

        current_time = self.current_time
        
        # Проверяем ограничение на 1 час при множественных ошибках
        if (self.consecutive_fail_count >= 10 and 
            (current_time - self.last_fail_time) < 3600000):  # 1 час = 3600000 мс
            return False
        
        # Проверяем обычный интервал отправки
        # Если last_ts_publish = 0, это первая отправка, разрешаем
        if self.last_ts_publish > 0 and (current_time - self.last_ts_publish) < self.mock_config.thingSpeakInterval:
            return False

        return True
        
    def _send_data_to_thingspeak(self, success=True):
        """Упрощенная версия функции sendDataToThingSpeak для тестирования"""
        # Проверяем возможность отправки
        if not self._can_send_to_thingspeak():
            return False

        # Мокаем отправку данных
        res = 200 if success else -401
        
        if res == 200:
            self.last_ts_publish = self.current_time
            self.consecutive_fail_count = 0
            self.last_fail_time = 0
            return True
        else:
            self.consecutive_fail_count += 1
            self.last_fail_time = self.current_time
            
            # Если слишком много ошибок подряд, устанавливаем ограничение на 1 час
            if self.consecutive_fail_count >= 10:
                pass  # Ограничение уже установлено
                
            return False


def run_thingspeak_rate_limiting_tests():
    """Запуск всех тестов ограничения частоты ThingSpeak"""
    print("🧪 Тестирование ограничения частоты запросов к ThingSpeak")
    print("=" * 60)
    
    test_instance = TestThingSpeakRateLimiting()
    test_methods = [method for method in dir(test_instance) 
                   if method.startswith('test_')]
    
    passed = 0
    failed = 0
    
    for method_name in test_methods:
        try:
            test_instance.setup_method()
            getattr(test_instance, method_name)()
            print(f"✅ {method_name}: PASS")
            passed += 1
        except Exception as e:
            import traceback
            print(f"❌ {method_name}: FAIL - {e}")
            print(f"   Подробности: {traceback.format_exc()}")
            failed += 1
    
    print("=" * 60)
    print(f"📊 Результаты: {passed} PASS, {failed} FAIL")
    
    if failed == 0:
        print("🎉 ВСЕ ТЕСТЫ ОГРАНИЧЕНИЯ ЧАСТОТЫ ПРОЙДЕНЫ УСПЕШНО!")
        return True
    else:
        print("⚠️ Некоторые тесты не прошли")
        return False


if __name__ == "__main__":
    success = run_thingspeak_rate_limiting_tests()
    sys.exit(0 if success else 1) 