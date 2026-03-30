/* SPDX-License-Identifier: TODO */
#define _GNU_SOURCE
#include "cache.h"
#include "fetcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char *sanitize(const char *s) {
    char *out = strdup(s);
    for (char *p = out; *p; ++p) {
        if (*p == '/' || *p == '@' || *p == ':') *p = '_';
    }
    return out;
}

char *cache_path(const char *module, const char *version) {
    char *clean_module = sanitize(module);
    char *clean_version = sanitize(version);
    char *path;
    asprintf(&path, ".cpm/cache/%s@%s", clean_module, clean_version);
    free(clean_module);
    free(clean_version);
    return path;
}

static int dir_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

int ensure_cached(const char *module, const char *version) {
    char *path = cache_path(module, version);
    if (dir_exists(path)) {
        free(path);
        return 0;
    }
    // Ensure base directory exists
    system("mkdir -p .cpm/cache");
    int rc = fetch_module(module, version, path);
    free(path);
    return rc;
}