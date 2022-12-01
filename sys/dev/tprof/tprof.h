/*	$NetBSD: tprof.h,v 1.7 2022/12/01 00:32:52 ryo Exp $	*/

/*-
 * Copyright (c)2008,2009,2010 YAMAMOTO Takashi,
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _DEV_TPROF_TPROF_H_
#define _DEV_TPROF_TPROF_H_

/*
 * definitions used by backend drivers
 */

#include <sys/types.h>

#include <dev/tprof/tprof_types.h>

struct tprof_backend_softc_counter {
	tprof_param_t ctr_param;
	u_int ctr_bitwidth;
	uint64_t ctr_counter_val;
	uint64_t ctr_counter_reset_val;
};

typedef struct tprof_backend_softc {
	u_int sc_ncounters;
	tprof_countermask_t sc_ctr_running_mask;/* start/stop */
	tprof_countermask_t sc_ctr_configured_mask;	/* configured */
	tprof_countermask_t sc_ctr_ovf_mask;	/* overflow intr required */
	tprof_countermask_t sc_ctr_prof_mask;	/* profiled */
	percpu_t *sc_ctr_offset_percpu;
	size_t sc_ctr_offset_percpu_size;
	struct tprof_backend_softc_counter sc_count[TPROF_MAXCOUNTERS];
} tprof_backend_softc_t;

typedef struct tprof_backend_ops {
	uint32_t (*tbo_ident)(void);
	u_int (*tbo_ncounters)(void);
	u_int (*tbo_counter_bitwidth)(u_int);
	uint64_t (*tbo_counter_read)(u_int);
	uint64_t (*tbo_counter_estimate_freq)(u_int);
	int (*tbo_valid_event)(u_int, const tprof_param_t *);
	void (*tbo_configure_event)(u_int, const tprof_param_t *);
	void (*tbo_start)(tprof_countermask_t);
	void (*tbo_stop)(tprof_countermask_t);
	int (*tbo_establish)(tprof_backend_softc_t *);
	void (*tbo_disestablish)(tprof_backend_softc_t *);
} tprof_backend_ops_t;

#define	TPROF_BACKEND_VERSION	4
int tprof_backend_register(const char *, const tprof_backend_ops_t *, int);
int tprof_backend_unregister(const char *);

typedef struct {
	uintptr_t tfi_pc;	/* program counter */
	u_int tfi_counter;	/* counter. 0..(TPROF_MAXCOUNTERS-1) */
	bool tfi_inkernel;	/* if tfi_pc is in the kernel address space */
} tprof_frame_info_t;

void tprof_sample(void *, const tprof_frame_info_t *);

#endif /* _DEV_TPROF_TPROF_H_ */
