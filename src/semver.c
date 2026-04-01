#include "../include/semver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Helper to duplicate strings safely
static char *semver_strdup(const char *s) {
    return s ? strdup(s) : NULL;
}

// Parses a version string into a semver_t struct.
// Returns 0 on success, -1 on error.
int semver_parse(const char *version_str, semver_t *out) {
    if (!version_str || !out) return -1;

    memset(out, 0, sizeof(semver_t)); // Initialize to zero
    out->major = -1;
    out->minor = -1;
    out->patch = -1;

    char *mutable_version = semver_strdup(version_str);
    if (!mutable_version) return -1;

    char *p = mutable_version;

    // Handle optional 'v' prefix
    if (*p == 'v' || *p == 'V') {
        p++;
    }

    if (!isdigit(*p)) {
        free(mutable_version);
        return -1;
    }

    char *major_str = p;
    char *dot1 = strchr(p, '.');
    char type = 0;
    
    if (!dot1) {
        // No minor/patch, just major
        char *pre = strchr(p, '-');
        char *bld = strchr(p, '+');
        char *pre_or_build = pre && bld ? (pre < bld ? pre : bld) : (pre ? pre : bld);
        
        if (pre_or_build) {
            type = *pre_or_build;
            *pre_or_build = '\0';
            out->major = atoi(major_str);
            p = pre_or_build + 1;
        } else {
            out->major = atoi(major_str);
            p = NULL; 
        }
    } else {
        *dot1 = '\0';
        out->major = atoi(major_str);

        char *minor_str = dot1 + 1;
        char *dot2 = strchr(minor_str, '.');
        if (!dot2) {
            // No patch version
            char *pre = strchr(minor_str, '-');
            char *bld = strchr(minor_str, '+');
            char *pre_or_build = pre && bld ? (pre < bld ? pre : bld) : (pre ? pre : bld);

            if (pre_or_build) {
                type = *pre_or_build;
                *pre_or_build = '\0';
                out->minor = atoi(minor_str);
                p = pre_or_build + 1;
            } else {
                out->minor = atoi(minor_str);
                p = NULL; // End of string
            }
        } else {
            *dot2 = '\0';
            out->minor = atoi(minor_str);

            char *patch_str = dot2 + 1;
            char *pre = strchr(patch_str, '-');
            char *bld = strchr(patch_str, '+');
            char *pre_or_build = pre && bld ? (pre < bld ? pre : bld) : (pre ? pre : bld);

            if (pre_or_build) {
                type = *pre_or_build;
                *pre_or_build = '\0';
                out->patch = atoi(patch_str);
                p = pre_or_build + 1;
            } else {
                out->patch = atoi(patch_str);
                p = NULL; // End of string
            }
        }
    }

    if (p && type) {
        if (type == '-') {
            char *build_ptr = strchr(p, '+');
            if (build_ptr) {
                *build_ptr = '\0';
                out->prerelease = semver_strdup(p);
                out->build = semver_strdup(build_ptr + 1);
            } else {
                out->prerelease = semver_strdup(p);
            }
        } else if (type == '+') {
            out->build = semver_strdup(p);
        }
    }

    free(mutable_version);
    return 0;
}

// Compares two prerelease strings.
// Returns -1 if p1 < p2, 0 if p1 == p2, 1 if p1 > p2.
// Implements SemVer 2.0.0 section 11.
static int compare_prerelease(const char *p1, const char *p2) {
    if (!p1 && !p2) return 0;
    if (!p1) return 1; // A version with a prerelease is lower than one without
    if (!p2) return -1; // A version without a prerelease is higher than one with

    char *s1 = semver_strdup(p1);
    char *s2 = semver_strdup(p2);
    if (!s1 || !s2) {
        free(s1); free(s2);
        return 0; // Error or memory allocation failure
    }

    char *token1, *token2;
    char *rest1 = s1, *rest2 = s2;
    int cmp = 0;

    while (true) {
        token1 = strtok_r(rest1, ".", &rest1);
        token2 = strtok_r(rest2, ".", &rest2);

        if (!token1 && !token2) {
            cmp = 0;
            break;
        }
        if (!token1) { // p1 is a subset of p2 (e.g., alpha < alpha.1)
            cmp = -1;
            break;
        }
        if (!token2) { // p2 is a subset of p1
            cmp = 1;
            break;
        }

        bool is_num1 = true, is_num2 = true;
        for (char *c = token1; *c; c++) { if (!isdigit(*c)) { is_num1 = false; break; } }
        for (char *c = token2; *c; c++) { if (!isdigit(*c)) { is_num2 = false; break; } }

        if (is_num1 && is_num2) {
            int n1 = atoi(token1);
            int n2 = atoi(token2);
            if (n1 < n2) { cmp = -1; break; }
            if (n1 > n2) { cmp = 1; break; }
        } else if (is_num1 && !is_num2) { // Numeric identifiers have lower precedence than non-numeric
            cmp = -1;
            break;
        } else if (!is_num1 && is_num2) {
            cmp = 1;
            break;
        } else { // Both are non-numeric
            cmp = strcmp(token1, token2);
            if (cmp != 0) break;
        }
    }

    free(s1);
    free(s2);
    return cmp;
}


// Returns true if version matches the grouping constraint.
// e.g., 1.0.5 matches 1.0 (major=1, minor=0, patch=-1)
bool semver_match(const semver_t version, const semver_t constraint) {
    if (constraint.major != -1 && version.major != constraint.major) return false;
    if (constraint.minor != -1 && version.minor != constraint.minor) return false;
    if (constraint.patch != -1 && version.patch != constraint.patch) return false;
    // For prerelease/build metadata, we usually expect them to be empty if the constraint is wild
    // but the user requirement was specifically about major.minor grouping.
    return true;
}

// Compares two semver_t structs absolutely. 
// Returns -1 if v1 < v2, 0 if v1 == v2, 1 if v1 > v2.
int semver_compare(const semver_t v1, const semver_t v2) {
    if (v1.major != v2.major) return v1.major > v2.major ? 1 : -1;
    if (v1.minor != v2.minor) return v1.minor > v2.minor ? 1 : -1;
    if (v1.patch != v2.patch) return v1.patch > v2.patch ? 1 : -1;

    int prerelease_cmp = compare_prerelease(v1.prerelease, v2.prerelease);
    if (prerelease_cmp != 0) return prerelease_cmp;

    // Build metadata is ignored when determining version precedence.
    return 0;
}

static bool semver_comp_match(const semver_t v, const semver_comp_t comp) {
    // SemVer 2.0.0 rule: if v has prerelease, it only matches if it has same major.minor.patch
    // as comp.version. (This is a simplified version of node-semver's rule)
    if (v.prerelease) {
        if (v.major != comp.version.major || v.minor != comp.version.minor || v.patch != comp.version.patch) {
            return false;
        }
    }

    int cmp = semver_compare(v, comp.version);
    switch (comp.op) {
        case SV_OP_EQ:
            if (comp.version.major != -1 && v.major != comp.version.major) return false;
            if (comp.version.minor != -1 && v.minor != comp.version.minor) return false;
            if (comp.version.patch != -1 && v.patch != comp.version.patch) return false;
            return true;
        case SV_OP_GT: return cmp > 0;
        case SV_OP_GTE: return cmp >= 0;
        case SV_OP_LT: return cmp < 0;
        case SV_OP_LTE: return cmp <= 0;
        case SV_OP_CARET:
            if (cmp < 0) return false;
            if (comp.version.major > 0) {
                return v.major == comp.version.major;
            } else if (comp.version.minor > 0) {
                return v.major == 0 && v.minor == comp.version.minor;
            } else if (comp.version.patch > 0) {
                return v.major == 0 && v.minor == 0 && v.patch == comp.version.patch;
            } else {
                if (comp.version.minor == 0) {
                    if (comp.version.patch == 0) {
                        return v.major == 0 && v.minor == 0 && v.patch == 0;
                    } else if (comp.version.patch == -1) {
                        return v.major == 0 && v.minor == 0;
                    }
                } else if (comp.version.minor == -1) {
                    return v.major == 0;
                }
                return v.major == 0 && v.minor == 0;
            }
        case SV_OP_TILDE:
            if (cmp < 0) return false;
            if (comp.version.minor != -1) {
                return v.major == comp.version.major && v.minor == comp.version.minor;
            } else {
                return v.major == comp.version.major;
            }
        default: return false;
    }
}

bool semver_range_match(const semver_t version, const semver_range_t range) {
    for (int i = 0; i < range.count; i++) {
        semver_comp_set_t *set = &range.sets[i];
        bool set_matches = true;
        for (int j = 0; j < set->count; j++) {
            if (!semver_comp_match(version, set->comps[j])) {
                set_matches = false;
                break;
            }
        }
        if (set_matches) return true;
    }
    return false;
}

int semver_range_parse(const char *range_str, semver_range_t *out) {
    if (!range_str || !out) return -1;
    memset(out, 0, sizeof(semver_range_t));

    char *mutable_range = semver_strdup(range_str);
    if (!mutable_range) return -1;

    char *p = mutable_range;
    while (p) {
        char *next_set = strstr(p, "||");
        if (next_set) {
            *next_set = '\0';
            next_set += 2;
        }

        // Trim whitespace
        while (isspace(*p)) p++;
        char *end = p + strlen(p) - 1;
        while (end >= p && isspace(*end)) *end-- = '\0';

        if (*p) {
            out->sets = realloc(out->sets, sizeof(semver_comp_set_t) * (out->count + 1));
            semver_comp_set_t *set = &out->sets[out->count++];
            memset(set, 0, sizeof(semver_comp_set_t));

            char *mutable_set = semver_strdup(p);
            char *saveptr;
            char *token = strtok_r(mutable_set, " ", &saveptr);
            while (token) {
                if (*token) {
                    set->comps = realloc(set->comps, sizeof(semver_comp_t) * (set->count + 1));
                    semver_comp_t *comp = &set->comps[set->count++];
                    memset(comp, 0, sizeof(semver_comp_t));

                    if (strncmp(token, ">=", 2) == 0) {
                        comp->op = SV_OP_GTE;
                        token += 2;
                    } else if (strncmp(token, "<=", 2) == 0) {
                        comp->op = SV_OP_LTE;
                        token += 2;
                    } else if (*token == '>') {
                        comp->op = SV_OP_GT;
                        token += 1;
                    } else if (*token == '<') {
                        comp->op = SV_OP_LT;
                        token += 1;
                    } else if (*token == '^') {
                        comp->op = SV_OP_CARET;
                        token += 1;
                    } else if (*token == '~') {
                        comp->op = SV_OP_TILDE;
                        token += 1;
                    } else if (*token == '=') {
                        comp->op = SV_OP_EQ;
                        token += 1;
                    } else {
                        comp->op = SV_OP_EQ;
                    }

                    // Handle space between operator and version
                    if (*token == '\0') {
                        token = strtok_r(NULL, " ", &saveptr);
                        if (!token) {
                            // Error: operator without version
                            free(mutable_set);
                            free(mutable_range);
                            semver_range_free(out);
                            return -1;
                        }
                    }

                    if (semver_parse(token, &comp->version) != 0) {
                        free(mutable_set);
                        free(mutable_range);
                        semver_range_free(out);
                        return -1;
                    }
                }
                token = strtok_r(NULL, " ", &saveptr);
            }
            free(mutable_set);
        }
        p = next_set;
    }

    free(mutable_range);
    return 0;
}

void semver_range_free(semver_range_t *range) {
    if (!range) return;
    for (int i = 0; i < range->count; i++) {
        semver_comp_set_t *set = &range->sets[i];
        for (int j = 0; j < set->count; j++) {
            semver_free(&set->comps[j].version);
        }
        free(set->comps);
    }
    free(range->sets);
    memset(range, 0, sizeof(semver_range_t));
}

// Frees dynamically allocated memory within a semver_t struct.
void semver_free(semver_t *sv) {
    if (sv) {
        free(sv->prerelease);
        free(sv->build);
        memset(sv, 0, sizeof(semver_t)); // Clear the struct
    }
}
