#include "../include/cache.h"
#include "test_util.h"
#include <sht.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

SHT_SUITE(Cache)

SHT_TEST(Cache, cache_path_basic) {
  char *path = cache_path("github.com/user/repo", "v1.2.3");
  SHT_ASSERT_STR_EQ(path, ".cpm/cache/github.com_user_repo@v1.2.3");
  free(path);
}

SHT_TEST(Cache, cache_path_complex) {
  char *path = cache_path("gitlab.com/user:group/repo@name", "commit:abcdef");
  SHT_ASSERT_STR_EQ(path,
                    ".cpm/cache/gitlab.com_user_group_repo_name@commit_abcdef");
  free(path);
}

SHT_TEST(Cache, ensure_cached_logic) {
  int sout, serr;
  sys_log("cache", "rm -rf .cpm/cache/dummy_module_test@v1.0.0");

  // Will fail because dummy_module_test is a fake repo
  cpm_log_begin("cache", &sout, &serr);
  int rc = ensure_cached("dummy/module/test", "v1.0.0");
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_NE(rc, 0);

  // Manually create the folder to trigger the dir_exists shortcut route
  sys_log("cache", "mkdir -p .cpm/cache/dummy_module_test@v1.0.0");

  cpm_log_begin("cache", &sout, &serr);
  rc = ensure_cached("dummy/module/test", "v1.0.0");
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(rc, 0);

  sys_log("cache", "rm -rf .cpm/cache/dummy_module_test@v1.0.0");
}
