/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/ieee1394/ieee1394reg.h>
#include <dev/ieee1394/fwohcireg.h>

#include <dev/ieee1394/ieee1394var.h>
#include <dev/ieee1394/fwohcivar.h>

static const char * const ieee1394_speeds[] = { IEEE1394_SPD_STRINGS };

int
fwohci_init(struct fwohci_softc *sc)
{
	u_int32_t val;

	/* What dialect of OHCI is this device?
	 */
	val = OHCI_CSR_READ(sc, OHCI_REG_Version);
	printf("%s: OHCI %u.%u", sc->sc_sc1394.sc1394_dev.dv_xname,
	    OHCI_Version_GET_Version(val), OHCI_Version_GET_Revision(val));

	/* Is the Global UID ROM present?
	 */
	if ((val & OHCI_Version_GUID_ROM) == 0) {
		printf("\n%x: fatal: no global UID ROM\n", sc->sc_sc1394.sc1394_dev.dv_xname);
		return -1;
	}

	/* Extract the Global UID
	 */
	val = OHCI_CSR_READ(sc, OHCI_REG_GUIDHi);
	sc->sc_sc1394.sc1394_guid[0] = (val >> 24) & 0xff;
	sc->sc_sc1394.sc1394_guid[1] = (val >> 16) & 0xff;
	sc->sc_sc1394.sc1394_guid[2] = (val >>  8) & 0xff;
	sc->sc_sc1394.sc1394_guid[3] = (val >>  0) & 0xff;

	val = OHCI_CSR_READ(sc, OHCI_REG_GUIDLo);
	sc->sc_sc1394.sc1394_guid[4] = (val >> 24) & 0xff;
	sc->sc_sc1394.sc1394_guid[5] = (val >> 16) & 0xff;
	sc->sc_sc1394.sc1394_guid[6] = (val >>  8) & 0xff;
	sc->sc_sc1394.sc1394_guid[7] = (val >>  0) & 0xff;

	printf(", %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
	    sc->sc_sc1394.sc1394_guid[0], sc->sc_sc1394.sc1394_guid[1],
	    sc->sc_sc1394.sc1394_guid[2], sc->sc_sc1394.sc1394_guid[3],
	    sc->sc_sc1394.sc1394_guid[4], sc->sc_sc1394.sc1394_guid[5],
	    sc->sc_sc1394.sc1394_guid[6], sc->sc_sc1394.sc1394_guid[7]);

	/* Get the maximum link speed and receive size
	 */
	val = OHCI_CSR_READ(sc, OHCI_REG_BusOptions);
	sc->sc_sc1394.sc1394_link_speed =
	    (val & OHCI_BusOptions_LinkSpd_MASK)
		>> OHCI_BusOptions_LinkSpd_BITPOS;
	if (sc->sc_sc1394.sc1394_link_speed < IEEE1394_SPD_MAX) {
		printf(", %s", ieee1394_speeds[sc->sc_sc1394.sc1394_link_speed]);
	} else {
		printf(", unknown speed %u", sc->sc_sc1394.sc1394_link_speed);
	}

	/* MaxRec is encoded as log2(max_rec_octets)-1
	 */
	sc->sc_sc1394.sc1394_max_receive =
	    1 << (((val & OHCI_BusOptions_MaxRec_MASK)
		       >> OHCI_BusOptions_MaxRec_BITPOS) + 1);
	printf(", %u byte packets max", sc->sc_sc1394.sc1394_max_receive);

	printf("\n");
	return 0;
}

int
fwohci_intr(void *arg)
{
	struct fwohci_softc * const sc = arg;
	int progress = 0;

	for (;;) {
		u_int32_t intmask = OHCI_CSR_READ(sc, OHCI_REG_IntEventClear);
		if (intmask == 0)
			return progress;
		OHCI_CSR_WRITE(sc, OHCI_REG_IntEventClear, intmask);
		progress = 1;
	}
}
