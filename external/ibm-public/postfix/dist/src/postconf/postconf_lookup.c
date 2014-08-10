/*	$NetBSD: postconf_lookup.c,v 1.1.1.1.2.1 2014/08/10 07:12:49 tls Exp $	*/

/*++
/* NAME
/*	postconf_lookup 3
/* SUMMARY
/*	parameter lookup routines
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	const char *pcf_lookup_parameter_value(mode, name, local_scope, node)
/*	int	mode;
/*	const char *name;
/*	PCF_MASTER_ENT *local_scope;
/*	PCF_PARAM_NODE *node;
/*
/*	char    *pcf_expand_parameter_value(buf, mode, value, local_scope)
/*	VSTRING *buf;
/*	int	mode;
/*	const char *value;
/*	PCF_MASTER_ENT *local_scope;
/* DESCRIPTION
/*	These functions perform parameter value lookups.  The order
/*	of decreasing precedence is:
/* .IP \(bu
/*	Search name=value parameter settings in master.cf.  These
/*	lookups are disabled with the PCF_SHOW_DEFS flag.
/* .IP \(bu
/*	Search name=value parameter settings in main.cf.  These
/*	lookups are disabled with the PCF_SHOW_DEFS flag.
/* .IP \(bu
/*	Search built-in default parameter settings. These lookups
/*	are disabled with the PCF_SHOW_NONDEF flag.
/* .PP
/*	pcf_lookup_parameter_value() looks up the value for the
/*	named parameter, and returns null if the name was not found.
/*
/*	pcf_expand_parameter_value() expands $name in the specified
/*	parameter value. This function ignores the PCF_SHOW_NONDEF
/*	flag.  The result value is a pointer to storage in a
/*	user-supplied buffer, or in a buffer that is overwritten
/*	with each call.
/*
/*	Arguments:
/* .IP buf
/*	Null buffer pointer, or pointer to user-supplied buffer.
/* .IP mode
/*	Bit-wise OR of zero or one of the following (other flags
/*	are ignored):
/* .RS
/* .IP PCF_SHOW_DEFS
/*	Search built-in default parameter settings only.
/* .IP PCF_SHOW_NONDEF
/*	Search local (master.cf) and global (main.cf) name=value
/*	parameter settings only.
/* .RE
/* .IP name
/*	The name of a parameter to be looked up.
/* .IP value
/*	The parameter value where $name should be expanded.
/* .IP local_scope
/*	Pointer to master.cf entry with local name=value settings,
/*	or a null pointer (i.e. no local parameter lookup).
/* .IP node
/*	Global default value for the named parameter, or a null
/*	pointer (i.e. do the global default lookup anyway).
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
#include <vstring.h>
#include <dict.h>
#include <stringops.h>
#include <mac_expand.h>

/* Global library. */

#include <mail_conf.h>

/* Application-specific. */

#include <postconf.h>

#define STR(x) vstring_str(x)

/* pcf_lookup_parameter_value - look up specific parameter value */

const char *pcf_lookup_parameter_value(int mode, const char *name,
				               PCF_MASTER_ENT *local_scope,
				               PCF_PARAM_NODE *node)
{
    const char *value = 0;

    /*
     * Local name=value entries in master.cf take precedence over global
     * name=value entries in main.cf. Built-in defaults have the lowest
     * precedence.
     */
    if ((mode & PCF_SHOW_DEFS) != 0
	|| ((local_scope == 0 || local_scope->all_params == 0
	     || (value = dict_get(local_scope->all_params, name)) == 0)
	    && (value = dict_lookup(CONFIG_DICT, name)) == 0
	    && (mode & PCF_SHOW_NONDEF) == 0)) {
	if (node != 0 || (node = PCF_PARAM_TABLE_FIND(pcf_param_table, name)) != 0)
	    value = pcf_convert_param_node(PCF_SHOW_DEFS, name, node);
    }
    return (value);
}

 /*
  * Data structure to pass private state while recursively expanding $name in
  * parameter values.
  */
typedef struct {
    int     mode;
    PCF_MASTER_ENT *local_scope;
} PCF_EVAL_CTX;

/* pcf_lookup_parameter_value_wrapper - macro parser call-back routine */

static const char *pcf_lookup_parameter_value_wrapper(const char *key,
						            int unused_type,
						              char *context)
{
    PCF_EVAL_CTX *cp = (PCF_EVAL_CTX *) context;

    return (pcf_lookup_parameter_value(cp->mode, key, cp->local_scope,
				       (PCF_PARAM_NODE *) 0));
}

/* pcf_expand_parameter_value - expand $name in parameter value */

char   *pcf_expand_parameter_value(VSTRING *buf, int mode, const char *value,
				           PCF_MASTER_ENT *local_scope)
{
    const char *myname = "pcf_expand_parameter_value";
    static VSTRING *local_buf;
    int     status;
    PCF_EVAL_CTX eval_ctx;

    /*
     * Initialize.
     */
    if (buf == 0) {
	if (local_buf == 0)
	    local_buf = vstring_alloc(10);
	buf = local_buf;
    }

    /*
     * Expand macros recursively.
     * 
     * When expanding $name in "postconf -n" parameter values, don't limit the
     * search to only non-default parameter values.
     * 
     * When expanding $name in "postconf -d" parameter values, do limit the
     * search to only default parameter values.
     */
#define DONT_FILTER (char *) 0

    eval_ctx.mode = (mode & ~PCF_SHOW_NONDEF);
    eval_ctx.local_scope = local_scope;
    status = mac_expand(buf, value, MAC_EXP_FLAG_RECURSE, DONT_FILTER,
		    pcf_lookup_parameter_value_wrapper, (char *) &eval_ctx);
    if (status & MAC_PARSE_ERROR)
	msg_fatal("macro processing error");
    if (msg_verbose > 1) {
	if (strcmp(value, STR(buf)) != 0)
	    msg_info("%s: expand %s -> %s", myname, value, STR(buf));
	else
	    msg_info("%s: const  %s", myname, value);
    }
    return (STR(buf));
}
