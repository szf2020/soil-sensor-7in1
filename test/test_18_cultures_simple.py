#!/usr/bin/env python3
"""
ПРОСТОЙ ТЕСТ: 18 КУЛЬТУР В СИСТЕМЕ JXCT
Проверяет наличие всех культур в системе
"""

import os

def test_all_18_crops():
    """Простой тест всех 18 культур"""
    print("ТЕСТ СИСТЕМЫ: 18 КУЛЬТУР")
    print("=" * 50)
    
    # Все 18 культур
    all_crops = [
        # Исходные 8
        "tomato", "cucumber", "pepper", "lettuce", 
        "blueberry", "strawberry", "apple", "grape",
        # Фаза 1: 3 культуры
        "spinach", "basil", "cannabis",
        # Фаза 2: 2 культуры  
        "wheat", "potato",
        # Фаза 3: 5 культур
        "kale", "raspberry", "blackberry", "soybean", "carrot"
    ]
    
    # Читаем файл движка
    engine_path = "src/business/crop_recommendation_engine.cpp"
    if not os.path.exists(engine_path):
        print("ОШИБКА: Файл crop_recommendation_engine.cpp не найден")
        return False
    
    with open(engine_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Читаем веб-интерфейс
    web_path = "src/web/routes_main.cpp"
    if not os.path.exists(web_path):
        print("ОШИБКА: Файл routes_main.cpp не найден")
        return False
        
    with open(web_path, 'r', encoding='utf-8') as f:
        web_content = f.read()
    
    print("\nПРОВЕРКА ВСЕХ КУЛЬТУР:")
    
    implemented_in_engine = 0
    implemented_in_web = 0
    
    for i, crop in enumerate(all_crops, 1):
        # Проверяем в движке
        engine_pattern = f'cropName == "{crop}"'
        engine_ok = engine_pattern in content
        
        # Проверяем в веб-интерфейсе
        web_pattern = f"value='{crop}'"
        web_ok = web_pattern in web_content
        
        if engine_ok:
            implemented_in_engine += 1
        if web_ok:
            implemented_in_web += 1
            
        status_engine = "OK" if engine_ok else "FAIL"
        status_web = "OK" if web_ok else "FAIL"
        
        print(f"  {i:2d}. {crop:12s} - Engine: {status_engine:4s} | Web: {status_web:4s}")
    
    print(f"\nРЕЗУЛЬТАТЫ:")
    print(f"  Всего культур: {len(all_crops)}")
    print(f"  В движке: {implemented_in_engine}/{len(all_crops)} ({implemented_in_engine/len(all_crops)*100:.1f}%)")
    print(f"  В веб-интерфейсе: {implemented_in_web}/{len(all_crops)} ({implemented_in_web/len(all_crops)*100:.1f}%)")
    
    success = implemented_in_engine == len(all_crops) and implemented_in_web == len(all_crops)
    
    if success:
        print("  СТАТУС: ВСЕ 18 КУЛЬТУР РЕАЛИЗОВАНЫ!")
    else:
        print("  СТАТУС: ТРЕБУЕТСЯ ДОРАБОТКА")
    
    # Статистика по фазам
    print(f"\nСТАТИСТИКА ПО ФАЗАМ:")
    print(f"  Исходные: 8 культур")
    print(f"  Фаза 1: 3 культуры (+37.5%)")
    print(f"  Фаза 2: 2 культуры (+25%)")
    print(f"  Фаза 3: 5 культур (+62.5%)")
    print(f"  ИТОГО: 18 культур (+125%)")
    
    return success

if __name__ == "__main__":
    success = test_all_18_crops()
    if success:
        print("\nСИСТЕМА ГОТОВА!")
    else:
        print("\nТРЕБУЕТСЯ ДОРАБОТКА!")
    exit(0 if success else 1) 