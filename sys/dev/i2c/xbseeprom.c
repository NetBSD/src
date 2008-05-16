/* $NetBSD: xbseeprom.c,v 1.4.4.1 2008/05/16 02:24:01 yamt Exp $ */

/*-
 * Copyright (c) 2007, 2008 Jared D. McNeill <jmcneill@invisible.ca>
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

/*
 * Microsoft XBOX Serial EEPROM
 *
 * Documentation lives here:
 *  http://www.xbox-linux.org/wiki/Xbox_Serial_EEPROM
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xbseeprom.c,v 1.4.4.1 2008/05/16 02:24:01 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>

#include <dev/i2c/i2cvar.h>

static int	xbseeprom_match(device_t, cfdata_t, void *);
static void	xbseeprom_attach(device_t, device_t, void *);

struct xbseeprom_data {
	/* factory settings */
	uint8_t		HMAC_SHA1_Hash[20];
	uint8_t		Confounder[8];
	uint8_t		HDDKkey[16];
	uint8_t		XBERegion[4];
	uint8_t		Checksum2[4];
	uint8_t		SerialNumber[12];
	uint8_t		MACAddress[6];
	uint8_t		UNKNOWN2[2];
	uint8_t		OnlineKey[16];
	uint8_t		VideoStandard[4];
	uint8_t		UNKNOWN3[4];
	/* user settings */
	uint8_t		Checksum3[4];
	uint8_t		TimeZoneBias[4];
	uint8_t		TimeZoneStdName[4];
	uint8_t		TimeZoneDltName[4];
	uint8_t		UNKNOWN4[8];
	uint8_t		TimeZoneStdDate[4];
	uint8_t		TimeZoneDltDate[4];
	uint8_t		UNKNOWN5[8];
	uint8_t		TimeZoneStdBias[4];
	uint8_t		TimeZoneDltBias[4];
	uint8_t		LanguageID[4];
	uint8_t		VideoFlags[4];
	uint8_t		AudioFlags[4];
	uint8_t		ParentalControlGames[4];
	uint8_t		ParentalControlPwd[4];
	uint8_t		ParentalControlMovies[4];
	uint8_t		XBOXLiveIPAddress[4];
	uint8_t		XBOXLiveDNS[4];
	uint8_t		XBOXLiveGateway[4];
	uint8_t		XBOXLiveSubnetMask[4];
	uint8_t		OtherSettings[4];
	uint8_t		DVDPlaybackKitZone[4];
	uint8_t		UNKNOWN6[64];
};

#define XBSEEPROM_SIZE	(sizeof(struct xbseeprom_data))

struct xbseeprom_softc {
	device_t	sc_dev;

	i2c_tag_t	sc_tag;
	i2c_addr_t	sc_addr;

	struct xbseeprom_data *sc_data;
};

static void	xbseeprom_read_1(struct xbseeprom_softc *, uint8_t, uint8_t *);

static char	xbseeprom_serial[17];

CFATTACH_DECL_NEW(xbseeprom, sizeof(struct xbseeprom_softc),
    xbseeprom_match, xbseeprom_attach, NULL, NULL);

static int
xbseeprom_match(device_t parent, cfdata_t cf, void *opaque)
{
	struct i2c_attach_args *ia = opaque;

	/* Serial EEPROM always lives at 0x54 on Xbox */
	if (ia->ia_addr == 0x54)
		return 1;

	return 0;
}

static void
xbseeprom_attach(device_t parent, device_t self, void *opaque)
{
	struct xbseeprom_softc *sc = device_private(self);
	struct i2c_attach_args *ia = opaque;
	uint8_t *ptr;
	int i;

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	sc->sc_data = kmem_alloc(XBSEEPROM_SIZE, KM_SLEEP);
	if (sc->sc_data == NULL) {
		aprint_error(": Not enough memory to copy EEPROM\n");
		return;
	}

	aprint_normal(": Xbox Serial EEPROM\n");

	/* Read in EEPROM contents */
	ptr = (uint8_t *)sc->sc_data;
	for (i = 0; i < XBSEEPROM_SIZE; i++, ptr++)
		xbseeprom_read_1(sc, i, ptr);

#if 0
	/* Display some interesting information */
	aprint_normal_dev(self, "MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
	    sc->sc_data->MACAddress[0], sc->sc_data->MACAddress[1],
	    sc->sc_data->MACAddress[2], sc->sc_data->MACAddress[3],
	    sc->sc_data->MACAddress[4], sc->sc_data->MACAddress[5]
	    );
#endif

	/* Setup sysctl subtree */
	memset(xbseeprom_serial, 0, sizeof(xbseeprom_serial));
	memcpy(xbseeprom_serial, sc->sc_data->SerialNumber, 16);

	sysctl_createv(NULL, 0, NULL, NULL,
	    0, CTLTYPE_STRING, "xbox_serial",
	    SYSCTL_DESCR("Xbox serial number"),
	    NULL, 0, &xbseeprom_serial, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	return;
}

static void
xbseeprom_read_1(struct xbseeprom_softc *sc, uint8_t cmd, uint8_t *valp)
{
	iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr, &cmd, 1, valp, 1, I2C_F_POLL);
}
