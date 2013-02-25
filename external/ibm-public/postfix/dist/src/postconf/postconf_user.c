/*	$NetBSD: postconf_user.c,v 1.1.1.1.6.2 2013/02/25 00:27:22 tls Exp $	*/

/*++
/* NAME
/*	postconf_user 3
/* SUMMARY
/*	support for user-defined parameter names
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	void	register_user_parameters()
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
/*	register_user_parameters() scans the global and per-service
/*	name spaces for user-defined parameters and flags
/*	parameters as "valid" in the global name space (param_table)
/*	or in the per-service name space (valid_params).
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
static HTABLE *rest_class_table;

 /*
  * SLMs.
  */
#define STR(x)	vstring_str(x)

 /*
  * Macros to make code with obscure constants more readable.
  */
#define NO_SCAN_RESULT	((VSTRING *) 0)
#define NO_SCAN_FILTER	((char *) 0)
#define NO_SCAN_MODE	(0)

/* SCAN_USER_PARAMETER_VALUE - examine macro names in parameter value */

#define SCAN_USER_PARAMETER_VALUE(value, local_scope) do { \
    (void) mac_expand(NO_SCAN_RESULT, (value), MAC_EXP_FLAG_SCAN, \
	    NO_SCAN_FILTER, flag_user_parameter, ((char *) (local_scope))); \
} while (0)

/* FLAG_USER_PARAMETER - flag user-defined name "valid" if it has name=value */

#define FLAG_USER_PARAMETER(name, local_scope) do { \
    flag_user_parameter((name), NO_SCAN_MODE, ((char *) (local_scope))); \
} while (0)

/* convert_user_parameter - get user-defined parameter string value */

static const char *convert_user_parameter(char *unused_ptr)
{
    return ("");			/* can't happen */
}

/* flag_user_parameter - flag user-defined name "valid" if it has name=value */

static const char *flag_user_parameter(const char *mac_name,
				               int unused_mode,
				               char *context)
{
    PC_MASTER_ENT *local_scope = (PC_MASTER_ENT *) context;

    /*
     * If the name=value exists in the local (or global) name space, update
     * the local (or global) "valid" parameter name table.
     * 
     * Do not "validate" user-defined parameters whose name appears only as
     * macro expansion; this is how Postfix historically implements backwards
     * compatibility after a feature name change.
     */
    if (local_scope && dict_get(local_scope->all_params, mac_name)) {
	/* $name in master.cf references name=value in master.cf. */
	if (PC_PARAM_TABLE_LOCATE(local_scope->valid_names, mac_name) == 0)
	    PC_PARAM_TABLE_ENTER(local_scope->valid_names, mac_name,
				 PC_PARAM_FLAG_USER, PC_PARAM_NO_DATA,
				 convert_user_parameter);
    } else if (mail_conf_lookup(mac_name) != 0) {
	/* $name in main/master.cf references name=value in main.cf. */
	if (PC_PARAM_TABLE_LOCATE(param_table, mac_name) == 0)
	    PC_PARAM_TABLE_ENTER(param_table, mac_name, PC_PARAM_FLAG_USER,
				 PC_PARAM_NO_DATA, convert_user_parameter);
    }
    if (local_scope == 0) {
	for (local_scope = master_table; local_scope->argv; local_scope++) {
	    if (local_scope->all_params != 0
		&& dict_get(local_scope->all_params, mac_name) != 0
	    /* $name in main.cf references name=value in master.cf. */
		&& PC_PARAM_TABLE_LOCATE(local_scope->valid_names, mac_name) == 0)
		PC_PARAM_TABLE_ENTER(local_scope->valid_names, mac_name,
				     PC_PARAM_FLAG_USER, PC_PARAM_NO_DATA,
				     convert_user_parameter);
	}
    }
    return (0);
}

/* pc_lookup_eval - generalized mail_conf_lookup_eval */

static const char *pc_lookup_eval(const char *dict_name, const char *name)
{
    const char *value;

#define RECURSIVE       1

    if ((value = dict_lookup(dict_name, name)) != 0)
	value = dict_eval(dict_name, value, RECURSIVE);
    return (value);
}

/* scan_user_parameter_namespace - scan parameters in name space */

static void scan_user_parameter_namespace(const char *dict_name,
					          PC_MASTER_ENT *local_scope)
{
    const char *myname = "scan_user_parameter_namespace";
    const char *class_list;
    char   *saved_class_list;
    char   *cp;
    DICT   *dict;
    char   *param_name;
    int     how;
    const char *cparam_name;
    const char *cparam_value;
    PC_PARAM_NODE *node;

    /*
     * Flag parameter names in smtpd_restriction_classes as "valid", but only
     * if they have a "name=value" entry. If we are in not in a local name
     * space, update the global restriction class name table, so that we can
     * query the global table from within a local master.cf name space.
     */
    if ((class_list = pc_lookup_eval(dict_name, VAR_REST_CLASSES)) != 0) {
	cp = saved_class_list = mystrdup(class_list);
	while ((param_name = mystrtok(&cp, ", \t\r\n")) != 0) {
	    if (local_scope == 0
		&& htable_locate(rest_class_table, param_name) == 0)
		htable_enter(rest_class_table, param_name, "");
	    FLAG_USER_PARAMETER(param_name, local_scope);
	}
	myfree(saved_class_list);
    }

    /*
     * For all "name=value" instances: a) if the name space is local and the
     * name appears in the global restriction class table, flag the name as
     * "valid" in the local name space; b) scan the value for macro
     * expansions of unknown parameter names, and flag those parameter names
     * as "valid" if they have a "name=value" entry.
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
	&& PC_PARAM_TABLE_LOCATE(local_scope->valid_names, cparam_name) == 0
	    && htable_locate(rest_class_table, cparam_name) != 0)
	    PC_PARAM_TABLE_ENTER(local_scope->valid_names, cparam_name,
				 PC_PARAM_FLAG_USER, PC_PARAM_NO_DATA,
				 convert_user_parameter);
	/* Skip "do not expand" parameters. */
	if ((node = PC_PARAM_TABLE_FIND(param_table, cparam_name)) != 0
	    && PC_RAW_PARAMETER(node))
	    continue;
	SCAN_USER_PARAMETER_VALUE(cparam_value, local_scope);
#ifdef LEGACY_DBMS_SUPPORT
	register_dbms_parameters(cparam_value, flag_user_parameter,
				 local_scope);
#endif
    }
}

/* scan_default_parameter_values - scan parameters at implicit defaults */

static void scan_default_parameter_values(HTABLE *valid_params,
					          const char *dict_name,
					          PC_MASTER_ENT *local_scope)
{
    const char *myname = "scan_default_parameter_values";
    PC_PARAM_INFO **list;
    PC_PARAM_INFO **ht;
    const char *param_value;

    list = PC_PARAM_TABLE_LIST(valid_params);
    for (ht = list; *ht; ht++) {
	/* Skip "do not expand" parameters. */
	if (PC_RAW_PARAMETER(PC_PARAM_INFO_NODE(*ht)))
	    continue;
	/* Skip parameters with a non-default value. */
	if (dict_lookup(dict_name, PC_PARAM_INFO_NAME(*ht)))
	    continue;
	if ((param_value = convert_param_node(SHOW_DEFS, PC_PARAM_INFO_NAME(*ht),
					      PC_PARAM_INFO_NODE(*ht))) == 0)
	    msg_panic("%s: parameter %s has no default value",
		      myname, PC_PARAM_INFO_NAME(*ht));
	SCAN_USER_PARAMETER_VALUE(param_value, local_scope);
	/* No need to scan default values for legacy DBMS configuration. */
    }
    myfree((char *) list);
}

/* register_user_parameters - add parameters with user-defined names */

void    register_user_parameters(void)
{
    const char *myname = "register_user_parameters";
    PC_MASTER_ENT *masterp;
    ARGV   *argv;
    char   *arg;
    int     field;
    char   *saved_arg;
    char   *param_name;
    char   *param_value;
    DICT   *dict;

    /*
     * Sanity checks.
     */
    if (param_table == 0)
	msg_panic("%s: global parameter table is not initialized", myname);
    if (master_table == 0)
	msg_panic("%s: master table is not initialized", myname);
    if (rest_class_table != 0)
	msg_panic("%s: restriction class table is already initialized", myname);

    /*
     * Initialize the table with global restriction class names.
     */
    rest_class_table = htable_create(1);

    /*
     * Initialize the per-service parameter name spaces.
     */
    for (masterp = master_table; (argv = masterp->argv) != 0; masterp++) {
	for (field = PC_MASTER_MIN_FIELDS; argv->argv[field] != 0; field++) {
	    arg = argv->argv[field];
	    if (arg[0] != '-' || strcmp(arg, "--") == 0)
		break;
	    if (strcmp(arg, "-o") == 0 && (arg = argv->argv[field + 1]) != 0) {
		saved_arg = mystrdup(arg);
		if (split_nameval(saved_arg, &param_name, &param_value) == 0)
		    dict_update(masterp->name_space, param_name, param_value);
		myfree(saved_arg);
		field += 1;
	    }
	}
	if ((dict = dict_handle(masterp->name_space)) != 0) {
	    masterp->all_params = dict;
	    masterp->valid_names = htable_create(1);
	}
    }

    /*
     * Scan parameter values that are left at their defaults in the global
     * name space. Some defaults contain the $name of an obsolete parameter
     * for backwards compatilility purposes. We might warn that an explicit
     * name=value is obsolete, but we must not warn that the parameter is
     * unused.
     */
    scan_default_parameter_values(param_table, CONFIG_DICT, (PC_MASTER_ENT *) 0);

    /*
     * Scan the explicit name=value entries in the global name space.
     */
    scan_user_parameter_namespace(CONFIG_DICT, (PC_MASTER_ENT *) 0);

    /*
     * Scan the "-o parameter=value" instances in each master.cf name space.
     */
    for (masterp = master_table; masterp->argv != 0; masterp++)
	if (masterp->all_params != 0)
	    scan_user_parameter_namespace(masterp->name_space, masterp);
}
