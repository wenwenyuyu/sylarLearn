/*
 * @Author       : wenwneyuyu
 * @Date         : 2024-03-18 14:09:10
 * @LastEditors  : wenwenyuyu
 * @LastEditTime : 2024-03-19 19:45:39
 * @FilePath     : /sylar/fiber.h
 * @Description  : 
 * Copyright 2024 OBKoro1, All Rights Reserved. 
 * 2024-03-18 14:09:10
 */

#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <cstdint>
#include <functional>
#include <memory>
#include <ucontext.h>

namespace sylar {

class Fiber : public std::enable_shared_from_this<Fiber>{
public:
  typedef std::shared_ptr<Fiber> ptr;

  enum State {
    INIT,
    HOLD,
    EXEC,
    TERM,
    READY,
    EXCEPT
  };

  Fiber(std::function<void()> cb, std::uint32_t size = 0);
  ~Fiber();
  void reset(std::function<void()> cb);
  void swapOut();
  static void wait();
  void resume();
  static void Func();
  static Fiber::ptr GetThis();
  static void SetThis(Fiber *f);

  uint64_t getId() const { return m_id; }
  State getState() const { return m_state; }
  static uint64_t TotalFibers();
  static uint64_t GetId();
private:
  Fiber();
private:
  std::uint64_t m_id = 0;
  ucontext_t m_ctx;
  void *m_stack = nullptr;
  std::uint32_t m_stacksize = 0;
  std::function<void()> m_cb;
  State m_state;
};

}

#endif