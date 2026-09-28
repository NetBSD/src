/*	$NetBSD: ebcvar.h,v 1.1 2026/09/28 20:41:01 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

#ifndef _POWERPC_IBM4XX_DEV_EBCVAR_H_
#define _POWERPC_IBM4XX_DEV_EBCVAR_H_

#include <sys/bus.h>

/*
 * External Bus Controller: one child per bank.
 */
struct ebc_attach_args {
	bus_space_tag_t	ebc_bt;
	bus_dma_tag_t	ebc_dmat;
	int		ebc_bank;	/* CS line number */
	bus_addr_t	ebc_addr;	/* bank base addr */
	bus_size_t	ebc_size;	/* bank size */
	int		ebc_width;	/* data bus width, bits */
	int		ebc_usage;	/* EBC_USAGE_* */
};

#define	EBC_USAGE_DISABLED	0
#define	EBC_USAGE_RO		1
#define	EBC_USAGE_WO		2
#define	EBC_USAGE_RW		3

#endif	/* _POWERPC_IBM4XX_DEV_EBCVAR_H_ */
