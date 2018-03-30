/*	$NetBSD: pmu.c,v 1.30.2.1 2018/03/30 06:20:11 pgoyette Exp $ */

/*-
 * Copyright (c) 2006 Michael Lorenz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
__KERNEL_RCSID(0, "$NetBSD: pmu.c,v 1.30.2.1 2018/03/30 06:20:11 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/atomic.h>
#include <sys/mutex.h>

#include <sys/bus.h>
#include <machine/pio.h>
#include <machine/autoconf.h>
#include <dev/clock_subr.h>
#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>

#include <macppc/dev/viareg.h>
#include <macppc/dev/pmuvar.h>
#include <macppc/dev/batteryvar.h>

#include <dev/ofw/openfirm.h>
#include <dev/adb/adbvar.h>
#include "opt_pmu.h"
#include "nadb.h"

#ifdef PMU_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

#define PMU_NOTREADY	0x1	/* has not been initialized yet */
#define PMU_IDLE	0x2	/* the bus is currently idle */
#define PMU_OUT		0x3	/* sending out a command */
#define PMU_IN		0x4	/* receiving data */

static void pmu_attach(device_t, device_t, void *);
static int pmu_match(device_t, cfdata_t, void *);
static void pmu_autopoll(void *, int);

static int pmu_intr(void *);


/* bits for sc_pending, as signals to the event thread */
#define PMU_EV_CARD0	1
#define PMU_EV_CARD1	2
#define PMU_EV_BUTTON	4
#define PMU_EV_LID	8

struct pmu_softc {
	device_t sc_dev;
	void *sc_ih;
	struct todr_chip_handle sc_todr;
	struct adb_bus_accessops sc_adbops;
	struct i2c_controller sc_i2c;
	kmutex_t sc_i2c_lock;
	struct pmu_ops sc_pmu_ops;
	struct sysmon_pswitch sc_lidswitch;
	struct sysmon_pswitch sc_powerbutton;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	uint32_t sc_flags;
#define PMU_HAS_BACKLIGHT_CONTROL	1
	int sc_node;
	int sc_error;
	int sc_autopoll;
	int sc_brightness, sc_brightness_wanted;
	int sc_volume, sc_volume_wanted;
	int sc_lid_closed;
	int sc_button;
	uint8_t sc_env_old;
	uint8_t sc_env_mask;
	/* deferred processing */
	lwp_t *sc_thread;
	int sc_pending;
	/* signalling the event thread */
	int sc_event;
	/* ADB */
	void (*sc_adb_handler)(void *, int, uint8_t *);
	void *sc_adb_cookie;
	void (*sc_callback)(void *);
	void *sc_cb_cookie;
};

CFATTACH_DECL_NEW(pmu, sizeof(struct pmu_softc),
    pmu_match, pmu_attach, NULL, NULL);

static inline void pmu_write_reg(struct pmu_softc *, int, uint8_t);
static inline uint8_t pmu_read_reg(struct pmu_softc *, int);
static void pmu_in(struct pmu_softc *);
static void pmu_out(struct pmu_softc *);
static void pmu_ack_off(struct pmu_softc *);
static void pmu_ack_on(struct pmu_softc *);
static int pmu_intr_state(struct pmu_softc *);

static void pmu_init(struct pmu_softc *);
static void pmu_thread(void *);
static void pmu_eject_card(struct pmu_softc *, int);
static void pmu_update_brightness(struct pmu_softc *);
static void pmu_register_callback(void *, void (*)(void *), void *);
/*
 * send a message to the PMU.
 */
static int pmu_send(void *, int, int, uint8_t *, int, uint8_t *);
static void pmu_adb_poll(void *);
static int pmu_todr_set(todr_chip_handle_t, struct timeval *);
static int pmu_todr_get(todr_chip_handle_t, struct timeval *);

static int pmu_adb_handler(void *, int, uint8_t *);

static struct pmu_softc *pmu0 = NULL;

/* ADB bus attachment stuff */
static 	int pmu_adb_send(void *, int, int, int, uint8_t *);
static	int pmu_adb_set_handler(void *, void (*)(void *, int, uint8_t *), void *);

/* i2c stuff */
static int pmu_i2c_acquire_bus(void *, int);
static void pmu_i2c_release_bus(void *, int);
static int pmu_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
		    void *, size_t, int);

static void pmu_attach_legacy_battery(struct pmu_softc *);
static void pmu_attach_smart_battery(struct pmu_softc *, int);
static int  pmu_print(void *, const char *);

/* these values shows that number of data returned after 'send' cmd is sent */
static signed char pm_send_cmd_type[] = {
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x01, 0x01,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00,   -1,   -1,   -1,   -1,   -1, 0x00,
	  -1, 0x00, 0x02, 0x01, 0x01,   -1,   -1,   -1,
	0x00,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x04, 0x14,   -1, 0x03,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x02, 0x02,   -1,   -1,   -1,   -1,
	0x01, 0x01,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00,   -1,   -1, 0x01,   -1,   -1,   -1,
	0x01, 0x00, 0x02, 0x02,   -1, 0x01, 0x03, 0x01,
	0x00, 0x01, 0x00, 0x00, 0x00,   -1,   -1,   -1,
	0x02,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   -1,   -1,
	0x01, 0x01, 0x01,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00,   -1,   -1,   -1,   -1, 0x04, 0x04,
	0x04,   -1, 0x00,   -1,   -1,   -1,   -1,   -1,
	0x00,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x01, 0x02,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00,   -1,   -1,   -1,   -1,   -1,   -1,
	0x02, 0x02, 0x02, 0x04,   -1, 0x00,   -1,   -1,
	0x01, 0x01, 0x03, 0x02,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x01, 0x01,   -1,   -1, 0x00, 0x00,   -1,   -1,
	  -1, 0x04, 0x00,   -1,   -1,   -1,   -1,   -1,
	0x03,   -1, 0x00,   -1, 0x00,   -1,   -1, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1
};

/* these values shows that number of data returned after 'receive' cmd is sent */
static signed char pm_receive_cmd_type[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02,   -1,   -1,   -1,   -1,   -1, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x05, 0x15,   -1, 0x02,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x03, 0x03,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x04, 0x04, 0x03, 0x09,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1, 0x01, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x06,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02,   -1,   -1, 0x02,   -1,   -1,   -1,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1, 0x02,   -1,   -1,   -1,   -1, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
};

static const char *has_legacy_battery[] = {
	"AAPL,3500",
	"AAPL,3400/2400",
	NULL };

static const char *has_two_smart_batteries[] = {
	"AAPL,PowerBook1998",
	"PowerBook1,1",
	NULL };

static int
pmu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (ca->ca_nreg < 8)
		return 0;

	if (ca->ca_nintr < 4)
		return 0;

	if (strcmp(ca->ca_name, "via-pmu") == 0) {
		return 10;
	}
	
	return 0;
}

static void
pmu_attach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux;
	struct pmu_softc *sc = device_private(self);
	struct i2cbus_attach_args iba;
	uint32_t regs[16];
	int irq = ca->ca_intr[0];
	int node, extint_node, root_node;
	int nbat = 1, i, pmnode;
	int type = IST_EDGE;
	uint8_t cmd[2] = {2, 0};
	uint8_t resp[16];
	char name[256], model[32];
	prop_dictionary_t dict = device_properties(self);

	extint_node = of_getnode_byname(OF_parent(ca->ca_node), "extint-gpio1");
	if (extint_node) {

		OF_getprop(extint_node, "interrupts", &irq, 4);
		type = IST_LEVEL;
	}

	aprint_normal(" irq %d: ", irq);

	sc->sc_dev = self;
	sc->sc_node = ca->ca_node;
	sc->sc_memt = ca->ca_tag;

	root_node = OF_finddevice("/");

	sc->sc_error = 0;
	sc->sc_autopoll = 0;
	sc->sc_pending = 0;
	sc->sc_env_old = 0;
	sc->sc_brightness = sc->sc_brightness_wanted = 0x80;
	sc->sc_volume = sc->sc_volume_wanted = 0x80;
	sc->sc_flags = 0;
	sc->sc_callback = NULL;
	sc->sc_lid_closed = 0;
	sc->sc_button = 0;
	sc->sc_env_mask = 0xff;

	/*
	 * core99 PowerMacs like to send environment messages with the lid
	 * switch bit set - since that doesn't make any sense here and it
	 * probably means something else anyway we mask it out
	 */

	if (OF_getprop(root_node, "model", model, 32) != 0) {
		if (strncmp(model, "PowerMac", 8) == 0) {
			sc->sc_env_mask = PMU_ENV_POWER_BUTTON;
		}
	}

	if (bus_space_map(sc->sc_memt, ca->ca_reg[0] + ca->ca_baseaddr,
	    ca->ca_reg[1], 0, &sc->sc_memh) != 0) {
		aprint_error_dev(self, "unable to map registers\n");
		return;
	}
	sc->sc_ih = intr_establish(irq, type, IPL_TTY, pmu_intr, sc);

	pmu_init(sc);

	sc->sc_pmu_ops.cookie = sc;
	sc->sc_pmu_ops.do_command = pmu_send;
	sc->sc_pmu_ops.register_callback = pmu_register_callback;

	if (pmu0 == NULL)
		pmu0 = sc;

	pmu_send(sc, PMU_SYSTEM_READY, 1, cmd, 16, resp);

	/* check what kind of PMU we're talking to */
	if (pmu_send(sc, PMU_GET_VERSION, 0, cmd, 16, resp) > 1)
		aprint_normal(" rev. %d", resp[1]);
	aprint_normal("\n");

	node = OF_child(sc->sc_node);

	while (node != 0) {

		if (OF_getprop(node, "name", name, 256) <= 0)
			goto next;

		if (strncmp(name, "pmu-i2c", 8) == 0) {
			int devs;
			uint32_t addr;
			char compat[256];
			prop_array_t cfg;
			prop_dictionary_t dev;
			prop_data_t data;

			aprint_normal_dev(self, "initializing IIC bus\n");

			cfg = prop_array_create();
			prop_dictionary_set(dict, "i2c-child-devices", cfg);
			prop_object_release(cfg);

			/* look for i2c devices */
			devs = OF_child(node);
			while (devs != 0) {
				if (OF_getprop(devs, "name", name, 256) <= 0)
					goto skip;
				if (OF_getprop(devs, "compatible",
				    compat, 256) <= 0)
					goto skip;
				if (OF_getprop(devs, "reg", &addr, 4) <= 0)
					goto skip;
				addr = (addr & 0xff) >> 1;
				DPRINTF("-> %s@%x\n", name, addr);
				dev = prop_dictionary_create();
				prop_dictionary_set_cstring(dev, "name", name);
				data = prop_data_create_data(compat, strlen(compat)+1);
				prop_dictionary_set(dev, "compatible", data);
				prop_object_release(data);
				prop_dictionary_set_uint32(dev, "addr", addr);
				prop_dictionary_set_uint64(dev, "cookie", devs);
				prop_array_add(cfg, dev);
				prop_object_release(dev);
			skip:
				devs = OF_peer(devs);
			}
			memset(&iba, 0, sizeof(iba));
			iba.iba_tag = &sc->sc_i2c;
			mutex_init(&sc->sc_i2c_lock, MUTEX_DEFAULT, IPL_NONE);
			sc->sc_i2c.ic_cookie = sc;
			sc->sc_i2c.ic_acquire_bus = pmu_i2c_acquire_bus;
			sc->sc_i2c.ic_release_bus = pmu_i2c_release_bus;
			sc->sc_i2c.ic_send_start = NULL;
			sc->sc_i2c.ic_send_stop = NULL;
			sc->sc_i2c.ic_initiate_xfer = NULL;
			sc->sc_i2c.ic_read_byte = NULL;
			sc->sc_i2c.ic_write_byte = NULL;
			sc->sc_i2c.ic_exec = pmu_i2c_exec;
			config_found_ia(sc->sc_dev, "i2cbus", &iba,
			    iicbus_print);
			goto next;
		}
		if (strncmp(name, "adb", 4) == 0) {
			aprint_normal_dev(self, "initializing ADB\n");
			sc->sc_adbops.cookie = sc;
			sc->sc_adbops.send = pmu_adb_send;
			sc->sc_adbops.poll = pmu_adb_poll;
			sc->sc_adbops.autopoll = pmu_autopoll;
			sc->sc_adbops.set_handler = pmu_adb_set_handler;
#if NNADB > 0
			config_found_ia(self, "adb_bus", &sc->sc_adbops,
			    nadb_print);
#endif
			goto next;
		}
		if (strncmp(name, "rtc", 4) == 0) {

			aprint_normal_dev(self, "initializing RTC\n");
			sc->sc_todr.todr_gettime = pmu_todr_get;
			sc->sc_todr.todr_settime = pmu_todr_set;
			sc->sc_todr.cookie = sc;
			todr_attach(&sc->sc_todr);
			goto next;
		}
		if (strncmp(name, "battery", 8) == 0)
			goto next;

		aprint_normal_dev(self, "%s not configured\n", name);
next:
		node = OF_peer(node);
	}

	if (OF_finddevice("/bandit/ohare") != -1) {
		aprint_normal_dev(self, "enabling ohare backlight control\n");
		sc->sc_flags |= PMU_HAS_BACKLIGHT_CONTROL;
		cmd[0] = 0;
		cmd[1] = 0;
		memset(resp, 0, 6);
		if (pmu_send(sc, PMU_READ_BRIGHTNESS, 1, cmd, 16, resp) > 1) {
			sc->sc_brightness_wanted = resp[1];
			pmu_update_brightness(sc);
		}
	}

	/* attach batteries */
	if (of_compatible(root_node, has_legacy_battery) != -1) {

		pmu_attach_legacy_battery(sc);
	} else if (of_compatible(root_node, has_two_smart_batteries) != -1) {

		pmu_attach_smart_battery(sc, 0);
		pmu_attach_smart_battery(sc, 1);
	} else {

		/* check how many batteries we have */
		pmnode = of_getnode_byname(ca->ca_node, "power-mgt");
		if (pmnode == -1)
			goto bat_done;
		if (OF_getprop(pmnode, "prim-info", regs, sizeof(regs)) < 24)
			goto bat_done;
		nbat = regs[6] >> 16;
		for (i = 0; i < nbat; i++)
			pmu_attach_smart_battery(sc, i);
	}
bat_done:
	
	if (kthread_create(PRI_NONE, 0, NULL, pmu_thread, sc, &sc->sc_thread,
	    "%s", "pmu") != 0) {
		aprint_error_dev(self, "unable to create event kthread\n");
	}

	sc->sc_lidswitch.smpsw_name = "Lid switch";
	sc->sc_lidswitch.smpsw_type = PSWITCH_TYPE_LID;
	if (sysmon_pswitch_register(&sc->sc_lidswitch) != 0)
		aprint_error_dev(self,
		    "unable to register lid switch with sysmon\n");

	sc->sc_powerbutton.smpsw_name = "Power button";
	sc->sc_powerbutton.smpsw_type = PSWITCH_TYPE_POWER;
	if (sysmon_pswitch_register(&sc->sc_powerbutton) != 0)
		aprint_error_dev(self,
		    "unable to register power button with sysmon\n");
}

static void
pmu_register_callback(void *pmu_cookie, void (*cb)(void *), void *cookie)
{
	struct pmu_softc *sc = pmu_cookie;

	sc->sc_callback = cb;
	sc->sc_cb_cookie = cookie;
}

static void
pmu_init(struct pmu_softc *sc)
{
	uint8_t pmu_imask, resp[16];

	pmu_imask =
	    PMU_INT_PCEJECT | PMU_INT_SNDBRT | PMU_INT_ADB/* | PMU_INT_TICK*/;
	pmu_imask |= PMU_INT_BATTERY;
	pmu_imask |= PMU_INT_ENVIRONMENT;
	pmu_send(sc, PMU_SET_IMASK, 1, &pmu_imask, 16, resp);

	pmu_write_reg(sc, vIER, 0x90);	/* enable VIA interrupts */
}

static inline void
pmu_write_reg(struct pmu_softc *sc, int offset, uint8_t value)
{

	bus_space_write_1(sc->sc_memt, sc->sc_memh, offset, value);
}

static inline uint8_t
pmu_read_reg(struct pmu_softc *sc, int offset)
{

	return bus_space_read_1(sc->sc_memt, sc->sc_memh, offset);
}

static inline int
pmu_send_byte(struct pmu_softc *sc, uint8_t data)
{

	pmu_out(sc);
	pmu_write_reg(sc, vSR, data);
	pmu_ack_off(sc);
	/* wait for intr to come up */
	/* XXX should add a timeout and bail if it expires */
	do {} while (pmu_intr_state(sc) == 0);
	pmu_ack_on(sc);
	do {} while (pmu_intr_state(sc));
	pmu_ack_on(sc);
	DPRINTF(" %02x>", data);
	return 0;
}

static inline int
pmu_read_byte(struct pmu_softc *sc, uint8_t *data)
{
	pmu_in(sc);
	(void)pmu_read_reg(sc, vSR);
	pmu_ack_off(sc);
	/* wait for intr to come up */
	do {} while (pmu_intr_state(sc) == 0);
	pmu_ack_on(sc);
	do {} while (pmu_intr_state(sc));
	*data = pmu_read_reg(sc, vSR);
	DPRINTF(" <%02x", *data);
	return 0;
}

static int
pmu_send(void *cookie, int cmd, int length, uint8_t *in_msg, int rlen,
    uint8_t *out_msg)
{
	struct pmu_softc *sc = cookie;
	int i, rcv_len = -1, s;
	uint8_t out_len, intreg;

	DPRINTF("pmu_send: ");

	s = splhigh();
	intreg = pmu_read_reg(sc, vIER);
	intreg &= 0x10;
	pmu_write_reg(sc, vIER, intreg);

	/* wait idle */
	do {} while (pmu_intr_state(sc));
	sc->sc_error = 0;

	/* send command */
	pmu_send_byte(sc, cmd);

	/* send length if necessary */
	if (pm_send_cmd_type[cmd] < 0) {
		pmu_send_byte(sc, length);
	}

	for (i = 0; i < length; i++) {
		pmu_send_byte(sc, in_msg[i]);
		DPRINTF(" next ");
	}
	DPRINTF("done sending\n");

	/* see if there's data to read */
	rcv_len = pm_receive_cmd_type[cmd];
	if (rcv_len == 0) 
		goto done;

	/* read command */
	if (rcv_len == 1) {
		pmu_read_byte(sc, out_msg);
		goto done;
	} else
		out_msg[0] = cmd;
	if (rcv_len < 0) {
		pmu_read_byte(sc, &out_len);
		rcv_len = out_len + 1;
	}
	for (i = 1; i < min(rcv_len, rlen); i++)
		pmu_read_byte(sc, &out_msg[i]);

done:
	DPRINTF("\n");
	pmu_write_reg(sc, vIER, (intreg == 0) ? 0 : 0x90);
	splx(s);

	return rcv_len;
}

static void
pmu_adb_poll(void *cookie)
{
	struct pmu_softc *sc = cookie;
	int s;

	s = spltty();
	pmu_intr(sc);
	splx(s);
}

static void
pmu_in(struct pmu_softc *sc)
{
	uint8_t reg;

	reg = pmu_read_reg(sc, vACR);
	reg &= ~vSR_OUT;
	reg |= 0x0c;
	pmu_write_reg(sc, vACR, reg);
}

static void
pmu_out(struct pmu_softc *sc)
{
	uint8_t reg;

	reg = pmu_read_reg(sc, vACR);
	reg |= vSR_OUT;
	reg |= 0x0c;
	pmu_write_reg(sc, vACR, reg);
}

static void
pmu_ack_off(struct pmu_softc *sc)
{
	uint8_t reg;

	reg = pmu_read_reg(sc, vBufB);
	reg &= ~vPB4;
	pmu_write_reg(sc, vBufB, reg);
}

static void
pmu_ack_on(struct pmu_softc *sc)
{
	uint8_t reg;

	reg = pmu_read_reg(sc, vBufB);
	reg |= vPB4;
	pmu_write_reg(sc, vBufB, reg);
}

static int
pmu_intr_state(struct pmu_softc *sc)
{
	return ((pmu_read_reg(sc, vBufB) & vPB3) == 0);
}

static int
pmu_intr(void *arg)
{
	struct pmu_softc *sc = arg;
	unsigned int len, i;
	uint8_t resp[16];

	DPRINTF(":");

	pmu_write_reg(sc, vIFR, 0x90);	/* Clear 'em */
	len = pmu_send(sc, PMU_INT_ACK, 0, NULL, 16, resp);
	if ((len < 1) || (resp[1] == 0))
		goto done;
#ifdef PMU_DEBUG
	{
		DPRINTF("intr: %02x", resp[0]);
		for (i = 1; i < len; i++)
			DPRINTF(" %02x", resp[i]);
		DPRINTF("\n");
	}
#endif
	if (resp[1] & PMU_INT_ADB) {
		pmu_adb_handler(sc, len - 1, &resp[1]);
		goto done;
	}
	if (resp[1] & PMU_INT_SNDBRT) {
		/* deal with the brightness / volume control buttons */
		DPRINTF("brightness: %d volume %d\n", resp[2], resp[3]);
		sc->sc_brightness_wanted = resp[2];
		sc->sc_volume_wanted = resp[3];
		wakeup(&sc->sc_event);
		goto done;
	}
	if (resp[1] & PMU_INT_PCEJECT) {
		/* deal with PCMCIA eject buttons */
		DPRINTF("card eject %d\n", resp[3]);
		atomic_or_32(&sc->sc_pending, (resp[3] & 3));
		wakeup(&sc->sc_event);
		goto done;
	}
	if (resp[1] & PMU_INT_BATTERY) {
		/* deal with battery messages */
		printf("battery:");
		for (i = 2; i < len; i++)
			printf(" %02x", resp[i]);
		printf("\n");
		goto done;
	}
	if (resp[1] & PMU_INT_ENVIRONMENT) {
		uint8_t diff;
#ifdef PMU_VERBOSE
		/* deal with environment messages */
		printf("environment:");
		for (i = 2; i < len; i++)
			printf(" %02x", resp[i]);
		printf("\n");
#endif
		diff = (resp[2] ^ sc->sc_env_old ) & sc->sc_env_mask;
		if (diff == 0) goto done;
		sc->sc_env_old = resp[2];
		if (diff & PMU_ENV_LID_CLOSED) {
			sc->sc_lid_closed = (resp[2] & PMU_ENV_LID_CLOSED) != 0;
			atomic_or_32(&sc->sc_pending, PMU_EV_LID);
			wakeup(&sc->sc_event);
		}
		if (diff & PMU_ENV_POWER_BUTTON) {
			sc->sc_button = (resp[2] & PMU_ENV_POWER_BUTTON) != 0;
			atomic_or_32(&sc->sc_pending, PMU_EV_BUTTON);
			wakeup(&sc->sc_event);
		}
		goto done;
	}
	if (resp[1] & PMU_INT_TICK) {
		/* don't bother */
		goto done;
	}

	/* unknown interrupt code?! */
#ifdef PMU_DEBUG
	printf("pmu intr: %02x:", resp[1]);
	for (i = 2; i < len; i++)
		printf(" %02x", resp[i]);
	printf("\n");
#endif
done:
	return 1;
}

#if 0
static int
pmu_error_handler(void *cookie, int len, uint8_t *data)
{
	struct pmu_softc *sc = cookie;

	/* 
	 * something went wrong
	 * byte 3 seems to be the failed command
	 */
	sc->sc_error = 1;
	wakeup(&sc->sc_todev);
	return 0;
}
#endif
#define DIFF19041970 2082844800

static int
pmu_todr_get(todr_chip_handle_t tch, struct timeval *tvp)
{
	struct pmu_softc *sc = tch->cookie;
	uint32_t sec;
	int count = 10;
	int ok = FALSE;
	uint8_t resp[16];

	DPRINTF("pmu_todr_get\n");
	while ((count > 0) && (!ok)) {
		pmu_send(sc, PMU_READ_RTC, 0, NULL, 16, resp);

		memcpy(&sec, &resp[1], 4);
		tvp->tv_sec = sec - DIFF19041970;
		ok = (sec > DIFF19041970) && (sec < 0xf0000000);
		if (!ok) aprint_error_dev(sc->sc_dev,
		    "got garbage from rtc (%08x)\n", sec);
		count--;
	}
	if (count == 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to get a sane time value\n");
		tvp->tv_sec = 0;
	}
	DPRINTF("tod: %" PRIo64 "\n", tvp->tv_sec);
	tvp->tv_usec = 0;
	return 0;
}

static int
pmu_todr_set(todr_chip_handle_t tch, struct timeval *tvp)
{
	struct pmu_softc *sc = tch->cookie;
	uint32_t sec;
	uint8_t resp[16];

	sec = tvp->tv_sec + DIFF19041970;
	if (pmu_send(sc, PMU_SET_RTC, 4, (uint8_t *)&sec, 16, resp) >= 0)
		return 0;
	return -1;		
}

void
pmu_poweroff(void)
{
	struct pmu_softc *sc;
	uint8_t cmd[] = {'M', 'A', 'T', 'T'};
	uint8_t resp[16];

	if (pmu0 == NULL)
		return;
	sc = pmu0;
	if (pmu_send(sc, PMU_POWER_OFF, 4, cmd, 16, resp) >= 0)
		while (1);
}

void
pmu_restart(void)
{
	struct pmu_softc *sc;
	uint8_t resp[16];

	if (pmu0 == NULL)
		return;
	sc = pmu0;
	if (pmu_send(sc, PMU_RESET_CPU, 0, NULL, 16, resp) >= 0)
		while (1);
}

void
pmu_modem(int on)
{
	struct pmu_softc *sc;
	uint8_t resp[16], cmd[2] = {0, 0};

	if (pmu0 == NULL)
		return;

	sc = pmu0;
	cmd[0] = PMU_POW0_MODEM | (on ? PMU_POW0_ON : 0);
	pmu_send(sc, PMU_POWER_CTRL0, 1, cmd, 16, resp);
}

static void
pmu_autopoll(void *cookie, int flag)
{
	struct pmu_softc *sc = cookie;
	/* magical incantation to re-enable autopolling */
	uint8_t cmd[] = {0, PMU_SET_POLL_MASK, (flag >> 8) & 0xff, flag & 0xff};
	uint8_t resp[16];

	if (sc->sc_autopoll == flag)
		return;

	if (flag) {
		pmu_send(sc, PMU_ADB_CMD, 4, cmd, 16, resp);
	} else {
		pmu_send(sc, PMU_ADB_POLL_OFF, 0, NULL, 16, resp);
	}
	sc->sc_autopoll = flag & 0xffff;
}

static int
pmu_adb_handler(void *cookie, int len, uint8_t *data)
{
	struct pmu_softc *sc = cookie;
	uint8_t resp[16];

	if (sc->sc_adb_handler != NULL) {
		sc->sc_adb_handler(sc->sc_adb_cookie, len, data);
		/*
		 * the PMU will turn off autopolling after each LISTEN so we
		 * need to re-enable it here whenever we receive an ACK for a
		 * LISTEN command
		 */
		if ((data[1] & 0x0c) == 0x08) {
			uint8_t cmd[] = {0, 0x86, (sc->sc_autopoll >> 8) & 0xff,
			    sc->sc_autopoll & 0xff};
			pmu_send(sc, PMU_ADB_CMD, 4, cmd, 16, resp);
		}
		return 0;
	}
	return -1;
}

static int
pmu_adb_send(void *cookie, int poll, int command, int len, uint8_t *data)
{
	struct pmu_softc *sc = cookie;
	int i;
	uint8_t packet[16], resp[16];

	/* construct an ADB command packet and send it */
	packet[0] = command;
	packet[1] = 0;
	packet[2] = len;
	for (i = 0; i < len; i++)
		packet[i + 3] = data[i];
	(void)pmu_send(sc, PMU_ADB_CMD, len + 3, packet, 16, resp);

	return 0;
}

static int
pmu_adb_set_handler(void *cookie, void (*handler)(void *, int, uint8_t *),
    void *hcookie)
{
	struct pmu_softc *sc = cookie;

	/* register a callback for incoming ADB messages */
	sc->sc_adb_handler = handler;
	sc->sc_adb_cookie = hcookie;
	return 0;
}

static int
pmu_i2c_acquire_bus(void *cookie, int flags)
{
	struct pmu_softc *sc = cookie;

	mutex_enter(&sc->sc_i2c_lock);

	return 0;
}

static void
pmu_i2c_release_bus(void *cookie, int flags)
{
	struct pmu_softc *sc = cookie;

	mutex_exit(&sc->sc_i2c_lock);
}

static int
pmu_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *_send,
    size_t send_len, void *_recv, size_t recv_len, int flags)
{
	struct pmu_softc *sc = cookie;
	const uint8_t *send = _send;
	uint8_t command[32] = {1,	/* bus number */
				PMU_I2C_MODE_SIMPLE,
				0,	/* bus2 */
				addr,
				0,	/* sub address */
				0,	/* comb address */
				0,	/* count */
				0	/* data */
				};
	uint8_t resp[16];
	int len, rw;

	rw = addr << 1;
	command[3] = rw;
	if (send_len > 0) {
		command[6] = send_len;
		memcpy(&command[7], send, send_len);
		len = send_len + 7;
		DPRINTF("pmu_i2c_exec(%02x, %d)\n", addr, send_len);

		len = pmu_send(sc, PMU_I2C_CMD, len, command, 16, resp);
		DPRINTF("resp(%d): %2x %2x\n", len, resp[0], resp[1]);

		if (resp[1] != PMU_I2C_STATUS_OK) {
			DPRINTF("%s: iic error %d\n", __func__, resp[1]);
			return -1;
		}
	}
	/* see if we're supposed to read */
	if (I2C_OP_READ_P(op)) {
		rw |= 1;
		command[3] = rw;
		command[6] = recv_len;
		len = pmu_send(sc, PMU_I2C_CMD, 7, command, 16, resp);
		DPRINTF("resp2(%d): %2x %2x\n", len, resp[0], resp[1]);
		
		command[0] = 0;
		len = pmu_send(sc, PMU_I2C_CMD, 1, command, 16, resp);
		DPRINTF("resp3(%d): %2x %2x %2x\n", len, resp[0], resp[1],
			resp[2]);
		if ((len - 2) != recv_len) {
			DPRINTF("%s: %s(%d) - got %d\n",
			    device_xname(sc->sc_dev),
			    __func__, recv_len, len - 2);
			return -1;
		}
		memcpy(_recv, &resp[2], len - 2);
		return 0;
	};
	return 0;
}

static void
pmu_eject_card(struct pmu_softc *sc, int socket)
{
	uint8_t buf[] = {socket | 4};
	uint8_t res[4];

	atomic_and_32(&sc->sc_pending, ~socket);
	pmu_send(sc, PMU_EJECT_PCMCIA, 1, buf, 4, res);
}

static void
pmu_update_brightness(struct pmu_softc *sc)
{
	int val;
	uint8_t cmd[2], resp[16];

	if (sc->sc_brightness == sc->sc_brightness_wanted)
		return;

	if ((sc->sc_flags & PMU_HAS_BACKLIGHT_CONTROL) == 0) {

		aprint_normal_dev(sc->sc_dev,
		     "this PMU doesn't support backlight control\n");
		sc->sc_brightness = sc->sc_brightness_wanted;	
		return;
	}

	if (sc->sc_brightness_wanted == 0) {
		
		/* turn backlight off completely */
		cmd[0] = PMU_POW_OFF | PMU_POW_BACKLIGHT;
		pmu_send(sc, PMU_POWER_CTRL, 1, cmd, 16, resp);
		sc->sc_brightness = sc->sc_brightness_wanted;
		
		/* don't bother with brightness */
		return;
	}

	/* turn backlight on if needed */
	if (sc->sc_brightness == 0) {
		cmd[0] = PMU_POW_ON | PMU_POW_BACKLIGHT;
		pmu_send(sc, PMU_POWER_CTRL, 1, cmd, 16, resp);
	}

	DPRINTF("pmu_update_brightness: %d -> %d\n", sc->sc_brightness,
	    sc->sc_brightness_wanted);

	val = 0x7f - (sc->sc_brightness_wanted >> 1);
	if (val < 0x08)
		val = 0x08;
	if (val > 0x78)
		val = 0x78;
	cmd[0] = val;
	pmu_send(sc, PMU_SET_BRIGHTNESS, 1, cmd, 16, resp);

	sc->sc_brightness = sc->sc_brightness_wanted;
}

static void
pmu_thread(void *cookie)
{
	struct pmu_softc *sc = cookie;
	//time_t time_bat = time_second;
	int ticks = hz, i;
	
	while (1) {
		tsleep(&sc->sc_event, PWAIT, "wait", ticks);
		if ((sc->sc_pending & 3) != 0) {
			DPRINTF("eject %d\n", sc->sc_pending & 3);
			for (i = 1; i < 3; i++) {
				if (i & sc->sc_pending)
					pmu_eject_card(sc, i);
			}
		}

		/* see if we need to update brightness */
		if (sc->sc_brightness_wanted != sc->sc_brightness) {
			pmu_update_brightness(sc);
		}

		/* see if we need to update audio volume */
		if (sc->sc_volume_wanted != sc->sc_volume) {
#if 0
			set_volume(sc->sc_volume_wanted);
#endif
			sc->sc_volume = sc->sc_volume_wanted;
		}

		if (sc->sc_pending & PMU_EV_LID) {
			atomic_and_32(&sc->sc_pending, ~PMU_EV_LID);
			sysmon_pswitch_event(&sc->sc_lidswitch, 
	    		    sc->sc_lid_closed ? PSWITCH_EVENT_PRESSED : 
			    PSWITCH_EVENT_RELEASED);
		}

		if (sc->sc_pending & PMU_EV_BUTTON) {
			atomic_and_32(&sc->sc_pending, ~PMU_EV_BUTTON);
			sysmon_pswitch_event(&sc->sc_powerbutton, 
	    		    sc->sc_button ? PSWITCH_EVENT_PRESSED : 
			    PSWITCH_EVENT_RELEASED);
		}

		if (sc->sc_callback != NULL)
			sc->sc_callback(sc->sc_cb_cookie);
	}
}

static int
pmu_print(void *aux, const char *what)
{

	return 0;
}

static void
pmu_attach_legacy_battery(struct pmu_softc *sc)
{
	struct battery_attach_args baa;

	baa.baa_type = BATTERY_TYPE_LEGACY;
	baa.baa_pmu_ops = &sc->sc_pmu_ops;
	config_found_ia(sc->sc_dev, "pmu_bus", &baa, pmu_print);
}

static void
pmu_attach_smart_battery(struct pmu_softc *sc, int num)
{
	struct battery_attach_args baa;

	baa.baa_type = BATTERY_TYPE_SMART;
	baa.baa_pmu_ops = &sc->sc_pmu_ops;
	baa.baa_num = num;
	config_found_ia(sc->sc_dev, "pmu_bus", &baa, pmu_print);
}
