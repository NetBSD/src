/*
 * setproctitle.c -- mimic setproctitle
 *
 * Copyright (c) 2020, NLnet Labs. All rights reserved.
 *
 * See LICENSE for the license.
 *
 */
#include "config.h"

#if defined(__linux)
#include <assert.h>
#include <errno.h>
#ifndef HAVE_CONFIG_H
#include <libgen.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <unistd.h>

static char *executable(void)
{
  char *ptr, *buf = NULL;
  size_t len = 0;
  ssize_t cnt = 0;

  buf = NULL;
  do {
    len += 32;
    ptr = realloc(buf, len);
    if (ptr == NULL) {
      if (buf != NULL) {
        free(buf);
      }
      return NULL;
    }
    buf = ptr;
    cnt = readlink("/proc/self/exe", buf, len);
  } while (cnt >= 0 && (size_t)cnt == len);

  if (cnt >= 0) {
    buf[cnt] = '\0';
    return buf;
  }

  free(buf);

  return NULL;
}

void setproctitle(const char *fmt, ...)
{
  va_list ap;
  char buf[32];
  int cnt = 0, off = 0;

  /* prepend executable name if fmt does not start with '-' */
  if (fmt == NULL || fmt[0] != '-' || fmt[(off = 1)] == '\0') {
    char *exe;
    const char *sep = (fmt && fmt[off] != '\0') ? ": " : "";
    if ((exe = executable()) != NULL) {
      cnt = snprintf(buf, sizeof(buf), "%s%s", basename(exe), sep);
      if ((size_t)cnt >= sizeof(buf)) {
        cnt = 31; /* leave room for '\0' */
      }
      free(exe);
    }
  }

  if (fmt != NULL && fmt[off] != '\0') {
    va_start(ap, fmt);
    cnt = vsnprintf(
      buf+(size_t)cnt, sizeof(buf)-(size_t)cnt, fmt+(size_t)off, ap);
    va_end(ap);
  }

  if (cnt > 0) {
    assert(cnt > 0 && (size_t)cnt < sizeof(buf));
    (void)prctl(PR_SET_NAME, buf, 0, 0, 0);
  }
}
#endif /* __linux */
