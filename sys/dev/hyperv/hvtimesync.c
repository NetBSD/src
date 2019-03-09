/*	$NetBSD: hvtimesync.c,v 1.2.2.2 2019/03/09 17:10:19 martin Exp $	*/

/*-
 * Copyright (c) 2014,2016-2017 Microsoft Corp.
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
__KERNEL_RCSID(0, "$NetBSD: hvtimesync.c,v 1.2.2.2 2019/03/09 17:10:19 martin Exp $");
#endif
#ifdef __FBSDID
__FBSDID("$FreeBSD: head/sys/dev/hyperv/utilities/vmbus_timesync.c 322488 2017-08-14 06:00:50Z sephe $");
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/pmf.h>
#include <sys/sysctl.h>
#include <sys/timetc.h>

#include <dev/hyperv/vmbusvar.h>
#include <dev/hyperv/vmbusicreg.h>
#include <dev/hyperv/vmbusicvar.h>

#define VMBUS_TIMESYNC_FWVER_MAJOR	3
#define VMBUS_TIMESYNC_FWVER		\
	    VMBUS_IC_VERSION(VMBUS_TIMESYNC_FWVER_MAJOR, 0)

#define VMBUS_TIMESYNC_MSGVER_MAJOR	4
#define VMBUS_TIMESYNC_MSGVER		\
	    VMBUS_IC_VERSION(VMBUS_TIMESYNC_MSGVER_MAJOR, 0)

#define VMBUS_TIMESYNC_MSGVER4(sc)	\
	    VMBUS_ICVER_LE(VMBUS_IC_VERSION(4, 0), (sc)->sc_vmbusic.sc_msgver)

#define VMBUS_TIMESYNC_DORTT(sc)	\
	    (VMBUS_TIMESYNC_MSGVER4((sc)) && (hyperv_tc64 != NULL))

static int	hvtimesync_match(device_t, cfdata_t, void *);
static void	hvtimesync_attach(device_t, device_t, void *);
static int	hvtimesync_detach(device_t, int);

static void	hvtimesync_channel_cb(void *);
static int	hvtimesync_sysctl_setup(device_t);

struct hvtimesync_softc {
	struct vmbusic_softc	sc_vmbusic;
};

CFATTACH_DECL_NEW(hvtimesync, sizeof(struct hvtimesync_softc),
    hvtimesync_match, hvtimesync_attach, hvtimesync_detach, NULL);

static int hvtimesync_ignore_sync;
static int hvtimesnyc_sample_verbose;
static int hvtimesync_sample_thresh = -1;

static int
hvtimesync_match(device_t parent, cfdata_t cf, void *aux)
{
	struct vmbus_attach_args *aa = aux;

	return vmbusic_probe(aa, &hyperv_guid_timesync);
}

static void
hvtimesync_attach(device_t parent, device_t self, void *aux)
{
	struct vmbus_attach_args *aa = aux;
	int error;

	aprint_naive("\n");
	aprint_normal(": Hyper-V Time Synchronization Service\n");

	error = vmbusic_attach(self, aa, hvtimesync_channel_cb);
	if (error)
		return;

	(void) pmf_device_register(self, NULL, NULL);

	(void) hvtimesync_sysctl_setup(self);
}

static int
hvtimesync_detach(device_t self, int flags)
{
	int error;

	error = vmbusic_detach(self, flags);
	if (error)
		return error;

	pmf_device_deregister(self);

	return 0;
}

static void
do_timesync(struct hvtimesync_softc *sc, uint64_t hvtime, uint64_t sent_tc,
    uint8_t tsflags)
{
	struct vmbusic_softc *vsc = &sc->sc_vmbusic;
	struct timespec vm_ts, hv_ts;
	uint64_t hv_ns, vm_ns, rtt = 0;
	int64_t diff;

	if (VMBUS_TIMESYNC_DORTT(sc))
		rtt = hyperv_tc64() - sent_tc;

	hv_ns = (hvtime - VMBUS_ICMSG_TS_BASE + rtt) * HYPERV_TIMER_NS_FACTOR;
	nanotime(&vm_ts);
	vm_ns = (vm_ts.tv_sec * NANOSECOND) + vm_ts.tv_nsec;

	if ((tsflags & VMBUS_ICMSG_TS_FLAG_SYNC) && !hvtimesync_ignore_sync) {
#if 0
		device_printf(vsc->sc_dev,
		    "apply sync request, hv: %ju, vm: %ju\n",
		    (uintmax_t)hv_ns, (uintmax_t)vm_ns);
#endif
		hv_ts.tv_sec = hv_ns / NANOSECOND;
		hv_ts.tv_nsec = hv_ns % NANOSECOND;
		tc_setclock(&hv_ts);
		/* Done! */
		return;
	}

	if ((tsflags & VMBUS_ICMSG_TS_FLAG_SAMPLE) &&
	    hvtimesync_sample_thresh >= 0) {
		if (hvtimesnyc_sample_verbose) {
			device_printf(vsc->sc_dev,
			    "sample request, hv: %ju, vm: %ju\n",
			    (uintmax_t)hv_ns, (uintmax_t)vm_ns);
		}

		if (hv_ns > vm_ns)
			diff = hv_ns - vm_ns;
		else
			diff = vm_ns - hv_ns;
		/* nanosec -> millisec */
		diff /= 1000000;

		if (diff > hvtimesync_sample_thresh) {
			device_printf(vsc->sc_dev,
			    "apply sample request, hv: %ju, vm: %ju\n",
			    (uintmax_t)hv_ns, (uintmax_t)vm_ns);
			hv_ts.tv_sec = hv_ns / NANOSECOND;
			hv_ts.tv_nsec = hv_ns % NANOSECOND;
			tc_setclock(&hv_ts);
		}
		/* Done */
		return;
	}
}

static void
hvtimesync_channel_cb(void *arg)
{
	struct hvtimesync_softc *sc = arg;
	struct vmbusic_softc *vsc = &sc->sc_vmbusic;
	struct vmbus_channel *ch = vsc->sc_chan;
	struct vmbus_icmsg_hdr *hdr;
	uint64_t rid;
	uint32_t rlen;
	int error;

	error = vmbus_channel_recv(ch, vsc->sc_buf, vsc->sc_buflen,
	    &rlen, &rid, 0);
	if (error || rlen == 0) {
		if (error != EAGAIN) {
			DPRINTF("%s: timesync error=%d len=%u\n",
			    device_xname(vsc->sc_dev), error, rlen);
		}
		return;
	}
	if (rlen < sizeof(*hdr)) {
		DPRINTF("%s: hvtimesync short read len=%u\n",
		    device_xname(vsc->sc_dev), rlen);
		return;
	}

	hdr = (struct vmbus_icmsg_hdr *)vsc->sc_buf;
	switch (hdr->ic_type) {
	case VMBUS_ICMSG_TYPE_NEGOTIATE:
		error = vmbusic_negotiate(vsc, hdr, &rlen, VMBUS_TIMESYNC_FWVER,
		    VMBUS_TIMESYNC_MSGVER);
		if (error)
			return;
		if (VMBUS_TIMESYNC_DORTT(sc)) {
			DPRINTF("%s: RTT\n", device_xname(vsc->sc_dev));
		}
		break;

	case VMBUS_ICMSG_TYPE_TIMESYNC:
		if (VMBUS_TIMESYNC_MSGVER4(sc)) {
			struct vmbus_icmsg_timesync4 *msg4;

			if (rlen < sizeof(*msg4)) {
				DPRINTF("%s: invalid timesync4 len=%u\n",
				    device_xname(vsc->sc_dev), rlen);
				return;
			}

			msg4 = (struct vmbus_icmsg_timesync4 *)hdr;
			do_timesync(sc, msg4->ic_hvtime, msg4->ic_sent_tc,
			    msg4->ic_tsflags);
		} else {
			struct vmbus_icmsg_timesync *msg;

			if (rlen < sizeof(*msg)) {
				DPRINTF("%s: invalid timesync len=%u\n",
				    device_xname(vsc->sc_dev), rlen);
				return;
			}

			msg = (struct vmbus_icmsg_timesync *)hdr;
			do_timesync(sc, msg->ic_hvtime, 0, msg->ic_tsflags);
		}
		break;

	default:
		device_printf(vsc->sc_dev,
		    "unhandled _timesync message type %u\n", hdr->ic_type);
		return;
	}

	(void) vmbusic_sendresp(vsc, ch, vsc->sc_buf, rlen, rid);
}

static int
hvtimesync_sysctl_setup(device_t self)
{
	struct hvtimesync_softc *sc = device_private(self);
	struct vmbusic_softc *vsc = &sc->sc_vmbusic;
	const struct sysctlnode *node;
	int error;

	error = sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);
	if (error)
		return error;
	error = sysctl_createv(NULL, 0, &node, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hyperv", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return error;

	error = sysctl_createv(&vsc->sc_log, 0, &node, &node,
	    0, CTLTYPE_NODE, "timesync", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return error;

	error = sysctl_createv(&vsc->sc_log, 0, &node, NULL,
	    CTLFLAG_READWRITE,
	    CTLTYPE_INT, "ignore_sync", NULL,
	    NULL, 0, &hvtimesync_ignore_sync, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		return error;
	error = sysctl_createv(&vsc->sc_log, 0, &node, NULL,
	    CTLFLAG_READWRITE,
	    CTLTYPE_INT, "sample_verbose", NULL,
	    NULL, 0, &hvtimesnyc_sample_verbose, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		return error;
	error = sysctl_createv(&vsc->sc_log, 0, &node, NULL,
	    CTLFLAG_READWRITE,
	    CTLTYPE_INT, "sample_thresh", NULL,
	    NULL, 0, &hvtimesync_sample_thresh, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		return error;

	return 0;
}

MODULE(MODULE_CLASS_DRIVER, hvtimesync, "vmbus");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
hvtimesync_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_hvtimesync,
		    cfattach_ioconf_hvtimesync, cfdata_ioconf_hvtimesync);
#endif
		break;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_hvtimesync,
		    cfattach_ioconf_hvtimesync, cfdata_ioconf_hvtimesync);
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
