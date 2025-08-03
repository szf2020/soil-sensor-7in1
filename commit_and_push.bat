@echo off
echo Создание коммита точки отката...
git add .
git commit -m "feat: комплексная научная валидация всех алгоритмов JXCT - точка отката"
echo Пуш в репозиторий...
git push origin refactoring
echo Готово!
pause 