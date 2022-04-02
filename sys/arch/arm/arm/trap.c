/*	$NetBSD: trap.c,v 1.1 2022/04/02 11:16:06 skrll Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: trap.c,v 1.1 2022/04/02 11:16:06 skrll Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <arm/frame.h>

#include <uvm/uvm_extern.h>

#include <arm/arm32/pmap.h>
#include <arm/arm32/machdep.h>

void
cpu_jump_onfault(struct trapframe *tf, const struct faultbuf *fb, int val)
{
	tf->tf_r4 = fb->fb_reg[FB_R4];
	tf->tf_r5 = fb->fb_reg[FB_R5];
	tf->tf_r6 = fb->fb_reg[FB_R6];
	tf->tf_r7 = fb->fb_reg[FB_R7];
	tf->tf_r8 = fb->fb_reg[FB_R8];
	tf->tf_r9 = fb->fb_reg[FB_R9];
	tf->tf_r10 = fb->fb_reg[FB_R10];
	tf->tf_r11 = fb->fb_reg[FB_R11];
	tf->tf_r12 = fb->fb_reg[FB_R12];
	tf->tf_svc_sp = fb->fb_reg[FB_R13];
	tf->tf_pc = fb->fb_reg[FB_R14];
	tf->tf_r0 = val;
}

