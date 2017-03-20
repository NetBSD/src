/*	$NetBSD: strlcpy-internal.h,v 1.1.1.2.12.1 2017/03/20 06:52:22 pgoyette Exp $	*/
#ifndef STRLCPY_INTERNAL_H_INCLUDED_
#define STRLCPY_INTERNAL_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include "event2/event-config.h"
#include "evconfig-private.h"

#ifndef EVENT__HAVE_STRLCPY
#include <string.h>
size_t event_strlcpy_(char *dst, const char *src, size_t siz);
#define strlcpy event_strlcpy_
#endif

#ifdef __cplusplus
}
#endif

#endif

