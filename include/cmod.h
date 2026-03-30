/* SPDX-License-Identifier: TODO */
#ifndef CPM_CMOD_H
#define CPM_CMOD_H

#include <stddef.h>

typedef struct {
    char *path;       // module import path, e.g. "github.com/user/lib"
    char *constraint; // version constraint string
} require_t;

typedef struct {
    char *module;         // own module path
    char *version;        // own version string (tag or commit)
    char *build_command;  // custom build command
    char **generators;    // array of build generators (e.g. "make", "cmake")
    size_t generators_len;
    require_t *requires;  // array of dependencies
    size_t requires_len;  // length of the requires array
    int compile_flags;    // if true, generate compile_flags.txt for clangd
} cmod_t;

/**
 * Load a cmod.toml file from `path` into `out`. Returns 0 on success, non‑zero on error.
 */
int cmod_load(const char *path, cmod_t *out);

/**
 * Free all heap-allocated members of a cmod_t.
 */
void cmod_free(cmod_t *mod);

/**
 * Save a cmod_t into a TOML file at `path`. Returns 0 on success.
 */
int cmod_save(const char *path, const cmod_t *mod);

#endif // CPM_CMOD_H
