/*	$NetBSD: platid.h,v 1.2 2001/03/25 12:17:35 takemura Exp $	*/

/*-
 * Copyright (c) 1999-2001
 *         Shin Takemura and PocketBSD Project. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _HPC_PLATID_H_
#define _HPC_PLATID_H_

#include <sys/cdefs.h>

#define PLATID_FLAGS_BITS		8
#define PLATID_CPU_SUBMODEL_BITS	6
#define PLATID_CPU_MODEL_BITS		6
#define PLATID_CPU_SERIES_BITS		6
#define PLATID_CPU_ARCH_BITS		6

#define PLATID_SUBMODEL_BITS		8
#define PLATID_MODEL_BITS		8
#define PLATID_SERIES_BITS		6
#define PLATID_VENDOR_BITS		10

typedef union {
	struct {
		unsigned long dw0, dw1;
	} dw;
#if BYTE_ORDER == LITTLE_ENDIAN
	struct platid {
		unsigned int 	flags		:PLATID_FLAGS_BITS;
		unsigned int	cpu_submodel	:PLATID_CPU_SUBMODEL_BITS;
		unsigned int	cpu_model	:PLATID_CPU_MODEL_BITS;
		unsigned int	cpu_series	:PLATID_CPU_SERIES_BITS;
		unsigned int	cpu_arch	:PLATID_CPU_ARCH_BITS;

		unsigned int 	submodel	:PLATID_SUBMODEL_BITS;
		unsigned int	model		:PLATID_MODEL_BITS;
		unsigned int	series		:PLATID_SERIES_BITS;
		unsigned int	vendor		:PLATID_VENDOR_BITS;
	} s;
#else
#error BYTE_ORDER != LITTLE_ENDIAN
#endif
} platid_t;

typedef platid_t platid_mask_t;

#ifdef UNICODE
typedef unsigned short tchar;
#ifndef TEXT
#define TEXT(x)		L##x
#endif
#else
typedef char tchar;
#define TEXT(x)		x
#endif

struct platid_name {
	platid_mask_t *mask;
	tchar *name;
};

struct platid_data {
	platid_mask_t *mask;
	void *data;
};

#define PLATID_FLAGS_SHIFT		0
#define PLATID_CPU_SUBMODEL_SHIFT	8
#define PLATID_CPU_MODEL_SHIFT		14
#define PLATID_CPU_SERIES_SHIFT		20
#define PLATID_CPU_ARCH_SHIFT		26

#define PLATID_SUBMODEL_SHIFT		0
#define PLATID_MODEL_SHIFT		8
#define PLATID_SERIES_SHIFT		16
#define PLATID_VENDOR_SHIFT		22

#define PLATID_FLAGS_MASK						\
	(((1 << PLATID_FLAGS_BITS) - 1) << PLATID_FLAGS_SHIFT)
#define PLATID_CPU_SUBMODEL_MASK					\
	(((1 << PLATID_CPU_SUBMODEL_BITS) - 1) << PLATID_CPU_SUBMODEL_SHIFT)
#define PLATID_CPU_MODEL_MASK						\
	(((1 << PLATID_CPU_MODEL_BITS) - 1) << PLATID_CPU_MODEL_SHIFT)
#define PLATID_CPU_SERIES_MASK						\
	(((1 << PLATID_CPU_SERIES_BITS) - 1) << PLATID_CPU_SERIES_SHIFT)
#define PLATID_CPU_ARCH_MASK						\
	(((1 << PLATID_CPU_ARCH_BITS) - 1) << PLATID_CPU_ARCH_SHIFT)

#define PLATID_SUBMODEL_MASK						\
	(((1 << PLATID_SUBMODEL_BITS) - 1) << PLATID_SUBMODEL_SHIFT)
#define PLATID_MODEL_MASK						\
	(((1 << PLATID_MODEL_BITS) - 1) << PLATID_MODEL_SHIFT)
#define PLATID_SERIES_MASK						\
	(((1 << PLATID_SERIES_BITS) - 1) << PLATID_SERIES_SHIFT)
#define PLATID_VENDOR_MASK						\
	(((1 << PLATID_VENDOR_BITS) - 1) << PLATID_VENDOR_SHIFT)

#define	PLATID_UNKNOWN		0
#define	PLATID_WILD		PLATID_UNKNOWN
#define PLATID_DEREFP(pp)	((platid_t *)(pp))
#define PLATID_DEREF(pp)	(*PLATID_DEREFP(pp))

__BEGIN_DECLS
extern platid_t platid_unknown;
extern platid_t platid_wild;
extern platid_t platid;
extern struct platid_name platid_name_table[];
extern int platid_name_table_size;

void platid_ntoh(platid_t *);
void platid_hton(platid_t *);
void platid_dump(char *, void *);
int platid_match(platid_t *, platid_mask_t *);
int platid_match_sub(platid_t *, platid_mask_t *, int);
tchar* platid_name(platid_t *);
struct platid_data *platid_search(platid_t *, struct platid_data *);
__END_DECLS

#include <machine/platid_generated.h>

#endif /* _HPC_PLATID_H_ */
