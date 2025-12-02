#pragma once

#include "parser.hpp"
#include "searcher.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dotenvpp {

/// Controls how load() searches and injects environment variables.
struct LoadOptions {
  /// Replace existing env vars with values from .env files.
  bool overwrite = false;

  /// Throw std::runtime_error if no .env file is found.
  bool throw_if_missing = false;

  /// Directory search configuration.
  SearchOptions search;

  /// Explicit root directory. Falls back to cmake_source_dir() / cwd.
  std::optional<std::filesystem::path> root;
};

/// Describes the outcome of a load() or load_file() call.
struct LoadResult {
  std::vector<std::filesystem::path> files_found;
  std::vector<std::filesystem::path> files_loaded;
  std::vector<std::filesystem::path> files_failed;
  std::size_t keys_loaded = 0;
  std::size_t keys_skipped = 0;

  [[nodiscard]] bool ok() const noexcept { return !files_loaded.empty(); }
};

/// Search for .env files and inject their contents into the process environment.
LoadResult load(LoadOptions opts = {});

/// Parse a single .env file and inject its contents into the process environment.
LoadResult load_file(const std::filesystem::path &path, bool overwrite = false);

/// Read an environment variable, returning @p default_value if unset.
[[nodiscard]]
std::string get(std::string_view key, std::string_view default_value = "");

/// Return true if the environment variable @p key is set (even if empty).
[[nodiscard]]
bool has(std::string_view key) noexcept;

/// Read a required environment variable; throws std::runtime_error if unset.
[[nodiscard]]
std::string require(std::string_view key);

/// Set an environment variable. Throws std::runtime_error on failure.
void set(std::string_view key, std::string_view value, bool overwrite = true);

/// Remove an environment variable from the process environment.
void unset(std::string_view key) noexcept;

} // namespace dotenvpp
