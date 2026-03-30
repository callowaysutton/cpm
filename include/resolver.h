/* SPDX-License-Identifier: TODO */
#ifndef CPM_RESOLVER_H
#define CPM_RESOLVER_H

#include "cmod.h"
#include "semver.h"
#include <stdbool.h>

// Represents a requirement for a module from another module
typedef struct {
    char *dependent_module_path; // Path of the module that has this requirement
    char *version;               // Required min version (e.g. "1.2.3" or hash)
} module_requirement_t;

// Represents a unique module in the dependency graph
typedef struct {
    char *module_path;                       // e.g., "github.com/user/lib"
    module_requirement_t *requirements;      // Array of all requirements for this module
    size_t requirements_len;
    size_t requirements_cap;
    char *resolved_version_str;              // The final resolved version string (e.g., "v1.2.5")
    semver_t resolved_version;               // The final resolved version struct
    bool is_resolved;
    bool is_root_dependency;                 // True if this is a direct dependency of the root module
} module_node_t;

// The entire dependency graph
typedef struct {
    module_node_t *nodes;                    // Array of unique module nodes
    size_t nodes_len;
    size_t nodes_cap;
} dependency_graph_t;

// The final output format, remains the same
typedef struct {
    char *module;
    char *version;
} resolved_t;



// Initializes a module_node_t struct.
void module_node_init(module_node_t *node);

// Frees dynamically allocated memory within a module_node_t struct.
void module_node_free(module_node_t *node);

// Adds a requirement to a module_node_t.
// Returns 0 on success, -1 on error.
int module_node_add_requirement(module_node_t *node, const char *dependent_path, const char *version);

// Initializes a dependency_graph_t struct.
void dependency_graph_init(dependency_graph_t *graph);

// Frees dynamically allocated memory within a dependency_graph_t struct.
void dependency_graph_free(dependency_graph_t *graph);

// Adds a module node to the dependency graph.
// If the module already exists, returns a pointer to the existing node.
// Returns a pointer to the newly added or existing node on success, NULL on error.
module_node_t *dependency_graph_add_node(dependency_graph_t *graph, const char *module_path, bool is_root_dep);

// Finds a module node in the dependency graph by its path.
// Returns a pointer to the node if found, NULL otherwise.
module_node_t *dependency_graph_find_node(dependency_graph_t *graph, const char *module_path);


/**
 * Resolve all dependencies for a module, returning an ordered list
 * of modules to download. The caller must free the array and its elements.
 *
 * Returns number of resolved modules on success, -1 on error.
 */
int resolve_dependencies(cmod_t *mod, resolved_t **out);

/**
 * Free a resolved list returned by resolve_dependencies.
 */
void resolved_free(resolved_t *list, size_t n);

#endif // CPM_RESOLVER_H