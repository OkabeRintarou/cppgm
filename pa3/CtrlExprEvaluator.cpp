#include <memory>
#include "CtrlExprEvaluator.h"
#include "common.h"

Value CtrlVisitor::visit(const PrimaryExpression &expr) const {
  if (expr.ctrlExpr != nullptr) {
    return expr.ctrlExpr->eval(this);
  }

  return expr.val;
}

Value CtrlVisitor::visit(const UnaryExpression &expr) const {
  if (expr.primaryExpr != nullptr) {
    return expr.primaryExpr->eval(this);
  }

  ASSERT(expr.unaryExpr != nullptr, "unary expression must exist");

  Value value = expr.unaryExpr->eval(this);

  switch (expr.op) {
    case OP_PLUS:
    case OP_MINUS:
    case OP_LNOT:
    case OP_COMPL:
      break;
    default:
      ASSERT(false, "never reach here");
  }

  return value;
}

Value CtrlVisitor::visit(const MultiplicativeExpression &expr) const {
  ASSERT(!expr.unaryExprs.empty() && expr.unaryExprs.size() == expr.ops.size() + 1,
         "must be valid multiplicative expression");
  size_t i = 0, sz = expr.unaryExprs.size();
  Value value = expr.unaryExprs[i]->eval(this);


  return value;
}

Value CtrlVisitor::visit(const AdditiveExpression &expr) const {
  return {0, false};
}

Value CtrlVisitor::visit(const ShiftExpression &expr) const {
  return {0, false};
}

Value CtrlVisitor::visit(const RelationalExpression &expr) const {
  return {0, false};
}

Value CtrlVisitor::visit(const EqualityExpression &expr) const {
  return {0, false};
}

Value CtrlVisitor::visit(const AndExpression &expr) const {
  return {0, false};
}

Value CtrlVisitor::visit(const ExclusiveExpression &expr) const {
  return {0, false};
}


Value CtrlVisitor::visit(const InclusiveExpression &expr) const {
  return {0, false};
}

Value CtrlVisitor::visit(const LogicalAndExpression &expr) const {
  return {0, false};
}

Value CtrlVisitor::visit(const LogicalOrExpression &expr) const {
  return {0, false};
}

Value CtrlVisitor::visit(const ControllingExpression &expr) const {
  return {0, false};
}

Value eval(const Expression &expr) {
  auto visitor = std::make_unique<CtrlVisitor>();
  return expr.eval(visitor.get());
}