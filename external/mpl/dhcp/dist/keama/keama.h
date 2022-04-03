/*	$NetBSD: keama.h,v 1.3 2022/04/03 01:10:59 christos Exp $	*/

/*
 * Copyright (C) 2017-2022 Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   PO Box 360
 *   Newmarket, NH 03857 USA
 *   <info@isc.org>
 *   https://www.isc.org/
 *
 */

#ifndef EOL
#define EOL '\n'
#endif

#include "data.h"
#include "dhctoken.h"

#include <time.h>

/* Resolution of FQDNs into IPv4 addresses */
enum resolve {
	perform = 0,	/* resolve */
	fatal,		/* raise a fatal error */
	pass		/* pass the string wth a warning */
};

extern enum resolve resolve;

/* From includes/dhcp.h */

#define HTYPE_ETHER	1
#define HTYPE_IEEE802	6
#define HTYPE_FDDI	8

#define DHO_DHCP_SERVER_IDENTIFIER		54
#define DHO_VENDOR_CLASS_IDENTIFIER		60
#define DHO_USER_CLASS				77
#define DHO_VIVSO_SUBOPTIONS			125

/* From includes/dhcp6.h */
#define D6O_VENDOR_OPTS				17
#define MAX_V6RELAY_HOPS			32

/* From includes/dhcpd.h */

extern int local_family;

/* A parsing context. */

struct parse {
	int lexline;
	int lexchar;
	char *token_line;
	char *prev_line;
	char *cur_line;
	const char *tlname;
	int eol_token;

	/*
	 * In order to give nice output when we have a parsing error
	 * in our file, we keep track of where we are in the line so
	 * that we can show the user.
	 *
	 * We need to keep track of two lines, because we can look
	 * ahead, via the "peek" function, to the next line sometimes.
	 *
	 * The "line1" and "line2" variables act as buffers for this
	 * information. The "lpos" variable tells us where we are in the
	 * line.
	 *
	 * When we "put back" a character from the parsing context, we
	 * do not want to have the character appear twice in the error
	 * output. So, we set a flag, the "ugflag", which the
	 * get_char() function uses to check for this condition.
	 */
	char line1[81];
	char line2[81];
	int lpos;
	int line;
	int tlpos;
	int tline;
	enum dhcp_token token;
	int ugflag;
	char *tval;
	int tlen;
	char tokbuf[1500];

	int warnings_occurred;
	int file;
	char *inbuf;
	size_t bufix, buflen;
	size_t bufsiz;

	/*
	 * Additions for the Kea Migration Assistant.
	 */
	struct element **stack;
	size_t stack_size;
	size_t stack_top;
	size_t issue_counter;

	/* don't save below this */
	struct comments comments;

	/* TAILQ_NEXT(self) is the saved state */
	TAILQ_ENTRY(parse) next;

};

extern TAILQ_HEAD(parses, parse) parses;

#define PARAMETER	0
#define TOPLEVEL	1
#define	ROOT_GROUP	2
#define HOST_DECL	3
#define SHARED_NET_DECL	4
#define SUBNET_DECL	5
#define CLASS_DECL	6
#define	GROUP_DECL	7
#define POOL_DECL	8

/* Used as an argument to parse_class_decl() */
#define CLASS_TYPE_VENDOR	0
#define CLASS_TYPE_USER		1
#define CLASS_TYPE_CLASS	2
#define CLASS_TYPE_SUBCLASS	3

#define CLASS_DECL_DELETED	1
#define CLASS_DECL_DYNAMIC	2
#define CLASS_DECL_STATIC	4
#define CLASS_DECL_SUBCLASS	8

/* Hardware buffer size */
#define HARDWARE_ADDR_LEN 20

/* Expression context */

enum expression_context {
	context_any, /* indefinite */
	context_boolean,
	context_data,
	context_numeric,
	context_dns,
	context_data_or_numeric, /* indefinite */
	context_function
};

/* Statements */

enum statement_op {
	null_statement,
	if_statement,
	add_statement,
	eval_statement,
	break_statement,
	default_option_statement,
	supersede_option_statement,
	append_option_statement,
	prepend_option_statement,
	send_option_statement,
	statements_statement,
	on_statement,
	switch_statement,
	case_statement,
	default_statement,
	set_statement,
	unset_statement,
	let_statement,
	define_statement,
	log_statement,
	return_statement,
	execute_statement,
	vendor_opt_statement
};

/* Expression tree structure. */

enum expr_op {
	expr_none,
	expr_match,
	expr_check,
	expr_equal,
	expr_substring,
	expr_suffix,
	expr_concat,
	expr_host_lookup,
	expr_and,
	expr_or,
	expr_not,
	expr_option,
	expr_hardware,
	expr_hw_type,		/* derived from expr_hardware */
	expr_hw_address,	/* derived from expr_hardware */
	expr_packet,
	expr_const_data,
	expr_extract_int8,
	expr_extract_int16,
	expr_extract_int32,
	expr_encode_int8,
	expr_encode_int16,
	expr_encode_int32,
	expr_const_int,
	expr_exists,
	expr_encapsulate,
	expr_known,
	expr_reverse,
	expr_leased_address,
	expr_binary_to_ascii,
	expr_config_option,
	expr_host_decl_name,
	expr_pick_first_value,
 	expr_lease_time,
 	expr_dns_transaction,
	expr_static,
	expr_ns_add,
 	expr_ns_delete,
 	expr_ns_exists,
 	expr_ns_not_exists,
	expr_not_equal,
	expr_null,
	expr_variable_exists,
	expr_variable_reference,
	expr_filename,
 	expr_sname,
	expr_arg,
	expr_funcall,
	expr_function,
	expr_add,
	expr_subtract,
	expr_multiply,
	expr_divide,
	expr_remainder,
	expr_binary_and,
	expr_binary_or,
	expr_binary_xor,
	expr_client_state,
	expr_ucase,
	expr_lcase,
	expr_regex_match,
	expr_iregex_match,
	expr_gethostname,
	expr_v6relay,
	expr_concat_dclist
};

/* options */

enum option_status {
	kea_unknown = 0,	/* known only by ISC DHCP */
	isc_dhcp_unknown = 1,	/* known only by Kea */
	known = 2,		/* known by both ISC DHCP and Kea */
	special = 3,		/* requires special processing */
	dynamic = 4		/* dynamic entry */
};

struct option_def {			/* ISC DHCP option definition */
	const char *name;		/* option name */
	const char *format;		/* format string */
	const char *space;		/* space (aka universe) */
	unsigned code;			/* code point */
	enum option_status status;	/* status */
};

struct space_def {			/* ISC DHCP space definition */
	const char *old;		/* ISC DHCP space name */
	const char *name;		/* Kea space name */
	enum option_status status;	/* status */
};

struct space {
	const char *old;		/* ISC DHCP space name */
	const char *name;		/* Kea space name */
	enum option_status status;	/* status */
	struct element *vendor;		/* vendor option */
	TAILQ_ENTRY(space) next;	/* next space */
};

struct option {
	const char *old;		/* ISC DHCP option name */
	const char *name;		/* Kea option name */
	const char *format;		/* ISC DHCP format string */
	const struct space *space;	/* space (aka universe) */
	unsigned code;			/* code point */
	enum option_status status;	/* status */
	TAILQ_ENTRY(option) next;	/* next option */
};

/* Kea parse tools */
void stackPush(struct parse *cfile, struct element *elem);

/* From command line */
extern char *hook_library_path;
extern isc_boolean_t use_isc_lifetimes;
extern isc_boolean_t global_hr;

/* From common/parse.c */
void parse_error(struct parse *, const char *, ...)
	__attribute__((__format__(__printf__,2,3)))
	__attribute__((noreturn));

/* conflex.c */
struct parse *new_parse(int, char *, size_t, const char *, int);
void end_parse(struct parse *);
void save_parse_state(struct parse *);
void restore_parse_state(struct parse *);
enum dhcp_token next_token(const char **, unsigned *, struct parse *);
enum dhcp_token peek_token(const char **, unsigned *, struct parse *);
enum dhcp_token next_raw_token(const char **, unsigned *, struct parse *);
enum dhcp_token peek_raw_token(const char **, unsigned *, struct parse *);
/*
 * Use skip_token when we are skipping a token we have previously
 * used peek_token on as we know what the result will be in this case.
 */
#define skip_token(a,b,c) ((void) next_token((a),(b),(c)))

/* confparse.c */
size_t conf_file_parse(struct parse *);
void read_conf_file(struct parse *, const char *, int);
size_t conf_file_subparse(struct parse *, int);
isc_boolean_t parse_statement(struct parse *, int, isc_boolean_t);
void get_permit(struct parse *, struct element *);
void parse_pool_statement(struct parse *, int);
void parse_lbrace(struct parse *);
void parse_host_declaration(struct parse *);
void parse_class_declaration(struct parse *, int);
void parse_shared_net_declaration(struct parse *);
void parse_subnet_declaration(struct parse *);
void parse_subnet6_declaration(struct parse *);
void parse_group_declaration(struct parse *);
void close_group(struct parse *, struct element *);
struct element *parse_fixed_addr_param(struct parse *, enum dhcp_token);
void parse_address_range(struct parse *, int, size_t);
void parse_address_range6(struct parse *, int, size_t);
void parse_prefix6(struct parse *, int, size_t);
void parse_fixed_prefix6(struct parse *, size_t);
void parse_pool6_statement(struct parse *, int);
struct element *parse_allow_deny(struct parse *, int);
void parse_server_duid_conf(struct parse *);
void parse_directive(struct parse *);
void parse_option_space_dir(struct parse *);
void parse_option_code_dir(struct parse *, struct option *);
void parse_option_status_dir(struct parse *, struct option *, enum dhcp_token);
void parse_option_local_dir(struct parse *, struct option *);
void parse_option_define_dir(struct parse *, struct option *);

/* parse.c */
void skip_to_semi(struct parse *);
void skip_to_rbrace(struct parse *, int);
void parse_semi(struct parse *);
void parse_string(struct parse *, char **, unsigned *);
struct string *parse_host_name(struct parse *);
struct string *parse_ip_addr_or_hostname(struct parse *, isc_boolean_t);
struct string *parse_ip_addr(struct parse *);
struct string *parse_ip6_addr(struct parse *);
struct string *parse_ip6_addr_txt(struct parse *);
struct element *parse_hardware_param(struct parse *);
void parse_lease_time(struct parse *, time_t *);
struct string *parse_numeric_aggregate(struct parse *,
				       unsigned char *, unsigned *,
				       int, int, unsigned);
void convert_num(struct parse *, unsigned char *, const char *,
		 int, unsigned);
struct option *parse_option_name(struct parse *, isc_boolean_t,
				 isc_boolean_t *);
void parse_option_space_decl(struct parse *);
void parse_option_code_definition(struct parse *, struct option *);
void parse_vendor_code_definition(struct parse *, struct option *);
struct string *convert_format(const char *, isc_boolean_t *, isc_boolean_t *);
struct string *parse_base64(struct parse *);
struct string *parse_cshl(struct parse *);
struct string *parse_hexa(struct parse *);
isc_boolean_t parse_executable_statements(struct element *,
					  struct parse *, isc_boolean_t *,
					  enum expression_context);
isc_boolean_t parse_executable_statement(struct element *,
					 struct parse *, isc_boolean_t *,
					 enum expression_context,
					 isc_boolean_t);
isc_boolean_t parse_zone(struct element *, struct parse *);
isc_boolean_t parse_key(struct element *, struct parse *);
isc_boolean_t parse_on_statement(struct element *, struct parse *,
				 isc_boolean_t *);
isc_boolean_t parse_switch_statement(struct element *, struct parse *,
				     isc_boolean_t *);
isc_boolean_t parse_case_statement(struct element *, struct parse *,
				   isc_boolean_t *, enum expression_context);
isc_boolean_t parse_if_statement(struct element *, struct parse *,
				 isc_boolean_t *);
isc_boolean_t parse_boolean_expression(struct element *, struct parse *,
				       isc_boolean_t *);
/* currently unused */
isc_boolean_t parse_boolean(struct parse *);
isc_boolean_t parse_data_expression(struct element *, struct parse *,
				    isc_boolean_t *);
isc_boolean_t numeric_expression(struct element *, struct parse *,
				 isc_boolean_t *);
isc_boolean_t parse_non_binary(struct element *, struct parse *,
			       isc_boolean_t *, enum expression_context);
isc_boolean_t parse_expression(struct element *, struct parse *,
			       isc_boolean_t *, enum expression_context,
			       struct element *, enum expr_op);
struct string *escape_option_string(unsigned, const char *,
				   isc_boolean_t *, isc_boolean_t *);
isc_boolean_t parse_option_data(struct element *, struct parse *,
				struct option *);
isc_boolean_t parse_option_binary(struct element *, struct parse *,
				  struct option *, isc_boolean_t);
struct string * parse_option_textbin(struct parse *, struct option *);
isc_boolean_t parse_option_statement(struct element *, struct parse *,
				     struct option *, enum statement_op);
isc_boolean_t parse_config_data(struct element *, struct parse *,
				struct option *);
isc_boolean_t parse_config_statement(struct element *, struct parse *,
				     struct option *, enum statement_op);
struct string *parse_option_token(struct parse *, const char *,
				  isc_boolean_t *, isc_boolean_t *,
				  isc_boolean_t *);
struct string *parse_option_token_binary(struct parse *, const char *);
struct string *parse_domain_list(struct parse *, isc_boolean_t);
isc_boolean_t is_boolean_expression(struct element *);
isc_boolean_t is_data_expression(struct element *);
isc_boolean_t is_numeric_expression(struct element *);
int expr_precedence(enum expr_op, struct element *);

/* options.c */
void spaces_init(void);
void options_init(void);
struct space *space_lookup(const char *);
struct option *option_lookup_name(const char *, const char *);
struct option *kea_lookup_name(const char *, const char *);
struct option *option_lookup_code(const char *, unsigned);
void push_space(struct space *);
void push_option(struct option *);
void add_option_data(struct element *, struct element *);
void merge_option_data(struct element *, struct element *);
struct comments *get_config_comments(unsigned);
const char *display_status(enum option_status);

/* json.c */
struct element *json_parse(struct parse *);
struct element *json_list_parse(struct parse *);
struct element *json_map_parse(struct parse *);

/* print.c */
const char *print_expression(struct element *, isc_boolean_t *);
const char *print_boolean_expression(struct element *, isc_boolean_t *);
const char *print_data_expression(struct element *, isc_boolean_t *);
const char *print_numeric_expression(struct element *, isc_boolean_t *);

/* reduce.c */
struct element *reduce_boolean_expression(struct element *);
struct element *reduce_data_expression(struct element *);
struct element *reduce_numeric_expression(struct element *);

/* eval */
struct element *eval_expression(struct element *, isc_boolean_t *);
struct element *eval_boolean_expression(struct element *, isc_boolean_t *);
struct element *eval_data_expression(struct element *, isc_boolean_t *);
struct element *eval_numeric_expression(struct element *, isc_boolean_t *);
