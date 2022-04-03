/*	$NetBSD: json.c,v 1.3 2022/04/03 01:10:59 christos Exp $	*/

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
__RCSID("$NetBSD: json.c,v 1.3 2022/04/03 01:10:59 christos Exp $");

/* From Kea src/lib/cc/data.cc fromJSON() */

#include "keama.h"

#include <stdlib.h>
#include <string.h>

struct element *
json_parse(struct parse *cfile)
{
	struct element *elem;
	const char *val;
	enum dhcp_token token;

	elem = create();
	stackPush(cfile, elem);
	cfile->stack[0] = elem;
	cfile->stack_top = 0;

	token = next_token(&val, NULL, cfile);
	switch (token) {
	case NUMBER:
		elem = createInt(atoll(val));
		TAILQ_CONCAT(&elem->comments, &cfile->comments);
		break;
	case STRING:
		elem = createString(makeString(-1, val));
		TAILQ_CONCAT(&elem->comments, &cfile->comments);
		break;
	case NAME:
		if (strcmp(val, "null") == 0)
			elem = createNull();
		else if (strcmp(val, "true") == 0)
			elem = createBool(ISC_TRUE);
		else if (strcmp(val, "false") == 0) {
			elem = createBool(ISC_FALSE);
			elem->skip = ISC_TRUE;
		} else
			parse_error(cfile, "unknown name %s", val);
		TAILQ_CONCAT(&elem->comments, &cfile->comments);
		break;
	case LBRACKET:
		elem = json_list_parse(cfile);
		break;
	case LBRACE:
		elem = json_map_parse(cfile);
		break;
	case END_OF_FILE:
		parse_error(cfile, "unexpected end of file");
	default:
		parse_error(cfile, "unexpected %s", val);
	}
	return elem;
}

struct element *
json_list_parse(struct parse *cfile)
{
	struct element *list;
	struct element *item;
	const char *val;
	enum dhcp_token token;
	isc_boolean_t done = ISC_FALSE;

	list = createList();
	TAILQ_CONCAT(&list->comments, &cfile->comments);
	stackPush(cfile, list);
	do {
		token = peek_token(&val, NULL, cfile);
		switch (token) {
		case RBRACKET:
			done = ISC_TRUE;
			break;
		case END_OF_FILE:
			parse_error(cfile, "unexpected end of file");
		case COMMA:
			skip_token(&val, NULL, cfile);
			if (listSize(list) == 0)
				parse_error(cfile, "unexpected ','");
			item = json_parse(cfile);
			listPush(list, item);
			break;
		default:
			if (listSize(list) > 0)
				parse_error(cfile, "expected ','");
			item = json_parse(cfile);
			listPush(list, item);
			break;
		}
	} while (!done);
	skip_token(&val, NULL, cfile);
	cfile->stack_top--;
	return list;
}

struct element *
json_map_parse(struct parse *cfile)
{
	struct element *map;
	struct element *item;
	const char *val;
	const char *key;
	enum dhcp_token token;
	isc_boolean_t done = ISC_FALSE;

	map = createMap();
	TAILQ_CONCAT(&map->comments, &cfile->comments);
	stackPush(cfile, map);
	do {
		token = peek_token(&val, NULL, cfile);
		switch (token) {
		case RBRACE:
			done = ISC_TRUE;
			break;
		case END_OF_FILE:
			parse_error(cfile, "unexpected end of file");
		case COMMA:
			skip_token(&val, NULL, cfile);
			if (mapSize(map) == 0)
				parse_error(cfile, "unexpected ','");
			token = next_token(&val, NULL, cfile);
			if (token != STRING)
				parse_error(cfile, "unexpected %s, "
					    "expected \"key\":value", val);
			key = strdup(val);
			token = next_token(&val, NULL, cfile);
			if (token != COLON)
				parse_error(cfile, "unexpected %s, "
					    "expected ':'", val);
			item = json_parse(cfile);
			mapSet(map, item, key);
			break;
		case STRING:
			skip_token(&val, NULL, cfile);
			if (mapSize(map) > 0)
				parse_error(cfile, "unexpected \"%s\", "
					    "expected ','", val);
			key = strdup(val);
			token = next_token(&val, NULL, cfile);
			if (token != COLON)
				parse_error(cfile, "unexpected %s, "
					    "expected ':'", val);
			item = json_parse(cfile);
			mapSet(map, item, key);
			break;
		default:
			if (mapSize(map) == 0)
				parse_error(cfile, "unexpected %s, "
					    "expected \"key\":value or '}'",
					    val);
			else
				parse_error(cfile, "unexpected %s, "
					    "expected ',' or '}'", val);
		}
	} while (!done);
	skip_token(&val, NULL, cfile);
	cfile->stack_top--;
	return map;
}
