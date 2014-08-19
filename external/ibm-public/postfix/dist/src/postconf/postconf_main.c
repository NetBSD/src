/*	$NetBSD: postconf_main.c,v 1.1.1.1.6.3 2014/08/19 23:59:43 tls Exp $	*/

/*++
/* NAME
/*	postconf_main 3
/* SUMMARY
/*	basic support for main.cf
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	void	pcf_read_parameters()
/*
/*	void	pcf_show_parameters(fp, mode, param_class, names)
/*	VSTREAM	*fp;
/*	int	mode;
/*	int	param_class;
/*	char	**names;
/* DESCRIPTION
/*	pcf_read_parameters() reads parameters from main.cf.
/*
/*	pcf_set_parameters() takes an array of \fIname=value\fR
/*	pairs and overrides settings read with pcf_read_parameters().
/*
/*	pcf_show_parameters() writes main.cf parameters to the
/*	specified output stream.
/*
/*	Arguments:
/* .IP fp
/*	Output stream.
/* .IP mode
/*	Bit-wise OR of zero or more of the following:
/* .RS
/* .IP PCF_FOLD_LINE
/*	Fold long lines.
/* .IP PCF_SHOW_DEFS
/*	Output default parameter values.
/* .IP PCF_SHOW_NONDEF
/*	Output explicit settings only.
/* .IP PCF_HIDE_NAME
/*	Output parameter values without the "name =" prefix.
/* .IP PCF_SHOW_EVAL
/*	Expand $name in parameter values.
/* .RE
/* .IP param_class
/*	Bit-wise OR of one or more of the following:
/* .RS
/* .IP PCF_PARAM_FLAG_BUILTIN
/*	Show built-in parameters.
/* .IP PCF_PARAM_FLAG_SERVICE
/*	Show service-defined parameters.
/* .IP PCF_PARAM_FLAG_USER
/*	Show user-defined parameters.
/* .RE
/* .IP names
/*	List of zero or more parameter names. If the list is empty,
/*	output all parameters.
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstream.h>
#include <vstring.h>
#include <readlline.h>
#include <dict.h>
#include <stringops.h>
#include <htable.h>
#include <mac_expand.h>

/* Global library. */

#include <mail_params.h>
#include <mail_conf.h>

/* Application-specific. */

#include <postconf.h>

#define STR(x) vstring_str(x)

/* pcf_read_parameters - read parameter info from file */

void    pcf_read_parameters(void)
{
    char   *path;

    /*
     * A direct rip-off of mail_conf_read(). XXX Avoid code duplication by
     * better code decomposition.
     */
    pcf_set_config_dir();
    path = concatenate(var_config_dir, "/", MAIN_CONF_FILE, (char *) 0);
    if (dict_load_file_xt(CONFIG_DICT, path) == 0)
	msg_fatal("open %s: %m", path);
    myfree(path);
}

/* pcf_set_parameters - add or override name=value pairs */

void    pcf_set_parameters(char **name_val_array)
{
    char   *name, *value, *junk;
    const char *err;
    char  **cpp;

    for (cpp = name_val_array; *cpp; cpp++) {
	junk = mystrdup(*cpp);
	if ((err = split_nameval(junk, &name, &value)) != 0)
	    msg_fatal("invalid parameter override: %s: %s", *cpp, err);
	mail_conf_update(name, value);
	myfree(junk);
    }
}

/* pcf_print_parameter - show specific parameter */

static void pcf_print_parameter(VSTREAM *fp, int mode, const char *name,
				        PCF_PARAM_NODE *node)
{
    const char *value;

    /*
     * Use the default or actual value.
     */
    value = pcf_lookup_parameter_value(mode, name, (PCF_MASTER_ENT *) 0, node);

    /*
     * Optionally expand $name in the parameter value. Print the result with
     * or without the name= prefix.
     */
    if (value != 0) {
	if ((mode & PCF_SHOW_EVAL) != 0 && PCF_RAW_PARAMETER(node) == 0)
	    value = pcf_expand_parameter_value((VSTRING *) 0, mode, value,
					       (PCF_MASTER_ENT *) 0);
	if ((mode & PCF_HIDE_NAME) == 0) {
	    pcf_print_line(fp, mode, "%s = %s\n", name, value);
	} else {
	    pcf_print_line(fp, mode, "%s\n", value);
	}
	if (msg_verbose)
	    vstream_fflush(fp);
    }
}

/* pcf_comp_names - qsort helper */

static int pcf_comp_names(const void *a, const void *b)
{
    PCF_PARAM_INFO **ap = (PCF_PARAM_INFO **) a;
    PCF_PARAM_INFO **bp = (PCF_PARAM_INFO **) b;

    return (strcmp(PCF_PARAM_INFO_NAME(ap[0]),
		   PCF_PARAM_INFO_NAME(bp[0])));
}

/* pcf_show_parameters - show parameter info */

void    pcf_show_parameters(VSTREAM *fp, int mode, int param_class, char **names)
{
    PCF_PARAM_INFO **list;
    PCF_PARAM_INFO **ht;
    char  **namep;
    PCF_PARAM_NODE *node;

    /*
     * Show all parameters.
     */
    if (*names == 0) {
	list = PCF_PARAM_TABLE_LIST(pcf_param_table);
	qsort((char *) list, pcf_param_table->used, sizeof(*list),
	      pcf_comp_names);
	for (ht = list; *ht; ht++)
	    if (param_class & PCF_PARAM_INFO_NODE(*ht)->flags)
		pcf_print_parameter(fp, mode, PCF_PARAM_INFO_NAME(*ht),
				    PCF_PARAM_INFO_NODE(*ht));
	myfree((char *) list);
	return;
    }

    /*
     * Show named parameters.
     */
    for (namep = names; *namep; namep++) {
	if ((node = PCF_PARAM_TABLE_FIND(pcf_param_table, *namep)) == 0) {
	    msg_warn("%s: unknown parameter", *namep);
	} else {
	    pcf_print_parameter(fp, mode, *namep, node);
	}
    }
}
