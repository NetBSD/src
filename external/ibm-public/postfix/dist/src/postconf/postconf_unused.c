/*	$NetBSD: postconf_unused.c,v 1.2 2025/02/25 19:15:47 christos Exp $	*/

/*++
/* NAME
/*	postconf_unused 3
/* SUMMARY
/*	report unused or deprecated parameters
/* SYNOPSIS
/*	#include <postconf.h>
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

/* Utility library. */

#include <msg.h>
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
    0,
};
static HTABLE *pcf_depr_param_table;

/* pcf_init_depr_params - initialize lookup table */

static void pcf_init_depr_params(void)
{
    const PCF_DEPR_PARAM_INFO *dp;

    pcf_depr_param_table = htable_create(30);
    for (dp = pcf_depr_param_info; dp->name; dp++)
	(void) htable_enter(pcf_depr_param_table, dp->name, (void *) dp);
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
	    vstream_fflush(VSTREAM_OUT);
	    if ((dp = (const PCF_DEPR_PARAM_INFO *)
		 htable_find(pcf_depr_param_table, param_name)) != 0) {
		msg_warn("%s/%s: support for parameter %s has been removed;"
			 " instead, %s", var_config_dir, conf_name,
			 param_name, dp->alternative);
	    } else {
		msg_warn("%s/%s: unused parameter: %s=%s",
			 var_config_dir, conf_name, param_name, param_value);
	    }
	}

	/*
	 * Flag a parameter that is used but deprecated. Note that this may
	 * falsely complain about a user-defined parameter whose name matches
	 * that of a deleted parameter.
	 */
	else if ((dp = (const PCF_DEPR_PARAM_INFO *)
		  htable_find(pcf_depr_param_table, param_name)) != 0) {
	    vstream_fflush(VSTREAM_OUT);
	    msg_warn("%s/%s: support for parameter \"%s\" will be removed;"
		     " instead, %s", var_config_dir, conf_name,
		     param_name, dp->alternative);
	}
    }
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
