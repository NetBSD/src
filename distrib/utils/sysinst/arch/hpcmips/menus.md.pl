/*	$NetBSD: menus.md.pl,v 1.1 2002/04/08 23:30:39 hubertf Exp $	*/
/*	Based on english version: */
/*	NetBSD: menus.md.en,v 1.8	2002/03/23 03:24:34 shin Exp */

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

/* Menu definitions for sysinst. hpcmips version, machine dependent. */

menu fullpart, title  "Wybierz";
	option "Uzyj tylko czesci dysku", exit, action  {usefull = 0;};
	option "Uzyj calego dysku", 	    exit, action  {usefull = 1;};

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
			  disp_cur_part((struct mbr_partition *)&mbr[MBR_PARTOFF
], activepart,-1);
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
			   disp_cur_part((struct mbr_partition *)&mbr[MBR_PARTOFF
], editpart,-1);
			   msg_display_add(MSG_newline);
			};
	option "Rodzaj", sub menu chooseid;
	option "Poczatek i rozmiar", action 
		{	char buf[40]; int start, size, inp, partn;

			msg_table_add(MSG_mbrpart_start_special);
			msg_prompt_add (MSG_start, NULL, buf, 40);
			inp = atoi(buf);
			/*
			 * -0, -1, -2, -3: start at end of part # given
			 * 0: start of disk.
			 */
			if ((inp == 0 && buf[0] == '-') ||
			    (inp < 0 && inp >= -3)) {
				partn = -inp;
				start = part[partn].mbrp_start +
				    part[partn].mbrp_size;
			} else if (inp == 0)
				start = bsec;
			else
				start = NUMSEC(inp,sizemult,dlcylsize);

			if (sizemult > 1 && start < bsec)
				start = bsec;
			msg_table_add(MSG_mbrpart_size_special);
			msg_prompt_add (MSG_size, NULL, buf, 40);
			inp = atoi(buf);
			/*
			 * -0, -1, -2, -3: until start of part # given
			 * 0: end of disk
			 */
			if ((inp == 0 && buf[0] == '-') ||
			    (inp < 0 && inp >= -3)) {
				partn = -inp;
				size = part[partn].mbrp_start - start;
			} else if (inp == 0)
				size = dlsize - start;
			else
				size = NUMSEC(inp,sizemult,dlcylsize);
			if (sizemult > 1 && start == bsec)
				size -= bsec;
			if (start + size > bsize)
				size = bsize - start;
			if (size < 0) {
				size = 0;
				start = 0;
			}
			part[editpart].mbrp_start = start;
			part[editpart].mbrp_size = size;
		};
	option "Ustaw aktywna", action { activepart = editpart; };
	option "Partycje OK", exit;

menu chooseid, title  "Rodzaj partycji?";
	option "NetBSD", 	exit,	action
	{
		part[editpart].mbrp_typ = 169;
	};
	option "DOS < 32 Meg",	exit,	action
	{
		part[editpart].mbrp_typ = 4;
	};
	option "DOS > 32 Meg",	exit,	action
	{
		part[editpart].mbrp_typ = 6;
	};
	option "nie uzywana",	exit,	action
	{
		part[editpart].mbrp_typ = 0;
	};

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
	option "Ustaw nowy przydzial rozmiarow", action { reask_sizemult(dlcylsize); };
 

menu md_distcustom, x=26, y=5, exit, title "Wybierz";
	display action { show_cur_distsets (); };
	option	"Kernel (GENERIC)",	 action { toggle_getit (0); };
	option	"Kernel (TX3912)", action { toggle_getit (1); };
	option	"Base",			 action { toggle_getit (2); };
	option	"System (/etc)",	 action { toggle_getit (3); };
	option  "Compiler Tools", 	 action { toggle_getit (4); };
	option  "Games", 		 action { toggle_getit (5); };
	option  "Online manual pages", 	 action { toggle_getit (6); };
	option  "Miscellaneous", 	 action { toggle_getit (7); };
	option  "Text Processing Tools", action { toggle_getit (8); };
	option  "X11 base and clients",	 action { toggle_getit (9); };
	option  "X11 fonts",		 action { toggle_getit (10); };
	option  "X11 servers",		 action { toggle_getit (11); };
	option  "X contrib clients",	 action { toggle_getit (12); };
	option  "X11 programming",	 action { toggle_getit (13); };
	option  "X11 Misc.",		 action { toggle_getit (14); };

/*
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
        display action  { msg_display(MSG_configbootsel);
                          disp_bootsel((struct mbr_partition *)&mbr[MBR_PARTOFF], mbs);
			  msg_display_add(MSG_bootseltimeout, (1000 * mbs->timeo) / 18200);
			  msg_display_add(MSG_defbootselopt);
			  if (mbs->defkey == SCAN_ENTER)
			  	msg_display_add(MSG_defbootseloptactive);
			  else if (mbs->defkey < (SCAN_F1 + 4))
				msg_display_add(MSG_defbootseloptpart,
				    defbootselpart);
			  else
				msg_display_add(MSG_defbootseloptdisk,
				    defbootseldisk);
                        };
        option "Edytuj wpis 0",
		action {
			if (part[0].mbrp_typ != 0)
				msg_prompt(MSG_bootselitemname, mbs->nametab[0],
				    mbs->nametab[0], 8);
		};
        option "Edytuj wpis 1",
		action {
			if (part[1].mbrp_typ != 0)
				msg_prompt(MSG_bootselitemname, mbs->nametab[1],
				    mbs->nametab[1], 8);
		};
        option "Edytuj wpis 2",
		action {
			if (part[2].mbrp_typ != 0)
				msg_prompt(MSG_bootselitemname, mbs->nametab[2],
				    mbs->nametab[2], 8);
		};
        option "Edytuj wpis 3",
		action {
			if (part[3].mbrp_typ != 0)
				msg_prompt(MSG_bootselitemname, mbs->nametab[3],
				    mbs->nametab[3], 8);
		};
	option "Ustaw opoznienie",
		action {
			char tstr[8];
			unsigned timo;

			do {
				snprintf(tstr, 8, "%u",
				    (1000 * mbs->timeo) / 18200);
				msg_prompt(MSG_bootseltimeoutval, tstr, tstr,
				    8);
				timo = (unsigned)atoi(tstr);
			} while (timo > 3600);
			mbs->timeo = (u_int16_t)((timo * 18200) / 1000);
		};
	option "Ustaw domyslna opcje", sub menu defaultbootsel;

menu defaultbootsel, title "Wybierz domyslna partycje/dysk do uruchomiania";
	option "Partycja 0", exit,
		action {
			if (mbs->nametab[0][0] != 0 && part[0].mbrp_typ != 0)
				mbs->defkey = SCAN_F1; defbootselpart = 0;
		};
	option "Partycja 1", exit,
		action {
			if (mbs->nametab[1][0] != 0 && part[1].mbrp_typ != 0)
				mbs->defkey = SCAN_F1 + 1; defbootselpart = 1;
		};
	option "Partycja 2", exit,
		action {
			if (mbs->nametab[2][0] != 0 && part[2].mbrp_typ != 0)
				mbs->defkey = SCAN_F1 + 2; defbootselpart = 2;
		};
	option "Partycja 3", exit,
		action {
			if (mbs->nametab[3][0] != 0 && part[3].mbrp_typ != 0)
				mbs->defkey = SCAN_F1 + 3; defbootselpart = 3;
		};
	option "Dysk twardy 0", exit,
		action { mbs->defkey = SCAN_F1 + 4; defbootseldisk = 0; };
	option "Dysk twardy 1", exit,
		action { mbs->defkey = SCAN_F1 + 5; defbootseldisk = 1; };
	option "Dysk twardy 2", exit,
		action { mbs->defkey = SCAN_F1 + 6; defbootseldisk = 2; };
	option "Dysk twardy 3", exit,
		action { mbs->defkey = SCAN_F1 + 7; defbootseldisk = 3; };
	option "Dysk twardy 4", exit,
		action { mbs->defkey = SCAN_F1 + 8; defbootseldisk = 4; };
	option "Dysk twardy 5", exit,
		action { mbs->defkey = SCAN_F1 + 9; defbootseldisk = 5; };
	option "Pierwsza aktywna partycja", exit,
		action { mbs->defkey = SCAN_ENTER; };
*/
