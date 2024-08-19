#include "date_utils.h"
#include <cstdio>

bool DateUtils::is_valid_date(const char *s)
{
  int y, m, d;
  sscanf(s, "%d-%d-%d", &y, &m, &d);  // not check return value eq 3, lex guarantee
  static int mon[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  bool       leap  = (y % 400 == 0 || (y % 100 && y % 4 == 0));
  if (y > 0 && (m > 0) && (m <= 12) && (d > 0) && (d <= ((m == 2 && leap) ? 1 : 0) + mon[m])) {
    return true;
  } else {
    return false;
  }
}