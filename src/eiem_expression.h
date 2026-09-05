#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <cctype>
#include <cstdlib>
#include <cerrno>
#include <cmath>

using EiemVariables = std::unordered_map<std::string, double>;

struct EiemExpression {
  enum Op { Number, Variable, Not, Negative, Equal, NotEqual, Less, LessEqual,
            Greater, GreaterEqual, And, Or } op = Number;
  double number = 0;
  std::string variable;
  std::shared_ptr<EiemExpression> left, right;

  bool Validate(const EiemVariables &variables, std::string &error) const {
    if (op == Variable && !variables.count(variable)) {
      error = "Undeclared variable: " + variable;
      return false;
    }
    return (!left || left->Validate(variables, error)) &&
           (!right || right->Validate(variables, error));
  }
  double Evaluate(const EiemVariables &variables) const {
    switch (op) {
      case Number: return number;
      case Variable: return variables.at(variable);
      case Not: return !left->Evaluate(variables);
      case Negative: return -left->Evaluate(variables);
      case And: return left->Evaluate(variables) && right->Evaluate(variables);
      case Or: return left->Evaluate(variables) || right->Evaluate(variables);
      case Equal: return left->Evaluate(variables) == right->Evaluate(variables);
      case NotEqual: return left->Evaluate(variables) != right->Evaluate(variables);
      case Less: return left->Evaluate(variables) < right->Evaluate(variables);
      case LessEqual: return left->Evaluate(variables) <= right->Evaluate(variables);
      case Greater: return left->Evaluate(variables) > right->Evaluate(variables);
      case GreaterEqual: return left->Evaluate(variables) >= right->Evaluate(variables);
    }
    return 0;
  }
};

static bool EiemVariableName(const std::string &name) {
  if (name.size() < 2 || name[0] != '$' ||
      !(std::isalpha((unsigned char)name[1]) || name[1] == '_')) return false;
  for (size_t i = 2; i < name.size(); ++i)
    if (!(std::isalnum((unsigned char)name[i]) || name[i] == '_')) return false;
  return true;
}

static bool EiemNumber(const std::string &text, double *out) {
  if (text == "true") { *out = 1; return true; }
  if (text == "false") { *out = 0; return true; }
  char *end = nullptr;
  errno = 0;
  double n = std::strtod(text.c_str(), &end);
  if (errno || end == text.c_str() || *end || !std::isfinite(n)) return false;
  *out = n;
  return true;
}

// Small precedence parser. Syntax is parsed once; evaluating it never calls Unity.
class EiemExpressionParser {
  using Node = std::shared_ptr<EiemExpression>;
  const std::string &text_;
  size_t pos_ = 0;
  size_t nodes_ = 0;
  std::string &error_;
  void Space() { while (pos_ < text_.size() && std::isspace((unsigned char)text_[pos_])) ++pos_; }
  Node Fail() { error_ = "Invalid expression at column " + std::to_string(pos_ + 1); return nullptr; }
  Node Primary(unsigned depth) {
    Space();
    if (depth > 128 || ++nodes_ > 256 || pos_ == text_.size()) return Fail();
    char c = text_[pos_];
    if (c == '!' || c == '-' || c == '+') {
      ++pos_;
      auto child = Primary(depth + 1);
      if (!child || c == '+') return child;
      auto node = std::make_shared<EiemExpression>();
      node->op = c == '!' ? EiemExpression::Not : EiemExpression::Negative;
      node->left = child;
      return node;
    }
    if (c == '(') {
      ++pos_;
      auto node = Binary(1, depth + 1);
      Space();
      if (!node || pos_ == text_.size() || text_[pos_] != ')') return Fail();
      ++pos_;
      return node;
    }
    auto node = std::make_shared<EiemExpression>();
    if (c == '$' || std::isalpha((unsigned char)c)) {
      size_t start = pos_++;
      while (pos_ < text_.size() && (std::isalnum((unsigned char)text_[pos_]) || text_[pos_] == '_')) ++pos_;
      std::string token = text_.substr(start, pos_ - start);
      if (token == "true" || token == "false") node->number = token == "true";
      else if (EiemVariableName(token)) { node->op = EiemExpression::Variable; node->variable = token; }
      else return Fail();
      return node;
    }
    char *end = nullptr;
    errno = 0;
    node->number = std::strtod(text_.c_str() + pos_, &end);
    if (errno || end == text_.c_str() + pos_ || !std::isfinite(node->number)) return Fail();
    pos_ = (size_t)(end - text_.c_str());
    return node;
  }
  Node Binary(int minimum, unsigned depth) {
    auto left = Primary(depth);
    if (!left) return nullptr;
    struct Operator { const char *text; int precedence; EiemExpression::Op op; };
    static const Operator ops[] = {{"||",1,EiemExpression::Or}, {"&&",2,EiemExpression::And},
      {"==",3,EiemExpression::Equal}, {"!=",3,EiemExpression::NotEqual},
      {"<=",4,EiemExpression::LessEqual}, {">=",4,EiemExpression::GreaterEqual},
      {"<",4,EiemExpression::Less}, {">",4,EiemExpression::Greater}};
    for (;;) {
      Space();
      const Operator *selected = nullptr;
      for (const auto &op : ops)
        if (text_.compare(pos_, std::char_traits<char>::length(op.text), op.text) == 0) { selected = &op; break; }
      if (!selected || selected->precedence < minimum) return left;
      pos_ += std::char_traits<char>::length(selected->text);
      auto right = Binary(selected->precedence + 1, depth + 1);
      if (!right) return nullptr;
      if (++nodes_ > 256) return Fail();
      auto node = std::make_shared<EiemExpression>();
      node->op = selected->op; node->left = left; node->right = right;
      left = node;
    }
  }
 public:
  EiemExpressionParser(const std::string &text, std::string &error) : text_(text), error_(error) {}
  Node Parse() {
    auto node = Binary(1, 0);
    Space();
    return pos_ == text_.size() ? node : Fail();
  }
};
