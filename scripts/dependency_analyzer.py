#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
JXCT Dependency Analyzer v1.0
Dependency analysis between modules for safe refactoring
"""
import sys
import os

# Принудительно устанавливаем UTF-8 для вывода в Windows
if sys.platform.startswith('win'):
    import codecs
    import io
    # Перенаправляем stdout и stderr через UTF-8
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

import os
import re
import json
from pathlib import Path
from typing import Dict, List, Set, Tuple
from dataclasses import dataclass
from collections import defaultdict

@dataclass
class ModuleInfo:
    name: str
    path: str
    dependencies: Set[str]
    dependents: Set[str]
    critical_level: int  # 0-5, где 5 - критический
    category: str

class DependencyAnalyzer:
    def __init__(self, project_root: str = "."):
        self.project_root = Path(project_root)
        self.modules: Dict[str, ModuleInfo] = {}
        self.critical_modules = {
            "main.cpp": 5,
            "validation_utils.cpp": 4,
            "sensor_compensation_service.cpp": 4,
            "calibration_manager.cpp": 4,
            "advanced_filters.cpp": 3,
            "mqtt_client.cpp": 3,
            "wifi_manager.cpp": 3,
            "web_routes.cpp": 3
        }

    def analyze_project(self) -> Dict:
        """Full dependency analysis of the project"""
        print("[INFO] Analyzing JXCT project dependencies...")

        # Scan all C++ files
        cpp_files = list(self.project_root.rglob("*.cpp"))
        h_files = list(self.project_root.rglob("*.h"))

        # Analyze dependencies
        for file_path in cpp_files + h_files:
            if "test/" in str(file_path) or "docs/" in str(file_path):
                continue
            self._analyze_file(file_path)

        # Build dependency graph
        dependency_graph = self._build_dependency_graph()

        # Analyze critical paths
        critical_paths = self._find_critical_paths()

        # Generate report
        report = {
            "modules": {name: {
                "path": info.path,
                "dependencies": list(info.dependencies),
                "dependents": list(info.dependents),
                "critical_level": info.critical_level,
                "category": info.category
            } for name, info in self.modules.items()},
            "dependency_graph": dependency_graph,
            "critical_paths": critical_paths,
            "recommendations": self._generate_recommendations()
        }

        return report

    def _analyze_file(self, file_path: Path):
        """Анализ отдельного файла"""
        try:
            content = file_path.read_text(encoding='utf-8', errors='ignore')
            module_name = file_path.name

            # Определяем категорию
            category = self._determine_category(file_path)

            # Находим зависимости
            dependencies = self._extract_dependencies(content, file_path)

            # Определяем критический уровень
            critical_level = self.critical_modules.get(module_name, 1)

            self.modules[module_name] = ModuleInfo(
                name=module_name,
                path=str(file_path),
                dependencies=dependencies,
                dependents=set(),
                critical_level=critical_level,
                category=category
            )

        except Exception as e:
            print(f"[WARNING] Ошибка анализа {file_path}: {e}")

    def _determine_category(self, file_path: Path) -> str:
        """Определяет категорию модуля"""
        path_str = str(file_path)

        if "business/" in path_str:
            return "business_logic"
        elif "web/" in path_str:
            return "web_interface"
        elif "include/" in path_str:
            return "headers"
        elif file_path.name == "main.cpp":
            return "core"
        elif "validation" in file_path.name:
            return "validation"
        elif "sensor" in file_path.name:
            return "sensors"
        elif "mqtt" in file_path.name or "wifi" in file_path.name:
            return "communication"
        else:
            return "utilities"

    def _extract_dependencies(self, content: str, file_path: Path) -> Set[str]:
        """Извлекает зависимости из содержимого файла"""
        dependencies = set()

        # Ищем #include
        include_pattern = r'#include\s*["<]([^">]+)[">]'
        includes = re.findall(include_pattern, content)

        for include in includes:
            # Убираем расширения и пути
            module_name = Path(include).stem
            if module_name and not module_name.startswith(('Arduino', 'WiFi', 'ESP32')):
                dependencies.add(module_name)

        return dependencies

    def _build_dependency_graph(self) -> Dict:
        """Строит граф зависимостей"""
        # Обновляем dependents
        for module_name, module_info in self.modules.items():
            for dep in module_info.dependencies:
                if dep in self.modules:
                    self.modules[dep].dependents.add(module_name)

        return {
            "nodes": len(self.modules),
            "edges": sum(len(m.dependencies) for m in self.modules.values()),
            "critical_modules": [name for name, info in self.modules.items()
                               if info.critical_level >= 4]
        }

    def _find_critical_paths(self) -> List[List[str]]:
        """Находит критические пути в графе зависимостей"""
        critical_paths = []

        # Ищем пути к main.cpp
        main_deps = self.modules.get("main.cpp", ModuleInfo("", "", set(), set(), 0, ""))
        if main_deps.dependencies:
            for dep in main_deps.dependencies:
                if dep in self.modules:
                    path = self._find_path_to_module(dep, set())
                    if path:
                        critical_paths.append(path)

        return critical_paths

    def _find_path_to_module(self, target: str, visited: Set[str]) -> List[str]:
        """Находит путь к модулю"""
        if target in visited:
            return []

        visited.add(target)
        module = self.modules.get(target)
        if not module:
            return []

        if module.critical_level >= 4:
            return [target]

        for dep in module.dependencies:
            if dep in self.modules:
                path = self._find_path_to_module(dep, visited)
                if path:
                    return [target] + path

        return []

    def _generate_recommendations(self) -> List[str]:
        """Генерирует рекомендации для рефакторинга"""
        recommendations = []

        # Анализ циклических зависимостей
        cycles = self._find_cycles()
        if cycles:
            recommendations.append(f"[WARNING] Обнаружены циклические зависимости: {cycles}")

        # Анализ критических модулей
        critical_count = sum(1 for m in self.modules.values() if m.critical_level >= 4)
        if critical_count > 5:
            recommendations.append(f"[WARNING] Много критических модулей ({critical_count}). Рассмотрите разделение.")

        # Анализ размера модулей
        large_modules = [name for name, info in self.modules.items()
                        if len(info.dependencies) > 10]
        if large_modules:
            recommendations.append(f"[WARNING] Большие модули с множественными зависимостями: {large_modules}")

        return recommendations

    def _find_cycles(self) -> List[List[str]]:
        """Находит циклические зависимости"""
        # Упрощенная реализация - в реальности нужен DFS
        return []

def main():
    analyzer = DependencyAnalyzer()
    report = analyzer.analyze_project()

    # Сохраняем отчет
    output_file = "test_reports/dependency_analysis.json"
    os.makedirs(os.path.dirname(output_file), exist_ok=True)

    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(report, f, indent=2, ensure_ascii=False)

    print(f"[SUCCESS] Отчет сохранен: {output_file}")
    print(f"[INFO] Проанализировано модулей: {len(report['modules'])}")
    print(f"[INFO] Критических путей: {len(report['critical_paths'])}")

    if report['recommendations']:
        print("\n[INFO] РЕКОМЕНДАЦИИ:")
        for rec in report['recommendations']:
            print(f"  {rec}")

if __name__ == "__main__":
    main()
