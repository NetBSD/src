/*	$NetBSD: eval.c,v 1.1.1.2 2022/04/03 01:08:42 christos Exp $	*/

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
__RCSID("$NetBSD: eval.c,v 1.1.1.2 2022/04/03 01:08:42 christos Exp $");

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

static struct element *eval_equal_expression(struct element *,
					     struct element *);
static isc_boolean_t cmp_hexa(struct element *, isc_boolean_t,
			      struct element *,isc_boolean_t);
static void debug(const char* fmt, ...);

struct element *
eval_expression(struct element *expr, isc_boolean_t *modifiedp)
{
	if ((expr->type == ELEMENT_BOOLEAN) ||
	    (expr->type == ELEMENT_INTEGER) ||
	    (expr->type == ELEMENT_STRING))
		return expr;

	if (is_boolean_expression(expr))
		return eval_boolean_expression(expr, modifiedp);
	if (is_numeric_expression(expr))
		return eval_numeric_expression(expr, modifiedp);
	if (is_data_expression(expr))
		return eval_data_expression(expr, modifiedp);
	debug("can't type expression");
	return expr;
}

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
eval_boolean_expression(struct element *expr, isc_boolean_t *modifiedp)
{
	/* trivial case: already done */
	if (expr->type == ELEMENT_BOOLEAN)
		return expr;

	/*
	 * From is_boolean_expression
	 */

	if (expr->type != ELEMENT_MAP)
		return expr;


	/* check */
	if (mapContains(expr, "check"))
		/*
		 * syntax := { "check": <collection_name> }
		 * semantic: check_collection
		 *  on server try to match classes of the collection
		 */
		return expr;


	/* exists */
	if (mapContains(expr, "exists"))
		/*
		 * syntax := { "exists":
		 *             { "universe": <option_space_old>,
		 *               "name":  <option_name> }
		 *           }
		 * semantic: check universe/code from incoming packet
		 */
		return expr;

	/* variable-exists */
	if (mapContains(expr, "variable-exists"))
		/*
		 * syntax := { "variable-exists": <variable_name> }
		 * semantics: find_binding(scope, name)
		 */
		return expr;

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
		struct element *equal;
		struct comments comments;
		isc_boolean_t lmodified = ISC_FALSE;
		isc_boolean_t rmodified = ISC_FALSE;

		arg = mapGet(expr, "equal");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		left = mapGet(arg, "left");
		if (left == NULL)
			return expr;
		right = mapGet(arg, "right");
		if (right == NULL)
			return expr;
		left = eval_expression(left, &lmodified);
		if (lmodified) {
			mapRemove(arg, "left");
			mapSet(arg, left, "left");
		}
		right = eval_expression(right, &rmodified);
		if (rmodified) {
			mapRemove(arg, "right");
			mapSet(arg, right, "right");
		}

		equal = eval_equal_expression(left, right);
		if ((equal == NULL) || (equal->type != ELEMENT_BOOLEAN))
			return expr;
		*modifiedp = ISC_TRUE;
		TAILQ_INIT(&comments);
		TAILQ_CONCAT(&comments, &expr->comments);
		TAILQ_CONCAT(&comments, &arg->comments);
		TAILQ_CONCAT(&comments, &equal->comments);
		TAILQ_CONCAT(&equal->comments, &comments);
		return equal;
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
		struct element *result;
		isc_boolean_t lmodified = ISC_FALSE;
		isc_boolean_t rmodified = ISC_FALSE;

		arg = mapGet(expr, "not-equal");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		left = mapGet(arg, "left");
		if (left == NULL)
			return expr;
		right = mapGet(arg, "right");
		if (right == NULL)
			return expr;
		left = eval_expression(left, &lmodified);
		if (lmodified) {
			mapRemove(arg, "left");
			mapSet(arg, left, "left");
		}
		right = eval_expression(right, &rmodified);
		if (rmodified) {
			mapRemove(arg, "right");
			mapSet(arg, right, "right");
		}

		equal = eval_equal_expression(left, right);
		if ((equal == NULL) || (equal->type != ELEMENT_BOOLEAN))
			return expr;
		*modifiedp = ISC_TRUE;
		result = createBool(ISC_TF(!boolValue(equal)));
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		TAILQ_CONCAT(&result->comments, &equal->comments);
		return result;
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
		return expr;

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
		return expr;

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
		struct element *result;
		isc_boolean_t lmodified = ISC_FALSE;
		isc_boolean_t rmodified = ISC_FALSE;

		arg = mapGet(expr, "and");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		left = mapGet(arg, "left");
		if (left == NULL)
			return expr;
		right = mapGet(arg, "right");
		if (right == NULL)
			debug("can't get and right branch");
		left = eval_boolean_expression(left, &lmodified);
		if (lmodified) {
			mapRemove(arg, "left");
			mapSet(arg, left, "left");
		}
		right = eval_boolean_expression(right, &rmodified);
		if (rmodified) {
			mapRemove(arg, "right");
			mapSet(arg, right, "right");
		}

		if (left->type == ELEMENT_BOOLEAN) {
			*modifiedp = ISC_TRUE;
			if (!boolValue(left))
				result = createBool(ISC_FALSE);
			else {
				result = copy(right);
				TAILQ_INIT(&result->comments);
			}
			TAILQ_CONCAT(&result->comments, &expr->comments);
			TAILQ_CONCAT(&result->comments, &arg->comments);
			TAILQ_CONCAT(&result->comments, &left->comments);
			TAILQ_CONCAT(&result->comments, &right->comments);
			return result;
		}
		if (right->type == ELEMENT_BOOLEAN) {
			*modifiedp = ISC_TRUE;
			if (!boolValue(right))
				result = createBool(ISC_FALSE);
			else {
				result = copy(left);
				TAILQ_INIT(&result->comments);
			}
			TAILQ_CONCAT(&result->comments, &expr->comments);
			TAILQ_CONCAT(&result->comments, &arg->comments);
			TAILQ_CONCAT(&result->comments, &left->comments);
			TAILQ_CONCAT(&result->comments, &right->comments);
			return result;
		}
		return expr;
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
		struct element *result;
		isc_boolean_t lmodified = ISC_FALSE;
		isc_boolean_t rmodified = ISC_FALSE;

		arg = mapGet(expr, "or");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		left = mapGet(arg, "left");
		if (left == NULL)
			return expr;
		right = mapGet(arg, "right");
		if (right == NULL)
			return expr;
		left = eval_boolean_expression(left, &lmodified);
		if (lmodified) {
			mapRemove(arg, "left");
			mapSet(arg, left, "left");
		}
		right = eval_boolean_expression(right, &rmodified);
		if (rmodified) {
			mapRemove(arg, "right");
			mapSet(arg, right, "right");
		}

		if (left->type == ELEMENT_BOOLEAN) {
			*modifiedp = ISC_TRUE;
			if (boolValue(left))
				result = createBool(ISC_TRUE);
			else {
				result = copy(right);
				TAILQ_INIT(&result->comments);
			}
			TAILQ_CONCAT(&result->comments, &expr->comments);
			TAILQ_CONCAT(&result->comments, &arg->comments);
			TAILQ_CONCAT(&result->comments, &left->comments);
			TAILQ_CONCAT(&result->comments, &right->comments);
			return result;
		}
		if (right->type == ELEMENT_BOOLEAN) {
			*modifiedp = ISC_TRUE;
			if (boolValue(right))
				result = createBool(ISC_TRUE);
			else {
				result = copy(left);
				TAILQ_INIT(&result->comments);
			}
			TAILQ_CONCAT(&result->comments, &expr->comments);
			TAILQ_CONCAT(&result->comments, &arg->comments);
			TAILQ_CONCAT(&result->comments, &left->comments);
			TAILQ_CONCAT(&result->comments, &right->comments);
			return result;
		}
		return expr;
	}

	/* not */
	if (mapContains(expr, "not")) {
		/*
		 * syntax := { "not": <boolean_expression> }
		 * semantic: evaluate its branch and return its negation
		 */
		struct element *arg;
		struct element *result;
		isc_boolean_t modified = ISC_FALSE;

		arg = mapGet(expr, "not");
		if (arg == NULL)
			return expr;
		arg = eval_boolean_expression(arg, &modified);
		if (modified) {
			mapRemove(expr, "not");
			mapSet(expr, arg, "not");
		}

		/* remove double not */
		if ((arg->type == ELEMENT_MAP) && mapContains(arg, "not")) {
			arg = mapGet(arg, "not");
			if (arg == NULL)
				return expr;
			*modifiedp = ISC_TRUE;
			return arg;
		}

		/* compose with equal */
		if ((arg->type == ELEMENT_MAP) &&
		    mapContains(arg, "equal")) {
			arg = mapGet(arg, "equal");
			if (arg == NULL)
				return expr;
			*modifiedp = ISC_TRUE;
			result = createMap();
			mapSet(result, arg, "not-equal");
			return result;
		}

		/* compose with not-equal */
		if ((arg->type == ELEMENT_MAP) &&
		    mapContains(arg, "not-equal")) {
			arg = mapGet(arg, "not-equal");
			if (arg == NULL)
				return expr;
			*modifiedp = ISC_TRUE;
			result = createMap();
			mapSet(result, arg, "equal");
			return result;
		}

		if (arg->type != ELEMENT_BOOLEAN)
			return expr;
		*modifiedp = ISC_TRUE;
		result = createBool(ISC_TF(!boolValue(arg)));
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		return result;
	}

	/* known */
	if (mapContains(expr, "known"))
		/*
		 * syntax := { "known": null }
		 * semantics: client is known, i.e., has a matching
		 * host declaration (aka reservation in Kea)
		 */
		return expr;

	/* static */
	if (mapContains(expr, "static"))
		/*
		 * syntax := { "static": null }
		 * semantics: lease is static (doesn't exist in Kea)
		 */
		return expr;

	return expr;
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
eval_data_expression(struct element *expr, isc_boolean_t *modifiedp)
{
	/* trivial case: already done */
	if (expr->type == ELEMENT_STRING)
		return expr;

	/*
	 * From is_data_expression
	 */

	if (expr->type != ELEMENT_MAP)
		return expr;

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
		struct element *result;
		struct string *s;
		struct string *r;
		int64_t off;
		int64_t len;
		isc_boolean_t smodified = ISC_FALSE;
		isc_boolean_t omodified = ISC_FALSE;
		isc_boolean_t lmodified = ISC_FALSE;

		arg = mapGet(expr, "substring");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		string = mapGet(arg, "expression");
		if (string == NULL)
			return expr;
		offset = mapGet(arg, "offset");
		if (offset  == NULL)
			return expr;
		length = mapGet(arg, "length");
		if (length  == NULL)
			return expr;
		string = eval_data_expression(string, &smodified);
		if (smodified) {
			mapRemove(arg, "expression");
			mapSet(arg, string, "expression");
		}
		offset = eval_numeric_expression(offset, &omodified);
		if (omodified) {
			mapRemove(arg, "offset");
			mapSet(arg, offset, "offset");
		}
		length = eval_numeric_expression(length, &lmodified);
		if (lmodified) {
			mapRemove(arg, "length");
			mapSet(arg, length, "length");
		}

		if ((offset->type != ELEMENT_INTEGER) ||
		    (length->type != ELEMENT_INTEGER))
			return expr;
		off = intValue(offset);
		len = intValue(length);
		if ((off < 0) || (len < 0))
			return expr;
		/* degenerated case */
		if (len == 0) {
			*modifiedp = ISC_TRUE;
			r = allocString();
			result = createString(r);
			return result;
		}

		/* return (part of) hw-address? */
		if ((local_family == AF_INET) &&
		    (string->type == ELEMENT_MAP) &&
		    mapContains(string, "concat") &&
		    (off >= 1)) {
			struct element *concat;
			struct element *left;
			struct element *right;

			concat = mapGet(string, "concat");
			if (concat->type != ELEMENT_MAP)
				return expr;
			left = mapGet(concat, "left");
			if (left == NULL)
				return expr;
			right = mapGet(concat, "right");
			if (right == NULL)
				return expr;
			/* from substring(hardware, ...) */
			if ((left->type == ELEMENT_MAP) &&
			    mapContains(left, "hw-type")) {
				*modifiedp = ISC_TRUE;
				mapRemove(arg, "expression");
				mapSet(arg, right, "expression");
				mapRemove(arg, "offset");
				mapSet(arg, createInt(off - 1), "offset");
				return expr;
			}
			return expr;
		}

		/* return hw-type? */
		if ((local_family == AF_INET) &&
		    (string->type == ELEMENT_MAP) &&
		    mapContains(string, "concat") &&
		    (off == 0) && (len == 1)) {
			struct element *concat;
			struct element *left;
			struct element *right;

			concat = mapGet(string, "concat");
			if (concat->type != ELEMENT_MAP)
				return expr;
			left = mapGet(concat, "left");
			if (left == NULL)
				return expr;
			right = mapGet(concat, "right");
			if (right == NULL)
				return expr;
			/* from substring(hardware, ...) */
			if ((left->type == ELEMENT_MAP) &&
			    mapContains(left, "hw-type")) {
				*modifiedp = ISC_TRUE;
				return left;
			}
			return expr;
		}

		if (string->type != ELEMENT_STRING)
			return expr;
		*modifiedp = ISC_TRUE;
		s = stringValue(string);
		if (s->length <= off)
			r = allocString();
		else {
			r = makeString(s->length - off, s->content + off);
			if (r->length > len)
				r->length = len;
		}
		result = createString(r);
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		TAILQ_CONCAT(&result->comments, &string->comments);
		TAILQ_CONCAT(&result->comments, &offset->comments);
		TAILQ_CONCAT(&result->comments, &length->comments);
		return result;
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
		struct element *result;
		struct string *r;
		int64_t len;
		isc_boolean_t smodified = ISC_FALSE;
		isc_boolean_t lmodified = ISC_FALSE;

		arg = mapGet(expr, "suffix");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		string = mapGet(arg, "expression");
		if (string == NULL)
			return expr;
		length = mapGet(arg, "length");
		if (length  == NULL)
			return expr;
		string = eval_data_expression(string, &smodified);
		if (smodified) {
			mapRemove(arg, "expression");
			mapSet(arg, string, "expression");
		}
		length = eval_numeric_expression(length, &lmodified);
		if (lmodified) {
			mapRemove(arg, "length");
			mapSet(arg, length, "length");
		}

		if ((string->type != ELEMENT_STRING) ||
		    (length->type != ELEMENT_INTEGER))
			return expr;
		len = intValue(length);
		if (len < 0)
			return expr;
		*modifiedp = ISC_TRUE;
		r = stringValue(string);
		if (r->length > len)
			r = makeString(r->length - len, r->content + len);
		result = createString(r);
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		TAILQ_CONCAT(&result->comments, &string->comments);
		TAILQ_CONCAT(&result->comments, &length->comments);
		return result;
	}

	/* lowercase */
	if (mapContains(expr, "lowercase")) {
		/*
		 * syntax := { "lowercase": <data_expression> }
		 * semantic: evaluate its argument and apply tolower to
		 * its content
		 */
		struct element *arg;
		struct element *result;
		struct string *r;
		size_t i;
		isc_boolean_t modified = ISC_FALSE;

		arg = mapGet(expr, "lowercase");
		if (arg == NULL)
			return expr;
		arg = eval_data_expression(arg, &modified);
		if (modified) {
			mapRemove(expr, "lowercase");
			mapSet(expr, arg, "lowercase");
		}

		if (arg->type != ELEMENT_STRING)
			return expr;
		*modifiedp = ISC_TRUE;
		r = allocString();
		concatString(r, stringValue(arg));
		for (i = 0; i < r->length; i++)
			r->content[i] = tolower(r->content[i]);
		result = createString(r);
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		return result;
	}

	/* uppercase */
	if (mapContains(expr, "uppercase")) {
		/*
		 * syntax := { "uppercase": <data_expression> }
		 * semantic: evaluate its argument and apply toupper to
		 * its content
		 */
		struct element *arg;
		struct element *result;
		struct string *r;
		size_t i;
		isc_boolean_t modified = ISC_FALSE;

		arg = mapGet(expr, "uppercase");
		if (arg == NULL)
			return expr;
		arg = eval_data_expression(arg, &modified);
		if (modified) {
			mapRemove(expr, "lowercase");
			mapSet(expr, arg, "lowercase");
		}

		if (arg->type != ELEMENT_STRING)
			return expr;
		*modifiedp = ISC_TRUE;
		r = allocString();
		concatString(r, stringValue(arg));
		for (i = 0; i < r->length; i++)
			r->content[i] = toupper(r->content[i]);
		result = createString(r);
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		return result;
	}

	/* option */
	if (mapContains(expr, "option"))
		/*
		 * syntax := { "option":
		 *             { "universe": <option_space_old>,
		 *               "name":  <option_name> }
		 *           }
		 * semantic: get universe/code option from incoming packet
		 */
		return expr;

	/* hardware */
	if (mapContains(expr, "hardware")) {
		/*
		 * syntax := { "hardware": null }
		 * semantic: get mac type and address from incoming packet
		 */
		struct element *left;
		struct element *right;
		struct element *concat;
		struct element *result;

		if (local_family != AF_INET)
			return expr;
		*modifiedp = ISC_TRUE;
		left = createMap();
		mapSet(left, createNull(), "hw-type");
		concat = createMap();
		mapSet(concat, left, "left");
		right = createMap();
		mapSet(right, createNull(), "hw-address");
		mapSet(concat, right, "right");
		result = createMap();
		mapSet(result, concat, "concat");
		return result;
	}

	/* hw-type */
	if (mapContains(expr, "hw-type"))
		/*
		 * syntax := { "hw-type": null }
		 * semantic: get mac type and address from incoming packet
		 */
		return expr;

	/* hw-address */
	if (mapContains(expr, "hw-address"))
		/*
		 * syntax := { "hw-address": null }
		 * semantic: get mac type and address from incoming packet
		 */
		return expr;

	/* const-data */
	if (mapContains(expr, "const-data"))
		/*
		 * syntax := { "const-data": <string> }
		 * semantic: embedded string value
		 */
		return expr;

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
		return expr;

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
		struct element *result;
		struct string *r;
		isc_boolean_t lmodified = ISC_FALSE;
		isc_boolean_t rmodified = ISC_FALSE;

		arg = mapGet(expr, "concat");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		left = mapGet(arg, "left");
		if (left == NULL)
			return expr;
		right = mapGet(arg, "right");
		if (right == NULL)
			return expr;
		left = eval_data_expression(left, &lmodified);
		if (lmodified) {
			mapRemove(arg, "left");
			mapSet(arg, left, "left");
		}
		right = eval_data_expression(right, &rmodified);
		if (rmodified) {
			mapRemove(arg, "right");
			mapSet(arg, right, "right");
		}

		/* degenerated cases */
		if ((left->type == ELEMENT_STRING) &&
		    (stringValue(left)->length == 0)) {
			*modifiedp = ISC_TRUE;
			return right;
		}
		if ((right->type == ELEMENT_STRING) &&
		    (stringValue(right)->length == 0)) {
			*modifiedp = ISC_TRUE;
			return left;
		}

		if ((left->type != ELEMENT_STRING) ||
		    (right->type != ELEMENT_STRING))
			return expr;
		*modifiedp = ISC_TRUE;
		r = allocString();
		concatString(r, stringValue(left));
		concatString(r, stringValue(right));
		result = createString(r);
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		TAILQ_CONCAT(&result->comments, &left->comments);
		TAILQ_CONCAT(&result->comments, &right->comments);
		return result;
	}

	/* encapsulate */
	if (mapContains(expr, "encapsulate"))
		/*
		 * syntax := { "encapsulate": <encapsulated_space> }
		 * semantic: encapsulate options of the given space
		 */
		return expr;

	/* encode-int8 */
	if (mapContains(expr, "encode-int8")) {
		/*
		 * syntax := { "encode-int8": <numeric_expression> }
		 * semantic: return a string buffer with the evaluated
		 * number as content
		 */
		struct element *arg;
		struct element *result;
		struct string *r;
		uint8_t val;
		isc_boolean_t modified = ISC_FALSE;

		arg = mapGet(expr, "encode-int8");
		if (arg == NULL)
			return expr;
		arg = eval_numeric_expression(arg, &modified);
		if (modified) {
			mapRemove(expr, "encode-int8");
			mapSet(expr, arg, "encode-int8");
		}

		if (arg->type != ELEMENT_INTEGER)
			return expr;
		*modifiedp = ISC_TRUE;
		val = (uint8_t)intValue(arg);
		r = makeString(sizeof(val), (char *)&val);
		result = createString(r);
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		return result;
	}

	/* encode-int16 */
	if (mapContains(expr, "encode-int16")) {
		/*
		 * syntax := { "encode-int16": <numeric_expression> }
		 * semantic: return a string buffer with the evaluated
		 * number as content
		 */
		struct element *arg;
		struct element *result;
		struct string *r;
		uint16_t val;
		isc_boolean_t modified = ISC_FALSE;

		arg = mapGet(expr, "encode-int16");
		if (arg == NULL)
			return expr;
		arg = eval_numeric_expression(arg, &modified);
		if (modified) {
			mapRemove(expr, "encode-int16");
			mapSet(expr, arg, "encode-int16");
		}

		if (arg->type != ELEMENT_INTEGER)
			return expr;
		*modifiedp = ISC_TRUE;
		val = (uint16_t)intValue(arg);
		val = htons(val);
		r = makeString(sizeof(val), (char *)&val);
		result = createString(r);
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		return result;
	}

	/* encode-int32 */
	if (mapContains(expr, "encode-int32")) {
		/*
		 * syntax := { "encode-int32": <numeric_expression> }
		 * semantic: return a string buffer with the evaluated
		 * number as content
		 */
		struct element *arg;
		struct element *result;
		struct string *r;
		uint32_t val;
		isc_boolean_t modified = ISC_FALSE;

		arg = mapGet(expr, "encode-int32");
		if (arg == NULL)
			return expr;
		arg = eval_numeric_expression(arg, &modified);
		if (modified) {
			mapRemove(expr, "encode-int32");
			mapSet(expr, arg, "encode-int32");
		}

		if (arg->type != ELEMENT_INTEGER)
			return expr;
		*modifiedp = ISC_TRUE;
		val = (uint32_t)intValue(arg);
		val = htonl(val);
		r = makeString(sizeof(val), (char *)&val);
		result = createString(r);
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		return result;
	}

	/* gethostbyname */
	if (mapContains(expr, "gethostbyname")) {
		/*
		 * syntax := { "gethostbyname": <string> }
		 * semantic: call gethostbyname and return
		 * a binary buffer with addresses
		 */
		struct element *arg;
		struct element *result;
		struct string *r;
		char *hostname;
		struct hostent *h;
		size_t i;

		if (local_family != AF_INET)
			return expr;
		arg = mapGet(expr, "gethostbyname");
		if ((arg == NULL) || (arg->type != ELEMENT_STRING))
			return expr;
		hostname = stringValue(arg)->content;
		h = gethostbyname(hostname);
		r = allocString();
		if (h == NULL) {
			switch (h_errno) {
			case HOST_NOT_FOUND:
				debug("gethostbyname: %s: host unknown",
				      hostname);
				break;
			case TRY_AGAIN:
				debug("gethostbyname: %s: temporary name "
				      "server failure", hostname);
				break;
			case NO_RECOVERY:
				debug("gethostbyname: %s: name server failed",
				      hostname);
				break;
			case NO_DATA:
				debug("gethostbyname: %s: no A record "
				      "associated with address", hostname);
				break;
			}
		} else
			for (i = 0; h->h_addr_list[i] != NULL; i++) {
				struct string *addr;

				addr = makeString(4, h->h_addr_list[i]);
				concatString(r, addr);
			}
		*modifiedp = ISC_TRUE;
		r = makeStringExt(r->length, r->content, 'X');
		result = createString(r);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		return result;
	}

	/* binary-to-ascii */
	if (mapContains(expr, "binary-to-ascii")) {
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
		struct element *arg;
		struct element *base;
		struct element *width;
		struct element *separator;
		struct element *buffer;
		struct element *result;
		struct string *sep;
		struct string *buf;
		struct string *r;
		int64_t b;
		int64_t w;
		isc_boolean_t bmodified = ISC_FALSE;
		isc_boolean_t wmodified = ISC_FALSE;
		isc_boolean_t smodified = ISC_FALSE;
		isc_boolean_t dmodified = ISC_FALSE;

		arg = mapGet(expr, "binary-to-ascii");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		base = mapGet(arg, "base");
		if (base == NULL)
			return expr;
		width = mapGet(arg, "width");
		if (width == NULL)
			return expr;
		separator = mapGet(arg, "separator");
		if (separator == NULL)
			return expr;
		buffer = mapGet(arg, "buffer");
		if (buffer == NULL)
			return expr;
		base = eval_numeric_expression(base, &bmodified);
		if (bmodified) {
			mapRemove(arg, "base");
			mapSet(arg, base, "base");
		}
		width = eval_numeric_expression(width, &wmodified);
		if (wmodified) {
			mapRemove(arg, "width");
			mapSet(arg, width, "width");
		}
		separator = eval_data_expression(separator, &smodified);
		if (smodified) {
			mapRemove(arg, "separator");
			mapSet(arg, separator, "separator");
		}
		buffer = eval_data_expression(buffer, &dmodified);
		if (dmodified) {
			mapRemove(arg, "buffer");
			mapSet(arg, buffer, "buffer");
		}

		if ((base->type != ELEMENT_INTEGER) ||
		    (width->type != ELEMENT_INTEGER) ||
		    (separator->type != ELEMENT_STRING) ||
		    (buffer->type != ELEMENT_STRING))
			return expr;
		b = intValue(base);
		if ((b < 2) || (b > 16))
			return expr;
		if ((b != 8) && (b != 10) && (b != 16))
			return expr;
		w = intValue(width);
		if ((w != 8) && (w != 16) && (w != 32))
			return expr;
		sep = stringValue(separator);
		buf = stringValue(buffer);
		r = allocString();
		if (w == 8) {
			size_t i;
			char *fmt;

			switch (b) {
			case 8:
				fmt = "%o";
				break;
			case 10:
				fmt = "%d";
				break;
			case 16:
            default:
				fmt = "%x";
				break;
			}

			for (i = 0; i < buf->length; i++) {
				uint8_t val;
				char num[4];

				if (i != 0)
					concatString(r, sep);
				val = (uint8_t)buf->content[i];
				snprintf(num, sizeof(num), fmt, (int)val);
				appendString(r, num);
			}
		} else if (w == 16) {
			size_t i;
			char *fmt;

			if ((buf->length % 2) != 0)
				return expr;

			switch (b) {
			case 8:
				fmt = "%o";
				break;
			case 10:
				fmt = "%d";
				break;
			case 16:
            default:
				fmt = "%x";
				break;
			}

			for (i = 0; i < buf->length; i += 2) {
				uint16_t val;
				char num[8];

				if (i != 0)
					concatString(r, sep);
				memcpy(&val, buf->content + i, 2);
				val = ntohs(val);
				snprintf(num, sizeof(num), fmt, (int)val);
				appendString(r, num);
			}
		} else if (w == 32) {
			size_t i;
			char *fmt;

			if ((buf->length % 4) != 0)
				return expr;

			switch (b) {
			case 8:
				fmt = "%llo";
				break;
			case 10:
				fmt = "%lld";
				break;
			case 16:
            default:
				fmt = "%llx";
				break;
			}

			for (i = 0; i < buf->length; i += 4) {
				uint32_t val;
				char num[40];

				if (i != 0)
					concatString(r, sep);
				memcpy(&val, buf->content + i, 4);
				val = ntohl(val);
				snprintf(num, sizeof(num), fmt,
					 (long long)val);
				appendString(r, num);
			}
		}
		*modifiedp = ISC_TRUE;
		result = createString(r);
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		TAILQ_CONCAT(&result->comments, &base->comments);
		TAILQ_CONCAT(&result->comments, &width->comments);
		TAILQ_CONCAT(&result->comments, &separator->comments);
		TAILQ_CONCAT(&result->comments, &buffer->comments);
		return result;
	}

	/* filename */
	if (mapContains(expr, "filename"))
		/*
		 * syntax := { "filename": null }
		 * semantic: get filename field from incoming DHCPv4 packet
		 */
		return expr;

	/* server-name */
	if (mapContains(expr, "server-name"))
		/*
		 * syntax := { "server-name": null }
		 * semantic: get server-name field from incoming DHCPv4 packet
		 */
		return expr;

	/* reverse */
	if (mapContains(expr, "reverse")) {
		/*
		 * syntax := { "reverse":
		 *             { "width": <numeric_expression>,
		 *               "buffer":    <data_expression> }
		 *           }
		 * semantic: reverse the input buffer by width chunks of bytes
		 */
		struct element *arg;
		struct element *width;
		struct element *buffer;
		struct element *result;
		struct string *buf;
		struct string *r;
		int64_t w;
		size_t i;
		isc_boolean_t wmodified = ISC_FALSE;
		isc_boolean_t bmodified = ISC_FALSE;

		arg = mapGet(expr, "reverse");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		width = mapGet(arg, "width");
		if (width == NULL)
			return expr;
		buffer = mapGet(arg, "buffer");
		if (buffer == NULL)
			return expr;
		width = eval_numeric_expression(width, &wmodified);
		if (wmodified) {
			mapRemove(arg, "width");
			mapSet(arg, width, "width");
		}
		buffer = eval_data_expression(buffer, &bmodified);
		if (bmodified) {
			mapRemove(arg, "buffer");
			mapSet(arg, buffer, "buffer");
		}

		if ((width->type != ELEMENT_INTEGER) ||
		    (buffer->type != ELEMENT_STRING))
			return expr;
		w = intValue(width);
		if (w <= 0)
			return expr;
		buf = stringValue(buffer);
		if ((buf->length % w) != 0)
			return expr;
		*modifiedp = ISC_TRUE;
		r = allocString();
		concatString(r, buf);
		for (i = 0; i < buf->length; i += w) {
			memcpy(r->content + i,
			       buf->content + (buf->length - i - w),
			       w);
		}
		result = createString(r);
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		TAILQ_CONCAT(&result->comments, &width->comments);
		TAILQ_CONCAT(&result->comments, &buffer->comments);
		return result;
	}

	/* pick-first-value */
	if (mapContains(expr, "pick-first-value")) {
		/*
		 * syntax := { "pick-first-value":
		 *             [ <data_expression>, ... ]
		 *           }
		 * semantic: evaluates expressions and return the first
		 * not null, return null if all are null
		 */
		struct element *arg;
		struct element *result;
		size_t i;
		isc_boolean_t modified;
		isc_boolean_t can_decide = ISC_TRUE;

		arg = mapGet(expr, "pick-first-value");
		if ((arg == NULL) || (arg->type != ELEMENT_LIST))
			return expr;

		for (i = 0; i < listSize(arg); i++) {
			struct element *item;

			item = listGet(arg, i);
			if (item == NULL)
				return expr;
			modified = ISC_FALSE;
			item = eval_data_expression(item, &modified);
			if (modified)
				listRemove(arg, i);
			if (!can_decide)
				goto restore;
			if (item->type != ELEMENT_STRING) {
				can_decide = ISC_FALSE;
				goto restore;
			}
			if (stringValue(item)->length != 0) {
				*modifiedp = ISC_TRUE;
				TAILQ_CONCAT(&item->comments, &expr->comments);
				TAILQ_CONCAT(&item->comments, &arg->comments);
				return item;
			}
		restore:
			listSet(arg, item, i);
		}
		if (!can_decide)
			return expr;
		*modifiedp = ISC_TRUE;
		result = createString(allocString());
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments,  &arg->comments);
		return result;
	}

	/* host-decl-name */
	if (mapContains(expr, "host-decl-name"))
		/*
		 * syntax := { "host-decl-name": null }
		 * semantic: return the name of the matching host
		 * declaration (aka revervation in kea) or null
		 */
		return expr;

	/* leased-address */
	if (mapContains(expr, "leased-address"))
		/*
		 * syntax := { "leased-address": null }
		 * semantic: return the address of the assigned lease or
		 * log a message
		 */
		return expr;

	/* config-option */
	if (mapContains(expr, "config-option"))
		/*
		 * syntax := { "config-option":
		 *             { "universe": <option_space_old>,
		 *               "name":  <option_name> }
		 *           }
		 * semantic: get universe/code option to send
		 */
		return expr;

	/* null */
	if (mapContains(expr, "null")) {
		/*
		 * syntax := { "null": null }
		 * semantic: return null
		 */
		struct element *result;

		*modifiedp = ISC_TRUE;
		result = createString(allocString());
		TAILQ_CONCAT(&result->comments, &expr->comments);
		return result;
	}

	/* gethostname */
	if (mapContains(expr, "gethostname")) {
		/*
		 * syntax := { "gethostname": null }
		 * semantic: return gethostname
		 */
		struct element *result;
		char buf[300 /* >= 255 + 1 */];

		if (gethostname(buf, sizeof(buf)) != 0) {
			debug("gethostname fails: %s", strerror(errno));
			return expr;
		}
		*modifiedp = ISC_TRUE;
		result = createString(makeString(-1, buf));
		TAILQ_CONCAT(&result->comments, &expr->comments);
		return result;
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
		isc_boolean_t modified = ISC_FALSE;

		if (local_family != AF_INET6)
			return expr;
		arg = mapGet(expr, "v6relay");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		relay = mapGet(arg, "relay");
		if (relay == NULL)
			return expr;
		relay = eval_numeric_expression(relay, &modified);
		if (modified) {
			mapRemove(arg, "relay");
			mapSet(arg, relay, "relay");
		}
		return expr;
	}

	return expr;
}

/*
 * numeric-expression :== EXTRACT_INT LPAREN data-expression
 *                                           COMMA number RPAREN |
 *                        NUMBER
 */

struct element *
eval_numeric_expression(struct element *expr, isc_boolean_t *modifiedp)
{
	/* trivial case: already done */
	if (expr->type == ELEMENT_INTEGER)
		return expr;

	/*
	 * From is_numeric_expression
	 */

	if (expr->type != ELEMENT_MAP)
		return expr;

	/* extract-int8 */
	if (mapContains(expr, "extract-int8")) {
		/*
		 * syntax := { "extract-int8": <data_expression> }
		 * semantic: extract from the evalkuated string buffer
		 * a number
		 */
		struct element *arg;
		struct element *result;
		uint8_t val = 0;
		isc_boolean_t modified = ISC_FALSE;

		arg = mapGet(expr, "extract-int8");
		if (arg == NULL)
			return expr;
		arg = eval_data_expression(arg, &modified);
		if (modified) {
			mapRemove(expr, "extract-int8");
			mapSet(expr, arg, "extract-int8");
		}

		if (arg->type != ELEMENT_STRING)
			return expr;
		*modifiedp = ISC_TRUE;
		if (stringValue(arg)->length > 0)
			val = (uint8_t) stringValue(arg)->content[0];
		result = createInt(val);
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		return result;
	}

	/* extract-int16 */
	if (mapContains(expr, "extract-int16")) {
		/*
		 * syntax := { "extract-int16": <data_expression> }
		 * semantic: extract from the evalkuated string buffer
		 * a number
		 */
		struct element *arg;
		struct element *result;
		uint16_t val;
		isc_boolean_t modified = ISC_FALSE;

		arg = mapGet(expr, "extract-int16");
		if (arg == NULL)
			return expr;
		arg = eval_data_expression(arg, &modified);
		if (modified) {
			mapRemove(expr, "extract-int16");
			mapSet(expr, arg, "extract-int16");
		}

		if (arg->type != ELEMENT_STRING)
			return expr;
		if (stringValue(arg)->length < 2)
			return expr;
		*modifiedp = ISC_TRUE;
		memcpy(&val, stringValue(arg)->content, 2);
		val = ntohs(val);
		result = createInt(val);
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		return result;
	}

	/* extract-int32 */
	if (mapContains(expr, "extract-int32")) {
		/*
		 * syntax := { "extract-int32": <data_expression> }
		 * semantic: extract from the evalkuated string buffer
		 * a number
		 */
		struct element *arg;
		struct element *result;
		uint32_t val;
		isc_boolean_t modified = ISC_FALSE;

		arg = mapGet(expr, "extract-int32");
		if (arg == NULL)
			return expr;
		arg = eval_data_expression(arg, &modified);
		if (modified) {
			mapRemove(expr, "extract-int32");
			mapSet(expr, arg, "extract-int32");
		}

		if (arg->type != ELEMENT_STRING)
			return expr;
		if (stringValue(arg)->length < 4)
			return expr;
		*modifiedp = ISC_TRUE;
		memcpy(&val, stringValue(arg)->content, 4);
		val = ntohl(val);
		result = createInt(val);
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		return result;
	}

	/* const-int */
	if (mapContains(expr, "const-int")) {
		/*
		 * syntax := { "const-int": <integer> }
		 * semantic: embedded integer value
		 */
		struct element *arg;
		struct element *result;

		arg = mapGet(expr, "const-int");
		if ((arg == NULL) || (arg->type != ELEMENT_INTEGER))
			return expr;
		*modifiedp = ISC_TRUE;
		result = createInt(intValue(arg));
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		return result;
	}

	/* lease-time */
	if (mapContains(expr, "lease-time"))
		/*
		 * syntax := { "lease-time": null }
		 * semantic: return duration of the current lease, i.e
		 * the difference between expire time and now
		 */
		return expr;

	/* add */
	if (mapContains(expr, "add")) {
		/*
		 * syntax := { "add":
		 *             { "left":  <boolean_expression>,
		 *               "right": <boolean_expression> }
		 *           }
		 * semantics: evaluate branches, return left plus right
		 * branches
		 */
		struct element *arg;
		struct element *left;
		struct element *right;
		struct element *result;
		isc_boolean_t lmodified = ISC_FALSE;
		isc_boolean_t rmodified = ISC_FALSE;

		arg = mapGet(expr, "add");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		left = mapGet(arg, "left");
		if (left == NULL)
			return expr;
		right = mapGet(arg, "right");
		if (right == NULL)
			return expr;
		left = eval_numeric_expression(left, &lmodified);
		if (lmodified) {
			mapRemove(arg, "left");
			mapSet(arg, left, "left");
		}
		right = eval_numeric_expression(right, &rmodified);
		if (rmodified) {
			mapRemove(arg, "right");
			mapSet(arg, right, "right");
		}

		if ((left->type != ELEMENT_INTEGER) ||
		    (right->type != ELEMENT_INTEGER))
			return expr;
		*modifiedp = ISC_TRUE;
		result = createInt(intValue(left) + intValue(right));
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		TAILQ_CONCAT(&result->comments, &left->comments);
		TAILQ_CONCAT(&result->comments, &right->comments);
		return result;
	}

	/* subtract */
	if (mapContains(expr, "subtract")) {
		/*
		 * syntax := { "subtract":
		 *             { "left":  <boolean_expression>,
		 *               "right": <boolean_expression> }
		 *           }
		 * semantics: evaluate branches, return left plus right
		 * branches
		 */
		struct element *arg;
		struct element *left;
		struct element *right;
		struct element *result;
		isc_boolean_t lmodified = ISC_FALSE;
		isc_boolean_t rmodified = ISC_FALSE;

		arg = mapGet(expr, "subtract");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		left = mapGet(arg, "left");
		if (left == NULL)
			return expr;
		right = mapGet(arg, "right");
		if (right == NULL)
			return expr;
		left = eval_numeric_expression(left, &lmodified);
		if (lmodified) {
			mapRemove(arg, "left");
			mapSet(arg, left, "left");
		}
		right = eval_numeric_expression(right, &rmodified);
		if (rmodified) {
			mapRemove(arg, "right");
			mapSet(arg, right, "right");
		}

		if ((left->type != ELEMENT_INTEGER) ||
		    (right->type != ELEMENT_INTEGER))
			return expr;
		*modifiedp = ISC_TRUE;
		result = createInt(intValue(left) - intValue(right));
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		TAILQ_CONCAT(&result->comments, &left->comments);
		TAILQ_CONCAT(&result->comments, &right->comments);
		return result;
	}

	/* multiply */
	if (mapContains(expr, "multiply")) {
		/*
		 * syntax := { "multiply":
		 *             { "left":  <boolean_expression>,
		 *               "right": <boolean_expression> }
		 *           }
		 * semantics: evaluate branches, return left plus right
		 * branches
		 */
		struct element *arg;
		struct element *left;
		struct element *right;
		struct element *result;
		isc_boolean_t lmodified = ISC_FALSE;
		isc_boolean_t rmodified = ISC_FALSE;

		arg = mapGet(expr, "multiply");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		left = mapGet(arg, "left");
		if (left == NULL)
			return expr;
		right = mapGet(arg, "right");
		if (right == NULL)
			return expr;
		left = eval_numeric_expression(left, &lmodified);
		if (lmodified) {
			mapRemove(arg, "left");
			mapSet(arg, left, "left");
		}
		right = eval_numeric_expression(right, &rmodified);
		if (rmodified) {
			mapRemove(arg, "right");
			mapSet(arg, right, "right");
		}

		if ((left->type != ELEMENT_INTEGER) ||
		    (right->type != ELEMENT_INTEGER))
			return expr;
		*modifiedp = ISC_TRUE;
		result = createInt(intValue(left) * intValue(right));
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		TAILQ_CONCAT(&result->comments, &left->comments);
		TAILQ_CONCAT(&result->comments, &right->comments);
		return result;
	}

	/* divide */
	if (mapContains(expr, "divide")) {
		/*
		 * syntax := { "divide":
		 *             { "left":  <boolean_expression>,
		 *               "right": <boolean_expression> }
		 *           }
		 * semantics: evaluate branches, return left plus right
		 * branches
		 */
		struct element *arg;
		struct element *left;
		struct element *right;
		struct element *result;
		isc_boolean_t lmodified = ISC_FALSE;
		isc_boolean_t rmodified = ISC_FALSE;

		arg = mapGet(expr, "divide");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		left = mapGet(arg, "left");
		if (left == NULL)
			return expr;
		right = mapGet(arg, "right");
		if (right == NULL)
			return expr;
		left = eval_numeric_expression(left, &lmodified);
		if (lmodified) {
			mapRemove(arg, "left");
			mapSet(arg, left, "left");
		}
		right = eval_numeric_expression(right, &rmodified);
		if (rmodified) {
			mapRemove(arg, "right");
			mapSet(arg, right, "right");
		}

		if ((left->type != ELEMENT_INTEGER) ||
		    (right->type != ELEMENT_INTEGER))
			return expr;
		if (intValue(right) == 0)
			return expr;
		*modifiedp = ISC_TRUE;
		result = createInt(intValue(left) / intValue(right));
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		TAILQ_CONCAT(&result->comments, &left->comments);
		TAILQ_CONCAT(&result->comments, &right->comments);
		return result;
	}

	/* remainder */
	if (mapContains(expr, "remainder")) {
		/*
		 * syntax := { "remainder":
		 *             { "left":  <boolean_expression>,
		 *               "right": <boolean_expression> }
		 *           }
		 * semantics: evaluate branches, return left plus right
		 * branches
		 */
		struct element *arg;
		struct element *left;
		struct element *right;
		struct element *result;
		isc_boolean_t lmodified = ISC_FALSE;
		isc_boolean_t rmodified = ISC_FALSE;

		arg = mapGet(expr, "remainder");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		left = mapGet(arg, "left");
		if (left == NULL)
			return expr;
		right = mapGet(arg, "right");
		if (right == NULL)
			return expr;
		left = eval_numeric_expression(left, &lmodified);
		if (lmodified) {
			mapRemove(arg, "left");
			mapSet(arg, left, "left");
		}
		right = eval_numeric_expression(right, &rmodified);
		if (rmodified) {
			mapRemove(arg, "right");
			mapSet(arg, right, "right");
		}

		if ((left->type != ELEMENT_INTEGER) ||
		    (right->type != ELEMENT_INTEGER))
			return expr;
		if (intValue(right) == 0)
			return expr;
		*modifiedp = ISC_TRUE;
		result = createInt(intValue(left) % intValue(right));
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		TAILQ_CONCAT(&result->comments, &left->comments);
		TAILQ_CONCAT(&result->comments, &right->comments);
		return result;
	}

	/* binary-and */
	if (mapContains(expr, "binary-and")) {
		/*
		 * syntax := { "binary-and":
		 *             { "left":  <boolean_expression>,
		 *               "right": <boolean_expression> }
		 *           }
		 * semantics: evaluate branches, return left plus right
		 * branches
		 */
		struct element *arg;
		struct element *left;
		struct element *right;
		struct element *result;
		isc_boolean_t lmodified = ISC_FALSE;
		isc_boolean_t rmodified = ISC_FALSE;

		arg = mapGet(expr, "binary-and");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		left = mapGet(arg, "left");
		if (left == NULL)
			return expr;
		right = mapGet(arg, "right");
		if (right == NULL)
			return expr;
		left = eval_numeric_expression(left, &lmodified);
		if (lmodified) {
			mapRemove(arg, "left");
			mapSet(arg, left, "left");
		}
		right = eval_numeric_expression(right, &rmodified);
		if (rmodified) {
			mapRemove(arg, "right");
			mapSet(arg, right, "right");
		}

		if ((left->type != ELEMENT_INTEGER) ||
		    (right->type != ELEMENT_INTEGER))
			return expr;
		*modifiedp = ISC_TRUE;
		result = createInt(intValue(left) & intValue(right));
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		TAILQ_CONCAT(&result->comments, &left->comments);
		TAILQ_CONCAT(&result->comments, &right->comments);
		return result;
	}

	/* binary-or */
	if (mapContains(expr, "binary-or")) {
		/*
		 * syntax := { "binary-or":
		 *             { "left":  <boolean_expression>,
		 *               "right": <boolean_expression> }
		 *           }
		 * semantics: evaluate branches, return left plus right
		 * branches
		 */
		struct element *arg;
		struct element *left;
		struct element *right;
		struct element *result;
		isc_boolean_t lmodified = ISC_FALSE;
		isc_boolean_t rmodified = ISC_FALSE;

		arg = mapGet(expr, "binary-or");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		left = mapGet(arg, "left");
		if (left == NULL)
			return expr;
		right = mapGet(arg, "right");
		if (right == NULL)
			return expr;
		left = eval_numeric_expression(left, &lmodified);
		if (lmodified) {
			mapRemove(arg, "left");
			mapSet(arg, left, "left");
		}
		right = eval_numeric_expression(right, &rmodified);
		if (rmodified) {
			mapRemove(arg, "right");
			mapSet(arg, right, "right");
		}

		if ((left->type != ELEMENT_INTEGER) ||
		    (right->type != ELEMENT_INTEGER))
			return expr;
		*modifiedp = ISC_TRUE;
		result = createInt(intValue(left) | intValue(right));
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		TAILQ_CONCAT(&result->comments, &left->comments);
		TAILQ_CONCAT(&result->comments, &right->comments);
		return result;
	}

	/* binary-xor */
	if (mapContains(expr, "binary-xor")) {
		/*
		 * syntax := { "binary-xor":
		 *             { "left":  <boolean_expression>,
		 *               "right": <boolean_expression> }
		 *           }
		 * semantics: evaluate branches, return left plus right
		 * branches
		 */
		struct element *arg;
		struct element *left;
		struct element *right;
		struct element *result;
		isc_boolean_t lmodified = ISC_FALSE;
		isc_boolean_t rmodified = ISC_FALSE;

		arg = mapGet(expr, "binary-xor");
		if ((arg == NULL) || (arg->type != ELEMENT_MAP))
			return expr;
		left = mapGet(arg, "left");
		if (left == NULL)
			return expr;
		right = mapGet(arg, "right");
		if (right == NULL)
			return expr;
		left = eval_numeric_expression(left, &lmodified);
		if (lmodified) {
			mapRemove(arg, "left");
			mapSet(arg, left, "left");
		}
		right = eval_numeric_expression(right, &rmodified);
		if (rmodified) {
			mapRemove(arg, "right");
			mapSet(arg, right, "right");
		}

		if ((left->type != ELEMENT_INTEGER) ||
		    (right->type != ELEMENT_INTEGER))
			return expr;
		*modifiedp = ISC_TRUE;
		result = createInt(intValue(left) ^ intValue(right));
		TAILQ_CONCAT(&result->comments, &expr->comments);
		TAILQ_CONCAT(&result->comments, &arg->comments);
		TAILQ_CONCAT(&result->comments, &left->comments);
		TAILQ_CONCAT(&result->comments, &right->comments);
		return result;
	}

	/* client-state */
	if (mapContains(expr, "client-state"))
		/*
		 * syntax := { "client-state": null }
		 * semantic: return client state
		 */
		return expr;

	return expr;
}

/*
 * Check if the two evaluated expressions are equal, not equal,
 * or we can't decide.
 */

static struct element *
eval_equal_expression(struct element *left, struct element *right)
{
	struct element *result = NULL;
	isc_boolean_t val;

	/* in theory boolean is not possible */
	if (left->type == ELEMENT_BOOLEAN) {
		if (right->type == ELEMENT_BOOLEAN)
			val = ISC_TF(boolValue(left) == boolValue(right));
		else if (right->type == ELEMENT_MAP)
			return NULL;
		else
			val = ISC_FALSE;
	} else
	/* right is boolean */
	if (right->type == ELEMENT_BOOLEAN) {
		if (left->type == ELEMENT_MAP)
			return NULL;
		else
			val = ISC_FALSE;
	} else
	/* left is numeric literal */
	if (left->type == ELEMENT_INTEGER) {
		if (right->type == ELEMENT_INTEGER)
			val = ISC_TF(intValue(left) == intValue(right));
		else if ((right->type == ELEMENT_MAP) &&
			 mapContains(right, "const-int")) {
			struct element *ci;

			ci = mapGet(right, "const-int");
			if ((ci == NULL) || (ci->type != ELEMENT_INTEGER)) {
				debug("bad const-int");
				return NULL;
			}
			val = ISC_TF(intValue(left) == intValue(ci));
		} else if (right->type == ELEMENT_MAP)
			return NULL;
		else
			val = ISC_FALSE;
	} else
	/* left is const-int */
	if ((left->type == ELEMENT_MAP) && mapContains(left, "const-int")) {
		if (right->type == ELEMENT_INTEGER) {
			struct element *ci;

			ci = mapGet(left, "const-int");
			if ((ci == NULL) || (ci->type != ELEMENT_INTEGER)) {
				debug("bad const-int");
				return NULL;
			}
			val = ISC_TF(intValue(ci) == intValue(right));
		} else if ((right->type == ELEMENT_MAP) &&
			   mapContains(right, "const-int")) {
			struct element *lci;
			struct element *rci;

			lci = mapGet(left, "const-int");
			rci = mapGet(right, "const-int");
			if ((lci == NULL) || (lci->type != ELEMENT_INTEGER) ||
			    (rci == NULL) || (rci->type != ELEMENT_INTEGER)) {
				debug("bad const-int");
				return NULL;
			}
			val = ISC_TF(intValue(lci) == intValue(rci));
		} else if (right->type == ELEMENT_MAP)
			return NULL;
		else
			val = ISC_FALSE;
	} else
	/* right is numeric literal */
	if (right->type == ELEMENT_INTEGER) {
		if (left->type == ELEMENT_MAP)
			return NULL;
		else
			val = ISC_FALSE;
	} else
	/* right is const-int */
	if ((right->type == ELEMENT_MAP) && mapContains(right, "const-int")) {
		if (left->type == ELEMENT_MAP)
			return NULL;
		else
			val = ISC_FALSE;
	} else
	/* left is data literal */
	if (left->type == ELEMENT_STRING) {
		if (right->type == ELEMENT_STRING)
			val = cmp_hexa(left, ISC_FALSE, right, ISC_FALSE);
		else if ((right->type == ELEMENT_MAP) &&
			 mapContains(right, "const-data")) {
			struct element *cd;

			cd = mapGet(right, "const-data");
			if ((cd == NULL) || (cd->type != ELEMENT_STRING)) {
				debug("bad const-data");
				return NULL;
			}
			val = cmp_hexa(left, ISC_FALSE, cd, ISC_TRUE);
		} else if (right->type == ELEMENT_MAP)
			return NULL;
		else
			val = ISC_FALSE;
	} else
	/* left is const-data */
	if ((left->type == ELEMENT_MAP) && mapContains(left, "const-data")) {
		if (right->type == ELEMENT_STRING) {
			struct element *cd;

			cd = mapGet(left, "const-data");
			if ((cd == NULL) || (cd->type != ELEMENT_STRING)) {
				debug("bad const-data");
				return NULL;
			}
			val = cmp_hexa(cd, ISC_TRUE, right, ISC_FALSE);
		} else if ((right->type == ELEMENT_MAP) &&
			   mapContains(right, "const-data")) {
			struct element *lcd;
			struct element *rcd;

			lcd = mapGet(left, "const-data");
			rcd = mapGet(right, "const-data");
			if ((lcd == NULL) || (lcd->type != ELEMENT_STRING) ||
			    (rcd == NULL) || (rcd->type != ELEMENT_STRING)) {
				debug("bad const-data");
				return NULL;
			}
			val = cmp_hexa(lcd, ISC_TRUE, rcd, ISC_TRUE);
		} else if (right->type == ELEMENT_MAP)
			return NULL;
		else
			val = ISC_FALSE;
	} else
	/* right is data literal */
	if (right->type == ELEMENT_STRING) {
		if (left->type == ELEMENT_MAP)
			return NULL;
		else
			val = ISC_FALSE;
	} else
	/* right is const-data */
	if ((right->type == ELEMENT_MAP) && mapContains(right, "const-data")) {
		if (left->type == ELEMENT_MAP)
			return NULL;
		else
			val = ISC_FALSE;
	} else
	/* impossible cases */
	if ((left->type != ELEMENT_MAP) || (right->type != ELEMENT_MAP)) {
		debug("equal between unexpected %s and %s",
		      type2name(left->type), type2name(right->type));
		val = ISC_FALSE;
	} else
	/* can't decide */
		return NULL;

	result = createBool(val);
	TAILQ_CONCAT(&result->comments, &left->comments);
	TAILQ_CONCAT(&result->comments, &right->comments);
	return result;
}

static isc_boolean_t
cmp_hexa(struct element *left, isc_boolean_t left_is_hexa,
	 struct element *right, isc_boolean_t right_is_hexa)
{
	struct string *sleft;
	struct string *sright;

	/* both are not hexa */
	if (!left_is_hexa && !right_is_hexa) {
		sleft = stringValue(left);
		sright = stringValue(right);
		/* eqString() compares lengths them use memcmp() */
		return eqString(sleft, sright);
	}

	/* both are hexa */
	if (left_is_hexa && right_is_hexa) {
		sleft = stringValue(left);
		sright = stringValue(right);
		if (sleft->length != sright->length)
			return ISC_FALSE;
		if (sleft->length == 0) {
			debug("empty const-data");
			return ISC_TRUE;
		}
		return ISC_TF(strcasecmp(sleft->content,
					 sright->content) == 0);
	}

	/* put the hexa at left */
	if (left_is_hexa) {
		sleft = hexaValue(left);
		sright = stringValue(right);
	} else {
		sleft = hexaValue(right);
		sright = stringValue(left);
	}

	/* hexa is double length */
	if (sleft->length != 2 * sright->length)
		return ISC_FALSE;

	/* build the hexa representation */
	makeStringExt(sright->length, sright->content, 'X');

	return ISC_TF(strcasecmp(sleft->content, sright->content) == 0);
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
