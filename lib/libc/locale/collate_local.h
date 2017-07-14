/*	$NetBSD: collate_local.h,v 1.1.2.1 2017/07/14 15:53:08 perseant Exp $	*/

#ifndef	_COLLATE_LOCAL_H_
#define	_COLLATE_LOCAL_H_

#include <locale.h>

#include "unicode_ucd.h"

typedef struct _CollateLocale {
	void		*coll_variable;
	size_t		 coll_variable_len;
	struct ucd_coll *coll_data;
	size_t           coll_data_len;
} _CollateLocale;

/*
 * global variables
 */
extern __dso_hidden const _CollateLocale _DefaultCollateLocale;

__BEGIN_DECLS
int _collate_load(const char * __restrict, size_t, _CollateLocale ** __restrict);
__END_DECLS

#endif	/* !_COLLATE_LOCAL_H_ */
