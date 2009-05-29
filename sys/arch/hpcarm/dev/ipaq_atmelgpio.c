/*	$NetBSD: ipaq_atmelgpio.c,v 1.15 2009/05/29 14:15:44 rjs Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA (ichiro@ichiro.org).
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
 * iPAQ uses Atmel microcontroller to service a few of peripheral devices.
 * This controller connect to UART1 of SA11x0.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipaq_atmelgpio.c,v 1.15 2009/05/29 14:15:44 rjs Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <hpcarm/dev/ipaq_saipvar.h>
#include <hpcarm/dev/ipaq_gpioreg.h>
#include <hpcarm/dev/ipaq_atmel.h>
#include <hpcarm/dev/ipaq_atmelvar.h>

#include <arm/sa11x0/sa11x0_gpioreg.h>
#include <arm/sa11x0/sa11x0_comreg.h>
#include <arm/sa11x0/sa11x0_reg.h>

#ifdef ATMEL_DEBUG
#define DPRINTF(x) aprint_normal x
#else
#define DPRINTF(x)
#endif

static	int	atmelgpio_match(device_t, cfdata_t, void *);
static	void	atmelgpio_attach(device_t, device_t, void *);
static	int	atmelgpio_print(void *, const char *);
static	int	atmelgpio_search(device_t, cfdata_t, const int *, void *);
static	void	atmelgpio_init(struct atmelgpio_softc *);

static	void	rxtx_data(struct atmelgpio_softc *, int, int,
			 uint8_t *, struct atmel_rx *);

CFATTACH_DECL_NEW(atmelgpio, sizeof(struct atmelgpio_softc),
    atmelgpio_match, atmelgpio_attach, NULL, NULL);

static int
atmelgpio_match(device_t parent, cfdata_t cf, void *aux)
{
	return (1);
}

static void
atmelgpio_attach(device_t parent, device_t self, void *aux)
{
	struct atmelgpio_softc *sc = device_private(self);
	struct ipaq_softc *psc = device_private(parent);

	struct atmel_rx rxbuf;

	aprint_normal("\n");
	aprint_normal_dev(self, "Atmel microcontroller GPIO\n");

	sc->sc_iot = psc->sc_iot;
	sc->sc_ioh = psc->sc_ioh;
	sc->sc_parent = psc;

	if (bus_space_map(sc->sc_iot, SACOM1_BASE, SACOM_NPORTS, 0,
                        &sc->sc_ioh)) {
                aprint_normal_dev(self, "unable to map of UART1 registers\n");
                return;
        }

	atmelgpio_init(sc);

	rxbuf.idx = 0;
	rxbuf.len = 0;

#if 1  /* this is sample */
	rxtx_data(sc, STATUS_BATTERY, 0, NULL, &rxbuf); 

	aprint_normal("ac_status          = %x\n", rxbuf.data[0]);
	aprint_normal("Battery kind       = %x\n", rxbuf.data[1]);
	aprint_normal("Voltage            = %d mV\n",
		1000 * (rxbuf.data[3] << 8 | rxbuf.data[2]) /228);
	aprint_normal("Battery Status     = %x\n", rxbuf.data[4]);
	aprint_normal("Battery percentage = %d\n",
		425 * (rxbuf.data[3] << 8 | rxbuf.data[2]) /1000 - 298);
#endif

	rxtx_data(sc, READ_IIC, 0, NULL, &rxbuf);

	/*
	 *  Attach each devices
	 */

	config_search_ia(atmelgpio_search, self, "atmelgpioif", NULL);
}

static int
atmelgpio_search(device_t parent, cfdata_t cf, const int *ldesc,
		 void *aux)
{
	if (config_match(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, atmelgpio_print);
	return 0;
}


static int
atmelgpio_print(void *aux, const char *name)
{
	return (UNCONF);
}

static void
atmelgpio_init(struct atmelgpio_softc *sc)
{
	/* 8 bits no parity 1 stop bit */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACOM_CR0, CR0_DSS);

	/* Set baud rate 115k */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACOM_CR1, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACOM_CR2, SACOMSPEED(115200));

	/* RX/TX enable, RX/TX FIFO interrupt enable */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACOM_CR3,
			 (CR3_RXE | CR3_TXE | CR3_RIE | CR3_TIE));
}

static void
rxtx_data(struct atmelgpio_softc  *sc, int id, int size, uint8_t *buf,
	  struct atmel_rx *rxbuf)
{
	int 		i, checksum, length, rx_data;
	uint8_t		data[MAX_SENDSIZE];

	length = size + FRAME_OVERHEAD_SIZE;

	while (! (bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACOM_SR0) & SR0_TFS))
		;

		data[0] = (uint8_t)FRAME_SOF; 
		data[1] = (uint8_t)((id << 4) | size);
		checksum = data[1];
		i = 2;
		while (size--)	{
			data[i++] = *buf;
			checksum += (uint8_t)(*buf++);
		}
		data[length-1] = checksum;

	while (! (bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACOM_SR1) & SR1_TNF))
		;
		i = 0;
		while (i < length)
			bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACOM_DR, data[i++]);

	delay(10000);
#if 0
	while (! (bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACOM_SR0) &
		 (SR0_RID | SR0_RFS)))
#endif
	rxbuf->state = STATE_SOF;
	while (bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACOM_SR1) & SR1_RNE) {

		rx_data = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACOM_DR);
			DPRINTF(("DATA = %x\n", rx_data));

		switch (rxbuf->state) {
		case STATE_SOF:
			if (rx_data == FRAME_SOF)
				rxbuf->state = STATE_ID;
			break;
		case STATE_ID:
			rxbuf->id = (rx_data & 0xf0) >> 4;
			rxbuf->len = rx_data & 0x0f;
			rxbuf->idx = 0;
			rxbuf->checksum = rx_data;
			rxbuf->state = (rxbuf->len > 0 ) ? STATE_DATA : STATE_EOF;
			break;
		case STATE_DATA:
			rxbuf->checksum += rx_data;
			rxbuf->data[rxbuf->idx] = rx_data;
			if (++rxbuf->idx == rxbuf->len)
				rxbuf->state = STATE_EOF;
			break;
		case STATE_EOF:
			rxbuf->state = STATE_SOF;
			if (rx_data == FRAME_EOF || rx_data == rxbuf->checksum)
				DPRINTF(("frame EOF\n"));
			else
				DPRINTF(("BadFrame\n"));
			break;
		default:
			break;
		}
	}
}
