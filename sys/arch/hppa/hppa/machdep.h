/*	$NetBSD: machdep.h,v 1.15.14.1 2014/08/20 00:03:04 tls Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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

/*
 * Definitions for the hppa that are completely private 
 * to the machine-dependent code.  Anything needed by
 * machine-independent code is covered in cpu.h or in
 * other headers.
 */

/*
 * XXX there is a lot of stuff in various headers under 
 * hppa/include, and a lot of one-off `extern's in C files
 * that could probably be moved here.
 */

#ifdef _KERNEL

#ifdef _KERNEL_OPT
#include "opt_useleds.h"
#endif

/* The primary (aka monarch) CPU HPA */
extern hppa_hpa_t hppa_mcpuhpa;

/*
 * cache configuration, for most machines is the same
 * numbers, so it makes sense to do defines w/ numbers depending
 * on configured CPU types in the kernel.
 */
extern int icache_stride, icache_line_mask;
extern int dcache_stride, dcache_line_mask;

extern vaddr_t vmmap;	/* XXX - See mem.c */

/* Kernel virtual address space available: */
extern vaddr_t virtual_start, virtual_end;

/* Total physical pages, and low reserved physical pages. */
extern int totalphysmem;
extern int availphysmem;
extern int resvmem;

/* BTLB minimum and maximum sizes, in pages. */
extern u_int hppa_btlb_size_min;
extern u_int hppa_btlb_size_max;

/* FPU variables and functions. */
extern int fpu_present;
extern u_int fpu_version;
extern u_int fpu_csw;
void hppa_fpu_bootstrap(u_int);
void hppa_fpu_flush(struct lwp *);
void hppa_fpu_emulate(struct trapframe *, struct lwp *, u_int);

/* Set up of space registers and protection IDs */
void hppa_setvmspace(struct lwp *);

/* Interrupt dispatching. */
void hppa_intr(struct trapframe *);

/* Special pmap functions. */
void pmap_redzone(vaddr_t, vaddr_t, int);

/* Functions to write low memory and the kernel text. */
void hppa_ktext_stw(vaddr_t, int);
void hppa_ktext_stb(vaddr_t, char);

/* Machine check handling. */
extern u_int os_hpmc;
extern u_int os_hpmc_cont;
extern u_int os_hpmc_cont_end;
int os_toc(void);
extern u_int os_toc_end;
void hppa_machine_check(int);

/* BTLB handling. */
int hppa_btlb_insert(pa_space_t, vaddr_t, paddr_t, vsize_t *, u_int); 
int hppa_btlb_reload(void);
int hppa_btlb_purge(pa_space_t, vaddr_t, vsize_t *);

/* The LEDs. */
#define	HPPA_LED_NETSND		(0)
#define	HPPA_LED_NETRCV		(1)
#define	HPPA_LED_DISK		(2)
#define	HPPA_LED_HEARTBEAT	(3)
#define	_HPPA_LEDS_BLINKABLE	(4)
#define	_HPPA_LEDS_COUNT	(8)

/* This forcefully reboots the machine. */
void cpu_die(void);

/* These map and unmap page zero. */
int hppa_pagezero_map(void);
void hppa_pagezero_unmap(int);

/* Blinking the LEDs. */
#ifdef USELEDS
#define	_HPPA_LED_FREQ		(16)
extern volatile uint8_t *machine_ledaddr;
extern int machine_ledword, machine_leds;
extern int _hppa_led_on_cycles[];
#define hppa_led_blink(i)			\
do {						\
	if (_hppa_led_on_cycles[i] == -1)	\
		_hppa_led_on_cycles[i] = 1;	\
} while (/* CONSTCOND */ 0)
#define hppa_led_on(i, ms)			\
do {						\
	_hppa_led_on_cycles[i] = (((ms) * _HPPA_LED_FREQ) / 1000); \
} while (/* CONSTCOND */ 0)
void hppa_led_ctl(int, int, int);
#else  /* !USELEDS */
#define hppa_led_blink(i)
#define hppa_led_on(i, ms)
#define hppa_led_ctl(off, on, toggle)
#endif /* !USELEDS */

#endif /* _KERNEL */
