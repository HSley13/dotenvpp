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

LoadResult load(LoadOptions opts) {
  std::filesystem::path root = opts.root.value_or(Searcher::cmake_source_dir());

  Searcher searcher(opts.search);
  auto found = searcher.find(root);

  LoadResult result;
  result.files_found = found;

  if (found.empty()) {
    if (opts.throw_if_missing) {
      throw std::runtime_error("[dotenvpp] No .env file found under: " + root.string());
    }
    return result;
  }

  for (const auto &file_path : found) {
    try {
      auto content = read_file_contents(file_path);
      auto parsed = Parser::parse(content, file_path);
      auto [loaded, skipped] = inject_into_env(parsed, opts.overwrite);

      result.files_loaded.push_back(file_path);
      result.keys_loaded += loaded;
      result.keys_skipped += skipped;
    } catch (const ParseError &) {
      throw;
    } catch (const std::filesystem::filesystem_error &) {
      result.files_failed.push_back(file_path);
    }
  }

  return result;
}

LoadResult load_file(const std::filesystem::path &path, bool overwrite) {
  LoadResult result;
  result.files_found = {path};

  auto content = read_file_contents(path);
  auto parsed = Parser::parse(content, path);
  auto [loaded, skipped] = inject_into_env(parsed, overwrite);

  result.files_loaded = {path};
  result.keys_loaded = loaded;
  result.keys_skipped = skipped;

  return result;
}

std::string get(std::string_view key, std::string_view default_value) {
  const char *val = std::getenv(std::string(key).c_str());
  return val ? std::string(val) : std::string(default_value);
}

bool has(std::string_view key) noexcept {
  return std::getenv(std::string(key).c_str()) != nullptr;
}

std::string require(std::string_view key) {
  const char *val = std::getenv(std::string(key).c_str());
  if (!val) {
    throw std::runtime_error("[dotenvpp] Required environment variable '" + std::string(key) + "' is not set");
  }
  return std::string(val);
}

void set(std::string_view key, std::string_view value, bool overwrite) {
  std::string k(key), v(value);
  if (!env_set(k.c_str(), v.c_str(), overwrite)) {
    throw std::runtime_error("[dotenvpp] Failed to set environment variable '" + k + "'");
  }
}

void unset(std::string_view key) noexcept {
  env_unset(std::string(key).c_str());
}

} // namespace dotenvpp
