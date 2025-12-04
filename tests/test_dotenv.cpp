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

// Parser tests

TEST("parser: basic KEY=VALUE") {
  auto map = dotenvpp::Parser::parse("KEY=VALUE\n");
  EXPECT_EQ(map.at("KEY"), "VALUE");
}

TEST("parser: multiple entries") {
  auto map = dotenvpp::Parser::parse("HOST=localhost\nPORT=5432\nNAME=mydb\n");
  EXPECT_EQ(map.at("HOST"), "localhost");
  EXPECT_EQ(map.at("PORT"), "5432");
  EXPECT_EQ(map.at("NAME"), "mydb");
  EXPECT_EQ(map.size(), std::size_t(3));
}

TEST("parser: blank lines and comments are ignored") {
  auto map = dotenvpp::Parser::parse("# This is a comment\n\nKEY=value\n   # indented comment\n\n");
  EXPECT_EQ(map.size(), std::size_t(1));
  EXPECT_EQ(map.at("KEY"), "value");
}

TEST("parser: whitespace around = is stripped") {
  auto map = dotenvpp::Parser::parse("KEY  =  VALUE\n");
  EXPECT_EQ(map.at("KEY"), "VALUE");
}

TEST("parser: empty value is valid") {
  auto map = dotenvpp::Parser::parse("EMPTY=\n");
  EXPECT_EQ(map.at("EMPTY"), "");
}

TEST("parser: double-quoted value strips quotes") {
  auto map = dotenvpp::Parser::parse("KEY=\"hello world\"\n");
  EXPECT_EQ(map.at("KEY"), "hello world");
}

TEST("parser: single-quoted value strips quotes") {
  auto map = dotenvpp::Parser::parse("KEY='hello world'\n");
  EXPECT_EQ(map.at("KEY"), "hello world");
}

TEST("parser: escape \\n in double-quoted value becomes real newline") {
  auto map = dotenvpp::Parser::parse("KEY=\"line1\\nline2\"\n");
  EXPECT_EQ(map.at("KEY"), "line1\nline2");
}

TEST("parser: escaped quote inside double-quoted value") {
  auto map = dotenvpp::Parser::parse("KEY=\"say \\\"hi\\\"\"\n");
  EXPECT_EQ(map.at("KEY"), "say \"hi\"");
}

TEST("parser: inline comment stripped from unquoted value") {
  auto map = dotenvpp::Parser::parse("PORT=8080 # default port\n");
  EXPECT_EQ(map.at("PORT"), "8080");
}

TEST("parser: hash without preceding space is kept in value") {
  auto map = dotenvpp::Parser::parse("TAG=my#tag\n");
  EXPECT_EQ(map.at("TAG"), "my#tag");
}

TEST("parser: Windows CRLF line endings handled") {
  auto map = dotenvpp::Parser::parse("KEY=VALUE\r\n");
  EXPECT_EQ(map.at("KEY"), "VALUE");
}

TEST("parser: value containing = sign after first") {
  auto map = dotenvpp::Parser::parse("URL=a=b=c\n");
  EXPECT_EQ(map.at("URL"), "a=b=c");
}

TEST("parser: duplicate key — first occurrence wins") {
  auto map = dotenvpp::Parser::parse("KEY=first\nKEY=second\n");
  EXPECT_EQ(map.at("KEY"), "first");
}

TEST("parser: throws ParseError on line with no =") {
  EXPECT_THROW(dotenvpp::Parser::parse("NOTAVALIDLINE\n"), dotenvpp::ParseError);
}

TEST("parser: throws ParseError on empty key") {
  EXPECT_THROW(dotenvpp::Parser::parse("=VALUE\n"), dotenvpp::ParseError);
}

TEST("parser: throws ParseError on unterminated quoted value") {
  EXPECT_THROW(dotenvpp::Parser::parse("KEY=\"unterminated\n"), dotenvpp::ParseError);
}

TEST("parser: throws ParseError on invalid key character (hyphen)") {
  EXPECT_THROW(dotenvpp::Parser::parse("MY-KEY=value\n"), dotenvpp::ParseError);
}

// Searcher tests

TEST("searcher: finds .env in root directory") {
  TempDir tmp;
  tmp.write(".env", "KEY=val\n");

  dotenvpp::Searcher s;
  auto results = s.find(tmp.path());

  EXPECT_EQ(results.size(), std::size_t(1));
  EXPECT_EQ(results[0].filename().string(), ".env");
}

TEST("searcher: finds .env in allowed subdirectory (src)") {
  TempDir tmp;
  tmp.write("src/.env", "SRC_KEY=val\n");

  dotenvpp::Searcher s;
  auto results = s.find(tmp.path());

  EXPECT_EQ(results.size(), std::size_t(1));
  EXPECT_TRUE(results[0].string().find("src") != std::string::npos);
}

TEST("searcher: does NOT enter disallowed directory (build)") {
  TempDir tmp;
  tmp.write("build/.env", "SHOULD_NOT_FIND=1\n");

  dotenvpp::Searcher s;
  auto results = s.find(tmp.path());

  EXPECT_EQ(results.size(), std::size_t(0));
}

TEST("searcher: finds .env files in multiple allowed subdirs") {
  TempDir tmp;
  tmp.write(".env", "ROOT=1\n");
  tmp.write("src/.env", "SRC=1\n");
  tmp.write("tests/.env", "TESTS=1\n");
  tmp.write("config/.env", "CONFIG=1\n");

  dotenvpp::Searcher s;
  auto results = s.find(tmp.path());

  EXPECT_EQ(results.size(), std::size_t(4));
}

TEST("searcher: root-first priority — root .env comes first") {
  TempDir tmp;
  tmp.write(".env", "ROOT=1\n");
  tmp.write("src/.env", "SRC=1\n");

  dotenvpp::SearchOptions opts;
  opts.root_first_priority = true;
  dotenvpp::Searcher s(opts);

  auto results = s.find(tmp.path());
  EXPECT_EQ(results.size(), std::size_t(2));
  EXPECT_TRUE(results[0].parent_path() == tmp.path());
}

TEST("searcher: leaf-first priority — deeper .env comes first") {
  TempDir tmp;
  tmp.write(".env", "ROOT=1\n");
  tmp.write("src/.env", "SRC=1\n");

  dotenvpp::SearchOptions opts;
  opts.root_first_priority = false;
  dotenvpp::Searcher s(opts);

  auto results = s.find(tmp.path());
  EXPECT_EQ(results.size(), std::size_t(2));
  EXPECT_TRUE(results[0].string().find("src") != std::string::npos);
}

TEST("searcher: max_depth=0 skips all subdirectories") {
  TempDir tmp;
  tmp.write(".env", "ROOT=1\n");
  tmp.write("src/.env", "SRC=1\n");

  dotenvpp::SearchOptions opts;
  opts.max_depth = 0;
  dotenvpp::Searcher s(opts);

  auto results = s.find(tmp.path());
  EXPECT_EQ(results.size(), std::size_t(1));
  EXPECT_TRUE(results[0].parent_path() == tmp.path());
}

TEST("searcher: find_first returns highest-priority result") {
  TempDir tmp;
  tmp.write(".env", "ROOT=1\n");
  tmp.write("src/.env", "SRC=1\n");

  dotenvpp::Searcher s;
  auto first = s.find_first(tmp.path());

  EXPECT_TRUE(first.has_value());
  EXPECT_TRUE(first->parent_path() == tmp.path());
}

TEST("searcher: find_first returns nullopt when nothing found") {
  TempDir tmp;

  dotenvpp::Searcher s;
  auto first = s.find_first(tmp.path());

  EXPECT_FALSE(first.has_value());
}

TEST("searcher: search_root=false skips the root .env") {
  TempDir tmp;
  tmp.write(".env", "ROOT=1\n");
  tmp.write("src/.env", "SRC=1\n");

  dotenvpp::SearchOptions opts;
  opts.search_root = false;
  dotenvpp::Searcher s(opts);

  auto results = s.find(tmp.path());
  EXPECT_EQ(results.size(), std::size_t(1));
  EXPECT_TRUE(results[0].string().find("src") != std::string::npos);
}

TEST("searcher: custom filename (.env.local)") {
  TempDir tmp;
  tmp.write(".env", "IGNORED=1\n");
  tmp.write(".env.local", "LOCAL=1\n");

  dotenvpp::SearchOptions opts;
  opts.filename = ".env.local";
  dotenvpp::Searcher s(opts);

  auto results = s.find(tmp.path());
  EXPECT_EQ(results.size(), std::size_t(1));
  EXPECT_EQ(results[0].filename().string(), ".env.local");
}

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
