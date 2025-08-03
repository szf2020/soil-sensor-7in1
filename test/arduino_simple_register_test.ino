/*
 * Простой тест регистров JXCT датчика
 * Для диагностики проблем с factory reset
 */

#include <ModbusRTU.h>

// Настройки Modbus
#define SENSOR_ADDRESS 0x01
#define RS485_RX_PIN 16
#define RS485_TX_PIN 17
#define RS485_DE_PIN 4

ModbusRTU modbus;

void setup() {
  Serial.begin(115200);
  Serial.println("🔍 ПРОСТОЙ ТЕСТ РЕГИСТРОВ JXCT");
  Serial.println("=" * 40);
  
  // Инициализация Modbus
  modbus.begin(&Serial2, RS485_DE_PIN);
  modbus.master();
  
  Serial.println("✅ Modbus инициализирован");
  Serial.println();
  
  // Тест 1: Чтение регистров
  Serial.println("📖 ТЕСТ 1: ЧТЕНИЕ РЕГИСТРОВ");
  testReadRegisters();
  
  // Тест 2: Запись регистров
  Serial.println("\n📝 ТЕСТ 2: ЗАПИСЬ РЕГИСТРОВ");
  testWriteRegisters();
  
  // Тест 3: Factory Reset
  Serial.println("\n🔄 ТЕСТ 3: FACTORY RESET");
  testFactoryReset();
  
  Serial.println("\n✅ ТЕСТЫ ЗАВЕРШЕНЫ");
}

void loop() {
  // Пустой цикл
}

void testReadRegisters() {
  uint16_t registers[10];
  
  // Читаем основные регистры
  uint16_t addresses[] = {0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069};
  String names[] = {"Humidity Offset", "Humidity Mult", "EC Offset", "EC Mult", "pH Offset", "pH Mult"};
  
  for (int i = 0; i < 6; i++) {
    uint8_t result = modbus.readHoldingRegisters(SENSOR_ADDRESS, addresses[i], 1, registers);
    
    if (result == modbus.ku8MBSuccess) {
      Serial.printf("✅ 0x%04X (%s): %d\n", addresses[i], names[i].c_str(), registers[0]);
    } else {
      Serial.printf("❌ 0x%04X (%s): ОШИБКА %d\n", addresses[i], names[i].c_str(), result);
    }
    delay(100);
  }
}

void testWriteRegisters() {
  // Тест записи одного регистра
  uint8_t result = modbus.writeSingleRegister(SENSOR_ADDRESS, 0x0064, 0);
  
  if (result == modbus.ku8MBSuccess) {
    Serial.println("✅ Запись регистра 0x0064 = 0: УСПЕХ");
  } else {
    Serial.printf("❌ Запись регистра 0x0064 = 0: ОШИБКА %d\n", result);
  }
  delay(100);
}

void testFactoryReset() {
  // Команда factory reset
  uint8_t result = modbus.writeSingleRegister(SENSOR_ADDRESS, 0x0063, 0x0001);
  
  if (result == modbus.ku8MBSuccess) {
    Serial.println("✅ Команда factory reset отправлена");
    delay(1000);
    
    // Проверяем результат
    uint16_t registers[1];
    result = modbus.readHoldingRegisters(SENSOR_ADDRESS, 0x0064, 1, registers);
    
    if (result == modbus.ku8MBSuccess) {
      Serial.printf("✅ Humidity Offset после сброса: %d\n", registers[0]);
    } else {
      Serial.printf("❌ Не удалось прочитать результат: %d\n", result);
    }
  } else {
    Serial.printf("❌ Команда factory reset не отправлена: %d\n", result);
  }
} 