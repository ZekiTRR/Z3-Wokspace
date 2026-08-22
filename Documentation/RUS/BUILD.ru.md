# Инструкция по сборке

## Предварительные требования

| Инструмент | Версия | Примечания |
|-----------|----------|----------------------------------------------------|
| CMake     | >= 3.24  | системный CMake или `E:/Qt/Tools/CMake_64`         |
| Ninja     | любая    | `E:/Qt/Tools/Ninja` на Windows                     |
| GCC (MinGW-w64) | 13.1.0 | `E:/Qt/Tools/mingw1310_64` на Windows          |
| Qt        | 6.11.0   | `E:/Qt/6.11.0/mingw_64` на Windows (см. ниже)      |
| Z3        | 5.1.0    | автоматически разворачивается в `ThirdParty/Z3`    |
| git       | любая    | нужен для bootstrap Z3                             |

Путь к Qt — это CMake-дефолт в корневом `CMakeLists.txt`; его можно
переопределить без правки файлов:

```bash
cmake -S . -B build/custom -DCMAKE_PREFIX_PATH="<другой Qt>/lib/cmake"
```

## Windows (MinGW)

```powershell
scripts/bootstrap_z3.ps1                 # однократная сборка Z3 (~5-10 мин)
scripts/configure.ps1                    # Debug; добавьте -BuildType Release
scripts/configure.ps1 -BuildType Release
scripts/build.ps1                        # добавьте -BuildType Release
scripts/test.ps1
scripts/run.ps1
```

Эквивалент через сырой CMake:

```powershell
$env:PATH = "E:\Qt\Tools\mingw1310_64\bin;E:\Qt\Tools\Ninja;$env:PATH"
cmake -S . -B build/mingw-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/mingw-debug
ctest --test-dir build/mingw-debug --output-on-failure
```

Запуск приложения из обычной оболочки требует DLL Qt в PATH:

```powershell
$env:PATH = "E:\Qt\6.11.0\mingw_64\bin;E:\Qt\Tools\mingw1310_64\bin;$env:PATH"
build\mingw-debug\Z3Workbench.exe
```

## Linux

```bash
scripts/bootstrap_z3.sh
cmake -S . -B build/gcc-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/gcc-debug
ctest --test-dir build/gcc-debug --output-on-failure
```

Qt 6 (Core, Gui, Qml, Quick, QuickControls2) должен быть установлен через
системный пакетный менеджер; при нестандартном префиксе Qt скорректируйте
`CMAKE_PREFIX_PATH`.

## Каталоги сборки

CMake фиксирует тулчейн в момент конфигурации, поэтому каждая пара
тулчейн/конфигурация получает собственный каталог сборки:

```text
build/mingw-debug    build/mingw-release
build/clang-debug    build/clang-release
```

`compile_commands.json` экспортируется и копируется в корень репозитория
скриптом `scripts/configure.ps1` — для clangd/IDE-инструментария.

## Детали bootstrap Z3

`scripts/bootstrap_z3.ps1` / `.sh`:

1. пропускает всё, если существует `ThirdParty/Z3/install/lib/cmake/z3/Z3Config.cmake`;
2. клонирует зафиксированный тег в `ThirdParty/Z3/source` (shallow);
3. проверяет зафиксированный SHA коммита (отказывается собирать
   непроверенные исходники);
4. configure + build + install статической `libz3` в `ThirdParty/Z3/install`.

На Windows Z3 всегда собирается из исходников тем же MinGW-тулчейном, что и
проект: официальные бинарники Z3 для Windows собраны MSVC и не могут
линковаться с GCC/Clang.

## Устранение неполадок

- **«Z3 was not found» при конфигурации** — запустите bootstrap-скрипт либо
  укажите существующую установку:
  `cmake -DZ3WORKBENCH_Z3_ROOT=<install prefix>`.
- **Приложение не стартует из-за отсутствия DLL** — каталоги bin Qt/MinGW не
  прописаны в PATH (см. раздел Windows выше).
- **Выбран неверный компилятор** — убедитесь, что
  `E:/Qt/Tools/mingw1310_64/bin` в PATH идёт раньше других тулчейнов
  (MSYS2 и т. п.) для этого проекта.
- **Предупреждения как ошибки** — по умолчанию выключено; включается
  `-DZ3WORKBENCH_WARNINGS_AS_ERRORS=ON`.
