/* SPDX-License-Identifier: MIT */
#ifndef CPM_VENDOR_H
#define CPM_VENDOR_H

#include "resolver.h"

/**
 * Copy resolved modules into .cpm/vendor/.
 * Returns 0 on success, non-zero on error.
 */
int vendor_populate(resolved_t *list, size_t n);

/**
 * Remove a specific vendored module from .cpm/vendor/.
 * Returns 0 on success, non-zero on error.
 */
int vendor_clean_unused(const char *module, const char *version);

#endif // CPM_VENDOR_H