/*	$NetBSD: seglist.h,v 1.5.150.1 2010/03/11 15:02:46 yamt Exp $ */

/*
 * This file was taken from from mvme68k/mvme68k/seglist.h
 * should probably be re-synced when needed.
 * Darrin B. Jewell <jewell@mit.edu>  Tue Nov 10 05:07:16 1998
 * original cvs id: NetBSD: seglist.h,v 1.4 1998/08/22 10:55:35 scw Exp
 */

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
typedef struct {
	paddr_t		ps_start;	/* Start of segment */
	paddr_t		ps_end;		/* End of segment */
	int		ps_startpage;	/* Page number of first page */
} phys_seg_list_t;

/* Instantiated in machdep.c */
extern phys_seg_list_t phys_seg_list[];
