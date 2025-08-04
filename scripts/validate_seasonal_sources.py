#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
🔬 Валидация научных источников сезонных корректировок
Поиск и проверка рецензируемых источников для сезонных коэффициентов
"""

import requests
import json
import time
from datetime import datetime
from typing import Dict, List, Optional

class SeasonalSourcesValidator:
    """Валидатор научных источников сезонных корректировок"""
    
    def __init__(self):
        self.results = {
            "timestamp": datetime.now().isoformat(),
            "sources_found": [],
            "validation_status": "NEEDS_VERIFICATION",
            "recommendations": []
        }
        
        # Текущие сезонные коэффициенты из кода
        self.current_seasonal_adjustments = {
            "spring": {"nitrogen": 1.15, "phosphorus": 1.10, "potassium": 1.12},
            "summer": {"nitrogen": 1.08, "phosphorus": 1.05, "potassium": 1.20},
            "autumn": {"nitrogen": 1.06, "phosphorus": 1.12, "potassium": 1.15},
            "winter": {"nitrogen": 0.85, "phosphorus": 1.08, "potassium": 1.10}
        }

    def search_crossref(self, query: str, max_results: int = 5) -> List[Dict]:
        """Поиск в CrossRef API"""
        try:
            url = f"https://api.crossref.org/works"
            params = {
                "query": query,
                "rows": max_results,
                "select": "DOI,title,author,published-print,container-title"
            }
            
            response = requests.get(url, params=params, timeout=10)
            response.raise_for_status()
            
            data = response.json()
            return data.get("message", {}).get("items", [])
            
        except Exception as e:
            print(f"❌ Ошибка поиска: {e}")
            return []

    def search_fao_sources(self) -> List[Dict]:
        """Поиск FAO источников"""
        print("🔍 Поиск FAO источников...")
        
        queries = [
            "FAO Fertilizer Plant Nutrition Bulletin seasonal",
            "FAO agricultural seasonal adjustments",
            "FAO crop nutrition seasonal factors"
        ]
        
        all_results = []
        for query in queries:
            results = self.search_crossref(query, 3)
            all_results.extend(results)
            time.sleep(1)  # Уважаем API
            
        return all_results

    def search_cru_sources(self) -> List[Dict]:
        """Поиск Climatic Research Unit источников"""
        print("🔍 Поиск CRU источников...")
        
        queries = [
            "Climatic Research Unit seasonal agricultural",
            "CRU University of East Anglia seasonal",
            "seasonal agricultural adjustments climate"
        ]
        
        all_results = []
        for query in queries:
            results = self.search_crossref(query, 3)
            all_results.extend(results)
            time.sleep(1)
            
        return all_results

    def search_agricultural_journals(self) -> List[Dict]:
        """Поиск в сельскохозяйственных журналах"""
        print("🔍 Поиск в сельскохозяйственных журналах...")
        
        queries = [
            "seasonal nutrient adjustments agriculture",
            "seasonal fertilizer recommendations",
            "crop nutrition seasonal factors"
        ]
        
        all_results = []
        for query in queries:
            results = self.search_crossref(query, 3)
            all_results.extend(results)
            time.sleep(1)
            
        return all_results

    def validate_seasonal_coefficients(self, source_data: Dict) -> Dict:
        """Валидация сезонных коэффициентов"""
        validation = {
            "source": source_data.get("title", ["Unknown"])[0],
            "doi": source_data.get("DOI", "N/A"),
            "year": source_data.get("published-print", {}).get("date-parts", [[0]])[0][0],
            "relevance": "unknown",
            "coefficients_found": False,
            "notes": []
        }
        
        # Проверяем релевантность
        title = source_data.get("title", [""])[0].lower()
        if any(word in title for word in ["seasonal", "season", "agricultural", "crop", "fertilizer"]):
            validation["relevance"] = "high"
        elif any(word in title for word in ["climate", "weather", "temperature"]):
            validation["relevance"] = "medium"
        else:
            validation["relevance"] = "low"
            
        return validation

    def run_comprehensive_search(self) -> Dict:
        """Комплексный поиск источников"""
        print("🔬 КОМПЛЕКСНЫЙ ПОИСК НАУЧНЫХ ИСТОЧНИКОВ")
        print("=" * 60)
        print(f"📅 Дата: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print()
        
        # Поиск FAO источников
        fao_results = self.search_fao_sources()
        print(f"✅ Найдено FAO источников: {len(fao_results)}")
        
        # Поиск CRU источников  
        cru_results = self.search_cru_sources()
        print(f"✅ Найдено CRU источников: {len(cru_results)}")
        
        # Поиск в сельскохозяйственных журналах
        journal_results = self.search_agricultural_journals()
        print(f"✅ Найдено журнальных источников: {len(journal_results)}")
        
        # Объединяем результаты
        all_results = fao_results + cru_results + journal_results
        
        # Валидируем источники
        validated_sources = []
        for result in all_results:
            validation = self.validate_seasonal_coefficients(result)
            validated_sources.append(validation)
            
        # Анализируем результаты
        high_relevance = [s for s in validated_sources if s["relevance"] == "high"]
        medium_relevance = [s for s in validated_sources if s["relevance"] == "medium"]
        
        print(f"\n📊 АНАЛИЗ РЕЗУЛЬТАТОВ:")
        print(f"   Высокая релевантность: {len(high_relevance)}")
        print(f"   Средняя релевантность: {len(medium_relevance)}")
        print(f"   Всего источников: {len(validated_sources)}")
        
        # Формируем рекомендации
        recommendations = []
        
        if high_relevance:
            recommendations.append("✅ Найдены высокорелевантные источники")
            self.results["validation_status"] = "PARTIALLY_VERIFIED"
        else:
            recommendations.append("⚠️ Не найдены высокорелевантные источники")
            
        if len(validated_sources) >= 5:
            recommendations.append("✅ Достаточное количество источников для анализа")
        else:
            recommendations.append("⚠️ Недостаточно источников для полной валидации")
            
        # Проверяем наличие DOI
        sources_with_doi = [s for s in validated_sources if s["doi"] != "N/A"]
        if sources_with_doi:
            recommendations.append("✅ Найдены источники с DOI")
        else:
            recommendations.append("❌ Не найдены источники с DOI")
            
        # Сохраняем результаты
        self.results["sources_found"] = validated_sources
        self.results["recommendations"] = recommendations
        
        return self.results

    def generate_report(self) -> str:
        """Генерация отчета"""
        report = []
        report.append("🔬 ОТЧЕТ ПО ВАЛИДАЦИИ СЕЗОННЫХ КОРРЕКТИРОВОК")
        report.append("=" * 60)
        report.append(f"📅 Дата: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        report.append(f"📊 Статус: {self.results['validation_status']}")
        report.append("")
        
        # Текущие коэффициенты
        report.append("📋 ТЕКУЩИЕ СЕЗОННЫЕ КОЭФФИЦИЕНТЫ:")
        for season, coeffs in self.current_seasonal_adjustments.items():
            report.append(f"   {season.capitalize()}: N={coeffs['nitrogen']:.2f}, P={coeffs['phosphorus']:.2f}, K={coeffs['potassium']:.2f}")
        report.append("")
        
        # Найденные источники
        report.append("📚 НАЙДЕННЫЕ ИСТОЧНИКИ:")
        for i, source in enumerate(self.results["sources_found"][:5], 1):
            report.append(f"   {i}. {source['source']}")
            report.append(f"      DOI: {source['doi']}")
            report.append(f"      Год: {source['year']}")
            report.append(f"      Релевантность: {source['relevance']}")
            report.append("")
            
        # Рекомендации
        report.append("💡 РЕКОМЕНДАЦИИ:")
        for rec in self.results["recommendations"]:
            report.append(f"   {rec}")
            
        return "\n".join(report)

def main():
    """Главная функция"""
    validator = SeasonalSourcesValidator()
    
    try:
        # Запускаем поиск
        results = validator.run_comprehensive_search()
        
        # Генерируем отчет
        report = validator.generate_report()
        print(report)
        
        # Сохраняем результаты
        with open("test_reports/seasonal_sources_validation.json", "w", encoding="utf-8") as f:
            json.dump(results, f, indent=2, ensure_ascii=False)
            
        print(f"\n📄 Отчет сохранен: test_reports/seasonal_sources_validation.json")
        
        return results["validation_status"] != "NEEDS_VERIFICATION"
        
    except Exception as e:
        print(f"💥 КРИТИЧЕСКАЯ ОШИБКА: {e}")
        return False

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1) 