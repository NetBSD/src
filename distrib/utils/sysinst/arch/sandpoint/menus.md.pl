/*	$NetBSD: menus.md.pl,v 1.3 2003/05/07 10:20:25 dsl Exp $	*/
/*	Based on english version: */
/*	NetBSD: menus.md.en,v 1.2 2001/11/29 23:21:01 thorpej Exp 	*/

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

/* Menu definitions for sysinst. sandpoint version, machine dependent. */

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
			  disp_cur_part(&mbr.mbr_parts[0], activepart,-1);
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
			   disp_cur_part(&mbr.mbr_parts[0], activepart,editpart);
			   msg_display_add(MSG_newline);
			};
	option "Rodzaj", sub menu chooseid;
	option "Poczatek i rozmiar", action 
		{	char buf[40]; int start, size;
			msg_prompt_add (MSG_start, NULL, buf, 40);
			start = NUMSEC(atoi(buf),sizemult,dlcylsize);
			if (sizemult > 1 && start < bsec)
				start = bsec;
			msg_prompt_add (MSG_size, NULL, buf, 40);
			size = NUMSEC(atoi(buf),sizemult,dlcylsize);
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
		part[editpart].mbrp_typ = 165;
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

menu editfsparts, y=12, exit;
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
	option	"Base",			 action { toggle_getit (1); };
	option	"System (/etc)",	 action { toggle_getit (2); };
	option  "Compiler Tools", 	 action { toggle_getit (3); };
	option  "Games", 		 action { toggle_getit (4); };
	option  "Online manual pages", 	 action { toggle_getit (5); };
	option  "Miscellaneous", 	 action { toggle_getit (6); };
	option  "Text Processing Tools", action { toggle_getit (7); };
	option  "X11 base and clients",	 action { toggle_getit (8); };
	option  "X11 fonts",		 action { toggle_getit (9); };
	option  "X11 servers",		 action { toggle_getit (10); };
	option  "X contrib clients",	 action { toggle_getit (11); };
	option  "X11 programming",	 action { toggle_getit (12); };
	option  "X11 Misc.",		 action { toggle_getit (13); };
