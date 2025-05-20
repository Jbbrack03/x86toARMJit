#include "logger.h"
#include <chrono>
#include <ctime>
#include <iomanip>

namespace XenoARM_JIT {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : currentLogLevel(INFO) {
    // Constructor
}

Logger::~Logger() {
    // Destructor
}

void Logger::setLogLevel(LogLevel level) {
    currentLogLevel = level;
}

LogLevel Logger::getLogLevel() const {
    return currentLogLevel;
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level >= currentLogLevel) {
        std::lock_guard<std::mutex> lock(logMutex);

        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::cout << std::put_time(std::localtime(&time_t_now), "[%Y-%m-%d %H:%M:%S")
                  << '.' << std::setfill('0') << std::setw(3) << ms.count() << "] ";

        switch (level) {
            case DEBUG:   std::cout << "[DEBUG] "; break;
            case INFO:    std::cout << "[INFO] ";  break;
            case WARNING: std::cout << "[WARN] ";  break;
            case ERROR:   std::cout << "[ERROR] "; break;
            case FATAL:   std::cout << "[FATAL] "; break;
        }

        std::cout << message << std::endl;
    }
}

} // namespace XenoARM_JIT