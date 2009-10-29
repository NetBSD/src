/*	$NetBSD: symtab.c,v 1.1.1.1 2009/10/29 00:46:53 christos Exp $	*/

/* Id: symtab.c,v 1.7 2008/11/24 21:30:35 tom Exp */

#include "defs.h"

/* TABLE_SIZE is the number of entries in the symbol table. */
/* TABLE_SIZE must be a power of two.			    */

#define	TABLE_SIZE 1024

static bucket **symbol_table = 0;
bucket *first_symbol;
bucket *last_symbol;

int
hash(const char *name)
{
    const char *s;
    int c, k;

    assert(name && *name);
    s = name;
    k = *s;
    while ((c = *++s) != 0)
	k = (31 * k + c) & (TABLE_SIZE - 1);

    return (k);
}

bucket *
make_bucket(const char *name)
{
    bucket *bp;

    assert(name);
    bp = (bucket *)MALLOC(sizeof(bucket));
    if (bp == 0)
	no_space();
    bp->link = 0;
    bp->next = 0;
    bp->name = MALLOC(strlen(name) + 1);
    if (bp->name == 0)
	no_space();
    bp->tag = 0;
    bp->value = UNDEFINED;
    bp->index = 0;
    bp->prec = 0;
    bp->class = UNKNOWN;
    bp->assoc = TOKEN;

    if (bp->name == 0)
	no_space();
    strcpy(bp->name, name);

    return (bp);
}

bucket *
lookup(const char *name)
{
    bucket *bp, **bpp;

    bpp = symbol_table + hash(name);
    bp = *bpp;

    while (bp)
    {
	if (strcmp(name, bp->name) == 0)
	    return (bp);
	bpp = &bp->link;
	bp = *bpp;
    }

    *bpp = bp = make_bucket(name);
    last_symbol->next = bp;
    last_symbol = bp;

    return (bp);
}

void
create_symbol_table(void)
{
    int i;
    bucket *bp;

    symbol_table = (bucket **)MALLOC(TABLE_SIZE * sizeof(bucket *));
    if (symbol_table == 0)
	no_space();
    for (i = 0; i < TABLE_SIZE; i++)
	symbol_table[i] = 0;

    bp = make_bucket("error");
    bp->index = 1;
    bp->class = TERM;

    first_symbol = bp;
    last_symbol = bp;
    symbol_table[hash("error")] = bp;
}

void
free_symbol_table(void)
{
    FREE(symbol_table);
    symbol_table = 0;
}

void
free_symbols(void)
{
    bucket *p, *q;

    for (p = first_symbol; p; p = q)
    {
	q = p->next;
	FREE(p);
    }
}
