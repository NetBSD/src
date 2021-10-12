/*	$NetBSD: parse_v2.c,v 1.6 2021/10/12 19:08:04 christos Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by James Browning, Gabe Coffland, Alex Gavin, and Solomon Ritzow.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: parse_v2.c,v 1.6 2021/10/12 19:08:04 christos Exp $");

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <err.h>

#include "inetd.h"
#include "ipsec.h"

typedef enum values_state {
	VALS_PARSING, VALS_END_KEY, VALS_END_DEF, VALS_ERROR
} values_state;

/* Values parsing state */
typedef struct val_parse_info {
	char *cp;
	/* Used so we can null-terminate values by overwriting ',' and ';' */
	//char terminal;
	values_state state;
} val_parse_info, *vlist;

/* The result of a call to parse_invoke_handler */
typedef enum invoke_result {
	INVOKE_SUCCESS, INVOKE_FINISH, INVOKE_ERROR
} invoke_result;

/* The result of a parse of key handler values */
typedef enum hresult {
	KEY_HANDLER_FAILURE, KEY_HANDLER_SUCCESS
} hresult;

/* v2 syntax key-value parsers */
static hresult	args_handler(struct servtab *, vlist);
static hresult	bind_handler(struct servtab *, vlist);
static hresult	exec_handler(struct servtab *, vlist);
static hresult	filter_handler(struct servtab *, vlist);
static hresult	group_handler(struct servtab *, vlist);
static hresult	service_max_handler(struct servtab *, vlist);
static hresult	ip_max_handler(struct servtab *, vlist);
static hresult	protocol_handler(struct servtab *, vlist);
static hresult	recv_buf_handler(struct servtab *, vlist);
static hresult	send_buf_handler(struct servtab *, vlist);
static hresult	socket_type_handler(struct servtab *, vlist);
static hresult	unknown_handler(struct servtab *, vlist);
static hresult	user_handler(struct servtab *, vlist);
static hresult	wait_handler(struct servtab *, vlist);

#ifdef IPSEC
static hresult	ipsec_handler(struct servtab *, vlist);
#endif

static invoke_result	parse_invoke_handler(bool *, char **, struct servtab *);
static bool fill_default_values(struct servtab *);
static bool parse_quotes(char **);
static bool	skip_whitespace(char **);
static int	size_to_bytes(char *);
static bool infer_protocol_ip_version(struct servtab *);
static bool	setup_internal(struct servtab *);
static void	try_infer_socktype(struct servtab *);
static int hex_to_bits(char);
#ifdef IPSEC
static void	setup_ipsec(struct servtab *);
#endif
static inline void	strmove(char *, size_t);

/* v2 Key handlers infrastructure */

/* v2 syntax Handler function, which must parse all values for its key */
typedef hresult (*key_handler_func)(struct servtab *, vlist);

/* List of v2 syntax key handlers */
static struct key_handler {
	const char *name;
	key_handler_func handler;
} key_handlers[] = {
	{ "bind", bind_handler },
	{ "socktype", socket_type_handler },
	{ "acceptfilter", filter_handler },
	{ "protocol", protocol_handler },
	{ "sndbuf", send_buf_handler },
	{ "recvbuf", recv_buf_handler },
	{ "wait", wait_handler },
	{ "service_max", service_max_handler },
	{ "user", user_handler },
	{ "group", group_handler },
	{ "exec", exec_handler },
	{ "args", args_handler },
	{ "ip_max", ip_max_handler },
#ifdef IPSEC
	{ "ipsec", ipsec_handler }
#endif
};

/* Error Not Initialized */
#define ENI(key) ERR("Required option '%s' not specified", (key))

#define WAIT_WRN "Option 'wait' for internal service '%s' was inferred"

/* Too Few Arguemnts (values) */
#define TFA(key) ERR("Option '%s' has too few arguments", (key))

/* Too Many Arguments (values) */
#define TMA(key) ERR("Option '%s' has too many arguments", (key))

/* Too Many Definitions */
#define TMD(key) ERR("Option '%s' is already specified", (key))

#define VALID_SOCKET_TYPES "stream, dgram, rdm, seqpacket, raw"

parse_v2_result
parse_syntax_v2(struct servtab *sep, char **cpp)
{

	/* Catch multiple semantic errors instead of skipping after one */
	bool is_valid_definition = true;
	/* Line number of service for error logging. */
	size_t line_number_start = line_number;

	for (;;) {
		switch(parse_invoke_handler(&is_valid_definition, cpp, sep)) {
		case INVOKE_SUCCESS:
			/* Keep reading more options in. */
			continue;
		case INVOKE_FINISH:
			/*
			 * Found a semicolon, do final checks and defaults
			 * and return.
			 * Skip whitespace after semicolon to end of line.
		         */
			while (isspace((unsigned char)**cpp)) {
				(*cpp)++;
			}

			if (is_valid_definition && fill_default_values(sep)) {
				if (**cpp == '\0') {
					*cpp = nextline(fconfig);
				}
				return V2_SUCCESS;
			}

			DPRINTCONF("Ignoring invalid definition.");
			/* Log the error for the starting line of the service */
			syslog(LOG_ERR, CONF_ERROR_FMT
			    "Ignoring invalid definition.", CONFIG,
			    line_number_start);
			if (**cpp == '\0') {
				*cpp = nextline(fconfig);
			}
			return V2_SKIP;
		case INVOKE_ERROR:
			DPRINTCONF("Syntax error; Exiting '%s'", CONFIG);
			return V2_ERROR;
		}
	}
}

/*
 * Fill in any remaining values that should be inferred
 * Log an error if a required parameter that isn't
 * provided by user can't be inferred from other servtab data.
 * Return true on success, false on failure.
 */
static bool
fill_default_values(struct servtab *sep)
{
	bool is_valid = true;

	if (sep->se_service_max == SERVTAB_UNSPEC_SIZE_T) {
		/* Set default to same as in v1 syntax. */
		sep->se_service_max = TOOMANY;
	}

	if (sep->se_hostaddr == NULL) {
		/* Set hostaddr to default */
		sep->se_hostaddr = newstr(defhost);
	}

	try_infer_socktype(sep);

	if (sep->se_server == NULL) {
		/* If an executable is not specified, assume internal. */
		is_valid = setup_internal(sep) && is_valid;
	}

	if (sep->se_socktype == SERVTAB_UNSPEC_VAL) {
		/* Ensure socktype is specified (either set or inferred) */
		ENI("socktype");
		is_valid = false;
	}

	if (sep->se_wait == SERVTAB_UNSPEC_VAL) {
		/* Ensure wait is specified */
		ENI("wait");
		is_valid = false;
	}

	if (sep->se_user == NULL) {
		/* Ensure user is specified */
		ENI("user");
		is_valid = false;
	}

	if (sep->se_proto == NULL) {
		/* Ensure protocol is specified */
		ENI("protocol");
		is_valid = false;
	} else {
		is_valid = infer_protocol_ip_version(sep) && is_valid;
	}

#ifdef IPSEC
	setup_ipsec(sep);
#endif
	return is_valid;
}

/* fill_default_values related functions */
#ifdef IPSEC
static void
setup_ipsec(struct servtab *sep)
{
	if (sep->se_policy == NULL) {
		/* Set to default global policy */
		sep->se_policy = policy;
	} else if (*sep->se_policy == '\0') {
		/* IPsec was intentionally disabled. */
		free(sep->se_policy);
		sep->se_policy = NULL;
	}
}
#endif

static void
try_infer_socktype(struct servtab *sep) {
	if (sep->se_socktype != SERVTAB_UNSPEC_VAL || sep->se_proto == NULL) {
		return;
	}

	/* Check values of se_proto udp, udp6, tcp, tcp6 to set dgram/stream */
	if (strncmp(sep->se_proto, "udp", 3) == 0) {
		sep->se_socktype = SOCK_DGRAM;
	} else if (strncmp(sep->se_proto, "tcp", 3) == 0) {
		sep->se_socktype = SOCK_STREAM;
	}
}

static bool
setup_internal(struct servtab *sep)
{
	pid_t wait_prev = sep->se_wait;
	if (parse_server(sep, "internal") != 0) {
		ENI("exec");
		return false;
	}

	if (wait_prev != SERVTAB_UNSPEC_VAL && wait_prev != sep->se_wait) {
		/* If wait was already specified throw an error. */
		WRN(WAIT_WRN, sep->se_service);
	}
	return true;
}

static bool
infer_protocol_ip_version(struct servtab *sep)
{
	struct in_addr tmp;

	if (strcmp("tcp", sep->se_proto) != 0
		&& strcmp("udp", sep->se_proto) != 0
		&& strcmp("rpc/tcp", sep->se_proto) != 0
		&& strcmp("rpc/udp", sep->se_proto) != 0) {
		return true;
	}

	if (inet_pton(AF_INET, sep->se_hostaddr, &tmp)) {
		sep->se_family = AF_INET;
		return true;
	}

	if (inet_pton(AF_INET6, sep->se_hostaddr, &tmp)) {
		sep->se_family = AF_INET6;
		return true;
	}

	ERR("Address family of %s is ambigous or invalid. "
		"Explicitly specify protocol", sep->se_hostaddr);
	return false;
}

/*
 * Skips whitespaces, newline characters, and comments,
 * and returns the next token. Returns false and logs error if an EOF is
 * encountered.
 */
static bool
skip_whitespace(char **cpp)
{
	char *cp = *cpp;

	size_t line_start = line_number;

	for (;;) {
		while (isblank((unsigned char)*cp))
			cp++;

		if (*cp == '\0' || *cp == '#') {
			cp = nextline(fconfig);

			/* Should never expect EOF when skipping whitespace */
			if (cp == NULL) {
				ERR("Early end of file after line %zu",
				    line_start);
				return false;
			}
			continue;
		}
		break;
	}

	*cpp = cp;
	return true;
}

/* Get the key handler function pointer for the given name */
static key_handler_func
get_handler(char *name)
{
	/* Call function to handle option parsing. */
	for (size_t i = 0; i < __arraycount(key_handlers); i++) {
		if (strcmp(key_handlers[i].name, name) == 0) {
			return key_handlers[i].handler;
		}
	}
	return NULL;
}

static inline void
strmove(char *buf, size_t off)
{
	memmove(buf, buf + off, strlen(buf + off) + 1);
}

/*
 * Perform an in-place parse of a single-line quoted string
 * with escape sequences. Sets *cpp to the position after the quoted characters.
 * Uses shell-style quote parsing.
 */
static bool
parse_quotes(char **cpp)
{
	char *cp = *cpp;
	char quote = *cp;

	strmove(cp, 1);
	while (*cp != '\0' && quote != '\0') {
		if (*cp == quote) {
			quote = '\0';
			strmove(cp, 1);
			continue;
		}

		if (*cp == '\\') {
			/* start is location of backslash */
			char *start = cp;
			cp++;
			switch (*cp) {
			case 'x': {
				int hi, lo;
				if ((hi = hex_to_bits(cp[1])) == -1
				|| (lo = hex_to_bits(cp[2])) == -1) {
					ERR("Invalid hexcode sequence '%.4s'",
					    start);
					return false;
				}
				*start = (char)((hi << 4) | lo);
				strmove(cp, 3);
				continue;
			}
			case '\\':
				*start = '\\';
				break;
			case 'n':
				*start = '\n';
				break;
			case 't':
				*start = '\t';
				break;
			case 'r':
				*start = '\r';
				break;
			case '\'':
				*start = '\'';
				break;
			case '"':
				*start = '"';
				break;
			case '\0':
				ERR("Dangling escape sequence backslash");
				return false;
			default:
				ERR("Unknown escape sequence '\\%c'", *cp);
				return false;
			}
			strmove(cp, 1);
			continue;
		}

		/* Regular character, advance to the next one. */
		cp++;
	}

	if (*cp == '\0' && quote != '\0') {
		ERR("Unclosed quote");
		return false;
	}
	*cpp = cp;
	return true;
}

static int
hex_to_bits(char in)
{
	switch(in) {
	case '0'...'9':
		return in - '0';
	case 'a'...'f':
		return in - 'a' + 10;
	case 'A'...'F':
		return in - 'A' + 10;
	default:
		return -1;
	}
}

/*
 * Parse the next value for a key handler and advance list->cp past the found
 * value. Return NULL if there are no more values or there was an error
 * during parsing, and set the list->state to the appropriate value.
 */
static char *
next_value(vlist list)
{
	char *cp = list->cp;

	if (list->state != VALS_PARSING) {
		/* Already at the end of a values list, or there was an error.*/
		return NULL;
	}

	if (!skip_whitespace(&cp)) {
		list->state = VALS_ERROR;
		return NULL;
	}

	if (*cp == ',' || *cp == ';') {
		/* Found end of args, but not immediately after value */
		list->state = (*cp == ',' ? VALS_END_KEY : VALS_END_DEF);
		list->cp = cp + 1;
		return NULL;
	}

	/* Check for end of line */
	if (!skip_whitespace(&cp)) {
		list->state = VALS_ERROR;
		return NULL;
	}

	/*
	 * Found the start of a potential value. Advance one character
	 * past the end of the value.
	 */
	char *start = cp;
	while (!isblank((unsigned char)*cp) && *cp != '#' &&
	    *cp != ',' && *cp != ';' && *cp != '\0' ) {
		if (*cp == '"' || *cp == '\'') {
			/* Found a quoted segment */
			if (!parse_quotes(&cp)) {
				list->state = VALS_ERROR;
				return NULL;
			}
		} else {
			/* Find the end of the value */
			cp++;
		}
	}

	/* Handle comments next to unquoted values */
	if (*cp == '#') {
		*cp = '\0';
		list->cp = cp;
		return start;
	}

	if (*cp == '\0') {
		/*
		 * Value ends with end of line, so it is already NUL-terminated
		 */
		list->cp = cp;
		return start;
	}

	if (*cp == ',') {
		list->state = VALS_END_KEY;
	} else if (*cp == ';') {
		list->state = VALS_END_DEF;
	}

	*cp = '\0';
	/* Advance past null so we don't skip the rest of the line */
	list->cp = cp + 1;
	return start;
}

/* Parse key name and invoke associated handler */
static invoke_result
parse_invoke_handler(bool *is_valid_definition, char **cpp, struct servtab *sep)
{
	char *key_name, save, *cp = *cpp;
	int is_blank;
	key_handler_func handler;
	val_parse_info info;

	/* Skip any whitespace if it exists, otherwise do nothing */
	if (!skip_whitespace(&cp)) {
		return INVOKE_ERROR;
	}

	/* Starting character of key */
	key_name = cp;


	/* alphabetical or underscore allowed in name */
	while (isalpha((unsigned char)*cp) || *cp == '_') {
		cp++;
	}

	is_blank = isblank((unsigned char)*cp);

	/* Get key handler and move to start of values */
	if (*cp != '=' && !is_blank && *cp != '#') {
		ERR("Expected '=' but found '%c'", *cp);
		return INVOKE_ERROR;
	}

	save = *cp;
	*cp = '\0';
	cp++;

	handler = get_handler(key_name);

	if (handler == NULL) {
		ERR("Unknown option '%s'", key_name);
		handler = unknown_handler;
	}

	/* If blank or new line, still need to find the '=' or throw error */
	if (is_blank || save == '#') {
		if (save == '#') {
			cp = nextline(fconfig);
		}

		skip_whitespace(&cp);
		if (*cp != '=') {
			ERR("Expected '=' but found '%c'", *cp);
			return INVOKE_ERROR;
		}
		cp++;
	}

	/* Skip whitespace to start of values */
	if (!skip_whitespace(&cp)) {
		return INVOKE_ERROR;
	}

	info = (val_parse_info) {cp, VALS_PARSING};

	/*
	 * Read values for key and write into sep.
	 * If parsing is successful, all values for key must be read.
	 */
	if (handler(sep, &info) == KEY_HANDLER_FAILURE) {
		/*
		 * Eat remaining values if an error happened
	         * so more errors can be caught.
		 */
		while (next_value(&info) != NULL)
			continue;
		*is_valid_definition = false;
	}

	if (info.state == VALS_END_DEF) {
		/*
		 * Exit definition handling for(;;).
		 * Set the position to the end of the definition,
		 * for multi-definition lines.
		 */
		*cpp = info.cp;
		return INVOKE_FINISH;
	}
	if (info.state == VALS_ERROR) {
		/* Parse error, stop reading config */
		return INVOKE_ERROR;
	}

	*cpp = info.cp;
	return INVOKE_SUCCESS;
}

/* Return true if sep must be a built-in service */
static bool
is_internal(struct servtab *sep)
{
	return sep->se_bi != NULL;
}

/*
 * Key-values handlers
 */

static hresult
/*ARGSUSED*/
unknown_handler(struct servtab *sep, vlist values)
{
	/* Return failure for an unknown service name. */
	return KEY_HANDLER_FAILURE;
}

/* Set listen address for this service */
static hresult
bind_handler(struct servtab *sep, vlist values)
{
	if (sep->se_hostaddr != NULL) {
		TMD("bind");
		return KEY_HANDLER_FAILURE;
	}

	char *val = next_value(values);
	sep->se_hostaddr = newstr(val);
	if (next_value(values) != NULL) {
		TMA("bind");
		return KEY_HANDLER_FAILURE;
	}
	return KEY_HANDLER_SUCCESS;
}

static hresult
socket_type_handler(struct servtab *sep, vlist values)
{
	char *type = next_value(values);
	if (type == NULL) {
		TFA("socktype");
		return KEY_HANDLER_FAILURE;
	}

	parse_socktype(type, sep);

	if (sep->se_socktype == -1) {
		ERR("Invalid socket type '%s'. Valid: " VALID_SOCKET_TYPES,
		    type);
		return KEY_HANDLER_FAILURE;
	}

	if (next_value(values) != NULL) {
		TMA("socktype");
		return KEY_HANDLER_FAILURE;
	}

	return KEY_HANDLER_SUCCESS;
}

/* Set accept filter SO_ACCEPTFILTER */
static hresult
filter_handler(struct servtab *sep, vlist values)
{
	/*
	 * See: SO_ACCEPTFILTER https://man.netbsd.org/setsockopt.2
	 * An accept filter can have one other argument.
	 * This code currently only supports one accept filter
	 * Also see parse_accept_filter(char* arg, struct servtab*sep)
	 */

	char *af_name, *af_arg;

	af_name = next_value(values);

	if (af_name == NULL) {
		TFA("filter");
		return KEY_HANDLER_FAILURE;
	}

	/* Store af_name in se_accf.af_name, no newstr call */
	strlcpy(sep->se_accf.af_name, af_name, sizeof(sep->se_accf.af_name));

	af_arg = next_value(values);

	if (af_arg != NULL) {
		strlcpy(sep->se_accf.af_arg, af_arg,
		    sizeof(sep->se_accf.af_arg));
		if (next_value(values) != NULL) {
			TMA("filter");
			return KEY_HANDLER_FAILURE;
		}
	} else {
		/* Store null string */
		sep->se_accf.af_arg[0] = '\0';
	}

	return KEY_HANDLER_SUCCESS;
}

/* Set protocol (udp, tcp, unix, etc.) */
static hresult
protocol_handler(struct servtab *sep, vlist values)
{
	char *val;

	if ((val = next_value(values)) == NULL) {
		TFA("protocol");
		return KEY_HANDLER_FAILURE;
	}

	if (sep->se_type == NORM_TYPE &&
	    strncmp(val, "faith/", strlen("faith/")) == 0) {
		val += strlen("faith/");
		sep->se_type = FAITH_TYPE;
	}
	sep->se_proto = newstr(val);

	if (parse_protocol(sep))
		return KEY_HANDLER_FAILURE;

	if ((val = next_value(values)) != NULL) {
		TMA("protocol");
		return KEY_HANDLER_FAILURE;
	}
	return KEY_HANDLER_SUCCESS;
}

/*
 * Convert a string number possible ending with k or m to an integer.
 * Based on MALFORMED, GETVAL, and ASSIGN in getconfigent(void).
 */
static int
size_to_bytes(char *arg)
{
	char *tail;
	int rstatus, count;

	count = (int)strtoi(arg, &tail, 10, 0, INT_MAX, &rstatus);

	if (rstatus != 0 && rstatus != ENOTSUP) {
		ERR("Invalid buffer size '%s': %s", arg, strerror(rstatus));
		return -1;
	}

	switch(tail[0]) {
	case 'm':
		if (__builtin_smul_overflow((int)count, 1024, &count)) {
			ERR("Invalid buffer size '%s': Result too large", arg);
			return -1;
		}
		/* FALLTHROUGH */
	case 'k':
		if (__builtin_smul_overflow((int)count, 1024, &count)) {
			ERR("Invalid buffer size '%s': Result too large", arg);
			return -1;
		}
		/* FALLTHROUGH */
	case '\0':
		return count;
	default:
		ERR("Invalid buffer size unit prefix");
		return -1;
	}
}

/* sndbuf size */
static hresult
send_buf_handler(struct servtab *sep, vlist values)
{
	char *arg;
	int buffer_size;

	if (ISMUX(sep)) {
		ERR("%s: can't specify buffer sizes for tcpmux services",
			sep->se_service);
		return KEY_HANDLER_FAILURE;
	}


	if ((arg = next_value(values)) == NULL) {
		TFA("sndbuf");
		return KEY_HANDLER_FAILURE;
	}

	buffer_size = size_to_bytes(arg);

	if (buffer_size == -1) {
		return KEY_HANDLER_FAILURE;
	}

	if ((arg = next_value(values)) != NULL) {
		TMA("sndbuf");
		return KEY_HANDLER_FAILURE;
	}

	sep->se_sndbuf = buffer_size;

	return KEY_HANDLER_SUCCESS;
}

/* recvbuf size */
static hresult
recv_buf_handler(struct servtab *sep, vlist values)
{
	char *arg;
	int buffer_size;

	if (ISMUX(sep)) {
		ERR("%s: Cannot specify buffer sizes for tcpmux services",
			sep->se_service);
		return KEY_HANDLER_FAILURE;
	}

	if ((arg = next_value(values)) == NULL){
		TFA("recvbuf");
		return KEY_HANDLER_FAILURE;
	}

	buffer_size = size_to_bytes(arg);

	if (buffer_size == -1) {
		return KEY_HANDLER_FAILURE;
	}

	if ((arg = next_value(values)) != NULL) {
		TMA("recvbuf");
		return KEY_HANDLER_FAILURE;
	}

	sep->se_rcvbuf = buffer_size;

	return KEY_HANDLER_SUCCESS;

}

/* Same as wait in positional */
static hresult
wait_handler(struct servtab *sep, vlist values)
{
	char *val;
	pid_t wait;

	/* If 'wait' is specified after internal exec */

	if (!is_internal(sep) && sep->se_wait != SERVTAB_UNSPEC_VAL) {
		/* Prevent duplicate wait keys */
		TMD("wait");
		return KEY_HANDLER_FAILURE;
	}

	val = next_value(values);

	if (val == NULL) {
		TFA("wait");
		return KEY_HANDLER_FAILURE;
	}

	if (strcmp(val, "yes") == 0) {
		wait = true;
	} else if (strcmp(val, "no") == 0) {
		wait = false;
	} else {
		ERR("Invalid value '%s' for wait. Valid: yes, no", val);
		return KEY_HANDLER_FAILURE;
	}

	if (is_internal(sep) && wait != sep->se_wait) {
		/* If wait was set for internal service check for correctness */
		WRN(WAIT_WRN, sep->se_service);
	} else if (parse_wait(sep, wait)) {
		return KEY_HANDLER_FAILURE;
	}

	if ((val = next_value(values)) != NULL) {
		TMA("wait");
		return KEY_HANDLER_FAILURE;
	}

	return KEY_HANDLER_SUCCESS;
}

/* Set max connections in interval rate-limit, same as max in positional */
static hresult
service_max_handler(struct servtab *sep, vlist values)
{
	char *count_str;
	int rstatus;

	if (sep->se_service_max != SERVTAB_UNSPEC_SIZE_T) {
		TMD("service_max");
		return KEY_HANDLER_FAILURE;
	}

	count_str = next_value(values);

	if (count_str == NULL) {
		TFA("service_max");
		return KEY_HANDLER_FAILURE;
	}

	size_t count = (size_t)strtou(count_str, NULL, 10, 0,
	    SERVTAB_COUNT_MAX, &rstatus);

	if (rstatus != 0) {
		ERR("Invalid service_max '%s': %s", count_str,
		    strerror(rstatus));
		return KEY_HANDLER_FAILURE;
	}

	if (next_value(values) != NULL) {
		TMA("service_max");
		return KEY_HANDLER_FAILURE;
	}

	sep->se_service_max = count;

	return KEY_HANDLER_SUCCESS;
}

static hresult
ip_max_handler(struct servtab *sep, vlist values)
{
	char *count_str;
	int rstatus;

	if (sep->se_ip_max != SERVTAB_UNSPEC_SIZE_T) {
		TMD("ip_max");
		return KEY_HANDLER_FAILURE;
	}

	count_str = next_value(values);

	if (count_str == NULL) {
		TFA("ip_max");
		return KEY_HANDLER_FAILURE;
	}

	size_t count = (size_t)strtou(count_str, NULL, 10, 0,
	    SERVTAB_COUNT_MAX, &rstatus);

	if (rstatus != 0) {
		ERR("Invalid ip_max '%s': %s", count_str, strerror(rstatus));
		return KEY_HANDLER_FAILURE;
	}

	if (next_value(values) != NULL) {
		TMA("ip_max");
		return KEY_HANDLER_FAILURE;
	}

	sep->se_ip_max = count;

	return KEY_HANDLER_SUCCESS;
}

/* Set user to execute as */
static hresult
user_handler(struct servtab *sep, vlist values)
{
	if (sep->se_user != NULL) {
		TMD("user");
		return KEY_HANDLER_FAILURE;
	}

	char *name = next_value(values);

	if (name == NULL) {
		TFA("user");
		return KEY_HANDLER_FAILURE;
	}

	sep->se_user = newstr(name);

	if (next_value(values) != NULL) {
		TMA("user");
		return KEY_HANDLER_FAILURE;
	}

	return KEY_HANDLER_SUCCESS;
}

/* Set group to execute as */
static hresult
group_handler(struct servtab *sep, vlist values)
{
	char *name = next_value(values);

	if (name == NULL) {
		TFA("group");
		return KEY_HANDLER_FAILURE;
	}

	sep->se_group = newstr(name);

	if (next_value(values) != NULL) {
		TMA("group");
		return KEY_HANDLER_FAILURE;
	}

	return KEY_HANDLER_SUCCESS;
}

/* Handle program path or "internal" */
static hresult
exec_handler(struct servtab *sep, vlist values)
{
	char *val;

	if ((val = next_value(values)) == NULL) {
		TFA("exec");
		return KEY_HANDLER_FAILURE;
	}

	pid_t wait_prev = sep->se_wait;
	if (parse_server(sep, val))
		return KEY_HANDLER_FAILURE;
	if (is_internal(sep) && wait_prev != SERVTAB_UNSPEC_VAL) {
		/*
		 * Warn if the user specifies a value for an internal which
		 * is different
		 */
		if (wait_prev != sep->se_wait) {
			WRN(WAIT_WRN, sep->se_service);
		}
	}

	if ((val = next_value(values)) != NULL) {
		TMA("exec");
		return KEY_HANDLER_FAILURE;
	}

	return KEY_HANDLER_SUCCESS;
}

/* Handle program arguments */
static hresult
args_handler(struct servtab *sep, vlist values)
{
	char *val;
	int argc;

	if (sep->se_argv[0] != NULL) {
		TMD("args");
		return KEY_HANDLER_FAILURE;
	}

	argc = 0;
	for (val = next_value(values); val != NULL; val = next_value(values)) {
		if (argc >= MAXARGV) {
			ERR("Must be fewer than " TOSTRING(MAXARGV)
			    " arguments");
			return KEY_HANDLER_FAILURE;
		}
		sep->se_argv[argc++] = newstr(val);
	}
	while (argc <= MAXARGV)
		sep->se_argv[argc++] = NULL;

	return KEY_HANDLER_SUCCESS;

}

#ifdef IPSEC
/*
 * ipsec_handler currently uses the ipsec.h utilities for parsing, requiring
 * all policies as a single value. This handler could potentially allow multiple
 * policies as separate values in the future, but strings would need to be
 * concatenated so the existing ipsec.h functions continue to work and policies
 * can continue to be stored in sep->policy.
 */
static hresult
ipsec_handler(struct servtab *sep, vlist values)
{
	if (sep->se_policy != NULL) {
		TMD("ipsec");
		return KEY_HANDLER_FAILURE;
	}

	char *ipsecstr = next_value(values);

	if (ipsecstr != NULL && ipsecsetup_test(ipsecstr) < 0) {
		ERR("IPsec policy '%s' is invalid", ipsecstr);
		return KEY_HANDLER_FAILURE;
	}

	/*
	 * Use 'ipsec=' with no argument to disable ipsec for this service
	 * An empty string indicates that IPsec was disabled, handled in
	 * fill_default_values.
	 */
	sep->se_policy = policy != NULL ? newstr(ipsecstr) : newstr("");

	if (next_value(values) != NULL) {
		TMA("ipsec");
		/* Currently only one semicolon separated string is allowed */
		return KEY_HANDLER_FAILURE;
	}

	return KEY_HANDLER_SUCCESS;
}
#endif
