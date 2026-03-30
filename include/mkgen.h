/* SPDX-License-Identifier: TODO */
#ifndef CPM_MKGEN_H
#define CPM_MKGEN_H

#include "resolver.h"

/**
 * Generate .cpm/cpm.mk with include/library flags for vendored modules.
 * Returns 0 on success, non-zero on error.
 */
int mkgen_generate(resolved_t *list, size_t n);

#endif // CPM_MKGEN_H