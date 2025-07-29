#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
🔍 Быстрая проверка готовности к рефакторингу JXCT
Упрощенная версия без зависающих анализов
"""

import os
import sys
import subprocess
import time
from pathlib import Path
from typing import Dict, List, Any
from datetime import datetime
import io

# Принудительно устанавливаем UTF-8 для вывода в Windows
if sys.platform.startswith('win'):
    import codecs
    # Перенаправляем stdout и stderr через UTF-8
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

class QuickRefactoringChecker:
    def __init__(self, project_root: str = "."):
        self.project_root = Path(project_root)
        self.results = {}
        self.start_time = time.time()

    def run_quick_check(self) -> Dict[str, Any]:
        """Запускает быструю проверку готовности к рефакторингу"""
        print("🔍 БЫСТРАЯ ПРОВЕРКА ГОТОВНОСТИ К РЕФАКТОРИНГУ")
        print("=" * 60)

        # 1. Проверка сборки
        print("\n1️⃣ ПРОВЕРКА СБОРКИ...")
        self.results["build"] = self._check_build()

        # 2. Проверка тестов
        print("\n2️⃣ ПРОВЕРКА ТЕСТОВ...")
        self.results["tests"] = self._check_tests()

        # 3. Проверка размера файлов
        print("\n3️⃣ ПРОВЕРКА РАЗМЕРОВ ФАЙЛОВ...")
        self.results["file_sizes"] = self._check_file_sizes()

        # 4. Проверка зависимостей
        print("\n4️⃣ ПРОВЕРКА ЗАВИСИМОСТЕЙ...")
        self.results["dependencies"] = self._check_dependencies()

        return self.results

    def _check_build(self) -> Dict[str, Any]:
        """Проверяет сборку проекта"""
        try:
            print("  Сборка ESP32...")
            result = subprocess.run(
                ["pio", "run", "-e", "esp32dev"],
                capture_output=True,
                text=True,
                encoding='utf-8',
                errors='replace',
                cwd=self.project_root,
                timeout=60  # Таймаут 60 секунд
            )

            success = result.returncode == 0
            if success:
                # Извлекаем размер прошивки
                output_lines = result.stdout.split('\n')
                firmware_size = "Неизвестно"
                for line in output_lines:
                    if "Flash:" in line and "used" in line:
                        firmware_size = line.strip()
                        break

                return {
                    "success": True,
                    "firmware_size": firmware_size,
                    "output": result.stdout
                }
            else:
                return {
                    "success": False,
                    "error": result.stderr,
                    "output": result.stdout
                }
        except subprocess.TimeoutExpired:
            return {"success": False, "error": "Таймаут сборки (60 сек)"}
        except Exception as e:
            return {"success": False, "error": str(e)}

    def _check_tests(self) -> Dict[str, Any]:
        """Проверяет основные тесты"""
        try:
            print("  Python тесты...")
            pytest_result = subprocess.run(
                [sys.executable, "-m", "pytest", "test/test_validation.py", "test/test_compensation_formulas.py", "-q"],
                capture_output=True,
                text=True,
                encoding='utf-8',
                errors='replace',
                cwd=self.project_root,
                timeout=30
            )

            print("  Native тесты...")
            native_result = subprocess.run(
                ["pio", "test", "-e", "native"],
                capture_output=True,
                text=True,
                encoding='utf-8',
                errors='replace',
                cwd=self.project_root,
                timeout=30
            )

            return {
                "pytest": {
                    "success": pytest_result.returncode == 0,
                    "output": pytest_result.stdout
                },
                "native": {
                    "success": native_result.returncode == 0,
                    "output": native_result.stdout
                }
            }
        except subprocess.TimeoutExpired:
            return {"error": "Таймаут тестов (30 сек)"}
        except Exception as e:
            return {"error": str(e)}

    def _check_file_sizes(self) -> Dict[str, Any]:
        """Проверяет размеры файлов"""
        try:
            large_files = []
            target_sizes = {
                "main.cpp": 200,
                "modules": 500
            }

            for cpp_file in self.project_root.rglob("*.cpp"):
                if cpp_file.is_file():
                    with open(cpp_file, 'r', encoding='utf-8') as f:
                        lines = len(f.readlines())
                    
                    relative_path = cpp_file.relative_to(self.project_root)
                    
                    if relative_path.name == "main.cpp" and lines > target_sizes["main.cpp"]:
                        large_files.append({
                            "file": str(relative_path),
                            "lines": lines,
                            "target": target_sizes["main.cpp"],
                            "type": "main"
                        })
                    elif lines > target_sizes["modules"]:
                        large_files.append({
                            "file": str(relative_path),
                            "lines": lines,
                            "target": target_sizes["modules"],
                            "type": "module"
                        })

            return {
                "large_files": large_files,
                "total_files": len(list(self.project_root.rglob("*.cpp")))
            }
        except Exception as e:
            return {"error": str(e)}

    def _check_dependencies(self) -> Dict[str, Any]:
        """Проверяет зависимости"""
        try:
            print("  Анализ зависимостей...")
            result = subprocess.run(
                [sys.executable, "scripts/dependency_analyzer.py"],
                capture_output=True,
                text=True,
                encoding='utf-8',
                errors='replace',
                cwd=self.project_root,
                timeout=30
            )

            return {
                "success": result.returncode == 0,
                "output": result.stdout,
                "error": result.stderr if result.returncode != 0 else None
            }
        except subprocess.TimeoutExpired:
            return {"error": "Таймаут анализа зависимостей (30 сек)"}
        except Exception as e:
            return {"error": str(e)}

    def generate_summary(self) -> str:
        """Генерирует сводку результатов"""
        summary = []
        summary.append("📊 СВОДКА ГОТОВНОСТИ К РЕФАКТОРИНГУ")
        summary.append("=" * 50)

        # Сборка
        build = self.results.get("build", {})
        if build.get("success"):
            summary.append("✅ СБОРКА: Успешна")
            summary.append(f"   Прошивка: {build.get('firmware_size', 'Неизвестно')}")
        else:
            summary.append("❌ СБОРКА: Ошибка")
            summary.append(f"   Ошибка: {build.get('error', 'Неизвестно')}")

        # Тесты
        tests = self.results.get("tests", {})
        if "error" in tests:
            summary.append(f"❌ ТЕСТЫ: {tests['error']}")
        else:
            pytest_success = tests.get("pytest", {}).get("success", False)
            native_success = tests.get("native", {}).get("success", False)
            summary.append(f"{'✅' if pytest_success else '❌'} PYTHON ТЕСТЫ: {'Пройдены' if pytest_success else 'Ошибка'}")
            summary.append(f"{'✅' if native_success else '❌'} NATIVE ТЕСТЫ: {'Пройдены' if native_success else 'Ошибка'}")

        # Размеры файлов
        file_sizes = self.results.get("file_sizes", {})
        if "error" in file_sizes:
            summary.append(f"❌ РАЗМЕРЫ ФАЙЛОВ: {file_sizes['error']}")
        else:
            large_files = file_sizes.get("large_files", [])
            if large_files:
                summary.append(f"⚠️ БОЛЬШИЕ ФАЙЛЫ: {len(large_files)} файлов превышают лимиты")
                for file_info in large_files[:5]:  # Показываем первые 5
                    summary.append(f"   {file_info['file']}: {file_info['lines']} строк (лимит: {file_info['target']})")
            else:
                summary.append("✅ РАЗМЕРЫ ФАЙЛОВ: Все в пределах нормы")

        # Зависимости
        deps = self.results.get("dependencies", {})
        if deps.get("success"):
            summary.append("✅ ЗАВИСИМОСТИ: Анализ успешен")
        else:
            summary.append(f"❌ ЗАВИСИМОСТИ: {deps.get('error', 'Неизвестно')}")

        # Общий вывод
        summary.append("\n🎯 ОБЩИЙ ВЫВОД:")
        
        all_success = (
            build.get("success", False) and
            tests.get("pytest", {}).get("success", False) and
            tests.get("native", {}).get("success", False) and
            deps.get("success", False) and
            len(file_sizes.get("large_files", [])) == 0
        )

        if all_success:
            summary.append("✅ ПРОЕКТ ГОТОВ К РЕФАКТОРИНГУ!")
        else:
            summary.append("❌ ПРОЕКТ НЕ ГОТОВ К РЕФАКТОРИНГУ")
            summary.append("   Нужно исправить проблемы выше")

        return "\n".join(summary)

def main():
    """Основная функция"""
    checker = QuickRefactoringChecker()
    results = checker.run_quick_check()
    
    print("\n" + "=" * 60)
    print(checker.generate_summary())
    
    # Сохраняем результаты
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_file = f"test_reports/quick_refactoring_check_{timestamp}.json"
    
    import json
    os.makedirs("test_reports", exist_ok=True)
    with open(report_file, 'w', encoding='utf-8') as f:
        json.dump(results, f, indent=2, ensure_ascii=False)
    
    print(f"\n📄 Отчет сохранен: {report_file}")
    
    return 0 if (
        results.get("build", {}).get("success", False) and
        results.get("tests", {}).get("pytest", {}).get("success", False) and
        results.get("tests", {}).get("native", {}).get("success", False)
    ) else 1

if __name__ == "__main__":
    sys.exit(main()) 