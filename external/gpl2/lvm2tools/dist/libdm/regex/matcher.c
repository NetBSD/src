/*	$NetBSD: matcher.c,v 1.1.1.1 2008/12/12 11:42:54 haad Exp $	*/

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
#include "ttree.h"
#include "assert.h"

struct dfa_state {
	int final;
	struct dfa_state *lookup[256];
};

struct state_queue {
	struct dfa_state *s;
	dm_bitset_t bits;
	struct state_queue *next;
};

struct dm_regex {		/* Instance variables for the lexer */
	struct dfa_state *start;
	unsigned num_nodes;
	int nodes_entered;
	struct rx_node **nodes;
	struct dm_pool *scratch, *mem;
};

#define TARGET_TRANS '\0'

static int _count_nodes(struct rx_node *rx)
{
	int r = 1;

	if (rx->left)
		r += _count_nodes(rx->left);

	if (rx->right)
		r += _count_nodes(rx->right);

	return r;
}

static void _fill_table(struct dm_regex *m, struct rx_node *rx)
{
	assert((rx->type != OR) || (rx->left && rx->right));

	if (rx->left)
		_fill_table(m, rx->left);

	if (rx->right)
		_fill_table(m, rx->right);

	m->nodes[m->nodes_entered++] = rx;
}

static void _create_bitsets(struct dm_regex *m)
{
	int i;

	for (i = 0; i < m->num_nodes; i++) {
		struct rx_node *n = m->nodes[i];
		n->firstpos = dm_bitset_create(m->scratch, m->num_nodes);
		n->lastpos = dm_bitset_create(m->scratch, m->num_nodes);
		n->followpos = dm_bitset_create(m->scratch, m->num_nodes);
	}
}

static void _calc_functions(struct dm_regex *m)
{
	int i, j, final = 1;
	struct rx_node *rx, *c1, *c2;

	for (i = 0; i < m->num_nodes; i++) {
		rx = m->nodes[i];
		c1 = rx->left;
		c2 = rx->right;

		if (dm_bit(rx->charset, TARGET_TRANS))
			rx->final = final++;

		switch (rx->type) {
		case CAT:
			if (c1->nullable)
				dm_bit_union(rx->firstpos,
					  c1->firstpos, c2->firstpos);
			else
				dm_bit_copy(rx->firstpos, c1->firstpos);

			if (c2->nullable)
				dm_bit_union(rx->lastpos,
					  c1->lastpos, c2->lastpos);
			else
				dm_bit_copy(rx->lastpos, c2->lastpos);

			rx->nullable = c1->nullable && c2->nullable;
			break;

		case PLUS:
			dm_bit_copy(rx->firstpos, c1->firstpos);
			dm_bit_copy(rx->lastpos, c1->lastpos);
			rx->nullable = c1->nullable;
			break;

		case OR:
			dm_bit_union(rx->firstpos, c1->firstpos, c2->firstpos);
			dm_bit_union(rx->lastpos, c1->lastpos, c2->lastpos);
			rx->nullable = c1->nullable || c2->nullable;
			break;

		case QUEST:
		case STAR:
			dm_bit_copy(rx->firstpos, c1->firstpos);
			dm_bit_copy(rx->lastpos, c1->lastpos);
			rx->nullable = 1;
			break;

		case CHARSET:
			dm_bit_set(rx->firstpos, i);
			dm_bit_set(rx->lastpos, i);
			rx->nullable = 0;
			break;

		default:
			log_error("Internal error: Unknown calc node type");
		}

		/*
		 * followpos has it's own switch
		 * because PLUS and STAR do the
		 * same thing.
		 */
		switch (rx->type) {
		case CAT:
			for (j = 0; j < m->num_nodes; j++) {
				if (dm_bit(c1->lastpos, j)) {
					struct rx_node *n = m->nodes[j];
					dm_bit_union(n->followpos,
						  n->followpos, c2->firstpos);
				}
			}
			break;

		case PLUS:
		case STAR:
			for (j = 0; j < m->num_nodes; j++) {
				if (dm_bit(rx->lastpos, j)) {
					struct rx_node *n = m->nodes[j];
					dm_bit_union(n->followpos,
						  n->followpos, rx->firstpos);
				}
			}
			break;
		}
	}
}

static struct dfa_state *_create_dfa_state(struct dm_pool *mem)
{
	return dm_pool_zalloc(mem, sizeof(struct dfa_state));
}

static struct state_queue *_create_state_queue(struct dm_pool *mem,
					       struct dfa_state *dfa,
					       dm_bitset_t bits)
{
	struct state_queue *r = dm_pool_alloc(mem, sizeof(*r));

	if (!r) {
		stack;
		return NULL;
	}

	r->s = dfa;
	r->bits = dm_bitset_create(mem, bits[0]);	/* first element is the size */
	dm_bit_copy(r->bits, bits);
	r->next = 0;
	return r;
}

static int _calc_states(struct dm_regex *m, struct rx_node *rx)
{
	unsigned iwidth = (m->num_nodes / DM_BITS_PER_INT) + 1;
	struct ttree *tt = ttree_create(m->scratch, iwidth);
	struct state_queue *h, *t, *tmp;
	struct dfa_state *dfa, *ldfa;
	int i, a, set_bits = 0, count = 0;
	dm_bitset_t bs, dfa_bits;

	if (!tt)
		return_0;

	if (!(bs = dm_bitset_create(m->scratch, m->num_nodes)))
		return_0;

	/* create first state */
	dfa = _create_dfa_state(m->mem);
	m->start = dfa;
	ttree_insert(tt, rx->firstpos + 1, dfa);

	/* prime the queue */
	h = t = _create_state_queue(m->scratch, dfa, rx->firstpos);
	while (h) {
		/* pop state off front of the queue */
		dfa = h->s;
		dfa_bits = h->bits;
		h = h->next;

		/* iterate through all the inputs for this state */
		dm_bit_clear_all(bs);
		for (a = 0; a < 256; a++) {
			/* iterate through all the states in firstpos */
			for (i = dm_bit_get_first(dfa_bits);
			     i >= 0; i = dm_bit_get_next(dfa_bits, i)) {
				if (dm_bit(m->nodes[i]->charset, a)) {
					if (a == TARGET_TRANS)
						dfa->final = m->nodes[i]->final;

					dm_bit_union(bs, bs,
						  m->nodes[i]->followpos);
					set_bits = 1;
				}
			}

			if (set_bits) {
				ldfa = ttree_lookup(tt, bs + 1);
				if (!ldfa) {
					/* push */
					ldfa = _create_dfa_state(m->mem);
					ttree_insert(tt, bs + 1, ldfa);
					tmp =
					    _create_state_queue(m->scratch,
								ldfa, bs);
					if (!h)
						h = t = tmp;
					else {
						t->next = tmp;
						t = tmp;
					}

					count++;
				}

				dfa->lookup[a] = ldfa;
				set_bits = 0;
				dm_bit_clear_all(bs);
			}
		}
	}

	log_debug("Matcher built with %d dfa states", count);
	return 1;
}

struct dm_regex *dm_regex_create(struct dm_pool *mem, const char **patterns,
				 unsigned num_patterns)
{
	char *all, *ptr;
	int i;
	size_t len = 0;
	struct rx_node *rx;
	struct dm_pool *scratch = dm_pool_create("regex matcher", 10 * 1024);
	struct dm_regex *m;

	if (!scratch)
		return_NULL;

	if (!(m = dm_pool_alloc(mem, sizeof(*m)))) {
		dm_pool_destroy(scratch);
		return_NULL;
	}

	memset(m, 0, sizeof(*m));

	/* join the regexps together, delimiting with zero */
	for (i = 0; i < num_patterns; i++)
		len += strlen(patterns[i]) + 8;

	ptr = all = dm_pool_alloc(scratch, len + 1);

	if (!all)
		goto_bad;

	for (i = 0; i < num_patterns; i++) {
		ptr += sprintf(ptr, "(.*(%s)%c)", patterns[i], TARGET_TRANS);
		if (i < (num_patterns - 1))
			*ptr++ = '|';
	}

	/* parse this expression */
	if (!(rx = rx_parse_tok(scratch, all, ptr))) {
		log_error("Couldn't parse regex");
		goto bad;
	}

	m->mem = mem;
	m->scratch = scratch;
	m->num_nodes = _count_nodes(rx);
	m->nodes = dm_pool_alloc(scratch, sizeof(*m->nodes) * m->num_nodes);

	if (!m->nodes)
		goto_bad;

	_fill_table(m, rx);
	_create_bitsets(m);
	_calc_functions(m);
	_calc_states(m, rx);
	dm_pool_destroy(scratch);
	m->scratch = NULL;

	return m;

      bad:
	dm_pool_destroy(scratch);
	dm_pool_free(mem, m);
	return NULL;
}

static struct dfa_state *_step_matcher(int c, struct dfa_state *cs, int *r)
{
	if (!(cs = cs->lookup[(unsigned char) c]))
		return NULL;

	if (cs->final && (cs->final > *r))
		*r = cs->final;

	return cs;
}

int dm_regex_match(struct dm_regex *regex, const char *s)
{
	struct dfa_state *cs = regex->start;
	int r = 0;

	if (!(cs = _step_matcher(HAT_CHAR, cs, &r)))
		goto out;

	for (; *s; s++)
		if (!(cs = _step_matcher(*s, cs, &r)))
			goto out;

	_step_matcher(DOLLAR_CHAR, cs, &r);

      out:
	/* subtract 1 to get back to zero index */
	return r - 1;
}
