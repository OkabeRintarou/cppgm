#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include "posttoken.h"

struct Value {
  Value(uintmax_t v, bool sign) : value(v), is_sign(sign) {}

  uintmax_t value;
  bool is_sign;
};

struct PrimaryExpression;
struct UnaryExpression;
struct MultiplicativeExpression;
struct AdditiveExpression;
struct ShiftExpression;
struct RelationalExpression;
struct EqualityExpression;
struct AndExpression;
struct ExclusiveExpression;
struct InclusiveExpression;
struct LogicalAndExpression;
struct LogicalOrExpression;
struct ControllingExpression;

class Visitor {
public:
  virtual ~Visitor() = default;

  virtual Value visit(const PrimaryExpression &expr) const = 0;

  virtual Value visit(const UnaryExpression &expr) const = 0;

  virtual Value visit(const MultiplicativeExpression &expr) const = 0;

  virtual Value visit(const AdditiveExpression &expr) const = 0;

  virtual Value visit(const ShiftExpression &expr) const = 0;

  virtual Value visit(const RelationalExpression &expr) const = 0;

  virtual Value visit(const EqualityExpression &expr) const = 0;

  virtual Value visit(const AndExpression &expr) const = 0;

  virtual Value visit(const ExclusiveExpression &expr) const = 0;

  virtual Value visit(const InclusiveExpression &expr) const = 0;

  virtual Value visit(const LogicalAndExpression &expr) const = 0;

  virtual Value visit(const LogicalOrExpression &expr) const = 0;

  virtual Value visit(const ControllingExpression &expr) const = 0;
};

struct Expression {
  virtual Value eval(const Visitor *v) const = 0;
};

struct PrimaryExpression : public Expression {

  explicit PrimaryExpression(Value v) : val(v), ctrlExpr(nullptr) {}

  explicit PrimaryExpression(std::unique_ptr<ControllingExpression> expr)
    : val(0, false), ctrlExpr(std::move(expr)) {}

  Value eval(const Visitor *v) const override { return v->visit(*this); }

  Value val;
  std::unique_ptr<ControllingExpression> ctrlExpr;
};

struct UnaryExpression : public Expression {

  explicit UnaryExpression(std::unique_ptr<PrimaryExpression> p)
    : primaryExpr(std::move(p)), op(OP_NONE), unaryExpr(nullptr) {}

  UnaryExpression(ETokenType o, std::unique_ptr<UnaryExpression> expr)
    : primaryExpr(nullptr), op(o), unaryExpr(std::move(expr)) {}

  Value eval(const Visitor *v) const override { return v->visit(*this); }

  std::unique_ptr<PrimaryExpression> primaryExpr;

  ETokenType op;
  std::unique_ptr<UnaryExpression> unaryExpr;
};

struct MultiplicativeExpression : public Expression {

  MultiplicativeExpression(std::vector<std::unique_ptr<UnaryExpression>> exprs,
                           std::vector<ETokenType> op)
    : unaryExprs(std::move(exprs)), ops(std::move(op)) {

  }

  Value eval(const Visitor *v) const override { return v->visit(*this); }

  std::vector<std::unique_ptr<UnaryExpression>> unaryExprs;
  std::vector<ETokenType> ops;
};

struct AdditiveExpression : public Expression {

  Value eval(const Visitor *v) const override { return v->visit(*this); }
};

struct ShiftExpression : public Expression {

  Value eval(const Visitor *v) const override { return v->visit(*this); }
};

struct RelationalExpression : public Expression {

  Value eval(const Visitor *v) const override { return v->visit(*this); }
};

struct EqualityExpression : public Expression {

  Value eval(const Visitor *v) const override { return v->visit(*this); }
};

struct AndExpression : public Expression {

  Value eval(const Visitor *v) const override { return v->visit(*this); }
};

struct ExclusiveExpression : public Expression {

  Value eval(const Visitor *v) const override { return v->visit(*this); }
};

struct InclusiveExpression : public Expression {

  Value eval(const Visitor *v) const override { return v->visit(*this); }
};


struct LogicalAndExpression : public Expression {

  Value eval(const Visitor *v) const override { return v->visit(*this); }
};

struct LogicalOrExpression : public Expression {


  Value eval(const Visitor *v) const override { return v->visit(*this); }

};


struct ControllingExpression : public Expression {

  Value eval(const Visitor *v) const override { return v->visit(*this); }

};


class CtrlVisitor : public Visitor {
public:
  Value visit(const PrimaryExpression &expr) const override;

  Value visit(const UnaryExpression &expr) const override;

  Value visit(const MultiplicativeExpression &expr) const override;

  Value visit(const AdditiveExpression &expr) const override;

  Value visit(const ShiftExpression &expr) const override;

  Value visit(const RelationalExpression &expr) const override;

  Value visit(const EqualityExpression &expr) const override;

  Value visit(const AndExpression &expr) const override;

  Value visit(const ExclusiveExpression &expr) const override;

  Value visit(const InclusiveExpression &expr) const override;

  Value visit(const LogicalAndExpression &expr) const override;

  Value visit(const LogicalOrExpression &expr) const override;

  Value visit(const ControllingExpression &expr) const override;
};

Value eval(const Expression &expr);

