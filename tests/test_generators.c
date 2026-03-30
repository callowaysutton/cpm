#include "../include/generator.h"
#include "test_util.h"
#include <sht.h>
#include <stdlib.h>
#include <unistd.h>

SHT_SUITE(Generators)

// Helper to assert a file was written with expected basic content
static void verify_gen_output(const char *path, const char *search_str) {
  FILE *f = fopen(path, "r");
  SHT_ASSERT_NE((void *)f, NULL);
  char buf[1024] = {0};
  fread(buf, 1, sizeof(buf) - 1, f);
  fclose(f);
  SHT_ASSERT_NE(strstr(buf, search_str), NULL);
}

SHT_TEST(Generators, test_all_generators) {
  int sout, serr;
  sys_log("generators", "mkdir -p .cpm");
  sys_log("generators", "mkdir -p include src");

  // Setup mock resolved list
  cmod_t mod = {0};
  mod.module = "com.test.root";
  mod.generators_len = 3;
  const char *gens[] = {"make", "cmake", "bazel"};
  mod.generators = (char **)gens;

  resolved_t list[2];
  list[0].module = "github.com/test/repo1";
  list[0].version = "v1.0.0";
  list[1].module = "gitlab.com/test/repo2";
  list[1].version = "commit:123456";

  // Call generate_all
  cpm_log_begin("generators", &sout, &serr);
  int rc = generate_all(&mod, list, 2);
  cpm_log_end(&sout, &serr);
  SHT_ASSERT_EQ(rc, 0);

  // Verify .cpm/cpm.mk was created
  verify_gen_output(".cpm/cpm.mk", "github.com_test_repo1@v1.0.0");
  verify_gen_output(".cpm/cpm.mk", "gitlab.com_test_repo2@commit_123456");

  // Verify .cpm/cpm.cmake was created
  verify_gen_output(".cpm/cpm.cmake", "include_directories(");
  verify_gen_output(".cpm/cpm.cmake", "github.com_test_repo1@v1.0.0");

  // Verify .cpm/cpm.bzl was created
  verify_gen_output(".cpm/cpm.bzl", "CPM_INCLUDE_PATHS = [");
  verify_gen_output(".cpm/cpm.bzl", "github.com_test_repo1@v1.0.0");

  sys_log("generators", "rm -f .cpm/cpm.*"); // cleanup generic files only
  sys_log("generators", "rmdir include src 2>/dev/null");
}
