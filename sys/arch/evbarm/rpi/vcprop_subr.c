/*	$NetBSD: vcprop_subr.c,v 1.1 2014/09/28 14:38:29 macallan Exp $	*/

/*
 * Copyright (c) 2014 Michael Lorenz
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
 * Mailbox property interface wrapper functions
 */
 
#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <arm/arm32/machdep.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835var.h>
#include <arm/broadcom/bcm2835_pmvar.h>
#include <arm/broadcom/bcm2835_mbox.h>

#include <evbarm/rpi/vcio.h>
#include <evbarm/rpi/vcpm.h>
#include <evbarm/rpi/vcprop.h>

#include <evbarm/rpi/rpi.h>

#include <dev/wscons/wsconsio.h>

int
rpi_fb_set_video(int b)
{
	int error;
	uint32_t res;

	/*
	 * might as well put it here since we need to re-init it every time
	 * and it's not like this is going to be called very often anyway
	 */
	struct __aligned(16) {
		struct vcprop_buffer_hdr	vb_hdr;
		struct vcprop_tag_blankscreen	vbt_blank;
		struct vcprop_tag end;
	} vb_setblank =
	{
		.vb_hdr = {
			.vpb_len = sizeof(vb_setblank),
			.vpb_rcode = VCPROP_PROCESS_REQUEST,
		},
		.vbt_blank = {
			.tag = {
				.vpt_tag = VCPROPTAG_BLANK_SCREEN,
				.vpt_len = VCPROPTAG_LEN(vb_setblank.vbt_blank),
				.vpt_rcode = VCPROPTAG_REQUEST,
			},
			.state = (b != 0) ? VCPROP_BLANK_OFF : VCPROP_BLANK_ON,
		},
		.end = {
			.vpt_tag = VCPROPTAG_NULL,
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_setblank,
	    sizeof(vb_setblank), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %d %d %d %08x %08x\n", __func__, b,
	    vb_setblank.vbt_blank.state, error, res,
	    vb_setblank.vbt_blank.tag.vpt_rcode);
#endif
	return (error == 0);
}

uint32_t
rpi_alloc_mem(uint32_t size, uint32_t align, uint32_t flags)
{
	int error;
	uint32_t res;

	struct __aligned(16) {
		struct vcprop_buffer_hdr	vb_hdr;
		struct vcprop_tag_allocmem	vbt_am;
		struct vcprop_tag end;
	} vb_allocmem =
	{
		.vb_hdr = {
			.vpb_len = sizeof(vb_allocmem),
			.vpb_rcode = VCPROP_PROCESS_REQUEST,
		},
		.vbt_am = {
			.tag = {
				.vpt_tag = VCPROPTAG_ALLOCMEM,
				.vpt_len = VCPROPTAG_LEN(vb_allocmem.vbt_am),
				.vpt_rcode = VCPROPTAG_REQUEST,
			},
			.size = size,
			.align = align,
			.flags = flags,
		},
		.end = {
			.vpt_tag = VCPROPTAG_NULL,
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_allocmem,
	    sizeof(vb_allocmem), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %d %d %08x %08x\n", __func__,
	    vb_allocmem.vbt_am.size, error, res,
	    vb_allocmem.vbt_am.tag.vpt_rcode);
#endif
	if (error == 0)
		return vb_allocmem.vbt_am.size;
	return 0;
}

bus_addr_t
rpi_lock_mem(uint32_t handle)
{
	int error;
	uint32_t res;

	struct __aligned(16) {
		struct vcprop_buffer_hdr	vb_hdr;
		struct vcprop_tag_lockmem	vbt_lm;
		struct vcprop_tag end;
	} vb_lockmem =
	{
		.vb_hdr = {
			.vpb_len = sizeof(vb_lockmem),
			.vpb_rcode = VCPROP_PROCESS_REQUEST,
		},
		.vbt_lm = {
			.tag = {
				.vpt_tag = VCPROPTAG_LOCKMEM,
				.vpt_len = VCPROPTAG_LEN(vb_lockmem.vbt_lm),
				.vpt_rcode = VCPROPTAG_REQUEST,
			},
			.handle = handle,
		},
		.end = {
			.vpt_tag = VCPROPTAG_NULL,
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_lockmem,
	    sizeof(vb_lockmem), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %d %d %08x %08x\n", __func__,
	    vb_lockmem.vbt_lm.handle, error, res,
	    vb_lockmem.vbt_lm.tag.vpt_rcode);
#endif
	if (error == 0)
		return (vb_lockmem.vbt_lm.handle /*& 0x3fffffff*/);
	return 0;
}

int
rpi_unlock_mem(uint32_t handle)
{
	int error;
	uint32_t res;

	struct __aligned(16) {
		struct vcprop_buffer_hdr	vb_hdr;
		struct vcprop_tag_lockmem	vbt_lm;
		struct vcprop_tag end;
	} vb_unlockmem =
	{
		.vb_hdr = {
			.vpb_len = sizeof(vb_unlockmem),
			.vpb_rcode = VCPROP_PROCESS_REQUEST,
		},
		.vbt_lm = {
			.tag = {
				.vpt_tag = VCPROPTAG_UNLOCKMEM,
				.vpt_len = VCPROPTAG_LEN(vb_unlockmem.vbt_lm),
				.vpt_rcode = VCPROPTAG_REQUEST,
			},
			.handle = handle,
		},
		.end = {
			.vpt_tag = VCPROPTAG_NULL,
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_unlockmem,
	    sizeof(vb_unlockmem), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %d %d %08x %08x\n", __func__,
	    vb_unlockmem.vbt_lm.handle, error, res,
	    vb_unlockmem.vbt_lm.tag.vpt_rcode);
#endif
	return (error == 0);
}

int
rpi_release_mem(uint32_t handle)
{
	int error;
	uint32_t res;

	struct __aligned(16) {
		struct vcprop_buffer_hdr	vb_hdr;
		struct vcprop_tag_lockmem	vbt_lm;
		struct vcprop_tag end;
	} vb_releasemem =
	{
		.vb_hdr = {
			.vpb_len = sizeof(vb_releasemem),
			.vpb_rcode = VCPROP_PROCESS_REQUEST,
		},
		.vbt_lm = {
			.tag = {
				.vpt_tag = VCPROPTAG_RELEASEMEM,
				.vpt_len = VCPROPTAG_LEN(vb_releasemem.vbt_lm),
				.vpt_rcode = VCPROPTAG_REQUEST,
			},
			.handle = handle,
		},
		.end = {
			.vpt_tag = VCPROPTAG_NULL,
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_releasemem,
	    sizeof(vb_releasemem), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %d %d %08x %08x\n", __func__,
	    vb_releasemem.vbt_lm.handle, error, res,
	    vb_releasemem.vbt_lm.tag.vpt_rcode);
#endif
	return (error == 0);
}

int
rpi_fb_movecursor(int x, int y, int on)
{
	int error;
	uint32_t res;

	struct __aligned(16) {
		struct vcprop_buffer_hdr	vb_hdr;
		struct vcprop_tag_cursorstate	vbt_cs;
		struct vcprop_tag end;
	} vb_cursorstate =
	{
		.vb_hdr = {
			.vpb_len = sizeof(vb_cursorstate),
			.vpb_rcode = VCPROP_PROCESS_REQUEST,
		},
		.vbt_cs = {
			.tag = {
				.vpt_tag = VCPROPTAG_SET_CURSOR_STATE,
				.vpt_len = VCPROPTAG_LEN(vb_cursorstate.vbt_cs),
				.vpt_rcode = VCPROPTAG_REQUEST,
			},
			.enable = (on != 0) ? 1 : 0,
			.x = x,
			.y = y,
			.flags = 1,
		},
		.end = {
			.vpt_tag = VCPROPTAG_NULL,
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_cursorstate,
	    sizeof(vb_cursorstate), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %08x %d %08x %08x\n", __func__,
	    vb_cursorstate.vbt_cs.enable, error, res,
	    vb_cursorstate.vbt_cs.tag.vpt_rcode);
#endif
	return (error == 0);
}

int
rpi_fb_initcursor(bus_addr_t pixels, int hx, int hy)
{
	int error;
	uint32_t res;
	

	struct __aligned(16) {
		struct vcprop_buffer_hdr	vb_hdr;
		struct vcprop_tag_cursorinfo	vbt_ci;
		struct vcprop_tag end;
	} vb_cursorinfo =
	{
		.vb_hdr = {
			.vpb_len = sizeof(vb_cursorinfo),
			.vpb_rcode = VCPROP_PROCESS_REQUEST,
		},
		.vbt_ci = {
			.tag = {
				.vpt_tag = VCPROPTAG_SET_CURSOR_INFO,
				.vpt_len = VCPROPTAG_LEN(vb_cursorinfo.vbt_ci),
				.vpt_rcode = VCPROPTAG_REQUEST,
			},
			.width = 64,
			.height = 64,
			.format = 0,
			.pixels = pixels,
			.hotspot_x = hx,
			.hotspot_y = hy,
		},
		.end = {
			.vpt_tag = VCPROPTAG_NULL,
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_cursorinfo,
	    sizeof(vb_cursorinfo), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %d %d %08x %08x\n", __func__,
	    vb_cursorinfo.vbt_ci.width, error, res,
	    vb_cursorinfo.vbt_ci.tag.vpt_rcode);
#endif
	return (error == 0);
}
