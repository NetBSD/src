/* $NetBSD: hd44780_subr.h,v 1.1 2003/01/20 01:20:51 soren Exp $ */

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

#ifndef _DEV_IC_HD44780_SUBR_H_
#define _DEV_IC_HD44780_SUBR_H_

/* IOCTL definitions */
#define HLCD_DISPCTL		_IOW('h',   1, struct hd44780_dispctl)
#define	HLCD_RESET		_IO('h',    2)
#define	HLCD_CLEAR		_IO('h',    3)
#define	HLCD_CURSOR_LEFT	_IO('h',    4)
#define	HLCD_CURSOR_RIGHT	_IO('h',    5)
#define	HLCD_GET_CURSOR_POS	_IOR('h',   6, struct hd44780_io)
#define	HLCD_SET_CURSOR_POS	_IOW('h',   7, struct hd44780_io)
#define	HLCD_GETC		_IOR('h',   8, struct hd44780_io)
#define	HLCD_PUTC		_IOW('h',   9, struct hd44780_io)
#define	HLCD_SHIFT_LEFT		_IO('h',   10)
#define	HLCD_SHIFT_RIGHT	_IO('h',   11)
#define	HLCD_HOME		_IO('h',   12)
#define	HLCD_WRITE		_IOWR('h', 13, struct hd44780_io)
#define	HLCD_READ		_IOWR('h', 14, struct hd44780_io)
#define	HLCD_REDRAW		_IOW('h',  15, struct hd44780_io)
#define	HLCD_WRITE_INST		_IOW('h',  16, struct hd44780_io)
#define	HLCD_WRITE_DATA		_IOW('h',  17, struct hd44780_io)
#define HLCD_GET_INFO		_IOR('h',  18, struct hd44780_info)

struct hd44780_dispctl {
	u_char	display_on:1,
		blink_on:1,
		cursor_on:1;
};

struct hd44780_io {
	u_int8_t dat;
	u_int8_t len;
	u_int8_t buf[HD_MAX_CHARS];
};

struct hd44780_info {
	u_char	lines;
	u_char	phys_rows;
	u_char	virt_rows;

	u_char	is_wide:1,
		is_bigfont:1,
		kp_present:1;
};

#ifdef _KERNEL

/* HLCD driver structure */
struct hd44780_chip {
#define HD_8BIT			0x01	/* 8-bit if set, 4-bit otherwise */
#define HD_MULTILINE		0x02	/* 2 lines if set, 1 otherwise */
#define HD_BIGFONT		0x04	/* 5x10 if set, 5x8 otherwise */
#define HD_KEYPAD		0x08	/* if set, keypad is connected */
	u_char sc_flags;

	u_char sc_rows;			/* visible rows */
	u_char sc_vrows;		/* virtual rows (normally 40) */
	u_char sc_dev_ok;

	bus_space_tag_t sc_iot;

	bus_space_handle_t sc_ioir;	/* instruction register */
	bus_space_handle_t sc_iodr;	/* data register */

	/*
	 * This one is here to make initialization generic. If 4-bit
	 * connection is used, the device still starts as if it was
	 * 8-bit connected, so a special care is needed for such case.
	 * If set to NULL, normal 'sc_rwrite()' function will be used
	 * during initialization.
	 */
	void     (* sc_irwrite)(bus_space_tag_t, bus_space_handle_t, u_int8_t);

	/* Generic write/read byte entries. */
	void     (* sc_rwrite)(bus_space_tag_t, bus_space_handle_t, u_int8_t);
	u_int8_t (* sc_rread)(bus_space_tag_t, bus_space_handle_t);
};

#define hd44780_busy_wait(sc) \
	while((hd44780_ir_read(sc) & BUSY_FLAG) == BUSY_FLAG)

#define hd44780_ir_write(sc, dat) \
	do {								\
		hd44780_busy_wait(sc);					\
		(sc)->sc_rwrite((sc)->sc_iot, (sc)->sc_ioir, (dat));	\
	} while(0)

#define hd44780_ir_read(sc) \
	(sc)->sc_rread((sc)->sc_iot, (sc)->sc_ioir)

#define hd44780_dr_write(sc, dat) \
	(sc)->sc_rwrite((sc)->sc_iot, (sc)->sc_iodr, (dat))

#define hd44780_dr_read(sc) \
	(sc)->sc_rread((sc)->sc_iot, (sc)->sc_iodr)

void hd44780_attach_subr(struct hd44780_chip *);
int  hd44780_ioctl_subr(struct hd44780_chip *, u_long, caddr_t);
void hd44780_ddram_redraw(struct hd44780_chip *, struct hd44780_io *);

#define HD_DDRAM_READ	0x0
#define HD_DDRAM_WRITE	0x1
int  hd44780_ddram_io(struct hd44780_chip *, struct hd44780_io *, u_char);

#if defined(HD44780_STD_SHORT)
void     hd44780_irwrite(bus_space_tag_t, bus_space_handle_t, u_int8_t);
#endif

#if defined(HD44780_STD_WIDE) || defined(HD44780_STD_SHORT)
void     hd44780_rwrite(bus_space_tag_t, bus_space_handle_t, u_int8_t);
u_int8_t hd44780_rread(bus_space_tag_t, bus_space_handle_t);
#endif

#endif /* _KERNEL */

#endif /* _DEV_IC_HD44780_SUBR_H_ */
