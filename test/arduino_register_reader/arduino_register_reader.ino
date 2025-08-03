/*
🔧 ДИАГНОСТИКА РЕГИСТРОВ КАЛИБРОВКИ JXCT
Arduino скетч для чтения и сброса регистров калибровки
*/

#include <ModbusMaster.h>

// Настройки Modbus
#define RXD2 16
#define TXD2 17
#define RS485_EN 4

ModbusMaster modbusMaster;

// Заводские значения регистров
const uint16_t FACTORY_VALUES[] = {0, 1000, 0, 1000}; // 0x0064, 0x0065, 0x0066, 0x0067
const uint16_t REGISTER_ADDRESSES[] = {0x0064, 0x0065, 0x0066, 0x0067};
const char* REGISTER_NAMES[] = {"Humidity Offset", "Humidity Multiplier", "EC Offset", "EC Multiplier"};

void setup() {
  Serial.begin(115200);
  Serial.println("🔧 ДИАГНОСТИКА РЕГИСТРОВ КАЛИБРОВКИ JXCT");
  Serial.println("=" * 60);
  
  // Настройка RS485
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(RS485_EN, OUTPUT);
  digitalWrite(RS485_EN, LOW);
  
  // Настройка Modbus
  modbusMaster.begin(1, Serial2);
  modbusMaster.preTransmission(preTransmission);
  modbusMaster.postTransmission(postTransmission);
  
  delay(1000);
  
  // Чтение регистров
  readAllRegisters();
  
  // Меню команд
  Serial.println("\n📋 КОМАНДЫ:");
  Serial.println("r - Читать регистры");
  Serial.println("f - Сброс к заводским настройкам");
  Serial.println("c - Калибровка в воде");
  Serial.println("s - Сохранить в EEPROM");
}

void loop() {
  if (Serial.available()) {
    char command = Serial.read();
    
    switch (command) {
      case 'r':
        readAllRegisters();
        break;
      case 'f':
        factoryReset();
        break;
      case 'c':
        calibrateInWater();
        break;
      case 's':
        saveToEEPROM();
        break;
      default:
        Serial.println("❌ Неизвестная команда");
        break;
    }
  }
}

// Функции Modbus
void preTransmission() {
  digitalWrite(RS485_EN, HIGH);
}

void postTransmission() {
  digitalWrite(RS485_EN, LOW);
}

// Чтение регистра
uint16_t readRegister(uint16_t address) {
  uint8_t result = modbusMaster.readHoldingRegisters(address, 1);
  
  if (result == modbusMaster.ku8MBSuccess) {
    return modbusMaster.getResponseBuffer(0);
  } else {
    Serial.printf("❌ Ошибка чтения регистра 0x%04X: %d\n", address, result);
    return 0xFFFF;
  }
}

// Запись регистра
bool writeRegister(uint16_t address, uint16_t value) {
  uint8_t result = modbusMaster.writeSingleRegister(address, value);
  
  if (result == modbusMaster.ku8MBSuccess) {
    Serial.printf("✅ Регистр 0x%04X = %d\n", address, value);
    return true;
  } else {
    Serial.printf("❌ Ошибка записи регистра 0x%04X: %d\n", address, result);
    return false;
  }
}

// Чтение всех регистров калибровки
void readAllRegisters() {
  Serial.println("\n🔧 ЧТЕНИЕ РЕГИСТРОВ КАЛИБРОВКИ");
  Serial.println("=" * 60);
  
  for (int i = 0; i < 4; i++) {
    uint16_t address = REGISTER_ADDRESSES[i];
    uint16_t value = readRegister(address);
    uint16_t factory = FACTORY_VALUES[i];
    
    Serial.printf("📊 Регистр 0x%04X: %d\n", address, value);
    Serial.printf("   Название: %s\n", REGISTER_NAMES[i]);
    Serial.printf("   Заводское значение: %d\n", factory);
    
    if (value == factory) {
      Serial.println("   ✅ В норме");
    } else {
      Serial.printf("   ⚠️ ОТКЛОНЕНИЕ: %d -> %d\n", factory, value);
    }
    Serial.println();
  }
}

// Сброс к заводским настройкам
void factoryReset() {
  Serial.println("\n🔄 СБРОС К ЗАВОДСКИМ НАСТРОЙКАМ");
  Serial.println("=" * 60);
  
  // Команда сброса
  if (writeRegister(0x0063, 0x0001)) {
    delay(1000);
    
    // Сброс регистров влажности
    writeRegister(0x0064, 0);       // Humidity Offset = 0
    writeRegister(0x0065, 1000);    // Humidity Multiplier = 1000
    
    // Сброс регистров EC
    writeRegister(0x0066, 0);       // EC Offset = 0
    writeRegister(0x0067, 1000);    // EC Multiplier = 1000
    
    Serial.println("✅ Сброс завершен!");
  } else {
    Serial.println("❌ Ошибка сброса!");
  }
}

// Калибровка в воде
void calibrateInWater() {
  Serial.println("\n🔧 КАЛИБРОВКА В ВОДЕ");
  Serial.println("=" * 60);
  Serial.println("Погрузите датчик в дистиллированную воду");
  Serial.println("Нажмите любую клавишу для продолжения...");
  
  while (!Serial.available()) {
    delay(100);
  }
  Serial.read(); // Очистка буфера
  
  // Читаем текущие значения
  uint16_t hum_mult = readRegister(0x0065);
  uint16_t ec_mult = readRegister(0x0067);
  
  Serial.printf("Текущий Humidity Multiplier: %d\n", hum_mult);
  Serial.printf("Текущий EC Multiplier: %d\n", ec_mult);
  
  // Корректировка (увеличиваем множители)
  uint16_t new_hum_mult = hum_mult + 150;  // +15%
  uint16_t new_ec_mult = ec_mult + 250;    // +25%
  
  writeRegister(0x0065, new_hum_mult);
  writeRegister(0x0067, new_ec_mult);
  
  // Сброс смещений
  writeRegister(0x0064, 0);
  writeRegister(0x0066, 0);
  
  Serial.println("✅ Калибровка завершена!");
}

// Сохранение в EEPROM
void saveToEEPROM() {
  Serial.println("\n💾 СОХРАНЕНИЕ В EEPROM");
  Serial.println("=" * 60);
  
  if (writeRegister(0xFFFF, 0x00A5)) {
    delay(500);
    Serial.println("✅ Настройки сохранены в EEPROM!");
  } else {
    Serial.println("❌ Ошибка сохранения!");
  }
}

// Дополнительные функции диагностики
void testSensorReadings() {
  Serial.println("\n📊 ТЕСТ ПОКАЗАНИЙ ДАТЧИКА");
  Serial.println("=" * 60);
  
  for (int i = 0; i < 5; i++) {
    uint16_t humidity = readRegister(0x0001);  // Регистр влажности
    uint16_t ec = readRegister(0x0002);        // Регистр EC
    
    Serial.printf("Измерение %d: Влажность=%d, EC=%d\n", i+1, humidity, ec);
    delay(1000);
  }
}

// Проверка связи с датчиком
bool testConnection() {
  Serial.println("\n🔗 ПРОВЕРКА СВЯЗИ С ДАТЧИКОМ");
  Serial.println("=" * 60);
  
  uint16_t device_id = readRegister(0x0000);
  
  if (device_id != 0xFFFF) {
    Serial.printf("✅ Связь установлена, Device ID: 0x%04X\n", device_id);
    return true;
  } else {
    Serial.println("❌ Нет связи с датчиком!");
    Serial.println("Проверьте:");
    Serial.println("1. Подключение проводов");
    Serial.println("2. Напряжение питания");
    Serial.println("3. Адрес устройства");
    return false;
  }
} 