/* $NetBSD: conf.c,v 1.2.6.2 2002/09/06 08:34:15 jdolecek Exp $ */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: conf.c,v 1.2.6.2 2002/09/06 08:34:15 jdolecek Exp $");

#undef	CONF_HAVE_ISDN
#undef	CONF_HAVE_PCI
#undef	CONF_HAVE_SCSIPI
#define	CONF_HAVE_USB
#undef	CONF_HAVE_WSCONS
#undef	CONF_HAVE_SPKR
#undef	CONF_HAVE_ISA		/* XXX needs a better name */
#undef	CONF_HAVE_KTTCP
#undef	CONF_HAVE_SYSMON
#define	CONF_HAVE_ALCHEMY
#undef	CONF_HAVE_SBMIPS

#include "arch/evbmips/evbmips/conf_common.c"
