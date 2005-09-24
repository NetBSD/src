/* $NetBSD: rmd160hl.c,v 1.1 2005/09/24 19:04:52 elad Exp $ */

/*
 * Derived from code ritten by Jason R. Thorpe <thorpej@NetBSD.org>,
 * April 29, 1997.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: rmd160hl.c,v 1.1 2005/09/24 19:04:52 elad Exp $");
#endif /* LIBC_SCCS and not lint */

#define	HASH_ALGORITHM	RMD160

#include "namespace.h"
#include <rmd160.h> /* XXX */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include "../hash.c"
