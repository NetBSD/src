/*	$NetBSD: menus.md,v 1.2 2003/06/03 11:54:52 dsl Exp $	*/

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

menu editfsparts, y=13, exit;
	display action  {
			ask_sizemult(dlcylsize);
			msg_display(MSG_fspart, multname);
			disp_cur_fspart(-1, 1);
		};
	option MSG_Change_a, action { editpart = A;}, sub menu edfspart;
	option MSG_Change_b, action { editpart = B;}, sub menu edfspart;
	option MSG_NetBSD_partition_cant_change, action {};
	option MSG_Whole_disk_cant_change, action {};
	option MSG_Change_e, action { editpart = E;}, sub menu edfspart;
	option MSG_Change_f, action { editpart = F;}, sub menu edfspart;
	option MSG_Change_g, action { editpart = G;}, sub menu edfspart;
	option MSG_Change_h, action { editpart = H;}, sub menu edfspart;
	option MSG_Change_i, action { editpart = I;}, sub menu edfspart;
	option MSG_Change_j, action { editpart = J;}, sub menu edfspart;
	option MSG_Change_k, action { editpart = K;}, sub menu edfspart;
	option MSG_Change_l, action { editpart = L;}, sub menu edfspart;
	option MSG_Change_m, action { editpart = M;}, sub menu edfspart;
	option MSG_Change_n, action { editpart = N;}, sub menu edfspart;
	option MSG_Change_o, action { editpart = O;}, sub menu edfspart;
	option MSG_Change_p, action { editpart = P;}, sub menu edfspart;
	option MSG_Set_new_allocation_size, action { reask_sizemult(dlcylsize); };
 

menu md_distcustom, x=26, y=5, exit, title MSG_Selection_toggles_inclusion;
	display action { show_cur_distsets (); };
	option MSG_Kernel_GENERIC,		action { toggle_getit (0); };
	option MSG_Kernel_GENERIC_TINY,		action { toggle_getit (1); };
	option MSG_Kernel_GENERIC_LAPTOP,	action { toggle_getit (2); };
	option MSG_Kernel_GENERIC_DIAGNOSTIC,	action { toggle_getit (3); };
	option MSG_Kernel_GENERIC_PS2TINY,	action { toggle_getit (4); };
	option MSG_Base,			action { toggle_getit (5); };
	option MSG_System_etc,			action { toggle_getit (6); };
	option MSG_Compiler_Tools,		action { toggle_getit (7); };
	option MSG_Games,			action { toggle_getit (8); };
	option MSG_Online_Manual_Pages,		action { toggle_getit (9); };
	option MSG_Miscellaneous,		action { toggle_getit (10); };
	option MSG_Text_Processing_Tools,	action { toggle_getit (11); };
	option MSG_X11_base_and_clients,	action { toggle_getit (12); };
	option MSG_X11_fonts,			action { toggle_getit (13); };
	option MSG_X11_servers,			action { toggle_getit (14); };
	option MSG_X_contrib_clients,		action { toggle_getit (15); };
	option MSG_X11_programming,		action { toggle_getit (16); };
	option MSG_X11_Misc,			action { toggle_getit (17); };

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
