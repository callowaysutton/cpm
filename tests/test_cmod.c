#include "../include/cmod.h"
#include <sht.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

TEST(cmod, basic_load_save) {
  const char *path = "/tmp/test_cmod.toml";

  // Create test TOML
  FILE *fp = fopen(path, "w");
  ASSERT_NOT_NULL(fp);
  fprintf(fp, "module = \"test/module\"\n");
  fprintf(fp, "version = \"1.0.0\"\n");
  fprintf(fp, "generators = [\"make\"]\n");
  fprintf(fp, "[requires]\n");
  fprintf(fp, "lib1 = \"1.2.3\"\n");
  fclose(fp);

  // Initialize cmod manually
  cmod_t out;
  int res = cmod_load(path, &out);
  ASSERT_EQ(res, 0);

  ASSERT_NOT_NULL(out.module);
  ASSERT_STR_EQ(out.module, "test/module");

  ASSERT_NOT_NULL(out.version);
  ASSERT_STR_EQ(out.version, "1.0.0");

  ASSERT_EQ(out.generators_len, 1);
  ASSERT_STR_EQ(out.generators[0], "make");

  ASSERT_EQ(out.requires_len, 1);
  ASSERT_STR_EQ(out.requires[0].path, "lib1");
  ASSERT_STR_EQ(out.requires[0].constraint, "1.2.3");

  // Test saving back
  const char *save_path = "/tmp/test_cmod_saved.toml";
  res = cmod_save(save_path, &out);
  ASSERT_EQ(res, 0);

  cmod_free(&out);

  // Re-load and verify
  cmod_t reload;
  res = cmod_load(save_path, &reload);
  ASSERT_EQ(res, 0);
  ASSERT_STR_EQ(reload.module, "test/module");
  ASSERT_STR_EQ(reload.requires[0].constraint, "1.2.3");

  cmod_free(&reload);

  unlink(path);
  unlink(save_path);
}

TEST(cmod, empty_file) {
  const char *path = "/tmp/test_cmod_empty.toml";
  FILE *fp = fopen(path, "w");
  ASSERT_NOT_NULL(fp);
  fclose(fp);

  cmod_t out;
  int res = cmod_load(path, &out);
  ASSERT_EQ(res, 0);
  ASSERT_NULL(out.module);
  ASSERT_NULL(out.version);
  ASSERT_EQ(out.generators_len, 0);
  ASSERT_EQ(out.requires_len, 0);

  cmod_free(&out);
  unlink(path);
}

TEST(cmod, invalid_file) {
  cmod_t out;
  int res = cmod_load("/tmp/nonexistent_file_qwe.toml", &out);
  ASSERT_NE(res, 0);
}
