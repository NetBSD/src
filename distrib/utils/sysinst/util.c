/*	$NetBSD: util.c,v 1.7 1997/10/29 01:07:01 phil Exp $	*/

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

/* util.c -- routines that don't really fit anywhere else... */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <curses.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"


void get_ramsize(void)
{
	long len=sizeof(long);
	int mib[2] = {CTL_HW, HW_PHYSMEM};
	
	sysctl(mib, 2, (void *)&ramsize, (size_t *)&len, NULL, 0);

	/* Find out how many Megs ... round up. */
	rammb = (ramsize + MEG - 1) / MEG;
}


static int asked = 0;

void ask_sizemult ()
{
	if (!asked) {
		msg_display (MSG_sizechoice, dlcylsize);
		process_menu (MENU_sizechoice);
	}
	asked = 1;
}

/* Returns 1 for "y" or "Y" and "n" otherwise.  CR => default. */
int
ask_ynquestion (char *quest, char def, ...)
{
	char line[STRSIZE];
	va_list ap;
	char c;

	va_start(ap, def);
	vsnprintf (line, STRSIZE, quest, ap);
	va_end(ap);

	printf ("%s [%c]: ", line, def);
	c = getchar();
	if (c == '\n')
		return def == 'y';

	while (getchar() != '\n') /* eat characters */;

	return c == 'y' || c == 'Y';
}

void
extract_dist (void)
{
	int verbose;
	int numchar;
	char *files;
	char *p;

	msg_display (MSG_verboseextract);
	process_menu (MENU_noyes);
	verbose = yesno;

	numchar = collect (T_OUTPUT, &files, "/bin/ls %s/*.tar.gz", dist_dir);
	if (numchar < 0) {
		endwin();
		(void)fprintf (stderr, msg_string(MSG_badls));
		exit(1);
	}
	files[numchar] = '\0';

#ifndef DEBUG
	if (chdir("/mnt")) {
		endwin();
		(void)fprintf(stderr, msg_string(MSG_realdir), "/mnt");
		exit(1);
	}
#else
	printf ("chdir (%s)\n", "/mnt");
#endif

	endwin();
	p = strtok (files, " \n");
	while (p != NULL) {
		(void)printf (msg_string(MSG_extracting), p);
		run_prog ("/usr/bin/tar --unlink -xpz%s -f %s",
			  verbose ? "v":"", p);
		p= strtok (NULL, " \n");
	}
	(void)printf(msg_string(MSG_endtar));
	getchar();
	puts(CL);
	wrefresh(stdscr);
}


void run_makedev (void)
{
	msg_display (MSG_makedev);
	sleep (1);
#ifndef DEBUG
	if (chdir("/mnt/dev")) {
		endwin();
		(void)fprintf(stderr, msg_string(MSG_realdir), "/mnt");
		exit(1);
	}
#else
	printf ("chdir (%s)\n", "/mnt/dev");
#endif
	run_prog ("/bin/sh MAKEDEV all");
}


/* Load files from floppy. */
int get_via_floppy (void)
{
	char distname[STRSIZE];
	char fddev[STRSIZE] = "/dev/fd0a";
	char fname[STRSIZE];
	char fullname[STRSIZE];
	distinfo *list;
	char post[4];
	int  mounted = 0;
	struct stat sb;


	cd_dist_dir ("unloading from floppy");

	msg_prompt_add (MSG_fddev, fddev, fddev, STRSIZE);

	list = dist_list;
	while (list->name) {
		strcpy (post, ".aa");
		snprintf (distname, STRSIZE, list->name, rels, dist_postfix);
		while (list->getit && strcmp(post,list->fdlast) <= 0) {
			snprintf (fname, STRSIZE, list->name, rels, post);
			snprintf (fullname, STRSIZE, "/mnt2/%s", fname);
			while (!mounted || stat(fullname, &sb)) {
 				if (mounted) {
					run_prog ("/sbin/umount /mnt2 "
						  "2>/dev/null");
					msg_display (MSG_fdnotfound, fname);
				} else 					
					msg_display (MSG_fdmount, fname);
				process_menu (MENU_ok);
				while (run_prog("/sbin/mount -t %s %s /mnt2",
						 fdtype, fddev)) {
					msg_display (MSG_fdremount, fname);
					process_menu (MENU_fdremount);
					if (!yesno)
						return 0;
				}
				mounted = 1;
			}
			run_prog ("/bin/cat /mnt2/%s >> %s", fullname,
				  distname);
			if (post[1] < 'z')
				post[1]++;
			else
				post[1]='a', post[2]++;
		}
		run_prog ("/sbin/umount /mnt2 2>/dev/null");
		mounted = 0;
		list++;
	}
#ifndef DEBUG
	chdir("/");
#endif
	return 1;
}

int
get_via_cdrom(void)
{
	/* Get server and filepath */
	process_menu (MENU_cdromsource);

	/* Mount it */
	while (!run_prog ("/sbin/mount -rt cd9660 %s /mnt2", cdrom_dev)) {
		process_menu (MENU_cdrombadmount);
		if (!yesno)
			return 0;
		/* Verify distribution files exist.  XXX */
	}

	/* return location, don't clean... */
	strcpy (dist_dir, "/mnt2");
	strncat (dist_dir, cdrom_dir, STRSIZE-strlen(dist_dir)-1);
	clean_dist_dir = 0;
	mnt2_mounted = 1;
	return 1;
}


void cd_dist_dir (char *forwhat)
{
	char realdir[STRSIZE];

	msg_prompt (MSG_distdir, dist_dir, dist_dir, STRSIZE, forwhat);
	if (*dist_dir == '/')
		snprintf (realdir, STRSIZE, "/mnt%s", dist_dir);
	else
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
}

/* Support for custom distribution fetches / unpacks. */

void toggle_getit (int num)
{
	dist_list[num].getit ^= 1;
}

void show_cur_distsets (void)
{
	distinfo *list;

	msg_display (MSG_cur_distsets);
	list = dist_list;
	while (list->name) {
		msg_printf_add ("%s%s\n", list->desc, list->getit ?
				msg_string(MSG_yes) : msg_string(MSG_no));
		list++;
	}
}
