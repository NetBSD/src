/*	$NetBSD: xboxlcd.c,v 1.2.36.1 2007/12/26 19:42:27 ad Exp $	*/

/*-
 * Copyright (c) 2006 Andrew Gillham
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
 *
 */

/*
 *  Driver for Xecuter X3 LCD add-on
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xboxlcd.c,v 1.2.36.1 2007/12/26 19:42:27 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <machine/bus.h>
#include <machine/xbox.h>
#include <machine/pio.h>

void xbox_lcd_init(void)
{
	outb(X3LCD_O_DAT, 0);
	outb(X3LCD_O_CMD, 0);
	outb(X3LCD_O_DIR_DAT, 0xFF);
	outb(X3LCD_O_DIR_CMD, 0x07);

	xbox_lcd_output(X3LCD_FUNC_SET | X3LCD_DL_FLAG, X3LCD_INI);
	delay(50000);
	xbox_lcd_output(X3LCD_FUNC_SET | X3LCD_DL_FLAG, X3LCD_INI);
	delay(50000);
	xbox_lcd_output(X3LCD_FUNC_SET | X3LCD_DL_FLAG, X3LCD_INI);
	delay(50000);
	xbox_lcd_output(X3LCD_FUNC_SET, X3LCD_INI);
	delay(50000);
	xbox_lcd_output(X3LCD_FUNC_SET | X3LCD_DL_FLAG | X3LCD_N_FLAG, X3LCD_CMD);
	delay(50000);
	xbox_lcd_output(X3LCD_CONTROL | X3LCD_D_FLAG, X3LCD_CMD);
	delay(50000);
	xbox_lcd_output(X3LCD_CLEAR, X3LCD_CMD);
	delay(50000);
	xbox_lcd_output(X3LCD_ENTRY_MODE | X3LCD_ID_FLAG, X3LCD_CMD);
	delay(50000);

	/* xbox_lcd_setpos(0,0); */

	outb(X3LCD_O_LIGHT, 81);
	delay(50000);
}

void xbox_lcd_output(unsigned char data, unsigned char command)
{
	unsigned char cmd = 0;
	int s;

	if (command & X3LCD_DAT)
	{
		cmd |= X3LCD_DISPCON_RS;
	}

	/* output upper nibble of data */
	/* outb(X3LCD_O_DAT, data & 0xF0); */
/*
	outb(X3LCD_O_DAT, data);
	outb(X3LCD_O_CMD, cmd);
	outb(X3LCD_O_CMD, X3LCD_DISPCON_E | cmd);
	outb(X3LCD_O_CMD, cmd);
*/

	s = splhigh();
	/* only if not init command */
	if ((command & X3LCD_INI) == 0)
	{
		/* output lower nibble of data */
		/* outb(X3LCD_O_DAT, (data << 4) & 0xF0); */
		outb(X3LCD_O_DAT, data);
		outb(X3LCD_O_CMD, cmd);
		outb(X3LCD_O_CMD, X3LCD_DISPCON_E | cmd);
		outb(X3LCD_O_CMD, cmd);
	}
	splx(s);
	delay(30000);
}

void xbox_lcd_writestring(char *lcdstring)
{
	unsigned char i;
	do {
		i = *lcdstring;
		if ( i == 0x00)
			break;

		xbox_lcd_output(i, X3LCD_DAT);
		i = *lcdstring++;
	} while(1);
}


void xbox_lcd_writetext(const char *lcdtext)
{
	unsigned char i;
	while ((i = *lcdtext++)) {
		if ((i > 31) && (i < 128)) {
			xbox_lcd_output(i, X3LCD_DAT);
		}
	}
}

void xbox_lcd_putchar(unsigned char data)
{
	if ((data >= ' ') && (data <= '~')) {
		xbox_lcd_output(data, X3LCD_DAT);
	}
}

void xbox_lcd_setpos(unsigned int pos, unsigned int line)
{
	xbox_lcd_output(X3LCD_DDRAM_SET | 0, X3LCD_CMD);
}

