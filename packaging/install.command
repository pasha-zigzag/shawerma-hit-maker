#!/bin/bash
# Установка шаВеRMA Hit Maker (macOS): кладёт VST3 и AU в папки пользователя
# и снимает карантин, чтобы macOS не блокировала неподписанный плагин.

cd "$(dirname "$0")" || exit 1

NAME="ShaVeRMA Hit Maker"
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"

echo ""
echo "  Ставим шаВеRMA Hit Maker…"
echo ""

mkdir -p "$VST3_DIR" "$AU_DIR"
rm -rf "$VST3_DIR/$NAME.vst3" "$AU_DIR/$NAME.component"

cp -R "$NAME.vst3" "$VST3_DIR/" && cp -R "$NAME.component" "$AU_DIR/" || {
    echo "  Не получилось скопировать файлы. Запусти скрипт из распакованной папки."
    read -n 1 -s -r -p "  Нажми любую клавишу…"
    exit 1
}

xattr -cr "$VST3_DIR/$NAME.vst3" "$AU_DIR/$NAME.component"

# Сбрасываем кэш Audio Units, чтобы Ableton сразу увидел плагин.
killall -9 AudioComponentRegistrar 2>/dev/null

echo "  Готово! Шаурма в лаваше."
echo ""
echo "  Дальше в Ableton:"
echo "    1. Settings → Plug-Ins → включи «Use Audio Units v2» и «Use VST3 Plug-In System Folders»"
echo "    2. Нажми «Rescan»"
echo "    3. Ищи «ShaVeRMA Hit Maker» в Plug-Ins → Melody Makers"
echo "    4. Кидай на мастер и жми большую красную кнопку"
echo ""
read -n 1 -s -r -p "  Нажми любую клавишу, чтобы закрыть"
echo ""
