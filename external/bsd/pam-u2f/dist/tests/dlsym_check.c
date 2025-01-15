/*
 *  Copyright (C) 2014-2022 Yubico AB - See COPYING
 */

#undef NDEBUG
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <dlfcn.h>

int main(void) {
  char *path;
  void *module;

  assert((path = getenv("PAM_U2F_MODULE")) != NULL);
  assert((module = dlopen(path, RTLD_NOW)) != NULL);
  assert(dlsym(module, "pam_sm_authenticate") != NULL);
  assert(dlsym(module, "pam_sm_setcred") != NULL);
  assert(dlsym(module, "nonexistent") == NULL);
  assert(dlclose(module) == 0);

  return 0;
}
