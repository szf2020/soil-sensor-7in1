#pragma once

#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>

/**
 * @file calibration_data.h
 * @brief Структуры данных для системы калибровки датчиков
 * @details Современная архитектура хранения калибровочных данных
 */

// ========== БАЗОВЫЕ СТРУКТУРЫ ==========

/**
 * @brief Точка калибровки (expected, measured)
 */
struct ModernCalibrationPoint {
    float expected;   // Эталонное значение (буферный раствор)
    float measured;   // Показание датчика
    uint32_t timestamp; // Время добавления точки
    
    ModernCalibrationPoint() : expected(0.0f), measured(0.0f), timestamp(0) {}
    ModernCalibrationPoint(float exp, float meas) : expected(exp), measured(meas), timestamp(millis()) {}
};

/**
 * @brief Коэффициенты линейной калибровки y = ax + b
 */
struct LinearCoefficients {
    float slope;      // Коэффициент наклона (a)
    float intercept;  // Смещение (b)
    float r_squared;  // Коэффициент детерминации R²
    bool is_valid;    // Флаг валидности коэффициентов
    
    LinearCoefficients() : slope(1.0f), intercept(0.0f), r_squared(0.0f), is_valid(false) {}
};

/**
 * @brief Простое смещение (offset) для температуры и влажности
 */
struct OffsetCalibration {
    float offset;     // Смещение: calibrated = raw + offset
    bool is_valid;    // Флаг валидности
    uint32_t timestamp; // Время последнего обновления
    
    OffsetCalibration() : offset(0.0f), is_valid(false), timestamp(0) {}
};

// ========== СПЕЦИАЛИЗИРОВАННЫЕ КАЛИБРОВКИ ==========

/**
 * @brief Калибровка pH (многоточечная с линейной регрессией)
 */
struct PHCalibration {
    std::vector<ModernCalibrationPoint> points;
    LinearCoefficients coefficients;
    
    bool hasPoints() const { return !points.empty(); }
    size_t getPointCount() const { return points.size(); }
    bool isReady() const { return coefficients.is_valid && points.size() >= 2; }
};

/**
 * @brief Калибровка EC (многоточечная с линейной регрессией)
 */
struct ECCalibration {
    std::vector<ModernCalibrationPoint> points;
    LinearCoefficients coefficients;
    
    bool hasPoints() const { return !points.empty(); }
    size_t getPointCount() const { return points.size(); }
    bool isReady() const { return coefficients.is_valid && points.size() >= 2; }
};

/**
 * @brief Калибровка NPK (нулевая точка в дистиллированной воде)
 */
struct NPKCalibration {
    OffsetCalibration nitrogen;
    OffsetCalibration phosphorus;
    OffsetCalibration potassium;
    uint32_t last_calibration; // Время последней калибровки
    
    NPKCalibration() : last_calibration(0) {}
    
    bool isReady() const { 
        return nitrogen.is_valid && phosphorus.is_valid && potassium.is_valid; 
    }
    
    void setZeroPoint(float n_measured, float p_measured, float k_measured) {
        nitrogen.offset = 0.0f - n_measured;
        nitrogen.is_valid = true;
        nitrogen.timestamp = millis();
        
        phosphorus.offset = 0.0f - p_measured;
        phosphorus.is_valid = true;
        phosphorus.timestamp = millis();
        
        potassium.offset = 0.0f - k_measured;
        potassium.is_valid = true;
        potassium.timestamp = millis();
        
        last_calibration = millis();
    }
};

// ========== ОСНОВНАЯ СТРУКТУРА КАЛИБРОВКИ ==========

/**
 * @brief Полная калибровка датчика
 */
struct SensorCalibrationData {
    // Offset калибровки
    OffsetCalibration temperature;
    OffsetCalibration humidity;
    
    // Многоточечные калибровки
    PHCalibration ph;
    ECCalibration ec;
    
    // NPK калибровка
    NPKCalibration npk;
    
    // Метаданные
    String device_id;
    uint32_t created_at;
    uint32_t updated_at;
    
    SensorCalibrationData() : created_at(millis()), updated_at(millis()) {}
    
    /**
     * @brief Проверка готовности всей калибровки
     */
    bool isComplete() const {
        return temperature.is_valid && humidity.is_valid && 
               ph.isReady() && ec.isReady() && npk.isReady();
    }
    
    /**
     * @brief Обновление времени модификации
     */
    void touch() {
        updated_at = millis();
    }
    
    /**
     * @brief Сброс всех калибровочных данных
     */
    void reset() {
        temperature = OffsetCalibration();
        humidity = OffsetCalibration();
        ph = PHCalibration();
        ec = ECCalibration();
        npk = NPKCalibration();
        touch();
    }
    
    /**
     * @brief Сериализация в JSON
     */
    String toJSON() const {
        DynamicJsonDocument doc(2048);
        
        // Метаданные
        doc["device_id"] = device_id;
        doc["created_at"] = created_at;
        doc["updated_at"] = updated_at;
        doc["is_complete"] = isComplete();
        
        // Temperature offset
        JsonObject temp = doc.createNestedObject("temperature");
        temp["offset"] = temperature.offset;
        temp["is_valid"] = temperature.is_valid;
        temp["timestamp"] = temperature.timestamp;
        
        // Humidity offset
        JsonObject hum = doc.createNestedObject("humidity");
        hum["offset"] = humidity.offset;
        hum["is_valid"] = humidity.is_valid;
        hum["timestamp"] = humidity.timestamp;
        
        // pH calibration
        JsonObject phObj = doc.createNestedObject("ph");
        JsonArray phPoints = phObj.createNestedArray("points");
        for (const auto& point : ph.points) {
            JsonObject p = phPoints.createNestedObject();
            p["expected"] = point.expected;
            p["measured"] = point.measured;
            p["timestamp"] = point.timestamp;
        }
        JsonObject phCoeff = phObj.createNestedObject("coefficients");
        phCoeff["slope"] = ph.coefficients.slope;
        phCoeff["intercept"] = ph.coefficients.intercept;
        phCoeff["r_squared"] = ph.coefficients.r_squared;
        phCoeff["is_valid"] = ph.coefficients.is_valid;
        
        // EC calibration
        JsonObject ecObj = doc.createNestedObject("ec");
        JsonArray ecPoints = ecObj.createNestedArray("points");
        for (const auto& point : ec.points) {
            JsonObject p = ecPoints.createNestedObject();
            p["expected"] = point.expected;
            p["measured"] = point.measured;
            p["timestamp"] = point.timestamp;
        }
        JsonObject ecCoeff = ecObj.createNestedObject("coefficients");
        ecCoeff["slope"] = ec.coefficients.slope;
        ecCoeff["intercept"] = ec.coefficients.intercept;
        ecCoeff["r_squared"] = ec.coefficients.r_squared;
        ecCoeff["is_valid"] = ec.coefficients.is_valid;
        
        // NPK calibration
        JsonObject npkObj = doc.createNestedObject("npk");
        npkObj["last_calibration"] = npk.last_calibration;
        
        JsonObject npkN = npkObj.createNestedObject("nitrogen");
        npkN["offset"] = npk.nitrogen.offset;
        npkN["is_valid"] = npk.nitrogen.is_valid;
        npkN["timestamp"] = npk.nitrogen.timestamp;
        
        JsonObject npkP = npkObj.createNestedObject("phosphorus");
        npkP["offset"] = npk.phosphorus.offset;
        npkP["is_valid"] = npk.phosphorus.is_valid;
        npkP["timestamp"] = npk.phosphorus.timestamp;
        
        JsonObject npkK = npkObj.createNestedObject("potassium");
        npkK["offset"] = npk.potassium.offset;
        npkK["is_valid"] = npk.potassium.is_valid;
        npkK["timestamp"] = npk.potassium.timestamp;
        
        String result;
        serializeJson(doc, result);
        return result;
    }
    
    /**
     * @brief Десериализация из JSON
     */
    bool fromJSON(const String& json) {
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, json);
        
        if (error) {
            return false;
        }
        
        // Метаданные
        device_id = doc["device_id"].as<String>();
        created_at = doc["created_at"];
        updated_at = doc["updated_at"];
        
        // Temperature
        if (doc.containsKey("temperature")) {
            JsonObject temp = doc["temperature"];
            temperature.offset = temp["offset"];
            temperature.is_valid = temp["is_valid"];
            temperature.timestamp = temp["timestamp"];
        }
        
        // Humidity
        if (doc.containsKey("humidity")) {
            JsonObject hum = doc["humidity"];
            humidity.offset = hum["offset"];
            humidity.is_valid = hum["is_valid"];
            humidity.timestamp = hum["timestamp"];
        }
        
        // pH
        if (doc.containsKey("ph")) {
            JsonObject phObj = doc["ph"];
            
            // Points
            if (phObj.containsKey("points")) {
                ph.points.clear();
                JsonArray phPoints = phObj["points"];
                for (JsonObject point : phPoints) {
                    ModernCalibrationPoint p;
                    p.expected = point["expected"];
                    p.measured = point["measured"];
                    p.timestamp = point["timestamp"];
                    ph.points.push_back(p);
                }
            }
            
            // Coefficients
            if (phObj.containsKey("coefficients")) {
                JsonObject phCoeff = phObj["coefficients"];
                ph.coefficients.slope = phCoeff["slope"];
                ph.coefficients.intercept = phCoeff["intercept"];
                ph.coefficients.r_squared = phCoeff["r_squared"];
                ph.coefficients.is_valid = phCoeff["is_valid"];
            }
        }
        
        // EC
        if (doc.containsKey("ec")) {
            JsonObject ecObj = doc["ec"];
            
            // Points
            if (ecObj.containsKey("points")) {
                ec.points.clear();
                JsonArray ecPoints = ecObj["points"];
                for (JsonObject point : ecPoints) {
                    ModernCalibrationPoint p;
                    p.expected = point["expected"];
                    p.measured = point["measured"];
                    p.timestamp = point["timestamp"];
                    ec.points.push_back(p);
                }
            }
            
            // Coefficients
            if (ecObj.containsKey("coefficients")) {
                JsonObject ecCoeff = ecObj["coefficients"];
                ec.coefficients.slope = ecCoeff["slope"];
                ec.coefficients.intercept = ecCoeff["intercept"];
                ec.coefficients.r_squared = ecCoeff["r_squared"];
                ec.coefficients.is_valid = ecCoeff["is_valid"];
            }
        }
        
        // NPK
        if (doc.containsKey("npk")) {
            JsonObject npkObj = doc["npk"];
            npk.last_calibration = npkObj["last_calibration"];
            
            if (npkObj.containsKey("nitrogen")) {
                JsonObject npkN = npkObj["nitrogen"];
                npk.nitrogen.offset = npkN["offset"];
                npk.nitrogen.is_valid = npkN["is_valid"];
                npk.nitrogen.timestamp = npkN["timestamp"];
            }
            
            if (npkObj.containsKey("phosphorus")) {
                JsonObject npkP = npkObj["phosphorus"];
                npk.phosphorus.offset = npkP["offset"];
                npk.phosphorus.is_valid = npkP["is_valid"];
                npk.phosphorus.timestamp = npkP["timestamp"];
            }
            
            if (npkObj.containsKey("potassium")) {
                JsonObject npkK = npkObj["potassium"];
                npk.potassium.offset = npkK["offset"];
                npk.potassium.is_valid = npkK["is_valid"];
                npk.potassium.timestamp = npkK["timestamp"];
            }
        }
        
        return true;
    }
}; 