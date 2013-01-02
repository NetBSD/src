/*	$NetBSD: postconf_node.c,v 1.1.1.1 2013/01/02 18:59:03 tron Exp $	*/

/*++
/* NAME
/*	postconf_node 3
/* SUMMARY
/*	low-level parameter node support
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	PC_PARAM_TABLE *PC_PARAM_TABLE_CREATE(size)
/*	ssize_t	size;
/*
/*	PC_PARAM_INFO **PC_PARAM_TABLE_LIST(table)
/*	PC_PARAM_TABLE *table;
/*
/*	const char *PC_PARAM_INFO_NAME(info)
/*	PC_PARAM_INFO *info;
/*
/*	PC_PARAM_NODE *PC_PARAM_INFO_NODE(info)
/*	PC_PARAM_INFO *info;
/*
/*	PC_PARAM_NODE *PC_PARAM_TABLE_FIND(table, name)
/*	PC_PARAM_TABLE *table;
/*	const char *name;
/*
/*	PC_PARAM_INFO *PC_PARAM_TABLE_LOCATE(table, name)
/*	PC_PARAM_TABLE *table;
/*	const char *name;
/*
/*	PC_PARAM_INFO *PC_PARAM_TABLE_ENTER(table, name, flags, 
/*						param_data, convert_fn)
/*	PC_PARAM_TABLE *table;
/*	const char *name;
/*	int	flags;
/*	char	*param_data;
/*	const char *(*convert_fn)(char *);
/*
/*	PC_PARAM_NODE *make_param_node(flags, param_data, convert_fn)
/*	int	flags;
/*	char	*param_data;
/*	const char *(*convert_fn) (char *);
/*
/*	const char *convert_param_node(mode, name, node)
/*	int	mode;
/*	const char *name;
/*	PC_PARAM_NODE *node;
/*
/*	VSTRING *param_string_buf;
/*
/*	PC_RAW_PARAMETER(node)
/*	const PC_PARAM_NODE *node;
/* DESCRIPTION
/*	This module maintains data structures (PC_PARAM_NODE) with
/*	information about known-legitimate parameters.  These data
/*	structures are stored in a hash table.
/*
/*	The PC_PARAM_MUMBLE() macros are wrappers around the htable(3)
/*	module. Their sole purpose is to encapsulate all the pointer
/*	casting from and to (PC_PARAM_NODE *). Apart from that, the
/*	macros have no features worth discussing.
/*
/*	make_param_node() creates a node for the global parameter
/*	table. This node provides a parameter default value, and a
/*	function that converts the default value to string.
/*
/*	convert_param_node() produces a string representation for
/*	a global parameter default value.
/*
/*	PC_RAW_PARAMETER() returns non-zero if the specified parameter
/*	node represents a "raw parameter". The value of such
/*	parameters must not be scanned for macro names.  Some "raw
/*	parameter" values contain "$" without macros, such as the
/*	smtpd_expansion_filter "safe character" set; and some contain
/*	$name from a private name space, such as forward_path.  Some
/*	"raw parameter" values in postscreen(8) are safe to expand
/*	by one level.  Support for that may be added later.
/*
/*	param_string_buf is a buffer that is initialized on the fly
/*	and that parameter-to-string conversion functions may use for
/*	temporary result storage.
/*
/*	Arguments:
/* .IP size
/*	The initial size of the hash table.
/* .IP table
/*	A hash table for storage of "valid parameter" information.
/* .IP info
/*	A data structure with a name component and a PC_PARAM_NODE
/*	component. Use PC_PARAM_INFO_NAME() and PC_PARAM_INFO_NODE()
/*	to access these components.
/* .IP name
/*	The name of a "valid parameter".
/* .IP flags
/*	PC_PARAM_FLAG_RAW for a "raw parameter", PC_PARAM_FLAG_NONE
/*	otherwise. See the PC_RAW_PARAMETER() discussion above for
/*	discussion of "raw parameter" values.
/* .IP param_data
/*	Information about the parameter value.  Specify PC_PARAM_NO_DATA
/*	if this is not applicable.
/* .IP convert_fn
/*	The function that will be invoked to produce a string
/*	representation of the information in param_data. The function
/*	receives the param_data value as argument.
/* .IP mode
/*	For now, the SHOW_DEFS flag is required.
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

VSTRING *param_string_buf;

/* make_param_node - make node for global parameter table */

PC_PARAM_NODE *make_param_node(int flags, char *param_data,
			               const char *(*convert_fn) (char *))
{
    PC_PARAM_NODE *node;

    node = (PC_PARAM_NODE *) mymalloc(sizeof(*node));
    node->flags = flags;
    node->param_data = param_data;
    node->convert_fn = convert_fn;
    return (node);
}

/* convert_param_node - get actual or default parameter value */

const char *convert_param_node(int mode, const char *name, PC_PARAM_NODE *node)
{
    const char *myname = "convert_param_node";
    const char *value;

    /*
     * One-off initialization.
     */
    if (param_string_buf == 0)
	param_string_buf = vstring_alloc(100);

    /*
     * Sanity check. A null value indicates that a parameter does not have
     * the requested value. At this time, the only requested value can be the
     * default value, and a null pointer value makes no sense here.
     */
    if ((mode & SHOW_DEFS) == 0)
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
