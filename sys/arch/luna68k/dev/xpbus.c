/*	$NetBSD: xpbus.c,v 1.1 2022/06/10 21:42:23 tsutsui Exp $	*/

/*-
 * Copyright (c) 2016 Izumi Tsutsui.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * LUNA's Hitachi HD647180 "XP" I/O processor
 */

/*
 * Specification of interrupts from XP to the host is confirmed
 * by Kenji Aoyama, in xptty(4) driver for OpenBSD/luna88k:
 *  https://gist.github.com/ao-kenji/790b0822e46a50ea63131cfa8d9110e7
 * and CP/M BIOS for HD647180 on LUNA:
 *  https://gist.github.com/ao-kenji/4f1e2b010f3b2b41ab07f3a8a3cc7484
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xpbus.c,v 1.1 2022/06/10 21:42:23 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/atomic.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/board.h>

#include <luna68k/dev/xpbusvar.h>
#include <luna68k/dev/xplxfirm.h>

/*
 * PIO 0 port C is connected to XP's reset line
 *
 * XXX: PIO port functions should be shared with machdep.c for DIP SWs
 */
#define PIO_ADDR	OBIO_PIO0_BASE
#define PORT_A		0
#define PORT_B		1
#define PORT_C		2
#define CTRL		3

/* PIO0 Port C bit definition */
#define XP_INT1_REQ	0	/* INTR B */
	/* unused */		/* IBF B */
#define XP_INT1_ENA	2	/* INTE B */
#define XP_INT5_REQ	3	/* INTR A */
#define XP_INT5_ENA	4	/* INTE A */
	/* unused */		/* IBF A */
#define PARITY		6	/* PC6 output to enable parity error */
#define XP_RESET	7	/* PC7 output to reset HD647180 XP */

/* Port control for PC6 and PC7 */
#define ON		1
#define OFF		0


struct xpbus_softc {
	device_t	sc_dev;
};

static const struct xpbus_attach_args xpdevs[] = {
	{ "xp" },
	{ "psgpam" },
};

static int xpbus_match(device_t, cfdata_t, void *);
static void xpbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(xpbus, sizeof(struct xpbus_softc),
    xpbus_match, xpbus_attach, NULL, NULL);

static bool xpbus_matched;

/*
 * xpbus acquired device sharing bitmap
 */
static volatile unsigned int xp_acquired;

/* XP SHM dirty flag */
static bool xp_shm_dirty = true;

static int
xpbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	/* only one XP processor */
	if (xpbus_matched)
		return 0;

	if (ma->ma_addr != XP_SHM_BASE)
		return 0;

	xpbus_matched = true;
	return 1;
}

static void
xpbus_attach(device_t parent, device_t self, void *aux)
{
	struct xpbus_softc *sc = device_private(self);
	struct xpbus_attach_args xa;
	int i;

	sc->sc_dev = self;
	aprint_normal("\n");

	for (i = 0; i < __arraycount(xpdevs); i++) {
		xa = xpdevs[i];
		config_found(self, &xa, NULL, CFARGS_NONE);
	}
}

/*
 * acquire xpbus from child devices
 * if success, return non-zero acquired map
 * if fail, return 0
 */
u_int
xp_acquire(int xplx_devid, u_int excl)
{

	for (;;) {
		unsigned int before, after;
		before = xp_acquired;
		if (before & XP_ACQ_EXCL)
			return 0;
		if (before & (1 << xplx_devid))
			return 0;
		after = before | (1 << xplx_devid) | excl;
		if (atomic_cas_uint(&xp_acquired, before, after) == before) {
			return after & ~(excl);
		}
	}
}

/* release xpbus by child devices */
void
xp_release(int xplx_devid)
{

	for (;;) {
		unsigned int before, after;
		before = xp_acquired;
		after = before & ~(1 << xplx_devid) & ~XP_ACQ_EXCL;
		if (atomic_cas_uint(&xp_acquired, before, after) == before) {
			return;
		}
	}
}

/* set the xp_shm_dirty flag */
void
xp_set_shm_dirty(void)
{

	xp_shm_dirty = true;
}

/* reload firmware if xp_shm_dirty */
void
xp_ensure_firmware(void)
{

	if (xp_shm_dirty) {
		/* firmware transfer */
		xp_cpu_reset_hold();
		delay(100);
		memcpy((void *)XP_SHM_BASE, xplx, xplx_size);
		/* XXX maybe not necessary */
		delay(100);
		xp_cpu_reset_release();
		xp_shm_dirty = false;
	}
}

/* PIO PORTC write */
uint8_t
put_pio0c(uint8_t bit, uint8_t set)
{
	volatile uint8_t * const pio0 = (uint8_t *)PIO_ADDR;

	pio0[CTRL] = (bit << 1) | (set & 0x01);

	return pio0[PORT_C];
}

/* hold XP RESET signal */
void
xp_cpu_reset_hold(void)
{

	put_pio0c(XP_RESET, ON);
}

/* release XP RESET signal */
void
xp_cpu_reset_release(void)
{

	put_pio0c(XP_RESET, OFF);
}

/* one-shot XP RESET signal */
void
xp_cpu_reset(void)
{

	xp_cpu_reset_hold();
	delay(100);
	xp_cpu_reset_release();
}

/* enable XP to Host interrupt 1 */
void
xp_intr1_enable(void)
{

	put_pio0c(XP_INT1_ENA, ON);
}

/* disable XP to Host interrupt 1 */
void
xp_intr1_disable(void)
{

	put_pio0c(XP_INT1_ENA, OFF);
}

/* interrupt 1 ack */
void
xp_intr1_acknowledge(void)
{

	/* reset the interrupt request: read PIO0 port A */
	/* XXX: probably */
	*(volatile uint8_t *)OBIO_PIO0A;
	/* XXX: just a guess */
	*(volatile uint8_t *)OBIO_PIO0B;
}

/* enable XP to Host interrupt 5 */
void
xp_intr5_enable(void)
{

	put_pio0c(XP_INT5_ENA, ON);
}

/* disable XP to Host interrupt 5 */
void
xp_intr5_disable(void)
{

	put_pio0c(XP_INT5_ENA, OFF);
}

/* interrupt 5 ack */
void
xp_intr5_acknowledge(void)
{

	/* reset the interrupt request: read PIO0 port A */
	(void)*(volatile uint8_t *)OBIO_PIO0A;
}

/* get XP shared memory pointer */
void *
xp_shmptr(int offset)
{

	return (uint8_t *)XP_SHM_BASE + offset;
}

/* read 1 byte */
int
xp_readmem8(int offset)
{

	return *((volatile uint8_t *)xp_shmptr(offset));
}

/* read 1 16bitLE */
int
xp_readmem16le(int offset)
{

	return le16toh(*(volatile uint16_t *)xp_shmptr(offset));
}

/* write 1 byte */
void
xp_writemem8(int offset, int v)
{

	*(volatile uint8_t *)(xp_shmptr(offset)) = v;
}

/* write 1 16bitLE */
void
xp_writemem16le(int offset, int v)
{

	*((volatile uint16_t *)xp_shmptr(offset)) = htole16((uint16_t)v);
}
