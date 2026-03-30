/* SPDX-License-Identifier: MIT */
#include "resolver.h"
#include "cache.h"
#include "cmod.h"
#include "semver.h" 
#include "fetcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define INITIAL_CAPACITY 64
#define MAX_DEPTH 64

// Helper to duplicate strings safely
static char *semver_strdup(const char *s) {
    return s ? strdup(s) : NULL;
}



// module_node_t functions
void module_node_init(module_node_t *node) {
    memset(node, 0, sizeof(module_node_t));
    node->requirements_cap = INITIAL_CAPACITY;
    node->requirements = calloc(node->requirements_cap, sizeof(module_requirement_t));
    if (!node->requirements) {
        // Handle error: TODO
    }
}

void module_node_free(module_node_t *node) {
    if (node) {
        free(node->module_path);
        free(node->resolved_version_str);
        semver_free(&node->resolved_version);
        for (size_t i = 0; i < node->requirements_len; i++) {
            free(node->requirements[i].dependent_module_path);
            free(node->requirements[i].version);
        }
        free(node->requirements);
        memset(node, 0, sizeof(module_node_t));
    }
}

int module_node_add_requirement(module_node_t *node, const char *dependent_path, const char *version) {
    if (!node || !dependent_path || !version) return -1;

    if (node->requirements_len >= node->requirements_cap) {
        node->requirements_cap *= 2;
        module_requirement_t *new_reqs = realloc(node->requirements, node->requirements_cap * sizeof(module_requirement_t));
        if (!new_reqs) return -1; // Realloc failed
        node->requirements = new_reqs;
    }

    module_requirement_t *req = &node->requirements[node->requirements_len];
    req->dependent_module_path = semver_strdup(dependent_path);
    req->version = semver_strdup(version);
    node->requirements_len++;
    return 0;
}

// dependency_graph_t functions
void dependency_graph_init(dependency_graph_t *graph) {
    memset(graph, 0, sizeof(dependency_graph_t));
    graph->nodes_cap = INITIAL_CAPACITY;
    graph->nodes = calloc(graph->nodes_cap, sizeof(module_node_t));
    if (!graph->nodes) {
        // Handle error: TODO
    }
}

void dependency_graph_free(dependency_graph_t *graph) {
    if (graph) {
        for (size_t i = 0; i < graph->nodes_len; i++) {
            module_node_free(&graph->nodes[i]);
        }
        free(graph->nodes);
        memset(graph, 0, sizeof(dependency_graph_t));
    }
}

module_node_t *dependency_graph_add_node(dependency_graph_t *graph, const char *module_path, bool is_root_dep) {
    if (!graph || !module_path) return NULL;

    // Check if node already exists
    module_node_t *node = dependency_graph_find_node(graph, module_path);
    if (node) {
        if (is_root_dep) node->is_root_dependency = true;
        return node;
    }

    // Add new node
    if (graph->nodes_len >= graph->nodes_cap) {
        graph->nodes_cap *= 2;
        module_node_t *new_nodes = realloc(graph->nodes, graph->nodes_cap * sizeof(module_node_t));
        if (!new_nodes) return NULL;
        graph->nodes = new_nodes;
    }

    node = &graph->nodes[graph->nodes_len];
    module_node_init(node);
    node->module_path = semver_strdup(module_path);
    node->is_root_dependency = is_root_dep;
    graph->nodes_len++;
    return node;
}

module_node_t *dependency_graph_find_node(dependency_graph_t *graph, const char *module_path) {
    if (!graph || !module_path) return NULL;
    for (size_t i = 0; i < graph->nodes_len; i++) {
        if (strcmp(graph->nodes[i].module_path, module_path) == 0) {
            return &graph->nodes[i];
        }
    }
    return NULL;
}

static char *resolve_version_group(const char *module_path, const char *version_constraint) {
    if (!version_constraint) return semver_strdup("latest");
    
    // Check if it's a local module (starts with '.' or '/')
    int is_local = (module_path[0] == '/' || (module_path[0] == '.' && module_path[1] == '/'));
    if (is_local) return semver_strdup(version_constraint);

    semver_t constraint;
    if (semver_parse(version_constraint, &constraint) != 0) {
        return semver_strdup(version_constraint); // Not a semver (branch/commit)
    }
    
    if (constraint.minor != -1 && constraint.patch != -1) {
        // Fully specified, no group matching needed
        semver_free(&constraint);
        return semver_strdup(version_constraint);
    }
    
    // Partial version, need to find the best match from tags
    char **tags = NULL;
    size_t count = 0;
    if (get_all_tags(module_path, &tags, &count) != 0) {
        semver_free(&constraint);
        return semver_strdup(version_constraint); // Fallback to raw if git fails
    }
    
    char *best_tag = NULL;
    semver_t best_sv;
    memset(&best_sv, 0, sizeof(semver_t));
    best_sv.major = -1; best_sv.minor = -1; best_sv.patch = -1;
    
    for (size_t i = 0; i < count; i++) {
        semver_t tag_sv;
        if (semver_parse(tags[i], &tag_sv) == 0) {
            if (semver_match(tag_sv, constraint)) {
                if (best_tag == NULL || semver_compare(tag_sv, best_sv) > 0) {
                    semver_free(&best_sv);
                    // Deep copy tag_sv to best_sv
                    best_sv.major = tag_sv.major;
                    best_sv.minor = tag_sv.minor;
                    best_sv.patch = tag_sv.patch;
                    best_sv.prerelease = tag_sv.prerelease ? semver_strdup(tag_sv.prerelease) : NULL;
                    best_sv.build = tag_sv.build ? semver_strdup(tag_sv.build) : NULL;
                    
                    if (best_tag) free(best_tag);
                    best_tag = strdup(tags[i]);
                }
            }
            semver_free(&tag_sv);
        }
    }
    
    // Cleanup
    for (size_t i = 0; i < count; i++) free(tags[i]);
    free(tags);
    semver_free(&constraint);
    semver_free(&best_sv);
    
    return best_tag ? best_tag : semver_strdup(version_constraint);
}


// Function to collect all available tags for a module



static int collect_all_requirements(
    const char *module_path,
    const char *version,        // The exact version we need to fetch
    const char *dependent_path, // The module that declared this dependency
    dependency_graph_t *graph,
    int depth
) {
    if (depth > MAX_DEPTH) {
        fprintf(stderr, "Error: maximum dependency depth exceeded (possible cycle) for %s\n", module_path);
        return -1;
    }

    module_node_t *current_module_node = dependency_graph_add_node(graph, module_path, false);
    if (!current_module_node) {
        fprintf(stderr, "Error: Failed to add module node %s to graph.\n", module_path);
        return -1;
    }

    if (dependent_path && version) {
        char *resolved_version = resolve_version_group(module_path, version);
        if (module_node_add_requirement(current_module_node, dependent_path, resolved_version) != 0) {
            fprintf(stderr, "Error: Failed to add requirement for %s to node %s.\n", resolved_version, module_path);
            free(resolved_version);
            return -1;
        }
        free(resolved_version);
    }

    if (current_module_node->is_resolved) { 
        return 0;
    }

    // Determine a version to fetch cmod.toml from.
    // In MVS we just fetch the EXACT version requested to find transitive dependencies.
    char *version_to_fetch = resolve_version_group(module_path, version);
    
    // Check if it's a local module (starts with '.' or '/')
    int is_local = (module_path[0] == '/' || (module_path[0] == '.' && module_path[1] == '/'));
    char cmod_path[512];

    if (is_local) {
        snprintf(cmod_path, sizeof(cmod_path), "%s/cmod.toml", module_path);
    } else {
        char *cached_path = cache_path(module_path, version_to_fetch);
        if (ensure_cached(module_path, version_to_fetch) != 0) {
            fprintf(stderr, "Warning: failed to cache %s@%s. Skipping internal dependencies.\n", module_path, version_to_fetch);
            free(cached_path);
            free(version_to_fetch);
            return 0;
        }
        snprintf(cmod_path, sizeof(cmod_path), "%s/cmod.toml", cached_path);
        free(cached_path);
    }
    
    cmod_t dep_mod;
    if (cmod_load(cmod_path, &dep_mod) != 0) {
        fprintf(stderr, "Debug: No cmod.toml found or failed to load for %s@%s. Assuming no internal dependencies.\n", module_path, version_to_fetch);
        free(version_to_fetch);
        return 0;
    }
    free(version_to_fetch); 

    for (size_t i = 0; i < dep_mod.requires_len; i++) {
        const char *next_dep_path = dep_mod.requires[i].path;
        const char *next_dep_constraint = dep_mod.requires[i].constraint;
        
        int rc = collect_all_requirements(
            next_dep_path,
            next_dep_constraint,
            module_path, 
            graph,
            depth + 1
        );
        
        if (rc < 0) {
            cmod_free(&dep_mod);
            return -1;
        }
    }
    
    cmod_free(&dep_mod);
    return 0;
}

// resolve_single is now an internal helper for getting tags.
// The public resolver should use the graph-based approach.

static int resolve_mvs_graph(dependency_graph_t *graph, resolved_t **out) {
    if (!graph || !out) return -1;
    
    size_t count = 0;
    size_t capacity = INITIAL_CAPACITY;
    resolved_t *list = calloc(capacity, sizeof(resolved_t));
    if (!list) return -1;

    // Phase 2: For each module in the graph, find the max requested version across all dependents.
    for (size_t i = 0; i < graph->nodes_len; i++) {
        module_node_t *node = &graph->nodes[i];
        
        // MVS: Take the max version out of all explicitly requested versions
        semver_t best_version = {0};
        char *best_raw = NULL;
        bool has_semver = false;

        for (size_t k = 0; k < node->requirements_len; k++) {
            semver_t current;
            if (semver_parse(node->requirements[k].version, &current) == 0) {
                if (!has_semver || semver_compare(current, best_version) > 0) {
                    if (has_semver) semver_free(&best_version);
                    memcpy(&best_version, &current, sizeof(semver_t));
                    best_version.prerelease = semver_strdup(current.prerelease);
                    best_version.build = semver_strdup(current.build);
                    best_raw = node->requirements[k].version;
                    has_semver = true;
                } else {
                    semver_free(&current);
                }
            } else {
                // Not semver (e.g., 'main', or commit hash). Just fallback to use it if nothing better
                if (!best_raw && !has_semver) {
                    best_raw = node->requirements[k].version;
                }
            }
        }

        if (best_raw) {
            node->resolved_version_str = semver_strdup(best_raw);
        } else {
            // Default edge case
            node->resolved_version_str = semver_strdup("latest");
        }

        if (has_semver) {
            semver_free(&best_version);
        }
        
        node->is_resolved = true;
        
        if (count >= capacity) {
            capacity *= 2;
            resolved_t *new_list = realloc(list, capacity * sizeof(resolved_t));
            if (!new_list) { /* handle error */ return -1; }
            list = new_list;
        }
        list[count].module = semver_strdup(node->module_path);
        list[count].version = semver_strdup(node->resolved_version_str);
        count++;
    }

    *out = list;
    return (int)count;
}


int resolve_dependencies(cmod_t *mod, resolved_t **out) {
    if (!mod || !out) return -1;
    
    dependency_graph_t graph;
    dependency_graph_init(&graph);

    // Phase 1: Collect all requirements
    for (size_t i = 0; i < mod->requires_len; i++) {
        const char *req_path = mod->requires[i].path;
        const char *req_constraint = mod->requires[i].constraint;
        
        // Add root dependency to graph
        module_node_t *root_dep_node = dependency_graph_add_node(&graph, req_path, true);
        if (!root_dep_node) {
            dependency_graph_free(&graph);
            return -1;
        }
        // Resolve root dependency constraint immediately
        char *resolved_constraint = resolve_version_group(req_path, req_constraint);
        if (!resolved_constraint) {
            dependency_graph_free(&graph);
            return -1;
        }

        // Add the root's requirement to itself for resolution
        if (module_node_add_requirement(root_dep_node, mod->module, resolved_constraint) != 0) {
            free(resolved_constraint);
            dependency_graph_free(&graph);
            return -1;
        }

        // Recursively collect all transitive dependencies
        int rc = collect_all_requirements(
            req_path,
            resolved_constraint,
            mod->module, // The root module is the dependent
            &graph,
            0
        );
        free(resolved_constraint);
        
        if (rc < 0) {
            dependency_graph_free(&graph);
            return -1;
        }
    }

    // Phase 2: Resolve the graph using MVS
    int resolved_count = resolve_mvs_graph(&graph, out);

    dependency_graph_free(&graph); // Free the graph after resolution
    
    return resolved_count;
}

void resolved_free(resolved_t *list, size_t n) {
    if (!list) return;
    for (size_t i = 0; i < n; i++) {
        free(list[i].module);
        free(list[i].version);
    }
    free(list);
}