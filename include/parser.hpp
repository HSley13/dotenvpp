#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dotenvpp {

/// Exception thrown when a .env file contains malformed syntax.
class ParseError : public std::runtime_error {
public:
  ParseError(const std::filesystem::path &file, std::size_t line, const std::string &msg)
      : std::runtime_error("[dotenvpp] Parse error in " + file.string() + " at line " + std::to_string(line) + ": " + msg), file_(file), line_(line) {}

  const std::filesystem::path &file() const noexcept { return file_; }
  std::size_t line() const noexcept { return line_; }

private:
  std::filesystem::path file_;
  std::size_t line_;
};

/// Parses the content of a single .env file into a key-value map.
///
/// Supported syntax:
///   - Comments: lines starting with `#`
///   - Basic: `KEY=VALUE`
///   - Quoted: `KEY="value"` or `KEY='value'`
///   - Escape sequences in double quotes: `\\`, `\"`, `\n`, `\t`, `\r`
///   - Inline comments: `KEY=value # comment` (space before `#` required)
///   - Only the first `=` splits key from value
///   - Keys must match `[A-Za-z0-9_]+`
///   - Duplicate keys: first occurrence wins
class Parser {
public:
  using EnvMap = std::unordered_map<std::string, std::string>;

  [[nodiscard]]
  static EnvMap parse(std::string_view content,
                      const std::filesystem::path &filepath = "<unknown>");

private:
  [[nodiscard]] static std::string_view trim(std::string_view s) noexcept;
  [[nodiscard]] static bool is_ignorable(std::string_view line) noexcept;
  [[nodiscard]] static bool is_valid_key(std::string_view key) noexcept;

  [[nodiscard]]
  static std::string parse_value(std::string_view raw, const std::filesystem::path &file, std::size_t line_no);

  [[nodiscard]]
  static std::string parse_quoted_value(std::string_view raw, char quote_char, const std::filesystem::path &file, std::size_t line_no);

  [[nodiscard]]
  static std::string parse_unquoted_value(std::string_view raw) noexcept;
};

} // namespace dotenvpp
