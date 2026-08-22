#include "core/parser/ProblemParser.hpp"

#include "core/parser/Lexer.hpp"
#include "core/parser/SemanticAnalyzer.hpp"

namespace z3wb {

namespace {

// Pass 1: register variable declarations (order-independent relative to
// constraints). Returns false when an error was reported.
bool registerVariables(Problem& oProblem, const std::vector<ast::Statement>& vecStatements,
    std::vector<Diagnostic>& vecDiags)
{
    bool bOk = true;

    for (const ast::Statement& oStatement : vecStatements)
    {
        const auto* pVarDecl = std::get_if<ast::VarDecl>(&oStatement);
        if (pVarDecl == nullptr)
        {
            continue;
        }

        Variable oVariable;
        oVariable.name = pVarDecl->name;
        oVariable.type = pVarDecl->type;
        oVariable.params = pVarDecl->params;

        if (!oProblem.addVariable(std::move(oVariable)))
        {
            Diagnostic oDiagnostic;
            oDiagnostic.severity = DiagnosticSeverity::Error;
            oDiagnostic.location = pVarDecl->location;
            oDiagnostic.message = "Variable \"" + pVarDecl->name + "\" is already defined";
            vecDiags.push_back(std::move(oDiagnostic));
            bOk = false;
        }
    }

    return bOk;
}

// Pass 2: resolve and type-check constraints; appends diagnostics.
void resolveConstraints(Problem& oProblem, const std::vector<ast::Statement>& vecStatements,
    std::vector<Diagnostic>& vecDiags)
{
    SemanticAnalyzer oAnalyzer(oProblem, vecDiags);

    for (const ast::Statement& oStatement : vecStatements)
    {
        const auto* pConstraint = std::get_if<ast::ConstraintDecl>(&oStatement);
        if (pConstraint == nullptr)
        {
            continue;
        }

        std::optional<Expression> oResolved = oAnalyzer.resolve(pConstraint->expr);
        if (!oResolved.has_value())
        {
            continue; // diagnostics already recorded; keep checking the rest
        }

        Constraint oConstraint;
        oConstraint.expr = std::move(*oResolved);
        oConstraint.location = pConstraint->location;

        // addConstraint only assigns an id; it cannot reject a valid entry.
        [[maybe_unused]] const bool bAdded = oProblem.addConstraint(std::move(oConstraint));
    }
}

} // namespace

bool rebuildProblemFromSource(Problem& oTarget, std::string_view svSource,
    std::vector<Diagnostic>& vecDiags)
{
    vecDiags.clear();

    const LexResult oLexed = lex(svSource);
    vecDiags.insert(vecDiags.end(), oLexed.diagnostics.begin(), oLexed.diagnostics.end());

    if (oLexed.hasErrors())
    {
        return false;
    }

    const ast::ParseOutput oParsed = ast::parse(oLexed.tokens);
    vecDiags.insert(vecDiags.end(), oParsed.diagnostics.begin(), oParsed.diagnostics.end());
    if (oParsed.hasErrors())
    {
        return false;
    }

    // Build into a temporary problem so the target keeps its last valid
    // state when the new source contains errors.
    Problem oTemp(oTarget.name());
    registerVariables(oTemp, oParsed.statements, vecDiags);
    resolveConstraints(oTemp, oParsed.statements, vecDiags);

    if (!vecDiags.empty())
    {
        return false;
    }

    oTarget.resetContents(
        oTemp.variables(), oTemp.constraints(), std::string(svSource));
    return true;
}

ParseResult parseProblem(std::string_view svSource, std::string strName)
{
    ParseResult oResult;

    Problem oProblem(std::move(strName));
    if (rebuildProblemFromSource(oProblem, svSource, oResult.diagnostics))
    {
        oResult.problem = std::move(oProblem);
    }
    return oResult;
}

} // namespace z3wb
