/* SPDX-License-Identifier: TODO */
#include "cmod.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Simple helper to trim whitespace from both ends */
static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

/* Remove surrounding quotes from a string literal, if present */
static char *strip_quotes(char *s) {
    if (s[0] == '"' && s[strlen(s) - 1] == '"') {
        s[strlen(s) - 1] = '\0';
        return s + 1;
    }
    return s;
}

int cmod_load(const char *path, cmod_t *out) {
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    char line[1024];
    int in_requires = 0;
    int in_build = 0;
    out->module = NULL;
    out->version = NULL;
    out->build_command = NULL;
    out->generators = NULL;
    out->generators_len = 0;
    out->requires = NULL;
    out->requires_len = 0;
    out->compile_flags = 0;

    while (fgets(line, sizeof(line), fp)) {
        char *p = trim(line);
        if (p[0] == '\0' || p[0] == '#') continue; // skip empty / comment lines
        if (p[0] == '[') {
            if (strncmp(p, "[requires]", 10) == 0) {
                in_requires = 1;
                in_build = 0;
            } else if (strncmp(p, "[build]", 7) == 0) {
                in_build = 1;
                in_requires = 0;
            } else {
                in_requires = 0; // ignore other tables
                in_build = 0;
            }
            continue;
        }
        // key = "value"
        char *eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = trim(p);
        char *val = trim(eq + 1);
        val = strip_quotes(val);
        if (!in_requires && !in_build) {
            if (strcmp(key, "module") == 0) {
                out->module = strdup(val);
            } else if (strcmp(key, "version") == 0) {
                out->version = strdup(val);
            } else if (strcmp(key, "compile_flags") == 0) {
                out->compile_flags = (strcmp(val, "true") == 0) ? 1 : 0;
            } else if (strcmp(key, "generators") == 0) {
                // Parse array e.g. ["make", "cmake"]
                char *start = strchr(val, '[');
                char *end = strrchr(val, ']');
                if (start && end && end > start) {
                    *end = '\0';
                    char *token = strtok(start + 1, ",");
                    while (token) {
                        token = trim(token);
                        token = strip_quotes(token);
                        if (*token) {
                            out->generators = realloc(out->generators, (out->generators_len + 1) * sizeof(char *));
                            out->generators[out->generators_len++] = strdup(token);
                        }
                        token = strtok(NULL, ",");
                    }
                }
            }
        } else if (in_requires) {
            // dependency entry
            require_t *new_arr = realloc(out->requires, (out->requires_len + 1) * sizeof(require_t));
            if (!new_arr) { fclose(fp); return -1; }
            out->requires = new_arr;
            out->requires[out->requires_len].path = strdup(key);
            out->requires[out->requires_len].constraint = strdup(val);
            out->requires_len++;
        } else if (in_build) {
            if (strcmp(key, "command") == 0) {
                out->build_command = strdup(val);
            }
        }
    }
    fclose(fp);
    return 0;
}

void cmod_free(cmod_t *mod) {
    if (!mod) return;
    free(mod->module);
    free(mod->version);
    free(mod->build_command);
    for (size_t i = 0; i < mod->generators_len; ++i) {
        free(mod->generators[i]);
    }
    free(mod->generators);
    for (size_t i = 0; i < mod->requires_len; ++i) {
        free(mod->requires[i].path);
        free(mod->requires[i].constraint);
    }
    free(mod->requires);
    mod->module = NULL;
    mod->version = NULL;
    mod->build_command = NULL;
    mod->generators = NULL;
    mod->generators_len = 0;
    mod->requires = NULL;
    mod->requires_len = 0;
}

int cmod_save(const char *path, const cmod_t *mod) {
    FILE *fp = fopen(path, "w");
    if (!fp) return -1;
    if (mod->module)
        fprintf(fp, "module = \"%s\"\n", mod->module);
    if (mod->version)
        fprintf(fp, "version = \"%s\"\n", mod->version);
    if (mod->generators_len > 0) {
        fprintf(fp, "generators = [");
        for (size_t i = 0; i < mod->generators_len; ++i) {
            fprintf(fp, "\"%s\"%s", mod->generators[i], i + 1 < mod->generators_len ? ", " : "");
        }
        fprintf(fp, "]\n");
    }
    if (mod->build_command) {
        fprintf(fp, "\n[build]\n");
        fprintf(fp, "command = \"%s\"\n", mod->build_command);
    }
    if (mod->requires_len > 0) {
        fprintf(fp, "\n[requires]\n");
        for (size_t i = 0; i < mod->requires_len; ++i) {
            fprintf(fp, "%s = \"%s\"\n", mod->requires[i].path, mod->requires[i].constraint);
        }
    }
    fclose(fp);
    return 0;
}
