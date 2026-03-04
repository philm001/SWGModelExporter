#include "stdafx.h"
#include "Logger.h"
#include <filesystem>

Logger& Logger::getInstance()
{
    static Logger instance;
    return instance;
}

void Logger::initialize(const std::string& logFilePath, bool enableConsole)
{
    // If already initialized, cleanly shut down first (avoid re-entrant locking)
    if (m_initialized)
    {
        shutdown();
    }

    // Phase 1: configure under lock
    {
        std::lock_guard<std::mutex> lock(m_logMutex);

        m_enableConsole = enableConsole;

        // Delete existing log file if it exists (non-throwing)
        {
            std::error_code ec;
            if (std::filesystem::exists(logFilePath, ec))
            {
                std::filesystem::remove(logFilePath, ec);
            }
        }
    
        // Create directory if it doesn't exist (non-throwing)
        std::filesystem::path logPath(logFilePath);
        if (logPath.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(logPath.parent_path(), ec);
        }

        // Open log file
        m_logFile.open(logFilePath, std::ios::out | std::ios::trunc);
        if (!m_logFile.is_open())
        {
            throw std::runtime_error("Failed to open log file: " + logFilePath);
        }

        // Write initial header
        m_logFile << "================================================================================\n";
        m_logFile << "SWG Model Exporter Debug Log\n";
        m_logFile << "Session started: " << getCurrentTimestamp() << "\n";
        m_logFile << "================================================================================\n\n";
        m_logFile.flush();

        // Print absolute log file path to console before redirecting std::cout
        try
        {
            auto absPath = std::filesystem::absolute(logFilePath);
            std::cout << "[Logger] Log file: " << absPath.string() << std::endl;
        }
        catch (...) { /* ignore console errors */ }

        // Store original cout buffer and attach custom buffer
        m_originalCoutBuffer = std::cout.rdbuf();
        m_logBuffer = std::make_unique<LogStreamBuffer>(this);
        std::cout.rdbuf(m_logBuffer.get());

        m_initialized = true;
    }

    // Phase 2: emit first message without holding the lock to avoid self-deadlock
    log("Logger initialized successfully. Output will be saved to: " + logFilePath);
}

void Logger::log(const std::string& message)
{
    std::lock_guard<std::mutex> lock(m_logMutex);
    
    if (!m_initialized)
        return;

    std::string timestampedMessage = getCurrentTimestamp() + " | " + message;
    
    // Write to file
    if (m_logFile.is_open())
    {
        m_logFile << timestampedMessage << std::endl;
        m_logFile.flush();
    }

    // Write to console if enabled (non-throwing)
    if (m_enableConsole && m_originalCoutBuffer)
    {
        try
        {
            m_originalCoutBuffer->sputn(timestampedMessage.c_str(), static_cast<std::streamsize>(timestampedMessage.length()));
            m_originalCoutBuffer->sputn("\n", 1);
            m_originalCoutBuffer->pubsync();
        }
        catch (...)
        {
            // swallow console errors to keep logging robust
        }
    }
}

void Logger::logWithPrefix(const std::string& prefix, const std::string& message)
{
    log(prefix + " " + message);
}

void Logger::flush()
{
    std::lock_guard<std::mutex> lock(m_logMutex);
    
    if (m_logFile.is_open())
    {
        m_logFile.flush();
    }
    
    if (m_originalCoutBuffer)
    {
        try { m_originalCoutBuffer->pubsync(); }
        catch (...) { /* ignore */ }
    }
}

void Logger::shutdown()
{
    std::unique_lock<std::mutex> lock(m_logMutex);
    
    if (!m_initialized)
        return;

    // Flush any buffered output from our custom streambuf outside the logger lock
    lock.unlock();
    if (m_logBuffer)
    {
        m_logBuffer->pubsync();
    }
    lock.lock();

    // Restore original cout buffer
    if (m_originalCoutBuffer)
    {
        std::cout.rdbuf(m_originalCoutBuffer);
        m_originalCoutBuffer = nullptr;
    }

    // Write session end
    if (m_logFile.is_open())
    {
        m_logFile << "\n================================================================================\n";
        m_logFile << "Session ended: " << getCurrentTimestamp() << "\n";
        m_logFile << "================================================================================\n";
        m_logFile.flush();
        m_logFile.close();
    }

    // Clean up
    m_logBuffer.reset();
    m_initialized = false;
}

std::string Logger::getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm local_tm;
#ifdef _WIN32
    localtime_s(&local_tm, &time_t);
#else
    localtime_r(&time_t, &local_tm);
#endif

    std::stringstream ss;
    ss << std::put_time(&local_tm, "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

Logger::~Logger()
{
    shutdown();
}

// LogStreamBuffer implementation
int Logger::LogStreamBuffer::overflow(int c)
{
    if (c != EOF)
    {
        std::lock_guard<std::mutex> g(m_bufMutex);
        m_buffer += static_cast<char>(c);

        // If we hit a newline, flush the buffer
        if (c == '\n')
        {
            if (!m_buffer.empty() && m_buffer.back() == '\n')
            {
                m_buffer.pop_back(); // Remove the newline, log() will add it back
            }

            if (!m_buffer.empty())
            {
                m_logger->log(m_buffer);
                m_buffer.clear();
            }
        }
    }
    return c;
}

std::streamsize Logger::LogStreamBuffer::xsputn(const char* s, std::streamsize count)
{
    std::lock_guard<std::mutex> g(m_bufMutex);

    // Append incoming data
    m_buffer.append(s, static_cast<size_t>(count));

    // Flush complete lines
    size_t pos = 0;
    while (true)
    {
        size_t newline_pos = m_buffer.find('\n', pos);
        if (newline_pos == std::string::npos)
            break;

        // Extract a single line without newline and log it
        if (newline_pos > 0)
        {
            std::string line = m_buffer.substr(0, newline_pos);
            m_logger->log(line);
        }
        else
        {
            // Empty line
            m_logger->log(std::string());
        }

        // Erase processed part including newline
        m_buffer.erase(0, newline_pos + 1);
        pos = 0;
    }

    return count;
}

int Logger::LogStreamBuffer::sync()
{
    std::lock_guard<std::mutex> g(m_bufMutex);

    // Flush any remaining buffered content as a line
    if (!m_buffer.empty())
    {
        m_logger->log(m_buffer);
        m_buffer.clear();
    }
    m_logger->flush();
    return 0; // success
}