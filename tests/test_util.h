/*
 * test_util.h - CPM test suite I/O helpers.
 *
 * Thin wrappers around sht_util.h (from the SHT library) that provide
 * CPM-friendly names with automatic per-suite log path construction.
 *
 *   sys_log(suite, cmd)
 *     Routes a shell command's stdout+stderr to "../<suite>_output.log".
 *     Drop-in replacement for system().
 *
 *   cpm_log_begin(suite, &sout, &serr)
 *   cpm_log_end(&sout, &serr)
 *     Redirects the process's stdout+stderr to "../<suite>_output.log"
 *     around in-process C function calls (e.g. cpm_dispatch, vendor_populate).
 */

#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <sht_util.h>
#include <stdio.h>

static inline void _test_util_logpath(const char *suite, char *buf, size_t n) {
  snprintf(buf, n, "../%s_output.log", suite);
}

static inline int sys_log(const char *suite, const char *cmd) {
  char path[256];
  _test_util_logpath(suite, path, sizeof(path));
  return sht_sys_log(path, cmd);
}

static inline void cpm_log_begin(const char *suite, int *saved_out,
                                 int *saved_err) {
  char path[256];
  _test_util_logpath(suite, path, sizeof(path));
  sht_redirect_begin(path, saved_out, saved_err);
}

static inline void cpm_log_end(int *saved_out, int *saved_err) {
  sht_redirect_end(saved_out, saved_err);
}

#endif /* TEST_UTIL_H */
