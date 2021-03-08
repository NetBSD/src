/*	$NetBSD: vcprop_subr.c,v 1.10 2021/03/08 13:53:08 mlelstv Exp $	*/

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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vcprop_subr.c,v 1.10 2021/03/08 13:53:08 mlelstv Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/endian.h>

#include <uvm/uvm_extern.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835var.h>
#include <arm/broadcom/bcm2835_mbox.h>

#include <evbarm/rpi/vcio.h>
#include <evbarm/rpi/vcpm.h>
#include <evbarm/rpi/vcprop.h>

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
			.vpb_len = htole32(sizeof(vb_setblank)),
			.vpb_rcode = htole32(VCPROP_PROCESS_REQUEST),
		},
		.vbt_blank = {
			.tag = {
				.vpt_tag = htole32(VCPROPTAG_BLANK_SCREEN),
				.vpt_len = htole32(VCPROPTAG_LEN(
				    vb_setblank.vbt_blank)),
				.vpt_rcode = htole32(VCPROPTAG_REQUEST),
			},
			.state = htole32((b != 0) ?
			    VCPROP_BLANK_OFF : VCPROP_BLANK_ON),
		},
		.end = {
			.vpt_tag = htole32(VCPROPTAG_NULL),
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_setblank,
	    sizeof(vb_setblank), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %d %d %d %08x %08x\n", __func__, b,
	    le32toh(vb_setblank.vbt_blank.state), error, res,
	    le32toh(vb_setblank.vbt_blank.tag.vpt_rcode));
#endif
	if (error)
		return error;

	if (!vcprop_buffer_success_p(&vb_setblank.vb_hdr) ||
	    !vcprop_tag_success_p(&vb_setblank.vbt_blank.tag)) {
		return EIO;
	}

	return 0;
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
			.vpb_len = htole32(sizeof(vb_allocmem)),
			.vpb_rcode = htole32(VCPROP_PROCESS_REQUEST),
		},
		.vbt_am = {
			.tag = {
				.vpt_tag = htole32(VCPROPTAG_ALLOCMEM),
				.vpt_len =
				    htole32(VCPROPTAG_LEN(vb_allocmem.vbt_am)),
				.vpt_rcode = htole32(VCPROPTAG_REQUEST),
			},
			.size = htole32(size),
			.align = htole32(align),
			.flags = htole32(flags),
		},
		.end = {
			.vpt_tag = htole32(VCPROPTAG_NULL),
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_allocmem,
	    sizeof(vb_allocmem), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %d %d %08x %08x\n", __func__,
	    le32toh(vb_allocmem.vbt_am.size), error, res,
	    le32toh(vb_allocmem.vbt_am.tag.vpt_rcode));
#endif
	if (error)
		return error;

	if (!vcprop_buffer_success_p(&vb_allocmem.vb_hdr) ||
	    !vcprop_tag_success_p(&vb_allocmem.vbt_am.tag)) {
		return EIO;
	}

	/* Return the handle from the VC */
	return le32toh(vb_allocmem.vbt_am.size);
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
			.vpb_len = htole32(sizeof(vb_lockmem)),
			.vpb_rcode = htole32(VCPROP_PROCESS_REQUEST),
		},
		.vbt_lm = {
			.tag = {
				.vpt_tag = htole32(VCPROPTAG_LOCKMEM),
				.vpt_len =
				    htole32(VCPROPTAG_LEN(vb_lockmem.vbt_lm)),
				.vpt_rcode = htole32(VCPROPTAG_REQUEST),
			},
			.handle = htole32(handle),
		},
		.end = {
			.vpt_tag = htole32(VCPROPTAG_NULL),
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_lockmem,
	    sizeof(vb_lockmem), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %d %d %08x %08x\n", __func__,
	    le32toh(vb_lockmem.vbt_lm.handle), error, res,
	    le32toh(vb_lockmem.vbt_lm.tag.vpt_rcode));
#endif
	if (error)
		return 0;

	if (!vcprop_buffer_success_p(&vb_lockmem.vb_hdr) ||
	    !vcprop_tag_success_p(&vb_lockmem.vbt_lm.tag)) {
		return 0;
	}

	return le32toh(vb_lockmem.vbt_lm.handle);
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
			.vpb_len = htole32(sizeof(vb_unlockmem)),
			.vpb_rcode = htole32(VCPROP_PROCESS_REQUEST),
		},
		.vbt_lm = {
			.tag = {
				.vpt_tag = htole32(VCPROPTAG_UNLOCKMEM),
				.vpt_len =
				    htole32(VCPROPTAG_LEN(vb_unlockmem.vbt_lm)),
				.vpt_rcode = htole32(VCPROPTAG_REQUEST),
			},
			.handle = htole32(handle),
		},
		.end = {
			.vpt_tag = htole32(VCPROPTAG_NULL),
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_unlockmem,
	    sizeof(vb_unlockmem), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %d %d %08x %08x\n", __func__,
	    le32toh(vb_unlockmem.vbt_lm.handle), error, res,
	    le32toh(vb_unlockmem.vbt_lm.tag.vpt_rcode));
#endif
	if (error)
		return error;

	if (!vcprop_buffer_success_p(&vb_unlockmem.vb_hdr) ||
	    !vcprop_tag_success_p(&vb_unlockmem.vbt_lm.tag)) {
		return EIO;
	}

	return 0;
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
			.vpb_len = htole32(sizeof(vb_releasemem)),
			.vpb_rcode = htole32(VCPROP_PROCESS_REQUEST),
		},
		.vbt_lm = {
			.tag = {
				.vpt_tag = htole32(VCPROPTAG_RELEASEMEM),
				.vpt_len = htole32(VCPROPTAG_LEN(
				    vb_releasemem.vbt_lm)),
				.vpt_rcode = htole32(VCPROPTAG_REQUEST),
			},
			.handle = htole32(handle),
		},
		.end = {
			.vpt_tag = htole32(VCPROPTAG_NULL),
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_releasemem,
	    sizeof(vb_releasemem), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %d %d %08x %08x\n", __func__,
	    le32toh(vb_releasemem.vbt_lm.handle), error, res,
	    le32toh(vb_releasemem.vbt_lm.tag.vpt_rcode));
#endif
	if (error)
		return error;

	if (!vcprop_buffer_success_p(&vb_releasemem.vb_hdr) ||
	    !vcprop_tag_success_p(&vb_releasemem.vbt_lm.tag)) {
		return EIO;
	}

	return 0;
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
			.vpb_len = htole32(sizeof(vb_cursorstate)),
			.vpb_rcode = htole32(VCPROP_PROCESS_REQUEST),
		},
		.vbt_cs = {
			.tag = {
				.vpt_tag = htole32(VCPROPTAG_SET_CURSOR_STATE),
				.vpt_len = htole32(VCPROPTAG_LEN(
				    vb_cursorstate.vbt_cs)),
				.vpt_rcode = htole32(VCPROPTAG_REQUEST),
			},
			.enable = htole32((on != 0) ? 1 : 0),
			.x = htole32(x),
			.y = htole32(y),
			.flags = htole32(1),
		},
		.end = {
			.vpt_tag = htole32(VCPROPTAG_NULL),
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_cursorstate,
	    sizeof(vb_cursorstate), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %08x %d %08x %08x\n", __func__,
	    le32toh(vb_cursorstate.vbt_cs.enable), error, res,
	    le32toh(vb_cursorstate.vbt_cs.tag.vpt_rcode));
#endif
	if (error)
		return error;

	if (!vcprop_buffer_success_p(&vb_cursorstate.vb_hdr) ||
	    !vcprop_tag_success_p(&vb_cursorstate.vbt_cs.tag)) {
		return EIO;
	}

	return 0;
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
			.vpb_len = htole32(sizeof(vb_cursorinfo)),
			.vpb_rcode = htole32(VCPROP_PROCESS_REQUEST),
		},
		.vbt_ci = {
			.tag = {
				.vpt_tag = htole32(VCPROPTAG_SET_CURSOR_INFO),
				.vpt_len = htole32(VCPROPTAG_LEN(
				    vb_cursorinfo.vbt_ci)),
				.vpt_rcode = htole32(VCPROPTAG_REQUEST),
			},
			.width = htole32(64),
			.height = htole32(64),
			.format = htole32(0),
			.pixels = htole32(pixels),
			.hotspot_x = htole32(hx),
			.hotspot_y = htole32(hy),
		},
		.end = {
			.vpt_tag = htole32(VCPROPTAG_NULL),
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_cursorinfo,
	    sizeof(vb_cursorinfo), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %d %d %08x %08x\n", __func__,
	    le32toh(vb_cursorinfo.vbt_ci.width), error, res,
	    le32toh(vb_cursorinfo.vbt_ci.tag.vpt_rcode));
#endif
	if (error)
		return error;

	if (!vcprop_buffer_success_p(&vb_cursorinfo.vb_hdr) ||
	    !vcprop_tag_success_p(&vb_cursorinfo.vbt_ci.tag)) {
		return EIO;
	}

	return 0;
}

int
rpi_fb_get_pixelorder(uint32_t *orderp)
{
	int error;
	uint32_t res;


	struct __aligned(16) {
		struct vcprop_buffer_hdr	vb_hdr;
		struct vcprop_tag_fbpixelorder	vbt_po;
		struct vcprop_tag end;
	} vb_pixelorder =
	{
		.vb_hdr = {
			.vpb_len = htole32(sizeof(vb_pixelorder)),
			.vpb_rcode = htole32(VCPROP_PROCESS_REQUEST),
		},
		.vbt_po = {
			.tag = {
				.vpt_tag = htole32(VCPROPTAG_GET_FB_PIXEL_ORDER),
				.vpt_len = htole32(VCPROPTAG_LEN(
				    vb_pixelorder.vbt_po)),
				.vpt_rcode = htole32(VCPROPTAG_REQUEST),
			},
		},
		.end = {
			.vpt_tag = htole32(VCPROPTAG_NULL),
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_pixelorder,
	    sizeof(vb_pixelorder), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %d %d %08x %08x\n", __func__,
	    le32toh(vb_pixelorder.vbt_po.order), error, res,
	    le32toh(vb_pixelorder.vbt_po.tag.vpt_rcode));
#endif
	if (error)
		return error;

	if (!vcprop_buffer_success_p(&vb_pixelorder.vb_hdr) ||
	    !vcprop_tag_success_p(&vb_pixelorder.vbt_po.tag)) {
		return EIO;
	}

	*orderp = vb_pixelorder.vbt_po.order;

	return 0;
}

int
rpi_fb_set_pixelorder(uint32_t order)
{
	int error;
	uint32_t res;


	struct __aligned(16) {
		struct vcprop_buffer_hdr	vb_hdr;
		struct vcprop_tag_fbpixelorder	vbt_po;
		struct vcprop_tag end;
	} vb_pixelorder =
	{
		.vb_hdr = {
			.vpb_len = htole32(sizeof(vb_pixelorder)),
			.vpb_rcode = htole32(VCPROP_PROCESS_REQUEST),
		},
		.vbt_po = {
			.tag = {
				.vpt_tag = htole32(VCPROPTAG_SET_FB_PIXEL_ORDER),
				.vpt_len = htole32(VCPROPTAG_LEN(
				    vb_pixelorder.vbt_po)),
				.vpt_rcode = htole32(VCPROPTAG_REQUEST),
			},
			.order = order
		},
		.end = {
			.vpt_tag = htole32(VCPROPTAG_NULL),
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_pixelorder,
	    sizeof(vb_pixelorder), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %d %d %08x %08x\n", __func__,
	    le32toh(vb_pixelorder.vbt_po.order), error, res,
	    le32toh(vb_pixelorder.vbt_po.tag.vpt_rcode));
#endif
	if (error)
		return error;

	if (!vcprop_buffer_success_p(&vb_pixelorder.vb_hdr) ||
	    !vcprop_tag_success_p(&vb_pixelorder.vbt_po.tag)) {
		return EIO;
	}

	return 0;
}

int
rpi_set_domain(uint32_t domain, uint32_t state)
{
	int error;
	uint32_t tag, res;

	tag = VCPROPTAG_SET_DOMAIN_STATE;
	if (domain == VCPROP_DOMAIN_USB) {
		/* use old interface */
		tag = VCPROPTAG_SET_POWERSTATE;
		domain = VCPROP_POWER_USB;
	}

	/*
	 * might as well put it here since we need to re-init it every time
	 * and it's not like this is going to be called very often anyway
	 */
	struct __aligned(16) {
		struct vcprop_buffer_hdr	vb_hdr;
		struct vcprop_tag_powerstate	vbt_power;
		struct vcprop_tag end;
	} vb_setpower =
	{
		.vb_hdr = {
			.vpb_len = sizeof(vb_setpower),
			.vpb_rcode = VCPROP_PROCESS_REQUEST,
		},
		.vbt_power = {
			.tag = {
				.vpt_tag = tag,
				.vpt_len = VCPROPTAG_LEN(vb_setpower.vbt_power),
				.vpt_rcode = VCPROPTAG_REQUEST,
			},
			.id = domain,
			.state = state
		},
		.end = {
			.vpt_tag = VCPROPTAG_NULL,
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_setpower,
	    sizeof(vb_setpower), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %08x %08x %d %08x %08x %d %d %08x %08x\n", __func__,
	    tag, domain, state,
	    vb_setpower.vbt_power.tag.vpt_tag,
	    vb_setpower.vbt_power.id,
	    vb_setpower.vbt_power.state,
	    error, res,
	    vb_setpower.vbt_power.tag.vpt_rcode);
#endif
	if (error)
		return error;

	if (!vcprop_buffer_success_p(&vb_setpower.vb_hdr) ||
	    !vcprop_tag_success_p(&vb_setpower.vbt_power.tag)) {
		return EIO;
	}

	return 0;
}

int
rpi_get_domain(uint32_t domain, uint32_t *statep)
{
	int error;
	uint32_t tag, res;

	tag = VCPROPTAG_GET_DOMAIN_STATE;
	if (domain == VCPROP_DOMAIN_USB) {
		/* use old interface */
		tag = VCPROPTAG_GET_POWERSTATE;
		domain = VCPROP_POWER_USB;
	}

	/*
	 * might as well put it here since we need to re-init it every time
	 * and it's not like this is going to be called very often anyway
	 */
	struct __aligned(16) {
		struct vcprop_buffer_hdr	vb_hdr;
		struct vcprop_tag_powerstate	vbt_power;
		struct vcprop_tag end;
	} vb_setpower =
	{
		.vb_hdr = {
			.vpb_len = sizeof(vb_setpower),
			.vpb_rcode = VCPROP_PROCESS_REQUEST,
		},
		.vbt_power = {
			.tag = {
				.vpt_tag = tag,
				.vpt_len = VCPROPTAG_LEN(vb_setpower.vbt_power),
				.vpt_rcode = VCPROPTAG_REQUEST,
			},
			.id = domain,
			.state = ~0
		},
		.end = {
			.vpt_tag = VCPROPTAG_NULL,
		},
	};

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb_setpower,
	    sizeof(vb_setpower), &res);
#ifdef RPI_IOCTL_DEBUG
	printf("%s: %08x %08x %d %08x %08x %d %d %08x %08x\n", __func__,
	    tag, domain, *statep,
	    vb_setpower.vbt_power.tag.vpt_tag,
	    vb_setpower.vbt_power.id,
	    vb_setpower.vbt_power.state,
	    error, res,
	    vb_setpower.vbt_power.tag.vpt_rcode);
#endif
	if (error)
		return error;

	if (!vcprop_buffer_success_p(&vb_setpower.vb_hdr) ||
	    !vcprop_tag_success_p(&vb_setpower.vbt_power.tag)) {
		return EIO;
	}

	*statep = vb_setpower.vbt_power.state;

	return 0;
}

int
rpi_vchiq_init(uint32_t *channelbasep)
{
	int error;
	uint32_t tag, res;
	struct __aligned(16) {
		struct vcprop_buffer_hdr	vb_hdr;
		struct vcprop_tag_vchiqinit	vbt_vchiq;
		struct vcprop_tag end;
	} vb;

	tag = VCPROPTAG_VCHIQ_INIT;

	VCPROP_INIT_REQUEST(vb);
	VCPROP_INIT_TAG(vb.vbt_vchiq, tag);
	vb.vbt_vchiq.base = *channelbasep;

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb, sizeof(vb), &res);
	if (error)
		return error;

	if (!vcprop_buffer_success_p(&vb.vb_hdr) ||
	    !vcprop_tag_success_p(&vb.vbt_vchiq.tag)) {
		return EIO;
	}
	*channelbasep = vb.vbt_vchiq.base;

	return 0;
}

int
rpi_notify_xhci_reset(uint32_t address)
{
	int error;
	uint32_t tag, res;
	struct __aligned(16) {
		struct vcprop_buffer_hdr	vb_hdr;
		struct vcprop_tag_notifyxhcireset	vbt_nhr;
		struct vcprop_tag end;
	} vb;

	tag = VCPROPTAG_NOTIFY_XHCI_RESET;

	VCPROP_INIT_REQUEST(vb);
	VCPROP_INIT_TAG(vb.vbt_nhr, tag);
	vb.vbt_nhr.deviceaddress = address;

	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb, sizeof(vb), &res);
	if (error)
		return error;

	if (!vcprop_buffer_success_p(&vb.vb_hdr) ||
	    !vcprop_tag_success_p(&vb.vbt_nhr.tag)) {
		return EIO;
	}

	return 0;
}

