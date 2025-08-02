/*
 * Copyright (C) 2021 Yubico AB - See COPYING
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#define DEFAULT_DEBUG_FILE stderr

#if defined(DEBUG_PAM)
#define D(file, ...)                                                           \
  debug_fprintf(file, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define D(file, ...) ((void) 0)
#endif /* DEBUG_PAM */

#define debug_dbg(cfg, ...)                                                    \
  do {                                                                         \
    if (cfg->debug) {                                                          \
      D(cfg->debug_file, __VA_ARGS__);                                         \
    }                                                                          \
  } while (0)

#ifdef __GNUC__
#define ATTRIBUTE_FORMAT(f, s, a) __attribute__((format(f, s, a)))
#else
#define ATTRIBUTE_FORMAT(f, s, a)
#endif

FILE *debug_open(const char *);
void debug_close(FILE *f);
void debug_fprintf(FILE *, const char *, int, const char *, const char *, ...)
  ATTRIBUTE_FORMAT(printf, 5, 6);

#endif /* DEBUG_H */
