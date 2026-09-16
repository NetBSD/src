/*	$NetBSD: pcf8584var.h,v 1.11 2026/09/16 06:51:01 jdc Exp $	*/
/*	$OpenBSD: pcf8584var.h,v 1.5 2007/10/20 18:46:21 kettenis Exp $ */

/*
 * Copyright (c) 2006 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _DEV_IC_PCF8584VAR_H_
#define	_DEV_IC_PCF8584VAR_H_

struct pcfiic_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;		/* Handle for bus */
	bus_space_handle_t	sc_mux_ioh;	/* Handle for mux */
	u_int8_t		sc_addr;	/* Our address */
	u_int8_t		sc_clock;	/* Our clock settings */
	u_int8_t		sc_regmap[2];

	bool			sc_has_mux;	/* We have 2 buses */
	bool			sc_poll;	/* Poll only (no intr) */

	int			sc_delay;	/* HW needs R, W delay */

	struct i2c_controller	sc_i2c;

	kmutex_t		sc_mutex;	/* Interrupt mutex ... */
	kcondvar_t		sc_cv;		/* ... and condvar */

	int			sc_op;		/* Operation to run */
	int			sc_stage;	/* Exec stage */
	i2c_addr_t		sc_target;	/* Saved exec args */
	const u_int8_t *	sc_cmdbuf;	/* ... */
	size_t			sc_cmdlen;	/* ... */
	u_int8_t *		sc_buf;		/* ... */
	size_t			sc_len;		/* ... */
	int			sc_i;		/* How far through buf */
	int			sc_err;		/* Error during transaction */
};

/*
 * The PCF8584 has only a single address input pin.  These are
 * indices into the sc_regmap[] that contain the offsets from
 * the base io handle needed to drive that pin according to the
 * desired register access.
 */
#define	PCF8584_S0		0
#define	PCF8584_S1		1

void	pcfiic_attach(struct pcfiic_softc *, i2c_addr_t, u_int8_t);
int	pcfiic_intr(void *);

/* i2c operations */
#define PCFIIC_OP_READ		0x01
#define PCFIIC_OP_WRITE		0x02
#define PCFIIC_OP_WR_RD		0x04

/* i2c stages */
#define PCFIIC_STAGE_IDLE	0x00
#define PCFIIC_STAGE_WRITE	0x01
#define PCFIIC_STAGE_READ	0x02
#define PCFIIC_STAGE_STOP	0x04
#endif /* _DEV_IC_PCF8584VAR_H_ */
