/*	$NetBSD: kern_ksyms.c,v 1.25 2005/06/23 23:15:12 thorpej Exp $	*/

/*
 * Copyright (c) 2001, 2003 Anders Magnusson (ragge@ludd.luth.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Code to deal with in-kernel symbol table management + /dev/ksyms.
 *
 * For each loaded module the symbol table info is kept track of by a
 * struct, placed in a circular list. The first entry is the kernel
 * symbol table.
 */

/*
 * TODO:
 *	Change the ugly way of adding new symbols (comes with linker)
 *	Add kernel locking stuff.
 *	(Ev) add support for poll.
 *	(Ev) fix support for mmap.
 *
 *	Export ksyms internal logic for use in post-mortem debuggers?
 *	  Need to move struct symtab to ksyms.h for that.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_ksyms.c,v 1.25 2005/06/23 23:15:12 thorpej Exp $");

#ifdef _KERNEL
#include "opt_ddb.h"
#include "opt_ddbparam.h"	/* for SYMTAB_SPACE */
#endif

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/exec.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <machine/elf_machdep.h> /* XXX */
#define ELFSIZE ARCH_ELFSIZE

#include <sys/exec_elf.h>
#include <sys/ksyms.h>

#include <lib/libkern/libkern.h>

#ifdef DDB
#include <ddb/db_output.h>
#endif

#include "ksyms.h"

static int ksymsinited = 0;

#if NKSYMS
static void ksyms_hdr_init(caddr_t hdraddr);
static void ksyms_sizes_calc(void);
static int ksyms_isopen;
static int ksyms_maxlen;
#endif

#ifdef KSYMS_DEBUG
#define	FOLLOW_CALLS		1
#define	FOLLOW_MORE_CALLS	2
#define	FOLLOW_DEVKSYMS		4
static int ksyms_debug;
#endif

#ifdef SYMTAB_SPACE
#define		SYMTAB_FILLER	"|This is the symbol table!"

char		db_symtab[SYMTAB_SPACE] = SYMTAB_FILLER;
int		db_symtabsize = SYMTAB_SPACE;
#endif

/*
 * Store the different symbol tables in a double-linked list.
 */
struct symtab {
	CIRCLEQ_ENTRY(symtab) sd_queue;
	const char *sd_name;	/* Name of this table */
	Elf_Sym *sd_symstart;	/* Address of symbol table */
	caddr_t sd_strstart;	/* Adderss of corresponding string table */
	int sd_usroffset;	/* Real address for userspace */
	int sd_symsize;		/* Size in bytes of symbol table */
	int sd_strsize;		/* Size of string table */
	int *sd_symnmoff;	/* Used when calculating the name offset */
};

static CIRCLEQ_HEAD(, symtab) symtab_queue =
    CIRCLEQ_HEAD_INITIALIZER(symtab_queue);

static struct symtab kernel_symtab;

#define	USE_PTREE
#ifdef USE_PTREE
/*
 * Patricia-tree-based lookup structure for the in-kernel global symbols.
 * Based on a design by Mikael Sundstrom, msm@sm.luth.se.
 */
struct ptree {
	int16_t bitno;
	int16_t lr[2];
} *symb;
static int16_t baseidx;
static int treex = 1;

#define	P_BIT(key, bit) ((key[bit >> 3] >> (bit & 7)) & 1)
#define	STRING(idx) kernel_symtab.sd_symstart[idx].st_name + \
			kernel_symtab.sd_strstart

/*
 * Walk down the tree until a terminal node is found.
 */
static int
symbol_traverse(const char *key)
{
	int16_t nb, rbit = baseidx;

	while (rbit > 0) {
		nb = symb[rbit].bitno;
		rbit = symb[rbit].lr[P_BIT(key, nb)];
	}
	return -rbit;
}

static int
ptree_add(char *key, int val)
{
	int idx;
	int nix, cix, bit, rbit, sb, lastrbit, svbit = 0, ix;
	char *m, *k;

	if (baseidx == 0) {
		baseidx = -val;
		return 0; /* First element */
	}

	/* Get string to match against */
	idx = symbol_traverse(key);

	/* Find first mismatching bit */
	m = STRING(idx);
	k = key;
	if (strcmp(m, k) == 0)
		return 1;

	for (cix = 0; *m && *k && *m == *k; m++, k++, cix += 8)
		;
	ix = ffs((int)*m ^ (int)*k) - 1;
	cix += ix;

	/* Create new node */
	nix = treex++;
	bit = P_BIT(key, cix);
	symb[nix].bitno = cix;
	symb[nix].lr[bit] = -val;

	/* Find where to insert node */
	rbit = baseidx;
	lastrbit = 0;
	for (;;) {
		if (rbit < 0)
			break;
		sb = symb[rbit].bitno;
		if (sb > cix)
			break;
		if (sb == cix)
			printf("symb[rbit].bitno == cix!!!\n");
		lastrbit = rbit;
		svbit = P_BIT(key, sb);
		rbit = symb[rbit].lr[svbit];
	}

	/* Do the actual insertion */
	if (lastrbit == 0) {
		/* first element */
		symb[nix].lr[!bit] = baseidx;
		baseidx = nix;
	} else {
		symb[nix].lr[!bit] = rbit;
		symb[lastrbit].lr[svbit] = nix;
	}
	return 0;
}

static int
ptree_find(const char *key)
{
	int idx;

	if (baseidx == 0)
		return 0;
	idx = symbol_traverse(key);

	if (strcmp(key, STRING(idx)) == 0)
		return idx;
	return 0;
}

static void
ptree_gen(char *off, struct symtab *tab)
{
	Elf_Sym *sym;
	int i, nsym;

	if (off != NULL)
		symb = (struct ptree *)ALIGN(off);
	else
		symb = malloc((tab->sd_symsize/sizeof(Elf_Sym)) *
		    sizeof(struct ptree), M_DEVBUF, M_WAITOK);
	symb--; /* sym index won't be 0 */

	sym = tab->sd_symstart;
	if ((nsym = tab->sd_symsize/sizeof(Elf_Sym)) > INT16_MAX) {
		printf("Too many symbols for tree, skipping %d symbols\n",
		    nsym-INT16_MAX);
		nsym = INT16_MAX;
	}
	for (i = 1; i < nsym; i++) {
		if (ELF_ST_BIND(sym[i].st_info) != STB_GLOBAL)
			continue;
		ptree_add(tab->sd_strstart+sym[i].st_name, i);
	}
}
#endif /* USE_PTREE */

/*
 * Finds a certain symbol name in a certain symbol table.
 */
static Elf_Sym *
findsym(const char *name, struct symtab *table)
{
	Elf_Sym *start = table->sd_symstart;
	int i, sz = table->sd_symsize/sizeof(Elf_Sym);
	char *np;
	caddr_t realstart = table->sd_strstart - table->sd_usroffset;

#ifdef USE_PTREE
	if (table == &kernel_symtab && (i = ptree_find(name)) != 0)
		return &start[i];
#endif

	for (i = 0; i < sz; i++) {
		np = realstart + start[i].st_name;
		if (name[0] == np[0] && name[1] == np[1] &&
		    strcmp(name, np) == 0)
			return &start[i];
	}
	return NULL;
}

/*
 * The "attach" is in reality done in ksyms_init().
 */
void ksymsattach(int);
void
ksymsattach(int arg)
{

#ifdef USE_PTREE
	if (baseidx == 0)
		ptree_gen(0, &kernel_symtab);
#endif

}

/*
 * Add a symbol table named name.
 * This is intended for use when the kernel loader enters the table.
 */
static void
addsymtab(const char *name, Elf_Ehdr *ehdr, struct symtab *tab)
{
	caddr_t start = (caddr_t)ehdr;
	caddr_t send;
	Elf_Shdr *shdr;
	Elf_Sym *sym, *nsym;
	int i, j, n, g;
	char *str;

	/* Find the symbol table and the corresponding string table. */
	shdr = (Elf_Shdr *)(start + ehdr->e_shoff);
	for (i = 1; i < ehdr->e_shnum; i++) {
		if (shdr[i].sh_type != SHT_SYMTAB)
			continue;
		if (shdr[i].sh_offset == 0)
			continue;
		tab->sd_symstart = (Elf_Sym *)(start + shdr[i].sh_offset);
		tab->sd_symsize = shdr[i].sh_size;
		j = shdr[i].sh_link;
		if (shdr[j].sh_offset == 0)
			continue; /* Can this happen? */
		tab->sd_strstart = start + shdr[j].sh_offset;
		tab->sd_strsize = shdr[j].sh_size;
		break;
	}
	tab->sd_name = name;
	send = tab->sd_strstart + tab->sd_strsize;

#ifdef KSYMS_DEBUG
	printf("start %p sym %p symsz %d str %p strsz %d send %p\n",
	    start, tab->sd_symstart, tab->sd_symsize,
	    tab->sd_strstart, tab->sd_strsize, send);
#endif

	/*
	 * Pack symbol table by removing all file name references
	 * and overwrite the elf header.
	 */
	sym = tab->sd_symstart;
	nsym = (Elf_Sym *)start;
	str = tab->sd_strstart;
	for (g = i = n = 0; i < tab->sd_symsize/sizeof(Elf_Sym); i++) {
		if (i == 0) {
			nsym[n++] = sym[i];
			continue;
		}
		/*
		 * Remove useless symbols.
		 * Should actually remove all typeless symbols.
		 */
		if (sym[i].st_name == 0)
			continue; /* Skip nameless entries */
		if (ELF_ST_TYPE(sym[i].st_info) == STT_FILE)
			continue; /* Skip filenames */
		if (ELF_ST_TYPE(sym[i].st_info) == STT_NOTYPE &&
		    sym[i].st_value == 0 &&
		    strcmp(str + sym[i].st_name, "*ABS*") == 0)
			continue; /* XXX */
		if (ELF_ST_TYPE(sym[i].st_info) == STT_NOTYPE &&
		    strcmp(str + sym[i].st_name, "gcc2_compiled.") == 0)
			continue; /* XXX */

#ifndef DDB
		/* Only need global symbols */
		if (ELF_ST_BIND(sym[i].st_info) != STB_GLOBAL)
			continue;
#endif

		/* Save symbol. Set it as an absolute offset */
		nsym[n] = sym[i];
		nsym[n].st_shndx = SHN_ABS;
		if (ELF_ST_BIND(nsym[n].st_info) == STB_GLOBAL)
			g++;
#if NKSYMS
		j = strlen(nsym[n].st_name + tab->sd_strstart) + 1;
		if (j > ksyms_maxlen)
			ksyms_maxlen = j;
#endif
		n++;

	}
	tab->sd_symstart = nsym;
	tab->sd_symsize = n * sizeof(Elf_Sym);

#ifdef notyet
	/*
	 * Remove left-over strings.
	 */
	sym = tab->sd_symstart;
	str = (caddr_t)tab->sd_symstart + tab->sd_symsize;
	str[0] = 0;
	n = 1;
	for (i = 1; i < tab->sd_symsize/sizeof(Elf_Sym); i++) {
		strcpy(str + n, tab->sd_strstart + sym[i].st_name);
		sym[i].st_name = n;
		n += strlen(str+n) + 1;
	}
	tab->sd_strstart = str;
	tab->sd_strsize = n;

#ifdef KSYMS_DEBUG
	printf("str %p strsz %d send %p\n", str, n, send);
#endif
#endif

	CIRCLEQ_INSERT_HEAD(&symtab_queue, tab, sd_queue);

#ifdef notyet
#ifdef USE_PTREE
	/* Try to use the freed space, if possible */
	if (send - str - n > g * sizeof(struct ptree))
		ptree_gen(str + n, tab);
#endif
#endif
}

/*
 * Setup the kernel symbol table stuff.
 */
void
ksyms_init(int symsize, void *start, void *end)
{
	Elf_Ehdr *ehdr;

#ifdef SYMTAB_SPACE
	if (symsize <= 0 &&
	    strncmp(db_symtab, SYMTAB_FILLER, sizeof(SYMTAB_FILLER))) {
		symsize = db_symtabsize;
		start = db_symtab;
		end = db_symtab + db_symtabsize;
	}
#endif
	if (symsize <= 0) {
		printf("[ Kernel symbol table missing! ]\n");
		return;
	}

	/* Sanity check */
	if (ALIGNED_POINTER(start, long) == 0) {
		printf("[ Kernel symbol table has bad start address %p ]\n",
		    start);
		return;
	}

	ehdr = (Elf_Ehdr *)start;

	/* check if this is a valid ELF header */
	/* No reason to verify arch type, the kernel is actually running! */
	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) ||
	    ehdr->e_ident[EI_CLASS] != ELFCLASS ||
	    ehdr->e_version > 1) {
#ifdef notyet /* DDB */
		if (ddb_init(symsize, start, end))
			return; /* old-style symbol table */
#endif
		printf("[ Kernel symbol table invalid! ]\n");
		return; /* nothing to do */
	}

#if NKSYMS
	/* Loaded header will be scratched in addsymtab */
	ksyms_hdr_init(start);
#endif

	addsymtab("netbsd", ehdr, &kernel_symtab);

#if NKSYMS
	ksyms_sizes_calc();
#endif

	ksymsinited = 1;

#ifdef DEBUG
	printf("Loaded initial symtab at %p, strtab at %p, # entries %ld\n",
	    kernel_symtab.sd_symstart, kernel_symtab.sd_strstart,
	    (long)kernel_symtab.sd_symsize/sizeof(Elf_Sym));
#endif
}

/*
 * Get the value associated with a symbol.
 * "mod" is the module name, or null if any module.
 * "sym" is the symbol name.
 * "val" is a pointer to the corresponding value, if call succeeded.
 * Returns 0 if success or ENOENT if no such entry.
 */
int
ksyms_getval(const char *mod, const char *sym, unsigned long *val, int type)
{
	struct symtab *st;
	Elf_Sym *es;

	if (ksymsinited == 0)
		return ENOENT;

#ifdef KSYMS_DEBUG
	if (ksyms_debug & FOLLOW_CALLS)
		printf("ksyms_getval: mod %s sym %s valp %p\n", mod, sym, val);
#endif

	CIRCLEQ_FOREACH(st, &symtab_queue, sd_queue) {
		if (mod && strcmp(st->sd_name, mod))
			continue;
		if ((es = findsym(sym, st)) == NULL)
			continue;

		/* Skip if bad binding */
		if (type == KSYMS_EXTERN &&
		    ELF_ST_BIND(es->st_info) != STB_GLOBAL)
			continue;

		if (val)
			*val = es->st_value;
		return 0;
	}
	return ENOENT;
}

/*
 * Get "mod" and "symbol" associated with an address.
 * Returns 0 if success or ENOENT if no such entry.
 */
int
ksyms_getname(const char **mod, const char **sym, vaddr_t v, int f)
{
	struct symtab *st;
	Elf_Sym *les, *es = NULL;
	vaddr_t laddr = 0;
	const char *lmod = NULL;
	char *stable = NULL;
	int type, i, sz;

	if (ksymsinited == 0)
		return ENOENT;

	CIRCLEQ_FOREACH(st, &symtab_queue, sd_queue) {
		sz = st->sd_symsize/sizeof(Elf_Sym);
		for (i = 0; i < sz; i++) {
			les = st->sd_symstart + i;
			type = ELF_ST_TYPE(les->st_info);

			if ((f & KSYMS_PROC) && (type != STT_FUNC))
				continue;

			if (type == STT_NOTYPE)
				continue;

			if (((f & KSYMS_ANY) == 0) &&
			    (type != STT_FUNC) && (type != STT_OBJECT))
				continue;

			if ((les->st_value <= v) && (les->st_value > laddr)) {
				laddr = les->st_value;
				es = les;
				lmod = st->sd_name;
				stable = st->sd_strstart - st->sd_usroffset;
			}
		}
	}
	if (es == NULL)
		return ENOENT;
	if ((f & KSYMS_EXACT) && (v != es->st_value))
		return ENOENT;
	if (mod)
		*mod = lmod;
	if (sym)
		*sym = stable + es->st_name;
	return 0;
}

#if NKSYMS
static int symsz, strsz;

/*
 * In case we exposing the symbol table to the userland using the pseudo-
 * device /dev/ksyms, it is easier to provide all the tables as one.
 * However, it means we have to change all the st_name fields for the
 * symbols so they match the ELF image that the userland will read
 * through the device.
 *
 * The actual (correct) value of st_name is preserved through a global
 * offset stored in the symbol table structure.
 */

static void
ksyms_sizes_calc(void)
{
        struct symtab *st;
	int i;

        symsz = strsz = 0;
        CIRCLEQ_FOREACH(st, &symtab_queue, sd_queue) {
		if (st != &kernel_symtab) {
			for (i = 0; i < st->sd_symsize/sizeof(Elf_Sym); i++)
				st->sd_symstart[i].st_name =
				    strsz + st->sd_symnmoff[i];
			st->sd_usroffset = strsz;
		}
                symsz += st->sd_symsize;
                strsz += st->sd_strsize;
        }
}
#endif /* NKSYMS */

/*
 * Temporary work structure for dynamic loaded symbol tables.
 * Will go away when in-kernel linker is in place.
 */

struct syminfo {
	size_t cursyms;
	size_t curnamep;
	size_t maxsyms;
	size_t maxnamep;
	Elf_Sym *syms;
	int *symnmoff;
	char *symnames;
};


/*
 * Add a symbol to the temporary save area for symbols.
 * This routine will go away when the in-kernel linker is in place.
 */
static void
addsym(struct syminfo *info, const Elf_Sym *sym, const char *name,
       const char *mod)
{
	int len, mlen;

#ifdef KSYMS_DEBUG
	if (ksyms_debug & FOLLOW_MORE_CALLS)
		printf("addsym: name %s val %lx\n", name, (long)sym->st_value);
#endif
	len = strlen(name) + 1;
	if (mod)
		mlen = 1 + strlen(mod);
	else
		mlen = 0;
	if (info->cursyms == info->maxsyms ||
	    (len + mlen + info->curnamep) > info->maxnamep) {
		printf("addsym: too many symbols, skipping '%s'\n", name);
		return;
	}
	strlcpy(&info->symnames[info->curnamep], name,
	    info->maxnamep - info->curnamep);
	if (mlen) {
		info->symnames[info->curnamep + len - 1] = '.';
		strlcpy(&info->symnames[info->curnamep + len], mod,
		    info->maxnamep - (info->curnamep + len));
		len += mlen;
	}
	info->syms[info->cursyms] = *sym;
	info->syms[info->cursyms].st_name = info->curnamep;
	info->symnmoff[info->cursyms] = info->curnamep;
	info->curnamep += len;
#if NKSYMS
	if (len > ksyms_maxlen)
		ksyms_maxlen = len;
#endif
	info->cursyms++;
}
/*
 * Adds a symbol table.
 * "name" is the module name, "start" and "size" is where the symbol table
 * is located, and "type" is in which binary format the symbol table is.
 * New memory for keeping the symbol table is allocated in this function.
 * Returns 0 if success and EEXIST if the module name is in use.
 */
static int
specialsym(const char *symname)
{
	return	!strcmp(symname, "_bss_start") ||
		!strcmp(symname, "__bss_start") ||
		!strcmp(symname, "_bss_end__") ||
		!strcmp(symname, "__bss_end__") ||
		!strcmp(symname, "_edata") ||
		!strcmp(symname, "_end") ||
		!strcmp(symname, "__end") ||
		!strcmp(symname, "__end__") ||
		!strncmp(symname, "__start_link_set_", 17) ||
		!strncmp(symname, "__stop_link_set_", 16);
}

int
ksyms_addsymtab(const char *mod, void *symstart, vsize_t symsize,
    char *strstart, vsize_t strsize)
{
	Elf_Sym *sym = symstart;
	struct symtab *st;
	unsigned long rval;
	int i;
	char *name;
	struct syminfo info;

#ifdef KSYMS_DEBUG
	if (ksyms_debug & FOLLOW_CALLS)
		printf("ksyms_addsymtab: mod %s symsize %lx strsize %lx\n",
		    mod, symsize, strsize);
#endif

#if NKSYMS
	/*
	 * Do not try to add a symbol table while someone is reading
	 * from /dev/ksyms.
	 */
	while (ksyms_isopen != 0)
		tsleep(&ksyms_isopen, PWAIT, "ksyms", 0);
#endif

	/* Check if this symtab already loaded */
	CIRCLEQ_FOREACH(st, &symtab_queue, sd_queue) {
		if (strcmp(mod, st->sd_name) == 0)
			return EEXIST;
	}

	/*
	 * XXX - Only add a symbol if it do not exist already.
	 * This is because of a flaw in the current LKM implementation,
	 * these loops will be removed once the in-kernel linker is in place.
	 */
	memset(&info, 0, sizeof(info));
	for (i = 0; i < symsize/sizeof(Elf_Sym); i++) {
		char * const symname = strstart + sym[i].st_name;
		if (sym[i].st_name == 0)
			continue; /* Just ignore */

		/* check validity of the symbol */
		/* XXX - save local symbols if DDB */
		if (ELF_ST_BIND(sym[i].st_info) != STB_GLOBAL)
			continue;

		/* Check if the symbol exists */
		if (ksyms_getval(NULL, symname, &rval, KSYMS_EXTERN) == 0) {
			/* Check (and complain) about differing values */
			if (sym[i].st_value != rval) {
				if (specialsym(symname)) {
					info.maxsyms++;
					info.maxnamep += strlen(symname) + 1 +
					    strlen(mod) + 1;
				} else {
					printf("%s: symbol '%s' redeclared with"
					    " different value (%lx != %lx)\n",
					    mod, symname,
					    rval, (long)sym[i].st_value);
				}
			}
		} else {
			/*
			 * Count this symbol
			 */
			info.maxsyms++;
			info.maxnamep += strlen(symname) + 1;
		}
	}

	/*
	 * Now that we know the sizes, malloc the structures.
	 */
	info.syms = malloc(sizeof(Elf_Sym)*info.maxsyms, M_DEVBUF, M_WAITOK);
	info.symnames = malloc(info.maxnamep, M_DEVBUF, M_WAITOK);
	info.symnmoff = malloc(sizeof(int)*info.maxsyms, M_DEVBUF, M_WAITOK);

	/*
	 * Now that we have the symbols, actually fill in the structures.
	 */
	for (i = 0; i < symsize/sizeof(Elf_Sym); i++) {
		char * const symname = strstart + sym[i].st_name;
		if (sym[i].st_name == 0)
			continue; /* Just ignore */

		/* check validity of the symbol */
		/* XXX - save local symbols if DDB */
		if (ELF_ST_BIND(sym[i].st_info) != STB_GLOBAL)
			continue;

		/* Check if the symbol exists */
		if (ksyms_getval(NULL, symname, &rval, KSYMS_EXTERN) == 0) {
			if ((sym[i].st_value != rval) && specialsym(symname)) {
				addsym(&info, &sym[i], symname, mod);
			}
		} else
			/* Ok, save this symbol */
			addsym(&info, &sym[i], symname, NULL);
	}

	st = malloc(sizeof(struct symtab), M_DEVBUF, M_WAITOK);
	i = strlen(mod) + 1;
	name = malloc(i, M_DEVBUF, M_WAITOK);
	strlcpy(name, mod, i);
	st->sd_name = name;
	st->sd_symnmoff = info.symnmoff;
	st->sd_symstart = info.syms;
	st->sd_symsize = sizeof(Elf_Sym)*info.maxsyms;
	st->sd_strstart = info.symnames;
	st->sd_strsize = info.maxnamep;

	/* Make them absolute references */
	sym = st->sd_symstart;
	for (i = 0; i < st->sd_symsize/sizeof(Elf_Sym); i++)
		sym[i].st_shndx = SHN_ABS;

	CIRCLEQ_INSERT_TAIL(&symtab_queue, st, sd_queue);
#if NKSYMS
	ksyms_sizes_calc();
#endif
	return 0;
}

/*
 * Remove a symbol table specified by name.
 * Returns 0 if success, EBUSY if device open and ENOENT if no such name.
 */
int
ksyms_delsymtab(const char *mod)
{
	struct symtab *st;
	int found = 0;

#if NKSYMS
	/*
	 * Do not try to delete a symbol table while someone is reading
	 * from /dev/ksyms.
	 */
	while (ksyms_isopen != 0)
		tsleep(&ksyms_isopen, PWAIT, "ksyms", 0);
#endif

	CIRCLEQ_FOREACH(st, &symtab_queue, sd_queue) {
		if (strcmp(mod, st->sd_name) == 0) {
			found = 1;
			break;
		}
	}
	if (found == 0)
		return ENOENT;
	CIRCLEQ_REMOVE(&symtab_queue, st, sd_queue);
	free(st->sd_symstart, M_DEVBUF);
	free(st->sd_strstart, M_DEVBUF);
	free(st->sd_symnmoff, M_DEVBUF);
	/* XXXUNCONST LINTED - const castaway */
	free(__UNCONST(st->sd_name), M_DEVBUF);
	free(st, M_DEVBUF);
#if NKSYMS
	ksyms_sizes_calc();
#endif
	return 0;
}

int
ksyms_rensymtab(const char *old, const char *new)
{
	struct symtab *st, *oldst = NULL;
	char *newstr;

	CIRCLEQ_FOREACH(st, &symtab_queue, sd_queue) {
		if (strcmp(old, st->sd_name) == 0)
			oldst = st;
		if (strcmp(new, st->sd_name) == 0)
			return (EEXIST);
	}
	if (oldst == NULL)
		return (ENOENT);

	newstr = malloc(strlen(new)+1, M_DEVBUF, M_WAITOK);
	if (!newstr)
		return (ENOMEM);
	strcpy(newstr, new);
	/*XXXUNCONST*/
	free(__UNCONST(oldst->sd_name), M_DEVBUF);
	oldst->sd_name = newstr;

	return (0);
}

#ifdef DDB
/*
 * Keep sifting stuff here, to avoid export of ksyms internals.
 */
int
ksyms_sift(char *mod, char *sym, int mode)
{
	struct symtab *st;
	char *sb;
	int i, sz;

	if (ksymsinited == 0)
		return ENOENT;

	CIRCLEQ_FOREACH(st, &symtab_queue, sd_queue) {
		if (mod && strcmp(mod, st->sd_name))
			continue;
		sb = st->sd_strstart;

		sz = st->sd_symsize/sizeof(Elf_Sym);
		for (i = 0; i < sz; i++) {
			Elf_Sym *les = st->sd_symstart + i;
			char c;

			if (strstr(sb + les->st_name - st->sd_usroffset, sym)
			    == NULL)
				continue;

			if (mode == 'F') {
				switch (ELF_ST_TYPE(les->st_info)) {
				case STT_OBJECT:
					c = '+';
					break;
				case STT_FUNC:
					c = '*';
					break;
				case STT_SECTION:
					c = '&';
					break;
				case STT_FILE:
					c = '/';
					break;
				default:
					c = ' ';
					break;
				}
				db_printf("%s%c ", sb + les->st_name -
				    st->sd_usroffset, c);
			} else
				db_printf("%s ", sb + les->st_name -
				    st->sd_usroffset);
		}
	}
	return ENOENT;
}
#endif /* DDB */

#if NKSYMS
/*
 * Static allocated ELF header.
 * Basic info is filled in at attach, sizes at open.
 */
#define	SYMTAB		1
#define	STRTAB		2
#define	SHSTRTAB	3
#define NSECHDR		4

#define	NPRGHDR		2
#define	SHSTRSIZ	28

static struct ksyms_hdr {
	Elf_Ehdr	kh_ehdr;
	Elf_Phdr	kh_phdr[NPRGHDR];
	Elf_Shdr	kh_shdr[NSECHDR];
	char 		kh_strtab[SHSTRSIZ];
} ksyms_hdr;


static void
ksyms_hdr_init(caddr_t hdraddr)
{

	/* Copy the loaded elf exec header */
	memcpy(&ksyms_hdr.kh_ehdr, hdraddr, sizeof(Elf_Ehdr));

	/* Set correct program/section header sizes, offsets and numbers */
	ksyms_hdr.kh_ehdr.e_phoff = offsetof(struct ksyms_hdr, kh_phdr[0]);
	ksyms_hdr.kh_ehdr.e_phentsize = sizeof(Elf_Phdr);
	ksyms_hdr.kh_ehdr.e_phnum = NPRGHDR;
	ksyms_hdr.kh_ehdr.e_shoff = offsetof(struct ksyms_hdr, kh_shdr[0]);
	ksyms_hdr.kh_ehdr.e_shentsize = sizeof(Elf_Shdr);
	ksyms_hdr.kh_ehdr.e_shnum = NSECHDR;
	ksyms_hdr.kh_ehdr.e_shstrndx = NSECHDR - 1; /* Last section */

	/*
	 * Keep program headers zeroed (unused).
	 * The section headers are hand-crafted.
	 * First section is section zero.
	 */

	/* Second section header; ".symtab" */
	ksyms_hdr.kh_shdr[SYMTAB].sh_name = 1; /* Section 3 offset */
	ksyms_hdr.kh_shdr[SYMTAB].sh_type = SHT_SYMTAB;
	ksyms_hdr.kh_shdr[SYMTAB].sh_offset = sizeof(struct ksyms_hdr);
/*	ksyms_hdr.kh_shdr[SYMTAB].sh_size = filled in at open */
	ksyms_hdr.kh_shdr[SYMTAB].sh_link = 2; /* Corresponding strtab */
	ksyms_hdr.kh_shdr[SYMTAB].sh_info = 0; /* XXX */
	ksyms_hdr.kh_shdr[SYMTAB].sh_addralign = sizeof(long);
	ksyms_hdr.kh_shdr[SYMTAB].sh_entsize = sizeof(Elf_Sym);

	/* Third section header; ".strtab" */
	ksyms_hdr.kh_shdr[STRTAB].sh_name = 9; /* Section 3 offset */
	ksyms_hdr.kh_shdr[STRTAB].sh_type = SHT_STRTAB;
/*	ksyms_hdr.kh_shdr[STRTAB].sh_offset = filled in at open */
/*	ksyms_hdr.kh_shdr[STRTAB].sh_size = filled in at open */
/*	ksyms_hdr.kh_shdr[STRTAB].sh_link = kept zero */
	ksyms_hdr.kh_shdr[STRTAB].sh_info = 0;
	ksyms_hdr.kh_shdr[STRTAB].sh_addralign = sizeof(char);
	ksyms_hdr.kh_shdr[STRTAB].sh_entsize = 0;

	/* Fourth section, ".shstrtab" */
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_name = 17; /* This section name offset */
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_type = SHT_STRTAB;
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_offset =
	    offsetof(struct ksyms_hdr, kh_strtab);
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_size = SHSTRSIZ;
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_addralign = sizeof(char);

	/* Set section names */
	strlcpy(&ksyms_hdr.kh_strtab[1], ".symtab",
	    sizeof(ksyms_hdr.kh_strtab) - 1);
	strlcpy(&ksyms_hdr.kh_strtab[9], ".strtab",
	    sizeof(ksyms_hdr.kh_strtab) - 9);
	strlcpy(&ksyms_hdr.kh_strtab[17], ".shstrtab",
	    sizeof(ksyms_hdr.kh_strtab) - 17);
};

static int
ksymsopen(dev_t dev, int oflags, int devtype, struct proc *p)
{

	if (minor(dev))
		return ENXIO;
	if (ksymsinited == 0)
		return ENXIO;

	ksyms_hdr.kh_shdr[SYMTAB].sh_size = symsz;
	ksyms_hdr.kh_shdr[STRTAB].sh_offset = symsz +
	    ksyms_hdr.kh_shdr[SYMTAB].sh_offset;
	ksyms_hdr.kh_shdr[STRTAB].sh_size = strsz;
	ksyms_isopen = 1;

#ifdef KSYMS_DEBUG
	if (ksyms_debug & FOLLOW_DEVKSYMS)
		printf("ksymsopen: symsz 0x%x strsz 0x%x\n", symsz, strsz);
#endif

	return 0;
}

static int
ksymsclose(dev_t dev, int oflags, int devtype, struct proc *p)
{

#ifdef KSYMS_DEBUG
	if (ksyms_debug & FOLLOW_DEVKSYMS)
		printf("ksymsclose\n");
#endif

	ksyms_isopen = 0;
	wakeup(&ksyms_isopen);
	return 0;
}

#define	HDRSIZ	sizeof(struct ksyms_hdr)

static int
ksymsread(dev_t dev, struct uio *uio, int ioflag)
{
	struct symtab *st;
	size_t filepos, inpos, off;

#ifdef KSYMS_DEBUG
	if (ksyms_debug & FOLLOW_DEVKSYMS)
		printf("ksymsread: offset 0x%llx resid 0x%lx\n",
		    (long long)uio->uio_offset, uio->uio_resid);
#endif

	off = uio->uio_offset;
	if (off >= (strsz + symsz + HDRSIZ))
		return 0; /* End of symtab */
	/*
	 * First: Copy out the ELF header.
	 */
	if (off < HDRSIZ)
		uiomove((char *)&ksyms_hdr + off, HDRSIZ - off, uio);

	/*
	 * Copy out the symbol table.
	 */
	filepos = HDRSIZ;
	CIRCLEQ_FOREACH(st, &symtab_queue, sd_queue) {
		if (uio->uio_resid == 0)
			return 0;
		if (uio->uio_offset <= st->sd_symsize + filepos) {
			inpos = uio->uio_offset - filepos;
			uiomove((char *)st->sd_symstart + inpos,
			   st->sd_symsize - inpos, uio);
		}
		filepos += st->sd_symsize;
	}

	if (filepos != HDRSIZ + symsz)
		panic("ksymsread: unsunc");

	/*
	 * Copy out the string table
	 */
	CIRCLEQ_FOREACH(st, &symtab_queue, sd_queue) {
		if (uio->uio_resid == 0)
			return 0;
		if (uio->uio_offset <= st->sd_strsize + filepos) {
			inpos = uio->uio_offset - filepos;
			uiomove((char *)st->sd_strstart + inpos,
			   st->sd_strsize - inpos, uio);
		}
		filepos += st->sd_strsize;
	}
	return 0;
}

static int
ksymswrite(dev_t dev, struct uio *uio, int ioflag)
{
	return EROFS;
}

static int
ksymsioctl(dev_t dev, u_long cmd, caddr_t data, int fflag, struct proc *p)
{
	struct ksyms_gsymbol *kg = (struct ksyms_gsymbol *)data;
	struct symtab *st;
	Elf_Sym *sym = NULL;
	unsigned long val;
	int error = 0;
	char *str = NULL;

	if (cmd == KIOCGVALUE || cmd == KIOCGSYMBOL)
		str = malloc(ksyms_maxlen, M_DEVBUF, M_WAITOK);

	switch (cmd) {
	case KIOCGVALUE:
		/*
		 * Use the in-kernel symbol lookup code for fast
		 * retreival of a value.
		 */
		if ((error = copyinstr(kg->kg_name, str, ksyms_maxlen, NULL)))
			break;
		if ((error = ksyms_getval(NULL, str, &val, KSYMS_EXTERN)))
			break;
		error = copyout(&val, kg->kg_value, sizeof(long));
		break;

	case KIOCGSYMBOL:
		/*
		 * Use the in-kernel symbol lookup code for fast
		 * retreival of a symbol.
		 */
		if ((error = copyinstr(kg->kg_name, str, ksyms_maxlen, NULL)))
			break;
		CIRCLEQ_FOREACH(st, &symtab_queue, sd_queue) {
			if ((sym = findsym(str, st)) == NULL) /* from userland */
				continue;

			/* Skip if bad binding */
			if (ELF_ST_BIND(sym->st_info) != STB_GLOBAL) {
				sym = NULL;
				continue;
			}
			break;
		}
		/*
		 * XXX which value of sym->st_name should be returned?  The real
		 * one, or the one that matches what reading /dev/ksyms get?
		 *
		 * Currently, we're returning the /dev/ksyms one.
		 */
		if (sym != NULL)
			error = copyout(sym, kg->kg_sym, sizeof(Elf_Sym));
		else
			error = ENOENT;
		break;

	case KIOCGSIZE:
		/*
		 * Get total size of symbol table.
		 */
		*(int *)data = strsz + symsz + HDRSIZ;
		break;

	default:
		error = ENOTTY;
		break;
	}

	if (cmd == KIOCGVALUE || cmd == KIOCGSYMBOL)
		free(str, M_DEVBUF);

	return error;
}

const struct cdevsw ksyms_cdevsw = {
	ksymsopen, ksymsclose, ksymsread, ksymswrite, ksymsioctl,
	    nullstop, notty, nopoll, nommap, nullkqfilter, DV_DULL
};
#endif /* NKSYMS */
