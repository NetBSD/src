/*	$NetBSD: npf_var.h,v 1.6 2012/11/26 20:34:28 rmind Exp $	*/

/*-
 * Copyright (c) 2011-2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NPF_VAR_H_
#define _NPF_VAR_H_

#define	NPFVAR_STRING		0
#define	NPFVAR_IDENTIFIER	1
#define	NPFVAR_VAR_ID		2
#define	NPFVAR_NUM		3
#define	NPFVAR_PORT_RANGE	4

/* Note: primitive types are equivalent. */
#define	NPFVAR_PRIM		NPFVAR_PORT_RANGE
#define	NPFVAR_TYPE(x)		(((x) > NPFVAR_PRIM) ? (x) : 0)

#define	NPFVAR_TABLE		5
#define	NPFVAR_FAM		6
#define	NPFVAR_PROC		7
#define	NPFVAR_PROC_PARAM	8
#define	NPFVAR_TCPFLAG		9
#define	NPFVAR_ICMP		10
#define	NPFVAR_ICMP6		11
#define	NPFVAR_INTERFACE	12

#ifdef _NPFVAR_PRIVATE
static const char *npfvar_types[ ] = {
	[NPFVAR_STRING]		= "string",
	[NPFVAR_IDENTIFIER]	= "identifier",
	[NPFVAR_VAR_ID]		= "variable-id",
	[NPFVAR_NUM]		= "number",
	[NPFVAR_PORT_RANGE]	= "port-range",
	[NPFVAR_TABLE]		= "table",
	[NPFVAR_FAM]		= "family-address-mask",
	[NPFVAR_PROC]		= "procedure",
	[NPFVAR_PROC_PARAM]	= "procedure-parameter",
	[NPFVAR_TCPFLAG]	= "tcp-flag",
	[NPFVAR_ICMP]		= "icmp",
	[NPFVAR_ICMP6]		= "icmp6",
	[NPFVAR_INTERFACE]	= "interface"
};
#endif

struct npfvar;
typedef struct npfvar npfvar_t;

npfvar_t *	npfvar_create(const char *);
npfvar_t *	npfvar_lookup(const char *);
const char *	npfvar_type(size_t);
void		npfvar_add(npfvar_t *);
npfvar_t *	npfvar_add_element(npfvar_t *, int, const void *, size_t);
npfvar_t *	npfvar_add_elements(npfvar_t *, npfvar_t *);
void		npfvar_destroy(npfvar_t *);

char *		npfvar_expand_string(const npfvar_t *);
size_t		npfvar_get_count(const npfvar_t *);
int		npfvar_get_type(const npfvar_t *, size_t);
void *		npfvar_get_data(const npfvar_t *, int, size_t);

#endif
