/*	$NetBSD: darwin_iohidsystem.c,v 1.34 2006/10/12 01:30:47 christos Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: darwin_iohidsystem.c,v 1.34 2006/10/12 01:30:47 christos Exp $");

#include "ioconf.h"
#include "wsmux.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kthread.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wseventvar.h>
#include <dev/wscons/wsmuxvar.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h>
#include <uvm/uvm.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_iokit.h>

#include <compat/darwin/darwin_exec.h>
#include <compat/darwin/darwin_iokit.h>
#include <compat/darwin/darwin_sysctl.h>
#include <compat/darwin/darwin_iohidsystem.h>

/* Redefined from sys/dev/wscons/wsmux.c */
extern const struct cdevsw wsmux_cdevsw;

static struct uvm_object *darwin_iohidsystem_shmem = NULL;
static void darwin_iohidsystem_shmeminit(vaddr_t);
static void darwin_iohidsystem_thread(void *);
static int darwin_findwsmux(dev_t *, int);
static void darwin_wscons_to_iohidsystem
    (struct wscons_event *, darwin_iohidsystem_event *);
static void mach_notify_iohidsystem(struct lwp *, struct mach_right *);

struct darwin_iohidsystem_thread_args {
	vaddr_t dita_shmem;
	struct lwp *dita_l;
	int dita_done;
	int *dita_hidsystem_finished;
};

static char darwin_ioresources_properties[] = "<dict ID=\"0\"><key>IOKit</key><string ID=\"1\">IOService</string><key>AccessMPC106PerformanceRegister</key><string ID=\"2\">AppleGracklePCI is not serializable</string><key>IONVRAM</key><reference IDREF=\"1\"/><key>IOiic0</key><string ID=\"3\">AppleCuda is not serializable</string><key>IORTC</key><reference IDREF=\"3\"/><key>IOBSD</key><reference IDREF=\"1\"/></dict>";

static char darwin_iokbd_properties[] = "<dict ID=\"0\"><key>ADB Match</key><string ID=\"1\">2</string><key>ADBVirtualKeys</key><string ID=\"2\">0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,0x3B,0x37,0x38,0x39,0x3A,0x7B,0x7C,0x7D,0x7E,0x3F,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x3C,0x3D,0x3E,0x36,0x7F</string><key>CFBundleIdentifier</key><string ID=\"3\">com.apple.driver.AppleADBKeyboard</string><key>IOClass</key><string ID=\"4\">AppleADBKeyboard</string><key>IOProbeScore</key><integer size=\"32\" ID=\"5\">0x3e8</integer><key>IOProviderClass</key><string ID=\"6\">IOADBDevice</string><key>PowerBook fn Foward Delete</key><integer size=\"64\" ID=\"7\">0x1</integer><key>KeyboardReserved</key><data ID=\"8\">Ag4wAAAAAAA=</data><key>IOMatchCategory</key><string ID=\"9\">IODefaultMatchCategory</string><key>HIDKeyMapping</key><data ID=\"11\">AAAIAAE5AQE4AgE7AwE6BAE3BRVSQUxTVFVFWFdWW1xDS1F7fX58TlkGAXIHAT9/DQBhAEEAAQABAMoAxwABAAENAHMAUwATABMA+wCnABMAEw0AZABEAAQABAFEAbYABAAEDQBmAEYABgAGAKYBrAAGAAYNAGgASAAIAAgA4wDrAAAYAA0AZwBHAAcABwDxAOEABwAHDQB6AFoAGgAaAM8BVwAaABoNAHgAWAAYABgBtAHOABgAGA0AYwBDAAMAAwHjAdMAAwADDQB2AFYAFgAWAdYB4AAWABYCADwAPg0AYgBCAAIAAgHlAfIAAgACDQBxAFEAEQARAPoA6gARABENAHcAVwAXABcByAHHABcAFw0AZQBFAAUABQDCAMUABQAFDQByAFIAEgASAeIB0gASABINAHkAWQAZABkApQHbABkAGQ0AdABUABQAFAHkAdQAFAAUCgAxACEBrQChDgAyAEAAMgAAALIAswAAAAAKADMAIwCjAboKADQAJACiAKgOADYAXgA2AB4AtgDDAB4AHgoANQAlAaUAvQoAPQArAbkBsQoAOQAoAKwAqwoANwAmAbABqw4ALQBfAB8AHwCxANAAHwAfCgA4ACoAtwC0CgAwACkArQC7DgBdAH0AHQAdACcAugAdAB0NAG8ATwAPAA8A+QDpAA8ADw0AdQBVABUAFQDIAM0AFQAVDgBbAHsAGwAbAGAAqgAbABsNAGkASQAJAAkAwQD1AAkACQ0AcABQABAAEAFwAVAAEAAQEAANAAMNAGwATAAMAAwA+ADoAAwADA0AagBKAAoACgDGAK4ACgAKCgAnACIAqQGuDQBrAEsACwALAM4ArwALAAsKADsAOgGyAaIOAFwAfAAcABwA4wDrABwAHAoALAA8AMsBowoALwA/AbgAvw0AbgBOAA4ADgDEAa8ADgAODQBtAE0ADQANAW0B2AANAA0KAC4APgC8AbMCAAkAGQwAIAAAAIAAAAoAYAB+AGABuwIAfwAI/wIAGwB+///////////////8AAC7/AAAq/wAAK/8AABv///8OAC8AXAAvABwALwBcAAAKAAAADf8AAC3//w4APQB8AD0AHAA9AHwAABhGAAAwAAAxAAAyAAAzAAA0AAA1AAA2AAA3/wAAOAAAOf///wD+JAD+JQD+JgD+IgD+JwD+KP8A/ir/AP4y/wD+M/8A/in/AP4r/wD+NP8A/i4A/jAA/i0A/iMA/i8A/iEA/jEA/iAAAawAAa4AAa8AAa0PAv8EADEC/wQAMgL/BAAzAv8EADQC/wQANQL/BAA2Av8EADcC/wQAOAL/BAA5Av8EADAC/wQALQL/BAA9Av8EAHAC/wQAXQL/BABbBgVyBn8HSgg+CT0KRw==</data><key>HIDKind</key><integer size=\"32\" ID=\"12\">0x1</integer><key>HIDInterfaceID</key><integer size=\"32\" ID=\"13\">0x2</integer><key>HIDSubinterfaceID</key><integer size=\"32\" ID=\"14\">0x2</integer></dict>";

static char darwin_iomouse_properties[] = "<dict ID=\"0\"><key>dpi</key><integer size=\"64\" ID=\"1\">0xc8</integer><key>IOClass</key><string ID=\"2\">AppleADBMouseType2</string><key>IOProbeScore</key><integer size=\"32\" ID=\"3\">0x2710</integer><key>IOProviderClass</key><string ID=\"4\">IOADBDevice</string><key>CFBundleIdentifier</key><string ID=\"5\">com.apple.driver.AppleADBMouse</string><key>ADB Match</key><string ID=\"6\">3</string><key>IOMatchCategory</key><string ID=\"7\">IODefaultMatchCategory</string><key>HIDPointerAccelerationType</key><string ID=\"8\">HIDMouseAcceleration</string><key>HIDPointerAccelerationSettings</key><array ID=\"9\"><data ID=\"10\">AAAAAA==</data><data ID=\"11\">AAAgAA==</data><data ID=\"12\">AABQAA==</data><data ID=\"13\">AACAAA==</data><data ID=\"14\">AACwAA==</data><data ID=\"15\">AADgAA==</data><data ID=\"16\">AAEAAA==</data></array><key>HIDPointerResolution</key><data ID=\"17\">AMgAAA==</data><key>HIDPointerConvertAbsolute</key><data ID=\"18\">AAAAAA==</data><key>HIDPointerContactToMove</key><data ID=\"19\">AAAAAA==</data><key>HIDKind</key><integer size=\"32\" ID=\"20\">0x2</integer><key>HIDInterfaceID</key><integer size=\"32\" ID=\"21\">0x2</integer><key>HIDSubinterfaceID</key><integer size=\"32\" ID=\"22\">0x2</integer></dict>";

static char darwin_iohidsystem_properties[] = "<dict ID=\"0\"><key>CFBundleIdentifier</key><string ID=\"1\">com.apple.iokit.IOHIDSystem</string><key>IOClass</key><string ID=\"2\">IOHIDSystem</string><key>IOMatchCategory</key><string ID=\"3\">IOHID</string><key>IOProviderClass</key><string ID=\"4\">IOResources</string><key>IOResourceMatch</key><string ID=\"5\">IOKit</string><key>IOProbeScore</key><integer size=\"32\" ID=\"6\">0x0</integer><key>HIDParameters</key><dict ID=\"7\"><key>HIDClickTime</key><data ID=\"8\">AAAAAB3NZQA=</data><key>HIDAutoDimThreshold</key><data ID=\"9\">AAAARdlkuAA=</data><key>HIDAutoDimBrightness</key><data ID=\"10\">AAAAAA==</data><key>HIDClickSpace</key><data ID=\"11\">AAAAAA==</data><key>HIDKeyRepeat</key><data ID=\"12\">AAAAAAHJw4A=</data><key>HIDInitialKeyRepeat</key><data ID=\"13\">AAAAABZaC8A=</data><key>HIDPointerAcceleration</key><data ID=\"14\">AACAAA==</data><key>HIDScrollAcceleration</key><data ID=\"15\">AABQAA==</data><key>HIDPointerButtonMode</key><data ID=\"16\">AAAAAg==</data><key>HIDF12EjectDelay</key><data ID=\"17\">AAAA+g==</data><key>HIDSlowKeysDelay</key><data ID=\"18\">AAAAAA==</data><key>HIDStickyKeysDisabled</key><data ID=\"19\">AAAAAA==</data><key>HIDStickyKeysOn</key><data ID=\"20\">AAAAAA==</data><key>HIDStickyKeysShiftToggles</key><data ID=\"21\">AAAAAA==</data><key>HIDMouseAcceleration</key><data ID=\"22\">AAGzMw==</data><key>Clicking</key><data ID=\"23\">AA==</data><key>DragLock</key><reference IDREF=\"23\"/><key>Dragging</key><reference IDREF=\"23\"/><key>JitterNoMove</key><integer size=\"32\" ID=\"24\">0x1</integer><key>JitterNoClick</key><integer size=\"32\" ID=\"25\">0x1</integer><key>PalmNoAction When Typing</key><integer size=\"32\" ID=\"26\">0x1</integer><key>PalmNoAction Permanent</key><integer size=\"32\" ID=\"27\">0x1</integer><key>TwofingerNoAction</key><integer size=\"32\" ID=\"28\">0x1</integer><key>OutsidezoneNoAction When Typing</key><integer size=\"32\" ID=\"29\">0x1</integer><key>Use Panther Settings for W</key><integer size=\"32\" ID=\"30\">0x0</integer><key>Trackpad Jitter Milliseconds</key><integer size=\"32\" ID=\"31\">0xc0</integer><key>USBMouseStopsTrackpad</key><integer size=\"32\" ID=\"32\">0x0</integer><key>HIDWaitCursorFrameInterval</key><data ID=\"33\">AfygVw==</data></dict><key>HIDAutoDimTime</key><data ID=\"34\">AAAAAAAAAAA=</data><key>HIDIdleTime</key><data ID=\"35\">AAAjRKwYjsI=</data><key>HIDAutoDimState</key><data ID=\"36\">AAAAAQ==</data><key>HIDBrightness</key><data ID=\"37\">AAAAQA==</data></dict>";

struct mach_iokit_devclass darwin_iokbd_devclass = {
	"(unknown)",
	{ &mach_ioroot_devclass, NULL },
	darwin_iokbd_properties,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"AppleADBKeyboard",
	NULL,
};

struct mach_iokit_devclass darwin_iomouse_devclass = {
	"(unknown)",
	{ &mach_ioroot_devclass, NULL },
	darwin_iomouse_properties,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"AppleADBMouse",
	NULL,
};

struct mach_iokit_devclass darwin_ioresources_devclass = {
	"(unknown)",
	{ &mach_ioroot_devclass, NULL },
	darwin_ioresources_properties,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"IOResources",
	NULL,
};

struct mach_iokit_devclass darwin_iohidsystem_devclass = {
	"<dict ID=\"0\"><key>IOProviderClass</key>"
	    "<string ID=\"1\">IOHIDSystem</string></dict>",
	{ &darwin_ioresources_devclass, &darwin_iokbd_devclass,
	    &darwin_iomouse_devclass, NULL },
	darwin_iohidsystem_properties,
	NULL,
	darwin_iohidsystem_connect_method_scalari_scalaro,
	NULL,
	darwin_iohidsystem_connect_method_structi_structo,
	NULL,
	darwin_iohidsystem_connect_map_memory,
	"IOHIDSystem",
	NULL,
};

int
darwin_iohidsystem_connect_method_scalari_scalaro(args)
	struct mach_trap_args *args;
{
	mach_io_connect_method_scalari_scalaro_request_t *req = args->smsg;
	mach_io_connect_method_scalari_scalaro_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	int maxoutcount;

#ifdef DEBUG_DARWIN
	printf("darwin_iohidsystem_connect_method_scalari_scalaro()\n");
#endif
	rep->rep_outcount = 0;
	maxoutcount = req->req_in[req->req_incount];

	switch (req->req_selector) {
	case DARWIN_IOHIDCREATESHMEM: {
		/* Create the shared memory for HID events */
		int vers;
		int error;
		size_t memsize;
		vaddr_t kvaddr;
		struct proc *p;
		struct darwin_iohidsystem_thread_args *dita;
		struct darwin_emuldata *ded;

		vers = req->req_in[0]; /* 1 */
#ifdef DEBUG_DARWIN
		printf("DARWIN_IOHIDCREATESHMEM: version = %d\n", vers);
#endif
		memsize = round_page(sizeof(struct darwin_iohidsystem_shmem));

		/* If it has not been used yet, initialize it */
		if (darwin_iohidsystem_shmem == NULL) {
			struct proc *dita_p;

			darwin_iohidsystem_shmem = uao_create(memsize, 0);

			error = uvm_map(kernel_map, &kvaddr, memsize,
			    darwin_iohidsystem_shmem, 0, PAGE_SIZE,
			    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW,
			    UVM_INH_SHARE, UVM_ADV_RANDOM, 0));
			if (error != 0) {
				uao_detach(darwin_iohidsystem_shmem);
				darwin_iohidsystem_shmem = NULL;
				return mach_msg_error(args, error);
			}

			error = uvm_map_pageable(kernel_map, kvaddr,
			    kvaddr + memsize, FALSE, 0);
			if (error != 0) {
				uao_detach(darwin_iohidsystem_shmem);
				darwin_iohidsystem_shmem = NULL;
				return mach_msg_error(args, error);
			}

			darwin_iohidsystem_shmeminit(kvaddr);

			p = args->l->l_proc;
			ded = (struct darwin_emuldata *)p->p_emuldata;

			dita = malloc(sizeof(*dita), M_TEMP, M_WAITOK);
			dita->dita_shmem = kvaddr;
			dita->dita_done = 0;

			kthread_create1(darwin_iohidsystem_thread,
			    (void *)dita, &dita_p, "iohidsystem");

			dita->dita_l = LIST_FIRST(&dita_p->p_lwps);

			/*
			 * Make sure the thread got the informations
			 * before exitting and destroying dita.
			 */
			while (!dita->dita_done)
				(void)tsleep(&dita->dita_done,
				    PZERO, "iohid_done", 0);

			ded->ded_hidsystem_finished =
			    dita->dita_hidsystem_finished;

			free(dita, M_TEMP);

		}
		rep->rep_outcount = 0;
		break;
	}

	case DARWIN_IOHIDSETEVENTSENABLE: {
		/* Enable or disable events */
		int enable;

		enable = req->req_in[0];
#ifdef DEBUG_DARWIN
		printf("DARWIN_IOHIDSETEVENTSENABLE: enable = %d\n", enable);
#endif
		/* For now, this is a no-op */
		rep->rep_outcount = 0;
		break;
	}

	case DARWIN_IOHIDSETCURSORENABLE: {
		/* Enable or disable the cursor */
		int enable;

		enable = req->req_in[0];
#ifdef DEBUG_DARWIN
		printf("DARWIN_IOHIDSETCURSORENABLE: enable = %d\n", enable);
#endif
		/* We don't support it */
		rep->rep_outcount = 0;
		break;
	}

	case DARWIN_IOHIDPOSTEVENT: {
		darwin_nxll_event *dne;

		dne = (darwin_nxll_event *)&req->req_in[0];
#ifdef DEBUG_DARWIN
		printf("DARWIN_IOHIDPOSTEVENT: setcursor = %d, type = %d, "
		    " location = (%d, %d), setflags = %x, flags = %x\n",
		dne->dne_setcursor, dne->dne_type, dne->dne_location.x,
		dne->dne_location.y, dne->dne_setflags, dne->dne_flags);
#endif
		/* We don't support it yet */
		rep->rep_outcount = 0;
		break;
	}

	default:
#ifdef DEBUG_DARWIN
		printf("Unknown selector %d\n", req->req_selector);
#endif
		return mach_msg_error(args, EINVAL);
		break;
	}

	*msglen = sizeof(*rep) - ((16 + rep->rep_outcount) * sizeof(int));
	mach_set_header(rep, req, *msglen);
	mach_set_trailer(rep, *msglen);

	return 0;
}

int
darwin_iohidsystem_connect_method_structi_structo(args)
	struct mach_trap_args *args;
{
	mach_io_connect_method_structi_structo_request_t *req = args->smsg;
	mach_io_connect_method_structi_structo_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	int maxoutcount;
	int error;

#ifdef DEBUG_DARWIN
	printf("darwin_iohidsystem_connect_method_structi_structo()\n");
#endif
	rep->rep_outcount = 0;
	/* maxoutcount is word aligned */
	maxoutcount = req->req_in[(req->req_incount & ~0x3UL) + 4];

	switch (req->req_selector) {
	case DARWIN_IOHIDSETMOUSELOCATION: {
		struct wscons_event wsevt;
		dev_t dev;
		const struct cdevsw *wsmux;
		darwin_iogpoint *pt = (darwin_iogpoint *)&req->req_in[0];

#ifdef DEBUG_DARWIN
		printf("DARWIN_IOHIDSETMOUSELOCATION: %d,%d\n", pt->x, pt->y);
#endif
		/*
		 * Use the wsmux given by sysctl emul.darwin.iohidsystem_mux
		 */
		error = darwin_findwsmux(&dev, darwin_iohidsystem_mux);
		if (error != 0)
			return mach_msg_error(args, error);

		if ((wsmux = cdevsw_lookup(dev)) == NULL)
			return mach_msg_error(args, ENXIO);

		wsevt.type = WSCONS_EVENT_MOUSE_ABSOLUTE_X;
		wsevt.value = pt->x;
		if ((error = (wsmux->d_ioctl)(dev,
		    WSMUXIO_INJECTEVENT, (caddr_t)&wsevt, 0,  l)) != 0)
			return mach_msg_error(args, error);

		wsevt.type = WSCONS_EVENT_MOUSE_ABSOLUTE_Y;
		wsevt.value = pt->y;
		if ((error = (wsmux->d_ioctl)(dev,
		    WSMUXIO_INJECTEVENT, (caddr_t)&wsevt, 0, l)) != 0)
			return mach_msg_error(args, error);

		rep->rep_outcount = 0;
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
darwin_iohidsystem_connect_map_memory(args)
	struct mach_trap_args *args;
{
	mach_io_connect_map_memory_request_t *req = args->smsg;
	mach_io_connect_map_memory_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct proc *p = args->l->l_proc;
	int error;
	size_t memsize;
	vaddr_t pvaddr;

#ifdef DEBUG_DARWIN
	printf("darwin_iohidsystem_connect_map_memory()\n");
#endif
	memsize = round_page(sizeof(struct darwin_iohidsystem_shmem));

	if (darwin_iohidsystem_shmem == NULL)
		return mach_msg_error(args, ENOMEM);

	uao_reference(darwin_iohidsystem_shmem);
	pvaddr = VM_DEFAULT_ADDRESS(p->p_vmspace->vm_daddr, memsize);

	if ((error = uvm_map(&p->p_vmspace->vm_map, &pvaddr,
	    memsize, darwin_iohidsystem_shmem, 0, PAGE_SIZE,
	    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW,
	    UVM_INH_SHARE, UVM_ADV_RANDOM, 0))) != 0)
		return mach_msg_error(args, error);

#ifdef DEBUG_DARWIN
	printf("pvaddr = 0x%08lx\n", (long)pvaddr);
#endif

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;
	rep->rep_addr = pvaddr;
	rep->rep_len = sizeof(struct darwin_iohidsystem_shmem);

	mach_set_trailer(rep, *msglen);
	return 0;
}

static void
darwin_iohidsystem_thread(args)
	void *args;
{
	struct darwin_iohidsystem_thread_args *dita;
	struct darwin_iohidsystem_shmem *shmem;
	struct darwin_iohidsystem_evglobals *evg;
	darwin_iohidsystem_event_item *diei = NULL; /* XXX: gcc */
	darwin_iohidsystem_event *die;
	dev_t dev;
	const struct cdevsw *wsmux;
	struct uio auio;
	struct iovec aiov;
	struct wscons_event wsevt;
	int error = 0;
	struct mach_right *mr;
	struct lwp *l;
	int finished = 0;

#ifdef DEBUG_DARWIN
	printf("darwin_iohidsystem_thread: start\n");
#endif
	dita = (struct darwin_iohidsystem_thread_args *)args;
	shmem = (struct darwin_iohidsystem_shmem *)dita->dita_shmem;
	l = dita->dita_l;
	dita->dita_hidsystem_finished = &finished;

	/*
	 * Allow the parent to read dita_hidsystem_finished
	 * and to get rid of dita. Once the parent is awaken,
	 * it holds a reference to our on-stack finished flag,
	 * hence we cannot exi before the parent sets this flag.
	 * This is done in darwin_proc_exit()
	 */
	dita->dita_done = 1;
	wakeup(&dita->dita_done);

	evg = (struct darwin_iohidsystem_evglobals *)&shmem->dis_evglobals;

	/*
	 * Use the wsmux given by sysctl emul.darwin.iohidsystem_mux
	 */
	if ((error = darwin_findwsmux(&dev, darwin_iohidsystem_mux)) != 0)
		goto out2;

	if ((wsmux = cdevsw_lookup(dev)) == NULL) {
		error = ENXIO;
		goto out2;
	}

	if ((error = (wsmux->d_open)(dev, FREAD|FWRITE, 0, l)) != 0)
		goto out2;

	while(!finished) {
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		aiov.iov_base = &wsevt;
		aiov.iov_len = sizeof(wsevt);
		auio.uio_resid = sizeof(wsevt);
		auio.uio_offset = 0;
		auio.uio_rw = UIO_READ;
		UIO_SETUP_SYSSPACE(&auio);

		if ((error = (wsmux->d_read)(dev, &auio, 0)) != 0) {
#ifdef DEBUG_DARWIN
			printf("iohidsystem: read error %d\n", error);
#endif
			goto out1;
		}

		diei = &evg->evg_evqueue[evg->evg_event_last];
		while (diei->diei_sem != 0)
			tsleep(&diei->diei_sem, PZERO, "iohid_lock", 1);

		/*
		 * No need to take the lock since we will not
		 * yield control to the user process.
		 */

		diei->diei_next = evg->evg_event_tail;

		diei = &evg->evg_evqueue[evg->evg_event_tail];
		die = (darwin_iohidsystem_event *)&diei->diei_event;

		darwin_wscons_to_iohidsystem(&wsevt, die);

		evg->evg_event_last = evg->evg_event_tail;
		evg->evg_event_tail++;
		if (evg->evg_event_tail == DARWIN_IOHIDSYSTEM_EVENTQUEUE_LEN)
			evg->evg_event_tail = 0;

		/*
		 * Send a I/O notification if the process
		 * has consumed all available entries
		 */
		if (evg->evg_event_last == evg->evg_event_head) {
			mr = darwin_iohidsystem_devclass.mid_notify;
			if (mr != NULL)
				mach_notify_iohidsystem(l, mr);
		}


		/*
		 * If the queue is full, ie: the next event slot is not
		 * yet consumed, sleep until the process consumes it.
		 */
		diei = &evg->evg_evqueue[evg->evg_event_tail];
		die = (darwin_iohidsystem_event *)&diei->diei_event;
		while (die->die_type != 0)
			tsleep(&die->die_type, PZERO, "iohid_full", 1);

	}

out1:
	(wsmux->d_close)(dev, FREAD|FWRITE, 0, l);

out2:
	while (!finished)
		tsleep((void *)&finished, PZERO, "iohid_exit", 0);

	uao_detach(darwin_iohidsystem_shmem);
	darwin_iohidsystem_shmem = NULL;
	kthread_exit(error);
	/* NOTREACHED */
};

static void
darwin_iohidsystem_shmeminit(kvaddr)
	vaddr_t kvaddr;
{
	struct darwin_iohidsystem_shmem *shmem;
	struct darwin_iohidsystem_evglobals *evglobals;
	int i;

	shmem = (struct darwin_iohidsystem_shmem *)kvaddr;
	shmem->dis_global_offset =
	    (size_t)&shmem->dis_evglobals - (size_t)&shmem->dis_global_offset;
	shmem->dis_private_offset =
	    shmem->dis_global_offset + sizeof(*evglobals);

	evglobals = &shmem->dis_evglobals;
	evglobals->evg_struct_size = sizeof(*evglobals);

	for (i = 0; i < DARWIN_IOHIDSYSTEM_EVENTQUEUE_LEN; i++)
		evglobals->evg_evqueue[i].diei_next = i + 1;
	evglobals->
	    evg_evqueue[DARWIN_IOHIDSYSTEM_EVENTQUEUE_LEN - 1].diei_next = 0;

	return;
}

static int
darwin_findwsmux(dev, mux)
	dev_t *dev;
	int mux;
{
	struct wsmux_softc *wsm_sc;
	int minor, major;

	if ((wsm_sc = wsmux_getmux(mux)) == NULL)
		return ENODEV;

	major = cdevsw_lookup_major(&wsmux_cdevsw);
	minor = device_unit(&wsm_sc->sc_base.me_dv);
	*dev = makedev(major, minor);

	return 0;
}

static void
darwin_wscons_to_iohidsystem(wsevt, hidevt)
	struct wscons_event *wsevt;
	darwin_iohidsystem_event *hidevt;
{
	struct timeval tv;
	static int px = 0; /* Previous mouse location */
	static int py = 0;
	static int pf = 0; /* previous kbd flags */

	microtime(&tv);
	(void)memset(hidevt, 0, sizeof(*hidevt));
	hidevt->die_time_hi = tv.tv_sec;
	hidevt->die_time_lo = tv.tv_usec;
	hidevt->die_location_x = px;
	hidevt->die_location_y = py;
	hidevt->die_flags = pf;

	switch (wsevt->type) {
	case WSCONS_EVENT_MOUSE_DELTA_X:
		hidevt->die_type = DARWIN_NX_MOUSEMOVED;
		px += wsevt->value;
		hidevt->die_data.mouse_move.dx = wsevt->value;
		hidevt->die_location_x = px;
		break;

	case WSCONS_EVENT_MOUSE_DELTA_Y:
		hidevt->die_type = DARWIN_NX_MOUSEMOVED;
		py -= wsevt->value;
		hidevt->die_data.mouse_move.dy = wsevt->value;
		hidevt->die_location_y = py;
		break;

	case WSCONS_EVENT_MOUSE_ABSOLUTE_X:
		hidevt->die_type = DARWIN_NX_MOUSEMOVED;
		hidevt->die_location_x = wsevt->value;
		px = wsevt->value;
		break;

	case WSCONS_EVENT_MOUSE_ABSOLUTE_Y:
		hidevt->die_type = DARWIN_NX_MOUSEMOVED;
		hidevt->die_location_y = wsevt->value;
		py = wsevt->value;
		break;

	case WSCONS_EVENT_MOUSE_DOWN:
		if (wsevt->value == 0)
			hidevt->die_type = DARWIN_NX_LMOUSEDOWN;
		else if (wsevt->value == 1)
			hidevt->die_type = DARWIN_NX_RMOUSEDOWN;
		else {
#ifdef DEBUG_DARWIN
			printf("Unknown mouse button %d\n", wsevt->value);
#endif
			break;
		}
		hidevt->die_data.mouse.subx = px;
		hidevt->die_data.mouse.suby = py;
		hidevt->die_data.mouse.buttonid = wsevt->value;
		break;

	case WSCONS_EVENT_MOUSE_UP:
		if (wsevt->value == 0)
			hidevt->die_type = DARWIN_NX_LMOUSEUP;
		else if (wsevt->value == 1)
			hidevt->die_type = DARWIN_NX_RMOUSEUP;
		else {
#ifdef DEBUG_DARWIN
			printf("Unknown mouse button %d\n", wsevt->value);
#endif
			break;
		}
		hidevt->die_data.mouse.subx = px;
		hidevt->die_data.mouse.suby = py;
		hidevt->die_data.mouse.buttonid = wsevt->value;
		break;

	case WSCONS_EVENT_KEY_DOWN:
		hidevt->die_type = DARWIN_NX_KEYDOWN;
		hidevt->die_data.key.charset = wsevt->value;
		hidevt->die_data.key.charcode = wsevt->value;
		hidevt->die_data.key.orig_charcode = wsevt->value;
		hidevt->die_data.key.keycode = wsevt->value; /* Translate */
		hidevt->die_data.key.keyboardtype = 0xcd; /* XXX */
		break;

	case WSCONS_EVENT_KEY_UP:
		hidevt->die_type = DARWIN_NX_KEYUP;
		hidevt->die_data.key.charset = wsevt->value;
		hidevt->die_data.key.charcode = wsevt->value;
		hidevt->die_data.key.orig_charcode = wsevt->value;
		hidevt->die_data.key.keycode = wsevt->value; /* Translate */
		hidevt->die_data.key.keyboardtype = 0xcd; /* XXX */
		break;

	default:

#ifdef DEBUG_DARWIN
		printf("Untranslated wsevt->type = %d, wsevt->value = %d\n",
		    wsevt->type, wsevt->value);
#endif
		break;
	}

	return;
}

static void
mach_notify_iohidsystem(struct lwp *l __unused, struct mach_right *mr)
{
	struct mach_port *mp;
	mach_notify_iohidsystem_request_t *req;

	mp = mr->mr_port;

#ifdef DEBUG_MACH
	if (mp == NULL) {
		printf("Warning: notification right without a port\n");
		return;
	}
#endif

	req = malloc(sizeof(*req), M_EMULDATA, M_WAITOK | M_ZERO);

	req->req_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	req->req_msgh.msgh_size = sizeof(*req) - sizeof(req->req_trailer);
	req->req_msgh.msgh_local_port = mr->mr_name;
	req->req_msgh.msgh_id = 0;

	mach_set_trailer(req, sizeof(*req));

#ifdef KTRACE
	if (KTRPOINT(l->l_proc, KTR_USER))
		ktruser(l, "notify_iohidsystem", NULL, 0, 0);
#endif

	mr->mr_refcount++;

	/*
	 * Call mach_message_get with a NULL lwp: the message is sent
	 * by the kernel, not a userprocess. If we do not do that,
	 * we crash because iohidsystem thread has p->p_emuldata == NULL.
	 */
	mach_message_get((mach_msg_header_t *)req, sizeof(*req), mp, NULL);
#ifdef DEBUG_MACH_MSG
	printf("pid %d: message queued on port %p (%d) [%p]\n",
	    l->l_proc->p_pid, mp, req->req_msgh.msgh_id,
	    mp->mp_recv->mr_sethead);
#endif
	wakeup(mp->mp_recv->mr_sethead);

	return;
}

void
darwin_iohidsystem_postfake(l)
	struct lwp *l;
{
	const struct cdevsw *wsmux;
	dev_t dev;
	int error;
	struct wscons_event wsevt;

	/*
	 * Use the wsmux given by sysctl emul.darwin.iohidsystem_mux
	 */
	if ((error = darwin_findwsmux(&dev, darwin_iohidsystem_mux)) != 0)
		return;

	if ((wsmux = cdevsw_lookup(dev)) == NULL)
		return;

	wsevt.type = 0;
	wsevt.value = 0;
	(wsmux->d_ioctl)(dev, WSMUXIO_INJECTEVENT, (caddr_t)&wsevt, 0,  l);

	return;
}
