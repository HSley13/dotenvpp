#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

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

} // namespace dotenvpp
