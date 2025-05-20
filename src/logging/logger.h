#ifndef XENOARM_JIT_LOGGER_H
#define XENOARM_JIT_LOGGER_H

#include <iostream>
#include <string>
#include <mutex>

namespace XenoARM_JIT {

enum LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& getInstance();

    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;

    void log(LogLevel level, const std::string& message);

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel currentLogLevel;
    std::mutex logMutex;
};

} // namespace XenoARM_JIT

// Define macros in the global namespace
#define LOG_DEBUG(msg) XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::DEBUG, msg)
#define LOG_INFO(msg) XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::INFO, msg)
#define LOG_WARNING(msg) XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::WARNING, msg)
#define LOG_ERROR(msg) XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::ERROR, msg)
#define LOG_FATAL(msg) XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::FATAL, msg)

#endif // XENOARM_JIT_LOGGER_H