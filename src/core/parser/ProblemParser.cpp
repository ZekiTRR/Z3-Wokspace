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

} // namespace

ParseResult parseProblem(std::string_view svSource, std::string strName)
{
    ParseResult oResult;

    const LexResult oLexed = lex(svSource);
    oResult.diagnostics = oLexed.diagnostics;
    if (oLexed.hasErrors())
    {
        return oResult;
    }

    ast::ParseOutput oParsed = ast::parse(oLexed.tokens);
    oResult.diagnostics.insert(oResult.diagnostics.end(),
        oParsed.diagnostics.begin(), oParsed.diagnostics.end());
    if (oParsed.hasErrors())
    {
        return oResult;
    }

    Problem oProblem(std::move(strName));
    if (!registerVariables(oProblem, oParsed.statements, oResult.diagnostics))
    {
        return oResult;
    }

    // Pass 2: resolve and type-check every constraint against the full set of
    // declared variables.
    SemanticAnalyzer oAnalyzer(oProblem, oResult.diagnostics);

    for (const ast::Statement& oStatement : oParsed.statements)
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

    const bool bHasErrors = !oResult.diagnostics.empty();
    if (!bHasErrors)
    {
        oResult.problem = std::move(oProblem);
    }
    return oResult;
}

} // namespace z3wb
