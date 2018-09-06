/*	$NetBSD: ldapdb.h,v 1.2.2.2 2018/09/06 06:54:56 pgoyette Exp $	*/

#include <isc/types.h>

isc_result_t ldapdb_init(void);

void ldapdb_clear(void);

