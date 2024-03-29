#include <libfol-parser/parser/parser.hpp>

namespace fol::parser {
Term ParseTerm(lexer::LexemeGenerator::iterator &iterator) {
  auto var = *iterator;
  ++iterator;
  return std::visit(
      details::utils::Overloaded{
          [&](lexer::Constant constant) -> Term {
            return {std::move(constant)};
          },
          [&](lexer::Variable var) -> Term { return {std::move(var)}; },
          [&](lexer::Function func) -> Term {
            if (!std::holds_alternative<lexer::OpenBracket>(*iterator)) {
              throw ParseError{"No args for function."};
            }
            ++iterator;

            auto fun = Term{FunctionFormula{
                std::make_unique<std::pair<lexer::Function, TermList>>(
                    std::move(func), ParseTermList(iterator))}};

            if (!std::holds_alternative<lexer::CloseBracket>(*iterator)) {
              throw ParseError{"No close bracket at args for function."};
            }
            ++iterator;

            return fun;
          },
          [](...) -> Term {
            throw ParseError{"Unhandled variant in Term parsing."};
          }},
      var);
}

TermListPrime ParseTermListPrime(lexer::LexemeGenerator::iterator &iterator) {
  auto var = *iterator;
  return std::visit(
      details::utils::Overloaded{
          [&](lexer::Comma) -> TermListPrime {
            ++iterator;
            return {std::make_unique<TermList>(ParseTermList(iterator))};
          },
          [&](...) -> TermListPrime { return {lexer::EPS{}}; }},
      var);
}

TermList ParseTermList(lexer::LexemeGenerator::iterator &iterator) {
  return TermList{ParseTerm(iterator), ParseTermListPrime(iterator)};
}

ConjunctionPrimeFormula ParseConjuctionPrimeFormula(
    lexer::LexemeGenerator::iterator &iterator) {
  auto var = *iterator;
  return std::visit(
      details::utils::Overloaded{
          [&](lexer::And) -> ConjunctionPrimeFormula {
            ++iterator;
            auto fol_formula = ParseFolFormula(iterator);
            auto conj_prime = ParseConjuctionPrimeFormula(iterator);
            return {std::make_unique<
                std::pair<UnaryFormula, ConjunctionPrimeFormula>>(
                std::move(fol_formula), std::move(conj_prime))};
          },
          [](...) -> ConjunctionPrimeFormula { return {lexer::EPS{}}; }},
      var);
}

ConjunctionFormula ParseConjuctionFormula(
    lexer::LexemeGenerator::iterator &iterator) {
  auto fol_formula = ParseFolFormula(iterator);
  auto conj_prime = ParseConjuctionPrimeFormula(iterator);
  return {std::make_unique<std::pair<UnaryFormula, ConjunctionPrimeFormula>>(
      std::move(fol_formula), std::move(conj_prime))};
}

DisjunctionPrimeFormula ParseDisjunctionPrimeFormula(
    lexer::LexemeGenerator::iterator &iterator) {
  auto var = *iterator;
  return std::visit(
      details::utils::Overloaded{
          [&](lexer::Or) -> DisjunctionPrimeFormula {
            ++iterator;
            auto conj = ParseConjuctionFormula(iterator);
            auto disj_prime = ParseDisjunctionPrimeFormula(iterator);
            return {std::make_unique<
                std::pair<ConjunctionFormula, DisjunctionPrimeFormula>>(
                std::move(conj), std::move(disj_prime))};
          },
          [](...) -> DisjunctionPrimeFormula { return {lexer::EPS{}}; }},
      var);
}

DisjunctionFormula ParseDisjunctionFormula(
    lexer::LexemeGenerator::iterator &iterator) {
  auto conj = ParseConjuctionFormula(iterator);
  auto disj_prime = ParseDisjunctionPrimeFormula(iterator);
  return {std::move(conj), std::move(disj_prime)};
}

ImplicationFormula ParseImplicationFormula(
    lexer::LexemeGenerator::iterator &iterator) {
  auto disjunction = ParseDisjunctionFormula(iterator);

  auto next_symbol = *iterator;
  if (!std::holds_alternative<lexer::Implies>(next_symbol)) {
    return {std::move(disjunction)};
  }

  ++iterator;

  auto impl = ImplicationFormula{
      std::make_unique<std::pair<DisjunctionFormula, ImplicationFormula>>(
          std::move(disjunction), ParseImplicationFormula(iterator))};
  return impl;
}

UnaryFormula ParseFolFormula(lexer::LexemeGenerator::iterator &iterator) {
  auto var = *iterator;
  return {std::visit(
      details::utils::Overloaded{
          [&iterator](lexer::OpenBracket) -> UnaryFormula {
            ++iterator;
            ImplicationFormula result = ParseImplicationFormula(iterator);
            if (!std::holds_alternative<lexer::CloseBracket>(*iterator)) {
              throw ParseError{"No close bracket at (<impl>)."};
            }
            ++iterator;

            return {BracketFormula{std::move(result)}};
          },
          [&iterator](lexer::Not) -> UnaryFormula {
            ++iterator;
            return {NotFormula{
                std::make_unique<UnaryFormula>(ParseFolFormula(iterator))}};
          },
          [&iterator](lexer::Forall) -> UnaryFormula {
            ++iterator;
            auto var = *iterator;
            if (!std::holds_alternative<lexer::Variable>(var)) {
              throw ParseError{
                  "Error in parsing @ <var> . <impl>: no variable after @."};
            }
            ++iterator;

            if (!std::holds_alternative<lexer::Dot>(*iterator)) {
              throw ParseError{
                  "Error in parsing @ <var> . <impl>: no . after <var>."};
            }
            ++iterator;

            ImplicationFormula impl = ParseImplicationFormula(iterator);
            return {ForallFormula{
                {std::move(std::get<lexer::Variable>(var)), std::move(impl)}}};
          },
          [&iterator](lexer::Exists) -> UnaryFormula {
            ++iterator;
            auto var = *iterator;
            if (!std::holds_alternative<lexer::Variable>(var)) {
              throw ParseError{
                  "Error in parsing ? <var> . <impl>: no variable after @."};
            }
            ++iterator;

            if (!std::holds_alternative<lexer::Dot>(*iterator)) {
              throw ParseError{
                  "Error in parsing ? <var> . <impl>: no . after <var>."};
            }
            ++iterator;

            ImplicationFormula impl = ParseImplicationFormula(iterator);
            return {ExistsFormula{
                {std::move(std::get<lexer::Variable>(var)), std::move(impl)}}};
          },
          [&iterator](lexer::Predicate predicate) -> UnaryFormula {
            ++iterator;
            if (!std::holds_alternative<lexer::OpenBracket>(*iterator)) {
              throw ParseError{"No args for predicate."};
            }
            ++iterator;

            auto pred = UnaryFormula{PredicateFormula{
                {std::move(predicate), ParseTermList(iterator)}}};
            if (!std::holds_alternative<lexer::CloseBracket>(*iterator)) {
              throw ParseError{"No close bracket at args for predicate."};
            }
            ++iterator;

            return pred;
          },
          [&](...) -> UnaryFormula {
            throw ParseError{"Unhandled variant in unary parsing"};
          }},
      var)};
}

FolFormula Parse(lexer::LexemeGenerator generator) {
  auto it = generator.begin();
  return ParseImplicationFormula(it);
}

}  // namespace fol::parser
