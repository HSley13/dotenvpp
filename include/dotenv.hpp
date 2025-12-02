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

} // namespace dotenvpp
