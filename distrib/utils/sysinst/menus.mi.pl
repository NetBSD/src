/*	$NetBSD: menus.mi.pl,v 1.7 2002/06/02 15:05:24 zuntum Exp $	*/
/*	Based on english version: */
/*	NetBSD: menus.mi.en,v 1.49 2002/04/04 14:26:44 ad Exp 	*/

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

/*
 * Menu system definitions -- machine independent
 *
 * Some menus may be called directly in the code rather than via the 
 * menu system.
 *
 *  This file must be first in the sed command line.
 *
 */

{
#include <stdio.h>
#include <time.h>
#include <curses.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
}

default y=12, no exit, scrollable;

allow dynamic menus;

menu netbsd, title "System Instalacyjny NetBSD-@@VERSION@@",
    exit, exitstring "Wyjdz z Systemu Instalacyjnego";
	display action  { toplevel(); };
	option "Zainstaluj NetBSD na twardym dysku",
		action { do_install(); };
	option "Zaktualizuj NetBSD na twardym dysku",
		action { do_upgrade(); };
	option "Przeinstaluj albo zainstaluj dodatkowe pakiety",
		action { do_reinstall_sets(); };
	option "Zrestartuj komputer", exit,
		action (endwin) { system("/sbin/reboot -q"); };
	option "Menu Narzedziowe", sub menu utility;

menu utility, title "Narzedzia NetBSD-@@VERSION@@", exit;
	option "Uruchom /bin/sh",
		action (endwin) { system("/bin/sh"); };
/*	option "test", action { run_prog(RUN_DISPLAY, NULL, "/bin/pwd"); }; */
	option "Ustaw strefe czasowa", 
		action {
			set_timezone();
		};
	option "Skonfiguruj siec",
		action {
			extern int network_up;

			network_up = 0;
			config_network();
		};
/*	option "Skonfiguruj dysk"; XXX add later.  */
	option "Funkcje logowania", action { do_logging(); };
	option "Zatrzymaj system", exit,
		action (endwin) { system("/sbin/halt -q"); };

menu yesno, title "tak lub nie?";
	option "Tak", exit, action  {yesno = 1;};
	option "Nie",  exit, action  {yesno = 0;};

menu noyes, title "tak lub nie?";
	option "Nie",  exit, action  {yesno = 0;};
	option "Tak", exit, action  {yesno = 1;};

menu ok, title "Nacisnij enter aby kontynuowac";
	option "ok", exit;

menu layout, title  "Wybierz swoja instalacje";
	option "Standardowa", 	  exit, action { layoutkind = 1; md_set_no_x(); };
	option "Standardowa z X", exit, action { layoutkind = 2; };
	option "Inna", 	  exit, action { layoutkind = 3; };
	option "Istniejaca", 	  exit, action { layoutkind = 4; };

menu layoutparts, exit, title "Wybierz systemy plikow";
	display action { show_cur_filesystems (); };
	option "swap",		  action { layout_swap = (layout_swap & 1) - 1; };
	option "/usr",		  action { layout_usr = (layout_usr & 1) - 1; };
	option "/var",	 	  action { layout_var = (layout_var & 1) - 1; };
	option "/home",		  action { layout_home = (layout_home & 1) - 1; };
	option "/tmp (mfs)",	  action { layout_tmp = (layout_tmp & 1) - 1; };

menu sizechoice, title  "Wybierz specyfikator rozmiaru";
	option "Megabajty", exit, action 
		{ sizemult = MEG / sectorsize;
		  multname = msg_string(MSG_megname);
		};
	option "Cylindry", exit, action 
		{ sizemult = current_cylsize; 
		  multname = msg_string(MSG_cylname);
		};
	option "Sektory", exit, action 
		{ sizemult = 1; 
		  multname = msg_string(MSG_secname);
		};

menu fspartok, title "Partycje ok?", y=15;
	display action  {
		msg_display(MSG_fspart, multname);
		disp_cur_fspart(-1, 0);
	};
	option "Zmien partycje", sub menu editfsparts;
	option "Partycje sa ok", exit;

menu edfspart, title "Co zmienic?", exit, y=14;
	display action  {
		msg_display (MSG_edfspart, 'a'+editpart);
		disp_cur_fspart(editpart, 1);
	};
	option "SystemPlikow", action
		{
			if (check_lfs_progs())
				process_menu (MENU_selfskindlfs);
			else
				process_menu (MENU_selfskind);
		};
	option "Przesuniecie/rozmiar", action  
		{	int start, size;
			msg_display_add(MSG_defaultunit, multname);
			start = getpartoff(MSG_offset, 0);
			size = getpartsize(MSG_size, start, 0);
			if (size == -1)
				size = dlsize - start;
			bsdlabel[editpart].pi_offset = start;
			bsdlabel[editpart].pi_size = size;
		};
	option "Bsize/Fsize", action 
		{	char buf[40]; int i;

			if (!PI_ISBSDFS(&bsdlabel[editpart])) {
				msg_display (MSG_not42bsd, 'a'+editpart);
				process_menu (MENU_ok);
				return FALSE;
			}
			msg_prompt_add (MSG_bsize, NULL, buf, 40);
			i = atoi(buf);
			bsdlabel[editpart].pi_bsize = i;
			msg_prompt_add (MSG_fsize, NULL, buf, 40);
			i = atoi(buf);
			bsdlabel[editpart].pi_fsize = i;
		};
	option "Punkt montazu", action 
		{	if (PI_ISBSDFS(&bsdlabel[editpart]) ||
			    bsdlabel[editpart].pi_fstype == FS_MSDOS)
				msg_prompt_add (MSG_mountpoint, NULL,
					fsmount[editpart], 20);
			else {
				msg_display (MSG_nomount, 'a'+editpart);
				process_menu (MENU_ok);
			}
		};
	option "Ochrona", action 
		{	preservemount[editpart] = 1 - preservemount[editpart];
		};

menu selfskind, title "Wybierz rodzaj", y=15;
	option "4.2BSD", exit, action 
			{ bsdlabel[editpart].pi_fstype = FS_BSDFFS;
			  bsdlabel[editpart].pi_bsize  = 8192;
			  bsdlabel[editpart].pi_fsize  = 1024;
			};
	option "nie uzywana", exit, action 
			{ bsdlabel[editpart].pi_fstype = FS_UNUSED;
			  bsdlabel[editpart].pi_bsize  = 0;
			  bsdlabel[editpart].pi_fsize  = 0;
			};
	option "wymiany", exit, action 
			{ bsdlabel[editpart].pi_fstype = FS_SWAP;
			  bsdlabel[editpart].pi_bsize  = 0;
			  bsdlabel[editpart].pi_fsize  = 0;
			};
	option "msdos", exit, action 
			{ bsdlabel[editpart].pi_fstype = FS_MSDOS;
			  bsdlabel[editpart].pi_bsize  = 0;
			  bsdlabel[editpart].pi_fsize  = 0;
			};

menu selfskindlfs, title "Wybierz rodzaj", y=15;
	option "4.2BSD", exit, action 
			{ bsdlabel[editpart].pi_fstype = FS_BSDFFS;
			  bsdlabel[editpart].pi_bsize  = 8192;
			  bsdlabel[editpart].pi_fsize  = 1024;
			};
	option "nie uzywana", exit, action 
			{ bsdlabel[editpart].pi_fstype = FS_UNUSED;
			  bsdlabel[editpart].pi_bsize  = 0;
			  bsdlabel[editpart].pi_fsize  = 0;
			};
	option "wymiany", exit, action 
			{ bsdlabel[editpart].pi_fstype = FS_SWAP;
			  bsdlabel[editpart].pi_bsize  = 0;
			  bsdlabel[editpart].pi_fsize  = 0;
			};
	option "msdos", exit, action 
			{ bsdlabel[editpart].pi_fstype = FS_MSDOS;
			  bsdlabel[editpart].pi_bsize  = 0;
			  bsdlabel[editpart].pi_fsize  = 0;
			};
	option "4.4LFS", exit, action 
			{ bsdlabel[editpart].pi_fstype = FS_BSDLFS;
			  bsdlabel[editpart].pi_bsize  = 8192;
			  bsdlabel[editpart].pi_fsize  = 1024;
			};


menu distmedium, title "Wybierz medium";
	display action { msg_display (MSG_distmedium); nodist = 0; };
	option "ftp",  action	{
				  got_dist = get_via_ftp();
			        },
				exit;
	option "nfs",  action	{ 
				  got_dist = get_via_nfs();
			 	}, exit;
	option "cdrom", action  {
				  got_dist = get_via_cdrom();
				}, exit; 
	option "dyskietka", action {
			          got_dist = get_via_floppy(); 
				}, exit;
	option "niezamontowany SP", action {
				  got_dist = get_via_localfs(); 
				}, exit;
	option "lokalny katalog", action {
				   got_dist = get_via_localdir();
				 }, exit;
	option "zadne",  action { nodist = 1; }, exit;

menu distset, title "Wybierz swoja dystrybucje";
	display action { msg_display (MSG_distset); };
	option "Pelna instalacja", exit;
	option "Inna instalacja", next menu md_distcustom;

menu ftpsource, title "Zmien";
	display action
		{ msg_clear();
		  msg_table_add (MSG_ftpsource, ftp_host, ftp_dir, ftp_user,
		     strcmp(ftp_user, "ftp") == 0 ? ftp_pass :
		       strlen(ftp_pass) != 0 ? "** hidden **" : "", ftp_proxy);
		};
	option "Host", action
		{ msg_prompt (MSG_host, ftp_host, ftp_host, 255); };
	option "Katalog", action
		{ msg_prompt (MSG_dir, ftp_dir, ftp_dir, 255); };
	option "Uzytkownik", action
		{ msg_prompt (MSG_user, ftp_user, ftp_user, 255); };
	option "Haslo", action
		{ if (strcmp(ftp_user, "ftp") == 0)
			msg_prompt (MSG_email, ftp_pass, ftp_pass, 255);
		  else {
			msg_prompt_noecho (MSG_passwd, "", ftp_pass, 255);
		  }
		};
	option "Proxy", action
		{ msg_prompt (MSG_proxy, ftp_proxy, ftp_proxy, 255);
		  if (strcmp(ftp_proxy, "") == 0)
			unsetenv("ftp_proxy");
		  else
			setenv("ftp_proxy", ftp_proxy, 1);
		};
	option "Sciagnij Dystrybucje", exit;

menu nfssource, title "Zmien";
	display action
		{ msg_display (MSG_nfssource, nfs_host, nfs_dir); };
	option "Host", action
		{ msg_prompt (MSG_host, NULL, nfs_host, 255); };
	option "Katalog", action
		{ msg_prompt (MSG_dir, NULL, nfs_dir, 255); };
	option "Kontynuuj", exit;

menu nfsbadmount, title "Co chcesz zrobic?";
	option "Sprobowac jeszcze raz", exit, sub menu nfssource, action
		{ yesno = 1; ignorerror = 0; };
	option "Poddac sie", exit, action
		{ yesno = 0; ignorerror = 0; };
	option "Zignorowac, kontynuowac", exit, action
		{ yesno = 1; ignorerror = 1; };


menu fdremount, title "Co chcesz zrobic?";
	option "Sprobowac jeszcze raz", exit, action { yesno = 1; };
	option "Przerwac instalacje", exit, action { yesno = 0; };

menu fdok, title "Nacisnij enter aby kontynuowac";
	option "OK", exit, action { yesno = 1; };
	option "Przerwac instalacje", exit, action { yesno = 0; };

menu crypttype, title "Kodowanie hasel";
	option "DES", exit, action { yesno = 1; };
	option "MD5", exit, action { yesno = 2; };
	option "Blowfish 2^7 round", exit, action { yesno = 3; };
	option "nie zmieniaj", exit, action { yesno = 0; };

menu cdromsource, title "Zmien";
	display action
		{ msg_display (MSG_cdromsource, cdrom_dev, cdrom_dir); };
	option "Urzadzenie", action
		{ msg_prompt (MSG_dev, cdrom_dev, cdrom_dev, SSTRSIZE); };
	option "Katalog", action
		{ msg_prompt (MSG_dir, cdrom_dir, cdrom_dir, STRSIZE); };
	option "Kontynuuj", exit;

menu cdrombadmount, title "Co chcesz zrobic?";
	option "Sprobowac jeszcze raz", exit, sub menu cdromsource, action
		{ yesno = 1; ignorerror = 0; };
	option "Poddac sie", exit, action
		{ yesno = 0; ignorerror = 0; };
	option "Zignorowac, kontynuowac", exit, action
		{ yesno = 1; ignorerror = 1; };


menu localfssource, title "Zmien";
	display action
		{ msg_display (MSG_localfssource, localfs_dev, localfs_fs, localfs_dir); };
	option "Urzadzenie", action
		{ msg_prompt (MSG_dev, localfs_dev, localfs_dev, SSTRSIZE); };
	option "SystemPlikow", action
		{ msg_prompt (MSG_filesys, localfs_fs, localfs_fs, STRSIZE); };
	option "Katalog", action
		{ msg_prompt (MSG_dir, localfs_dir, localfs_dir, STRSIZE); };
	option "Kontynuuj", exit;

menu localfsbadmount, title "Co chcesz zrobic?";
	option "Sprobowac jeszcze raz", exit, sub menu localfssource, action
		{ yesno = 1; ignorerror = 0; };
	option "Poddac sie", exit, action
		{ yesno = 0; ignorerror = 0; };
	option "Zignorowac, kontynuowac", exit, action
		{ yesno = 1; ignorerror = 1; };

menu localdirsource, title "Zmien";
	display action
		{ msg_display(MSG_localdir, localfs_dir); };
	option "Katalog", action
		{ msg_prompt (MSG_dir, localfs_dir, localfs_dir, STRSIZE); },
		exit;
	option "Kontynuuj", exit;

menu localdirbad, title "Co chcesz zrobic?";
	option "Zmien sciezke katalogu",  action
		{ yesno = 1;
	          msg_prompt(MSG_localdir, localfs_dir, localfs_dir, STRSIZE);
		}, exit;
	option "Poddac sie", exit, action
		{ yesno = 0; ignorerror = 0; };
	option "Zignorowac, kontynuowac", exit, action
		{ yesno = 1; ignorerror = 1; };

menu namesrv6, title "  Wybierz serwer nazw IPv6";
	option "paradise.v6.kame.net", exit, action
		{
#ifdef INET6
		  strncpy(net_namesvr6, "3ffe:501:4819::42",
		      sizeof(net_namesvr6));
		  yesno = 1;
#else
		  yesno = 0;
#endif
		}; 
	option "kiwi.itojun.org", exit, action
		{
#ifdef INET6
		  strncpy(net_namesvr6, "3ffe:501:410:100:5254:ff:feda:48bf",
		      sizeof(net_namesvr6));
		  yesno = 1;
#else
		  yesno = 0;
#endif
		}; 
	option "sh1.iijlab.net", exit, action
		{
#ifdef INET6
		  strncpy(net_namesvr6, "3ffe:507:0:1:260:97ff:fe07:69ea",
		      sizeof(net_namesvr6));
		  yesno = 1;
#else
		  yesno = 0;
#endif
		}; 
	option "ns1.v6.intec.co.jp", exit, action 
		{
#ifdef INET6
		  strncpy(net_namesvr6, "3ffe:508:0:1::53",
		      sizeof(net_namesvr6));
		  yesno = 1;
#else
		  yesno = 0;
#endif
		}; 
	option "nttv6.net", exit, action
		{
#ifdef INET6
		  strncpy(net_namesvr6, "3ffe:1800:1000::1",
		      sizeof(net_namesvr6));
		  yesno = 1;
#else
		  yesno = 0;
#endif
		}; 
	option "light.imasy.or.jp", exit, action
		{
#ifdef INET6
		  strncpy(net_namesvr6, "3ffe:505:0:1:2a0:c9ff:fe61:6521",
		      sizeof(net_namesvr6));
		  yesno = 1;
#else
		  yesno = 0;
#endif
		}; 
	option "inny  ", exit, action
		{ yesno = 0; };

menu ip6autoconf, title "Wykonac autokonfiguracje IPv6?";
	option "Tak", exit, action  {yesno = 1;};
	option "Nie",  exit, action  {yesno = 0;};

menu dhcpautoconf, title "Wykonac autkonfiguracje DHCP?";
	option "Tak", exit, action  {yesno = 1;};
	option "Nie",  exit, action  {yesno = 0;};

