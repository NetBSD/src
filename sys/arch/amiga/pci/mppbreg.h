/*	$NetBSD: mppbreg.h,v 1.1 2011/09/17 16:55:34 rkujawa Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

/*
 * Prometheus PCI spaces. All base addresses are relative to the card's 
 * address.
 */
#ifndef _AMIGA_MPPBREG_H_

#define MPPB_CONF_BASE		0x000F0000
#define MPPB_CONF_SIZE		0x000060FF
#define MPPB_CONF_STRIDE	0x00002000	/* 0x2000 per slot */

#define MPPB_IO_BASE		0x00000000
#define MPPB_IO_SIZE		0x000EFFFF

#define MPPB_MEM_BASE		0x00100000
#define MPPB_MEM_SIZE		0x1FEFFFFF

/* All PCI interrupt lines are wired to INT2 */
#define MPPB_INT		2

#endif /* _AMIGA_MPPBREG_H_ */

