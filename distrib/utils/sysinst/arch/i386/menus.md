/*	$NetBSD: menus.md,v 1.7 2003/07/27 20:25:07 dsl Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* Menu definitions for sysinst. i386 version, machine dependent. */

menu getboottype, title MSG_Bootblocks_selection, y=10, exit;
	display action { msg_display(MSG_getboottype);
		if (((struct i386_boot_params *)arg)->bp_consdev == 0)
			msg_display_add(MSG_console_PC);
		else
			msg_display_add(MSG_console_com,
			((struct i386_boot_params *)arg)->bp_consdev - 1,
			((struct i386_boot_params *)arg)->bp_conspeed);
		};
	option MSG_Use_normal_bootblocks, action
	    {((struct i386_boot_params *)arg)->bp_consdev = 0;};
	option MSG_Use_serial_com0, action
	    {((struct i386_boot_params *)arg)->bp_consdev = 1;};
	option MSG_Use_serial_com1, action
	    {((struct i386_boot_params *)arg)->bp_consdev = 2;};
	option MSG_Use_serial_com2, action
	    {((struct i386_boot_params *)arg)->bp_consdev = 3;};
	option MSG_Use_serial_com3, action
	    {((struct i386_boot_params *)arg)->bp_consdev = 4;};
	option MSG_serial_baud_rate, sub menu consolebaud;
	option MSG_Use_existing_bootblocks, action
	    {((struct i386_boot_params *)arg)->bp_consdev = ~0;};

menu consolebaud, title MSG_serial_baud_rate, x=40, y=13;
	display action {
		switch (((struct i386_boot_params *)arg)->bp_conspeed) {
		case   9600: menu->cursel = 0; break;
		case  19200: menu->cursel = 1; break;
		case  38400: menu->cursel = 2; break;
		case  57600: menu->cursel = 3; break;
		case 115200: menu->cursel = 4; break;
		}};
	option "9600", exit, action
	    {((struct i386_boot_params *)arg)->bp_conspeed = 9600;};
	option "19200", exit, action
	    {((struct i386_boot_params *)arg)->bp_conspeed = 19200;};
	option "38400", exit, action
	    {((struct i386_boot_params *)arg)->bp_conspeed = 38400;};
	option "57600", exit, action
	    {((struct i386_boot_params *)arg)->bp_conspeed = 57600;};
	option "115200", exit, action
	    {((struct i386_boot_params *)arg)->bp_conspeed = 115200;};

menu biosonematch;
	option MSG_This_is_the_correct_geometry, exit, action { };
	option MSG_Set_the_geometry_by_hand, exit, action
	    {*(void **)arg = NULL;};

menu biosmultmatch;
	option MSG_Use_one_of_these_disks, exit, action {
		int sel;
		char res[4];

		do {
			strcpy(res, "0");
			msg_prompt_win(MSG_pickdisk, -1, -1, 0, 0,
					res, res, sizeof res);
			sel = atoi(res);
		} while (sel < 0 || sel >= *(int *)arg);
		*(int *)arg = sel;
	};
	option MSG_Set_the_geometry_by_hand, exit, action {
		*(int *)arg = -1;
	};
