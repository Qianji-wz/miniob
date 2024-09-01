/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/5/22.
//

#include "sql/stmt/filter_stmt.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/rc.h"
#include "sql/parser/parse_defs.h"
#include "sql/parser/value.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

FilterStmt::~FilterStmt()
{
  for (FilterUnit *unit : filter_units_) {
    delete unit;
  }
  filter_units_.clear();
}

RC FilterStmt::create(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
    const ConditionSqlNode *conditions, int condition_num, FilterStmt *&stmt)
{
  RC rc = RC::SUCCESS;
  stmt  = nullptr;

  FilterStmt *tmp_stmt = new FilterStmt();
  for (int i = 0; i < condition_num; i++) {
    FilterUnit *filter_unit = nullptr;

    rc = create_filter_unit(db, default_table, tables, conditions[i], filter_unit);
    if (rc != RC::SUCCESS) {
      delete tmp_stmt;
      LOG_WARN("failed to create filter unit. condition index=%d", i);
      return rc;
    }
    tmp_stmt->filter_units_.push_back(filter_unit);
  }

  stmt = tmp_stmt;
  return rc;
}

RC get_table_and_field(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
    const RelAttrSqlNode &attr, Table *&table, const FieldMeta *&field)
{
  if (common::is_blank(attr.relation_name.c_str())) {
    table = default_table;
  } else if (nullptr != tables) {
    auto iter = tables->find(attr.relation_name);
    if (iter != tables->end()) {
      table = iter->second;
    }
  } else {
    table = db->find_table(attr.relation_name.c_str());
  }
  if (nullptr == table) {
    LOG_WARN("No such table: attr.relation_name: %s", attr.relation_name.c_str());
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  field = table->table_meta().field(attr.attribute_name.c_str());
  if (nullptr == field) {
    LOG_WARN("no such field in table: table %s, field %s", table->name(), attr.attribute_name.c_str());
    table = nullptr;
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  return RC::SUCCESS;
}

double stringToDouble(const std::string &str)
{
  std::string numericPart;
  bool        decimalPointEncountered = false;

  // Traverse the string and accumulate numeric characters, including a single decimal point
  for (char c : str) {
    if (isdigit(c)) {
      numericPart += c;
    } else if (c == '.' && !decimalPointEncountered) {
      numericPart += c;
      decimalPointEncountered = true;  // Only allow one decimal point
    } else {
      break;  // Stop at the first non-numeric, non-decimal character
    }
  }

  // Convert the accumulated numeric part to a double
  return numericPart.empty() ? 0.0 : std::stod(numericPart);
}

RC FilterStmt::create_filter_unit(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
    const ConditionSqlNode &condition, FilterUnit *&filter_unit)
{
  RC rc = RC::SUCCESS;

  CompOp comp = condition.comp;
  if (comp < EQUAL_TO || comp >= NO_OP) {
    LOG_WARN("invalid compare operator : %d", comp);
    return RC::INVALID_ARGUMENT;
  }

  filter_unit = new FilterUnit;
  //定义两个attrtype，用于判断类型是否相同
  AttrType attr_left, attr_right;
  int      left_is_attr  = condition.left_is_attr;
  int      right_is_attr = condition.right_is_attr;
  if (condition.left_is_attr) {
    Table           *table = nullptr;
    const FieldMeta *field = nullptr;
    rc                     = get_table_and_field(db, default_table, tables, condition.left_attr, table, field);
    if (rc != RC::SUCCESS) {
      LOG_WARN("cannot find attr");
      return rc;
    }
    attr_left = field->type();
    FilterObj filter_obj;
    filter_obj.init_attr(Field(table, field));
    filter_unit->set_left(filter_obj);
  } else {
    attr_left = condition.left_value.attr_type();
    FilterObj filter_obj;
    filter_obj.init_value(condition.left_value);
    filter_unit->set_left(filter_obj);
  }

  if (condition.right_is_attr) {
    Table           *table = nullptr;
    const FieldMeta *field = nullptr;
    rc                     = get_table_and_field(db, default_table, tables, condition.right_attr, table, field);
    if (rc != RC::SUCCESS) {
      LOG_WARN("cannot find attr");
      return rc;
    }
    attr_right = field->type();
    FilterObj filter_obj;
    filter_obj.init_attr(Field(table, field));
    filter_unit->set_right(filter_obj);
  } else {
    attr_right = condition.right_value.attr_type();
    FilterObj filter_obj;
    filter_obj.init_value(condition.right_value);
    filter_unit->set_right(filter_obj);
  }
  filter_unit->set_comp(comp);

  //检查两个类型是否能够比较
  if (attr_right != attr_left) {
    //尝试类型转换
    if (attr_left == AttrType::FLOATS && attr_right == AttrType::INTS) {
      if (left_is_attr && !right_is_attr) {
        //左属性右数据
        FilterObj filter_obj;
        Value     new_value;
        new_value.set_float(condition.right_value.get_int());
        filter_obj.init_value(new_value);
        filter_unit->set_right(filter_obj);
      } else if (!left_is_attr && right_is_attr) {
        //左数据右属性
        FilterObj filter_obj;
        Value     new_value;
        new_value.set_int(condition.left_value.get_float());
        filter_obj.init_value(new_value);
        filter_unit->set_left(filter_obj);
      } else {
        return rc;
      }

    } else if (attr_left == AttrType::INTS && attr_right == AttrType::FLOATS) {
      if (left_is_attr && !right_is_attr) {
        //左属性右数据
        FilterObj filter_obj;
        Value     new_value;
        new_value.set_int(condition.right_value.get_float());
        filter_obj.init_value(new_value);
        filter_unit->set_right(filter_obj);
      } else if (!left_is_attr && right_is_attr) {
        //左数据右属性
        FilterObj filter_obj;
        Value     new_value;
        new_value.set_float(condition.left_value.get_int());
        filter_obj.init_value(new_value);
        filter_unit->set_left(filter_obj);
      } else {
        return rc;
      }
    } else if (attr_left == AttrType::DATES && attr_right == AttrType::CHARS) {
      //不必尝试将字符串转成日期的格式
      //因为在词法解析时已经判断过了
      return RC::SCHEMA_FIELD_TYPE_MISMATCH;
    } else if (attr_left == AttrType::CHARS && attr_right == AttrType::DATES) {
      //将date转成char
      return rc;
    }
    // else if (attr_left == AttrType::CHARS && attr_right == AttrType::FLOATS) {
    //   //将float转成char

    // } else {
    //   // LOG_INFO("set comop=NO_COMP");
    //   // filter_unit->set_comp(CompOp::NO_COMP);
    //   // return rc;
    // }
  }

  return rc;
}
