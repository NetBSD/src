/*	$NetBSD: net.c,v 1.8.2.7 1997/11/22 00:33:04 simonb Exp $	*/

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

static int network_up = 0;

/* Get the list of network interfaces. */

static void get_ifconfig_info (void)
{
	char *textbuf;
	int   textsize;
	char *t;

	/* Get ifconfig information */
	
	textsize = collect (T_OUTPUT, &textbuf,
			    "/sbin/ifconfig -l 2>/dev/null");
	if (textsize < 0) {
		endwin();
		(void) fprintf (stderr, "Could not run ifconfig.");
		exit (1);
	}
	(void) strtok(textbuf,"\n");
	strncpy (net_devices, textbuf, textsize<STRSIZE ? textsize : STRSIZE);
	net_devices[STRSIZE] = 0;
	free (textbuf);

	/* Remove lo0 and anything after ... */
	t = strstr (net_devices, "lo0");
	if (t != NULL)
		*t = 0;
}

/* Get the information to configure the network, configure it and
   make sure both the gateway and the name server are up. */

int config_network (void)
{	char *tp;
	char defname[255];
	int  octet0;
	int  pass;

	FILE *f;
	time_t now;

	if (network_up)
		return 1;

	net_devices[0] = '\0';
	get_ifconfig_info ();
	if (strlen(net_devices) == 0) {
		/* No network interfaces found! */
		msg_display (MSG_nonet);
		process_menu (MENU_ok);
		return -1;
	}
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
		msg_prompt_add (MSG_net_defroute, net_defroute, net_defroute,
				STRSIZE);
		msg_prompt_add (MSG_net_namesrv, net_namesvr, net_namesvr,
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
	time(&now);
	/* NB: ctime() returns a string ending in  '\n' */
	(void)fprintf (f, ";\n; BIND data file\n; %s %s;\n", 
		       "Created by NetBSD sysinst on", ctime(&now)); 
	(void)fprintf (f, "nameserver %s\nlookup file bind\nsearch %s\n",
		       net_namesvr, net_domain);
	fclose (f);

	run_prog ("/sbin/ifconfig lo0 127.0.0.1");
	run_prog ("/sbin/ifconfig %s inet %s netmask %s", net_dev, net_ip,
		  net_mask);
	run_prog ("/sbin/route -f > /dev/null 2> /dev/null");
	run_prog ("/sbin/route add default %s > /dev/null 2> /dev/null",
		  net_defroute);

	network_up = !run_prog ("/sbin/ping -c 2 %s > /dev/null", net_defroute)
		&& !run_prog ("/sbin/ping -c 2 %s > /dev/null", net_namesvr);

	return network_up;
}

int
get_via_ftp (void)
{ 
	distinfo *list;
	char filename[SSTRSIZE];
	int  ret;

	while (!config_network ()) {
		msg_display (MSG_netnotup);
		process_menu (MENU_yesno);
		if (!yesno)
			return 0;
	}

	cd_dist_dir ("ftp");

	/* Fill in final values for ftp_dir. */
	strncat (ftp_dir, rels, STRSIZE-strlen(ftp_dir));
	strcat  (ftp_dir, "/");
	strncat (ftp_dir, machine, STRSIZE-strlen(ftp_dir));
	strncat (ftp_dir, ftp_prefix, STRSIZE-strlen(ftp_dir));
	process_menu (MENU_ftpsource);
	
	list = dist_list;
	endwin();
	while (list->name) {
		if (!list->getit) {
			list++;
			continue;
		}
		snprintf (filename, SSTRSIZE, list->name, rels, dist_postfix);
		if (strcmp ("ftp", ftp_user) == 0)
			ret = run_prog("/usr/bin/ftp -a ftp://%s/%s/%s",
				       ftp_host, ftp_dir,
				       filename);
		else
			ret = run_prog("/usr/bin/ftp ftp://%s:%s@%s/%s/%s",
				       ftp_user, ftp_pass, ftp_host, ftp_dir,
				       filename);
		if (ret) {
			/* Error getting the file.  Bad host name ... ? */
			puts (CL);
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
	chdir("/");	/* back to current real root */
#endif
	return 1;
}

int
get_via_nfs(void)
{
        while (!config_network ()) {
                msg_display (MSG_netnotup);
                process_menu (MENU_yesno);
                if (!yesno)
                        return 0;
        }

	/* Get server and filepath */
	process_menu (MENU_nfssource);

	/* Mount it */
	while(run_prog("/sbin/mount -t nfs %s:%s /mnt2", nfs_host, nfs_dir)) {
		process_menu (MENU_nfsbadmount);
		if (!yesno)
			return 0;
		/* verify distribution files are there. XXX */
	}

	/* return location, don't clean... */
	strcpy (ext_dir, "/mnt2");
	clean_dist_dir = 0;
	mnt2_mounted = 1;
	return 1;
}

/*
 * Write the network config info the user entered via menus into the
 * config files in the target disk.  Be careful not to lose any
 * information we don't immediately add back, in case the install
 * target is the currently-active root. 
 */
void
mnt_net_config(void)
{
	char ans [5] = "y";
	char ifconfig_fn [STRSIZE];
	FILE *f;

	if (network_up) {
		msg_prompt (MSG_mntnetconfig, ans, ans, 5);
		if (*ans == 'y') {

			/* If not running in target, copy resolv.conf there. */
			dup_file_into_target("/etc/resolv.conf");
			/* 
			 * Add IPaddr/hostname to  /etc/hosts.
			 * Be careful not to clobber any existing contents.
			 * Relies on ordered seach of /etc/hosts. XXX YP?
			 */
			f = target_fopen("/etc/hosts", "a");
			if (f != 0) {
				fprintf(f, msg_string(MSG_etc_hosts),
					net_ip, net_host, net_domain,
					net_host);
				fclose(f);
			}

			/* Write IPaddr and netmask to /etc/ifconfig.if[0-9] */
			snprintf (ifconfig_fn, STRSIZE,
				  "/etc/ifconfig.%s", net_dev);
			f = target_fopen(ifconfig_fn, "w");
			if (f != 0) {
				fprintf(f, "%s netmask %s\n",
					net_ip, net_mask);
				fclose(f);
			}

			f = target_fopen("/etc/mygate", "w");
			if (f != 0) {
				fprintf(f, "%s\n", net_defroute);
				fclose(f);
			}
		}
	}
}
