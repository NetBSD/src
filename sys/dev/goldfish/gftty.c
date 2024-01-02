/*	$NetBSD: gftty.c,v 1.1 2024/01/02 07:29:39 thorpej Exp $	*/

/*-     
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *              
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Support for the Goldfish virtual Programmable Interrupt Controller.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gftty.c,v 1.1 2024/01/02 07:29:39 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <dev/goldfish/gfttyvar.h>

/*
 * Goldfish TTY registers.
 */
#define	GFTTY_PUT_CHAR		0x00	/* 8 bit output value */
#define	GFTTY_BYTES_READY	0x04	/* number of input bytes available */
#define	GFTTY_CMD		0x08	/* command */
#define	GFTTY_DATA_PTR		0x10	/* DMA pointer */
#define	GFTTY_DATA_LEN		0x14	/* DMA length */
#define	GFTTY_DATA_PTR_HIGH	0x18	/* DMA pointer (64-bit) */
#define	GFTTY_VERSION		0x20	/* TTY version */

#define	CMD_INT_DISABLE		0x00
#define	CMD_INT_ENABLE		0x01
#define	CMD_WRITE_BUFFER	0x02
#define	CMD_READ_BUFFER		0x03

#define	REG_READ0(c, r)		\
	bus_space_read_4((c)->c_bst, (c)->c_bsh, (r))
#define	REG_WRITE0(c, r, v)	\
	bus_space_write_4((c)->c_bst, (c)->c_bsh, (r), (v))

#define	REG_READ(sc, r)		REG_READ0((sc)->sc_config, (r))
#define	REG_WRITE(sc, r, v)	REG_WRITE0((sc)->sc_config, (r), (v))

static int	gftty_cngetc(dev_t);
static void	gftty_cnputc(dev_t, int);
static void	gftty_cnpollc(dev_t, int);

static struct gftty_config gftty_cnconfig;
static struct cnm_state gftty_cnmagic_state;
static struct consdev gftty_consdev = {
	.cn_getc	=	gftty_cngetc,
	.cn_putc	=	gftty_cnputc,
	.cn_pollc	=	gftty_cnpollc,
	.cn_dev		=	NODEV,
	.cn_pri		=	CN_NORMAL,
};

/*
 * gftty_attach --
 *	Attach a Goldfish virual TTY.
 */
void
gftty_attach(struct gftty_softc *sc)
{

	aprint_naive("\n");
	aprint_normal(": Google Goldfish TTY\n");

	/* If we got here without a config, we're the console. */
	if (sc->sc_config == NULL) {
		KASSERT(gftty_is_console(sc));
		sc->sc_config = &gftty_cnconfig;
		aprint_normal_dev(sc->sc_dev, "console\n");
	}
}

/*
 * gftty_is_console --
 *	Returns true if the specified gftty instance is currently
 *	the console.
 */
bool
gftty_is_console(struct gftty_softc *sc)
{
	if (cn_tab == &gftty_consdev) {
		bool val;

		if (prop_dictionary_get_bool(device_properties(sc->sc_dev),
					     "is-console", &val)) {
			return val;
		}
	}
	return false;
}

/*
 * gftty_init_config --
 *	Initialize a config structure.
 */
static void
gftty_init_config(struct gftty_config *c, bus_space_tag_t bst,
    bus_space_handle_t bsh)
{
	c->c_bst = bst;
	c->c_bsh = bsh;
	c->c_version = REG_READ0(c, GFTTY_VERSION);
}

/*
 * gftty_alloc_config --
 *	Allocate a config structure, initialize it, and assign
 *	it to this device.
 */
void
gftty_alloc_config(struct gftty_softc *sc, bus_space_tag_t bst,
    bus_space_handle_t bsh)
{
	struct gftty_config *c = kmem_zalloc(sizeof(*c), KM_SLEEP);

	gftty_init_config(c, bst, bsh);
	sc->sc_config = c;
}

/*
 * gftty_set_buffer --
 *	Set the buffer address / length for an I/O operation.
 */
static void
gftty_set_buffer(struct gftty_config *c, bus_addr_t addr, bus_size_t size)
{
	REG_WRITE0(c, GFTTY_DATA_PTR, BUS_ADDR_LO32(addr));
	if (sizeof(bus_addr_t) == 8) {
		REG_WRITE0(c, GFTTY_DATA_PTR_HIGH, BUS_ADDR_HI32(addr));
	}
	REG_WRITE0(c, GFTTY_DATA_LEN, (uint32_t)size);
}

/*
 * gftty console routines.
 */
static int
gftty_cngetc(dev_t dev)
{
	struct gftty_config * const c = &gftty_cnconfig;

	if (REG_READ0(c, GFTTY_BYTES_READY) == 0) {
		return -1;
	}

	/*
	 * XXX This is all terrible and should burn to the ground.
	 * XXX This device desperately needs to be improved with
	 * XXX a GET_CHAR register.
	 */
	bus_addr_t addr;
	uint8_t buf[1];

	if (c->c_version == 0) {
		addr = (bus_addr_t)buf;
	} else {
		addr = vtophys((vaddr_t)buf);
	}

	gftty_set_buffer(c, addr, sizeof(buf));
	REG_WRITE0(c, GFTTY_CMD, CMD_READ_BUFFER);

	return buf[0];
}

static void
gftty_cnputc(dev_t dev, int ch)
{
	REG_WRITE0(&gftty_cnconfig, GFTTY_PUT_CHAR, (unsigned char)ch);
}

static void
gftty_cnpollc(dev_t dev, int on)
{
	/* XXX */
}

/*
 * gftty_cnattach --
 *	Attach a Goldfish virtual TTY console.
 */
void
gftty_cnattach(bus_space_tag_t bst, bus_space_handle_t bsh)
{
	gftty_init_config(&gftty_cnconfig, bst, bsh);

	cn_tab = &gftty_consdev;
	cn_init_magic(&gftty_cnmagic_state);
	cn_set_magic("+++++");
}
