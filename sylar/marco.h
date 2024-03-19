/*
 * @Author       : wenwneyuyu
 * @Date         : 2024-03-18 13:52:51
 * @LastEditors  : wenwenyuyu
 * @LastEditTime : 2024-03-18 14:04:36
 * @FilePath     : /sylar/marco.h
 * @Description  : 
 * Copyright 2024 OBKoro1, All Rights Reserved. 
 * 2024-03-18 13:52:51
 */
#ifndef __SYLAR_MARCO_H__
#define __SYLAR_MARCO_H__

#include <assert.h>
#include "util.h"

#define SYLAR_ASSERT(x)                                                        \
  if (!(x)) {                                                                  \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())                                          \
        << "\nASSERTION: " #x                                                  \
        << "\nbacktrace:" << sylar::BackTraceToString(100, 2, "    ");         \
    assert(x);                                                                 \
  }


#define SYLAR_ASSERT2(x, w)                                                    \
  if (!(x)) {                                                                  \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())                                          \
        << "\nASSERTION: " #x << "    " #w                                     \
        << "\nbacktrace:" << sylar::BackTraceToString(100, 2, "    ");         \
    assert(x);                                                                 \
  }

#endif