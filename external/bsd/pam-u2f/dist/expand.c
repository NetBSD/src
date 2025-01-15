/*
 * Copyright (C) 2023 Yubico AB - See COPYING
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "util.h"

static int buf_write(uint8_t **dst, size_t *size, const void *src, size_t n) {
  if (*size < n) {
    return -1;
  }

  memcpy(*dst, src, n);
  *dst += n;
  *size -= n;

  return 0;
}

static int buf_write_byte(uint8_t **dst, size_t *size, uint8_t c) {
  return buf_write(dst, size, &c, sizeof(c));
}

static const char *lookup(char var, const char *user) {
  switch (var) {
    case 'u':
      return user;
    case '%':
      return "%";
    default:
      // Capture all unknown variables (incl. null byte).
      return NULL;
  }
}

char *expand_variables(const char *str, const char *user) {
  uint8_t *tail, *head;
  size_t size = PATH_MAX;
  int ok = -1;

  if (str == NULL || (tail = head = malloc(size)) == NULL) {
    return NULL;
  }

  for (; *str != '\0'; str++) {
    if (*str == '%') {
      str++;
      const char *value = lookup(*str, user);
      if (value == NULL || *value == '\0' ||
          buf_write(&head, &size, value, strlen(value)) != 0) {
        goto fail;
      }
    } else if (buf_write_byte(&head, &size, *str) != 0) {
      goto fail;
    }
  }

  ok = buf_write_byte(&head, &size, '\0');

fail:
  if (ok != 0) {
    free(tail);
    return NULL;
  }
  return (char *) tail;
}
