/*++
/* NAME
/*	scan_dir 3
/* SUMMARY
/*	directory scanning
/* SYNOPSIS
/*	#include <scan_dir.h>
/*
/*	SCAN_DIR *scan_dir_open(path)
/*	const char *path;
/*
/*	char	*scan_dir_next(scan)
/*	SCAN_DIR *scan;
/*
/*	char	*scan_dir_path(scan)
/*	SCAN_DIR *scan;
/*
/*	void	scan_push(scan, entry)
/*	SCAN_DIR *scan;
/*	const char *entry;
/*
/*	SCAN_DIR *scan_pop(scan)
/*	SCAN_DIR *scan;
/*
/*	SCAN_DIR *scan_dir_close(scan)
/*	SCAN_DIR *scan;
/* DESCRIPTION
/*	These functions scan directories for names. The "." and
/*	".." names are skipped. Essentially, this is <dirent>
/*	extended with error handling and with knowledge of the
/*	name of the directory being scanned.
/*
/*	scan_dir_open() opens the named directory and
/*	returns a handle for subsequent use.
/*
/*	scan_dir_close() terminates the directory scan, cleans up
/*	and returns a null pointer.
/*
/*	scan_dir_next() returns the next requested object in the specified
/*	directory. It skips the "." and ".." entries.
/*
/*	scan_dir_path() returns the name of the directory being scanned.
/*
/*	scan_dir_push() causes the specified directory scan to enter the
/*	named subdirectory.
/*
/*	scan_dir_pop() leaves the directory being scanned and returns
/*	to the previous one. The result is the argument, null if no
/*	previous directory information is available.
/* DIAGNOSTICS
/*	All errors are fatal.
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
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#else
#define dirent direct
#ifdef HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#ifdef HAVE_NDIR_H
#include <ndir.h>
#endif
#endif
#include <string.h>

/* Utility library. */

#include "msg.h"
#include "mymalloc.h"
#include "stringops.h"
#include "vstring.h"
#include "scan_dir.h"

 /*
  * The interface is based on an opaque structure, so we don't have to expose
  * the user to the guts. Subdirectory info sits in front of parent directory
  * info: a simple last-in, first-out list.
  */
typedef struct SCAN_INFO SCAN_INFO;

struct SCAN_INFO {
    char   *path;			/* directory name */
    DIR    *dir;			/* directory structure */
    SCAN_INFO *parent;			/* linkage */
};
struct SCAN_DIR {
    SCAN_INFO *current;			/* current scan */
};

#define SCAN_DIR_PATH(scan)	(scan->current->path)
#define STR(x)			vstring_str(x)

/* scan_dir_path - return the path of the directory being read.  */

char   *scan_dir_path(SCAN_DIR *scan)
{
    return (SCAN_DIR_PATH(scan));
}

/* scan_dir_push - enter directory */

void    scan_dir_push(SCAN_DIR *scan, const char *path)
{
    char   *myname = "scan_dir_push";
    SCAN_INFO *info;

    info = (SCAN_INFO *) mymalloc(sizeof(*info));
    if (scan->current)
	info->path = concatenate(SCAN_DIR_PATH(scan), "/", path, (char *) 0);
    else
	info->path = mystrdup(path);
    if ((info->dir = opendir(info->path)) == 0)
	msg_fatal("%s: open directory %s: %m", myname, info->path);
    if (msg_verbose > 1)
	msg_info("%s: open %s", myname, info->path);
    info->parent = scan->current;
    scan->current = info;
}

/* scan_dir_pop - leave directory */

SCAN_DIR *scan_dir_pop(SCAN_DIR *scan)
{
    char   *myname = "scan_dir_pop";
    SCAN_INFO *info = scan->current;
    SCAN_INFO *parent;

    if (info == 0)
	return (0);
    parent = info->parent;
    if (closedir(info->dir))
	msg_fatal("%s: close directory %s: %m", myname, info->path);
    if (msg_verbose > 1)
	msg_info("%s: close %s", myname, info->path);
    myfree(info->path);
    myfree((char *) info);
    scan->current = parent;
    return (parent ? scan : 0);
}

/* scan_dir_open - start directory scan */

SCAN_DIR *scan_dir_open(const char *path)
{
    SCAN_DIR *scan;

    scan = (SCAN_DIR *) mymalloc(sizeof(*scan));
    scan->current = 0;
    scan_dir_push(scan, path);
    return (scan);
}

/* scan_dir_next - find next entry */

char   *scan_dir_next(SCAN_DIR *scan)
{
    char   *myname = "scan_dir_next";
    SCAN_INFO *info = scan->current;
    struct dirent *dp;

#define STREQ(x,y)	(strcmp((x),(y)) == 0)

    if (info) {
	while ((dp = readdir(info->dir)) != 0) {
	    if (STREQ(dp->d_name, ".") || STREQ(dp->d_name, "..")) {
		if (msg_verbose > 1)
		    msg_info("%s: skip %s", myname, dp->d_name);
		continue;
	    } else {
		if (msg_verbose > 1)
		    msg_info("%s: found %s", myname, dp->d_name);
		return (dp->d_name);
	    }
	}
    }
    return (0);
}

/* scan_dir_close - terminate directory scan */

SCAN_DIR *scan_dir_close(SCAN_DIR *scan)
{
    while (scan->current)
	scan_dir_pop(scan);
    myfree((char *) scan);
    return (0);
}
