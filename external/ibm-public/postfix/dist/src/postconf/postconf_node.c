/*	$NetBSD: postconf_node.c,v 1.1.1.1.6.3 2014/08/19 23:59:43 tls Exp $	*/

/*++
/* NAME
/*	postconf_node 3
/* SUMMARY
/*	low-level parameter node support
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	PCF_PARAM_TABLE *PCF_PARAM_TABLE_CREATE(size)
/*	ssize_t	size;
/*
/*	PCF_PARAM_INFO **PCF_PARAM_TABLE_LIST(table)
/*	PCF_PARAM_TABLE *table;
/*
/*	const char *PCF_PARAM_INFO_NAME(info)
/*	PCF_PARAM_INFO *info;
/*
/*	PCF_PARAM_NODE *PCF_PARAM_INFO_NODE(info)
/*	PCF_PARAM_INFO *info;
/*
/*	PCF_PARAM_NODE *PCF_PARAM_TABLE_FIND(table, name)
/*	PCF_PARAM_TABLE *table;
/*	const char *name;
/*
/*	PCF_PARAM_INFO *PCF_PARAM_TABLE_LOCATE(table, name)
/*	PCF_PARAM_TABLE *table;
/*	const char *name;
/*
/*	PCF_PARAM_INFO *PCF_PARAM_TABLE_ENTER(table, name, flags,
/*						param_data, convert_fn)
/*	PCF_PARAM_TABLE *table;
/*	const char *name;
/*	int	flags;
/*	char	*param_data;
/*	const char *(*convert_fn)(char *);
/*
/*	PCF_PARAM_NODE *pcf_make_param_node(flags, param_data, convert_fn)
/*	int	flags;
/*	char	*param_data;
/*	const char *(*convert_fn) (char *);
/*
/*	const char *pcf_convert_param_node(mode, name, node)
/*	int	mode;
/*	const char *name;
/*	PCF_PARAM_NODE *node;
/*
/*	VSTRING *pcf_param_string_buf;
/*
/*	PCF_RAW_PARAMETER(node)
/*	const PCF_PARAM_NODE *node;
/* DESCRIPTION
/*	This module maintains data structures (PCF_PARAM_NODE) with
/*	information about known-legitimate parameters.  These data
/*	structures are stored in a hash table.
/*
/*	The PCF_PARAM_MUMBLE() macros are wrappers around the
/*	htable(3) module. Their sole purpose is to encapsulate all
/*	the pointer casting from and to (PCF_PARAM_NODE *). Apart
/*	from that, the macros have no features worth discussing.
/*
/*	pcf_make_param_node() creates a node for the global parameter
/*	table. This node provides a parameter default value, and a
/*	function that converts the default value to string.
/*
/*	pcf_convert_param_node() produces a string representation
/*	for a global parameter default value.
/*
/*	PCF_RAW_PARAMETER() returns non-zero if the specified
/*	parameter node represents a "raw parameter". The value of
/*	such parameters must not be scanned for macro names.  Some
/*	"raw parameter" values contain "$" without macros, such as
/*	the smtpd_expansion_filter "safe character" set; and some
/*	contain $name from a private name space, such as forward_path.
/*	Some "raw parameter" values in postscreen(8) are safe to
/*	expand by one level.  Support for that may be added later.
/*
/*	pcf_param_string_buf is a buffer that is initialized on the
/*	fly and that parameter-to-string conversion functions may
/*	use for temporary result storage.
/*
/*	Arguments:
/* .IP size
/*	The initial size of the hash table.
/* .IP table
/*	A hash table for storage of "valid parameter" information.
/* .IP info
/*	A data structure with a name component and a PCF_PARAM_NODE
/*	component. Use PCF_PARAM_INFO_NAME() and PCF_PARAM_INFO_NODE()
/*	to access these components.
/* .IP name
/*	The name of a "valid parameter".
/* .IP flags
/*	PCF_PARAM_FLAG_RAW for a "raw parameter", PCF_PARAM_FLAG_NONE
/*	otherwise. See the PCF_RAW_PARAMETER() discussion above for
/*	discussion of "raw parameter" values.
/* .IP param_data
/*	Information about the parameter value.  Specify PCF_PARAM_NO_DATA
/*	if this is not applicable.
/* .IP convert_fn
/*	The function that will be invoked to produce a string
/*	representation of the information in param_data. The function
/*	receives the param_data value as argument.
/* .IP mode
/*	For now, the PCF_SHOW_DEFS flag is required.
/* .IP name
/*	The name of the parameter whose value is requested.  This
/*	is used for diagnostics.
/* .IP node
/*	The (flags, param_data, convert_fn) information that needs
/*	to be converted to a string representation of the default
/*	value.
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
#include <mymalloc.h>
#include <vstring.h>

/* Application-specific. */

#include <postconf.h>

VSTRING *pcf_param_string_buf;

/* pcf_make_param_node - make node for global parameter table */

PCF_PARAM_NODE *pcf_make_param_node(int flags, char *param_data,
				         const char *(*convert_fn) (char *))
{
    PCF_PARAM_NODE *node;

    node = (PCF_PARAM_NODE *) mymalloc(sizeof(*node));
    node->flags = flags;
    node->param_data = param_data;
    node->convert_fn = convert_fn;
    return (node);
}

/* pcf_convert_param_node - get default parameter value */

const char *pcf_convert_param_node(int mode, const char *name, PCF_PARAM_NODE *node)
{
    const char *myname = "pcf_convert_param_node";
    const char *value;

    /*
     * One-off initialization.
     */
    if (pcf_param_string_buf == 0)
	pcf_param_string_buf = vstring_alloc(100);

    /*
     * Sanity check. A null value indicates that a parameter does not have
     * the requested value. At this time, the only requested value can be the
     * default value, and a null pointer value makes no sense here.
     */
    if ((mode & PCF_SHOW_DEFS) == 0)
	msg_panic("%s: request for non-default value of parameter %s",
		  myname, name);
    if ((value = node->convert_fn(node->param_data)) == 0)
	msg_panic("%s: parameter %s has null pointer default value",
		  myname, name);

    /*
     * Return the parameter default value.
     */
    return (value);
}
