#pragma once
// Logging is output only; the stub evaluates its arguments (as the real one does) and discards them.
#define LOG_LEVEL_SILENT  0
#define LOG_LEVEL_FATAL   1
#define LOG_LEVEL_ERROR   2
#define LOG_LEVEL_WARNING 3
#define LOG_LEVEL_NOTICE  4
#define LOG_LEVEL_TRACE   5
#define LOG_LEVEL_VERBOSE 6
#define CR "\n"

struct Logging {
  template <typename... A> void fatal(A&&...) {}
  template <typename... A> void error(A&&...) {}
  template <typename... A> void warning(A&&...) {}
  template <typename... A> void notice(A&&...) {}
  template <typename... A> void trace(A&&...) {}
  template <typename... A> void verbose(A&&...) {}
};
extern Logging Log;
