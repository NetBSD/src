/*	$NetBSD: types.h,v 1.16 2023/02/05 22:42:39 mrg Exp $	*/

#ifndef _EVBARM_TYPES_H_
#define	_EVBARM_TYPES_H_

#define	__HAVE_CPU_BOOTCONF

#ifdef __aarch64__
#include <aarch64/types.h>

#define	__HAVE_DEVICE_REGISTER
#else
#include <arm/arm32/types.h>

#define	__HAVE_NEW_STYLE_BUS_H
#define	__HAVE_COMPAT_NETBSD32

#endif
#endif
