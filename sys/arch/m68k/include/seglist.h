/*	$NetBSD: seglist.h,v 1.1 2026/04/04 12:24:41 thorpej Exp $	*/

/*
 * Copyright (c) 1997 The Steve Woodford
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _M68K_SEGLIST_H_
#define	_M68K_SEGLIST_H_

/*
 * Struture describing a physical memory segment.  ps_start and ps_end
 * cover the entire segment, while ps_avail_start and ps_avail_end
 * account for memory stolen from the segment for various reasons.
 * Ultimately, [ps_avail_start,ps_avail_end) is what gets loaded
 * into UVM.
 */
typedef struct {
	paddr_t		ps_start;	/* Start of segment */
	paddr_t		ps_end;		/* End of segment */
	paddr_t		ps_avail_start;	/* Available start of segment */
	paddr_t		ps_avail_end;	/* Available end of segment */
} phys_seg_list_t;

extern phys_seg_list_t phys_seg_list[];

#endif /* _M68K_SEGLIST_H_ */
