/*	$NetBSD: menus.md.pl,v 1.4 2003/05/07 19:02:54 dsl Exp $	*/

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

menu getboottype, title "Wybor bootblokow";
	option "Uzyj normalnych bootblokow", exit, action
	    {boottype = "normal";};
	option "Uzyj bootblokow na zewn. konsole (9600)", exit, action
	    {boottype = "serial9600";};
	option "Uzyj bootblokow na zewn. konsole (38400)", exit, action
	    {boottype = "serial38400";};
	option "Uzyj bootblokow na zewn. konsole (57600)", exit, action
	    {boottype = "serial57600";};
	option "Uzyj bootblokow na zewn. konsole (115200)", exit, action
	    {boottype = "serial115200";};

menu fullpart, title  "Wybierz";
	option "Uzyj tylko czesci dysku", exit, action  {usefull = 0;};
	option "Uzyj calego dysku", 	  exit, action  {usefull = 1;};

menu wdtype, title  "Wybierz typ";
	display action { msg_display (MSG_wdtype, diskdev); };
	option "IDE", 	exit;
	option "ESDI", 	exit, action
		{ msg_display (MSG_sectforward);
		  process_menu (MENU_yesno);
		  if (yesno)
			doessf = "sf:";
		};
	option "ST506", exit, action
		{ msg_display (MSG_sectforward);
		  process_menu (MENU_yesno);
		  if (yesno)
			doessf = "sf:";
		};


menu dlgeom, title "Wybierz opcje";
	display action { msg_display (MSG_dlgeom, diskdev, dlcyl, dlhead,
				dlsec, disk->dd_cyl, disk->dd_head,
				disk->dd_sec);
			};
	option "Uzyj prawdziwej geometrii", exit, action {
			dlcyl  = disk->dd_cyl;
			dlhead = disk->dd_head;
			dlsec  = disk->dd_sec;
		};
	option "Uzyj geometrii disklabel", exit, action {
			disk->dd_cyl = dlcyl;
			disk->dd_head = dlhead;
			disk->dd_sec = dlsec;
		};

menu editparttable, title  "Wybierz swoje partycje", exit;
	display action  { msg_display (MSG_editparttable);
			  disp_cur_part(&mbr.mbr_parts[0], activepart, -1);
			};
	option "Edytuj partycje 0",  sub menu editpart,
		action  { editpart = 0; };
	option "Edytuj partycje 1",  sub menu editpart,
		action  { editpart = 1; };
	option "Edytuj partycje 2",  sub menu editpart,
		action  { editpart = 2; };
	option "Edytuj partycje 3",  sub menu editpart,
		action  { editpart = 3; };
	option "Zmien specyfikator rozmiaru",
		action  { reask_sizemult(bcylsize); }; 

menu editpart, title  "Wybierz aby zmienic";
	display action { msg_display (MSG_editpart, editpart);
			   disp_cur_part(&mbr.mbr_parts[0], editpart, -1);
			   msg_display_add(MSG_newline);
			};
	option "Rodzaj", sub menu chooseid;
	option "Poczatek i rozmiar", action { edit_ptn_bounds(); };
	option "Ustaw aktywna", action { activepart = editpart; };
	option "Partycje OK", exit;

menu chooseid, title  "Rodzaj partycji?";
	option "NetBSD", 	exit,	action
	{
		part[editpart].mbrp_typ = MBR_PTYPE_NETBSD;
	};
	option "DOS < 32 Meg",	exit,	action
	{
		part[editpart].mbrp_typ = MBR_PTYPE_FAT16S;
	};
	option "DOS > 32 Meg",	exit,	action
	{
		part[editpart].mbrp_typ = MBR_PTYPE_FAT16B;
	};
	option "nie uzywana",	exit,	action
	{
		part[editpart].mbrp_typ = 0;
	};
	option "nie zmienia",	exit;

menu cyl1024;
	display action {
		msg_display(MSG_cyl1024);
	};
	option "Zmien MBR i disklabel", exit, action
	{
		/* XXX UGH */
		extern int c1024_resp;

		c1024_resp = 1;
	};
	option "Zmien disklabel", exit, action
	{
		extern int c1024_resp;

		c1024_resp = 2;
	};
	option "Uzyj, mimo to",	exit, action
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
	option "Zmien a", action { editpart = A;}, sub menu edfspart;
	option "Zmien b", action { editpart = B;}, sub menu edfspart;
	option "partycja NetBSD - nie mozna zmienic", action {};
	option "Caly dysk - nie mozna zmienic", action {};
	option "Zmien e", action { editpart = E;}, sub menu edfspart;
	option "Zmien f", action { editpart = F;}, sub menu edfspart;
	option "Zmien g", action { editpart = G;}, sub menu edfspart;
	option "Zmien h", action { editpart = H;}, sub menu edfspart;
	option "Zmien i", action { editpart = I;}, sub menu edfspart;
	option "Zmien j", action { editpart = J;}, sub menu edfspart;
	option "Zmien k", action { editpart = K;}, sub menu edfspart;
	option "Zmien l", action { editpart = L;}, sub menu edfspart;
	option "Zmien m", action { editpart = M;}, sub menu edfspart;
	option "Zmien n", action { editpart = N;}, sub menu edfspart;
	option "Zmien o", action { editpart = O;}, sub menu edfspart;
	option "Zmien p", action { editpart = P;}, sub menu edfspart;
	option "Ustaw nowy przydzial rozmiarow", action { reask_sizemult(dlcylsize); };
 

menu md_distcustom, x=26, y=5, exit, title "Wybierz";
	display action { show_cur_distsets (); };
	option	"Kernel (GENERIC)",	 action { toggle_getit (0); };
	option	"Kernel (GENERIC_TINY)", action { toggle_getit (1); };
	option	"Kernel (GENERIC_LAPTOP)", action { toggle_getit (2); };
	option	"Kernel (GENERIC_DIAGNOSTIC)", action { toggle_getit (3); };
	option	"Kernel (GENERIC_PS2TINY)", action { toggle_getit (4); };
	option	"Base",			 action { toggle_getit (5); };
	option	"System (/etc)",	 action { toggle_getit (6); };
	option  "Compiler Tools", 	 action { toggle_getit (7); };
	option  "Games", 		 action { toggle_getit (8); };
	option  "Online manual pages", 	 action { toggle_getit (9); };
	option  "Miscellaneous", 	 action { toggle_getit (10); };
	option  "Text Processing Tools", action { toggle_getit (11); };
	option  "X11 base and clients",	 action { toggle_getit (12); };
	option  "X11 fonts",		 action { toggle_getit (13); };
	option  "X11 servers",		 action { toggle_getit (14); };
	option  "X contrib clients",	 action { toggle_getit (15); };
	option  "X11 programming",	 action { toggle_getit (16); };
	option  "X11 Misc.",		 action { toggle_getit (17); };

menu biosonematch;
	option "To jest prawidlowa geometria", exit, action {
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
	option "Ustaw geometrie recznie", exit, action {
		set_bios_geom(dlcyl, dlhead, dlsec);
		biosdisk = NULL;
	};

menu biosmultmatch;
	option "Uzyj jednego z tych dyskow", exit, action {
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
	option "Ustaw geometrie recznie", exit, action {
		set_bios_geom(dlcyl, dlhead, dlsec);
		biosdisk = NULL;
	};

menu configbootsel, y=16, title  "Zmien bootmenu", exit;
        display action  { disp_bootsel(); };
        option "Edytuj wpis 0",
		action { edit_bootsel_entry(0); };
        option "Edytuj wpis 1",
		action { edit_bootsel_entry(1); };
        option "Edytuj wpis 2",
		action { edit_bootsel_entry(2); };
        option "Edytuj wpis 3",
		action { edit_bootsel_entry(3); };
	option "Ustaw opoznienie",
		action { edit_bootsel_timeout(); };
	option "Ustaw domyslna opcje", sub menu defaultbootsel;

menu defaultbootsel, title "Wybierz domyslna partycje/dysk do uruchomiania";
	option "Pierwsza aktywna partycja", exit,
		action { mbs->mbrb_defkey = SCAN_ENTER; };
	option "Partycja 0", exit,
		action { edit_bootsel_default_ptn(0); };
	option "Partycja 1", exit,
		action { edit_bootsel_default_ptn(1); };
	option "Partycja 2", exit,
		action { edit_bootsel_default_ptn(2); };
	option "Partycja 3", exit,
		action { edit_bootsel_default_ptn(3); };
	option "Dysk twardy 0", exit,
		action { edit_bootsel_default_disk(0); };
	option "Dysk twardy 1", exit,
		action { edit_bootsel_default_disk(1); };
	option "Dysk twardy 2", exit,
		action { edit_bootsel_default_disk(2); };
	option "Dysk twardy 3", exit,
		action { edit_bootsel_default_disk(3); };
	option "Dysk twardy 4", exit,
		action { edit_bootsel_default_disk(4); };
	option "Dysk twardy 5", exit,
		action { edit_bootsel_default_disk(5); };
