/* $NetBSD: vmt_subr.c,v 1.3.16.1 2024/09/11 15:52:17 martin Exp $ */
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
#include <sys/types.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <net/if.h>
#include <netinet/in.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>
#include <dev/vmt/vmtreg.h>
#include <dev/vmt/vmtvar.h>

/* #define VMT_DEBUG */

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
static bool vmt_shutdown(device_t, int);

static void vmt_update_guest_info(struct vmt_softc *);
static void vmt_update_guest_uptime(struct vmt_softc *);
static void vmt_sync_guest_clock(struct vmt_softc *);

static void vmt_tick(void *);
static void vmt_clock_sync_tick(void *);
static void vmt_pswitch_event(void *);

static void vmt_tclo_tick(void *);
static int vmt_tclo_process(struct vmt_softc *, const char *);
static void vmt_tclo_reset(struct vmt_softc *);
static void vmt_tclo_ping(struct vmt_softc *);
static void vmt_tclo_halt(struct vmt_softc *);
static void vmt_tclo_reboot(struct vmt_softc *);
static void vmt_tclo_poweron(struct vmt_softc *);
static void vmt_tclo_suspend(struct vmt_softc *);
static void vmt_tclo_resume(struct vmt_softc *);
static void vmt_tclo_capreg(struct vmt_softc *);
static void vmt_tclo_broadcastip(struct vmt_softc *);

struct vmt_tclo_rpc {
	const char	*name;
	void		(*cb)(struct vmt_softc *);
} vmt_tclo_rpc[] = {
	/* Keep sorted by name (case-sensitive) */
	{ "Capabilities_Register",	vmt_tclo_capreg },
	{ "OS_Halt",			vmt_tclo_halt },
	{ "OS_PowerOn",			vmt_tclo_poweron },
	{ "OS_Reboot",			vmt_tclo_reboot },
	{ "OS_Resume",			vmt_tclo_resume },
	{ "OS_Suspend",			vmt_tclo_suspend },
	{ "Set_Option broadcastIP 1",	vmt_tclo_broadcastip },
	{ "ping",			vmt_tclo_ping },
	{ "reset",			vmt_tclo_reset },
#if 0
	/* Various unsupported commands */
	{ "Set_Option autohide 0" },
	{ "Set_Option copypaste 1" },
	{ "Set_Option enableDnD 1" },
	{ "Set_Option enableMessageBusTunnel 0" },
	{ "Set_Option linkRootHgfsShare 0" },
	{ "Set_Option mapRootHgfsShare 0" },
	{ "Set_Option synctime 1" },
	{ "Set_Option synctime.period 0" },
	{ "Set_Option time.synchronize.tools.enable 1" },
	{ "Set_Option time.synchronize.tools.percentCorrection 0" },
	{ "Set_Option time.synchronize.tools.slewCorrection 1" },
	{ "Set_Option time.synchronize.tools.startup 1" },
	{ "Set_Option toolScripts.afterPowerOn 1" },
	{ "Set_Option toolScripts.afterResume 1" },
	{ "Set_Option toolScripts.beforePowerOff 1" },
	{ "Set_Option toolScripts.beforeSuspend 1" },
	{ "Time_Synchronize 0" },
	{ "Vix_1_Relayed_Command \"38cdcae40e075d66\"" },
#endif
	{ NULL, NULL },
};

extern char hostname[MAXHOSTNAMELEN];

static void
vmt_probe_cmd(struct vm_backdoor *frame, uint16_t cmd)
{
	memset(frame, 0, sizeof(*frame));

	frame->eax = VM_MAGIC;
	frame->ebx = ~VM_MAGIC & VM_REG_WORD_MASK;
	frame->ecx = VM_REG_CMD(0xffff, cmd);
	frame->edx = VM_REG_CMD(0, VM_PORT_CMD);

	vm_cmd(frame);
}

bool
vmt_probe(void)
{
	struct vm_backdoor frame;

	vmt_probe_cmd(&frame, VM_CMD_GET_VERSION);
	if (__SHIFTOUT(frame.eax, VM_REG_WORD_MASK) == 0xffffffff ||
	    __SHIFTOUT(frame.ebx, VM_REG_WORD_MASK) != VM_MAGIC)
		return false;

	vmt_probe_cmd(&frame, VM_CMD_GET_SPEED);
	if (__SHIFTOUT(frame.eax, VM_REG_WORD_MASK) == VM_MAGIC)
		return false;

	return true;
}

void
vmt_common_attach(struct vmt_softc *sc)
{
	device_t self;
	struct vm_backdoor frame;
	int rv;

	self = sc->sc_dev;
	sc->sc_log = NULL;

	/* check again */
	vmt_probe_cmd(&frame, VM_CMD_GET_VERSION);
	if (__SHIFTOUT(frame.eax, VM_REG_WORD_MASK) == 0xffffffff ||
	    __SHIFTOUT(frame.ebx, VM_REG_WORD_MASK) != VM_MAGIC) {
		aprint_error_dev(self, "failed to get VMware version\n");
		return;
	}

	/* show uuid */
	{
		struct uuid uuid;
		uint32_t u;

		vmt_probe_cmd(&frame, VM_CMD_GET_BIOS_UUID);
		uuid.time_low =
		    bswap32(__SHIFTOUT(frame.eax, VM_REG_WORD_MASK));
		u = bswap32(__SHIFTOUT(frame.ebx, VM_REG_WORD_MASK));
		uuid.time_mid = u >> 16;
		uuid.time_hi_and_version = u;
		u = bswap32(__SHIFTOUT(frame.ecx, VM_REG_WORD_MASK));
		uuid.clock_seq_hi_and_reserved = u >> 24;
		uuid.clock_seq_low = u >> 16;
		uuid.node[0] = u >> 8;
		uuid.node[1] = u;
		u = bswap32(__SHIFTOUT(frame.edx, VM_REG_WORD_MASK));
		uuid.node[2] = u >> 24;
		uuid.node[3] = u >> 16;
		uuid.node[4] = u >> 8;
		uuid.node[5] = u;

		uuid_snprintf(sc->sc_uuid, sizeof(sc->sc_uuid), &uuid);
		aprint_verbose_dev(sc->sc_dev, "UUID: %s\n", sc->sc_uuid);
	}

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
		aprint_error_dev(self, "failed to open backdoor RPC channel "
		    "(TCLO protocol)\n");
		goto free;
	}
	sc->sc_tclo_rpc_open = true;

	/* don't know if this is important at all yet */
	if (vm_rpc_send_rpci_tx(sc,
	    "tools.capability.hgfs_server toolbox 1") != 0) {
		aprint_error_dev(self,
		    "failed to set HGFS server capability\n");
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

int
vmt_common_detach(struct vmt_softc *sc)
{
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

	rv = sysctl_createv(&sc->sc_log, 0, &vmt_node, NULL,
	    CTLFLAG_READONLY, CTLTYPE_STRING, "uuid",
	    SYSCTL_DESCR("UUID of virtual machine"),
	    NULL, 0, sc->sc_uuid, 0,
	    CTL_CREATE, CTL_EOL);

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
	 * we're supposed to pass the full network address information back
	 * here, but that involves xdr (sunrpc) data encoding, which seems
	 * a bit unreasonable.
	 */

	if (sc->sc_set_guest_os == 0) {
		if (vm_rpc_send_rpci_tx(sc, "SetGuestInfo  %d %s %s %s",
		    VM_GUEST_INFO_OS_NAME_FULL,
		    ostype, osrelease, machine_arch) != 0) {
			device_printf(sc->sc_dev,
			    "unable to set full guest OS\n");
			sc->sc_rpc_error = 1;
		}

		/*
		 * Host doesn't like it if we send an OS name it doesn't
		 * recognise, so use "other" for i386 and "other-64" for amd64.
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
	frame.eax = VM_MAGIC;
	frame.ecx = VM_CMD_GET_TIME_FULL;
	frame.edx = VM_REG_CMD(0, VM_PORT_CMD);
	vm_cmd(&frame);

	if (__SHIFTOUT(frame.eax, VM_REG_WORD_MASK) != 0xffffffff) {
		ts.tv_sec = ((uint64_t)(
		    __SHIFTOUT(frame.esi, VM_REG_WORD_MASK) << 32)) |
		    __SHIFTOUT(frame.edx, VM_REG_WORD_MASK);
		ts.tv_nsec = __SHIFTOUT(frame.ebx, VM_REG_WORD_MASK) * 1000;
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
		device_printf(sc->sc_dev,
		    "unable to send state change result\n");
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

	if (vm_rpc_send_rpci_tx(sc,
	    "tools.capability.hgfs_server toolbox 0") != 0) {
		device_printf(sc->sc_dev,
		    "failed to disable hgfs server capability\n");
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
vmt_tclo_reset(struct vmt_softc *sc)
{

	if (sc->sc_rpc_error != 0) {
		device_printf(sc->sc_dev, "resetting rpc\n");
		vm_rpc_close(&sc->sc_tclo_rpc);

		/* reopen and send the reset reply next time around */
		return;
	}

	if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_RESET_REPLY) != 0) {
		device_printf(sc->sc_dev, "failed to send reset reply\n");
		sc->sc_rpc_error = 1;
	}

}

static void
vmt_tclo_ping(struct vmt_softc *sc)
{

	vmt_update_guest_info(sc);
	if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_REPLY_OK) != 0) {
		device_printf(sc->sc_dev, "error sending ping response\n");
		sc->sc_rpc_error = 1;
	}
}

static void
vmt_tclo_halt(struct vmt_softc *sc)
{

	vmt_do_shutdown(sc);
}

static void
vmt_tclo_reboot(struct vmt_softc *sc)
{

	vmt_do_reboot(sc);
}

static void
vmt_tclo_poweron(struct vmt_softc *sc)
{

	vmt_tclo_state_change_success(sc, 1, VM_STATE_CHANGE_POWERON);
	if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_REPLY_OK) != 0) {
		device_printf(sc->sc_dev, "error sending poweron response\n");
		sc->sc_rpc_error = 1;
	}
}

static void
vmt_tclo_suspend(struct vmt_softc *sc)
{

	log(LOG_KERN | LOG_NOTICE,
	    "VMware guest entering suspended state\n");

	vmt_tclo_state_change_success(sc, 1, VM_STATE_CHANGE_SUSPEND);
	if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_REPLY_OK) != 0) {
		device_printf(sc->sc_dev, "error sending suspend response\n");
		sc->sc_rpc_error = 1;
	}
}

static void
vmt_tclo_resume(struct vmt_softc *sc)
{

	vmt_do_resume(sc); /* XXX msaitoh extract */
}

static void
vmt_tclo_capreg(struct vmt_softc *sc)
{

	/* don't know if this is important at all */
	if (vm_rpc_send_rpci_tx(sc,
		"vmx.capability.unified_loop toolbox") != 0) {
		device_printf(sc->sc_dev, "unable to set unified loop\n");
		sc->sc_rpc_error = 1;
	}
	if (vm_rpci_response_successful(sc) == 0) {
		device_printf(sc->sc_dev,
		    "host rejected unified loop setting\n");
	}

	/* the trailing space is apparently important here */
	if (vm_rpc_send_rpci_tx(sc,
		"tools.capability.statechange ") != 0) {
		device_printf(sc->sc_dev,
		    "unable to send statechange capability\n");
		sc->sc_rpc_error = 1;
	}
	if (vm_rpci_response_successful(sc) == 0) {
		device_printf(sc->sc_dev,
		    "host rejected statechange capability\n");
	}

	if (vm_rpc_send_rpci_tx(sc,
		"tools.set.version %u", VM_VERSION_UNMANAGED) != 0) {
		device_printf(sc->sc_dev, "unable to set tools version\n");
		sc->sc_rpc_error = 1;
	}

	vmt_update_guest_uptime(sc);

	if (vm_rpc_send_str(&sc->sc_tclo_rpc, VM_RPC_REPLY_OK) != 0) {
		device_printf(sc->sc_dev,
		    "error sending capabilities_register response\n");
		sc->sc_rpc_error = 1;
	}
}

static void
vmt_tclo_broadcastip(struct vmt_softc *sc)
{
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
		    iface->if_xname[2] >= '0' &&
		    iface->if_xname[2] <= '9') {
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
			device_printf(sc->sc_dev,
			    "unable to send guest IP address\n");
			sc->sc_rpc_error = 1;
		}
		ifa_release(iface_addr, &psref);

		if (vm_rpc_send_str(&sc->sc_tclo_rpc,
			VM_RPC_REPLY_OK) != 0) {
			device_printf(sc->sc_dev,
			    "error sending broadcastIP response\n");
			sc->sc_rpc_error = 1;
		}
	} else {
		if (vm_rpc_send_str(&sc->sc_tclo_rpc,
			VM_RPC_REPLY_ERROR_IP_ADDR) != 0) {
			device_printf(sc->sc_dev,
			    "error sending broadcastIP"
			    " error response\n");
			sc->sc_rpc_error = 1;
		}
	}
}

int
vmt_tclo_process(struct vmt_softc *sc, const char *name)
{
	int i;

	/* Search for rpc command and call handler */
	for (i = 0; vmt_tclo_rpc[i].name != NULL; i++) {
		if (strcmp(vmt_tclo_rpc[i].name, sc->sc_rpc_buf) == 0) {
			vmt_tclo_rpc[i].cb(sc);
			return (0);
		}
	}

	device_printf(sc->sc_dev, "unknown command: \"%s\"\n", name);

	return (-1);
}

static void
vmt_tclo_tick(void *xarg)
{
	struct vmt_softc *sc = xarg;
	u_int32_t rlen;
	u_int16_t ack;
	int delay;

	/* By default, poll every second for new messages */
	delay = 1;
	
	/* reopen tclo channel if it's currently closed */
	if (sc->sc_tclo_rpc.channel == 0 &&
	    sc->sc_tclo_rpc.cookie1 == 0 &&
	    sc->sc_tclo_rpc.cookie2 == 0) {
		if (vm_rpc_open(&sc->sc_tclo_rpc, VM_RPC_OPEN_TCLO) != 0) {
			device_printf(sc->sc_dev,
			    "unable to reopen TCLO channel\n");
			delay = 15;
			goto out;
		}

		if (vm_rpc_send_str(&sc->sc_tclo_rpc,
		    VM_RPC_RESET_REPLY) != 0) {
			device_printf(sc->sc_dev,
			    "failed to send reset reply\n");
			sc->sc_rpc_error = 1;
			goto out;
		} else {
			sc->sc_rpc_error = 0;
		}
	}

	if (sc->sc_tclo_ping) {
		if (vm_rpc_send(&sc->sc_tclo_rpc, NULL, 0) != 0) {
			device_printf(sc->sc_dev,
			    "failed to send TCLO outgoing ping\n");
			sc->sc_rpc_error = 1;
			goto out;
		}
	}

	if (vm_rpc_get_length(&sc->sc_tclo_rpc, &rlen, &ack) != 0) {
		device_printf(sc->sc_dev,
		    "failed to get length of incoming TCLO data\n");
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
		device_printf(sc->sc_dev,
		    "failed to get incoming TCLO data\n");
		sc->sc_rpc_error = 1;
		goto out;
	}
	sc->sc_tclo_ping = 0;

	/* The VM host can queue multiple messages; continue without delay */
	delay = 0;

#ifdef VMT_DEBUG
	printf("vmware: received message '%s'\n", sc->sc_rpc_buf);
#endif

	if (vmt_tclo_process(sc, sc->sc_rpc_buf) != 0) {
		if (vm_rpc_send_str(&sc->sc_tclo_rpc,
		    VM_RPC_REPLY_ERROR) != 0) {
			device_printf(sc->sc_dev,
			    "error sending unknown command reply\n");
			sc->sc_rpc_error = 1;
		}
	}

	if (sc->sc_rpc_error == 1) {
		/* On error, give time to recover and wait a second */
		delay = 1;
	}

out:
	callout_schedule(&sc->sc_tclo_tick, hz * delay);
}

static void
vm_cmd(struct vm_backdoor *frame)
{
	BACKDOOR_OP(BACKDOOR_OP_CMD, frame);
}

static void
vm_ins(struct vm_backdoor *frame)
{
	BACKDOOR_OP(BACKDOOR_OP_IN, frame);
}

static void
vm_outs(struct vm_backdoor *frame)
{
	BACKDOOR_OP(BACKDOOR_OP_OUT, frame);
}

static int
vm_rpc_open(struct vm_rpc *rpc, uint32_t proto)
{
	struct vm_backdoor frame;

	memset(&frame, 0, sizeof(frame));
	frame.eax = VM_MAGIC;
	frame.ebx = proto | VM_RPC_FLAG_COOKIE;
	frame.ecx = VM_REG_CMD_RPC(VM_RPC_OPEN);
	frame.edx = VM_REG_PORT_CMD(0);

	vm_cmd(&frame);

	if (__SHIFTOUT(frame.ecx, VM_REG_HIGH_MASK) != 1 ||
	    __SHIFTOUT(frame.edx, VM_REG_LOW_MASK) != 0) {
		/* open-vm-tools retries without VM_RPC_FLAG_COOKIE here.. */
		printf("vmware: open failed, eax=%#"PRIxREGISTER
		    ", ecx=%#"PRIxREGISTER", edx=%#"PRIxREGISTER"\n",
		    frame.eax, frame.ecx, frame.edx);
		return EIO;
	}

	rpc->channel = __SHIFTOUT(frame.edx, VM_REG_HIGH_MASK);
	rpc->cookie1 = __SHIFTOUT(frame.esi, VM_REG_WORD_MASK);
	rpc->cookie2 = __SHIFTOUT(frame.edi, VM_REG_WORD_MASK);

	return 0;
}

static int
vm_rpc_close(struct vm_rpc *rpc)
{
	struct vm_backdoor frame;

	memset(&frame, 0, sizeof(frame));
	frame.eax = VM_MAGIC;
	frame.ebx = 0;
	frame.ecx = VM_REG_CMD_RPC(VM_RPC_CLOSE);
	frame.edx = VM_REG_PORT_CMD(rpc->channel);
	frame.edi = rpc->cookie2;
	frame.esi = rpc->cookie1;

	vm_cmd(&frame);

	if (__SHIFTOUT(frame.ecx, VM_REG_HIGH_MASK) == 0 ||
	    __SHIFTOUT(frame.ecx, VM_REG_LOW_MASK) != 0) {
		printf("vmware: close failed, "
		    "eax=%#"PRIxREGISTER", ecx=%#"PRIxREGISTER"\n",
		    frame.eax, frame.ecx);
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
	frame.eax = VM_MAGIC;
	frame.ebx = length;
	frame.ecx = VM_REG_CMD_RPC(VM_RPC_SET_LENGTH);
	frame.edx = VM_REG_PORT_CMD(rpc->channel);
	frame.esi = rpc->cookie1;
	frame.edi = rpc->cookie2;

	vm_cmd(&frame);

	if ((__SHIFTOUT(frame.ecx, VM_REG_HIGH_MASK) & VM_RPC_REPLY_SUCCESS) ==
	    0) {
		printf("vmware: sending length failed, "
		    "eax=%#"PRIxREGISTER", ecx=%#"PRIxREGISTER"\n",
		    frame.eax, frame.ecx);
		return EIO;
	}

	if (length == 0)
		return 0; /* Only need to poke once if command is null. */

	/* Send the command using enhanced RPC. */
	memset(&frame, 0, sizeof(frame));
	frame.eax = VM_MAGIC;
	frame.ebx = VM_RPC_ENH_DATA;
	frame.ecx = length;
	frame.edx = VM_REG_PORT_RPC(rpc->channel);
	frame.ebp = rpc->cookie1;
	frame.edi = rpc->cookie2;
	frame.esi = (register_t)buf;

	vm_outs(&frame);

	if (__SHIFTOUT(frame.ebx, VM_REG_WORD_MASK) != VM_RPC_ENH_DATA) {
		/* open-vm-tools retries on VM_RPC_REPLY_CHECKPOINT */
		printf("vmware: send failed, ebx=%#"PRIxREGISTER"\n",
		    frame.ebx);
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
	frame.eax = VM_MAGIC;
	frame.ebx = VM_RPC_ENH_DATA;
	frame.ecx = length;
	frame.edx = VM_REG_PORT_RPC(rpc->channel);
	frame.esi = rpc->cookie1;
	frame.edi = (register_t)data;
	frame.ebp = rpc->cookie2;

	vm_ins(&frame);

	/* NUL-terminate the data */
	data[length] = '\0';

	if (__SHIFTOUT(frame.ebx, VM_REG_WORD_MASK) != VM_RPC_ENH_DATA) {
		printf("vmware: get data failed, ebx=%#"PRIxREGISTER"\n",
		    frame.ebx);
		return EIO;
	}

	/* Acknowledge data received. */
	memset(&frame, 0, sizeof(frame));
	frame.eax = VM_MAGIC;
	frame.ebx = dataid;
	frame.ecx = VM_REG_CMD_RPC(VM_RPC_GET_END);
	frame.edx = VM_REG_PORT_CMD(rpc->channel);
	frame.esi = rpc->cookie1;
	frame.edi = rpc->cookie2;

	vm_cmd(&frame);

	if (__SHIFTOUT(frame.ecx, VM_REG_HIGH_MASK) == 0) {
		printf("vmware: ack data failed, "
		    "eax=%#"PRIxREGISTER", ecx=%#"PRIxREGISTER"\n",
		    frame.eax, frame.ecx);
		return EIO;
	}

	return 0;
}

static int
vm_rpc_get_length(const struct vm_rpc *rpc, uint32_t *length, uint16_t *dataid)
{
	struct vm_backdoor frame;

	memset(&frame, 0, sizeof(frame));
	frame.eax = VM_MAGIC;
	frame.ebx = 0;
	frame.ecx = VM_REG_CMD_RPC(VM_RPC_GET_LENGTH);
	frame.edx = VM_REG_PORT_CMD(rpc->channel);
	frame.esi = rpc->cookie1;
	frame.edi = rpc->cookie2;

	vm_cmd(&frame);

	if ((__SHIFTOUT(frame.ecx, VM_REG_HIGH_MASK) & VM_RPC_REPLY_SUCCESS) ==
	    0) {
		printf("vmware: get length failed, "
		    "eax=%#"PRIxREGISTER", ecx=%#"PRIxREGISTER"\n",
		    frame.eax, frame.ecx);
		return EIO;
	}
	if ((__SHIFTOUT(frame.ecx, VM_REG_HIGH_MASK) & VM_RPC_REPLY_DORECV) ==
	    0) {
		*length = 0;
		*dataid = 0;
	} else {
		*length = __SHIFTOUT(frame.ebx, VM_REG_WORD_MASK);
		*dataid = __SHIFTOUT(frame.edx, VM_REG_HIGH_MASK);
	}

	return 0;
}

static int
vm_rpci_response_successful(struct vmt_softc *sc)
{
	return (sc->sc_rpc_buf[0] == '1' && sc->sc_rpc_buf[1] == ' ');
}

static int
vm_rpc_send_rpci_tx_buf(struct vmt_softc *sc, const uint8_t *buf,
    uint32_t length)
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
		device_printf(sc->sc_dev,
		    "failed to get length of rpci response data\n");
		result = EIO;
		goto out;
	}

	if (rlen > 0) {
		if (rlen >= VMT_RPC_BUFLEN) {
			rlen = VMT_RPC_BUFLEN - 1;
		}

		if (vm_rpc_get_data(&rpci, sc->sc_rpc_buf, rlen, ack) != 0) {
			device_printf(sc->sc_dev,
			    "failed to get rpci response data\n");
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
		device_printf(sc->sc_dev,
		    "rpci command didn't fit in buffer\n");
		return EIO;
	}

	return vm_rpc_send_rpci_tx_buf(sc, sc->sc_rpc_buf, len);
}

#if 0
	struct vm_backdoor frame;

	memset(&frame, 0, sizeof(frame));

	frame.eax = VM_MAGIC;
	frame.ecx = VM_CMD_GET_VERSION;
	frame.edx = VM_PORT_CMD;

	printf("\n");
	printf("eax %#"PRIxREGISTER"\n", frame.eax);
	printf("ebx %#"PRIxREGISTER"\n", frame.ebx);
	printf("ecx %#"PRIxREGISTER"\n", frame.ecx);
	printf("edx %#"PRIxREGISTER"\n", frame.edx)
	printf("ebp %#"PRIxREGISTER"\n", frame.ebp);
	printf("edi %#"PRIxREGISTER"\n", frame.edi);
	printf("esi %#"PRIxREGISTER"\n", frame.esi);

	vm_cmd(&frame);

	printf("-\n");
	printf("eax %#"PRIxREGISTER"\n", frame.eax);
	printf("ebx %#"PRIxREGISTER"\n", frame.ebx);
	printf("ecx %#"PRIxREGISTER"\n", frame.ecx);
	printf("edx %#"PRIxREGISTER"\n", frame.edx);
	printf("ebp %#"PRIxREGISTER"\n", frame.ebp);
	printf("edi %#"PRIxREGISTER"\n", frame.edi);
	printf("esi %#"PRIxREGISTER"\n", frame.esi);
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
