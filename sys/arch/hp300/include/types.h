/*	$NetBSD: types.h,v 1.20.42.1 2011/06/23 14:19:11 cherry Exp $	*/

#ifndef _HP300_TYPES_H_
#define	_HP300_TYPES_H_

#include <m68k/types.h>

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_MM_MD_KERNACC

#if defined(_KERNEL)
#define	__HAVE_RAS
#endif

#endif /* !_HP300_TYPES_H_ */
