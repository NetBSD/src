/*++
/* NAME
/*	mail_params 3
/* SUMMARY
/*	global mail configuration parameters
/* SYNOPSIS
/*	#include <mail_params.h>
/*
/*	char	*var_myhostname;
/*	char	*var_mydomain;
/*	char	*var_myorigin;
/*	char	*var_mydest;
/*	char	*var_relayhost;
/*	char	*var_transit_origin;
/*	char	*var_transit_dest;
/*	char	*var_mail_name;
/*	char	*var_mail_owner;
/*	uid_t	var_owner_uid;
/*	gid_t	var_owner_gid;
/*	char	*var_default_privs;
/*	uid_t	var_default_uid;
/*	gid_t	var_default_gid;
/*	char	*var_config_dir;
/*	char	*var_program_dir;
/*	char	*var_daemon_dir;
/*	char	*var_command_dir;
/*	char	*var_queue_dir;
/*	int	var_use_limit;
/*	int	var_idle_limit;
/*	int	var_bundle_rcpt;
/*	char	*var_procname;
/*	int	var_pid;
/*	int	var_ipc_timeout;
/*	char	*var_pid_dir;
/*	int	var_dont_remove;
/*	char	*var_inet_interfaces;
/*	char	*var_mynetworks;
/*	char	*var_double_bounce_sender;
/*	int	var_line_limit;
/*	char	*var_alias_db_map;
/*	int	var_message_limit;
/*	char	*var_mail_version;
/*	int	var_ipc_idle_limit;
/*	char	*var_db_type;
/*	char	*var_hash_queue_names;
/*	int	var_hash_queue_depth;
/*	int	var_trigger_timeout;
/*	char	*var_rcpt_delim;
/*	int	var_fork_tries;
/*	int	var_fork_delay;
/*	int	var_flock_tries;
/*	int	var_flock_delay;
/*	int	var_flock_stale;
/*	int	var_disable_dns;
/*	int	var_soft_bounce;
/*	time_t	var_starttime;
/*	int	var_ownreq_special;
/*	int	var_daemon_timeout;
/*	char	*var_syslog_facility;
/*	char	*var_relay_domains;
/*	char	*var_fflush_domains;
/*	char	*var_def_transport;
/*	char	*var_mynetworks_style;
/*
/*	char	*var_import_environ;
/*	char	*var_export_environ;
/*
/*	void	mail_params_init()
/* DESCRIPTION
/*	This module (actually the associated include file) define the names
/*	and defaults of all mail configuration parameters.
/*
/*	mail_params_init() initializes the built-in parameters listed above.
/*	These parameters are relied upon by library routines, so they are
/*	initialized globally so as to avoid hard-to-find errors due to
/*	missing initialization. This routine must be called early, at
/*	least before entering a chroot jail.
/* DIAGNOSTICS
/*	Fatal errors: out of memory; null system or domain name.
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
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <time.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <msg_syslog.h>
#include <get_hostname.h>
#include <valid_hostname.h>
#include <stringops.h>

/* Global library. */

#include "mynetworks.h"
#include "mail_conf.h"
#include "mail_version.h"
#include "mail_proto.h"
#include "mail_params.h"

 /*
  * Special configuration variables.
  */
char   *var_myhostname;
char   *var_mydomain;
char   *var_myorigin;
char   *var_mydest;
char   *var_relayhost;
char   *var_transit_origin;
char   *var_transit_dest;
char   *var_mail_name;
char   *var_mail_owner;
uid_t   var_owner_uid;
gid_t   var_owner_gid;
char   *var_default_privs;
uid_t   var_default_uid;
gid_t   var_default_gid;
char   *var_config_dir;
char   *var_program_dir;
char   *var_daemon_dir;
char   *var_command_dir;
char   *var_queue_dir;
int     var_use_limit;
int     var_idle_limit;
int     var_bundle_rcpt;
char   *var_procname;
int     var_pid;
int     var_ipc_timeout;
char   *var_pid_dir;
int     var_dont_remove;
char   *var_inet_interfaces;
char   *var_mynetworks;
char   *var_double_bounce_sender;
int     var_line_limit;
char   *var_alias_db_map;
int     var_message_limit;
char   *var_mail_version;
int     var_ipc_idle_limit;
char   *var_db_type;
char   *var_hash_queue_names;
int     var_hash_queue_depth;
int     var_trigger_timeout;
char   *var_rcpt_delim;
int     var_fork_tries;
int     var_fork_delay;
int     var_flock_tries;
int     var_flock_delay;
int     var_flock_stale;
int     var_disable_dns;
int     var_soft_bounce;
time_t  var_starttime;
int     var_ownreq_special;
int     var_daemon_timeout;
char   *var_syslog_facility;
char   *var_relay_domains;
char   *var_fflush_domains;
char   *var_def_transport;
char   *var_mynetworks_style;

char   *var_import_environ;
char   *var_export_environ;

/* check_myhostname - lookup hostname and validate */

static const char *check_myhostname(void)
{
    static const char *name;
    const char *dot;
    const char *domain;

    /*
     * Use cached result.
     */
    if (name)
	return (name);

    /*
     * If the local machine name is not in FQDN form, try to append the
     * contents of $mydomain.
     */
    name = get_hostname();
    if ((dot = strchr(name, '.')) == 0) {
	if ((domain = mail_conf_lookup_eval(VAR_MYDOMAIN)) == 0)
	    msg_fatal("My hostname %s is not a fully qualified name - set %s or %s in %s/main.cf",
		      name, VAR_MYHOSTNAME, VAR_MYDOMAIN, var_config_dir);
	name = concatenate(name, ".", domain, (char *) 0);
    }
    return (name);
}

/* check_mydomainname - lookup domain name and validate */

static const char *check_mydomainname(void)
{
    char   *dot;

    /*
     * Use the hostname when it is not a FQDN ("foo"), or when the hostname
     * actually is a domain name ("foo.com").
     */
    if ((dot = strchr(var_myhostname, '.')) == 0 || strchr(dot + 1, '.') == 0)
	return (var_myhostname);
    return (dot + 1);
}

/* check_default_privs - lookup default user attributes and validate */

static void check_default_privs(void)
{
    struct passwd *pwd;

    if ((pwd = getpwnam(var_default_privs)) == 0)
	msg_fatal("unknown %s configuration parameter value: %s",
		  VAR_DEFAULT_PRIVS, var_default_privs);
    if ((var_default_uid = pwd->pw_uid) == 0)
	msg_fatal("%s: %s: privileged user is not allowed",
		  VAR_DEFAULT_PRIVS, var_default_privs);
    if ((var_default_gid = pwd->pw_gid) == 0)
	msg_fatal("%s: %s: privileged group is not allowed",
		  VAR_DEFAULT_PRIVS, var_default_privs);
}

/* check_mail_owner - lookup owner user attributes and validate */

static void check_mail_owner(void)
{
    struct passwd *pwd;

    if ((pwd = getpwnam(var_mail_owner)) == 0)
	msg_fatal("unknown %s configuration parameter value: %s",
		  VAR_MAIL_OWNER, var_mail_owner);
    if ((var_owner_uid = pwd->pw_uid) == 0)
	msg_fatal("%s: %s: privileged user is not allowed",
		  VAR_MAIL_OWNER, var_mail_owner);
    if ((var_owner_gid = pwd->pw_gid) == 0)
	msg_fatal("%s: %s: privileged group is not allowed",
		  VAR_DEFAULT_PRIVS, var_mail_owner);
}

/* mail_params_init - configure built-in parameters */

void    mail_params_init()
{
    static CONFIG_STR_TABLE first_str_defaults[] = {
	VAR_SYSLOG_FACILITY, DEF_SYSLOG_FACILITY, &var_syslog_facility, 1, 0,
	0,
    };
    static CONFIG_STR_FN_TABLE function_str_defaults[] = {
	VAR_MYHOSTNAME, check_myhostname, &var_myhostname, 1, 0,
	VAR_MYDOMAIN, check_mydomainname, &var_mydomain, 1, 0,
	0,
    };
    static CONFIG_STR_TABLE other_str_defaults[] = {
	VAR_MAIL_NAME, DEF_MAIL_NAME, &var_mail_name, 1, 0,
	VAR_MAIL_OWNER, DEF_MAIL_OWNER, &var_mail_owner, 1, 0,
	VAR_MYDEST, DEF_MYDEST, &var_mydest, 0, 0,
	VAR_MYORIGIN, DEF_MYORIGIN, &var_myorigin, 1, 0,
	VAR_RELAYHOST, DEF_RELAYHOST, &var_relayhost, 0, 0,
	VAR_PROGRAM_DIR, DEF_PROGRAM_DIR, &var_program_dir, 1, 0,
	VAR_DAEMON_DIR, DEF_DAEMON_DIR, &var_daemon_dir, 1, 0,
	VAR_COMMAND_DIR, DEF_COMMAND_DIR, &var_command_dir, 1, 0,
	VAR_QUEUE_DIR, DEF_QUEUE_DIR, &var_queue_dir, 1, 0,
	VAR_PID_DIR, DEF_PID_DIR, &var_pid_dir, 1, 0,
	VAR_INET_INTERFACES, DEF_INET_INTERFACES, &var_inet_interfaces, 1, 0,
	VAR_DOUBLE_BOUNCE, DEF_DOUBLE_BOUNCE, &var_double_bounce_sender, 1, 0,
	VAR_DEFAULT_PRIVS, DEF_DEFAULT_PRIVS, &var_default_privs, 1, 0,
	VAR_ALIAS_DB_MAP, DEF_ALIAS_DB_MAP, &var_alias_db_map, 0, 0,
	VAR_MAIL_VERSION, DEF_MAIL_VERSION, &var_mail_version, 1, 0,
	VAR_DB_TYPE, DEF_DB_TYPE, &var_db_type, 1, 0,
	VAR_HASH_QUEUE_NAMES, DEF_HASH_QUEUE_NAMES, &var_hash_queue_names, 1, 0,
	VAR_RCPT_DELIM, DEF_RCPT_DELIM, &var_rcpt_delim, 0, 1,
	VAR_RELAY_DOMAINS, DEF_RELAY_DOMAINS, &var_relay_domains, 0, 0,
	VAR_FFLUSH_DOMAINS, DEF_FFLUSH_DOMAINS, &var_fflush_domains, 0, 0,
	VAR_EXPORT_ENVIRON, DEF_EXPORT_ENVIRON, &var_export_environ, 0, 0,
	VAR_IMPORT_ENVIRON, DEF_IMPORT_ENVIRON, &var_import_environ, 0, 0,
	VAR_DEF_TRANSPORT, DEF_DEF_TRANSPORT, &var_def_transport, 0, 0,
	VAR_MYNETWORKS_STYLE, DEF_MYNETWORKS_STYLE, &var_mynetworks_style, 1, 0,
	0,
    };
    static CONFIG_STR_FN_TABLE function_str_defaults_2[] = {
	VAR_MYNETWORKS, mynetworks, &var_mynetworks, 1, 0,
	0,
    };
    static CONFIG_INT_TABLE other_int_defaults[] = {
	VAR_MAX_USE, DEF_MAX_USE, &var_use_limit, 1, 0,
	VAR_DONT_REMOVE, DEF_DONT_REMOVE, &var_dont_remove, 0, 0,
	VAR_LINE_LIMIT, DEF_LINE_LIMIT, &var_line_limit, 512, 0,
	VAR_MESSAGE_LIMIT, DEF_MESSAGE_LIMIT, &var_message_limit, 0, 0,
	VAR_HASH_QUEUE_DEPTH, DEF_HASH_QUEUE_DEPTH, &var_hash_queue_depth, 1, 0,
	VAR_FORK_TRIES, DEF_FORK_TRIES, &var_fork_tries, 1, 0,
	VAR_FLOCK_TRIES, DEF_FLOCK_TRIES, &var_flock_tries, 1, 0,
	0,
    };
    static CONFIG_TIME_TABLE time_defaults[] = {
	VAR_MAX_IDLE, DEF_MAX_IDLE, &var_idle_limit, 1, 0,
	VAR_IPC_TIMEOUT, DEF_IPC_TIMEOUT, &var_ipc_timeout, 1, 0,
	VAR_IPC_IDLE, DEF_IPC_IDLE, &var_ipc_idle_limit, 1, 0,
	VAR_TRIGGER_TIMEOUT, DEF_TRIGGER_TIMEOUT, &var_trigger_timeout, 1, 0,
	VAR_FORK_DELAY, DEF_FORK_DELAY, &var_fork_delay, 1, 0,
	VAR_FLOCK_DELAY, DEF_FLOCK_DELAY, &var_flock_delay, 1, 0,
	VAR_FLOCK_STALE, DEF_FLOCK_STALE, &var_flock_stale, 1, 0,
	VAR_DAEMON_TIMEOUT, DEF_DAEMON_TIMEOUT, &var_daemon_timeout, 1, 0,
	0,
    };
    static CONFIG_BOOL_TABLE bool_defaults[] = {
	VAR_DISABLE_DNS, DEF_DISABLE_DNS, &var_disable_dns,
	VAR_SOFT_BOUNCE, DEF_SOFT_BOUNCE, &var_soft_bounce,
	VAR_OWNREQ_SPECIAL, DEF_OWNREQ_SPECIAL, &var_ownreq_special,
	0,
    };

    /*
     * Extract syslog_facility early, so that from here on all errors are
     * logged with the proper facility.
     */
    get_mail_conf_str_table(first_str_defaults);

    if (!msg_syslog_facility(var_syslog_facility))
	msg_fatal("unknown %s configuration parameter value: %s",
		  VAR_SYSLOG_FACILITY, var_syslog_facility);

    /*
     * Variables whose defaults are determined at runtime. Some sites use
     * short hostnames in the host table; some sites name their system after
     * the domain.
     */
    get_mail_conf_str_fn_table(function_str_defaults);
    if (!valid_hostname(var_myhostname, DO_GRIPE)
	|| !valid_hostname(var_mydomain, DO_GRIPE))
	msg_fatal("main.cf configuration error: bad %s or %s parameter value",
		  VAR_MYHOSTNAME, VAR_MYDOMAIN);

    /*
     * Variables that are needed by almost every program.
     */
    get_mail_conf_str_table(other_str_defaults);
    get_mail_conf_int_table(other_int_defaults);
    get_mail_conf_bool_table(bool_defaults);
    get_mail_conf_time_table(time_defaults);
    check_default_privs();
    check_mail_owner();

    /*
     * Variables whose defaults are determined at runtime, after other
     * variables have been set. This dependency is admittedly a bit tricky.
     * XXX Perhaps we should just register variables, and let the evaluator
     * figure out in what order to evaluate things.
     */
    get_mail_conf_str_fn_table(function_str_defaults_2);

    /*
     * The PID variable cannot be set from the configuration file!!
     */
    set_mail_conf_int(VAR_PID, var_pid = getpid());

    /*
     * Neither can the start time variable. It isn't even visible.
     */
    time(&var_starttime);

    /*
     * I have seen this happen just too often.
     */
    if (strcasecmp(var_myhostname, var_relayhost) == 0)
	msg_fatal("myhostname == relayhost");
}
