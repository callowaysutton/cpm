/* SPDX-License-Identifier: TODO */
#ifndef CPM_FETCHER_H
#define CPM_FETCHER_H

#include <stddef.h>

/**
 * Clone a git repository for the given module path.
 *
 * `module_path` is a git URL or a bare path like "github.com/user/lib".
 * `ref` is either a tag name (e.g. "v1.2.3") or a commit SHA prefixed with "commit:".
 * `dest_dir` is the directory where the repository will be cloned (must not exist).
 * Returns 0 on success, non‑zero on failure.
 */
int fetch_module(const char *module_path, const char *ref, const char *dest_dir);

/**
 * Fetch all available tags for the given module path using git ls-remote.
 * The tags are sorted and unique. Caller must free the tags array and its contents.
 * Returns 0 on success, non-zero on failure.
 */
int get_all_tags(const char *module_path, char ***out_tags, size_t *out_count);

/**
 * Resolves a given branch or tag ref into its full 40-character commit SHA.
 * Returns 0 on success, non-zero on failure.
 * The caller must free `*out_commit` if success.
 */
int get_commit_for_ref(const char *module_path, const char *ref, char **out_commit);

#endif // CPM_FETCHER_H
