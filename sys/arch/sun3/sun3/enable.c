/*	$NetBSD: enable.c,v 1.7.50.1 2014/05/18 17:45:27 rmind Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
__KERNEL_RCSID(0, "$NetBSD: enable.c,v 1.7.50.1 2014/05/18 17:45:27 rmind Exp $");

#include <sys/param.h>
#include <dev/sun/fbio.h>
#include <sun3/dev/fbvar.h>

#include <sun3/sun3/control.h>
#include <sun3/sun3/enable.h>
#include <sun3/sun3/machdep.h>

/*
 * External interfaces to the system enable register.
 */

void
enable_fpu(int on)
{
	int s, ena;

	s = splhigh();
	ena = get_control_byte(SYSTEM_ENAB);

	if (on)
		ena |= ENA_FPP;
	else
		ena &= ~ENA_FPP;

	set_control_byte(SYSTEM_ENAB, ena);
	splx(s);
}

void
enable_video(int on)
{
	int s, ena;

	s = splhigh();
	ena = get_control_byte(SYSTEM_ENAB);

	if (on)
		ena |= ENA_VIDEO;
	else
		ena &= ~ENA_VIDEO;

	set_control_byte(SYSTEM_ENAB, ena);
	splx(s);
}
