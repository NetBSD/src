/*
 *  Copyright (C) 2023 Yubico AB - See COPYING
 */

#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

#define ASSERT_STR_EQ(a, b) assert(!strcmp(a, b))
#define ASSERT_EXPANDED_EQ(str, user, result)                                  \
  do {                                                                         \
    char *tmp = expand_variables(str, user);                                   \
    assert(tmp != NULL);                                                       \
    ASSERT_STR_EQ(tmp, result);                                                \
    free(tmp);                                                                 \
  } while (0)

#define ASSERT_NULL(x) assert((x) == NULL)

int main(void) {
  ASSERT_EXPANDED_EQ("foobar", "user", "foobar");
  ASSERT_EXPANDED_EQ("", "user", "");
  ASSERT_EXPANDED_EQ("%%", "user", "%");
  ASSERT_EXPANDED_EQ("%u", "user", "user");
  ASSERT_EXPANDED_EQ("x%u", "user", "xuser");
  ASSERT_EXPANDED_EQ("%ux", "user", "userx");
  ASSERT_EXPANDED_EQ("x%ux", "user", "xuserx");
  ASSERT_EXPANDED_EQ("%%%u", "user", "%user");
  ASSERT_EXPANDED_EQ("%u%%", "user", "user%");
  ASSERT_EXPANDED_EQ("%%u", "user", "%u");
  ASSERT_EXPANDED_EQ("%u", "%user", "%user");
  ASSERT_EXPANDED_EQ("%u%u", "user", "useruser");
  ASSERT_EXPANDED_EQ("%%%u%%", "user", "%user%");

  ASSERT_NULL(expand_variables("%", "user"));  // Unexpected end of string.
  ASSERT_NULL(expand_variables("%x", "user")); // Unknown variable.
  ASSERT_NULL(expand_variables("%u", ""));     // Disallow empty username.

  return 0;
}
