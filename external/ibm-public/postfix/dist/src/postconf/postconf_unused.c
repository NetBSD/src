/*	$NetBSD: postconf_unused.c,v 1.1.1.1.10.1 2014/08/10 07:12:49 tls Exp $	*/

/*++
/* NAME
/*	postconf_unused 3
/* SUMMARY
/*	report unused parameters
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
/*	pcf_flag_unused_main_parameters() reports unused "name=value"
/*	entries in main.cf.
/*
/*	pcf_flag_unused_master_parameters() reports unused "-o
/*	name=value" entries in master.cf.
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

/* pcf_flag_unused_parameters - warn about unused parameters */

static void pcf_flag_unused_parameters(DICT *dict, const char *conf_name,
				               PCF_MASTER_ENT *local_scope)
{
    const char *myname = "pcf_flag_unused_parameters";
    const char *param_name;
    const char *param_value;
    int     how;

    /*
     * Sanity checks.
     */
    if (pcf_param_table == 0)
	msg_panic("%s: global parameter table is not initialized", myname);

    /*
     * Iterate over all entries, and flag parameter names that aren't used
     * anywhere. Show the warning message at the end of the output.
     */
    if (dict->sequence == 0)
	msg_panic("%s: parameter dictionary %s has no iterator",
		  myname, conf_name);
    for (how = DICT_SEQ_FUN_FIRST;
	 dict->sequence(dict, how, &param_name, &param_value) == 0;
	 how = DICT_SEQ_FUN_NEXT) {
	if (PCF_PARAM_TABLE_LOCATE(pcf_param_table, param_name) == 0
	    && (local_scope == 0
		|| PCF_PARAM_TABLE_LOCATE(local_scope->valid_names, param_name) == 0)) {
	    vstream_fflush(VSTREAM_OUT);
	    msg_warn("%s/%s: unused parameter: %s=%s",
		     var_config_dir, conf_name, param_name, param_value);
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
