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
} // namespace dotenvpp
