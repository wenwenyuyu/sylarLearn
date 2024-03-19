/*
 * @Author       : wenwneyuyu
 * @Date         : 2023-12-15 21:34:57
 * @LastEditors  : wenwenyuyu
 * @LastEditTime : 2024-03-19 20:27:35
 * @FilePath     : /sylar/log.h
 * @Description  : 
 * Copyright 2024 OBKoro1, All Rights Reserved. 
 * 2023-12-15 21:34:57
 */
#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include "singleton.h"
#include "sylar/mutex.h"
#include "sylar/thread.h"
#include <cstdint>
#include <stdint.h>
#include <string>
#include <memory>
#include <list>
#include <fstream>
#include <iostream>  
#include <sstream>
#include <vector>
#include <ctime>
#include <map>
#define SYLAR_LOG_LEVEL(logger, level)                                         \
  if (logger->getLevel() <= level)                                             \
  sylar::LogEventWrap(                                                         \
      sylar::LogEvent::ptr(new sylar::LogEvent(                                \
          logger, level, __FILE__, __LINE__, 0, sylar::getThreadId(),          \
          sylar::getFiberId(), time(0), sylar::Thread::GetName())))            \
      .getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

#define SYLAR_LOG_FORMAT_LEVEL(logger, level, fmt, ...)                        \
  if (logger->getLevel() <= level)                                             \
  sylar::LogEventWrap(                                                         \
      sylar::LogEvent::ptr(new sylar::LogEvent(                                \
          logger, level, __FILE__, __LINE__, 0, sylar::getThreadId(),          \
          sylar::getFiberId(), time(0), sylar::Thread::GetName())))            \
      .getEvent()                                                              \
      ->format(fmt, __VA_ARGS__)

#define SYLAR_LOG_FORMAT_DEBUG(logger, fmt, ...)                               \
  SYLAR_LOG_FORMAT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FORMAT_INFO(logger, fmt, ...)                               \
  SYLAR_LOG_FORMAT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FORMAT_WARN(logger, fmt, ...)                               \
  SYLAR_LOG_FORMAT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FORMAT_ERROR(logger, fmt, ...)                               \
  SYLAR_LOG_FORMAT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FORMAT_FATAL(logger, fmt, ...)                               \
  SYLAR_LOG_FORMAT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

#define SYLAR_LOG_ROOT() sylar::LoggerMagr::getInstance()->getRoot()
#define SYLAR_LOG_NAME(name) sylar::LoggerMagr::getInstance()->getLogger(name)

namespace sylar{
class Logger;
class LogFormatter;

//日志等级
class LogLevel{
public:
  enum Level{
    UNKNOW = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
  };
  static const char *ToString(LogLevel::Level level);
  static LogLevel::Level FromString(const std::string& val);
};

//日志事件
class LogEvent{
public:
  typedef std::shared_ptr<LogEvent> ptr;
  LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
           const char *file, uint32_t line, uint32_t elapse, uint32_t threadId,
           uint32_t fiberId, uint64_t time, const std::string &threadName);

  const char* getFile() const {return m_file;}
  uint32_t getLine() const {return m_line;}
  uint32_t getElapse() const {return m_elapse;}
  uint32_t getThreadId() const {return m_threadId;}
  uint32_t getFiberId() const {return m_fiberId;}
  uint32_t getTime() const {return m_time;}
  std::string getContent() { return m_ss.str(); }
  const std::string &getThreadName() const { return m_threadName; }

  std::stringstream &getSS() { return m_ss; }
  std::shared_ptr<Logger> getLogger() const { return m_logger; }
  LogLevel::Level getLevel() const { return m_level; }

  void format(const char *fmt, ...);
  void format(const char *fmt, va_list al);
private:
  const char* m_file = nullptr;   //文件名
  uint32_t m_line = 0;            //行号
  uint32_t m_elapse = 0;          //程序启动到现在的开始时间
  uint32_t m_threadId = 0;        //线程id
  uint32_t m_fiberId = 0;         //协程id
  uint64_t m_time = 0;            // 时间戳
  std::string m_threadName;
  std::stringstream m_ss;         // 内容

  std::shared_ptr<Logger> m_logger;
  LogLevel::Level m_level;
};

// 通过wrap类输出内容
class LogEventWrap {
public:
  LogEventWrap(const LogEvent::ptr& event);
  ~LogEventWrap();
  std::stringstream &getSS();
  const LogEvent::ptr getEvent() const {return m_event;}
private:
  LogEvent::ptr m_event;
};

// 日志格式解析成为相应的item内容存储
class LogFormatter{
public:
  typedef std::shared_ptr<LogFormatter> ptr;     
  LogFormatter(const std::string& pattern);

  std::string format(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event);
  void init();
  bool isError() { return m_error; }
  
  const std::string getPattern() const {return m_pattern;}
  class FormatItem{
  public:
    typedef std::shared_ptr<FormatItem> ptr;
    FormatItem(const std::string& fmt = ""){};
    virtual ~FormatItem(){};
    //virtual std::string format(LogEvent::ptr event) = 0;
    virtual void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) = 0;
  };
private:
  std::string m_pattern;
  std::vector<FormatItem::ptr> m_items;
  bool m_error = false;
};

//输出地
class LogAppender {
  friend class Logger;
public:
  typedef std::shared_ptr<LogAppender> ptr;
  typedef Spinlock MutexType;

  virtual ~LogAppender(){};
  virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   LogEvent::ptr event) = 0;
  virtual std::string toYamlString() = 0;

  void setFormatter(LogFormatter::ptr format);
  LogFormatter::ptr getFormatter() { return m_formatter; }
  LogLevel::Level getLevel() const { return m_level; }
  void setLevel(LogLevel::Level level) { m_level = level; }
  
protected:
  LogLevel::Level m_level = LogLevel::DEBUG;
  LogFormatter::ptr m_formatter;
  bool has_formatter = false;
  MutexType m_mutex;
};

class StdoutLogAppender : public LogAppender{
public:
  typedef std::shared_ptr<StdoutLogAppender> ptr;
  std::string toYamlString() override;
  void log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override;
};

class FileAppender : public LogAppender{
public:
  typedef std::shared_ptr<FileAppender> ptr;
  FileAppender(const std::string &filename);
  void log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override;
  bool reopen();
  std::string toYamlString() override;
private:
  std::string m_filename;
  std::ofstream m_filestream;
  uint64_t m_lastTime;
};

//日志器
//包括输出者、输出等级和输出目的地
class Logger : public std::enable_shared_from_this<Logger>{
  friend class LoggerManager;
  public:
  typedef std::shared_ptr<Logger> ptr;
  typedef Spinlock MutexType;

  Logger(const std::string& name = "root");

  void log(LogLevel::Level level, LogEvent::ptr event);
  void debug(LogEvent::ptr event);
  void info(LogEvent::ptr event);
  void warn(LogEvent::ptr event);
  void error(LogEvent::ptr event);
  void fatal(LogEvent::ptr event);

  void addAppender(LogAppender::ptr appender);
  void delAppender(LogAppender::ptr appender);
  void clearAppenders();
  
  LogLevel::Level getLevel() const {return m_level;}
  void setLevel(LogLevel::Level val) {m_level = val;}

  const std::string getName() const {return m_name;}
  void setName(const std::string &val) { m_name = val; }

  const LogFormatter::ptr getFormatter() const { return m_formatter; }
  void setFormatter(LogFormatter::ptr val);
  void setFormatter(const std::string &val);

  std::string toYamlString();
private:
  std::string m_name;
  LogLevel::Level m_level;
  std::list<LogAppender::ptr> m_appenders;
  LogFormatter::ptr m_formatter;
  Logger::ptr m_root;
  MutexType m_mutex;
};

// 日志管理器
class LoggerManager {
public:
  typedef Spinlock MutexType;
  LoggerManager();
  void init();

  Logger::ptr getLogger(const std::string &name);
  Logger::ptr getRoot() const { return m_root; }

  std::string toYamlString();
private:
  std::map<std::string, Logger::ptr> m_loggers;
  Logger::ptr m_root;
  MutexType m_mutex;
};

// 全局唯一日志管理器
typedef sylar::Singleton<LoggerManager> LoggerMagr;
}

#endif // !DEBUG__SYLAR_LOG_H__

