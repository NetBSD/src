/*	$NetBSD: vald_acpi.c,v 1.8 2003/05/20 12:14:17 wiz Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Masanori Kanaoka.
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

/*
 * Copyright 2001 Bill Sommerfeld.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* #define VALD_ACPI_DEBUG */

/*
 * ACPI VALD Driver for Toshiba Libretto L3.
 *	This driver is based on acpibat driver.
 */

/*
 * Obtain information of Toshiba "GHCI" Method from next URL.
 *           http://www.buzzard.org.uk/toshiba/docs.html
 *           http://memebeam.org/toys/ToshibaAcpiDriver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vald_acpi.c,v 1.8 2003/05/20 12:14:17 wiz Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>		/* for hz */
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/callout.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#define GHCI_WORDS 6
#define GHCI_FIFO_EMPTY  0x8c00
#define GHCI_NOT_SUPPORT  0x8000

#define GHCI_BACKLIGHT		0x0002
#define GHCI_ACADAPTOR		0x0003
#define GHCI_FAN		0x0004
#define GHCI_SYSTEM_EVENT_FIFO	0x0016
#define GHCI_DISPLAY_DEVICE	0x001C
#define GHCI_HOTKEY_EVENT	0x001E

#define GHCI_ON			0x0001
#define GHCI_OFF		0x0000
#define GHCI_ENABLE		0x0001
#define GHCI_DISABLE		0x0000

#define GHCI_CRT		0x0002
#define GHCI_LCD		0x0001


struct vald_acpi_softc {
	struct device sc_dev;		/* base device glue */
	struct acpi_devnode *sc_node;	/* our ACPI devnode */
	int sc_flags;			/* see below */
	struct callout sc_callout; 	/* XXX temporary polling */
	
	ACPI_HANDLE lcd_handle;		/* lcd handle */
	int *lcd_level;			/* lcd brightness table */
	int lcd_num;			/* size of lcd brightness table */ 
	int lcd_index;			/* index of lcd brightness table */

	int	sc_ac_status;		/* AC adaptor status when attach */
	
	int	sc_timer;		/* event timer setting */

	/* for GHCI Method */
	ACPI_OBJECT sc_Arg[GHCI_WORDS];		/* Input Arg */
	ACPI_OBJECT sc_Ret[GHCI_WORDS+1];	/* Return buffer */
};

#define AVALD_F_VERBOSE		0x01	/* verbose events */

#define LIBRIGHT_HOLD	0x00
#define LIBRIGHT_UP	0x01
#define LIBRIGHT_DOWN	0x02

int	vald_acpi_match(struct device *, struct cfdata *, void *);
void	vald_acpi_attach(struct device *, struct device *, void *);

void		vald_acpi_event(void *);
static void	vald_acpi_tick(void *);
static void	vald_acpi_notifyhandler(ACPI_HANDLE, UINT32, void *);

#define ACPI_NOTIFY_ValdStatusChanged	0x80


ACPI_STATUS	vald_acpi_ghci_get(struct vald_acpi_softc *, UINT32, UINT32 *,
    UINT32 *);
ACPI_STATUS	vald_acpi_ghci_set(struct vald_acpi_softc *, UINT32, UINT32,
    UINT32 *);

ACPI_STATUS	vald_acpi_libright_get_bus(ACPI_HANDLE, UINT32, void *,
    void **);
void		vald_acpi_libright_get(struct vald_acpi_softc *);
void		vald_acpi_libright_set(struct vald_acpi_softc *, int);

void		vald_acpi_video_switch(struct vald_acpi_softc *);
void		vald_acpi_fan_switch(struct vald_acpi_softc *);

ACPI_STATUS	vald_acpi_bcm_set(ACPI_HANDLE, UINT32);
ACPI_STATUS	vald_acpi_dssx_set(UINT32);

CFATTACH_DECL(vald_acpi, sizeof(struct vald_acpi_softc),
    vald_acpi_match, vald_acpi_attach, NULL, NULL);

/*
 * vald_acpi_match:
 *
 *	Autoconfiguration `match' routine.
 */
int
vald_acpi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return (0);

	if (strcmp(aa->aa_node->ad_devinfo.HardwareId, "TOS6200") == 0)
		return (1);

	return (0);
}

/*
 * vald_acpi_attach:
 *
 *	Autoconfiguration `attach' routine.
 */
void
vald_acpi_attach(struct device *parent, struct device *self, void *aux)
{
	struct vald_acpi_softc *sc = (void *) self;
	struct acpi_attach_args *aa = aux;
	ACPI_STATUS rv;
	UINT32 i, value, result;

	printf(": Toshiba VALD\n");

	sc->sc_node = aa->aa_node;
	sc->sc_timer = 5 * hz;

	/* Initialize input Arg */ 
	for (i = 0; i < GHCI_WORDS; i++) {
		sc->sc_Arg[i].Type = ACPI_TYPE_INTEGER;
		sc->sc_Arg[i].Integer.Value = 0;
	}

	/* Get AC adaptor status via _PSR. */
	rv = acpi_eval_integer(ACPI_ROOT_OBJECT,
		"\\_SB_.ADP1._PSR",&sc->sc_ac_status);
	if (rv != AE_OK)
		printf("%s: Unable to evaluate _PSR error %x\n",
		    sc->sc_dev.dv_xname, rv);
	else
		printf("AC adaptor status %sconnected\n", 
		    (sc->sc_ac_status == 0 ? "not ": ""));

	/* Get LCD backlight status. */
	rv = vald_acpi_ghci_get(sc, GHCI_BACKLIGHT, &value, &result);
	if (rv != AE_OK)
		printf("%s: Unable to evaluate GHCI\n", sc->sc_dev.dv_xname);
	if (result != 0)
		printf("%s: Can't get status of LCD backlight error=%x\n",
		    sc->sc_dev.dv_xname, result);
	else 
		printf("LCD backlight status %s\n",
		    ((value == GHCI_ON) ? "on" : "off"));

	/* Enable SystemEventFIFO,HotkeyEvent */
	rv = vald_acpi_ghci_set(sc, GHCI_SYSTEM_EVENT_FIFO, GHCI_ENABLE,
	    &result); 
	if (rv != AE_OK) 
		printf("%s: unable to evaluate GHCI\n", sc->sc_dev.dv_xname);
	if (result != 0)
		printf("%s: Can't enable SystemEventFIFO error=%d\n",
		    sc->sc_dev.dv_xname, result);

	rv = vald_acpi_ghci_set(sc, GHCI_HOTKEY_EVENT, GHCI_ENABLE, &result); 
	if (rv != AE_OK) 
		printf("%s: unable to evaluate GHCI\n", sc->sc_dev.dv_xname);
	if (result != 0)
		printf("%s: Can't enable HotkeyEvent error=%d\n",
		    sc->sc_dev.dv_xname, result);

	/* Check SystemFIFO events. */
	vald_acpi_event(sc);

	/* Get LCD brightness level via _BCL. */
	vald_acpi_libright_get(sc);

	/* Set LCD brightness level via _BCM. */
	vald_acpi_libright_set(sc, LIBRIGHT_HOLD);

	/* enable vald notify */
	rv = AcpiEvaluateObject(sc->sc_node->ad_handle, "ENAB", NULL, NULL);
	if (rv == AE_OK) {
		rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
		    ACPI_DEVICE_NOTIFY, vald_acpi_notifyhandler, sc);
		if (rv != AE_OK)
			printf("%s: Can't install notify handler (%04x)\n",
			    sc->sc_dev.dv_xname, (uint)rv);
	} else {
		/*
		 * XXX poll hotkey event in the driver for now.
		 * in the future, when we have an API, let userland do this
		 * polling
		 */
		callout_init(&sc->sc_callout);
		callout_reset(&sc->sc_callout, sc->sc_timer,
		    vald_acpi_tick, sc);
	}
}

/*
 * vald_acpi_notifyhandler:
 *
 *	Notify handler.
 */
static void
vald_acpi_notifyhandler(ACPI_HANDLE handle, UINT32 value, void *context)
{
	struct vald_acpi_softc *sc = context;
	ACPI_STATUS rv;
	
	switch (value) {
	case ACPI_NOTIFY_ValdStatusChanged:
#ifdef VALD_ACPI_DEBUG
		printf("%s: received ValdStatusChanged message. \n",
		    sc->sc_dev.dv_xname);
#endif /* ACPI_VALD_DEBUG */

		rv = AcpiOsQueueForExecution(OSD_PRIORITY_LO,
		    vald_acpi_event, sc);

		if (ACPI_FAILURE(rv))
			printf("%s : WARNING: unable to queue vald change "
			    "event: %04x\n", sc->sc_dev.dv_xname, (uint)rv);
		break;

	default:
		printf("vald_acpi_notifyhandler: unknown event: %04"
		    PRIu32 "\n", value);
		break;
	}
}

/*
 * vald_acpi_tick:
 *
 *	Poll hotkey event.
 */
static void
vald_acpi_tick(void *arg)
{
	struct vald_acpi_softc *sc = arg;
	callout_reset(&sc->sc_callout, sc->sc_timer, vald_acpi_tick, arg);
	AcpiOsQueueForExecution(OSD_PRIORITY_LO, vald_acpi_event, sc);
}

/*
 * vald_acpi_event:
 *
 *	Check hotkey event and do it, if event occur.
 */
void
vald_acpi_event(void *arg)
{
	struct vald_acpi_softc *sc = arg;
	ACPI_STATUS rv;
	UINT32 value, result;

	while(1) {
		rv = vald_acpi_ghci_get(sc, GHCI_SYSTEM_EVENT_FIFO, &value,
		    &result);
		if ((rv == AE_OK) && (result == 0)) {
#ifdef VALD_ACPI_DEBUG
			printf("%s: System Event %x\n", sc->sc_dev.dv_xname, 
			    value);
#endif
			switch (value) {
			case 0x1c3: /* Fn + F9 */
				if (sc->sc_timer > hz/2)
					sc->sc_timer = hz/2;
				else
					sc->sc_timer = 5*hz;
#ifdef ACPI_DEBUG
				printf("%s: Change poll time to %x\n",
				    sc->sc_dev.dv_xname, sc->sc_timer);	
#endif
				break;
			case 0x1c2: /* Fn + F8 */
				vald_acpi_fan_switch(sc);
				break;
			case 0x1c1: /* Fn + F7 */
				vald_acpi_libright_set(sc, LIBRIGHT_UP);
				break;
			case 0x1c0: /* Fn + F6 */
				vald_acpi_libright_set(sc, LIBRIGHT_DOWN);
				break;
			case 0x1bf: /* Fn + F5 */
				vald_acpi_video_switch(sc);
				break;
			default:
				break;
			}
		}
		if (rv != AE_OK || result == GHCI_FIFO_EMPTY) 
			break;
	}	
}

/*
 * vald_acpi_ghci_get:
 *
 *	Get value via "GHCI" Method.
 */
ACPI_STATUS 
vald_acpi_ghci_get(struct vald_acpi_softc *sc, 
    UINT32 reg, UINT32 *value, UINT32 *result)
{
	ACPI_STATUS rv;
	ACPI_OBJECT_LIST ArgList;
	ACPI_OBJECT *param, *PrtElement;
	ACPI_BUFFER buf;
	
	sc->sc_Arg[0].Integer.Value = 0xfe00;
	sc->sc_Arg[1].Integer.Value = reg;
	sc->sc_Arg[2].Integer.Value = 0;

	ArgList.Count = GHCI_WORDS;
	ArgList.Pointer = sc->sc_Arg;

	buf.Pointer = sc->sc_Ret;
	buf.Length = sizeof(sc->sc_Ret);

	rv = AcpiEvaluateObject(sc->sc_node->ad_handle, 
	    "GHCI", &ArgList, &buf);	

	*result = GHCI_NOT_SUPPORT;
	*value = 0;
	param = (ACPI_OBJECT *)buf.Pointer;
	if (param->Type == ACPI_TYPE_PACKAGE) {
		PrtElement = param->Package.Elements;
		if (PrtElement->Type == ACPI_TYPE_INTEGER)
			*result = PrtElement->Integer.Value;
		PrtElement++;
		PrtElement++;
		if (PrtElement->Type == ACPI_TYPE_INTEGER)
			*value = PrtElement->Integer.Value;
	}
	return (rv);
}

/*
 * vald_acpi_ghci_set:
 *
 *	Set value via "GHCI" Method.
 */
ACPI_STATUS 
vald_acpi_ghci_set(struct vald_acpi_softc *sc, 
    UINT32 reg, UINT32 value, UINT32 *result)
{
	ACPI_STATUS rv;
	ACPI_OBJECT_LIST ArgList;
	ACPI_OBJECT *param, *PrtElement;
	ACPI_BUFFER buf;
	
	sc->sc_Arg[0].Integer.Value = 0xff00;
	sc->sc_Arg[1].Integer.Value = reg;
	sc->sc_Arg[2].Integer.Value = value;

	ArgList.Count = GHCI_WORDS;
	ArgList.Pointer = sc->sc_Arg;

	buf.Pointer = sc->sc_Ret;
	buf.Length = sizeof(sc->sc_Ret);

	rv = AcpiEvaluateObject(sc->sc_node->ad_handle, 
	    "GHCI", &ArgList, &buf);	

	*result = GHCI_NOT_SUPPORT;	
	param = (ACPI_OBJECT *)buf.Pointer;
	if (param->Type == ACPI_TYPE_PACKAGE) {
		PrtElement = param->Package.Elements;
	    	if (PrtElement->Type == ACPI_TYPE_INTEGER)
			*result = PrtElement->Integer.Value;
	}
	return (rv);
}

/*
 * vald_acpi_libright_get_bus:
 *
 *	Get LCD brightness level via "_BCL" Method,
 *	and save this handle.
 */
ACPI_STATUS
vald_acpi_libright_get_bus(ACPI_HANDLE handle, UINT32 level, void *context,
    void **status)
{
	struct vald_acpi_softc *sc = context;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;
	ACPI_OBJECT *param, *PrtElement;
	int i, *pi;

	rv = acpi_eval_struct(handle, "_BCL", &buf);
	if (ACPI_FAILURE(rv))
		return (AE_OK);

	sc->lcd_handle = handle;
	param = (ACPI_OBJECT *)buf.Pointer;
	if (param->Type == ACPI_TYPE_PACKAGE) {
		printf("_BCL retrun: %d packages\n", param->Package.Count);

		sc->lcd_num = param->Package.Count;
		sc->lcd_level = AcpiOsAllocate(sizeof(int) * sc->lcd_num);

		PrtElement = param->Package.Elements;
		pi = sc->lcd_level;
		for (i = 0; i < param->Package.Count; i++) {
			if (PrtElement->Type == ACPI_TYPE_INTEGER) {
				*pi = (unsigned)PrtElement->Integer.Value;
				PrtElement++;
				pi++;
			}
		}
		if (sc->sc_ac_status == 1) /* AC adaptor on when attach */
			sc->lcd_index = sc->lcd_num -1; /* MAX Brightness */
		else
			sc->lcd_index = 3;

#ifdef ACPI_DEBUG
		pi = sc->lcd_level;
		printf("\t Full Power Level: %d\n", *pi);
		printf("\t on Battery Level: %d\n", *(pi+1));
		printf("\t Possible Level: ");
		for (i = 2;i < sc->lcd_num; i++)
			printf(" %d", *(pi+i));
		printf("\n");
#endif
	}

	free(buf.Pointer, M_DEVBUF);
	return (AE_OK);
}

/*
 * vald_acpi_libright_get:
 *
 *	Search node that have "_BCL" Method.
 */
void
vald_acpi_libright_get(struct vald_acpi_softc *sc)
{
	ACPI_HANDLE parent;

	printf("%s: Get LCD brightness via _BCL\n", sc->sc_dev.dv_xname);

#ifdef ACPI_DEBUG
	printf("acpi_libright_get: start\n");
#endif
	if (AcpiGetHandle(ACPI_ROOT_OBJECT, "\\_SB_", &parent) != AE_OK)
		return;

	AcpiWalkNamespace(ACPI_TYPE_DEVICE, parent, 100,
	    vald_acpi_libright_get_bus, sc, NULL);
}

/*
 * vald_acpi_libright_set:
 *
 *	Figure up next status and set it.
 */
void 
vald_acpi_libright_set(struct vald_acpi_softc *sc, int UpDown)
{
	ACPI_STATUS rv;
	int *pi;
	UINT32 backlight, backlight_new, result, bright;

	/* Skip,if it does not support _BCL. */
	if (sc->lcd_handle == NULL)
		return;

	/* Get LCD backlight status. */
	rv = vald_acpi_ghci_get(sc, GHCI_BACKLIGHT, &backlight, &result);
	if (rv != AE_OK)
		printf("%s: unable to evaluate GHCI\n", sc->sc_dev.dv_xname);
	if (result != 0)
		printf("%s: Can't get LCD backlight status error=%x\n",	
		    sc->sc_dev.dv_xname, result);

	/* Figure up next status. */
	backlight_new = backlight;
	if (UpDown == LIBRIGHT_UP) {
		if (backlight == 1)
			sc->lcd_index++;
		else {
			/* backlight on */
			backlight_new = 1;
			sc->lcd_index = 2;
		}
	} else if (UpDown == LIBRIGHT_DOWN) {
		if ((backlight == 1) && (sc->lcd_index > 2))
			sc->lcd_index--;
		else { 
			/* backlight off */
			backlight_new = 0;
			sc->lcd_index = 2;
		}
	}

	/* Check index value. */
	if (sc->lcd_index < 2)
		sc->lcd_index = 2; /* index Minium Value */
	if (sc->lcd_index >= sc->lcd_num)
		sc->lcd_index = sc->lcd_num - 1;

	/* Set LCD backlight,if status is changed. */
	if (backlight_new != backlight) {
		rv = vald_acpi_ghci_set(sc, GHCI_BACKLIGHT, backlight_new,
		    &result);
		if (rv != AE_OK)
			printf("%s: unable to evaluate GHCI\n", 
			    sc->sc_dev.dv_xname);
		if (result != 0)
			printf("%s: Can't set LCD backlight %s error=%x\n",
			    sc->sc_dev.dv_xname, 
			    ((backlight_new == 1) ? "on" : "off"), result);
	}

	if (backlight_new == 1) {

		pi = sc->lcd_level;
		bright = *(pi + sc->lcd_index);

		rv = vald_acpi_bcm_set(sc->lcd_handle, bright);
		if (rv != AE_OK)
			printf("Unable to evaluate _BCM: %d\n", rv);
	} else { 
		bright = 0;
	}
#ifdef ACPI_DEBUG
	printf("LCD bright");
	printf(" %s", ((UpDown == LIBRIGHT_UP) ? "up":""));
	printf("%s\n", ((UpDown == LIBRIGHT_DOWN) ? "down":""));
	printf("\t acpi_libright_set: Set brightness to %d%%\n", bright);
#endif
}

/*
 * vald_acpi_video_switch:
 *
 *	Get video status(LCD/CRT) and set new video status.
 */
void
vald_acpi_video_switch(struct vald_acpi_softc *sc)
{
	ACPI_STATUS	rv;
	UINT32		value, result;

	/* Get video status. */
	rv = vald_acpi_ghci_get(sc, GHCI_DISPLAY_DEVICE, &value, &result);
	if (rv != AE_OK)
		printf("%s: unable to evaluate GHCI\n", sc->sc_dev.dv_xname);
	if (result != 0)
		printf("%s: Can't get video status  error=%x\n",
		    sc->sc_dev.dv_xname, result);

#ifdef ACPI_DEBUG
	printf("Toggle LCD/CRT\n");
	printf("\t Before switch, video status:   %s", 
	    (((value & GHCI_LCD) == GHCI_LCD) ? "LCD" : ""));
	printf("%s\n", (((value & GHCI_CRT) == GHCI_CRT) ? "CRT": "")); 
#endif

	/* Toggle LCD/CRT */	
	if (value & GHCI_LCD) {
		value &= ~GHCI_LCD;
		value |= GHCI_CRT; 
	} else if (value & GHCI_CRT){
		value &= ~GHCI_CRT;
		value |= GHCI_LCD;
	} 

	rv = vald_acpi_dssx_set(value);
	if (rv != AE_OK)
		printf("Unable to evaluate DSSX: %d\n", rv);

}

/*
 * vald_acpi_bcm_set:
 *
 *	Set LCD brightness via "_BCM" Method.
 */
ACPI_STATUS
vald_acpi_bcm_set(ACPI_HANDLE handle, UINT32 bright)
{
	ACPI_STATUS rv;
	ACPI_OBJECT Arg;
	ACPI_OBJECT_LIST ArgList;

	ArgList.Count = 1;
	ArgList.Pointer = &Arg;

	Arg.Type = ACPI_TYPE_INTEGER;
	Arg.Integer.Value = bright;

	rv = AcpiEvaluateObject(handle, "_BCM", &ArgList, NULL);
	return (rv);
}

/*
 * vald_acpi_dssx_set:
 *
 *	Set value via "\\_SB_.VALX.DSSX" Method.
 */
ACPI_STATUS
vald_acpi_dssx_set(UINT32 value)
{
	ACPI_STATUS rv;
	ACPI_OBJECT Arg;
	ACPI_OBJECT_LIST ArgList;
	
	ArgList.Count = 1;
	ArgList.Pointer = &Arg;

	Arg.Type = ACPI_TYPE_INTEGER;
	Arg.Integer.Value = value;

	rv = AcpiEvaluateObject(ACPI_ROOT_OBJECT, "\\_SB_.VALX.DSSX", 
	    &ArgList, NULL);

	return (rv);
}

/*
 * vald_acpi_fan_switch:
 *
 *	Get FAN status and set new FAN status.
 */
void 
vald_acpi_fan_switch(struct vald_acpi_softc *sc)
{
	ACPI_STATUS rv;
	UINT32	value, result;

	/* Get FAN status */
	rv = vald_acpi_ghci_get(sc, GHCI_FAN, &value, &result); 
	if (rv != AE_OK) 
		printf("%s: unable to evaluate GHCI\n", sc->sc_dev.dv_xname);
	if (result != 0)
		printf("%s: Can't get FAN status error=%d\n",
		    sc->sc_dev.dv_xname, result);

#ifdef ACPI_DEBUG
	printf("Toggle FAN on/off\n");
	printf("\t Before toggle, FAN status %s\n", 
	    (value == GHCI_OFF ? "off" : "on"));
#endif

	/* Toggle FAN on/off */	
	if (value == GHCI_OFF) 
		value = GHCI_ON;
	else 
		value = GHCI_OFF;

	/* Set FAN new status. */
	rv = vald_acpi_ghci_set(sc, GHCI_FAN, value, &result); 
	if (rv != AE_OK) 
		printf("%s: unable to evaluate GHCI\n", sc->sc_dev.dv_xname);
	if (result != 0) 
		printf("%s: Can't set FAN status error=%d\n",
		    sc->sc_dev.dv_xname, result);

#ifdef ACPI_DEBUG
	printf("\t After toggle, FAN status %s\n", 
	    (value == GHCI_OFF ? "off" : "on"));
#endif
}
