/*	$NetBSD: util.c,v 1.5 1997/10/17 21:10:48 phil Exp $	*/

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
	char realdir[STRSIZE];
	char distname[STRSIZE];
	char fddev[STRSIZE] = "/dev/fd0a";
	char dirname[STRSIZE];
	char fname[STRSIZE];
	char fullname[STRSIZE];
	char **list;
	char **last;
	char post[4];
	int  mounted = 0;
	struct stat sb;

	msg_prompt (MSG_distdir, dist_dir, dist_dir, STRSIZE,
		    "unloading from floppy");
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

	msg_prompt_add (MSG_fddev, fddev, fddev, STRSIZE);

	list = dist_list;
	last = fd_last;
	while (*last) {
		strcpy (post, ".aa");
		snprintf (dirname, STRSIZE, *list, rels, "/" );
		snprintf (distname, STRSIZE, *list, rels, dist_postfix);
		msg_display (MSG_fdload, dirname);
		process_menu (MENU_yesno);
		while (yesno && strcmp(post,*last) <= 0) {
			snprintf (fname, STRSIZE, *list, rels, post);
			snprintf (fullname, STRSIZE, "/mnt2/%s%s", dirname,
				  fname);
			while (!mounted || stat(fullname, &sb)) {
				if (mounted)
					run_prog ("/sbin/umount /mnt2");
				msg_display (MSG_fdmount, dirname, fname);
				process_menu (MENU_ok);
				while (!run_prog("/sbin/mount -t %s %s /mnt2",
						 fdtype, fddev)) {
					msg_display (MSG_fdremount, dirname,
						     fname);
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
		run_prog ("/sbin/umount /mnt2");
		mounted = 0;
		list++;
		last++;
	}
#ifndef DEBUG
	chdir("/");
#endif
	return 1;
}
