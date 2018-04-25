#pragma once

#include <iostream>
#include <string>
#include <sys/stat.h> // start(), createDir()
#include <time.h>     // dateTimeString()
#include <fstream>    // Log recording

#ifdef _WIN32
    #include <direct.h>  // createDir()
    #include <Windows.h> // colors
#endif // _WIN32

#ifndef S_ISDIR
    #define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif // S_ISDIR

namespace logger {

    namespace data {
        // OS specific variables
        #ifdef _WIN32
            const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            WORD defaultColorAttribs;
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            
            // Remember original attributes
            void setConsoleInfo() {
                GetConsoleScreenBufferInfo(hOut, &csbi);
            
                if (!csbi.dwMaximumWindowSize.X && !csbi.dwMaximumWindowSize.Y)
                    defaultColorAttribs = csbi.wAttributes;
            }
        #else
            const char *lastFgColor = "39";
            const char *lastConsoleColor = "49";
            const char *fc[16] = { "30","34","32","36","31","35","33","37", "90", "94", "92", "96", "91", "95", "93", "97" };
            const char *bc[16] = { "40","44","42","46","41","45","43","47","100","104","102","106","101","105","103","107" };
        #endif // _WIN32

        // Log-level Properties
        struct LogLevel {
            uint8_t     fgColor;
            uint8_t     bgColor;
            bool     enumerable;
            bool       writable;
            std::string  suffix;
            std::string  prefix;
            uint8_t    severity;
        };

        static inline std::ofstream file; // Log file

        int max_severity      = 5; // Maximum log-level severity for console write
        int max_file_severity = 5; // Maximum log-level severity for file write

    } // namespace data

    // Log file and folder names
    std::string LOG_NAME        = "MainLog";
    std::string LOG_FLDR        = "./logs";
    std::string LOG_BACKUP_FLDR = "./logs/LogBackups";

    // Log-Level properties. Can be accessed externally by logger::LEVEL.property;
    data::LogLevel PRINT = {
        7,    // Text foreground color
        0,    // Text background color
        true, // Whether or not level is affected by 'max severity' checks before being written
        true, // Whether or not level is able to be written to log file
        "\n", // Suffix of log-level added after string
        "",   // Prefix of log-level added before string
        0     // Severity of log-level
    },

    INFO  = { 15, 0, true,  true, "\n", "| [INFO]  ", 1 },

    WARN  = { 14, 0, true,  true, "\n", "| [WARN]  ", 2 },

    ERR   = { 4,  0, true,  true, "\n", "| [ERROR] ", 3 },

    FATAL = { 12, 0, true,  true, "\n", "| [FATAL] ", 4 },

    DEBUG = { 10, 0, false, true, "\n", "| [DEBUG] ", 5 };

    // logger::createDir("dirName");
    // Creates a directory, if it does not already exist
    void createDir(const char *name) {
        struct stat info;

        if (stat(name, &info) != 0 || !S_ISDIR(info.st_mode)) {
            #ifdef _WIN32
                _mkdir(name);
            #else
                mkdir(name, 0733);
            #endif // _WIN32
        }
    }

    // logger::dateTimeString((bool)returnTimeOnly);
    // Returns a string of the current date/time
    std::string dateTimeString(const bool &returnTimeOnly = false) {
        char      buf[80];
        time_t    now = time(0);
        struct tm tstruct = *localtime(&now);

        if (returnTimeOnly == true)
            strftime(buf, sizeof(buf), "%H;%M;%S %p", &tstruct);
        else if (returnTimeOnly == false)
            strftime(buf, sizeof(buf), "%Y-%m-%d %H;%M;%S %p", &tstruct);
        return buf;
    }

    void resetColors() {
        #ifdef _WIN32
            data::setConsoleInfo();
            SetConsoleTextAttribute(data::hOut, data::defaultColorAttribs);
        #else
            std::cout << "\e[0m\n";
            data::lastFgColor = "39";
            data::lastConsoleColor = "49";
        #endif // _WIN32
    }

    // logger::clearConsole();
    // Clears console output
    void clearConsole() {
        using namespace data;
        #ifdef _WIN32
            setConsoleInfo();
            GetConsoleScreenBufferInfo(hOut, &csbi);
            DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
            DWORD count;
            COORD home = { 0, 0 };

            FillConsoleOutputCharacter(hOut, ' ', cellCount, home, &count);
            FillConsoleOutputAttribute(hOut, csbi.wAttributes, cellCount, home, &count);
            SetConsoleCursorPosition(hOut, home);
        #else
            if (data::lastConsoleColor != "49")
                std::cout << "\e[0;" << data::lastFgColor << ";" << data::lastConsoleColor << "m";
            std::cout << "\e[2J\e[1;1H\n";
        #endif // _WIN32
    }

    // logger::setTextColor((uint8)foregroundColor, (uint8)backgroundColor);
    // Sets the foreground and background color of text.
    // Colors only range from 0-15
    void setTextColor(const uint8_t &foreground, const uint8_t &background = 0) {
        if (background < 0 || foreground < 0 || background > 15 || foreground > 15) {
            std::cerr << "Colors can only be 0-15.\n";
            return;
        }
        #ifdef _WIN32
            data::setConsoleInfo();
            SetConsoleTextAttribute(data::hOut, (WORD)(foreground + (background * 16)));
        #else
            data::lastFgColor = data::fc[foreground];
            std::cout << "\e[0;" << data::lastFgColor << ";" << data::bc[background] << "m";
        #endif // _WIN32
    }

    // logger::setConsoleColor((uint8)color);
    // Sets background color of the entire console.
    // Color only ranges from 0-15
    void setConsoleColor(const uint8_t &color) {
        if (color < 0 || color > 15) {
            std::cerr << "Colors can only be 0-15.\n";
            return;
        }
        #ifdef _WIN32
            data::setConsoleInfo();
            SetConsoleTextAttribute(data::hOut, (WORD)(color << 4));
        #else
            data::lastConsoleColor = data::bc[color];
            std::cout << "\e[0;" << data::lastFgColor << ";" << data::lastConsoleColor << "m";
        #endif // _WIN32
        clearConsole();
    }

    // logger::writeTo(&LEVEL, message, (bool)toLog, (bool)toConsole);
    // Writes message + log-level properties to file or console
    template <typename T>
    void writeTo(const data::LogLevel *lvl, const T &msg, const bool &toLog, const bool &toConsole) {
        std::ios_base::sync_with_stdio(false);

        // Write message to file
        if (toLog == true && data::file.is_open() && lvl->writable && (!lvl->enumerable || lvl->severity < data::max_file_severity)) {
            std::string newPrfx = lvl->prefix + "[" + dateTimeString(true) + "] ";

            try {
                if (data::file.fail()) throw;
                data::file << newPrfx << msg << lvl->suffix;
            }
            catch (...) {
                data::file.close();
                std::cerr << "Error writing to log.\n";
            }
        }
        // Write message to console
        if (toConsole == true && (!lvl->enumerable || lvl->severity < data::max_severity)) {
            // Print colored string
            setTextColor(lvl->fgColor, lvl->bgColor);
            std::cout << lvl->prefix << msg << lvl->suffix;

            // Reset colors
            #ifdef _WIN32
                SetConsoleTextAttribute(data::hOut, data::csbi.wAttributes);
            #else
                std::cout << "\e[0m\n";
            #endif // _WIN32
        }
    }

    // Example: logger::print(message);
    // Outputs message to console and log file
    template <typename T>
    void print(const T &msg) { writeTo(&PRINT, msg, true, true); }

    template <typename T>
    void info(const T &msg) { writeTo(&INFO, msg, true, true); }

    template <typename T>
    void warn(const T &msg) { writeTo(&WARN, msg, true, true); }

    template <typename T>
    void error(const T &msg) { writeTo(&ERR, msg, true, true); }

    template <typename T>
    void fatal(const T &msg) { writeTo(&FATAL, msg, true, true); }

    template <typename T>
    void debug(const T &msg) { writeTo(&DEBUG, msg, true, true); }

    // Example: logger::write(message);
    // Outputs message to log file only
    template <typename T>
    void write(const T &msg) { writeTo(&PRINT, msg, true, false); }

    template <typename T>
    void writeError(const T &msg) { writeTo(&ERR, msg, true, false); }

    template <typename T>
    void writeDebug(const T &msg) { writeTo(&DEBUG, msg, true, false); }

    // Example: logger::setSeverity(int);
    // Sets maximum log-level severity for console/log file output
    void setSeverity(const int &level) { data::max_severity = level; }
    void setFileSeverity(const int &level) { data::max_file_severity = level; }

    // Example: logger::getSeverity();
    // Gets maximum log-level severity for console/log file output
    const int &getSeverity() { return data::max_severity; }
    const int &getFileSeverity() { return data::max_file_severity; }

    // logger::start();
    // Opens log file stream for recording logs.
    // Logging is still possible without calling start(), but logs will not be recorded.
    void start() {
        if (data::file.is_open())
            return;

        try {
            struct stat info;
            const std::string timeStr = dateTimeString();
            const std::string fileName = LOG_FLDR + "/" + LOG_NAME + ".log";
            const std::string backupFileName = LOG_BACKUP_FLDR + "/" + LOG_NAME + "-" + timeStr + ".log";

            createDir(LOG_FLDR.c_str());
            if (stat(fileName.c_str(), &info) == 0) {
                // Backup previous log
                createDir(LOG_BACKUP_FLDR.c_str());
                std::rename(fileName.c_str(), backupFileName.c_str());
            }

            data::file.open(fileName, std::ofstream::out);
            if (data::file.fail())
                throw;

            data::file << "=== Started " + timeStr + " ===\n";
        }
        catch (...) {
            data::file.close();
            std::cerr << "Error starting logger\n";
        }
    }

    // logger::end();
    // Closes log file stream, saves and stops recording logs.
    void end() {
        if (!data::file.is_open()) 
            return;
        data::file << "=== Shutdown " + dateTimeString() + " ===\n";
        data::file.close();
    }
} // namespace logger