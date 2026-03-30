/* SPDX-License-Identifier: MIT */
#include "generator.h"
#include <stdio.h>
#include <string.h>

int generate_all(cmod_t *mod, resolved_t *list, size_t n) {
    if (!mod) return -1;
    
    // Default to 'make' if no generators specified
    if (mod->generators_len == 0) {
        if (make_generate(list, n, mod->compile_flags) != 0) {
            fprintf(stderr, "Warning: make generator failed\n");
            return -1;
        }
        printf("Generated Make bindings.\n");
        return 0;
    }
    
    int errors = 0;
    for (size_t i = 0; i < mod->generators_len; i++) {
        const char *gen = mod->generators[i];
        if (strcmp(gen, "make") == 0) {
            if (make_generate(list, n, mod->compile_flags) != 0) {
                fprintf(stderr, "Warning: make generator failed\n");
                errors++;
            } else {
                printf("Generated Make bindings.\n");
            }
        } else if (strcmp(gen, "cmake") == 0) {
            if (cmake_generate(list, n) != 0) {
                fprintf(stderr, "Warning: cmake generator failed\n");
                errors++;
            } else {
                printf("Generated CMake bindings.\n");
            }
        } else if (strcmp(gen, "bazel") == 0) {
            if (bazel_generate(list, n) != 0) {
                fprintf(stderr, "Warning: bazel generator failed\n");
                errors++;
            } else {
                printf("Generated Bazel bindings.\n");
            }
        } else {
            fprintf(stderr, "Warning: unknown generator '%s'\n", gen);
        }
    }
    
    return errors > 0 ? -1 : 0;
}
