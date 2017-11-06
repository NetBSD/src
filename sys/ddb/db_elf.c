/*	$NetBSD: db_elf.c,v 1.29 2017/11/06 04:08:02 christos Exp $	*/

/*-
 * Copyright (c) 1997, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_elf.c,v 1.29 2017/11/06 04:08:02 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <machine/db_machdep.h>
#include <machine/pmap.h>
#include <machine/vmparam.h>

#ifdef DB_ELF_SYMBOLS

#include <ddb/ddb.h>
#include <sys/exec_elf.h>

static char	*db_elf_find_strtab(db_symtab_t *);

#define	STAB_TO_SYMSTART(stab)	((Elf_Sym *)((stab)->start))
#define	STAB_TO_SYMEND(stab)	((Elf_Sym *)((stab)->end))
#define	STAB_TO_EHDR(stab)	((Elf_Ehdr *)((stab)->private))
#define	STAB_TO_SHDR(stab, e)	((Elf_Shdr *)((stab)->private + (e)->e_shoff))

static bool db_elf_sym_init(int, void *, void *, const char *);
static db_sym_t	db_elf_lookup(db_symtab_t *, const char *);
static db_sym_t	db_elf_search_symbol(db_symtab_t *, db_addr_t, db_strategy_t,
		    db_expr_t *);
static void	db_elf_symbol_values(db_symtab_t *, db_sym_t, const char **,
		    db_expr_t *);
static bool db_elf_line_at_pc(db_symtab_t *, db_sym_t, char **, int *,
		    db_expr_t);
static bool db_elf_sym_numargs(db_symtab_t *, db_sym_t, int *, char **);
static void	db_elf_forall(db_symtab_t *, db_forall_func_t db_forall_func,
		    void *);

const db_symformat_t db_symformat_elf = {
	"ELF",
	db_elf_sym_init,
	db_elf_lookup,
	db_elf_search_symbol,
	db_elf_symbol_values,
	db_elf_line_at_pc,
	db_elf_sym_numargs,
	db_elf_forall
};

static db_symtab_t db_symtabs;

/*
 * Add symbol table, with given name, to symbol tables.
 */
static int
db_add_symbol_table(char *start, char *end, const char *name, char *ref)
{

	db_symtabs.start = start;
	db_symtabs.end = end;
	db_symtabs.name = name;
	db_symtabs.private = ref;

	return(0);
}

/*
 * Find the symbol table and strings; tell ddb about them.
 */
static bool
db_elf_sym_init(
	int symsize,		/* size of symbol table */
	void *symtab,		/* pointer to start of symbol table */
	void *esymtab,		/* pointer to end of string table,
				   for checking - rounded up to integer
				   boundary */
	const char *name
)
{
	Elf_Ehdr *elf;
	Elf_Shdr *shp;
	Elf_Sym *symp, *symtab_start, *symtab_end;
	char *strtab_start, *strtab_end;
	int i, j;

	if (ALIGNED_POINTER(symtab, long) == 0) {
		printf("[ %s symbol table has bad start address %p ]\n",
		    name, symtab);
		return (false);
	}

	symtab_start = symtab_end = NULL;
	strtab_start = strtab_end = NULL;

	/*
	 * The format of the symbols loaded by the boot program is:
	 *
	 *	Elf exec header
	 *	first section header
	 *	. . .
	 *	. . .
	 *	last section header
	 *	first symbol or string table section
	 *	. . .
	 *	. . .
	 *	last symbol or string table section
	 */

	/*
	 * Validate the Elf header.
	 */
	elf = (Elf_Ehdr *)symtab;
	if (memcmp(elf->e_ident, ELFMAG, SELFMAG) != 0 ||
	    elf->e_ident[EI_CLASS] != ELFCLASS)
		goto badheader;

	switch (elf->e_machine) {

	ELFDEFNNAME(MACHDEP_ID_CASES)

	default:
		goto badheader;
	}

	/*
	 * Find the first (and, we hope, only) SHT_SYMTAB section in
	 * the file, and the SHT_STRTAB section that goes with it.
	 */
	if (elf->e_shoff == 0)
		goto badheader;
	shp = (Elf_Shdr *)((char *)symtab + elf->e_shoff);
	for (i = 0; i < elf->e_shnum; i++) {
		if (shp[i].sh_type == SHT_SYMTAB) {
			if (shp[i].sh_offset == 0)
				continue;
			/* Got the symbol table. */
			symtab_start = (Elf_Sym *)((char *)symtab +
			    shp[i].sh_offset);
			symtab_end = (Elf_Sym *)((char *)symtab +
			    shp[i].sh_offset + shp[i].sh_size);
			/* Find the string table to go with it. */
			j = shp[i].sh_link;
			if (shp[j].sh_offset == 0)
				continue;
			strtab_start = (char *)symtab + shp[j].sh_offset;
			strtab_end = (char *)symtab + shp[j].sh_offset +
			    shp[j].sh_size;
			/* There should only be one symbol table. */
			break;
		}
	}

	/*
	 * Now, sanity check the symbols against the string table.
	 */
	if (symtab_start == NULL || strtab_start == NULL ||
	    ALIGNED_POINTER(symtab_start, long) == 0 ||
	    ALIGNED_POINTER(strtab_start, long) == 0)
		goto badheader;
	for (symp = symtab_start; symp < symtab_end; symp++)
		if (symp->st_name + strtab_start > strtab_end)
			goto badheader;

	/*
	 * Link the symbol table into the debugger.
	 */
	if (db_add_symbol_table((char *)symtab_start,
	    (char *)symtab_end, name, (char *)symtab) != -1) {
		return (true);
	}

	return (false);

 badheader:
	printf("[ %s ELF symbol table not valid ]\n", name);
	return (false);
}

/*
 * Internal helper function - return a pointer to the string table
 * for the current symbol table.
 */
static char *
db_elf_find_strtab(db_symtab_t *stab)
{
	Elf_Ehdr *elf = STAB_TO_EHDR(stab);
	Elf_Shdr *shp = STAB_TO_SHDR(stab, elf);
	int i;

	stab = &db_symtabs;

	/*
	 * We don't load ELF header for ELF modules.
	 * Find out if this is a loadable module. If so,
	 * string table comes right after symbol table.
	 */
	if ((Elf_Sym *)elf == STAB_TO_SYMSTART(stab)) {
		return ((char *)STAB_TO_SYMEND(stab));
	}
	for (i = 0; i < elf->e_shnum; i++) {
		if (shp[i].sh_type == SHT_SYMTAB)
			return ((char*)elf + shp[shp[i].sh_link].sh_offset);
	}

	return (NULL);
}

/*
 * Lookup the symbol with the given name.
 */
static db_sym_t
db_elf_lookup(db_symtab_t *stab, const char *symstr)
{
	Elf_Sym *symp, *symtab_start, *symtab_end;
	char *strtab;

	stab = &db_symtabs;

	symtab_start = STAB_TO_SYMSTART(stab);
	symtab_end = STAB_TO_SYMEND(stab);

	strtab = db_elf_find_strtab(stab);
	if (strtab == NULL)
		return ((db_sym_t)0);

	for (symp = symtab_start; symp < symtab_end; symp++) {
		if (symp->st_name != 0 &&
		    db_eqname(strtab + symp->st_name, symstr, 0))
			return ((db_sym_t)symp);
	}

	return ((db_sym_t)0);
}

/*
 * Search for the symbol with the given address (matching within the
 * provided threshold).
 */
static db_sym_t
db_elf_search_symbol(db_symtab_t *symtab, db_addr_t off, db_strategy_t strategy,
    db_expr_t *diffp)
{
	Elf_Sym *rsymp, *symp, *symtab_start, *symtab_end;
	db_addr_t diff = *diffp;

	symtab = &db_symtabs;

	symtab_start = STAB_TO_SYMSTART(symtab);
	symtab_end = STAB_TO_SYMEND(symtab);

	rsymp = NULL;

	for (symp = symtab_start; symp < symtab_end; symp++) {
		if (symp->st_name == 0)
			continue;

#if 0
		/* This prevents me from seeing anythin in locore.s -- eeh */
		if (ELF_ST_TYPE(symp->st_info) != STT_OBJECT &&
		    ELF_ST_TYPE(symp->st_info) != STT_FUNC)
			continue;
#endif

		if (off >= symp->st_value) {
			if (off - symp->st_value < diff) {
				diff = off - symp->st_value;
				rsymp = symp;
				if (diff == 0) {
					if (strategy == DB_STGY_PROC &&
					    ELFDEFNNAME(ST_TYPE)(symp->st_info)
					      == STT_FUNC &&
					    ELFDEFNNAME(ST_BIND)(symp->st_info)
					      != STB_LOCAL)
						break;
					if (strategy == DB_STGY_ANY &&
					    ELFDEFNNAME(ST_BIND)(symp->st_info)
					      != STB_LOCAL)
						break;
				}
			} else if (off - symp->st_value == diff) {
				if (rsymp == NULL)
					rsymp = symp;
				else if (ELFDEFNNAME(ST_BIND)(rsymp->st_info)
				      == STB_LOCAL &&
				    ELFDEFNNAME(ST_BIND)(symp->st_info)
				      != STB_LOCAL) {
					/* pick the external symbol */
					rsymp = symp;
				}
			}
		}
	}

	if (rsymp == NULL)
		*diffp = off;
	else
		*diffp = diff;

	return ((db_sym_t)rsymp);
}

/*
 * Return the name and value for a symbol.
 */
static void
db_elf_symbol_values(db_symtab_t *symtab, db_sym_t sym, const char **namep,
    db_expr_t *valuep)
{
	Elf_Sym *symp = (Elf_Sym *)sym;
	char *strtab;

	symtab = &db_symtabs;

	if (namep) {
		strtab = db_elf_find_strtab(symtab);
		if (strtab == NULL)
			*namep = NULL;
		else
			*namep = strtab + symp->st_name;
	}

	if (valuep)
		*valuep = symp->st_value;
}

/*
 * Return the file and line number of the current program counter
 * if we can find the appropriate debugging symbol.
 */
static bool
db_elf_line_at_pc(db_symtab_t *symtab, db_sym_t cursym, char **filename, int *linenum, db_expr_t off)
{

	/*
	 * XXX We don't support this (yet).
	 */
	return (false);
}

/*
 * Returns the number of arguments to a function and their
 * names if we can find the appropriate debugging symbol.
 */
static bool
db_elf_sym_numargs(db_symtab_t *symtab, db_sym_t cursym, int *nargp,
    char **argnamep)
{

	/*
	 * XXX We don't support this (yet).
	 */
	return (false);
}

static void
db_elf_forall(db_symtab_t *stab, db_forall_func_t db_forall_func, void *arg)
{
	char *strtab;
	static char suffix[2];
	Elf_Sym *symp, *symtab_start, *symtab_end;

	stab = &db_symtabs;

	symtab_start = STAB_TO_SYMSTART(stab);
	symtab_end = STAB_TO_SYMEND(stab);

	strtab = db_elf_find_strtab(stab);
	if (strtab == NULL)
		return;

	for (symp = symtab_start; symp < symtab_end; symp++)
		if (symp->st_name != 0) {
			suffix[1] = '\0';
			switch (ELFDEFNNAME(ST_TYPE)(symp->st_info)) {
			case STT_OBJECT:
				suffix[0] = '+';
				break;
			case STT_FUNC:
				suffix[0] = '*';
				break;
			case STT_SECTION:
				suffix[0] = '&';
				break;
			case STT_FILE:
				suffix[0] = '/';
				break;
			default:
				suffix[0] = '\0';
			}
			(*db_forall_func)(stab, (db_sym_t)symp,
			    strtab + symp->st_name, suffix, 0, arg);
		}
	return;
}
#endif /* DB_ELF_SYMBOLS */
