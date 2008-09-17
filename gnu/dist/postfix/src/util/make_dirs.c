/*++
/* NAME
/*	make_dirs 3
/* SUMMARY
/*	create directory hierarchy
/* SYNOPSIS
/*	#include <make_dirs.h>
/*
/*	int	make_dirs(path, perms)
/*	const char *path;
/*	int	perms;
/* DESCRIPTION
/*	make_dirs() creates the directory specified in \fIpath\fR, and
/*	creates any missing intermediate directories as well. Directories
/*	are created with the permissions specified in \fIperms\fR, as
/*	modified by the process umask.
/* DIAGNOSTICS:
/*	Fatal: out of memory. make_dirs() returns 0 in case of success.
/*	In case of problems. make_dirs() returns -1 and \fIerrno\fR
/*	reflects the nature of the problem.
/* SEE ALSO
/*	mkdir(2)
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

/* Utility library. */

#include "msg.h"
#include "mymalloc.h"
#include "stringops.h"
#include "make_dirs.h"

/* make_dirs - create directory hierarchy */

int     make_dirs(const char *path, int perms)
{
    char   *saved_path;
    unsigned char *cp;
    int     saved_ch;
    struct stat st;
    int     ret;
    mode_t  saved_mode = 0;

    /*
     * Initialize. Make a copy of the path that we can safely clobber.
     */
    cp = (unsigned char *) (saved_path = mystrdup(path));

    /*
     * I didn't like the 4.4BSD "mkdir -p" implementation, but coming up with
     * my own took a day, spread out over several days.
     */
#define SKIP_WHILE(cond, ptr) { while(*ptr && (cond)) ptr++; }

    SKIP_WHILE(*cp == '/', cp);

    for (;;) {
	SKIP_WHILE(*cp != '/', cp);
	if ((saved_ch = *cp) != 0)
	    *cp = 0;
	if ((ret = stat(saved_path, &st)) >= 0) {
	    if (!S_ISDIR(st.st_mode)) {
		errno = ENOTDIR;
		ret = -1;
		break;
	    }
	    saved_mode = st.st_mode;
	} else {
	    if (errno != ENOENT)
		break;

	    /*
	     * mkdir(foo) fails with EEXIST if foo is a symlink.
	     */
#if 0

	    /*
	     * Create a new directory. Unfortunately, mkdir(2) has no
	     * equivalent of open(2)'s O_CREAT|O_EXCL safety net, so we must
	     * require that the parent directory is not world writable.
	     * Detecting a lost race condition after the fact is not
	     * sufficient, as an attacker could repeat the attack and add one
	     * directory level at a time.
	     */
	    if (saved_mode & S_IWOTH) {
		msg_warn("refusing to mkdir %s: parent directory is writable by everyone",
			 saved_path);
		errno = EPERM;
		ret = -1;
		break;
	    }
#endif
	    if ((ret = mkdir(saved_path, perms)) < 0) {
		if (errno != EEXIST)
		    break;
		/* Race condition? */
		if ((ret = stat(saved_path, &st)) < 0)
		    break;
		if (!S_ISDIR(st.st_mode)) {
		    errno = ENOTDIR;
		    ret = -1;
		    break;
		}
	    }
	}
	if (saved_ch != 0)
	    *cp = saved_ch;
	SKIP_WHILE(*cp == '/', cp);
	if (*cp == 0)
	    break;
    }

    /*
     * Cleanup.
     */
    myfree(saved_path);
    return (ret);
}

#ifdef TEST

 /*
  * Test program. Usage: make_dirs path...
  */
#include <stdlib.h>
#include <msg_vstream.h>

int     main(int argc, char **argv)
{
    msg_vstream_init(argv[0], VSTREAM_ERR);
    if (argc < 2)
	msg_fatal("usage: %s path...", argv[0]);
    while (--argc > 0 && *++argv != 0)
	if (make_dirs(*argv, 0755))
	    msg_fatal("%s: %m", *argv);
    exit(0);
}

#endif
