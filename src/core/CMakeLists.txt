# -----------------------------------------------------------------------------
# Z3WorkbenchCore
#
# Qt-free library containing domain, application services, parser, solver
# abstraction and serialization. Z3 is linked PRIVATE: no z3 headers may leak
# into the public interface of this target.
# -----------------------------------------------------------------------------
set(Z3WB_CORE_SOURCES
    domain/Expression.cpp
    domain/Ids.cpp
    domain/Problem.cpp
    domain/Project.cpp
    domain/SolverResult.cpp
    domain/Variable.cpp
    parser/Lexer.cpp
    parser/Parser.cpp
    parser/ProblemParser.cpp
    parser/SemanticAnalyzer.cpp
    solver/z3/Z3ExpressionConverter.cpp
    solver/z3/Z3ModelConverter.cpp
    solver/z3/Z3Solver.cpp
    utils/Version.cpp
)

add_library(Z3WorkbenchCore STATIC ${Z3WB_CORE_SOURCES})
add_library(Z3Workbench::Core ALIAS Z3WorkbenchCore)

configure_options(Z3WorkbenchCore)
configure_warnings(Z3WorkbenchCore)

# Public include root is src/, so includes read "core/domain/Problem.hpp".
target_include_directories(Z3WorkbenchCore PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Z3WorkbenchCore PRIVATE
    z3::libz3
)
