/* SPDX-License-Identifier: MIT */
#ifndef CPM_GENERATOR_H
#define CPM_GENERATOR_H

#include "resolver.h"
#include "cmod.h"

// Generate build files based on the generators specified in the cmod_t manifest
int generate_all(cmod_t *mod, resolved_t *list, size_t n);

// Individual generator functions
int make_generate(resolved_t *list, size_t n, int compile_flags);
int cmake_generate(resolved_t *list, size_t n);
int bazel_generate(resolved_t *list, size_t n);

#endif // CPM_GENERATOR_H
