/*++
/* NAME
/*	mailbox 3
/* SUMMARY
/*	mailbox delivery
/* SYNOPSIS
/*	#include "virtual.h"
/*
/*	int	deliver_mailbox(state, usr_attr, statusp)
/*	LOCAL_STATE state;
/*	USER_ATTR usr_attr;
/*	int	*statusp;
/* DESCRIPTION
/*	deliver_mailbox() delivers to UNIX-style mailbox or to maildir.
/*
/*	A zero result means that the named user was not found.
/*
/*	Arguments:
/* .IP state
/*	The attributes that specify the message, recipient and more.
/* .IP usr_attr
/*	Attributes describing user rights and mailbox location.
/* .IP statusp
/*	Delivery status: see below.
/* DIAGNOSTICS
/*	The message delivery status is non-zero when delivery should be tried
/*	again.
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
#include <errno.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <mymalloc.h>
#include <stringops.h>
#include <set_eugid.h>

/* Global library. */

#include <mail_copy.h>
#include <mbox_open.h>
#include <defer.h>
#include <sent.h>
#include <mail_params.h>

#ifndef EDQUOT
#define EDQUOT EFBIG
#endif

/* Application-specific. */

#include "virtual.h"

#define YES	1
#define NO	0

/* deliver_mailbox_file - deliver to recipient mailbox */

static int deliver_mailbox_file(LOCAL_STATE state, USER_ATTR usr_attr)
{
    char   *myname = "deliver_mailbox_file";
    VSTRING *why;
    MBOX   *mp;
    int     mail_copy_status;
    int     deliver_status;
    int     copy_flags;
    long    end;
    struct stat st;

    /*
     * Make verbose logging easier to understand.
     */
    state.level++;
    if (msg_verbose)
	MSG_LOG_STATE(myname, state);

    /*
     * Initialize. Assume the operation will fail. Set the delivered
     * attribute to reflect the final recipient.
     */
    if (vstream_fseek(state.msg_attr.fp, state.msg_attr.offset, SEEK_SET) < 0)
	msg_fatal("seek message file %s: %m", VSTREAM_PATH(state.msg_attr.fp));
    state.msg_attr.delivered = state.msg_attr.recipient;
    mail_copy_status = MAIL_COPY_STAT_WRITE;
    why = vstring_alloc(100);

    /*
     * Lock the mailbox and open/create the mailbox file.
     * 
     * Write the file as the recipient, so that file quota work.
     */
    copy_flags = MAIL_COPY_MBOX;

    set_eugid(usr_attr.uid, usr_attr.gid);
    mp = mbox_open(usr_attr.mailbox, O_APPEND | O_WRONLY | O_CREAT,
		   S_IRUSR | S_IWUSR, &st, -1, -1,
		   virtual_mbox_lock_mask, why);
    if (mp != 0) {
	if (S_ISREG(st.st_mode) == 0) {
	    vstream_fclose(mp->fp);
	    vstring_sprintf(why, "destination is not a regular file");
	    errno = 0;
	} else {
	    end = vstream_fseek(mp->fp, (off_t) 0, SEEK_END);
	    mail_copy_status = mail_copy(COPY_ATTR(state.msg_attr), mp->fp,
					 copy_flags, "\n", why);
	}
	mbox_release(mp);
    }
    set_eugid(var_owner_uid, var_owner_gid);

    /*
     * As the mail system, bounce, defer delivery, or report success.
     */
    if (mail_copy_status & MAIL_COPY_STAT_CORRUPT) {
	deliver_status = DEL_STAT_DEFER;
    } else if (mail_copy_status != 0) {
	deliver_status = (errno == EDQUOT || errno == EFBIG ?
			  bounce_append : defer_append)
	    (BOUNCE_FLAG_KEEP, BOUNCE_ATTR(state.msg_attr),
	     "mailbox %s: %s", usr_attr.mailbox, vstring_str(why));
    } else {
	deliver_status = sent(SENT_ATTR(state.msg_attr), "mailbox");
    }
    vstring_free(why);
    return (deliver_status);
}

/* deliver_mailbox - deliver to recipient mailbox */

int     deliver_mailbox(LOCAL_STATE state, USER_ATTR usr_attr, int *statusp)
{
    char   *myname = "deliver_mailbox";
    const char *mailbox_res;
    const char *uid_res;
    const char *gid_res;
    long    n;

    /*
     * Make verbose logging easier to understand.
     */
    state.level++;
    if (msg_verbose)
	MSG_LOG_STATE(myname, state);

    /*
     * Sanity check.
     */
    if (*var_virt_mailbox_base != '/')
	msg_fatal("do not specify relative pathname: %s = %s",
		  VAR_VIRT_MAILBOX_BASE, var_virt_mailbox_base);

    /*
     * Look up the mailbox location. Bounce if not found, defer in case of
     * trouble.
     */
    mailbox_res = maps_find(virtual_mailbox_maps, state.msg_attr.user,
			    DICT_FLAG_FIXED);
    if (mailbox_res == 0) {
	if (dict_errno == 0)
	    return (NO);

	*statusp = defer_append(BOUNCE_FLAG_KEEP, BOUNCE_ATTR(state.msg_attr),
				"%s: lookup %s: %m",
			  virtual_mailbox_maps->title, state.msg_attr.user);
	return (YES);
    }
    usr_attr.mailbox = concatenate(var_virt_mailbox_base, "/",
				   mailbox_res, (char *) 0);

#define RETURN(res) { myfree(usr_attr.mailbox); return (res); }

    /*
     * Look up the mailbox owner rights. Defer in case of trouble.
     */
    if ((uid_res = maps_find(virtual_uid_maps, state.msg_attr.user,
			     DICT_FLAG_FIXED)) == 0) {
	*statusp = defer_append(BOUNCE_FLAG_KEEP, BOUNCE_ATTR(state.msg_attr),
				"recipient %s: uid not found in %s",
			      state.msg_attr.user, virtual_uid_maps->title);
	RETURN(YES);
    }
    if ((n = atol(uid_res)) < var_virt_minimum_uid) {
	*statusp = defer_append(BOUNCE_FLAG_KEEP, BOUNCE_ATTR(state.msg_attr),
				"recipient %s: bad uid %s in %s",
		     state.msg_attr.user, uid_res, virtual_uid_maps->title);
	RETURN(YES);
    }
    usr_attr.uid = (uid_t) n;

    /*
     * Look up the mailbox group rights. Defer in case of trouble.
     */
    if ((gid_res = maps_find(virtual_gid_maps, state.msg_attr.user,
			     DICT_FLAG_FIXED)) == 0) {
	*statusp = defer_append(BOUNCE_FLAG_KEEP, BOUNCE_ATTR(state.msg_attr),
				"recipient %s: gid not found in %s",
			      state.msg_attr.user, virtual_gid_maps->title);
	RETURN(YES);
    }
    if ((n = atol(gid_res)) <= 0) {
	*statusp = defer_append(BOUNCE_FLAG_KEEP, BOUNCE_ATTR(state.msg_attr),
				"recipient %s: bad gid %s in %s",
		     state.msg_attr.user, gid_res, virtual_gid_maps->title);
	RETURN(YES);
    }
    usr_attr.gid = (gid_t) n;

    if (msg_verbose)
	msg_info("%s[%d]: set user_attr: %s, uid = %u, gid = %u",
		 myname, state.level, usr_attr.mailbox,
		 (unsigned) usr_attr.uid, (unsigned) usr_attr.gid);

    /*
     * Deliver to mailbox or to external command.
     */
#define LAST_CHAR(s) (s[strlen(s) - 1])

    if (LAST_CHAR(usr_attr.mailbox) == '/')
	*statusp = deliver_maildir(state, usr_attr);
    else
	*statusp = deliver_mailbox_file(state, usr_attr);

    /*
     * Cleanup.
     */
    RETURN(YES);
}
