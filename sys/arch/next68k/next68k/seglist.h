/*	$NetBSD: seglist.h,v 1.1.1.1 1998/06/09 07:53:06 dbj Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Steve Woodford.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/*
 * The following structure is passed to pmap_bootstrap by the startup
 * code in locore.s.
 * It simply describes the start and end addresses of the memory
 * segments available to the board.
 * If the offboard RAM segment spans multiple boards, they must be
 * configured to appear physically contiguous in the VMEbus address
 * space.
 *
 * NOTE: If you change this, you'll need to update locore.s ...
 */
struct phys_seg_list_t {
	vm_offset_t	ps_start;	/* Start of segment */
	vm_offset_t	ps_end;		/* End of segment */
	int		ps_startpage;	/* Page number of first page */
};

#define	N_SIMM		4		/* number of SIMMs in machine */

/* SIMM types */
#define SIMM_SIZE       0x03
#define	SIMM_SIZE_EMPTY	0x00
#define	SIMM_SIZE_16MB	0x01
#define	SIMM_SIZE_4MB	0x02
#define	SIMM_SIZE_1MB	0x03
#define	SIMM_PAGE_MODE	0x04
#define	SIMM_PARITY	0x08 /* ?? */

/* Space for onboard RAM
 */
#define	MAX_PHYS_SEGS	N_SIMM+1

/* Instantiated in pmap.c */
/* size +1 is for list termination */
extern struct phys_seg_list_t phys_seg_list[MAX_PHYS_SEGS];
