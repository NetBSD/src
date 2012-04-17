/* $NetBSD: sysarch.h,v 1.19.2.1 2012/04/17 00:05:56 yamt Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _ALPHA_SYSARCH_H_
#define _ALPHA_SYSARCH_H_

#include <machine/bus_user.h>
#include <machine/ieeefp.h>

/*
 * Architecture specific syscalls (ALPHA)
 */

#define	ALPHA_FPGETMASK			0
#define	ALPHA_FPSETMASK			1
#define	ALPHA_FPSETSTICKY		2
#define	ALPHA_BUS_GET_WINDOW_COUNT	3
#define	ALPHA_BUS_GET_WINDOW		4
#define	ALPHA_PCI_CONF_READWRITE	5
#define	ALPHA_FPGETSTICKY		6
#define	ALPHA_GET_FP_C			7
#define	ALPHA_SET_FP_C			8

struct alpha_fp_except_args {
	fp_except mask;
};

struct alpha_fp_c_args {
	uint64_t fp_c;
};

struct alpha_bus_get_window_count_args {
	u_int type;
	u_int count;	/* output */
};

struct alpha_bus_get_window_args {
	u_int type;
	u_int window;
	struct alpha_bus_space_translation *translation; /* output */
};

#define	ALPHA_BUS_TYPE_PCI_IO		0
#define	ALPHA_BUS_TYPE_PCI_MEM		1
#define	ALPHA_BUS_TYPE_MAX		1

struct alpha_pci_conf_readwrite_args {
	int write;
	u_int bus;
	u_int device;
	u_int function;
	u_int reg;
	uint32_t val;
};

#ifdef _KERNEL
extern	u_int alpha_bus_window_count[];
extern	int (*alpha_bus_get_window)(int, int,
	    struct alpha_bus_space_translation *);
extern	struct alpha_pci_chipset *alpha_pci_chipset;
#else
#include <sys/cdefs.h>

struct alpha_bus_window {
	void *		abw_addr;
	size_t		abw_size;
	struct alpha_bus_space_translation abw_abst;
};

__BEGIN_DECLS
int	alpha_bus_getwindows(int, struct alpha_bus_window **);
int	alpha_bus_mapwindow(struct alpha_bus_window *);
void	alpha_bus_unmapwindow(struct alpha_bus_window *);

void	*alpha_pci_mem_map(bus_addr_t, bus_size_t, int,
	    struct alpha_bus_space_translation *);
void	alpha_pci_mem_unmap(struct alpha_bus_space_translation *,
	    void *addr, bus_size_t);

uint32_t alpha_pci_conf_read(u_int, u_int, u_int, u_int);
void	alpha_pci_conf_write(u_int, u_int, u_int, u_int, uint32_t);

int	sysarch(int, void *);
__END_DECLS
#endif /* _KERNEL */

#endif /* !_ALPHA_SYSARCH_H_ */
