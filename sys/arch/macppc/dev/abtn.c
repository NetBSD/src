/*	$NetBSD: abtn.c,v 1.18.12.2 2017/12/03 11:36:24 jdolecek Exp $	*/

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
__KERNEL_RCSID(0, "$NetBSD: abtn.c,v 1.18.12.2 2017/12/03 11:36:24 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <macppc/dev/akbdvar.h>
#include <macppc/dev/adbvar.h>
#include <macppc/dev/pm_direct.h>

#define NVRAM_BRIGHTNESS 0x140e
#define ABTN_HANDLER_ID 31

#define BUTTON_LOUDER	0x06
#define BUTTON_SOFTER	0x07
#define BUTTON_MUTE	0x08
#define BUTTON_BRIGHTER	0x09
#define BUTTON_DIMMER	0x0a
#define BUTTON_EJECT	0x0b
#define BUTTON_DISPLAY	0x0c
#define BUTTON_KEYPAD	0x7f
#define BUTTON_DEPRESS	0x80

struct abtn_softc {
	device_t sc_dev;

	int origaddr;		/* ADB device type */
	int adbaddr;		/* current ADB address */
	int handler_id;

	int brightness;		/* backlight brightness */
	int volume;		/* speaker volume (not yet) */
};

static int abtn_match(device_t, cfdata_t, void *);
static void abtn_attach(device_t, device_t, void *);
static void abtn_adbcomplete(uint8_t *, uint8_t *, int);

CFATTACH_DECL_NEW(abtn, sizeof(struct abtn_softc),
    abtn_match, abtn_attach, NULL, NULL);

int
abtn_match(device_t parent, cfdata_t cf, void *aux)
{
	struct adb_attach_args *aa = aux;

	if (aa->origaddr == ADBADDR_MISC &&
	    aa->handler_id == ABTN_HANDLER_ID)
		return 1;

	return 0;
}

void
abtn_attach(device_t parent, device_t self, void *aux)
{
	struct abtn_softc *sc = device_private(self);
	struct adb_attach_args *aa = aux;
	ADBSetInfoBlock adbinfo;
	int bright;

	sc->sc_dev = self;

	printf("buttons\n");

	bright = pm_read_nvram(NVRAM_BRIGHTNESS);
	if (bright != 0)
		pm_set_brightness(bright);
	sc->brightness = bright;

	sc->origaddr = aa->origaddr;
	sc->adbaddr = aa->adbaddr;
	sc->handler_id = aa->handler_id;

	adbinfo.siServiceRtPtr = (Ptr)abtn_adbcomplete;
	adbinfo.siDataAreaAddr = (void *)sc;

	SetADBInfo(&adbinfo, sc->adbaddr);
}

extern struct cfdriver akbd_cd;

void
abtn_adbcomplete(uint8_t *buffer, uint8_t *data, int adb_command)
{
	struct abtn_softc *sc = (struct abtn_softc *)data;
	u_int cmd;
#ifdef FORCE_FUNCTION_KEYS
	int key = 0;
#endif

	cmd = buffer[1];

#ifdef FORCE_FUNCTION_KEYS
	switch (cmd & 0x7f) {
	case 0x0a: /* f1 */
		key = 122;
		break;
	case 0x09: /* f2 */
		key = 120;
		break;
	case 0x08: /* f3 */
		key =  99;
		break;
	case 0x07: /* f4 */
		key = 118;
		break;
	case 0x06: /* f5 */
		key =  96;
		break;
	case 0x7f: /* f6 */
		key = 97;
		break;
	case 0x0b: /* f12 */
		key = 111;
		break;
	}
	if (key != 0) {
		key |= cmd & 0x80;
		kbd_passup(device_lookup_private(&akbd_cd, 0),key);
		return;
	}
#endif

	if (cmd >= BUTTON_DEPRESS)
		return;

	switch (cmd) {
	case BUTTON_DIMMER:
		sc->brightness -= 8;
		if (sc->brightness < 8)
			sc->brightness = 8;
		pm_set_brightness(sc->brightness);
		pm_write_nvram(NVRAM_BRIGHTNESS, sc->brightness);
		break;

	case BUTTON_BRIGHTER:
		sc->brightness += 8;
		if (sc->brightness > 0x78)
			sc->brightness = 0x78;
		pm_set_brightness(sc->brightness);
		pm_write_nvram(NVRAM_BRIGHTNESS, sc->brightness);
		break;

	case BUTTON_MUTE:
	case BUTTON_SOFTER:
	case BUTTON_LOUDER:
		printf("%s: volume setting not implemented\n",
		       device_xname(sc->sc_dev));
		break;
	case BUTTON_DISPLAY:
		printf("%s: display selection not implemented\n",
		       device_xname(sc->sc_dev));
		break;
	case BUTTON_EJECT:
		printf("%s: eject not implemented\n",
		       device_xname(sc->sc_dev));
		break;

	/* The keyboard gets wacky when in keypad mode. */
	case BUTTON_KEYPAD:
		break;

	default:
		printf("%s: unknown button 0x%x\n",
		       device_xname(sc->sc_dev), cmd);
	}
}
