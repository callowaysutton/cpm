# CPM - A Git-based C Package Manager

A lightweight, Git-based package manager for C projects, inspired by Go's package ecosystem. CPM leverages your system's `git` binary to fetch dependencies from any Git repository (local or remote), providing a zero-runtime-dependency solution for managing C libraries.

## Features

- **Git-based**: Uses your system `git` to fetch dependencies from any Git repository
- **Project-local environment**: All dependencies stored in `.cpm/` directory (like Python's `.venv`)
- **Multiple Git sources**:
  - Remote repositories (GitHub, GitLab, Bitbucket, etc.)
  - Local directories (absolute or relative paths)
  - SSH and HTTPS protocols
  - Tag or commit-based versioning
- **Simple manifest**: Uses `cmod.toml` (TOML-based module definition)
- **Vendor mode**: Creates vendored copies for reproducible builds
- **Make integration**: Automatically generates `.cpm/cpm.mk` with proper include/link flags
- **Zero runtime dependencies**: Only requires `git` and your system C compiler

## Installation

### Build from source

```bash
cd /path/to/cpm
make
sudo make install  # Installs to ~/.cpm/bin/cpm
```

Add `~/.cpm/bin` to your PATH:

```bash
echo 'export PATH="$HOME/.cpm/bin:$PATH"' >> ~/.bashrc  # or ~/.zshrc
source ~/.bashrc
```

## Quick Start

### 1. Initialize a project

```bash
cd my-project
cpm init
```

This creates a `cmod.toml` file:

```toml
module = "github.com/yourname/my-project"
version = "v0.1.0"
```

### 2. Add dependencies

```bash
cpm get github.com/username/lib@v1.0.0
```

This will:
- Clone the repository to `.cpm/cache/`
- Copy to `.cpm/vendor/`
- Generate `.cpm/cpm.mk` with proper flags

### 3. Build your project

Create a `Makefile` that includes the CPM-generated fragment:

```makefile
CC = cc
CFLAGS := -Wall -Wextra -O2
LDFLAGS :=
TARGET = myapp

# Include CPM-generated flags (if they exist)
-include .cpm/cpm.mk

SRCS = main.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
```

Build using:

```bash
cpm build  # or just 'make'
```

## Commands

### `cpm init`

Initialize a new project with a `cmod.toml` manifest. If the project is a Git repository, it will use the git remote URL as the module name.

### `cpm get <module>@<version>`

Add a dependency:
- `<module>`: Git repository path (e.g., `github.com/user/lib` or `/local/path/to/lib`)
- `<version>`: Tag name (e.g., `v1.0.0`) or commit SHA (prefix with `commit:`)

Examples:
```bash
cpm get github.com/callowaysutton/sht-lib@main
cpm get https://github.com/user/repo.git@v1.0.0
cpm get git@github.com:user/repo.git@commit:9a1b2c3
cpm get /absolute/path/to/lib@v1.0.0
cpm get ../../relative/path@main
```

### `cpm build`

Generate `.cpm/cpm.mk` and run `make`. Ensures all dependencies are available before building.

### `cpm vendor`

Populate the `.cpm/vendor/` directory with copies of all dependencies.

### `cpm clean`

Remove the `.cpm/` directory (cache and vendor).

### `cpm tidy`

*(Not yet implemented)* - Remove unused dependencies from `cmod.toml`.

## Manifest Format

`cmod.toml` uses simple TOML syntax:

```toml
module = "github.com/username/project"
version = "v0.1.0"

[requires]
github.com/callowaysutton/sht-lib = "main"
github.com/other/lib = "v2.0.0"
```

## Directory Structure

After running CPM commands:

```
my-project/
├── cmod.toml          # Module manifest
├── Makefile           # Your build configuration
├── main.c             # Your source code
└── .cpm/              # CPM working directory
    ├── cache/         # Downloaded modules
    │   └── <module>@<version>/
    ├── vendor/        # Vendored copies
    │   └── <module>@<version>/
    └── cpm.mk         # Generated Makefile fragment
```

## Examples

See the `examples/simple-test/` directory for a complete working example using the sht-lib testing library.

### Running the example

```bash
cd examples/simple-test
cpm init
cpm get github.com/callowaysutton/sht-lib@main
make
./simple-test
```

## How It Works

1. **Dependency Resolution**: CPM reads `cmod.toml` and builds a dependency graph
2. **Fetching**: Uses `git clone --depth 1` to fetch dependencies at specified versions
3. **Caching**: Store downloaded modules in `.cpm/cache/<module>@<version>/`
4. **Vendoring**: Copy dependencies to `.cpm/vendor/` for reproducible builds
5. **Build Integration**: Generate `.cpm/cpm.mk` with proper `-I` and `-L` flags

## Versioning

CPM supports two version identifier formats:

- **Tags**: `v1.0.0`, `v2.1.3`, `release-1.0` - fetches the specified tag
- **Commits**: `commit:9a1b2c3d` - fetches that specific commit SHA

## Git Repository Formats

CPM accepts various Git repository formats:

- **GitHub/GitLab/Bitbucket**: `github.com/user/repo`
- **HTTPS**: `https://github.com/user/repo.git`
- **SSH**: `git@github.com:user/repo.git`
- **Local paths**: `/absolute/path/to/repo`, `../../relative/path`
- **Custom URLs**: Any valid Git URL with `://`

## Authentication

CPM uses your system's Git configuration. If you can access a repository normally with `git clone`, CPM can fetch it too. This includes:
- SSH key authentication
- HTTPS credential helpers
- Personal access tokens
- Any other Git-supported authentication method

## Build System Integration

### Make

CPM generates a Makefile fragment that can be included:

```makefile
include .cpm/cpm.mk
```

The fragment sets:
- `CFLAGS += -I$(CPM_VENDOR_PATH)/<module>@<version>`
- `LDFLAGS += -L$(CPM_VENDOR_PATH)/<module>@<version>/lib` (if lib directory exists)

### Future Support

Planned support for:
- CMake (`cpm_cmake.cmake`)
- Bazel (`cpm_bazel.bzl`)
- Meson (via generator)

## Troubleshooting

### "Git command failed"

Ensure `git` is installed and accessible:

```bash
git --version
```

### "Repository not found"

- Check that the repository URL is correct
- Ensure you have access permissions
- For private repos, verify your Git credentials are configured

### "Include file not found"

- Ensure you've run `cpm get` to fetch dependencies
- Check that the library has an `include/` directory
- Verify the library structure matches your expectations

## Architecture

CPM is built with zero runtime dependencies (except `git`):

- **TOML Parser**: Hand-rolled minimal parser for `cmod.toml`
- **Git Integration**: Invokes system `git` binary via `system()` calls
- **Caching**: Uses filesystem operations for module storage
- **Build Generation**: Creates Makefile fragments with proper compiler flags

## Contributing

CPM is designed to be simple and maintainable. Contributions welcome!

Areas for improvement:
- `cpm tidy` command implementation
- Support for more version constraints (range operators)
- Native Git protocol implementation (replacing system `git`)
- Additional build system generators (CMake, Bazel)
- Dependency conflict resolution
- Global cache support

## Acknowledgments

Inspired by Go's package management philosophy and the need for a simple, Git-based C package manager that works with any Git repository.