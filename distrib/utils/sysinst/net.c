/*	$NetBSD: net.c,v 1.2 1997/09/27 00:09:28 phil Exp $	*/

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
 *      This product includes software develooped for the NetBSD Project by
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

/* net.c -- routines to fetch files off the network. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <unistd.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "txtwalk.h"

static void found_net (struct data* list, int num);

struct lookfor netbuf[] = {
	{"ed",  "%s:", "c", NULL, found_net},
	{"el",  "%s:", "c", NULL, found_net},
	{"ep",  "%s:", "c", NULL, found_net},
	{"ie",  "%s:", "c", NULL, found_net},
	{"lc",  "%s:", "c", NULL, found_net},
	{"fea", "%s:", "c", NULL, found_net},
	{"le",  "%s:", "c", NULL, found_net},
	{"de",  "%s:", "c", NULL, found_net},
	{"fpa", "%s:", "c", NULL, found_net},
	{"fxp", "%s:", "c", NULL, found_net}
};

int numnetbuf = sizeof(netbuf) / sizeof(struct lookfor);


static void found_net (struct data* list, int num)
{
	int i;
	strncat (net_devices, list[0].u.s_val, 253-strlen(net_devices));
	i = strlen(net_devices);
	net_devices[i] = ' ';
	net_devices[i+1] = '\0';
}

static void get_ifconfig_info (void)
{
	char *textbuf;
	int   textsize;

	/* Get ifconfig information */
	
	textsize = collect (T_OUTPUT, &textbuf,
			    "/sbin/ifconfig -a 2>/dev/null");
	if (textsize < 0) {
		endwin();
		(void) fprintf (stderr, "Could not run ifconfig.");
		exit (1);
	}
	walk (textbuf, textsize, netbuf, numnetbuf);
	free (textbuf);
}


static void config_network (void)
{	char *tp;
	char defname[255];
	int  octet0;
	int  pass;

	FILE *f;

	net_devices[0] = '\0';
	get_ifconfig_info ();
	strncpy (defname, net_devices, 255);
	tp = defname;
	strsep(&tp, " ");
	msg_prompt (MSG_asknetdev, defname, net_dev, 255, net_devices);
	tp = net_dev;
	strsep(&tp, " ");
	net_dev[strlen(net_dev)+1] = 0;
	net_dev[strlen(net_dev)] = ' ';
	while ((strlen(net_dev) != 4 && strlen(net_dev) != 5) ||
		       strstr(net_devices, net_dev) == NULL) {
		msg_prompt (MSG_badnet, defname,  net_dev, 10,
			    net_devices);
		tp = net_dev;
		strsep(&tp, " ");
		net_dev[strlen(net_dev)+1] = 0;
		net_dev[strlen(net_dev)] = ' ';
	}

	/* Remove that space we added. */
	net_dev[strlen(net_dev)-1] = 0;
	
	/* Get other net information */
	msg_display (MSG_netinfo);
	pass = 0;
	do {
		msg_prompt_add (MSG_net_domain, net_domain, net_domain,
				STRSIZE);
		msg_prompt_add (MSG_net_host, net_host, net_host, STRSIZE);
		msg_prompt_add (MSG_net_ip, net_ip, net_ip, STRSIZE);
		octet0 = atoi(net_ip);
		if (!pass) {
			if (0 <= octet0 && octet0 <= 127)
				strcpy (net_mask, "0xff000000");
			else if (127 <= octet0 && octet0 <= 191)
				strcpy (net_mask, "0xffff0000");
			else if (192 <= octet0 && octet0 <= 223)
				strcpy (net_mask, "0xffff0000");
		}
		msg_prompt_add (MSG_net_mask, net_mask, net_mask, STRSIZE);
		msg_prompt_add (MSG_net_namesrv, net_namesvr, net_namesvr,
				STRSIZE);
		msg_prompt_add (MSG_net_defroute, net_defroute, net_defroute,
				STRSIZE);

		msg_display (MSG_netok, net_domain, net_host, net_ip,
			     net_mask, net_namesvr, net_defroute);
		process_menu (MENU_yesno);
		if (!yesno)
			msg_display (MSG_netagain);
		pass++;
	} while (!yesno);

	/* Create /etc/resolv.conf */
#ifdef DEBUG
	f = fopen ("/tmp/resolv.conf", "w");
#else
	f = fopen ("/etc/resolv.conf", "w");
#endif
	if (f == NULL) {
		endwin();
		(void)fprintf(stderr, "%s", msg_string(MSG_resolv));
		exit(1);
	}
	(void)fprintf (f, "nameserver %s\nlookup file bind\nsearch %s",
		       net_namesvr, net_domain);
	fclose (f);

	run_prog ("/sbin/ifconfig lo0 127.0.0.1");
	run_prog ("/sbin/ifconfig %s inet %s netmask %s", net_dev, net_ip,
		  net_mask);
	run_prog ("/sbin/route add default %s > /dev/null", net_defroute);
}

int
get_via_ftp (void)
{ 
	char **list;
	char realdir[STRSIZE];
	int  ret;

	config_network ();
	strncat (ftp_dir, ftp_prefix, STRSIZE-strlen(ftp_dir));
	process_menu (MENU_ftpsource);
	msg_prompt (MSG_netdir, dist_dir, dist_dir, STRSIZE, "ftp");
	snprintf (realdir, STRSIZE, "/mnt/%s", dist_dir);
	strcpy (dist_dir, realdir);
	run_prog ("/bin/mkdir %s", realdir);
	clean_dist_dir = 1;
#ifndef DEBUG
	if (chdir(realdir)) {
		endwin();
		(void)fprintf(stderr, msg_string(MSG_realdir), realdir);
		exit(1);
	}
#else
	printf ("chdir (%s)\n", realdir);
#endif
	
	list = ftp_list;
	endwin();
	while (*list) {
		if (strcmp ("ftp", ftp_user) == 0)
			ret = run_prog("/usr/bin/ftp -a ftp://%s/%s/%s%s%s",
				       ftp_host, ftp_dir,
				       *list, rels, ftp_postfix);
		else
			ret = run_prog("/usr/bin/ftp ftp://%s:%s@%s/%s/%s%s%s",
				       ftp_user, ftp_pass, ftp_host, ftp_dir,
				       *list, rels, ftp_postfix);
		if (ret) {
			/* Error getting the file.  Bad host name ... ? */
			wrefresh (stdscr);
			msg_display (MSG_ftperror);
			process_menu (MENU_yesno);
			if (yesno)
				process_menu (MENU_ftpsource);
			else
				return 0;
			endwin();
		} else 
			list++;
	}
	puts (CL); /* Just to make sure. */
	wrefresh (stdscr);
#ifndef DEBUG
	chdir("/");
#endif
	return 1;
}

void
get_via_nfs(void)
{
	config_network ();
	/* Get server and filepath */
	/* Mount it */
}
