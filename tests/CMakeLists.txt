add_executable(Z3WorkbenchTests
    main.cpp
    core/AtomicCancellationTest.cpp
    core/ExpressionTest.cpp
    core/IdsTest.cpp
    core/ProblemTest.cpp
    core/ProjectTest.cpp
    core/SolverResultTest.cpp
    core/VersionTest.cpp
    integration/EndToEndTest.cpp
    parser/LexerTest.cpp
    parser/ParserTest.cpp
    parser/SemanticTest.cpp
    solver/Z3SmokeTest.cpp
)

configure_options(Z3WorkbenchTests)
configure_warnings(Z3WorkbenchTests)

target_link_libraries(Z3WorkbenchTests PRIVATE
    Z3Workbench::Core
    doctest::doctest
    # The Z3 smoke test intentionally exercises the Z3 C++ API directly to
    # validate the bootstrap -> find_package -> link chain end-to-end.
    z3::libz3
)

add_test(NAME Z3WorkbenchTests COMMAND Z3WorkbenchTests)
