/* $NetBSD: getcwd.c,v 1.3.2.1 1999/11/21 15:19:06 he Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bill Sommerfeld.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * test SYS___getcwd.
 */

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/param.h>		/* for MAXPATHLEN */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "getcwd.h"

int	main __P((int, char *[]));

static void check1 __P((char *dir, char *buf, char *calltext,
    int actual, int expected, int experr));

static void time_old_getcwd __P((void));
static void time_kern_getcwd __P((void));
static void time_func __P((char *name,
    void (*func)(void)));

static void test_speed __P((void));
static void test___getcwd  __P((void));
static void test___getcwd_perms  __P((void));
static void test___getcwd_chroot __P((void));

static void stress_test_getcwd __P((void));
static void usage __P((char *progname));

/* libc-private interface */
int __getcwd __P((char *, size_t));

/*
 * test cases:
 * 	NULL pointer
 *	broken pointer
 * 	zero-length buffer
 *	negative length
 *	one-character buffer
 * 	two-character buffer
 *	full-length buffer
 *	large (uncacheable) name in path.
 *	deleted directory
 *	after rename of parent.
 *	permission failure.
 *	good pointer near end of address space
 *	really huge length
 *	really large (multi-block) directories
 *	chroot interactions:
 *		chroot, at / inside the directory.
 *		chroot, at some other inside directory.
 */

/*
 * test cases not yet done:
 *		-o union mount
 *		chroot interactions:
 *			chroot to mounted directory.
 *			(i.e., proc a: chroot /foo; sleep;
 *		       		proc b: mount blort /foo)
 *		concurrent with force-unmounting of filesystem.
 */
 
#define bigname "Funkelhausersteinweitz.SIPBADMIN.a" 	 /* don't ask */
#define littlename "getcwdtest"
#define othername "testgetcwd"

static int verbose = 0;
static int test = 1;
static int fail = 0;
static int pass = 0;
static int sleepflag = 0;

static uid_t altid = -1;

static void
check1 (dir, buf, calltext, actual, expected, experr)
	char *dir;
	char *buf;
	char *calltext;
	int actual, expected, experr;
{
	int ntest = test++;			
	if (actual != expected) {
		fprintf(stderr,
		    "test %d: in %s, %s failed; expected %d, got %d\n",
		    ntest, dir, calltext, expected, actual);
		if (actual < 0) perror("getcwd");
		fail++;
	} else if ((expected == -1) && (errno != (experr))) {
		fprintf(stderr,
		    "test %d: in %s, %s failed; expected error %d, got %d\n", 
		    ntest, dir, calltext, experr, errno);
		if (actual < 0) perror("getcwd"); 
		fail++;
	} else if ((expected > 0) &&
	    (buf != NULL) &&
	    (strcmp (dir, buf) != 0)) {
		fprintf(stderr,
		    "test %d: in %s, %s got wrong dir %s\n", 
		    ntest, dir, calltext, buf);
		fail++;
	} else {
		if (expected > 0) {
			char newbuf[1024];
			char *cp = old_getcwd(newbuf, sizeof(newbuf));
			if (cp == NULL) {
				fail++;
				fprintf(stderr,
				    "test %d: in %s, old getcwd failed!\n",
				    ntest, dir);
			} else if (strcmp(cp, buf)) {
				fail++;
				fprintf(stderr,
				    "test %d: in %s, old_getcwd returned different dir %s\n",
				    ntest, dir, cp);
			}
		}
		pass++;
		if (verbose)
			printf("test %d: in %s, %s passed\n", ntest, dir, calltext);
	}
	if (sleepflag)
		sleep(1);
}

int nloops = 100;

void
time_old_getcwd()
{
	char result_buf[1024];
	if (old_getcwd(result_buf, 1024) == NULL) {
		fprintf(stderr, "old_getcwd failed during timing test!\n");
		perror("old_getcwd");
		exit(1);
	}
	
}

void
time_kern_getcwd()
{
	char result_buf[1024];
	if (__getcwd(result_buf, sizeof(result_buf)) < 0) {
		fprintf(stderr, "getcwd failed during timing test!");
		perror("getcwd");
		exit(1);
	}
}

static void
time_func(name, func)
	char *name;
	void (*func) __P((void));
{
	struct timeval before, after;
	double delta_t;
	
	int i;
	chdir ("/usr/share/examples/emul/ultrix/etc");
	
	gettimeofday(&before, 0);
	for (i=0; i<nloops; i++) {
		(*func)();
	}
	gettimeofday(&after, 0);

	delta_t = after.tv_sec - before.tv_sec;

	delta_t += ((double)(after.tv_usec - before.tv_usec))/1000000.0;

	printf("%s: %d calls in %10.3f seconds; ", name, nloops, delta_t);
	printf("%10.6f ms/call\n", (delta_t*1000.0)/nloops);	
}

void
test_speed()
{
	int i;
	for (i=0; i<5; i++)
		time_func("kernel getcwd", time_kern_getcwd);

	for (i=0; i<5; i++)
		time_func("old user-space getcwd", time_old_getcwd);
}

#define CHECK(dir, call, ret, err) \
	check1((dir), kbuf, #call, (call), (ret), (err))


void
test___getcwd_perms()
{
	char kbuf[1024];
	
	if (geteuid() != 0) 
	  {
	    fprintf(stderr, "Not root; skipping permission tests\n");
	    return;	    
	  }
	    
	mkdir ("/tmp/permdir", 0700);
	mkdir ("/tmp/permdir/subdir", 0755);
	chdir ("/tmp/permdir/subdir");
	
	seteuid(altid);

	CHECK("/tmp/permdir/subdir", __getcwd(kbuf, sizeof(kbuf)), -1, EACCES);

	seteuid(0);
	chdir ("/");
	rmdir ("/tmp/permdir/subdir");
	rmdir ("/tmp/permdir");

	mkdir ("/tmp/permdir", 0755);
	mkdir ("/tmp/permdir/subdir", 0711);
	chdir ("/tmp/permdir/subdir");
	
	seteuid(altid);

	CHECK("/tmp/permdir/subdir", __getcwd(kbuf, sizeof(kbuf)), 20, 0);

	seteuid(0);
	chdir ("/");
	rmdir ("/tmp/permdir/subdir");
	rmdir ("/tmp/permdir");
}

void
test___getcwd_chroot()
{
	int pid, status;
	char kbuf[1024];
	
	if (geteuid() != 0) 
	  {
	    fprintf(stderr, "Not root; skipping chroot tests\n");
	    return;
	  }

	/* XXX we need fchroot to do this properly.. */
	mkdir ("/tmp/chrootdir", 0755);
	mkdir ("/tmp/chrootdir/subdir", 0755);

	chdir ("/tmp/chrootdir");

	CHECK ("/tmp/chrootdir", __getcwd(kbuf, sizeof(kbuf)), 15, 0);

	fflush(NULL);
	
	pid = fork();

	if (pid < 0) {
		perror("fork");
		fail++;
	} else if (pid == 0) {
		fail = 0;
		pass = 0;
		/* chroot to root of filesystem (assuming MFS /tmp) */
		chroot ("/tmp");
		CHECK ("/chrootdir", __getcwd(kbuf, sizeof(kbuf)), 11, 0);
		/* chroot to further down */
		chroot ("/chrootdir");
		CHECK ("/", __getcwd(kbuf, sizeof(kbuf)), 2, 0);
		chdir("subdir");
		CHECK ("/subdir", __getcwd(kbuf, sizeof(kbuf)), 8, 0);

		if (fail)
			exit(1);
		else
			exit(0);
	} else {
		waitpid(pid, &status, 0);

		if (WIFEXITED(status) &&
		    (WEXITSTATUS(status) == 0))
			pass++;
		else
			fail++;
		
	}

	chdir ("/");
	rmdir ("/tmp/chrootdir/subdir");
	rmdir ("/tmp/chrootdir");
}




void
test___getcwd()
{
	int i;
	static char kbuf[1024];
	
	chdir("/");

	CHECK("/", __getcwd(0, 0), -1, ERANGE);
	CHECK("/", __getcwd(0, -1), -1, ERANGE);
	CHECK("/", __getcwd(kbuf, 0xdeadbeef), -1, ERANGE); /* large negative */
	CHECK("/", __getcwd(kbuf, 0x7000beef), 2, 0); /* large positive, rounds down */
	CHECK("/", __getcwd(kbuf, 0x10000), 2, 0); /* slightly less large positive, rounds down */
	CHECK("/", __getcwd(kbuf+0x100000, sizeof(kbuf)), -1, EFAULT); /* outside address space */	
	CHECK("/", __getcwd(0, 30), -1, EFAULT);
	CHECK("/", __getcwd((void*)0xdeadbeef, 30), -1, EFAULT);
	CHECK("/", __getcwd(kbuf, 2), 2, 0);
	assert (strcmp(kbuf, "/") == 0);
	CHECK("/", __getcwd(kbuf, sizeof(kbuf)), 2, 0);

	CHECK("/", __getcwd(kbuf, 0), -1, ERANGE);
	CHECK("/", __getcwd(kbuf, 1), -1, ERANGE);

	chdir("/sbin");
	CHECK("/sbin", __getcwd(kbuf, sizeof(kbuf)), 6, 0);
	/* verify that cacheable path gets range check right.. */
	CHECK("/sbin", __getcwd(kbuf, 3), -1, ERANGE);	
	chdir("/etc/mtree");
	CHECK("/etc/mtree", __getcwd(kbuf, sizeof(kbuf)), 11, 0);
	CHECK("/etc/mtree", __getcwd(kbuf, sizeof(kbuf)), 11, 0);	
	/* mount point */
	chdir("/usr/bin");
	CHECK("/usr/bin", __getcwd(kbuf, sizeof(kbuf)), 9, 0);

	/* really large (non-cacheable) entry name */
	chdir("/tmp");
	(void) rmdir(bigname);
	mkdir(bigname, 0755);
	chdir(bigname);

	/* verify that non-cachable path gets range check right.. */
	CHECK("/tmp/" bigname, __getcwd(kbuf, 10), -1, ERANGE);
	CHECK("/tmp/" bigname, __getcwd(kbuf, sizeof(kbuf)), 40, 0);
	
	if (rmdir("/tmp/" bigname) < 0) {
		perror("rmdir");
	}
	CHECK("deleted directory", __getcwd(kbuf, sizeof(kbuf)), -1, ENOENT);

	chdir("/tmp");
	(void) rmdir(littlename);
	mkdir(littlename, 0755);
	chdir(littlename);
	CHECK("/tmp/" littlename, __getcwd(kbuf, sizeof(kbuf)), 16, 0);
	if (rename("/tmp/" littlename, "/tmp/" othername) < 0) {
		perror("rename");
		fail++;
	}
	CHECK("/tmp/" othername, __getcwd(kbuf, sizeof(kbuf)), 16, 0);	
	if (rmdir("/tmp/" othername) < 0) {
		perror("rmdir");
		fail++;
	}
	CHECK("deleted directory", __getcwd(kbuf, sizeof(kbuf)), -1, ENOENT);

	mkdir("/tmp/bigdir", 0755);
	for (i=0; i<nloops; i++) {
		char buf[MAXPATHLEN];
		snprintf(buf, MAXPATHLEN, "/tmp/bigdir/bigsubdirwithanamewhichistoolongtocache%04d", i);
		(void)rmdir(buf);
		if (mkdir (buf, 0755) < 0) {
			perror("mkdir");
			fail++;
			break;
		}
	}
	for (i=0; i<nloops; i++) {
		char buf[MAXPATHLEN];
		snprintf(buf, MAXPATHLEN, "/tmp/bigdir/bigsubdirwithanamewhichistoolongtocache%04d", i);
		if (chdir(buf) < 0) {
			perror("chdir");
			fail++;
			break;
		}
		CHECK(buf, __getcwd(kbuf, sizeof(kbuf)), strlen(buf)+1, 0);	
	}
	for (i=0; i<nloops; i++) {
		char buf[MAXPATHLEN];
		snprintf(buf, MAXPATHLEN, "/tmp/bigdir/bigsubdirwithanamewhichistoolongtocache%04d", i);
		(void)rmdir(buf);
	}
	(void)rmdir("/tmp/bigdir");

	test___getcwd_perms();
	test___getcwd_chroot();
}


void
stress_test_getcwd()
{
	char buf[MAXPATHLEN];
	char ubuf[MAXPATHLEN];
	char kbuf[MAXPATHLEN];	
	printf("reading directories from stdin..\n");
	while (fgets(buf, MAXPATHLEN, stdin)) {
		char *cp = strrchr(buf, '\n');
		if (cp) *cp = '\0';

		if (chdir (buf) < 0) {
			warn("Can't change directory to %s", buf);
			continue;
		}
		

		cp = old_getcwd (ubuf, MAXPATHLEN);
		if (strcmp(buf, ubuf) != 0) {
			warnx("In %s, old_getcwd says %s\n",
			    buf, ubuf);
		}


		CHECK(buf, __getcwd (kbuf, MAXPATHLEN),
		    strlen(ubuf)+1, 0);
	}
}


/*
 *	- large directories.
 *
 *	- every single filesystem type
 *
 *	- walk filesystem, compare sys_getcwd with getcwd for each
 *	directory
 */

void
usage(progname)
	char *progname;
{
	fprintf(stderr, "usage: %s [-srpvw] [-l nloops]\n", progname);
	exit(1);
}

int run_stress = 0;
int run_regression = 0;
int run_performance = 0;

int
main(argc, argv)
	int argc;
	char **argv;
{
	int ch;
	char *progname = argv[0];

	uid_from_user("nobody", &altid);

	while ((ch = getopt(argc, argv, "srpvwl:u:")) != -1)
		switch (ch) {
		case 's':
			run_stress++;
			break;
		case 'r':
			run_regression++;
			break;
		case 'p':
			run_performance++;
			break;
		case 'v':
			verbose++;
			break;
		case 'w':
			sleepflag++;
			break;
		case 'l':
			nloops = atoi(optarg);
			if (nloops == 0)
				nloops = 100;
			break;
		case 'u':
			if (uid_from_user(optarg, &altid) != 0) {
				fprintf(stderr, "unknown user %s\n", optarg);
				usage(progname);
				exit(1);
			}
			break;
		case '?':
		default:
			usage(progname);
		}
	if (argc != optind)
		usage(progname);

	if (run_regression) 
		test___getcwd();
	
	if (!fail && run_performance)
		test_speed();

	if (!fail && run_stress)
		stress_test_getcwd();

	
	if (verbose)
		printf ("%d passes\n", pass);
	if (!fail)
		exit (0);
	else {
		printf("%d failures\n", fail);
		exit(1);
	}
}

     
