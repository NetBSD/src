/* $NetBSD: conf.c,v 1.5 2002/07/26 03:23:05 simonb Exp $ */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: conf.c,v 1.5 2002/07/26 03:23:05 simonb Exp $");

#define	CONF_HAVE_ISDN
#define	CONF_HAVE_PCI
#define	CONF_HAVE_SCSIPI
#define	CONF_HAVE_USB
#define	CONF_HAVE_WSCONS
#define	CONF_HAVE_SPKR
#define	CONF_HAVE_ISA	/* XXX bad name */
#undef	CONF_HAVE_ALCHEMY

#include "arch/evbmips/evbmips/conf_common.c"
