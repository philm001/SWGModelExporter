#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <sstream>
#include <mutex>
#include <chrono>
#include <iomanip>

/// <summary>
/// Singleton logger class that redirects std::cout and other debug outputs to both console and log file
/// Automatically creates a new log file each time the program runs, deleting the previous one
/// </summary>
class Logger
{
public:
    /// <summary>
    /// Get the singleton instance of the logger
    /// </summary>
    static Logger& getInstance();

    /// <summary>
    /// Initialize the logger with the specified log file path
    /// Deletes any existing log file and creates a new one
    /// </summary>
    /// <param name="logFilePath">Path to the log file (e.g., "debug_output.log")</param>
    /// <param name="enableConsole">Whether to also output to console (default: true)</param>
    void initialize(const std::string& logFilePath, bool enableConsole = true);

    /// <summary>
    /// Log a message with timestamp
    /// </summary>
    /// <param name="message">Message to log</param>
    void log(const std::string& message);

    /// <summary>
    /// Log a message with custom prefix (e.g., "[ERROR]", "[BONE]", etc.)
    /// </summary>
    /// <param name="prefix">Prefix to add to the message</param>
    /// <param name="message">Message to log</param>
    void logWithPrefix(const std::string& prefix, const std::string& message);

    /// <summary>
    /// Flush the log file to ensure all data is written
    /// </summary>
    void flush();

    /// <summary>
    /// Close the logger and restore original std::cout
    /// </summary>
    void shutdown();

    /// <summary>
    /// Custom stream buffer that redirects to both file and console
    /// </summary>
    class LogStreamBuffer : public std::streambuf
    {
    public:
        LogStreamBuffer(Logger* logger) : m_logger(logger) {}

    protected:
        virtual int overflow(int c) override;
        virtual std::streamsize xsputn(const char* s, std::streamsize count) override;
        virtual int sync() override;

    private:
        Logger* m_logger;
        std::string m_buffer;
        std::mutex m_bufMutex;
    };

    /// <summary>
    /// Destructor - automatically shuts down the logger
    /// </summary>
    ~Logger();

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream m_logFile;
    std::unique_ptr<LogStreamBuffer> m_logBuffer;
    std::streambuf* m_originalCoutBuffer = nullptr;
    bool m_enableConsole = true;
    bool m_initialized = false;
    std::mutex m_logMutex;

    /// <summary>
    /// Get current timestamp as string
    /// </summary>
    std::string getCurrentTimestamp();
};

/// <summary>
/// Convenience macros for logging
/// </summary>
#define LOG(message) \
    do { \
        std::ostringstream oss; \
        oss << message; \
        Logger::getInstance().log(oss.str()); \
    } while(0)

#define LOG_PREFIX(prefix, message) \
    do { \
        std::ostringstream oss; \
        oss << message; \
        Logger::getInstance().logWithPrefix(prefix, oss.str()); \
    } while(0)

#define LOG_BONE(message) LOG_PREFIX("[BONE]", message)
#define LOG_SKELETON(message) LOG_PREFIX("[SKELETON]", message)  
#define LOG_ANIMATION(message) LOG_PREFIX("[ANIMATION]", message)
#define LOG_FBX(message) LOG_PREFIX("[FBX]", message)
#define LOG_ERROR(message) LOG_PREFIX("[ERROR]", message)
#define LOG_WARNING(message) LOG_PREFIX("[WARNING]", message)
#define LOG_INFO(message) LOG_PREFIX("[INFO]", message)

/// <summary>
/// RAII helper class to automatically initialize and shutdown logger
/// </summary>
class LoggerInitializer
{
public:
    LoggerInitializer(const std::string& logFilePath, bool enableConsole = true)
    {
        Logger::getInstance().initialize(logFilePath, enableConsole);
    }
    
    ~LoggerInitializer()
    {
        Logger::getInstance().shutdown();
    }
};

#define INIT_LOGGER(logFile) LoggerInitializer _logger_init(logFile)