/*	$NetBSD: types.h,v 1.16.30.1 2017/02/05 13:40:20 skrll Exp $	*/

#ifndef _SGIMIPS_TYPES_H_
#define	_SGIMIPS_TYPES_H_

#define _MIPS_PADDR_T_64BIT

#include <mips/types.h>

#define	__HAVE_NEW_STYLE_BUS_H

/* MIPS specific options */
#define	__HAVE_BOOTINFO_H
#define	__HAVE_MIPS_MACHDEP_CACHE_CONFIG
#define	__HAVE_COMPAT_NETBSD32

#endif
