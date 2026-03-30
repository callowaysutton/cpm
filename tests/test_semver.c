#include "../include/semver.h"
#include <sht.h>

// test_semver.c
// Tests for semver parsing and comparison
TEST(semver_parse, valid_simple_versions) {
  semver_t sv;
  int res = semver_parse("1.2.3", &sv);
  ASSERT_EQ(res, 0);
  ASSERT_EQ(sv.major, 1);
  ASSERT_EQ(sv.minor, 2);
  ASSERT_EQ(sv.patch, 3);
  ASSERT_NULL(sv.prerelease);
  semver_free(&sv);

  res = semver_parse("v1.5.0", &sv);
  ASSERT_EQ(res, 0);
  ASSERT_EQ(sv.major, 1);
  ASSERT_EQ(sv.minor, 5);
  ASSERT_EQ(sv.patch, 0);
  semver_free(&sv);
}

TEST(semver_parse, partial_versions) {
  semver_t sv;
  int res = semver_parse("1.2", &sv);
  ASSERT_EQ(res, 0);
  ASSERT_EQ(sv.major, 1);
  ASSERT_EQ(sv.minor, 2);
  ASSERT_EQ(sv.patch, 0);
  semver_free(&sv);

  res = semver_parse("v5", &sv);
  ASSERT_NE(res, 0);
}

TEST(semver_parse, complex_versions) {
  semver_t sv;
  int res = semver_parse("1.2.3-alpha.1+build.xyz", &sv);
  ASSERT_EQ(res, 0);
  ASSERT_EQ(sv.major, 1);
  ASSERT_EQ(sv.minor, 2);
  ASSERT_EQ(sv.patch, 3);
  ASSERT_STR_EQ(sv.prerelease, "alpha.1");
  ASSERT_STR_EQ(sv.build, "build.xyz");
  semver_free(&sv);

  res = semver_parse("1.2.3+buildOnly", &sv);
  ASSERT_EQ(res, 0);
  ASSERT_NULL(sv.prerelease);
  ASSERT_STR_EQ(sv.build, "buildOnly");
  semver_free(&sv);

  res = semver_parse("invalid_version", &sv);
  ASSERT_NE(res, 0);
}

TEST(semver_compare, greater_than) {
  semver_t v1, v2;
  semver_parse("1.2.3", &v1);
  semver_parse("1.2.2", &v2);
  ASSERT_GT(semver_compare(v1, v2), 0);
  semver_free(&v1);
  semver_free(&v2);
}

TEST(semver_compare, prerelease_rules) {
  semver_t v1, v2;

  semver_parse("1.0.0-alpha", &v1);
  semver_parse("1.0.0", &v2);
  ASSERT_LT(semver_compare(v1, v2), 0);
  semver_free(&v1);
  semver_free(&v2);

  semver_parse("1.0.0-alpha", &v1);
  semver_parse("1.0.0-alpha.1", &v2);
  ASSERT_LT(semver_compare(v1, v2), 0);
  semver_free(&v1);
  semver_free(&v2);

  semver_parse("1.0.0-alpha.beta", &v1);
  semver_parse("1.0.0-beta", &v2);
  ASSERT_LT(semver_compare(v1, v2), 0);
  semver_free(&v1);
  semver_free(&v2);

  semver_parse("1.0.0-beta.2", &v1);
  semver_parse("1.0.0-beta.11", &v2);
  ASSERT_LT(semver_compare(v1, v2), 0); // Numeric compare
  semver_free(&v1);
  semver_free(&v2);
}
