/*	$NetBSD: collate_local.h,v 1.1.2.2 2017/07/31 04:29:50 perseant Exp $	*/

#ifndef	_COLLATE_LOCAL_H_
#define	_COLLATE_LOCAL_H_

#include <locale.h>
#include "collate.h"

#include "unicode_ucd.h"

typedef struct _CollateLocale {
	void		*coll_variable;
	size_t		 coll_variable_len;
	struct ucd_coll        *coll_data; /* XXX obsolescent */
	size_t           coll_data_len;
	const struct _FileCollateLocale *coll_fcl;
#define coll_collinfo  coll_fcl->fcl_collinfo
#define coll_char_data coll_fcl->fcl_char_data
	const collate_subst_t  *coll_subst;
	const collate_chain_t  *coll_chains;
	const collate_large_t  *coll_large;
} _CollateLocale;

typedef struct _FileCollateLocale {
	collate_info_t  fcl_collinfo;
	collate_char_t  fcl_char_data[0x100];
/*
        These fields are variable length (perhaps 0)
	and follow the previous fields in the file:
	collate_chain_t *chains;
	collate_large_t *large;
	collate_subst_t *subst;
*/
} _FileCollateLocale;

/*
 * global variables
 */
extern __dso_hidden const struct xlocale_collate _DefaultCollateLocale;

__BEGIN_DECLS
int _collate_load(const char * __restrict, size_t, struct xlocale_collate ** __restrict);
__END_DECLS

#endif	/* !_COLLATE_LOCAL_H_ */
