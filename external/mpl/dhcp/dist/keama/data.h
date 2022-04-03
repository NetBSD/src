/*	$NetBSD: data.h,v 1.1.1.2 2022/04/03 01:08:42 christos Exp $	*/

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
 *   http://www.isc.org/
 */

#ifndef DATA_H
#define DATA_H

#include <stdint.h>
#include <stdio.h>

/* From FreeBSD sys/queue.h */

/*
 * Tail queue declarations.
 */
#define	TAILQ_HEAD(name, type)						\
struct name {								\
	struct type *tqh_first;	/* first element */			\
	struct type **tqh_last;	/* addr of last next element */		\
}

#define	TAILQ_ENTRY(type)						\
struct {								\
	struct type *tqe_next;	/* next element */			\
	struct type **tqe_prev;	/* address of previous next element */	\
}

/*
 * Tail queue functions.
 */
#define	TAILQ_CONCAT(head1, head2) do {					\
	if (!TAILQ_EMPTY(head2)) {					\
		*(head1)->tqh_last = (head2)->tqh_first;		\
		(head2)->tqh_first->next.tqe_prev = (head1)->tqh_last;	\
		(head1)->tqh_last = (head2)->tqh_last;			\
		TAILQ_INIT((head2));					\
	}								\
} while (0)

#define	TAILQ_EMPTY(head)	((head)->tqh_first == NULL)

#define	TAILQ_FIRST(head)	((head)->tqh_first)

#define	TAILQ_FOREACH(var, head)					\
	for ((var) = TAILQ_FIRST((head));				\
	    (var);							\
	    (var) = TAILQ_NEXT((var)))

#define	TAILQ_FOREACH_SAFE(var, head, tvar)				\
	for ((var) = TAILQ_FIRST((head));				\
	    (var) && ((tvar) = TAILQ_NEXT((var)), 1);			\
	    (var) = (tvar))

#define	TAILQ_INIT(head) do {						\
	TAILQ_FIRST((head)) = NULL;					\
	(head)->tqh_last = &TAILQ_FIRST((head));			\
} while (0)

#define	TAILQ_INSERT_AFTER(head, listelm, elm) do {			\
	if ((TAILQ_NEXT((elm)) = TAILQ_NEXT((listelm))) != NULL)	\
		TAILQ_NEXT((elm))->next.tqe_prev = 			\
		    &TAILQ_NEXT((elm));					\
	else {								\
		(head)->tqh_last = &TAILQ_NEXT((elm));			\
	}								\
	TAILQ_NEXT((listelm)) = (elm);					\
	(elm)->next.tqe_prev = &TAILQ_NEXT((listelm));			\
} while (0)

#define	TAILQ_INSERT_BEFORE(listelm, elm) do {				\
	(elm)->next.tqe_prev = (listelm)->next.tqe_prev;		\
	TAILQ_NEXT((elm)) = (listelm);					\
	*(listelm)->next.tqe_prev = (elm);				\
	(listelm)->next.tqe_prev = &TAILQ_NEXT((elm));			\
} while (0)

#define	TAILQ_INSERT_HEAD(head, elm) do {				\
	if ((TAILQ_NEXT((elm)) = TAILQ_FIRST((head))) != NULL)		\
		TAILQ_FIRST((head))->next.tqe_prev =			\
		    &TAILQ_NEXT((elm));					\
	else								\
		(head)->tqh_last = &TAILQ_NEXT((elm));			\
	TAILQ_FIRST((head)) = (elm);					\
	(elm)->next.tqe_prev = &TAILQ_FIRST((head));			\
} while (0)

#define	TAILQ_INSERT_TAIL(head, elm) do {				\
	TAILQ_NEXT((elm)) = NULL;					\
	(elm)->next.tqe_prev = (head)->tqh_last;			\
	*(head)->tqh_last = (elm);					\
	(head)->tqh_last = &TAILQ_NEXT((elm));				\
} while (0)

#define	TAILQ_LAST(head, headname)					\
	(*(((struct headname *)((head)->tqh_last))->tqh_last))

#define	TAILQ_NEXT(elm) ((elm)->next.tqe_next)

#define	TAILQ_PREV(elm, headname)					\
	(*(((struct headname *)((elm)->next.tqe_prev))->tqh_last))

#define	TAILQ_REMOVE(head, elm) do {					\
	if ((TAILQ_NEXT((elm))) != NULL)				\
		TAILQ_NEXT((elm))->next.tqe_prev = 			\
		    (elm)->next.tqe_prev;				\
	else								\
		(head)->tqh_last = (elm)->next.tqe_prev;		\
	*(elm)->next.tqe_prev = TAILQ_NEXT((elm));			\
	(elm)->next.tqe_next = (void *)-1;				\
	(elm)->next.tqe_prev = (void *)-1;				\
} while (0)

#define TAILQ_SWAP(head1, head2, type) do {				\
	struct type *swap_first = (head1)->tqh_first;			\
	struct type **swap_last = (head1)->tqh_last;			\
	(head1)->tqh_first = (head2)->tqh_first;			\
	(head1)->tqh_last = (head2)->tqh_last;				\
	(head2)->tqh_first = swap_first;				\
	(head2)->tqh_last = swap_last;					\
	if ((swap_first = (head1)->tqh_first) != NULL)			\
		swap_first->next.tqe_prev = &(head1)->tqh_first;	\
	else								\
		(head1)->tqh_last = &(head1)->tqh_first;		\
	if ((swap_first = (head2)->tqh_first) != NULL)			\
		swap_first->next.tqe_prev = &(head2)->tqh_first;	\
	else								\
		(head2)->tqh_last = &(head2)->tqh_first;		\
} while (0)

/* From bind9 lib/isc/include/isc/boolean.h */

typedef enum { isc_boolean_false = 0, isc_boolean_true = 1 } isc_boolean_t;

#define ISC_FALSE isc_boolean_false
#define ISC_TRUE isc_boolean_true
#define ISC_TF(x) ((x) ? ISC_TRUE : ISC_FALSE)

/* From Kea src/lib/cc/data.h */

struct element;

/* Element types */
#define ELEMENT_NONE		0
#define ELEMENT_INTEGER		1
#define ELEMENT_REAL		2
#define ELEMENT_BOOLEAN		3
#define ELEMENT_NULL		4
#define ELEMENT_STRING		5
#define ELEMENT_LIST		6
#define ELEMENT_MAP		7

/* Element string */
struct string {
	size_t length;		/* string length */
	char *content;		/* string data */
};

struct string *allocString(void);
/* In makeString() l == -1 means use strlen(s) */
struct string *makeString(int l, const char *s);
/* format ZlLsSbBfXI6 + h */
struct string *makeStringExt(int l, const char *s, char fmt);
/* format 6lLIsSbBj */
struct string *makeStringArray(int l, const char *s, char fmt);
void appendString(struct string *s, const char *a);
void concatString(struct string *s, const struct string *a);
isc_boolean_t eqString(const struct string *s, const struct string *o);
/* quoting */
struct string *quote(struct string *);

/* Comments */
struct comment {
	char *line;			/* comment line */
	TAILQ_ENTRY(comment) next;	/* next line */
};
TAILQ_HEAD(comments, comment);

struct comment *createComment(const char *line);

/* Element list */
TAILQ_HEAD(list, element);

/* Element map */
TAILQ_HEAD(map, element);

/* Element value */
union value {
	int64_t int_value;		/* integer */
	double double_value;		/* real */
	isc_boolean_t bool_value;	/* boolean */
        /**/				/* null */
	struct string string_value;	/* string */
	struct list list_value;		/* list */
	struct map map_value;		/* map */
};

/* Element */
struct element {
	int type;			/* element type (ELEMENT_XXX) */
	int kind;			/* element kind (e.g. ROOT_GROUP) */
	isc_boolean_t skip;		/* skip as not converted */
	char *key;			/* element key (for map) */
	union value value;		/* value */
	struct comments comments;	/* associated comments */
	TAILQ_ENTRY(element) next;	/* next item in list or map chain */
};

/* Value getters */
int64_t intValue(const struct element *e);
double doubleValue(const struct element *e);
isc_boolean_t boolValue(const struct element *e);
struct string *stringValue(struct element *e);
struct list *listValue(struct element *e);
struct map *mapValue(struct element *e);

/* Creators */
struct element *create(void);
struct element *createInt(int64_t i);
struct element *createDouble(double d);
struct element *createBool(isc_boolean_t b);
struct element *createNull(void);
struct element *createString(const struct string *s);
struct element *createList(void);
struct element *createMap(void);

/* Reset */
void resetInt(struct element *e, int64_t i);
void resetDouble(struct element *e, double d);
void resetBool(struct element *e, isc_boolean_t b);
void resetNull(struct element *e);
void resetString(struct element *e, const struct string *s);
void resetList(struct element *e);
void resetMap(struct element *e);
void resetBy(struct element *e, struct element *o);

/* List functions */
struct element *listGet(struct element *l, int i);
void listSet(struct element *l, struct element *e, int i);
void listPush(struct element *l, struct element *e);
void listRemove(struct element *l, int i);
size_t listSize(const struct element *l);
void concat(struct element *l, struct element *o);

/* Map functions */
struct element *mapGet(struct element *m, const char *k);
void mapSet(struct element *m, struct element *e, const char *k);
void mapRemove(struct element *m, const char *k);
isc_boolean_t mapContains(const struct element *m, const char *k);
size_t mapSize(const struct element *m);
void merge(struct element *m, struct element *o);

/* Tools */
const char *type2name(int t);
int name2type(const char *n);
void print(FILE *fp, const struct element *e,
	   isc_boolean_t skip, unsigned indent);
void printList(FILE *fp, const struct list *l,
	       isc_boolean_t skip, unsigned indent);
void printMap(FILE *fp, const struct map *m,
	      isc_boolean_t skip, unsigned indent);
void printString(FILE *fp, const struct string *s);
isc_boolean_t skip_to_end(const struct element *e);

struct element *copy(struct element *e);
struct element *copyList(struct element *l);
struct element *copyMap(struct element *m);

/* Handles */
TAILQ_HEAD(handles, handle);

struct handle {
	unsigned order;			/* order */
	char *key;			/* key */
	struct element *value;		/* value */
	struct handles values;		/* children */
	TAILQ_ENTRY(handle) next;	/* siblings */
};

struct handle* mapPop(struct element *);
void derive(struct handle *, struct handle *);

/* Hexadecimal literals */
struct string *hexaValue(struct element *);
struct element *createHexa(struct string *);

#endif /* DATA_H */
