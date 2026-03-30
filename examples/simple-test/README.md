# Simple Test Example

This example demonstrates how to use the **cpm** package manager in a simple C project that uses a testing library.

## Overview

The `simple-test` example creates a basic program that uses the **sht-lib** (Simple Header Test) library, which is a single-header testing library for C.

## Prerequisites

- `cpm` binary built and in your PATH
- `make` installed
- `git` installed
- A C compiler (gcc, clang, etc.)

## Getting Started

### 1. Navigate to the example directory

```bash
cd examples/simple-test
```

### 2. Initialize a new cmod.toml (optional)

```bash
cpm init
```

This will create a `cmod.toml` file with your module information.

### 3. Add dependencies using cpm

```bash
cpm get github.com/callowaysutton/sht-lib@v0.1.0
```

This command will:
- Add the dependency to `cmod.toml`
- Clone the sht-lib repository to `.cpm/cache/`
- Copy the library to `.cpm/vendor/`
- Generate `.cpm/cpm.mk` with appropriate build flags

### 4. Build the project

You can build using either method:

```bash
# Using cpm build command
cpm build

# Or using make directly
make
```

The `make` command works because the generated `.cpm/cpm.mk` file sets up the correct include paths.

### 5. Run the program

```bash
./simple-test
```

You should see output showing the tests running:

```
Running simple test with sht-lib...
[TEST] addition test: PASS
[TEST] string comparison: PASS
[TEST] always passing: PASS
Tests completed!
```

## How It Works

### Directory Structure

```
examples/simple-test/
├── main.c          # Main program using sht-lib
├── Makefile        # Build configuration
├── cmod.toml       # CPM module manifest (created by cpm)
└── .cpm/           # CPM working directory
    ├── cache/      # Downloaded module cache
    ├── vendor/     # Vendored dependencies
    └── cpm.mk      # Generated Makefile fragment
```

### The Makefile

The `Makefile` is designed to work seamlessly with cpm:

1. **Include cpm.mk**: `include .cpm/cpm.mk` brings in all the necessary include and library flags
2. **Standard targets**: `make` builds the project, `make clean` cleans build artifacts
3. **cpm integration**: `make build` calls `cpm build` to ensure dependencies are up-to-date

### The cmod.toml

After running `cpm get`, your `cmod.toml` will look like:

```toml
module = "github.com/yourusername/simple-test"
version = "v0.1.0"

[build]
command = "make" # Optional: command to build this library when vendored

[requires]
github.com/callowaysutton/sht-lib = "v0.1.0"
```

## Understanding the Dependencies

The example uses **sht-lib** from GitHub. This library provides:

- Single header testing framework
- Simple assertion macros
- Test discovery and execution

The library is pulled from: `https://github.com/callowaysutton/sht-lib`

## Available Commands

- `cpm init` - Initialize a new project
- `cpm get <module>@<version>` - Add a dependency
- `cpm build` - Build the project with dependencies
- `cpm vendor` - Populate vendor directory
- `cpm clean` - Clean cpm cache and vendor directory
- `cpm tidy` - Remove unused dependencies (not yet implemented)

## Using Different Git Repositories

cpm works with any Git repository:

### Remote GitHub Repositories

```bash
cpm get github.com/username/repo@v1.0.0
```

### Local Repositories

```bash
cpm get /absolute/path/to/repo@main
cpm get ../../relative/path/to/repo@v1.0.0
```

### SSH Repositories

```bash
cpm get git@github.com:username/repo.git@v1.0.0
```

### HTTPS Repositories

```bash
cpm get https://github.com/username/repo.git@v1.0.0
```

## Troubleshooting

### Build fails with "sht.h not found"

Make sure you've run `cpm get` to download and set up the dependencies:

```bash
cpm get github.com/callowaysutton/sht-lib@v0.1.0
```

### Git authentication issues

For private repositories, make sure your Git credentials are set up properly. cpm uses your system's Git configuration.

### Permission denied errors

Make sure you have write permissions in the current directory and that Git has proper permissions.

## Next Steps

1. Try adding more dependencies to see how cpm manages multiple packages
2. Create your own library and package it for use with cpm
3. Experiment with different Git repositories (local, remote, private)
4. Integrate cpm into your existing C projects

## Example Output

When everything works correctly, you should see:

```
$ cpm build
Resolving 1 dependencies:
  github.com/callowaysutton/sht-lib@v0.1.0
Vendor directory populated.
Generated .cpm/cpm.mk (add 'include .cpm/cpm.mk' to your Makefile).
cc -Wall -Wextra -O2 -I.cpm/vendor/github.com/callowaysutton/sht-lib@v0.1.0/include -c main.c -o main.o
cc  -o simple-test main.o
```

And running the program:

```
$ ./simple-test
Running simple test with sht-lib...
[TEST] addition test: PASS
[TEST] string comparison: PASS
[TEST] always passing: PASS
Tests completed!
```