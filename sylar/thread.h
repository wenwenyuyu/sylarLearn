/*
 * @Author       : wenwneyuyu
 * @Date         : 2024-03-16 11:45:25
 * @LastEditors  : wenwenyuyu
 * @LastEditTime : 2024-03-20 12:17:24
 * @FilePath     : /sylar/thread.h
 * @Description  : 
 * Copyright 2024 OBKoro1, All Rights Reserved. 
 * 2024-03-16 11:45:25
 */

#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include "sylar/mutex.h"
#include <functional>
#include <memory>
#include <pthread.h>
#include <sched.h>
#include <string>

namespace sylar {
class Thread {
public:
  typedef std::shared_ptr<Thread> ptr;
  Thread(const Thread &) = delete;
  Thread &operator=(const Thread &) = delete;

  Thread(std::function<void()> cb, const std::string name);
  ~Thread();

  void join();
  pid_t getId() const { return m_pid; }
  const std::string &getName() const { return m_name; }
  
  static Thread *GetThis();
  static const std::string &GetName() ;
  static void SetName(const std::string &);

  static void *Run(void * args);
private:
  pid_t m_pid = 0;
  pthread_t m_thread = -1;
  std::string m_name;
  std::function<void()> m_cb;

  Semaphore m_sem;
};
}

#endif
