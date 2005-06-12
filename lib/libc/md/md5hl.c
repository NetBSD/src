/*	$NetBSD: md5hl.c,v 1.6 2005/06/12 05:34:34 lukem Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@NetBSD.org>, April 29, 1997.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: md5hl.c,v 1.6 2005/06/12 05:34:34 lukem Exp $");
#endif /* LIBC_SCCS and not lint */

#define	MDALGORITHM	MD5

#include "namespace.h"
#include <md5.h>

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#if !HAVE_MD5_H
#include "mdXhl.c"
#endif
