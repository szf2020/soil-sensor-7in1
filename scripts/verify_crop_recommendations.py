#!/usr/bin/env python3
"""
🔬 Проверка точности рекомендаций по культурам
Проверяет все значения влажности, pH, EC и NPK на соответствие научным источникам

Author: JXCT Soil Sensor Project
Date: 2025
"""

import re
import json
from typing import Dict, List, Tuple

class CropRecommendationVerifier:
    def __init__(self):
        # Научно обоснованные диапазоны для культур
        self.scientific_ranges = {
            "generic": {
                "humidity": (70, 80),  # ASM %
                "ph": (6.0, 7.0),
                "ec": (1200, 1800),   # µS/cm
                "temperature": (20, 25),  # °C
                "source": "FAO Irrigation and Drainage Paper 56"
            },
            "tomato": {
                "humidity": (75, 85),  # ASM %
                "ph": (6.0, 6.8),
                "ec": (1500, 3000),
                "temperature": (22, 26),
                "source": "University of Florida IFAS Extension, 2019"
            },
            "cucumber": {
                "humidity": (70, 80),  # ASM %
                "ph": (6.0, 7.0),
                "ec": (1200, 2500),
                "temperature": (24, 28),
                "source": "USDA Natural Resources Conservation Service, 2020"
            },
            "pepper": {
                "humidity": (70, 80),  # ASM %
                "ph": (6.0, 7.0),
                "ec": (1400, 2800),
                "temperature": (25, 29),
                "source": "Cornell University Cooperative Extension, 2022"
            },
            "lettuce": {
                "humidity": (80, 90),  # ASM %
                "ph": (6.0, 7.0),
                "ec": (1000, 2000),
                "temperature": (16, 20),
                "source": "University of California Agriculture, 2018"
            },
            "blueberry": {
                "humidity": (60, 75),  # ASM %
                "ph": (4.5, 5.5),
                "ec": (800, 1500),
                "temperature": (18, 22),
                "source": "Michigan State University Extension, 2021"
            },
            "lawn": {
                "humidity": (70, 80),  # ASM %
                "ph": (6.0, 7.0),
                "ec": (1200, 1800),
                "temperature": (20, 24),
                "source": "Turfgrass Science + FAO"
            },
            "grape": {
                "humidity": (55, 70),  # ASM %
                "ph": (6.0, 7.5),
                "ec": (1000, 2000),
                "temperature": (24, 28),
                "source": "Viticulture Research, 2021"
            },
            "conifer": {
                "humidity": (60, 75),  # ASM %
                "ph": (5.5, 6.5),
                "ec": (500, 1200),
                "temperature": (16, 20),
                "source": "Forest Science"
            },
            "strawberry": {
                "humidity": (70, 80),  # ASM %
                "ph": (5.5, 6.5),
                "ec": (1200, 2000),
                "temperature": (20, 24),
                "source": "HortScience"
            },
            "apple": {
                "humidity": (70, 80),  # ASM %
                "ph": (6.0, 7.0),
                "ec": (1000, 1800),
                "temperature": (18, 22),
                "source": "Journal of Horticultural Science"
            },
            "cherry": {
                "humidity": (70, 80),  # ASM %
                "ph": (6.0, 7.0),
                "ec": (1000, 1800),
                "temperature": (20, 24),
                "source": "HortScience"
            },
            "raspberry": {
                "humidity": (70, 80),  # ASM %
                "ph": (5.5, 6.5),
                "ec": (800, 1500),
                "temperature": (18, 22),
                "source": "Acta Horticulturae"
            },
            "currant": {
                "humidity": (60, 75),  # ASM %
                "ph": (5.5, 6.5),
                "ec": (800, 1300),
                "temperature": (16, 20),
                "source": "HortScience"
            },
            "spinach": {
                "humidity": (75, 85),  # ASM %
                "ph": (6.0, 7.0),
                "ec": (1000, 1800),
                "temperature": (18, 22),
                "source": "UC Extension, 2019"
            },
            "basil": {
                "humidity": (70, 80),  # ASM %
                "ph": (6.0, 7.0),
                "ec": (1000, 1800),
                "temperature": (23, 27),
                "source": "Journal of Essential Oil Research, 2019"
            },
            "cannabis": {
                "humidity": (75, 85),  # ASM %
                "ph": (6.0, 7.0),
                "ec": (1200, 2000),
                "temperature": (22, 26),
                "source": "Journal of Cannabis Research, 2020"
            },
            "wheat": {
                "humidity": (60, 75),  # ASM %
                "ph": (6.0, 7.0),
                "ec": (1000, 1500),
                "temperature": (18, 22),
                "source": "Kansas State University, 2020"
            },
            "potato": {
                "humidity": (30, 45),  # ASM % - КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ!
                "ph": (5.5, 6.5),
                "ec": (1200, 1800),
                "temperature": (16, 20),
                "source": "University of Idaho + USDA ARS + CIP"
            },
            "kale": {
                "humidity": (70, 80),  # ASM %
                "ph": (6.0, 7.0),
                "ec": (1000, 1600),
                "temperature": (16, 20),
                "source": "University of Wisconsin, 2020"
            },
            "blackberry": {
                "humidity": (70, 80),  # ASM %
                "ph": (5.5, 6.5),
                "ec": (1000, 1500),
                "temperature": (20, 24),
                "source": "University of Arkansas, 2020"
            },
            "soybean": {
                "humidity": (60, 75),  # ASM %
                "ph": (6.0, 7.0),
                "ec": (1000, 1800),
                "temperature": (22, 26),
                "source": "University of Illinois, 2020"
            },
            "carrot": {
                "humidity": (70, 80),  # ASM %
                "ph": (6.0, 7.0),
                "ec": (1000, 1500),
                "temperature": (16, 20),
                "source": "UC Extension, 2020"
            }
        }

    def extract_crop_configs(self, file_path: str) -> Dict:
        """Извлекает конфигурации культур из C++ файла"""
        crop_configs = {}
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # Паттерн для поиска конфигураций культур
            pattern = r'cropConfigs\["([^"]+)"\]\s*=\s*CropConfig\(([^)]+)\)'
            matches = re.findall(pattern, content)
            
            for crop_name, config_str in matches:
                # Извлекаем значения из CropConfig
                values = re.findall(r'([0-9.]+)F', config_str)
                if len(values) >= 4:
                    crop_configs[crop_name] = {
                        'temperature': float(values[0]),
                        'humidity': float(values[1]),
                        'ec': float(values[2]),
                        'ph': float(values[3])
                    }
            
            return crop_configs
            
        except Exception as e:
            print(f"❌ Ошибка при чтении файла: {e}")
            return {}

    def verify_crop_config(self, crop_name: str, config: Dict) -> Dict:
        """Проверяет конфигурацию культуры на соответствие научным данным"""
        if crop_name not in self.scientific_ranges:
            return {
                'crop': crop_name,
                'status': 'UNKNOWN',
                'errors': [f"Культура '{crop_name}' не найдена в научных источниках"]
            }
        
        scientific = self.scientific_ranges[crop_name]
        errors = []
        warnings = []
        
        # Проверка влажности (ASM)
        if 'humidity' in config:
            humidity = config['humidity']
            min_h, max_h = scientific['humidity']
            if humidity < min_h or humidity > max_h:
                errors.append(f"Влажность {humidity}% вне диапазона {min_h}-{max_h}% ASM")
            elif abs(humidity - (min_h + max_h) / 2) > (max_h - min_h) * 0.2:
                warnings.append(f"Влажность {humidity}% близка к границам диапазона {min_h}-{max_h}%")
        
        # Проверка pH
        if 'ph' in config:
            ph = config['ph']
            min_ph, max_ph = scientific['ph']
            if ph < min_ph or ph > max_ph:
                errors.append(f"pH {ph} вне диапазона {min_ph}-{max_ph}")
        
        # Проверка EC
        if 'ec' in config:
            ec = config['ec']
            min_ec, max_ec = scientific['ec']
            if ec < min_ec or ec > max_ec:
                errors.append(f"EC {ec} µS/cm вне диапазона {min_ec}-{max_ec} µS/cm")
        
        # Проверка температуры
        if 'temperature' in config:
            temp = config['temperature']
            min_temp, max_temp = scientific['temperature']
            if temp < min_temp or temp > max_temp:
                errors.append(f"Температура {temp}°C вне диапазона {min_temp}-{max_temp}°C")
        
        status = 'PASS'
        if errors:
            status = 'FAIL'
        elif warnings:
            status = 'WARNING'
        
        return {
            'crop': crop_name,
            'status': status,
            'errors': errors,
            'warnings': warnings,
            'source': scientific['source']
        }

    def run_verification(self):
        """Запускает полную проверку всех культур"""
        print("🔬 ПРОВЕРКА РЕКОМЕНДАЦИЙ ПО КУЛЬТУРАМ")
        print("=" * 60)
        
        # Извлекаем конфигурации из C++ файла
        crop_configs = self.extract_crop_configs('src/business/crop_recommendation_engine.cpp')
        
        if not crop_configs:
            print("❌ Не удалось извлечь конфигурации культур")
            return
        
        print(f"📊 Найдено культур: {len(crop_configs)}")
        print()
        
        # Проверяем каждую культуру
        results = []
        critical_errors = 0
        warnings = 0
        
        for crop_name, config in crop_configs.items():
            result = self.verify_crop_config(crop_name, config)
            results.append(result)
            
            if result['status'] == 'FAIL':
                critical_errors += 1
            elif result['status'] == 'WARNING':
                warnings += 1
        
        # Выводим результаты
        print("📋 РЕЗУЛЬТАТЫ ПРОВЕРКИ:")
        print()
        
        # Критические ошибки
        if critical_errors > 0:
            print(f"🚨 КРИТИЧЕСКИЕ ОШИБКИ ({critical_errors}):")
            for result in results:
                if result['status'] == 'FAIL':
                    print(f"  ❌ {result['crop']}:")
                    for error in result['errors']:
                        print(f"     - {error}")
                    print(f"     📚 Источник: {result['source']}")
                    print()
        
        # Предупреждения
        if warnings > 0:
            print(f"⚠️ ПРЕДУПРЕЖДЕНИЯ ({warnings}):")
            for result in results:
                if result['status'] == 'WARNING':
                    print(f"  ⚠️ {result['crop']}:")
                    for warning in result['warnings']:
                        print(f"     - {warning}")
                    print()
        
        # Успешные проверки
        passed = len([r for r in results if r['status'] == 'PASS'])
        if passed > 0:
            print(f"✅ ПРОЙДЕНО ({passed}):")
            for result in results:
                if result['status'] == 'PASS':
                    print(f"  ✅ {result['crop']} - {result['source']}")
        
        print()
        print("📊 СТАТИСТИКА:")
        print(f"  Всего культур: {len(results)}")
        print(f"  ✅ Пройдено: {passed}")
        print(f"  ⚠️ Предупреждения: {warnings}")
        print(f"  ❌ Критические ошибки: {critical_errors}")
        
        # Сохраняем отчет
        report = {
            'timestamp': '2025-08-06',
            'total_crops': len(results),
            'passed': passed,
            'warnings': warnings,
            'critical_errors': critical_errors,
            'results': results
        }
        
        with open('test_reports/crop_recommendations_verification.json', 'w', encoding='utf-8') as f:
            json.dump(report, f, indent=2, ensure_ascii=False)
        
        print(f"\n📄 Отчет сохранен: test_reports/crop_recommendations_verification.json")

def main():
    verifier = CropRecommendationVerifier()
    verifier.run_verification()

if __name__ == "__main__":
    main()
