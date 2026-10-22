# шаВеRMA Hit Maker

Шуточный (но рабочий) плагин для мастер-канала Ableton от группы **Melody Makers**, Resonance Music Academy.

Одна большая красная кнопка **«СДЕЛАТЬ ХИТ»**: нажал кнопочку — и готово. Sonic Music уже в очереди, Ninja Records атакуют личку.

Как выглядит — не скажем. Сюрприз: ставь и смотри сам.

## Скачать

Последняя версия — на странице [Releases](../../releases/latest):

- **Mac** (Intel и Apple Silicon): `ShaVeRMA-Hit-Maker-mac.zip` — VST3 + AU
- **Windows**: `ShaVeRMA-Hit-Maker-windows.zip` — VST3

Распакуй, запусти `install.command` (Mac) или `install.bat` (Windows), в Ableton нажми **Rescan**. Подробная инструкция — `INSTALL-RU.txt` в архиве.

## Что внутри

| Крутилка | Что делает на самом деле |
|---|---|
| **Мясо** | низкие частоты, low shelf 110 Гц, до +9 dB |
| **Соус** | сатурация (tanh), тепло и плотность |
| **Острота** | верхние частоты, high shelf 7.5 кГц, до +8 dB |
| **Лаваш** | ширина стерео (mid/side) |
| **Эмбиент** (by Марина) | большой ревер с шиммером на октаву вверх |
| **Режим Виталия** | недоступно: пользователь вышел из группы |

- **СДЕЛАТЬ ХИТ** — подкручивает всё сразу и добавляет громкости. На выходе всегда стоит лимитер, так что клиппинга не будет.
- **Режим Стаса** — +6 dB, чтобы точно было слышно.
- **AIR HORN** — гудок при готовности хита (по умолчанию выключен, синтезирован в коде).
- **Идея для Максима** — генератор продакшен-идей, когда не придумывается.
- **Титры** — все участники группы.

## Сборка

Нужны CMake ≥ 3.22 и компилятор C++17 (Xcode / Command Line Tools на Mac, Visual Studio 2022 на Windows). JUCE подтягивается автоматически.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DHITMAKER_COPY_AFTER_BUILD=ON
cmake --build build --config Release
```

Оффлайн-проверка DSP и скриншоты интерфейса:

```sh
cmake -B build -DHITMAKER_BUILD_TESTS=ON
cmake --build build --config Release --target HitMakerSelfTest
./build/HitMakerSelfTest_artefacts/Release/HitMakerSelfTest shots/
```

Релиз собирается GitHub Actions на тег `v*`: Mac (universal) + Windows, с прогоном self-test и [pluginval](https://github.com/Tracktion/pluginval).

## Лицензия

AGPL-3.0 (как у [JUCE](https://juce.com) для open source проектов).

Шрифты [Manrope](https://github.com/googlefonts/manrope) и [Unbounded](https://github.com/googlefonts/unbounded) — SIL Open Font License, тексты лицензий в `assets/fonts/`. Логотип — шаВеRMA / Resonance Music Academy.
