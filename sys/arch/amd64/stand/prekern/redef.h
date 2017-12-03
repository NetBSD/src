/*	$NetBSD: redef.h,v 1.2.2.2 2017/12/03 11:35:48 jdolecek Exp $	*/

/*
 * Copyright (c) 2017 The NetBSD Foundation, Inc. All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

#define PAGE_SIZE 4096

/* --------------------------------------------------------------------------
 * *** dev/isa/isareg.h *****************************************************
 * -------------------------------------------------------------------------- */

#define	IOM_BEGIN	0x0a0000		/* Start of I/O Memory "hole" */
#define	IOM_END		0x100000		/* End of I/O Memory "hole" */
#define	IOM_SIZE	(IOM_END - IOM_BEGIN)

/* --------------------------------------------------------------------------
 * *** amd64/genassym.cf ****************************************************
 * -------------------------------------------------------------------------- */

#define PDE_SIZE 8
#define FRAMESIZE 240
