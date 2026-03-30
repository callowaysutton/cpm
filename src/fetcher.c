/* SPDX-License-Identifier: TODO */
#include "fetcher.h"
#include "fetcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *fetcher_strdup(const char *s) {
    return s ? strdup(s) : NULL;
}

static int run_command(const char *cmd) {
    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "Command failed: %s (exit %d)\n", cmd, rc);
    }
    return rc;
}

int fetch_module(const char *module_path, const char *ref, const char *dest_dir) {
    // Check if module_path is a local path (absolute or relative)
    int is_local = (module_path[0] == '/' || 
                   (module_path[0] == '.' && module_path[1] == '/') ||
                   (module_path[0] == '.' && module_path[1] == '.' && module_path[2] == '/'));

    if (is_local) {
        // For local paths, check if it's a git repository
        char test_cmd[1024];
        snprintf(test_cmd, sizeof(test_cmd), "test -d %s/.git", module_path);
        int is_git = (system(test_cmd) == 0);

        if (is_git) {
            // It's a git repo - clone from it using local path as URL
            const char *branch = ref;
            if (strncmp(ref, "commit:", 7) == 0) {
                branch = ref + 7;
            }
            char cmd[1024];
            if (strncmp(ref, "commit:", 7) == 0) {
                snprintf(cmd, sizeof(cmd),
                         "git clone --no-checkout %s %s && git -C %s checkout %s",
                         module_path, dest_dir, dest_dir, branch);
                fprintf(stderr, "Fetching local commit from %s at %s\n", module_path, branch);
            } else {
                snprintf(cmd, sizeof(cmd),
                         "git clone --depth 1 --branch %s %s %s",
                         branch, module_path, dest_dir);
                fprintf(stderr, "Cloning local git module from %s at %s\n", module_path, branch);
            }
            return run_command(cmd);
        } else {
            // Not a git repo - just copy the directory
            char cmd[1024];
            snprintf(cmd, sizeof(cmd), "cp -r %s %s", module_path, dest_dir);
            fprintf(stderr, "Copying local module from %s to %s\n", module_path, dest_dir);
            return run_command(cmd);
        }
    }

    // Build a git URL. Handle various formats:
    char url[512];
    if (strstr(module_path, "://")) {
        // Already a full URL (https://, git://, etc.)
        snprintf(url, sizeof(url), "%s", module_path);
    } else if (strstr(module_path, "github.com/") == module_path ||
               strstr(module_path, "gitlab.com/") == module_path ||
               strstr(module_path, "bitbucket.org/") == module_path) {
        // Standard Git hosting format (e.g., github.com/user/repo)
        // Convert to HTTPS URL
        snprintf(url, sizeof(url), "https://%s.git", module_path);
    } else if (strstr(module_path, "@") && !strstr(module_path, "://")) {
        // Already SSH format (e.g., git@github.com:user/repo)
        snprintf(url, sizeof(url), "%s", module_path);
        // Ensure it has .git extension
        if (strlen(url) > 4 && strcmp(url + strlen(url) - 4, ".git") != 0) {
            strcat(url, ".git");
        }
    } else {
        // Fallback: treat as HTTPS URL with .git
        snprintf(url, sizeof(url), "https://%s.git", module_path);
    }

    // Resolve ref: if it starts with "commit:" strip the prefix.
    const char *branch = ref;
    if (strncmp(ref, "commit:", 7) == 0) {
        branch = ref + 7;
    }

    // Prepare command: shallow clone or commit fetch
    char cmd[1024];
    if (strncmp(ref, "commit:", 7) == 0) {
        snprintf(cmd, sizeof(cmd),
                 "git clone --no-checkout %s %s && git -C %s checkout %s",
                 url, dest_dir, dest_dir, branch);
        fprintf(stderr, "Fetching remote commit: %s@%s\n", module_path, branch);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "git clone --depth 1 --branch %s %s %s",
                 branch, url, dest_dir);
        fprintf(stderr, "Cloning remote branch/tag: %s@%s\n", module_path, branch);
    }
    
    fprintf(stderr, "URL: %s\n", url);
    return run_command(cmd);
}

int get_all_tags(const char *module, char ***out_tags, size_t *out_count) {
    // Basic heuristics to resolve 'module' to a URL
    char url[512];
    int is_local = (module[0] == '/' || 
                   (module[0] == '.' && module[1] == '/') ||
                   (module[0] == '.' && module[1] == '.' && module[2] == '/'));

    if (is_local) {
        snprintf(url, sizeof(url), "%s", module);
    } else if (strstr(module, "://")) {
        snprintf(url, sizeof(url), "%s", module);
    } else if (strstr(module, "github.com/") == module ||
               strstr(module, "gitlab.com/") == module ||
               strstr(module, "bitbucket.org/") == module) {
        snprintf(url, sizeof(url), "https://%s.git", module);
    } else if (strstr(module, "@") && !strstr(module, "://")) {
        snprintf(url, sizeof(url), "%s", module);
    } else {
        snprintf(url, sizeof(url), "https://%s.git", module); // fallback
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), 
             "git ls-remote --tags %s 2>/dev/null | "
             "grep -E 'refs/tags/.*' | "
             "sed 's/.*refs\\/tags\\///' | "
             "sed 's/\\^{}//' | " // remove peeled tags ^{}
             "sort -V | uniq", url);
    
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        *out_tags = NULL;
        *out_count = 0;
        return -1;
    }
    
    char line[256];
    size_t capacity = 16;
    char **tags = calloc(capacity, sizeof(char*));
    size_t count = 0;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (strlen(line) == 0) continue;
        if (count >= capacity) {
            capacity *= 2;
            char **new_tags = realloc(tags, capacity * sizeof(char*));
            if (!new_tags) {
                for (size_t i = 0; i < count; i++) free(tags[i]);
                free(tags);
                pclose(fp);
                return -1;
            }
            tags = new_tags;
        }
        tags[count++] = fetcher_strdup(line);
    }
    pclose(fp);

    *out_tags = tags;
    *out_count = count;
    return 0;
}

int get_commit_for_ref(const char *module, const char *ref, char **out_commit) {
    char url[512];
    int is_local = (module[0] == '/' || 
                   (module[0] == '.' && module[1] == '/') ||
                   (module[0] == '.' && module[1] == '.' && module[2] == '/'));

    if (is_local) {
        snprintf(url, sizeof(url), "%s", module);
    } else if (strstr(module, "://")) {
        snprintf(url, sizeof(url), "%s", module);
    } else if (strstr(module, "github.com/") == module ||
               strstr(module, "gitlab.com/") == module ||
               strstr(module, "bitbucket.org/") == module) {
        snprintf(url, sizeof(url), "https://%s.git", module);
    } else if (strstr(module, "@") && !strstr(module, "://")) {
        snprintf(url, sizeof(url), "%s", module);
    } else {
        snprintf(url, sizeof(url), "https://%s.git", module); // fallback
    }

    char cmd[1024];
    // We look up the specific ref and output its hash.
    snprintf(cmd, sizeof(cmd), "git ls-remote %s %s 2>/dev/null | awk '{print $1}'", url, ref);
    
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return -1;
    }

    char line[256];
    if (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n\r")] = '\0';
        pclose(fp);
        if (strlen(line) == 40) { // git commit hash length
            *out_commit = strdup(line);
            return 0;
        }
        return -1;
    }
    
    pclose(fp);
    return -1;
}
