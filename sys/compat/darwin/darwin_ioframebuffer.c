/*	$NetBSD: darwin_ioframebuffer.c,v 1.32.4.3 2008/01/21 09:40:47 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: darwin_ioframebuffer.c,v 1.32.4.3 2008/01/21 09:40:47 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/mman.h>
#include <sys/vnode.h>
#include <sys/resourcevar.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_object.h>
#include <uvm/uvm_device.h>
#include <uvm/uvm_map.h>
#include <uvm/uvm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>

#include <compat/sys/signal.h>

#include <compat/common/compat_util.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_exec.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_iokit.h>

#include <compat/darwin/darwin_exec.h>
#include <compat/darwin/darwin_iokit.h>
#include <compat/darwin/darwin_sysctl.h>
#include <compat/darwin/darwin_ioframebuffer.h>

#include "ioconf.h"

/* Redefined from sys/dev/wscons/wsdisplay.c */
extern const struct cdevsw wsdisplay_cdevsw;

#define WSDISPLAYMINOR(unit, screen)        (((unit) << 8) | (screen))

static int darwin_findscreen(dev_t *, int, int);

static struct uvm_object *darwin_ioframebuffer_shmem = NULL;
static void darwin_ioframebuffer_shmeminit(vaddr_t);

/* This is ugly, but we hope to see it going away quickly */

static char darwin_iofbconfig[] = "<dict ID=\"0\"><key>IOFBModes</key><array ID=\"1\"><dict ID=\"2\"><key>AID</key><integer size=\"32\" ID=\"3\">0xee</integer><key>DM</key><data ID=\"4\">AAACAAAAAYAAMgAAAAAAAgAQAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"5\">0x3e</integer></dict><dict ID=\"6\"><key>AID</key><integer size=\"32\" ID=\"7\">0x82</integer><key>DM</key><data ID=\"8\">AAACAAAAAYAAPAAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"9\">0x3</integer></dict><dict ID=\"10\"><key>AID</key><integer size=\"32\" ID=\"11\">0xe8</integer><key>DM</key><data ID=\"12\">AAACAAAAAYAAPAAAAAAAAgAQAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"13\">0x37</integer></dict><dict ID=\"14\"><key>DM</key><data ID=\"15\">AAACAAAAAYAARgAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"16\">0x5</integer></dict><dict ID=\"17\"><key>AID</key><reference IDREF=\"11\"/><key>DM</key><data ID=\"18\">AAACgAAAAeAAPAAAAAAAAgAQAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"19\">0x38</integer></dict><dict ID=\"20\"><key>AID</key><reference IDREF=\"3\"/><key>DM</key><data ID=\"21\">AAACgAAAAeAAMgAAAAAAAgAQAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"22\">0x3f</integer></dict><dict ID=\"23\"><key>AID</key><integer size=\"32\" ID=\"24\">0x96</integer><key>DM</key><data ID=\"25\">AAACgAAAAeAAPAAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"26\">0x6</integer></dict><dict ID=\"27\"><key>AID</key><integer size=\"32\" ID=\"28\">0x8c</integer><key>DM</key><data ID=\"29\">AAACgAAAAeAAQwAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"30\">0x7</integer></dict><dict ID=\"31\"><key>AID</key><integer size=\"32\" ID=\"32\">0x98</integer><key>DM</key><data ID=\"33\">AAACgAAAAeAASAAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"34\">0x8</integer></dict><dict ID=\"35\"><key>AID</key><integer size=\"32\" ID=\"36\">0x9a</integer><key>DM</key><data ID=\"37\">AAACgAAAAeAASwAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"38\">0x9</integer></dict><dict ID=\"39\"><key>AID</key><integer size=\"32\" ID=\"40\">0x9e</integer><key>DM</key><data ID=\"41\">AAACgAAAAeAAVQAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"42\">0xa</integer></dict><dict ID=\"43\"><key>AID</key><integer size=\"32\" ID=\"44\">0xa0</integer><key>DM</key><data ID=\"45\">AAACgAAAA2YASwAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"46\">0xe</integer></dict><dict ID=\"47\"><key>AID</key><reference IDREF=\"3\"/><key>DM</key><data ID=\"48\">AAAC0AAAAeAAMgAAAAAAAgAQAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"49\">0x4b</integer></dict><dict ID=\"50\"><key>AID</key><reference IDREF=\"11\"/><key>DM</key><data ID=\"51\">AAAC0AAAAeAAPAAAAAAAAgAQAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"52\">0x4d</integer></dict><dict ID=\"53\"><key>AID</key><reference IDREF=\"3\"/><key>DM</key><data ID=\"54\">AAAC0AAAAkAAMgAAAAAAAgAQAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"55\">0x4c</integer></dict><dict ID=\"56\"><key>AID</key><reference IDREF=\"11\"/><key>DM</key><data ID=\"57\">AAAC0AAAAkAAPAAAAAAAAgAQAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"58\">0x4e</integer></dict><dict ID=\"59\"><key>AID</key><reference IDREF=\"3\"/><key>DM</key><data ID=\"60\">AAADIAAAAlgAMgAAAAAAAgAQAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"61\">0x40</integer></dict><dict ID=\"62\"><key>AID</key><integer size=\"32\" ID=\"63\">0xb4</integer><key>DM</key><data ID=\"64\">AAADIAAAAlgAOAAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"65\">0xf</integer></dict><dict ID=\"66\"><key>AID</key><integer size=\"32\" ID=\"67\">0xb6</integer><key>DM</key><data ID=\"68\">AAADIAAAAlgAPAAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"69\">0x10</integer></dict><dict ID=\"70\"><key>AID</key><reference IDREF=\"11\"/><key>DM</key><data ID=\"71\">AAADIAAAAlgAPAAAAAAAAgAQAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"72\">0x39</integer></dict><dict ID=\"73\"><key>AID</key><integer size=\"32\" ID=\"74\">0x2a</integer><key>ID</key><integer size=\"32\" ID=\"75\">0x2d</integer><key>DF</key><integer size=\"32\" ID=\"76\">0x400</integer><key>DM</key><data ID=\"77\">AAADIAAAAlgAAAAAAAAAAgAABAAAAAAAAAAAAAAAAAAAAAAA</data></dict><dict ID=\"78\"><key>DF</key><integer size=\"32\" ID=\"79\">0x100</integer><key>DM</key><data ID=\"80\">AAADIAAAAlgAPAAAAAAAAgAAAQAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"81\">0x32</integer></dict><dict ID=\"82\"><key>AID</key><integer size=\"32\" ID=\"83\">0xb8</integer><key>DM</key><data ID=\"84\">AAADIAAAAlgASAAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"85\">0x11</integer></dict><dict ID=\"86\"><key>AID</key><integer size=\"32\" ID=\"87\">0xba</integer><key>DM</key><data ID=\"88\">AAADIAAAAlgASwAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"89\">0x12</integer></dict><dict ID=\"90\"><key>AID</key><reference IDREF=\"74\"/><key>ID</key><integer size=\"32\" ID=\"91\">0x46</integer><key>DF</key><reference IDREF=\"76\"/><key>DM</key><reference IDREF=\"77\"/></dict><dict ID=\"92\"><key>AID</key><integer size=\"32\" ID=\"93\">0xbc</integer><key>DM</key><data ID=\"94\">AAADIAAAAlgAVQAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"95\">0x13</integer></dict><dict ID=\"96\"><key>AID</key><reference IDREF=\"3\"/><key>DM</key><data ID=\"97\">AAADQAAAAnAAMgAAAAAAAgAQAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"98\">0x41</integer></dict><dict ID=\"99\"><key>AID</key><reference IDREF=\"11\"/><key>DM</key><data ID=\"100\">AAADQAAAAnAAPAAAAAAAAgAQAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"101\">0x3a</integer></dict><dict ID=\"102\"><key>AID</key><integer size=\"32\" ID=\"103\">0xaa</integer><key>DM</key><data ID=\"104\">AAADQAAAAnAASwAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"105\">0x17</integer></dict><dict ID=\"106\"><key>DM</key><data ID=\"107\">AAADVAAAAoAAPAAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"108\">0x18</integer></dict><dict ID=\"109\"><key>AID</key><integer size=\"32\" ID=\"110\">0xbe</integer><key>DM</key><data ID=\"111\">AAAEAAAAAwAAPAAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"112\">0x19</integer></dict><dict ID=\"113\"><key>AID</key><reference IDREF=\"74\"/><key>ID</key><integer size=\"32\" ID=\"114\">0x2e</integer><key>DF</key><integer size=\"32\" ID=\"115\">0x407</integer><key>DM</key><data ID=\"116\">AAAEAAAAAwAAAAAAAAAAAgAABAcAAAAAAAAAAAAAAAAAAAAA</data></dict><dict ID=\"117\"><key>AID</key><reference IDREF=\"110\"/><key>ID</key><integer size=\"32\" ID=\"118\">0x33</integer><key>DF</key><reference IDREF=\"79\"/><key>DM</key><data ID=\"119\">AAAEAAAAAwAAPAAAAAAAAgAAAQAAAAAAAAAAAAAAAAAAAAAA</data></dict><dict ID=\"120\"><key>AID</key><integer size=\"32\" ID=\"121\">0xc8</integer><key>DM</key><data ID=\"122\">AAAEAAAAAwAARgAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"123\">0x1a</integer></dict><dict ID=\"124\"><key>AID</key><integer size=\"32\" ID=\"125\">0xcc</integer><key>DM</key><data ID=\"126\">AAAEAAAAAwAASwAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"127\">0x1b</integer></dict><dict ID=\"128\"><key>AID</key><integer size=\"32\" ID=\"129\">0xd2</integer><key>DM</key><reference IDREF=\"126\"/><key>ID</key><integer size=\"32\" ID=\"130\">0x1c</integer></dict><dict ID=\"131\"><key>AID</key><integer size=\"32\" ID=\"132\">0xd0</integer><key>DM</key><data ID=\"133\">AAAEAAAAAwAAVQAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"134\">0x1d</integer></dict><dict ID=\"135\"><key>AID</key><integer size=\"32\" ID=\"136\">0xdc</integer><key>DM</key><data ID=\"137\">AAAEgAAAA2YASwAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"138\">0x21</integer></dict><dict ID=\"139\"><key>AID</key><integer size=\"32\" ID=\"140\">0xfa</integer><key>DM</key><data ID=\"141\">AAAFAAAAA8AASwAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"142\">0x22</integer></dict><dict ID=\"143\"><key>AID</key><integer size=\"32\" ID=\"144\">0x104</integer><key>DM</key><data ID=\"145\">AAAFAAAABAAAPAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"146\">0x23</integer></dict><dict ID=\"147\"><key>AID</key><integer size=\"32\" ID=\"148\">0x106</integer><key>DM</key><data ID=\"149\">AAAFAAAABAAASwAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAA</data><key>ID</key><integer size=\"32\" ID=\"150\">0x24</integer></dict></array><key>IODisplayConnectFlags</key><integer size=\"32\" ID=\"151\">0x0</integer></dict>";

static char darwin_ioframebuffer_properties[] = "<dict ID=\"0\"><key>IOClass</key><string ID=\"1\">AppleBacklightDisplay</string><key>IOProbeScore</key><integer size=\"32\" ID=\"2\">0xbb8</integer><key>IOProviderClass</key><string ID=\"3\">IODisplayConnect</string><key>CFBundleIdentifier</key><string ID=\"4\">com.apple.iokit.IOGraphicsFamily</string><key>IOMatchCategory</key><string ID=\"5\">IODefaultMatchCategory</string><key>IODisplayConnectFlags</key><data ID=\"6\">AAAIxA==</data><key>AppleDisplayType</key><integer size=\"32\" ID=\"7\">0x2</integer><key>AppleSense</key><integer size=\"32\" ID=\"8\">0x400</integer><key>DisplayVendorID</key><integer size=\"32\" ID=\"9\">0x756e6b6e</integer><key>DisplayProductID</key><integer size=\"32\" ID=\"10\">0x20000</integer><key>IODisplayParameters</key><dict ID=\"11\"><key>brightness</key><dict ID=\"12\"><key>max</key><integer size=\"32\" ID=\"13\">0x73</integer><key>min</key><integer size=\"32\" ID=\"14\">0x3a</integer><key>value</key><integer size=\"32\" ID=\"15\">0x7f</integer></dict><key>commit</key><dict ID=\"16\"><key>reg</key><integer size=\"32\" ID=\"17\">0x0</integer></dict></dict><key>IODisplayGUID</key><integer size=\"64\" ID=\"18\">0x610000000000000</integer><key>Power Management protected data</key><string ID=\"19\">{ theNumberOfPowerStates = 4, version 1, power state 0 = { capabilityFlags 00000000, outputPowerCharacter 00000000, inputPowerRequirement 00000000, staticPower 0, unbudgetedPower 0, powerToAttain 0, timeToAttain 0, settleUpTime 0, timeToLower 0, settleDownTime 0, powerDomainBudget 0 }, power state 1 = { capabilityFlags 00000000, outputPowerCharacter 00000000, inputPowerRequirement 00000000, staticPower 0, unbudgetedPower 0, powerToAttain 0, timeToAttain 0, settleUpTime 0, timeToLower 0, settleDownTime 0, powerDomainBudget 0 }, power state 2 = { capabilityFlags 00008000, outputPowerCharacter 00000000, inputPowerRequirement 00000002, staticPower 0, unbudgetedPower 0, powerToAttain 0, timeToAttain 0, settleUpTime 0, timeToLower 0, settleDownTime 0, powerDomainBudget 0 }, power state 3 = { capabilityFlags 0000c000, outputPowerCharacter 00000000, inputPowerRequirement 00000002, staticPower 0, unbudgetedPower 0, powerToAttain 0, timeToAttain 0, settleUpTime 0, timeToLower 0, settleDownTime 0, powerDomainBudget 0 }, aggressiveness = 0, myCurrentState = 0, parentsCurrentPowerFlags = 00000002, maxCapability = 3 }</string><key>Power Management private data</key><string ID=\"20\">{ this object = 0114c800, interested driver = 0114c800, driverDesire = 0, deviceDesire = 0, ourDesiredPowerState = 0, previousReqest = 0 }</string></dict>";

struct mach_iokit_property darwin_ioframebuffer_properties_array[] = {
	{ "IOFBDependentID", NULL },
	{ "IOFBDependentIndex", NULL },
	{ "graphic-options", "<integer size=\"32\" ID=\"0\">0x0</integer>"},
	{ "IOFBConfig", darwin_iofbconfig },
	{ "IOFBMemorySize",
	    "<integer size=\"32\" ID=\"0\">0x1000000</integer>"},
	{ "AAPL,boot-display", ""},
	{ NULL, 0}
};

struct mach_iokit_devclass darwin_ioframebuffer_devclass = {
	"<dict ID=\"0\"><key>IOProviderClass</key>"
	    "<string ID=\"1\">IOFramebuffer</string></dict>",
	{ &mach_ioroot_devclass, NULL },
	darwin_ioframebuffer_properties,
	darwin_ioframebuffer_properties_array,
	darwin_ioframebuffer_connect_method_scalari_scalaro,
	darwin_ioframebuffer_connect_method_scalari_structo,
	darwin_ioframebuffer_connect_method_structi_structo,
	darwin_ioframebuffer_connect_method_scalari_structi,
	darwin_ioframebuffer_connect_map_memory,
	"IOFramebuffer",
	NULL,
};

int
darwin_ioframebuffer_connect_method_scalari_scalaro(struct mach_trap_args *args)
{
	mach_io_connect_method_scalari_scalaro_request_t *req = args->smsg;
	mach_io_connect_method_scalari_scalaro_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	int maxoutcount;
	int error;

#ifdef DEBUG_DARWIN
	printf("darwin_ioframebuffer_connect_method_scalari_scalaro()\n");
#endif
	rep->rep_outcount = 0;
	maxoutcount = req->req_in[req->req_incount];

	switch (req->req_selector) {
	case DARWIN_IOFBCREATESHAREDCURSOR: {
		/* Create the shared memory containing cursor information */
		int shmemvers;
		int maxwidth;
		int maxheight;
		size_t memsize;
		vaddr_t kvaddr;

		shmemvers = req->req_in[0]; /* 0x2 */
		maxwidth = req->req_in[1];  /* 0x20 */
		maxheight = req->req_in[2]; /* 0x20 */
#ifdef DEBUG_DARWIN
		printf("DARWIN_IOFBCREATESHAREDCURSOR: shmemvers = %d, "
		    "maxwidth = %d, maxheight = %d\n", shmemvers,
		    maxwidth, maxheight);
#endif
		if (darwin_ioframebuffer_shmem == NULL) {
			memsize =
			    round_page(sizeof(*darwin_ioframebuffer_shmem));

			darwin_ioframebuffer_shmem = uao_create(memsize, 0);

			error = uvm_map(kernel_map, &kvaddr, memsize,
			    darwin_ioframebuffer_shmem, 0, PAGE_SIZE,
			    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW,
			    UVM_INH_SHARE, UVM_ADV_RANDOM, 0));
			if (error != 0) {
				uao_detach(darwin_ioframebuffer_shmem);
				darwin_ioframebuffer_shmem = NULL;
				return mach_msg_error(args, error);
			}

			if ((error = uvm_map_pageable(kernel_map, kvaddr,
			    kvaddr + memsize, FALSE, 0)) != 0) {
				uao_detach(darwin_ioframebuffer_shmem);
				darwin_ioframebuffer_shmem = NULL;
				return mach_msg_error(args, error);
			}

			darwin_ioframebuffer_shmeminit(kvaddr);
		}

		/* No output, the zone is mapped during another call */
		rep->rep_outcount = 0;
		break;
	}

	case DARWIN_IOFBSETSTARTUPDISPLAYMODE: {
		darwin_iodisplaymodeid mode;
		darwin_ioindex depth;

		mode = req->req_in[0];
		depth = req->req_in[1];
#ifdef DEBUG_DARWIN
		printf("DARWIN_IOFBSETSTARTUPDISPLAYMODE: mode = %d, "
		    "depth = %d\n", mode, depth);
#endif
		/* Nothing for now */
		break;
	}

	case DARWIN_IOFBSETDISPLAYMODE: {
		darwin_iodisplaymodeid mode;
		darwin_ioindex depth;

		mode = req->req_in[0];
		depth = req->req_in[1];
#ifdef DEBUG_DARWIN
		printf("DARWIN_IOFBSETDISPLAYMODE: mode = %d, "
		    "depth = %d\n", mode, depth);
#endif
		/* Nothing for now */
		break;
	}

	case DARWIN_IOFBGETCURRENTDISPLAYMODE: {
		/* Get current display mode and depth. No input args */

#ifdef DEBUG_DARWIN
		printf("DARWIN_IOFBGETCURRENTDISPLAYMODE\n");
#endif
		if (maxoutcount < 2)
			return mach_msg_error(args, EINVAL);

		rep->rep_outcount = 2;
		rep->rep_out[0] = 0x2e; /* mode XXX */
		rep->rep_out[1] = 0; /* depth  (0=>8b 1=>15b 2=>24b) */
		break;
	}

	case DARWIN_IOFBSETCURSORVISIBLE: {
		mach_boolean_t	visible;

		visible = req->req_in[0];
#ifdef DEBUG_DARWIN
		printf("DARWIN_IOFBSETCURSORVISIBLE: visible = %d\n", visible);
#endif
		/* Nothing for now */
		break;
	}

	case DARWIN_IOFBGETATTRIBUTE: {
		/* Get attribute value */
		char *name;

		name = (char *)&req->req_in[0];
#ifdef DEBUG_DARWIN
		printf("DARWIN_IOFBGETATTRIBUTE: name = %s\n", name);
#endif

		/* We only heard about the mrdf attribute. What is it? */
		if (memcmp(name, "mrdf", 4) == 0) {
			if (maxoutcount < 1)
				return mach_msg_error(args, EINVAL);

			rep->rep_outcount = 1;
			rep->rep_out[0] = 0; /* XXX */
		} else {
#ifdef DEBUG_DARWIN
			printf("Unknown attribute %c%c%c%c\n",
			    req->req_in[0], req->req_in[1],
			    req->req_in[2], req->req_in[3]);
#endif
			return mach_msg_error(args, EINVAL);
		}
		break;
	}

	case DARWIN_IOFBGETVRAMMAPOFFSET: {
		darwin_iopixelaperture aperture; /* 0 XXX Current aperture? */
		struct darwin_emuldata *ded = l->l_proc->p_emuldata;

		aperture = req->req_in[0];
#ifdef DEBUG_DARWIN
		printf("DARWIN_IOFBGETVRAMMAPOFFSET: aperture = %d\n",
		    aperture);
#endif
		if (maxoutcount < 1)
			return mach_msg_error(args, EINVAL);

		rep->rep_outcount = 1;
		rep->rep_out[0] = (int)ded->ded_vramoffset;
		break;
	}

	default:
#ifdef DEBUG_DARWIN
		printf("Unknown selector %d\n", req->req_selector);
#endif
		return mach_msg_error(args, EINVAL);
		break;
	}

	*msglen = sizeof(*rep) - ((16 - rep->rep_outcount) * sizeof(int));
	mach_set_header(rep, req, *msglen);
	mach_set_trailer(rep, *msglen);

	return 0;
}

int
darwin_ioframebuffer_connect_method_scalari_structo(struct mach_trap_args *args)
{
	mach_io_connect_method_scalari_structo_request_t *req = args->smsg;
	mach_io_connect_method_scalari_structo_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	int maxoutcount;

#ifdef DEBUG_DARWIN
	printf("darwin_ioframebuffer_connect_method_scalari_structo()\n");
#endif
	rep->rep_outcount = 0;
	maxoutcount = req->req_in[req->req_incount];

	switch(req->req_selector) {
	case DARWIN_IOFBGETPIXELINFORMATION: {
		/* Get bit per pixel, etc... */
		darwin_iodisplaymodeid displaymode;
		darwin_ioindex depth;
		darwin_iopixelaperture aperture;
		darwin_iopixelinformation *pixelinfo;

		displaymode = req->req_in[0]; /* 0x2e */
		depth = req->req_in[1]; /* 0 or 1 */
		aperture = req->req_in[2]; /* 0 */
#ifdef DEBUG_DARWIN
		printf("DARWIN_IOFBGETPIXELINFORMATION: displaymode = %d, "
		    "depth = %d, aperture = %d\n", displaymode, depth,
		    aperture);
#endif
		pixelinfo = (darwin_iopixelinformation *)&rep->rep_out[0];

		if (maxoutcount < sizeof(*pixelinfo))
			return mach_msg_error(args, EINVAL);

		/*
		 * darwin_iopixelinformation is shorter than the buffer
		 * usually supplied, but Darwin still returns the whole buffer
		 */
		rep->rep_outcount = maxoutcount;
		memset(pixelinfo, 0, maxoutcount * sizeof(int));

		switch (depth) {
		case 0: /* 8 bpp */
			pixelinfo->bytesperrow = 0x400;
			pixelinfo->bytesperplane = 0;
			pixelinfo->bitsperpixel = 0x8;
			pixelinfo->pixeltype = DARWIN_IOFB_CLUTPIXELS;
			pixelinfo->componentcount = 1;
			pixelinfo->bitspercomponent = 8;
			pixelinfo->componentmasks[0] = 0x000000ff;
			memcpy(&pixelinfo->pixelformat, "PPPPPPPP", 8);
			pixelinfo->flags = 0;
			pixelinfo->activewidth = 0;
			pixelinfo->activeheight = 0;
			break;

		case 1: /* 15 bpp */
			pixelinfo->bytesperrow = 0x800;
			pixelinfo->bytesperplane = 0;
			pixelinfo->bitsperpixel = 0x10;
			pixelinfo->pixeltype = DARWIN_IOFB_RGBDIRECTPIXELS;
			pixelinfo->componentcount = 3;
			pixelinfo->bitspercomponent = 5;
			pixelinfo->componentmasks[0] = 0x00007c00; /* Red */
			pixelinfo->componentmasks[1] = 0x000003e0; /* Green */
			pixelinfo->componentmasks[2] = 0x0000001f; /* Blue */
			memcpy(&pixelinfo->pixelformat, "-RRRRRGGGGGBBBBB", 16);
			pixelinfo->flags = 0;
			pixelinfo->activewidth = 0;
			pixelinfo->activeheight = 0;
			break;

		case 2: /* 24 bpp */
			pixelinfo->bytesperrow = 0x1000;
			pixelinfo->bytesperplane = 0;
			pixelinfo->bitsperpixel = 0x20;
			pixelinfo->pixeltype = DARWIN_IOFB_RGBDIRECTPIXELS;
			pixelinfo->componentcount = 3;
			pixelinfo->bitspercomponent = 8;
			pixelinfo->componentmasks[0] = 0x00ff0000; /* Red */
			pixelinfo->componentmasks[1] = 0x0000ff00; /* Green */
			pixelinfo->componentmasks[2] = 0x000000ff; /* Blue */
			memcpy(&pixelinfo->pixelformat,
			    "--------RRRRRRRRGGGGGGGGBBBBBBBB", 32);
			pixelinfo->flags = 0;
			pixelinfo->activewidth = 0;
			pixelinfo->activeheight = 0;
			break;

		default:
			printf("unknown depth %d\n", depth);
			break;
		}

		/* Probably useless */
		rep->rep_out[158] = 0x4;
		rep->rep_out[162] = 0x3;

		break;
	}

	default:
#ifdef DEBUG_DARWIN
		printf("Unknown selector %d\n", req->req_selector);
#endif
		return mach_msg_error(args, EINVAL);
		break;
	}

	*msglen = sizeof(*rep) - (4096 - rep->rep_outcount);
	mach_set_header(rep, req, *msglen);
	mach_set_trailer(rep, *msglen);

	return 0;
}

int
darwin_ioframebuffer_connect_method_structi_structo(struct mach_trap_args *args)
{
	mach_io_connect_method_structi_structo_request_t *req = args->smsg;
	mach_io_connect_method_structi_structo_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	int maxoutcount;

#ifdef DEBUG_DARWIN
	printf("darwin_ioframebuffer_connect_method_structi_structo()\n");
#endif

	rep->rep_outcount = 0;
	/* maxoutcount is word aligned */
	maxoutcount = req->req_in[(req->req_incount & ~0x3UL) + 4];

	switch(req->req_selector) {
	case DARWIN_IOFBSETBOUNDS: {
		darwin_iogbounds *bounds;

		bounds = (darwin_iogbounds *)&req->req_in[0];

#ifdef DEBUG_DARWIN
		printf("DARWIN_IOFBSETBOUNDS: bounds (%d, %d) - (%d, %d)\n",
		    bounds->minx, bounds->miny, bounds->maxx, bounds->maxy);
#endif
		/* Nothing yet */
		break;
	}

	default:
#ifdef DEBUG_DARWIN
		printf("Unknown selector %d\n", req->req_selector);
#endif
		return mach_msg_error(args, EINVAL);
		break;
	}

	*msglen = sizeof(*rep) - (4096 - rep->rep_outcount);
	mach_set_header(rep, req, *msglen);
	mach_set_trailer(rep, *msglen);

	return 0;
}

int
darwin_ioframebuffer_connect_map_memory(struct mach_trap_args *args)
{
	mach_io_connect_map_memory_request_t *req = args->smsg;
	mach_io_connect_map_memory_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	struct proc *p = args->l->l_proc;
	int error = 0;
	size_t memsize;
	size_t len;
	vaddr_t pvaddr;

#ifdef DEBUG_DARWIN
	printf("darwin_ioframebuffer_connect_map_memory()\n");
#endif
	switch (req->req_memtype) {
	case DARWIN_IOFRAMEBUFFER_CURSOR_MEMORY:
		if (darwin_ioframebuffer_shmem == NULL)
			return mach_msg_error(args, error);

		len = sizeof(struct darwin_ioframebuffer_shmem);
		memsize = round_page(len);

		uao_reference(darwin_ioframebuffer_shmem);
		pvaddr = VM_DEFAULT_ADDRESS(p->p_vmspace->vm_daddr, memsize);

		if ((error = uvm_map(&p->p_vmspace->vm_map, &pvaddr,
		    memsize, darwin_ioframebuffer_shmem, 0, PAGE_SIZE,
		    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW,
		    UVM_INH_SHARE, UVM_ADV_RANDOM, 0))) != 0)
			return mach_msg_error(args, error);

#ifdef DEBUG_DARWIN
		printf("pvaddr = 0x%08lx\n", (long)pvaddr);
#endif
		break;

	case DARWIN_IOFRAMEBUFFER_VRAM_MEMORY:
	case DARWIN_IOFRAMEBUFFER_SYSTEM_APERTURE: {
		dev_t device;
		const struct cdevsw *wsdisplay;
		int unit;
		int mode, screen;
		struct wsdisplay_fbinfo fbi;
		struct darwin_emuldata *ded;
		struct vnode *vp;

		/*
		 * Use unit given by sysctl emul.darwin.ioframebuffer.unit
		 * and emul.darwin.ioframebuffer.screen
		 */
		unit = darwin_ioframebuffer_unit;
		screen = darwin_ioframebuffer_screen;
		if ((error = darwin_findscreen(&device, unit, screen)) != 0)
			return mach_msg_error(args, error);

		if ((wsdisplay = cdevsw_lookup(device)) == NULL)
			return mach_msg_error(args, ENXIO);

		/* Find the framebuffer's size */
		if ((error = (wsdisplay->d_ioctl)(device,
		    WSDISPLAYIO_GINFO, (void *)&fbi, 0, l)) != 0) {
#ifdef DEBUG_DARWIN
			printf("*** Cannot get screen params ***\n");
#endif
			return mach_msg_error(args, error);
		}
#ifdef DEBUG_DARWIN
		printf("framebuffer: %d x %d x %d\n",
		    fbi.width, fbi.height, fbi.depth);
#endif
		len = round_page(fbi.height * fbi.width * fbi.depth / 8);

		/*
		 * The framebuffer cannot be mapped if the console is
		 * not in graphic mode. We will do the switch, but it
		 * screws the console. Therefore we attempt to restore
		 * its original state on process exit. ded->ded_wsdev
		 * is used to remember the console device. If it is not
		 * NODEV on process exit, we use it to restore text mode.
		 */
		ded = (struct darwin_emuldata *)p->p_emuldata;
		if ((error = (wsdisplay->d_ioctl)(device,
		    WSDISPLAYIO_GMODE, (void *)&mode, 0, l)) != 0) {
#ifdef DEBUG_DARWIN
			printf("*** Cannot get console state ***\n");
#endif
			return mach_msg_error(args, ENODEV);
		}
		if (mode == WSDISPLAYIO_MODE_EMUL)
			ded->ded_wsdev = device;

		/* Switch to graphic mode */
		mode = WSDISPLAYIO_MODE_MAPPED;
		if ((error = (wsdisplay->d_ioctl)(device,
		    WSDISPLAYIO_SMODE, (void *)&mode, 0, l)) != 0) {
#ifdef DEBUG_DARWIN
			printf("*** Cannot switch to graphic mode ***\n");
#endif
			return mach_msg_error(args, ENODEV);
		}

		if ((error = cdevvp(device, &vp)) != 0) {
#ifdef DEBUG_DARWIN
			printf("*** cdevvp failed ***\n");
#endif
			return mach_msg_error(args, error);
		}

		pvaddr = 0;
		if ((error = uvm_mmap(&p->p_vmspace->vm_map, &pvaddr,
		    len, UVM_PROT_RW, UVM_PROT_RW, MAP_SHARED, vp, 0,
		    p->p_rlimit[RLIMIT_MEMLOCK].rlim_cur)) != 0) {
#ifdef DEBUG_DARWIN
			printf("*** uvm_mmap failed ***\n");
#endif
			return mach_msg_error(args, error);
		}

#ifdef DEBUG_DARWIN
		printf("mapped framebuffer at %p\n", (void *)pvaddr);
#endif
		break;
	}

	default:
#ifdef DEBUG_DARWIN
		printf("unimplemented memtype %d\n", req->req_memtype);
#endif
		return mach_msg_error(args, EINVAL);
		break;
	}

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;
	rep->rep_addr = pvaddr;
	rep->rep_len = len;

	mach_set_trailer(rep, *msglen);

	/* Track VRAM offset for connect_method IOFBGETVRAMMAPOFFSET */
	if (req->req_memtype == DARWIN_IOFRAMEBUFFER_VRAM_MEMORY) {
		struct darwin_emuldata *ded;

		ded = p->p_emuldata;
		/*
		 * This seems to be the offset of the framebuffer
		 * within the VRAM. For now 0, as we are only
		 * able to map the framebuffer.
		 */
		ded->ded_vramoffset = (void *)(pvaddr - pvaddr);
	}

	return 0;
}

void
darwin_ioframebuffer_shmeminit(vaddr_t kvaddr)
{
	struct darwin_ioframebuffer_shmem *shmem;

	shmem = (struct darwin_ioframebuffer_shmem *)kvaddr;

	return;
}

int
darwin_ioframebuffer_connect_method_scalari_structi(struct mach_trap_args *args)
{
	mach_io_connect_method_scalari_structi_request_t *req = args->smsg;
	mach_io_connect_method_scalari_structi_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	int scalar_len;
	int struct_len;
	char *struct_data;

#ifdef DEBUG_DARWIN
	printf("darwin_ioframebuffer_connect_method_scalari_structi()\n");
#endif
	scalar_len = req->req_incount;
	struct_len = req->req_in[scalar_len];
	struct_data = (char *)&req->req_in[scalar_len + 1];

	switch (req->req_selector) {
	case DARWIN_IOFBSETCOLORCONVERTTABLE: {
		int select;
		int *data;
		size_t tablelen;

		select = req->req_in[0];
		tablelen = struct_len / sizeof(*data);
#ifdef DEBUG_DARWIN
		printf("DARWIN_IOFBSETCOLORCONVERTTABLE: select = %d, "
		    "tablelen = %d\n", select, tablelen);
#endif
		if (tablelen == 0)
			break;

		data = (int *)struct_data;
		break;
	}

	case DARWIN_IOFBSETGAMMATABLE: {
		int entries;
		int count;
		int width;
		int *data;

		entries = req->req_in[0];
		count = req->req_in[1];
		width = req->req_in[2];

#ifdef DEBUG_DARWIN
		printf("DARWIN_IOFBSETGAMMATABLE: entries = %d, "
		    "count = %d, width = %d\n", entries, count, width);
#endif

		data = &req->req_in[3];
		break;
	}

#if 0	/* comment out stackgap using code - needs to be done another way */
	case DARWIN_IOFBSETCLUTWITHENTRIES: {
		struct lwp *l = args->l;
		struct proc *p = args->l->l_proc;
		int index;
		int option;
		struct darwin_iocolorentry *clut;
		size_t clutlen;
		size_t tablen;
		size_t kcolorsz;
		void *sg = stackgap_init(p, 0);
		int error;
		struct wsdisplay_cmap cmap;
		u_char *red;
		u_char *green;
		u_char *blue;
		u_char kred[256];
		u_char kgreen[256];
		u_char kblue[256];
		int unit, screen;
		dev_t dev;
		const struct cdevsw *wsdisplay;
		int i;

		index = req->req_in[0];
		option = req->req_in[1];
		clutlen = struct_len / sizeof(*clut);
		clut = (struct darwin_iocolorentry *)struct_data;

		if ((clutlen == 0) || (index >= 256))
			break;

#ifdef DEBUG_DARWIN
		printf("DARWIN_IOFBSETCLUTWITHENTRIES: index = %d, "
		    "option = %d, clutlen = %d\n", index, option, clutlen);
#endif

		/*
		 * Find wsdisplay. Use the screen given by sysctl
		 * emul.darwin.ioframebuffer.screen
		 */
		unit = darwin_ioframebuffer_unit;
		screen = darwin_ioframebuffer_screen;
		if ((error = darwin_findscreen(&dev, unit, screen)) != 0)
			return mach_msg_error(args, error);

		if ((wsdisplay = cdevsw_lookup(dev)) == NULL)
			return mach_msg_error(args, ENXIO);

		/*
		 * We only support 256 entries
		 */
		if (index + clutlen > 256)
			clutlen = 256 - index;

		/*
		 * Big tables will not fit in the stackgap.
		 * We have to split the data.
		 */
		if (clutlen <= 128)
			tablen = clutlen;
		else
			tablen = 128;

		kcolorsz = sizeof(u_char) * tablen;
		red = stackgap_alloc(p, &sg, kcolorsz);
		green = stackgap_alloc(p, &sg, kcolorsz);
		blue = stackgap_alloc(p, &sg, kcolorsz);

		do {
			for (i = 0; i < tablen; i++) {
				kred[i] = (u_char)(clut->red >> 8);
				kgreen[i] = (u_char)(clut->green >> 8);
				kblue[i] = (u_char)(clut->blue >> 8);
				clut++;
			}

			cmap.index = index;
			cmap.count = tablen;
			cmap.red = red;
			cmap.green = green;
			cmap.blue = blue;

			if (((error = copyout(kred, red, kcolorsz)) != 0) ||
			    ((error = copyout(kgreen, green, kcolorsz)) != 0) ||
			    ((error = copyout(kblue, blue, kcolorsz)) != 0))
				return mach_msg_error(args, error);

			if ((error = (wsdisplay->d_ioctl)(dev,
			    WSDISPLAYIO_PUTCMAP, (void *)&cmap, 0, l)) != 0)
				return mach_msg_error(args, error);

			index += tablen;
			clutlen -= tablen;
			if (clutlen <= 128)
				tablen = clutlen;
		} while (clutlen > 0);

		break;
	}
#endif

	default:
#ifdef DEBUG_DARWIN
		printf("Unknown selector %d\n", req->req_selector);
#endif
		return mach_msg_error(args, EINVAL);
		break;
	}

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;

	mach_set_trailer(rep, *msglen);

	return 0;
}


/* Find a wsdisplay from unit and screen */
static int
darwin_findscreen(dev, unit, screen)
	dev_t *dev;
	int unit, screen;
{
	struct device *dv;
	struct wsdisplay_softc *sc;
	int major, minor;

	/* Find a wsdisplay */
	TAILQ_FOREACH(dv, &alldevs, dv_list)
		if (device_is_a(dv, "wsdisplay") &&
		    device_unit(dv) == unit)
			break;
	if (dv == NULL)
		return ENODEV;

	sc = (struct wsdisplay_softc *)dv;

	/* Derive the device number */
	major = cdevsw_lookup_major(&wsdisplay_cdevsw);
	minor = WSDISPLAYMINOR(device_unit(dv), screen);
	*dev = makedev(major, minor);

#ifdef DEBUG_DARWIN
	printf("ioframebuffer uses major = %d, minor = %d\n", major, minor);
#endif
	return 0;
}
