/*	$NetBSD: combining_table.h,v 1.2.10.1 2023/08/11 13:40:02 martin Exp $	*/

/* ./combining_table.h */
/* Automatically generated at 2022-11-15T13:59:51.697168 */

#ifndef COMBINING_TABLE_H
#define COMBINING_TABLE_H 1

#include <krb5/krb5-types.h>

struct translation {
  uint32_t key;
  unsigned combining_class;	
};

extern const struct translation _wind_combining_table[];

extern const size_t _wind_combining_table_size;
#endif /* COMBINING_TABLE_H */
