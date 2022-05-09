#pragma once

#include <iostream>
#include <libfol-basictypes/term.hpp>
#include <libfol-basictypes/variable.hpp>
#include <libfol-matcher/matcher.hpp>
#include <libfol-parser/parser/print.hpp>
#include <libfol-parser/parser/types.hpp>
#include <libfol-transform/replace.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace fol::types {
class Atom {
 public:
  friend std::ostream& operator<<(std::ostream& os, const Atom& atom) {
    os << atom.predicate_name_ << "(";
    if (!atom.term_list_.empty()) {
      os << parser::ToString(atom.term_list_[0]);
    }
    for (std::vector<Term>::size_type i = 1; i < atom.term_list_.size(); ++i) {
      os << ", " << parser::ToString(atom.term_list_[i]);
    }
    os << ")\n";
    return os;
  }

  Atom(parser::PredicateFormula formula)
      : predicate_name_(formula.data.first),
        term_list_(FromTermList(std::move(formula.data.second))) {}

  Atom(const Atom& a) : predicate_name_(a.predicate_name_) {
    term_list_.reserve(a.terms_size());
    for (auto& t : a.term_list_) {
      term_list_.push_back(transform::ReplaceTerm(t, "", ""));
    }
  }

  Atom& operator=(const Atom& a) {
    auto copy = a;
    copy.swap(*this);
    return *this;
  }

  void swap(Atom& o) {
    o.term_list_.swap(term_list_);
    o.predicate_name_.swap(predicate_name_);
  }

  bool operator==(const Atom& o) const {
    return parser::ToString(*this) == parser::ToString(o);
  }

  bool operator<(const Atom& o) const {
    return parser::ToString(*this) < parser::ToString(o);
  }

  void Substitute(const Variable& from, const Term& to) {
    for (auto& term : term_list_) {
      term = transform::ReplaceTerm(term, from, parser::ToString(to));
    }
  }

  const Term& operator[](std::size_t i) const { return term_list_[i]; }
  const auto& terms() const { return term_list_; }
  auto& terms() { return term_list_; }
  std::size_t terms_size() const { return term_list_.size(); }
  const std::string& predicate_name() const { return predicate_name_; }

 private:
  std::pair<Term, std::optional<parser::TermList>> PopTermList(
      parser::TermList term_list) {
    if (std::holds_alternative<lexer::EPS>(term_list.data.second.data)) {
      return {std::move(term_list.data.first), std::nullopt};
    } else {
      return {std::move(term_list.data.first),
              std::move(*std::get<std::unique_ptr<parser::TermList>>(
                  term_list.data.second.data))};
    }
  }

  std::vector<Term> FromTermList(parser::TermList term_list) {
    std::vector<Term> terms;

    while (true) {
      auto [t, opt_t_list] = PopTermList(std::move(term_list));
      terms.push_back(std::move(t));

      if (opt_t_list.has_value()) {
        term_list = std::move(*opt_t_list);
      } else {
        break;
      }
    }

    return terms;
  }

  std::string predicate_name_;
  std::vector<Term> term_list_;
};
}  // namespace fol::types
