/*	$NetBSD: console.h,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#define	FB_LINEBYTES	2048
#define	FB_WIDTH	1284
#define	FB_HEIGHT	1024

#define	CONS_WIDTH	(FB_WIDTH / ROM_FONT_WIDTH)
#define	CONS_HEIGHT	(FB_HEIGHT / ROM_FONT_HEIGHT)

#define	X_INIT		0
#define	Y_INIT		0

#define	CONS_FG		0xff
#define	CONS_BG		0x00

/*
 * TR2, TR2A IPL CLUT.
 *
 * 1	 red
 * 2	 green
 * 4	 blue
 * 8	 yellow
 * 16	 cyan
 * 32	 magenta
 * 64	 light gray
 * 128	 dark gray
 * 255	 white
 * other black
 */

enum console_type {
	CONS_ROM,	/* ROM console I/O */
	CONS_FB_KSEG2,	/* direct fb device access via KSEG2 */
	CONS_FB_KSEG1,	/* direct fb device access via KSEG1 */
	CONS_SIO1,	/* serial console port 1 */
	CONS_SIO2,	/* serial console port 2 */
};

struct cons {
	void (*init)(void);
	void (*putc)(int, int, int);
	int (*getc)(void);
	int (*scan)(void);
	void (*scroll)(void);
	void (*cursor)(int, int);
	int x, y;
	enum console_type type;
	boolean_t erace_previous_cursor;
	boolean_t cursor_enable;
};

struct fb {
	uint8_t *fb_addr;
	uint32_t fb_size;
	uint8_t *font_addr;
	boolean_t active;
};

struct zskbd {
	volatile uint8_t *status;
	volatile uint8_t *data;
	const uint8_t *normal;
	const uint8_t *shift;
	const uint8_t *ctrl;
	const uint8_t *capslock;
	u_int keymap;
	int print;
};

struct zs {
	volatile uint8_t *csr;
	volatile uint8_t *data;
	int clock;
};

void fb_set_addr(uint32_t, uint32_t, uint32_t);
void *fb_get_addr(void);
void fb_init(void);
void fb_scroll(void);
void fb_drawchar(int, int, int);
void fb_drawfont(int, int, uint16_t *);
void fb_drawcursor(int, int);
void fb_clear(int, int, int, int, int);
void fb_copy(int, int, int, int, int, int);
void fb_active(boolean_t);

void zskbd_set_addr(uint32_t, uint32_t);
int zskbd_getc(void);
int zskbd_scan(void);
void zskbd_print_keyscan(int);

void zs_set_addr(uint32_t, uint32_t, int);

void cons_rom_scroll(void);
void cons_rom_init(void);
int rom_scan(void);

enum console_type console_type(void);
void console_init(void);
void console_cursor(boolean_t);

int cnscan(void);

extern struct cons cons;
extern struct zskbd zskbd;
extern struct fb fb;
extern struct zs zs;
