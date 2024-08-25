#pragma once

#include "sql/operator/physical_operator.h"

class Trx;
class DeleteStmt;

/**
 * @brief 物理算子，删除
 * @ingroup PhysicalOperator
 */
class UpdatePhysicalOperator : public PhysicalOperator
{
public:
  UpdatePhysicalOperator(Table *table, Value value, const std::string &attr_name)
      : table_(table), value_(value), attr_name_(attr_name)
  {}

  virtual ~UpdatePhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::UPDATE; }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override { return nullptr; }

  Value              value() const { return value_; }
  const std::string &attr_name() const { return attr_name_; }

private:
  Table              *table_ = nullptr;
  Trx                *trx_   = nullptr;
  std::vector<Record> records_;

  Value       value_;
  std::string attr_name_;
};
