/* SPDX-License-Identifier: TODO */
#ifndef CPM_CACHE_H
#define CPM_CACHE_H

#include <stddef.h>

/**
 * Return the cache path for a module@version.
 * Returns a newly allocated string; caller must free.
 */
char *cache_path(const char *module, const char *version);

/**
 * Ensure the module is cached; if not, fetch it.
 * Returns 0 on success, non-zero on error.
 */
int ensure_cached(const char *module, const char *version);

#endif // CPM_CACHE_H