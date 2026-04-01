#ifndef CPM_SEMVER_H
#define CPM_SEMVER_H

#include <stdbool.h>

// Semantic Versioning 2.0.0 components
typedef struct {
    int major;
    int minor;
    int patch;
    char *prerelease; // e.g., "alpha.1", "beta.2"
    char *build;      // e.g., "20240329", "commit.abcdef"
} semver_t;

// Parses a version string into a semver_t struct.
// Returns 0 on success, -1 on error.
// Caller is responsible for freeing prerelease and build fields.
int semver_parse(const char *version_str, semver_t *out);

// Returns true if version matches the grouping constraint.
bool semver_match(const semver_t version, const semver_t constraint);

// Compares two semver_t structs.
// Returns -1 if v1 < v2, 0 if v1 == v2, 1 if v1 > v2.
int semver_compare(const semver_t v1, const semver_t v2);

// Semantic Version Range support
typedef enum {
    SV_OP_EQ,
    SV_OP_GT,
    SV_OP_GTE,
    SV_OP_LT,
    SV_OP_LTE,
    SV_OP_CARET,
    SV_OP_TILDE,
} semver_op_t;

typedef struct {
    semver_op_t op;
    semver_t version;
} semver_comp_t;

typedef struct {
    semver_comp_t *comps;
    int count;
} semver_comp_set_t;

typedef struct {
    semver_comp_set_t *sets;
    int count;
} semver_range_t;

// Parses a range string (e.g., "^1.2.3 || >=2.0.0") into a semver_range_t.
// Returns 0 on success, -1 on error.
int semver_range_parse(const char *range_str, semver_range_t *out);

// Returns true if version matches the range.
bool semver_range_match(const semver_t version, const semver_range_t range);

// Frees dynamically allocated memory within a semver_range_t struct.
void semver_range_free(semver_range_t *range);

// Frees dynamically allocated memory within a semver_t struct.
void semver_free(semver_t *sv);

#endif // CPM_SEMVER_H
