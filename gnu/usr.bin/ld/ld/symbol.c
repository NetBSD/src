/*	$NetBSD: symbol.c,v 1.15 1998/12/17 14:34:52 pk Exp $	*/

/*
 *	- symbol table routines
 */

/* Create the symbol table entries for `etext', `edata' and `end'.  */

#include <sys/param.h>
#include <sys/types.h>
#include <fcntl.h>
#include <a.out.h>
#include <stab.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shlib.h"
#include "ld.h"
#include "ld_i.h"

symbol	*symtab[SYMTABSIZE];	/* The symbol table. */
int	num_hash_tab_syms;	/* Number of symbols in symbol hash table. */

symbol	*edata_symbol;		/* the symbol _edata */
symbol	*etext_symbol;		/* the symbol _etext */
symbol	*end_symbol;		/* the symbol _end */
symbol	*got_symbol;		/* the symbol __GLOBAL_OFFSET_TABLE_ */
symbol	*plt_symbol;		/* the symbol __PROCEDURE_LINKAGE_TABLE_ */
symbol	*dynamic_symbol;	/* the symbol __DYNAMIC */

static int hash_string __P((char *));

void
symtab_init(relocatable_output)
	int	relocatable_output;
{
	/*
	 * Put linker reserved symbols into symbol table.
	 */

	dynamic_symbol = getsym(DYN_SYM);
	dynamic_symbol->defined = relocatable_output?N_UNDF:(N_DATA | N_EXT);

	got_symbol = getsym(GOT_SYM);
	got_symbol->defined = N_DATA | N_EXT;

	plt_symbol = getsym(PLT_SYM);
	plt_symbol->defined = N_DATA | N_EXT;

	if (relocatable_output)
		return;

	etext_symbol = getsym(ETEXT_SYM);
	edata_symbol = getsym(EDATA_SYM);
	end_symbol = getsym(END_SYM);

	etext_symbol->defined = N_TEXT | N_EXT;
	edata_symbol->defined = N_DATA | N_EXT;
	end_symbol->defined = N_BSS | N_EXT;

	etext_symbol->flags |= GS_REFERENCED;
	edata_symbol->flags |= GS_REFERENCED;
	end_symbol->flags |= GS_REFERENCED;
}

/*
 * Compute the hash code for symbol name KEY.
 */
int
hash_string (key)
     char *key;
{
	register char *cp;
	register int k;

	cp = key;
	k = 0;
	while (*cp)
		k = (((k << 1) + (k >> 14)) ^ (*cp++)) & 0x3fff;

	return k;
}

/*
 * Get the symbol table entry for the global symbol named KEY.
 * Create one if there is none.
 */

symbol *
getsym(key)
	char *key;
{
	register int hashval;
	register symbol *bp;

	/* Determine the proper bucket.  */
	hashval = hash_string(key) % SYMTABSIZE;

	/* Search the bucket.  */
	for (bp = symtab[hashval]; bp; bp = bp->link)
		if (strcmp(key, bp->name) == 0)
			return bp;

	/* Nothing was found; create a new symbol table entry.  */
	bp = (symbol *)xmalloc(sizeof(symbol));
	bp->name = (char *)xmalloc(strlen(key) + 1);
	strcpy (bp->name, key);
	bp->refs = 0;
	bp->defined = 0;
	bp->value = 0;
	bp->common_size = 0;
	bp->warning = 0;
	bp->undef_refs = 0;
	bp->mult_defs = 0;
	bp->alias = 0;
	bp->setv_count = 0;
	bp->symbolnum = 0;
	bp->rrs_symbolnum = 0;

	bp->size = 0;
	bp->aux = 0;
	bp->sorefs = 0;
	bp->so_defined = 0;
	bp->def_lsp = 0;
	bp->jmpslot_offset = -1;
	bp->gotslot_offset = -1;
	bp->flags = 0;

	/* Add the entry to the bucket.  */
	bp->link = symtab[hashval];
	symtab[hashval] = bp;

	++num_hash_tab_syms;

	return bp;
}

/* Like `getsym' but return 0 if the symbol is not already known.  */

symbol *
getsym_soft (key)
	char *key;
{
	register int hashval;
	register symbol *bp;

	/* Determine which bucket. */
	hashval = hash_string(key) % SYMTABSIZE;

	/* Search the bucket. */
	for (bp = symtab[hashval]; bp; bp = bp->link)
		if (strcmp(key, bp->name) == 0)
			return bp;

	return 0;
}
