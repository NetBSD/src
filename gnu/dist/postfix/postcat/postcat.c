/*++
/* NAME
/*	postcat 1
/* SUMMARY
/*	show Postfix queue file contents
/* SYNOPSIS
/*	\fBpostcat\fR [\fB-v\fR] [\fIfiles\fR...]
/* DESCRIPTION
/*	The \fBpostcat\fR command prints the contents of the named
/*	Postfix queue \fIfiles\fR in human-readable form. If no
/*	\fIfiles\fR are specified on the command line, the program
/*	reads from standard input.
/*
/*	Options:
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
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
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <msg_vstream.h>
#include <vstring_vstream.h>

/* Global library. */

#include <record.h>
#include <rec_type.h>

/* Application-specific. */

#define STR	vstring_str

/* postcat - visualize Postfix queue file contents */

static void postcat(VSTREAM *fp, VSTRING *buffer)
{
    int     prev_type = 0;
    int     rec_type;
    time_t  time;
    int     first = 1;
    int     ch;

#define TEXT_RECORD(rec_type) \
	    (rec_type == REC_TYPE_CONT || rec_type == REC_TYPE_NORM)

    /*
     * See if this is a plausible file.
     */
    if ((ch = VSTREAM_GETC(fp)) != VSTREAM_EOF) {
	if (ch != REC_TYPE_TIME && ch != REC_TYPE_SIZE) {
	    msg_warn("%s: input is not a valid queue file", VSTREAM_PATH(fp));
	    return;
	}
	vstream_ungetc(fp, ch);
    }

    /*
     * Now look at the rest.
     */
    for (;;) {
	rec_type = rec_get(fp, buffer, 0);
	if (rec_type == REC_TYPE_ERROR)
	    msg_fatal("record read error");
	if (rec_type == REC_TYPE_EOF)
	    return;
	if (first == 1) {
	    vstream_printf("*** ENVELOPE RECORDS %s ***\n", VSTREAM_PATH(fp));
	    first = 0;
	}
	if (prev_type == REC_TYPE_CONT && !TEXT_RECORD(rec_type))
	    VSTREAM_PUTCHAR('\n');
	switch (rec_type) {
	case REC_TYPE_SIZE:
	    vstream_printf("message_size: %s\n", STR(buffer));
	    break;
	case REC_TYPE_TIME:
	    time = atol(STR(buffer));
	    vstream_printf("arrival_time: %s", asctime(localtime(&time)));
	    break;
	case REC_TYPE_WARN:
	    time = atol(STR(buffer));
	    vstream_printf("defer_warn_time: %s", asctime(localtime(&time)));
	    break;
	case REC_TYPE_CONT:
	    vstream_printf("%s", STR(buffer));
	    break;
	case REC_TYPE_NORM:
	    vstream_printf("%s\n", STR(buffer));
	    break;
	case REC_TYPE_MESG:
	    vstream_printf("*** MESSAGE CONTENTS %s ***\n", VSTREAM_PATH(fp));
	    break;
	case REC_TYPE_XTRA:
	    vstream_printf("*** HEADER EXTRACTED %s ***\n", VSTREAM_PATH(fp));
	    break;
	case REC_TYPE_END:
	    vstream_printf("*** MESSAGE FILE END %s ***\n", VSTREAM_PATH(fp));
	    break;
	default:
	    vstream_printf("%s: %s\n", rec_type_name(rec_type), STR(buffer));
	    break;
	}
	prev_type = rec_type;
	vstream_fflush(VSTREAM_OUT);
    }
}

/* usage - explain and terminate */

static NORETURN usage(char *myname)
{
    msg_fatal("usage: %s [-v] [file(s)...]", myname);
}

int     main(int argc, char **argv)
{
    VSTRING *buffer;
    VSTREAM *fp;
    int     ch;
    int     fd;
    struct stat st;

    /*
     * To minimize confusion, make sure that the standard file descriptors
     * are open before opening anything else. XXX Work around for 44BSD where
     * fstat can return EBADF on an open file descriptor.
     */
    for (fd = 0; fd < 3; fd++)
	if (fstat(fd, &st) == -1
	    && (close(fd), open("/dev/null", O_RDWR, 0)) != fd)
	    msg_fatal("open /dev/null: %m");

    /*
     * Set up logging.
     */
    msg_vstream_init(argv[0], VSTREAM_ERR);

    /*
     * Parse JCL.
     */
    while ((ch = GETOPT(argc, argv, "v")) > 0) {
	switch (ch) {
	case 'v':
	    msg_verbose++;
	    break;
	default:
	    usage(argv[0]);
	}
    }

    /*
     * Initialize.
     */
    buffer = vstring_alloc(10);

    /*
     * If no file names are given, copy stdin.
     */
    if (argc == optind) {
	vstream_control(VSTREAM_IN,
			VSTREAM_CTL_PATH, "stdin",
			VSTREAM_CTL_END);
	postcat(VSTREAM_IN, buffer);
    }

    /*
     * Copy the named files in the specified order.
     */
    else {
	while (optind < argc) {
	    if ((fp = vstream_fopen(argv[optind], O_RDONLY, 0)) == 0)
		msg_fatal("open %s: %m", argv[optind]);
	    postcat(fp, buffer);
	    if (vstream_fclose(fp))
		msg_warn("close %s: %m", argv[optind]);
	    optind++;
	}
    }
    vstring_free(buffer);
    exit(0);
}
