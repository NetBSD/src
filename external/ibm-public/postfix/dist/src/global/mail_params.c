/*	$NetBSD: mail_params.c,v 1.1.1.3 2013/01/02 18:58:58 tron Exp $	*/

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
/*	int	var_helpful_warnings;
/*	char	*var_syslog_name;
/*	char	*var_mail_owner;
/*	uid_t	var_owner_uid;
/*	gid_t	var_owner_gid;
/*	char	*var_sgid_group;
/*	gid_t	var_sgid_gid;
/*	char	*var_default_privs;
/*	uid_t	var_default_uid;
/*	gid_t	var_default_gid;
/*	char	*var_config_dir;
/*	char	*var_daemon_dir;
/*	char	*var_data_dir;
/*	char	*var_command_dir;
/*	char	*var_queue_dir;
/*	int	var_use_limit;
/*	int	var_idle_limit;
/*	int	var_event_drain;
/*	int	var_bundle_rcpt;
/*	char	*var_procname;
/*	int	var_pid;
/*	int	var_ipc_timeout;
/*	char	*var_pid_dir;
/*	int	var_dont_remove;
/*	char	*var_inet_interfaces;
/*	char	*var_proxy_interfaces;
/*	char	*var_inet_protocols;
/*	char	*var_mynetworks;
/*	char	*var_double_bounce_sender;
/*	int	var_line_limit;
/*	char	*var_alias_db_map;
/*	long	var_message_limit;
/*	char	*var_mail_release;
/*	char	*var_mail_version;
/*	int	var_ipc_idle_limit;
/*	int	var_ipc_ttl_limit;
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
/*	char	*var_mynetworks_style;
/*	char	*var_verp_delims;
/*	char	*var_verp_filter;
/*	char	*var_par_dom_match;
/*	char	*var_config_dirs;
/*
/*	int	var_inet_windowsize;
/*	char	*var_import_environ;
/*	char	*var_export_environ;
/*	char	*var_debug_peer_list;
/*	int	var_debug_peer_level;
/*	int	var_in_flow_delay;
/*	int	var_fault_inj_code;
/*	char   *var_bounce_service;
/*	char   *var_cleanup_service;
/*	char   *var_defer_service;
/*	char   *var_pickup_service;
/*	char   *var_queue_service;
/*	char   *var_rewrite_service;
/*	char   *var_showq_service;
/*	char   *var_error_service;
/*	char   *var_flush_service;
/*	char   *var_verify_service;
/*	char   *var_trace_service;
/*	char   *var_proxymap_service;
/*	char   *var_proxywrite_service;
/*	int	var_db_create_buf;
/*	int	var_db_read_buf;
/*	int	var_mime_maxdepth;
/*	int	var_mime_bound_len;
/*	int	var_header_limit;
/*	int	var_token_limit;
/*	int	var_disable_mime_input;
/*	int	var_disable_mime_oconv;
/*	int     var_strict_8bitmime;
/*	int     var_strict_7bit_hdrs;
/*	int     var_strict_8bit_body;
/*	int     var_strict_encoding;
/*	int     var_verify_neg_cache;
/*	int	var_oldlog_compat;
/*	int	var_delay_max_res;
/*	char	*var_int_filt_classes;
/*	int	var_cyrus_sasl_authzid;
/*
/*	char	*var_multi_conf_dirs;
/*	char	*var_multi_wrapper;
/*	char	*var_multi_group;
/*	char	*var_multi_name;
/*	bool	var_multi_enable;
/*	bool	var_long_queue_ids;
/*	bool	var_daemon_open_fatal;
/*
/*	void	mail_params_init()
/*
/*	const	char null_format_string[1];
/* DESCRIPTION
/*	This module (actually the associated include file) define the names
/*	and defaults of all mail configuration parameters.
/*
/*	mail_params_init() initializes the built-in parameters listed above.
/*	These parameters are relied upon by library routines, so they are
/*	initialized globally so as to avoid hard-to-find errors due to
/*	missing initialization. This routine must be called early, at
/*	least before entering a chroot jail.
/*
/*	null_format_string is a workaround for gcc compilers that complain
/*	about empty or null format strings.
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
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <ctype.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <msg_syslog.h>
#include <get_hostname.h>
#include <valid_hostname.h>
#include <stringops.h>
#include <safe.h>
#include <safe_open.h>
#include <mymalloc.h>
#include <dict.h>
#ifdef HAS_DB
#include <dict_db.h>
#endif
#include <inet_proto.h>
#include <vstring_vstream.h>
#include <iostuff.h>

/* Global library. */

#include <mynetworks.h>
#include <mail_conf.h>
#include <mail_version.h>
#include <mail_proto.h>
#include <verp_sender.h>
#include <own_inet_addr.h>
#include <mail_params.h>

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
int     var_helpful_warnings;
char   *var_syslog_name;
char   *var_mail_owner;
uid_t   var_owner_uid;
gid_t   var_owner_gid;
char   *var_sgid_group;
gid_t   var_sgid_gid;
char   *var_default_privs;
uid_t   var_default_uid;
gid_t   var_default_gid;
char   *var_config_dir;
char   *var_daemon_dir;
char   *var_data_dir;
char   *var_command_dir;
char   *var_queue_dir;
int     var_use_limit;
int     var_event_drain;
int     var_idle_limit;
int     var_bundle_rcpt;
char   *var_procname;
int     var_pid;
int     var_ipc_timeout;
char   *var_pid_dir;
int     var_dont_remove;
char   *var_inet_interfaces;
char   *var_proxy_interfaces;
char   *var_inet_protocols;
char   *var_mynetworks;
char   *var_double_bounce_sender;
int     var_line_limit;
char   *var_alias_db_map;
long    var_message_limit;
char   *var_mail_release;
char   *var_mail_version;
int     var_ipc_idle_limit;
int     var_ipc_ttl_limit;
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
char   *var_mynetworks_style;
char   *var_verp_delims;
char   *var_verp_filter;
int     var_in_flow_delay;
char   *var_par_dom_match;
char   *var_config_dirs;

int     var_inet_windowsize;
char   *var_import_environ;
char   *var_export_environ;
char   *var_debug_peer_list;
int     var_debug_peer_level;
int     var_fault_inj_code;
char   *var_bounce_service;
char   *var_cleanup_service;
char   *var_defer_service;
char   *var_pickup_service;
char   *var_queue_service;
char   *var_rewrite_service;
char   *var_showq_service;
char   *var_error_service;
char   *var_flush_service;
char   *var_verify_service;
char   *var_trace_service;
char   *var_proxymap_service;
char   *var_proxywrite_service;
int     var_db_create_buf;
int     var_db_read_buf;
int     var_mime_maxdepth;
int     var_mime_bound_len;
int     var_header_limit;
int     var_token_limit;
int     var_disable_mime_input;
int     var_disable_mime_oconv;
int     var_strict_8bitmime;
int     var_strict_7bit_hdrs;
int     var_strict_8bit_body;
int     var_strict_encoding;
int     var_verify_neg_cache;
int     var_oldlog_compat;
int     var_delay_max_res;
char   *var_int_filt_classes;
int     var_cyrus_sasl_authzid;

char   *var_multi_conf_dirs;
char   *var_multi_wrapper;
char   *var_multi_group;
char   *var_multi_name;
bool    var_multi_enable;
bool    var_long_queue_ids;
bool    var_daemon_open_fatal;

const char null_format_string[1] = "";

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
     * contents of $mydomain. Use a default domain as a final workaround.
     */
    name = get_hostname();
    if ((dot = strchr(name, '.')) == 0) {
	if ((domain = mail_conf_lookup_eval(VAR_MYDOMAIN)) == 0)
	    domain = DEF_MYDOMAIN;
	name = concatenate(name, ".", domain, (char *) 0);
    }
    return (name);
}

/* check_mydomainname - lookup domain name and validate */

static const char *check_mydomainname(void)
{
    char   *dot;

    /*
     * Use a default domain when the hostname is not a FQDN ("foo").
     */
    if ((dot = strchr(var_myhostname, '.')) == 0)
	return (DEF_MYDOMAIN);
    return (dot + 1);
}

/* check_default_privs - lookup default user attributes and validate */

static void check_default_privs(void)
{
    struct passwd *pwd;

    if ((pwd = getpwnam(var_default_privs)) == 0)
	msg_fatal("file %s/%s: parameter %s: unknown user name value: %s",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_DEFAULT_PRIVS, var_default_privs);
    if ((var_default_uid = pwd->pw_uid) == 0)
	msg_fatal("file %s/%s: parameter %s: user %s has privileged user ID",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_DEFAULT_PRIVS, var_default_privs);
    if ((var_default_gid = pwd->pw_gid) == 0)
	msg_fatal("file %s/%s: parameter %s: user %s has privileged group ID",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_DEFAULT_PRIVS, var_default_privs);
}

/* check_mail_owner - lookup owner user attributes and validate */

static void check_mail_owner(void)
{
    struct passwd *pwd;

    if ((pwd = getpwnam(var_mail_owner)) == 0)
	msg_fatal("file %s/%s: parameter %s: unknown user name value: %s",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_MAIL_OWNER, var_mail_owner);
    if ((var_owner_uid = pwd->pw_uid) == 0)
	msg_fatal("file %s/%s: parameter %s: user %s has privileged user ID",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_MAIL_OWNER, var_mail_owner);
    if ((var_owner_gid = pwd->pw_gid) == 0)
	msg_fatal("file %s/%s: parameter %s: user %s has privileged group ID",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_MAIL_OWNER, var_mail_owner);

    /*
     * This detects only some forms of sharing. Enumerating the entire
     * password file name space could be expensive. The purpose of this code
     * is to discourage user ID sharing by developers and package
     * maintainers.
     */
    if ((pwd = getpwuid(var_owner_uid)) != 0
	&& strcmp(pwd->pw_name, var_mail_owner) != 0)
	msg_fatal("file %s/%s: parameter %s: user %s has same user ID as %s",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_MAIL_OWNER, var_mail_owner, pwd->pw_name);
}

/* check_sgid_group - lookup setgid group attributes and validate */

static void check_sgid_group(void)
{
    struct group *grp;

    if ((grp = getgrnam(var_sgid_group)) == 0)
	msg_fatal("file %s/%s: parameter %s: unknown group name: %s",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_SGID_GROUP, var_sgid_group);
    if ((var_sgid_gid = grp->gr_gid) == 0)
	msg_fatal("file %s/%s: parameter %s: group %s has privileged group ID",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_SGID_GROUP, var_sgid_group);

    /*
     * This detects only some forms of sharing. Enumerating the entire group
     * file name space could be expensive. The purpose of this code is to
     * discourage group ID sharing by developers and package maintainers.
     */
    if ((grp = getgrgid(var_sgid_gid)) != 0
	&& strcmp(grp->gr_name, var_sgid_group) != 0)
	msg_fatal("file %s/%s: parameter %s: group %s has same group ID as %s",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_SGID_GROUP, var_sgid_group, grp->gr_name);
}

/* check_overlap - disallow UID or GID sharing */

static void check_overlap(void)
{
    if (strcmp(var_default_privs, var_mail_owner) == 0)
	msg_fatal("file %s/%s: parameters %s and %s specify the same user %s",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_DEFAULT_PRIVS, VAR_MAIL_OWNER,
		  var_default_privs);
    if (var_default_uid == var_owner_uid)
	msg_fatal("file %s/%s: parameters %s and %s: users %s and %s have the same user ID: %ld",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_DEFAULT_PRIVS, VAR_MAIL_OWNER,
		  var_default_privs, var_mail_owner,
		  (long) var_owner_uid);
    if (var_default_gid == var_owner_gid)
	msg_fatal("file %s/%s: parameters %s and %s: users %s and %s have the same group ID: %ld",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_DEFAULT_PRIVS, VAR_MAIL_OWNER,
		  var_default_privs, var_mail_owner,
		  (long) var_owner_gid);
    if (var_default_gid == var_sgid_gid)
	msg_fatal("file %s/%s: parameters %s and %s: user %s and group %s have the same group ID: %ld",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_DEFAULT_PRIVS, VAR_SGID_GROUP,
		  var_default_privs, var_sgid_group,
		  (long) var_sgid_gid);
    if (var_owner_gid == var_sgid_gid)
	msg_fatal("file %s/%s: parameters %s and %s: user %s and group %s have the same group ID: %ld",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_MAIL_OWNER, VAR_SGID_GROUP,
		  var_mail_owner, var_sgid_group,
		  (long) var_sgid_gid);
}

#ifdef MYORIGIN_FROM_FILE

/* read_param_from_file - read parameter value from file */

static char *read_param_from_file(const char *path)
{
    VSTRING *why = vstring_alloc(100);
    VSTRING *buf = vstring_alloc(100);
    VSTREAM *fp;
    char   *bp;
    char   *result;

    /*
     * Ugly macros to make complex expressions less unreadable.
     */
#define SKIP(start, var, cond) \
	for (var = start; *var && (cond); var++);

#define TRIM(s) { \
	char *p; \
	for (p = (s) + strlen(s); p > (s) && ISSPACE(p[-1]); p--); \
	*p = 0; \
    }

    fp = safe_open(path, O_RDONLY, 0, (struct stat *) 0, -1, -1, why);
    if (fp == 0)
	msg_fatal("%s: %s", path, vstring_str(why));
    vstring_get_nonl(buf, fp);
    if (vstream_ferror(fp))			/* FIX 20070501 */
	msg_fatal("%s: read error: %m", path);
    vstream_fclose(fp);
    SKIP(vstring_str(buf), bp, ISSPACE(*bp));
    TRIM(bp);
    result = mystrdup(bp);

    vstring_free(why);
    vstring_free(buf);
    return (result);
}

#endif

/* mail_params_init - configure built-in parameters */

void    mail_params_init()
{
    static const CONFIG_STR_TABLE first_str_defaults[] = {
	VAR_SYSLOG_FACILITY, DEF_SYSLOG_FACILITY, &var_syslog_facility, 1, 0,
	VAR_INET_PROTOCOLS, DEF_INET_PROTOCOLS, &var_inet_protocols, 0, 0,
	VAR_MULTI_CONF_DIRS, DEF_MULTI_CONF_DIRS, &var_multi_conf_dirs, 0, 0,
	/* multi_instance_wrapper may have dependencies but not dependents. */
	VAR_MULTI_GROUP, DEF_MULTI_GROUP, &var_multi_group, 0, 0,
	VAR_MULTI_NAME, DEF_MULTI_NAME, &var_multi_name, 0, 0,
	0,
    };
    static const CONFIG_BOOL_TABLE first_bool_defaults[] = {
	/* read and process the following before opening tables. */
	VAR_DAEMON_OPEN_FATAL, DEF_DAEMON_OPEN_FATAL, &var_daemon_open_fatal,
	0,
    };
    static const CONFIG_STR_FN_TABLE function_str_defaults[] = {
	VAR_MYHOSTNAME, check_myhostname, &var_myhostname, 1, 0,
	VAR_MYDOMAIN, check_mydomainname, &var_mydomain, 1, 0,
	0,
    };
    static const CONFIG_STR_TABLE other_str_defaults[] = {
	VAR_MAIL_NAME, DEF_MAIL_NAME, &var_mail_name, 1, 0,
	VAR_SYSLOG_NAME, DEF_SYSLOG_NAME, &var_syslog_name, 1, 0,
	VAR_MAIL_OWNER, DEF_MAIL_OWNER, &var_mail_owner, 1, 0,
	VAR_SGID_GROUP, DEF_SGID_GROUP, &var_sgid_group, 1, 0,
	VAR_MYDEST, DEF_MYDEST, &var_mydest, 0, 0,
	VAR_MYORIGIN, DEF_MYORIGIN, &var_myorigin, 1, 0,
	VAR_RELAYHOST, DEF_RELAYHOST, &var_relayhost, 0, 0,
	VAR_DAEMON_DIR, DEF_DAEMON_DIR, &var_daemon_dir, 1, 0,
	VAR_DATA_DIR, DEF_DATA_DIR, &var_data_dir, 1, 0,
	VAR_COMMAND_DIR, DEF_COMMAND_DIR, &var_command_dir, 1, 0,
	VAR_QUEUE_DIR, DEF_QUEUE_DIR, &var_queue_dir, 1, 0,
	VAR_PID_DIR, DEF_PID_DIR, &var_pid_dir, 1, 0,
	VAR_INET_INTERFACES, DEF_INET_INTERFACES, &var_inet_interfaces, 0, 0,
	VAR_PROXY_INTERFACES, DEF_PROXY_INTERFACES, &var_proxy_interfaces, 0, 0,
	VAR_DOUBLE_BOUNCE, DEF_DOUBLE_BOUNCE, &var_double_bounce_sender, 1, 0,
	VAR_DEFAULT_PRIVS, DEF_DEFAULT_PRIVS, &var_default_privs, 1, 0,
	VAR_ALIAS_DB_MAP, DEF_ALIAS_DB_MAP, &var_alias_db_map, 0, 0,
	VAR_MAIL_RELEASE, DEF_MAIL_RELEASE, &var_mail_release, 1, 0,
	VAR_MAIL_VERSION, DEF_MAIL_VERSION, &var_mail_version, 1, 0,
	VAR_DB_TYPE, DEF_DB_TYPE, &var_db_type, 1, 0,
	VAR_HASH_QUEUE_NAMES, DEF_HASH_QUEUE_NAMES, &var_hash_queue_names, 1, 0,
	VAR_RCPT_DELIM, DEF_RCPT_DELIM, &var_rcpt_delim, 0, 1,
	VAR_RELAY_DOMAINS, DEF_RELAY_DOMAINS, &var_relay_domains, 0, 0,
	VAR_FFLUSH_DOMAINS, DEF_FFLUSH_DOMAINS, &var_fflush_domains, 0, 0,
	VAR_EXPORT_ENVIRON, DEF_EXPORT_ENVIRON, &var_export_environ, 0, 0,
	VAR_IMPORT_ENVIRON, DEF_IMPORT_ENVIRON, &var_import_environ, 0, 0,
	VAR_MYNETWORKS_STYLE, DEF_MYNETWORKS_STYLE, &var_mynetworks_style, 1, 0,
	VAR_DEBUG_PEER_LIST, DEF_DEBUG_PEER_LIST, &var_debug_peer_list, 0, 0,
	VAR_VERP_DELIMS, DEF_VERP_DELIMS, &var_verp_delims, 2, 2,
	VAR_VERP_FILTER, DEF_VERP_FILTER, &var_verp_filter, 1, 0,
	VAR_PAR_DOM_MATCH, DEF_PAR_DOM_MATCH, &var_par_dom_match, 0, 0,
	VAR_CONFIG_DIRS, DEF_CONFIG_DIRS, &var_config_dirs, 0, 0,
	VAR_BOUNCE_SERVICE, DEF_BOUNCE_SERVICE, &var_bounce_service, 1, 0,
	VAR_CLEANUP_SERVICE, DEF_CLEANUP_SERVICE, &var_cleanup_service, 1, 0,
	VAR_DEFER_SERVICE, DEF_DEFER_SERVICE, &var_defer_service, 1, 0,
	VAR_PICKUP_SERVICE, DEF_PICKUP_SERVICE, &var_pickup_service, 1, 0,
	VAR_QUEUE_SERVICE, DEF_QUEUE_SERVICE, &var_queue_service, 1, 0,
	VAR_REWRITE_SERVICE, DEF_REWRITE_SERVICE, &var_rewrite_service, 1, 0,
	VAR_SHOWQ_SERVICE, DEF_SHOWQ_SERVICE, &var_showq_service, 1, 0,
	VAR_ERROR_SERVICE, DEF_ERROR_SERVICE, &var_error_service, 1, 0,
	VAR_FLUSH_SERVICE, DEF_FLUSH_SERVICE, &var_flush_service, 1, 0,
	VAR_VERIFY_SERVICE, DEF_VERIFY_SERVICE, &var_verify_service, 1, 0,
	VAR_TRACE_SERVICE, DEF_TRACE_SERVICE, &var_trace_service, 1, 0,
	VAR_PROXYMAP_SERVICE, DEF_PROXYMAP_SERVICE, &var_proxymap_service, 1, 0,
	VAR_PROXYWRITE_SERVICE, DEF_PROXYWRITE_SERVICE, &var_proxywrite_service, 1, 0,
	VAR_INT_FILT_CLASSES, DEF_INT_FILT_CLASSES, &var_int_filt_classes, 0, 0,
	/* multi_instance_wrapper may have dependencies but not dependents. */
	VAR_MULTI_WRAPPER, DEF_MULTI_WRAPPER, &var_multi_wrapper, 0, 0,
	0,
    };
    static const CONFIG_STR_FN_TABLE function_str_defaults_2[] = {
	VAR_MYNETWORKS, mynetworks, &var_mynetworks, 0, 0,
	0,
    };
    static const CONFIG_INT_TABLE other_int_defaults[] = {
	VAR_MAX_USE, DEF_MAX_USE, &var_use_limit, 1, 0,
	VAR_DONT_REMOVE, DEF_DONT_REMOVE, &var_dont_remove, 0, 0,
	VAR_LINE_LIMIT, DEF_LINE_LIMIT, &var_line_limit, 512, 0,
	VAR_HASH_QUEUE_DEPTH, DEF_HASH_QUEUE_DEPTH, &var_hash_queue_depth, 1, 0,
	VAR_FORK_TRIES, DEF_FORK_TRIES, &var_fork_tries, 1, 0,
	VAR_FLOCK_TRIES, DEF_FLOCK_TRIES, &var_flock_tries, 1, 0,
	VAR_DEBUG_PEER_LEVEL, DEF_DEBUG_PEER_LEVEL, &var_debug_peer_level, 1, 0,
	VAR_FAULT_INJ_CODE, DEF_FAULT_INJ_CODE, &var_fault_inj_code, 0, 0,
	VAR_DB_CREATE_BUF, DEF_DB_CREATE_BUF, &var_db_create_buf, 1, 0,
	VAR_DB_READ_BUF, DEF_DB_READ_BUF, &var_db_read_buf, 1, 0,
	VAR_HEADER_LIMIT, DEF_HEADER_LIMIT, &var_header_limit, 1, 0,
	VAR_TOKEN_LIMIT, DEF_TOKEN_LIMIT, &var_token_limit, 1, 0,
	VAR_MIME_MAXDEPTH, DEF_MIME_MAXDEPTH, &var_mime_maxdepth, 1, 0,
	VAR_MIME_BOUND_LEN, DEF_MIME_BOUND_LEN, &var_mime_bound_len, 1, 0,
	VAR_DELAY_MAX_RES, DEF_DELAY_MAX_RES, &var_delay_max_res, MIN_DELAY_MAX_RES, MAX_DELAY_MAX_RES,
	VAR_INET_WINDOW, DEF_INET_WINDOW, &var_inet_windowsize, 0, 0,
	0,
    };
    static const CONFIG_LONG_TABLE long_defaults[] = {
	VAR_MESSAGE_LIMIT, DEF_MESSAGE_LIMIT, &var_message_limit, 0, 0,
	0,
    };
    static const CONFIG_TIME_TABLE time_defaults[] = {
	VAR_EVENT_DRAIN, DEF_EVENT_DRAIN, &var_event_drain, 1, 0,
	VAR_MAX_IDLE, DEF_MAX_IDLE, &var_idle_limit, 1, 0,
	VAR_IPC_TIMEOUT, DEF_IPC_TIMEOUT, &var_ipc_timeout, 1, 0,
	VAR_IPC_IDLE, DEF_IPC_IDLE, &var_ipc_idle_limit, 1, 0,
	VAR_IPC_TTL, DEF_IPC_TTL, &var_ipc_ttl_limit, 1, 0,
	VAR_TRIGGER_TIMEOUT, DEF_TRIGGER_TIMEOUT, &var_trigger_timeout, 1, 0,
	VAR_FORK_DELAY, DEF_FORK_DELAY, &var_fork_delay, 1, 0,
	VAR_FLOCK_DELAY, DEF_FLOCK_DELAY, &var_flock_delay, 1, 0,
	VAR_FLOCK_STALE, DEF_FLOCK_STALE, &var_flock_stale, 1, 0,
	VAR_DAEMON_TIMEOUT, DEF_DAEMON_TIMEOUT, &var_daemon_timeout, 1, 0,
	VAR_IN_FLOW_DELAY, DEF_IN_FLOW_DELAY, &var_in_flow_delay, 0, 10,
	0,
    };
    static const CONFIG_BOOL_TABLE bool_defaults[] = {
	VAR_DISABLE_DNS, DEF_DISABLE_DNS, &var_disable_dns,
	VAR_SOFT_BOUNCE, DEF_SOFT_BOUNCE, &var_soft_bounce,
	VAR_OWNREQ_SPECIAL, DEF_OWNREQ_SPECIAL, &var_ownreq_special,
	VAR_STRICT_8BITMIME, DEF_STRICT_8BITMIME, &var_strict_8bitmime,
	VAR_STRICT_7BIT_HDRS, DEF_STRICT_7BIT_HDRS, &var_strict_7bit_hdrs,
	VAR_STRICT_8BIT_BODY, DEF_STRICT_8BIT_BODY, &var_strict_8bit_body,
	VAR_STRICT_ENCODING, DEF_STRICT_ENCODING, &var_strict_encoding,
	VAR_DISABLE_MIME_INPUT, DEF_DISABLE_MIME_INPUT, &var_disable_mime_input,
	VAR_DISABLE_MIME_OCONV, DEF_DISABLE_MIME_OCONV, &var_disable_mime_oconv,
	VAR_VERIFY_NEG_CACHE, DEF_VERIFY_NEG_CACHE, &var_verify_neg_cache,
	VAR_OLDLOG_COMPAT, DEF_OLDLOG_COMPAT, &var_oldlog_compat,
	VAR_HELPFUL_WARNINGS, DEF_HELPFUL_WARNINGS, &var_helpful_warnings,
	VAR_CYRUS_SASL_AUTHZID, DEF_CYRUS_SASL_AUTHZID, &var_cyrus_sasl_authzid,
	VAR_MULTI_ENABLE, DEF_MULTI_ENABLE, &var_multi_enable,
	VAR_LONG_QUEUE_IDS, DEF_LONG_QUEUE_IDS, &var_long_queue_ids,
	0,
    };
    const char *cp;
    INET_PROTO_INFO *proto_info;

    /*
     * Extract syslog_facility early, so that from here on all errors are
     * logged with the proper facility.
     */
    get_mail_conf_str_table(first_str_defaults);

    if (!msg_syslog_facility(var_syslog_facility))
	msg_fatal("file %s/%s: parameter %s: unrecognized value: %s",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_SYSLOG_FACILITY, var_syslog_facility);

    /*
     * Should daemons terminate after table open error, or should they
     * continue execution with reduced functionality?
     */
    get_mail_conf_bool_table(first_bool_defaults);
    if (var_daemon_open_fatal)
	dict_allow_surrogate = 0;

    /*
     * What protocols should we attempt to support? The result is stored in
     * the global inet_proto_table variable.
     */
    proto_info = inet_proto_init(VAR_INET_PROTOCOLS, var_inet_protocols);

    /*
     * Variables whose defaults are determined at runtime. Some sites use
     * short hostnames in the host table; some sites name their system after
     * the domain.
     */
    get_mail_conf_str_fn_table(function_str_defaults);
    if (!valid_hostname(var_myhostname, DO_GRIPE))
	msg_fatal("file %s/%s: parameter %s: bad parameter value: %s",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_MYHOSTNAME, var_myhostname);
    if (!valid_hostname(var_mydomain, DO_GRIPE))
	msg_fatal("file %s/%s: parameter %s: bad parameter value: %s",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_MYDOMAIN, var_mydomain);

    /*
     * Variables that are needed by almost every program.
     * 
     * XXX Reading the myorigin value from file is originally a Debian Linux
     * feature. This code is not enabled by default because of problems: 1)
     * it re-implements its own parameter syntax checks, and 2) it does not
     * implement $name expansions.
     */
    get_mail_conf_str_table(other_str_defaults);
#ifdef MYORIGIN_FROM_FILE
    if (*var_myorigin == '/') {
	char   *origin = read_param_from_file(var_myorigin);

	if (*origin == 0)
	    msg_fatal("%s file %s is empty", VAR_MYORIGIN, var_myorigin);
	myfree(var_myorigin);			/* FIX 20070501 */
	var_myorigin = origin;
    }
#endif
    get_mail_conf_int_table(other_int_defaults);
    get_mail_conf_long_table(long_defaults);
    get_mail_conf_bool_table(bool_defaults);
    get_mail_conf_time_table(time_defaults);
    check_default_privs();
    check_mail_owner();
    check_sgid_group();
    check_overlap();
#ifdef HAS_DB
    dict_db_cache_size = var_db_read_buf;
#endif
    inet_windowsize = var_inet_windowsize;

    /*
     * Variables whose defaults are determined at runtime, after other
     * variables have been set. This dependency is admittedly a bit tricky.
     * XXX Perhaps we should just register variables, and let the evaluator
     * figure out in what order to evaluate things.
     */
    get_mail_conf_str_fn_table(function_str_defaults_2);

    /*
     * FIX 200412 The IPv6 patch did not call own_inet_addr_list() before
     * entering the chroot jail on Linux IPv6 systems. Linux has the IPv6
     * interface list in /proc, which is not available after chrooting.
     */
    (void) own_inet_addr_list();

    /*
     * The PID variable cannot be set from the configuration file!!
     */
    set_mail_conf_int(VAR_PID, var_pid = getpid());

    /*
     * Neither can the start time variable. It isn't even visible.
     */
    time(&var_starttime);

    /*
     * Export the syslog name so children can inherit and use it before they
     * have initialized.
     */
    if ((cp = safe_getenv(CONF_ENV_LOGTAG)) == 0
	|| strcmp(cp, var_syslog_name) != 0)
	if (setenv(CONF_ENV_LOGTAG, var_syslog_name, 1) < 0)
	    msg_fatal("setenv %s %s: %m", CONF_ENV_LOGTAG, var_syslog_name);

    /*
     * I have seen this happen just too often.
     */
    if (strcasecmp(var_myhostname, var_relayhost) == 0)
	msg_fatal("%s and %s parameter settings must not be identical: %s",
		  VAR_MYHOSTNAME, VAR_RELAYHOST, var_myhostname);

    /*
     * XXX These should be caught by a proper parameter parsing algorithm.
     */
    if (var_myorigin[strcspn(var_myorigin, ", \t\r\n")])
	msg_fatal("%s parameter setting must not contain multiple values: %s",
		  VAR_MYORIGIN, var_myorigin);

    if (var_relayhost[strcspn(var_relayhost, ", \t\r\n")])
	msg_fatal("%s parameter setting must not contain multiple values: %s",
		  VAR_RELAYHOST, var_relayhost);

    /*
     * One more sanity check.
     */
    if ((cp = verp_delims_verify(var_verp_delims)) != 0)
	msg_fatal("file %s/%s: parameters %s and %s: %s",
		  var_config_dir, MAIN_CONF_FILE,
		  VAR_VERP_DELIMS, VAR_VERP_FILTER, cp);
}
