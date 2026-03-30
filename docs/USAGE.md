# C Package Manager (CPM) Usage Guide

Welcome to CPM, a minimal, dependency-resolving package manager for C based around `git` and Minimal Version Selection (MVS).

This guide will walk you through a step-by-step example of creating a library module, then creating an application module that depends on the library. All without external package indices—just standard `git` repositories.

## Outline
1. **Initialize a Library Module** (`my-lib`)
2. **Setup Library Dependencies**
3. **Initialize an Application Module** (`my-app`)
4. **Require the Local Library**
5. **Vendoring and Generating Makefiles**
6. **Compiling the App**
7. **Tidy and Housekeeping**

---

### Step 1: Initialize a Library Module

Create a folder for your library and run `cpm init`:
```console
$ mkdir my-lib
$ cd my-lib
$ cpm init
```

This generates a `cmod.toml` file automatically inferred from the local repository constraints (if any) or defaults to an `example.com` structured name:
```toml
module = "example.com/your/module"
version = "v0.1.0"
```

Because we want our library to be used by other projects via `local` referencing or `git`, we simply build out the library source files:
**`include/mylib.h`**:
```c
#pragma once
int do_math(int a, int b);
```
**`src/mylib.c`**:
```c
#include "mylib.h"
int do_math(int a, int b) { return a + b; }
```

Commit this to a git repository:
```console
$ git init
$ git add cmod.toml include src
$ git commit -m "Initial commit of my-lib"
$ git branch -m main
```

---

### Step 2: Initialize an Application Module

Now let's build an app that relies on `my-lib`. Since they are side-by-side, we can use local directory referencing.

```console
$ cd ..
$ mkdir my-app
$ cd my-app
$ cpm init
```

Your app's directory contains a fresh `cmod.toml`. Let's add a main file:
**`main.c`**:
```c
#include <stdio.h>
#include "mylib.h" // Dependency on my-lib

int main(void) {
    int sum = do_math(10, 5);
    printf("10 + 5 = %d\n", sum);
    return 0;
}
```

---

### Step 3: Require the Local Library

We can use `cpm get` to resolve our library via its local directory and exact branch:
```console
$ cpm get ../my-lib@main
```

Behind the scenes, CPM:
1. Clones the library into its internal cache (`.cpm/cache`).
2. Scans `my-lib` for further dependencies (none in this case).
3. Populates `cmod.toml` with:
   ```toml
   [requires]
   ../my-lib = "main"
   ```
4. Resolves the graph and copies everything into `.cpm/vendor/.._my-lib@main`.
5. Emits `Make` instructions inside `.cpm/cpm.mk` so your build system can compile the code seamlessly!

*(Note: to fetch a remote repo, you would simply type `cpm get github.com/user/project@v1.2.3` and CPM handles the version semantics)*

---

### Step 4: Compiling

Now that CPM has handled resolution and linking, create a small `Makefile` to link the auto-generated fragments from the vendored caches.
Because vendored `.c` files might need direct compilation, a common wildcard pattern is added to `SRCS`.

**`Makefile`**:
```makefile
TARGET = myapp
CC = cc

# Standard flags
CFLAGS = -Wall -O2

# Include the CPM-generated make overrides (this appends -I flags to CFLAGS automatically)
-include .cpm/cpm.mk

# Grab local source files and vendored C files
SRCS = main.c $(wildcard .cpm/vendor/*/src/*.c) \
              $(wildcard .cpm/vendor/.*@*/src/*.c) \
              $(wildcard .cpm/vendor/*/*.c) \
              $(wildcard .cpm/vendor/.*@*/*.c)
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
```

Now, build your code:
```console
$ make
$ ./myapp
10 + 5 = 15
```
CPM completely obscured the complex internal cache pathing and nested C includes! Your application just includes `#include "mylib.h"` dynamically from `.cpm/vendor/` cleanly.

---

### Step 5: Tidy and Clean

During active development, if you manually delete dependencies from `cmod.toml` or stop referencing them, you can tell CPM to purge non-canonical files:
```console
$ cpm tidy
Removing unused vendored modules...
```

If you wish to flush `.cpm` entirely and redraw from scratch based on `cmod.toml`:
```console
$ cpm clean
Cleaned .cpm directory
$ cpm vendor
Vendor directory populated.
```

---

## Remote Repositories vs Local Directories
When `cpm get <url>@<version>` evaluates:
1. If `<url>` starts with `.` or `/`, it is treated as a **local** repository.
2. The `<version>` parameter defaults to looking up `tags` (if numeric/semver format) or explicit hashes (`commit:xyz`) or branches (`@main`).
3. Exact dependency collisions across nested requirements are strictly resolved utilizing Minimal Version Selection.
