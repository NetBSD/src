/*	$NetBSD: errorlist_table.h,v 1.3 2023/06/19 21:41:45 christos Exp $	*/

/* ./errorlist_table.h */
/* Automatically generated at 2022-11-15T13:59:51.730690 */

#ifndef ERRORLIST_TABLE_H
#define ERRORLIST_TABLE_H 1

#include "windlocl.h"

struct error_entry {
  uint32_t start;
  unsigned len;
  wind_profile_flags flags;
};

extern const struct error_entry _wind_errorlist_table[];

extern const size_t _wind_errorlist_table_size;

#endif /* ERRORLIST_TABLE_H */
