/*	$NetBSD: hvheartbeat.c,v 1.1 2019/02/15 08:54:01 nonaka Exp $	*/

/*-
 * Copyright (c) 2014,2016 Microsoft Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
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

#include <sys/cdefs.h>
#ifdef __KERNEL_RCSID
__KERNEL_RCSID(0, "$NetBSD: hvheartbeat.c,v 1.1 2019/02/15 08:54:01 nonaka Exp $");
#endif
#ifdef __FBSDID
__FBSDID("$FreeBSD: head/sys/dev/hyperv/utilities/vmbus_heartbeat.c 310324 2016-12-20 09:46:14Z sephe $");
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/pmf.h>

#include <dev/hyperv/vmbusvar.h>
#include <dev/hyperv/vmbusicreg.h>
#include <dev/hyperv/vmbusicvar.h>

#define VMBUS_HEARTBEAT_FWVER_MAJOR	3
#define VMBUS_HEARTBEAT_FWVER		\
	    VMBUS_IC_VERSION(VMBUS_HEARTBEAT_FWVER_MAJOR, 0)

#define VMBUS_HEARTBEAT_MSGVER_MAJOR	3
#define VMBUS_HEARTBEAT_MSGVER		\
	    VMBUS_IC_VERSION(VMBUS_HEARTBEAT_MSGVER_MAJOR, 0)

static int	hvheartbeat_match(device_t, cfdata_t, void *);
static void	hvheartbeat_attach(device_t, device_t, void *);
static int	hvheartbeat_detach(device_t, int);

static void	hvheartbeat_channel_cb(void *);

struct hvheartbeat_softc {
	struct vmbusic_softc	sc_vmbusic;
};

CFATTACH_DECL_NEW(hvheartbeat, sizeof(struct hvheartbeat_softc),
    hvheartbeat_match, hvheartbeat_attach, hvheartbeat_detach, NULL);

static int
hvheartbeat_match(device_t parent, cfdata_t cf, void *aux)
{
	struct vmbus_attach_args *aa = aux;

	return vmbusic_probe(aa, &hyperv_guid_heartbeat);
}

static void
hvheartbeat_attach(device_t parent, device_t self, void *aux)
{
	struct vmbus_attach_args *aa = aux;
	int error;

	aprint_naive("\n");
	aprint_normal(": Hyper-V Heartbeat\n");

	error = vmbusic_attach(self, aa, hvheartbeat_channel_cb);
	if (error)
		return;

	(void) pmf_device_register(self, NULL, NULL);
}

static int
hvheartbeat_detach(device_t self, int flags)
{
	int error;

	error = vmbusic_detach(self, flags);
	if (error)
		return error;

	pmf_device_deregister(self);

	return 0;
}

static void
hvheartbeat_channel_cb(void *arg)
{
	struct hvheartbeat_softc *sc = arg;
	struct vmbusic_softc *vsc = &sc->sc_vmbusic;
	struct vmbus_channel *ch = vsc->sc_chan;
	struct vmbus_icmsg_hdr *hdr;
	struct vmbus_icmsg_heartbeat *msg;
	uint64_t rid;
	uint32_t rlen;
	int error;

	error = vmbus_channel_recv(ch, vsc->sc_buf, vsc->sc_buflen,
	    &rlen, &rid, 0);
	if (error || rlen == 0) {
		if (error != EAGAIN) {
			DPRINTF("%s: heartbeat error=%d len=%u\n",
			    device_xname(vsc->sc_dev), error, rlen);
		}
		return;
	}
	if (rlen < sizeof(*hdr)) {
		DPRINTF("%s: heartbeat short read len=%u\n",
		    device_xname(vsc->sc_dev), rlen);
		return;
	}

	hdr = (struct vmbus_icmsg_hdr *)vsc->sc_buf;
	switch (hdr->ic_type) {
	case VMBUS_ICMSG_TYPE_NEGOTIATE:
		error = vmbusic_negotiate(vsc, hdr, &rlen,
		    VMBUS_HEARTBEAT_FWVER, VMBUS_HEARTBEAT_MSGVER);
		if (error)
			return;
		break;

	case VMBUS_ICMSG_TYPE_HEARTBEAT:
		if (rlen < VMBUS_ICMSG_HEARTBEAT_SIZE_MIN) {
			DPRINTF("%s: invalid heartbeat len=%u\n",
			    device_xname(vsc->sc_dev), rlen);
			return;
		}

		msg = (struct vmbus_icmsg_heartbeat *)hdr;
		msg->ic_seq++;
		break;

	default:
		device_printf(vsc->sc_dev,
		    "unhandled heartbeat message type %u\n", hdr->ic_type);
		return;
	}

	(void) vmbusic_sendresp(vsc, ch, vsc->sc_buf, rlen, rid);
}

MODULE(MODULE_CLASS_DRIVER, hvheartbeat, "vmbus");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
hvheartbeat_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_hvheartbeat,
		    cfattach_ioconf_hvheartbeat, cfdata_ioconf_hvheartbeat);
#endif
		break;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_hvheartbeat,
		    cfattach_ioconf_hvheartbeat, cfdata_ioconf_hvheartbeat);
#endif
		break;

	case MODULE_CMD_AUTOUNLOAD:
		error = EBUSY;
		break;

	default:
		error = ENOTTY;
		break;
	}

	return error;
}
