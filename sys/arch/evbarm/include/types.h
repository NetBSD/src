/*	$NetBSD: types.h,v 1.4.2.2 2002/02/11 20:07:43 jdolecek Exp $	*/

#ifndef _EVBARM_TYPES_H_
#define _EVBARM_TYPES_H_

#include <arm/arm32/types.h>

#ifndef __OLD_INTERRUPT_CODE		/* XXX */
#define	__HAVE_GENERIC_SOFT_INTERRUPTS
#endif
#define __HAVE_DEVICE_REGISTER
#define __HAVE_NWSCONS

#endif
