/*	$NetBSD: mlx_eisa.c,v 1.2 2001/05/06 20:34:41 ad Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * EISA front-end for mlx(4) driver.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/eisa/eisavar.h>
#include <dev/eisa/eisadevs.h>

#include <dev/ic/mlxreg.h>
#include <dev/ic/mlxio.h>
#include <dev/ic/mlxvar.h>

#define MLX_EISA_SLOT_OFFSET		0x0c89
#define MLX_EISA_IOSIZE			(0x0ce0 - MLX_EISA_SLOT_OFFSET)
#define MLX_EISA_IOCONF1		(0x0cc1 - MLX_EISA_SLOT_OFFSET)
#define MLX_EISA_IOCONF2		(0x0cc3 - MLX_EISA_SLOT_OFFSET)

static void	mlx_eisa_attach(struct device *, struct device *, void *);
static int	mlx_eisa_match(struct device *, struct cfdata *, void *);

static int	mlx_v1_submit(struct mlx_softc *, struct mlx_ccb *);
static int	mlx_v1_findcomplete(struct mlx_softc *, u_int *, u_int *);
static void	mlx_v1_intaction(struct mlx_softc *, int);
static int	mlx_v1_fw_handshake(struct mlx_softc *, int *, int *, int *);
#ifdef MLX_RESET
static int	mlx_v1_reset(struct mlx_softc *);
#endif

struct cfattach mlx_eisa_ca = {
	sizeof(struct mlx_softc), mlx_eisa_match, mlx_eisa_attach
};

static const char * const mlx_eisa_prod[] = {
	"MLX0070",
	"MLX0071",
	"MLX0072",
	"MLX0073",
	"MLX0074",
	"MLX0075",
	"MLX0076",
	"MLX0077",
};

static int
mlx_eisa_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct eisa_attach_args *ea;
	int i;

	ea = aux;

	for (i = 0; i < sizeof(mlx_eisa_prod) / sizeof(mlx_eisa_prod[0]); i++)
		if (strcmp(ea->ea_idstring, mlx_eisa_prod[i]) == 0)
			return (1);

	return (0);
}

static void
mlx_eisa_attach(struct device *parent, struct device *self, void *aux)
{
	struct eisa_attach_args *ea;
	bus_space_handle_t ioh;
	eisa_chipset_tag_t ec;
	eisa_intr_handle_t ih;
	struct mlx_softc *mlx;
	bus_space_tag_t iot;
	const char *intrstr;
	int irq, le;
	
	ea = aux;
	mlx = (struct mlx_softc *)self;
	iot = ea->ea_iot;
	ec = ea->ea_ec;
	
	if (bus_space_map(iot, EISA_SLOT_ADDR(ea->ea_slot) +
	    MLX_EISA_SLOT_OFFSET, MLX_EISA_IOSIZE, 0, &ioh)) {
		printf("can't map i/o space\n");
		return;
	}

	mlx->mlx_iot = iot;
	mlx->mlx_ioh = ioh;
	mlx->mlx_dmat = ea->ea_dmat;

	/* 
	 * Map and establish the interrupt.
	 */
	switch (bus_space_read_1(iot, ioh, MLX_EISA_IOCONF1) & 0xf0) {
	case 0xa0:
		irq = 11;
		break;
	case 0xc0:
		irq = 12;
		break;
	case 0xe0:
		irq = 14;
		break;
	case 0x80:
		irq = 15;
		break;
	default:
		printf("controller on invalid IRQ\n");
		return;
	}

	if (eisa_intr_map(ec, irq, &ih)) {
		printf("can't map interrupt (%d)\n", irq);
		return;
	}

	if ((bus_space_read_1(iot, ioh, MLX_EISA_IOCONF1) & 0x08) != 0)
		le = IST_LEVEL;
	else
		le = IST_EDGE;

	intrstr = eisa_intr_string(ec, ih);
	mlx->mlx_ih = eisa_intr_establish(ec, ih, le, IPL_BIO, mlx_intr, mlx);
	if (mlx->mlx_ih == NULL) {
		printf("can't establish interrupt");
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}

	mlx->mlx_flags = MLXF_EISA;

	mlx->mlx_submit = mlx_v1_submit;
	mlx->mlx_findcomplete = mlx_v1_findcomplete;
	mlx->mlx_intaction = mlx_v1_intaction;
	mlx->mlx_fw_handshake = mlx_v1_fw_handshake;
#ifdef MLX_RESET
	mlx->mlx_reset = mlx_v1_reset;
#endif

	mlx_init(mlx, intrstr);
}

/*
 * ================= V1 interface linkage =================
 */

/*
 * Try to give (mc) to the controller.  Returns 1 if successful, 0 on
 * failure (the controller is not ready to take a command).
 *
 * Must be called at splbio or in a fashion that prevents reentry.
 */
static int
mlx_v1_submit(struct mlx_softc *mlx, struct mlx_ccb *mc)
{

	/* Ready for our command? */
	if ((mlx_inb(mlx, MLX_V1REG_IDB) & MLX_V1_IDB_FULL) == 0) {
		/* Copy mailbox data to window. */
		bus_space_write_region_1(mlx->mlx_iot, mlx->mlx_ioh,
		    MLX_V1REG_MAILBOX, mc->mc_mbox, MLX_V1_MAILBOX_LEN);
		bus_space_barrier(mlx->mlx_iot, mlx->mlx_ioh,
		    MLX_V1REG_MAILBOX, MLX_V1_MAILBOX_LEN,
		    BUS_SPACE_BARRIER_WRITE);

		/* Post command. */
		mlx_outb(mlx, MLX_V1REG_IDB, MLX_V3_IDB_FULL);
		return (1);
	}

	return (0);
}

/*
 * See if a command has been completed, if so acknowledge its completion and
 * recover the slot number and status code.
 *
 * Must be called at splbio or in a fashion that prevents reentry.
 */
static int
mlx_v1_findcomplete(struct mlx_softc *mlx, u_int *slot, u_int *status)
{

	/* Status available? */
	if ((mlx_inb(mlx, MLX_V3REG_ODB) & MLX_V3_ODB_SAVAIL) != 0) {
		*slot = mlx_inb(mlx, MLX_V1REG_MAILBOX + 0x0d);
		*status = mlx_inw(mlx, MLX_V1REG_MAILBOX + 0x0e);

		/* Acknowledge completion. */
		mlx_outb(mlx, MLX_V1REG_ODB, MLX_V1_ODB_SAVAIL);
		mlx_outb(mlx, MLX_V1REG_IDB, MLX_V1_IDB_SACK);
		return (1);
	}

	return (0);
}

/*
 * Enable/disable interrupts as requested. (No acknowledge required)
 *
 * Must be called at splbio or in a fashion that prevents reentry.
 */
static void
mlx_v1_intaction(struct mlx_softc *mlx, int action)
{

	mlx_outb(mlx, MLX_V1REG_IE, action ? 1 : 0);
}

/*
 * Poll for firmware error codes during controller initialisation.
 *
 * Returns 0 if initialisation is complete, 1 if still in progress but no
 * error has been fetched, 2 if an error has been retrieved.
 */
static int 
mlx_v1_fw_handshake(struct mlx_softc *mlx, int *error, int *param1, int *param2)
{
	u_int8_t fwerror;

	/*
	 * First time around, enable the IDB interrupt and clear any
	 * hardware completion status.
	 */
	if ((mlx->mlx_flags & MLXF_FW_INITTED) == 0) {
		mlx_outb(mlx, MLX_V1REG_ODB_EN, 1);
		DELAY(1000);
		mlx_outb(mlx, MLX_V1REG_IDB, MLX_V1_IDB_SACK);
		DELAY(1000);
		mlx->mlx_flags |= MLXF_FW_INITTED;
	}

	/* Init in progress? */
	if ((mlx_inb(mlx, MLX_V1REG_IDB) & MLX_V1_IDB_INIT_BUSY) == 0)
		return (0);

	/* Test error value. */
	fwerror = mlx_inb(mlx, MLX_V1REG_ODB);

	if ((fwerror & MLX_V1_FWERROR_PEND) == 0)
		return (1);

	/* XXX Fetch status. */
	*error = fwerror & 0xf0;
	*param1 = -1;
	*param2 = -1;

	/* Acknowledge. */
	mlx_outb(mlx, MLX_V1REG_ODB, fwerror);

	return (2);
}

#ifdef MLX_RESET
/*
 * Reset the controller.  Return non-zero on failure.
 */
static int 
mlx_v1_reset(struct mlx_softc *mlx)
{
	int i;

	mlx_outb(mlx, MLX_V1REG_IDB, MLX_V1_IDB_SACK);
	delay(1000000);

	/* Wait up to 2 minutes for the bit to clear. */
	for (i = 120; i != 0; i--) {
		delay(1000000);
		if ((mlx_inb(mlx, MLX_V1REG_IDB) & MLX_V1_IDB_SACK) == 0)
			break;
	}
	if (i == 0)
		return (-1);

	mlx_outb(mlx, MLX_V1REG_ODB, MLX_V1_ODB_RESET);
	mlx_outb(mlx, MLX_V1REG_IDB, MLX_V1_IDB_RESET);

	/* Wait up to 5 seconds for the bit to clear... */
	for (i = 5; i != 0; i--) {
		delay(1000000);
		if ((mlx_inb(mlx, MLX_V1REG_IDB) & MLX_V1_IDB_RESET) == 0)
			break;
	}
	if (i == 0)
		return (-1);

	/* Wait up to 3 seconds for the other bit to clear... */
	for (i = 5; i != 0; i--) {
		delay(1000000);
		if ((mlx_inb(mlx, MLX_V1REG_ODB) & MLX_V1_ODB_RESET) == 0)
			break;
	}
	if (i == 0)
		return (-1);

	return (0);
}
#endif	/* MLX_RESET */
