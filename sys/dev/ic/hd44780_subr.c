/* $NetBSD: hd44780_subr.c,v 1.1 2003/01/20 01:20:50 soren Exp $ */

/*
 * Copyright (c) 2002 Dennis I. Chernoivanov
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 * Subroutines for Hitachi HD44870 style displays
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hd44780_subr.c,v 1.1 2003/01/20 01:20:50 soren Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/types.h>
#include <sys/ioccom.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/hd44780reg.h>
#include <dev/ic/hd44780_subr.h>

static void	hd44780_init(struct hd44780_chip *);

/*
 * Finish device attach. sc_writereg, sc_readreg and sc_flags must be properly
 * initialized prior to this call.
 */
void
hd44780_attach_subr(sc)
	struct hd44780_chip *sc;
{
	/* Putc/getc are supposed to be set by platform-dependent code. */
	if ((sc->sc_rwrite == NULL) || (sc->sc_rread == NULL))
		sc->sc_dev_ok = 0;

	/* Make sure that HD_MAX_CHARS is enough. */
	if ((sc->sc_flags & HD_MULTILINE) && (2 * sc->sc_rows > HD_MAX_CHARS))
		sc->sc_dev_ok = 0;
	else if (sc->sc_rows > HD_MAX_CHARS)
		sc->sc_dev_ok = 0;

	if (sc->sc_dev_ok) {
		hd44780_init(sc);

		/* Turn display on and clear it. */
		hd44780_ir_write(sc, cmd_dispctl(1, 0, 0));
		hd44780_ir_write(sc, cmd_clear());
	}
}

/*
 * Initialize 4-bit or 8-bit connected device.
 */
static void
hd44780_init(sc)
	struct hd44780_chip *sc;
{
	u_int8_t cmd;

	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	iot = sc->sc_iot;
	ioh = sc->sc_ioir;

	if (sc->sc_irwrite == NULL)
		sc->sc_irwrite = sc->sc_rwrite;

	cmd = cmd_init(sc->sc_flags & HD_8BIT);
	sc->sc_irwrite(iot, ioh, cmd);
	delay(TIMEOUT_LONG);
	sc->sc_irwrite(iot, ioh, cmd);
	sc->sc_irwrite(iot, ioh, cmd);

	cmd = cmd_funcset(
			sc->sc_flags & HD_8BIT,
			sc->sc_flags & HD_MULTILINE,
			sc->sc_flags & HD_BIGFONT);

	if ((sc->sc_flags & HD_8BIT) == 0)
		sc->sc_irwrite(iot, ioh, cmd);

	/* Interface is set to the proper width, use normal 'write' op. */
	sc->sc_rwrite(iot, ioh, cmd);
	cmd = cmd_dispctl(0, 0, 0);
	sc->sc_rwrite(iot, ioh, cmd);
	cmd = cmd_clear();
	sc->sc_rwrite(iot, ioh, cmd);
	cmd = cmd_modset(1, 0);
	sc->sc_rwrite(iot, ioh, cmd);
}

/*
 * Standard hd44780 ioctl() functions.
 */
int
hd44780_ioctl_subr(sc, cmd, data)
	struct hd44780_chip *sc;
	u_long cmd;
	caddr_t data;
{
	u_int8_t tmp;
	int error = 0;

#define hd44780_io()	((struct hd44780_io *)data)
#define hd44780_info()	((struct hd44780_info*)data)
#define hd44780_ctrl()	((struct hd44780_dispctl*)data)

	switch (cmd) {
		/* Clear the LCD. */
		case HLCD_CLEAR:
			hd44780_ir_write(sc, cmd_clear());
			break;

		/* Move the cursor one position to the left. */
		case HLCD_CURSOR_LEFT:
			hd44780_ir_write(sc, cmd_shift(0, 0));
			break;

		/* Move the cursor one position to the right. */
		case HLCD_CURSOR_RIGHT:
			hd44780_ir_write(sc, cmd_shift(0, 1));
			break;	

		/* Control the LCD. */
		case HLCD_DISPCTL:
			hd44780_ir_write(sc, cmd_dispctl(
						hd44780_ctrl()->display_on,
						hd44780_ctrl()->cursor_on,
						hd44780_ctrl()->blink_on));
			break;

		/* Get LCD configuration. */
		case HLCD_GET_INFO:
			hd44780_info()->lines
				= (sc->sc_flags & HD_MULTILINE) ? 2 : 1;
			hd44780_info()->phys_rows = sc->sc_rows;
			hd44780_info()->virt_rows = sc->sc_vrows;
			hd44780_info()->is_wide = sc->sc_flags & HD_8BIT;
			hd44780_info()->is_bigfont = sc->sc_flags & HD_BIGFONT;
			hd44780_info()->kp_present = sc->sc_flags & HD_KEYPAD;
			break;


		/* Reset the LCD. */
		case HLCD_RESET:
			hd44780_ir_write(sc, cmd_init(sc->sc_flags & HD_8BIT));
			hd44780_ir_write(sc, cmd_init(sc->sc_flags & HD_8BIT));
			hd44780_ir_write(sc, cmd_init(sc->sc_flags & HD_8BIT));
			hd44780_ir_write(sc, cmd_init(sc->sc_flags & HD_8BIT));
			hd44780_ir_write(sc, cmd_modset(1, 0));
			hd44780_ir_write(sc, cmd_dispctl(1, 0, 0));
			hd44780_ir_write(sc, cmd_clear());
			break;

		/* Get the current cursor position. */
		case HLCD_GET_CURSOR_POS:
			hd44780_io()->dat = (hd44780_ir_read(sc) & 0x7f);
			break;

		/* Set the cursor position. */
		case HLCD_SET_CURSOR_POS:
			hd44780_ir_write(sc, cmd_ddramset(hd44780_io()->dat));
			break;

		/* Get the value at the current cursor position. */
		case HLCD_GETC:
			tmp = (hd44780_ir_read(sc) & 0x7f);
			hd44780_ir_write(sc, cmd_ddramset(tmp));
			hd44780_io()->dat = hd44780_dr_read(sc);
			break;

		/* Set the character at the cursor position + advance cursor. */
		case HLCD_PUTC:
			hd44780_dr_write(sc, hd44780_io()->dat);
			break;

		/* Shift display left. */
		case HLCD_SHIFT_LEFT:
			hd44780_ir_write(sc, cmd_shift(1, 0));
			break;

		/* Shift display right. */
		case HLCD_SHIFT_RIGHT:
			hd44780_ir_write(sc, cmd_shift(1, 1));
			break;

		/* Return home. */
		case HLCD_HOME:
			hd44780_ir_write(sc, cmd_rethome());
			break;

		/* Write a string to the LCD virtual area. */
		case HLCD_WRITE:
			error = hd44780_ddram_io(sc, hd44780_io(), HD_DDRAM_WRITE);
			break;	

		/* Read LCD virtual area. */
		case HLCD_READ:
			error = hd44780_ddram_io(sc, hd44780_io(), HD_DDRAM_READ);
			break;

		/* Write to the LCD visible area. */
		case HLCD_REDRAW:
			hd44780_ddram_redraw(sc, hd44780_io());
			break;

		/* Write raw instruction. */
		case HLCD_WRITE_INST:
			hd44780_ir_write(sc, hd44780_io()->dat);
			break;

		/* Write raw data. */
		case HLCD_WRITE_DATA:
			hd44780_dr_write(sc, hd44780_io()->dat);
			break;

		default:
			error = EINVAL;
	}

	return error;
}

/*
 * Read/write particular area of the LCD screen.
 */
int
hd44780_ddram_io(sc, io, dir)
	struct hd44780_chip *sc;
	struct hd44780_io *io;
	u_char dir;
{
	u_int8_t hi;
	u_int8_t addr;

	int error = 0;
	u_int8_t i = 0;

	if (io->dat < sc->sc_vrows) {
		hi = HD_ROW1_ADDR + sc->sc_vrows;
		addr = HD_ROW1_ADDR + io->dat;
		for (; (addr < hi) && (i < io->len); addr++, i++) {
			hd44780_ir_write(sc, cmd_ddramset(addr));
			if (dir == HD_DDRAM_READ)
				io->buf[i] = hd44780_dr_read(sc);
			else
				hd44780_dr_write(sc, io->buf[i]);
		}
	}
	if (io->dat < 2 * sc->sc_vrows) {
		hi = HD_ROW2_ADDR + sc->sc_vrows;
		if (io->dat >= sc->sc_vrows)
			addr = HD_ROW2_ADDR + io->dat - sc->sc_vrows;
		else
			addr = HD_ROW2_ADDR;
		for (; (addr < hi) && (i < io->len); addr++, i++) {
			hd44780_ir_write(sc, cmd_ddramset(addr));
			if (dir == HD_DDRAM_READ)
				io->buf[i] = hd44780_dr_read(sc);
			else
				hd44780_dr_write(sc, io->buf[i]);
		}
		if (i < io->len)
			io->len = i;
	} else {
		error = EINVAL;
	}
	return error;
}

/*
 * Write to the visible area of the display.
 */
void
hd44780_ddram_redraw(sc, io)
	struct hd44780_chip *sc;
	struct hd44780_io *io;
{
	u_int8_t i;

	hd44780_ir_write(sc, cmd_clear());
	hd44780_ir_write(sc, cmd_rethome());
	for (i = 0; (i < io->len) && (i < sc->sc_rows); i++) {
		hd44780_dr_write(sc, io->buf[i]);
	}
	hd44780_ir_write(sc, cmd_ddramset(HD_ROW2_ADDR));
	for (; (i < io->len); i++)
		hd44780_dr_write(sc, io->buf[i]);
}

#if defined(HD44780_STD_WIDE)
/*
 * Standard 8-bit version of 'sc_rwrite' (8-bit port, 8-bit access)
 */
void
hd44780_rwrite(iot, ioh, cmd)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_int8_t cmd;
{
	bus_space_write_1(iot, ioh, 0x00, cmd);
	delay(TIMEOUT_NORMAL);
}

/*
 * Standard 8-bit version of 'sc_rread' (8-bit port, 8-bit access)
 */
u_int8_t
hd44780_rread(iot, ioh)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
{
	delay(TIMEOUT_NORMAL);
	return bus_space_read_1(iot, ioh, 0x00);
}
#elif defined(HD44780_STD_SHORT)
/*
 * Standard 4-bit version of 'sc_irwrite' (4-bit port, 8-bit access)
 */
void
hd44780_irwrite(iot, ioh, cmd)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_int8_t cmd;
{
	/* first four instructions emitted in 8-bit mode */
	bus_space_write_1(iot, ioh, 0x00, hi_bits(cmd));
	delay(TIMEOUT_NORMAL);
}

/*
 * Standard 4-bit version of 'sc_rrwrite' (4-bit port, 8-bit access)
 */
void
hd44780_rwrite(iot, ioh, cmd)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_int8_t cmd;
{
	bus_space_write_1(iot, ioh, 0x00, hi_bits(cmd));
	bus_space_write_1(iot, ioh, 0x00, lo_bits(cmd));
	delay(TIMEOUT_NORMAL);
}

/*
 * Standard 4-bit version of 'sc_rread' (4-bit port, 8-bit access)
 */
u_int8_t
hd44780_rread(iot, ioh)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
{
	u_int8_t rd;
	u_int8_t dat;

	delay(TIMEOUT_NORMAL);
	rd = bus_space_read_1(iot, ioh, 0x00);
	dat = (rd & 0x0f) << 4;
	rd = bus_space_read_1(iot, ioh, 0x00);
	return (dat | (rd & 0x0f));
}
#endif
