/*++
/* NAME
/*	postcat 1
/* SUMMARY
/*	show Postfix queue file contents
/* SYNOPSIS
/*	\fBpostcat\fR [\fB-oqv\fR] [\fB-c \fIconfig_dir\fR] [\fIfiles\fR...]
/* DESCRIPTION
/*	The \fBpostcat\fR(1) command prints the contents of the named
/*	\fIfiles\fR in human-readable form. The files are expected
/*	to be in Postfix queue file format. If no
/*	\fIfiles\fR are specified on the command line, the program
/*	reads from standard input.
/*
/*	Options:
/* .IP "\fB-c \fIconfig_dir\fR"
/*	The \fBmain.cf\fR configuration file is in the named directory
/*	instead of the default configuration directory.
/* .IP \fB-o\fR
/*	Print the queue file offset of each record.
/* .IP \fB-q\fR
/*	Search the Postfix queue for the named \fIfiles\fR instead
/*	of taking the names literally.
/*
/*	Available in Postfix version 2.0 and later.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
/* ENVIRONMENT
/* .ad
/* .fi
/* .IP \fBMAIL_CONFIG\fR
/*	Directory with Postfix configuration files.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* FILES
/*	/var/spool/postfix, Postfix queue directory
/* SEE ALSO
/*	postconf(5), Postfix configuration
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
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <msg_vstream.h>
#include <vstring_vstream.h>
#include <stringops.h>

/* Global library. */

#include <record.h>
#include <rec_type.h>
#include <mail_queue.h>
#include <mail_conf.h>
#include <mail_params.h>
#include <mail_version.h>
#include <mail_proto.h>

/* Application-specific. */

#define PC_FLAG_QUEUE	(1<<0)		/* search queue */
#define PC_FLAG_OFFSET	(1<<1)		/* print record offsets */

#define STR	vstring_str
#define LEN	VSTRING_LEN

/* postcat - visualize Postfix queue file contents */

static void postcat(VSTREAM *fp, VSTRING *buffer, int flags)
{
    int     prev_type = 0;
    int     rec_type;
    struct timeval tv;
    time_t  time;
    int     first = 1;
    int     ch;
    off_t   offset;
    int     in_message = 0;
    const char *error_text;
    char   *attr_name;
    char   *attr_value;
    int     rec_flags = (msg_verbose ? REC_FLAG_NONE : REC_FLAG_DEFAULT);

#define TEXT_RECORD(rec_type) \
	    (rec_type == REC_TYPE_CONT || rec_type == REC_TYPE_NORM)

    /*
     * See if this is a plausible file.
     */
    if ((ch = VSTREAM_GETC(fp)) != VSTREAM_EOF) {
	if (!strchr(REC_TYPE_ENVELOPE, ch)) {
	    msg_warn("%s: input is not a valid queue file", VSTREAM_PATH(fp));
	    return;
	}
	vstream_ungetc(fp, ch);
    }

    /*
     * Now look at the rest.
     */
    do {
	if (flags & PC_FLAG_OFFSET)
	    offset = vstream_ftell(fp);
	rec_type = rec_get_raw(fp, buffer, 0, rec_flags);
	if (rec_type == REC_TYPE_ERROR)
	    msg_fatal("record read error");
	if (rec_type == REC_TYPE_EOF)
	    break;
	if (first == 1) {
	    vstream_printf("*** ENVELOPE RECORDS %s ***\n", VSTREAM_PATH(fp));
	    first = 0;
	}
	if (prev_type == REC_TYPE_CONT && !TEXT_RECORD(rec_type))
	    VSTREAM_PUTCHAR('\n');
	if (flags & PC_FLAG_OFFSET)
	    vstream_printf("%9lu ", (unsigned long) offset);
	switch (rec_type) {
	case REC_TYPE_TIME:
	    REC_TYPE_TIME_SCAN(STR(buffer), tv);
	    time = tv.tv_sec;
	    vstream_printf("%s: %s", rec_type_name(rec_type),
			   asctime(localtime(&time)));
	    break;
	case REC_TYPE_WARN:
	    REC_TYPE_WARN_SCAN(STR(buffer), time);
	    vstream_printf("%s: %s", rec_type_name(rec_type),
			   asctime(localtime(&time)));
	    break;
	case REC_TYPE_PTR:			/* pointer */
	    vstream_printf("%s: ", rec_type_name(rec_type));
	    vstream_fwrite(VSTREAM_OUT, STR(buffer), LEN(buffer));
	    VSTREAM_PUTCHAR('\n');
	    if (rec_goto(fp, STR(buffer)) == REC_TYPE_ERROR)
		msg_fatal("bad pointer record, or input is not seekable");
	    break;
	case REC_TYPE_CONT:			/* REC_TYPE_FILT collision */
	    if (!in_message)
		vstream_printf("%s: ", rec_type_name(rec_type));
	    else if (msg_verbose)
		vstream_printf("unterminated_text: ");
	    vstream_fwrite(VSTREAM_OUT, STR(buffer), LEN(buffer));
	    if (!in_message || msg_verbose || (flags & PC_FLAG_OFFSET) != 0) {
		rec_type = 0;
		VSTREAM_PUTCHAR('\n');
	    }
	    break;
	case REC_TYPE_NORM:
	    if (msg_verbose)
		vstream_printf("%s: ", rec_type_name(rec_type));
	    vstream_fwrite(VSTREAM_OUT, STR(buffer), LEN(buffer));
	    VSTREAM_PUTCHAR('\n');
	    break;
	case REC_TYPE_DTXT:
	    if (msg_verbose) {
		vstream_printf("%s: ", rec_type_name(rec_type));
		vstream_fwrite(VSTREAM_OUT, STR(buffer), LEN(buffer));
		VSTREAM_PUTCHAR('\n');
	    }
	    break;
	case REC_TYPE_MESG:
	    vstream_printf("*** MESSAGE CONTENTS %s ***\n", VSTREAM_PATH(fp));
	    in_message = 1;
	    break;
	case REC_TYPE_XTRA:
	    vstream_printf("*** HEADER EXTRACTED %s ***\n", VSTREAM_PATH(fp));
	    in_message = 0;
	    break;
	case REC_TYPE_END:
	    vstream_printf("*** MESSAGE FILE END %s ***\n", VSTREAM_PATH(fp));
	    break;
	case REC_TYPE_ATTR:
	    error_text = split_nameval(STR(buffer), &attr_name, &attr_value);
	    if (error_text != 0) {
		msg_warn("%s: malformed attribute: %s: %.100s",
			 VSTREAM_PATH(fp), error_text, STR(buffer));
		break;
	    }
	    if (strcmp(attr_name, MAIL_ATTR_CREATE_TIME) == 0) {
		time = atol(attr_value);
		vstream_printf("%s: %s", MAIL_ATTR_CREATE_TIME,
			       asctime(localtime(&time)));
		break;
	    }
	    vstream_printf("%s: %s=%s\n", rec_type_name(rec_type),
			   attr_name, attr_value);
	    break;
	default:
	    vstream_printf("%s: %s\n", rec_type_name(rec_type), STR(buffer));
	    break;
	}
	prev_type = rec_type;

	/*
	 * In case the next record is broken.
	 */
	vstream_fflush(VSTREAM_OUT);
    } while (rec_type != REC_TYPE_END);
}

/* usage - explain and terminate */

static NORETURN usage(char *myname)
{
    msg_fatal("usage: %s [-c config_dir] [-q (access queue)] [-v] [file(s)...]",
	      myname);
}

MAIL_VERSION_STAMP_DECLARE;

int     main(int argc, char **argv)
{
    VSTRING *buffer;
    VSTREAM *fp;
    int     ch;
    int     fd;
    struct stat st;
    int     flags = 0;
    static char *queue_names[] = {
	MAIL_QUEUE_MAILDROP,
	MAIL_QUEUE_INCOMING,
	MAIL_QUEUE_ACTIVE,
	MAIL_QUEUE_DEFERRED,
	MAIL_QUEUE_HOLD,
	0,
    };
    char  **cpp;
    int     tries;

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

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
    while ((ch = GETOPT(argc, argv, "c:oqv")) > 0) {
	switch (ch) {
	case 'c':
	    if (setenv(CONF_ENV_PATH, optarg, 1) < 0)
		msg_fatal("out of memory");
	    break;
	case 'o':
	    flags |= PC_FLAG_OFFSET;
	    break;
	case 'q':
	    flags |= PC_FLAG_QUEUE;
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	default:
	    usage(argv[0]);
	}
    }

    /*
     * Further initialization...
     */
    mail_conf_read();

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
	postcat(VSTREAM_IN, buffer, flags);
    }

    /*
     * Copy the named queue files in the specified order.
     */
    else if (flags & PC_FLAG_QUEUE) {
	if (chdir(var_queue_dir))
	    msg_fatal("chdir %s: %m", var_queue_dir);
	while (optind < argc) {
	    if (!mail_queue_id_ok(argv[optind]))
		msg_fatal("bad mail queue ID: %s", argv[optind]);
	    for (fp = 0, tries = 0; fp == 0 && tries < 2; tries++)
		for (cpp = queue_names; fp == 0 && *cpp != 0; cpp++)
		    fp = mail_queue_open(*cpp, argv[optind], O_RDONLY, 0);
	    if (fp == 0)
		msg_fatal("open queue file %s: %m", argv[optind]);
	    postcat(fp, buffer, flags);
	    if (vstream_fclose(fp))
		msg_warn("close %s: %m", argv[optind]);
	    optind++;
	}
    }

    /*
     * Copy the named files in the specified order.
     */
    else {
	while (optind < argc) {
	    if ((fp = vstream_fopen(argv[optind], O_RDONLY, 0)) == 0)
		msg_fatal("open %s: %m", argv[optind]);
	    postcat(fp, buffer, flags);
	    if (vstream_fclose(fp))
		msg_warn("close %s: %m", argv[optind]);
	    optind++;
	}
    }

    /*
     * Clean up.
     */
    vstring_free(buffer);
    exit(0);
}
