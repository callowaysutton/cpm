/* SPDX-License-Identifier: MIT */
#define _GNU_SOURCE
#include "vendor.h"
#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

static char *sanitize(const char *s) {
    char *out = strdup(s);
    for (char *p = out; *p; ++p) {
        if (*p == '/' || *p == '@' || *p == ':') *p = '_';
    }
    return out;
}

static int create_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return 0;
        fprintf(stderr, "Error: %s exists but is not a directory\n", path);
        return -1;
    }
    
    char *tmp = strdup(path);
    char *p = tmp;
    
    if (tmp[0] == '/') p++;
    
    for (char *slash = strchr(p, '/'); slash; slash = strchr(p, '/')) {
        *slash = '\0';
        
        if (strlen(tmp) > 0) {
            if (stat(tmp, &st) != 0) {
                if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                    free(tmp);
                    return -1;
                }
            } else if (!S_ISDIR(st.st_mode)) {
                free(tmp);
                return -1;
            }
        }
        *slash = '/';
        p = slash + 1;
    }
    
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        free(tmp);
        return -1;
    }
    
    free(tmp);
    return 0;
}

static int copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) {
        fprintf(stderr, "Warning: cannot read %s: %s\n", src, strerror(errno));
        return -1;
    }
    
    FILE *out = fopen(dst, "wb");
    if (!out) {
        fprintf(stderr, "Warning: cannot write %s: %s\n", dst, strerror(errno));
        fclose(in);
        return -1;
    }
    
    char buf[8192];
    size_t n;
    int rc = 0;
    
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fprintf(stderr, "Warning: write error for %s\n", dst);
            rc = -1;
            break;
        }
    }
    
    if (ferror(in)) rc = -1;
    
    fclose(in);
    fclose(out);
    
    struct stat st;
    if (stat(src, &st) == 0) {
        chmod(dst, st.st_mode);
    }
    
    return rc;
}

static int copy_dir_recursive(const char *src, const char *dst) {
    DIR *dir = opendir(src);
    if (!dir) {
        fprintf(stderr, "Warning: cannot open directory %s: %s\n", src, strerror(errno));
        return -1;
    }
    
    if (create_dir(dst) != 0) {
        closedir(dir);
        return -1;
    }
    
    struct dirent *entry;
    int rc = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        if (strcmp(entry->d_name, ".git") == 0) {
            continue;
        }
        
        char src_path[1024];
        char dst_path[1024];
        
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);
        
        struct stat st;
        if (stat(src_path, &st) != 0) {
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            if (copy_dir_recursive(src_path, dst_path) != 0) {
                rc = -1;
            }
        } else if (S_ISREG(st.st_mode)) {
            if (copy_file(src_path, dst_path) != 0) {
                rc = -1;
            }
        }
    }
    
    closedir(dir);
    return rc;
}

int vendor_populate(resolved_t *list, size_t n) {
    if (create_dir(".cpm/vendor") != 0) {
        fprintf(stderr, "Error: failed to create .cpm/vendor directory\n");
        return -1;
    }
    
    for (size_t i = 0; i < n; ++i) {
        char *src_path = cache_path(list[i].module, list[i].version);
        
        char *clean_module = sanitize(list[i].module);
        char *clean_version = sanitize(list[i].version);
        char *dst_path;
        asprintf(&dst_path, ".cpm/vendor/%s@%s", clean_module, clean_version);
        free(clean_module);
        free(clean_version);
        
        fprintf(stderr, "Vendoring %s@%s...\n", list[i].module, list[i].version);
        
        if (copy_dir_recursive(src_path, dst_path) != 0) {
            fprintf(stderr, "Warning: failed to vendor %s@%s\n", list[i].module, list[i].version);
        }
        
        free(src_path);
        free(dst_path);
    }
    
    return 0;
}

int vendor_clean_unused(const char *module, const char *version) {
    char *clean_module = sanitize(module);
    char *clean_version = sanitize(version);
    char *path;
    asprintf(&path, ".cpm/vendor/%s@%s", clean_module, clean_version);
    free(clean_module);
    free(clean_version);
    
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", path);
    int rc = system(cmd);
    free(path);
    return rc;
}
