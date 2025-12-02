#include "searcher.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace dotenvpp {

// Construct a searcher with the given directory-walking options.
Searcher::Searcher(SearchOptions opts)
    : opts_(std::move(opts)) {}

// Case-insensitive check whether a directory name is in the allow-list.
bool Searcher::is_allowed_dir(const std::string &dir_name) const noexcept {
  std::string lower = dir_name;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
  return opts_.allowed_dirs.count(lower) > 0;
}

} // namespace dotenvpp
