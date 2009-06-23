/*	$NetBSD: load_file.c,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

/*++
/* NAME
/*	load_file 3
/* SUMMARY
/*	load file with some prejudice
/* SYNOPSIS
/*	#include <load_file.h>
/*
/*	void	load_file(path, action, context)
/*	const char *path;
/*	void	(*action)(VSTREAM, void *);
/*	void	*context;
/* DESCRIPTION
/*	This routine reads a file and reads it again when the
/*	file changed recently.
/*
/*	Arguments:
/* .IP path
/*	The file to be opened, read-only.
/* .IP action
/*	The function that presumably reads the file.
/* .IP context
/*	Application-specific context for the action routine.
/* DIAGNOSTICS
/*	Fatal errors: out of memory, cannot open file.
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
#include <time.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <iostuff.h>
#include <load_file.h>

/* load_file - load file with some prejudice */

void    load_file(const char *path, LOAD_FILE_FN action, void *context)
{
    VSTREAM *fp;
    struct stat st;
    time_t  before;
    time_t  after;

    /*
     * Read the file again if it is hot. This may result in reading a partial
     * parameter name or missing end marker when a file changes in the middle
     * of a read.
     */
    for (before = time((time_t *) 0); /* see below */ ; before = after) {
	if ((fp = vstream_fopen(path, O_RDONLY, 0)) == 0)
	    msg_fatal("open %s: %m", path);
	action(fp, context);
	if (fstat(vstream_fileno(fp), &st) < 0)
	    msg_fatal("fstat %s: %m", path);
	if (vstream_ferror(fp) || vstream_fclose(fp))
	    msg_fatal("read %s: %m", path);
	after = time((time_t *) 0);
	if (st.st_mtime < before - 1 || st.st_mtime > after)
	    break;
	if (msg_verbose)
	    msg_info("pausing to let %s cool down", path);
	doze(300000);
    }
}
