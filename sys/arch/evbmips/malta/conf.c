/* $NetBSD: conf.c,v 1.6 2002/07/31 03:42:55 simonb Exp $ */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: conf.c,v 1.6 2002/07/31 03:42:55 simonb Exp $");

#define	CONF_HAVE_ISDN
#define	CONF_HAVE_PCI
#define	CONF_HAVE_SCSIPI
#define	CONF_HAVE_USB
#define	CONF_HAVE_WSCONS
#define	CONF_HAVE_SPKR
#define	CONF_HAVE_ISA		/* XXX needs a better name */
#undef	CONF_HAVE_KTTCP
#undef	CONF_HAVE_SYSMON
#undef	CONF_HAVE_ALCHEMY
#undef	CONF_HAVE_SBMIPS

#include "arch/evbmips/evbmips/conf_common.c"
