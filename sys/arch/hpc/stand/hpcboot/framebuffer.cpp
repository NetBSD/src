/* -*-C++-*-	$NetBSD: framebuffer.cpp,v 1.8 2001/06/20 17:36:00 uch Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <hpcmenu.h>
#include <machine/bootinfo.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <framebuffer.h>

//
// framebuffer configuration table can be found in machine_config.cpp
//

FrameBufferInfo::FrameBufferInfo(u_int32_t cpu, u_int32_t machine)
{
	struct framebuffer_info *tab = _table;
	platid_mask_t target, entry;
	framebuffer_info *alt = 0;

	// get current bpp.
	HDC hdc = GetDC(0);
	int bpp = GetDeviceCaps(hdc, BITSPIXEL);
	ReleaseDC(0, hdc);

	target.dw.dw0 = cpu;
	target.dw.dw1 = machine;
	// search apriori setting if any.
	for (; tab->cpu; tab++) {
		entry.dw.dw0 = tab->cpu;
		entry.dw.dw1 = tab->machine;
		if (platid_match(&target, &entry)) {
			if (tab->bpp == bpp) {
				_fb = tab;
				return;
			} else {
				alt = tab;
			}
		}
	}

	// use alternative framebuffer setting, if any.
	if (alt) {
		_fb = alt;
		return;
	}

	// no apriori setting. fill default.
	memset(&_default, 0, sizeof(struct framebuffer_info));

	_default.cpu = cpu;
	_default.machine = machine;
	hdc = GetDC(0);
	_default.bpp = bpp;
	_default.width = GetDeviceCaps(hdc, HORZRES);
	_default.height = GetDeviceCaps(hdc, VERTRES);
	ReleaseDC(0, hdc);
	_fb = &_default;
}

FrameBufferInfo::~FrameBufferInfo()
{
	/* NO-OP */
}

int
FrameBufferInfo::type()
{
	BOOL reverse = HPC_PREFERENCE.reverse_video;
	int type;
	
	switch(_fb->bpp) {
	default:
		// FALLTHROUGH
	case 2:
		type = reverse ? BIFB_D2_M2L_3 : BIFB_D2_M2L_0;
		break;
	case 4:
		type = reverse ? BIFB_D4_M2L_F : BIFB_D4_M2L_0;
		break;
	case 8:
		type = reverse ? BIFB_D8_FF : BIFB_D8_00;
		break;
	case 16:
		type = reverse ? BIFB_D16_FFFF : BIFB_D16_0000;
		break;
	}

	return type;
}
