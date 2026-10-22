# шаВеRMA Hit Maker

Шуточный (но рабочий) плагин для мастер-канала Ableton от группы **Melody Makers**, Resonance Music Academy.

Что внутри — не скажем. Сюрприз: ставь и смотри сам.

## Скачать

Последняя версия — на странице [Releases](../../releases/latest):

- **Mac** (Intel и Apple Silicon): `ShaVeRMA-Hit-Maker-mac.zip` — VST3 + AU
- **Windows**: `ShaVeRMA-Hit-Maker-windows.zip` — VST3

Распакуй, запусти `install.command` (Mac) или `install.bat` (Windows), в Ableton нажми **Rescan**. Подробная инструкция — `INSTALL-RU.txt` в архиве.

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
