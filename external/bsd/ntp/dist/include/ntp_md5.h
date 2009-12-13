/*	$NetBSD: ntp_md5.h,v 1.1.1.1 2009/12/13 16:54:52 kardel Exp $	*/

/*
 * ntp_md5.h: deal with md5.h headers
 */

#if defined HAVE_MD5_H && defined HAVE_MD5INIT
# include <md5.h>
#else
# include "isc/md5.h"
# define MD5_CTX	isc_md5_t
# define MD5Init	isc_md5_init
# define MD5Update	isc_md5_update
# define MD5Final(d,c)	isc_md5_final(c,d)
#endif
