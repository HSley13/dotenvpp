#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace dotenvpp {

/// Controls how the Searcher walks the directory tree.
struct SearchOptions {
  /// Maximum recursion depth (0 = root only).
  std::size_t max_depth = 8;

  /// Filename to search for.
  std::string filename = ".env";

  /// Only descend into directories whose name appears here (case-insensitive).
  std::unordered_set<std::string> allowed_dirs = {
      "src",
      "source",
      "test",
      "tests",
      "include",
      "config",
      "configs",
      "configuration",
      "app",
      "apps",
      "lib",
      "libs",
      "env",
      "envs",
  };

  /// Also look in the root directory itself.
  bool search_root = true;

  /// If true, shorter (closer-to-root) paths have higher priority.
  bool root_first_priority = true;

  /// Follow symbolic links when traversing directories.
  bool follow_symlinks = false;
};

/// Walks a directory tree and collects paths to .env files.
class Searcher {
public:
  explicit Searcher(SearchOptions opts = {});

  /// Return all matching .env file paths under @p root, ordered by priority.
  [[nodiscard]]
  std::vector<std::filesystem::path> find(const std::filesystem::path &root) const;

  /// Return the highest-priority .env file, or std::nullopt if none found.
  [[nodiscard]]
  std::optional<std::filesystem::path> find_first(const std::filesystem::path &root) const;

  /// Resolve the project root via DOTENVPP_CMAKE_SOURCE_DIR env var,
  /// compile-time hint, or std::filesystem::current_path().
  [[nodiscard]]
  static std::filesystem::path cmake_source_dir();

  [[nodiscard]]
  const SearchOptions &options() const noexcept { return opts_; }

private:
  void walk(const std::filesystem::path &dir, std::size_t depth, std::vector<std::filesystem::path> &out) const;

  [[nodiscard]]
  bool is_allowed_dir(const std::string &dir_name) const noexcept;

  SearchOptions opts_;
};

} // namespace dotenvpp
