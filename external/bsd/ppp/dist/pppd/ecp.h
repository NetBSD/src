/*	$NetBSD: ecp.h,v 1.5 2025/01/08 19:59:39 christos Exp $	*/

/*
 * ecp.h - Definitions for PPP Encryption Control Protocol.
 *
 * Copyright (c) 2002 Google, Inc.
 * All rights reserved.
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
 * 3. The name(s) of the authors of this software must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef PPP_ECP_H
#define PPP_ECP_H

#include "pppdconf.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PPP_ECP
#define PPP_ECP 0x8053
#endif


typedef struct ecp_options {
    bool required;		/* Is ECP required? */
    unsigned enctype;		/* Encryption type */
} ecp_options;

extern fsm ecp_fsm[];
extern ecp_options ecp_wantoptions[];
extern ecp_options ecp_gotoptions[];
extern ecp_options ecp_allowoptions[];
extern ecp_options ecp_hisoptions[];

extern struct protent ecp_protent;

#ifdef __cplusplus
}
#endif

#endif	// PPP_ECP_H
