/*	$NetBSD: postconf_unused.c,v 1.3 2026/05/09 18:49:18 christos Exp $	*/

/*++
/* NAME
/*	postconf_unused 3
/* SUMMARY
/*	report unused or deprecated parameters
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	int	pcf_found_deprecated;
/*
/*	void    pcf_flag_unused_main_parameters()
/*
/*	void    pcf_flag_unused_master_parameters()
/* DESCRIPTION
/*	These functions must be called after all parameter information
/*	is initialized: built-ins, service-defined and user-defined.
/*	In other words, don't call these functions with "postconf
/*	-d" which ignores user-defined main.cf settings.
/*
/*	pcf_flag_unused_main_parameters() reports unused or deprecated
/*	"name=value" entries in main.cf.
/*
/*	pcf_flag_unused_master_parameters() reports unused or
/*	deprecated "-o name=value" entries in master.cf.
/*
/*	pcf_found_deprecated is non-zero if deprecated parameters were
/*	reported.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
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
/*	porcupine.org
/*	Amawalk, NY 10501, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <dict.h>
#include <vstream.h>

/* Global library. */

#include <mail_params.h>
#include <mail_conf.h>

/* Application-specific. */

#include <postconf.h>

 /*
  * Deprecated parameter names and suggested alternatives. If we keep deleted
  * parameter names in the table, a warning can still suggest alternatives.
  * The downside of keeping deleted names in the table is that we may falsely
  * warn about a user-defined parameter whose name matches that of a deleted
  * parameter.
  */
typedef struct {
    char   *name;
    char   *alternative;
} PCF_DEPR_PARAM_INFO;

static const PCF_DEPR_PARAM_INFO pcf_depr_param_info[] = {

    /*
     * Parameters with deprecation warnings as of Postfix 3.9. The
     * disable_dns_lookups parameter was documented as deprecated since
     * Postfix 2.11 but nothing was logged.
     */
    "disable_dns_lookups", "specify \"smtp_dns_support_level\"",
    "lmtp_use_tls", "specify \"lmtp_tls_security_level\"",
    "postscreen_use_tls", "specify \"postscreen_tls_security_level\"",
    "smtp_use_tls", "specify \"smtp_tls_security_level\"",
    "smtpd_use_tls", "specify \"smtpd_tls_security_level\"",
    "tlsproxy_client_use_tls", "specify \"tlsproxy_client_security_level\"",
    "tlsproxy_use_tls", "specify \"tlsproxy_tls_security_level\"",
    "lmtp_enforce_tls", "lmtp_tls_security_level",
    "postscreen_enforce_tls", "specify \"postscreen_tls_security_level\"",
    "smtp_enforce_tls", "specify \"smtp_tls_security_level\"",
    "smtpd_enforce_tls", "specify \"smtpd_tls_security_level\"",
    "tlsproxy_client_enforce_tls", "specify \"tlsproxy_client_security_level\"",
    "tlsproxy_enforce_tls", "specify \"tlsproxy_tls_security_level\"",
    "lmtp_tls_per_site", "specify \"lmtp_tls_policy_maps\"",
    "smtp_tls_per_site", "specify \"smtp_tls_policy_maps\"",
    "tlsproxy_client_per_site", "specify \"tlsproxy_client_policy_maps\"",
    "smtpd_tls_dh1024_param_file", "do not specify (leave at default)",
    "smtpd_tls_eecdh_grade", "do not specify (leave at default)",
    "deleted-test-only", "do not specify",	/* For testing */

    /*
     * Deprecated as of Postfix 3.11.
     */
    "authorized_verp_clients", "specify \"smtpd_authorized_verp_clients\"",
    "fallback_relay", "specify \"smtp_fallback_relay\"",
    "lmtp_per_record_deadline", "specify \"lmtp_per_request_deadline\"",
    "lmtp_tls_enforce_peername", "specify \"lmtp_tls_security_level\"",
    "postscreen_blacklist_action", "specify \"postscreen_denylist_action\"",
    "postscreen_dnsbl_ttl", "specify \"postscreen_dnsbl_max_ttl\"",
    "postscreen_dnsbl_whitelist_threshold", "specify \"postscreen_dnsbl_allowlist_threshold\"",
    "postscreen_whitelist_interfaces", "specify \"postscreen_allowlist_interfaces\"",
    "smtpd_client_connection_limit_exceptions", "specify \"smtpd_client_event_limit_exceptions\"",
    "smtpd_per_record_deadline", "specify \"smtpd_per_request_deadline\"",
    "smtp_per_record_deadline", "specify \"smtp_per_request_deadline\"",
    "smtp_tls_enforce_peername", "specify \"smtp_tls_security_level\"",
    "tlsproxy_client_level", "specify \"tlsproxy_client_security_level\"",
    "tlsproxy_client_policy", "specify \"tlsproxy_client_policy_maps\"",
    "virtual_maps", "specify \"virtual_alias_maps\"",
#if 0 && OPENSSL_VERSION_PREREQ(3,5)
    "tls_eecdh_auto_curves", "do not specify with OpenSSL 3.5 or later",
    "tls_ffdhe_auto_groups", "do not specify with OpenSSL 3.5 or later",
#endif
    "lmtp_cname_overrides_servername", "do not specify",
    "smtp_cname_overrides_servername", "do not specify",

    /*
     * Terminator.
     */
    0,
};

static HTABLE *pcf_depr_param_table;
int     pcf_found_deprecated;

#define STR(x)	vstring_str(x)

/* pcf_init_depr_params - initialize lookup table */

static void pcf_init_depr_params(void)
{
    const PCF_DEPR_PARAM_INFO *dp;

    pcf_depr_param_table = htable_create(30);
    for (dp = pcf_depr_param_info; dp->name; dp++)
	(void) htable_enter(pcf_depr_param_table, dp->name, (void *) dp);
}

/* pcf_cmp_ht_key - qsort helper for ht_info pointer array */

static int pcf_cmp_ht_key(const void *a, const void *b)
{
    HTABLE_INFO **ap = (HTABLE_INFO **) a;
    HTABLE_INFO **bp = (HTABLE_INFO **) b;

    return (strcmp(ap[0]->key, bp[0]->key));
}

/* pcf_flag_unused_parameters - warn about unused parameters */

static void pcf_flag_unused_parameters(DICT *dict, const char *conf_name,
				               PCF_MASTER_ENT *local_scope)
{
    const char *myname = "pcf_flag_unused_parameters";
    const PCF_DEPR_PARAM_INFO *dp;
    const char *param_name;
    const char *param_value;
    int     how;
    HTABLE *flagged;
    VSTRING *buf;

    /*
     * Sanity checks.
     */
    if (pcf_param_table == 0)
	msg_panic("%s: global parameter table is not initialized", myname);
    if (dict->sequence == 0)
	msg_panic("%s: parameter dictionary %s has no iterator",
		  myname, conf_name);

    /*
     * One-time initialization.
     */
    if (pcf_depr_param_table == 0)
	pcf_init_depr_params();

    /*
     * Iterate over all entries, and flag parameter names that aren't used
     * anywhere, or that are deprecated. Show the warning message(s) after
     * the end of the stdout output.
     */
    flagged = htable_create(1);
    buf = vstring_alloc(100);
    for (how = DICT_SEQ_FUN_FIRST;
	 dict->sequence(dict, how, &param_name, &param_value) == 0;
	 how = DICT_SEQ_FUN_NEXT) {

	/*
	 * Flag a parameter that is not used (deleted name, or incorrect
	 * name).
	 */
	if (PCF_PARAM_TABLE_LOCATE(pcf_param_table, param_name) == 0
	    && (local_scope == 0
		|| PCF_PARAM_TABLE_LOCATE(local_scope->valid_names, param_name) == 0)) {
	    if ((dp = (const PCF_DEPR_PARAM_INFO *)
		 htable_find(pcf_depr_param_table, param_name)) != 0) {
		vstring_sprintf(buf, "%s/%s: support for parameter %s"
				" has been removed; instead, %s",
				var_config_dir, conf_name,
				param_name, dp->alternative);
		pcf_found_deprecated = 1;
	    } else {
		vstring_sprintf(buf, "%s/%s: unused parameter: %s=%s",
			var_config_dir, conf_name, param_name, param_value);
	    }
	    (void) htable_enter(flagged, param_name, mystrdup(STR(buf)));
	}

	/*
	 * Flag a parameter that is used but deprecated. Note that this may
	 * falsely complain about a user-defined parameter whose name matches
	 * that of a deleted parameter.
	 */
	else if ((dp = (const PCF_DEPR_PARAM_INFO *)
		  htable_find(pcf_depr_param_table, param_name)) != 0) {
	    vstring_sprintf(buf, "%s/%s: support for parameter \"%s\""
			    " will be removed; instead, %s",
			    var_config_dir, conf_name,
			    param_name, dp->alternative);
	    pcf_found_deprecated = 1;
	    (void) htable_enter(flagged, param_name, mystrdup(STR(buf)));
	}
    }

    /*
     * Log flagged parameters in sorted order, for predictable results.
     */
    if (flagged->used > 0) {
	HTABLE_INFO **ht_info;
	HTABLE_INFO **ht;

	vstream_fflush(VSTREAM_OUT);
	ht_info = htable_list(flagged);
	qsort((void *) ht_info, flagged->used, sizeof(*ht_info),
	      pcf_cmp_ht_key);
	for (ht = ht_info; *ht; ht++)
	    msg_warn("%s", (char *) ht[0]->value);
	myfree(ht_info);
    }
    htable_free(flagged, myfree);
    vstring_free(buf);
}

/* pcf_flag_unused_main_parameters - warn about unused parameters */

void    pcf_flag_unused_main_parameters(void)
{
    const char *myname = "pcf_flag_unused_main_parameters";
    DICT   *dict;

    /*
     * Iterate over all main.cf entries, and flag parameter names that aren't
     * used anywhere.
     */
    if ((dict = dict_handle(CONFIG_DICT)) == 0)
	msg_panic("%s: parameter dictionary %s not found",
		  myname, CONFIG_DICT);
    pcf_flag_unused_parameters(dict, MAIN_CONF_FILE, (PCF_MASTER_ENT *) 0);
}

/* pcf_flag_unused_master_parameters - warn about unused parameters */

void    pcf_flag_unused_master_parameters(void)
{
    const char *myname = "pcf_flag_unused_master_parameters";
    PCF_MASTER_ENT *masterp;
    DICT   *dict;

    /*
     * Sanity checks.
     */
    if (pcf_master_table == 0)
	msg_panic("%s: master table is not initialized", myname);

    /*
     * Iterate over all master.cf entries, and flag parameter names that
     * aren't used anywhere.
     */
    for (masterp = pcf_master_table; masterp->argv != 0; masterp++)
	if ((dict = masterp->all_params) != 0)
	    pcf_flag_unused_parameters(dict, MASTER_CONF_FILE, masterp);
}
