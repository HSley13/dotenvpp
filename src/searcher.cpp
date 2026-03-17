#include "searcher.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace dotenvpp {

// Construct a searcher with the given directory-walking options.
Searcher::Searcher(SearchOptions opts)
    : opts_(std::move(opts)) {}

// Collect all matching .env files under root, sorted by priority (path length).
std::vector<std::filesystem::path> Searcher::find(const std::filesystem::path &root) const {
  namespace fs = std::filesystem;

  if (!fs::exists(root) || !fs::is_directory(root)) {
    throw fs::filesystem_error("dotenvpp: search root does not exist or is not a directory", root, std::make_error_code(std::errc::not_a_directory));
  }

  std::vector<fs::path> results;

  if (opts_.search_root) {
    fs::path candidate = root / opts_.filename;
    if (fs::is_regular_file(candidate)) {
      results.push_back(candidate);
    }
  }

  walk(root, 0, results);

  // Sort by path length as a proxy for depth.
  if (opts_.root_first_priority) {
    std::sort(results.begin(), results.end(), [](const fs::path &a, const fs::path &b) {
      return a.string().size() < b.string().size();
    });
  } else {
    std::sort(results.begin(), results.end(), [](const fs::path &a, const fs::path &b) {
      return a.string().size() > b.string().size();
    });
  }

  return results;
}

// Return the single highest-priority .env file, or nullopt if none exist.
std::optional<std::filesystem::path> Searcher::find_first(const std::filesystem::path &root) const {
  auto all = find(root);
  if (all.empty()) {
    return std::nullopt;
  }
  return all.front();
}

// Resolve the project root: env var > compile-time hint > current working directory.
std::filesystem::path Searcher::cmake_source_dir() {
  const char *env = std::getenv("DOTENVPP_CMAKE_SOURCE_DIR");
  if (env && *env != '\0') {
    return std::filesystem::path(env);
  }

#ifdef DOTENVPP_SOURCE_DIR_HINT
#define DOTENVPP_STRINGIFY(x) #x
#define DOTENVPP_TOSTRING(x) DOTENVPP_STRINGIFY(x)
  return std::filesystem::path(DOTENVPP_TOSTRING(DOTENVPP_SOURCE_DIR_HINT));
#undef DOTENVPP_STRINGIFY
#undef DOTENVPP_TOSTRING
#endif

  return std::filesystem::current_path();
}

// Recursively descend into allowed subdirectories, collecting .env file paths.
void Searcher::walk(const std::filesystem::path &dir, std::size_t depth, std::vector<std::filesystem::path> &out) const {
  namespace fs = std::filesystem;

  if (depth >= opts_.max_depth) return;

  std::error_code ec;
  auto iter = fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec);

  if (ec) {
    return;
  }

  for (const auto &entry : iter) {
    std::error_code entry_ec;

    auto symlink_status = entry.symlink_status(entry_ec);
    if (entry_ec) {
      continue;
    }

    if (fs::is_symlink(symlink_status) && !opts_.follow_symlinks) {
      continue;
    }

    auto status = entry.status(entry_ec);
    if (entry_ec) {
      continue;
    }

    if (fs::is_directory(status)) {
      const std::string name = entry.path().filename().string();
      if (!is_allowed_dir(name)) {
        continue;
      }

      fs::path candidate = entry.path() / opts_.filename;
      if (fs::is_regular_file(candidate)) {
        out.push_back(candidate);
      }

      walk(entry.path(), depth + 1, out);
    }
  }
}

// Case-insensitive check whether a directory name is in the allow-list.
bool Searcher::is_allowed_dir(const std::string &dir_name) const noexcept {
  std::string lower = dir_name;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return opts_.allowed_dirs.count(lower) > 0;
}

} // namespace dotenvpp
