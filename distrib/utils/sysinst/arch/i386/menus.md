/*	$NetBSD: menus.md,v 1.4 2003/06/11 21:35:43 dsl Exp $	*/

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

menu getboottype, title MSG_Bootblocks_selection;
	option MSG_Use_normal_bootblocks, exit, action
	    {boottype = "normal";};
	option MSG_Use_serial_9600_bootblocks, exit, action
	    {boottype = "serial9600";};
	option MSG_Use_serial_38400_bootblocks, exit, action
	    {boottype = "serial38400";};
	option MSG_Use_serial_57600_bootblocks, exit, action
	    {boottype = "serial57600";};
	option MSG_Use_serial_115200_bootblocks, exit, action
	    {boottype = "serial115200";};

menu wdtype, title MSG_Select_type;
	display action { msg_display (MSG_wdtype, diskdev); };
	option "IDE",	exit;
	option "ESDI",	exit, action
		{ msg_display (MSG_sectforward);
		  process_menu (MENU_yesno, NULL);
		  if (yesno)
			doessf = "sf:";
		};
	option "ST506", exit, action
		{ msg_display (MSG_sectforward);
		  process_menu (MENU_yesno, NULL);
		  if (yesno)
			doessf = "sf:";
		};


menu dlgeom, title MSG_Choose_an_option;
	display action { msg_display (MSG_dlgeom, diskdev, dlcyl, dlhead,
				dlsec, disk->dd_cyl, disk->dd_head,
				disk->dd_sec);
			};
	option MSG_Use_real_geometry, exit, action {
			dlcyl  = disk->dd_cyl;
			dlhead = disk->dd_head;
			dlsec  = disk->dd_sec;
		};
	option MSG_Use_disklabel_geometry, exit, action {
			disk->dd_cyl = dlcyl;
			disk->dd_head = dlhead;
			disk->dd_sec = dlsec;
		};

menu cyl1024;
	display action {
		msg_display(MSG_cyl1024);
	};
	option MSG_Reedit_both_MBR_and_label, exit, action
	{
		/* XXX UGH */
		extern int c1024_resp;

		c1024_resp = 1;
	};
	option MSG_Reedit_the_label, exit, action
	{
		extern int c1024_resp;

		c1024_resp = 2;
	};
	option MSG_Use_it_anyway,	exit, action
	{	
		extern int c1024_resp;

		c1024_resp = 3;
	};

menu biosonematch;
	option MSG_This_is_the_correct_geometry, exit, action {
		extern struct disklist *disklist;
		extern struct nativedisk_info *nativedisk;
		struct biosdisk_info *bip;
		extern struct biosdisk_info *biosdisk;

		bip = &disklist->dl_biosdisks[nativedisk->ni_biosmatches[0]];
		bcyl = bip->bi_cyl;
		bhead = bip->bi_head;
		bsec = bip->bi_sec;
		biosdisk = bip;
	};
	option MSG_Set_the_geometry_by_hand, exit, action {
		set_bios_geom(dlcyl, dlhead, dlsec);
		biosdisk = NULL;
	};

menu biosmultmatch;
	option MSG_Use_one_of_these_disks, exit, action {
		extern struct disklist *disklist;
		extern struct nativedisk_info *nativedisk;
		struct biosdisk_info *bip;
		extern struct biosdisk_info *biosdisk;
		int sel;
		char res[80];

		do {
			strcpy(res, "0");
			msg_prompt(MSG_pickdisk, res, res, 80);
			sel = atoi(res);
		} while (sel < 0 || sel >= nativedisk->ni_nmatches);
		bip = &disklist->dl_biosdisks[nativedisk->ni_biosmatches[0]];
		bcyl = bip->bi_cyl;
		bhead = bip->bi_head;
		bsec = bip->bi_sec;
		biosdisk = bip;
	};
	option MSG_Set_the_geometry_by_hand, exit, action {
		set_bios_geom(dlcyl, dlhead, dlsec);
		biosdisk = NULL;
	};
