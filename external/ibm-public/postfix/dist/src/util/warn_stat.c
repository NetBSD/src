/*	$NetBSD: warn_stat.c,v 1.1.1.1 2013/01/02 18:59:15 tron Exp $	*/

/*++
/* NAME
/*	warn_stat 3
/* SUMMARY
/*	baby-sit stat() error returns
/* SYNOPSIS
/*	#include <warn_stat.h>
/*
/*	int     warn_stat(path, st)
/*	const char *path;
/*	struct stat *st;
/*
/*	int     warn_lstat(path, st)
/*	const char *path;
/*	struct stat *st;
/*
/*	int     warn_fstat(fd, st)
/*	int     fd;
/*	struct stat *st;
/* DESCRIPTION
/*	warn_stat(), warn_fstat() and warn_lstat() wrap the stat(),
/*	fstat() and lstat() system calls with code that logs a
/*	diagnosis for common error cases.
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

/* Utility library. */

#include <msg.h>
#define WARN_STAT_INTERNAL
#include <warn_stat.h>

/* diagnose_stat - log stat warning */

static void diagnose_stat(void)
{
    struct stat st;

    /*
     * When *stat() fails with EOVERFLOW, and the interface uses 32-bit data
     * types, suggest that the program be recompiled with larger data types.
     */
#ifdef EOVERFLOW
    if (errno == EOVERFLOW && sizeof(st.st_size) == 4) {
	msg_warn("this program was built for 32-bit file handles, "
		 "but some number does not fit in 32 bits");
	msg_warn("possible solution: recompile in 64-bit mode, or "
		 "recompile in 32-bit mode with 'large file' support");
    }
#endif
}

/* warn_stat - stat with warning */

int     warn_stat(const char *path, struct stat * st)
{
    int     ret;

    ret = stat(path, st);
    if (ret < 0)
	diagnose_stat();
    return (ret);
}

/* warn_lstat - lstat with warning */

int     warn_lstat(const char *path, struct stat * st)
{
    int     ret;

    ret = lstat(path, st);
    if (ret < 0)
	diagnose_stat();
    return (ret);
}

/* warn_fstat - fstat with warning */

int     warn_fstat(int fd, struct stat * st)
{
    int     ret;

    ret = fstat(fd, st);
    if (ret < 0)
	diagnose_stat();
    return (ret);
}
