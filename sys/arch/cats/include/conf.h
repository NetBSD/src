/*	$NetBSD: conf.h,v 1.6.4.2 2002/01/08 00:23:50 nathanw Exp $	*/

#ifndef _CATS_CONF_H
#define	_CATS_CONF_H

/*
 * CATS specific device includes go in here
 */
#include "fcom.h"

#define	CONF_HAVE_PCI
#define	CONF_HAVE_USB
#define	CONF_HAVE_SCSIPI
#define	CONF_HAVE_WSCONS

#endif	/* _CATS_CONF_H */
