#include "../include/fetcher.h"
#include "test_util.h"
#include <sht.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void cleanup_dir(const char *path) {
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "rm -rf %s", path);
  sys_log("fetcher", cmd);
}

TEST(fetcher, local_directory_copy) {
  int sout, serr;
  const char *src = "/tmp/cpm_test_local_src";
  const char *dst = "/tmp/cpm_test_local_dst";

  cleanup_dir(src);
  cleanup_dir(dst);

  mkdir(src, 0755);
  FILE *fp = fopen("/tmp/cpm_test_local_src/hello.txt", "w");
  fprintf(fp, "hello world\n");
  fclose(fp);

  cpm_log_begin("fetcher", &sout, &serr);
  int res = fetch_module(src, "main", dst);
  cpm_log_end(&sout, &serr);
  ASSERT_EQ(res, 0);

  FILE *checked = fopen("/tmp/cpm_test_local_dst/hello.txt", "r");
  ASSERT_NOT_NULL(checked);
  fclose(checked);

  cleanup_dir(src);
  cleanup_dir(dst);
}

TEST(fetcher, local_git_clone) {
  int sout, serr;
  const char *src = "/tmp/cpm_test_git_src";
  const char *dst = "/tmp/cpm_test_git_dst";

  cleanup_dir(src);
  cleanup_dir(dst);

  mkdir(src, 0755);
  // Create a local git repo
  sys_log("fetcher", "git -C /tmp/cpm_test_git_src init --initial-branch=main >/dev/null "
         "2>&1");
  sys_log("fetcher", "git -C /tmp/cpm_test_git_src config user.email 'test@example.com'");
  sys_log("fetcher", "git -C /tmp/cpm_test_git_src config user.name 'Test'");

  FILE *fp = fopen("/tmp/cpm_test_git_src/hello.txt", "w");
  fprintf(fp, "commit 1\n");
  fclose(fp);

  sys_log("fetcher", "git -C /tmp/cpm_test_git_src add hello.txt");
  sys_log("fetcher", "git -C /tmp/cpm_test_git_src commit -m 'Initial'");

  char hash[41];
  FILE *p = popen("git -C /tmp/cpm_test_git_src rev-parse HEAD", "r");
  ASSERT_NOT_NULL(p);
  fgets(hash, sizeof(hash), p);
  pclose(p);
  hash[strcspn(hash, "\n")] = '\0';

  char commit_ref[100];
  snprintf(commit_ref, sizeof(commit_ref), "commit:%s", hash);

  cpm_log_begin("fetcher", &sout, &serr);
  int res = fetch_module(src, commit_ref, dst);
  cpm_log_end(&sout, &serr);
  ASSERT_EQ(res, 0);

  FILE *checked = fopen("/tmp/cpm_test_git_dst/hello.txt", "r");
  ASSERT_NOT_NULL(checked);
  char buf[100];
  fgets(buf, sizeof(buf), checked);
  ASSERT_STR_EQ(buf, "commit 1\n");
  fclose(checked);

  cleanup_dir(src);
  cleanup_dir(dst);
}

TEST(fetcher, git_get_all_tags) {
  int sout, serr;
  const char *src = "/tmp/cpm_test_git_tags_src";
  cleanup_dir(src);
  mkdir(src, 0755);

  sys_log("fetcher", "git -C /tmp/cpm_test_git_tags_src init --initial-branch=main "
         ">/dev/null 2>&1");
  sys_log("fetcher",
      "git -C /tmp/cpm_test_git_tags_src config user.email 'test@example.com'");
  sys_log("fetcher", "git -C /tmp/cpm_test_git_tags_src config user.name 'Test'");

  sys_log("fetcher", "touch /tmp/cpm_test_git_tags_src/file && git -C "
         "/tmp/cpm_test_git_tags_src add file && git -C "
         "/tmp/cpm_test_git_tags_src commit -m 'Initial'");
  sys_log("fetcher", "git -C /tmp/cpm_test_git_tags_src tag v1.0.0");
  sys_log("fetcher", "touch /tmp/cpm_test_git_tags_src/file2 && git -C "
         "/tmp/cpm_test_git_tags_src add file2 && git -C "
         "/tmp/cpm_test_git_tags_src commit -m 'Second'");
  sys_log("fetcher", "git -C /tmp/cpm_test_git_tags_src tag v1.1.0");

  char **tags;
  size_t count;
  cpm_log_begin("fetcher", &sout, &serr);
  int res = get_all_tags(src, &tags, &count);
  cpm_log_end(&sout, &serr);
  ASSERT_EQ(res, 0);
  ASSERT_EQ(count, 2);
  ASSERT_STR_EQ(tags[0], "v1.0.0");
  ASSERT_STR_EQ(tags[1], "v1.1.0");

  for (size_t i = 0; i < count; i++)
    free(tags[i]);
  free(tags);
  cleanup_dir(src);
}

TEST(fetcher, git_get_commit_for_ref) {
  int sout, serr;
  const char *src = "/tmp/cpm_test_git_ref_src";
  cleanup_dir(src);
  mkdir(src, 0755);

  sys_log("fetcher", "git -C /tmp/cpm_test_git_ref_src init --initial-branch=main "
         ">/dev/null 2>&1");
  sys_log("fetcher",
      "git -C /tmp/cpm_test_git_ref_src config user.email 'test@example.com'");
  sys_log("fetcher", "git -C /tmp/cpm_test_git_ref_src config user.name 'Test'");

  sys_log("fetcher", "touch /tmp/cpm_test_git_ref_src/file && git -C "
         "/tmp/cpm_test_git_ref_src add file && git -C "
         "/tmp/cpm_test_git_ref_src commit -m 'Initial'");
  sys_log("fetcher", "git -C /tmp/cpm_test_git_ref_src tag v1.0.0");

  char hash[41];
  FILE *p = popen("git -C /tmp/cpm_test_git_ref_src rev-parse HEAD", "r");
  fgets(hash, sizeof(hash), p);
  pclose(p);
  hash[strcspn(hash, "\n")] = '\0';

  char *out_commit = NULL;
  cpm_log_begin("fetcher", &sout, &serr);
  int res = get_commit_for_ref(src, "v1.0.0", &out_commit);
  cpm_log_end(&sout, &serr);
  ASSERT_EQ(res, 0);
  ASSERT_STR_EQ(out_commit, hash);

  free(out_commit);
  cleanup_dir(src);
}
