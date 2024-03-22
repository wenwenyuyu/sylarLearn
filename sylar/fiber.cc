/*
 * @Author       : wenwneyuyu
 * @Date         : 2024-03-18 19:44:17
 * @LastEditors  : wenwenyuyu
 * @LastEditTime : 2024-03-22 20:42:32
 * @FilePath     : /sylar/fiber.cc
 * @Description  : 
 * Copyright 2024 OBKoro1, All Rights Reserved. 
 * 2024-03-18 19:44:17
 */
#include "fiber.h"
#include "sylar/config.h"
#include "sylar/log.h"
#include "sylar/marco.h"
#include "sylar/scheduler.h"
#include "sylar/thread.h"
#include "sylar/util.h"
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <ucontext.h>

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 正在运行的协程变量
static thread_local Fiber *t_fiber = nullptr;
// 主协程
static thread_local Fiber::ptr t_threadFiber = nullptr;

static thread_local std::atomic_int64_t f_id = {0};
static thread_local std::atomic_uint64_t f_count = {0};

static ConfigVar<std::uint32_t>::ptr g_conf =
    Config::Lookup<std::uint32_t>("fiber.stack.size", 1024 * 1024, "fiber size");

class MallocStackAllocator {
public:
  static void *Allocator(uint32_t size) {
    return malloc(size);
  }

  static void Dealloc(void *vp, uint32_t size) {
    free(vp);
  }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetId() {
  if (t_fiber) {
    return t_fiber->m_id;
  }
  return 0;
}
/**
 * @func: 
 * @return {*}
 * @description: 线程中主协程初始化，id默认设置为0，没有cb函数，不需要栈空间，只用来调度其他协程
 */
Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    ++f_count;
    SYLAR_LOG_INFO(g_logger) << "main fiber";
}
/**
 * @func: 
 * @param {function<void()>} cb
 * @param {uint32_t} size
 * @return {*}
 * @description: 真正的协程初始化函数，用来执行协程任务
 */
Fiber::Fiber(std::function<void()> cb, std::uint32_t size)
    : m_id(++f_id), m_cb(cb) {
  ++f_count;
  m_stacksize = size ? size : g_conf->getValue();
  m_stack = StackAllocator::Allocator(m_stacksize);
  SYLAR_ASSERT(m_stack);
  if (getcontext(&m_ctx)) {
    SYLAR_ASSERT2(false, "Fiber::Fiber getcontext");
  }
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;
  m_ctx.uc_link = &Scheduler::GetMainFiber()->m_ctx;

  makecontext(&m_ctx, &Fiber::Func, 0);
  SYLAR_LOG_INFO(g_logger) << "new fiber id = " << m_id;
}

/**
 * @func: 
 * @return {*}
 * @description: 析构函数，分为主协程析构和子协程析构，通过栈空间进行判断
 */
Fiber::~Fiber() {
  --f_count;
  if (m_stack) {
    SYLAR_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
    StackAllocator::Dealloc(m_stack, m_stacksize);
    SYLAR_LOG_INFO(g_logger) << "Thread id = " << sylar::getThreadId() <<" ~fiber id = " << getId();
  } else {
    SYLAR_ASSERT(!m_cb);
    SYLAR_ASSERT(m_state == EXEC);

    Fiber *cur = t_fiber;
    if (cur == t_fiber) {
      SetThis(nullptr);
      SYLAR_LOG_INFO(g_logger) << "Thread id = " << sylar::getThreadId() << " ~main fiber";
    }
  }
}

/**
 * @func: 
 * @param {function<void()>} cb
 * @return {*}
 * @description: 重置协程中的函数
 */
void Fiber::reset(std::function<void()> cb) {
  SYLAR_ASSERT(m_stack);
  SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
  m_cb = cb;
  if(getcontext(&m_ctx)) {
    SYLAR_ASSERT2(false, "getcontext");
  }

  m_ctx.uc_link = &Scheduler::GetMainFiber()->m_ctx;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  makecontext(&m_ctx, &Fiber::Func, 0);
  m_state = INIT;
}
/**
 * @func: 
 * @return {*}
 * @description: 协程恢复，交换主协程和子协程的栈空间
 */
void Fiber::resume() {
  SetThis(this);
  SYLAR_ASSERT(m_state != EXEC);
  m_state = EXEC;
  if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
    SYLAR_ASSERT2(false, "resume");
  }
}

/**
 * @func: 
 * @return {*}
 * @description: 交换子协程和主协程的栈空间
 */
void Fiber::swapOut() {
  SetThis(Scheduler::GetMainFiber());
  if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
    SYLAR_ASSERT2(false, "swap out");
  }
}

/**
 * @func: 
 * @return {*}
 * @description: 切换到主协程中
 */
void Fiber::wait() {
  Fiber::ptr cur = GetThis();
  SYLAR_ASSERT(cur->m_state == EXEC);
  cur->m_state = HOLD;
  cur->swapOut();
}

/**
 * @func: 
 * @return {*}
 * @description: 获得当前运行的协程，同时若主协程未初始化则初始化主协程
 */
Fiber::ptr Fiber::GetThis() {
  if (t_fiber) {
    return t_fiber->shared_from_this();
  }

  Fiber::ptr main_fiber(new Fiber);
  SYLAR_ASSERT(t_fiber == main_fiber.get());
  t_threadFiber = main_fiber;
  return t_fiber->shared_from_this();

}

void Fiber::SetThis(Fiber *f) { t_fiber = f; }

/**
 * @func: 
 * @return {*}
 * @description: 协程运行函数
 */
void Fiber::Func() {
  Fiber::ptr cur = GetThis();
  SYLAR_ASSERT(cur);
  try {
    cur->m_cb();
    cur->m_cb = nullptr;
    cur->m_state = TERM;
  } catch (std::exception &ex) {
    cur->m_state = EXCEPT;
    SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BackTraceToString();
  }
}

uint64_t Fiber::TotalFibers() { return f_count; }

// void Fiber::call() {
//     SetThis(this);
//     m_state = EXEC;
//     if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
//         SYLAR_ASSERT2(false, "swapcontext");
//     }
// }

// void Fiber::back() {
//     SetThis(t_threadFiber.get());
//     if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
//         SYLAR_ASSERT2(false, "swapcontext");
//     }
// }

// void Fiber::MainFunc() {
//     Fiber::ptr cur = GetThis();
//     SYLAR_ASSERT(cur);
//     try {
//         cur->m_cb();
//         cur->m_cb = nullptr;
//         cur->m_state = TERM;
//     } catch (std::exception& ex) {
//         cur->m_state = EXCEPT;
//         SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
//             << " fiber_id=" << cur->getId()
//             << std::endl
//             << sylar::BacktraceToString();
//     } catch (...) {
//         cur->m_state = EXCEPT;
//         SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
//             << " fiber_id=" << cur->getId()
//             << std::endl
//             << sylar::BacktraceToString();
//     }

//     auto raw_ptr = cur.get();
//     cur.reset();
//     raw_ptr->swapOut();

//     SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
// }

// void Fiber::CallerMainFunc() {
//     Fiber::ptr cur = GetThis();
//     SYLAR_ASSERT(cur);
//     try {
//         cur->m_cb();
//         cur->m_cb = nullptr;
//         cur->m_state = TERM;
//     } catch (std::exception& ex) {
//         cur->m_state = EXCEPT;
//         SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
//             << " fiber_id=" << cur->getId()
//             << std::endl
//             << sylar::BacktraceToString();
//     } catch (...) {
//         cur->m_state = EXCEPT;
//         SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
//             << " fiber_id=" << cur->getId()
//             << std::endl
//             << sylar::BacktraceToString();
//     }

//     auto raw_ptr = cur.get();
//     cur.reset();
//     raw_ptr->back();
//     SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));

// }
}