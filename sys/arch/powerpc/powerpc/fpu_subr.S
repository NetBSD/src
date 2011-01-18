/*	$NetBSD: fpu_subr.S,v 1.2 2011/01/18 01:02:55 matt Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
 * LINTSTUB: void fpu_load_from_fpreg(const struct fpreg *);
 */
ENTRY(fpu_load_from_fpreg)
	lfd	%f0,FPREG_FPSCR(%r3)
	mtfsf	0xff,%f0

	lfd	%f0,FPREG_F0(%r3)
	lfd	%f1,FPREG_F1(%r3)
	lfd	%f2,FPREG_F2(%r3)
	lfd	%f3,FPREG_F3(%r3)
	lfd	%f4,FPREG_F4(%r3)
	lfd	%f5,FPREG_F5(%r3)
	lfd	%f6,FPREG_F6(%r3)
	lfd	%f7,FPREG_F7(%r3)
	lfd	%f8,FPREG_F8(%r3)
	lfd	%f9,FPREG_F9(%r3)
	lfd	%f10,FPREG_F10(%r3)
	lfd	%f11,FPREG_F11(%r3)
	lfd	%f12,FPREG_F12(%r3)
	lfd	%f13,FPREG_F13(%r3)
	lfd	%f14,FPREG_F14(%r3)
	lfd	%f15,FPREG_F15(%r3)
	lfd	%f16,FPREG_F16(%r3)
	lfd	%f17,FPREG_F17(%r3)
	lfd	%f18,FPREG_F18(%r3)
	lfd	%f19,FPREG_F19(%r3)
	lfd	%f20,FPREG_F20(%r3)
	lfd	%f21,FPREG_F21(%r3)
	lfd	%f22,FPREG_F22(%r3)
	lfd	%f23,FPREG_F23(%r3)
	lfd	%f24,FPREG_F24(%r3)
	lfd	%f25,FPREG_F25(%r3)
	lfd	%f26,FPREG_F26(%r3)
	lfd	%f27,FPREG_F27(%r3)
	lfd	%f28,FPREG_F28(%r3)
	lfd	%f29,FPREG_F29(%r3)
	lfd	%f30,FPREG_F30(%r3)
	lfd	%f31,FPREG_F31(%r3)

	isync
	blr
END(fpu_load_from_fpreg)

/*
 * LINTSTUB: void fpu_unload_to_fpreg(struct fpreg *);
 */
ENTRY(fpu_unload_to_fpreg)
	stfd	%f0,FPREG_F0(%r3)
	stfd	%f1,FPREG_F1(%r3)
	stfd	%f2,FPREG_F2(%r3)
	stfd	%f3,FPREG_F3(%r3)
	stfd	%f4,FPREG_F4(%r3)
	stfd	%f5,FPREG_F5(%r3)
	stfd	%f6,FPREG_F6(%r3)
	stfd	%f7,FPREG_F7(%r3)
	stfd	%f8,FPREG_F8(%r3)
	stfd	%f9,FPREG_F9(%r3)
	stfd	%f10,FPREG_F10(%r3)
	stfd	%f11,FPREG_F11(%r3)
	stfd	%f12,FPREG_F12(%r3)
	stfd	%f13,FPREG_F13(%r3)
	stfd	%f14,FPREG_F14(%r3)
	stfd	%f15,FPREG_F15(%r3)
	stfd	%f16,FPREG_F16(%r3)
	stfd	%f17,FPREG_F17(%r3)
	stfd	%f18,FPREG_F18(%r3)
	stfd	%f19,FPREG_F19(%r3)
	stfd	%f20,FPREG_F20(%r3)
	stfd	%f21,FPREG_F21(%r3)
	stfd	%f22,FPREG_F22(%r3)
	stfd	%f23,FPREG_F23(%r3)
	stfd	%f24,FPREG_F24(%r3)
	stfd	%f25,FPREG_F25(%r3)
	stfd	%f26,FPREG_F26(%r3)
	stfd	%f27,FPREG_F27(%r3)
	stfd	%f28,FPREG_F28(%r3)
	stfd	%f29,FPREG_F29(%r3)
	stfd	%f30,FPREG_F30(%r3)
	stfd	%f31,FPREG_F31(%r3)

	mffs	%f0
	stfd	%f0,FPREG_FPSCR(%r3)

	sync
	blr
END(fpu_unload_to_fpreg)
