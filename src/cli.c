/* SPDX-License-Identifier: MIT */
#include "cli.h"
#include "cmod.h"
#include "resolver.h"
#include "cache.h"
#include "vendor.h"
#include "generator.h"
#include "semver.h"
#include "fetcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

static char *sanitize(const char *s) {
    char *out = strdup(s);
    for (char *p = out; *p; ++p) {
        if (*p == '/' || *p == '@' || *p == ':') *p = '_';
    }
    return out;
}

/* Helper: create a default cmod_t for init */
static cmod_t *default_module(void) {
    cmod_t *mod = calloc(1, sizeof(cmod_t));
    if (!mod) return NULL;
    // Infer module path from git remote if possible; otherwise use placeholder.
    FILE *p = popen("git rev-parse --show-toplevel 2>/dev/null", "r");
    if (p) {
        char cwd[1024];
        if (fgets(cwd, sizeof(cwd), p)) {
            // Trim newline
            cwd[strcspn(cwd, "\n")] = '\0';
            // Try to get remote URL
            FILE *p2 = popen("git config --get remote.origin.url 2>/dev/null", "r");
            if (p2) {
                char url[1024];
                if (fgets(url, sizeof(url), p2)) {
                    url[strcspn(url, "\n")] = '\0';
                    // Strip possible .git suffix
                    size_t len = strlen(url);
                    if (len > 4 && strcmp(url + len - 4, ".git") == 0) {
                        url[len - 4] = '\0';
                    }
                    mod->module = strdup(url);
                }
                pclose(p2);
            }
        }
        pclose(p);
    }
    if (!mod->module) {
        mod->module = strdup("example.com/your/module");
    }
    mod->version = strdup("v0.1.0");
    return mod;
}

static char *resolve_cli_version(const char *module, const char *requested) {
    if (!requested) return strdup("latest");
    
    char **tags = NULL;
    size_t num_tags = 0;
    
    if (get_all_tags(module, &tags, &num_tags) != 0) {
        return strdup(requested);
    }
    
    semver_t best_sv = {0};
    char *best_raw = NULL;
    int is_latest = (strcmp(requested, "latest") == 0);
    
    for (size_t i = 0; i < num_tags; i++) {
        semver_t sv;
        if (semver_parse(tags[i], &sv) == 0) {
            int match = 0;
            if (is_latest) {
                match = 1;
            } else {
                const char *r_cmp = requested;
                if (r_cmp[0] == 'v' || r_cmp[0] == 'V') r_cmp++;
                const char *t_cmp = tags[i];
                if (t_cmp[0] == 'v' || t_cmp[0] == 'V') t_cmp++;
                
                if (strcmp(r_cmp, t_cmp) == 0) {
                    match = 1;
                } else {
                    size_t r_len = strlen(r_cmp);
                    if (strncmp(r_cmp, t_cmp, r_len) == 0) {
                        if (t_cmp[r_len] == '.') match = 1;
                        if (r_len > 0 && r_cmp[r_len-1] == '.') match = 1;
                    }
                }
            }
            
            if (match) {
                if (!best_raw || semver_compare(sv, best_sv) > 0) {
                    if (best_raw) semver_free(&best_sv);
                    best_raw = strdup(tags[i]);
                    best_sv.major = sv.major;
                    best_sv.minor = sv.minor;
                    best_sv.patch = sv.patch;
                    best_sv.prerelease = sv.prerelease ? strdup(sv.prerelease) : NULL;
                    best_sv.build = sv.build ? strdup(sv.build) : NULL;
                }
            }
            semver_free(&sv);
        }
    }
    
    for (size_t i = 0; i < num_tags; i++) free(tags[i]);
    free(tags);
    
    if (best_raw) {
        semver_free(&best_sv);
        return best_raw;
    }

    if (strncmp(requested, "commit:", 7) == 0) {
        return strdup(requested);
    }
    
    const char *ref_to_query = requested;
    if (strcmp(requested, "latest") == 0) {
        ref_to_query = "HEAD";
    }
    
    char *commit = NULL;
    if (get_commit_for_ref(module, ref_to_query, &commit) == 0 && commit) {
        char *pseudo;
        asprintf(&pseudo, "commit:%s", commit);
        free(commit);
        return pseudo;
    }
    
    return strdup(requested);
}

void print_help(const char *prog);

int cpm_dispatch(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "cpm: missing command\n");
        return EXIT_FAILURE;
    }
    const char *cmd = argv[1];

    if (strcmp(cmd, "init") == 0) {
        cmod_t *mod = default_module();
        if (!mod) {
            fprintf(stderr, "cpm: failed to allocate module\n");
            return EXIT_FAILURE;
        }
        if (cmod_save("cmod.toml", mod) != 0) {
            fprintf(stderr, "cpm: failed to write cmod.toml\n");
            cmod_free(mod);
            return EXIT_FAILURE;
        }
        printf("cmod.toml created (module=%s version=%s)\n", mod->module, mod->version);
        cmod_free(mod);
        return EXIT_SUCCESS;
    }

    if (strcmp(cmd, "get") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: cpm get <module>@<version>\n");
            return EXIT_FAILURE;
        }
        const char *arg = argv[2];
        char *at = strchr(arg, '@');
        if (!at) {
            fprintf(stderr, "Error: expected <module>@<version>\n");
            return EXIT_FAILURE;
        }
        *at = '\0';
        const char *module = arg;
        const char *version = at + 1;

        // Load existing cmod.toml or create new
        cmod_t mod;
        if (cmod_load("cmod.toml", &mod) != 0) {
            fprintf(stderr, "Warning: creating new cmod.toml\n");
            cmod_t *tmp = default_module();
            if (!tmp) {
                fprintf(stderr, "Failed to default module\n");
                return EXIT_FAILURE;
            }
            mod = *tmp;
            free(tmp);
        }

        // Expand version
        char *expanded_version = resolve_cli_version(module, version);
        printf("Resolved %s@%s to %s\n", module, version, expanded_version);

        // Add or update the requirement
        int found = 0;
        for (size_t i = 0; i < mod.requires_len; ++i) {
            if (strcmp(mod.requires[i].path, module) == 0) {
                free(mod.requires[i].constraint);
                mod.requires[i].constraint = expanded_version;
                found = 1;
                break;
            }
        }
        if (!found) {
            require_t *new_arr = realloc(mod.requires, (mod.requires_len + 1) * sizeof(require_t));
            if (!new_arr) { fprintf(stderr, "Out of memory\n"); return EXIT_FAILURE; }
            mod.requires = new_arr;
            mod.requires[mod.requires_len].path = strdup(module);
            mod.requires[mod.requires_len].constraint = expanded_version;
            mod.requires_len++;
        }

        // Save updated cmod.toml
        if (cmod_save("cmod.toml", &mod) != 0) {
            fprintf(stderr, "Failed to write cmod.toml\n");
            cmod_free(&mod);
            return EXIT_FAILURE;
        }

        // Resolve and cache
        resolved_t *list = NULL;
        int n = resolve_dependencies(&mod, &list);
        if (n < 0) {
            fprintf(stderr, "Failed to resolve dependencies\n");
            cmod_free(&mod);
            return EXIT_FAILURE;
        }

        printf("Resolving %d dependencies:\n", n);
        for (int i = 0; i < n; ++i) {
            printf("  %s@%s\n", list[i].module, list[i].version);
            if (ensure_cached(list[i].module, list[i].version) != 0) {
                fprintf(stderr, "Failed to cache %s@%s\n", list[i].module, list[i].version);
            }
        }

        // Vendor
        if (vendor_populate(list, n) != 0) {
            fprintf(stderr, "Warning: vendor population failed\n");
        } else {
            printf("Vendor directory populated.\n");
        }

        // Generate bindings
        if (generate_all(&mod, list, n) != 0) {
            fprintf(stderr, "Warning: generator failed\n");
        }

        for (int i = 0; i < n; ++i) {
            free(list[i].module);
            free(list[i].version);
        }
        free(list);
        cmod_free(&mod);
        return EXIT_SUCCESS;
    }

    if (strcmp(cmd, "tidy") == 0) {
        cmod_t mod;
        if (cmod_load("cmod.toml", &mod) != 0) {
            fprintf(stderr, "No cmod.toml found\n");
            return EXIT_FAILURE;
        }
        
        resolved_t *list = NULL;
        int n = resolve_dependencies(&mod, &list);
        if (n < 0) {
            fprintf(stderr, "Failed to resolve dependencies\n");
            cmod_free(&mod);
            return EXIT_FAILURE;
        }
        
        printf("Removing unused vendored modules...\n");
        
        DIR *vendor_dir = opendir(".cpm/vendor");
        if (vendor_dir) {
            struct dirent *entry;
            while ((entry = readdir(vendor_dir)) != NULL) {
                if (entry->d_name[0] == '.') continue;
                
                int found = 0;
                for (int i = 0; i < n; i++) {
                    char *clean_mod = sanitize(list[i].module);
                    char *clean_ver = sanitize(list[i].version);
                    char expected[512];
                    snprintf(expected, sizeof(expected), "%s@%s", clean_mod, clean_ver);
                    free(clean_mod);
                    free(clean_ver);
                    
                    if (strcmp(entry->d_name, expected) == 0) {
                        found = 1;
                        break;
                    }
                }
                
                if (!found) {
                    char path[1024];
                    snprintf(path, sizeof(path), ".cpm/vendor/%s", entry->d_name);
                    printf("  Removing %s\n", entry->d_name);
                    char cmd[2048];
                    snprintf(cmd, sizeof(cmd), "rm -rf %s", path);
                    system(cmd);
                }
            }
            closedir(vendor_dir);
        }
        
        generate_all(&mod, list, n);
        printf("Done. Run 'make' (or corresponding build tool) to rebuild.\n");
        
        for (int i = 0; i < n; i++) {
            free(list[i].module);
            free(list[i].version);
        }
        free(list);
        cmod_free(&mod);
        return EXIT_SUCCESS;
    }

    if (strcmp(cmd, "vendor") == 0) {
        cmod_t mod;
        if (cmod_load("cmod.toml", &mod) != 0) {
            fprintf(stderr, "No cmod.toml found\n");
            return EXIT_FAILURE;
        }
        resolved_t *list = NULL;
        int n = resolve_dependencies(&mod, &list);
        if (n < 0) {
            fprintf(stderr, "Failed to resolve dependencies\n");
            cmod_free(&mod);
            return EXIT_FAILURE;
        }
        if (vendor_populate(list, n) != 0) {
            fprintf(stderr, "Vendor population failed\n");
        } else {
            printf("Vendor directory populated.\n");
        }
        generate_all(&mod, list, n);
        for (int i = 0; i < n; ++i) {
            free(list[i].module);
            free(list[i].version);
        }
        free(list);
        cmod_free(&mod);
        return EXIT_SUCCESS;
    }

    if (strcmp(cmd, "build") == 0) {
        cmod_t mod;
        if (cmod_load("cmod.toml", &mod) != 0) {
            fprintf(stderr, "No cmod.toml found\n");
            return EXIT_FAILURE;
        }
        resolved_t *list = NULL;
        int n = resolve_dependencies(&mod, &list);
        if (n < 0) {
            fprintf(stderr, "Failed to resolve dependencies\n");
            cmod_free(&mod);
            return EXIT_FAILURE;
        }

        // Build dependencies
        for (int i = 0; i < n; ++i) {
            char path[1024];
            snprintf(path, sizeof(path), ".cpm/vendor/%s@%s/cmod.toml", sanitize(list[i].module), sanitize(list[i].version));
            cmod_t dep_mod;
            if (cmod_load(path, &dep_mod) == 0) {
                if (dep_mod.build_command) {
                    char build_cmd[2048];
                    snprintf(build_cmd, sizeof(build_cmd), "cd .cpm/vendor/%s@%s && %s", sanitize(list[i].module), sanitize(list[i].version), dep_mod.build_command);
                    printf("Building dependency %s@%s...\n", list[i].module, list[i].version);
                    int ret = system(build_cmd);
                    if (ret != 0) {
                        fprintf(stderr, "Failed to build %s@%s\n", list[i].module, list[i].version);
                    }
                }
                cmod_free(&dep_mod);
            }
        }

        generate_all(&mod, list, n);

        for (int i = 0; i < n; ++i) {
            free(list[i].module);
            free(list[i].version);
        }
        free(list);

        if (mod.build_command) {
            printf("Building project: %s\n", mod.build_command);
            system(mod.build_command);
        } else {
            printf("Running default build (make)...\n");
            system("make");
        }
        cmod_free(&mod);
        return EXIT_SUCCESS;
    }

    if (strcmp(cmd, "clean") == 0) {
        system("rm -rf .cpm");
        printf("Cleaned .cpm directory\n");
        return EXIT_SUCCESS;
    }

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_help(argv[0]);
        return EXIT_SUCCESS;
    }

    fprintf(stderr, "cpm: unknown command '%s'\n", cmd);
    fprintf(stderr, "Run 'cpm help' for usage information.\n");
    return EXIT_FAILURE;
}

void print_help(const char *prog) {
    printf(
        "CPM - C Package Manager\n"
        "=======================\n"
        "\n"
        "Usage: %s <command> [options]\n"
        "\n"
        "CPM manages C library dependencies via Git repositories. It resolves\n"
        "dependency graphs, caches modules locally, vendors them into your\n"
        "project, and generates build flags for your build system.\n"
        "\n"
        "Commands:\n"
        "  init           Initialize a new cmod.toml in the current directory.\n"
        "                 Infers the module name from the local git remote.\n"
        "\n"
        "  get <mod>@<ver>\n"
        "                 Fetch a dependency and add it to cmod.toml.\n"
        "                 - <mod> is a Git URL or path (e.g. github.com/user/lib)\n"
        "                 - <ver> is a tag (v1.0.0 or 1.0.0) or commit SHA\n"
        "                 Examples:\n"
        "                   cpm get github.com/user/lib@v1.0.0\n"
        "                   cpm get github.com/user/lib@v1.0\n"
        "                   cpm get github.com/user/lib@latest-tag\n"
        "                   cpm get github.com/user/lib@abc1234\n"
        "\n"
        "  tidy           Remove vendored modules that are no longer required.\n"
        "                 Reads cmod.toml and cleans up .cpm/vendor/ accordingly.\n"
        "\n"
        "  vendor         Re-populate the vendor directory with all dependencies.\n"
        "                 Useful after modifying cmod.toml manually.\n"
        "\n"
        "  build          Generate build flags and build the project/dependencies.\n"
        "                 Uses [build] command in cmod.toml if defined, or runs 'make'.\n"
        "\n"
        "  clean          Remove the entire .cpm directory (cache + vendor).\n"
        "                 Fetches will re-download on next 'get'.\n"
        "\n"
        "  help           Show this help screen.\n"
        "\n"
        "Version Matching:\n"
        "  - Tags with 'v' prefix are equivalent (v1.0.0 == 1.0.0)\n"
        "  - Partial versions resolve to the highest available patch\n"
        "    (v1.0 matches v1.0.2 if it's the latest 1.0.x tag)\n"
        "  - Plain SHAs or commit references are used verbatim\n"
        "\n"
        "Files:\n"
        "  cmod.toml      Manifest file listing your module and dependencies\n"
        "  .cpm/          Local cache and vendor directory (can be gitignored)\n"
        "  .cpm/cpm.mk    Generated Makefile fragment (include in your Makefile)\n"
        "\n"
        "Integration:\n"
        "  Add this line to your Makefile AFTER defining CFLAGS:\n"
        "    -include .cpm/cpm.mk\n"
        "\n"
        "  Then build normally:\n"
        "    make all\n"
        "\n",
        prog
    );
}
