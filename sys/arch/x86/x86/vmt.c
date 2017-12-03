/* $NetBSD: vmt.c,v 1.7.12.3 2017/12/03 11:36:51 jdolecek Exp $ */
/* $OpenBSD: vmt.c,v 1.11 2011/01/27 21:29:25 dtucker Exp $ */

/*
 * Copyright (c) 2007 David Crawshaw <david@zentus.com>
 * Copyright (c) 2008 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Protocol reverse engineered by Ken Kato:
 * https://sites.google.com/site/chitchatvmback/backdoor
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/callout.h>
#include <sys/reboot.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/timetc.h>
#include <sys/module.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <netinet/in.h>

#include <machine/cpuvar.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <x86/x86/vmtreg.h>
#include <x86/x86/vmtvar.h>

/* #define VMT_DEBUG */

static int	vmt_match(device_t, cfdata_t, void *);
static void	vmt_attach(device_t, device_t, void *);
static int	vmt_detach(device_t, int);

struct vmt_event {
	struct sysmon_pswitch	ev_smpsw;
	int			ev_code;
};

struct vmt_softc {
	device_t		sc_dev;

	struct sysctllog	*sc_log;
	struct vm_rpc		sc_tclo_rpc;
	bool			sc_tclo_rpc_open;
	char			*sc_rpc_buf;
	int			sc_rpc_error;
	int			sc_tclo_ping;
	int			sc_set_guest_os;
#define VMT_RPC_BUFLEN			256

	struct callout		sc_tick;
	struct callout		sc_tclo_tick;

#define VMT_CLOCK_SYNC_PERIOD_SECONDS 60
	int			sc_clock_sync_period_seconds;
	struct callout		sc_clock_sync_tick;

	struct vmt_event	sc_ev_power;
	struct vmt_event	sc_ev_reset;
	struct vmt_event	sc_ev_sleep;
	bool			sc_smpsw_valid;

	char			sc_hostname[MAXHOSTNAMELEN];
};

CFATTACH_DECL_NEW(vmt, sizeof(struct vmt_softc),
	vmt_match, vmt_attach, vmt_detach, NULL);

static int vmt_sysctl_setup_root(device_t);
static int vmt_sysctl_setup_clock_sync(device_t, const struct sysctlnode *);
static int vmt_sysctl_update_clock_sync_period(SYSCTLFN_PROTO);

static void vm_cmd(struct vm_backdoor *);
static void vm_ins(struct vm_backdoor *);
static void vm_outs(struct vm_backdoor *);

/* Functions for communicating with the VM Host. */
static int vm_rpc_open(struct vm_rpc *, uint32_t);
static int vm_rpc_close(struct vm_rpc *);
static int vm_rpc_send(const struct vm_rpc *, const uint8_t *, uint32_t);
static int vm_rpc_send_str(const struct vm_rpc *, const uint8_t *);
static int vm_rpc_get_length(const struct vm_rpc *, uint32_t *, uint16_t *);
static int vm_rpc_get_data(const struct vm_rpc *, char *, uint32_t, uint16_t);
static int vm_rpc_send_rpci_tx_buf(struct vmt_softc *, const uint8_t *, uint32_t);
static int vm_rpc_send_rpci_tx(struct vmt_softc *, const char *, ...)
    __printflike(2, 3);
static int vm_rpci_response_successful(struct vmt_softc *);

static void vmt_tclo_state_change_success(struct vmt_softc *, int, char);
static void vmt_do_reboot(struct vmt_softc *);
static void vmt_do_shutdown(struct vmt_softc *);

static void vmt_update_guest_info(struct vmt_softc *);
static void vmt_update_guest_uptime(struct vmt_softc *);
static void vmt_sync_guest_clock(struct vmt_softc *);

static void vmt_tick(void *);
static void vmt_tclo_tick(void *);
static void vmt_clock_sync_tick(void *);
static bool vmt_shutdown(device_t, int);
static void vmt_pswitch_event(void *);

extern char hostname[MAXHOSTNAMELEN];

static void
vmt_probe_cmd(struct vm_backdoor *frame, uint16_t cmd)
{
	memset(frame, 0, sizeof(*frame));

	(frame->eax).word = VM_MAGIC;
	(frame->ebx).word = ~VM_MAGIC;
	(frame->ecx).part.low = cmd;
	(frame->ecx).part.high = 0xffff;
	(frame->edx).part.low  = VM_PORT_CMD;
	(frame->edx).part.high = 0;

	vm_cmd(frame);
}

static bool
vmt_probe(void)
{
	struct vm_backdoor frame;

	vmt_probe_cmd(&frame, VM_CMD_GET_VERSION);
	if (frame.eax.word == 0xffffffff ||
	    frame.ebx.word != VM_MAGIC)
		return false;

	vmt_probe_cmd(&frame, VM_CMD_GET_SPEED);
	if (frame.eax.word == VM_MAGIC)
		return false;

	return true;
}

static int
vmt_match(device_t parent, cfdata_t match, void *aux)
{
	struct cpufeature_attach_args *cfaa = aux;
	struct cpu_info *ci = cfaa->ci;

	if (strcmp(cfaa->name, "vm") != 0)
		return 0;
	if ((ci->ci_flags & (CPUF_BSP|CPUF_SP|CPUF_PRIMARY)) == 0)
		return 0;

	return vmt_probe();
}

static void
vmt_attach(device_t parent, device_t self, void *aux)
{
	int rv;
	struct vmt_softc *sc = device_private(self);

	aprint_naive("\n");

	sc->sc_dev = self;
	sc->sc_log = NULL;

	callout_init(&sc->sc_tick, 0);
	callout_init(&sc->sc_tclo_tick, 0);
	callout_init(&sc->sc_clock_sync_tick, 0);

	sc->sc_clock_sync_period_seconds = VMT_CLOCK_SYNC_PERIOD_SECONDS;

	rv = vmt_sysctl_setup_root(self);
	if (rv != 0) {
		aprint_error_dev(self, "failed to initialize sysctl "
		    "(err %d)\n", rv);
		goto free;
	}

	sc->sc_rpc_buf = kmem_alloc(VMT_RPC_BUFLEN, KM_SLEEP);

	if (vm_rpc_open(&sc->sc_tclo_rpc, VM_RPC_OPEN_TCLO) != 0) {
		aprint_error_dev(self, "failed to open backdoor RPC channel (TCLO protocol)\n");
		goto free;
	}
	sc->sc_tclo_rpc_open = true;

	/* don't know if this is important at all yet */
	if (vm_rpc_send_rpci_tx(sc, "tools.capability.hgfs_server toolbox 1") != 0) {
		aprint_error_dev(self, "failed to set HGFS server capability\n");
		goto free;
	}

	pmf_device_register1(self, NULL, NULL, vmt_shutdown);

	sysmon_task_queue_init();

	sc->sc_ev_power.ev_smpsw.smpsw_type = PSWITCH_TYPE_POWER;
	sc->sc_ev_power.ev_smpsw.smpsw_name = device_xname(self);
	sc->sc_ev_power.ev_code = PSWITCH_EVENT_PRESSED;
	sysmon_pswitch_register(&sc->sc_ev_power.ev_smpsw);
	sc->sc_ev_reset.ev_smpsw.smpsw_type = PSWITCH_TYPE_RESET;
	sc->sc_ev_reset.ev_smpsw.smpsw_name = device_xname(self);
	sc->sc_ev_reset.ev_code = PSWITCH_EVENT_PRESSED;
	sysmon_pswitch_register(&sc->sc_ev_reset.ev_smpsw);
	sc->sc_ev_sleep.ev_smpsw.smpsw_type = PSWITCH_TYPE_SLEEP;
	sc->sc_ev_sleep.ev_smpsw.smpsw_name = device_xname(self);
	sc->sc_ev_sleep.ev_code = PSWITCH_EVENT_RELEASED;
	sysmon_pswitch_register(&sc->sc_ev_sleep.ev_smpsw);
	sc->sc_smpsw_valid = true;

	callout_setfunc(&sc->sc_tick, vmt_tick, sc);
	callout_schedule(&sc->sc_tick, hz);

	callout_setfunc(&sc->sc_tclo_tick, vmt_tclo_tick, sc);
	callout_schedule(&sc->sc_tclo_tick, hz);
	sc->sc_tclo_ping = 1;

	callout_setfunc(&sc->sc_clock_sync_tick, vmt_clock_sync_tick, sc);
	callout_schedule(&sc->sc_clock_sync_tick,
	    mstohz(sc->sc_clock_sync_period_seconds * 1000));

	vmt_sync_guest_clock(sc);

	return;

free:
	if (sc->sc_rpc_buf)
		kmem_free(sc->sc_rpc_buf, VMT_RPC_BUFLEN);
	pmf_device_register(self, NULL, NULL);
	if (sc->sc_log)
		sysctl_teardown(&sc->sc_log);
}

static int
vmt_detach(device_t self, int flags)
{
	struct vmt_softc *sc = device_private(self);

	if (sc->sc_tclo_rpc_open)
		vm_rpc_close(&sc->sc_tclo_rpc);

	if (sc->sc_smpsw_valid) {
		sysmon_pswitch_unregister(&sc->sc_ev_sleep.ev_smpsw);
		sysmon_pswitch_unregister(&sc->sc_ev_reset.ev_smpsw);
		sysmon_pswitch_unregister(&sc->sc_ev_power.ev_smpsw);
	}

	callout_halt(&sc->sc_tick, NULL);
	callout_destroy(&sc->sc_tick);

	callout_halt(&sc->sc_tclo_tick, NULL);
	callout_destroy(&sc->sc_tclo_tick);

	callout_halt(&sc->sc_clock_sync_tick, NULL);
	callout_destroy(&sc->sc_clock_sync_tick);

	if (sc->sc_rpc_buf)
		kmem_free(sc->sc_rpc_buf, VMT_RPC_BUFLEN);

	if (sc->sc_log) {
		sysctl_teardown(&sc->sc_log);
		sc->sc_log = NULL;
	}

	return 0;
}

static int
vmt_sysctl_setup_root(device_t self)
{
	const struct sysctlnode *machdep_node, *vmt_node;
	struct vmt_softc *sc = device_private(self);
	int rv;

	rv = sysctl_createv(&sc->sc_log, 0, NULL, &machdep_node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);
	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &machdep_node, &vmt_node,
	    0, CTLTYPE_NODE, device_xname(self), NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;

	rv = vmt_sysctl_setup_clock_sync(self, vmt_node);
	if (rv != 0)
		goto fail;

	return 0;

fail:
	sysctl_teardown(&sc->sc_log);
	sc->sc_log = NULL;

	return rv;
}

static int
vmt_sysctl_setup_clock_sync(device_t self, const struct sysctlnode *root_node)
{
	const struct sysctlnode *node, *period_node;
	struct vmt_softc *sc = device_private(self);
	int rv;

	rv = sysctl_createv(&sc->sc_log, 0, &root_node, &node,
	    0, CTLTYPE_NODE, "clock_sync", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		return rv;

	rv = sysctl_createv(&sc->sc_log, 0, &node, &period_node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "period",
	    SYSCTL_DESCR("Period, in seconds, at which to update the "
	        "guest's clock"),
	    vmt_sysctl_update_clock_sync_period, 0, (void *)sc, 0,
	    CTL_CREATE, CTL_EOL);
	return rv;
}

static int
vmt_sysctl_update_clock_sync_period(SYSCTLFN_ARGS)
{
	int error, period;
	struct sysctlnode node;
	struct vmt_softc *sc;

	node = *rnode;
	sc = (struct vmt_softc *)node.sysctl_data;

	period = sc->sc_clock_sync_period_seconds;
	node.sysctl_data = &period;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (sc->sc_clock_sync_period_seconds != period) {
		callout_halt(&sc->sc_clock_sync_tick, NULL);
		sc->sc_clock_sync_period_seconds = period;
		if (sc->sc_clock_sync_period_seconds > 0)
			callout_schedule(&sc->sc_clock_sync_tick,
			    mstohz(sc->sc_clock_sync_period_seconds * 1000));
	}
	return 0;
}

static void
vmt_clock_sync_tick(void *xarg)
{
	struct vmt_softc *sc = xarg;

	vmt_sync_guest_clock(sc);

	callout_schedule(&sc->sc_clock_sync_tick,
	    mstohz(sc->sc_clock_sync_period_seconds * 1000));
}

static void
vmt_update_guest_uptime(struct vmt_softc *sc)
{
	/* host wants uptime in hundredths of a second */
	if (vm_rpc_send_rpci_tx(sc, "SetGuestInfo  %d %" PRId64 "00",
	    VM_GUEST_INFO_UPTIME, time_uptime) != 0) {
		device_printf(sc->sc_dev, "unable to set guest uptime\n");
		sc->sc_rpc_error = 1;
	}
}

static void
vmt_update_guest_info(struct vmt_softc *sc)
{
	if (strncmp(sc->sc_hostname, hostname, sizeof(sc->sc_hostname)) != 0) {
		strlcpy(sc->sc_hostname, hostname, sizeof(sc->sc_hostname));

		if (vm_rpc_send_rpci_tx(sc, "SetGuestInfo  %d %s",
		    VM_GUEST_INFO_DNS_NAME, sc->sc_hostname) != 0) {
			device_printf(sc->sc_dev, "unable to set hostname\n");
			sc->sc_rpc_error = 1;
		}
	}

	/*
	 * we're supposed to pass the full network address information back here,
	 * but that involves xdr (sunrpc) data encoding, which seems a bit unreasonable.
	 */

	if (sc->sc_set_guest_os == 0) {
		if (vm_rpc_send_rpci_tx(sc, "SetGuestInfo  %d %s %s %s",
		    VM_GUEST_INFO_OS_NAME_FULL, ostype, osrelease, machine_arch) != 0) {
			device_printf(sc->sc_dev, "unable to set full guest OS\n");
			sc->sc_rpc_error = 1;
		}

		/*
		 * host doesn't like it if we send an OS name it doesn't recognise,
		 * so use "other" for i386 and "other-64" for amd64
		 */
		if (vm_rpc_send_rpci_tx(sc, "SetGuestInfo  %d %s",
		    VM_GUEST_INFO_OS_NAME, VM_OS_NAME) != 0) {
			device_printf(sc->sc_dev, "unable to set guest OS\n");
			sc->sc_rpc_error = 1;
		}

		sc->sc_set_guest_os = 1;
	}
}

static void
vmt_sync_guest_clock(struct vmt_softc *sc)
{
	struct vm_backdoor frame;
	struct timespec ts;

	memset(&frame, 0, sizeof(frame));
	frame.eax.word = VM_MAGIC;
	frame.ecx.part.low = VM_CMD_GET_TIME_FULL;
	frame.edx.part.low  = VM_PORT_CMD;
	vm_cmd(&frame);

	if (frame.eax.word != 0xffffffff) {
		ts.tv_sec = ((uint64_t)frame.esi.word << 32) | frame.edx.word;
		ts.tv_nsec = frame.ebx.word * 1000;
		tc_setclock(&ts);
	}
}

static void
vmt_tick(void *xarg)
{
	struct vmt_softc *sc = xarg;

	vmt_update_guest_info(sc);
	vmt_update_guest_uptime(sc);

	callout_schedule(&sc->sc_tick, hz * 15);
}

static void
vmt_tclo_state_change_success(struct vmt_softc *sc, int success, char state)
{
	if (vm_rpc_send_rpci_tx(sc, "tools.os.statechange.status %d %d",
	    success, state) != 0) {
		device_printf(sc->sc_dev, "unable to send state change result\n");
		sc->sc_rpc_error = 1;
	}
}

static void
vmt_do_shutdown(struct vmt_softc *sc)
{
	vmt_tclo_state_change_success(sc, 1, VM_STATE_CHANGE_HALT);
	vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_REPLY_OK);

	device_printf(sc->sc_dev, "host requested shutdown\n");
	sysmon_task_queue_sched(0, vmt_pswitch_event, &sc->sc_ev_power);
}

static void
vmt_do_reboot(struct vmt_softc *sc)
{
	vmt_tclo_state_change_success(sc, 1, VM_STATE_CHANGE_REBOOT);
	vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_REPLY_OK);

	device_printf(sc->sc_dev, "host requested reboot\n");
	sysmon_task_queue_sched(0, vmt_pswitch_event, &sc->sc_ev_reset);
}

static void
vmt_do_resume(struct vmt_softc *sc)
{
	device_printf(sc->sc_dev, "guest resuming from suspended state\n");

	vmt_sync_guest_clock(sc);

	/* force guest info update */
	sc->sc_hostname[0] = '\0';
	sc->sc_set_guest_os = 0;
	vmt_update_guest_info(sc);

	vmt_tclo_state_change_success(sc, 1, VM_STATE_CHANGE_RESUME);
	if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_REPLY_OK) != 0) {
		device_printf(sc->sc_dev, "error sending resume response\n");
		sc->sc_rpc_error = 1;
	}

	sysmon_task_queue_sched(0, vmt_pswitch_event, &sc->sc_ev_sleep);
}

static bool
vmt_shutdown(device_t self, int flags)
{
	struct vmt_softc *sc = device_private(self);

	if (vm_rpc_send_rpci_tx(sc, "tools.capability.hgfs_server toolbox 0") != 0) {
		device_printf(sc->sc_dev, "failed to disable hgfs server capability\n");
	}

	if (vm_rpc_send(&sc->sc_tclo_rpc, NULL, 0) != 0) {
		device_printf(sc->sc_dev, "failed to send shutdown ping\n");
	}

	vm_rpc_close(&sc->sc_tclo_rpc);

	return true;
}

static void
vmt_pswitch_event(void *xarg)
{
	struct vmt_event *ev = xarg;

	sysmon_pswitch_event(&ev->ev_smpsw, ev->ev_code);
}

static void
vmt_tclo_tick(void *xarg)
{
	struct vmt_softc *sc = xarg;
	u_int32_t rlen;
	u_int16_t ack;

	/* reopen tclo channel if it's currently closed */
	if (sc->sc_tclo_rpc.channel == 0 &&
	    sc->sc_tclo_rpc.cookie1 == 0 &&
	    sc->sc_tclo_rpc.cookie2 == 0) {
		if (vm_rpc_open(&sc->sc_tclo_rpc, VM_RPC_OPEN_TCLO) != 0) {
			device_printf(sc->sc_dev, "unable to reopen TCLO channel\n");
			callout_schedule(&sc->sc_tclo_tick, hz * 15);
			return;
		}

		if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_RESET_REPLY) != 0) {
			device_printf(sc->sc_dev, "failed to send reset reply\n");
			sc->sc_rpc_error = 1;
			goto out;
		} else {
			sc->sc_rpc_error = 0;
		}
	}

	if (sc->sc_tclo_ping) {
		if (vm_rpc_send(&sc->sc_tclo_rpc, NULL, 0) != 0) {
			device_printf(sc->sc_dev, "failed to send TCLO outgoing ping\n");
			sc->sc_rpc_error = 1;
			goto out;
		}
	}

	if (vm_rpc_get_length(&sc->sc_tclo_rpc, &rlen, &ack) != 0) {
		device_printf(sc->sc_dev, "failed to get length of incoming TCLO data\n");
		sc->sc_rpc_error = 1;
		goto out;
	}

	if (rlen == 0) {
		sc->sc_tclo_ping = 1;
		goto out;
	}

	if (rlen >= VMT_RPC_BUFLEN) {
		rlen = VMT_RPC_BUFLEN - 1;
	}
	if (vm_rpc_get_data(&sc->sc_tclo_rpc, sc->sc_rpc_buf, rlen, ack) != 0) {
		device_printf(sc->sc_dev, "failed to get incoming TCLO data\n");
		sc->sc_rpc_error = 1;
		goto out;
	}
	sc->sc_tclo_ping = 0;

#ifdef VMT_DEBUG
	printf("vmware: received message '%s'\n", sc->sc_rpc_buf);
#endif

	if (strcmp(sc->sc_rpc_buf, "reset") == 0) {

		if (sc->sc_rpc_error != 0) {
			device_printf(sc->sc_dev, "resetting rpc\n");
			vm_rpc_close(&sc->sc_tclo_rpc);
			/* reopen and send the reset reply next time around */
			goto out;
		}

		if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_RESET_REPLY) != 0) {
			device_printf(sc->sc_dev, "failed to send reset reply\n");
			sc->sc_rpc_error = 1;
		}

	} else if (strcmp(sc->sc_rpc_buf, "ping") == 0) {

		vmt_update_guest_info(sc);
		if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_REPLY_OK) != 0) {
			device_printf(sc->sc_dev, "error sending ping response\n");
			sc->sc_rpc_error = 1;
		}

	} else if (strcmp(sc->sc_rpc_buf, "OS_Halt") == 0) {
		vmt_do_shutdown(sc);
	} else if (strcmp(sc->sc_rpc_buf, "OS_Reboot") == 0) {
		vmt_do_reboot(sc);
	} else if (strcmp(sc->sc_rpc_buf, "OS_PowerOn") == 0) {
		vmt_tclo_state_change_success(sc, 1, VM_STATE_CHANGE_POWERON);
		if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_REPLY_OK) != 0) {
			device_printf(sc->sc_dev, "error sending poweron response\n");
			sc->sc_rpc_error = 1;
		}
	} else if (strcmp(sc->sc_rpc_buf, "OS_Suspend") == 0) {
		log(LOG_KERN | LOG_NOTICE, "VMware guest entering suspended state\n");

		vmt_tclo_state_change_success(sc, 1, VM_STATE_CHANGE_SUSPEND);
		if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_REPLY_OK) != 0) {
			device_printf(sc->sc_dev, "error sending suspend response\n");
			sc->sc_rpc_error = 1;
		}
	} else if (strcmp(sc->sc_rpc_buf, "OS_Resume") == 0) {
		vmt_do_resume(sc);
	} else if (strcmp(sc->sc_rpc_buf, "Capabilities_Register") == 0) {

		/* don't know if this is important at all */
		if (vm_rpc_send_rpci_tx(sc, "vmx.capability.unified_loop toolbox") != 0) {
			device_printf(sc->sc_dev, "unable to set unified loop\n");
			sc->sc_rpc_error = 1;
		}
		if (vm_rpci_response_successful(sc) == 0) {
			device_printf(sc->sc_dev, "host rejected unified loop setting\n");
		}

		/* the trailing space is apparently important here */
		if (vm_rpc_send_rpci_tx(sc, "tools.capability.statechange ") != 0) {
			device_printf(sc->sc_dev, "unable to send statechange capability\n");
			sc->sc_rpc_error = 1;
		}
		if (vm_rpci_response_successful(sc) == 0) {
			device_printf(sc->sc_dev, "host rejected statechange capability\n");
		}

		if (vm_rpc_send_rpci_tx(sc, "tools.set.version %u", VM_VERSION_UNMANAGED) != 0) {
			device_printf(sc->sc_dev, "unable to set tools version\n");
			sc->sc_rpc_error = 1;
		}

		vmt_update_guest_uptime(sc);

		if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_REPLY_OK) != 0) {
			device_printf(sc->sc_dev, "error sending capabilities_register response\n");
			sc->sc_rpc_error = 1;
		}
	} else if (strcmp(sc->sc_rpc_buf, "Set_Option broadcastIP 1") == 0) {
		struct ifaddr *iface_addr = NULL;
		struct ifnet *iface;
		struct sockaddr_in *guest_ip;
		int s;
		struct psref psref;

		/* find first available ipv4 address */
		guest_ip = NULL;
		s = pserialize_read_enter();
		IFNET_READER_FOREACH(iface) {

			/* skip loopback */
			if (strncmp(iface->if_xname, "lo", 2) == 0 &&
			    iface->if_xname[2] >= '0' && iface->if_xname[2] <= '9') {
				continue;
			}

			IFADDR_READER_FOREACH(iface_addr, iface) {
				if (iface_addr->ifa_addr->sa_family != AF_INET) {
					continue;
				}

				guest_ip = satosin(iface_addr->ifa_addr);
				ifa_acquire(iface_addr, &psref);
				goto got;
			}
		}
	got:
		pserialize_read_exit(s);

		if (guest_ip != NULL) {
			if (vm_rpc_send_rpci_tx(sc, "info-set guestinfo.ip %s",
			    inet_ntoa(guest_ip->sin_addr)) != 0) {
				device_printf(sc->sc_dev, "unable to send guest IP address\n");
				sc->sc_rpc_error = 1;
			}
			ifa_release(iface_addr, &psref);

			if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_REPLY_OK) != 0) {
				device_printf(sc->sc_dev, "error sending broadcastIP response\n");
				sc->sc_rpc_error = 1;
			}
		} else {
			if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_REPLY_ERROR_IP_ADDR) != 0) {
				device_printf(sc->sc_dev,
				    "error sending broadcastIP error response\n");
				sc->sc_rpc_error = 1;
			}
		}
	} else {
		if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_REPLY_ERROR) != 0) {
			device_printf(sc->sc_dev, "error sending unknown command reply\n");
			sc->sc_rpc_error = 1;
		}
	}

out:
	callout_schedule(&sc->sc_tclo_tick, sc->sc_tclo_ping ? hz : 1);
}

static void
vm_cmd(struct vm_backdoor *frame)
{
	BACKDOOR_OP("inl %%dx, %%eax;", frame);
}

static void
vm_ins(struct vm_backdoor *frame)
{
	BACKDOOR_OP("cld;\n\trep insb;", frame);
}

static void
vm_outs(struct vm_backdoor *frame)
{
	BACKDOOR_OP("cld;\n\trep outsb;", frame);
}

static int
vm_rpc_open(struct vm_rpc *rpc, uint32_t proto)
{
	struct vm_backdoor frame;

	memset(&frame, 0, sizeof(frame));
	frame.eax.word      = VM_MAGIC;
	frame.ebx.word      = proto | VM_RPC_FLAG_COOKIE;
	frame.ecx.part.low  = VM_CMD_RPC;
	frame.ecx.part.high = VM_RPC_OPEN;
	frame.edx.part.low  = VM_PORT_CMD;
	frame.edx.part.high = 0;

	vm_cmd(&frame);

	if (frame.ecx.part.high != 1 || frame.edx.part.low != 0) {
		/* open-vm-tools retries without VM_RPC_FLAG_COOKIE here.. */
		printf("vmware: open failed, eax=%08x, ecx=%08x, edx=%08x\n",
			frame.eax.word, frame.ecx.word, frame.edx.word);
		return EIO;
	}

	rpc->channel = frame.edx.part.high;
	rpc->cookie1 = frame.esi.word;
	rpc->cookie2 = frame.edi.word;

	return 0;
}

static int
vm_rpc_close(struct vm_rpc *rpc)
{
	struct vm_backdoor frame;

	memset(&frame, 0, sizeof(frame));
	frame.eax.word      = VM_MAGIC;
	frame.ebx.word      = 0;
	frame.ecx.part.low  = VM_CMD_RPC;
	frame.ecx.part.high = VM_RPC_CLOSE;
	frame.edx.part.low  = VM_PORT_CMD;
	frame.edx.part.high = rpc->channel;
	frame.edi.word      = rpc->cookie2;
	frame.esi.word      = rpc->cookie1;

	vm_cmd(&frame);

	if (frame.ecx.part.high == 0 || frame.ecx.part.low != 0) {
		printf("vmware: close failed, eax=%08x, ecx=%08x\n",
				frame.eax.word, frame.ecx.word);
		return EIO;
	}

	rpc->channel = 0;
	rpc->cookie1 = 0;
	rpc->cookie2 = 0;

	return 0;
}

static int
vm_rpc_send(const struct vm_rpc *rpc, const uint8_t *buf, uint32_t length)
{
	struct vm_backdoor frame;

	/* Send the length of the command. */
	memset(&frame, 0, sizeof(frame));
	frame.eax.word = VM_MAGIC;
	frame.ebx.word = length;
	frame.ecx.part.low  = VM_CMD_RPC;
	frame.ecx.part.high = VM_RPC_SET_LENGTH;
	frame.edx.part.low  = VM_PORT_CMD;
	frame.edx.part.high = rpc->channel;
	frame.esi.word = rpc->cookie1;
	frame.edi.word = rpc->cookie2;

	vm_cmd(&frame);

	if ((frame.ecx.part.high & VM_RPC_REPLY_SUCCESS) == 0) {
		printf("vmware: sending length failed, eax=%08x, ecx=%08x\n",
				frame.eax.word, frame.ecx.word);
		return EIO;
	}

	if (length == 0)
		return 0; /* Only need to poke once if command is null. */

	/* Send the command using enhanced RPC. */
	memset(&frame, 0, sizeof(frame));
	frame.eax.word = VM_MAGIC;
	frame.ebx.word = VM_RPC_ENH_DATA;
	frame.ecx.word = length;
	frame.edx.part.low  = VM_PORT_RPC;
	frame.edx.part.high = rpc->channel;
	frame.ebp.word = rpc->cookie1;
	frame.edi.word = rpc->cookie2;
#ifdef __amd64__
	frame.esi.quad = (uint64_t)buf;
#else
	frame.esi.word = (uint32_t)buf;
#endif

	vm_outs(&frame);

	if (frame.ebx.word != VM_RPC_ENH_DATA) {
		/* open-vm-tools retries on VM_RPC_REPLY_CHECKPOINT */
		printf("vmware: send failed, ebx=%08x\n", frame.ebx.word);
		return EIO;
	}

	return 0;
}

static int
vm_rpc_send_str(const struct vm_rpc *rpc, const uint8_t *str)
{
	return vm_rpc_send(rpc, str, strlen(str));
}

static int
vm_rpc_get_data(const struct vm_rpc *rpc, char *data, uint32_t length,
    uint16_t dataid)
{
	struct vm_backdoor frame;

	/* Get data using enhanced RPC. */
	memset(&frame, 0, sizeof(frame));
	frame.eax.word      = VM_MAGIC;
	frame.ebx.word      = VM_RPC_ENH_DATA;
	frame.ecx.word      = length;
	frame.edx.part.low  = VM_PORT_RPC;
	frame.edx.part.high = rpc->channel;
	frame.esi.word      = rpc->cookie1;
#ifdef __amd64__
	frame.edi.quad      = (uint64_t)data;
#else
	frame.edi.word      = (uint32_t)data;
#endif
	frame.ebp.word      = rpc->cookie2;

	vm_ins(&frame);

	/* NUL-terminate the data */
	data[length] = '\0';

	if (frame.ebx.word != VM_RPC_ENH_DATA) {
		printf("vmware: get data failed, ebx=%08x\n",
				frame.ebx.word);
		return EIO;
	}

	/* Acknowledge data received. */
	memset(&frame, 0, sizeof(frame));
	frame.eax.word      = VM_MAGIC;
	frame.ebx.word      = dataid;
	frame.ecx.part.low  = VM_CMD_RPC;
	frame.ecx.part.high = VM_RPC_GET_END;
	frame.edx.part.low  = VM_PORT_CMD;
	frame.edx.part.high = rpc->channel;
	frame.esi.word      = rpc->cookie1;
	frame.edi.word      = rpc->cookie2;

	vm_cmd(&frame);

	if (frame.ecx.part.high == 0) {
		printf("vmware: ack data failed, eax=%08x, ecx=%08x\n",
				frame.eax.word, frame.ecx.word);
		return EIO;
	}

	return 0;
}

static int
vm_rpc_get_length(const struct vm_rpc *rpc, uint32_t *length, uint16_t *dataid)
{
	struct vm_backdoor frame;

	memset(&frame, 0, sizeof(frame));
	frame.eax.word      = VM_MAGIC;
	frame.ebx.word      = 0;
	frame.ecx.part.low  = VM_CMD_RPC;
	frame.ecx.part.high = VM_RPC_GET_LENGTH;
	frame.edx.part.low  = VM_PORT_CMD;
	frame.edx.part.high = rpc->channel;
	frame.esi.word      = rpc->cookie1;
	frame.edi.word      = rpc->cookie2;

	vm_cmd(&frame);

	if ((frame.ecx.part.high & VM_RPC_REPLY_SUCCESS) == 0) {
		printf("vmware: get length failed, eax=%08x, ecx=%08x\n",
				frame.eax.word, frame.ecx.word);
		return EIO;
	}
	if ((frame.ecx.part.high & VM_RPC_REPLY_DORECV) == 0) {
		*length = 0;
		*dataid = 0;
	} else {
		*length = frame.ebx.word;
		*dataid = frame.edx.part.high;
	}

	return 0;
}

static int
vm_rpci_response_successful(struct vmt_softc *sc)
{
	return (sc->sc_rpc_buf[0] == '1' && sc->sc_rpc_buf[1] == ' ');
}

static int
vm_rpc_send_rpci_tx_buf(struct vmt_softc *sc, const uint8_t *buf, uint32_t length)
{
	struct vm_rpc rpci;
	u_int32_t rlen;
	u_int16_t ack;
	int result = 0;

	if (vm_rpc_open(&rpci, VM_RPC_OPEN_RPCI) != 0) {
		device_printf(sc->sc_dev, "rpci channel open failed\n");
		return EIO;
	}

	if (vm_rpc_send(&rpci, sc->sc_rpc_buf, length) != 0) {
		device_printf(sc->sc_dev, "unable to send rpci command\n");
		result = EIO;
		goto out;
	}

	if (vm_rpc_get_length(&rpci, &rlen, &ack) != 0) {
		device_printf(sc->sc_dev, "failed to get length of rpci response data\n");
		result = EIO;
		goto out;
	}

	if (rlen > 0) {
		if (rlen >= VMT_RPC_BUFLEN) {
			rlen = VMT_RPC_BUFLEN - 1;
		}

		if (vm_rpc_get_data(&rpci, sc->sc_rpc_buf, rlen, ack) != 0) {
			device_printf(sc->sc_dev, "failed to get rpci response data\n");
			result = EIO;
			goto out;
		}
	}

out:
	if (vm_rpc_close(&rpci) != 0) {
		device_printf(sc->sc_dev, "unable to close rpci channel\n");
	}

	return result;
}

static int
vm_rpc_send_rpci_tx(struct vmt_softc *sc, const char *fmt, ...)
{
	va_list args;
	int len;

	va_start(args, fmt);
	len = vsnprintf(sc->sc_rpc_buf, VMT_RPC_BUFLEN, fmt, args);
	va_end(args);

	if (len >= VMT_RPC_BUFLEN) {
		device_printf(sc->sc_dev, "rpci command didn't fit in buffer\n");
		return EIO;
	}

	return vm_rpc_send_rpci_tx_buf(sc, sc->sc_rpc_buf, len);
}

#if 0
	struct vm_backdoor frame;

	memset(&frame, 0, sizeof(frame));

	frame.eax.word = VM_MAGIC;
	frame.ecx.part.low = VM_CMD_GET_VERSION;
	frame.edx.part.low  = VM_PORT_CMD;

	printf("\n");
	printf("eax 0x%08x\n", frame.eax.word);
	printf("ebx 0x%08x\n", frame.ebx.word);
	printf("ecx 0x%08x\n", frame.ecx.word);
	printf("edx 0x%08x\n", frame.edx.word);
	printf("ebp 0x%08x\n", frame.ebp.word);
	printf("edi 0x%08x\n", frame.edi.word);
	printf("esi 0x%08x\n", frame.esi.word);

	vm_cmd(&frame);

	printf("-\n");
	printf("eax 0x%08x\n", frame.eax.word);
	printf("ebx 0x%08x\n", frame.ebx.word);
	printf("ecx 0x%08x\n", frame.ecx.word);
	printf("edx 0x%08x\n", frame.edx.word);
	printf("ebp 0x%08x\n", frame.ebp.word);
	printf("edi 0x%08x\n", frame.edi.word);
	printf("esi 0x%08x\n", frame.esi.word);
#endif

/*
 * Notes on tracing backdoor activity in vmware-guestd:
 *
 * - Find the addresses of the inl / rep insb / rep outsb
 *   instructions used to perform backdoor operations.
 *   One way to do this is to disassemble vmware-guestd:
 *
 *   $ objdump -S /emul/freebsd/sbin/vmware-guestd > vmware-guestd.S
 *
 *   and search for '<tab>in ' in the resulting file.  The rep insb and
 *   rep outsb code is directly below that.
 *
 * - Run vmware-guestd under gdb, setting up breakpoints as follows:
 *   (the addresses shown here are the ones from VMware-server-1.0.10-203137,
 *   the last version that actually works in FreeBSD emulation on OpenBSD)
 *
 * break *0x805497b   (address of 'in' instruction)
 * commands 1
 * silent
 * echo INOUT\n
 * print/x $ecx
 * print/x $ebx
 * print/x $edx
 * continue
 * end
 * break *0x805497c   (address of instruction after 'in')
 * commands 2
 * silent
 * echo ===\n
 * print/x $ecx
 * print/x $ebx
 * print/x $edx
 * echo \n
 * continue
 * end
 * break *0x80549b7   (address of instruction before 'rep insb')
 * commands 3
 * silent
 * set variable $inaddr = $edi
 * set variable $incount = $ecx
 * continue
 * end
 * break *0x80549ba   (address of instruction after 'rep insb')
 * commands 4
 * silent
 * echo IN\n
 * print $incount
 * x/s $inaddr
 * echo \n
 * continue
 * end
 * break *0x80549fb    (address of instruction before 'rep outsb')
 * commands 5
 * silent
 * echo OUT\n
 * print $ecx
 * x/s $esi
 * echo \n
 * continue
 * end
 *
 * This will produce a log of the backdoor operations, including the
 * data sent and received and the relevant register values.  You can then
 * match the register values to the various constants in this file.
 */

MODULE(MODULE_CLASS_DRIVER, vmt, "sysmon_power,sysmon_taskq");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
vmt_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_vmt,
		    cfattach_ioconf_vmt, cfdata_ioconf_vmt);
#endif
		break;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_vmt,
		    cfattach_ioconf_vmt, cfdata_ioconf_vmt);
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
