/*
 * Copyright (C) 2021 Yubico AB - See COPYING
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "debug.h"

#define DEBUG_FMT "debug(pam_u2f): %s:%d (%s): %s%s"
#define MSGLEN 2048

FILE *debug_open(const char *filename) {
  struct stat st;
  FILE *file;
  int fd;

  if (strcmp(filename, "stdout") == 0)
    return stdout;
  if (strcmp(filename, "stderr") == 0)
    return stderr;
  if (strcmp(filename, "syslog") == 0)
    return NULL;

  fd = open(filename, O_WRONLY | O_APPEND | O_CLOEXEC | O_NOFOLLOW | O_NOCTTY);
  if (fd == -1 || fstat(fd, &st) != 0)
    goto err;

#ifndef WITH_FUZZING
  if (!S_ISREG(st.st_mode))
    goto err;
#endif

  if ((file = fdopen(fd, "a")) != NULL)
    return file;

err:
  if (fd != -1)
    close(fd);

  return DEFAULT_DEBUG_FILE; /* fallback to default */
}

void debug_close(FILE *f) {
  if (f != NULL && f != stdout && f != stderr)
    fclose(f);
}

static void do_log(FILE *debug_file, const char *file, int line,
                   const char *func, const char *msg, const char *suffix) {
#ifndef WITH_FUZZING
  if (debug_file == NULL) {
    syslog(LOG_AUTHPRIV | LOG_DEBUG, DEBUG_FMT, file, line, func, msg, suffix);
  } else {
    fprintf(debug_file, DEBUG_FMT "\n", file, line, func, msg, suffix);
  }
#else
  (void) debug_file;
  snprintf(NULL, 0, DEBUG_FMT, file, line, func, msg, suffix);
#endif
}

ATTRIBUTE_FORMAT(printf, 5, 0)
static void debug_vfprintf(FILE *debug_file, const char *file, int line,
                           const char *func, const char *fmt, va_list args) {
  const char *bn;
  char msg[MSGLEN];
  int r;

  if ((bn = strrchr(file, '/')) != NULL)
    file = bn + 1;

  if ((r = vsnprintf(msg, sizeof(msg), fmt, args)) < 0)
    do_log(debug_file, file, line, func, __func__, "");
  else
    do_log(debug_file, file, line, func, msg,
           (size_t) r < sizeof(msg) ? "" : "[truncated]");
}

void debug_fprintf(FILE *debug_file, const char *file, int line,
                   const char *func, const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  debug_vfprintf(debug_file, file, line, func, fmt, ap);
  va_end(ap);
}
