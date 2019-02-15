/*	$NetBSD: vmbus.c,v 1.2 2019/02/15 16:37:54 hannken Exp $	*/
/*	$OpenBSD: hyperv.c,v 1.43 2017/06/27 13:56:15 mikeb Exp $	*/

/*-
 * Copyright (c) 2009-2012 Microsoft Corp.
 * Copyright (c) 2012 NetApp Inc.
 * Copyright (c) 2012 Citrix Inc.
 * Copyright (c) 2016 Mike Belopuhov <mike@esdenera.com>
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

/*
 * The OpenBSD port was done under funding by Esdenera Networks GmbH.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vmbus.c,v 1.2 2019/02/15 16:37:54 hannken Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/atomic.h>
#include <sys/bitops.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/xcall.h>

#include <uvm/uvm_extern.h>

#include <dev/hyperv/vmbusvar.h>

#define VMBUS_GPADL_START		0xffff /* 0x10000 effectively */

/* Command submission flags */
#define HCF_SLEEPOK	0x0000
#define HCF_NOSLEEP	0x0002	/* M_NOWAIT */
#define HCF_NOREPLY	0x0004

static void	vmbus_attach_deferred(device_t);
static int	vmbus_alloc_dma(struct vmbus_softc *);
static void	vmbus_free_dma(struct vmbus_softc *);
static int	vmbus_init_interrupts(struct vmbus_softc *);
static void	vmbus_deinit_interrupts(struct vmbus_softc *);
static void	vmbus_init_synic(void *, void *);
static void	vmbus_deinit_synic(void *, void *);

static int	vmbus_connect(struct vmbus_softc *);
static int	vmbus_cmd(struct vmbus_softc *, void *, size_t, void *, size_t,
		    int);
static int	vmbus_start(struct vmbus_softc *, struct vmbus_msg *, paddr_t);
static int	vmbus_reply(struct vmbus_softc *, struct vmbus_msg *);
static void	vmbus_wait(struct vmbus_softc *,
		    int (*done)(struct vmbus_softc *, struct vmbus_msg *),
		    struct vmbus_msg *, void *, const char *);
static uint16_t vmbus_intr_signal(struct vmbus_softc *, paddr_t);
static void	vmbus_event_proc(void *, struct cpu_info *);
static void	vmbus_event_proc_compat(void *, struct cpu_info *);
static void	vmbus_message_proc(void *, struct cpu_info *);
static void	vmbus_message_softintr(void *);
static void	vmbus_channel_response(struct vmbus_softc *,
		    struct vmbus_chanmsg_hdr *);
static void	vmbus_channel_offer(struct vmbus_softc *,
		    struct vmbus_chanmsg_hdr *);
static void	vmbus_channel_rescind(struct vmbus_softc *,
		    struct vmbus_chanmsg_hdr *);
static void	vmbus_channel_delivered(struct vmbus_softc *,
		    struct vmbus_chanmsg_hdr *);
static int	vmbus_channel_scan(struct vmbus_softc *);
static void	vmbus_channel_cpu_default(struct vmbus_channel *);
static void	vmbus_process_offer(struct vmbus_softc *, struct vmbus_offer *);
static struct vmbus_channel *
		vmbus_channel_lookup(struct vmbus_softc *, uint32_t);
static int	vmbus_channel_ring_create(struct vmbus_channel *, uint32_t);
static void	vmbus_channel_ring_destroy(struct vmbus_channel *);
static void	vmbus_channel_pause(struct vmbus_channel *);
static uint32_t	vmbus_channel_unpause(struct vmbus_channel *);
static uint32_t	vmbus_channel_ready(struct vmbus_channel *);
static int	vmbus_attach_icdevs(struct vmbus_softc *);
static int	vmbus_attach_devices(struct vmbus_softc *);

static struct vmbus_softc *vmbus_sc;

static const struct {
	int	hmd_response;
	int	hmd_request;
	void	(*hmd_handler)(struct vmbus_softc *,
		    struct vmbus_chanmsg_hdr *);
} vmbus_msg_dispatch[] = {
	{ 0,					0, NULL },
	{ VMBUS_CHANMSG_CHOFFER,		0, vmbus_channel_offer },
	{ VMBUS_CHANMSG_CHRESCIND,		0, vmbus_channel_rescind },
	{ VMBUS_CHANMSG_CHREQUEST,		VMBUS_CHANMSG_CHOFFER, NULL },
	{ VMBUS_CHANMSG_CHOFFER_DONE,		0, vmbus_channel_delivered },
	{ VMBUS_CHANMSG_CHOPEN,			0, NULL },
	{ VMBUS_CHANMSG_CHOPEN_RESP,		VMBUS_CHANMSG_CHOPEN,
	  vmbus_channel_response },
	{ VMBUS_CHANMSG_CHCLOSE,		0, NULL },
	{ VMBUS_CHANMSG_GPADL_CONN,		0, NULL },
	{ VMBUS_CHANMSG_GPADL_SUBCONN,		0, NULL },
	{ VMBUS_CHANMSG_GPADL_CONNRESP,		VMBUS_CHANMSG_GPADL_CONN,
	  vmbus_channel_response },
	{ VMBUS_CHANMSG_GPADL_DISCONN,		0, NULL },
	{ VMBUS_CHANMSG_GPADL_DISCONNRESP,	VMBUS_CHANMSG_GPADL_DISCONN,
	  vmbus_channel_response },
	{ VMBUS_CHANMSG_CHFREE,			0, NULL },
	{ VMBUS_CHANMSG_CONNECT,		0, NULL },
	{ VMBUS_CHANMSG_CONNECT_RESP,		VMBUS_CHANMSG_CONNECT,
	  vmbus_channel_response },
	{ VMBUS_CHANMSG_DISCONNECT,		0, NULL },
};

const struct hyperv_guid hyperv_guid_network = {
	{ 0x63, 0x51, 0x61, 0xf8, 0x3e, 0xdf, 0xc5, 0x46,
	  0x91, 0x3f, 0xf2, 0xd2, 0xf9, 0x65, 0xed, 0x0e }
};

const struct hyperv_guid hyperv_guid_ide = {
	{ 0x32, 0x26, 0x41, 0x32, 0xcb, 0x86, 0xa2, 0x44,
	  0x9b, 0x5c, 0x50, 0xd1, 0x41, 0x73, 0x54, 0xf5 }
};

const struct hyperv_guid hyperv_guid_scsi = {
	{ 0xd9, 0x63, 0x61, 0xba, 0xa1, 0x04, 0x29, 0x4d,
	  0xb6, 0x05, 0x72, 0xe2, 0xff, 0xb1, 0xdc, 0x7f }
};

const struct hyperv_guid hyperv_guid_shutdown = {
	{ 0x31, 0x60, 0x0b, 0x0e, 0x13, 0x52, 0x34, 0x49,
	  0x81, 0x8b, 0x38, 0xd9, 0x0c, 0xed, 0x39, 0xdb }
};

const struct hyperv_guid hyperv_guid_timesync = {
	{ 0x30, 0xe6, 0x27, 0x95, 0xae, 0xd0, 0x7b, 0x49,
	  0xad, 0xce, 0xe8, 0x0a, 0xb0, 0x17, 0x5c, 0xaf }
};

const struct hyperv_guid hyperv_guid_heartbeat = {
	{ 0x39, 0x4f, 0x16, 0x57, 0x15, 0x91, 0x78, 0x4e,
	  0xab, 0x55, 0x38, 0x2f, 0x3b, 0xd5, 0x42, 0x2d }
};

const struct hyperv_guid hyperv_guid_kvp = {
	{ 0xe7, 0xf4, 0xa0, 0xa9, 0x45, 0x5a, 0x96, 0x4d,
	  0xb8, 0x27, 0x8a, 0x84, 0x1e, 0x8c, 0x03, 0xe6 }
};

const struct hyperv_guid hyperv_guid_vss = {
	{ 0x29, 0x2e, 0xfa, 0x35, 0x23, 0xea, 0x36, 0x42,
	  0x96, 0xae, 0x3a, 0x6e, 0xba, 0xcb, 0xa4, 0x40 }
};

const struct hyperv_guid hyperv_guid_dynmem = {
	{ 0xdc, 0x74, 0x50, 0x52, 0x85, 0x89, 0xe2, 0x46,
	  0x80, 0x57, 0xa3, 0x07, 0xdc, 0x18, 0xa5, 0x02 }
};

const struct hyperv_guid hyperv_guid_mouse = {
	{ 0x9e, 0xb6, 0xa8, 0xcf, 0x4a, 0x5b, 0xc0, 0x4c,
	  0xb9, 0x8b, 0x8b, 0xa1, 0xa1, 0xf3, 0xf9, 0x5a }
};

const struct hyperv_guid hyperv_guid_kbd = {
	{ 0x6d, 0xad, 0x12, 0xf9, 0x17, 0x2b, 0xea, 0x48,
	  0xbd, 0x65, 0xf9, 0x27, 0xa6, 0x1c, 0x76, 0x84 }
};

const struct hyperv_guid hyperv_guid_video = {
	{ 0x02, 0x78, 0x0a, 0xda, 0x77, 0xe3, 0xac, 0x4a,
	  0x8e, 0x77, 0x05, 0x58, 0xeb, 0x10, 0x73, 0xf8 }
};

const struct hyperv_guid hyperv_guid_fc = {
	{ 0x4a, 0xcc, 0x9b, 0x2f, 0x69, 0x00, 0xf3, 0x4a,
	  0xb7, 0x6b, 0x6f, 0xd0, 0xbe, 0x52, 0x8c, 0xda }
};

const struct hyperv_guid hyperv_guid_fcopy = {
	{ 0xe3, 0x4b, 0xd1, 0x34, 0xe4, 0xde, 0xc8, 0x41,
	  0x9a, 0xe7, 0x6b, 0x17, 0x49, 0x77, 0xc1, 0x92 }
};

const struct hyperv_guid hyperv_guid_pcie = {
	{ 0x1d, 0xf6, 0xc4, 0x44, 0x44, 0x44, 0x00, 0x44,
	  0x9d, 0x52, 0x80, 0x2e, 0x27, 0xed, 0xe1, 0x9f }
};

const struct hyperv_guid hyperv_guid_netdir = {
	{ 0x3d, 0xaf, 0x2e, 0x8c, 0xa7, 0x32, 0x09, 0x4b,
	  0xab, 0x99, 0xbd, 0x1f, 0x1c, 0x86, 0xb5, 0x01 }
};

const struct hyperv_guid hyperv_guid_rdesktop = {
	{ 0xf4, 0xac, 0x6a, 0x27, 0x15, 0xac, 0x6c, 0x42,
	  0x98, 0xdd, 0x75, 0x21, 0xad, 0x3f, 0x01, 0xfe }
};

/* Automatic Virtual Machine Activation (AVMA) Services */
const struct hyperv_guid hyperv_guid_avma1 = {
	{ 0x55, 0xb2, 0x87, 0x44, 0x8c, 0xb8, 0x3f, 0x40,
	  0xbb, 0x51, 0xd1, 0xf6, 0x9c, 0xf1, 0x7f, 0x87 }
};

const struct hyperv_guid hyperv_guid_avma2 = {
	{ 0xf4, 0xba, 0x75, 0x33, 0x15, 0x9e, 0x30, 0x4b,
	  0xb7, 0x65, 0x67, 0xac, 0xb1, 0x0d, 0x60, 0x7b }
};

const struct hyperv_guid hyperv_guid_avma3 = {
	{ 0xa0, 0x1f, 0x22, 0x99, 0xad, 0x24, 0xe2, 0x11,
	  0xbe, 0x98, 0x00, 0x1a, 0xa0, 0x1b, 0xbf, 0x6e }
};

const struct hyperv_guid hyperv_guid_avma4 = {
	{ 0x16, 0x57, 0xe6, 0xf8, 0xb3, 0x3c, 0x06, 0x4a,
	  0x9a, 0x60, 0x18, 0x89, 0xc5, 0xcc, 0xca, 0xb5 }
};

int
vmbus_match(device_t parent, cfdata_t cf, void *aux)
{

	if (cf->cf_unit != 0 ||
	    !hyperv_hypercall_enabled() ||
	    !hyperv_synic_supported())
		return 0;

	return 1;
}

int
vmbus_attach(struct vmbus_softc *sc)
{

	aprint_naive("\n");
	aprint_normal(": Hyper-V VMBus\n");

	vmbus_sc = sc;

	sc->sc_msgpool = pool_cache_init(sizeof(struct vmbus_msg), 8, 0, 0,
	    "hvmsg", NULL, IPL_NET, NULL, NULL, NULL);
	hyperv_set_message_proc(vmbus_message_proc, sc);

	if (vmbus_alloc_dma(sc))
		goto cleanup;

	if (vmbus_init_interrupts(sc))
		goto cleanup;

	if (vmbus_connect(sc))
		goto cleanup;

	aprint_normal_dev(sc->sc_dev, "protocol %d.%d\n",
	    VMBUS_VERSION_MAJOR(sc->sc_proto),
	    VMBUS_VERSION_MINOR(sc->sc_proto));

	if (sc->sc_proto == VMBUS_VERSION_WS2008 ||
	    sc->sc_proto == VMBUS_VERSION_WIN7) {
		hyperv_set_event_proc(vmbus_event_proc_compat, sc);
		sc->sc_channel_max = VMBUS_CHAN_MAX_COMPAT;
	} else {
		hyperv_set_event_proc(vmbus_event_proc, sc);
		sc->sc_channel_max = VMBUS_CHAN_MAX;
	}

	if (vmbus_channel_scan(sc))
		goto cleanup;

	/* Attach heartbeat, KVP and other "internal" services */
	vmbus_attach_icdevs(sc);

	/* Attach devices with external drivers */
	vmbus_attach_devices(sc);

	config_interrupts(sc->sc_dev, vmbus_attach_deferred);

	return 0;

cleanup:
	vmbus_deinit_interrupts(sc);
	vmbus_free_dma(sc);
	return -1;
}

static void
vmbus_attach_deferred(device_t self)
{
	struct vmbus_softc *sc = device_private(self);

	xc_wait(xc_broadcast(0, vmbus_init_synic, sc, NULL));
}

int
vmbus_detach(struct vmbus_softc *sc, int flags)
{

	vmbus_deinit_interrupts(sc);
	vmbus_free_dma(sc);

	return 0;
}

static int
vmbus_alloc_dma(struct vmbus_softc *sc)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	struct vmbus_percpu_data *pd;
	int i;

	/*
	 * Per-CPU messages and event flags.
	 */
	for (CPU_INFO_FOREACH(cii, ci)) {
		pd = &sc->sc_percpu[cpu_index(ci)];

		pd->simp = hyperv_dma_alloc(sc->sc_dmat, &pd->simp_dma,
		    PAGE_SIZE, PAGE_SIZE, 0, 1);
		if (pd->simp == NULL)
			return ENOMEM;

		pd->siep = hyperv_dma_alloc(sc->sc_dmat, &pd->siep_dma,
		    PAGE_SIZE, PAGE_SIZE, 0, 1);
		if (pd->siep == NULL)
			return ENOMEM;
	}

	sc->sc_events = hyperv_dma_alloc(sc->sc_dmat, &sc->sc_events_dma,
	    PAGE_SIZE, PAGE_SIZE, 0, 1);
	if (sc->sc_events == NULL)
		return ENOMEM;
	sc->sc_wevents = (u_long *)sc->sc_events;
	sc->sc_revents = (u_long *)((uint8_t *)sc->sc_events + (PAGE_SIZE / 2));

	for (i = 0; i < __arraycount(sc->sc_monitor); i++) {
		sc->sc_monitor[i] = hyperv_dma_alloc(sc->sc_dmat,
		    &sc->sc_monitor_dma[i], PAGE_SIZE, PAGE_SIZE, 0, 1);
		if (sc->sc_monitor[i] == NULL)
			return ENOMEM;
	}

	return 0;
}

static void
vmbus_free_dma(struct vmbus_softc *sc)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	int i;

	if (sc->sc_events != NULL) {
		sc->sc_events = sc->sc_wevents = sc->sc_revents = NULL;
		hyperv_dma_free(sc->sc_dmat, &sc->sc_events_dma);
	}

	for (i = 0; i < __arraycount(sc->sc_monitor); i++) {
		sc->sc_monitor[i] = NULL;
		hyperv_dma_free(sc->sc_dmat, &sc->sc_monitor_dma[i]);
	}

	for (CPU_INFO_FOREACH(cii, ci)) {
		struct vmbus_percpu_data *pd = &sc->sc_percpu[cpu_index(ci)];

		if (pd->simp != NULL) {
			pd->simp = NULL;
			hyperv_dma_free(sc->sc_dmat, &pd->simp_dma);
		}
		if (pd->siep != NULL) {
			pd->siep = NULL;
			hyperv_dma_free(sc->sc_dmat, &pd->siep_dma);
		}
	}
}

static int
vmbus_init_interrupts(struct vmbus_softc *sc)
{

	TAILQ_INIT(&sc->sc_reqs);
	mutex_init(&sc->sc_req_lock, MUTEX_DEFAULT, IPL_NET);

	TAILQ_INIT(&sc->sc_rsps);
	mutex_init(&sc->sc_rsp_lock, MUTEX_DEFAULT, IPL_NET);

	sc->sc_proto = VMBUS_VERSION_WS2008;

	/* XXX event_tq */

	sc->sc_msg_sih = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
	    vmbus_message_softintr, sc);
	if (sc->sc_msg_sih == NULL)
		return -1;

	vmbus_init_interrupts_md(sc);

	kcpuset_create(&sc->sc_intr_cpuset, true);
	if (cold) {
		/* Initialize other CPUs later. */
		vmbus_init_synic(sc, NULL);
	} else
		xc_wait(xc_broadcast(0, vmbus_init_synic, sc, NULL));
	atomic_or_32(&sc->sc_flags, VMBUS_SCFLAG_SYNIC);

	return 0;
}

static void
vmbus_deinit_interrupts(struct vmbus_softc *sc)
{

	if (ISSET(sc->sc_flags, VMBUS_SCFLAG_SYNIC)) {
		if (cold)
			vmbus_deinit_synic(sc, NULL);
		else
			xc_wait(xc_broadcast(0, vmbus_deinit_synic, sc, NULL));
		atomic_and_32(&sc->sc_flags, (uint32_t)~VMBUS_SCFLAG_SYNIC);
	}

	/* XXX event_tq */

	if (sc->sc_msg_sih != NULL) {
		softint_disestablish(sc->sc_msg_sih);
		sc->sc_msg_sih = NULL;
	}

	vmbus_deinit_interrupts_md(sc);
}

static void
vmbus_init_synic(void *arg1, void *arg2)
{
	struct vmbus_softc *sc = arg1;
	cpuid_t cpu;
	int s;

	s = splhigh();

	cpu = cpu_index(curcpu());
	if (!kcpuset_isset(sc->sc_intr_cpuset, cpu)) {
		kcpuset_atomic_set(sc->sc_intr_cpuset, cpu);
		vmbus_init_synic_md(sc, cpu);
	}

	splx(s);
}

static void
vmbus_deinit_synic(void *arg1, void *arg2)
{
	struct vmbus_softc *sc = arg1;
	cpuid_t cpu;
	int s;

	s = splhigh();

	cpu = cpu_index(curcpu());
	if (kcpuset_isset(sc->sc_intr_cpuset, cpu)) {
		vmbus_deinit_synic_md(sc, cpu);
		kcpuset_atomic_clear(sc->sc_intr_cpuset, cpu);
	}

	splx(s);
}

static int
vmbus_connect(struct vmbus_softc *sc)
{
	static const uint32_t versions[] = {
		VMBUS_VERSION_WIN8_1,
		VMBUS_VERSION_WIN8,
		VMBUS_VERSION_WIN7,
		VMBUS_VERSION_WS2008
	};
	struct vmbus_chanmsg_connect cmd;
	struct vmbus_chanmsg_connect_resp rsp;
	int i, rv;

	memset(&cmd, 0, sizeof(cmd));
	cmd.chm_hdr.chm_type = VMBUS_CHANMSG_CONNECT;
	cmd.chm_evtflags = hyperv_dma_get_paddr(&sc->sc_events_dma);
	cmd.chm_mnf1 = hyperv_dma_get_paddr(&sc->sc_monitor_dma[0]);
	cmd.chm_mnf2 = hyperv_dma_get_paddr(&sc->sc_monitor_dma[1]);

	memset(&rsp, 0, sizeof(rsp));

	for (i = 0; i < __arraycount(versions); i++) {
		cmd.chm_ver = versions[i];
		rv = vmbus_cmd(sc, &cmd, sizeof(cmd), &rsp, sizeof(rsp),
		    cold ? HCF_NOSLEEP : HCF_SLEEPOK);
		if (rv) {
			DPRINTF("%s: CONNECT failed\n",
			    device_xname(sc->sc_dev));
			return rv;
		}
		if (rsp.chm_done) {
			atomic_or_32(&sc->sc_flags, VMBUS_SCFLAG_CONNECTED);
			sc->sc_proto = versions[i];
			sc->sc_handle = VMBUS_GPADL_START;
			break;
		}
	}
	if (i == __arraycount(versions)) {
		device_printf(sc->sc_dev,
		    "failed to negotiate protocol version\n");
		return ENXIO;
	}

	return 0;
}

static int
vmbus_cmd(struct vmbus_softc *sc, void *cmd, size_t cmdlen, void *rsp,
    size_t rsplen, int flags)
{
	const int prflags = cold ? PR_NOWAIT : PR_WAITOK;
	struct vmbus_msg *msg;
	paddr_t pa;
	int rv;

	if (cmdlen > VMBUS_MSG_DSIZE_MAX) {
		device_printf(sc->sc_dev, "payload too large (%zu)\n",
		    cmdlen);
		return EMSGSIZE;
	}

	msg = pool_cache_get_paddr(sc->sc_msgpool, prflags, &pa);
	if (msg == NULL) {
		device_printf(sc->sc_dev, "couldn't get msgpool\n");
		return ENOMEM;
	}
	memset(msg, 0, sizeof(*msg));
	msg->msg_req.hc_dsize = cmdlen;
	memcpy(msg->msg_req.hc_data, cmd, cmdlen);

	if (!(flags & HCF_NOREPLY)) {
		msg->msg_rsp = rsp;
		msg->msg_rsplen = rsplen;
	} else
		msg->msg_flags |= MSGF_NOQUEUE;

	if (flags & HCF_NOSLEEP)
		msg->msg_flags |= MSGF_NOSLEEP;

	rv = vmbus_start(sc, msg, pa);
	if (rv == 0)
		rv = vmbus_reply(sc, msg);
	pool_cache_put_paddr(sc->sc_msgpool, msg, pa);
	return rv;
}

static int
vmbus_start(struct vmbus_softc *sc, struct vmbus_msg *msg, paddr_t msg_pa)
{
	static const int delays[] = {
		100, 100, 100, 500, 500, 5000, 5000, 5000
	};
	const char *wchan = "hvstart";
	uint16_t status;
	int i, s;

	msg->msg_req.hc_connid = VMBUS_CONNID_MESSAGE;
	msg->msg_req.hc_msgtype = 1;

	if (!(msg->msg_flags & MSGF_NOQUEUE)) {
		mutex_enter(&sc->sc_req_lock);
		TAILQ_INSERT_TAIL(&sc->sc_reqs, msg, msg_entry);
		mutex_exit(&sc->sc_req_lock);
	}

	for (i = 0; i < __arraycount(delays); i++) {
		status = hyperv_hypercall_post_message(
		    msg_pa + offsetof(struct vmbus_msg, msg_req));
		if (status == HYPERCALL_STATUS_SUCCESS)
			break;

		if (msg->msg_flags & MSGF_NOSLEEP) {
			delay(delays[i]);
			s = splnet();
			hyperv_intr();
			splx(s);
		} else
			tsleep(wchan, PRIBIO, wchan, 1);
	}
	if (status != HYPERCALL_STATUS_SUCCESS) {
		device_printf(sc->sc_dev,
		    "posting vmbus message failed with %d\n", status);
		if (!(msg->msg_flags & MSGF_NOQUEUE)) {
			mutex_enter(&sc->sc_req_lock);
			TAILQ_REMOVE(&sc->sc_reqs, msg, msg_entry);
			mutex_exit(&sc->sc_req_lock);
		}
		return EIO;
	}

	return 0;
}

static int
vmbus_reply_done(struct vmbus_softc *sc, struct vmbus_msg *msg)
{
	struct vmbus_msg *m;

	mutex_enter(&sc->sc_rsp_lock);
	TAILQ_FOREACH(m, &sc->sc_rsps, msg_entry) {
		if (m == msg) {
			mutex_exit(&sc->sc_rsp_lock);
			return 1;
		}
	}
	mutex_exit(&sc->sc_rsp_lock);
	return 0;
}

static int
vmbus_reply(struct vmbus_softc *sc, struct vmbus_msg *msg)
{

	if (msg->msg_flags & MSGF_NOQUEUE)
		return 0;

	vmbus_wait(sc, vmbus_reply_done, msg, msg, "hvreply");

	mutex_enter(&sc->sc_rsp_lock);
	TAILQ_REMOVE(&sc->sc_rsps, msg, msg_entry);
	mutex_exit(&sc->sc_rsp_lock);

	return 0;
}

static void
vmbus_wait(struct vmbus_softc *sc,
    int (*cond)(struct vmbus_softc *, struct vmbus_msg *),
    struct vmbus_msg *msg, void *wchan, const char *wmsg)
{
	int s;

	while (!cond(sc, msg)) {
		if (msg->msg_flags & MSGF_NOSLEEP) {
			delay(1000);
			s = splnet();
			hyperv_intr();
			splx(s);
		} else
			tsleep(wchan, PRIBIO, wmsg ? wmsg : "hvwait", 1);
	}
}

static uint16_t
vmbus_intr_signal(struct vmbus_softc *sc, paddr_t con_pa)
{
	uint64_t status;

	status = hyperv_hypercall_signal_event(con_pa);
	return (uint16_t)status;
}

#if LONG_BIT == 64
#define ffsl(v)	ffs64(v)
#elif LONG_BIT == 32
#define ffsl(v)	ffs32(v)
#else
#error unsupport LONG_BIT
#endif	/* LONG_BIT */

static void
vmbus_event_flags_proc(struct vmbus_softc *sc, volatile u_long *revents,
    int maxrow)
{
	struct vmbus_channel *ch;
	u_long pending;
	uint32_t chanid, chanid_base;
	int row, chanid_ofs;

	for (row = 0; row < maxrow; row++) {
		if (revents[row] == 0)
			continue;

		pending = atomic_swap_ulong(&revents[row], 0);
		chanid_base = row * LONG_BIT;

		while ((chanid_ofs = ffsl(pending)) != 0) {
			chanid_ofs--;	/* NOTE: ffs is 1-based */
			pending &= ~(1UL << chanid_ofs);

			chanid = chanid_base + chanid_ofs;
			/* vmbus channel protocol message */
			if (chanid == 0)
				continue;

			ch = vmbus_channel_lookup(sc, chanid);
			if (ch == NULL) {
				device_printf(sc->sc_dev,
				    "unhandled event on %d\n", chanid);
				continue;
			}
			if (ch->ch_state != VMBUS_CHANSTATE_OPENED) {
				device_printf(sc->sc_dev,
				    "channel %d is not active\n", chanid);
				continue;
			}
			ch->ch_evcnt.ev_count++;
			vmbus_channel_schedule(ch);
		}
	}
}

static void
vmbus_event_proc(void *arg, struct cpu_info *ci)
{
	struct vmbus_softc *sc = arg;
	struct vmbus_evtflags *evt;

	/*
	 * On Host with Win8 or above, the event page can be
	 * checked directly to get the id of the channel
	 * that has the pending interrupt.
	 */
	evt = (struct vmbus_evtflags *)sc->sc_percpu[cpu_index(ci)].siep +
	    VMBUS_SINT_MESSAGE;

	vmbus_event_flags_proc(sc, evt->evt_flags,
	    __arraycount(evt->evt_flags));
}

static void
vmbus_event_proc_compat(void *arg, struct cpu_info *ci)
{
	struct vmbus_softc *sc = arg;
	struct vmbus_evtflags *evt;

	evt = (struct vmbus_evtflags *)sc->sc_percpu[cpu_index(ci)].siep +
	    VMBUS_SINT_MESSAGE;

	if (test_bit(0, &evt->evt_flags[0])) {
		clear_bit(0, &evt->evt_flags[0]);
		/*
		 * receive size is 1/2 page and divide that by 4 bytes
		 */
		vmbus_event_flags_proc(sc, sc->sc_revents,
		    VMBUS_CHAN_MAX_COMPAT / VMBUS_EVTFLAG_LEN);
	}
}

static void
vmbus_message_proc(void *arg, struct cpu_info *ci)
{
	struct vmbus_softc *sc = arg;
	struct vmbus_message *msg;

	msg = (struct vmbus_message *)sc->sc_percpu[cpu_index(ci)].simp +
	    VMBUS_SINT_MESSAGE;
	if (__predict_false(msg->msg_type != HYPERV_MSGTYPE_NONE)) {
		if (__predict_true(!cold))
			softint_schedule_cpu(sc->sc_msg_sih, ci);
		else
			vmbus_message_softintr(sc);
	}
}

static void
vmbus_message_softintr(void *arg)
{
	struct vmbus_softc *sc = arg;
	struct vmbus_message *msg;
	struct vmbus_chanmsg_hdr *hdr;
	uint32_t type;
	cpuid_t cpu;

	cpu = cpu_index(curcpu());

	for (;;) {
		msg = (struct vmbus_message *)sc->sc_percpu[cpu].simp +
		    VMBUS_SINT_MESSAGE;
		if (msg->msg_type == HYPERV_MSGTYPE_NONE)
			break;

		hdr = (struct vmbus_chanmsg_hdr *)msg->msg_data;
		type = hdr->chm_type;
		if (type >= VMBUS_CHANMSG_COUNT) {
			device_printf(sc->sc_dev,
			    "unhandled message type %u flags %#x\n", type,
			    msg->msg_flags);
		} else {
			if (vmbus_msg_dispatch[type].hmd_handler) {
				vmbus_msg_dispatch[type].hmd_handler(sc, hdr);
			} else {
				device_printf(sc->sc_dev,
				    "unhandled message type %u\n", type);
			}
		}

		msg->msg_type = HYPERV_MSGTYPE_NONE;
		membar_sync();
		if (msg->msg_flags & VMBUS_MSGFLAG_PENDING)
			hyperv_send_eom();
	}
}

static void
vmbus_channel_response(struct vmbus_softc *sc, struct vmbus_chanmsg_hdr *rsphdr)
{
	struct vmbus_msg *msg;
	struct vmbus_chanmsg_hdr *reqhdr;
	int req;

	req = vmbus_msg_dispatch[rsphdr->chm_type].hmd_request;
	mutex_enter(&sc->sc_req_lock);
	TAILQ_FOREACH(msg, &sc->sc_reqs, msg_entry) {
		reqhdr = (struct vmbus_chanmsg_hdr *)&msg->msg_req.hc_data;
		if (reqhdr->chm_type == req) {
			TAILQ_REMOVE(&sc->sc_reqs, msg, msg_entry);
			break;
		}
	}
	mutex_exit(&sc->sc_req_lock);
	if (msg != NULL) {
		memcpy(msg->msg_rsp, rsphdr, msg->msg_rsplen);
		mutex_enter(&sc->sc_rsp_lock);
		TAILQ_INSERT_TAIL(&sc->sc_rsps, msg, msg_entry);
		mutex_exit(&sc->sc_rsp_lock);
		wakeup(msg);
	}
}

static void
vmbus_channel_offer(struct vmbus_softc *sc, struct vmbus_chanmsg_hdr *hdr)
{
	struct vmbus_offer *co;

	co = kmem_intr_zalloc(sizeof(*co), KM_NOSLEEP);
	if (co == NULL) {
		device_printf(sc->sc_dev, "couldn't allocate offer\n");
		return;
	}

	memcpy(&co->co_chan, hdr, sizeof(co->co_chan));

	mutex_enter(&sc->sc_offer_lock);
	SIMPLEQ_INSERT_TAIL(&sc->sc_offers, co, co_entry);
	mutex_exit(&sc->sc_offer_lock);
}

static void
vmbus_channel_rescind(struct vmbus_softc *sc, struct vmbus_chanmsg_hdr *hdr)
{
	const struct vmbus_chanmsg_chrescind *cmd;

	cmd = (const struct vmbus_chanmsg_chrescind *)hdr;
	device_printf(sc->sc_dev, "revoking channel %u\n", cmd->chm_chanid);
}

static void
vmbus_channel_delivered(struct vmbus_softc *sc, struct vmbus_chanmsg_hdr *hdr)
{

	atomic_or_32(&sc->sc_flags, VMBUS_SCFLAG_OFFERS_DELIVERED);
	wakeup(&sc->sc_offers);
}

static void
hyperv_guid_sprint(struct hyperv_guid *guid, char *str, size_t size)
{
	static const struct {
		const struct hyperv_guid *guid;
		const char *ident;
	} map[] = {
		{ &hyperv_guid_network,		"network" },
		{ &hyperv_guid_ide,		"ide" },
		{ &hyperv_guid_scsi,		"scsi" },
		{ &hyperv_guid_shutdown,	"shutdown" },
		{ &hyperv_guid_timesync,	"timesync" },
		{ &hyperv_guid_heartbeat,	"heartbeat" },
		{ &hyperv_guid_kvp,		"kvp" },
		{ &hyperv_guid_vss,		"vss" },
		{ &hyperv_guid_dynmem,		"dynamic-memory" },
		{ &hyperv_guid_mouse,		"mouse" },
		{ &hyperv_guid_kbd,		"keyboard" },
		{ &hyperv_guid_video,		"video" },
		{ &hyperv_guid_fc,		"fiber-channel" },
		{ &hyperv_guid_fcopy,		"file-copy" },
		{ &hyperv_guid_pcie,		"pcie-passthrough" },
		{ &hyperv_guid_netdir,		"network-direct" },
		{ &hyperv_guid_rdesktop,	"remote-desktop" },
		{ &hyperv_guid_avma1,		"avma-1" },
		{ &hyperv_guid_avma2,		"avma-2" },
		{ &hyperv_guid_avma3,		"avma-3" },
		{ &hyperv_guid_avma4,		"avma-4" },
	};
	int i;

	for (i = 0; i < __arraycount(map); i++) {
		if (memcmp(guid, map[i].guid, sizeof(*guid)) == 0) {
			strlcpy(str, map[i].ident, size);
			return;
		}
	}
	hyperv_guid2str(guid, str, size);
}

static int
vmbus_channel_scan_done(struct vmbus_softc *sc, struct vmbus_msg *msg __unused)
{

	return ISSET(sc->sc_flags, VMBUS_SCFLAG_OFFERS_DELIVERED);
}

static int
vmbus_channel_scan(struct vmbus_softc *sc)
{
	struct vmbus_chanmsg_hdr hdr;
	struct vmbus_chanmsg_choffer rsp;
	struct vmbus_offer *co;

	SIMPLEQ_INIT(&sc->sc_offers);
	mutex_init(&sc->sc_offer_lock, MUTEX_DEFAULT, IPL_NET);

	memset(&hdr, 0, sizeof(hdr));
	hdr.chm_type = VMBUS_CHANMSG_CHREQUEST;

	if (vmbus_cmd(sc, &hdr, sizeof(hdr), &rsp, sizeof(rsp),
	    HCF_NOREPLY | (cold ? HCF_NOSLEEP : HCF_SLEEPOK))) {
		DPRINTF("%s: CHREQUEST failed\n", device_xname(sc->sc_dev));
		return -1;
	}

	vmbus_wait(sc, vmbus_channel_scan_done, (struct vmbus_msg *)&hdr,
	    &sc->sc_offers, "hvscan");

	TAILQ_INIT(&sc->sc_channels);
	mutex_init(&sc->sc_channel_lock, MUTEX_DEFAULT, IPL_NET);

	mutex_enter(&sc->sc_offer_lock);
	while (!SIMPLEQ_EMPTY(&sc->sc_offers)) {
		co = SIMPLEQ_FIRST(&sc->sc_offers);
		SIMPLEQ_REMOVE_HEAD(&sc->sc_offers, co_entry);
		mutex_exit(&sc->sc_offer_lock);

		vmbus_process_offer(sc, co);
		kmem_free(co, sizeof(*co));

		mutex_enter(&sc->sc_offer_lock);
	}
	mutex_exit(&sc->sc_offer_lock);

	return 0;
}

static struct vmbus_channel *
vmbus_channel_alloc(struct vmbus_softc *sc)
{
	struct vmbus_channel *ch;

	ch = kmem_zalloc(sizeof(*ch), cold ? KM_NOSLEEP : KM_SLEEP);

	ch->ch_monprm = hyperv_dma_alloc(sc->sc_dmat, &ch->ch_monprm_dma,
	    sizeof(*ch->ch_monprm), 8, 0, 1);
	if (ch->ch_monprm == NULL) {
		device_printf(sc->sc_dev, "monprm alloc failed\n");
		kmem_free(ch, sizeof(*ch));
		return NULL;
	}
	memset(ch->ch_monprm, 0, sizeof(*ch->ch_monprm));

	ch->ch_refs = 1;
	ch->ch_sc = sc;
	mutex_init(&ch->ch_subchannel_lock, MUTEX_DEFAULT, IPL_NET);
	TAILQ_INIT(&ch->ch_subchannels);

	ch->ch_state = VMBUS_CHANSTATE_CLOSED;

	return ch;
}

static void
vmbus_channel_free(struct vmbus_channel *ch)
{
	struct vmbus_softc *sc = ch->ch_sc;

	KASSERTMSG(TAILQ_EMPTY(&ch->ch_subchannels) &&
	    ch->ch_subchannel_count == 0, "still owns sub-channels");
	KASSERTMSG(ch->ch_state == 0 || ch->ch_state == VMBUS_CHANSTATE_CLOSED,
	    "free busy channel");
	KASSERTMSG(ch->ch_refs == 0, "channel %u: invalid refcnt %d",
	    ch->ch_id, ch->ch_refs);

	hyperv_dma_free(sc->sc_dmat, &ch->ch_monprm_dma);
	mutex_destroy(&ch->ch_subchannel_lock);
	/* XXX ch_evcnt */
	softint_disestablish(ch->ch_taskq);
	kmem_free(ch, sizeof(*ch));
}

static int
vmbus_channel_add(struct vmbus_channel *nch)
{
	struct vmbus_softc *sc = nch->ch_sc;
	struct vmbus_channel *ch;
	u_int refs __diagused;

	if (nch->ch_id == 0) {
		device_printf(sc->sc_dev, "got channel 0 offer, discard\n");
		return EINVAL;
	} else if (nch->ch_id >= sc->sc_channel_max) {
		device_printf(sc->sc_dev, "invalid channel %u offer\n",
		    nch->ch_id);
		return EINVAL;
	}

	mutex_enter(&sc->sc_channel_lock);
	TAILQ_FOREACH(ch, &sc->sc_channels, ch_entry) {
		if (!memcmp(&ch->ch_type, &nch->ch_type, sizeof(ch->ch_type)) &&
		    !memcmp(&ch->ch_inst, &nch->ch_inst, sizeof(ch->ch_inst)))
			break;
	}
	if (VMBUS_CHAN_ISPRIMARY(nch)) {
		if (ch == NULL) {
			TAILQ_INSERT_TAIL(&sc->sc_channels, nch, ch_entry);
			mutex_exit(&sc->sc_channel_lock);
			goto done;
		} else {
			mutex_exit(&sc->sc_channel_lock);
			device_printf(sc->sc_dev,
			    "duplicated primary channel%u\n", nch->ch_id);
			return EINVAL;
		}
	} else {
		if (ch == NULL) {
			mutex_exit(&sc->sc_channel_lock);
			device_printf(sc->sc_dev, "no primary channel%u\n",
			    nch->ch_id);
			return EINVAL;
		}
	}
	mutex_exit(&sc->sc_channel_lock);

	KASSERT(!VMBUS_CHAN_ISPRIMARY(nch));
	KASSERT(ch != NULL);

	refs = atomic_add_int_nv(&nch->ch_refs, 1);
	KASSERT(refs == 1);

	nch->ch_primary_channel = ch;
	nch->ch_dev = ch->ch_dev;

	mutex_enter(&ch->ch_subchannel_lock);
	TAILQ_INSERT_TAIL(&ch->ch_subchannels, nch, ch_subentry);
	ch->ch_subchannel_count++;
	mutex_exit(&ch->ch_subchannel_lock);
	wakeup(ch);

done:
	vmbus_channel_cpu_default(nch);

	return 0;
}

void
vmbus_channel_cpu_set(struct vmbus_channel *ch, int cpu)
{
	struct vmbus_softc *sc = ch->ch_sc;

	KASSERTMSG(cpu >= 0 && cpu < ncpu, "invalid cpu %d", cpu);

	if (sc->sc_proto == VMBUS_VERSION_WS2008 ||
	    sc->sc_proto == VMBUS_VERSION_WIN7) {
		/* Only cpu0 is supported */
		cpu = 0;
	}

	ch->ch_cpuid = cpu;
	ch->ch_vcpu = sc->sc_percpu[cpu].vcpuid;
}

void
vmbus_channel_cpu_rr(struct vmbus_channel *ch)
{
	static uint32_t vmbus_channel_nextcpu;
	int cpu;

	cpu = atomic_add_32_nv(&vmbus_channel_nextcpu, 1) % ncpu;
	vmbus_channel_cpu_set(ch, cpu);
}

static void
vmbus_channel_cpu_default(struct vmbus_channel *ch)
{

        /*
	 * By default, pin the channel to cpu0.  Devices having
	 * special channel-cpu mapping requirement should call
	 * vmbus_channel_cpu_{set,rr}().
	 */
	vmbus_channel_cpu_set(ch, 0);
}

static void
vmbus_process_offer(struct vmbus_softc *sc, struct vmbus_offer *co)
{
	struct vmbus_channel *ch;

	ch = vmbus_channel_alloc(sc);
	if (ch == NULL) {
		device_printf(sc->sc_dev, "allocate channel %u failed\n",
		    co->co_chan.chm_chanid);
		return;
	}

	/*
	 * By default we setup state to enable batched reading.
	 * A specific service can choose to disable this prior
	 * to opening the channel.
	 */
	ch->ch_flags |= CHF_BATCHED;

	hyperv_guid_sprint(&co->co_chan.chm_chtype, ch->ch_ident,
	    sizeof(ch->ch_ident));

	ch->ch_monprm->mp_connid = VMBUS_CONNID_EVENT;
	if (sc->sc_proto > VMBUS_VERSION_WS2008)
		ch->ch_monprm->mp_connid = co->co_chan.chm_connid;

	if (co->co_chan.chm_flags1 & VMBUS_CHOFFER_FLAG1_HASMNF) {
		ch->ch_mgroup = co->co_chan.chm_montrig / VMBUS_MONTRIG_LEN;
		ch->ch_mindex = co->co_chan.chm_montrig % VMBUS_MONTRIG_LEN;
		ch->ch_flags |= CHF_MONITOR;
	}

	ch->ch_id = co->co_chan.chm_chanid;
	ch->ch_subidx = co->co_chan.chm_subidx;

	memcpy(&ch->ch_type, &co->co_chan.chm_chtype, sizeof(ch->ch_type));
	memcpy(&ch->ch_inst, &co->co_chan.chm_chinst, sizeof(ch->ch_inst));

	if (VMBUS_CHAN_ISPRIMARY(ch)) {
		/* set primary channel mgmt wq */
	} else {
		/* set sub channel mgmt wq */
	}

	if (vmbus_channel_add(ch) != 0) {
		vmbus_channel_free(ch);
		return;
	}

	ch->ch_state = VMBUS_CHANSTATE_OFFERED;

#ifdef HYPERV_DEBUG
	printf("%s: channel %u: \"%s\"", device_xname(sc->sc_dev), ch->ch_id,
	    ch->ch_ident);
	if (ch->ch_flags & CHF_MONITOR)
		printf(", monitor %u\n", co->co_chan.chm_montrig);
	else
		printf("\n");
#endif
}

static int
vmbus_channel_release(struct vmbus_channel *ch)
{
	struct vmbus_softc *sc = ch->ch_sc;
	struct vmbus_chanmsg_chfree cmd;
	int rv;

	memset(&cmd, 0, sizeof(cmd));
	cmd.chm_hdr.chm_type = VMBUS_CHANMSG_CHFREE;
	cmd.chm_chanid = ch->ch_id;

	rv = vmbus_cmd(sc, &cmd, sizeof(cmd), NULL, 0,
	    HCF_NOREPLY | (cold ? HCF_NOSLEEP : HCF_SLEEPOK));
	if (rv) {
		DPRINTF("%s: CHFREE failed with %d\n", device_xname(sc->sc_dev),
		    rv);
	}
	return rv;
}

struct vmbus_channel **
vmbus_subchannel_get(struct vmbus_channel *prich, int cnt)
{
	struct vmbus_channel **ret, *ch;
	int i;

	KASSERT(cnt > 0);

	ret = kmem_alloc(sizeof(struct vmbus_channel *) * cnt,
	    cold ? KM_NOSLEEP : KM_SLEEP);

	mutex_enter(&prich->ch_subchannel_lock);

	while (prich->ch_subchannel_count < cnt)
		/* XXX use condvar(9) instead of mtsleep */
		mtsleep(prich, PRIBIO, "hvvmsubch", 0,
		    &prich->ch_subchannel_lock);

	i = 0;
	TAILQ_FOREACH(ch, &prich->ch_subchannels, ch_subentry) {
		ret[i] = ch;	/* XXX inc refs */

		if (++i == cnt)
			break;
	}

	mutex_exit(&prich->ch_subchannel_lock);

	return ret;
}

void
vmbus_subchannel_put(struct vmbus_channel **subch, int cnt)
{

	kmem_free(subch, sizeof(struct vmbus_channel *) * cnt);
}

static struct vmbus_channel *
vmbus_channel_lookup(struct vmbus_softc *sc, uint32_t relid)
{
	struct vmbus_channel *ch;

	TAILQ_FOREACH(ch, &sc->sc_channels, ch_entry) {
		if (ch->ch_id == relid)
			return ch;
	}
	return NULL;
}

static int
vmbus_channel_ring_create(struct vmbus_channel *ch, uint32_t buflen)
{
	struct vmbus_softc *sc = ch->ch_sc;

	buflen = roundup(buflen, PAGE_SIZE) + sizeof(struct vmbus_bufring);
	ch->ch_ring_size = 2 * buflen;
	ch->ch_ring = hyperv_dma_alloc(sc->sc_dmat, &ch->ch_ring_dma,
	    ch->ch_ring_size, PAGE_SIZE, 0, 1);	/* page aligned memory */
	if (ch->ch_ring == NULL) {
		device_printf(sc->sc_dev,
		    "failed to allocate channel ring\n");
		return ENOMEM;
	}

	memset(&ch->ch_wrd, 0, sizeof(ch->ch_wrd));
	ch->ch_wrd.rd_ring = (struct vmbus_bufring *)ch->ch_ring;
	ch->ch_wrd.rd_size = buflen;
	ch->ch_wrd.rd_dsize = buflen - sizeof(struct vmbus_bufring);
	mutex_init(&ch->ch_wrd.rd_lock, MUTEX_DEFAULT, IPL_NET);

	memset(&ch->ch_rrd, 0, sizeof(ch->ch_rrd));
	ch->ch_rrd.rd_ring = (struct vmbus_bufring *)((uint8_t *)ch->ch_ring +
	    buflen);
	ch->ch_rrd.rd_size = buflen;
	ch->ch_rrd.rd_dsize = buflen - sizeof(struct vmbus_bufring);
	mutex_init(&ch->ch_rrd.rd_lock, MUTEX_DEFAULT, IPL_NET);

	if (vmbus_handle_alloc(ch, &ch->ch_ring_dma, ch->ch_ring_size,
	    &ch->ch_ring_gpadl)) {
		device_printf(sc->sc_dev,
		    "failed to obtain a PA handle for the ring\n");
		vmbus_channel_ring_destroy(ch);
		return ENOMEM;
	}

	return 0;
}

static void
vmbus_channel_ring_destroy(struct vmbus_channel *ch)
{
	struct vmbus_softc *sc = ch->ch_sc;

	hyperv_dma_free(sc->sc_dmat, &ch->ch_ring_dma);
	ch->ch_ring = NULL;
	vmbus_handle_free(ch, ch->ch_ring_gpadl);

	mutex_destroy(&ch->ch_wrd.rd_lock);
	memset(&ch->ch_wrd, 0, sizeof(ch->ch_wrd));
	mutex_destroy(&ch->ch_rrd.rd_lock);
	memset(&ch->ch_rrd, 0, sizeof(ch->ch_rrd));
}

int
vmbus_channel_open(struct vmbus_channel *ch, size_t buflen, void *udata,
    size_t udatalen, void (*handler)(void *), void *arg)
{
	struct vmbus_softc *sc = ch->ch_sc;
	struct vmbus_chanmsg_chopen cmd;
	struct vmbus_chanmsg_chopen_resp rsp;
	int rv = EINVAL;

	if (ch->ch_ring == NULL &&
	    (rv = vmbus_channel_ring_create(ch, buflen))) {
		DPRINTF("%s: failed to create channel ring\n",
		    device_xname(sc->sc_dev));
		return rv;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.chm_hdr.chm_type = VMBUS_CHANMSG_CHOPEN;
	cmd.chm_openid = ch->ch_id;
	cmd.chm_chanid = ch->ch_id;
	cmd.chm_gpadl = ch->ch_ring_gpadl;
	cmd.chm_txbr_pgcnt = atop(ch->ch_wrd.rd_size);
	cmd.chm_vcpuid = ch->ch_vcpu;
	if (udata && udatalen > 0)
		memcpy(cmd.chm_udata, udata, udatalen);

	memset(&rsp, 0, sizeof(rsp));

	ch->ch_handler = handler;
	ch->ch_ctx = arg;
	ch->ch_state = VMBUS_CHANSTATE_OPENED;

	rv = vmbus_cmd(sc, &cmd, sizeof(cmd), &rsp, sizeof(rsp),
	    cold ? HCF_NOSLEEP : HCF_SLEEPOK);
	if (rv) {
		vmbus_channel_ring_destroy(ch);
		DPRINTF("%s: CHOPEN failed with %d\n", device_xname(sc->sc_dev),
		    rv);
		ch->ch_handler = NULL;
		ch->ch_ctx = NULL;
		ch->ch_state = VMBUS_CHANSTATE_OFFERED;
		return rv;
	}
	return 0;
}

static void
vmbus_channel_detach(struct vmbus_channel *ch)
{
	u_int refs;

	refs = atomic_add_int_nv(&ch->ch_refs, -1);
	if (refs == 1) {
		/* XXX on workqueue? */
		if (VMBUS_CHAN_ISPRIMARY(ch)) {
			vmbus_channel_release(ch);
			vmbus_channel_free(ch);
		} else {
			struct vmbus_channel *prich = ch->ch_primary_channel;

			vmbus_channel_release(ch);

			mutex_enter(&prich->ch_subchannel_lock);
			TAILQ_REMOVE(&prich->ch_subchannels, ch, ch_subentry);
			prich->ch_subchannel_count--;
			mutex_exit(&prich->ch_subchannel_lock);
			wakeup(prich);

			vmbus_channel_free(ch);
		}
	}
}

static int
vmbus_channel_close_internal(struct vmbus_channel *ch)
{
	struct vmbus_softc *sc = ch->ch_sc;
	struct vmbus_chanmsg_chclose cmd;
	int rv;

	memset(&cmd, 0, sizeof(cmd));
	cmd.chm_hdr.chm_type = VMBUS_CHANMSG_CHCLOSE;
	cmd.chm_chanid = ch->ch_id;

	ch->ch_state = VMBUS_CHANSTATE_CLOSING;
	rv = vmbus_cmd(sc, &cmd, sizeof(cmd), NULL, 0,
	    HCF_NOREPLY | (cold ? HCF_NOSLEEP : HCF_SLEEPOK));
	if (rv) {
		DPRINTF("%s: CHCLOSE failed with %d\n",
		    device_xname(sc->sc_dev), rv);
		return rv;
	}
	ch->ch_state = VMBUS_CHANSTATE_CLOSED;
	vmbus_channel_ring_destroy(ch);
	return 0;
}

int
vmbus_channel_close_direct(struct vmbus_channel *ch)
{
	int rv;

	rv = vmbus_channel_close_internal(ch);
	if (!VMBUS_CHAN_ISPRIMARY(ch))
		vmbus_channel_detach(ch);
	return rv;
}

int
vmbus_channel_close(struct vmbus_channel *ch)
{
	struct vmbus_channel **subch;
	int i, cnt, rv;

	if (!VMBUS_CHAN_ISPRIMARY(ch))
		return 0;

	cnt = ch->ch_subchannel_count;
	if (cnt > 0) {
		subch = vmbus_subchannel_get(ch, cnt);
		for (i = 0; i < ch->ch_subchannel_count; i++) {
			rv = vmbus_channel_close_internal(subch[i]);
			(void) rv;	/* XXX */
			vmbus_channel_detach(ch);
		}
		vmbus_subchannel_put(subch, cnt);
	}

	return vmbus_channel_close_internal(ch);
}

static inline void
vmbus_channel_setevent(struct vmbus_softc *sc, struct vmbus_channel *ch)
{
	struct vmbus_mon_trig *mtg;

	/* Each uint32_t represents 32 channels */
	set_bit(ch->ch_id, sc->sc_wevents);
	if (ch->ch_flags & CHF_MONITOR) {
		mtg = &sc->sc_monitor[1]->mnf_trigs[ch->ch_mgroup];
		set_bit(ch->ch_mindex, &mtg->mt_pending);
	} else
		vmbus_intr_signal(sc, hyperv_dma_get_paddr(&ch->ch_monprm_dma));
}

static void
vmbus_channel_intr(void *arg)
{
	struct vmbus_channel *ch = arg;

	if (vmbus_channel_ready(ch))
		ch->ch_handler(ch->ch_ctx);

	if (vmbus_channel_unpause(ch) == 0)
		return;

	vmbus_channel_pause(ch);
	vmbus_channel_schedule(ch);
}

int
vmbus_channel_setdeferred(struct vmbus_channel *ch, const char *name)
{

	ch->ch_taskq = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
	    vmbus_channel_intr, ch);
	if (ch->ch_taskq == NULL)
		return -1;
	return 0;
}

void
vmbus_channel_schedule(struct vmbus_channel *ch)
{

	if (ch->ch_handler) {
		if (!cold && (ch->ch_flags & CHF_BATCHED)) {
			vmbus_channel_pause(ch);
			softint_schedule(ch->ch_taskq);
		} else
			ch->ch_handler(ch->ch_ctx);
	}
}

static __inline void
vmbus_ring_put(struct vmbus_ring_data *wrd, uint8_t *data, uint32_t datalen)
{
	int left = MIN(datalen, wrd->rd_dsize - wrd->rd_prod);

	memcpy(&wrd->rd_ring->br_data[wrd->rd_prod], data, left);
	memcpy(&wrd->rd_ring->br_data[0], data + left, datalen - left);
	wrd->rd_prod += datalen;
	if (wrd->rd_prod >= wrd->rd_dsize)
		wrd->rd_prod -= wrd->rd_dsize;
}

static inline void
vmbus_ring_get(struct vmbus_ring_data *rrd, uint8_t *data, uint32_t datalen,
    int peek)
{
	int left = MIN(datalen, rrd->rd_dsize - rrd->rd_cons);

	memcpy(data, &rrd->rd_ring->br_data[rrd->rd_cons], left);
	memcpy(data + left, &rrd->rd_ring->br_data[0], datalen - left);
	if (!peek) {
		rrd->rd_cons += datalen;
		if (rrd->rd_cons >= rrd->rd_dsize)
			rrd->rd_cons -= rrd->rd_dsize;
	}
}

static __inline void
vmbus_ring_avail(struct vmbus_ring_data *rd, uint32_t *towrite,
    uint32_t *toread)
{
	uint32_t ridx = rd->rd_ring->br_rindex;
	uint32_t widx = rd->rd_ring->br_windex;
	uint32_t r, w;

	if (widx >= ridx)
		w = rd->rd_dsize - (widx - ridx);
	else
		w = ridx - widx;
	r = rd->rd_dsize - w;
	if (towrite)
		*towrite = w;
	if (toread)
		*toread = r;
}

static int
vmbus_ring_write(struct vmbus_ring_data *wrd, struct iovec *iov, int iov_cnt,
    int *needsig)
{
	uint64_t indices = 0;
	uint32_t avail, oprod, datalen = sizeof(indices);
	int i;

	for (i = 0; i < iov_cnt; i++)
		datalen += iov[i].iov_len;

	KASSERT(datalen <= wrd->rd_dsize);

	vmbus_ring_avail(wrd, &avail, NULL);
	if (avail <= datalen) {
		DPRINTF("%s: avail %u datalen %u\n", __func__, avail, datalen);
		return EAGAIN;
	}

	oprod = wrd->rd_prod;

	for (i = 0; i < iov_cnt; i++)
		vmbus_ring_put(wrd, iov[i].iov_base, iov[i].iov_len);

	indices = (uint64_t)oprod << 32;
	vmbus_ring_put(wrd, (uint8_t *)&indices, sizeof(indices));

	membar_sync();
	wrd->rd_ring->br_windex = wrd->rd_prod;
	membar_sync();

	/* Signal when the ring transitions from being empty to non-empty */
	if (wrd->rd_ring->br_imask == 0 &&
	    wrd->rd_ring->br_rindex == oprod)
		*needsig = 1;
	else
		*needsig = 0;

	return 0;
}

int
vmbus_channel_send(struct vmbus_channel *ch, void *data, uint32_t datalen,
    uint64_t rid, int type, uint32_t flags)
{
	struct vmbus_softc *sc = ch->ch_sc;
	struct vmbus_chanpkt cp;
	struct iovec iov[3];
	uint32_t pktlen, pktlen_aligned;
	uint64_t zeropad = 0;
	int rv, needsig = 0;

	pktlen = sizeof(cp) + datalen;
	pktlen_aligned = roundup(pktlen, sizeof(uint64_t));

	cp.cp_hdr.cph_type = type;
	cp.cp_hdr.cph_flags = flags;
	VMBUS_CHANPKT_SETLEN(cp.cp_hdr.cph_hlen, sizeof(cp));
	VMBUS_CHANPKT_SETLEN(cp.cp_hdr.cph_tlen, pktlen_aligned);
	cp.cp_hdr.cph_tid = rid;

	iov[0].iov_base = &cp;
	iov[0].iov_len = sizeof(cp);

	iov[1].iov_base = data;
	iov[1].iov_len = datalen;

	iov[2].iov_base = &zeropad;
	iov[2].iov_len = pktlen_aligned - pktlen;

	mutex_enter(&ch->ch_wrd.rd_lock);
	rv = vmbus_ring_write(&ch->ch_wrd, iov, 3, &needsig);
	mutex_exit(&ch->ch_wrd.rd_lock);
	if (rv == 0 && needsig)
		vmbus_channel_setevent(sc, ch);

	return rv;
}

int
vmbus_channel_send_sgl(struct vmbus_channel *ch, struct vmbus_gpa *sgl,
    uint32_t nsge, void *data, uint32_t datalen, uint64_t rid)
{
	struct vmbus_softc *sc = ch->ch_sc;
	struct vmbus_chanpkt_sglist cp;
	struct iovec iov[4];
	uint32_t buflen, pktlen, pktlen_aligned;
	uint64_t zeropad = 0;
	int rv, needsig = 0;

	buflen = sizeof(struct vmbus_gpa) * nsge;
	pktlen = sizeof(cp) + datalen + buflen;
	pktlen_aligned = roundup(pktlen, sizeof(uint64_t));

	cp.cp_hdr.cph_type = VMBUS_CHANPKT_TYPE_GPA;
	cp.cp_hdr.cph_flags = VMBUS_CHANPKT_FLAG_RC;
	VMBUS_CHANPKT_SETLEN(cp.cp_hdr.cph_hlen, sizeof(cp) + buflen);
	VMBUS_CHANPKT_SETLEN(cp.cp_hdr.cph_tlen, pktlen_aligned);
	cp.cp_hdr.cph_tid = rid;
	cp.cp_gpa_cnt = nsge;
	cp.cp_rsvd = 0;

	iov[0].iov_base = &cp;
	iov[0].iov_len = sizeof(cp);

	iov[1].iov_base = sgl;
	iov[1].iov_len = buflen;

	iov[2].iov_base = data;
	iov[2].iov_len = datalen;

	iov[3].iov_base = &zeropad;
	iov[3].iov_len = pktlen_aligned - pktlen;

	mutex_enter(&ch->ch_wrd.rd_lock);
	rv = vmbus_ring_write(&ch->ch_wrd, iov, 4, &needsig);
	mutex_exit(&ch->ch_wrd.rd_lock);
	if (rv == 0 && needsig)
		vmbus_channel_setevent(sc, ch);

	return rv;
}

int
vmbus_channel_send_prpl(struct vmbus_channel *ch, struct vmbus_gpa_range *prpl,
    uint32_t nprp, void *data, uint32_t datalen, uint64_t rid)
{
	struct vmbus_softc *sc = ch->ch_sc;
	struct vmbus_chanpkt_prplist cp;
	struct iovec iov[4];
	uint32_t buflen, pktlen, pktlen_aligned;
	uint64_t zeropad = 0;
	int rv, needsig = 0;

	buflen = sizeof(struct vmbus_gpa_range) * (nprp + 1);
	pktlen = sizeof(cp) + datalen + buflen;
	pktlen_aligned = roundup(pktlen, sizeof(uint64_t));

	cp.cp_hdr.cph_type = VMBUS_CHANPKT_TYPE_GPA;
	cp.cp_hdr.cph_flags = VMBUS_CHANPKT_FLAG_RC;
	VMBUS_CHANPKT_SETLEN(cp.cp_hdr.cph_hlen, sizeof(cp) + buflen);
	VMBUS_CHANPKT_SETLEN(cp.cp_hdr.cph_tlen, pktlen_aligned);
	cp.cp_hdr.cph_tid = rid;
	cp.cp_range_cnt = 1;
	cp.cp_rsvd = 0;

	iov[0].iov_base = &cp;
	iov[0].iov_len = sizeof(cp);

	iov[1].iov_base = prpl;
	iov[1].iov_len = buflen;

	iov[2].iov_base = data;
	iov[2].iov_len = datalen;

	iov[3].iov_base = &zeropad;
	iov[3].iov_len = pktlen_aligned - pktlen;

	mutex_enter(&ch->ch_wrd.rd_lock);
	rv = vmbus_ring_write(&ch->ch_wrd, iov, 4, &needsig);
	mutex_exit(&ch->ch_wrd.rd_lock);
	if (rv == 0 && needsig)
		vmbus_channel_setevent(sc, ch);

	return rv;
}

static int
vmbus_ring_peek(struct vmbus_ring_data *rrd, void *data, uint32_t datalen)
{
	uint32_t avail;

	KASSERT(datalen <= rrd->rd_dsize);

	vmbus_ring_avail(rrd, NULL, &avail);
	if (avail < datalen)
		return EAGAIN;

	vmbus_ring_get(rrd, (uint8_t *)data, datalen, 1);
	return 0;
}

static int
vmbus_ring_read(struct vmbus_ring_data *rrd, void *data, uint32_t datalen,
    uint32_t offset)
{
	uint64_t indices;
	uint32_t avail;

	KASSERT(datalen <= rrd->rd_dsize);

	vmbus_ring_avail(rrd, NULL, &avail);
	if (avail < datalen) {
		DPRINTF("%s: avail %u datalen %u\n", __func__, avail, datalen);
		return EAGAIN;
	}

	if (offset) {
		rrd->rd_cons += offset;
		if (rrd->rd_cons >= rrd->rd_dsize)
			rrd->rd_cons -= rrd->rd_dsize;
	}

	vmbus_ring_get(rrd, (uint8_t *)data, datalen, 0);
	vmbus_ring_get(rrd, (uint8_t *)&indices, sizeof(indices), 0);

	membar_sync();
	rrd->rd_ring->br_rindex = rrd->rd_cons;

	return 0;
}

int
vmbus_channel_recv(struct vmbus_channel *ch, void *data, uint32_t datalen,
    uint32_t *rlen, uint64_t *rid, int raw)
{
	struct vmbus_softc *sc = ch->ch_sc;
	struct vmbus_chanpkt_hdr cph;
	uint32_t offset, pktlen;
	int rv;

	*rlen = 0;

	mutex_enter(&ch->ch_rrd.rd_lock);

	if ((rv = vmbus_ring_peek(&ch->ch_rrd, &cph, sizeof(cph))) != 0) {
		mutex_exit(&ch->ch_rrd.rd_lock);
		return rv;
	}

	offset = raw ? 0 : VMBUS_CHANPKT_GETLEN(cph.cph_hlen);
	pktlen = VMBUS_CHANPKT_GETLEN(cph.cph_tlen) - offset;
	if (pktlen > datalen) {
		mutex_exit(&ch->ch_rrd.rd_lock);
		device_printf(sc->sc_dev, "%s: pktlen %u datalen %u\n",
		    __func__, pktlen, datalen);
		return EINVAL;
	}

	rv = vmbus_ring_read(&ch->ch_rrd, data, pktlen, offset);
	if (rv == 0) {
		*rlen = pktlen;
		*rid = cph.cph_tid;
	}

	mutex_exit(&ch->ch_rrd.rd_lock);

	return rv;
}

static inline void
vmbus_ring_mask(struct vmbus_ring_data *rd)
{

	membar_sync();
	rd->rd_ring->br_imask = 1;
	membar_sync();
}

static inline void
vmbus_ring_unmask(struct vmbus_ring_data *rd)
{

	membar_sync();
	rd->rd_ring->br_imask = 0;
	membar_sync();
}

static void
vmbus_channel_pause(struct vmbus_channel *ch)
{

	vmbus_ring_mask(&ch->ch_rrd);
}

static uint32_t
vmbus_channel_unpause(struct vmbus_channel *ch)
{
	uint32_t avail;

	vmbus_ring_unmask(&ch->ch_rrd);
	vmbus_ring_avail(&ch->ch_rrd, NULL, &avail);

	return avail;
}

static uint32_t
vmbus_channel_ready(struct vmbus_channel *ch)
{
	uint32_t avail;

	vmbus_ring_avail(&ch->ch_rrd, NULL, &avail);

	return avail;
}

/* How many PFNs can be referenced by the header */
#define VMBUS_NPFNHDR	((VMBUS_MSG_DSIZE_MAX -	\
	  sizeof(struct vmbus_chanmsg_gpadl_conn)) / sizeof(uint64_t))

/* How many PFNs can be referenced by the body */
#define VMBUS_NPFNBODY	((VMBUS_MSG_DSIZE_MAX -	\
	  sizeof(struct vmbus_chanmsg_gpadl_subconn)) / sizeof(uint64_t))

int
vmbus_handle_alloc(struct vmbus_channel *ch, const struct hyperv_dma *dma,
    uint32_t buflen, uint32_t *handle)
{
	const int prflags = cold ? PR_NOWAIT : PR_WAITOK;
	const int kmemflags = cold ? KM_NOSLEEP : KM_SLEEP;
	const int msgflags = cold ? MSGF_NOSLEEP : 0;
	const int hcflags = cold ? HCF_NOSLEEP : HCF_SLEEPOK;
	struct vmbus_softc *sc = ch->ch_sc;
	struct vmbus_chanmsg_gpadl_conn *hdr;
	struct vmbus_chanmsg_gpadl_subconn *cmd;
	struct vmbus_chanmsg_gpadl_connresp rsp;
	struct vmbus_msg *msg;
	int i, j, last, left, rv;
	int bodylen = 0, ncmds = 0, pfn = 0;
	uint64_t *frames;
	paddr_t pa;
	uint8_t *body;
	/* Total number of pages to reference */
	int total = atop(buflen);
	/* Number of pages that will fit the header */
	int inhdr = MIN(total, VMBUS_NPFNHDR);

	KASSERT((buflen & PAGE_MASK) == 0);
	KASSERT(buflen == (uint32_t)dma->map->dm_mapsize);

	msg = pool_cache_get_paddr(sc->sc_msgpool, prflags, &pa);
	if (msg == NULL)
		return ENOMEM;

	/* Prepare array of frame addresses */
	frames = kmem_zalloc(total * sizeof(*frames), kmemflags);
	if (frames == NULL) {
		pool_cache_put_paddr(sc->sc_msgpool, msg, pa);
		return ENOMEM;
	}
	for (i = 0, j = 0; i < dma->map->dm_nsegs && j < total; i++) {
		bus_dma_segment_t *seg = &dma->map->dm_segs[i];
		bus_addr_t addr = seg->ds_addr;

		KASSERT((addr & PAGE_MASK) == 0);
		KASSERT((seg->ds_len & PAGE_MASK) == 0);

		while (addr < seg->ds_addr + seg->ds_len && j < total) {
			frames[j++] = atop(addr);
			addr += PAGE_SIZE;
		}
	}

	memset(msg, 0, sizeof(*msg));
	msg->msg_req.hc_dsize = sizeof(struct vmbus_chanmsg_gpadl_conn) +
	    inhdr * sizeof(uint64_t);
	hdr = (struct vmbus_chanmsg_gpadl_conn *)msg->msg_req.hc_data;
	msg->msg_rsp = &rsp;
	msg->msg_rsplen = sizeof(rsp);
	msg->msg_flags = msgflags;

	left = total - inhdr;

	/* Allocate additional gpadl_body structures if required */
	if (left > 0) {
		ncmds = MAX(1, left / VMBUS_NPFNBODY + left % VMBUS_NPFNBODY);
		bodylen = ncmds * VMBUS_MSG_DSIZE_MAX;
		body = kmem_zalloc(bodylen, kmemflags);
		if (body == NULL) {
			kmem_free(frames, total * sizeof(*frames));
			pool_cache_put_paddr(sc->sc_msgpool, msg, pa);
			return ENOMEM;
		}
	}

	*handle = atomic_add_int_nv(&sc->sc_handle, 1);

	hdr->chm_hdr.chm_type = VMBUS_CHANMSG_GPADL_CONN;
	hdr->chm_chanid = ch->ch_id;
	hdr->chm_gpadl = *handle;

	/* Single range for a contiguous buffer */
	hdr->chm_range_cnt = 1;
	hdr->chm_range_len = sizeof(struct vmbus_gpa_range) + total *
	    sizeof(uint64_t);
	hdr->chm_range.gpa_ofs = 0;
	hdr->chm_range.gpa_len = buflen;

	/* Fit as many pages as possible into the header */
	for (i = 0; i < inhdr; i++)
		hdr->chm_range.gpa_page[i] = frames[pfn++];

	for (i = 0; i < ncmds; i++) {
		cmd = (struct vmbus_chanmsg_gpadl_subconn *)(body +
		    VMBUS_MSG_DSIZE_MAX * i);
		cmd->chm_hdr.chm_type = VMBUS_CHANMSG_GPADL_SUBCONN;
		cmd->chm_gpadl = *handle;
		last = MIN(left, VMBUS_NPFNBODY);
		for (j = 0; j < last; j++)
			cmd->chm_gpa_page[j] = frames[pfn++];
		left -= last;
	}

	rv = vmbus_start(sc, msg, pa);
	if (rv != 0) {
		DPRINTF("%s: GPADL_CONN failed\n", device_xname(sc->sc_dev));
		goto out;
	}
	for (i = 0; i < ncmds; i++) {
		int cmdlen = sizeof(*cmd);
		cmd = (struct vmbus_chanmsg_gpadl_subconn *)(body +
		    VMBUS_MSG_DSIZE_MAX * i);
		/* Last element can be short */
		if (i == ncmds - 1)
			cmdlen += last * sizeof(uint64_t);
		else
			cmdlen += VMBUS_NPFNBODY * sizeof(uint64_t);
		rv = vmbus_cmd(sc, cmd, cmdlen, NULL, 0, HCF_NOREPLY | hcflags);
		if (rv != 0) {
			DPRINTF("%s: GPADL_SUBCONN (iteration %d/%d) failed "
			    "with %d\n", device_xname(sc->sc_dev), i, ncmds,
			    rv);
			goto out;
		}
	}
	rv = vmbus_reply(sc, msg);
	if (rv != 0) {
		DPRINTF("%s: GPADL allocation failed with %d\n",
		    device_xname(sc->sc_dev), rv);
	}

 out:
	if (bodylen > 0)
		kmem_free(body, bodylen);
	kmem_free(frames, total * sizeof(*frames));
	pool_cache_put_paddr(sc->sc_msgpool, msg, pa);
	if (rv)
		return rv;

	KASSERT(*handle == rsp.chm_gpadl);

	return 0;
}

void
vmbus_handle_free(struct vmbus_channel *ch, uint32_t handle)
{
	struct vmbus_softc *sc = ch->ch_sc;
	struct vmbus_chanmsg_gpadl_disconn cmd;
	struct vmbus_chanmsg_gpadl_disconn rsp;
	int rv;

	memset(&cmd, 0, sizeof(cmd));
	cmd.chm_hdr.chm_type = VMBUS_CHANMSG_GPADL_DISCONN;
	cmd.chm_chanid = ch->ch_id;
	cmd.chm_gpadl = handle;

	rv = vmbus_cmd(sc, &cmd, sizeof(cmd), &rsp, sizeof(rsp),
	    cold ? HCF_NOSLEEP : HCF_SLEEPOK);
	if (rv) {
		DPRINTF("%s: GPADL_DISCONN failed with %d\n",
		    device_xname(sc->sc_dev), rv);
	}
}

static int
vmbus_attach_print(void *aux, const char *name)
{
	struct vmbus_attach_args *aa = aux;

	if (name)
		printf("\"%s\" at %s", aa->aa_ident, name);

	return UNCONF;
}

static int
vmbus_attach_icdevs(struct vmbus_softc *sc)
{
	struct vmbus_dev *dv;
	struct vmbus_channel *ch;

	SLIST_INIT(&sc->sc_icdevs);
	mutex_init(&sc->sc_icdev_lock, MUTEX_DEFAULT, IPL_NET);

	TAILQ_FOREACH(ch, &sc->sc_channels, ch_entry) {
		if (ch->ch_state != VMBUS_CHANSTATE_OFFERED)
			continue;
		if (ch->ch_flags & CHF_MONITOR)
			continue;

		dv = kmem_zalloc(sizeof(*dv), cold ? KM_NOSLEEP : KM_SLEEP);
		if (dv == NULL) {
			device_printf(sc->sc_dev,
			    "failed to allocate ic device object\n");
			return ENOMEM;
		}
		dv->dv_aa.aa_type = &ch->ch_type;
		dv->dv_aa.aa_inst = &ch->ch_inst;
		dv->dv_aa.aa_ident = ch->ch_ident;
		dv->dv_aa.aa_chan = ch;
		mutex_enter(&sc->sc_icdev_lock);
		SLIST_INSERT_HEAD(&sc->sc_icdevs, dv, dv_entry);
		mutex_exit(&sc->sc_icdev_lock);
		ch->ch_dev = config_found_ia(sc->sc_dev, "hypervvmbus",
		    &dv->dv_aa, vmbus_attach_print);
	}
	return 0;
}

static int
vmbus_attach_devices(struct vmbus_softc *sc)
{
	struct vmbus_dev *dv;
	struct vmbus_channel *ch;

	SLIST_INIT(&sc->sc_devs);
	mutex_init(&sc->sc_dev_lock, MUTEX_DEFAULT, IPL_NET);

	TAILQ_FOREACH(ch, &sc->sc_channels, ch_entry) {
		if (ch->ch_state != VMBUS_CHANSTATE_OFFERED)
			continue;
		if (!(ch->ch_flags & CHF_MONITOR))
			continue;

		dv = kmem_zalloc(sizeof(*dv), cold ? KM_NOSLEEP : KM_SLEEP);
		if (dv == NULL) {
			device_printf(sc->sc_dev,
			    "failed to allocate device object\n");
			return ENOMEM;
		}
		dv->dv_aa.aa_type = &ch->ch_type;
		dv->dv_aa.aa_inst = &ch->ch_inst;
		dv->dv_aa.aa_ident = ch->ch_ident;
		dv->dv_aa.aa_chan = ch;
		mutex_enter(&sc->sc_dev_lock);
		SLIST_INSERT_HEAD(&sc->sc_devs, dv, dv_entry);
		mutex_exit(&sc->sc_dev_lock);
		ch->ch_dev = config_found_ia(sc->sc_dev, "hypervvmbus",
		    &dv->dv_aa, vmbus_attach_print);
	}
	return 0;
}

MODULE(MODULE_CLASS_DRIVER, vmbus, "hyperv");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
vmbus_modcmd(modcmd_t cmd, void *aux)
{
	int rv = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		rv = config_init_component(cfdriver_ioconf_vmbus,
		    cfattach_ioconf_vmbus, cfdata_ioconf_vmbus);
#endif
		break;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		rv = config_fini_component(cfdriver_ioconf_vmbus,
		    cfattach_ioconf_vmbus, cfdata_ioconf_vmbus);
#endif
		break;

	default:
		rv = ENOTTY;
		break;
	}

	return rv;
}
