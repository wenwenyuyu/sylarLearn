/*
 * @Author       : wenwneyuyu
 * @Date         : 2024-03-18 14:09:10
 * @LastEditors  : wenwenyuyu
 * @LastEditTime : 2024-05-20 19:56:53
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

// 将协程作为调度的主体
// 每个线程拥有一个主协程负责调度子协程，主协程不进行任何工作，只是创建或销毁子协程，不需要任何栈空间，不能够主动创建
// 子协程可以由线程主动创建，创建后当前线程获得运行的子协程指针，当子协程运行结束后将子协程返回到主协程处

class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
public:
  typedef std::shared_ptr<Fiber> ptr;

  /**
  * @brief 协程状态
  */
  enum State {
  /// 初始化状态
    INIT,
  /// 暂停状态
    HOLD,
  /// 执行中状态
    EXEC,
        /// 结束状态
        TERM,
        /// 可执行状态
        READY,
        /// 异常状态
        EXCEPT
    };
private:
    /**
     * @brief 无参构造函数
     * @attention 每个线程第一个协程的构造
     */
    Fiber();

public:
    /**
     * @brief 构造函数
     * @param[in] cb 协程执行的函数
     * @param[in] stacksize 协程栈大小
     * @param[in] use_caller 是否在MainFiber上调度
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);

    /**
     * @brief 析构函数
     */
    ~Fiber();

    /**
     * @brief 重置协程执行函数,并设置状态
     * @pre getState() 为 INIT, TERM, EXCEPT
     * @post getState() = INIT
     */
    void reset(std::function<void()> cb);

    /**
     * @brief 将当前协程切换到运行状态
     * @pre getState() != EXEC
     * @post getState() = EXEC
     */
    void swapIn();

    /**
     * @brief 将当前协程切换到后台
     */
    void swapOut();

    /**
     * @brief 将当前线程切换到执行状态
     * @pre 执行的为当前线程的主协程
     */
    void call();

    /**
     * @brief 将当前线程切换到后台
     * @pre 执行的为该协程
     * @post 返回到线程的主协程
     */
    void back();

    /**
     * @brief 返回协程id
     */
    uint64_t getId() const { return m_id;}

    /**
     * @brief 返回协程状态
     */
    State getState() const { return m_state;}
public:

    /**
     * @brief 设置当前线程的运行协程
     * @param[in] f 运行协程
     */
    static void SetThis(Fiber* f);

    /**
     * @brief 返回当前所在的协程
     */
    static Fiber::ptr GetThis();

    /**
     * @brief 将当前协程切换到后台,并设置为READY状态
     * @post getState() = READY
     */
    static void YieldToReady();

    /**
     * @brief 将当前协程切换到后台,并设置为HOLD状态
     * @post getState() = HOLD
     */
    static void YieldToHold();

    /**
     * @brief 返回当前协程的总数量
     */
    static uint64_t TotalFibers();

    /**
     * @brief 协程执行函数
     * @post 执行完成返回到线程主协程
     */
    static void MainFunc();

    /**
     * @brief 协程执行函数
     * @post 执行完成返回到线程调度协程
     */
    static void CallerMainFunc();

    /**
     * @brief 获取当前协程的id
     */
    static uint64_t GetFiberId();
private:
  /// 协程id
  uint64_t m_id = 0;
  /// 协程运行栈大小
  uint32_t m_stacksize = 0;
  /// 协程状态
  State m_state = INIT;
  /// 协程上下文
  ucontext_t m_ctx;
  /// 协程运行栈指针
  void* m_stack = nullptr;
  /// 协程运行函数
  std::function<void()> m_cb;
};

}

#endif