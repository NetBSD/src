/*      $NetBSD: genfb_xen.c,v 1.2.2.2 2023/10/18 16:53:03 martin Exp $      */

/*
 * Copyright (c) 2023 Manuel Bouyer.
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: genfb_xen.c,v 1.2.2.2 2023/10/18 16:53:03 martin Exp $");


#include <sys/device.h>
#include <xen/include/xen.h>
#include <xen/include/hypervisor.h>
#include <xen/include/public/xen.h>
#include <arch/x86/include/genfb_machdep.h>
#include <arch/x86/include/bootinfo.h>

static struct btinfo_framebuffer _xen_genfb_btinfo = {0};

const struct btinfo_framebuffer *
xen_genfb_getbtinfo(void)
{
	dom0_vga_console_info_t *d0_consi;
	int info_size;

	if (!xendomain_is_dom0())
		return NULL;

	if (_xen_genfb_btinfo.common.type == BTINFO_FRAMEBUFFER)
		return &_xen_genfb_btinfo;

#ifdef XENPVHVM
	struct xen_platform_op op = {
		.cmd = XENPF_get_dom0_console,
	};
	info_size = HYPERVISOR_platform_op(&op);
	if (info_size < sizeof(dom0_vga_console_info_t)) {
		printf("XENPF_get_dom0_console fail %d\n", info_size);
		return NULL;
	}
	d0_consi = &op.u.dom0_console;
#else
	d0_consi = (void *)((char *)&xen_start_info +
	    xen_start_info.console.dom0.info_off);
	info_size = xen_start_info.console.dom0.info_size;
#endif

	if (d0_consi->video_type != XEN_VGATYPE_VESA_LFB &&
	    d0_consi->video_type != XEN_VGATYPE_EFI_LFB)
		return NULL;

	_xen_genfb_btinfo.common.type = BTINFO_FRAMEBUFFER;
	_xen_genfb_btinfo.common.len = sizeof(struct btinfo_framebuffer);
	_xen_genfb_btinfo.physaddr = d0_consi->u.vesa_lfb.lfb_base;
	if (info_size >=
	    offsetof(dom0_vga_console_info_t, u.vesa_lfb.ext_lfb_base)) {
		_xen_genfb_btinfo.physaddr |=
		    (uint64_t)d0_consi->u.vesa_lfb.ext_lfb_base << 32;
	}
	_xen_genfb_btinfo.flags = 0;
	_xen_genfb_btinfo.width = d0_consi->u.vesa_lfb.width;
	_xen_genfb_btinfo.height = d0_consi->u.vesa_lfb.height;
	_xen_genfb_btinfo.stride = d0_consi->u.vesa_lfb.bytes_per_line;
	_xen_genfb_btinfo.depth = d0_consi->u.vesa_lfb.bits_per_pixel;
	_xen_genfb_btinfo.rnum = d0_consi->u.vesa_lfb.red_pos;
	_xen_genfb_btinfo.gnum = d0_consi->u.vesa_lfb.green_pos;
	_xen_genfb_btinfo.bnum = d0_consi->u.vesa_lfb.blue_pos;
	_xen_genfb_btinfo.rpos = d0_consi->u.vesa_lfb.red_size;
	_xen_genfb_btinfo.gpos = d0_consi->u.vesa_lfb.green_size;
	_xen_genfb_btinfo.bpos = d0_consi->u.vesa_lfb.blue_size;
	_xen_genfb_btinfo.vbemode = 0; /* XXX */

	return &_xen_genfb_btinfo;
}
