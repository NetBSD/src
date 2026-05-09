/*	$NetBSD: nbdb_reindexd.c,v 1.2 2026/05/09 18:49:17 christos Exp $	*/

/*++
/* NAME
/*	nbdb_reindexd 8
/* SUMMARY
/*	Postfix non-Berkeley-DB migration
/* SYNOPSIS
/*	\fBnbdb_reindexd\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	\fINOTE: This service should be enabled only temporarily to
/*	generate most of the non-Berkeley-DB indexed files that Postfix
/*	needs. Leaving this service enabled may expose the system to
/*	privilege-escalation attacks.\fR
/*
/*	The nbdb_reindexd(8) server handles requests to generate
/*	a non-Berkeley-DB indexed database file for an existing Berkeley
/*	DB database (example: "hash:/path/to/file" or
/*	"btree:/path/to/file"). It implements the service by running
/*	the postmap(1) or postalias(1) command with appropriate
/*	privileges.
/*
/*	The service reports a success status when the non-Berkeley-DB
/*	indexed file already exists. This can happen when multiple clients
/*	make the same request. When one request is completed successfully,
/*	the service also reports success for the other requests.
/*
/*	This service enforces the following safety policy:
/* .IP \(bu
/*	The legacy Berkeley DB indexed file must exist (file name ends in
/*	".db"). The nbdb_reindexd(8) service will use the owner"s (uid,
/*	gid) of this file, when it runs postmap(1) or postalias(1). It
/*	also uses the (uid,gid) for a number of safety checks as
/*	described next.
/* .IP \(bu
/*	The non-indexed source file must exist (file name without
/*	".db"  suffix). This file is needed as input for postmap(1)
/*	or postalias(1). The file must be owned by "root" or by the
/*	above uid, and must not allow "group" or "other" write access.
/* .IP \(bu
/*	The parent directory must be owned by "root" or by the above uid,
/*	and it must not allow "group" or "other" write access.
/* .IP \(bu
/*	Additionally, the "non_bdb_migration_allow_root_prefixes"
/*	parameter limits the source file directory prefixes that are
/*	allowed when this service needs to run postmap(1) or postalias(1)
/*	with "root" privileges.
/* .IP \(bu
/*	A similar parameter, "non_bdb_migration_allow_user_prefixes",
/*	limits the source file directory prefixes that are allowed when
/*	this service needs to run postmap(1) or postalias(1) as an
/*	unprivileged user.
/* SECURITY
/* .ad
/* .fi
/*	The nbdb_reindexd(8) server is security sensitive.  It accepts
/*	requests only from processes that can access sockets under
/*	$queue_directory/private (i.e., processes that run with "root"
/*	or "mail_owner" (usually, postfix) privileges).
/*
/*	The threat is therefore a corrupted Postfix daemon process that
/*	wants to elevate privileges, by sending requests with crafted
/*	pathnames, and racing against the service by quickly swapping
/*	files or directories, hoping that Postfix will be tricked to
/*	overwrite a sensitive file with attacker-controlled data.
/*
/*	When the service runs postmap(1) or postalias(1) as
/*	"root", such racing attacks should not be possible if
/*	non_bdb_migration_allow_root_prefixes specifies only prefixes
/*	that are already trusted.
/*
/*	This service could block all requests with crafted pathnames,
/*	if given complete information about all lookup tables that are
/*	referenced through Postfix configuration files. Unfortunately
/*	that information was not available at the time that this program
/*	was needed.
/* DIAGNOSTICS
/*	Problems and transactions are logged to syslogd(8) or
/*	postlogd(8). If an attempt to create an index file fails, this
/*	service will attempt to delete the incomplete file.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to main.cf are not picked up automatically, as
/*	nbdb_reindexd(8) processes are long-lived. Use the command
/*	"postfix reload" after a configuration change.
/*
/*	The text below provides only a parameter summary. See
/*	postconf(5) for more details including examples.
/* SERVICE-SPECIFIC CONTROLS
/* .ad
/* .fi
/* .IP "\fBnon_bdb_migration_level (disable)\fR"
/*	The non-Berkeley-DB migration service level.
/* .IP "\fBnon_bdb_migration_allow_root_prefixes (see 'postconf -d non_bdb_migration_allow_root_prefixes' output)\fR"
/*	A list of trusted pathname prefixes that must be matched when
/*	the non-Berkeley-DB migration service (\fBnbdb_reindexd\fR(8)) needs to
/*	run \fBpostmap\fR(1) or \fBpostalias\fR(1) commands with "root" privilege.
/* .IP "\fBnon_bdb_migration_allow_user_prefixes (see 'postconf -d non_bdb_migration_allow_user_prefixes' output)\fR"
/*	A list of trusted pathname prefixes that must be matched when
/*	the non-Berkeley-DB migration service (\fBnbdb_reindexd\fR(8)) needs to
/*	run \fBpostmap\fR(1) or \fBpostalias\fR(1) commands with non-root privilege.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	A prefix that is prepended to the process name in syslog
/*	records, so that, for example, "smtpd" becomes "prefix/smtpd".
/* .IP "\fBservice_name (read-only)\fR"
/*	The master.cf service name of a Postfix daemon process.
/* SEE ALSO
/*	postfix-non-bdb(1), migration management
/*	postconf(5), configuration parameters
/*	postlogd(8), Postfix logging
/*	syslogd(8), system logging
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	NON_BERKELEYDB_README, Non-Berkeley-DB migration guide
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/* .ad
/* .fi
/*	This service was introduced with Postfix version 3.11.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <sys/stat.h>
#include <stdlib.h>

 /*
  * Utility library.
  */
#include <attr.h>
#include <msg.h>
#include <set_eugid.h>
#include <vstream.h>
#include <vstring.h>
#include <wrap_stat.h>
#include <split_at.h>
#include <msg_vstream.h>

 /*
  * Global library.
  */
#include <allowed_prefix.h>
#include <mail_params.h>
#include <mail_proto.h>
#include <mail_task.h>
#include <mail_version.h>
#include <nbdb_util.h>

/* Single-threaded server skeleton. */

#include <mail_server.h>

 /*
  * Application-specific.
  */
#include <nbdb_index_as.h>
#include <nbdb_process.h>
#include <nbdb_reindexd.h>
#include <nbdb_safe.h>
#include <nbdb_sniffer.h>

char   *var_nbdb_allow_root_pfxs;
char   *var_nbdb_allow_user_pfxs;

ALLOWED_PREFIX *parsed_allow_root_pfxs;
ALLOWED_PREFIX *parsed_allow_user_pfxs;

#define STR(x)	vstring_str(x)

/* nbdb_service - CLI or RPC protocol wrapper */

static void nbdb_service(VSTREAM *stream, char *service, char **argv)
{
    static VSTRING *leg_type;
    static VSTRING *source_path;
    static VSTRING *why;
    int     status;

    if (leg_type == 0) {
	leg_type = vstring_alloc(10);
	source_path = vstring_alloc(100);
	why = vstring_alloc(100);
    } else {
	VSTRING_RESET(why);
	VSTRING_TERMINATE(why);
    }

    /*
     * Command-line mode (a.k.a. stand-alone).
     */
    if (stream == VSTREAM_IN) {
	char   *path;

	msg_vstream_init(mail_task(var_procname), VSTREAM_ERR);
	if (*argv == 0 || argv[1] != 0)
	    msg_fatal("stand-alone mode requires one type:/path argument");
	vstring_strcpy(leg_type, *argv);
	if ((path = split_at(STR(leg_type), ':')) == 0 || *path != '/')
	    msg_fatal("stand-alone mode called with bad argument: '%s'", *argv);
	if ((status = nbdb_process(STR(leg_type), path, why)) != 0)
	    msg_warn("stand-alone request to re-index '%s:%s' failed: %s",
		     STR(leg_type), path, STR(why));
	exit(status != 0);
    }

    /*
     * Daemon mode.
     */
    attr_print(stream, ATTR_FLAG_NONE,
	       SEND_ATTR_STR(MAIL_ATTR_PROTO, MAIL_ATTR_PROTO_NBDB_REINDEX),
	       ATTR_TYPE_END);
    if (attr_scan(stream, ATTR_FLAG_STRICT,
		  RECV_ATTR_STR(MAIL_ATTR_NBDB_TYPE, leg_type),
		  RECV_ATTR_STR(MAIL_ATTR_NBDB_PATH, source_path),
		  ATTR_TYPE_END) != 2)
	return;
    status = nbdb_process(STR(leg_type), STR(source_path), why);
    if (status != NBDB_STAT_OK)
	msg_warn("request to index %s:%s failed: %s",
		 STR(leg_type), STR(source_path), STR(why));
    attr_print(stream, ATTR_FLAG_NONE,
	       SEND_ATTR_INT(MAIL_ATTR_STATUS, status),
	       SEND_ATTR_STR(MAIL_ATTR_WHY, STR(why)),
	       ATTR_TYPE_END);
}

/* post_init - late initialization */

static void post_init(char *unused_name, char **unused_argv)
{

    /*
     * Drop root privileges most of the time. Modern UNIX-like systems will
     * prevent a process with mail_owner privs from manipulating this process
     * which still runs with a real uid of root.
     */
    set_eugid(var_owner_uid, var_owner_gid);

    /*
     * Initialize the allowed pathname prefix matcher.
     */
    parsed_allow_root_pfxs = allowed_prefix_create(var_nbdb_allow_root_pfxs);
    parsed_allow_user_pfxs = allowed_prefix_create(var_nbdb_allow_user_pfxs);

    /*
     * TODO(wietse) alternatively, maybe run "proxymap -Sndump-xxxx" commands
     * while privileged, so that the result cannot be manipulated by a
     * compromised non-root process. Unfortunately, proxymap still does find
     * lookup tables in "domain" matchlists of LDAP, SQL, etc. clients.
     */
}

 /*
  * Fingerprint executables and core dumps.
  */
MAIL_VERSION_STAMP_DECLARE;

int     main(int argc, char **argv)
{

    /*
     * var_nbdb_enable and var_nbdb_cust_map are used in many programs,
     * therefore they are managed by mail_params.c.
     */
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_NBDB_ALLOW_ROOT_PFXS, DEF_NBDB_ALLOW_ROOT_PFXS, &var_nbdb_allow_root_pfxs, 0, 0,
	VAR_NBDB_ALLOW_USER_PFXS, DEF_NBDB_ALLOW_USER_PFXS, &var_nbdb_allow_user_pfxs, 0, 0,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    /*
     * Disable sending database reindexing requests.
     */
    nbdb_reindexing_lockout = true;

    single_server_main(argc, argv, nbdb_service,
		       CA_MAIL_SERVER_STR_TABLE(str_table),
		       CA_MAIL_SERVER_POST_INIT(post_init),
		       CA_MAIL_SERVER_PRIVILEGED,
		       CA_MAIL_SERVER_SOLITARY,
		       0);
}
