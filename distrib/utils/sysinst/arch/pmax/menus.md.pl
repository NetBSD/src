/*	$NetBSD: menus.md.pl,v 1.1 2002/04/08 23:30:44 hubertf Exp $	*/
/* Based on english version: */
/*	NetBSD: menus.md.en,v 1.17 2001/11/29 23:21:00 thorpej Exp */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.
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

/* pmax machine dependent menus, Polish */

menu editfsparts, y=12, exit;
	display action  {
			ask_sizemult(dlcylsize);
			msg_display(MSG_fspart, multname);
			disp_cur_fspart(-1, 1);
		};
	option "Zmien a", action { editpart = A;}, sub menu edfspart;
	option "Zmien b", action { editpart = B;}, sub menu edfspart;
	option "Caly dysk - nie mozna zmienic", action {};
	option "Zmien d", action { editpart = D;}, sub menu edfspart;
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
	option  "Online Manual Pages", 	 action { toggle_getit (5); };
	option  "Miscellaneous", 	 action { toggle_getit (6); };
	option  "Text Processing Tools", action { toggle_getit (7); };
	option  "X11 base and clients",	 action { toggle_getit (8); };
	option  "X11 fonts",		 action { toggle_getit (9); };
	option  "X11 servers",		 action { toggle_getit (10); };
	option  "X contrib clients",	 action { toggle_getit (11); };
	option  "X11 programming",	 action { toggle_getit (12); };
	option	"X11 Misc.",		 action { toggle_getit (13); };
