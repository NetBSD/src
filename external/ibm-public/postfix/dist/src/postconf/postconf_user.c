/*	$NetBSD: postconf_user.c,v 1.1.1.2.2.1 2014/08/10 07:12:49 tls Exp $	*/

/*++
/* NAME
/*	postconf_user 3
/* SUMMARY
/*	support for user-defined main.cf parameter names
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	void	pcf_register_user_parameters()
/* DESCRIPTION
/*	Postfix has multiple parameter name spaces: the global
/*	main.cf parameter name space, and the local parameter name
/*	space of each master.cf entry. Parameters in local name
/*	spaces take precedence over global parameters.
/*
/*	There are three categories of known parameter names: built-in,
/*	service-defined (see postconf_service.c), and valid
/*	user-defined.
/*
/*	There are two categories of valid user-defined parameter
/*	names:
/*
/*	- Parameters whose user-defined-name appears in the value
/*	of smtpd_restriction_classes in main.cf or master.cf.
/*
/*	- Parameters whose $user-defined-name appear in the value
/*	of "name=value" entries in main.cf or master.cf.
/*
/*	- In both cases the parameters must have a
/*	"user-defined-name=value" entry in main.cf or master.cf.
/*
/*	Other user-defined parameter names are flagged as "unused".
/*
/*	pcf_register_user_parameters() scans the global and per-service
/*	name spaces for user-defined parameters and flags parameters
/*	as "valid" in the global name space (pcf_param_table) or
/*	in the per-service name space (valid_params).
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
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <htable.h>
#include <mac_expand.h>
#include <stringops.h>

/* Global library. */

#include <mail_conf.h>
#include <mail_params.h>

/* Application-specific. */

#include <postconf.h>

 /*
  * Hash with all user-defined names in the global smtpd_restriction_classes
  * value. This is used when validating "-o user-defined-name=value" entries
  * in master.cf.
  */
static HTABLE *pcf_rest_class_table;

 /*
  * SLMs.
  */
#define STR(x)	vstring_str(x)

 /*
  * Macros to make code with obscure constants more readable.
  */
#define NO_SCAN_RESULT	((VSTRING *) 0)
#define NO_SCAN_FILTER	((char *) 0)

/* SCAN_USER_PARAMETER_VALUE - examine macro names in parameter value */

#define SCAN_USER_PARAMETER_VALUE(value, class, scope) do { \
    PCF_PARAM_CTX _ctx; \
    _ctx.local_scope = (scope); \
    _ctx.param_class = (class); \
    (void) mac_expand(NO_SCAN_RESULT, (value), MAC_EXP_FLAG_SCAN, \
	    NO_SCAN_FILTER, pcf_flag_user_parameter_wrapper, (char *) &_ctx); \
} while (0)

/* pcf_convert_user_parameter - get user-defined parameter string value */

static const char *pcf_convert_user_parameter(char *unused_ptr)
{
    return ("");			/* can't happen */
}

/* pcf_flag_user_parameter - flag user-defined name "valid" if it has name=value */

static const char *pcf_flag_user_parameter(const char *mac_name,
					           int param_class,
					        PCF_MASTER_ENT *local_scope)
{
    const char *source = local_scope ? MASTER_CONF_FILE : MAIN_CONF_FILE;
    int     user_supplied = 0;

    /*
     * If the name=value exists in the local (or global) name space, update
     * the local (or global) "valid" parameter name table.
     * 
     * Do not "validate" user-defined parameters whose name appears only as
     * macro expansion; this is how Postfix historically implements backwards
     * compatibility after a feature name change.
     */
    if (local_scope && dict_get(local_scope->all_params, mac_name)) {
	user_supplied = 1;
	/* $name in master.cf references name=value in master.cf. */
	if (PCF_PARAM_TABLE_LOCATE(local_scope->valid_names, mac_name) == 0) {
	    PCF_PARAM_TABLE_ENTER(local_scope->valid_names, mac_name,
				  param_class, PCF_PARAM_NO_DATA,
				  pcf_convert_user_parameter);
	    if (msg_verbose)
		msg_info("$%s in %s:%s validates %s=value in %s:%s",
			 mac_name, MASTER_CONF_FILE,
			 local_scope->name_space,
			 mac_name, MASTER_CONF_FILE,
			 local_scope->name_space);
	}
    } else if (mail_conf_lookup(mac_name) != 0) {
	user_supplied = 1;
	/* $name in main/master.cf references name=value in main.cf. */
	if (PCF_PARAM_TABLE_LOCATE(pcf_param_table, mac_name) == 0) {
	    PCF_PARAM_TABLE_ENTER(pcf_param_table, mac_name, param_class,
			     PCF_PARAM_NO_DATA, pcf_convert_user_parameter);
	    if (msg_verbose) {
		if (local_scope)
		    msg_info("$%s in %s:%s validates %s=value in %s",
			     mac_name, MASTER_CONF_FILE,
			     local_scope->name_space,
			     mac_name, MAIN_CONF_FILE);
		else
		    msg_info("$%s in %s validates %s=value in %s",
			     mac_name, MAIN_CONF_FILE,
			     mac_name, MAIN_CONF_FILE);
	    }
	}
    }
    if (local_scope == 0) {
	for (local_scope = pcf_master_table; local_scope->argv; local_scope++) {
	    if (local_scope->all_params != 0
		&& dict_get(local_scope->all_params, mac_name) != 0) {
		user_supplied = 1;
		/* $name in main.cf references name=value in master.cf. */
		if (PCF_PARAM_TABLE_LOCATE(local_scope->valid_names, mac_name) == 0) {
		    PCF_PARAM_TABLE_ENTER(local_scope->valid_names, mac_name,
					  param_class, PCF_PARAM_NO_DATA,
					  pcf_convert_user_parameter);
		    if (msg_verbose)
			msg_info("$%s in %s validates %s=value in %s:%s",
				 mac_name, MAIN_CONF_FILE,
				 mac_name, MASTER_CONF_FILE,
				 local_scope->name_space);
		}
	    }
	}
    }

    /*
     * Warn about a $name that has no user-supplied explicit value or
     * Postfix-supplied default value. We don't enforce this for legacy DBMS
     * parameters because they exist only for backwards compatibility, so we
     * don't bother to figure out which parameters come without defaults.
     */
    if (user_supplied == 0 && (param_class & PCF_PARAM_FLAG_DBMS) == 0
	&& PCF_PARAM_TABLE_LOCATE(pcf_param_table, mac_name) == 0)
	msg_warn("%s/%s: undefined parameter: %s",
		 var_config_dir, source, mac_name);
    return (0);
}

/* pcf_flag_user_parameter_wrapper - mac_expand call-back helper */

static const char *pcf_flag_user_parameter_wrapper(const char *mac_name,
						           int unused_mode,
						           char *context)
{
    PCF_PARAM_CTX *ctx = (PCF_PARAM_CTX *) context;

    return (pcf_flag_user_parameter(mac_name, ctx->param_class, ctx->local_scope));
}

/* pcf_lookup_eval - generalized mail_conf_lookup_eval */

static const char *pcf_lookup_eval(const char *dict_name, const char *name)
{
    const char *value;

#define RECURSIVE       1

    if ((value = dict_lookup(dict_name, name)) != 0)
	value = dict_eval(dict_name, value, RECURSIVE);
    return (value);
}

/* pcf_scan_user_parameter_namespace - scan parameters in name space */

static void pcf_scan_user_parameter_namespace(const char *dict_name,
					        PCF_MASTER_ENT *local_scope)
{
    const char *myname = "pcf_scan_user_parameter_namespace";
    const char *class_list;
    char   *saved_class_list;
    char   *cp;
    DICT   *dict;
    char   *param_name;
    int     how;
    const char *cparam_name;
    const char *cparam_value;
    PCF_PARAM_NODE *node;
    const char *source = local_scope ? MASTER_CONF_FILE : MAIN_CONF_FILE;

    /*
     * Flag parameter names in smtpd_restriction_classes as "valid", but only
     * if they have a "name=value" entry. If we are in not in a local name
     * space, update the global restriction class name table, so that we can
     * query the global table from within a local master.cf name space.
     */
    if ((class_list = pcf_lookup_eval(dict_name, VAR_REST_CLASSES)) != 0) {
	cp = saved_class_list = mystrdup(class_list);
	while ((param_name = mystrtok(&cp, ", \t\r\n")) != 0) {
	    if (local_scope == 0
		&& htable_locate(pcf_rest_class_table, param_name) == 0)
		htable_enter(pcf_rest_class_table, param_name, "");
	    pcf_flag_user_parameter(param_name, PCF_PARAM_FLAG_USER, local_scope);
	}
	myfree(saved_class_list);
    }

    /*
     * For all "name=value" instances: a) if the name space is local and the
     * name appears in the global restriction class table, flag the name as
     * "valid" in the local name space; b) scan the value for macro
     * expansions of unknown parameter names, and flag those parameter names
     * as "valid" if they have a "name=value" entry.
     * 
     * We delete name=value entries for read-only parameters, to maintain
     * compatibility with Postfix programs that ignore such settings.
     */
    if ((dict = dict_handle(dict_name)) == 0)
	msg_panic("%s: parameter dictionary %s not found",
		  myname, dict_name);
    if (dict->sequence == 0)
	msg_panic("%s: parameter dictionary %s has no iterator",
		  myname, dict_name);
    for (how = DICT_SEQ_FUN_FIRST;
	 dict->sequence(dict, how, &cparam_name, &cparam_value) == 0;
	 how = DICT_SEQ_FUN_NEXT) {
	if (local_scope != 0
	&& PCF_PARAM_TABLE_LOCATE(local_scope->valid_names, cparam_name) == 0
	    && htable_locate(pcf_rest_class_table, cparam_name) != 0)
	    PCF_PARAM_TABLE_ENTER(local_scope->valid_names, cparam_name,
				  PCF_PARAM_FLAG_USER, PCF_PARAM_NO_DATA,
				  pcf_convert_user_parameter);
	if ((node = PCF_PARAM_TABLE_FIND(pcf_param_table, cparam_name)) != 0) {
	    if (PCF_READONLY_PARAMETER(node)) {
		msg_warn("%s/%s: read-only parameter assignment: %s=%s",
			 var_config_dir, source, cparam_name, cparam_value);
		/* Can't use dict_del() with Postfix<2.10 htable_sequence(). */
		if (dict_del(dict, cparam_name) != 0)
		    msg_panic("%s: can't delete %s/%s parameter entry for %s",
			      myname, var_config_dir, source, cparam_name);
		continue;
	    }
	    /* Re-label legacy parameter as user-defined, so it's printed. */
	    if (PCF_LEGACY_PARAMETER(node))
		PCF_PARAM_CLASS_OVERRIDE(node, PCF_PARAM_FLAG_USER);
	    /* Skip "do not expand" parameters. */
	    if (PCF_RAW_PARAMETER(node))
		continue;
	}
	SCAN_USER_PARAMETER_VALUE(cparam_value, PCF_PARAM_FLAG_USER, local_scope);
#ifdef LEGACY_DBMS_SUPPORT
	pcf_register_dbms_parameters(cparam_value, pcf_flag_user_parameter,
				     local_scope);
#endif
    }
}

/* pcf_scan_default_parameter_values - scan parameters at implicit defaults */

static void pcf_scan_default_parameter_values(HTABLE *valid_params,
					              const char *dict_name,
					        PCF_MASTER_ENT *local_scope)
{
    const char *myname = "pcf_scan_default_parameter_values";
    PCF_PARAM_INFO **list;
    PCF_PARAM_INFO **ht;
    const char *param_value;

    list = PCF_PARAM_TABLE_LIST(valid_params);
    for (ht = list; *ht; ht++) {
	/* Skip "do not expand" parameters. */
	if (PCF_RAW_PARAMETER(PCF_PARAM_INFO_NODE(*ht)))
	    continue;
	/* Skip parameters with a non-default value. */
	if (dict_lookup(dict_name, PCF_PARAM_INFO_NAME(*ht)))
	    continue;
	if ((param_value = pcf_convert_param_node(PCF_SHOW_DEFS, PCF_PARAM_INFO_NAME(*ht),
					    PCF_PARAM_INFO_NODE(*ht))) == 0)
	    msg_panic("%s: parameter %s has no default value",
		      myname, PCF_PARAM_INFO_NAME(*ht));
	SCAN_USER_PARAMETER_VALUE(param_value, PCF_PARAM_FLAG_USER, local_scope);
	/* No need to scan default values for legacy DBMS configuration. */
    }
    myfree((char *) list);
}

/* pcf_register_user_parameters - add parameters with user-defined names */

void    pcf_register_user_parameters(void)
{
    const char *myname = "pcf_register_user_parameters";
    PCF_MASTER_ENT *masterp;
    ARGV   *argv;
    char   *arg;
    char   *aval;
    int     field;
    char   *saved_arg;
    char   *param_name;
    char   *param_value;
    DICT   *dict;

    /*
     * Sanity checks.
     */
    if (pcf_param_table == 0)
	msg_panic("%s: global parameter table is not initialized", myname);
    if (pcf_master_table == 0)
	msg_panic("%s: master table is not initialized", myname);
    if (pcf_rest_class_table != 0)
	msg_panic("%s: restriction class table is already initialized", myname);

    /*
     * Initialize the table with global restriction class names.
     */
    pcf_rest_class_table = htable_create(1);

    /*
     * Initialize the per-service parameter name spaces.
     */
    for (masterp = pcf_master_table; (argv = masterp->argv) != 0; masterp++) {
	for (field = PCF_MASTER_MIN_FIELDS; argv->argv[field] != 0; field++) {
	    arg = argv->argv[field];
	    if (arg[0] != '-' || strcmp(arg, "--") == 0)
		break;
	    if (strchr(pcf_daemon_options_expecting_value, arg[1]) == 0
		|| (aval = argv->argv[field + 1]) == 0)
		continue;
	    if (strcmp(arg, "-o") == 0) {
		saved_arg = mystrdup(aval);
		if (split_nameval(saved_arg, &param_name, &param_value) == 0)
		    dict_update(masterp->name_space, param_name, param_value);
		myfree(saved_arg);
	    }
	    field += 1;
	}
	if ((dict = dict_handle(masterp->name_space)) != 0) {
	    masterp->all_params = dict;
	    masterp->valid_names = htable_create(1);
	}
    }

    /*
     * Scan the "-o parameter=value" instances in each master.cf name space.
     */
    for (masterp = pcf_master_table; masterp->argv != 0; masterp++)
	if (masterp->all_params != 0)
	    pcf_scan_user_parameter_namespace(masterp->name_space, masterp);

    /*
     * Scan parameter values that are left at their defaults in the global
     * name space. Some defaults contain the $name of an obsolete parameter
     * for backwards compatilility purposes. We might warn that an explicit
     * name=value is obsolete, but we must not warn that the parameter is
     * unused.
     */
    pcf_scan_default_parameter_values(pcf_param_table, CONFIG_DICT,
				      (PCF_MASTER_ENT *) 0);

    /*
     * Scan the explicit name=value entries in the global name space.
     */
    pcf_scan_user_parameter_namespace(CONFIG_DICT, (PCF_MASTER_ENT *) 0);
}
