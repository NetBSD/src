/*	$NetBSD: readfile.h,v 1.3 2002/07/14 00:26:18 wiz Exp $	*/

/* readfile.h */

#include "bptypes.h"
#include "hash.h"

extern boolean hwlookcmp(hash_datum *, hash_datum *);
extern boolean iplookcmp(hash_datum *, hash_datum *);
extern boolean nmcmp(hash_datum *, hash_datum *);
extern void readtab(int);
extern void rdtab_init(void);
