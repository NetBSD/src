/*
 * multilink.h - support routines for multilink.
 *
 * Copyright (c) 2000-2024 Paul Mackerras. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef PPP_MULTILINK_H
#define PPP_MULTILINK_H

#include "pppdconf.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * values for epdisc.class
 */
#define EPD_NULL        0	/* null discriminator, no data */
#define EPD_LOCAL       1
#define EPD_IP          2
#define EPD_MAC         3
#define EPD_MAGIC       4
#define EPD_PHONENUM    5

struct epdisc;

#ifdef PPP_WITH_MULTILINK

/*
 * Check multilink-related options
 */
void mp_check_options(void);

/*
 * Join our link to an appropriate bundle
 */
int mp_join_bundle(void);

/*
 * Disconnected our link from the bundle
 */
void mp_exit_bundle(void);

/*
 * Multipoint bundle terminated
 */
void mp_bundle_terminated(void);

/*
 * Acting as a multilink master
 */
bool mp_master(void);

/*
 * Was multilink negotiated
 */
bool mp_on(void);

/*
 * Convert an endpoint discriminator to a string
 */
char *epdisc_to_str(struct epdisc *);

/*
 * Convert a string to an endpoint discriminator
 */
int str_to_epdisc(struct epdisc *, char *);

/*
 * Hook for plugin to hear when an interface joins a multilink bundle
 */
typedef void (multilink_join_hook_fn)(void);
extern multilink_join_hook_fn *multilink_join_hook;

#else

#define mp_check_options(x)     ((void)0)
#define mp_join_bundle(x)       ((void)0)
#define mp_exit_bundle(x)       ((void)0)
#define mp_bundle_terminated(x) ((void)0)

static inline bool mp_on() {
    return false;
}

static inline bool mp_master() {
    return false;
}

#endif // PPP_WITH_MULTILINK

#ifdef __cplusplus
}
#endif

#endif // PPP_MULTILINK_H
