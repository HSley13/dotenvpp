# dotenvpp

A zero-dependency C++20 library that recursively searches your project tree for `.env` files, parses them, and injects key-value pairs into the process environment.

## Features

- Recursive `.env` file discovery with configurable allow-list of directories
- Quoted values with escape sequence support (`\n`, `\t`, `\\`, `\"`)
- Inline comments, blank lines, and `#` full-line comments
- Priority control: root-first (default) or leaf-first
- Overwrite control: protect existing env vars or let `.env` win
- Cross-platform: POSIX (`setenv`) and Windows (`_putenv_s`)
- CMake `FetchContent` and `find_package` ready

## Requirements

- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019 16.8+)
- CMake 3.21+

## Installation

### Option A: FetchContent (recommended)

Add to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
    dotenvpp
    GIT_REPOSITORY https://github.com/<your-username>/dotenvpp.git
    GIT_TAG        v0.1.0
)
FetchContent_MakeAvailable(dotenvpp)

target_link_libraries(your_target PRIVATE dotenvpp::dotenvpp)
```

You can also use a local path during development:

```cmake
FetchContent_Declare(dotenvpp SOURCE_DIR /path/to/dotenvpp)
```

### Option B: System install + find_package

```bash
cmake -S . -B build
cmake --build build
cmake --install build --prefix ~/.local
```

Then in your project:

```cmake
list(APPEND CMAKE_PREFIX_PATH "~/.local")
find_package(dotenvpp REQUIRED)

target_link_libraries(your_target PRIVATE dotenvpp::dotenvpp)
```

## Usage

### Basic

```cpp
#include <dotenvpp/dotenv.hpp>
#include <iostream>

int main() {
    dotenvpp::load();

    std::string db   = dotenvpp::get("DATABASE_URL");
    std::string port = dotenvpp::get("PORT", "8080");

    if (dotenvpp::has("DEBUG"))
        std::cout << "Debug mode enabled\n";

    // Throws std::runtime_error if not set
    std::string secret = dotenvpp::require("JWT_SECRET");
}
```

### Load from a specific file

```cpp
dotenvpp::load_file("/path/to/.env");
dotenvpp::load_file("/path/to/.env.production", /*overwrite=*/true);
```

### Custom search options

```cpp
dotenvpp::LoadOptions opts {
    .overwrite        = true,
    .throw_if_missing = true,
    .search = {
        .max_depth           = 3,
        .filename            = ".env.local",
        .root_first_priority = false,
    },
    .root = "/explicit/project/root",
};

auto result = dotenvpp::load(opts);

std::cout << result.files_loaded.size() << " file(s), "
          << result.keys_loaded << " key(s) loaded, "
          << result.keys_skipped << " skipped\n";
```

### Programmatic env manipulation

```cpp
dotenvpp::set("MY_VAR", "value");
dotenvpp::unset("MY_VAR");
```

## `.env` File Syntax

```bash
# Comments
DATABASE_URL=postgres://localhost:5432/mydb

# Quoted values
APP_NAME="My App"
GREETING='hello world'

# Escape sequences in double quotes
MULTILINE="line1\nline2"
ESCAPED="She said \"hello\""

# Inline comments (space before # required)
PORT=8080 # default

# Hash in value (no space before # = not a comment)
TAG=v1.0#stable

# Whitespace around = is trimmed
KEY  =  VALUE

# Empty values are valid
EMPTY=

# Duplicate keys: first occurrence wins
KEY=first
KEY=second  # ignored
```

## API Reference

| Function | Description |
|----------|-------------|
| `load(opts)` | Search for `.env` files and inject into env. Returns `LoadResult`. |
| `load_file(path, overwrite)` | Parse a single `.env` file and inject into env. |
| `get(key, default)` | Read an env var. Returns `default` (or `""`) if unset. |
| `has(key)` | Check if an env var is set. |
| `require(key)` | Read an env var. Throws `std::runtime_error` if unset. |
| `set(key, value, overwrite)` | Set an env var directly. |
| `unset(key)` | Remove an env var. |

### SearchOptions

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `max_depth` | `size_t` | `8` | Max directory recursion depth |
| `filename` | `string` | `".env"` | Filename to search for |
| `allowed_dirs` | `unordered_set<string>` | `src`, `test`, `config`, ... | Directories to enter |
| `search_root` | `bool` | `true` | Also check the root directory |
| `root_first_priority` | `bool` | `true` | Shorter paths = higher priority |
| `follow_symlinks` | `bool` | `false` | Follow symlinks during traversal |

### Directory Search Behavior

By default, `load()` starts at the CMake source directory (or cwd) and only enters these subdirectories:

`src`, `source`, `test`, `tests`, `include`, `config`, `configs`, `configuration`, `app`, `apps`, `lib`, `libs`, `env`, `envs`

Directories like `build/`, `.git/`, `node_modules/`, etc. are never entered.

## Build & Test

```bash
git clone https://github.com/<your-username>/dotenvpp.git
cd dotenvpp
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## License

MIT
