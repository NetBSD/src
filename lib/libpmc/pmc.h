/*	$NetBSD: pmc.h,v 1.2 2005/02/04 15:27:44 perry Exp $	*/

/*
 * Copyright (c 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PMC_H_
#define	_PMC_H_

#include <sys/types.h>
#include <sys/pmc.h>

struct pmc_event {
	const char *name;
	pmc_evid_t val;
};

__BEGIN_DECLS
int	pmc_configure_counter(int, const char *, pmc_ctr_t, uint32_t);
int	pmc_start_counter(int);
int	pmc_stop_counter(int);
int	pmc_get_num_counters(void);
int	pmc_get_counter_class(void);
int	pmc_get_counter_type(int, int *);
int	pmc_get_counter_value(int, uint64_t *);
int	pmc_get_accumulated_counter_value(int, uint64_t *);

const char *pmc_get_counter_class_name(int);
const char *pmc_get_counter_type_name(int);
const char *pmc_get_counter_event_name(pmc_evid_t);
const struct pmc_event *pmc_get_counter_event_list(void);
__END_DECLS

#endif /* _PMC_H_ */
