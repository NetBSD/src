/*	$NetBSD: gayle.c,v 1.6.122.2 2017/12/03 11:35:48 jdolecek Exp $	*/

/* public domain */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gayle.c,v 1.6.122.2 2017/12/03 11:35:48 jdolecek Exp $");

/*
 * Gayle management - provide functions for use in the drivers that utilise
 * the chip.
 *
 * These overly complicated routines try to deal with a few variants of
 * Gayle chip that exists. 
 */
#include <sys/bus.h>
#include <sys/systm.h>

#include <machine/cpu.h>

#include <amiga/amiga/gayle.h>
#include <amiga/dev/zbusvar.h>

/* #define GAYLE_DEBUG 1 */

struct gayle_tag {
	struct bus_space_tag gayle_io_bst;
	bus_space_tag_t gayle_io_t;
	bus_space_handle_t gayle_io_h;

	struct bus_space_tag gayle_isr_bst;
	bus_space_tag_t gayle_isr_t;
	bus_space_handle_t gayle_isr_h;
};

typedef struct gayle_tag *gayle_tag_t;

/*
 * Having these as static variables is ugly, but they don't fit into
 * driver's softc, as the chip might be utilised by many different drivers.
 * And since we can only have one Gayle per system it should be okay.
 */
static struct gayle_tag gayle;
static gayle_tag_t gayle_t = NULL;

/*
 * Any module that uses gayle should call gayle_init() before using anything
 *   related to gayle. gayle_init() can be called multiple times.
 */
void
gayle_init(void) {
	bus_addr_t gayle_vbase;

	if (gayle_t != NULL)
		return;

#ifdef GAYLE_DEBUG
	aprint_normal("gayle: doing init\n");
#endif /* GAYLE_DEBUG */

	gayle_t = &gayle;

	gayle_vbase = (bus_addr_t) __UNVOLATILE(ztwomap(GAYLE_REGS_BASE));

	gayle_t->gayle_io_bst.base = gayle_vbase;
	gayle_t->gayle_io_bst.absm = &amiga_bus_stride_0x1000;
	gayle_t->gayle_io_t = &(gayle_t->gayle_io_bst);

	bus_space_map(gayle_t->gayle_io_t, 0, 0x4, 0, &gayle_t->gayle_io_h);

	/* 
	 * The A4000 variant of Gayle has interrupt status register at offset
	 * +0x1000 from IDE registers.
	 * XXX: in fact, on A4000 we should initialise only this part...
	 */
	if (is_a4000()) {
		gayle_t->gayle_isr_bst.base = (bus_addr_t) __UNVOLATILE(ztwomap(
		    GAYLE_IDE_BASE_A4000+GAYLE_IDE_INTREQ_A4000));
		gayle_t->gayle_isr_bst.absm = &amiga_bus_stride_1;
		gayle_t->gayle_isr_t = &(gayle_t->gayle_isr_bst);

		bus_space_map(gayle_t->gayle_isr_t, 0, 0x1, 0,
		    &(gayle_t->gayle_isr_h));
	} else {
		bus_space_subregion(gayle_t->gayle_io_t, gayle_t->gayle_io_h, 
		    GAYLE_INTREQ, 0x1, &(gayle_t->gayle_isr_h));

		gayle_t->gayle_isr_bst = gayle_t->gayle_io_bst;
		gayle_t->gayle_isr_t = gayle_t->gayle_io_t;
	}

}

uint8_t 
gayle_intr_status(void)
{
	uint8_t rv;

	rv = bus_space_read_1(gayle_t->gayle_isr_t, gayle_t->gayle_isr_h, 0); 
#ifdef GAYLE_DEBUG
	aprint_normal("gayle: intr status %x\n", rv);
#endif /* GAYLE_DEBUG */

	return rv;
}

uint8_t
gayle_intr_enable_read(void)
{
	uint8_t rv;

	rv = bus_space_read_1(gayle_t->gayle_io_t, gayle_t->gayle_io_h, 
	    GAYLE_INTENA);
#ifdef GAYLE_DEBUG
	aprint_normal("gayle: intr enable register read %x\n", rv);
#endif /* GAYLE_DEBUG */

	return rv;
}

void
gayle_intr_enable_write(uint8_t val)
{
#ifdef GAYLE_DEBUG
	aprint_normal("gayle: intr enable register write %x\n", val);
#endif /* GAYLE_DEBUG */
	bus_space_write_1(gayle_t->gayle_io_t, gayle_t->gayle_io_h, 
	    GAYLE_INTENA, val);
}

void
gayle_intr_enable_set(uint8_t bits)
{
	uint8_t val;
	val = gayle_intr_enable_read();
	gayle_intr_enable_write(val | bits);
}

void 
gayle_intr_ack(uint8_t val)
{
#ifdef GAYLE_DEBUG
	aprint_normal("gayle: intr ack write %x\n", val);
#endif /* GAYLE_DEBUG */
	bus_space_write_1(gayle_t->gayle_io_t, gayle_t->gayle_io_h, 
	    GAYLE_INTREQ, val);
}

uint8_t
gayle_pcmcia_status_read(void)
{
	uint8_t rv;

	rv = bus_space_read_1(gayle_t->gayle_io_t, gayle_t->gayle_io_h, 
	    GAYLE_PCC_STATUS);
#ifdef GAYLE_DEBUG
	aprint_normal("gayle: pcmcia status read %x\n", rv);
#endif /* GAYLE_DEBUG */

	return rv;
}

void 
gayle_pcmcia_status_write(uint8_t val)
{
#ifdef GAYLE_DEBUG
	aprint_normal("gayle: pcmcia status write %x\n", val);
#endif /* GAYLE_DEBUG */
	bus_space_write_1(gayle_t->gayle_io_t, gayle_t->gayle_io_h, 
	    GAYLE_PCC_STATUS, val);
}

void 
gayle_pcmcia_config_write(uint8_t val)
{
#ifdef GAYLE_DEBUG
	aprint_normal("gayle: pcmcia config write %x\n", val);
#endif /* GAYLE_DEBUG */
	bus_space_write_1(gayle_t->gayle_io_t, gayle_t->gayle_io_h, 
	    GAYLE_PCC_CONFIG, val);
}

uint8_t
gayle_pcmcia_config_read(void)
{
	uint8_t rv;

	rv = bus_space_read_1(gayle_t->gayle_io_t, gayle_t->gayle_io_h, 
	    GAYLE_PCC_CONFIG);
#ifdef GAYLE_DEBUG
	aprint_normal("gayle: pcmcia config read %x\n", rv);
#endif /* GAYLE_DEBUG */

	return rv;
}

