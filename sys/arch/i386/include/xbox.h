/* $NetBSD: xbox.h,v 1.3.6.2 2007/02/26 09:07:03 yamt Exp $ */

/*-
 * Copyright (c) 2005 Rink Springer
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
 * $FreeBSD: /repoman/r/ncvs/src/sys/i386/include/xbox.h,v 1.3.2.1 2006/08/23 16:28:03 rink Exp $
 */
#ifndef _MACHINE_XBOX_H_
#define _MACHINE_XBOX_H_

#define XBOX_LED_GREEN		0x0f
#define XBOX_LED_RED		0xf0
#define XBOX_LED_FLASHRED	0xa0
#define XBOX_LED_FLASHGREEN	0x03

#define XBOX_RAM_SIZE		(arch_i386_xbox_memsize * 1024 * 1024)
#define XBOX_FB_SIZE		(0x400000)
#define XBOX_FB_START		(0xf0000000 | (XBOX_RAM_SIZE - XBOX_FB_SIZE))
#define XBOX_FB_START_PTR	(0xFD600800)

/*
 * Defines for X3LCD controller
 */
#define X3LCD_O_LIGHT		0xF503
#define X3LCD_O_DAT		0xF504
#define X3LCD_O_CMD		0xF505
#define X3LCD_O_DIR_DAT		0xF506
#define X3LCD_O_DIR_CMD		0xF507
#define X3LCD_O_CTR_TIME	2
#define X3LCD_DISPCON_RS	0x01
#define X3LCD_DISPCON_RW	0x02
#define X3LCD_DISPCON_E		0x04

#define X3LCD_CMD		0x00
#define X3LCD_INI		0x01
#define X3LCD_DAT		0x02

#define X3LCD_CLEAR		0x01
#define X3LCD_HOME		0x02
#define X3LCD_ENTRY_MODE	0x04
#define X3LCD_S_FLAG		0x01
#define X3LCD_ID_FLAG		0x02

#define X3LCD_CONTROL		0x08
#define X3LCD_D_FLAG		0x04
#define X3LCD_C_FLAG		0x02
#define X3LCD_B_FLAG		0x01

#define X3LCD_FUNC_SET		0x20
#define X3LCD_DL_FLAG		0x10
#define X3LCD_N_FLAG		0x08
#define X3LCD_F_FLAG		0x04

#define X3LCD_DDRAM_SET		0x80

extern int arch_i386_is_xbox;
extern uint32_t arch_i386_xbox_memsize; /* Megabytes */

void xbox_setled(uint8_t);
void xbox_reboot(void);
void xbox_poweroff(void);
void xbox_startup(void);

void xbox_lcd_init(void);
void xbox_lcd_output(unsigned char data, unsigned char command);
void xbox_lcd_setpos(unsigned int pos, unsigned int line);
void xbox_lcd_writestring(char *lcdstring);
void xbox_lcd_writetext(const char *lcdtext);
void xbox_lcd_putchar(unsigned char data);

/* xboxfb related */
int xboxfb_cnattach(void);


#endif /* !_MACHINE_XBOX_H_ */
