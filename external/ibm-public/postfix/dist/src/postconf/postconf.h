/*	$NetBSD: postconf.h,v 1.1.1.2.2.1 2014/08/10 07:12:49 tls Exp $	*/

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
  * System library.
  */
#include <unistd.h>

 /*
  * Utility library.
  */
#include <htable.h>
#include <argv.h>
#include <dict.h>
#include <name_code.h>

 /*
  * What we're supposed to be doing.
  */
#define PCF_SHOW_NONDEF		(1<<0)	/* show main.cf non-default settings */
#define PCF_SHOW_DEFS		(1<<1)	/* show main.cf default setting */
#define PCF_HIDE_NAME		(1<<2)	/* hide main.cf parameter name */
#define PCF_SHOW_MAPS		(1<<3)	/* show map types */
#define PCF_EDIT_CONF		(1<<4)	/* edit main.cf or master.cf */
#define PCF_SHOW_LOCKS		(1<<5)	/* show mailbox lock methods */
#define PCF_SHOW_EVAL		(1<<6)	/* expand main.cf right-hand sides */
#define PCF_SHOW_SASL_SERV	(1<<7)	/* show server auth plugin types */
#define PCF_SHOW_SASL_CLNT	(1<<8)	/* show client auth plugin types */
#define PCF_COMMENT_OUT		(1<<9)	/* #-out selected main.cf entries */
#define PCF_MASTER_ENTRY	(1<<10)	/* manage master.cf entries */
#define PCF_FOLD_LINE		(1<<11)	/* fold long *.cf entries */
#define PCF_EDIT_EXCL		(1<<12)	/* exclude main.cf entries */
#define PCF_MASTER_FLD		(1<<13)	/* hierarchical pathname */
#define PCF_MAIN_PARAM		(1<<14)	/* manage main.cf entries */
#define PCF_EXP_DSN_TEMPL	(1<<15)	/* expand bounce templates */
#define PCF_PARAM_CLASS		(1<<16)	/* select parameter class */
#define PCF_MAIN_OVER		(1<<17)	/* override parameter values */
#define PCF_DUMP_DSN_TEMPL	(1<<18)	/* show bounce templates */
#define PCF_MASTER_PARAM	(1<<19)	/* manage master.cf -o name=value */

#define PCF_DEF_MODE	0

 /*
  * Structure for one "valid parameter" (built-in, service-defined or valid
  * user-defined). See the postconf_builtin, postconf_service and
  * postconf_user modules for narrative text.
  */
typedef struct {
    int     flags;			/* see below */
    char   *param_data;			/* mostly, the default value */
    const char *(*convert_fn) (char *);	/* value to string */
} PCF_PARAM_NODE;

 /* Values for flags. See the postconf_node module for narrative text. */
#define PCF_PARAM_FLAG_RAW	(1<<0)	/* raw parameter value */
#define PCF_PARAM_FLAG_BUILTIN	(1<<1)	/* built-in parameter name */
#define PCF_PARAM_FLAG_SERVICE	(1<<2)	/* service-defined parameter name */
#define PCF_PARAM_FLAG_USER	(1<<3)	/* user-defined parameter name */
#define PCF_PARAM_FLAG_LEGACY	(1<<4)	/* legacy parameter name */
#define PCF_PARAM_FLAG_READONLY	(1<<5)	/* legacy parameter name */
#define PCF_PARAM_FLAG_DBMS	(1<<6)	/* dbms-defined parameter name */

#define PCF_PARAM_MASK_CLASS \
	(PCF_PARAM_FLAG_BUILTIN | PCF_PARAM_FLAG_SERVICE | PCF_PARAM_FLAG_USER)
#define PCF_PARAM_CLASS_OVERRIDE(node, class) \
	((node)->flags = (((node)->flags & ~PCF_PARAM_MASK_CLASS) | (class)))

#define PCF_RAW_PARAMETER(node) ((node)->flags & PCF_PARAM_FLAG_RAW)
#define PCF_LEGACY_PARAMETER(node) ((node)->flags & PCF_PARAM_FLAG_LEGACY)
#define PCF_READONLY_PARAMETER(node) ((node)->flags & PCF_PARAM_FLAG_READONLY)
#define PCF_DBMS_PARAMETER(node) ((node)->flags & PCF_PARAM_FLAG_DBMS)

 /* Values for param_data. See postconf_node module for narrative text. */
#define PCF_PARAM_NO_DATA	((char *) 0)

 /*
  * Lookup table for global "valid parameter" information.
  */
#define PCF_PARAM_TABLE			HTABLE
#define PCF_PARAM_INFO			HTABLE_INFO

extern PCF_PARAM_TABLE *pcf_param_table;

 /*
  * postconf_node.c.
  */
#define PCF_PARAM_TABLE_CREATE(size)	htable_create(size);
#define PCF_PARAM_NODE_CAST(ptr)	((PCF_PARAM_NODE *) (ptr))

#define PCF_PARAM_TABLE_LIST(table)	htable_list(table)
#define PCF_PARAM_INFO_NAME(ht)		((const char *) (ht)->key)
#define PCF_PARAM_INFO_NODE(ht)		PCF_PARAM_NODE_CAST((ht)->value)

#define PCF_PARAM_TABLE_FIND(table, name) \
	PCF_PARAM_NODE_CAST(htable_find((table), (name)))
#define PCF_PARAM_TABLE_LOCATE(table, name) htable_locate((table), (name))
#define PCF_PARAM_TABLE_ENTER(table, name, flags, data, func) \
	htable_enter((table), (name), (char *) pcf_make_param_node((flags), \
	    (data), (func)))

extern PCF_PARAM_NODE *pcf_make_param_node(int, char *, const char *(*) (char *));
extern const char *pcf_convert_param_node(int, const char *, PCF_PARAM_NODE *);
extern VSTRING *pcf_param_string_buf;

 /*
  * Structure of one master.cf entry.
  */
typedef struct {
    char   *name_space;			/* service/type, parameter name space */
    ARGV   *argv;			/* null, or master.cf fields */
    DICT   *all_params;			/* null, or all name=value entries */
    HTABLE *valid_names;		/* null, or "valid" parameter names */
} PCF_MASTER_ENT;

#define PCF_MASTER_MIN_FIELDS	8	/* mandatory field minimum */

#define PCF_MASTER_NAME_SERVICE	"service"
#define PCF_MASTER_NAME_TYPE	"type"
#define PCF_MASTER_NAME_PRIVATE	"private"
#define PCF_MASTER_NAME_UNPRIV	"unprivileged"
#define PCF_MASTER_NAME_CHROOT	"chroot"
#define PCF_MASTER_NAME_WAKEUP	"wakeup"
#define PCF_MASTER_NAME_MAXPROC	"process_limit"
#define PCF_MASTER_NAME_CMD	"command"

#define PCF_MASTER_FLD_SERVICE	0	/* service name */
#define PCF_MASTER_FLD_TYPE	1	/* service type */
#define PCF_MASTER_FLD_PRIVATE	2	/* private service */
#define PCF_MASTER_FLD_UNPRIV	3	/* unprivileged service */
#define PCF_MASTER_FLD_CHROOT	4	/* chrooted service */
#define PCF_MASTER_FLD_WAKEUP	5	/* wakeup timer */
#define PCF_MASTER_FLD_MAXPROC	6	/* process limit */
#define PCF_MASTER_FLD_CMD	7	/* command */

#define PCF_MASTER_FLD_WILDC	-1	/* wild-card */
#define PCF_MASTER_FLD_NONE	-2	/* not available */

 /*
  * Lookup table for master.cf entries. The table is terminated with an entry
  * that has a null argv member.
  */
PCF_MASTER_ENT *pcf_master_table;

 /*
  * Line-wrapping support.
  */
#define PCF_LINE_LIMIT	80		/* try to fold longer lines */
#define PCF_SEPARATORS	" \t\r\n"
#define PCF_INDENT_LEN	4		/* indent long text by 4 */
#define PCF_INDENT_TEXT	"    "

 /*
  * XXX Global so that postconf_builtin.c call-backs can see it.
  */
extern int pcf_cmd_mode;

 /*
  * postconf_misc.c.
  */
extern void pcf_set_config_dir(void);

 /*
  * postconf_main.c
  */
extern void pcf_read_parameters(void);
extern void pcf_set_parameters(char **);
extern void pcf_show_parameters(VSTREAM *, int, int, char **);

 /*
  * postconf_edit.c
  */
extern void pcf_edit_main(int, int, char **);
extern void pcf_edit_master(int, int, char **);

 /*
  * postconf_master.c.
  */
extern const char pcf_daemon_options_expecting_value[];
extern void pcf_read_master(int);
extern void pcf_show_master_entries(VSTREAM *, int, int, char **);
extern const char *pcf_parse_master_entry(PCF_MASTER_ENT *, const char *);
extern void pcf_print_master_entry(VSTREAM *, int, PCF_MASTER_ENT *);
extern void pcf_free_master_entry(PCF_MASTER_ENT *);
extern void pcf_show_master_fields(VSTREAM *, int, int, char **);
extern void pcf_edit_master_field(PCF_MASTER_ENT *, int, const char *);
extern void pcf_show_master_params(VSTREAM *, int, int, char **);
extern void pcf_edit_master_param(PCF_MASTER_ENT *, int, const char *, const char *);

#define PCF_WARN_ON_OPEN_ERROR	0
#define PCF_FAIL_ON_OPEN_ERROR	1

#define PCF_MASTER_BLANKS	" \t\r\n"	/* XXX */

 /*
  * Master.cf parameter namespace management. The idea is to manage master.cf
  * "-o name=value" settings with other tools than text editors.
  * 
  * The natural choice is to use "service-name.service-type.parameter-name", but
  * unfortunately the '.' may appear in service and parameter names.
  * 
  * For example, a spawn(8) listener can have a service name 127.0.0.1:10028.
  * This service name becomes part of a service-dependent parameter name
  * "127.0.0.1:10028_time_limit". All those '.' characters mean we can't use
  * '.' as the parameter namespace delimiter.
  * 
  * (We could require that such service names are specified as $foo:port with
  * the value of "foo" defined in main.cf or at the top of master.cf.)
  * 
  * But it is easier if we use '/' instead.
  */
#define PCF_NAMESP_SEP_CH	'/'
#define PCF_NAMESP_SEP_STR	"/"

#define PCF_LEGACY_SEP_CH	'.'

 /*
  * postconf_match.c.
  */
#define PCF_MATCH_WILDC_STR	"*"
#define PCF_MATCH_ANY(p)		((p)[0] == PCF_MATCH_WILDC_STR[0] && (p)[1] == 0)
#define PCF_MATCH_STRING(p, s)	(PCF_MATCH_ANY(p) || strcmp((p), (s)) == 0)

extern ARGV *pcf_parse_service_pattern(const char *, int, int);
extern int pcf_parse_field_pattern(const char *);

#define PCF_IS_MAGIC_SERVICE_PATTERN(pat) \
    (PCF_MATCH_ANY((pat)->argv[0]) || PCF_MATCH_ANY((pat)->argv[1]))
#define PCF_MATCH_SERVICE_PATTERN(pat, name, type) \
    (PCF_MATCH_STRING((pat)->argv[0], (name)) \
	&& PCF_MATCH_STRING((pat)->argv[1], (type)))

#define pcf_is_magic_field_pattern(pat) ((pat) == PCF_MASTER_FLD_WILDC)
#define pcf_str_field_pattern(pat) ((const char *) (pcf_field_name_offset[pat].name))

#define PCF_IS_MAGIC_PARAM_PATTERN(pat) PCF_MATCH_ANY(pat)
#define PCF_MATCH_PARAM_PATTERN(pat, name) PCF_MATCH_STRING((pat), (name))

/* The following is not part of the postconf_match API. */
extern NAME_CODE pcf_field_name_offset[];

 /*
  * postconf_builtin.c.
  */
extern void pcf_register_builtin_parameters(const char *, pid_t);

 /*
  * postconf_service.c.
  */
extern void pcf_register_service_parameters(void);

 /*
  * Parameter context structure.
  */
typedef struct {
    PCF_MASTER_ENT *local_scope;
    int     param_class;
} PCF_PARAM_CTX;

 /*
  * postconf_user.c.
  */
extern void pcf_register_user_parameters(void);

 /*
  * postconf_dbms.c
  */
extern void pcf_register_dbms_parameters(const char *,
	              const char *(*) (const char *, int, PCF_MASTER_ENT *),
					         PCF_MASTER_ENT *);

 /*
  * postconf_lookup.c.
  */
extern const char *pcf_lookup_parameter_value(int, const char *,
					              PCF_MASTER_ENT *,
					              PCF_PARAM_NODE *);

extern char *pcf_expand_parameter_value(VSTRING *, int, const char *,
					        PCF_MASTER_ENT *);

 /*
  * postconf_print.c.
  */
extern void pcf_print_line(VSTREAM *, int, const char *,...);

 /*
  * postconf_unused.c.
  */
extern void pcf_flag_unused_main_parameters(void);
extern void pcf_flag_unused_master_parameters(void);

 /*
  * postconf_other.c.
  */
extern void pcf_show_maps(void);
extern void pcf_show_locks(void);
extern void pcf_show_sasl(int);

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
