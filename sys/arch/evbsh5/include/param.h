/*	$NetBSD: param.h,v 1.2.4.2 2002/09/06 08:34:51 jdolecek Exp $	*/

#ifndef _EVBSH5_PARAM_H
#define _EVBSH5_PARAM_H

#include <sh5/param.h>

#ifdef __LITTLE_ENDIAN__
#define	_MACHINE_ARCH	sh5el
#define	MACHINE_ARCH	"sh5el"
#else
#define	_MACHINE_ARCH	sh5eb
#define	MACHINE_ARCH	"sh5eb"
#endif

#define	_MACHINE	evbsh5
#define	MACHINE		"evbsh5"

#endif /* _EVBSH5_PARAM_H */
