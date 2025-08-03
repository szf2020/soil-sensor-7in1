#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
🔧 ДИАГНОСТИКА РЕГИСТРОВ КАЛИБРОВКИ JXCT
Тест для чтения и анализа регистров калибровки датчика
"""

import sys
import time
from typing import Dict, List, Tuple, Optional

# Принудительно устанавливаем stdout в utf-8 для Windows
if hasattr(sys.stdout, 'encoding') and sys.stdout.encoding and sys.stdout.encoding.lower() not in ['utf-8', 'utf8']:
    try:
        sys.stdout = open(sys.stdout.fileno(), mode='w', encoding='utf-8', buffering=1)
    except Exception:
        pass

class RegisterDiagnostics:
    """Диагностика регистров калибровки JXCT"""
    
    def __init__(self):
        # Заводские значения регистров калибровки
        self.factory_values = {
            0x0064: {"name": "Humidity Offset", "value": 0, "description": "Смещение влажности"},
            0x0065: {"name": "Humidity Multiplier", "value": 1000, "description": "Множитель влажности"},
            0x0066: {"name": "EC Offset", "value": 0, "description": "Смещение EC"},
            0x0067: {"name": "EC Multiplier", "value": 1000, "description": "Множитель EC"},
            0x0063: {"name": "Reset Command", "value": 0, "description": "Команда сброса"},
            0xFFFF: {"name": "Save to EEPROM", "value": 0, "description": "Сохранение в EEPROM"}
        }
        
        # Диапазоны допустимых значений
        self.valid_ranges = {
            0x0064: {"min": -1000, "max": 1000, "unit": "offset"},
            0x0065: {"min": 500, "max": 2000, "unit": "multiplier"},
            0x0066: {"min": -1000, "max": 1000, "unit": "offset"},
            0x0067: {"min": 500, "max": 2000, "unit": "multiplier"}
        }

    def simulate_read_register(self, address: int) -> int:
        """Симуляция чтения регистра (замените на реальную функцию)"""
        # Здесь должна быть реальная функция чтения регистра
        # Пока симулируем возможные проблемные значения
        
        # Симуляция проблемных значений для демонстрации
        if address == 0x0064:  # Humidity Offset
            return 150  # Смещение влажности
        elif address == 0x0065:  # Humidity Multiplier
            return 850  # Пониженный множитель
        elif address == 0x0066:  # EC Offset
            return -200  # Отрицательное смещение EC
        elif address == 0x0067:  # EC Multiplier
            return 750  # Пониженный множитель EC
        else:
            return 0

    def read_all_calibration_registers(self) -> Dict[int, int]:
        """Чтение всех регистров калибровки"""
        print("🔧 ЧТЕНИЕ РЕГИСТРОВ КАЛИБРОВКИ")
        print("=" * 60)
        
        registers = {}
        
        for address in [0x0064, 0x0065, 0x0066, 0x0067]:
            try:
                value = self.simulate_read_register(address)
                registers[address] = value
                
                print(f"📊 Регистр 0x{address:04X}: {value}")
                print(f"   Название: {self.factory_values[address]['name']}")
                print(f"   Описание: {self.factory_values[address]['description']}")
                print(f"   Заводское значение: {self.factory_values[address]['value']}")
                
                # Проверка диапазона
                if address in self.valid_ranges:
                    range_info = self.valid_ranges[address]
                    if range_info["min"] <= value <= range_info["max"]:
                        print(f"   ✅ В допустимом диапазоне: {range_info['min']} - {range_info['max']}")
                    else:
                        print(f"   ❌ ВНЕ диапазона: {range_info['min']} - {range_info['max']}")
                
                print()
                
            except Exception as e:
                print(f"❌ Ошибка чтения регистра 0x{address:04X}: {e}")
                registers[address] = None
        
        return registers

    def analyze_calibration_values(self, registers: Dict[int, int]) -> Dict[str, str]:
        """Анализ значений калибровки"""
        print("🔍 АНАЛИЗ ЗНАЧЕНИЙ КАЛИБРОВКИ")
        print("=" * 60)
        
        analysis = {}
        
        # Анализ влажности
        hum_offset = registers.get(0x0064, 0)
        hum_mult = registers.get(0x0065, 1000)
        
        print(f"💧 ВЛАЖНОСТЬ:")
        print(f"   Смещение: {hum_offset}")
        print(f"   Множитель: {hum_mult}")
        
        if hum_offset != 0:
            print(f"   ⚠️ Ненулевое смещение: {hum_offset}")
            analysis["humidity_offset"] = f"Смещение влажности: {hum_offset}"
        
        if hum_mult < 1000:
            print(f"   ⚠️ Пониженный множитель: {hum_mult} (норма: 1000)")
            analysis["humidity_multiplier"] = f"Пониженный множитель влажности: {hum_mult}"
        elif hum_mult > 1000:
            print(f"   ⚠️ Повышенный множитель: {hum_mult} (норма: 1000)")
            analysis["humidity_multiplier"] = f"Повышенный множитель влажности: {hum_mult}"
        else:
            print(f"   ✅ Множитель в норме: {hum_mult}")
        
        print()
        
        # Анализ EC
        ec_offset = registers.get(0x0066, 0)
        ec_mult = registers.get(0x0067, 1000)
        
        print(f"⚡ EC (ЭЛЕКТРОПРОВОДНОСТЬ):")
        print(f"   Смещение: {ec_offset}")
        print(f"   Множитель: {ec_mult}")
        
        if ec_offset != 0:
            print(f"   ⚠️ Ненулевое смещение: {ec_offset}")
            analysis["ec_offset"] = f"Смещение EC: {ec_offset}"
        
        if ec_mult < 1000:
            print(f"   ⚠️ Пониженный множитель: {ec_mult} (норма: 1000)")
            analysis["ec_multiplier"] = f"Пониженный множитель EC: {ec_mult}"
        elif ec_mult > 1000:
            print(f"   ⚠️ Повышенный множитель: {ec_mult} (норма: 1000)")
            analysis["ec_multiplier"] = f"Повышенный множитель EC: {ec_mult}"
        else:
            print(f"   ✅ Множитель в норме: {ec_mult}")
        
        return analysis

    def generate_factory_reset_code(self) -> str:
        """Генерация кода для сброса к заводским настройкам"""
        print("\n🛠️ КОД ДЛЯ СБРОСА К ЗАВОДСКИМ НАСТРОЙКАМ")
        print("=" * 60)
        
        reset_code = """
// Функция сброса к заводским настройкам
void factoryReset() {
    Serial.println("🔄 Сброс к заводским настройкам...");
    
    // Команда сброса
    writeRegister(0x0063, 0x0001);
    delay(1000);
    
    // Сброс регистров влажности
    writeRegister(0x0064, 0);       // Humidity Offset = 0
    writeRegister(0x0065, 1000);    // Humidity Multiplier = 1000
    
    // Сброс регистров EC
    writeRegister(0x0066, 0);       // EC Offset = 0
    writeRegister(0x0067, 1000);    // EC Multiplier = 1000
    
    // Сохранение в EEPROM
    writeRegister(0xFFFF, 0x00A5);
    delay(500);
    
    Serial.println("✅ Сброс завершен!");
}

// Функция чтения регистра
uint16_t readRegister(uint16_t address) {
    uint8_t response[4];
    uint8_t request[] = {0x01, 0x03, (address >> 8) & 0xFF, address & 0xFF, 0x00, 0x01};
    
    if (modbusMaster.sendRequest(request, sizeof(request))) {
        if (modbusMaster.readResponse(response, sizeof(response))) {
            return (response[3] << 8) | response[4];
        }
    }
    return 0xFFFF; // Ошибка
}

// Функция записи регистра
bool writeRegister(uint16_t address, uint16_t value) {
    uint8_t request[] = {0x01, 0x06, (address >> 8) & 0xFF, address & 0xFF, (value >> 8) & 0xFF, value & 0xFF};
    uint8_t response[8];
    
    if (modbusMaster.sendRequest(request, sizeof(request))) {
        return modbusMaster.readResponse(response, sizeof(response));
    }
    return false;
}
"""
        
        print(reset_code)
        return reset_code

    def generate_calibration_code(self, registers: Dict[int, int]) -> str:
        """Генерация кода для калибровки на основе текущих значений"""
        print("\n🔧 КОД ДЛЯ КАЛИБРОВКИ")
        print("=" * 60)
        
        # Расчет корректирующих значений
        hum_correction = 1000 - registers.get(0x0065, 1000)
        ec_correction = 1000 - registers.get(0x0067, 1000)
        
        calibration_code = f"""
// Код для корректировки калибровки
void calibrateSensor() {{
    Serial.println("🔧 Корректировка калибровки...");
    
    // Текущие значения (проблемные):
    // Humidity Multiplier: {registers.get(0x0065, 1000)}
    // EC Multiplier: {registers.get(0x0067, 1000)}
    
    // Корректировка влажности
    uint16_t new_hum_mult = {registers.get(0x0065, 1000)} + {hum_correction};
    writeRegister(0x0065, new_hum_mult);
    Serial.printf("Humidity Multiplier: %d -> %d\\n", {registers.get(0x0065, 1000)}, new_hum_mult);
    
    // Корректировка EC
    uint16_t new_ec_mult = {registers.get(0x0067, 1000)} + {ec_correction};
    writeRegister(0x0067, new_ec_mult);
    Serial.printf("EC Multiplier: %d -> %d\\n", {registers.get(0x0067, 1000)}, new_ec_mult);
    
    // Сброс смещений
    writeRegister(0x0064, 0);  // Humidity Offset
    writeRegister(0x0066, 0);  // EC Offset
    
    // Сохранение
    writeRegister(0xFFFF, 0x00A5);
    delay(500);
    
    Serial.println("✅ Калибровка завершена!");
}}
"""
        
        print(calibration_code)
        return calibration_code

    def run_diagnostics(self) -> Dict:
        """Запуск полной диагностики"""
        print("🔧 ДИАГНОСТИКА РЕГИСТРОВ КАЛИБРОВКИ JXCT")
        print("=" * 60)
        print(f"📅 Дата: {time.strftime('%Y-%m-%d %H:%M:%S')}")
        print()
        
        # Чтение регистров
        registers = self.read_all_calibration_registers()
        
        # Анализ значений
        analysis = self.analyze_calibration_values(registers)
        
        # Генерация кода
        reset_code = self.generate_factory_reset_code()
        calibration_code = self.generate_calibration_code(registers)
        
        # Итоговые рекомендации
        print("\n📋 ИТОГОВЫЕ РЕКОМЕНДАЦИИ")
        print("=" * 60)
        
        if analysis:
            print("❌ ОБНАРУЖЕНЫ ПРОБЛЕМЫ:")
            for issue, description in analysis.items():
                print(f"   • {description}")
            print()
            print("🛠️ РЕКОМЕНДАЦИИ:")
            print("   1. Выполните factoryReset() для сброса к заводским настройкам")
            print("   2. Если проблема сохраняется - проверьте физическое состояние датчика")
            print("   3. При необходимости выполните калибровку в дистиллированной воде")
        else:
            print("✅ Все регистры в норме!")
            print("   Проблема может быть в физическом состоянии датчика")
        
        return {
            "registers": registers,
            "analysis": analysis,
            "reset_code": reset_code,
            "calibration_code": calibration_code
        }

def main():
    """Главная функция"""
    diagnostics = RegisterDiagnostics()
    results = diagnostics.run_diagnostics()
    
    print("\n📄 Для использования в Arduino IDE:")
    print("1. Скопируйте код factoryReset() в ваш скетч")
    print("2. Добавьте функции readRegister() и writeRegister()")
    print("3. Вызовите factoryReset() в setup()")
    print("4. Проверьте показания после сброса")
    
    return len(results["analysis"]) == 0

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1) 