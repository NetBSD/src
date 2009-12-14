/*	$NetBSD: sbdvar.h,v 1.5 2009/12/14 00:46:03 matt Exp $	*/

/*-
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#ifndef _EWS4800MIPS_SBDVAR_H_
#define	_EWS4800MIPS_SBDVAR_H_

#include <sys/kcore.h>		/* phys_ram_seg_t */

#include <machine/autoconf.h>	/* mainbus_attach_args */
#include <machine/vmparam.h>	/* VM_PHYSSEG_MAX */
#include <machine/sbd.h>	/* enum sbd_machine_type */
#include <machine/sbdiovar.h>	/* sbdio_attach_args */

/* Software representation of system board */
struct sbd {
	/* Machine identification */
	enum sbd_machine_type machine;
	char name[32];
	int cpu_clock;

	/* mainbus node table */
	const char **mainbusdevs;

	/* System Board I/O device table */
	const struct sbdiodevdesc *sbdiodevs;

	/* Memory */
	phys_ram_seg_t mem_clseters[VM_PHYSSEG_MAX];
	size_t mem_size; /* total size of memory */

	void (*mem_init)(void *, void *);

	/* Cache configuration (determine L2-cache size) */
	void (*cache_config)(void);

	/* Write buffer */
	void (*wbflush)(void);

	/* Interrupt services */
	void (*intr_init)(void);
	void *(*intr_establish)(int, int (*)(void *), void *);
	void (*intr_disestablish)(void *);
	void (*intr)(uint32_t, uint32_t, vaddr_t, uint32_t);

	/* Interval timer helper routines */
	void (*initclocks)(void);

	/* Miscellaneous */
	void (*consinit)(void);
	int (*ipl_bootdev)(void);
	void (*reboot)(void);
	void (*poweroff)(void);
	void (*ether_addr)(uint8_t *);
};

#define	SBD_DECL(x)							\
void x ## _cache_config(void);						\
void x ## _wbflush(void);						\
void x ## _mem_init(void *, void *);					\
void x ## _intr_init(void);						\
void *x ## _intr_establish(int, int (*)(void *), void *);		\
void x ## _intr_disestablish(void *);					\
void x ## _intr(uint32_t, uint32_t, vaddr_t, uint32_t);			\
void x ## _initclocks(void);						\
void x ## _consinit(void);						\
int x ## _ipl_bootdev(void);						\
void x ## _reboot(void);						\
void x ## _poweroff(void);						\
void x ## _ether_addr(uint8_t *);					\
extern const uint32_t x ## _sr_bits[]

#define	_SBD_OPS_SET(m, x)	platform . x = m ## _ ## x

#define	_SBD_OPS_REGISTER_ALL(m)					\
	_SBD_OPS_SET(m, cache_config);					\
	_SBD_OPS_SET(m, wbflush);					\
	_SBD_OPS_SET(m, mem_init);					\
	_SBD_OPS_SET(m, intr_init);					\
	_SBD_OPS_SET(m, intr_establish);				\
	_SBD_OPS_SET(m, intr_disestablish);				\
	_SBD_OPS_SET(m, intr);						\
	_SBD_OPS_SET(m, initclocks);					\
	_SBD_OPS_SET(m, consinit);				       	\
	_SBD_OPS_SET(m, ipl_bootdev);					\
	_SBD_OPS_SET(m, reboot);					\
	_SBD_OPS_SET(m, poweroff);					\
	_SBD_OPS_SET(m, ether_addr);					\


extern struct sbd platform;

extern int mem_cluster_cnt;
extern phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];

void sbd_init(void);
void sbd_set_mainfo(uint32_t);

void sbd_memcluster_init(uint32_t);
void sbd_memcluster_setup(void *, void *);
void sbd_memcluster_check(void);

void tr2_init(void);
void tr2a_init(void);

#endif /* !_EWS4800MIPS_SBDVAR_H_ */

