/*	$NetBSD: param.h,v 1.1 2002/07/05 13:31:47 scw Exp $	*/

/*
 * XXX: Not sure if these are actually correct for SH-5
 */

#ifndef _EVBSH5_PARAM_H
#define _EVBSH5_PARAM_H

#include <sh5/param.h>

#ifdef __LITTLE_ENDIAN__
#define	_MACHINE_ARCH	sh5le
#define	MACHINE_ARCH	"sh5le"
#else
#define	_MACHINE_ARCH	sh5
#define	MACHINE_ARCH	"sh5"
#endif

#define	_MACHINE	evbsh5
#define	MACHINE		"evbsh5"

#define	MID_MACHINE	MID_SH5

#endif /* _EVBSH5_PARAM_H */
