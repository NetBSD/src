/*	$NetBSD: types.h,v 1.16.38.1 2017/04/21 16:53:35 bouyer Exp $	*/

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
