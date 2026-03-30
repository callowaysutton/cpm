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

// Compares two semver_t structs.
// Returns -1 if v1 < v2, 0 if v1 == v2, 1 if v1 > v2.
int semver_compare(const semver_t v1, const semver_t v2);

// Frees dynamically allocated memory within a semver_t struct.
void semver_free(semver_t *sv);

#endif // CPM_SEMVER_H
