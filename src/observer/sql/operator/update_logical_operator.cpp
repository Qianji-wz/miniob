#include "sql/operator/update_logical_operator.h"

UpdateLogicalOperator::UpdateLogicalOperator(Table *table, Value value, const std::string &attr_name)
    : table_(table), value_(value), attr_name_(attr_name)
{}
