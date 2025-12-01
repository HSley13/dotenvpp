#include "parser.hpp"

#include <algorithm>
#include <cctype>

namespace dotenvpp {

// Strip leading and trailing whitespace (spaces, tabs, vertical tab, form feed, carriage return).
std::string_view Parser::trim(std::string_view s) noexcept {
  auto is_ws = [](unsigned char c) {
    return c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r';
  };

  while (!s.empty() && is_ws(static_cast<unsigned char>(s.front()))) {
    s.remove_prefix(1);
  }

  while (!s.empty() && is_ws(static_cast<unsigned char>(s.back()))) {
    s.remove_suffix(1);
  }

  return s;
}

// Return true for blank lines and comment lines (starting with '#').
bool Parser::is_ignorable(std::string_view line) noexcept {
  return line.empty() || line.front() == '#';
}

// Validate that a key contains only alphanumeric characters and underscores.
bool Parser::is_valid_key(std::string_view key) noexcept {
  if (key.empty()) {
    return false;
  }

  for (unsigned char c : key) {
    if (!std::isalnum(c) && c != '_') {
      return false;
    }
  }

  return true;
}

// Return a trimmed unquoted value, stripping any inline comment after ' #'.
std::string Parser::parse_unquoted_value(std::string_view raw) noexcept {
  // Strip inline comments: '#' preceded by whitespace.
  for (std::size_t i = 1; i < raw.size(); i++) {
    if (raw[i] == '#') {
      unsigned char prev = static_cast<unsigned char>(raw[i - 1]);
      if (prev == ' ' || prev == '\t') {
        raw = raw.substr(0, i - 1);
        break;
      }
    }
  }

  return std::string(trim(raw));
}

} // namespace dotenvpp
