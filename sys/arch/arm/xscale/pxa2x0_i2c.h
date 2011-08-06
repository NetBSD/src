/*	$NetBSD: pxa2x0_i2c.h,v 1.5 2011/08/06 03:42:11 kiyohara Exp $	*/
/*	$OpenBSD: pxa2x0_i2c.h,v 1.2 2005/05/26 03:52:07 pascoe Exp $	*/

/*
 * Copyright (c) 2005 Christopher Pascoe <pascoe@openbsd.org>
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

#ifndef _PXA2X0_I2C_H_
#define _PXA2X0_I2C_H_

#include <sys/bus.h>

struct pxa2x0_i2c_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	bus_addr_t sc_addr;
	bus_size_t sc_size;

	uint32_t sc_icr;
	uint32_t sc_isar;	/* I2C Slave Address */

	enum {
		PI2C_STAT_UNKNOWN = -2,
		PI2C_STAT_ERROR = -1,
		PI2C_STAT_INIT = 0,
		PI2C_STAT_SEND,
		PI2C_STAT_RECEIVE,
		PI2C_STAT_STOP,
	} sc_stat;

	uint32_t sc_flags;
#define	PI2CF_FAST_MODE		(1U << 0)
};

int	pxa2x0_i2c_attach_sub(struct pxa2x0_i2c_softc *);
int	pxa2x0_i2c_detach_sub(struct pxa2x0_i2c_softc *);
void	pxa2x0_i2c_init(struct pxa2x0_i2c_softc *);
void	pxa2x0_i2c_open(struct pxa2x0_i2c_softc *);
void	pxa2x0_i2c_close(struct pxa2x0_i2c_softc *);
int	pxa2x0_i2c_read(struct pxa2x0_i2c_softc *, u_char, u_char *);
int	pxa2x0_i2c_write(struct pxa2x0_i2c_softc *, u_char, u_char);
int	pxa2x0_i2c_write_2(struct pxa2x0_i2c_softc *, u_char, u_short);
int	pxa2x0_i2c_quick(struct pxa2x0_i2c_softc *, u_char, u_char);

int	pxa2x0_i2c_send_start(struct pxa2x0_i2c_softc *, int flags);
int	pxa2x0_i2c_send_stop(struct pxa2x0_i2c_softc *, int flags);
int	pxa2x0_i2c_initiate_xfer(struct pxa2x0_i2c_softc *, uint16_t, int);
int	pxa2x0_i2c_read_byte(struct pxa2x0_i2c_softc *, uint8_t *, int);
int	pxa2x0_i2c_write_byte(struct pxa2x0_i2c_softc *, uint8_t, int);

void	pxa2x0_i2c_reset(struct pxa2x0_i2c_softc *);
int	pxa2x0_i2c_wait(struct pxa2x0_i2c_softc *, int, int);

int	pxa2x0_i2c_poll(struct pxa2x0_i2c_softc *, int, char *, int);
int	pxa2x0_i2c_intr_sub(struct pxa2x0_i2c_softc *, int *, uint8_t *);

#endif	/* _PXA2X0_I2C_H_ */
