# Архитектура

## Обзор

Слоистая архитектура с единственным направлением зависимостей:

```text
QML Views  →  ViewModels/Controllers  →  Application Services  →  Domain
                                                                        ↑
                                                    Infrastructure ─────┘
                                                    (Z3 adapter, serialization)
```

Жёсткие правила:

- `Z3WorkbenchCore` не зависит от Qt.
- Domain не зависит от Z3, Qt и GUI.
- Z3 виден только внутри `src/core/solver/z3/`.
- QML не содержит логики решателя; GUI никогда не касается сырых типов Z3.
- Решение никогда не блокирует GUI-поток.

## Цели CMake

```text
Z3WorkbenchApp (exe)   → Z3WorkbenchGui
Z3WorkbenchGui (lib)   → Qt6 (Core, Gui, Qml, Quick, QuickControls2)
Z3WorkbenchTests (exe) → Z3WorkbenchCore, doctest, z3::libz3 (smoke-тест)
Z3WorkbenchCore (lib)  → z3::libz3 (PRIVATE)
```

QML-модуль (`URI Z3Workbench`) привязан к исполняемому файлу: иначе
линкер выбрасывает ресурсные инициализаторы модулей, объявленных в
статических библиотеках.

## Структура каталогов

```text
src/core/    domain/, application/, solver/, parser/, serialization/, utils/
src/gui/     viewmodels/, controllers/, models/ (добавлены в поздних фазах)
src/app/     тонкий main() + QML-модуль
qml/         Main.qml, views/, panels/, components/, dialogs/
tests/       core/, parser/, solver/, serialization/, integration/
scripts/     bootstrap_z3, configure, build, test, run
cmake/       CompilerWarnings, CompilerOptions, Dependencies, Qt, Z3
ThirdParty/  Z3/{source,build,install} (генерируется, не коммитится)
```

## Доменная модель (Phase 2)

Value-типы с сильнотипированными ID:

```cpp
enum class VariableType { Bool, Int, Real, BitVec, String, Array };
struct Variable    { VariableId id; std::string name; VariableType type; TypeParams params; };
struct Constraint  { ConstraintId id; Expression expr; bool enabled; std::string comment; SourceLocation loc; };
struct Problem     { ProblemId id; std::string name; std::vector<Variable>; std::vector<Constraint>; };
struct Project     { std::string name; std::vector<Problem> problems; };
```

`Expression` — иммутабельное дерево (`std::variant<VariableRef, Constant,
Unary, Binary, Call>`), независимое от `z3::expr`.

## Абстракция решателя (Phase 4)

```cpp
class ISolver {
public:
    virtual SolverResult solve(const Problem&, const SolverConfig&,
                               std::shared_ptr<ICancellation>) = 0;
    virtual std::string toSmtLib2(const Problem&) const = 0;
    virtual ~ISolver() = default;
};
```

`Z3Solver` создаёт изолированный `z3::context` на каждый запрос решения.
Конвертация разделена на `Z3ExpressionConverter` (Domain → Z3 AST) и
`Z3ModelConverter` (модель Z3 → `ModelValue`, с dec/hex/bin для BitVec).
`SolverResult` несёт статус, модель, диагностику, статистику, тайминги.

## Парсер (Phase 3, готово)

```text
Source → Lexer → Parser (recursive descent, Pratt priorities) → statements
       → SemanticAnalyzer (unknown variables, sort/width checks)
       → resolved Domain Expression → Z3 adapter
```

Проектные решения:

- Слой инструкций имеет собственный AST (`ast::VarDecl`,
  `ast::ConstraintDecl`); выражения напрямую переиспользуют доменное дерево
  `Expression` (неразрешённые `VariableRef` несут невалидные id) —
  параллельная иерархия AST-выражений дублировала бы каждый тип узлов,
  не добавляя информации.
- Операторы хранятся в разрешённой форме; сравнения/деление/сдвиги BitVec —
  беззнаковые.
- Приоритеты RE-ориентированные: побитовые связывают сильнее сравнений,
  поэтому `x ^ 0x1337 == 0x4242` разбирается как `(x ^ 0x1337) == 0x4242`.
- Обычные целые литералы приводятся к ширине окружающего BitVec
  (`x << 2` при `x: BitVec(8)`), с проверкой диапазона.
- Int и Real никогда не смешиваются неявно; корни ограничений обязаны быть
  Bool.
- Ошибки несут строку/столбец; восстановление на уровне инструкций сообщает
  несколько ошибок за проход. Канонический вывод: константы Int в
  десятичной системе, значения BitVec как `0x…:width`.

DSL: инструкции `var <name>: <Type>` и `constraint <expr>`; комментарии
`//` и `/* */`; литералы dec/hex/bin/real/string.

## Модель потоков (Phase 6, готово)

```text
GUI thread: solve() → snapshot Problem into SolveJob → queued emit → worker
Worker thread (solver-worker): Z3Solver::solve(job.problem, config, cancel)
Stop: AtomicCancellation.cancel() — cooperative; Z3 :timeout bounds the wait
Result: SolveJobResult via queued signal → GUI thread → panels updated
```

- `SolveJob`/`SolveJobResult` — value-структуры слоя GUI, обращённые к Qt;
  доменные типы остаются свободными от метатайпов.
- Долгоживущий `SolverWorker` живёт в выделенном QThread; каждый запрос
  получает свежее состояние бэкенда, поэтому пересоздание воркеров не нужно.
- Результаты для задачи, которая больше не выбрана, отбрасываются с записью
  в лог. Stop во время работы ставит статус CANCELLING, пока бэкенд не
  вернётся.

## Персистентность (Phase 7, готово)

Файлы проектов `.z3w` — это JSON с полем `version`:

```json
{
    "version": 1,
    "name": "Example",
    "problems": [
        { "name": "crackme_01", "source": "var x: BitVec(32)\nconstraint ..." }
    ]
}
```

- `JsonProjectStorage` живёт в core (`serialization/`, без Qt) и возвращает
  типизированные исходы (`StorageOutcome` +
  `StorageError{Io, Format, Version}`); через границу не проходят
  исключения.
- **DSL-исходник — единственный источник истины**: загрузка пересобирает
  каждую задачу настоящим парсером, поэтому формат физически не может
  разойтись с языком. Хранимый исходник, который больше не парсится,
  даёт ошибку Format с именем задачи.
- Точка расширения цепочки миграций: `migrateDocument()` обновляет
  документы пошагово; файлы более новых версий схемы отклоняются с
  понятным сообщением.
- Проводка в GUI: Ctrl+O / Ctrl+S / Ctrl+Shift+S, нативные файловые
  диалоги, отслеживание изменений (`*` в заголовке/тулбаре), блокировки
  во время решения. `Project::adoptProblem` перемещает загруженные задачи
  в проект.

## Экспорт / импорт (Phase 8, готово)

```text
Domain Problem → SmtLib2Serializer → .smt2   (Z3-free, portable output)
.smt2 → SmtLib2Reader → expressions → DslPrinter → DSL source → normal pipeline
Domain Problem → ProblemExporter {SmtLib2 | Json | Txt} → files
```

- Сериализатор выводит типы из объявленных переменных, поэтому операции
  BitVec отображаются в беззнаковые функции (bvult, bvudiv, bvneg, …), а
  отрицание выбирает правильную форму по типу.
- Reader принимает подмножество экспортёра (declare-const/assert над
  набором операторов workbench); неизвестные команды пропускаются,
  неизвестные операторы сообщаются с номерами строк.
- Импортированные задачи становятся полноценными: DslPrinter генерирует
  редактируемый DSL-исходник, дальше всё (валидация, персистентность,
  решение) работает как обычно.
- GUI: File ▸ Export Problem ▸ SMT-LIB2/JSON/TXT, File ▸ Import SMT-LIB2,
  тулбар «Export SMT2»; просмотрщик Ctrl+M продолжает показывать вид
  бэкенда (`ISolver::toSmtLib2` — то, что реально получает Z3).

## GUI (Phase 5, готово)

Тёмная раскладка в стиле IDE: строка меню, тулбар, обозреватель проекта
(слева), редактор задач с живой диагностикой (в центре), панель
переменных/модели (справа), консоль (внизу), статус-бар. `Theme.qml` —
QML-синглтон с палитрой.

- QML отвечает только за раскладку/биндинги; всё состояние живёт в
  `gui::WorkspaceViewModel`, доступном как context-свойство `workspace`.
  QML не касается типов core beyond того, что выставляет viewmodel.
- Списки опираются на специализированные `QAbstractListModel`
  (`ProblemsModel`, `VariablesModel`, `DiagnosticsModel`,
  `ConsoleLogModel`); viewmodel пушит снапшоты, модели остаются тупыми.
- Редактирование переразбирается при каждом изменении: ошибки живо
  появляются в диагностике, пока задача хранит последнее валидное
  содержимое (`rebuildProblemFromSource`). Solve всегда выполняется на
  самом свежем тексте.
- Решатель внедряется через `ISolver` (`makeDefaultSolver()`), поэтому GUI
  и app никогда не видят заголовков Z3.
- Горячие клавиши: Ctrl+N — новая задача, F5 — решение, Shift+F5 — стоп
  (Phase 6), Ctrl+M — просмотр SMT-LIB2, Ctrl+Q — выход.

## Тестирование

doctest + CTest: парсер (лексер/парсер/позиции), core (проверка типов),
решатель (SAT/UNSAT/UNKNOWN, конвертация модели), сериализация
(round-trip), интеграция (source → solve → model). Уже с Phase 1 полная
цепочка Z3 покрыта smoke-тестами.

## CI (Phase 9)

GitHub Actions, только GCC/Clang (`.github/workflows/ci.yml`):

```text
linux-gcc / linux-clang: apt Qt6 + libz3-dev → configure → build → ctest
windows-mingw:           MSYS2 MINGW64 pacman (gcc, ninja, qt6, z3) → те же шаги
[Future] MSVC job — задокументированная точка расширения; обязан
         переиспользовать те же шаги
```

CI использует системный/пакетный Z3; project-local bootstrap остаётся путём
локальной разработки. Оба потока сходятся на одних и тех же CMake-целях.

## Полировка, доставленная в Phase 9

- Подсветка синтаксиса DSL (`DslHighlighter`, состояния блочных
  комментариев, палитра зеркалит Theme.qml).
- Недавние проекты (QSettings, File ▸ Open Recent) и персистентная
  геометрия окна.
- Оставшаяся будущая работа: поиск в редакторе, диалог настроек, unsat
  cores, дополнительные бэкенды решателей.

## Дорожная карта

```text
Phase 1  build skeleton, Z3 bootstrap            ✓ done
Phase 2  domain model                            ✓ done
Phase 3  parser + diagnostics                    ✓ done
Phase 4  Z3 solver adapter                       ✓ done
Phase 5  GUI panels, dark theme                  ✓ done
Phase 6  async solving, cancellation, timeout    ✓ done
Phase 7  JSON persistence (.z3w)                 ✓ done
Phase 8  SMT-LIB2 / JSON / TXT export, import    ✓ done
Phase 9  polish, syntax highlighting, CI         ✓ done (GCC/Clang matrix)
```
