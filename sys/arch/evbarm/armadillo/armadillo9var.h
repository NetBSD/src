/*	$NetBSD: armadillo9var.h,v 1.2.10.2 2006/04/22 11:37:22 simonb Exp $	*/

/*
 * Copyright (c) 2006 HAMAJIMA Katsuomi. All rights reserved.
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

#ifndef _ARMADILLO9VAR_H_
#define	_ARMADILLO9VAR_H_

/* model type */

#define DEVCFG_ARMADILLO9	0x08140000
#define DEVCFG_ARMADILLO210	0x0a140d00

struct armadillo_model_t {
	unsigned long devcfg;
	const char *name;
};

extern struct armadillo_model_t *armadillo_model;


/* information from bootloader */

#define BOOTPARAM_TAG_NONE	0x00000000
#define BOOTPARAM_TAG_MEM	0x54410002
#define BOOTPARAM_TAG_CMDLINE	0x54410009

struct bootparam_tag_header {
	unsigned long  size;
	unsigned long  tag;
};

struct bootparam_tag_mem32 {
	unsigned long 	size;
	unsigned long 	start;
};

struct bootparam_tag_cmdline {
	char	cmdline[1];
};

struct bootparam_tag {
	struct bootparam_tag_header hdr;
	union {
		struct bootparam_tag_mem32	mem;
		struct bootparam_tag_cmdline	cmdline;
	} u;
};

#define bootparam_tag_next(t)		\
	((struct bootparam_tag *)((unsigned long *)(t) + (t)->hdr.size))

extern char bootparam[];

extern uint8_t armadillo9_ethaddr[];

#endif /* _ARMADILLO9VAR_H_ */
