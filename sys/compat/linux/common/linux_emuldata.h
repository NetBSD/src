/*	$NetBSD: linux_emuldata.h,v 1.16.14.2 2010/11/06 08:08:25 uebayasi Exp $	*/

/*-
 * Copyright (c) 1998,2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
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

#include <compat/linux/common/linux_futex.h>

#ifndef _COMMON_LINUX_EMULDATA_H
#define _COMMON_LINUX_EMULDATA_H

/*
 * This is auxillary data the linux compat code
 * needs to do its work.  A pointer to it is
 * stored in the emuldata field of the proc
 * structure.
 */

struct linux_emuldata {
	int	led_debugreg[8];	/* GDB information for ptrace - for use, */
				/* see ../arch/i386/linux_ptrace.c */

	void	*led_child_tidptr;	/* Used during clone() */
	void	*led_clear_tid;		/* Own TID to clear on exit */
	struct linux_robust_list_head *led_robust_head;
	long	led_personality;
};

#endif /* !_COMMON_LINUX_EMULDATA_H */
