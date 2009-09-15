/*	$NetBSD: alias.c,v 1.1.1.1.2.2 2009/09/15 06:02:56 snj Exp $	*/

/*++
/* NAME
/*	alias 3
/* SUMMARY
/*	alias data base lookups
/* SYNOPSIS
/*	#include "local.h"
/*
/*	int	deliver_alias(state, usr_attr, name, statusp)
/*	LOCAL_STATE state;
/*	USER_ATTR usr_attr;
/*	char	*name;
/*	int	*statusp;
/* DESCRIPTION
/*	deliver_alias() looks up the expansion of the recipient in
/*	the global alias database and delivers the message to the
/*	listed destinations. The result is zero when no alias was found
/*	or when the message should be delivered to the user instead.
/*
/*	deliver_alias() has wired-in knowledge about a few reserved
/*	recipient names.
/* .IP \(bu
/*	When no alias is found for the local \fIpostmaster\fR or
/*	\fImailer-daemon\fR a warning is issued and the message
/*	is discarded.
/* .IP \(bu
/*	When an alias exists for recipient \fIname\fR, and an alias
/*	exists for \fIowner-name\fR, the sender address is changed
/*	to \fIowner-name\fR, and the owner delivery attribute is
/*	set accordingly. This feature is disabled with
/*	"owner_request_special = no".
/* .PP
/*	Arguments:
/* .IP state
/*	Attributes that specify the message, recipient and more.
/*	Expansion type (alias, include, .forward).
/*	A table with the results from expanding aliases or lists.
/*	A table with delivered-to: addresses taken from the message.
/* .IP usr_attr
/*	User attributes (rights, environment).
/* .IP name
/*	The alias to be looked up.
/* .IP statusp
/*	Delivery status. See below.
/* DIAGNOSTICS
/*	Fatal errors: out of memory. The delivery status is non-zero
/*	when delivery should be tried again.
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
#include <string.h>
#include <fcntl.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <htable.h>
#include <dict.h>
#include <argv.h>
#include <stringops.h>
#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>

/* Global library. */

#include <mail_params.h>
#include <defer.h>
#include <maps.h>
#include <bounce.h>
#include <mypwd.h>
#include <canon_addr.h>
#include <sent.h>
#include <trace.h>
#include <dsn_mask.h>

/* Application-specific. */

#include "local.h"

/* Application-specific. */

#define NO	0
#define YES	1

/* dict_owner - find out alias database owner */

static uid_t dict_owner(char *table)
{
    const char *myname = "dict_owner";
    DICT   *dict;
    struct stat st;

    /*
     * This code sits here for now, but we may want to move it to the library
     * some time.
     */
    if ((dict = dict_handle(table)) == 0)
	msg_panic("%s: can't find dictionary: %s", myname, table);
    if (dict->stat_fd < 0)
	return (0);
    if (fstat(dict->stat_fd, &st) < 0)
	msg_fatal("%s: fstat dictionary %s: %m", myname, table);
    return (st.st_uid);
}

/* deliver_alias - expand alias file entry */

int     deliver_alias(LOCAL_STATE state, USER_ATTR usr_attr,
		              char *name, int *statusp)
{
    const char *myname = "deliver_alias";
    const char *alias_result;
    char   *saved_alias_result;
    char   *owner;
    char  **cpp;
    uid_t   alias_uid;
    struct mypasswd *alias_pwd;
    VSTRING *canon_owner;
    DICT   *dict;
    const char *owner_rhs;		/* owner alias, RHS */
    int     alias_count;
    int     dsn_notify;
    char   *dsn_envid;
    int     dsn_ret;
    const char *dsn_orcpt;

    /*
     * Make verbose logging easier to understand.
     */
    state.level++;
    if (msg_verbose)
	MSG_LOG_STATE(myname, state);

    /*
     * DUPLICATE/LOOP ELIMINATION
     * 
     * We cannot do duplicate elimination here. Sendmail compatibility requires
     * that we allow multiple deliveries to the same alias, even recursively!
     * For example, we must deliver to mailbox any messags that are addressed
     * to the alias of a user that lists that same alias in her own .forward
     * file. Yuck! This is just an example of some really perverse semantics
     * that people will expect Postfix to implement just like sendmail.
     * 
     * We can recognize one special case: when an alias includes its own name,
     * deliver to the user instead, just like sendmail. Otherwise, we just
     * bail out when nesting reaches some unreasonable depth, and blame it on
     * a possible alias loop.
     */
    if (state.msg_attr.exp_from != 0
	&& strcasecmp(state.msg_attr.exp_from, name) == 0)
	return (NO);
    if (state.level > 100) {
	msg_warn("alias database loop for %s", name);
	dsb_simple(state.msg_attr.why, "5.4.6",
		   "alias database loop for %s", name);
	*statusp = bounce_append(BOUNCE_FLAGS(state.request),
				 BOUNCE_ATTR(state.msg_attr));
	return (YES);
    }
    state.msg_attr.exp_from = name;

    /*
     * There are a bunch of roles that we're trying to keep track of.
     * 
     * First, there's the issue of whose rights should be used when delivering
     * to "|command" or to /file/name. With alias databases, the rights are
     * those of who owns the alias, i.e. the database owner. With aliases
     * owned by root, a default user is used instead. When an alias with
     * default rights references an include file owned by an ordinary user,
     * we must use the rights of the include file owner, otherwise the
     * include file owner could take control of the default account.
     * 
     * Secondly, there's the question of who to notify of delivery problems.
     * With aliases that have an owner- alias, the latter is used to set the
     * sender and owner attributes. Otherwise, the owner attribute is reset
     * (the alias is globally visible and could be sent to by anyone).
     */
    for (cpp = alias_maps->argv->argv; *cpp; cpp++) {
	if ((dict = dict_handle(*cpp)) == 0)
	    msg_panic("%s: dictionary not found: %s", myname, *cpp);
	if ((alias_result = dict_get(dict, name)) != 0) {
	    if (msg_verbose)
		msg_info("%s: %s: %s = %s", myname, *cpp, name, alias_result);

	    /*
	     * Don't expand a verify-only request.
	     */
	    if (state.request->flags & DEL_REQ_FLAG_MTA_VRFY) {
		dsb_simple(state.msg_attr.why, "2.0.0",
			   "aliased to %s", alias_result);
		*statusp = sent(BOUNCE_FLAGS(state.request),
				SENT_ATTR(state.msg_attr));
		return (YES);
	    }

	    /*
	     * DELIVERY POLICY
	     * 
	     * Update the expansion type attribute, so we can decide if
	     * deliveries to |command and /file/name are allowed at all.
	     */
	    state.msg_attr.exp_type = EXPAND_TYPE_ALIAS;

	    /*
	     * DELIVERY RIGHTS
	     * 
	     * What rights to use for |command and /file/name deliveries? The
	     * command and file code will use default rights when the alias
	     * database is owned by root, otherwise it will use the rights of
	     * the alias database owner.
	     */
	    if ((alias_uid = dict_owner(*cpp)) == 0) {
		alias_pwd = 0;
		RESET_USER_ATTR(usr_attr, state.level);
	    } else {
		if ((alias_pwd = mypwuid(alias_uid)) == 0) {
		    msg_warn("cannot find alias database owner for %s", *cpp);
		    dsb_simple(state.msg_attr.why, "4.3.0",
			       "cannot find alias database owner");
		    *statusp = defer_append(BOUNCE_FLAGS(state.request),
					    BOUNCE_ATTR(state.msg_attr));
		    return (YES);
		}
		SET_USER_ATTR(usr_attr, alias_pwd, state.level);
	    }

	    /*
	     * WHERE TO REPORT DELIVERY PROBLEMS.
	     * 
	     * Use the owner- alias if one is specified, otherwise reset the
	     * owner attribute and use the include file ownership if we can.
	     * Save the dict_lookup() result before something clobbers it.
	     * 
	     * Don't match aliases that are based on regexps.
	     */
#define OWNER_ASSIGN(own) \
	    (own = (var_ownreq_special == 0 ? 0 : \
	    concatenate("owner-", name, (char *) 0)))

	    saved_alias_result = mystrdup(alias_result);
	    if (OWNER_ASSIGN(owner) != 0
		&& (owner_rhs = maps_find(alias_maps, owner, DICT_FLAG_NONE)) != 0) {
		canon_owner = canon_addr_internal(vstring_alloc(10),
				     var_exp_own_alias ? owner_rhs : owner);
		/* Set envelope sender and owner attribute. */
		SET_OWNER_ATTR(state.msg_attr, STR(canon_owner), state.level);
	    } else {
		canon_owner = 0;
		/* Note: this does not reset the envelope sender. */
		RESET_OWNER_ATTR(state.msg_attr, state.level);
	    }

	    /*
	     * EXTERNAL LOOP CONTROL
	     * 
	     * Set the delivered message attribute to the recipient, so that
	     * this message will list the correct forwarding address.
	     */
	    if (var_frozen_delivered == 0)
		state.msg_attr.delivered = state.msg_attr.rcpt.address;

	    /*
	     * Deliver.
	     */
	    alias_count = 0;
	    if (dict_errno != 0) {
		dsb_simple(state.msg_attr.why, "4.3.0",
			   "alias database unavailable");
		*statusp = defer_append(BOUNCE_FLAGS(state.request),
					BOUNCE_ATTR(state.msg_attr));
	    } else {

		/*
		 * XXX DSN
		 * 
		 * When delivering to a mailing list (i.e. the envelope sender
		 * is replaced) the ENVID, NOTIFY, RET, and ORCPT parameters
		 * which accompany the redistributed message MUST NOT be
		 * derived from those of the original message.
		 * 
		 * When delivering to an alias (i.e. the envelope sender is not
		 * replaced) any ENVID, RET, or ORCPT parameters are
		 * propagated to all forwarding addresses associated with
		 * that alias.  The NOTIFY parameter is propagated to the
		 * forwarding addresses, except that any SUCCESS keyword is
		 * removed.
		 */
#define DSN_SAVE_UPDATE(saved, old, new) do { \
	saved = old; \
	old = new; \
    } while (0)

		DSN_SAVE_UPDATE(dsn_notify, state.msg_attr.rcpt.dsn_notify,
				dsn_notify == DSN_NOTIFY_SUCCESS ?
				DSN_NOTIFY_NEVER :
				dsn_notify & ~DSN_NOTIFY_SUCCESS);
		if (canon_owner != 0) {
		    DSN_SAVE_UPDATE(dsn_envid, state.msg_attr.dsn_envid, "");
		    DSN_SAVE_UPDATE(dsn_ret, state.msg_attr.dsn_ret, 0);
		    DSN_SAVE_UPDATE(dsn_orcpt, state.msg_attr.rcpt.dsn_orcpt, "");
		    state.msg_attr.rcpt.orig_addr = "";
		}
		*statusp =
		    deliver_token_string(state, usr_attr, saved_alias_result,
					 &alias_count);
#if 0
		if (var_ownreq_special
		    && strncmp("owner-", state.msg_attr.sender, 6) != 0
		    && alias_count > 10)
		    msg_warn("mailing list \"%s\" needs an \"owner-%s\" alias",
			     name, name);
#endif
		if (alias_count < 1) {
		    msg_warn("no recipient in alias lookup result for %s", name);
		    dsb_simple(state.msg_attr.why, "4.3.0",
			       "alias database unavailable");
		    *statusp = defer_append(BOUNCE_FLAGS(state.request),
					    BOUNCE_ATTR(state.msg_attr));
		} else {

		    /*
		     * XXX DSN
		     * 
		     * When delivering to a mailing list (i.e. the envelope
		     * sender address is replaced) and NOTIFY=SUCCESS was
		     * specified, report a DSN of "delivered".
		     * 
		     * When delivering to an alias (i.e. the envelope sender
		     * address is not replaced) and NOTIFY=SUCCESS was
		     * specified, report a DSN of "expanded".
		     */
		    if (dsn_notify & DSN_NOTIFY_SUCCESS) {
			state.msg_attr.rcpt.dsn_notify = dsn_notify;
			if (canon_owner != 0) {
			    state.msg_attr.dsn_envid = dsn_envid;
			    state.msg_attr.dsn_ret = dsn_ret;
			    state.msg_attr.rcpt.dsn_orcpt = dsn_orcpt;
			}
			dsb_update(state.msg_attr.why, "2.0.0", canon_owner ?
				   "delivered" : "expanded",
				   DSB_SKIP_RMTA, DSB_SKIP_REPLY,
				   "alias expanded");
			(void) trace_append(BOUNCE_FLAG_NONE,
					    SENT_ATTR(state.msg_attr));
		    }
		}
	    }
	    myfree(saved_alias_result);
	    if (owner)
		myfree(owner);
	    if (canon_owner)
		vstring_free(canon_owner);
	    if (alias_pwd)
		mypwfree(alias_pwd);
	    return (YES);
	}

	/*
	 * If the alias database was inaccessible for some reason, defer
	 * further delivery for the current top-level recipient.
	 */
	if (dict_errno != 0) {
	    dsb_simple(state.msg_attr.why, "4.3.0",
		       "alias database unavailable");
	    *statusp = defer_append(BOUNCE_FLAGS(state.request),
				    BOUNCE_ATTR(state.msg_attr));
	    return (YES);
	} else {
	    if (msg_verbose)
		msg_info("%s: %s: %s not found", myname, *cpp, name);
	}
    }

    /*
     * Try delivery to a local user instead.
     */
    return (NO);
}
