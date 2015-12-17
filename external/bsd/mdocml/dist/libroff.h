/*	$Id: libroff.h,v 1.1.1.5 2015/12/17 21:58:48 christos Exp $ */
/*
 * Copyright (c) 2009, 2010, 2011 Kristaps Dzonsons <kristaps@bsd.lv>
 * Copyright (c) 2014, 2015 Ingo Schwarze <schwarze@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

enum	tbl_part {
	TBL_PART_OPTS, /* in options (first line) */
	TBL_PART_LAYOUT, /* describing layout */
	TBL_PART_DATA, /* creating data rows */
	TBL_PART_CDATA /* continue previous row */
};

struct	tbl_node {
	struct mparse	 *parse; /* parse point */
	int		  pos; /* invocation column */
	int		  line; /* invocation line */
	enum tbl_part	  part;
	struct tbl_opts	  opts;
	struct tbl_row	 *first_row;
	struct tbl_row	 *last_row;
	struct tbl_span	 *first_span;
	struct tbl_span	 *current_span;
	struct tbl_span	 *last_span;
	struct tbl_node	 *next;
};

struct	eqn_node {
	struct eqn	  eqn;    /* syntax tree of this equation */
	struct mparse	 *parse;  /* main parser, for error reporting */
	struct eqn_node  *next;   /* singly linked list of equations */
	struct eqn_def	 *defs;   /* array of definitions */
	char		 *data;   /* source code of this equation */
	size_t		  defsz;  /* number of definitions */
	size_t		  sz;     /* length of the source code */
	size_t		  cur;    /* parse point in the source code */
	size_t		  rew;    /* beginning of the current token */
	int		  gsize;  /* default point size */
	int		  delim;  /* in-line delimiters enabled */
	char		  odelim; /* in-line opening delimiter */
	char		  cdelim; /* in-line closing delimiter */
};

struct	eqn_def {
	char		 *key;
	size_t		  keysz;
	char		 *val;
	size_t		  valsz;
};

__BEGIN_DECLS

struct tbl_node	*tbl_alloc(int, int, struct mparse *);
void		 tbl_restart(int, int, struct tbl_node *);
void		 tbl_free(struct tbl_node *);
void		 tbl_reset(struct tbl_node *);
enum rofferr	 tbl_read(struct tbl_node *, int, const char *, int);
void		 tbl_option(struct tbl_node *, int, const char *, int *);
void		 tbl_layout(struct tbl_node *, int, const char *, int);
void		 tbl_data(struct tbl_node *, int, const char *, int);
int		 tbl_cdata(struct tbl_node *, int, const char *, int);
const struct tbl_span	*tbl_span(struct tbl_node *);
int		 tbl_end(struct tbl_node **);
struct eqn_node	*eqn_alloc(int, int, struct mparse *);
enum rofferr	 eqn_end(struct eqn_node **);
void		 eqn_free(struct eqn_node *);
enum rofferr	 eqn_read(struct eqn_node **, int,
			const char *, int, int *);

__END_DECLS
