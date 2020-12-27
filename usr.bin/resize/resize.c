/* $NetBSD: resize.c,v 1.1 2020/12/27 21:13:18 reinoud Exp $ */
/* $XTermId: resize.c,v 1.139 2017/05/31 08:58:56 tom Exp $ */

/*
 * Copyright 2003-2015,2017 by Thomas E. Dickey
 *
 *                         All Rights Reserved
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization.
 *
 *
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 *
 *                         All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Digital Equipment
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 *
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * Extracted version from Xterm tailored for NetBSD
 */
/* resize.c */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>
#include <libgen.h>
#include <termios.h>
#include "xstrings.h"


/* imported from origional <xterm.h> */
#define DFT_TERMTYPE "xterm"
#define UIntClr(dst,bits) dst = dst & (unsigned) ~(bits)

#include <signal.h>
#include <pwd.h>

#define ESCAPE(string) "\033" string

#define	EMULATIONS	2
#define	SUN		1
#define	VT100		0

#define	TIMEOUT		10

#define	SHELL_UNKNOWN	0
#define	SHELL_C		1
#define	SHELL_BOURNE	2
/* *INDENT-OFF* */
static struct {
    const char *name;
    int type;
} shell_list[] = {
    { "csh",	SHELL_C },	/* vanilla cshell */
    { "jcsh",   SHELL_C },
    { "tcsh",   SHELL_C },
    { "sh",	SHELL_BOURNE }, /* vanilla Bourne shell */
    { "ash",    SHELL_BOURNE },
    { "bash",	SHELL_BOURNE }, /* GNU Bourne again shell */
    { "dash",	SHELL_BOURNE },
    { "jsh",    SHELL_BOURNE },
    { "ksh",	SHELL_BOURNE }, /* Korn shell (from AT&T toolchest) */
    { "ksh-i",	SHELL_BOURNE }, /* another name for Korn shell */
    { "ksh93",	SHELL_BOURNE }, /* Korn shell */
    { "mksh",   SHELL_BOURNE },
    { "pdksh",  SHELL_BOURNE },
    { "zsh",    SHELL_BOURNE },
    { NULL,	SHELL_BOURNE }	/* default (same as xterm's) */
};
/* *INDENT-ON* */

static const char *const emuname[EMULATIONS] =
{
    "VT100",
    "Sun",
};
static char *myname;
static int shell_type = SHELL_UNKNOWN;
static const char *const getsize[EMULATIONS] =
{
    ESCAPE("7") ESCAPE("[r") ESCAPE("[9999;9999H") ESCAPE("[6n"),
    ESCAPE("[18t"),
};
static const char *const restore[EMULATIONS] =
{
    ESCAPE("8"),
    0,
};
static const char *const setsize[EMULATIONS] =
{
    0,
    ESCAPE("[8;%s;%st"),
};

static struct termios tioorig;

static const char *const size[EMULATIONS] =
{
    ESCAPE("[%d;%dR"),
    ESCAPE("[8;%d;%dt"),
};
static const char sunname[] = "sunsize";
static int tty;
static FILE *ttyfp;


static void
failed(const char *s)
{
    int save = errno;
    IGNORE_RC(write(2, myname, strlen(myname)));
    IGNORE_RC(write(2, ": ", (size_t) 2));
    errno = save;
    perror(s);
    exit(EXIT_FAILURE);
}

/* ARGSUSED */
static void
onintr(int sig GCC_UNUSED)
{
    (void) tcsetattr(tty, TCSADRAIN, &tioorig);
    exit(EXIT_FAILURE);
}

static void
resize_timeout(int sig)
{
    fprintf(stderr, "\n%s: Time out occurred\r\n", myname);
    onintr(sig);
}

static void
Usage(void)
{
    fprintf(stderr, strcmp(myname, sunname) == 0 ?
	    "Usage: %s [rows cols]\n" :
	    "Usage: %s [-v] [-u] [-c] [-s [rows cols]]\n", myname);
    exit(EXIT_FAILURE);
}


static int
checkdigits(char *str)
{
    while (*str) {
	if (!isdigit(CharOf(*str)))
	    return (0);
	str++;
    }
    return (1);
}

static void
readstring(FILE *fp, char *buf, const char *str)
{
    int last, c;
    struct itimerval it;

    signal(SIGALRM, resize_timeout);
    memset((char *) &it, 0, sizeof(struct itimerval));
    it.it_value.tv_sec = TIMEOUT;
    setitimer(ITIMER_REAL, &it, (struct itimerval *) NULL);
    if ((c = getc(fp)) == 0233) {	/* meta-escape, CSI */
	c = ESCAPE("")[0];
	*buf++ = (char) c;
	*buf++ = '[';
    } else {
	*buf++ = (char) c;
    }
    if (c != *str) {
	fprintf(stderr, "%s: unknown character, exiting.\r\n", myname);
	onintr(0);
    }
    last = str[strlen(str) - 1];
    while ((*buf++ = (char) getc(fp)) != last) {
	;
    }
    memset((char *) &it, 0, sizeof(struct itimerval));
    setitimer(ITIMER_REAL, &it, (struct itimerval *) NULL);
    *buf = 0;
}

/*
   resets termcap string to reflect current screen size
 */
int
main(int argc, char **argv ENVP_ARG)
{
    char *ptr;
    int emu = VT100;
    char *shell;
    int i;
    int rc;
    int rows, cols;
    struct termios tio;
    char buf[BUFSIZ];
    char *name_of_tty;
    const char *setname = "";

    myname = x_basename(argv[0]);
    if (strcmp(myname, sunname) == 0)
	emu = SUN;
    for (argv++, argc--; argc > 0 && **argv == '-'; argv++, argc--) {
	switch ((*argv)[1]) {
	case 's':		/* Sun emulation */
	    if (emu == SUN)
		Usage();	/* Never returns */
	    emu = SUN;
	    break;
	case 'u':		/* Bourne (Unix) shell */
	    shell_type = SHELL_BOURNE;
	    break;
	case 'c':		/* C shell */
	    shell_type = SHELL_C;
	    break;
	case 'v':
	    printf("Xterm(330)\n");
	    exit(EXIT_SUCCESS);
	default:
	    Usage();		/* Never returns */
	}
    }

    if (SHELL_UNKNOWN == shell_type) {
	/* Find out what kind of shell this user is running.
	 * This is the same algorithm that xterm uses.
	 */
	if ((ptr = x_getenv("SHELL")) == NULL) {
	    uid_t uid = getuid();
	    struct passwd pw;

	    if (x_getpwuid(uid, &pw)) {
		(void) x_getlogin(uid, &pw);
	    }
	    if (!OkPasswd(&pw)
		|| *(ptr = pw.pw_shell) == 0) {
		/* this is the same default that xterm uses */
		ptr = x_strdup("/bin/sh");
	    }
	}

	shell = x_basename(ptr);

	/* now that we know, what kind is it? */
	for (i = 0; shell_list[i].name; i++) {
	    if (!strcmp(shell_list[i].name, shell)) {
		break;
	    }
	}
	shell_type = shell_list[i].type;
    }

    if (argc == 2) {
	if (!setsize[emu]) {
	    fprintf(stderr,
		    "%s: Can't set window size under %s emulation\n",
		    myname, emuname[emu]);
	    exit(EXIT_FAILURE);
	}
	if (!checkdigits(argv[0]) || !checkdigits(argv[1])) {
	    Usage();		/* Never returns */
	}
    } else if (argc != 0) {
	Usage();		/* Never returns */
    }
	name_of_tty = x_strdup("/dev/tty");

    if ((ttyfp = fopen(name_of_tty, "r+")) == NULL) {
	fprintf(stderr, "%s:  can't open terminal %s\n",
		myname, name_of_tty);
	exit(EXIT_FAILURE);
    }
    tty = fileno(ttyfp);
    if (x_getenv("TERM") == 0) {
	if (SHELL_BOURNE == shell_type) {
	    setname = "TERM=" DFT_TERMTYPE ";\nexport TERM;\n";
	} else {
	    setname = "setenv TERM " DFT_TERMTYPE ";\n";
	}
    }

    rc = tcgetattr(tty, &tioorig);
    tio = tioorig;
    UIntClr(tio.c_iflag, ICRNL);
    UIntClr(tio.c_lflag, (ICANON | ECHO));
    tio.c_cflag |= CS8;
    tio.c_cc[VMIN] = 6;
    tio.c_cc[VTIME] = 1;
    if (rc != 0)
	failed("get tty settings");

    signal(SIGINT, onintr);
    signal(SIGQUIT, onintr);
    signal(SIGTERM, onintr);

    rc = tcsetattr(tty, TCSADRAIN, &tio);
    if (rc != 0)
	failed("set tty settings");

    if (argc == 2) {		/* look for optional parameters of "-s" */
	char *tmpbuf = TypeMallocN(char,
				   strlen(setsize[emu]) +
				   strlen(argv[0]) +
				   strlen(argv[1]) +
				   1);
	if (tmpbuf == 0) {
	    fprintf(stderr, "%s: Cannot query size\n", myname);
	    onintr(0);
	} else {
	    sprintf(tmpbuf, setsize[emu], argv[0], argv[1]);
	    IGNORE_RC(write(tty, tmpbuf, strlen(tmpbuf)));
	    free(tmpbuf);
	}
    }
    IGNORE_RC(write(tty, getsize[emu], strlen(getsize[emu])));
    readstring(ttyfp, buf, size[emu]);
    if (sscanf(buf, size[emu], &rows, &cols) != 2) {
	fprintf(stderr, "%s: Can't get rows and columns\r\n", myname);
	onintr(0);
    }
    if (restore[emu])
	IGNORE_RC(write(tty, restore[emu], strlen(restore[emu])));

    rc = tcsetattr(tty, TCSADRAIN, &tioorig);
    if (rc != 0)
	failed("set tty settings");

    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);


    if (SHELL_BOURNE == shell_type) {

	printf("%sCOLUMNS=%d;\nLINES=%d;\nexport COLUMNS LINES;\n",
	       setname, cols, rows);

    } else {			/* not Bourne shell */

	printf("set noglob;\n%ssetenv COLUMNS '%d';\nsetenv LINES '%d';\nunset noglob;\n",
	       setname, cols, rows);
    }
    exit(EXIT_SUCCESS);
}
