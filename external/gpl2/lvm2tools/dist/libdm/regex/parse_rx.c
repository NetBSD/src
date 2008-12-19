/*	$NetBSD: parse_rx.c,v 1.2 2008/12/19 15:24:09 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "dmlib.h"
#include "parse_rx.h"

struct parse_sp {		/* scratch pad for the parsing process */
	struct dm_pool *mem;
	int type;		/* token type, 0 indicates a charset */
	dm_bitset_t charset;	/* The current charset */
	const char *cursor;	/* where we are in the regex */
	const char *rx_end;	/* 1pte for the expression being parsed */
};

static struct rx_node *_or_term(struct parse_sp *ps);

static void _single_char(struct parse_sp *ps, unsigned int c, const char *ptr)
{
	ps->type = 0;
	ps->cursor = ptr + 1;
	dm_bit_clear_all(ps->charset);
	dm_bit_set(ps->charset, c);
}

/*
 * Get the next token from the regular expression.
 * Returns: 1 success, 0 end of input, -1 error.
 */
static int _rx_get_token(struct parse_sp *ps)
{
	int neg = 0, range = 0;
	char c, lc = 0;
	const char *ptr = ps->cursor;
	if (ptr == ps->rx_end) {	/* end of input ? */
		ps->type = -1;
		return 0;
	}

	switch (*ptr) {
		/* charsets and ncharsets */
	case '[':
		ptr++;
		if (*ptr == '^') {
			dm_bit_set_all(ps->charset);

			/* never transition on zero */
			dm_bit_clear(ps->charset, 0);
			neg = 1;
			ptr++;

		} else
			dm_bit_clear_all(ps->charset);

		while ((ptr < ps->rx_end) && (*ptr != ']')) {
			if (*ptr == '\\') {
				/* an escaped character */
				ptr++;
				switch (*ptr) {
				case 'n':
					c = '\n';
					break;
				case 'r':
					c = '\r';
					break;
				case 't':
					c = '\t';
					break;
				default:
					c = *ptr;
				}
			} else if (*ptr == '-' && lc) {
				/* we've got a range on our hands */
				range = 1;
				ptr++;
				if (ptr == ps->rx_end) {
					log_error("Incomplete range"
						  "specification");
					return -1;
				}
				c = *ptr;
			} else
				c = *ptr;

			if (range) {
				/* add lc - c into the bitset */
				if (lc > c) {
					char tmp = c;
					c = lc;
					lc = tmp;
				}

				for (; lc <= c; lc++) {
					if (neg)
						dm_bit_clear(ps->charset, lc);
					else
						dm_bit_set(ps->charset, lc);
				}
				range = 0;
			} else {
				/* add c into the bitset */
				if (neg)
					dm_bit_clear(ps->charset, c);
				else
					dm_bit_set(ps->charset, c);
			}
			ptr++;
			lc = c;
		}

		if (ptr >= ps->rx_end) {
			ps->type = -1;
			return -1;
		}

		ps->type = 0;
		ps->cursor = ptr + 1;
		break;

		/* These characters are special, we just return their ASCII
		   codes as the type.  Sorted into ascending order to help the
		   compiler */
	case '(':
	case ')':
	case '*':
	case '+':
	case '?':
	case '|':
		ps->type = (int) *ptr;
		ps->cursor = ptr + 1;
		break;

	case '^':
		_single_char(ps, HAT_CHAR, ptr);
		break;

	case '$':
		_single_char(ps, DOLLAR_CHAR, ptr);
		break;

	case '.':
		/* The 'all but newline' character set */
		ps->type = 0;
		ps->cursor = ptr + 1;
		dm_bit_set_all(ps->charset);
		dm_bit_clear(ps->charset, (int) '\n');
		dm_bit_clear(ps->charset, (int) '\r');
		dm_bit_clear(ps->charset, 0);
		break;

	case '\\':
		/* escaped character */
		ptr++;
		if (ptr >= ps->rx_end) {
			log_error("Badly quoted character at end "
				  "of expression");
			ps->type = -1;
			return -1;
		}

		ps->type = 0;
		ps->cursor = ptr + 1;
		dm_bit_clear_all(ps->charset);
		switch (*ptr) {
		case 'n':
			dm_bit_set(ps->charset, (int) '\n');
			break;
		case 'r':
			dm_bit_set(ps->charset, (int) '\r');
			break;
		case 't':
			dm_bit_set(ps->charset, (int) '\t');
			break;
		default:
			dm_bit_set(ps->charset, (int) *ptr);
		}
		break;

	default:
		/* add a single character to the bitset */
		ps->type = 0;
		ps->cursor = ptr + 1;
		dm_bit_clear_all(ps->charset);
		dm_bit_set(ps->charset, (int) *ptr);
		break;
	}

	return 1;
}

static struct rx_node *_node(struct dm_pool *mem, int type,
			     struct rx_node *l, struct rx_node *r)
{
	struct rx_node *n = dm_pool_zalloc(mem, sizeof(*n));

	if (n) {
		if (!(n->charset = dm_bitset_create(mem, 256))) {
			dm_pool_free(mem, n);
			return NULL;
		}

		n->type = type;
		n->left = l;
		n->right = r;
	}

	return n;
}

static struct rx_node *_term(struct parse_sp *ps)
{
	struct rx_node *n;

	switch (ps->type) {
	case 0:
		if (!(n = _node(ps->mem, CHARSET, NULL, NULL))) {
			stack;
			return NULL;
		}

		dm_bit_copy(n->charset, ps->charset);
		_rx_get_token(ps);	/* match charset */
		break;

	case '(':
		_rx_get_token(ps);	/* match '(' */
		n = _or_term(ps);
		if (ps->type != ')') {
			log_error("missing ')' in regular expression");
			return 0;
		}
		_rx_get_token(ps);	/* match ')' */
		break;

	default:
		n = 0;
	}

	return n;
}

static struct rx_node *_closure_term(struct parse_sp *ps)
{
	struct rx_node *l, *n;

	if (!(l = _term(ps)))
		return NULL;

	for (;;) {
		switch (ps->type) {
		case '*':
			n = _node(ps->mem, STAR, l, NULL);
			break;

		case '+':
			n = _node(ps->mem, PLUS, l, NULL);
			break;

		case '?':
			n = _node(ps->mem, QUEST, l, NULL);
			break;

		default:
			return l;
		}

		if (!n) {
			stack;
			return NULL;
		}

		_rx_get_token(ps);
		l = n;
	}

	return n;
}

static struct rx_node *_cat_term(struct parse_sp *ps)
{
	struct rx_node *l, *r, *n;

	if (!(l = _closure_term(ps)))
		return NULL;

	if (ps->type == '|')
		return l;

	if (!(r = _cat_term(ps)))
		return l;

	if (!(n = _node(ps->mem, CAT, l, r)))
		stack;

	return n;
}

static struct rx_node *_or_term(struct parse_sp *ps)
{
	struct rx_node *l, *r, *n;

	if (!(l = _cat_term(ps)))
		return NULL;

	if (ps->type != '|')
		return l;

	_rx_get_token(ps);		/* match '|' */

	if (!(r = _or_term(ps))) {
		log_error("Badly formed 'or' expression");
		return NULL;
	}

	if (!(n = _node(ps->mem, OR, l, r)))
		stack;

	return n;
}

struct rx_node *rx_parse_tok(struct dm_pool *mem,
			     const char *begin, const char *end)
{
	struct rx_node *r;
	struct parse_sp *ps = dm_pool_zalloc(mem, sizeof(*ps));

	if (!ps) {
		stack;
		return NULL;
	}

	ps->mem = mem;
	ps->charset = dm_bitset_create(mem, 256);
	ps->cursor = begin;
	ps->rx_end = end;
	_rx_get_token(ps);		/* load the first token */

	if (!(r = _or_term(ps))) {
		log_error("Parse error in regex");
		dm_pool_free(mem, ps);
	}

	return r;
}

struct rx_node *rx_parse_str(struct dm_pool *mem, const char *str)
{
	return rx_parse_tok(mem, str, str + strlen(str));
}
/*	$NetBSD: parse_rx.c,v 1.2 2008/12/19 15:24:09 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "dmlib.h"
#include "parse_rx.h"

struct parse_sp {		/* scratch pad for the parsing process */
	struct dm_pool *mem;
	int type;		/* token type, 0 indicates a charset */
	dm_bitset_t charset;	/* The current charset */
	const char *cursor;	/* where we are in the regex */
	const char *rx_end;	/* 1pte for the expression being parsed */
};

static struct rx_node *_or_term(struct parse_sp *ps);

static void _single_char(struct parse_sp *ps, unsigned int c, const char *ptr)
{
	ps->type = 0;
	ps->cursor = ptr + 1;
	dm_bit_clear_all(ps->charset);
	dm_bit_set(ps->charset, c);
}

/*
 * Get the next token from the regular expression.
 * Returns: 1 success, 0 end of input, -1 error.
 */
static int _rx_get_token(struct parse_sp *ps)
{
	int neg = 0, range = 0;
	char c, lc = 0;
	const char *ptr = ps->cursor;
	if (ptr == ps->rx_end) {	/* end of input ? */
		ps->type = -1;
		return 0;
	}

	switch (*ptr) {
		/* charsets and ncharsets */
	case '[':
		ptr++;
		if (*ptr == '^') {
			dm_bit_set_all(ps->charset);

			/* never transition on zero */
			dm_bit_clear(ps->charset, 0);
			neg = 1;
			ptr++;

		} else
			dm_bit_clear_all(ps->charset);

		while ((ptr < ps->rx_end) && (*ptr != ']')) {
			if (*ptr == '\\') {
				/* an escaped character */
				ptr++;
				switch (*ptr) {
				case 'n':
					c = '\n';
					break;
				case 'r':
					c = '\r';
					break;
				case 't':
					c = '\t';
					break;
				default:
					c = *ptr;
				}
			} else if (*ptr == '-' && lc) {
				/* we've got a range on our hands */
				range = 1;
				ptr++;
				if (ptr == ps->rx_end) {
					log_error("Incomplete range"
						  "specification");
					return -1;
				}
				c = *ptr;
			} else
				c = *ptr;

			if (range) {
				/* add lc - c into the bitset */
				if (lc > c) {
					char tmp = c;
					c = lc;
					lc = tmp;
				}

				for (; lc <= c; lc++) {
					if (neg)
						dm_bit_clear(ps->charset, lc);
					else
						dm_bit_set(ps->charset, lc);
				}
				range = 0;
			} else {
				/* add c into the bitset */
				if (neg)
					dm_bit_clear(ps->charset, c);
				else
					dm_bit_set(ps->charset, c);
			}
			ptr++;
			lc = c;
		}

		if (ptr >= ps->rx_end) {
			ps->type = -1;
			return -1;
		}

		ps->type = 0;
		ps->cursor = ptr + 1;
		break;

		/* These characters are special, we just return their ASCII
		   codes as the type.  Sorted into ascending order to help the
		   compiler */
	case '(':
	case ')':
	case '*':
	case '+':
	case '?':
	case '|':
		ps->type = (int) *ptr;
		ps->cursor = ptr + 1;
		break;

	case '^':
		_single_char(ps, HAT_CHAR, ptr);
		break;

	case '$':
		_single_char(ps, DOLLAR_CHAR, ptr);
		break;

	case '.':
		/* The 'all but newline' character set */
		ps->type = 0;
		ps->cursor = ptr + 1;
		dm_bit_set_all(ps->charset);
		dm_bit_clear(ps->charset, (int) '\n');
		dm_bit_clear(ps->charset, (int) '\r');
		dm_bit_clear(ps->charset, 0);
		break;

	case '\\':
		/* escaped character */
		ptr++;
		if (ptr >= ps->rx_end) {
			log_error("Badly quoted character at end "
				  "of expression");
			ps->type = -1;
			return -1;
		}

		ps->type = 0;
		ps->cursor = ptr + 1;
		dm_bit_clear_all(ps->charset);
		switch (*ptr) {
		case 'n':
			dm_bit_set(ps->charset, (int) '\n');
			break;
		case 'r':
			dm_bit_set(ps->charset, (int) '\r');
			break;
		case 't':
			dm_bit_set(ps->charset, (int) '\t');
			break;
		default:
			dm_bit_set(ps->charset, (int) *ptr);
		}
		break;

	default:
		/* add a single character to the bitset */
		ps->type = 0;
		ps->cursor = ptr + 1;
		dm_bit_clear_all(ps->charset);
		dm_bit_set(ps->charset, (int) *ptr);
		break;
	}

	return 1;
}

static struct rx_node *_node(struct dm_pool *mem, int type,
			     struct rx_node *l, struct rx_node *r)
{
	struct rx_node *n = dm_pool_zalloc(mem, sizeof(*n));

	if (n) {
		if (!(n->charset = dm_bitset_create(mem, 256))) {
			dm_pool_free(mem, n);
			return NULL;
		}

		n->type = type;
		n->left = l;
		n->right = r;
	}

	return n;
}

static struct rx_node *_term(struct parse_sp *ps)
{
	struct rx_node *n;

	switch (ps->type) {
	case 0:
		if (!(n = _node(ps->mem, CHARSET, NULL, NULL))) {
			stack;
			return NULL;
		}

		dm_bit_copy(n->charset, ps->charset);
		_rx_get_token(ps);	/* match charset */
		break;

	case '(':
		_rx_get_token(ps);	/* match '(' */
		n = _or_term(ps);
		if (ps->type != ')') {
			log_error("missing ')' in regular expression");
			return 0;
		}
		_rx_get_token(ps);	/* match ')' */
		break;

	default:
		n = 0;
	}

	return n;
}

static struct rx_node *_closure_term(struct parse_sp *ps)
{
	struct rx_node *l, *n;

	if (!(l = _term(ps)))
		return NULL;

	for (;;) {
		switch (ps->type) {
		case '*':
			n = _node(ps->mem, STAR, l, NULL);
			break;

		case '+':
			n = _node(ps->mem, PLUS, l, NULL);
			break;

		case '?':
			n = _node(ps->mem, QUEST, l, NULL);
			break;

		default:
			return l;
		}

		if (!n) {
			stack;
			return NULL;
		}

		_rx_get_token(ps);
		l = n;
	}

	return n;
}

static struct rx_node *_cat_term(struct parse_sp *ps)
{
	struct rx_node *l, *r, *n;

	if (!(l = _closure_term(ps)))
		return NULL;

	if (ps->type == '|')
		return l;

	if (!(r = _cat_term(ps)))
		return l;

	if (!(n = _node(ps->mem, CAT, l, r)))
		stack;

	return n;
}

static struct rx_node *_or_term(struct parse_sp *ps)
{
	struct rx_node *l, *r, *n;

	if (!(l = _cat_term(ps)))
		return NULL;

	if (ps->type != '|')
		return l;

	_rx_get_token(ps);		/* match '|' */

	if (!(r = _or_term(ps))) {
		log_error("Badly formed 'or' expression");
		return NULL;
	}

	if (!(n = _node(ps->mem, OR, l, r)))
		stack;

	return n;
}

struct rx_node *rx_parse_tok(struct dm_pool *mem,
			     const char *begin, const char *end)
{
	struct rx_node *r;
	struct parse_sp *ps = dm_pool_zalloc(mem, sizeof(*ps));

	if (!ps) {
		stack;
		return NULL;
	}

	ps->mem = mem;
	ps->charset = dm_bitset_create(mem, 256);
	ps->cursor = begin;
	ps->rx_end = end;
	_rx_get_token(ps);		/* load the first token */

	if (!(r = _or_term(ps))) {
		log_error("Parse error in regex");
		dm_pool_free(mem, ps);
	}

	return r;
}

struct rx_node *rx_parse_str(struct dm_pool *mem, const char *str)
{
	return rx_parse_tok(mem, str, str + strlen(str));
}
