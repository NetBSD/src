/*	$NetBSD: types.h,v 1.15.16.1 2023/10/20 16:13:04 martin Exp $	*/

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
