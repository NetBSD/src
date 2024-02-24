/*	$NetBSD: regex.c,v 1.1.2.2 2024/02/24 13:07:21 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <stdbool.h>

#include <isc/file.h>
#include <isc/print.h>
#include <isc/regex.h>
#include <isc/string.h>

#if VALREGEX_REPORT_REASON
#define FAIL(x)               \
	do {                  \
		reason = (x); \
		goto error;   \
	} while (0)
#else /* if VALREGEX_REPORT_REASON */
#define FAIL(x) goto error
#endif /* if VALREGEX_REPORT_REASON */

/*
 * Validate the regular expression 'C' locale.
 */
int
isc_regex_validate(const char *c) {
	enum {
		none,
		parse_bracket,
		parse_bound,
		parse_ce,
		parse_ec,
		parse_cc
	} state = none;
	/* Well known character classes. */
	const char *cc[] = { ":alnum:", ":digit:", ":punct:", ":alpha:",
			     ":graph:", ":space:", ":blank:", ":lower:",
			     ":upper:", ":cntrl:", ":print:", ":xdigit:" };
	bool seen_comma = false;
	bool seen_high = false;
	bool seen_char = false;
	bool seen_ec = false;
	bool seen_ce = false;
	bool have_atom = false;
	int group = 0;
	int range = 0;
	int sub = 0;
	bool empty_ok = false;
	bool neg = false;
	bool was_multiple = false;
	unsigned int low = 0;
	unsigned int high = 0;
	const char *ccname = NULL;
	int range_start = 0;
#if VALREGEX_REPORT_REASON
	const char *reason = "";
#endif /* if VALREGEX_REPORT_REASON */

	if (c == NULL || *c == 0) {
		FAIL("empty string");
	}

	while (c != NULL && *c != 0) {
		switch (state) {
		case none:
			switch (*c) {
			case '\\': /* make literal */
				++c;
				switch (*c) {
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if ((*c - '0') > sub) {
						FAIL("bad back reference");
					}
					have_atom = true;
					was_multiple = false;
					break;
				case 0:
					FAIL("escaped end-of-string");
				default:
					goto literal;
				}
				++c;
				break;
			case '[': /* bracket start */
				++c;
				neg = false;
				was_multiple = false;
				seen_char = false;
				state = parse_bracket;
				break;
			case '{': /* bound start */
				switch (c[1]) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (!have_atom) {
						FAIL("no atom");
					}
					if (was_multiple) {
						FAIL("was multiple");
					}
					seen_comma = false;
					seen_high = false;
					low = high = 0;
					state = parse_bound;
					break;
				default:
					goto literal;
				}
				++c;
				have_atom = true;
				was_multiple = true;
				break;
			case '}':
				goto literal;
			case '(': /* group start */
				have_atom = false;
				was_multiple = false;
				empty_ok = true;
				++group;
				++sub;
				++c;
				break;
			case ')': /* group end */
				if (group && !have_atom && !empty_ok) {
					FAIL("empty alternative");
				}
				have_atom = true;
				was_multiple = false;
				if (group != 0) {
					--group;
				}
				++c;
				break;
			case '|': /* alternative separator */
				if (!have_atom) {
					FAIL("no atom");
				}
				have_atom = false;
				empty_ok = false;
				was_multiple = false;
				++c;
				break;
			case '^':
			case '$':
				have_atom = true;
				was_multiple = true;
				++c;
				break;
			case '+':
			case '*':
			case '?':
				if (was_multiple) {
					FAIL("was multiple");
				}
				if (!have_atom) {
					FAIL("no atom");
				}
				have_atom = true;
				was_multiple = true;
				++c;
				break;
			case '.':
			default:
			literal:
				have_atom = true;
				was_multiple = false;
				++c;
				break;
			}
			break;
		case parse_bound:
			switch (*c) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if (!seen_comma) {
					low = low * 10 + *c - '0';
					if (low > 255) {
						FAIL("lower bound too big");
					}
				} else {
					seen_high = true;
					high = high * 10 + *c - '0';
					if (high > 255) {
						FAIL("upper bound too big");
					}
				}
				++c;
				break;
			case ',':
				if (seen_comma) {
					FAIL("multiple commas");
				}
				seen_comma = true;
				++c;
				break;
			default:
			case '{':
				FAIL("non digit/comma");
			case '}':
				if (seen_high && low > high) {
					FAIL("bad parse bound");
				}
				seen_comma = false;
				state = none;
				++c;
				break;
			}
			break;
		case parse_bracket:
			switch (*c) {
			case '^':
				if (seen_char || neg) {
					goto inside;
				}
				neg = true;
				++c;
				break;
			case '-':
				if (range == 2) {
					goto inside;
				}
				if (!seen_char) {
					goto inside;
				}
				if (range == 1) {
					FAIL("bad range");
				}
				range = 2;
				++c;
				break;
			case '[':
				++c;
				switch (*c) {
				case '.': /* collating element */
					if (range != 0) {
						--range;
					}
					++c;
					state = parse_ce;
					seen_ce = false;
					break;
				case '=': /* equivalence class */
					if (range == 2) {
						FAIL("equivalence class in "
						     "range");
					}
					++c;
					state = parse_ec;
					seen_ec = false;
					break;
				case ':': /* character class */
					if (range == 2) {
						FAIL("character class in "
						     "range");
					}
					ccname = c;
					++c;
					state = parse_cc;
					break;
				}
				seen_char = true;
				break;
			case ']':
				if (!c[1] && !seen_char) {
					FAIL("unfinished brace");
				}
				if (!seen_char) {
					goto inside;
				}
				++c;
				range = 0;
				have_atom = true;
				state = none;
				break;
			default:
			inside:
				seen_char = true;
				if (range == 2 && (*c & 0xff) < range_start) {
					FAIL("out of order range");
				}
				if (range != 0) {
					--range;
				}
				range_start = *c & 0xff;
				++c;
				break;
			}
			break;
		case parse_ce:
			switch (*c) {
			case '.':
				++c;
				switch (*c) {
				case ']':
					if (!seen_ce) {
						FAIL("empty ce");
					}
					++c;
					state = parse_bracket;
					break;
				default:
					if (seen_ce) {
						range_start = 256;
					} else {
						range_start = '.';
					}
					seen_ce = true;
					break;
				}
				break;
			default:
				if (seen_ce) {
					range_start = 256;
				} else {
					range_start = *c;
				}
				seen_ce = true;
				++c;
				break;
			}
			break;
		case parse_ec:
			switch (*c) {
			case '=':
				++c;
				switch (*c) {
				case ']':
					if (!seen_ec) {
						FAIL("no ec");
					}
					++c;
					state = parse_bracket;
					break;
				default:
					seen_ec = true;
					break;
				}
				break;
			default:
				seen_ec = true;
				++c;
				break;
			}
			break;
		case parse_cc:
			switch (*c) {
			case ':':
				++c;
				switch (*c) {
				case ']': {
					unsigned int i;
					bool found = false;
					for (i = 0;
					     i < sizeof(cc) / sizeof(*cc); i++)
					{
						unsigned int len;
						len = strlen(cc[i]);
						if (len !=
						    (unsigned int)(c - ccname))
						{
							continue;
						}
						if (strncmp(cc[i], ccname, len))
						{
							continue;
						}
						found = true;
					}
					if (!found) {
						FAIL("unknown cc");
					}
					++c;
					state = parse_bracket;
					break;
				}
				default:
					break;
				}
				break;
			default:
				++c;
				break;
			}
			break;
		}
	}
	if (group != 0) {
		FAIL("group open");
	}
	if (state != none) {
		FAIL("incomplete");
	}
	if (!have_atom) {
		FAIL("no atom");
	}
	return (sub);

error:
#if VALREGEX_REPORT_REASON
	fprintf(stderr, "%s\n", reason);
#endif /* if VALREGEX_REPORT_REASON */
	return (-1);
}
