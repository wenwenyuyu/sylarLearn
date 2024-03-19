#include "log.h"
#include <cctype>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <map>
#include <functional>
#include <algorithm>
#include <utility>
#include <vector>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include "config.h"
#include "sylar/mutex.h"

namespace sylar{
    /******************logEvent start********************/
    /**
     * @func: LogEvent
     * @return {*}
     * @description: 日志事件初始化
     */
LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   const char *file, uint32_t line, uint32_t elapse,
                   uint32_t threadId, uint32_t fiberId, uint64_t time,
                   const std::string &threadName)
    : m_file(file), m_line(line), m_elapse(elapse), m_threadId(threadId),
      m_fiberId(fiberId), m_time(time), m_threadName(threadName),
      m_logger(logger), m_level(level) {}

/**
 * @func: 
 * @param {char} *fmt
 * @return {*}
 * @description: 根据format输出相应的信息
 */
void LogEvent::format(const char *fmt, ...) {
  va_list al;
  va_start(al, fmt);
  format(fmt, al);
  va_end(al);
}

void LogEvent::format(const char *fmt, va_list al) {
  char *buf = nullptr;
  int len = vasprintf(&buf, fmt, al);
  if (len != -1) {
    m_ss << std::string(buf, len);
    free(buf);
  }
}

LogEventWrap::LogEventWrap(const LogEvent::ptr& event) : m_event(event) {}
/**
 * @func: 
 * @return {*}
 * @description: 析构时进行日志的打印
 */
LogEventWrap::~LogEventWrap() {
  m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream &LogEventWrap::getSS() { return m_event->getSS(); }

    /******************LogLevel start********************/
    const char *LogLevel::ToString(LogLevel::Level level){
        switch(level){
    #define XX(name) \
        case LogLevel::name:\
            return #name;   \
            break;
            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);
    #undef XX
            default:
                return "NUKNOW";
        }
        return "UNKNOW";
    }

    LogLevel::Level LogLevel::FromString(const std::string &val) {
#define XX(name, va)                                                           \
      if (val == #name) {                                                      \
        return LogLevel::va;                                                 \
      }                                                                        
        XX(DEBUG, DEBUG);
        XX(INFO, INFO);
        XX(WARN, WARN);
        XX(ERROR, ERROR);
        XX(FATAL, FATAL);
        XX(debug, DEBUG);
        XX(info, INFO);
        XX(warn, WARN);
        XX(error, ERROR);
        XX(fatal, FATAL);
        return LogLevel::UNKNOW;
    #undef XX
    }
    
/******************Format start********************/
    class MessageFormatItem : public LogFormatter::FormatItem{
    public:
        MessageFormatItem(const std::string& str = ""){}
        void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem{
    public:
        LevelFormatItem(const std::string& str = ""){}
        void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << LogLevel::ToString(level);
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem{
    public:
        ElapseFormatItem(const std::string& str = ""){}
        void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getElapse();
        }
    };

    class NameFormatItem : public LogFormatter::FormatItem{
    public:
        NameFormatItem(const std::string& str = ""){}
        void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getLogger()->getName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem{
    public:
        ThreadIdFormatItem(const std::string& str = ""){}
        void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getThreadId();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem{
    public:
        FiberIdFormatItem(const std::string& str = ""){}
        void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getFiberId();
        }
    };

    class DateTimeFormatItem : public LogFormatter::FormatItem{
    public:
      DateTimeFormatItem(const std::string &format = "%Y:%m:%d %H:%m:%S")
          : m_format(format) {
            if(m_format.empty()) {
                m_format = "%Y:%m:%d %H:%m:%S";
            }
        }

      void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override {
            struct tm tm;
            time_t time = event->getTime();
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            os << buf;
        }
    private:
        std::string m_format;
    };

    class FileNameFormatItem : public LogFormatter::FormatItem{
    public:
        FileNameFormatItem(const std::string& str = ""){}
        void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getFile();
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem{
    public:
        LineFormatItem(const std::string& str = ""){}
        void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getLine();
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem{
    public:
        NewLineFormatItem(const std::string& str = ""){}
        void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << std::endl;
        }
    };

    class StringFormatItem : public LogFormatter::FormatItem{
    public:
        StringFormatItem(const std::string& str = "")
            :FormatItem(str), m_string(str) {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << m_string;
        }
    private:
        std::string m_string;
    };

    class TabFormatItem : public LogFormatter::FormatItem{
    public:
        TabFormatItem(const std::string& str = ""){}
        void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << "\t";
        }
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem{
    public:
        ThreadNameFormatItem(const std::string& str = ""){}
        void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getThreadName() << "    ";
        }
    };
    /**
     * @func: LogFormatter
     * @param {string&} pattern
     * @return {*}
     * @description: 通过pattern的日志格式，解析分为不同的item
     */
    LogFormatter::LogFormatter(const std::string& pattern)
        :m_pattern(pattern){
        init();
    }
    /**
     * @func: format
     * @param {ptr} event
     * @return {string}
     * @description: 将事件通过不同的item解析成字符串
     */
    std::string LogFormatter::format(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event){
        std::stringstream ss;
        for(auto& i : m_items){
            i->format(ss, logger, level, event);
        }
        return ss.str();
    }
    /**
     * @func: init
     * @return {*}
     * @description: 根据输入的pattern（[%d{HH\:mm\:ss\:SSS}][%p] (%c\:%L) -
%m%n）解析成相应的item
     *  %d{%Y-%m-%d %H:%M:%S} [%p] %f %l %m %n
        (d) - (%Y-%m-%d %H:%M:%S) - (1)
        ( [) - () - (0)
        (p) - () - (1)
        (] ) - () - (0)
        (f) - () - (1)
        ( ) - () - (0)
        (l) - () - (1)
        ( ) - () - (0)
        (m) - () - (1)
        ( ) - () - (0)
        (n) - () - (1)
        接着通过枚举tuple内容，填充m_items内容
     */
    void LogFormatter::init(){
        //格式有以下几种 %xxx %xxx{} %%
        //init函数主要解析上述格式，将其放入一个<str, fmt, status>的tuple中

        std::vector<std::tuple<std::string, std::string, int> >vec;
        std::string raw;
        for (size_t i = 0; i < m_pattern.size(); ++i) {
            if (m_pattern[i] != '%') {
                raw.append(1, m_pattern[i]);
                continue;
            }

            //m_pattern[i] == '%'
            if ((i + 1) < m_pattern.size() && m_pattern[i + 1] == '%') {
                raw.append(1, '%');
                continue;
            }

            //m_pattern[i] == '%'
            size_t n = i + 1;
            std::string str, fmt;
            int fmt_status = 0;
            std::size_t fmt_begin = 0;

            while (n < m_pattern.size()) {
                //%d
                if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                    && m_pattern[n] != '}')) {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    break;
                }

                if (fmt_status == 0) {
                    if (m_pattern[n] == '{') {
                        str = m_pattern.substr(i + 1, n - i - 1);
                        fmt_begin = n;
                        fmt_status = 1;
                        ++n;
                        continue;
                    }
                } else if (fmt_status == 1) {
                    if (m_pattern[n] == '}') {
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        fmt_status = 0;
                        ++n;
                        break;
                    }
                }
                ++n;
                if(n == m_pattern.size()) {
                    if(str.empty()) {
                        str = m_pattern.substr(i + 1);
                    }
                }
            }
            
            if (!raw.empty()) {
              vec.push_back(std::make_tuple(std::move(raw), std::string(), 0));
              i = n - 1;
            }

            if (fmt_status == 0) {
              vec.push_back(std::make_tuple(std::move(str), std::move(fmt), 1));
              i = n - 1;
            } else if(fmt_status == 1) {
              std::cout << "pattern parse error: " << m_pattern << " - "
                        << m_pattern.substr(i) << std::endl;
              m_error = true;
                vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
            }

        }

        static std::map<std::string,
                        std::function<FormatItem::ptr(const std::string &str)>>
            s_format_items = {
#define XX(str, C)                                                         \
        {                                                                      \
          #str, [](const std::string &fmt) {                                   \
            return FormatItem::ptr(new C(fmt));                                \
          }                                                                    \
        }
              XX(m, MessageFormatItem),
              XX(p, LevelFormatItem),
              XX(r, ElapseFormatItem),
              XX(n, NewLineFormatItem),
              XX(c, NameFormatItem),
              XX(t, ThreadIdFormatItem),
              XX(d, DateTimeFormatItem),
              XX(f, FileNameFormatItem),
              XX(l, LineFormatItem),
              XX(F, FiberIdFormatItem),
              XX(T, TabFormatItem),
              XX(N, ThreadNameFormatItem)
        #undef XX
        };

        for(auto& i : vec) {
            if(std::get<2>(i) == 0) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            } else {
                auto it = s_format_items.find(std::get<0>(i));
                if(it == s_format_items.end()) {
                  m_items.push_back(FormatItem::ptr(new StringFormatItem(
                      "<<error_format %" + std::get<0>(i) + ">>")));
                  m_error = true;
                } else {
                    m_items.push_back(it->second(std::get<1>(i)));
                }
                
            }
            //std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
        }
    //std::cout << m_items.size() << std::endl;
    }
    
    /******************Logger start********************/
    Logger::Logger(const std::string &name)
        :m_name(name)
        ,m_level(LogLevel::DEBUG){
      m_formatter.reset(new LogFormatter(
          "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%F%T[%p]%T[%c]%T[%f:%l]%T%m%n"));
    }

    void Logger::log(LogLevel::Level level, LogEvent::ptr event){
        if(level >= m_level){
          auto self = shared_from_this();
          MutexType::Lock lock(m_mutex);
          if (!m_appenders.empty()) {
            for(auto &i : m_appenders){
                i->log(self, level, event);
            }
          } else if (m_root) {
            m_root->log(level, event);
          }

        }
    }

    void Logger::debug(LogEvent::ptr event){
        log(LogLevel::DEBUG, event);
    }

    void Logger::info(LogEvent::ptr event){
        log(LogLevel::INFO, event);
    }

    void Logger::warn(LogEvent::ptr event){
        log(LogLevel::WARN, event);
    }

    void Logger::error(LogEvent::ptr event){
        log(LogLevel::ERROR, event);
    }

    void Logger::fatal(LogEvent::ptr event){
        log(LogLevel::FATAL, event);
    }

    void Logger::addAppender(LogAppender::ptr appender) {
      MutexType::Lock lock(m_mutex);
      if (!appender->getFormatter()) {
        MutexType::Lock l(appender->m_mutex);
            appender->m_formatter = m_formatter;
        }
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender) {
      MutexType::Lock lock(m_mutex);
        auto it = find(m_appenders.begin(), m_appenders.end(), appender);
        if(it != m_appenders.end()){
            it = m_appenders.erase(it);
        }
    }

    void Logger::clearAppenders() {
      MutexType::Lock lock(m_mutex);
      m_appenders.clear();
    }

    void Logger::setFormatter(LogFormatter::ptr val) {
      MutexType::Lock lock(m_mutex);
      m_formatter = val;

      for (auto &i : m_appenders) {
        MutexType::Lock l(i->m_mutex);
        if (!i->has_formatter) {
          i->m_formatter = m_formatter;
        }
      }
    }

    void Logger::setFormatter(const std::string &val) {
      MutexType::Lock lock(m_mutex);
      LogFormatter::ptr new_val(new LogFormatter(val));
      if (new_val->isError()) {
        std::cout << "Logger Formatter name=" << m_name << " value=" << val
                  << " invalid formatter" << std::endl;
        return;
      }
      m_formatter = new_val;
    }

    std::string Logger::toYamlString() {
      MutexType::Lock lock(m_mutex);
      YAML::Node node;
      node["name"] = m_name;

      if (m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
      }

      if (m_formatter) {
        node["formatter"] = m_formatter->getPattern();
      }

      for (auto &appender : m_appenders) {
        node["appenders"].push_back(YAML::Load(appender->toYamlString()));
      }

      std::stringstream ss;
      ss << node;
      return ss.str();
    }


    /******************FileAppender start********************/
    void LogAppender::setFormatter(LogFormatter::ptr format) {
      if (format) {
        m_formatter = format;
        has_formatter = true;
      }
    }

    FileAppender::FileAppender(const std::string &filename){
      m_filename = filename;
      reopen();
    }

    void FileAppender::log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event){
      if (m_level <= level) {
        // 防止在长时间写入文件的过程中，文件被删除，隔段时间就reopen一次
        uint64_t now = event->getTime();
        if(now >= (m_lastTime + 3)) {
            reopen();
            m_lastTime = now;
        }
        m_filestream << m_formatter->format(logger,level, event);
      }
    }

    bool FileAppender::reopen() {
      MutexType::Lock lock(m_mutex);
        if(m_filestream){
            m_filestream.close();
        }
        m_filestream.open(m_filename);
        return !!m_filestream;
    }

    std::string FileAppender::toYamlString() {
      MutexType::Lock lock(m_mutex);
      YAML::Node node;
      node["type"] = "FileLogAppender";
      node["path"] = m_filename;

      if (m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
      }

      if (has_formatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
      }

      std::stringstream ss;
      ss << node;
      return ss.str();
    }

    /******************StdoutLogAppender start********************/
    void StdoutLogAppender::log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event){
      if (m_level <= level) {
        MutexType::Lock lock(m_mutex);
        std::cout << m_formatter->format( logger, level, event);
      }
    }

    std::string StdoutLogAppender::toYamlString() {
      MutexType::Lock lock(m_mutex);
      YAML::Node node;
      node["type"] = "StdoutLogAppender";

      if (m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
      }

      if (has_formatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
      }
      std::stringstream ss;
      ss << node;
      return ss.str();
    }

    /******************Yaml start********************/
    struct LogAppenderDefine {
      int type = 0;
      LogLevel::Level level = LogLevel::UNKNOW;
      std::string formatter;
      std::string file;

      bool operator==(const LogAppenderDefine &oth) const {
        return type == oth.type && level == oth.level &&
               formatter == oth.formatter && file == oth.file;
      }
    };

    struct LogDefine {
      std::string name;
      LogLevel::Level level = LogLevel::UNKNOW;
      std::string formatter;
      std::vector<struct LogAppenderDefine> appenders;

      bool operator==(const LogDefine &oth) const {
        return name == oth.name && level == oth.level &&
               formatter == oth.formatter && appenders == oth.appenders;
      }
      /**
       * @func: 
       * @return {*}
       * @description: set元素要operator<操作符
       */
      bool operator<(const LogDefine &oth) const {
        return name < oth.name;
      }
    };

    template <>
    class LexicalCast<std::string, LogDefine> {
    public:
    LogDefine operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        LogDefine ld;
        if (!node["name"].IsDefined()) {
          SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "cannot find config log name";
          throw std::logic_error("log config name is null");
        }
        ld.name = node["name"].as<std::string>();
        ld.level = LogLevel::FromString(
            (node["level"].IsDefined() ? node["level"].as<std::string>() : ""));
        if (node["formatter"].IsDefined()) {
          ld.formatter = node["formatter"].as<std::string>();
        }

        if (node["appenders"].IsDefined()) {
          for (std::size_t i = 0; i < node["appenders"].size(); i++) {
            LogAppenderDefine lad;
            auto tmp = node["appenders"][i];
            if (!tmp["type"].IsDefined()) {
              SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                  << "connot find appender type. logs name: "
                  << node["name"].as<std::string>()
                  << " appenders index: " << i;
              continue;
            }

            std::string type = tmp["type"].as<std::string>();
            if (type == "FileLogAppender") {
              lad.type = 1;
              if (!tmp["path"].IsDefined()) {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                    << "connot find appender path. logs name: "
                    << node["name"].as<std::string>()
                    << " appenders index: " << i;
                continue;
              }
              lad.file = tmp["path"].as<std::string>();
              if (tmp["formatter"].IsDefined()) {
                lad.formatter = tmp["formatter"].as<std::string>();
              }
            } else if (type == "StdoutLogAppender") {
              lad.type = 2;
              if (tmp["formatter"].IsDefined()) {
                lad.formatter = tmp["formatter"].as<std::string>();
              }
            } else {
              SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                  << "connot config appender type:" << type;
              continue;
            }
          ld.appenders.push_back(lad);            
          }
        }
        return ld;
    }
    };

    template <>
    class LexicalCast<LogDefine,std::string> {
    public:
      std::string operator()(const LogDefine &v) {
        YAML::Node node;
        node["name"] = v.name;
        node["level"] = LogLevel::ToString(v.level);
        node["formatter"] = v.formatter;
        for (auto &appender : v.appenders) {
          YAML::Node ap;
          if (appender.level == 1) {
            ap["type"] = "FileLogAppender";
            ap["path"] = appender.file;
          } else if (appender.type == 2) {
            ap["type"] = "StdoutLogAppender";
          }

          if (appender.level != LogLevel::UNKNOW) {
            ap["level"] = LogLevel::ToString(appender.level);
          }

          if (!appender.formatter.empty()) {
            ap["formatter"] = appender.formatter;
          }

          node["appenders"].push_back(ap);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
      }
    };

    ConfigVar<std::set<LogDefine>>::ptr g_log_define =
        Config::Lookup("logs", std::set<LogDefine>(), "logs config");

    struct LogIniter {

      /**
       * @func: LogIniter
       * @return {*}
       * @description: 静态类初始化。将logs配置设置监听器，确保在全局配置发生改变时修改相应的全局log指针
       */
      LogIniter() {
        g_log_define->addListener([](const std::set<LogDefine> &old_value,
                                        const std::set<LogDefine> &new_value) {
          for (auto &i : new_value) {
            auto it = old_value.find(i);
            Logger::ptr logger;
            if (it == old_value.end()) {
              // 新增
              logger = SYLAR_LOG_NAME(i.name);
            } else {
              if (!(i == *it)) {
                // 修改
                logger = SYLAR_LOG_NAME(i.name);
              }
            }
            logger->setLevel(i.level);
            if (!i.formatter.empty()) {
              logger->setFormatter(i.formatter);
            }
            logger->clearAppenders();
            for (auto &a : i.appenders) {
              LogAppender::ptr ap;
              if (a.type == 1) {
                ap.reset(new FileAppender(a.file));
              } else if (a.type == 2) {
                ap.reset(new StdoutLogAppender);
              }
              ap->setLevel(a.level);

              if (!a.formatter.empty()) {
                LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                if (!fmt->isError()) {
                  ap->setFormatter(fmt);
                } else {
                  std::cout << "log.name=" << i.name
                            << " appender type=" << a.type
                            << " formatter=" << a.formatter << " is invalid"
                            << std::endl;
                }
              }
              logger->addAppender(ap);
            }
          }
          //删除
          for (auto &i: old_value) {
            auto it = new_value.find(i);
            if (it == new_value.end()) {
              auto logger = SYLAR_LOG_NAME(i.name);
              logger->setLevel((LogLevel::Level)100);
              logger->clearAppenders();
            }
          }
        });
      }
    };

    static LogIniter __log_init;

    /******************LoggerManager start********************/
    LoggerManager::LoggerManager() {
      m_root.reset(new Logger);
      m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
      m_loggers["root"] = m_root;
      init();
    }

    Logger::ptr LoggerManager::getLogger(const std::string &name) {
      MutexType::Lock lock(m_mutex);
      auto it = m_loggers.find(name);
      if (it != m_loggers.end())
        return it->second;
      Logger::ptr logger(new Logger(name));
      logger->m_root = m_root;
      m_loggers[name] = logger;
      return logger;
    }

    std::string LoggerManager::toYamlString() {
      MutexType::Lock lock(m_mutex);
      YAML::Node node;
      for (auto &logger : m_loggers) {
        node.push_back(YAML::Load(logger.second->toYamlString()));
      }
      std::stringstream ss;
      ss << node;
      return ss.str();
    }

    void LoggerManager::init() {
      
    }
    
} 