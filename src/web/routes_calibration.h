/**
 * @file routes_calibration.h
 * @brief Заголовочный файл для веб-маршрутов калибровки датчика
 */

#ifndef ROUTES_CALIBRATION_H
#define ROUTES_CALIBRATION_H

// Основная страница калибровки
void handleCalibrationPage();

// API маршруты для калибровки
void handleCalibrationStatus();      // GET /api/calibration/status
void handlePHCalibration();          // POST /api/calibration/ph
void handleECCalibration();          // POST /api/calibration/ec
void handleTemperatureCalibration(); // POST /api/calibration/temperature
void handleHumidityCalibration();    // POST /api/calibration/humidity
void handleNPKCalibration();         // POST /api/calibration/npk
void handleEnableCalibration();      // POST /api/calibration/enable
void handleDisableCalibration();     // POST /api/calibration/disable
void handleResetCalibration();       // POST /api/calibration/reset

#endif // ROUTES_CALIBRATION_H
