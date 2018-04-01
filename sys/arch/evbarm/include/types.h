/*	$NetBSD: types.h,v 1.14 2018/04/01 04:35:05 ryo Exp $	*/

#ifndef _EVBARM_TYPES_H_
#define	_EVBARM_TYPES_H_

#ifdef __aarch64__
#include <aarch64/types.h>

#define __HAVE_DEVICE_REGISTER
#else
#include <arm/arm32/types.h>

#define	__HAVE_NEW_STYLE_BUS_H
#define	__HAVE_COMPAT_NETBSD32

#endif
#endif
