/*
 * Copyright 2006 Kyma Systems LLC.
 * All rights reserved.
 *
 * Written by Sanjay Lal <sanjayl@kymasys.com>
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
 *      This product includes software developed for the NetBSD Project by
 *      Kyma Systems LLC.
 * 4. The name of Kyma Systems LLC may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY KYMA SYSTEMS LLC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL KYMA SYSTEMS LLC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* com device attachment to mainbus for MAMBO G5 simulator */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <sys/bus.h>
#include <dev/ofw/openfirm.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

struct com_mainbus_softc
{
    struct com_softc sc_com;    /* real "com" softc */
    void *sc_ih;                /* interrupt handler */
};

int com_mainbus_probe(device_t, cfdata_t , void *);
void com_mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_mainbus, sizeof(struct com_mainbus_softc),
              com_mainbus_probe, com_mainbus_attach, NULL, NULL);

int com_mainbus_attached = 0;
int
com_mainbus_probe(device_t parent, cfdata_t match, void *aux)
{
    struct confargs *ca = aux;

    if (strcmp(ca->ca_name, "com") != 0)
        return 0;

    if (!com_mainbus_attached) {
        com_mainbus_attached = 1;
        return (1);
    }
    else
        return 0;
}

void
com_mainbus_attach(device_t parent, device_t self, void *aux)
{
    struct com_mainbus_softc *msc = device_private(self);
    struct com_softc *sc = &msc->sc_com;
    int serial, interrupt_length;
    int interrupts[8];
    bus_space_tag_t iot;
    bus_space_handle_t ioh;
    bus_addr_t iobase;

    sc->sc_dev = self;

    serial = OF_finddevice("/ht@0/isa@4/serial@0x3f8");
    if (serial != -1) {
        interrupt_length =
            OF_getprop(serial, "interrupts", interrupts, sizeof(interrupts));
    }


    iot = (bus_space_tag_t)0xf4000000;
    iobase = 0x3f8;
    comcnattach(iot, iobase, 9600, 1843200, COM_TYPE_NORMAL, (CREAD | CS8));
    bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh);
    COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

    sc->sc_frequency = 1843200;

    com_attach_subr(sc);
#if 1
    msc->sc_ih =
        intr_establish(interrupts[0], IST_LEVEL, IPL_SERIAL, comintr, sc);

    if (msc->sc_ih == NULL)
        panic("failed to establish int handler");
#endif


}
