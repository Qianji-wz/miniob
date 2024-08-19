#ifndef DATE_UTILS
#define DATE_UTILS

class DateUtils
{
public:
  DateUtils()  = default;
  ~DateUtils() = default;

  static bool is_valid_date(const char *date_str);
  // 实现日期验证逻
  // 其他日期相关的函数...
};
#endif