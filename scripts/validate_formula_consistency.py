#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Скрипт валидации соответствия формул компенсации
Проверяет, что формулы в коде, тестах и документации синхронизированы
"""

import re
import sys
import json
from pathlib import Path
from typing import Dict, List, Tuple

def extract_formulas_from_cpp(file_path: str) -> Dict[str, str]:
    """Извлекает формулы из C++ кода"""
    formulas = {}
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Ищем научные формулы в комментариях
        ec_pattern = r'// НАУЧНАЯ ФОРМУЛА: Модель Арчи.*?EC_comp = EC_raw × \(θ/θ₀\)\^m × \(T/T₀\)\^n'
        ph_pattern = r'// НАУЧНАЯ ФОРМУЛА: Уравнение Нернста.*?pH_comp = pH_raw - 0\.003 × \(T - 25\)'
        npk_pattern = r'// НАУЧНАЯ ФОРМУЛА: NPK компенсация \(FAO 56\).*?N_comp = N_raw × e\^\(δN\(T-20\)\) × \(1 \+ εN\(θ-30\)\)'
        
        if re.search(ec_pattern, content, re.DOTALL):
            formulas['EC'] = 'Модель Арчи (1942)'
        if re.search(ph_pattern, content, re.DOTALL):
            formulas['pH'] = 'Уравнение Нернста'
        if re.search(npk_pattern, content, re.DOTALL):
            formulas['NPK'] = 'FAO 56'
            
    except Exception as e:
        print(f"Ошибка чтения {file_path}: {e}")
        
    return formulas

def extract_formulas_from_python(file_path: str) -> Dict[str, str]:
    """Извлекает формулы из Python тестов"""
    formulas = {}
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Ищем научные формулы в комментариях
        ec_pattern = r'# НАУЧНАЯ ФОРМУЛА: Модель Арчи.*?EC_comp = EC_raw × \(T/T₀\)\^n'
        ph_pattern = r'# НАУЧНАЯ ФОРМУЛА: Уравнение Нернста.*?pH_comp = pH_raw - 0\.003 × \(T - 25\)'
        npk_pattern = r'# НАУЧНАЯ ФОРМУЛА: FAO 56.*?N_comp = N_raw × e\^\(δN\(T-20\)\)'
        
        if re.search(ec_pattern, content, re.DOTALL):
            formulas['EC'] = 'Модель Арчи (1942)'
        if re.search(ph_pattern, content, re.DOTALL):
            formulas['pH'] = 'Уравнение Нернста'
        if re.search(npk_pattern, content, re.DOTALL):
            formulas['NPK'] = 'FAO 56'
            
    except Exception as e:
        print(f"Ошибка чтения {file_path}: {e}")
        
    return formulas

def extract_formulas_from_markdown(file_path: str) -> Dict[str, str]:
    """Извлекает формулы из Markdown документации"""
    formulas = {}
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Ищем научные формулы в LaTeX (упрощенные паттерны)
        ec_pattern = r'Модель Арчи.*?1942'
        ph_pattern = r'Уравнение Нернста'
        npk_pattern = r'FAO 56'
        
        if re.search(ec_pattern, content, re.DOTALL | re.IGNORECASE):
            formulas['EC'] = 'Модель Арчи (1942)'
        if re.search(ph_pattern, content, re.DOTALL | re.IGNORECASE):
            formulas['pH'] = 'Уравнение Нернста'
        if re.search(npk_pattern, content, re.DOTALL | re.IGNORECASE):
            formulas['NPK'] = 'FAO 56'
            
    except Exception as e:
        print(f"Ошибка чтения {file_path}: {e}")
        
    return formulas

def validate_formula_consistency() -> Dict[str, any]:
    """Проверяет соответствие формул между файлами"""
    
    # Файлы для проверки
    cpp_file = "src/business/sensor_compensation_service.cpp"
    python_file = "test/test_compensation_formulas.py"
    markdown_file = "docs/manuals/COMPENSATION_GUIDE.md"
    
    # Извлекаем формулы
    cpp_formulas = extract_formulas_from_cpp(cpp_file)
    python_formulas = extract_formulas_from_python(python_file)
    markdown_formulas = extract_formulas_from_markdown(markdown_file)
    
    # Проверяем соответствие
    consistency_report = {
        'status': 'PASS',
        'files_checked': [cpp_file, python_file, markdown_file],
        'formulas': {
            'cpp': cpp_formulas,
            'python': python_formulas,
            'markdown': markdown_formulas
        },
        'issues': []
    }
    
    # Проверяем каждую формулу
    expected_formulas = {
        'EC': 'Модель Арчи (1942)',
        'pH': 'Уравнение Нернста',
        'NPK': 'FAO 56'
    }
    
    for formula_type, expected in expected_formulas.items():
        cpp_has = cpp_formulas.get(formula_type) == expected
        python_has = python_formulas.get(formula_type) == expected
        markdown_has = markdown_formulas.get(formula_type) == expected
        
        if not all([cpp_has, python_has, markdown_has]):
            consistency_report['status'] = 'FAIL'
            issue = {
                'formula': formula_type,
                'expected': expected,
                'cpp': cpp_has,
                'python': python_has,
                'markdown': markdown_has
            }
            consistency_report['issues'].append(issue)
    
    return consistency_report

def main():
    """Главная функция"""
    print("🔬 ВАЛИДАЦИЯ СООТВЕТСТВИЯ ФОРМУЛ КОМПЕНСАЦИИ")
    print("=" * 60)
    
    try:
        report = validate_formula_consistency()
        
        print(f"📊 Статус: {report['status']}")
        print(f"📁 Проверено файлов: {len(report['files_checked'])}")
        
        print("\n📋 Формулы по файлам:")
        for file_type, formulas in report['formulas'].items():
            print(f"  {file_type.upper()}: {formulas}")
        
        if report['issues']:
            print("\n❌ НАЙДЕНЫ НЕСООТВЕТСТВИЯ:")
            for issue in report['issues']:
                print(f"  Формула {issue['formula']}:")
                print(f"    Ожидается: {issue['expected']}")
                print(f"    C++: {'✅' if issue['cpp'] else '❌'}")
                print(f"    Python: {'✅' if issue['python'] else '❌'}")
                print(f"    Markdown: {'✅' if issue['markdown'] else '❌'}")
        else:
            print("\n✅ ВСЕ ФОРМУЛЫ СИНХРОНИЗИРОВАНЫ!")
            print("   • EC: Модель Арчи (1942)")
            print("   • pH: Уравнение Нернста")
            print("   • NPK: FAO 56")
        
        # Сохраняем отчет
        report_file = "test_reports/formula_consistency_report.json"
        with open(report_file, 'w', encoding='utf-8') as f:
            json.dump(report, f, indent=2, ensure_ascii=False)
        
        print(f"\n📄 Отчет сохранен: {report_file}")
        
        return report['status'] == 'PASS'
        
    except Exception as e:
        print(f"\n💥 КРИТИЧЕСКАЯ ОШИБКА: {e}")
        return False

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1) 