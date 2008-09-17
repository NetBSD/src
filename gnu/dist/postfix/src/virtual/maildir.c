/*++
/* NAME
/*	maildir 3
/* SUMMARY
/*	delivery to maildir
/* SYNOPSIS
/*	#include "virtual.h"
/*
/*	int	deliver_maildir(state, usr_attr)
/*	LOCAL_STATE state;
/*	USER_ATTR usr_attr;
/* DESCRIPTION
/*	deliver_maildir() delivers a message to a qmail-style maildir.
/*
/*	Arguments:
/* .IP state
/*	The attributes that specify the message, recipient and more.
/* .IP usr_attr
/*	Attributes describing user rights and environment information.
/* DIAGNOSTICS
/*	deliver_maildir() always succeeds or it bounces the message.
/* SEE ALSO
/*	bounce(3)
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

#include "sys_defs.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <vstream.h>
#include <vstring.h>
#include <make_dirs.h>
#include <set_eugid.h>
#include <get_hostname.h>
#include <sane_fsops.h>

/* Global library. */

#include <mail_copy.h>
#include <bounce.h>
#include <defer.h>
#include <sent.h>
#include <mail_params.h>
#include <mbox_open.h>
#include <dsn_util.h>

/* Application-specific. */

#include "virtual.h"

/* deliver_maildir - delivery to maildir-style mailbox */

int     deliver_maildir(LOCAL_STATE state, USER_ATTR usr_attr)
{
    const char *myname = "deliver_maildir";
    char   *newdir;
    char   *tmpdir;
    char   *curdir;
    char   *tmpfile;
    char   *newfile;
    DSN_BUF *why = state.msg_attr.why;
    VSTRING *buf;
    VSTREAM *dst;
    int     mail_copy_status;
    int     deliver_status;
    int     copy_flags;
    struct stat st;
    struct timeval starttime;

    GETTIMEOFDAY(&starttime);

    /*
     * Make verbose logging easier to understand.
     */
    state.level++;
    if (msg_verbose)
	MSG_LOG_STATE(myname, state);

    /*
     * Don't deliver trace-only requests.
     */
    if (DEL_REQ_TRACE_ONLY(state.request->flags)) {
	dsb_simple(why, "2.0.0", "delivers to maildir");
	return (sent(BOUNCE_FLAGS(state.request),
		     SENT_ATTR(state.msg_attr)));
    }

    /*
     * Initialize. Assume the operation will fail. Set the delivered
     * attribute to reflect the final recipient.
     */
    if (vstream_fseek(state.msg_attr.fp, state.msg_attr.offset, SEEK_SET) < 0)
	msg_fatal("seek message file %s: %m", VSTREAM_PATH(state.msg_attr.fp));
    state.msg_attr.delivered = state.msg_attr.rcpt.address;
    mail_copy_status = MAIL_COPY_STAT_WRITE;
    buf = vstring_alloc(100);

    copy_flags = MAIL_COPY_TOFILE | MAIL_COPY_RETURN_PATH
	| MAIL_COPY_DELIVERED | MAIL_COPY_ORIG_RCPT;

    newdir = concatenate(usr_attr.mailbox, "new/", (char *) 0);
    tmpdir = concatenate(usr_attr.mailbox, "tmp/", (char *) 0);
    curdir = concatenate(usr_attr.mailbox, "cur/", (char *) 0);

    /*
     * Create and write the file as the recipient, so that file quota work.
     * Create any missing directories on the fly. The file name is chosen
     * according to ftp://koobera.math.uic.edu/www/proto/maildir.html:
     * 
     * "A unique name has three pieces, separated by dots. On the left is the
     * result of time(). On the right is the result of gethostname(). In the
     * middle is something that doesn't repeat within one second on a single
     * host. I fork a new process for each delivery, so I just use the
     * process ID. If you're delivering several messages from one process,
     * use starttime.pid_count.host, where starttime is the time that your
     * process started, and count is the number of messages you've
     * delivered."
     * 
     * Well, that stopped working on fast machines, and on operating systems
     * that randomize process ID values. When creating a file in tmp/ we use
     * the process ID because it still is an exclusive resource. When moving
     * the file to new/ we use the device number and inode number. I do not
     * care if this breaks on a remote AFS file system, because people should
     * know better.
     * 
     * On January 26, 2003, http://cr.yp.to/proto/maildir.html said:
     * 
     * A unique name has three pieces, separated by dots. On the left is the
     * result of time() or the second counter from gettimeofday(). On the
     * right is the result of gethostname(). (To deal with invalid host
     * names, replace / with \057 and : with \072.) In the middle is a
     * delivery identifier, discussed below.
     * 
     * [...]
     * 
     * Modern delivery identifiers are created by concatenating enough of the
     * following strings to guarantee uniqueness:
     * 
     * [...]
     * 
     * In, where n is (in hexadecimal) the UNIX inode number of this file.
     * Unfortunately, inode numbers aren't always available through NFS.
     * 
     * Vn, where n is (in hexadecimal) the UNIX device number of this file.
     * Unfortunately, device numbers aren't always available through NFS.
     * (Device numbers are also not helpful with the standard UNIX
     * filesystem: a maildir has to be within a single UNIX device for link()
     * and rename() to work.)
     * 
     * Mn, where n is (in decimal) the microsecond counter from the same
     * gettimeofday() used for the left part of the unique name.
     * 
     * Pn, where n is (in decimal) the process ID.
     * 
     * [...]
     */
    set_eugid(usr_attr.uid, usr_attr.gid);
    vstring_sprintf(buf, "%lu.P%d.%s",
		 (unsigned long) starttime.tv_sec, var_pid, get_hostname());
    tmpfile = concatenate(tmpdir, STR(buf), (char *) 0);
    newfile = 0;
    if ((dst = vstream_fopen(tmpfile, O_WRONLY | O_CREAT | O_EXCL, 0600)) == 0
	&& (errno != ENOENT
	    || make_dirs(tmpdir, 0700) < 0
	    || (dst = vstream_fopen(tmpfile, O_WRONLY | O_CREAT | O_EXCL, 0600)) == 0)) {
	dsb_simple(why, mbox_dsn(errno, "4.2.0"),
		   "create maildir file %s: %m", tmpfile);
    } else if (fstat(vstream_fileno(dst), &st) < 0) {

	/*
	 * Coverity 200604: file descriptor leak in code that never executes.
	 * Code replaced by msg_fatal(), as it is not worthwhile to continue
	 * after an impossible error condition.
	 */
	msg_fatal("fstat %s: %m", tmpfile);
    } else {
	vstring_sprintf(buf, "%lu.V%lxI%lxM%lu.%s",
			(unsigned long) starttime.tv_sec,
			(unsigned long) st.st_dev,
			(unsigned long) st.st_ino,
			(unsigned long) starttime.tv_usec,
			get_hostname());
	newfile = concatenate(newdir, STR(buf), (char *) 0);
	if ((mail_copy_status = mail_copy(COPY_ATTR(state.msg_attr),
					  dst, copy_flags, "\n",
					  why)) == 0) {
	    if (sane_link(tmpfile, newfile) < 0
		&& (errno != ENOENT
		    || (make_dirs(curdir, 0700), make_dirs(newdir, 0700)) < 0
		    || sane_link(tmpfile, newfile) < 0)) {
		dsb_simple(why, mbox_dsn(errno, "4.2.0"),
			   "create maildir file %s: %m", newfile);
		mail_copy_status = MAIL_COPY_STAT_WRITE;
	    }
	}
	if (unlink(tmpfile) < 0)
	    msg_warn("remove %s: %m", tmpfile);
    }
    set_eugid(var_owner_uid, var_owner_gid);

    /*
     * The maildir location is controlled by the mail administrator. If
     * delivery fails, try again later. We would just bounce when the maildir
     * location possibly under user control.
     */
    if (mail_copy_status & MAIL_COPY_STAT_CORRUPT) {
	deliver_status = DEL_STAT_DEFER;
    } else if (mail_copy_status != 0) {
	if (errno == EACCES) {
	    msg_warn("maildir access problem for UID/GID=%lu/%lu: %s",
		     (long) usr_attr.uid, (long) usr_attr.gid,
		     STR(why->reason));
	    msg_warn("perhaps you need to create the maildirs in advance");
	}
	vstring_sprintf_prepend(why->reason, "maildir delivery failed: ");
	deliver_status =
	    (STR(why->status)[0] == '4' ?
	     defer_append : bounce_append)
	    (BOUNCE_FLAGS(state.request),
	     BOUNCE_ATTR(state.msg_attr));
    } else {
	dsb_simple(why, "2.0.0", "delivered to maildir");
	deliver_status = sent(BOUNCE_FLAGS(state.request),
			      SENT_ATTR(state.msg_attr));
    }
    vstring_free(buf);
    myfree(newdir);
    myfree(tmpdir);
    myfree(curdir);
    myfree(tmpfile);
    if (newfile)
	myfree(newfile);
    return (deliver_status);
}
