#ifndef ANSI_UTILS_H
#define ANSI_UTILS_H

// ANSI escape codes for terminal control
namespace ANSI {
  // Cursor control
  const char* const CLEAR_SCREEN = "\033[2J";
  const char* const CURSOR_HOME = "\033[H";
  const char* const HIDE_CURSOR = "\033[?25l";
  const char* const SHOW_CURSOR = "\033[?25h";

  // Text colors (foreground)
  const char* const FG_BLACK = "\033[30m";
  const char* const FG_RED = "\033[31m";
  const char* const FG_GREEN = "\033[32m";
  const char* const FG_YELLOW = "\033[33m";
  const char* const FG_BLUE = "\033[34m";
  const char* const FG_MAGENTA = "\033[35m";
  const char* const FG_CYAN = "\033[36m";
  const char* const FG_WHITE = "\033[37m";
  const char* const FG_BRIGHT_BLACK = "\033[90m";
  const char* const FG_BRIGHT_RED = "\033[91m";
  const char* const FG_BRIGHT_GREEN = "\033[92m";
  const char* const FG_BRIGHT_YELLOW = "\033[93m";
  const char* const FG_BRIGHT_BLUE = "\033[94m";
  const char* const FG_BRIGHT_MAGENTA = "\033[95m";
  const char* const FG_BRIGHT_CYAN = "\033[96m";
  const char* const FG_BRIGHT_WHITE = "\033[97m";

  // Text attributes
  const char* const RESET = "\033[0m";
  const char* const BOLD = "\033[1m";
  const char* const DIM = "\033[2m";
  const char* const UNDERLINE = "\033[4m";
  const char* const BLINK = "\033[5m";
  const char* const REVERSE = "\033[7m";

  // Helper function to move cursor to specific position (1-indexed)
  inline String cursorTo(int row, int col) {
    return "\033[" + String(row) + ";" + String(col) + "H";
  }

  // Helper function to clear from cursor to end of line
  const char* const CLEAR_TO_EOL = "\033[K";
}

#endif // ANSI_UTILS_H
