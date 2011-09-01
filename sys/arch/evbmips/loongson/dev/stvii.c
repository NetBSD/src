/*	$NetBSD: stvii.c,v 1.1 2011/09/01 14:07:37 macallan Exp $	*/

/*-
 * Copyright (C) 2005 Michael Lorenz.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* 
 * a driver for the ST7 microcontroller found in Gdium Liberty 1000 notebooks
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: stvii.c,v 1.1 2011/09/01 14:07:37 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/kthread.h>
#include <sys/proc.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <dev/i2c/i2cvar.h>

#include "opt_stvii.h"

#ifdef STVII_DEBUG
#define DPRINTF aprint_error
#else
#define DPRINTF while (0) printf
#endif

/* register definitions from OpenBSD */
#define	ST7_VERSION	0x00	/* only on later mobos */

#define	ST7_STATUS	0x01
#define	STS_LID_CLOSED		0x01
#define	STS_POWER_BTN_DOWN	0x02
#define	STS_BATTERY_PRESENT	0x04	/* not available on old mobo */
#define	STS_POWER_AVAILABLE	0x08
#define	STS_WAVELAN_BTN_DOWN	0x10	/* ``enable'' on old mobo */
#define	STS_AC_AVAILABLE	0x20
#define	ST7_CONTROL	0x02
#define	STC_DDR_CLOCK		0x01
#define	STC_CHARGE_LED_LIT	0x02
#define	STC_BEEP		0x04
#define	STC_DDR_POWER		0x08
#define	STC_TRICKLE		0x10	/* trickle charge rate */
#define	STC_RADIO_ENABLE	0x20	/* enable wavelan rf, later mobos */
#define	STC_MAIN_POWER		0x40
#define	STC_CHARGE_ENABLE	0x80
#define	ST7_BATTERY_L	0x03
#define	ST7_BATTERY_H	0x04
#define	ST7_SIGNATURE	0x05
#define	STSIG_EC_CONTROL	0x00
#define	STSIG_OS_CONTROL	0xae
/* rough battery operating state limits */
#define STSEC_BAT_MIN_VOLT	7000000	/* 7V */
#define STSEC_BAT_MAX_VOLT	8000000	/* 8V */

#define BAT_AC_PRESENT		0
#define BAT_BATTERY_PRESENT	1
#define BAT_CHARGING		2
#define BAT_CHARGE		3
#define BAT_MAX_CHARGE		4
#define BAT_NSENSORS		5

struct stvii_softc {
	device_t sc_dev;
	i2c_tag_t sc_i2c;
	int sc_address;
	int sc_sleep;
	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[BAT_NSENSORS];
	struct sysmon_pswitch sc_sm_acpower;
	struct sysmon_pswitch sc_sm_lid;
};

static void stvii_attach(device_t, device_t, void *);
static int stvii_match(device_t, cfdata_t, void *);
static void stvii_writereg(struct stvii_softc *, int, uint8_t);
static uint8_t stvii_readreg(struct stvii_softc *, int);
static void stvii_worker(void *);

CFATTACH_DECL_NEW(stvii, sizeof(struct stvii_softc),
    stvii_match, stvii_attach, NULL, NULL);

static int
stvii_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *args = aux;
	int ret = -1;
	uint8_t out = ST7_VERSION, in = 0;

	/* see if we can talk to something at address 0x40 */
	if (args->ia_addr == 0x40) {
		iic_acquire_bus(args->ia_tag, 0);
		ret = iic_exec(args->ia_tag, I2C_OP_READ_WITH_STOP, args->ia_addr,
		    &out, 1, &in, 1, 0);
		DPRINTF("%02x\n", in);
		iic_release_bus(args->ia_tag, 0);
	}
	return (ret >= 0);
}

static void
stvii_attach(device_t parent, device_t self, void *aux)
{
	struct stvii_softc *sc = device_private(self);
	struct i2c_attach_args *args = aux;
	uint8_t ver, reg;

	sc->sc_dev = self;
	sc->sc_address = args->ia_addr;
	aprint_normal(": ST7 Microcontroller\n");
	sc->sc_i2c = args->ia_tag;
	ver = stvii_readreg(sc, ST7_VERSION);

	aprint_normal_dev(sc->sc_dev, "firmware version %d.%d\n", (ver >> 4) & 0xf, ver & 0xf);
#ifdef STVII_DEBUG
	{
		int i;

		for (i = 0; i < 6; i++) {
			printf("%02x ", stvii_readreg(sc, i));
		}
		printf("\n");
	}
#endif
	stvii_writereg(sc, ST7_SIGNATURE, STSIG_EC_CONTROL);
	reg = stvii_readreg(sc, ST7_CONTROL);
	reg |= STC_RADIO_ENABLE;
	stvii_writereg(sc, ST7_CONTROL, reg);
	reg = stvii_readreg(sc, ST7_CONTROL);

	if (kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL, stvii_worker, sc,
	    NULL, "stvii") != 0) {
	    	aprint_error_dev(sc->sc_dev, "Failed to start kernel thread\n");
	}

	memset(&sc->sc_sm_acpower, 0, sizeof(struct sysmon_pswitch));
	sc->sc_sm_acpower.smpsw_name = "AC Power";
	sc->sc_sm_acpower.smpsw_type = PSWITCH_TYPE_ACADAPTER;
	if (sysmon_pswitch_register(&sc->sc_sm_acpower) != 0)
		printf("%s: unable to register AC power status with sysmon\n",
		    device_xname(sc->sc_dev));
	memset(&sc->sc_sm_lid, 0, sizeof(struct sysmon_pswitch));
	sc->sc_sm_lid.smpsw_name = "Lid Switch";
	sc->sc_sm_lid.smpsw_type = PSWITCH_TYPE_LID;
	if (sysmon_pswitch_register(&sc->sc_sm_lid) != 0)
		printf("%s: unable to register lid switch with sysmon\n",
		    device_xname(sc->sc_dev));
}

static void
stvii_writereg(struct stvii_softc *sc, int reg, uint8_t val)
{
	uint8_t out[2] = {reg, val};

	if ((reg < 0) || (reg > 5))
		return;

	iic_acquire_bus(sc->sc_i2c, 0);
	iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_address, out, 2, NULL, 0, 0);
	iic_release_bus(sc->sc_i2c, 0);
}

static uint8_t
stvii_readreg(struct stvii_softc *sc, int reg)
{
	uint8_t inreg[1], outreg[1];

	if ((reg < 0) || (reg > 5))
		return 0xff;
	inreg[0] = 0x77;
	outreg[0] = reg;
	iic_acquire_bus(sc->sc_i2c, 0);
	iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_address, outreg,
	    1, inreg, 1, 0);
	iic_release_bus(sc->sc_i2c, 0);
	return inreg[0];
}

static int
stvii_battery_level(struct stvii_softc *sc)
{
	int bl, bh;

	bl = stvii_readreg(sc, ST7_BATTERY_L);
	bh = stvii_readreg(sc, ST7_BATTERY_H);
	return (bl & 3) | (bh << 2);	
}

static void
stvii_worker(void *cookie)
{
	struct stvii_softc *sc = cookie;
	uint8_t status = 0, st;
	int battery_level = 0, bl;
	int ok = TRUE;

	while (ok) {
		st = stvii_readreg(sc, ST7_STATUS);
		if (st != status) {
			if ((status ^ st) & STS_LID_CLOSED) {
				sysmon_pswitch_event(&sc->sc_sm_lid, 
				    ((st & STS_LID_CLOSED) ?
				     PSWITCH_EVENT_PRESSED :
				     PSWITCH_EVENT_RELEASED));
			}
			if ((status ^ st) & STS_AC_AVAILABLE) {
				sysmon_pswitch_event(&sc->sc_sm_acpower, 
				    ((st & STS_AC_AVAILABLE) ?
				     PSWITCH_EVENT_PRESSED :
				     PSWITCH_EVENT_RELEASED));
			}
			status = st;
		}
		if (0) {
			bl = stvii_battery_level(sc);
			if (bl != battery_level) {
				printf("battery: %d\n", bl);
				battery_level = bl;
			}
		}
		tsleep(&sc->sc_sleep, 0, "stvii", hz / 2);
	}
}
