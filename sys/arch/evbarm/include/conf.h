/*	$NetBSD: conf.h,v 1.4.12.3 2002/09/06 08:34:03 jdolecek Exp $	*/

#ifndef _EVBARM_CONF_H
#define	_EVBARM_CONF_H

/*
 * EVBARM specifc device includes go in here
 */
#include "plcom.h"
#include "ixpcom.h"

#define	CONF_HAVE_PCI
#define	CONF_HAVE_SCSIPI
#define	CONF_HAVE_USB
#define	CONF_HAVE_WSCONS

#endif	/* _EVBARM_CONF_H */
