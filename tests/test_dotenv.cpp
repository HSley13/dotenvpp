#include "dotenv.hpp"
#include "parser.hpp"
#include "searcher.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

// Minimal test harness (zero external dependencies)

struct TestCase {
  std::string name;
  std::function<void()> fn;
};

// Global registry of all test cases, populated at static-init time.
static std::vector<TestCase> &test_registry() {
  static std::vector<TestCase> reg;
  return reg;
}

// Auto-registers a test function into the global registry on construction.
struct TestRegistrar {
  TestRegistrar(const char *name, std::function<void()> fn) {
    test_registry().push_back({name, std::move(fn)});
  }
};

#define DOTENVPP_CAT_(a, b) a##b
#define DOTENVPP_CAT(a, b) DOTENVPP_CAT_(a, b)
#define DOTENVPP_FN(n) DOTENVPP_CAT(dotenvpp_fn_, n)
#define DOTENVPP_REG(n) DOTENVPP_CAT(dotenvpp_reg_, n)

#define DOTENVPP_TEST_IMPL(name, c_fn, c_reg) \
  static void DOTENVPP_FN(c_fn)();            \
  static TestRegistrar DOTENVPP_REG(c_reg)(   \
      name, DOTENVPP_FN(c_fn));               \
  static void DOTENVPP_FN(c_fn)()

#define TEST(name) DOTENVPP_TEST_IMPL(name, __COUNTER__, __COUNTER__)

#define EXPECT_EQ(a, b)                              \
  do {                                               \
    auto _a = (a);                                   \
    auto _b = (b);                                   \
    if (_a != _b) {                                  \
      std::cerr << "  FAIL: " << #a << " == " << #b  \
                << "\n    got:      " << _a          \
                << "\n    expected: " << _b << "\n"; \
      throw std::runtime_error("assertion failed");  \
    }                                                \
  } while (0)

#define EXPECT_TRUE(expr)                                      \
  do {                                                         \
    if (!(expr)) {                                             \
      std::cerr << "  FAIL: expected true: " << #expr << "\n"; \
      throw std::runtime_error("assertion failed");            \
    }                                                          \
  } while (0)

#define EXPECT_FALSE(expr)                                      \
  do {                                                          \
    if ((expr)) {                                               \
      std::cerr << "  FAIL: expected false: " << #expr << "\n"; \
      throw std::runtime_error("assertion failed");             \
    }                                                           \
  } while (0)

#define EXPECT_THROW(expr, exc_type)                \
  do {                                              \
    bool caught = false;                            \
    try {                                           \
      (void)(expr);                                 \
    } catch (const exc_type &) { caught = true; }   \
    if (!caught) {                                  \
      std::cerr << "  FAIL: expected " << #exc_type \
                << " from: " << #expr << "\n";      \
      throw std::runtime_error("assertion failed"); \
    }                                               \
  } while (0)

// TempDir — RAII temporary directory for filesystem tests

class TempDir {
public:
  TempDir() {
    auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
    static std::size_t seq = 0;
    path_ = fs::temp_directory_path() /
            ("dotenvpp_test_" + std::to_string(tid) + "_" +
             std::to_string(seq++));
    fs::create_directories(path_);
  }

  ~TempDir() {
    std::error_code ec;
    fs::remove_all(path_, ec);
  }

  TempDir(const TempDir &) = delete;
  TempDir &operator=(const TempDir &) = delete;

  const fs::path &path() const { return path_; }

  void write(const fs::path &rel_path, const std::string &content) const {
    fs::path full = path_ / rel_path;
    fs::create_directories(full.parent_path());
    std::ofstream f(full);
    if (!f) {
      throw std::runtime_error("TempDir::write: cannot create " + full.string());
    }

    f << content;
  }

private:
  fs::path path_;
};

// Test runner

int main() {
  int passed = 0, failed = 0;

  std::cout << "\n** dotenvpp test suite **\n\n";

  for (const auto &tc : test_registry()) {
    std::cout << "  [ RUN  ] " << tc.name << "\n";
    try {
      tc.fn();
      std::cout << "  [  OK  ] " << tc.name << "\n";
      passed++;
    } catch (const std::exception &e) {
      std::cout << "  [ FAIL ] " << tc.name << "\n"
                << "           " << e.what() << "\n";
      failed++;
    } catch (...) {
      std::cout << "  [ FAIL ] " << tc.name << " (unknown exception)\n";
      failed++;
    }
  }

  std::cout << "\n"
            << passed << " passed, " << failed << " failed"
            << "  (total " << (passed + failed) << ")\n\n";

  return failed == 0 ? 0 : 1;
}
