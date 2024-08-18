/*	$NetBSD: strlcpy-internal.h,v 1.1.1.8 2024/08/18 20:37:40 christos Exp $	*/

#ifndef STRLCPY_INTERNAL_H_INCLUDED_
#define STRLCPY_INTERNAL_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include "event2/event-config.h"
#include "event2/visibility.h"
#include "evconfig-private.h"

#ifndef EVENT__HAVE_STRLCPY
#include <string.h>
EVENT2_EXPORT_SYMBOL
size_t event_strlcpy_(char *dst, const char *src, size_t siz);
#define strlcpy event_strlcpy_
#endif

#ifdef __cplusplus
}
#endif

#endif

