/* $NetBSD: conf.c,v 1.1 2002/07/29 16:22:56 simonb Exp $ */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: conf.c,v 1.1 2002/07/29 16:22:56 simonb Exp $");

#undef	CONF_HAVE_ISDN
#undef	CONF_HAVE_PCI
#undef	CONF_HAVE_SCSIPI
#define	CONF_HAVE_USB
#undef	CONF_HAVE_WSCONS
#undef	CONF_HAVE_SPKR
#undef	CONF_HAVE_ISA		/* XXX needs a better name */
#define	CONF_HAVE_ALCHEMY

#include "arch/evbmips/evbmips/conf_common.c"
