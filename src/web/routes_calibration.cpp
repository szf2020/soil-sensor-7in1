/**
 * @file routes_calibration.cpp
 * @brief Веб-маршруты для калибровки датчика
 * @details Обработка страницы калибровки и API для настройки калибровочных параметров
 */

#include <ArduinoJson.h>
#include "../../include/jxct_config_vars.h"
#include "../../include/jxct_constants.h"
#include "../../include/jxct_ui_system.h"
#include "../../include/logger.h"
#include "../../include/web/csrf_protection.h"
#include "../../include/web_routes.h"
#include "../wifi_manager.h"
#include "../sensor_correction.h"
#include "routes_calibration.h"

extern WebServer webServer;
extern WiFiMode currentWiFiMode;
extern SensorCorrection gSensorCorrection;

void handleCalibrationPage()
{
    logWebRequest("GET", "/calibration", webServer.client().remoteIP().toString());

    if (currentWiFiMode == WiFiMode::AP)
    {
        webServer.send(200, "text/html; charset=utf-8",
                       generateApModeUnavailablePage("Калибровка", UI_ICON_CALIBRATION));
        return;
    }

    String html = generatePageHeader("Калибровка датчика", UI_ICON_CALIBRATION);
    html += navHtml();
    html += "<h1>" UI_ICON_CALIBRATION " Калибровка датчика JXCT 7-in-1</h1>";

    // Инструкции по калибровке
    html += "<div class='section' style='background:#f8f9fa;padding:15px;border-radius:8px;margin:15px 0;'>";
    html += "<h3>📋 Инструкции по калибровке</h3>";
    html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:20px;font-size:14px;'>";
    
    // Левая колонка
    html += "<div>";
    html += "<h4>🧪 pH Калибровка (3-точечная)</h4>";
    html += "<ul style='margin:0;padding-left:20px;'>";
    html += "<li>Используйте стандартные буферные растворы</li>";
    html += "<li>pH 4.01, 6.86, 9.18 (или другие)</li>";
    html += "<li>Качество калибровки: R² > 0.95</li>";
    html += "</ul>";
    
    html += "<h4>⚡ EC Калибровка (2-точечная)</h4>";
    html += "<ul style='margin:0;padding-left:20px;'>";
    html += "<li>Используйте стандартные растворы KCl</li>";
    html += "<li>1.41 и 12.88 mS/cm (или другие)</li>";
    html += "<li>Линейная коррекция по прямой</li>";
    html += "</ul>";
    html += "</div>";
    
    // Правая колонка
    html += "<div>";
    html += "<h4>🌡️ Температурная калибровка</h4>";
    html += "<ul style='margin:0;padding-left:20px;'>";
    html += "<li>Сравните с эталонным термометром</li>";
    html += "<li>Измерьте в стабильных условиях</li>";
    html += "<li>Точность ±0.5°C</li>";
    html += "</ul>";
    
    html += "<h4>💧 Влажностная калибровка</h4>";
    html += "<ul style='margin:0;padding-left:20px;'>";
    html += "<li>Сравните с эталонным гигрометром</li>";
    html += "<li>Измерьте в стабильных условиях</li>";
    html += "<li>Точность ±2%</li>";
    html += "</ul>";
    
    html += "<h4>🌿 NPK Калибровка</h4>";
    html += "<ul style='margin:0;padding-left:20px;'>";
    html += "<li>Выставьте ноль по дистиллированной воде</li>";
    html += "<li>Нулевая точка для каждого элемента</li>";
    html += "<li>Компенсация фонового сигнала</li>";
    html += "</ul>";
    html += "</div>";
    
    html += "</div>";
    html += "</div>";

    // Статус калибровок (серверный рендер без авто-JS)
    html += "<div class='section' style='background:#f8f9fa;padding:15px;border-radius:8px;margin:15px 0;'>";
    html += "<h3>📊 Статус калибровок</h3>";

    // Гарантируем инициализацию и получаем текущие коэффициенты
    if (!gSensorCorrection.isInitialized()) {
        gSensorCorrection.init();
    }
    CorrectionFactors s = gSensorCorrection.getCorrectionFactors();

    html += "<div id='calibrationStatus' style='font-size:14px;'>";
    html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:20px;'>";
    html += "<div><h4>Статус калибровки:</h4><p>";
    html += (s.calibrationEnabled ? "✅ Включена" : "❌ Отключена");
    html += "</p></div>";
    html += "<div><h4>Откалиброванные датчики:</h4>";
    html += "<ul style='margin:0;padding-left:20px;'>";
    if (s.phCalibrated)          html += "<li>✅ pH</li>";
    if (s.ecCalibrated)          html += "<li>✅ EC</li>";
    if (s.temperatureCalibrated) html += "<li>✅ Температура</li>";
    if (s.humidityCalibrated)    html += "<li>✅ Влажность</li>";
    if (s.npkCalibrated)         html += "<li>✅ NPK</li>";
    html += "</ul></div>";
    html += "</div>"; // grid
    html += "</div>"; // calibrationStatus
    html += "</div>";

    // pH Калибровка
    html += "<div class='section' style='background:#f8f9fa;padding:15px;border-radius:8px;margin:15px 0;'>";
    html += "<h3>🧪 pH Калибровка (3-точечная)</h3>";
    html += "<div style='display:grid;grid-template-columns:1fr 1fr 1fr;gap:20px;margin:15px 0;'>";
    
    html += "<div>";
    html += "<div class='form-group'>";
    html += "<label>Ожидаемое значение pH:</label>";
    html += "<input type='number' id='phExpected1' step='0.01' placeholder='4.01' value='4.01' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label>Измеренное значение pH:</label>";
    html += "<input type='number' id='phMeasured1' step='0.01' placeholder='Введите измеренное значение' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "</div>";
    
    html += "<div>";
    html += "<div class='form-group'>";
    html += "<label>Ожидаемое значение pH:</label>";
    html += "<input type='number' id='phExpected2' step='0.01' placeholder='6.86' value='6.86' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label>Измеренное значение pH:</label>";
    html += "<input type='number' id='phMeasured2' step='0.01' placeholder='Введите измеренное значение' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "</div>";
    
    html += "<div>";
    html += "<div class='form-group'>";
    html += "<label>Ожидаемое значение pH:</label>";
    html += "<input type='number' id='phExpected3' step='0.01' placeholder='9.18' value='9.18' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label>Измеренное значение pH:</label>";
    html += "<input type='number' id='phMeasured3' step='0.01' placeholder='Введите измеренное значение' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "</div>";
    
    html += "</div>";
    
    html += "<button onclick='calibratePH()' class='btn btn-primary' style='width:100%;margin-top:15px;background:#4CAF50;color:white;border:none;padding:12px;border-radius:6px;cursor:pointer;'>Калибровать pH</button>";
    html += "<div id='phResult' style='margin-top:10px;font-size:14px;min-height:20px;'></div>";
    html += "</div>";

    // EC Калибровка
    html += "<div class='section' style='background:#f8f9fa;padding:15px;border-radius:8px;margin:15px 0;'>";
    html += "<h3>⚡ EC Калибровка (2-точечная)</h3>";
    html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:20px;margin:15px 0;'>";
    
    html += "<div>";
    html += "<div class='form-group'>";
    html += "<label>Ожидаемое значение EC (mS/cm):</label>";
    html += "<input type='number' id='ecExpected1' step='0.01' placeholder='1.41' value='1.41' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label>Измеренное значение EC (mS/cm):</label>";
    html += "<input type='number' id='ecMeasured1' step='0.01' placeholder='Введите измеренное значение' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "</div>";
    
    html += "<div>";
    html += "<div class='form-group'>";
    html += "<label>Ожидаемое значение EC (mS/cm):</label>";
    html += "<input type='number' id='ecExpected2' step='0.01' placeholder='12.88' value='12.88' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label>Измеренное значение EC (mS/cm):</label>";
    html += "<input type='number' id='ecMeasured2' step='0.01' placeholder='Введите измеренное значение' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "</div>";
    
    html += "</div>";
    html += "<button onclick='calibrateEC()' class='btn btn-primary' style='width:100%;margin-top:15px;background:#4CAF50;color:white;border:none;padding:12px;border-radius:6px;cursor:pointer;'>Калибровать EC</button>";
    html += "<div id='ecResult' style='margin-top:10px;font-size:14px;min-height:20px;'></div>";
    html += "</div>";

    // Температурная калибровка
    html += "<div class='section' style='background:#f8f9fa;padding:15px;border-radius:8px;margin:15px 0;'>";
    html += "<h3>🌡️ Температурная калибровка</h3>";
    html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:20px;margin:15px 0;'>";
    
    html += "<div>";
    html += "<div class='form-group'>";
    html += "<label>Эталонная температура (°C):</label>";
    html += "<input type='number' id='tempReference' step='0.1' placeholder='25.0' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "</div>";
    
    html += "<div>";
    html += "<div class='form-group'>";
    html += "<label>Измеренная температура (°C):</label>";
    html += "<input type='number' id='tempMeasured' step='0.1' placeholder='Введите измеренное значение' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "</div>";
    
    html += "</div>";
    html += "<button onclick='calibrateTemperature()' class='btn btn-primary' style='width:100%;margin-top:15px;background:#4CAF50;color:white;border:none;padding:12px;border-radius:6px;cursor:pointer;'>Калибровать температуру</button>";
    html += "<div id='tempResult' style='margin-top:10px;font-size:14px;min-height:20px;'></div>";
    html += "</div>";

    // Влажностная калибровка
    html += "<div class='section' style='background:#f8f9fa;padding:15px;border-radius:8px;margin:15px 0;'>";
    html += "<h3>💧 Влажностная калибровка</h3>";
    html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:20px;margin:15px 0;'>";
    
    html += "<div>";
    html += "<div class='form-group'>";
    html += "<label>Эталонная влажность (%):</label>";
    html += "<input type='number' id='humidityReference' step='0.1' placeholder='50.0' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "</div>";
    
    html += "<div>";
    html += "<div class='form-group'>";
    html += "<label>Измеренная влажность (%):</label>";
    html += "<input type='number' id='humidityMeasured' step='0.1' placeholder='Введите измеренное значение' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "</div>";
    
    html += "</div>";
    html += "<button onclick='calibrateHumidity()' class='btn btn-primary' style='width:100%;margin-top:15px;background:#4CAF50;color:white;border:none;padding:12px;border-radius:6px;cursor:pointer;'>Калибровать влажность</button>";
    html += "<div id='humidityResult' style='margin-top:10px;font-size:14px;min-height:20px;'></div>";
    html += "</div>";

    // NPK Калибровка
    html += "<div class='section' style='background:#f8f9fa;padding:15px;border-radius:8px;margin:15px 0;'>";
    html += "<h3>🌿 NPK Калибровка (нулевая точка)</h3>";
    html += "<p style='font-size:14px;color:#666;margin-bottom:15px;'>Выставьте ноль по дистиллированной воде для каждого элемента</p>";
    html += "<div style='display:grid;grid-template-columns:1fr 1fr 1fr;gap:20px;margin:15px 0;'>";
    
    html += "<div>";
    html += "<div class='form-group'>";
    html += "<label>Азот (N) - нулевая точка:</label>";
    html += "<input type='number' id='npkNitrogen' step='0.1' placeholder='0.0' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "</div>";
    
    html += "<div>";
    html += "<div class='form-group'>";
    html += "<label>Фосфор (P) - нулевая точка:</label>";
    html += "<input type='number' id='npkPhosphorus' step='0.1' placeholder='0.0' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "</div>";
    
    html += "<div>";
    html += "<div class='form-group'>";
    html += "<label>Калий (K) - нулевая точка:</label>";
    html += "<input type='number' id='npkPotassium' step='0.1' placeholder='0.0' style='width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;'>";
    html += "</div>";
    html += "</div>";
    
    html += "</div>";
    html += "<button onclick='calibrateNPK()' class='btn btn-primary' style='width:100%;margin-top:15px;background:#4CAF50;color:white;border:none;padding:12px;border-radius:6px;cursor:pointer;'>Калибровать NPK</button>";
    html += "<div id='npkResult' style='margin-top:10px;font-size:14px;min-height:20px;'></div>";
    html += "</div>";

    // Управление калибровкой
    html += "<div class='section' style='background:#f8f9fa;padding:15px;border-radius:8px;margin:15px 0;'>";
    html += "<h3>⚙️ Управление калибровкой</h3>";
    html += "<div style='display:grid;grid-template-columns:1fr 1fr 1fr;gap:15px;margin:15px 0;'>";
    
    html += "<button onclick='enableCalibration()' class='btn btn-success' style='background:#4CAF50;color:white;border:none;padding:12px;border-radius:6px;cursor:pointer;'>Включить калибровку</button>";
    html += "<button onclick='disableCalibration()' class='btn btn-warning' style='background:#FFC107;color:white;border:none;padding:12px;border-radius:6px;cursor:pointer;'>Отключить калибровку</button>";
    html += "<button onclick='resetCalibration()' class='btn btn-danger' style='background:#F44336;color:white;border:none;padding:12px;border-radius:6px;cursor:pointer;'>Сбросить к заводским</button>";
    
    html += "</div>";
    html += "</div>";

    // JavaScript
    html += "<script>";
    html += "function showResult(elementId, message, type) {";
    html += "  const element = document.getElementById(elementId);";
    html += "  if (element) {";
    html += "    const color = type === 'success' ? '#28a745' : type === 'error' ? '#dc3545' : '#ffc107';";
    html += "    element.innerHTML = '<p style=\"color:' + color + ';margin:0;\">' + message + '</p>';";
    html += "  }";
    html += "}";
    
    html += "function loadCalibrationStatus() {";
    html += "  console.log('Loading calibration status...');";
    html += "  fetch('/api/calibration/status')";
    html += "    .then(response => {";
    html += "      console.log('Response status:', response.status);";
    html += "      return response.json();";
    html += "    })";
    html += "    .then(data => {";
    html += "      console.log('Calibration data:', data);";
    html += "      console.log('ph_calibrated:', data.ph_calibrated, typeof data.ph_calibrated);";
    html += "      console.log('ec_calibrated:', data.ec_calibrated, typeof data.ec_calibrated);";
    html += "      console.log('temperature_calibrated:', data.temperature_calibrated, typeof data.temperature_calibrated);";
    html += "      console.log('humidity_calibrated:', data.humidity_calibrated, typeof data.humidity_calibrated);";
    html += "      console.log('npk_calibrated:', data.npk_calibrated, typeof data.npk_calibrated);";
    html += "      if(data.success) {";
    html += "        let statusHtml = '<div style=\"display:grid;grid-template-columns:1fr 1fr;gap:20px;\">';";
    html += "        statusHtml += '<div><h4>Статус калибровки:</h4>';";
    html += "        statusHtml += '<p>' + (data.calibration_enabled ? '✅ Включена' : '❌ Отключена') + '</p></div>';";
    html += "        statusHtml += '<div><h4>Откалиброванные датчики:</h4>';";
    html += "        statusHtml += '<ul style=\"margin:0;padding-left:20px;\">';";
    html += "        if(data.ph_calibrated === true) statusHtml += '<li>✅ pH</li>';";
    html += "        if(data.ec_calibrated === true) statusHtml += '<li>✅ EC</li>';";
    html += "        if(data.temperature_calibrated === true) statusHtml += '<li>✅ Температура</li>';";
    html += "        if(data.humidity_calibrated === true) statusHtml += '<li>✅ Влажность</li>';";
    html += "        if(data.npk_calibrated === true) statusHtml += '<li>✅ NPK</li>';";
    html += "        statusHtml += '</ul></div>';";
    html += "        statusHtml += '</div>';";
    html += "        document.getElementById('calibrationStatus').innerHTML = statusHtml;";
    html += "      } else {";
    html += "        document.getElementById('calibrationStatus').innerHTML = '<p style=\"color:#dc3545;\">❌ Ошибка загрузки статуса</p>';";
    html += "      }";
    html += "    })";
    html += "    .catch(err => {";
    html += "      console.error('Load calibration status error:', err);";
    html += "      document.getElementById('calibrationStatus').innerHTML = '<p style=\"color:#dc3545;\">❌ Ошибка соединения</p>';";
    html += "    });";
    html += "}";
    html += "";
    // Убираем автообновление статуса - оно ломает интерфейс
    
    html += "function calibratePH() {";
    html += "  const phExpected1 = parseFloat(document.getElementById('phExpected1').value);";
    html += "  const phMeasured1 = parseFloat(document.getElementById('phMeasured1').value);";
    html += "  const phExpected2 = parseFloat(document.getElementById('phExpected2').value);";
    html += "  const phMeasured2 = parseFloat(document.getElementById('phMeasured2').value);";
    html += "  const phExpected3 = parseFloat(document.getElementById('phExpected3').value);";
    html += "  const phMeasured3 = parseFloat(document.getElementById('phMeasured3').value);";
    html += "  ";
    html += "  if (!phExpected1 || !phMeasured1 || !phExpected2 || !phMeasured2 || !phExpected3 || !phMeasured3) {";
    html += "    showResult('phResult', 'Пожалуйста, заполните все поля', 'error');";
    html += "    return;";
    html += "  }";
    html += "  ";
    html += "  fetch('/api/calibration/ph', {";
    html += "    method: 'POST',";
    html += "    headers: {'Content-Type': 'application/json'},";
    html += "    body: JSON.stringify({";
    html += "      expected_1: phExpected1,";
    html += "      measured_1: phMeasured1,";
    html += "      expected_2: phExpected2,";
    html += "      measured_2: phMeasured2,";
    html += "      expected_3: phExpected3,";
    html += "      measured_3: phMeasured3";
    html += "    })";
    html += "  })";
    html += "  .then(response => response.json())";
    html += "  .then(data => {";
    html += "    if(data.success) {";
    html += "      showResult('phResult', '✅ pH калибровка успешна! R² = ' + data.r_squared + ' (' + data.quality + ')', 'success');";
    html += "      loadCalibrationStatus();";
    html += "    } else {";
    html += "      showResult('phResult', '❌ Ошибка: ' + (data.error || 'Неизвестная ошибка'), 'error');";
    html += "    }";
    html += "  })";
    html += "  .catch(err => {";
    html += "    console.error('pH calibration error:', err);";
    html += "    showResult('phResult', '❌ Ошибка соединения', 'error');";
    html += "  });";
    html += "}";
    
    html += "function calibrateEC() {";
    html += "  const ecExpected1 = parseFloat(document.getElementById('ecExpected1').value);";
    html += "  const ecMeasured1 = parseFloat(document.getElementById('ecMeasured1').value);";
    html += "  const ecExpected2 = parseFloat(document.getElementById('ecExpected2').value);";
    html += "  const ecMeasured2 = parseFloat(document.getElementById('ecMeasured2').value);";
    html += "  ";
    html += "  if (!ecExpected1 || !ecMeasured1 || !ecExpected2 || !ecMeasured2) {";
    html += "    showResult('ecResult', 'Пожалуйста, заполните все поля', 'error');";
    html += "    return;";
    html += "  }";
    html += "  ";
    html += "  fetch('/api/calibration/ec', {";
    html += "    method: 'POST',";
    html += "    headers: {'Content-Type': 'application/json'},";
    html += "    body: JSON.stringify({";
    html += "      expected_1: ecExpected1,";
    html += "      measured_1: ecMeasured1,";
    html += "      expected_2: ecExpected2,";
    html += "      measured_2: ecMeasured2";
    html += "    })";
    html += "  })";
    html += "  .then(response => response.json())";
    html += "  .then(data => {";
    html += "    if(data.success) {";
    html += "      showResult('ecResult', '✅ EC калибровка успешна! R² = ' + data.r_squared + ' (' + data.quality + ')', 'success');";
    html += "      loadCalibrationStatus();";
    html += "    } else {";
    html += "      showResult('ecResult', '❌ Ошибка: ' + (data.error || 'Неизвестная ошибка'), 'error');";
    html += "    }";
    html += "  })";
    html += "  .catch(err => {";
    html += "    console.error('EC calibration error:', err);";
    html += "    showResult('ecResult', '❌ Ошибка соединения', 'error');";
    html += "  });";
    html += "}";
    
    html += "function calibrateTemperature() {";
    html += "  const reference = parseFloat(document.getElementById('tempReference').value);";
    html += "  const measured = parseFloat(document.getElementById('tempMeasured').value);";
    html += "  ";
    html += "  if (!reference || !measured) {";
    html += "    showResult('tempResult', 'Пожалуйста, заполните все поля', 'error');";
    html += "    return;";
    html += "  }";
    html += "  ";
    html += "  fetch('/api/calibration/temperature', {";
    html += "    method: 'POST',";
    html += "    headers: {'Content-Type': 'application/json'},";
    html += "    body: JSON.stringify({";
    html += "      reference: reference,";
    html += "      measured: measured";
    html += "    })";
    html += "  })";
    html += "  .then(response => response.json())";
    html += "  .then(data => {";
    html += "    if(data.success) {";
    html += "      showResult('tempResult', '✅ Температурная калибровка успешна! Качество: ' + data.quality, 'success');";
    html += "      loadCalibrationStatus();";
    html += "    } else {";
    html += "      showResult('tempResult', '❌ Ошибка: ' + (data.error || 'Неизвестная ошибка'), 'error');";
    html += "    }";
    html += "  })";
    html += "  .catch(err => {";
    html += "    console.error('Temperature calibration error:', err);";
    html += "    showResult('tempResult', '❌ Ошибка соединения', 'error');";
    html += "  });";
    html += "}";
    
    html += "function calibrateHumidity() {";
    html += "  const reference = parseFloat(document.getElementById('humidityReference').value);";
    html += "  const measured = parseFloat(document.getElementById('humidityMeasured').value);";
    html += "  ";
    html += "  if (!reference || !measured) {";
    html += "    showResult('humidityResult', 'Пожалуйста, заполните все поля', 'error');";
    html += "    return;";
    html += "  }";
    html += "  ";
    html += "  fetch('/api/calibration/humidity', {";
    html += "    method: 'POST',";
    html += "    headers: {'Content-Type': 'application/json'},";
    html += "    body: JSON.stringify({";
    html += "      reference: reference,";
    html += "      measured: measured";
    html += "    })";
    html += "  })";
    html += "  .then(response => response.json())";
    html += "  .then(data => {";
    html += "    if(data.success) {";
    html += "      showResult('humidityResult', '✅ Влажностная калибровка успешна! Качество: ' + data.quality, 'success');";
    html += "      loadCalibrationStatus();";
    html += "    } else {";
    html += "      showResult('humidityResult', '❌ Ошибка: ' + (data.error || 'Неизвестная ошибка'), 'error');";
    html += "    }";
    html += "  })";
    html += "  .catch(err => {";
    html += "    console.error('Humidity calibration error:', err);";
    html += "    showResult('humidityResult', '❌ Ошибка соединения', 'error');";
    html += "  });";
    html += "}";
    
    html += "function calibrateNPK() {";
    html += "  const nitrogen = parseFloat(document.getElementById('npkNitrogen').value) || 0;";
    html += "  const phosphorus = parseFloat(document.getElementById('npkPhosphorus').value) || 0;";
    html += "  const potassium = parseFloat(document.getElementById('npkPotassium').value) || 0;";
    html += "  ";
    html += "  fetch('/api/calibration/npk', {";
    html += "    method: 'POST',";
    html += "    headers: {'Content-Type': 'application/json'},";
    html += "    body: JSON.stringify({";
    html += "      nitrogen: nitrogen,";
    html += "      phosphorus: phosphorus,";
    html += "      potassium: potassium";
    html += "    })";
    html += "  })";
    html += "  .then(response => response.json())";
    html += "  .then(data => {";
    html += "    if(data.success) {";
    html += "      showResult('npkResult', '✅ NPK калибровка успешна! Нулевые точки установлены', 'success');";
    html += "      loadCalibrationStatus();";
    html += "    } else {";
    html += "      showResult('npkResult', '❌ Ошибка: ' + (data.error || 'Неизвестная ошибка'), 'error');";
    html += "    }";
    html += "  })";
    html += "  .catch(err => {";
    html += "    console.error('NPK calibration error:', err);";
    html += "    showResult('npkResult', '❌ Ошибка соединения', 'error');";
    html += "  });";
    html += "}";
    
    html += "function enableCalibration() {";
    html += "  fetch('/api/calibration/enable', {";
    html += "    method: 'POST',";
    html += "    headers: {'Content-Type': 'application/json'},";
    html += "    body: JSON.stringify({enabled: true})";
    html += "  })";
    html += "  .then(response => response.json())";
    html += "  .then(data => {";
    html += "    if(data.success) {";
    html += "      alert('✅ Калибровка включена');";
    html += "      loadCalibrationStatus();";
    html += "    } else {";
    html += "      alert('❌ Ошибка: ' + (data.error || 'Неизвестная ошибка'));";
    html += "    }";
    html += "  })";
    html += "  .catch(err => {";
    html += "    console.error('Enable calibration error:', err);";
    html += "    alert('❌ Ошибка соединения');";
    html += "  });";
    html += "}";
    
    html += "function disableCalibration() {";
    html += "  fetch('/api/calibration/disable', {";
    html += "    method: 'POST',";
    html += "    headers: {'Content-Type': 'application/json'},";
    html += "    body: JSON.stringify({enabled: false})";
    html += "  })";
    html += "  .then(response => response.json())";
    html += "  .then(data => {";
    html += "    if(data.success) {";
    html += "      alert('✅ Калибровка отключена');";
    html += "      loadCalibrationStatus();";
    html += "    } else {";
    html += "      alert('❌ Ошибка: ' + (data.error || 'Неизвестная ошибка'));";
    html += "    }";
    html += "  })";
    html += "  .catch(err => {";
    html += "    console.error('Disable calibration error:', err);";
    html += "    alert('❌ Ошибка соединения');";
    html += "  });";
    html += "}";
    
    html += "function resetCalibration() {";
    html += "  if(confirm('⚠️ Вы уверены? Это сбросит все калибровки к заводским настройкам.')) {";
    html += "    fetch('/api/calibration/reset', {";
    html += "      method: 'POST',";
    html += "      headers: {'Content-Type': 'application/json'}";
    html += "    })";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      if(data.success) {";
    html += "        alert('✅ Калибровки сброшены к заводским настройкам');";
    html += "        loadCalibrationStatus();";
    html += "      } else {";
    html += "        alert('❌ Ошибка: ' + (data.error || 'Неизвестная ошибка'));";
    html += "      }";
    html += "    })";
    html += "    .catch(err => {";
    html += "      console.error('Reset calibration error:', err);";
    html += "      alert('❌ Ошибка соединения');";
    html += "    });";
    html += "  }";
    html += "}";
    
    // Убираем автообновление статуса - оно ломает интерфейс
    html += "</script>";

    html += generatePageFooter();
    webServer.send(HTTP_OK, HTTP_CONTENT_TYPE_HTML, html);
}

// Функция-помощник для добавления CORS заголовков
void addCORSHeaders() {
    webServer.enableCORS(true);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    webServer.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleCalibrationStatus() {
    logDebugSafe("Запрос статуса калибровки");
    
    // Проверяем инициализацию
    if (!gSensorCorrection.isInitialized()) {
        logWarnSafe("Система коррекции не инициализирована, инициализируем...");
        gSensorCorrection.init();
    }
    
    try {
        CorrectionFactors factors = gSensorCorrection.getCorrectionFactors();
        
        DynamicJsonDocument doc(1024);
        JsonObject root = doc.to<JsonObject>();
        
        // Принудительно добавляем все поля в JSON объект
        root["success"] = true;
        root["ph_calibrated"] = factors.phCalibrated;
        root["ec_calibrated"] = factors.ecCalibrated;
        root["temperature_calibrated"] = factors.temperatureCalibrated;
        root["humidity_calibrated"] = factors.humidityCalibrated;
        root["npk_calibrated"] = factors.npkCalibrated;
        root["calibration_enabled"] = factors.calibrationEnabled;
        
        String response;
        serializeJson(doc, response);
        
        logDebugSafe("Отправляем статус калибровки: %s", response.c_str());
        
        // Добавляем CORS заголовки
        addCORSHeaders();
        
        webServer.send(200, "application/json", response);
    } catch (...) {
        logErrorSafe("Ошибка при получении статуса калибровки");
        DynamicJsonDocument doc(512);
        doc["success"] = false;
        doc["error"] = "Ошибка инициализации";
        
        String response;
        serializeJson(doc, response);
        webServer.send(500, "application/json", response);
    }
}

 void handlePHCalibration() {
     logDebugSafe("Запрос калибровки pH");
     
     String body = webServer.arg("plain");
     DynamicJsonDocument doc(512);
     deserializeJson(doc, body);
    
         float expected_1 = doc["expected_1"];
     float measured_1 = doc["measured_1"];
     float expected_2 = doc["expected_2"];
     float measured_2 = doc["measured_2"];
     float expected_3 = doc["expected_3"];
     float measured_3 = doc["measured_3"];
     
     CalibrationResult result = gSensorCorrection.calculatePHCalibration(
         expected_1, expected_2, expected_3,
         measured_1, measured_2, measured_3
     );
    
    DynamicJsonDocument response(512);
    response["success"] = result.success;
    response["quality"] = result.quality;
    response["r_squared"] = result.r_squared;
    
    if (result.success) {
        // Применяем калибровку
        CorrectionFactors factors = gSensorCorrection.getCorrectionFactors();
        factors.phSlope = result.slope;
        factors.phOffset = result.offset;
        factors.phCalibrationQuality = result.r_squared;
        factors.phCalibrated = true;
        factors.lastCalibrationTime = millis();
        strcpy(factors.lastCalibratedBy, "Web Interface");
        gSensorCorrection.setCorrectionFactors(factors);
        
        logSuccess("pH калибровка успешна: R² = " + String(result.r_squared, 3));
    } else {
        response["error"] = "Недостаточное качество калибровки (R² < 0.95)";
        logWarn("pH калибровка неудачна: R² = " + String(result.r_squared, 3));
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    
    // Добавляем CORS заголовки
    addCORSHeaders();
    
    webServer.send(200, "application/json", responseStr);
}

void handleECCalibration() {
    logDebugSafe("Запрос калибровки EC");
    
    String body = webServer.arg("plain");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, body);
    
    float expected_1 = doc["expected_1"];
    float measured_1 = doc["measured_1"];
    float expected_2 = doc["expected_2"];
    float measured_2 = doc["measured_2"];
    
    CalibrationResult result = gSensorCorrection.calculateECCalibration(
        expected_1, expected_2, 0.0f,  // 3-я точка = 0 для 2-точечной калибровки
        measured_1, measured_2, 0.0f
    );
    
    DynamicJsonDocument response(512);
    response["success"] = result.success;
    response["quality"] = result.quality;
    response["r_squared"] = result.r_squared;
    
    if (result.success) {
        // Применяем калибровку
        CorrectionFactors factors = gSensorCorrection.getCorrectionFactors();
        factors.ecCalibrationSlope = result.slope;
        factors.ecCalibrationOffset = result.offset;
        factors.ecCalibrationQuality = result.r_squared;
        factors.ecCalibrated = true;
        factors.lastCalibrationTime = millis();
        strcpy(factors.lastCalibratedBy, "Web Interface");
        gSensorCorrection.setCorrectionFactors(factors);
        
        logSuccess("EC калибровка успешна: R² = " + String(result.r_squared, 3));
    } else {
        response["error"] = "Недостаточное качество калибровки";
        logWarn("EC калибровка неудачна: R² = " + String(result.r_squared, 3));
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    
    // Добавляем CORS заголовки
    addCORSHeaders();
    
    webServer.send(200, "application/json", responseStr);
}

void handleTemperatureCalibration() {
    logDebugSafe("Запрос калибровки температуры");
    
    String body = webServer.arg("plain");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, body);
    
    float reference = doc["reference"];
    float measured = doc["measured"];
    
    CalibrationResult result = gSensorCorrection.calculateTemperatureCalibration(
        reference, measured
    );
    
    DynamicJsonDocument response(512);
    response["success"] = result.success;
    response["quality"] = result.quality;
    
    if (result.success) {
        // Применяем калибровку
        CorrectionFactors factors = gSensorCorrection.getCorrectionFactors();
        factors.temperatureCalibrationSlope = result.slope;
        factors.temperatureCalibrationOffset = result.offset;
        factors.temperatureCalibrated = true;
        factors.lastCalibrationTime = millis();
        strcpy(factors.lastCalibratedBy, "Web Interface");
        gSensorCorrection.setCorrectionFactors(factors);
        
        logSuccess("Температурная калибровка успешна");
    } else {
        response["error"] = "Ошибка калибровки";
        logWarn("Температурная калибровка неудачна");
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    
    // Добавляем CORS заголовки
    addCORSHeaders();
    
    webServer.send(200, "application/json", responseStr);
}

void handleHumidityCalibration() {
    logDebugSafe("Запрос калибровки влажности");
    
    String body = webServer.arg("plain");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, body);
    
    float reference = doc["reference"];
    float measured = doc["measured"];
    
    CalibrationResult result = gSensorCorrection.calculateHumidityCalibration(
        reference, measured
    );
    
    DynamicJsonDocument response(512);
    response["success"] = result.success;
    response["quality"] = result.quality;
    
    if (result.success) {
        // Применяем калибровку
        CorrectionFactors factors = gSensorCorrection.getCorrectionFactors();
        factors.humidityCalibrationSlope = result.slope;
        factors.humidityCalibrationOffset = result.offset;
        factors.humidityCalibrated = true;
        factors.lastCalibrationTime = millis();
        strcpy(factors.lastCalibratedBy, "Web Interface");
        gSensorCorrection.setCorrectionFactors(factors);
        
        logSuccess("Влажностная калибровка успешна");
    } else {
        response["error"] = "Ошибка калибровки";
        logWarn("Влажностная калибровка неудачна");
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    
    // Добавляем CORS заголовки
    addCORSHeaders();
    
    webServer.send(200, "application/json", responseStr);
}

void handleNPKCalibration() {
    logDebugSafe("Запрос калибровки NPK");
    
    String body = webServer.arg("plain");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, body);
    
    float nitrogen = doc["nitrogen"];
    float phosphorus = doc["phosphorus"];
    float potassium = doc["potassium"];
    
    DynamicJsonDocument response(512);
    response["success"] = true;
    
    // Применяем калибровку (нулевая точка)
    CorrectionFactors factors = gSensorCorrection.getCorrectionFactors();
    factors.npkNitrogenOffset = nitrogen;
    factors.npkPhosphorusOffset = phosphorus;
    factors.npkPotassiumOffset = potassium;
    factors.npkCalibrated = true;
    factors.lastCalibrationTime = millis();
    strcpy(factors.lastCalibratedBy, "Web Interface");
    gSensorCorrection.setCorrectionFactors(factors);
    
    logSuccess("NPK калибровка успешна: N=" + String(nitrogen, 1) + ", P=" + String(phosphorus, 1) + ", K=" + String(potassium, 1));
    
    String responseStr;
    serializeJson(response, responseStr);
    
    // Добавляем CORS заголовки
    addCORSHeaders();
    
    webServer.send(200, "application/json", responseStr);
}

void handleEnableCalibration() {
    logDebugSafe("Включение калибровки");
    gSensorCorrection.enableCalibration(true);
    
    DynamicJsonDocument response(256);
    response["success"] = true;
    response["message"] = "Калибровка включена";
    
    String responseStr;
    serializeJson(response, responseStr);
    
    // Добавляем CORS заголовки
    addCORSHeaders();
    
    webServer.send(200, "application/json", responseStr);
}

void handleDisableCalibration() {
    logDebugSafe("Отключение калибровки");
    gSensorCorrection.enableCalibration(false);
    
    DynamicJsonDocument response(256);
    response["success"] = true;
    response["message"] = "Калибровка отключена";
    
    String responseStr;
    serializeJson(response, responseStr);
    
    // Добавляем CORS заголовки
    addCORSHeaders();
    
    webServer.send(200, "application/json", responseStr);
}

void handleResetCalibration() {
    logDebugSafe("Сброс всех калибровок");
    
    CorrectionFactors factors = gSensorCorrection.getCorrectionFactors();
    
    // Сбрасываем все калибровки
    factors.phCalibrated = false;
    factors.ecCalibrated = false;
    factors.temperatureCalibrated = false;
    factors.humidityCalibrated = false;
    factors.npkCalibrated = false;
    
    // Сбрасываем коэффициенты
    factors.phSlope = 1.0f;
    factors.phOffset = 0.0f;
    factors.ecCalibrationSlope = 1.0f;
    factors.ecCalibrationOffset = 0.0f;
    factors.temperatureCalibrationSlope = 1.0f;
    factors.temperatureCalibrationOffset = 0.0f;
    factors.humidityCalibrationSlope = 1.0f;
    factors.humidityCalibrationOffset = 0.0f;
    factors.npkNitrogenOffset = 0.0f;
    factors.npkPhosphorusOffset = 0.0f;
    factors.npkPotassiumOffset = 0.0f;
    
    gSensorCorrection.setCorrectionFactors(factors);
    
    DynamicJsonDocument response(256);
    response["success"] = true;
    response["message"] = "Все калибровки сброшены";
    
    String responseStr;
    serializeJson(response, responseStr);
    
    // Добавляем CORS заголовки
    addCORSHeaders();
    
    webServer.send(200, "application/json", responseStr);
}
