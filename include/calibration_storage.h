#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <LittleFS.h>
#include "calibration_data.h"

/**
 * @file calibration_storage.h
 * @brief Менеджер сохранения/загрузки калибровочных данных
 * @details Работает с EEPROM через Preferences и файловой системой LittleFS
 */

class CalibrationStorage {
private:
    static const char* PREFS_NAMESPACE;
    static const char* CALIBRATION_KEY;
    static const char* CALIBRATION_FILE;
    
    Preferences prefs;
    
public:
    CalibrationStorage() = default;
    ~CalibrationStorage() = default;
    
    /**
     * @brief Инициализация хранилища
     */
    bool init() {
        return prefs.begin(PREFS_NAMESPACE, false);
    }
    
    /**
     * @brief Сохранение калибровочных данных
     * @param data Данные для сохранения
     * @return true если сохранение успешно
     */
    bool save(const SensorCalibrationData& data) {
        String json = data.toJSON();
        
        // Сохраняем в Preferences (EEPROM)
        bool prefsOk = prefs.putString(CALIBRATION_KEY, json);
        
        // Дублируем в файловую систему
        bool fileOk = saveToFile(json);
        
        return prefsOk && fileOk;
    }
    
    /**
     * @brief Загрузка калибровочных данных
     * @param data Структура для загрузки данных
     * @return true если загрузка успешна
     */
    bool load(SensorCalibrationData& data) {
        // Сначала пробуем загрузить из Preferences
        String json = prefs.getString(CALIBRATION_KEY, "");
        
        if (json.isEmpty()) {
            // Если нет в Preferences, пробуем файл
            json = loadFromFile();
        }
        
        if (json.isEmpty()) {
            return false;
        }
        
        return data.fromJSON(json);
    }
    
    /**
     * @brief Проверка наличия сохраненных данных
     */
    bool hasData() {
        return prefs.isKey(CALIBRATION_KEY) || LittleFS.exists(CALIBRATION_FILE);
    }
    
    /**
     * @brief Очистка всех сохраненных данных
     */
    bool clear() {
        prefs.remove(CALIBRATION_KEY);
        
        if (LittleFS.exists(CALIBRATION_FILE)) {
            LittleFS.remove(CALIBRATION_FILE);
        }
        
        return true;
    }
    
    /**
     * @brief Экспорт калибровочных данных в файл
     * @param filename Имя файла для экспорта
     * @param data Данные для экспорта
     */
    bool exportToFile(const String& filename, const SensorCalibrationData& data) {
        File file = LittleFS.open(filename, "w");
        if (!file) {
            return false;
        }
        
        String json = data.toJSON();
        size_t written = file.print(json);
        file.close();
        
        return written > 0;
    }
    
    /**
     * @brief Импорт калибровочных данных из файла
     * @param filename Имя файла для импорта
     * @param data Структура для загрузки данных
     */
    bool importFromFile(const String& filename, SensorCalibrationData& data) {
        if (!LittleFS.exists(filename)) {
            return false;
        }
        
        File file = LittleFS.open(filename, "r");
        if (!file) {
            return false;
        }
        
        String json = file.readString();
        file.close();
        
        return data.fromJSON(json);
    }
    
private:
    /**
     * @brief Сохранение в файл
     */
    bool saveToFile(const String& json) {
        File file = LittleFS.open(CALIBRATION_FILE, "w");
        if (!file) {
            return false;
        }
        
        size_t written = file.print(json);
        file.close();
        
        return written > 0;
    }
    
    /**
     * @brief Загрузка из файла
     */
    String loadFromFile() {
        if (!LittleFS.exists(CALIBRATION_FILE)) {
            return "";
        }
        
        File file = LittleFS.open(CALIBRATION_FILE, "r");
        if (!file) {
            return "";
        }
        
        String json = file.readString();
        file.close();
        
        return json;
    }
};

// Статические константы
const char* CalibrationStorage::PREFS_NAMESPACE = "calibration";
const char* CalibrationStorage::CALIBRATION_KEY = "sensor_cal";
const char* CalibrationStorage::CALIBRATION_FILE = "/calibration.json"; 