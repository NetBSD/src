/*	$NetBSD: volhdr.h,v 1.4.108.1 2011/02/17 11:59:41 bouyer Exp $	*/

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
	int16_t	vol_id;
	char	vol_label[6];
	int32_t	vol_addr;
	int16_t	vol_oct;
	int16_t	vol_dummy;
	int32_t	vol_dirsize;
	int16_t	vol_version;
	int16_t	vol_zero;
	int32_t	vol_huh1;
	int32_t	vol_huh2;
	int32_t	vol_length;
};

struct	lifdir {
	char	dir_name[10];
	uint16_t dir_type;
	int32_t	dir_addr;
	int32_t	dir_length;
	char	dir_toc[6];
	int16_t dir_flag;
	int32_t	dir_exec;
};

/* load header for boot rom */
struct load {
	int32_t address;
	int32_t count;
};

#define VOL_ID		-32768
#define VOL_OCT		4096
#define	DIR_TYPE	-5822
#define DIR_FLAG	0x8001	/* dont ask me! */
#define	SECTSIZE	256
