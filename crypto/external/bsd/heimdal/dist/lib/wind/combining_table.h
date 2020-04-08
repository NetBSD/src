/*	$NetBSD: combining_table.h,v 1.2.4.2 2020/04/08 14:03:15 martin Exp $	*/

/* ./combining_table.h */
/* Automatically generated at 2019-06-07T02:26:41.530328 */

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
