/*
 * @Author       : wenwneyuyu
 * @Date         : 2024-03-16 16:00:36
 * @LastEditors  : wenwenyuyu
 * @LastEditTime : 2024-03-18 13:00:42
 * @FilePath     : /sylar/mutex.h
 * @Description  : 
 * Copyright 2024 OBKoro1, All Rights Reserved. 
 * 2024-03-16 16:00:36
 */

#ifndef __SYLAR_MUTEX_H__
#define __SYLAR_MUTEX_H__

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <ostream>
#include <pthread.h>
#include <semaphore.h>

namespace sylar {

class Semaphore {
public:
  Semaphore(const Semaphore &) = delete;
  Semaphore(const Semaphore &&) = delete;
  Semaphore &operator=(const Semaphore &) = delete;
  Semaphore &operator=(const Semaphore &&) = delete;

  Semaphore(std::uint32_t count = 0);
  ~Semaphore();

  void wait();
  void notify();
private:
  sem_t m_sem;
};
/**
 * @func:
 * @return {*}
 * @description: 使用模板类的方式来统一管理mutex，不需要mutex显示加锁和解锁。
 * 模板类生成时自动加锁，生命周期结束时自动解锁。RAII的编程思想
 */
template <class T> struct ScopedLockImpl {
public:
  ScopedLockImpl(T &mutex) : m_mutx(mutex) {
    m_mutx.lock();
    locked = true;
  }

  ~ScopedLockImpl() {
    unlock();
  }

  void lock() {
    if (!locked) {
      m_mutx.lock();
      locked = true;
    }
  }

  void unlock() {
    if (locked) {
      m_mutx.unlock();
      locked = false;
    }
  }
  
private:
  T &m_mutx;
  bool locked;
};

template <class T> struct ReadScopedLockImpl {
public:
  ReadScopedLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.rdlock();
    locked = true;
  }

  ~ReadScopedLockImpl() { unlock(); }

  void lock() {
    if (!locked) {
      m_mutex.rdlock();
      locked = true;
    }
  }

  void unlock() {
    if (locked) {
      m_mutex.unlock();
      locked = false;
    }
  }

private:
  T &m_mutex;
  bool locked;
};

template <class T> struct WriteScopedLockImpl {
public:
  WriteScopedLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.wrlock();
    locked = true;
  }

  ~WriteScopedLockImpl() { unlock(); }
  
  void lock() {
    if (!locked) {
      m_mutex.wrlock();
      locked = true;
    }
  }

  void unlock() {
    if (locked) {
      m_mutex.unlock();
      locked = false;
    }
  }
  
private:
  T &m_mutex;
  bool locked;
};

class Mutex {
public:
  Mutex(const Mutex &) = delete;
  Mutex(const Mutex &&) = delete;
  Mutex &operator=(const Mutex &) = delete;
  Mutex &operator=(const Mutex &&) = delete;

  typedef ScopedLockImpl<Mutex> Lock;

  Mutex();
  ~Mutex();
  void lock();
  void unlock();
private:
  pthread_mutex_t m_mutex;
};

// 读多写少使用读写锁
class RWMutex {
public:
  RWMutex(const RWMutex &) = delete;
  RWMutex(const RWMutex &&) = delete;
  RWMutex &operator=(const RWMutex &) = delete;
  RWMutex &operator=(const RWMutex &&) = delete;

  typedef ReadScopedLockImpl<RWMutex> ReadLock;
  typedef WriteScopedLockImpl<RWMutex> WriteLock;

  RWMutex();
  ~RWMutex();

  void rdlock();
  void wrlock();
  void unlock();
private:
  pthread_rwlock_t m_lock;
};

// 使用自旋锁防止互斥锁陷入内核中，性能损耗
class Spinlock {
public:
  Spinlock(const Spinlock &) = delete;
  Spinlock(const Spinlock &&) = delete;
  Spinlock &operator=(const Spinlock &) = delete;
  Spinlock &operator=(const Spinlock &&) = delete;

  typedef ScopedLockImpl<Spinlock> Lock;

  Spinlock();
  ~Spinlock();
  void lock();
  void unlock();
private:
  pthread_spinlock_t m_mutex;
};

//原子锁
class CASLock {
public:
  CASLock(const CASLock &) = delete;
  CASLock(const CASLock &&) = delete;
  CASLock &operator=(const CASLock &) = delete;
  CASLock &operator=(const CASLock &&) = delete;

  typedef ScopedLockImpl<CASLock> Lock;

  CASLock();
  ~CASLock();
  void lock();
  void unlock();
private:
  volatile std::atomic_flag m_mutex;
};

}
#endif