/*	$NetBSD: postcat.c,v 1.1.1.3 2013/01/02 18:59:03 tron Exp $	*/

/*++
/* NAME
/*	postcat 1
/* SUMMARY
/*	show Postfix queue file contents
/* SYNOPSIS
/*	\fBpostcat\fR [\fB-bdehnoqv\fR] [\fB-c \fIconfig_dir\fR] [\fIfiles\fR...]
/* DESCRIPTION
/*	The \fBpostcat\fR(1) command prints the contents of the
/*	named \fIfiles\fR in human-readable form. The files are
/*	expected to be in Postfix queue file format. If no \fIfiles\fR
/*	are specified on the command line, the program reads from
/*	standard input.
/*
/*	By default, \fBpostcat\fR(1) shows the envelope and message
/*	content, as if the options \fB-beh\fR were specified. To
/*	view message content only, specify \fB-bh\fR (Postfix 2.7
/*	and later).
/*
/*	Options:
/* .IP \fB-b\fR
/*	Show body content.  The \fB-b\fR option starts producing
/*	output at the first non-header line, and stops when the end
/*	of the message is reached.
/* .sp
/*	This feature is available in Postfix 2.7 and later.
/* .IP "\fB-c \fIconfig_dir\fR"
/*	The \fBmain.cf\fR configuration file is in the named directory
/*	instead of the default configuration directory.
/* .IP \fB-d\fR
/*	Print the decimal type of each record.
/* .IP \fB-e\fR
/*	Show message envelope content.
/* .sp
/*	This feature is available in Postfix 2.7 and later.
/* .IP \fB-h\fR
/*	Show message header content.  The \fB-h\fR option produces
/*	output from the beginning of the message up to, but not
/*	including, the first non-header line.
/* .sp
/*	This feature is available in Postfix 2.7 and later.
/* .IP \fB-o\fR
/*	Print the queue file offset of each record.
/* .IP \fB-q\fR
/*	Search the Postfix queue for the named \fIfiles\fR instead
/*	of taking the names literally.
/*
/*	This feature is available in Postfix 2.0 and later.
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
#include <stdio.h>			/* sscanf() */

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <msg_vstream.h>
#include <vstring_vstream.h>
#include <stringops.h>
#include <warn_stat.h>

/* Global library. */

#include <record.h>
#include <rec_type.h>
#include <mail_queue.h>
#include <mail_conf.h>
#include <mail_params.h>
#include <mail_version.h>
#include <mail_proto.h>
#include <is_header.h>
#include <lex_822.h>

/* Application-specific. */

#define PC_FLAG_SEARCH_QUEUE	(1<<0)	/* search queue */
#define PC_FLAG_PRINT_OFFSET	(1<<1)	/* print record offsets */
#define PC_FLAG_PRINT_ENV	(1<<2)	/* print envelope records */
#define PC_FLAG_PRINT_HEADER	(1<<3)	/* print header records */
#define PC_FLAG_PRINT_BODY	(1<<4)	/* print body records */
#define PC_FLAG_PRINT_RTYPE_DEC	(1<<5)	/* print decimal record type */
#define PC_FLAG_PRINT_RTYPE_SYM	(1<<6)	/* print symbolic record type */

#define PC_MASK_PRINT_TEXT	(PC_FLAG_PRINT_HEADER | PC_FLAG_PRINT_BODY)
#define PC_MASK_PRINT_ALL	(PC_FLAG_PRINT_ENV | PC_MASK_PRINT_TEXT)

 /*
  * State machine.
  */
#define PC_STATE_ENV	0		/* initial or extracted envelope */
#define PC_STATE_HEADER	1		/* primary header */
#define PC_STATE_BODY	2		/* other */

#define STR	vstring_str
#define LEN	VSTRING_LEN

/* postcat - visualize Postfix queue file contents */

static void postcat(VSTREAM *fp, VSTRING *buffer, int flags)
{
    int     prev_type = 0;
    int     rec_type;
    struct timeval tv;
    time_t  time;
    int     ch;
    off_t   offset;
    const char *error_text;
    char   *attr_name;
    char   *attr_value;
    int     rec_flags = (msg_verbose ? REC_FLAG_NONE : REC_FLAG_DEFAULT);
    int     state;			/* state machine, input type */
    int     do_print;			/* state machine, output control */
    long    data_offset;		/* state machine, read optimization */
    long    data_size;			/* state machine, read optimization */

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
     * Other preliminaries.
     */
    if (flags & PC_FLAG_PRINT_ENV)
	vstream_printf("*** ENVELOPE RECORDS %s ***\n",
		       VSTREAM_PATH(fp));
    state = PC_STATE_ENV;
    do_print = (flags & PC_FLAG_PRINT_ENV);
    data_offset = data_size = -1;

    /*
     * Now look at the rest.
     */
    for (;;) {
	if (flags & PC_FLAG_PRINT_OFFSET)
	    offset = vstream_ftell(fp);
	rec_type = rec_get_raw(fp, buffer, 0, rec_flags);
	if (rec_type == REC_TYPE_ERROR)
	    msg_fatal("record read error");
	if (rec_type == REC_TYPE_EOF)
	    break;

	/*
	 * First inspect records that have side effects on the (envelope,
	 * header, body) state machine or on the record reading order.
	 * 
	 * XXX Comments marked "Optimization:" identify subtle code that will
	 * likely need to be revised when the queue file organization is
	 * changed.
	 */
#define PRINT_MARKER(flags, fp, offset, type, text) do { \
    if ((flags) & PC_FLAG_PRINT_OFFSET) \
	vstream_printf("%9lu ", (unsigned long) (offset)); \
    if (flags & PC_FLAG_PRINT_RTYPE_DEC) \
	vstream_printf("%3d ", (type)); \
    vstream_printf("*** %s %s ***\n", (text), VSTREAM_PATH(fp)); \
    vstream_fflush(VSTREAM_OUT); \
} while (0)

#define PRINT_RECORD(flags, offset, type, value) do { \
    if ((flags) & PC_FLAG_PRINT_OFFSET) \
	vstream_printf("%9lu ", (unsigned long) (offset)); \
    if (flags & PC_FLAG_PRINT_RTYPE_DEC) \
	vstream_printf("%3d ", (type)); \
    vstream_printf("%s: %s\n", rec_type_name(rec_type), (value)); \
    vstream_fflush(VSTREAM_OUT); \
} while (0)

	if (TEXT_RECORD(rec_type)) {
	    /* This is wrong when the message starts with whitespace. */
	    if (state == PC_STATE_HEADER && (flags & (PC_MASK_PRINT_TEXT))
		&& prev_type != REC_TYPE_CONT && TEXT_RECORD(rec_type)
	     && !(is_header(STR(buffer)) || IS_SPACE_TAB(STR(buffer)[0]))) {
		/* Update the state machine. */
		state = PC_STATE_BODY;
		do_print = (flags & PC_FLAG_PRINT_BODY);
		/* Optimization: terminate if nothing left to print. */
		if (do_print == 0 && (flags & PC_FLAG_PRINT_ENV) == 0)
		    break;
		/* Optimization: skip to extracted segment marker. */
		if (do_print == 0 && (flags & PC_FLAG_PRINT_ENV)
		    && data_offset >= 0 && data_size >= 0
		    && vstream_fseek(fp, data_offset + data_size, SEEK_SET) < 0)
		    msg_fatal("seek error: %m");
	    }
	    /* Optional output happens further down below. */
	} else if (rec_type == REC_TYPE_MESG) {
	    /* Sanity check. */
	    if (state != PC_STATE_ENV)
		msg_warn("%s: out-of-order message content marker",
			 VSTREAM_PATH(fp));
	    /* Optional output. */
	    if (flags & PC_FLAG_PRINT_ENV)
		PRINT_MARKER(flags, fp, offset, rec_type, "MESSAGE CONTENTS");
	    /* Optimization: skip to extracted segment marker. */
	    if ((flags & PC_MASK_PRINT_TEXT) == 0
		&& data_offset >= 0 && data_size >= 0
		&& vstream_fseek(fp, data_offset + data_size, SEEK_SET) < 0)
		msg_fatal("seek error: %m");
	    /* Update the state machine, even when skipping. */
	    state = PC_STATE_HEADER;
	    do_print = (flags & PC_FLAG_PRINT_HEADER);
	    continue;
	} else if (rec_type == REC_TYPE_XTRA) {
	    /* Sanity check. */
	    if (state != PC_STATE_HEADER && state != PC_STATE_BODY)
		msg_warn("%s: out-of-order extracted segment marker",
			 VSTREAM_PATH(fp));
	    /* Optional output (terminate preceding header/body line). */
	    if (do_print && prev_type == REC_TYPE_CONT)
		VSTREAM_PUTCHAR('\n');
	    if (flags & PC_FLAG_PRINT_ENV)
		PRINT_MARKER(flags, fp, offset, rec_type, "HEADER EXTRACTED");
	    /* Update the state machine. */
	    state = PC_STATE_ENV;
	    do_print = (flags & PC_FLAG_PRINT_ENV);
	    /* Optimization: terminate if nothing left to print. */
	    if (do_print == 0)
		break;
	    continue;
	} else if (rec_type == REC_TYPE_END) {
	    /* Sanity check. */
	    if (state != PC_STATE_ENV)
		msg_warn("%s: out-of-order message end marker",
			 VSTREAM_PATH(fp));
	    /* Optional output. */
	    if (flags & PC_FLAG_PRINT_ENV)
		PRINT_MARKER(flags, fp, offset, rec_type, "MESSAGE FILE END");
	    /* Terminate the state machine. */
	    break;
	} else if (rec_type == REC_TYPE_PTR) {
	    /* Optional output. */
	    /* This record type is exposed only with '-v'. */
	    if (do_print)
		PRINT_RECORD(flags, offset, rec_type, STR(buffer));
	    /* Skip to the pointer's target record. */
	    if (rec_goto(fp, STR(buffer)) == REC_TYPE_ERROR)
		msg_fatal("bad pointer record, or input is not seekable");
	    continue;
	} else if (rec_type == REC_TYPE_SIZE) {
	    /* Optional output (here before we update the state machine). */
	    if (do_print)
		PRINT_RECORD(flags, offset, rec_type, STR(buffer));
	    /* Read the message size/offset for the state machine optimizer. */
	    if (data_size >= 0 || data_offset >= 0) {
		msg_warn("file contains multiple size records");
	    } else {
		if (sscanf(STR(buffer), "%ld %ld", &data_size, &data_offset) != 2
		    || data_offset <= 0 || data_size <= 0)
		    msg_fatal("invalid size record: %.100s", STR(buffer));
		/* Optimization: skip to the message header. */
		if ((flags & PC_FLAG_PRINT_ENV) == 0) {
		    if (vstream_fseek(fp, data_offset, SEEK_SET) < 0)
			msg_fatal("seek error: %m");
		    /* Update the state machine. */
		    state = PC_STATE_HEADER;
		    do_print = (flags & PC_FLAG_PRINT_HEADER);
		}
	    }
	    continue;
	}

	/*
	 * Don't inspect side-effect-free records that aren't printed.
	 */
	if (do_print == 0)
	    continue;
	if (flags & PC_FLAG_PRINT_OFFSET)
	    vstream_printf("%9lu ", (unsigned long) offset);
	if (flags & PC_FLAG_PRINT_RTYPE_DEC)
	    vstream_printf("%3d ", rec_type);
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
	case REC_TYPE_CONT:			/* REC_TYPE_FILT collision */
	    if (state == PC_STATE_ENV)
		vstream_printf("%s: ", rec_type_name(rec_type));
	    else if (msg_verbose)
		vstream_printf("unterminated_text: ");
	    vstream_fwrite(VSTREAM_OUT, STR(buffer), LEN(buffer));
	    if (state == PC_STATE_ENV || msg_verbose
		|| (flags & PC_FLAG_PRINT_OFFSET) != 0) {
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
	    /* This record type is exposed only with '-v'. */
	    vstream_printf("%s: ", rec_type_name(rec_type));
	    vstream_fwrite(VSTREAM_OUT, STR(buffer), LEN(buffer));
	    VSTREAM_PUTCHAR('\n');
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
	    } else {
		vstream_printf("%s: %s=%s\n", rec_type_name(rec_type),
			       attr_name, attr_value);
	    }
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
    }
}

/* usage - explain and terminate */

static NORETURN usage(char *myname)
{
    msg_fatal("usage: %s [-b (body text)] [-c config_dir] [-d (decimal record type)] [-e (envelope records)] [-h (header text)] [-q (access queue)] [-v] [file(s)...]",
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
	MAIL_QUEUE_SAVED,
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
    while ((ch = GETOPT(argc, argv, "bc:dehoqv")) > 0) {
	switch (ch) {
	case 'b':
	    flags |= PC_FLAG_PRINT_BODY;
	    break;
	case 'c':
	    if (setenv(CONF_ENV_PATH, optarg, 1) < 0)
		msg_fatal("out of memory");
	    break;
	case 'd':
	    flags |= PC_FLAG_PRINT_RTYPE_DEC;
	    break;
	case 'e':
	    flags |= PC_FLAG_PRINT_ENV;
	    break;
	case 'h':
	    flags |= PC_FLAG_PRINT_HEADER;
	    break;
	case 'o':
	    flags |= PC_FLAG_PRINT_OFFSET;
	    break;
	case 'q':
	    flags |= PC_FLAG_SEARCH_QUEUE;
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	default:
	    usage(argv[0]);
	}
    }
    if ((flags & PC_MASK_PRINT_ALL) == 0)
	flags |= PC_MASK_PRINT_ALL;

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
    else if (flags & PC_FLAG_SEARCH_QUEUE) {
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
