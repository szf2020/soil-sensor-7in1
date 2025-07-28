#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
🎯 АНАЛИЗ РАСПРОСТРАНЕННОСТИ ТИПОВ ПОЧВ И СОЗДАНИЕ ВЕБ-ИНТЕРФЕЙСА
"""

def analyze_soil_prevalence():
    """Анализ распространенности типов почв"""
    
    soil_types = {
        # ТЕКУЩИЕ ТИПЫ
        "SAND": {
            "russian_name": "Песок",
            "prevalence": "VERY_HIGH",  # Очень распространен
            "description": "Песчаная почва с крупными частицами",
            "usage_percentage": 25
        },
        "LOAM": {
            "russian_name": "Суглинок", 
            "prevalence": "VERY_HIGH",  # Очень распространен
            "description": "Сбалансированная почва, идеальная для большинства культур",
            "usage_percentage": 35
        },
        "CLAY": {
            "russian_name": "Глина",
            "prevalence": "HIGH",  # Распространен
            "description": "Тяжелая почва с мелкими частицами",
            "usage_percentage": 20
        },
        "PEAT": {
            "russian_name": "Торф",
            "prevalence": "MEDIUM",  # Средне распространен
            "description": "Органическая почва с высоким содержанием углерода",
            "usage_percentage": 10
        },
        "SANDPEAT": {
            "russian_name": "Песчано-торфяная смесь",
            "prevalence": "MEDIUM",  # Средне распространен
            "description": "Смесь песка и торфа для улучшения структуры",
            "usage_percentage": 10
        },
        
        # НОВЫЕ ТИПЫ
        "SILT": {
            "russian_name": "Силт (Иловая почва)",
            "prevalence": "HIGH",  # Распространен
            "description": "Почва с преобладанием илистых частиц",
            "usage_percentage": 15
        },
        "CLAY_LOAM": {
            "russian_name": "Глинистый суглинок",
            "prevalence": "HIGH",  # Распространен
            "description": "Смесь глины и суглинка с преобладанием глинистых частиц",
            "usage_percentage": 18
        },
        "ORGANIC": {
            "russian_name": "Органическая почва",
            "prevalence": "MEDIUM",  # Средне распространен
            "description": "Почва с высоким содержанием органического вещества",
            "usage_percentage": 12
        },
        "SANDY_LOAM": {
            "russian_name": "Песчанистый суглинок",
            "prevalence": "MEDIUM",  # Средне распространен
            "description": "Смесь песка и суглинка с преобладанием песчаных частиц",
            "usage_percentage": 8
        },
        "SILTY_LOAM": {
            "russian_name": "Иловатый суглинок",
            "prevalence": "MEDIUM",  # Средне распространен
            "description": "Смесь ила и суглинка с преобладанием илистых частиц",
            "usage_percentage": 7
        },
        "LOAMY_CLAY": {
            "russian_name": "Суглинистая глина",
            "prevalence": "LOW",  # Менее распространен
            "description": "Смесь глины и суглинка с преобладанием глинистых частиц",
            "usage_percentage": 5
        },
        "SALINE": {
            "russian_name": "Засоленная почва",
            "prevalence": "LOW",  # Менее распространен
            "description": "Почва с высоким содержанием растворимых солей",
            "usage_percentage": 3
        },
        "ALKALINE": {
            "russian_name": "Щелочная почва",
            "prevalence": "LOW",  # Менее распространен
            "description": "Почва с высоким pH и содержанием карбонатов",
            "usage_percentage": 2
        }
    }
    
    return soil_types

def generate_web_interface_code():
    """Генерация кода веб-интерфейса с выделением распространенных типов"""
    
    soil_types = analyze_soil_prevalence()
    
    print("🎯 АНАЛИЗ РАСПРОСТРАНЕННОСТИ ТИПОВ ПОЧВ")
    print("=" * 60)
    
    # Группировка по распространенности
    very_high = [k for k, v in soil_types.items() if v["prevalence"] == "VERY_HIGH"]
    high = [k for k, v in soil_types.items() if v["prevalence"] == "HIGH"]
    medium = [k for k, v in soil_types.items() if v["prevalence"] == "MEDIUM"]
    low = [k for k, v in soil_types.items() if v["prevalence"] == "LOW"]
    
    print(f"\n🔥 ОЧЕНЬ РАСПРОСТРАНЕННЫЕ ({len(very_high)} типов):")
    for soil_type in very_high:
        info = soil_types[soil_type]
        print(f"  • {soil_type} ({info['russian_name']}) - {info['usage_percentage']}% использования")
    
    print(f"\n📈 РАСПРОСТРАНЕННЫЕ ({len(high)} типов):")
    for soil_type in high:
        info = soil_types[soil_type]
        print(f"  • {soil_type} ({info['russian_name']}) - {info['usage_percentage']}% использования")
    
    print(f"\n🔶 СРЕДНЕ РАСПРОСТРАНЕННЫЕ ({len(medium)} типов):")
    for soil_type in medium:
        info = soil_types[soil_type]
        print(f"  • {soil_type} ({info['russian_name']}) - {info['usage_percentage']}% использования")
    
    print(f"\n🔴 МАЛО РАСПРОСТРАНЕННЫЕ ({len(low)} типов):")
    for soil_type in low:
        info = soil_types[soil_type]
        print(f"  • {soil_type} ({info['russian_name']}) - {info['usage_percentage']}% использования")
    
    # Генерация HTML кода
    print("\n💻 КОД ВЕБ-ИНТЕРФЕЙСА С ВЫДЕЛЕНИЕМ:")
    print("-" * 50)
    
    html_code = """
// HTML код для выпадающего списка с выделением
String generateSoilTypeOptions() {
    String html = "<select name='soil_type' id='soil_type'>";
    
    // Очень распространенные - жирным шрифтом
    html += "<optgroup label='🔥 Очень распространенные'>";
    html += "<option value='0' style='font-weight: bold;'>Песок</option>";
    html += "<option value='1' style='font-weight: bold;'>Суглинок</option>";
    html += "</optgroup>";
    
    // Распространенные - полужирным
    html += "<optgroup label='📈 Распространенные'>";
    html += "<option value='3' style='font-weight: 600;'>Глина</option>";
    html += "<option value='5' style='font-weight: 600;'>Силт (Иловая почва)</option>";
    html += "<option value='6' style='font-weight: 600;'>Глинистый суглинок</option>";
    html += "</optgroup>";
    
    // Средне распространенные - обычным шрифтом
    html += "<optgroup label='🔶 Средне распространенные'>";
    html += "<option value='2'>Торф</option>";
    html += "<option value='4'>Песчано-торфяная смесь</option>";
    html += "<option value='7'>Органическая почва</option>";
    html += "<option value='8'>Песчанистый суглинок</option>";
    html += "<option value='9'>Иловатый суглинок</option>";
    html += "</optgroup>";
    
    // Мало распространенные - мелким шрифтом
    html += "<optgroup label='🔴 Специализированные'>";
    html += "<option value='10' style='font-size: 0.9em;'>Суглинистая глина</option>";
    html += "<option value='11' style='font-size: 0.9em;'>Засоленная почва</option>";
    html += "<option value='12' style='font-size: 0.9em;'>Щелочная почва</option>";
    html += "</optgroup>";
    
    html += "</select>";
    return html;
}
"""
    
    print(html_code)
    
    # Генерация CSS стилей
    print("\n🎨 CSS СТИЛИ ДЛЯ ВЫДЕЛЕНИЯ:")
    print("-" * 50)
    
    css_code = """
/* CSS стили для выпадающего списка */
.soil-type-select {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    padding: 8px;
    border: 2px solid #ddd;
    border-radius: 4px;
    background-color: #f9f9f9;
}

.soil-type-select optgroup {
    font-weight: bold;
    color: #333;
    background-color: #e8f4f8;
}

.soil-type-select option {
    padding: 4px 8px;
}

/* Очень распространенные */
.soil-type-select option[value="0"],
.soil-type-select option[value="1"] {
    font-weight: bold;
    color: #2c5aa0;
    background-color: #e8f4f8;
}

/* Распространенные */
.soil-type-select option[value="3"],
.soil-type-select option[value="5"],
.soil-type-select option[value="6"] {
    font-weight: 600;
    color: #4a7c59;
    background-color: #f0f8f0;
}

/* Средне распространенные */
.soil-type-select option[value="2"],
.soil-type-select option[value="4"],
.soil-type-select option[value="7"],
.soil-type-select option[value="8"],
.soil-type-select option[value="9"] {
    color: #666;
    background-color: #fafafa;
}

/* Мало распространенные */
.soil-type-select option[value="10"],
.soil-type-select option[value="11"],
.soil-type-select option[value="12"] {
    font-size: 0.9em;
    color: #999;
    background-color: #f5f5f5;
    font-style: italic;
}
"""
    
    print(css_code)
    
    # Улучшенные русские названия
    print("\n🇷🇺 УЛУЧШЕННЫЕ РУССКИЕ НАЗВАНИЯ:")
    print("-" * 50)
    
    improved_names = {
        "SAND": "Песок",
        "LOAM": "Суглинок", 
        "CLAY": "Глина",
        "PEAT": "Торф",
        "SANDPEAT": "Песчано-торфяная смесь",
        "SILT": "Силт (Иловая почва)",
        "CLAY_LOAM": "Глинистый суглинок",
        "ORGANIC": "Органическая почва",
        "SANDY_LOAM": "Песчанистый суглинок",
        "SILTY_LOAM": "Иловатый суглинок", 
        "LOAMY_CLAY": "Суглинистая глина",
        "SALINE": "Засоленная почва",
        "ALKALINE": "Щелочная почва"
    }
    
    for eng, rus in improved_names.items():
        print(f"  {eng} → {rus}")
    
    return soil_types

def main():
    """Основная функция"""
    soil_types = generate_web_interface_code()
    
    print("\n📊 ИТОГОВАЯ СТАТИСТИКА:")
    print("-" * 50)
    
    total_usage = sum(info["usage_percentage"] for info in soil_types.values())
    print(f"Общий процент покрытия: {total_usage}%")
    print(f"Количество типов почв: {len(soil_types)}")
    
    print("\n✅ РЕКОМЕНДАЦИИ:")
    print("-" * 50)
    print("1. Выделить очень распространенные типы жирным шрифтом")
    print("2. Использовать группировку по распространенности")
    print("3. Применить улучшенные русские названия")
    print("4. Добавить описания для каждого типа почвы")
    print("5. Использовать цветовое кодирование для быстрой идентификации")

if __name__ == "__main__":
    main() 