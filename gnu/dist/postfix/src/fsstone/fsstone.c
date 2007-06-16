/*	$NetBSD: fsstone.c,v 1.1.1.2.10.1 2007/06/16 16:59:53 snj Exp $	*/

/*++
/* NAME
/*	fsstone 1
/* SUMMARY
/*	measure directory operation overhead
/* SYNOPSIS
/*	\fBfsstone\fR [\fB-cr\fR] [\fB-s \fIsize\fR]
/*		\fImsg_count files_per_dir\fR
/* DESCRIPTION
/*	The \fBfsstone\fR command measures the cost of creating, renaming
/*	and deleting queue files versus appending messages to existing
/*	files and truncating them after use.
/*
/*	The program simulates the arrival of \fImsg_count\fR short messages,
/*	and arranges for at most \fIfiles_per_dir\fR simultaneous files
/*	in the same directory.
/*
/*	Options:
/* .IP \fB-c\fR
/*	Create and delete files.
/* .IP \fB-r\fR
/*	Rename files twice (requires \fB-c\fR).
/* .IP \fB-s \fIsize\fR
/*	Specify the file size in kbytes.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
/* BUGS
/*	The \fB-r\fR option renames files within the same directory.
/*	For a more realistic simulation, the program should rename files
/*	<i>between</i> directories, and should also have an option to use
/*	<i>hashed</i> directories as implemented with, for example, the
/*	\fBdir_forest\fR(3) module.
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/* Utility library. */

#include <msg.h>
#include <msg_vstream.h>

/* rename_file - rename a file */

static void rename_file(int old, int new)
{
    char    new_path[BUFSIZ];
    char    old_path[BUFSIZ];

    sprintf(new_path, "%06d", new);
    sprintf(old_path, "%06d", old);
    if (rename(old_path, new_path))
	msg_fatal("rename %s to %s: %m", old_path, new_path);
}

/* make_file - create a little file and use it */

static void make_file(int seqno, int size)
{
    char    path[BUFSIZ];
    char    buf[1024];
    FILE   *fp;
    int     i;

    sprintf(path, "%06d", seqno);
    if ((fp = fopen(path, "w")) == 0)
	msg_fatal("open %s: %m", path);
    memset(buf, 'x', sizeof(buf));
    for (i = 0; i < size; i++)
	if (fwrite(buf, 1, sizeof(buf), fp) != sizeof(buf))
	    msg_fatal("fwrite: %m");
    if (fsync(fileno(fp)))
	msg_fatal("fsync: %m");
    if (fclose(fp))
	msg_fatal("fclose: %m");
    if ((fp = fopen(path, "r")) == 0)
	msg_fatal("open %s: %m", path);
    while (fgets(path, sizeof(path), fp))
	 /* void */ ;
    if (fclose(fp))
	msg_fatal("fclose: %m");
}

/* use_file - use existing file */

static void use_file(int seqno)
{
    char    path[BUFSIZ];
    FILE   *fp;
    int     i;

    sprintf(path, "%06d", seqno);
    if ((fp = fopen(path, "w")) == 0)
	msg_fatal("open %s: %m", path);
    for (i = 0; i < 400; i++)
	fprintf(fp, "hello");
    if (fsync(fileno(fp)))
	msg_fatal("fsync: %m");
    if (fclose(fp))
	msg_fatal("fclose: %m");
    if ((fp = fopen(path, "r+")) == 0)
	msg_fatal("open %s: %m", path);
    while (fgets(path, sizeof(path), fp))
	 /* void */ ;
    if (ftruncate(fileno(fp), (off_t) 0))
	msg_fatal("ftruncate: %m");;
    if (fclose(fp))
	msg_fatal("fclose: %m");
}

/* remove_file - delete specified file */

static void remove_file(int seq)
{
    char    path[BUFSIZ];

    sprintf(path, "%06d", seq);
    if (remove(path))
	msg_fatal("remove %s: %m", path);
}

/* remove_silent - delete specified file, silently */

static void remove_silent(int seq)
{
    char    path[BUFSIZ];

    sprintf(path, "%06d", seq);
    (void) remove(path);
}

/* usage - explain */

static void usage(char *myname)
{
    msg_fatal("usage: %s [-cr] [-s size] messages directory_entries", myname);
}

MAIL_VERSION_STAMP_DECLARE;

int     main(int argc, char **argv)
{
    int     op_count;
    int     max_file;
    time_t  start;
    int     do_rename = 0;
    int     do_create = 0;
    int     seq;
    int     ch;
    int     size = 2;

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    msg_vstream_init(argv[0], VSTREAM_ERR);
    while ((ch = GETOPT(argc, argv, "crs:")) != EOF) {
	switch (ch) {
	case 'c':
	    do_create++;
	    break;
	case 'r':
	    do_rename++;
	    break;
	case 's':
	    if ((size = atoi(optarg)) <= 0)
		usage(argv[0]);
	    break;
	default:
	    usage(argv[0]);
	}
    }

    if (argc - optind != 2 || (do_rename && !do_create))
	usage(argv[0]);
    if ((op_count = atoi(argv[optind])) <= 0)
	usage(argv[0]);
    if ((max_file = atoi(argv[optind + 1])) <= 0)
	usage(argv[0]);

    /*
     * Populate the directory with little files.
     */
    for (seq = 0; seq < max_file; seq++)
	make_file(seq, size);

    /*
     * Simulate arrival and delivery of mail messages.
     */
    start = time((time_t *) 0);
    while (op_count > 0) {
	seq %= max_file;
	if (do_create) {
	    remove_file(seq);
	    make_file(seq, size);
	    if (do_rename) {
		rename_file(seq, seq + max_file);
		rename_file(seq + max_file, seq);
	    }
	} else {
	    use_file(seq);
	}
	seq++;
	op_count--;
    }
    printf("elapsed time: %ld\n", (long) time((time_t *) 0) - start);

    /*
     * Clean up directory fillers.
     */
    for (seq = 0; seq < max_file; seq++)
	remove_silent(seq);
    return (0);
}
