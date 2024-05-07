/*	$NetBSD: volhdr.h,v 1.6 2024/05/07 19:55:14 tsutsui Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)volhdr.h	8.1 (Berkeley) 6/10/93
 */

/*
 * vohldr.h: volume header for "LIF" format volumes
 */

struct	lifvol {
	uint16_t	vol_id;
	char		vol_label[6];
	uint32_t	vol_addr;
	uint16_t	vol_oct;
	uint16_t	vol_dummy;
	uint32_t	vol_dirsize;
	uint16_t	vol_version;
	uint16_t	vol_zero;
	uint32_t	vol_huh1;
	uint32_t	vol_huh2;
	uint32_t	vol_length;
};

struct	lifdir {
	char		dir_name[10];
	uint16_t	dir_type;
	uint32_t	dir_addr;
	uint32_t	dir_length;
	char		dir_toc[6];
	uint16_t	dir_flag;
	int32_t	dir_exec;
};

/* load header for boot rom */
struct load {
	uint32_t	address;
	uint32_t	count;
};

#define VOL_ID		0x8000	/* always $8000 */
#define VOL_OCT		4096
#define DIR_TYPE	0xe942	/* "SYS9k Series 9000" */
#define DIR_FLAG	0x8001	/* don't ask me! */
#define SECTSIZE	256
