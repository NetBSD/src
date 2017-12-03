/* $NetBSD: display_timing.c,v 1.1.10.2 2017/12/03 11:37:01 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: display_timing.c,v 1.1.10.2 2017/12/03 11:37:01 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/display_timing.h>

#define	GETPROP(n, v)	\
	of_getprop_uint32(phandle, (n), (v))

int
display_timing_parse(int phandle, struct display_timing *timing)
{
	if (GETPROP("clock-frequency", &timing->clock_freq) ||
	    GETPROP("hactive", &timing->hactive) ||
	    GETPROP("vactive", &timing->vactive) ||
	    GETPROP("hfront-porch", &timing->hfront_porch) ||
	    GETPROP("hback-porch", &timing->hback_porch) ||
	    GETPROP("hsync-len", &timing->hsync_len) ||
	    GETPROP("vfront-porch", &timing->vfront_porch) ||
	    GETPROP("vback-porch", &timing->vback_porch) ||
	    GETPROP("vsync-len", &timing->vsync_len))
		return EINVAL;

	return 0;
}
