/*	$NetBSD: postconf_main.c,v 1.1.1.2 2013/09/25 19:06:33 tron Exp $	*/

/*++
/* NAME
/*	postconf_main 3
/* SUMMARY
/*	basic support for main.cf
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	void	read_parameters()
/*
/*	void	show_parameters(fp, mode, param_class, names)
/*	VSTREAM	*fp;
/*	int	mode;
/*	int	param_class;
/*	char	**names;
/* DESCRIPTION
/*	read_parameters() reads parameters from main.cf.
/*
/*	set_parameters() takes an array of \fIname=value\fR pairs
/*	and overrides settings read with read_parameters().
/*
/*	show_parameters() writes main.cf parameters to the specified
/*	output stream.
/*
/*	Arguments:
/* .IP fp
/*	Output stream.
/* .IP mode
/*	Bit-wise OR of zero or more of the following:
/* .RS
/* .IP FOLD_LINE
/*	Fold long lines.
/* .IP SHOW_DEFS
/*	Output default parameter values.
/* .IP SHOW_NONDEF
/*	Output explicit settings only.
/* .IP SHOW_NAME
/*	Output the parameter as "name = value".
/* .IP SHOW_EVAL
/*	Expand $name in parameter values.
/* .RE
/* .IP param_class
/*	Bit-wise OR of one or more of the following:
/* .RS
/* .IP PC_PARAM_FLAG_BUILTIN
/*	Show built-in parameters.
/* .IP PC_PARAM_FLAG_SERVICE
/*	Show service-defined parameters.
/* .IP PC_PARAM_FLAG_USER
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

/* read_parameters - read parameter info from file */

void    read_parameters(void)
{
    char   *path;

    /*
     * A direct rip-off of mail_conf_read(). XXX Avoid code duplication by
     * better code decomposition.
     */
    set_config_dir();
    path = concatenate(var_config_dir, "/", MAIN_CONF_FILE, (char *) 0);
    if (dict_load_file_xt(CONFIG_DICT, path) == 0)
	msg_fatal("open %s: %m", path);
    myfree(path);
}

/* set_parameters - add or override name=value pairs */

void    set_parameters(char **name_val_array)
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

/* print_line - show line possibly folded, and with normalized whitespace */

static void print_line(VSTREAM *fp, int mode, const char *fmt,...)
{
    va_list ap;
    static VSTRING *buf = 0;
    char   *start;
    char   *next;
    int     line_len = 0;
    int     word_len;

    /*
     * One-off initialization.
     */
    if (buf == 0)
	buf = vstring_alloc(100);

    /*
     * Format the text.
     */
    va_start(ap, fmt);
    vstring_vsprintf(buf, fmt, ap);
    va_end(ap);

    /*
     * Normalize the whitespace. We don't use the line_wrap() routine because
     * 1) that function does not normalize whitespace between words and 2) we
     * want to normalize whitespace even when not wrapping lines.
     * 
     * XXX Some parameters preserve whitespace: for example, smtpd_banner and
     * smtpd_reject_footer. If we have to preserve whitespace between words,
     * then perhaps readlline() can be changed to canonicalize whitespace
     * that follows a newline.
     */
    for (start = STR(buf); *(start += strspn(start, SEPARATORS)) != 0; start = next) {
	word_len = strcspn(start, SEPARATORS);
	if (*(next = start + word_len) != 0)
	    *next++ = 0;
	if (word_len > 0 && line_len > 0) {
	    if ((mode & FOLD_LINE) == 0 || line_len + word_len < LINE_LIMIT) {
		vstream_fputs(" ", fp);
		line_len += 1;
	    } else {
		vstream_fputs("\n" INDENT_TEXT, fp);
		line_len = INDENT_LEN;
	    }
	}
	vstream_fputs(start, fp);
	line_len += word_len;
    }
    vstream_fputs("\n", fp);
}

/* print_parameter - show specific parameter */

static void print_parameter(VSTREAM *fp, int mode, const char *name,
			            PC_PARAM_NODE *node)
{
    const char *value;

    /*
     * Use the default or actual value.
     */
    value = lookup_parameter_value(mode, name, (PC_MASTER_ENT *) 0, node);

    /*
     * Optionally expand $name in the parameter value. Print the result with
     * or without the name= prefix.
     */
    if (value != 0) {
	if ((mode & SHOW_EVAL) != 0 && PC_RAW_PARAMETER(node) == 0)
	    value = expand_parameter_value((VSTRING *) 0, mode, value,
					   (PC_MASTER_ENT *) 0);
	if (mode & SHOW_NAME) {
	    print_line(fp, mode, "%s = %s\n", name, value);
	} else {
	    print_line(fp, mode, "%s\n", value);
	}
	if (msg_verbose)
	    vstream_fflush(fp);
    }
}

/* comp_names - qsort helper */

static int comp_names(const void *a, const void *b)
{
    PC_PARAM_INFO **ap = (PC_PARAM_INFO **) a;
    PC_PARAM_INFO **bp = (PC_PARAM_INFO **) b;

    return (strcmp(PC_PARAM_INFO_NAME(ap[0]),
		   PC_PARAM_INFO_NAME(bp[0])));
}

/* show_parameters - show parameter info */

void    show_parameters(VSTREAM *fp, int mode, int param_class, char **names)
{
    PC_PARAM_INFO **list;
    PC_PARAM_INFO **ht;
    char  **namep;
    PC_PARAM_NODE *node;

    /*
     * Show all parameters.
     */
    if (*names == 0) {
	list = PC_PARAM_TABLE_LIST(param_table);
	qsort((char *) list, param_table->used, sizeof(*list), comp_names);
	for (ht = list; *ht; ht++)
	    if (param_class & PC_PARAM_INFO_NODE(*ht)->flags)
		print_parameter(fp, mode, PC_PARAM_INFO_NAME(*ht),
				PC_PARAM_INFO_NODE(*ht));
	myfree((char *) list);
	return;
    }

    /*
     * Show named parameters.
     */
    for (namep = names; *namep; namep++) {
	if ((node = PC_PARAM_TABLE_FIND(param_table, *namep)) == 0) {
	    msg_warn("%s: unknown parameter", *namep);
	} else {
	    print_parameter(fp, mode, *namep, node);
	}
    }
}
