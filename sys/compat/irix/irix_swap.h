/*	$NetBSD: irix_swap.h,v 1.2 2002/03/18 20:34:54 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _IRIX_SWAPCTL_H_
#define _IRIX_SWAPCTL_H_

/* From IRIX's <sys/swap.h> */
#define IRIX_SC_ADD		1
#define IRIX_SC_LIST		2
#define IRIX_SC_REMOVE		3
#define IRIX_SC_GETNSWP		4
#define IRIX_SC_SGIADD		100
#define IRIX_SC_KSGIADD		101
#define IRIX_SC_LREMOVE		102
#define IRIX_SC_GETFREESWAP	103
#define IRIX_SC_GETSWAPMAX	104
#define IRIX_SC_GETSWAPVIRT	105
#define IRIX_SC_GETRESVSWAP	106
#define IRIX_SC_GETSWAPTOT	107
#define IRIX_SC_GETLSWAPTOT	108

struct irix_swapent {
	char *ste_path;
	irix_off_t ste_start;
	irix_off_t ste_length;
	long ste_pages;
	long ste_free;
	long ste_flags;
	long ste_vpages;
	long ste_maxpages;
	short ste_lswap;
	signed char ste_pri;
};

struct irix_swaptable {
	int swt_n;
	struct irix_swapent swt_ent[1];
};

#define IRIX_ST_INDEL		0x01
#define IRIX_ST_NOTREADY	0x02
#define IRIX_ST_STALE		0x04
#define IRIX_ST_LOCAP_SWAP	0x08
#define IRIX_ST_IOERR		0x10
#define IRIX_ST_EACCESS		0x20
#define IRIX_ST_BOOTSWAP	0x40

struct irix_swapres {
	char *sr_name;
	irix_off_t sr_start;
	irix_off_t sr_length;
};

struct irix_xswapres {
	char *sr_name;
	irix_off_t sr_start;
	irix_off_t sr_length;
	irix_off_t sr_maxlength;
	irix_off_t sr_vlength;
	signed char sr_pri;
};

#endif /* _IRIX_SWAPCTL_H_ */
