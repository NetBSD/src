/*	$NetBSD: hvshutdown.c,v 1.2.2.2 2019/03/09 17:10:19 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: hvshutdown.c,v 1.2.2.2 2019/03/09 17:10:19 martin Exp $");
#endif
#ifdef __FBSDID
__FBSDID("$FreeBSD: head/sys/dev/hyperv/utilities/vmbus_shutdown.c 310324 2016-12-20 09:46:14Z sephe $");
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/pmf.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <dev/hyperv/vmbusvar.h>
#include <dev/hyperv/vmbusicreg.h>
#include <dev/hyperv/vmbusicvar.h>

#define VMBUS_SHUTDOWN_FWVER_MAJOR	3
#define VMBUS_SHUTDOWN_FWVER		\
	    VMBUS_IC_VERSION(VMBUS_SHUTDOWN_FWVER_MAJOR, 0)

#define VMBUS_SHUTDOWN_MSGVER_MAJOR	3
#define VMBUS_SHUTDOWN_MSGVER		\
	    VMBUS_IC_VERSION(VMBUS_SHUTDOWN_MSGVER_MAJOR, 0)

static int	hvshutdown_match(device_t, cfdata_t, void *);
static void	hvshutdown_attach(device_t, device_t, void *);
static int	hvshutdown_detach(device_t, int);

static void	hvshutdown_channel_cb(void *);

struct hvshutdown_softc {
	struct vmbusic_softc	sc_vmbusic;

	struct sysmon_pswitch	sc_smpsw;
};

CFATTACH_DECL_NEW(hvshutdown, sizeof(struct hvshutdown_softc),
    hvshutdown_match, hvshutdown_attach, hvshutdown_detach, NULL);

static int
hvshutdown_match(device_t parent, cfdata_t cf, void *aux)
{
	struct vmbus_attach_args *aa = aux;

	return vmbusic_probe(aa, &hyperv_guid_shutdown);
}

static void
hvshutdown_attach(device_t parent, device_t self, void *aux)
{
	struct hvshutdown_softc *sc = device_private(self);
	struct vmbus_attach_args *aa = aux;
	int error;

	aprint_naive("\n");
	aprint_normal(": Hyper-V Guest Shutdown Service\n");

	error = vmbusic_attach(self, aa, hvshutdown_channel_cb);
	if (error)
		return;

	(void) pmf_device_register(self, NULL, NULL);

	sysmon_task_queue_init();

	sc->sc_smpsw.smpsw_name = device_xname(self);
	sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_POWER;
	(void) sysmon_pswitch_register(&sc->sc_smpsw);
}

static int
hvshutdown_detach(device_t self, int flags)
{
	struct hvshutdown_softc *sc = device_private(self);
	int error;

	error = vmbusic_detach(self, flags);
	if (error)
		return error;

	pmf_device_deregister(self);
	sysmon_pswitch_unregister(&sc->sc_smpsw);

	return 0;
}

static void
hvshutdown_do_shutdown(void *arg)
{
	struct hvshutdown_softc *sc = arg;

	sysmon_pswitch_event(&sc->sc_smpsw, PSWITCH_EVENT_PRESSED);
}

static void
hvshutdown_channel_cb(void *arg)
{
	struct hvshutdown_softc *sc = arg;
	struct vmbusic_softc *vsc = &sc->sc_vmbusic;
	struct vmbus_channel *ch = vsc->sc_chan;
	struct vmbus_icmsg_hdr *hdr;
	struct vmbus_icmsg_shutdown *msg;
	uint64_t rid;
	uint32_t rlen;
	int error;
	bool do_shutdown = false;

	error = vmbus_channel_recv(ch, vsc->sc_buf, vsc->sc_buflen,
	    &rlen, &rid, 0);
	if (error || rlen == 0) {
		if (error != EAGAIN) {
			DPRINTF("%s: shutdown error=%d len=%u\n",
			    device_xname(vsc->sc_dev), error, rlen);
		}
		return;
	}
	if (rlen < sizeof(*hdr)) {
		DPRINTF("%s: shutdown short read len=%u\n",
		    device_xname(vsc->sc_dev), rlen);
		return;
	}

	hdr = (struct vmbus_icmsg_hdr *)vsc->sc_buf;
	switch (hdr->ic_type) {
	case VMBUS_ICMSG_TYPE_NEGOTIATE:
		error = vmbusic_negotiate(vsc, hdr, &rlen, VMBUS_SHUTDOWN_FWVER,
		    VMBUS_SHUTDOWN_MSGVER);
		if (error)
			return;
		break;

	case VMBUS_ICMSG_TYPE_SHUTDOWN:
		if (rlen < VMBUS_ICMSG_SHUTDOWN_SIZE_MIN) {
			DPRINTF("%s: invalid shutdown len=%u\n",
			    device_xname(vsc->sc_dev), rlen);
			return;
		}

		msg = (struct vmbus_icmsg_shutdown *)hdr;
		if (msg->ic_haltflags == 0 || msg->ic_haltflags == 1) {
			device_printf(vsc->sc_dev, "shutdown requested\n");
			hdr->ic_status = VMBUS_ICMSG_STATUS_OK;
			do_shutdown = true;
		} else {
			device_printf(vsc->sc_dev,
			    "unknown shutdown flags 0x%08x\n",
			    msg->ic_haltflags);
			hdr->ic_status = VMBUS_ICMSG_STATUS_FAIL;
		}
		break;

	default:
		device_printf(vsc->sc_dev,
		    "unhandled shutdown message type %u\n", hdr->ic_type);
		return;
	}

	(void) vmbusic_sendresp(vsc, ch, vsc->sc_buf, rlen, rid);

	if (do_shutdown)
		sysmon_task_queue_sched(0, hvshutdown_do_shutdown, sc);
}

MODULE(MODULE_CLASS_DRIVER, hvshutdown, "vmbus");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
hvshutdown_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_hvshutdown,
		    cfattach_ioconf_hvshutdown, cfdata_ioconf_hvshutdown);
#endif
		break;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_hvshutdown,
		    cfattach_ioconf_hvshutdown, cfdata_ioconf_hvshutdown);
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
