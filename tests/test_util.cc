/*
 * @Author       : wenwneyuyu
 * @Date         : 2024-03-18 13:40:00
 * @LastEditors  : wenwenyuyu
 * @LastEditTime : 2024-03-19 19:07:13
 * @FilePath     : /tests/test_util.cc
 * @Description  : 
 * Copyright 2024 OBKoro1, All Rights Reserved. 
 * 2024-03-18 13:40:00
 */

#include "sylar/log.h"
#include "sylar/util.h"
#include "sylar/marco.h"
#include <cassert>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_assert() {
  SYLAR_LOG_INFO(g_logger) << sylar::BackTraceToString(10, 2, "    ");
  SYLAR_ASSERT2(1 == 2, "error");
}

int main() {
  test_assert();
  return 0;
}