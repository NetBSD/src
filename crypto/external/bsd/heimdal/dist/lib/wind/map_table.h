/*	$NetBSD: map_table.h,v 1.2 2019/12/15 22:50:51 christos Exp $	*/

/* ./map_table.h */
/* Automatically generated at 2019-06-07T02:26:41.397498 */

#ifndef MAP_TABLE_H
#define MAP_TABLE_H 1

#include "windlocl.h"

struct translation {
  uint32_t key;
  unsigned short val_len;
  unsigned short val_offset;
  wind_profile_flags flags;
};

extern const struct translation _wind_map_table[];

extern const size_t _wind_map_table_size;

extern const uint32_t _wind_map_table_val[];

#endif /* MAP_TABLE_H */
