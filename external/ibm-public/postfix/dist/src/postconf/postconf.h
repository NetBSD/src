/*	$NetBSD: postconf.h,v 1.1.1.1.6.2 2013/02/25 00:27:22 tls Exp $	*/

/*++
/* NAME
/*	postconf 3h
/* SUMMARY
/*	module interfaces
/* SYNOPSIS
/*	#include <postconf.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <htable.h>
#include <argv.h>
#include <dict.h>

 /*
  * What we're supposed to be doing.
  */
#define SHOW_NONDEF	(1<<0)		/* show main.cf non-default settings */
#define SHOW_DEFS	(1<<1)		/* show main.cf default setting */
#define SHOW_NAME	(1<<2)		/* show main.cf parameter name */
#define SHOW_MAPS	(1<<3)		/* show map types */
#define EDIT_MAIN	(1<<4)		/* edit main.cf */
#define SHOW_LOCKS	(1<<5)		/* show mailbox lock methods */
#define SHOW_EVAL	(1<<6)		/* expand main.cf right-hand sides */
#define SHOW_SASL_SERV	(1<<7)		/* show server auth plugin types */
#define SHOW_SASL_CLNT	(1<<8)		/* show client auth plugin types */
#define COMMENT_OUT	(1<<9)		/* #-out selected main.cf entries */
#define SHOW_MASTER	(1<<10)		/* show master.cf entries */
#define FOLD_LINE	(1<<11)		/* fold long *.cf entries */

#define DEF_MODE	SHOW_NAME	/* default mode */

 /*
  * Structure for one "valid parameter" (built-in, service-defined or valid
  * user-defined). See the postconf_builtin, postconf_service and
  * postconf_user modules for narrative text.
  */
typedef struct {
    int     flags;			/* see below */
    char   *param_data;			/* mostly, the default value */
    const char *(*convert_fn) (char *);	/* value to string */
} PC_PARAM_NODE;

 /* Values for flags. See the postconf_node module for narrative text. */
#define PC_PARAM_FLAG_RAW	(1<<0)	/* raw parameter value */
#define PC_PARAM_FLAG_BUILTIN	(1<<1)	/* built-in parameter name */
#define PC_PARAM_FLAG_SERVICE	(1<<2)	/* service-defined parameter name */
#define PC_PARAM_FLAG_USER	(1<<3)	/* user-defined parameter name */

#define PC_PARAM_MASK_CLASS \
	(PC_PARAM_FLAG_BUILTIN | PC_PARAM_FLAG_SERVICE | PC_PARAM_FLAG_USER)
#define PC_PARAM_CLASS_OVERRIDE(node, class) \
	((node)->flags = (((node)->flags & ~PC_PARAM_MASK_CLASS) | (class)))

#define PC_RAW_PARAMETER(node) ((node)->flags & PC_PARAM_FLAG_RAW)

 /* Values for param_data. See postconf_node module for narrative text. */
#define PC_PARAM_NO_DATA	((char *) 0)

 /*
  * Lookup table for global "valid parameter" information.
  */
#define PC_PARAM_TABLE			HTABLE
#define PC_PARAM_INFO			HTABLE_INFO

extern PC_PARAM_TABLE *param_table;

 /*
  * postconf_node.c.
  */
#define PC_PARAM_TABLE_CREATE(size)	htable_create(size);
#define PC_PARAM_NODE_CAST(ptr)		((PC_PARAM_NODE *) (ptr))

#define PC_PARAM_TABLE_LIST(table)	htable_list(table)
#define PC_PARAM_INFO_NAME(ht)		((const char *) (ht)->key)
#define PC_PARAM_INFO_NODE(ht)		PC_PARAM_NODE_CAST((ht)->value)

#define PC_PARAM_TABLE_FIND(table, name) \
	PC_PARAM_NODE_CAST(htable_find((table), (name)))
#define PC_PARAM_TABLE_LOCATE(table, name) htable_locate((table), (name))
#define PC_PARAM_TABLE_ENTER(table, name, flags, data, func) \
	htable_enter((table), (name), (char *) make_param_node((flags), \
	    (data), (func)))

PC_PARAM_NODE *make_param_node(int, char *, const char *(*) (char *));
const char *convert_param_node(int, const char *, PC_PARAM_NODE *);
extern VSTRING *param_string_buf;

 /*
  * Structure of one master.cf entry.
  */
typedef struct {
    char   *name_space;			/* service.type, parameter name space */
    ARGV   *argv;			/* null, or master.cf fields */
    DICT   *all_params;			/* null, or all name=value entries */
    HTABLE *valid_names;		/* null, or "valid" parameter names */
} PC_MASTER_ENT;

#define PC_MASTER_MIN_FIELDS	8	/* mandatory field count */

 /*
  * Lookup table for master.cf entries. The table is terminated with an entry
  * that has a null argv member.
  */
PC_MASTER_ENT *master_table;

 /*
  * Line-wrapping support.
  */
#define LINE_LIMIT	80		/* try to fold longer lines */
#define SEPARATORS	" \t\r\n"
#define INDENT_LEN	4		/* indent long text by 4 */
#define INDENT_TEXT	"    "

 /*
  * XXX Global so that postconf_builtin.c call-backs can see it.
  */
extern int cmd_mode;

 /*
  * postconf_misc.c.
  */
extern void set_config_dir(void);

 /*
  * postconf_main.c
  */
extern void read_parameters(void);
extern void set_parameters(void);
extern void show_parameters(int, int, char **);

 /*
  * postconf_edit.c
  */
extern void edit_parameters(int, int, char **);

 /*
  * postconf_master.c.
  */
extern void read_master(int);
extern void show_master(int, char **);

#define WARN_ON_OPEN_ERROR	0
#define FAIL_ON_OPEN_ERROR	1

 /*
  * postconf_builtin.c.
  */
extern void register_builtin_parameters(void);

 /*
  * postconf_service.c.
  */
extern void register_service_parameters(void);

 /*
  * postconf_user.c.
  */
extern void register_user_parameters(void);

 /*
  * postconf_dbms.c
  */
extern void register_dbms_parameters(const char *,
			         const char *(*) (const char *, int, char *),
				             PC_MASTER_ENT *);

 /*
  * postconf_unused.c.
  */
extern void flag_unused_main_parameters(void);
extern void flag_unused_master_parameters(void);

 /*
  * postconf_other.c.
  */
extern void show_maps(void);
extern void show_locks(void);
extern void show_sasl(int);

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
