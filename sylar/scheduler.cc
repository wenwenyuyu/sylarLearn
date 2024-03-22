/*
 * @Author       : wenwneyuyu
 * @Date         : 2024-03-21 10:15:45
 * @LastEditors  : wenwenyuyu
 * @LastEditTime : 2024-03-22 20:14:47
 * @FilePath     : /sylar/scheduler.cc
 * @Description  : 
 * Copyright 2024 OBKoro1, All Rights Reserved. 
 * 2024-03-21 10:15:45
 */

#include "scheduler.h"
#include "sylar/fiber.h"
#include "sylar/marco.h"
#include "log.h"
#include "sylar/thread.h"
#include "sylar/util.h"
#include <cstddef>
#include <functional>
#include <string>

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_ROOT();
static thread_local Scheduler *t_scheduler = nullptr;
static thread_local Fiber *t_scheduler_fiber = nullptr;
/**
 * @func: 
 * @return {*}
 * @description: use_caller为true，则将scheduler初始化的线程也纳入线程池中
 */
Scheduler::Scheduler(std::size_t threads, bool use_caller,
                     const std::string &name)
    : m_name(name) {
  SYLAR_ASSERT(threads > 0);

  if (use_caller) {
    sylar::Fiber::GetThis();
    --threads;

    SYLAR_ASSERT(GetThis() == nullptr);
    t_scheduler = GetThis();

    m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0));
    t_scheduler_fiber = m_rootFiber.get();
    m_rootThread = sylar::getThreadId();
    m_threadIds.push_back(m_rootThread);
  } else {
    m_rootThread = -1;
  }
  m_threadCount = threads;
}

Scheduler::~Scheduler() {
  SYLAR_ASSERT(m_stopping);
  if (GetThis() == this) {
    t_scheduler = nullptr;
  }
  SYLAR_LOG_INFO(g_logger) << "finish";
}

/**
 * @func: 
 * @return {*}
 * @description: 初始化线程池，并进入协程队列中运行相应任务
 */
void Scheduler::start() {
  MutexType::Lock lock(m_mutex);
  if (!m_stopping) {
    return;
  }
  m_stopping = false;
  m_threads.resize(m_threadCount);
  for (size_t i = 0; i < m_threadCount; ++i) {
    m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                  "thread_" + std::to_string(i)));
    m_threadIds.push_back(m_threads[i]->getId());
  }
  lock.unlock();
}

void Scheduler::stop() {
  auto_stopping = true;
  if (m_rootFiber && m_threadCount == 0 &&
      (m_rootFiber->getState() == Fiber::INIT ||
       m_rootFiber->getState() == Fiber::TERM)) {
    SYLAR_LOG_INFO(g_logger) << this << "stopped";
    m_stopping = true;

    if (stopping()) {
      return;
    }
  }

  // if(m_rootThread != -1) {
  //     SYLAR_ASSERT(GetThis() == this);
  // } else {
  //     SYLAR_ASSERT(GetThis() != this);
  // }

   m_stopping = true;
  for(size_t i = 0; i < m_threadCount; ++i) {
      tickle();
  }

  // if(m_rootFiber) {
  //     tickle();
  // }

  // if(m_rootFiber) {
  //   if(!stopping()) {
  //       m_rootFiber->call();
  //     }
  // }
  
  std::vector<Thread::ptr> thrs;
  {
    MutexType::Lock lock(m_mutex);
    thrs.swap(m_threads);
  }

  for(auto& i : thrs) {
    i->join();
  }
}

/**
 * @func: 
 * @return {*}
 * @description: 通过std::bind传入this指针；线程在协程任务队列中取出任务运行
 */
void Scheduler::run() {
  SYLAR_LOG_INFO(g_logger) << "thread run , id = " << sylar::getThreadId();
  setThis();
  
  if(sylar::getThreadId() != m_rootThread) {
    t_scheduler_fiber = Fiber::GetThis().get();
  }

  Fiber::ptr cb_fiber;
  Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
  FiberAndThread ft;

  while (true) {
    ft.reset();

    bool need_tickle = false;
    bool is_active = false;

    {
      MutexType::Lock lock(m_mutex);
      auto it = m_fibers.begin();
      while (it != m_fibers.end()) {
        if (it->m_threadId != -1 && it->m_threadId != sylar::getThreadId()) {
          ++it;
          continue;
        }

        SYLAR_ASSERT(it->m_cb || it->m_fiber);
        if (it->m_fiber && it->m_fiber->getState() == Fiber::EXEC) {
          ++it;
          continue;
        }

        ft = *it;
        m_fibers.erase(it);
        ++m_activeThreadCount;
        is_active = true;
        it++;
        break;
      }
    need_tickle |= it != m_fibers.end();      
    }

    if (need_tickle) {
      tickle();
    }

    if (ft.m_fiber && (ft.m_fiber->getState() != Fiber::TERM &&
                       ft.m_fiber->getState() != Fiber::EXCEPT)) {
      SYLAR_LOG_INFO(g_logger) << "fiber get";
      ft.m_fiber->resume();
      --m_activeThreadCount;

      if (ft.m_fiber->getState() == Fiber::READY) {
        schedule(ft.m_fiber);
      } else if (ft.m_fiber->getState() != Fiber::TERM &&
                 ft.m_fiber->getState() != Fiber::EXCEPT) {
        ft.m_fiber->m_state = Fiber::HOLD;
      }
      ft.reset();

    } else if (ft.m_cb) {
      if (cb_fiber) {
        cb_fiber->reset(ft.m_cb);
      } else {
        cb_fiber.reset(new Fiber(ft.m_cb));
      }
      ft.reset();
      cb_fiber->resume();
      --m_activeThreadCount;

      if (cb_fiber->getState() == Fiber::READY) {
        schedule(cb_fiber);
        cb_fiber.reset();
      } else if (cb_fiber->getState() == Fiber::EXCEPT ||
                 cb_fiber->getState() == Fiber::TERM) {
        cb_fiber->reset(nullptr);
      } else {
        cb_fiber->m_state = Fiber::HOLD;
        cb_fiber.reset();
      }

    } else {

      if (is_active) {
        --m_activeThreadCount;
        continue;
      }

      if (idle_fiber->getState() == Fiber::TERM) {
        SYLAR_LOG_INFO(g_logger) << "idle fiber term";
        break;
      }

      ++m_idleThreadCount;
      idle_fiber->resume();
      --m_idleThreadCount;
      if (idle_fiber->getState() != Fiber::EXCEPT &&
          idle_fiber->getState() != Fiber::TERM) {
        idle_fiber->m_state = Fiber::HOLD;
      }

    }
    
  }
}

Scheduler *Scheduler::GetThis() { return t_scheduler; }

Fiber *Scheduler::GetMainFiber() { return t_scheduler_fiber; }

void Scheduler::setThis() { t_scheduler = this; }

void Scheduler::tickle() {
  SYLAR_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
  MutexType::Lock lock(m_mutex);
  return auto_stopping && m_stopping && m_activeThreadCount == 0 &&
         m_fibers.empty();
}

/**
 * @func: 
 * @return {*}
 * @description: 如果没有用stop函数，使协程一直运行该函数空转；使用while而不用if的原因是主协程可能会调度第二次该函数，导致协程结束
 */
void Scheduler::idle() {
  while (!stopping()) {
    //SYLAR_LOG_INFO(g_logger) << "idle fiber stuck, thread id = " << sylar::getThreadId() << " Fiber id = " << Fiber::GetId();
    Fiber::wait();
  }
}

}