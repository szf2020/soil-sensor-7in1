#!/usr/bin/env python3
"""
Тест покрытия культур в системе JXCT
Проверяет, что все культуры правильно отображаются и работают
"""

import requests
from typing import List, Dict, Any

class CropCoverageTest:
    """Тест покрытия культур"""
    
    def __init__(self, base_url: str = "http://localhost:8080"):
        self.base_url = base_url
        self.session = requests.Session()
    
    def get_expected_crops(self) -> List[Dict[str, Any]]:
        """Возвращает список ожидаемых культур"""
        return [
            {"id": "none", "name": "нет", "category": "Базовые"},
            {"id": "tomato", "name": "Томат", "category": "Овощные"},
            {"id": "cucumber", "name": "Огурец", "category": "Овощные"},
            {"id": "pepper", "name": "Перец", "category": "Овощные"},
            {"id": "lettuce", "name": "Салат", "category": "Овощные"},
            {"id": "strawberry", "name": "Клубника", "category": "Ягодные"},
            {"id": "apple", "name": "Яблоня", "category": "Плодовые"},
            {"id": "pear", "name": "Груша", "category": "Плодовые"},
            {"id": "cherry", "name": "Вишня/Черешня", "category": "Плодовые"},
            {"id": "raspberry", "name": "Малина", "category": "Ягодные"},
            {"id": "currant", "name": "Смородина", "category": "Ягодные"},
            {"id": "blueberry", "name": "Голубика", "category": "Ягодные"},
            {"id": "lawn", "name": "Газон", "category": "Специальные"},
            {"id": "grape", "name": "Виноград", "category": "Плодовые"},
            {"id": "conifer", "name": "Хвойные деревья", "category": "Специальные"},
            {"id": "spinach", "name": "Шпинат", "category": "Новые (Фаза 1)"},
            {"id": "basil", "name": "Базилик", "category": "Новые (Фаза 1)"},
            {"id": "cannabis", "name": "Конопля медицинская", "category": "Новые (Фаза 1)"},
            {"id": "wheat", "name": "Пшеница", "category": "Новые (Фаза 2)"},
            {"id": "potato", "name": "Картофель", "category": "Новые (Фаза 2)"},
            {"id": "kale", "name": "Кале", "category": "Новые (Фаза 3)"},
            {"id": "blackberry", "name": "Ежевика", "category": "Новые (Фаза 3)"},
            {"id": "soybean", "name": "Соя", "category": "Новые (Фаза 3)"},
            {"id": "carrot", "name": "Морковь", "category": "Новые (Фаза 3)"},
        ]
    
    def test_main_page_crops(self) -> Dict[str, Any]:
        """Тестирует отображение культур на главной странице"""
        try:
            response = self.session.get(f"{self.base_url}/")
            response.raise_for_status()
            
            html_content = response.text
            expected_crops = self.get_expected_crops()
            
            missing_crops = []
            found_crops = []
            
            for crop in expected_crops:
                # Проверяем наличие option с value
                if f'value=\'{crop["id"]}\'' in html_content:
                    found_crops.append(crop)
                else:
                    missing_crops.append(crop)
            
            return {
                "success": len(missing_crops) == 0,
                "total_expected": len(expected_crops),
                "found": len(found_crops),
                "missing": missing_crops,
                "found_crops": found_crops
            }
            
        except Exception as e:
            return {
                "success": False,
                "error": str(e),
                "total_expected": len(self.get_expected_crops()),
                "found": 0,
                "missing": self.get_expected_crops(),
                "found_crops": []
            }
    
    def test_crop_recommendations(self) -> Dict[str, Any]:
        """Тестирует получение рекомендаций для всех культур"""
        test_data = {
            "temperature": 22.0,
            "humidity": 70.0,
            "ec": 1500.0,
            "ph": 6.5,
            "nitrogen": 150.0,
            "phosphorus": 60.0,
            "potassium": 200.0
        }
        
        results = {}
        expected_crops = self.get_expected_crops()
        
        for crop in expected_crops:
            if crop["id"] == "none":
                continue  # Пропускаем "нет"
                
            try:
                # Тестируем API рекомендаций
                response = self.session.post(
                    f"{self.base_url}/api/recommendations",
                    json={
                        "crop": crop["id"],
                        "data": test_data,
                        "environment": "outdoor",
                        "season": "summer",
                        "soil": "loam"
                    },
                    timeout=10
                )
                
                if response.status_code == 200:
                    results[crop["id"]] = {
                        "success": True,
                        "status_code": response.status_code,
                        "data": response.json()
                    }
                else:
                    results[crop["id"]] = {
                        "success": False,
                        "status_code": response.status_code,
                        "error": response.text
                    }
                    
            except Exception as e:
                results[crop["id"]] = {
                    "success": False,
                    "error": str(e)
                }
        
        successful = sum(1 for r in results.values() if r.get("success", False))
        total = len(results)
        
        return {
            "success": successful == total,
            "total_tested": total,
            "successful": successful,
            "failed": total - successful,
            "results": results
        }
    
    def test_crop_config_api(self) -> Dict[str, Any]:
        """Тестирует API конфигурации культур"""
        try:
            response = self.session.get(f"{self.base_url}/api/crops")
            response.raise_for_status()
            
            crops_data = response.json()
            expected_crops = self.get_expected_crops()
            
            found_crops = []
            missing_crops = []
            
            for expected in expected_crops:
                found = False
                for crop_data in crops_data:
                    if crop_data.get("id") == expected["id"]:
                        found_crops.append({
                            "expected": expected,
                            "actual": crop_data
                        })
                        found = True
                        break
                
                if not found:
                    missing_crops.append(expected)
            
            return {
                "success": len(missing_crops) == 0,
                "total_expected": len(expected_crops),
                "found": len(found_crops),
                "missing": missing_crops,
                "crops_data": crops_data
            }
            
        except Exception as e:
            return {
                "success": False,
                "error": str(e),
                "total_expected": len(self.get_expected_crops()),
                "found": 0,
                "missing": self.get_expected_crops(),
                "crops_data": []
            }
    
    def run_all_tests(self) -> Dict[str, Any]:
        """Запускает все тесты"""
        print("🌱 Запуск тестов покрытия культур...")
        
        results = {
            "main_page": self.test_main_page_crops(),
            "recommendations": self.test_crop_recommendations(),
            "config_api": self.test_crop_config_api()
        }
        
        # Общая статистика
        total_tests = 3
        passed_tests = sum(1 for r in results.values() if r.get("success", False))
        
        results["summary"] = {
            "total_tests": total_tests,
            "passed": passed_tests,
            "failed": total_tests - passed_tests,
            "success_rate": (passed_tests / total_tests) * 100 if total_tests > 0 else 0
        }
        
        return results

def main():
    """Основная функция"""
    print("🔍 ТЕСТ ПОКРЫТИЯ КУЛЬТУР JXCT")
    print("=" * 50)
    
    # Тест без реального сервера (симуляция)
    test = CropCoverageTest()
    
    # Проверяем код на наличие всех культур
    print("\n📋 Проверка кода...")
    
    # Проверяем файлы на наличие культур
    import os
    
    # Проверяем routes_main.cpp
    routes_file = "src/web/routes_main.cpp"
    if os.path.exists(routes_file):
        with open(routes_file, 'r', encoding='utf-8') as f:
            content = f.read()
            
        expected_crops = test.get_expected_crops()
        missing_in_routes = []
        
        for crop in expected_crops:
            if f"value='{crop['id']}'" not in content:
                missing_in_routes.append(crop)
        
        print(f"✅ Routes: {len(expected_crops) - len(missing_in_routes)}/{len(expected_crops)} культур найдено")
        if missing_in_routes:
            print(f"❌ Отсутствуют в routes: {[c['id'] for c in missing_in_routes]}")
    
    # Проверяем crop_recommendation_engine.cpp
    engine_file = "src/business/crop_recommendation_engine.cpp"
    if os.path.exists(engine_file):
        with open(engine_file, 'r', encoding='utf-8') as f:
            content = f.read()
            
        expected_crops = test.get_expected_crops()
        missing_in_engine = []
        
        for crop in expected_crops:
            if crop["id"] == "none":
                continue  # "none" не имеет конфигурации
            if f'cropConfigs["{crop["id"]}"]' not in content:
                missing_in_engine.append(crop)
        
        print(f"✅ Engine: {len(expected_crops) - len(missing_in_engine) - 1}/{len(expected_crops) - 1} культур найдено")
        if missing_in_engine:
            print(f"❌ Отсутствуют в engine: {[c['id'] for c in missing_in_engine]}")
    
    # Проверяем документацию
    docs_file = "docs/manuals/AGRO_RECOMMENDATIONS.md"
    if os.path.exists(docs_file):
        with open(docs_file, 'r', encoding='utf-8') as f:
            content = f.read()
            
        expected_crops = test.get_expected_crops()
        missing_in_docs = []
        
        for crop in expected_crops:
            if crop["id"] not in content:
                missing_in_docs.append(crop)
        
        print(f"✅ Docs: {len(expected_crops) - len(missing_in_docs)}/{len(expected_crops)} культур найдено")
        if missing_in_docs:
            print(f"❌ Отсутствуют в docs: {[c['id'] for c in missing_in_docs]}")
    
    print("\n📊 ИТОГОВАЯ СТАТИСТИКА:")
    print(f"Всего культур: {len(test.get_expected_crops())}")
    print("Категории:")
    
    categories = {}
    for crop in test.get_expected_crops():
        cat = crop["category"]
        if cat not in categories:
            categories[cat] = []
        categories[cat].append(crop["name"])
    
    for category, crops in categories.items():
        print(f"  {category}: {len(crops)} культур")
        for crop in crops:
            print(f"    - {crop}")
    
    print("\n✅ Тест завершен!")

if __name__ == "__main__":
    main() 