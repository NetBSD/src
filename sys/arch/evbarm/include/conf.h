/*	$NetBSD: conf.h,v 1.2.2.4 2002/08/01 02:41:34 nathanw Exp $	*/

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
