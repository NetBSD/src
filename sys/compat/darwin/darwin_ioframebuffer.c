/*	$NetBSD: darwin_ioframebuffer.c,v 1.3 2003/03/09 18:33:29 manu Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: darwin_ioframebuffer.c,v 1.3 2003/03/09 18:33:29 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/device.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_iokit.h>

#include <compat/darwin/darwin_ioframebuffer.h>
#include <compat/darwin/darwin_iokit.h>

struct mach_iokit_devclass darwin_ioframebuffer_devclass = {
	"<dict ID=\"0\"><key>IOProviderClass</key>"
	    "<string ID=\"1\">IOFramebuffer</string></dict>",
	"<dict ID=\"0\"><key>IOClass</key>"
	    "<string ID=\"1\">AppleBacklightDisplay</string>"
	    "<key>IOProbeScore</key>"
	    "<integer size=\"32\" ID=\"2\">0xbb8</integer>"
	    "<key>IOProviderClass</key>"
	    "<string ID=\"3\">IODisplayConnect</string>"
	    "<key>CFBundleIdentifier</key>"
	    "<string ID=\"4\">com.apple.iokit.IOGraphicsFamily</string>"
	    "<key>IOMatchCategory</key>"
	    "<string ID=\"5\">IODefaultMatchCategory</string>"
	    "<key>IODisplayConnectFlags</key>"
	    "<data ID=\"6\">AAAIxA==</data>"
	    "<key>AppleDisplayType</key>"
	    "<integer size=\"32\" ID=\"7\">0x2</integer>"
	    "<key>AppleSense</key>"
	    "<integer size=\"32\" ID=\"8\">0x400</integer>"
	    "<key>DisplayVendorID</key>"
	    "<integer size=\"32\" ID=\"9\">0x756e6b6e</integer>"
	    "<key>DisplayProductID</key>"
	    "<integer size=\"32\" ID=\"10\">0x20000</integer>"
	    "<key>IODisplayParameters</key>"
	    "</dict>",
	darwin_ioframebuffer_registry_entry_get_property,
	darwin_ioframebuffer_connect_method_scalari_scalaro,
	"IOFramebuffer",
};

struct mach_iokit_property darwin_ioframebuffer_properties[] = {
	{ "IOFBDependentID", 0 },
	{ "IOFBDependentIndex", 0},
	{ "graphic-options", 0x28},
	{ "IOFBConfig", 0x219f},
	{ "IOFBMemorySize", 0x2e}, 
	{ NULL, 0}
};

int
darwin_ioframebuffer_registry_entry_get_property(args)
	struct mach_trap_args *args;
{
	mach_io_registry_entry_get_property_request_t *req = args->smsg;
	mach_io_registry_entry_get_property_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	struct mach_port *mp;
	struct mach_right *mr;
	struct mach_iokit_property *mip;
	size_t len;

#ifdef DEBUG_DARWIN
	printf("darwin_ioframebuffer_registry_entry_get_property()\n");
#endif
	len = strlen(mip->mip_name);
	for (mip = darwin_ioframebuffer_properties; mip->mip_name; mip++) {
		if (memcmp(mip->mip_name, req->req_propery_name, len) == 0)
			break;
	}
	if (mip->mip_value == 0) 
		return mach_iokit_error(args, MACH_IOKIT_ENOENT);
		
	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);
	
	rep->rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_body.msgh_descriptor_count = 1;
	rep->rep_properties.name = (mach_port_t)mr->mr_name;
	rep->rep_properties.disposition = 0x11; /* XXX */
	rep->rep_properties_count = mip->mip_value;
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
darwin_ioframebuffer_connect_method_scalari_scalaro(args)
	struct mach_trap_args *args;
{
	mach_io_connect_method_scalari_scalaro_request_t *req = args->smsg;
	mach_io_connect_method_scalari_scalaro_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;

#ifdef DEBUG_DARWIN
	printf("darwin_ioframebuffer_connect_method_scalari_scalaro()\n");
#endif
	rep->rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_outcount = 1;
	rep->rep_out[0] = 1;
	rep->rep_out[rep->rep_outcount + 1] = 8; /* XXX Trailer */

	*msglen = sizeof(*rep) - ((4096 + rep->rep_outcount) * sizeof(int));
	return 0;
}
