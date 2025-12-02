#include "searcher.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace dotenvpp {

// Construct a searcher with the given directory-walking options.
Searcher::Searcher(SearchOptions opts)
    : opts_(std::move(opts)) {}

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
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
  return opts_.allowed_dirs.count(lower) > 0;
}

} // namespace dotenvpp
