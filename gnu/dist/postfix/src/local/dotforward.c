/*++
/* NAME
/*	dotforward 3
/* SUMMARY
/*	$HOME/.forward file expansion
/* SYNOPSIS
/*	#include "local.h"
/*
/*	int	deliver_dotforward(state, usr_attr, statusp)
/*	LOCAL_STATE state;
/*	USER_ATTR usr_attr;
/*	int	*statusp;
/* DESCRIPTION
/*	deliver_dotforward() delivers a message to the destinations
/*	listed in a recipient's .forward file(s) as specified through
/*	the forward_path configuration parameter.  The result is
/*	zero when no acceptable .forward file was found, or when
/*	a recipient is listed in her own .forward file. Expansions
/*	are scrutinized with the forward_expansion_filter parameter.
/*
/*	Arguments:
/* .IP state
/*	Message delivery attributes (sender, recipient etc.).
/*	Attributes describing alias, include or forward expansion.
/*	A table with the results from expanding aliases or lists.
/*	A table with delivered-to: addresses taken from the message.
/* .IP usr_attr
/*	Attributes describing user rights and environment.
/* .IP statusp
/*	Message delivery status. See below.
/* DIAGNOSTICS
/*	Fatal errors: out of memory. Warnings: bad $HOME/.forward
/*	file type, permissions or ownership.  The message delivery
/*	status is non-zero when delivery should be tried again.
/* SEE ALSO
/*	include(3) include file processor.
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
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#ifdef USE_PATHS_H
#include <paths.h>
#endif
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <htable.h>
#include <open_as.h>
#include <lstat_as.h>
#include <iostuff.h>
#include <stringops.h>
#include <mymalloc.h>
#include <mac_expand.h>

/* Global library. */

#include <mypwd.h>
#include <bounce.h>
#include <been_here.h>
#include <mail_params.h>
#include <mail_conf.h>
#include <ext_prop.h>
#include <sent.h>
#include <dsn_mask.h>
#include <trace.h>

/* Application-specific. */

#include "local.h"

#define NO	0
#define YES	1

/* deliver_dotforward - expand contents of .forward file */

int     deliver_dotforward(LOCAL_STATE state, USER_ATTR usr_attr, int *statusp)
{
    const char *myname = "deliver_dotforward";
    struct stat st;
    VSTRING *path;
    struct mypasswd *mypwd;
    int     fd;
    VSTREAM *fp;
    int     status;
    int     forward_found = NO;
    int     lookup_status;
    int     addr_count;
    char   *saved_forward_path;
    char   *lhs;
    char   *next;
    int     expand_status;
    int     saved_notify;

    /*
     * Make verbose logging easier to understand.
     */
    state.level++;
    if (msg_verbose)
	MSG_LOG_STATE(myname, state);

    /*
     * Skip this module if per-user forwarding is disabled.
     */
    if (*var_forward_path == 0)
	return (NO);

    /*
     * Skip non-existing users. The mailbox delivery routine will catch the
     * error.
     */
    if ((mypwd = mypwnam(state.msg_attr.user)) == 0)
	return (NO);

    /*
     * From here on no early returns or we have a memory leak.
     */

    /*
     * EXTERNAL LOOP CONTROL
     * 
     * Set the delivered message attribute to the recipient, so that this
     * message will list the correct forwarding address.
     */
    if (var_frozen_delivered == 0)
	state.msg_attr.delivered = state.msg_attr.rcpt.address;

    /*
     * DELIVERY RIGHTS
     * 
     * Do not inherit rights from the .forward file owner. Instead, use the
     * recipient's rights, and insist that the .forward file is owned by the
     * recipient. This is a small but significant difference. Use the
     * recipient's rights for all /file and |command deliveries, and pass on
     * these rights to command/file destinations in included files. When
     * these are the rights of root, the /file and |command delivery routines
     * will use unprivileged default rights instead. Better safe than sorry.
     */
    SET_USER_ATTR(usr_attr, mypwd, state.level);

    /*
     * DELIVERY POLICY
     * 
     * Update the expansion type attribute so that we can decide if deliveries
     * to |command and /file/name are allowed at all.
     */
    state.msg_attr.exp_type = EXPAND_TYPE_FWD;

    /*
     * WHERE TO REPORT DELIVERY PROBLEMS
     * 
     * Set the owner attribute so that 1) include files won't set the sender to
     * be this user and 2) mail forwarded to other local users will be
     * resubmitted as a new queue file.
     */
    state.msg_attr.owner = state.msg_attr.user;

    /*
     * Search the forward_path for an existing forward file.
     * 
     * If unmatched extensions should never be propagated, or if a forward file
     * name includes the address extension, don't propagate the extension to
     * the recipient addresses.
     */
    status = 0;
    path = vstring_alloc(100);
    saved_forward_path = mystrdup(var_forward_path);
    next = saved_forward_path;
    lookup_status = -1;

    while ((lhs = mystrtok(&next, ", \t\r\n")) != 0) {
	expand_status = local_expand(path, lhs, &state, &usr_attr,
				     var_fwd_exp_filter);
	if ((expand_status & (MAC_PARSE_ERROR | MAC_PARSE_UNDEF)) == 0) {
	    lookup_status =
		lstat_as(STR(path), &st, usr_attr.uid, usr_attr.gid);
	    if (msg_verbose)
		msg_info("%s: path %s expand_status %d look_status %d", myname,
			 STR(path), expand_status, lookup_status);
	    if (lookup_status >= 0) {
		if ((expand_status & LOCAL_EXP_EXTENSION_MATCHED) != 0
		    || (local_ext_prop_mask & EXT_PROP_FORWARD) == 0)
		    state.msg_attr.unmatched = 0;
		break;
	    }
	}
    }

    /*
     * Process the forward file.
     * 
     * Assume that usernames do not have file system meta characters. Open the
     * .forward file as the user. Ignore files that aren't regular files,
     * files that are owned by the wrong user, or files that have world write
     * permission enabled.
     * 
     * DUPLICATE/LOOP ELIMINATION
     * 
     * If this user includes (an alias of) herself in her own .forward file,
     * deliver to the user instead.
     */
    if (lookup_status >= 0) {

	/*
	 * Don't expand a verify-only request.
	 */
	if (state.request->flags & DEL_REQ_FLAG_MTA_VRFY) {
	    dsb_simple(state.msg_attr.why, "2.0.0",
		       "forward via file: %s", STR(path));
	    *statusp = sent(BOUNCE_FLAGS(state.request),
			    SENT_ATTR(state.msg_attr));
	    forward_found = YES;
	} else if (been_here(state.dup_filter, "forward %s", STR(path)) == 0) {
	    state.msg_attr.exp_from = state.msg_attr.local;
	    if (S_ISREG(st.st_mode) == 0) {
		msg_warn("file %s is not a regular file", STR(path));
	    } else if (st.st_uid != 0 && st.st_uid != usr_attr.uid) {
		msg_warn("file %s has bad owner uid %ld",
			 STR(path), (long) st.st_uid);
	    } else if (st.st_mode & 002) {
		msg_warn("file %s is world writable", STR(path));
	    } else if ((fd = open_as(STR(path), O_RDONLY, 0, usr_attr.uid, usr_attr.gid)) < 0) {
		msg_warn("cannot open file %s: %m", STR(path));
	    } else {

		/*
		 * XXX DSN. When delivering to an alias (i.e. the envelope
		 * sender address is not replaced) any ENVID, RET, or ORCPT
		 * parameters are propagated to all forwarding addresses
		 * associated with that alias.  The NOTIFY parameter is
		 * propagated to the forwarding addresses, except that any
		 * SUCCESS keyword is removed.
		 */
		close_on_exec(fd, CLOSE_ON_EXEC);
		addr_count = 0;
		fp = vstream_fdopen(fd, O_RDONLY);
		saved_notify = state.msg_attr.rcpt.dsn_notify;
		state.msg_attr.rcpt.dsn_notify =
		    (saved_notify == DSN_NOTIFY_SUCCESS ?
		     DSN_NOTIFY_NEVER : saved_notify & ~DSN_NOTIFY_SUCCESS);
		status = deliver_token_stream(state, usr_attr, fp, &addr_count);
		if (vstream_fclose(fp))
		    msg_warn("close file %s: %m", STR(path));
		if (addr_count > 0) {
		    forward_found = YES;
		    been_here(state.dup_filter, "forward-done %s", STR(path));

		    /*
		     * XXX DSN. When delivering to an alias (i.e. the
		     * envelope sender address is not replaced) and the
		     * original NOTIFY parameter for the alias contained the
		     * SUCCESS keyword, an "expanded" DSN is issued for the
		     * alias.
		     */
		    if (status == 0 && (saved_notify & DSN_NOTIFY_SUCCESS)) {
			state.msg_attr.rcpt.dsn_notify = saved_notify;
			dsb_update(state.msg_attr.why, "2.0.0", "expanded",
				   DSB_SKIP_RMTA, DSB_SKIP_REPLY,
				   "alias expanded");
			(void) trace_append(BOUNCE_FLAG_NONE,
					    SENT_ATTR(state.msg_attr));
		    }
		}
	    }
	} else if (been_here_check(state.dup_filter, "forward-done %s", STR(path)) != 0)
	    forward_found = YES;		/* else we're recursive */
    }

    /*
     * Clean up.
     */
    vstring_free(path);
    myfree(saved_forward_path);
    mypwfree(mypwd);

    *statusp = status;
    return (forward_found);
}
