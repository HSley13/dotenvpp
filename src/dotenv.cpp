#include "dotenv.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>

#if defined(_WIN32)
#include <cstdlib>
namespace {
// Set an environment variable using the platform-specific API.
bool env_set(const char *key, const char *value, bool overwrite) {
  if (!overwrite && std::getenv(key) != nullptr)
    return true;
  return _putenv_s(key, value) == 0;
}
// Remove an environment variable on Windows.
void env_unset(const char *key) { _putenv_s(key, ""); }
} // namespace
#else
#include <unistd.h>
namespace {
// Set an environment variable using the platform-specific API.
bool env_set(const char *key, const char *value, bool overwrite) {
  return ::setenv(key, value, overwrite ? 1 : 0) == 0;
}
// Remove an environment variable on POSIX.
void env_unset(const char *key) { ::unsetenv(key); }
} // namespace
#endif

namespace dotenvpp {
namespace {

// Apply a parsed key-value map to the process environment, returning (loaded, skipped) counts.
std::pair<std::size_t, std::size_t> inject_into_env(const Parser::EnvMap &map, bool overwrite) {
  std::size_t loaded = 0;
  std::size_t skipped = 0;

  for (const auto &[key, value] : map) {
    if (!overwrite && std::getenv(key.c_str()) != nullptr) {
      skipped++;
      continue;
    }

    if (env_set(key.c_str(), value.c_str(), overwrite)) {
      loaded++;
    }
  }

  return {loaded, skipped};
}

// Read an entire file into a string, or throw on failure.
std::string read_file_contents(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::filesystem::filesystem_error("dotenvpp: cannot open file", path, std::make_error_code(std::errc::no_such_file_or_directory));
  }

  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

} // namespace
} // namespace dotenvpp
