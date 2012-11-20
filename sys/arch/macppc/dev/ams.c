/*	$NetBSD: ams.c,v 1.28.12.1 2012/11/20 03:01:31 tls Exp $	*/

/*
 * Copyright (C) 1998	Colin Wood
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
 *	This product includes software developed by Colin Wood.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ams.c,v 1.28.12.1 2012/11/20 03:01:31 tls Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/sysctl.h>

#include <machine/autoconf.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <macppc/dev/adbvar.h>
#include <macppc/dev/aedvar.h>
#include <macppc/dev/amsvar.h>

#include "aed.h"

/*
 * Function declarations.
 */
static int	amsmatch(device_t, cfdata_t, void *);
static void	amsattach(device_t, device_t, void *);
static void	ems_init(struct ams_softc *);
static void	ms_processevent(adb_event_t *event, struct ams_softc *);
static void	init_trackpad(struct ams_softc *);

/* Driver definition. */
CFATTACH_DECL_NEW(ams, sizeof(struct ams_softc),
    amsmatch, amsattach, NULL, NULL);

int ams_enable(void *);
int ams_ioctl(void *, u_long, void *, int, struct lwp *);
void ams_disable(void *);

/*
 * handle tapping the trackpad
 * different pads report different button counts and use slightly different
 * protocols
 */
static void ams_mangle_2(struct ams_softc *, int);
static void ams_mangle_4(struct ams_softc *, int);
static int  sysctl_ams_tap(SYSCTLFN_ARGS);

const struct wsmouse_accessops ams_accessops = {
	ams_enable,
	ams_ioctl,
	ams_disable,
};

static int
amsmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct adb_attach_args *aa_args = aux;

	if (aa_args->origaddr == ADBADDR_MS)
		return 1;
	else
		return 0;
}

static void
amsattach(device_t parent, device_t self, void *aux)
{
	ADBSetInfoBlock adbinfo;
	struct ams_softc *sc = device_private(self);
	struct adb_attach_args *aa_args = aux;
	int error;
	struct wsmousedev_attach_args a;

	sc->sc_dev = self;
	sc->origaddr = aa_args->origaddr;
	sc->adbaddr = aa_args->adbaddr;
	sc->handler_id = aa_args->handler_id;

	sc->sc_class = MSCLASS_MOUSE;
	sc->sc_buttons = 1;
	sc->sc_res = 100;
	sc->sc_devid[0] = 0;
	sc->sc_devid[4] = 0;

	adbinfo.siServiceRtPtr = (Ptr)ms_adbcomplete;
	adbinfo.siDataAreaAddr = (void *)sc;

	ems_init(sc);

	/* print out the type of mouse we have */
	switch (sc->handler_id) {
	case ADBMS_100DPI:
		printf("%d-button, %d dpi mouse\n", sc->sc_buttons,
		    (int)(sc->sc_res));
		break;
	case ADBMS_200DPI:
		sc->sc_res = 200;
		printf("%d-button, %d dpi mouse\n", sc->sc_buttons,
		    (int)(sc->sc_res));
		break;
	case ADBMS_MSA3:
		printf("Mouse Systems A3 mouse, %d-button, %d dpi\n",
		    sc->sc_buttons, (int)(sc->sc_res));
		break;
	case ADBMS_USPEED:
		printf("MicroSpeed mouse, default parameters\n");
		break;
	case ADBMS_UCONTOUR:
		printf("Contour mouse, default parameters\n");
		break;
	case ADBMS_TURBO:
		printf("Kensington Turbo Mouse\n");
		break;
	case ADBMS_EXTENDED:
		if (sc->sc_devid[0] == '\0') {
			printf("Logitech ");
			switch (sc->sc_class) {
			case MSCLASS_MOUSE:
				printf("MouseMan (non-EMP) mouse");
				break;
			case MSCLASS_TRACKBALL:
				printf("TrackMan (non-EMP) trackball");
				break;
			default:
				printf("non-EMP relative positioning device");
				break;
			}
			printf("\n");
		} else {
			printf("EMP ");
			switch (sc->sc_class) {
			case MSCLASS_TABLET:
				printf("tablet");
				break;
			case MSCLASS_MOUSE:
				printf("mouse");
				break;
			case MSCLASS_TRACKBALL:
				printf("trackball");
				break;
			case MSCLASS_TRACKPAD:
				printf("trackpad");
				init_trackpad(sc);
				break;
			default:
				printf("unknown device");
				break;
			}
			printf(" <%s> %d-button, %d dpi\n", sc->sc_devid,
			    sc->sc_buttons, (int)(sc->sc_res));
		}
		break;
	default:
		printf("relative positioning device (mouse?) (%d)\n",
			sc->handler_id);
		break;
	}
	error = SetADBInfo(&adbinfo, sc->adbaddr);
#ifdef ADB_DEBUG
	if (adb_debug)
		printf("ams: returned %d from SetADBInfo\n", error);
#endif

	a.accessops = &ams_accessops;
	a.accesscookie = sc;
	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);
}


/*
 * Initialize extended mouse support -- probes devices as described
 * in Inside Macintosh: Devices, Chapter 5 "ADB Manager".
 *
 * Extended Mouse Protocol is documented in TechNote HW1:
 * 	"ADB - The Untold Story:  Space Aliens Ate My Mouse"
 *
 * Supports: Extended Mouse Protocol, MicroSpeed Mouse Deluxe,
 * 	     Mouse Systems A^3 Mouse, Logitech non-EMP MouseMan
 */
void
ems_init(struct ams_softc *sc)
{
	int adbaddr;
	short cmd;
	u_char buffer[9];

	adbaddr = sc->adbaddr;
	if (sc->origaddr != ADBADDR_MS)
		return;
	if (sc->handler_id == ADBMS_USPEED ||
	    sc->handler_id == ADBMS_UCONTOUR) {
		/* Found MicroSpeed Mouse Deluxe Mac or Contour Mouse */
		cmd = ADBLISTEN(adbaddr, 1);

		/*
		 * To setup the MicroSpeed or the Contour, it appears
		 * that we can send the following command to the mouse
		 * and then expect data back in the form:
		 *  buffer[0] = 4 (bytes)
		 *  buffer[1], buffer[2] as std. mouse
		 *  buffer[3] = buffer[4] = 0xff when no buttons
		 *   are down.  When button N down, bit N is clear.
		 * buffer[4]'s locking mask enables a
		 * click to toggle the button down state--sort of
		 * like the "Easy Access" shift/control/etc. keys.
		 * buffer[3]'s alternative speed mask enables using
		 * different speed when the corr. button is down
		 */
		buffer[0] = 4;
		buffer[1] = 0x00;	/* Alternative speed */
		buffer[2] = 0x00;	/* speed = maximum */
		buffer[3] = 0x10;	/* enable extended protocol,
					 * lower bits = alt. speed mask
					 *            = 0000b
					 */
		buffer[4] = 0x07;	/* Locking mask = 0000b,
					 * enable buttons = 0111b
					 */
		adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd);

		sc->sc_buttons = 3;
		sc->sc_res = 200;
		return;
	}
	if (sc->handler_id == ADBMS_TURBO) {
		/* Found Kensington Turbo Mouse */
		static u_char data1[] =
			{ 8, 0xe7, 0x8c, 0, 0, 0, 0xff, 0xff, 0x94 };
		static u_char data2[] =
			{ 8, 0xa5, 0x14, 0, 0, 0x69, 0xff, 0xff, 0x27 };

		buffer[0] = 0;
		adb_op_sync((Ptr)buffer, NULL, (Ptr)0, ADBFLUSH(adbaddr));

		adb_op_sync((Ptr)data1, NULL, (Ptr)0, ADBLISTEN(adbaddr, 2));

		buffer[0] = 0;
		adb_op_sync((Ptr)buffer, NULL, (Ptr)0, ADBFLUSH(adbaddr));

		adb_op_sync((Ptr)data2, NULL, (Ptr)0, ADBLISTEN(adbaddr, 2));
		return;
	}
	if ((sc->handler_id == ADBMS_100DPI) ||
	    (sc->handler_id == ADBMS_200DPI)) {
		/* found a mouse */
		cmd = ADBTALK(adbaddr, 3);
		if (adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd)) {
#ifdef ADB_DEBUG
			if (adb_debug)
				printf("adb: ems_init timed out\n");
#endif
			return;
		}

		/* Attempt to initialize Extended Mouse Protocol */
		buffer[2] = 4; /* make handler ID 4 */
		cmd = ADBLISTEN(adbaddr, 3);
		if (adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd)) {
#ifdef ADB_DEBUG
			if (adb_debug)
				printf("adb: ems_init timed out\n");
#endif
			return;
		}

		/*
		 * Check to see if successful, if not
		 * try to initialize it as other types
		 */
		cmd = ADBTALK(adbaddr, 3);
		if (adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd) == 0 &&
		    buffer[2] == ADBMS_EXTENDED) {
			sc->handler_id = ADBMS_EXTENDED;
			cmd = ADBTALK(adbaddr, 1);
			if (adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd)) {
#ifdef ADB_DEBUG
				if (adb_debug)
					printf("adb: ems_init timed out\n");
#endif
			} else if (buffer[0] == 8) {
				/* we have a true EMP device */
#ifdef ADB_PRINT_EMP
				printf("EMP: %02x %02x %02x %02x %02x %02x %02x %02x\n",
				    buffer[1], buffer[2], buffer[3], buffer[4],
				    buffer[5], buffer[6], buffer[7], buffer[8]);
#endif
				sc->sc_class = buffer[7];
				sc->sc_buttons = buffer[8];
				sc->sc_res = (int)*(short *)&buffer[5];
				memcpy(sc->sc_devid, &(buffer[1]), 4);
			} else if (buffer[1] == 0x9a &&
			    ((buffer[2] == 0x20) || (buffer[2] == 0x21))) {
				/*
				 * Set up non-EMP Mouseman/Trackman to put
				 * button bits in 3rd byte instead of sending
				 * via pseudo keyboard device.
				 */
				cmd = ADBLISTEN(adbaddr, 1);
				buffer[0]=2;
				buffer[1]=0x00;
				buffer[2]=0x81;
				adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd);

				cmd = ADBLISTEN(adbaddr, 1);
				buffer[0]=2;
				buffer[1]=0x01;
				buffer[2]=0x81;
				adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd);

				cmd = ADBLISTEN(adbaddr, 1);
				buffer[0]=2;
				buffer[1]=0x02;
				buffer[2]=0x81;
				adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd);

				cmd = ADBLISTEN(adbaddr, 1);
				buffer[0]=2;
				buffer[1]=0x03;
				buffer[2]=0x38;
				adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd);

				sc->sc_buttons = 3;
				sc->sc_res = 400;
				if (buffer[2] == 0x21)
					sc->sc_class = MSCLASS_TRACKBALL;
				else
					sc->sc_class = MSCLASS_MOUSE;
			} else
				/* unknown device? */;
		} else {
			/* Attempt to initialize as an A3 mouse */
			buffer[2] = 0x03; /* make handler ID 3 */
			cmd = ADBLISTEN(adbaddr, 3);
			if (adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd)) {
#ifdef ADB_DEBUG
				if (adb_debug)
					printf("adb: ems_init timed out\n");
#endif
				return;
			}

			/*
			 * Check to see if successful, if not
			 * try to initialize it as other types
			 */
			cmd = ADBTALK(adbaddr, 3);
			if (adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd) == 0
			    && buffer[2] == ADBMS_MSA3) {
				sc->handler_id = ADBMS_MSA3;
				/* Initialize as above */
				cmd = ADBLISTEN(adbaddr, 2);
				/* listen 2 */
				buffer[0] = 3;
				buffer[1] = 0x00;
				/* Irrelevant, buffer has 0x77 */
				buffer[2] = 0x07;
				/*
				 * enable 3 button mode = 0111b,
				 * speed = normal
				 */
				adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd);
				sc->sc_buttons = 3;
				sc->sc_res = 300;
			} else {
				/* No special support for this mouse */
			}
		}
	}
}

/*
 * Handle putting the mouse data received from the ADB into
 * an ADB event record.
 */
void
ms_adbcomplete(uint8_t *buffer, uint8_t *data_area, int adb_command)
{
	adb_event_t event;
	struct ams_softc *sc;
	int adbaddr;
#ifdef ADB_DEBUG
	int i;

	if (adb_debug)
		printf("adb: transaction completion\n");
#endif

	adbaddr = ADB_CMDADDR(adb_command);
	sc = (struct ams_softc *)data_area;

	if ((sc->handler_id == ADBMS_EXTENDED) && (sc->sc_devid[0] == 0)) {
		/* massage the data to look like EMP data */
		if ((buffer[3] & 0x04) == 0x04)
			buffer[1] &= 0x7f;
		else
			buffer[1] |= 0x80;
		if ((buffer[3] & 0x02) == 0x02)
			buffer[2] &= 0x7f;
		else
			buffer[2] |= 0x80;
		if ((buffer[3] & 0x01) == 0x01)
			buffer[3] = 0x00;
		else
			buffer[3] = 0x80;
	}

	event.addr = adbaddr;
	event.hand_id = sc->handler_id;
	event.def_addr = sc->origaddr;
	event.byte_count = buffer[0];
	memcpy(event.bytes, buffer + 1, event.byte_count);

#ifdef ADB_DEBUG
	if (adb_debug) {
		printf("ams: from %d at %d (org %d) %d:", event.addr,
		    event.hand_id, event.def_addr, buffer[0]);
		for (i = 1; i <= buffer[0]; i++)
			printf(" %x", buffer[i]);
		printf("\n");
	}
#endif

	microtime(&event.timestamp);

	ms_processevent(&event, sc);
}

/*
 * Given a mouse ADB event, record the button settings, calculate the
 * x- and y-axis motion, and handoff the event to the appropriate subsystem.
 */
static void
ms_processevent(adb_event_t *event, struct ams_softc *sc)
{
	adb_event_t new_event;
	int i, button_bit, max_byte, mask, buttons, dx, dy;

	new_event = *event;
	buttons = 0;

	/*
	 * This should handle both plain ol' Apple mice and mice
	 * that claim to support the Extended Apple Mouse Protocol.
	 */
	max_byte = event->byte_count;
	button_bit = 1;
	switch (event->hand_id) {
	case ADBMS_USPEED:
	case ADBMS_UCONTOUR:
		/* MicroSpeed mouse and Contour mouse */
		if (max_byte == 4)
			buttons = (~event->bytes[2]) & 0xff;
		else
			buttons = (event->bytes[0] & 0x80) ? 0 : 1;
		break;
	case ADBMS_MSA3:
		/* Mouse Systems A3 mouse */
		if (max_byte == 3)
			buttons = (~event->bytes[2]) & 0x07;
		else
			buttons = (event->bytes[0] & 0x80) ? 0 : 1;
		break;
	default:	
		/* Classic Mouse Protocol (up to 2 buttons) */
		for (i = 0; i < 2; i++, button_bit <<= 1)
			/* 0 when button down */
			if (!(event->bytes[i] & 0x80))
				buttons |= button_bit;
			else
				buttons &= ~button_bit;
		/* Extended Protocol (up to 6 more buttons) */
		for (mask = 0x80; i < max_byte;
		     i += (mask == 0x80), button_bit <<= 1) {
			/* 0 when button down */
			if (!(event->bytes[i] & mask))
				buttons |= button_bit;
			else
				buttons &= ~button_bit;
			mask = ((mask >> 4) & 0xf)
				| ((mask & 0xf) << 4);
		}				
		break;
	}

	dx = ((int)(event->bytes[1] & 0x3f)) -
				((event->bytes[1] & 0x40) ? 64 : 0);
	dy = ((int) (event->bytes[0] & 0x3f)) -
				((event->bytes[0] & 0x40) ? 64 : 0);

	if (sc->sc_class == MSCLASS_TRACKPAD) {
		if (sc->sc_tapping == 1) {

			if (sc->sc_down) {
				/* finger is down - collect motion data */
				sc->sc_x += dx;
				sc->sc_y += dy;
			}
			switch (sc->sc_buttons) {
				case 2:
					ams_mangle_2(sc, buttons);
					break;
				case 4:
					ams_mangle_4(sc, buttons);
					break;
			}
		}
		/* filter the pseudo-buttons out */
		buttons &= 1;
	}

	new_event.u.m.buttons = sc->sc_mb | buttons;
	new_event.u.m.dx = dx;
	new_event.u.m.dy = dy;

	if (sc->sc_wsmousedev)
		wsmouse_input(sc->sc_wsmousedev, new_event.u.m.buttons,
			      new_event.u.m.dx, -new_event.u.m.dy, 0, 0,
			      WSMOUSE_INPUT_DELTA);
#if NAED > 0
	aed_input(&new_event);
#endif
}

static void
ams_mangle_2(struct ams_softc *sc, int buttons)
{

	if (buttons & 4) {
		/* finger down on pad */
		if (sc->sc_down == 0) {
			sc->sc_down = 1;
			sc->sc_x = 0;
			sc->sc_y = 0;
		}
	}
	if (buttons & 8) {
		/* finger up */
		if (sc->sc_down) {
			if (((sc->sc_x * sc->sc_x + 
			    sc->sc_y * sc->sc_y) < 20) && 
			    (sc->sc_wsmousedev)) {
				/* 
				 * if there wasn't much movement between
				 * finger down and up again we assume
				 * someone tapped the pad and we just
				 * send a mouse button event
				 */
				wsmouse_input(sc->sc_wsmousedev,
				    1, 0, 0, 0, 0, WSMOUSE_INPUT_DELTA);
			}
			sc->sc_down = 0;
		}
	}
}

static void
ams_mangle_4(struct ams_softc *sc, int buttons)
{

	if (buttons & 0x20) {
		/* finger down on pad */
		if (sc->sc_down == 0) {
			sc->sc_down = 1;
			sc->sc_x = 0;
			sc->sc_y = 0;
		}
	}
	if ((buttons & 0x20) == 0) {
		/* finger up */
		if (sc->sc_down) {
			if (((sc->sc_x * sc->sc_x + 
			    sc->sc_y * sc->sc_y) < 20) && 
			    (sc->sc_wsmousedev)) {
				/* 
				 * if there wasn't much movement between
				 * finger down and up again we assume
				 * someone tapped the pad and we just
				 * send a mouse button event
				 */
				wsmouse_input(sc->sc_wsmousedev,
				    1, 0, 0, 0, 0, WSMOUSE_INPUT_DELTA);
			}
			sc->sc_down = 0;
		}
	}
}

int
ams_enable(void *v)
{
	return 0;
}

int
ams_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_ADB;
		break;
	}
	return EPASSTHROUGH;
}

void
ams_disable(void *v)
{
}

static void
init_trackpad(struct ams_softc *sc)
{
	struct sysctlnode *me = NULL, *node = NULL;
	int cmd, res, ret;
	u_char buffer[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	u_char b2[10];
	u_char b3[9] = {8, 0x99, 0x94, 0x19, 0xff, 0xb2, 0x8a, 0x1b, 0x50};
	
	cmd = ADBTALK(sc->adbaddr, 1);
	res = adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd);

	if (buffer[0] != 8)
		return;

	/* now whack the pad */
	cmd = ADBLISTEN(sc->adbaddr, 1);
	memcpy(b2, buffer, 10);
	b2[7] = 0x0d;
	adb_op_sync((Ptr)b2, NULL, (Ptr)0, cmd);

	cmd = ADBLISTEN(sc->adbaddr, 2);
	adb_op_sync((Ptr)b3, NULL, (Ptr)0, cmd);

	cmd = ADBLISTEN(sc->adbaddr, 1);
	b2[7] = 0x03;
	adb_op_sync((Ptr)b2, NULL, (Ptr)0, cmd);

	buffer[0] = 0;
	cmd = ADBFLUSH(sc->adbaddr);
	adb_op_sync((Ptr)buffer, NULL, (Ptr)0, cmd);

	/*
	 * setup a sysctl node to control whether tapping the pad should
	 * trigger mouse button events
	 */

	sc->sc_tapping = 1;
	
	ret = sysctl_createv(NULL, 0, NULL, (const struct sysctlnode **)&me,
	    CTLFLAG_READWRITE,
	    CTLTYPE_NODE, device_xname(sc->sc_dev), NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	ret=sysctl_createv(NULL, 0, NULL, (const struct sysctlnode **)&node,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC | CTLFLAG_IMMEDIATE,
	    CTLTYPE_INT, "tapping", "tapping the pad causes button events",
	    sysctl_ams_tap, 1, NULL, 0,
	    CTL_MACHDEP, me->sysctl_num, CTL_CREATE, CTL_EOL);
	if (node != NULL) {
		node->sysctl_data = sc;
	}	
}

static int
sysctl_ams_tap(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct ams_softc *sc = node.sysctl_data;

	node.sysctl_idata = sc->sc_tapping;

	if (newp) {

		/* we're asked to write */	
		node.sysctl_data = &sc->sc_tapping;
		if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {

			sc->sc_tapping = (node.sysctl_idata == 0) ? 0 : 1;
			return 0;
		}
		return EINVAL;
	} else {

		node.sysctl_size = 4;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}

	return 0;
}

SYSCTL_SETUP(sysctl_ams_setup, "sysctl ams subtree setup")
{

	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);
}

