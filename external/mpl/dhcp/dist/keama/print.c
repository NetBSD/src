/*	$NetBSD: print.c,v 1.1.1.2 2022/04/03 01:08:42 christos Exp $	*/

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
__RCSID("$NetBSD: print.c,v 1.1.1.2 2022/04/03 01:08:42 christos Exp $");

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

static void debug(const char* fmt, ...);

const char *
print_expression(struct element *expr, isc_boolean_t *lose)
{
	if (expr->type == ELEMENT_BOOLEAN)
		return print_boolean_expression(expr, lose);
	if (expr->type == ELEMENT_INTEGER)
		return print_numeric_expression(expr, lose);
	if (expr->type == ELEMENT_STRING)
		return print_data_expression(expr, lose);

	if (is_boolean_expression(expr))
		return print_boolean_expression(expr, lose);
	if (is_numeric_expression(expr))
		return print_numeric_expression(expr, lose);
	if (is_data_expression(expr))
		return print_data_expression(expr, lose);
	*lose = ISC_TRUE;
	return "???";
}

const char *
print_boolean_expression(struct element *expr, isc_boolean_t *lose)
{
	struct string *result;

	if (expr->type == ELEMENT_BOOLEAN) {
		if (boolValue(expr))
			return "true";
		else
			return "false";
	}

	/*
	 * From is_boolean_expression
	 */
	if (expr->type != ELEMENT_MAP) {
		*lose = ISC_TRUE;
		return "???";
	}
	result = allocString();

	/* check */
	if (mapContains(expr, "check")) {
		struct element *name;

		appendString(result, "check ");
		name = mapGet(expr, "check");
		if ((name == NULL) || (name->type != ELEMENT_STRING)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
		} else
			concatString(result, stringValue(name));
		return result->content;
	}

	/* exists */
	if (mapContains(expr, "exists")) {
		struct element *arg;
		struct element *universe;
		struct element *name;

		appendString(result, "exists ");
		arg = mapGet(expr, "exists");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		universe = mapGet(arg, "universe");
		if ((universe == NULL) || (universe->type != ELEMENT_STRING)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		concatString(result, stringValue(universe));
		appendString(result, ".");
		name = mapGet(arg, "name");
		if ((name == NULL) || (name->type != ELEMENT_STRING)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		concatString(result, stringValue(name));
		return result->content;
	}

	/* variable-exists */
	if (mapContains(expr, "variable-exists")) {
		struct element *name;

		appendString(result, "variable-exists ");
		name = mapGet(expr, "variable-exists");
		if ((name == NULL) || (name->type != ELEMENT_STRING)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
		} else
			concatString(result, stringValue(name));
		return result->content;
	}

	/* equal */
	if (mapContains(expr, "equal")) {
		struct element *arg;
		struct element *left;
		struct element *right;
		isc_boolean_t add_parenthesis;

		appendString(result, "equal ");
		arg = mapGet(expr, "equal");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		add_parenthesis = ISC_TF(expr_precedence(expr_equal,
							 left) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(left, lose));
		if (add_parenthesis)
			appendString(result, ")");
		appendString(result, " = ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		add_parenthesis = ISC_TF(expr_precedence(expr_equal,
							 right) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(right, lose));
		if (add_parenthesis)
			appendString(result, ")");
		return result->content;
	}

	/* not-equal */
	if (mapContains(expr, "not-equal")) {
		struct element *arg;
		struct element *left;
		struct element *right;
		isc_boolean_t add_parenthesis;

		appendString(result, "not-equal ");
		arg = mapGet(expr, "not-equal");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		add_parenthesis = ISC_TF(expr_precedence(expr_not_equal,
							 left) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(left, lose));
		if (add_parenthesis)
			appendString(result, ")");
		appendString(result, " != ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		add_parenthesis = ISC_TF(expr_precedence(expr_not_equal,
							 right) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(right, lose));
		if (add_parenthesis)
			appendString(result, ")");
		return result->content;
	}

	/* regex-match */
	if (mapContains(expr, "regex-match")) {
		struct element *arg;
		struct element *left;
		struct element *right;
		isc_boolean_t add_parenthesis;

		appendString(result, "regex-match ");
		arg = mapGet(expr, "regex-match");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		add_parenthesis = ISC_TF(expr_precedence(expr_regex_match,
							 left) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(left, lose));
		if (add_parenthesis)
			appendString(result, ")");
		appendString(result, " ~= ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		appendString(result, print_expression(right, lose));
		return result->content;
	}

	/* iregex-match */
	if (mapContains(expr, "iregex-match")) {
		struct element *arg;
		struct element *left;
		struct element *right;
		isc_boolean_t add_parenthesis;

		appendString(result, "iregex-match ");
		arg = mapGet(expr, "iregex-match");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		add_parenthesis = ISC_TF(expr_precedence(expr_iregex_match,
							 left) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(left, lose));
		if (add_parenthesis)
			appendString(result, ")");
		appendString(result, " ~~ ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		appendString(result, print_expression(right, lose));
		return result->content;
	}

	/* and */
	if (mapContains(expr, "and")) {
		struct element *arg;
		struct element *left;
		struct element *right;
		isc_boolean_t add_parenthesis;

		appendString(result, "and ");
		arg = mapGet(expr, "and");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		add_parenthesis = ISC_TF(expr_precedence(expr_and,
							 left) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(left, lose));
		if (add_parenthesis)
			appendString(result, ")");
		appendString(result, " and ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		add_parenthesis = ISC_TF(expr_precedence(expr_and,
							 right) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(right, lose));
		if (add_parenthesis)
			appendString(result, ")");
		return result->content;
	}

	/* or */
	if (mapContains(expr, "or")) {
		struct element *arg;
		struct element *left;
		struct element *right;
		isc_boolean_t add_parenthesis;

		appendString(result, "or ");
		arg = mapGet(expr, "or");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		add_parenthesis = ISC_TF(expr_precedence(expr_or,
							 left) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(left, lose));
		if (add_parenthesis)
			appendString(result, ")");
		appendString(result, " or ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		add_parenthesis = ISC_TF(expr_precedence(expr_or,
							 right) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(right, lose));
		if (add_parenthesis)
			appendString(result, ")");
		return result->content;
	}

	/* not */
	if (mapContains(expr, "not")) {
		struct element *arg;
		isc_boolean_t add_parenthesis;

		appendString(result, "not ");
		arg = mapGet(expr, "not");
		if (arg == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		add_parenthesis = ISC_TF(expr_precedence(expr_not,
							 arg) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(arg, lose));
		if (add_parenthesis)
			appendString(result, ")");
		return result->content;
	}

	/* known */
	if (mapContains(expr, "known")) {
		return "known";
	}

	/* static */
	if (mapContains(expr, "static")) {
		return "static";
	}

	/* variable-reference */
	if (mapContains(expr, "variable-reference")) {
		struct element *name;

		appendString(result, "variable-reference ");
		name = mapGet(expr, "variable-reference");
		if ((name == NULL) || (name->type != ELEMENT_STRING)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		return stringValue(name)->content;
	}

	/* funcall */
	if (mapContains(expr, "funcall")) {
		struct element *arg;
		struct element *name;
		struct element *args;
		size_t i;

		appendString(result, "funcall ");
		arg = mapGet(expr, "funcall");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		name = mapGet(arg, "name");
		if ((name == NULL) || (name->type != ELEMENT_STRING)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		concatString(result, stringValue(name));
		appendString(result, "(");
		args = mapGet(arg, "arguments");
		if ((args == NULL) || (args->type != ELEMENT_LIST)) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		for (i = 0; i < listSize(args); i++) {
			struct element *item;

			if (i != 0)
				appendString(result, ", ");
			item = listGet(args, i);
			if (item == NULL) {
				debug("funcall null argument %u",
				      (unsigned)i);
				*lose = ISC_TRUE;
				appendString(result, "???");
				continue;
			}
			appendString(result, print_expression(item, lose));
		}
		appendString(result, ")");
		return result->content;
	}

	*lose = ISC_TRUE;
	appendString(result, "???");
	return result->content;
}

const char *
print_data_expression(struct element *expr, isc_boolean_t *lose)
{
	struct string *result;

	if (expr->type == ELEMENT_STRING)
		return quote(stringValue(expr))->content;

	/*
	 * From is_data_expression
	 */
	if (expr->type != ELEMENT_MAP) {
		*lose = ISC_TRUE;
		return "???";
	}
	result = allocString();

	/* substring */
	if (mapContains(expr, "substring")) {
		struct element *arg;
		struct element *string;
		struct element *offset;
		struct element *length;

		appendString(result, "substring(");
		arg = mapGet(expr, "substring");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		string = mapGet(arg, "expression");
		if (string == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_data_expression(string, lose));
		appendString(result, ", ");
		offset = mapGet(arg, "offset");
		if (offset  == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_numeric_expression(offset, lose));
		appendString(result, ", ");
		length = mapGet(arg, "length");
		if (length  == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_numeric_expression(length, lose));
		appendString(result, ")");
		return result->content;
	}

	/* suffix */
	if (mapContains(expr, "suffix")) {
		struct element *arg;
		struct element *string;
		struct element *length;

		appendString(result, "suffix(");
		arg = mapGet(expr, "suffix");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		string = mapGet(arg, "expression");
		if (string == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_data_expression(string, lose));
		appendString(result, ", ");
		length = mapGet(arg, "length");
		if (length  == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_numeric_expression(length, lose));
		appendString(result, ")");
		return result->content;
	}

	/* lowercase */
	if (mapContains(expr, "lowercase")) {
		struct element *arg;

		appendString(result, "lowercase(");
		arg = mapGet(expr, "lowercase");
		if (arg == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_data_expression(arg, lose));
		appendString(result, ")");
		return result->content;
	}

	/* uppercase */
	if (mapContains(expr, "uppercase")) {
		struct element *arg;

		appendString(result, "uppercase(");
		arg = mapGet(expr, "uppercase");
		if (arg == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_data_expression(arg, lose));
		appendString(result, ")");
		return result->content;
	}

	/* option */
	if (mapContains(expr, "option")) {
		struct element *arg;
		struct element *universe;
		struct element *name;

		appendString(result, "option ");
		arg = mapGet(expr, "option");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		universe = mapGet(arg, "universe");
		if ((universe == NULL) || (universe->type != ELEMENT_STRING)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		concatString(result, stringValue(universe));
		appendString(result, ".");
		name = mapGet(arg, "name");
		if ((name == NULL) || (name->type != ELEMENT_STRING)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		concatString(result, stringValue(name));
		return result->content;
	}

	/* hardware */
	if (mapContains(expr, "hardware"))
		return "hardware";

	/* hw-type */
	if (mapContains(expr, "hw-type"))
		return "hw-type";

	/* hw-address */
	if (mapContains(expr, "hw-address"))
		return "hw-address";

	/* const-data */
	if (mapContains(expr, "const-data")) {
		struct element *arg;

		arg = mapGet(expr, "const-data");
		if ((arg == NULL) || (arg->type != ELEMENT_STRING)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		concatString(result, stringValue(arg));
		return result->content;
	}

	/* packet */
	if (mapContains(expr, "packet")) {
		struct element *arg;
		struct element *offset;
		struct element *length;

		appendString(result, "packet(");
		arg = mapGet(expr, "packet");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		offset = mapGet(arg, "offset");
		if (offset  == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_numeric_expression(offset, lose));
		appendString(result, ", ");
		length = mapGet(arg, "length");
		if (length  == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_numeric_expression(length, lose));
		appendString(result, ")");
		return result->content;
	}

	/* concat */
	if (mapContains(expr, "concat")) {
		struct element *arg;
		struct element *left;
		struct element *right;

		appendString(result, "concat(");
		arg = mapGet(expr, "concat");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_data_expression(left, lose));
		appendString(result, ", ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_data_expression(right, lose));
		appendString(result, ")");
		return result->content;
	}

	/* encapsulate */
	if (mapContains(expr, "encapsulate")) {
		struct element *arg;

		appendString(result, "encapsulate ");
		arg = mapGet(expr, "encapsulate");
		if (arg == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		appendString(result, print_data_expression(arg, lose));
		return result->content;
	}

	/* encode-int8 */
	if (mapContains(expr, "encode-int8")) {
		struct element *arg;

		appendString(result, "encode-int(");
		arg = mapGet(expr, "encode-int8");
		if (arg == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???, 8)");
			return result->content;
		}
		appendString(result, print_numeric_expression(arg, lose));
		appendString(result, ", 8)");
		return result->content;
	}

	/* encode-int16 */
	if (mapContains(expr, "encode-int16")) {
		struct element *arg;

		appendString(result, "encode-int(");
		arg = mapGet(expr, "encode-int16");
		if (arg == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???, 16)");
			return result->content;
		}
		appendString(result, print_numeric_expression(arg, lose));
		appendString(result, ", 16)");
		return result->content;
	}

	/* encode-int32 */
	if (mapContains(expr, "encode-int32")) {
		struct element *arg;

		appendString(result, "encode-int(");
		arg = mapGet(expr, "encode-int32");
		if (arg == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???, 32)");
			return result->content;
		}
		appendString(result, print_numeric_expression(arg, lose));
		appendString(result, ", 32)");
		return result->content;
	}

	/* gethostbyname */
	if (mapContains(expr, "gethostbyname")) {
		struct element *arg;

		appendString(result, "gethostbyname(");
		arg = mapGet(expr, "gethostbyname");
		if (arg == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		appendString(result, print_data_expression(arg, lose));
		appendString(result, ")");
		return result->content;
	}

	/* binary-to-ascii */
	if (mapContains(expr, "binary-to-ascii")) {
		struct element *arg;
		struct element *base;
		struct element *width;
		struct element *separator;
		struct element *buffer;

		appendString(result, "binary-to-ascii(");
		arg = mapGet(expr, "binary-to-ascii");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		base = mapGet(arg, "base");
		if (base == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_numeric_expression(base, lose));
		appendString(result, ", ");
		width = mapGet(arg, "width");
		if (width == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_numeric_expression(width, lose));
		appendString(result, ", ");
		separator = mapGet(arg, "separator");
		if (separator == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_data_expression(separator, lose));
		appendString(result, ", ");
		buffer = mapGet(arg, "buffer");
		if (buffer == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_data_expression(buffer, lose));
		appendString(result, ")");
		return result->content;
	}

	/* filename */
	if (mapContains(expr, "filename"))
		return "filename";

	/* server-name */
	if (mapContains(expr, "server-name"))
		return "server-name";

	/* reverse */
	if (mapContains(expr, "reverse")) {
		struct element *arg;
		struct element *width;
		struct element *buffer;

		appendString(result, "reverse(");
		arg = mapGet(expr, "reverse");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		width = mapGet(arg, "width");
		if (width == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_numeric_expression(width, lose));
		appendString(result, ", ");
		buffer = mapGet(arg, "buffer");
		if (buffer == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_data_expression(buffer, lose));
		appendString(result, ")");
		return result->content;
	}

	/* pick-first-value */
	if (mapContains(expr, "pick-first-value")) {
		struct element *arg;
		size_t i;

		appendString(result, "pick-first-value(");
		arg = mapGet(expr, "pick-first-value");
		if ((arg == NULL) || (arg->type != ELEMENT_LIST)) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		for (i = 0; i < listSize(arg); i++) {
			struct element *item;

			if (i != 0)
				appendString(result, ", ");
			item = listGet(arg, i);
			if (item == NULL) {
				*lose = ISC_TRUE;
				appendString(result, "???");
				continue;
			}
			appendString(result,
				     print_data_expression(item, lose));
		}
		appendString(result, ")");
		return result->content;
	}

	/* host-decl-name */
	if (mapContains(expr, "host-decl-name"))
		return "host-decl-name";

	/* leased-address */
	if (mapContains(expr, "leased-address"))
		return "leased-address";

	/* config-option */
	if (mapContains(expr, "config-option")) {
		struct element *arg;
		struct element *universe;
		struct element *name;

		appendString(result, "config-option ");
		arg = mapGet(expr, "config-option");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		universe = mapGet(arg, "universe");
		if ((universe == NULL) || (universe->type != ELEMENT_STRING)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		concatString(result, stringValue(universe));
		appendString(result, ".");
		name = mapGet(arg, "name");
		if ((name == NULL) || (name->type != ELEMENT_STRING)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		concatString(result, stringValue(name));
		return result->content;
	}

	/* null */
	if (mapContains(expr, "null"))
		return "null";

	/* gethostname */
	if (mapContains(expr, "gethostname"))
		return "gethostname";

	/* v6relay */
	if (mapContains(expr, "v6relay")) {
		struct element *arg;
		struct element *relay;
		struct element *option;


		appendString(result, "v6relay(");
		arg = mapGet(expr, "v6relay");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		relay = mapGet(arg, "relay");
		if (relay == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_numeric_expression(relay, lose));
		appendString(result, ", ");
		option = mapGet(arg, "relay-option");
		if (option == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???" ")");
			return result->content;
		}
		appendString(result, print_data_expression(option, lose));
		appendString(result, ")");
		return result->content;
	}

	*lose = ISC_TRUE;
	appendString(result, "???");
	return result->content;
}

const char *
print_numeric_expression(struct element *expr, isc_boolean_t *lose)
{
	struct string *result;

	if (expr->type == ELEMENT_INTEGER) {
		char buf[20];

		snprintf(buf, sizeof(buf), "%lld", (long long)intValue(expr));
		result = makeString(-1, buf);
		return result->content;
	}

	/*
	 * From is_numeric_expression
	 */
	if (expr->type != ELEMENT_MAP) {
		*lose = ISC_TRUE;
		return "???";
	}
	result = allocString();

	/* extract-int8 */
	if (mapContains(expr, "extract-int8")) {
		struct element *arg;

		appendString(result, "extract-int(");
		arg = mapGet(expr, "extract-int8");
		if (arg == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???, 8)");
			return result->content;
		}
		appendString(result, print_data_expression(arg, lose));
		appendString(result, ", 8)");
		return result->content;
	}

	/* extract-int16 */
	if (mapContains(expr, "extract-int16")) {
		struct element *arg;

		appendString(result, "extract-int(");
		arg = mapGet(expr, "extract-int16");
		if (arg == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???, 16)");
			return result->content;
		}
		appendString(result, print_data_expression(arg, lose));
		appendString(result, ", 16)");
		return result->content;
	}

	/* extract-int32 */
	if (mapContains(expr, "extract-int32")) {
		struct element *arg;

		appendString(result, "extract-int(");
		arg = mapGet(expr, "extract-int32");
		if (arg == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???, 32)");
			return result->content;
		}
		appendString(result, print_data_expression(arg, lose));
		appendString(result, ", 32)");
		return result->content;
	}

	/* const-int */
	if (mapContains(expr, "const-int")) {
		struct element *arg;
		char buf[20];

		arg = mapGet(expr, "const-int");
		if ((arg == NULL) || (arg->type != ELEMENT_INTEGER)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		snprintf(buf, sizeof(buf), "%lld", (long long)intValue(arg));
		result = makeString(-1, buf);
		return result->content;
	}

	/* lease-time */
	if (mapContains(expr, "lease-time"))
		return "lease-time";

	/* add */
	if (mapContains(expr, "add")) {
		struct element *arg;
		struct element *left;
		struct element *right;
		isc_boolean_t add_parenthesis;

		appendString(result, "add ");
		arg = mapGet(expr, "add");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		add_parenthesis = ISC_TF(expr_precedence(expr_add,
							 left) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(left, lose));
		if (add_parenthesis)
			appendString(result, ")");
		appendString(result, " + ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		add_parenthesis = ISC_TF(expr_precedence(expr_add,
							 right) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(right, lose));
		if (add_parenthesis)
			appendString(result, ")");
		return result->content;
	}

	/* subtract */
	if (mapContains(expr, "subtract")) {
		struct element *arg;
		struct element *left;
		struct element *right;
		isc_boolean_t add_parenthesis;

		appendString(result, "subtract ");
		arg = mapGet(expr, "subtract");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		add_parenthesis = ISC_TF(expr_precedence(expr_subtract,
							 left) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(left, lose));
		if (add_parenthesis)
			appendString(result, ")");
		appendString(result, " - ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		add_parenthesis = ISC_TF(expr_precedence(expr_subtract,
							 right) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(right, lose));
		if (add_parenthesis)
			appendString(result, ")");
		return result->content;
	}

	/* multiply */
	if (mapContains(expr, "multiply")) {
		struct element *arg;
		struct element *left;
		struct element *right;
		isc_boolean_t add_parenthesis;

		appendString(result, "multiply ");
		arg = mapGet(expr, "multiply");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		add_parenthesis = ISC_TF(expr_precedence(expr_multiply,
							 left) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(left, lose));
		if (add_parenthesis)
			appendString(result, ")");
		appendString(result, " * ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		add_parenthesis = ISC_TF(expr_precedence(expr_multiply,
							 right) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(right, lose));
		if (add_parenthesis)
			appendString(result, ")");
		return result->content;
	}

	/* divide */
	if (mapContains(expr, "divide")) {
		struct element *arg;
		struct element *left;
		struct element *right;
		isc_boolean_t add_parenthesis;

		appendString(result, "divide ");
		arg = mapGet(expr, "divide");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		add_parenthesis = ISC_TF(expr_precedence(expr_divide,
							 left) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(left, lose));
		if (add_parenthesis)
			appendString(result, ")");
		appendString(result, " / ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		add_parenthesis = ISC_TF(expr_precedence(expr_divide,
							 right) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(right, lose));
		if (add_parenthesis)
			appendString(result, ")");
		return result->content;
	}

	/* remainder */
	if (mapContains(expr, "remainder")) {
		struct element *arg;
		struct element *left;
		struct element *right;
		isc_boolean_t add_parenthesis;

		appendString(result, "remainder ");
		arg = mapGet(expr, "remainder");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		add_parenthesis = ISC_TF(expr_precedence(expr_remainder,
							 left) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(left, lose));
		if (add_parenthesis)
			appendString(result, ")");
		appendString(result, " % ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		add_parenthesis = ISC_TF(expr_precedence(expr_remainder,
							 right) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(right, lose));
		if (add_parenthesis)
			appendString(result, ")");
		return result->content;
	}

	/* binary-and */
	if (mapContains(expr, "binary-and")) {
		struct element *arg;
		struct element *left;
		struct element *right;
		isc_boolean_t add_parenthesis;

		appendString(result, "binary-and ");
		arg = mapGet(expr, "binary-and");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		add_parenthesis = ISC_TF(expr_precedence(expr_binary_and,
							 left) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(left, lose));
		if (add_parenthesis)
			appendString(result, ")");
		appendString(result, " & ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		add_parenthesis = ISC_TF(expr_precedence(expr_binary_and,
							 right) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(right, lose));
		if (add_parenthesis)
			appendString(result, ")");
		return result->content;
	}

	/* binary-or */
	if (mapContains(expr, "binary-or")) {
		struct element *arg;
		struct element *left;
		struct element *right;
		isc_boolean_t add_parenthesis;

		appendString(result, "binary-or ");
		arg = mapGet(expr, "binary-or");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		add_parenthesis = ISC_TF(expr_precedence(expr_binary_or,
							 left) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(left, lose));
		if (add_parenthesis)
			appendString(result, ")");
		appendString(result, " | ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		add_parenthesis = ISC_TF(expr_precedence(expr_binary_or,
							 right) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(right, lose));
		if (add_parenthesis)
			appendString(result, ")");
		return result->content;
	}

	/* binary-xor */
	if (mapContains(expr, "binary-xor")) {
		struct element *arg;
		struct element *left;
		struct element *right;
		isc_boolean_t add_parenthesis;

		appendString(result, "binary-xor ");
		arg = mapGet(expr, "binary-xor");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP)) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		left = mapGet(arg, "left");
		if (left == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		result = allocString();
		add_parenthesis = ISC_TF(expr_precedence(expr_binary_xor,
							 left) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(left, lose));
		if (add_parenthesis)
			appendString(result, ")");
		appendString(result, " ^ ");
		right = mapGet(arg, "right");
		if (right == NULL) {
			*lose = ISC_TRUE;
			appendString(result, "???");
			return result->content;
		}
		add_parenthesis = ISC_TF(expr_precedence(expr_binary_xor,
							 right) < 0);
		if (add_parenthesis)
			appendString(result, "(");
		appendString(result, print_expression(right, lose));
		if (add_parenthesis)
			appendString(result, ")");
		return result->content;
	}

	/* client-state */
	if (mapContains(expr, "client-state"))
		return "client-state";

	*lose = ISC_TRUE;
	appendString(result, "???");
	return result->content;
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
