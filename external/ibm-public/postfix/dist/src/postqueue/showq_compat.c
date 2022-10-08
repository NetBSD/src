/*	$NetBSD: showq_compat.c,v 1.4 2022/10/08 16:12:48 christos Exp $	*/

/*++
/* NAME
/*	showq_compat 8
/* SUMMARY
/*	Sendmail mailq compatibility adapter
/* SYNOPSIS
/*	void	showq_compat(
/*	VSTREAM	*showq)
/* DESCRIPTION
/*	This function converts a record stream from the showq(8)
/*	daemon to of an approximation of Sendmail mailq command
/*	output.
/* DIAGNOSTICS
/*	Fatal errors: out of memory, malformed showq(8) daemon output.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sysexits.h>
#include <errno.h>

/* Utility library. */

#include <vstring.h>
#include <vstream.h>
#include <stringops.h>
#include <mymalloc.h>
#include <msg.h>

/* Global library. */

#include <mail_proto.h>
#include <mail_queue.h>
#include <mail_date.h>
#include <mail_params.h>

/* Application-specific. */

#include <postqueue.h>

 /*
  * The enable_long_queue_ids parameter determines the output format.
  * 
  * The historical output format for short queue IDs (inode number and time in
  * microseconds modulo 1) is not suitable for large inode numbers, but we
  * won't change it to avoid breaking compatibility with programs that parse
  * this output.
  */
#define S_STRING_FORMAT	"%-11s %7s %-20s %s\n"
#define S_SENDER_FORMAT	"%-11s %7ld %20.20s %s\n"
#define S_HEADINGS	"-Queue ID-", "--Size--", \
			    "----Arrival Time----", "-Sender/Recipient-------"

#define L_STRING_FORMAT	"%-17s %8s %-19s %s\n"
#define L_SENDER_FORMAT	"%-17s %8ld %19.19s %s\n"
#define L_HEADINGS	"----Queue ID-----", "--Size--", \
			    "---Arrival Time----", "--Sender/Recipient------"

#define STR(x)	vstring_str(x)

/* showq_message - report status for one message */

static unsigned long showq_message(VSTREAM *showq_stream)
{
    static VSTRING *queue_name = 0;
    static VSTRING *queue_id = 0;
    static VSTRING *id_status = 0;
    static VSTRING *addr = 0;
    static VSTRING *why = 0;
    long    arrival_time;
    long    message_size;
    char   *saved_reason = mystrdup("");
    const char *show_reason;
    int     padding;
    int     showq_status;
    time_t  time_t_arrival_time;
    int     forced_expire;

    /*
     * One-time initialization.
     */
    if (queue_name == 0) {
	queue_name = vstring_alloc(100);
	queue_id = vstring_alloc(100);
	id_status = vstring_alloc(100);
	addr = vstring_alloc(100);
	why = vstring_alloc(100);
    }

    /*
     * Read the message properties and sender address.
     */
    if (attr_scan(showq_stream, ATTR_FLAG_MORE | ATTR_FLAG_STRICT
		  | ATTR_FLAG_PRINTABLE,
		  RECV_ATTR_STR(MAIL_ATTR_QUEUE, queue_name),
		  RECV_ATTR_STR(MAIL_ATTR_QUEUEID, queue_id),
		  RECV_ATTR_LONG(MAIL_ATTR_TIME, &arrival_time),
		  RECV_ATTR_LONG(MAIL_ATTR_SIZE, &message_size),
		  RECV_ATTR_INT(MAIL_ATTR_FORCED_EXPIRE, &forced_expire),
		  RECV_ATTR_STR(MAIL_ATTR_SENDER, addr),
		  ATTR_TYPE_END) != 6)
	msg_fatal_status(EX_SOFTWARE, "malformed showq server response");

    /*
     * Decorate queue file names in specific states, then print the result
     * left-aligned, followed by other status info and the sender address
     * which is already in externalized RFC 5321 form.
     */
    vstring_strcpy(id_status, STR(queue_id));
    if (strcmp(STR(queue_name), MAIL_QUEUE_ACTIVE) == 0)
	vstring_strcat(id_status, "*");
    else if (strcmp(STR(queue_name), MAIL_QUEUE_HOLD) == 0)
	vstring_strcat(id_status, "!");
    if (forced_expire)
	vstring_strcat(id_status, "#");
    time_t_arrival_time = arrival_time;
    vstream_printf(var_long_queue_ids ?
		   L_SENDER_FORMAT : S_SENDER_FORMAT, STR(id_status),
		   message_size, asctime(localtime(&time_t_arrival_time)),
		   STR(addr));

    /*
     * Read zero or more (recipient, reason) pair(s) until attr_scan_more()
     * consumes a terminator. If the showq daemon messes up, don't try to
     * resynchronize.
     */
    while ((showq_status = attr_scan_more(showq_stream)) > 0) {
	if (attr_scan(showq_stream, ATTR_FLAG_MORE | ATTR_FLAG_STRICT
		      | ATTR_FLAG_PRINTABLE,
		      RECV_ATTR_STR(MAIL_ATTR_RECIP, addr),
		      RECV_ATTR_STR(MAIL_ATTR_WHY, why),
		      ATTR_TYPE_END) != 2)
	    msg_fatal_status(EX_SOFTWARE, "malformed showq server response");

	/*
	 * Don't output a "(reason)" line when no recipient has a reason, or
	 * when the previous recipient has the same (non)reason as the
	 * current recipient. Do output a "(reason unavailable)" when the
	 * previous recipient has a reason, and the current recipient has
	 * none.
	 */
	if (strcmp(saved_reason, STR(why)) != 0) {
	    myfree(saved_reason);
	    saved_reason = mystrdup(STR(why));
	    show_reason = *saved_reason ? saved_reason : "reason unavailable";
	    if ((padding = 76 - (int) strlen(show_reason)) < 0)
		padding = 0;
	    vstream_printf("%*s(%s)\n", padding, "", show_reason);
	}
	vstream_printf(var_long_queue_ids ?
		       L_STRING_FORMAT : S_STRING_FORMAT,
		       "", "", "", STR(addr));
    }
    if (showq_status < 0)
	msg_fatal_status(EX_SOFTWARE, "malformed showq server response");
    myfree(saved_reason);
    return (message_size);
}

/* showq_compat - legacy mailq-style output adapter */

void    showq_compat(VSTREAM *showq_stream)
{
    unsigned long file_count = 0;
    unsigned long queue_size = 0;
    int     showq_status;

    /*
     * Process zero or more queue file objects until attr_scan_more()
     * consumes a terminator.
     */
    while ((showq_status = attr_scan_more(showq_stream)) > 0) {
	if (file_count > 0) {
	    vstream_printf("\n");
	} else if (var_long_queue_ids) {
	    vstream_printf(L_STRING_FORMAT, L_HEADINGS);
	} else {
	    vstream_printf(S_STRING_FORMAT, S_HEADINGS);
	}
	queue_size += showq_message(showq_stream);
	file_count++;
	if (vstream_fflush(VSTREAM_OUT)) {
	    if (errno != EPIPE)
		msg_fatal_status(EX_IOERR, "output write error: %m");
	    return;
	}
    }
    if (showq_status < 0)
	msg_fatal_status(EX_SOFTWARE, "malformed showq server response");

    /*
     * Print the queue summary.
     */
    if (file_count == 0)
	vstream_printf("Mail queue is empty\n");
    else {
	vstream_printf("\n-- %lu Kbytes in %lu Request%s.\n",
		       queue_size / 1024, file_count,
		       file_count == 1 ? "" : "s");
    }
    if (vstream_fflush(VSTREAM_OUT) && errno != EPIPE)
	msg_fatal_status(EX_IOERR, "output write error: %m");
}
