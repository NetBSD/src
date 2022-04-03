/*	$NetBSD: reduce.c,v 1.1.1.2 2022/04/03 01:08:42 christos Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: reduce.c,v 1.1.1.2 2022/04/03 01:08:42 christos Exp $");

#include "keama.h"

#include <sys/errno.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static struct element *reduce_equal_expression(struct element *left,
					       struct element *right);
static void debug(const char* fmt, ...);

/*
 * boolean_expression :== CHECK STRING |
 *                        NOT boolean-expression |
 *                        data-expression EQUAL data-expression |
 *                        data-expression BANG EQUAL data-expression |
 *                        data-expression REGEX_MATCH data-expression |
 *                        boolean-expression AND boolean-expression |
 *                        boolean-expression OR boolean-expression
 *                        EXISTS OPTION-NAME
 */

struct element *
reduce_boolean_expression(struct element *expr)
{
	/* trivial case: already done */
	if (expr->type == ELEMENT_BOOLEAN)
		return expr;

	/*
	 * From is_boolean_expression
	 */

	if (expr->type != ELEMENT_MAP)
		return NULL;

	/* check */
	if (mapContains(expr, "check"))
		/*
		 * syntax := { "check": <collection_name> }
		 * semantic: check_collection
		 *  on server try to match classes of the collection
		 */
		return NULL;


	/* exists */
	if (mapContains(expr, "exists")) {
		/*
		 * syntax := { "exists":
		 *             { "universe": <option_space_old>,
		 *               "name":  <option_name> }
		 *           }
		 * semantic: check universe/code from incoming packet
		 */
		struct element *arg;
		struct element *universe;
		struct element *name;
		struct option *option;
		char result[80];

		arg = mapGet(expr, "exists");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			debug("can't get exists argument");
			return NULL;
		}
		universe = mapGet(arg, "universe");
		if ((universe == NULL) || (universe->type != ELEMENT_STRING)) {
			debug("can't get exists option universe");
			return NULL;
		}
		name = mapGet(arg, "name");
		if ((name == NULL) || (name->type != ELEMENT_STRING)) {
			debug("can't get exists option name");
			return NULL;
		}
		option = option_lookup_name(stringValue(universe)->content,
					    stringValue(name)->content);
		if ((option == NULL) || (option->code == 0))
			return NULL;
		if (((local_family == AF_INET) &&
		     (strcmp(option->space->name, "dhcp4") != 0)) ||
		    ((local_family == AF_INET6) &&
		     (strcmp(option->space->name, "dhcp6") != 0)))
			return NULL;
		snprintf(result, sizeof(result),
			 "option[%u].exists", option->code);
		return createString(makeString(-1, result));
	}

	/* variable-exists */
	if (mapContains(expr, "variable-exists"))
		/*
		 * syntax := { "variable-exists": <variable_name> }
		 * semantics: find_binding(scope, name)
		 */
		return NULL;

	/* equal */
	if (mapContains(expr, "equal")) {
		/*
		 * syntax := { "equal":
		 *             { "left":  <expression>,
		 *               "right": <expression> }
		 *           }
		 * semantics: evaluate branches and return true
		 * if same type and same value
		 */
		struct element *arg;
		struct element *left;
		struct element *right;

		arg = mapGet(expr, "equal");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			debug("can't get equal argument");
			return NULL;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			debug("can't get equal left branch");
			return NULL;
		}
		right = mapGet(arg, "right");
		if (right == NULL) {
			debug("can't get equal right branch");
			return NULL;
		}
		return reduce_equal_expression(left, right);
	}

	/* not-equal */
	if (mapContains(expr, "not-equal")) {
		/*
		 * syntax := { "not-equal":
		 *             { "left":  <expression>,
                 *               "right": <expression> }
                 *           }
                 * semantics: evaluate branches and return true
                 * if different type or different value
                 */
		struct element *arg;
		struct element *left;
		struct element *right;
		struct element *equal;
		struct string *result;

		arg = mapGet(expr, "not-equal");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			debug("can't get not-equal argument");
			return NULL;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			debug("can't get not-equal left branch");
			return NULL;
		}
		right = mapGet(arg, "right");
		if (right == NULL) {
			debug("can't get not-equal right branch");
			return NULL;
		}
		equal = reduce_equal_expression(left, right);
		if ((equal == NULL) || (equal->type != ELEMENT_STRING))
			return NULL;
		result = makeString(-1, "not (");
		concatString(result, stringValue(equal));
		appendString(result, ")");
		return createString(result);
	}

	/* regex-match */
	if (mapContains(expr, "regex-match"))
		/*
		 * syntax := { "regex-match":
		 *             { "left":  <data_expression>,
		 *               "right": <data_expression> }
		 *           }
		 * semantics: evaluate branches, compile right as a
		 * regex and apply it to left
		 */
		return NULL;

	/* iregex-match */
	if (mapContains(expr, "iregex-match"))
		/*
		 * syntax := { "regex-match":
		 *             { "left":  <data_expression>,
		 *               "right": <data_expression> }
		 *           }
		 * semantics: evaluate branches, compile right as a
		 * case insensistive regex and apply it to left
		 */
		return NULL;

	/* and */
	if (mapContains(expr, "and")) {
		/*
		 * syntax := { "and":
		 *             { "left":  <boolean_expression>,
		 *               "right": <boolean_expression> }
		 *           }
		 * semantics: evaluate branches, return true
		 * if both are true
		 */
		struct element *arg;
		struct element *left;
		struct element *right;
		struct string *result;

		arg = mapGet(expr, "and");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			debug("can't get and argument");
			return NULL;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			debug("can't get and left branch");
			return NULL;
		}
		right = mapGet(arg, "right");
		if (right == NULL) {
			debug("can't get and right branch");
			return NULL;
		}
		left = reduce_boolean_expression(left);
		if ((left == NULL) || (left->type != ELEMENT_STRING))
			return NULL;
		right = reduce_boolean_expression(right);
		if ((right == NULL) || (right->type != ELEMENT_STRING))
			return NULL;
		result = makeString(-1, "(");
		concatString(result, stringValue(left));
		appendString(result, ") and (");
		concatString(result, stringValue(right));
		appendString(result, ")");
		return createString(result);
	}

	/* or */
	if (mapContains(expr, "or")) {
		/*
		 * syntax := { "or":
		 *             { "left":  <boolean_expression>,
		 *               "right": <boolean_expression> }
		 *           }
		 * semantics: evaluate branches, return true
		 * if any is true
		 */
		struct element *arg;
		struct element *left;
		struct element *right;
		struct string *result;

		arg = mapGet(expr, "or");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			debug("can't get or argument");
			return NULL;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			debug("can't get or left branch");
			return NULL;
		}
		right = mapGet(arg, "right");
		if (right == NULL) {
			debug("can't get or right branch");
			return NULL;
		}
		left = reduce_boolean_expression(left);
		if ((left == NULL) || (left->type != ELEMENT_STRING))
			return NULL;
		right = reduce_boolean_expression(right);
		if ((right == NULL) || (right->type != ELEMENT_STRING))
			return NULL;
		result = makeString(-1, "(");
		concatString(result, stringValue(left));
		appendString(result, ") or (");
		concatString(result, stringValue(right));
		appendString(result, ")");
		return createString(result);
	}

	/* not */
	if (mapContains(expr, "not")) {
		/*
		 * syntax := { "not": <boolean_expression> }
		 * semantic: evaluate its branch and return its negation
		 */
		struct element *arg;
		struct string *result;

		arg = mapGet(expr, "not");
		if (arg == NULL) {
			debug("can't get not argument");
			return NULL;
		}
		arg = reduce_boolean_expression(arg);
		if ((arg == NULL) || (arg->type != ELEMENT_STRING))
			return NULL;
		result = makeString(-1, "not (");
		concatString(result, stringValue(arg));
		appendString(result, ")");
		return createString(result);
	}

	/* known */
	if (mapContains(expr, "known"))
		/*
		 * syntax := { "known": null }
		 * semantics: client is known, i.e., has a matching
		 * host declaration (aka reservation in Kea)
		 */
		return NULL;

	/* static */
	if (mapContains(expr, "static"))
		/*
		 * syntax := { "static": null }
		 * semantics: lease is static (doesn't exist in Kea)
		 */
		return NULL;

	return NULL;
}

/*
 * data_expression :== SUBSTRING LPAREN data-expression COMMA
 *                                      numeric-expression COMMA
 *                                      numeric-expression RPAREN |
 *                     CONCAT LPAREN data-expression COMMA
 *                                      data-expression RPAREN
 *                     SUFFIX LPAREN data_expression COMMA
 *                                   numeric-expression RPAREN |
 *                     LCASE LPAREN data_expression RPAREN |
 *                     UCASE LPAREN data_expression RPAREN |
 *                     OPTION option_name |
 *                     HARDWARE |
 *                     PACKET LPAREN numeric-expression COMMA
 *                                   numeric-expression RPAREN |
 *                     V6RELAY LPAREN numeric-expression COMMA
 *                                    data-expression RPAREN |
 *                     STRING |
 *                     colon_separated_hex_list
 */

struct element *
reduce_data_expression(struct element *expr)
{
	/* trivial case: already done */
	if (expr->type == ELEMENT_STRING)
		return expr;

	/*
	 * From is_data_expression
	 */

	if (expr->type != ELEMENT_MAP)
		return NULL;

	/* substring */
	if (mapContains(expr, "substring")) {
		/*
		 * syntax := { "substring":
		 *             { "expression": <data_expression>,
		 *               "offset":     <numeric_expression>,
		 *               "length":     <numeric_expression> }
		 *           }
		 * semantic: evaluate arguments, if the string is
		 * shorter than offset return "" else return substring
		 */
		struct element *arg;
		struct element *string;
		struct element *offset;
		struct element *length;
		struct string *result;
		int64_t off;
		int64_t len;
		char buf[80];

		arg = mapGet(expr, "substring");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			debug("can't get substring argument");
			return NULL;
		}
		string = mapGet(arg, "expression");
		if (string == NULL) {
			debug("can't get substring expression");
			return NULL;
		}
		offset = mapGet(arg, "offset");
		if (offset  == NULL) {
			debug("can't get substring offset");
			return NULL;
		}
		length = mapGet(arg, "length");
		if (length  == NULL) {
			debug("can't get substring length");
			return NULL;
		}
		/* can't be a literal as it was evaluated before */
		string = reduce_data_expression(string);
		if ((string == NULL) || (string->type != ELEMENT_STRING))
			return NULL;
		offset = reduce_numeric_expression(offset);
		if ((offset == NULL) || (offset->type != ELEMENT_INTEGER))
			return NULL;
		off = intValue(offset);
		if (off < 0) {
			debug("substring with a negative offset (%lld)",
			      (long long)off);
			return NULL;
		}
		length = reduce_numeric_expression(length);
		if ((length == NULL) || (length->type != ELEMENT_INTEGER))
			return NULL;
		len = intValue(length);
		if (len < 0) {
			debug("substring with a negative length (%lld)",
			      (long long)len);
			return NULL;
		}
		result = makeString(-1, "substring(");
		concatString(result, stringValue(string));
		snprintf(buf, sizeof(buf),
			 ",%u,%u)", (unsigned)off, (unsigned)len);
		appendString(result, buf);
		return createString(result);
	}

	/* suffix */
	if (mapContains(expr, "suffix")) {
		/*
		 * syntax := { "suffix":
		 *             { "expression": <data_expression>,
		 *               "length":     <numeric_expression> }
		 *           }
		 * semantic: evaluate arguments, if the string is
		 * shorter than length return it else return suffix
		 */
		struct element *arg;
		struct element *string;
		struct element *length;
		struct string *result;
		int64_t len;
		char buf[80];

		arg = mapGet(expr, "suffix");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			debug("can't get suffix argument");
			return NULL;
		}
		string = mapGet(arg, "expression");
		if (string == NULL) {
			debug("can't get suffix expression");
			return NULL;
		}
		length = mapGet(arg, "length");
		if (length  == NULL) {
			debug("can't get suffix length");
			return NULL;
		}
		/* can't be a literal as it was evaluated before */
		string = reduce_data_expression(string);
		if ((string == NULL) || (string->type != ELEMENT_STRING))
			return NULL;
		length = reduce_numeric_expression(length);
		if ((length == NULL) || (length->type != ELEMENT_INTEGER))
			return NULL;
		len = intValue(length);
		if (len < 0) {
			debug("suffix with a negative length (%lld)",
			      (long long)len);
			return NULL;
		}
		result = makeString(-1, "substring(");
		concatString(result, stringValue(string));
		snprintf(buf, sizeof(buf), ",-%u,all)", (unsigned)len);
		appendString(result, buf);
		return createString(result);
	}

	/* lowercase */
	if (mapContains(expr, "lowercase"))
		/*
		 * syntax := { "lowercase": <data_expression> }
		 * semantic: evaluate its argument and apply tolower to
		 * its content
		 */
		return NULL;

	/* uppercase */
	if (mapContains(expr, "uppercase"))
		/*
		 * syntax := { "uppercase": <data_expression> }
		 * semantic: evaluate its argument and apply toupper to
		 * its content
		 */
		return NULL;

	/* option */
	if (mapContains(expr, "option")) {
		/*
		 * syntax := { "option":
		 *             { "universe": <option_space_old>,
		 *               "name":  <option_name> }
		 *           }
		 * semantic: get universe/code option from incoming packet
		 */
		struct element *arg;
		struct element *universe;
		struct element *name;
		struct option *option;
		char result[80];

		arg = mapGet(expr, "option");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			debug("can't get option argument");
			return NULL;
		}
		universe = mapGet(arg, "universe");
		if ((universe == NULL) || (universe->type != ELEMENT_STRING)) {
			debug("can't get option universe");
			return NULL;
		}
		name = mapGet(arg, "name");
		if ((name == NULL) || (name->type != ELEMENT_STRING)) {
			debug("can't get option name");
			return NULL;
		}
		option = option_lookup_name(stringValue(universe)->content,
					    stringValue(name)->content);
		if ((option == NULL) || (option->code == 0))
			return NULL;
		if (((local_family == AF_INET) &&
		     (strcmp(option->space->name, "dhcp4") != 0)) ||
		    ((local_family == AF_INET6) &&
		     (strcmp(option->space->name, "dhcp6") != 0)))
			return NULL;
		snprintf(result, sizeof(result),
			 "option[%u].hex", option->code);
		return createString(makeString(-1, result));
	}

	/* hardware */
	if (mapContains(expr, "hardware")) {
		/*
		 * syntax := { "hardware": null }
		 * semantic: get mac type and address from incoming packet
		 */
		struct string *result;

		if (local_family != AF_INET) {
			debug("get hardware for DHCPv6");
			return NULL;
		}
		result = makeString(-1,
			    "concat(substring(pkt4.htype,-1,all),pkt4.mac)");
		return createString(result);
	}

	/* hw-type */
	if (mapContains(expr, "hw-type")) {
		/*
		 * ADDED
		 * syntax := { "hw-type": null }
		 * semantic: get mac type from incoming packet
		 */
		struct string *result;

		if (local_family != AF_INET) {
			debug("get hw-type for DHCPv6");
			return NULL;
		}
		result = makeString(-1, "substring(pkt4.htype,-1,all)");
		return createString(result);
	}

	/* hw-address */
	if (mapContains(expr, "hw-address")) {
		/*
		 * ADDED
		 * syntax := { "hw-address": null }
		 * semantic: get mac address from incoming packet
		 */
		struct string *result;

		if (local_family != AF_INET) {
			debug("get hw-address for DHCPv6");
			return NULL;
		}
		result = makeString(-1, "pkt4.mac");
		return createString(result);
	}

	/* const-data */
	if (mapContains(expr, "const-data")) {
		/*
		 * syntax := { "const-data": <string> }
		 * semantic: embedded string value
		 */
		struct element *arg;

		arg = mapGet(expr, "const-data");
		if ((arg == NULL) || (arg->type != ELEMENT_STRING)) {
			debug("can't get const-data argument");
			return NULL;
		}
		return createString(stringValue(arg));
	}

	/* packet */
	if (mapContains(expr, "packet"))
		/*
		 * syntax := { "packet":
		 *             { "offset": <numeric_expression>,
		 *               "length": <numeric_expression> }
		 *           }
		 * semantic: return the selected substring of the incoming
		 * packet content
		 */
		return NULL;

	/* concat */
	if (mapContains(expr, "concat")) {
		/*
		 * syntax := { "concat":
		 *             { "left":  <data_expression>,
		 *               "right": <data_expression> }
		 *           }
		 * semantic: evaluate arguments and return the concatenation
		 */
		struct element *arg;
		struct element *left;
		struct element *right;
		struct string *result;

		arg = mapGet(expr, "concat");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			debug("can't get concat argument");
			return NULL;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			debug("can't get concat left branch");
			return NULL;
		}
		right = mapGet(arg, "right");
		if (right == NULL) {
			debug("can't get concat right branch");
			return NULL;
		}
		/* left is a literal case */
		if (left->type == ELEMENT_STRING) {
			/* can't be a literal as it was evaluated before */
			right = reduce_data_expression(right);
			if ((right == NULL) || (right->type != ELEMENT_STRING))
				return NULL;
			result = makeString(-1, "concat(");
			concatString(result, quote(stringValue(left)));
			appendString(result, ", ");
			concatString(result, stringValue(right));
			appendString(result, ")");
			return createString(result);
		}
		left = reduce_data_expression(left);
		if ((left == NULL) || (left->type != ELEMENT_STRING))
			return NULL;
		/* right is a literal case */
		if (right->type == ELEMENT_STRING) {
			/* literal left was handled before */
			result = makeString(-1, "concat(");
			concatString(result, stringValue(left));
			appendString(result, ", ");
			concatString(result, quote(stringValue(right)));
			appendString(result, ")");
			return createString(result);
		}
		right = reduce_data_expression(right);
		if ((right == NULL) || (right->type != ELEMENT_STRING))
			return NULL;
		result = makeString(-1, "concat(");
		concatString(result, stringValue(left));
		appendString(result, ", ");
		concatString(result, stringValue(right));
		appendString(result, ")");
		return createString(result);
	}

	/* encapsulate */
	if (mapContains(expr, "encapsulate"))
		/*
		 * syntax := { "encapsulate": <encapsulated_space> }
		 * semantic: encapsulate options of the given space
		 */
		return NULL;

	/* encode-int8 */
	if (mapContains(expr, "encode-int8"))
		/*
		 * syntax := { "encode-int8": <numeric_expression> }
		 * semantic: return a string buffer with the evaluated
		 * number as content
		 */
		return NULL;

	/* encode-int16 */
	if (mapContains(expr, "encode-int16"))
		/*
		 * syntax := { "encode-int16": <numeric_expression> }
		 * semantic: return a string buffer with the evaluated
		 * number as content
		 */
		return NULL;

	/* encode-int32 */
	if (mapContains(expr, "encode-int32"))
		/*
		 * syntax := { "encode-int32": <numeric_expression> }
		 * semantic: return a string buffer with the evaluated
		 * number as content
		 */
		return NULL;

	/* gethostbyname */
	if (mapContains(expr, "gethostbyname"))
		/*
		 * syntax := { "gethostbyname": <string> }
		 * semantic: call gethostbyname and return
		 * a binary buffer with addresses
		 */
		return NULL;

	/* binary-to-ascii */
	if (mapContains(expr, "binary-to-ascii"))
		/*
		 * syntax := { "binary-to-ascii":
		 *             { "base":      <numeric_expression 2..16>,
		 *               "width":     <numeric_expression 8, 16 or 32>,
		 *               "separator": <data_expression>,
		 *               "buffer":    <data_expression> }
		 *           }
		 * semantic: split the input buffer into int8/16/32 numbers,
		 * output them separated by the given string
		 */
		return NULL;

	/* filename */
	if (mapContains(expr, "filename"))
		/*
		 * syntax := { "filename": null }
		 * semantic: get filename field from incoming DHCPv4 packet
		 */
		return NULL;

	/* server-name */
	if (mapContains(expr, "server-name"))
		/*
		 * syntax := { "server-name": null }
		 * semantic: get server-name field from incoming DHCPv4 packet
		 */
		return NULL;

	/* reverse */
	if (mapContains(expr, "reverse"))
		/*
		 * syntax := { "reverse":
		 *             { "width": <numeric_expression>,
		 *               "buffer":    <data_expression> }
		 *           }
		 * semantic: reverse the input buffer by width chunks of bytes
		 */
		return NULL;

	/* pick-first-value */
	if (mapContains(expr, "pick-first-value"))
		/*
		 * syntax := { "pick-first-value":
		 *             [ <data_expression>, ... ]
		 *           }
		 * semantic: evaluates expressions and return the first
		 * not null, return null if all are null
		 */
		return NULL;

	/* host-decl-name */
	if (mapContains(expr, "host-decl-name"))
		/*
		 * syntax := { "host-decl-name": null }
		 * semantic: return the name of the matching host
		 * declaration (aka revervation in kea) or null
		 */
		return NULL;

	/* leased-address */
	if (mapContains(expr, "leased-address"))
		/*
		 * syntax := { "leased-address": null }
		 * semantic: return the address of the assigned lease or
		 * log a message
		 */
		return NULL;

	/* config-option */
	if (mapContains(expr, "config-option"))
		/*
		 * syntax := { "config-option":
		 *             { "universe": <option_space_old>,
		 *               "name":  <option_name> }
		 *           }
		 * semantic: get universe/code option to send
		 */
		return NULL;

	/* null */
	if (mapContains(expr, "null")) {
		/*
		 * syntax := { "null": null }
		 * semantic: return null
		 */
		debug("unexpected null: this expression was not evaluated");
		return NULL;
	}

	/* gethostname */
	if (mapContains(expr, "gethostname")) {
		/*
		 * syntax := { "gethostname": null }
		 * semantic: return gethostname
		 */
		debug("unexpected gethostname: this expression was not "
		      "evaluated");
		return NULL;
	}

	/* v6relay */
	if (mapContains(expr, "v6relay")) {
		/*
		 * syntax := { "v6relay":
		 *             { "relay": <numeric_expression>,
		 *               "relay-option" <data_expression> }
		 *           }
		 * semantic: relay is a counter from client, 0 is no-op,
		 * 1 is the relay closest to the client, etc, option
		 * is a dhcp6 option ans is return when found
		 */
		struct element *arg;
		struct element *relay;
		struct element *universe;
		struct element *name;
		struct option *option;
		int64_t r;
		char result[100];

		if (local_family != AF_INET6) {
			debug("get v6relay for DHCPv4");
			return NULL;
		}
		arg = mapGet(expr, "v6relay");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			debug("can't get v6relay argument");
			return NULL;
		}
		relay = mapGet(arg, "relay");
		if (relay == NULL) {
			debug("can't get v6relay relay");
			return NULL;
		}
		relay = reduce_numeric_expression(relay);
		if ((relay == NULL) || (relay->type != ELEMENT_INTEGER))
			return NULL;
		r = intValue(relay);
		if (r < 0) {
			debug("v6relay called with illegal relay (%lld)",
			      (long long)r);
			return NULL;
		}
		arg = mapGet(arg, "relay-option");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			debug("can't get v6relay relay-option");
			return NULL;
		}
		universe = mapGet(arg, "universe");
		if ((universe == NULL) || (universe->type != ELEMENT_STRING)) {
			debug("can't get v6relay option universe");
			NULL;
		}
		name = mapGet(arg, "name");
		if ((name == NULL) || (name->type != ELEMENT_STRING)) {
			debug("can't get v6relay option name");
			return NULL;
		}
		option = option_lookup_name(stringValue(universe)->content,
					    stringValue(name)->content);
		if ((option == NULL) || (option->code == 0) ||
		    (strcmp(option->space->name, "dhcp6") != 0))
			return NULL;
		if (r == 0)
			snprintf(result, sizeof(result),
				 "option[%u].hex", option->code);
		else {
			/* r > MAX_V6RELAY_HOPS means the relay closest
			   to server */
			if (r > MAX_V6RELAY_HOPS)
				r = 0;
			/* Kea counts from the server, use negative nesting
			   levels to count from the client */
			snprintf(result, sizeof(result),
				 "relay6[%d].option[%u].hex",
				 (int)-r, option->code);
		}
		return createString(makeString(-1, result));
	}

	return NULL;
}

struct element *
reduce_numeric_expression(struct element *expr)
{
	/* trivial case: already done */
	if (expr->type == ELEMENT_INTEGER)
		return expr;

	if (expr->type != ELEMENT_MAP)
		return NULL;

	/* Kea has no numeric operators... */
	return NULL;
}

static struct element *
reduce_equal_expression(struct element *left, struct element *right)
{
	struct string *result;

	/*
	 * numeric case was handled by evaluation
	 */

	if (!is_data_expression(left) || !is_data_expression(right))
		return NULL;

	/* left is a literal case */
	if (left->type == ELEMENT_STRING) {
		/* can't be a literal as it was evaluated before */
		right = reduce_data_expression(right);
		if ((right == NULL) || (right->type != ELEMENT_STRING))
			return NULL;
		result = allocString();
		concatString(result, quote(stringValue(left)));
		appendString(result, " == ");
		concatString(result, stringValue(right));
		return createString(result);
	}
	left = reduce_data_expression(left);
	if ((left == NULL) || (left->type != ELEMENT_STRING))
		return NULL;

	/* right is a literal case */
	if (right->type == ELEMENT_STRING) {
		/* literal left was handled before */
		result = allocString();
		concatString(result, stringValue(left));
		appendString(result, " == ");
		concatString(result, quote(stringValue(right)));
		return createString(result);
	}
	right = reduce_data_expression(right);
	if ((right == NULL) || (right->type != ELEMENT_STRING))
		return NULL;

	result = allocString();
	concatString(result, stringValue(left));
	appendString(result, " == ");
	concatString(result, stringValue(right));
	return createString(result);
}

static void
debug(const char* fmt, ...)
{
	va_list list;

	va_start(list, fmt);
	vfprintf(stderr, fmt, list);
	fprintf(stderr, "\n");
	va_end(list);
}
