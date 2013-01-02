/*	$NetBSD: postconf_main.c,v 1.1.1.1 2013/01/02 18:59:03 tron Exp $	*/

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
/*	void	set_parameters()
/*
/*	void	show_parameters(mode, param_class, names)
/*	int	mode;
/*	int	param_class;
/*	char	**names;
/* DESCRIPTION
/*	read_parameters() reads parameters from main.cf.
/*
/*	set_parameters() does nothing. It is a place holder for
/*	code that assigns actual or default parameter values, which
/*	could be needed to implement "postconf -e" parameter value
/*	expansion.
/*
/*	show_parameters() writes main.cf parameters to the standard
/*	output stream.
/*
/*	Arguments:
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
/*	Expand parameter values (not implemented).
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

/* set_parameters - set parameter values from default or explicit setting */

void set_parameters(void)
{

    /*
     * The proposal below describes some of the steps needed to expand
     * parameter values. It has a problem: it updates the configuration
     * parameter dictionary, and in doing so breaks the "postconf -d"
     * implementation. This makes "-d" and "-e" mutually exclusive.
     * 
     * Populate the configuration parameter dictionary with default settings or
     * with actual settings.
     * 
     * Iterate over each entry in str_fn_table, str_fn_table_2, time_table,
     * bool_table, int_table, str_table, and raw_table. Look up each
     * parameter name in the configuration parameter dictionary. If the
     * parameter is not set, take the default value, or take the value from
     * main.cf, without doing $name expansions. This includes converting
     * default values from numeric/boolean internal forms to external string
     * form.
     * 
     * Once the configuration parameter dictionary is populated, printing a
     * parameter setting is a matter of querying the configuration parameter
     * dictionary, optionally expanding of $name values, and printing the
     * result.
     */
}

/* print_line - show line possibly folded, and with normalized whitespace */

static void print_line(int mode, const char *fmt,...)
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
		vstream_fputs(" ", VSTREAM_OUT);
		line_len += 1;
	    } else {
		vstream_fputs("\n" INDENT_TEXT, VSTREAM_OUT);
		line_len = INDENT_LEN;
	    }
	}
	vstream_fputs(start, VSTREAM_OUT);
	line_len += word_len;
    }
    vstream_fputs("\n", VSTREAM_OUT);
}

/* print_parameter - show specific parameter */

static void print_parameter(int mode, const char *name,
			            PC_PARAM_NODE *node)
{
    const char *value;

    /*
     * Use the default or actual value.
     */
    if ((mode & SHOW_DEFS) != 0
	|| ((value = dict_lookup(CONFIG_DICT, name)) == 0
	    && (mode & SHOW_NONDEF) == 0))
	value = convert_param_node(SHOW_DEFS, name, node);

    /*
     * Print with or without the name= prefix.
     */
    if (value != 0) {
	if (mode & SHOW_NAME) {
	    print_line(mode, "%s = %s\n", name, value);
	} else {
	    print_line(mode, "%s\n", value);
	}
	if (msg_verbose)
	    vstream_fflush(VSTREAM_OUT);
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

void    show_parameters(int mode, int param_class, char **names)
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
		print_parameter(mode, PC_PARAM_INFO_NAME(*ht),
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
	    print_parameter(mode, *namep, node);
	}
    }
}
