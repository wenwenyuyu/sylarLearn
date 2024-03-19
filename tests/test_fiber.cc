/*
 * @Author       : wenwneyuyu
 * @Date         : 2024-03-19 19:09:36
 * @LastEditors  : wenwenyuyu
 * @LastEditTime : 2024-03-19 20:28:36
 * @FilePath     : /tests/test_fiber.cc
 * @Description  : 
 * Copyright 2024 OBKoro1, All Rights Reserved. 
 * 2024-03-19 19:09:36
 */
#include "sylar/fiber.h"
#include "sylar/log.h"
#include "sylar/config.h"
#include "sylar/thread.h"
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void fun1() {
  SYLAR_LOG_INFO(g_logger) << "in sub fiber";
  sylar::Fiber::wait();
  SYLAR_LOG_INFO(g_logger)  << "step 1" ;
  sylar::Fiber::wait();
  SYLAR_LOG_INFO(g_logger) << "sub fiber finish" ;
}

void func() {
  sylar::Fiber::GetThis();
  sylar::Fiber::ptr fiber(new sylar::Fiber(fun1));
  SYLAR_LOG_INFO(g_logger) << "create fiber";
  fiber->resume();
  SYLAR_LOG_INFO(g_logger) << "main fiber 1";
  fiber->resume();
  SYLAR_LOG_INFO(g_logger) << "main fiber 2";
  fiber->resume();
  SYLAR_LOG_INFO(g_logger) << "main fiber finish";
}

int main() {
  sylar::Thread::SetName("main");
  std::vector<sylar::Thread::ptr> thrs;

  for (int i = 0; i < 3; i++) {
    thrs.push_back(sylar::Thread::ptr(
        new sylar::Thread(func, "thead_" + std::to_string(i))));
  }

  for (size_t i = 0; i < thrs.size(); i++) {
    thrs[i]->join();
  }

  return 0;
}