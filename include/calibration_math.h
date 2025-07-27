#pragma once

#include <Arduino.h>
#include <vector>
#include "calibration_data.h"

/**
 * @file calibration_math.h
 * @brief Математические утилиты для калибровки датчиков
 * @details Линейная регрессия, валидация, статистика
 */

class CalibrationMath {
public:
    /**
     * @brief Расчет коэффициентов линейной регрессии методом наименьших квадратов
     * @param points Массив калибровочных точек
     * @param coefficients Результирующие коэффициенты
     * @return true если расчет успешен
     */
    static bool calculateLinearRegression(const std::vector<ModernCalibrationPoint>& points, 
                                        LinearCoefficients& coefficients) {
        if (points.size() < 2) {
            return false;
        }
        
        size_t n = points.size();
        float sum_x = 0.0f, sum_y = 0.0f, sum_xy = 0.0f, sum_x2 = 0.0f, sum_y2 = 0.0f;
        
        // Суммируем все необходимые значения
        for (const auto& point : points) {
            sum_x += point.measured;    // x = measured (показание датчика)
            sum_y += point.expected;    // y = expected (эталонное значение)
            sum_xy += point.measured * point.expected;
            sum_x2 += point.measured * point.measured;
            sum_y2 += point.expected * point.expected;
        }
        
        // Рассчитываем коэффициенты: y = ax + b
        float denominator = n * sum_x2 - sum_x * sum_x;
        
        if (abs(denominator) < 1e-6f) {
            // Избегаем деления на ноль
            return false;
        }
        
        coefficients.slope = (n * sum_xy - sum_x * sum_y) / denominator;
        coefficients.intercept = (sum_y - coefficients.slope * sum_x) / n;
        
        // Рассчитываем коэффициент детерминации R²
        coefficients.r_squared = calculateRSquared(points, coefficients.slope, coefficients.intercept);
        
        // Проверяем разумность коэффициентов
        coefficients.is_valid = validateCoefficients(coefficients);
        
        return coefficients.is_valid;
    }
    
    /**
     * @brief Расчет коэффициента детерминации R²
     */
    static float calculateRSquared(const std::vector<ModernCalibrationPoint>& points, 
                                 float slope, float intercept) {
        if (points.empty()) {
            return 0.0f;
        }
        
        // Среднее значение y (expected)
        float mean_y = 0.0f;
        for (const auto& point : points) {
            mean_y += point.expected;
        }
        mean_y /= points.size();
        
        // Сумма квадратов остатков (SSres) и общая сумма квадратов (SStot)
        float ss_res = 0.0f, ss_tot = 0.0f;
        
        for (const auto& point : points) {
            float predicted = slope * point.measured + intercept;
            ss_res += (point.expected - predicted) * (point.expected - predicted);
            ss_tot += (point.expected - mean_y) * (point.expected - mean_y);
        }
        
        if (ss_tot < 1e-6f) {
            return 1.0f; // Все точки на одной линии
        }
        
        float r_squared = 1.0f - (ss_res / ss_tot);
        
        // Ограничиваем R² в диапазоне [0, 1]
        return constrain(r_squared, 0.0f, 1.0f);
    }
    
    /**
     * @brief Валидация коэффициентов калибровки
     */
    static bool validateCoefficients(const LinearCoefficients& coefficients) {
        // Проверяем разумность коэффициентов
        if (isnan(coefficients.slope) || isnan(coefficients.intercept) || 
            isnan(coefficients.r_squared)) {
            return false;
        }
        
        // Проверяем, что slope не слишком экстремальный
        if (abs(coefficients.slope) > 10.0f || abs(coefficients.slope) < 0.1f) {
            return false;
        }
        
        // Проверяем качество регрессии (R² должен быть достаточно высоким)
        if (coefficients.r_squared < 0.5f) {
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief Применение калибровки к значению
     */
    static float applyCalibration(float raw_value, const LinearCoefficients& coefficients) {
        if (!coefficients.is_valid) {
            return raw_value;
        }
        
        return coefficients.slope * raw_value + coefficients.intercept;
    }
    
    /**
     * @brief Применение offset калибровки
     */
    static float applyOffsetCalibration(float raw_value, const OffsetCalibration& offset) {
        if (!offset.is_valid) {
            return raw_value;
        }
        
        return raw_value + offset.offset;
    }
    
    /**
     * @brief Валидация калибровочной точки
     */
    static bool validateCalibrationPoint(float expected, float measured, const String& sensor_type) {
        // Общие проверки
        if (isnan(expected) || isnan(measured) || 
            isinf(expected) || isinf(measured)) {
            return false;
        }
        
        // Специфичные проверки для разных типов датчиков
        if (sensor_type == "ph") {
            return (expected >= 0.0f && expected <= 14.0f && 
                   measured >= 0.0f && measured <= 14.0f);
        }
        else if (sensor_type == "ec") {
            return (expected >= 0.0f && expected <= 20000.0f && 
                   measured >= 0.0f && measured <= 20000.0f);
        }
        else if (sensor_type == "temperature") {
            return (expected >= -40.0f && expected <= 80.0f && 
                   measured >= -40.0f && measured <= 80.0f);
        }
        else if (sensor_type == "humidity") {
            return (expected >= 0.0f && expected <= 100.0f && 
                   measured >= 0.0f && measured <= 100.0f);
        }
        else if (sensor_type == "npk") {
            return (expected >= 0.0f && expected <= 2000.0f && 
                   measured >= 0.0f && measured <= 2000.0f);
        }
        
        return true;
    }
    
    /**
     * @brief Удаление выбросов из калибровочных точек
     * @param points Массив точек
     * @param threshold Порог для определения выбросов (в стандартных отклонениях)
     */
    static void removeOutliers(std::vector<ModernCalibrationPoint>& points, float threshold = 2.0f) {
        if (points.size() < 3) {
            return; // Слишком мало точек для анализа выбросов
        }
        
        // Рассчитываем среднее и стандартное отклонение отношений expected/measured
        float mean_ratio = 0.0f;
        std::vector<float> ratios;
        
        for (const auto& point : points) {
            if (point.measured != 0.0f) {
                float ratio = point.expected / point.measured;
                ratios.push_back(ratio);
                mean_ratio += ratio;
            }
        }
        
        if (ratios.empty()) {
            return;
        }
        
        mean_ratio /= ratios.size();
        
        // Рассчитываем стандартное отклонение
        float variance = 0.0f;
        for (float ratio : ratios) {
            variance += (ratio - mean_ratio) * (ratio - mean_ratio);
        }
        variance /= ratios.size();
        float std_dev = sqrt(variance);
        
        // Удаляем выбросы
        auto it = points.begin();
        while (it != points.end()) {
            if (it->measured != 0.0f) {
                float ratio = it->expected / it->measured;
                if (abs(ratio - mean_ratio) > threshold * std_dev) {
                    it = points.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }
    
    /**
     * @brief Получение статистики по калибровочным точкам
     */
    struct CalibrationStats {
        size_t point_count;
        float mean_error;
        float max_error;
        float r_squared;
        bool is_reliable;
    };
    
    static CalibrationStats getCalibrationStats(const std::vector<ModernCalibrationPoint>& points, 
                                              const LinearCoefficients& coefficients) {
        CalibrationStats stats = {};
        stats.point_count = points.size();
        stats.r_squared = coefficients.r_squared;
        
        if (points.empty() || !coefficients.is_valid) {
            return stats;
        }
        
        float total_error = 0.0f;
        float max_error = 0.0f;
        
        for (const auto& point : points) {
            float predicted = coefficients.slope * point.measured + coefficients.intercept;
            float error = abs(point.expected - predicted);
            
            total_error += error;
            if (error > max_error) {
                max_error = error;
            }
        }
        
        stats.mean_error = total_error / points.size();
        stats.max_error = max_error;
        stats.is_reliable = (stats.r_squared > 0.8f && stats.mean_error < 0.5f);
        
        return stats;
    }
}; 