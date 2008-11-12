/*	$NetBSD: kern_ksyms.c,v 1.42 2008/11/12 12:36:16 ad Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Andrew Doran.
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
 *
 *	Consider replacing patricia tree with simpler binary search
 *	for symbol tables.
 *
 *	Add support for mmap, poll.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_ksyms.c,v 1.42 2008/11/12 12:36:16 ad Exp $");

#ifdef _KERNEL
#include "opt_ddb.h"
#include "opt_ddbparam.h"	/* for SYMTAB_SPACE */
#endif

#define _KSYMS_PRIVATE

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/exec.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/atomic.h>
#include <sys/ksyms.h>

#include <lib/libkern/libkern.h>

#ifdef DDB
#include <ddb/db_output.h>
#endif

#include "ksyms.h"

static int ksyms_maxlen;
static bool ksyms_isopen;
static bool ksyms_initted;
static struct ksyms_hdr ksyms_hdr;
static kmutex_t ksyms_lock;

void ksymsattach(int);
static void ksyms_hdr_init(void *);
static void ksyms_sizes_calc(void);

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

int ksyms_symsz;
int ksyms_strsz;
TAILQ_HEAD(, ksyms_symtab) ksyms_symtabs =
    TAILQ_HEAD_INITIALIZER(ksyms_symtabs);
static struct ksyms_symtab kernel_symtab;

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
#define	STRING(idx) (kernel_symtab.sd_symstart[idx].st_name + \
			kernel_symtab.sd_strstart)

static int
ksyms_verify(void *symstart, void *strstart)
{
#if defined(DIAGNOSTIC) || defined(DEBUG)
	if (symstart == NULL)
		printf("ksyms: Symbol table not found\n");
	if (strstart == NULL)
		printf("ksyms: String table not found\n");
	if (symstart == NULL || strstart == NULL)
		printf("ksyms: Perhaps the kernel is stripped?\n");
#endif
	if (symstart == NULL || strstart == NULL)
		return 0;
	KASSERT(symstart <= strstart);
	return 1;
}

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
ptree_gen(char *off, struct ksyms_symtab *tab)
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
		if (tab->sd_minsym == NULL ||
		    sym[i].st_value < tab->sd_minsym->st_value)
			tab->sd_minsym = &sym[i];
		if (tab->sd_maxsym == NULL ||
		    sym[i].st_value > tab->sd_maxsym->st_value)
			tab->sd_maxsym = &sym[i];
	}
}

/*
 * Finds a certain symbol name in a certain symbol table.
 */
static Elf_Sym *
findsym(const char *name, struct ksyms_symtab *table)
{
	Elf_Sym *start = table->sd_symstart;
	int i, sz = table->sd_symsize/sizeof(Elf_Sym);
	char *np;
	char *realstart = table->sd_strstart - table->sd_usroffset;

	if (table == &kernel_symtab && (i = ptree_find(name)) != 0)
		return &start[i];

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
void
ksymsattach(int arg)
{

	if (baseidx == 0)
		ptree_gen(0, &kernel_symtab);
}

/*
 * Add a symbol table.
 * This is intended for use when the symbol table and its corresponding
 * string table are easily available.  If they are embedded in an ELF
 * image, use addsymtab_elf() instead.
 *
 * name - Symbol's table name.
 * symstart, symsize - Address and size of the symbol table.
 * strstart, strsize - Address and size of the string table.
 * tab - Symbol table to be updated with this information.
 * newstart - Address to which the symbol table has to be copied during
 *            shrinking.  If NULL, it is not moved.
 */
static void
addsymtab(const char *name, void *symstart, size_t symsize,
	  void *strstart, size_t strsize, struct ksyms_symtab *tab,
	  void *newstart)
{
	Elf_Sym *sym, *nsym;
	int i, j, n;
	char *str;

	tab->sd_symstart = symstart;
	tab->sd_symsize = symsize;
	tab->sd_strstart = strstart;
	tab->sd_strsize = strsize;
	tab->sd_name = name;
	tab->sd_minsym = NULL;
	tab->sd_maxsym = NULL;
	tab->sd_usroffset = 0;
	tab->sd_gone = false;
#ifdef KSYMS_DEBUG
	printf("newstart %p sym %p ksyms_symsz %d str %p strsz %d send %p\n",
	    newstart, symstart, symsize, strstart, strsize,
	    tab->sd_strstart + tab->sd_strsize);
#endif

	/* Pack symbol table by removing all file name references. */
	sym = tab->sd_symstart;
	nsym = (Elf_Sym *)newstart;
	str = tab->sd_strstart;
	for (i = n = 0; i < tab->sd_symsize/sizeof(Elf_Sym); i++) {
		/*
		 * Remove useless symbols.
		 * Should actually remove all typeless symbols.
		 */
		if (sym[i].st_name == 0)
			continue; /* Skip nameless entries */
		if (sym[i].st_shndx == SHN_UNDEF)
			continue; /* Skip external references */
		if (ELF_ST_TYPE(sym[i].st_info) == STT_FILE)
			continue; /* Skip filenames */
		if (ELF_ST_TYPE(sym[i].st_info) == STT_NOTYPE &&
		    sym[i].st_value == 0 &&
		    strcmp(str + sym[i].st_name, "*ABS*") == 0)
			continue; /* XXX */
		if (ELF_ST_TYPE(sym[i].st_info) == STT_NOTYPE &&
		    strcmp(str + sym[i].st_name, "gcc2_compiled.") == 0)
			continue; /* XXX */

		/* Save symbol. Set it as an absolute offset */
		nsym[n] = sym[i];
		nsym[n].st_shndx = SHN_ABS;
		j = strlen(nsym[n].st_name + tab->sd_strstart) + 1;
		if (j > ksyms_maxlen)
			ksyms_maxlen = j;
		n++;

	}
	tab->sd_symstart = nsym;
	tab->sd_symsize = n * sizeof(Elf_Sym);
	/* ksymsread() is unlocked, so membar. */
	membar_producer();
	TAILQ_INSERT_TAIL(&ksyms_symtabs, tab, sd_queue);
	ksyms_sizes_calc();
	ksyms_initted = true;
}

/*
 * Setup the kernel symbol table stuff.
 */
void
ksyms_init(int symsize, void *start, void *end)
{
	int i, j;
	Elf_Shdr *shdr;
	char *symstart = NULL, *strstart = NULL;
	size_t strsize = 0;
	Elf_Ehdr *ehdr;

	mutex_init(&ksyms_lock, MUTEX_DEFAULT, IPL_NONE);
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
		printf("[ Kernel symbol table invalid! ]\n");
		return; /* nothing to do */
	}

	/* Loaded header will be scratched in addsymtab */
	ksyms_hdr_init(start);

	/* Find the symbol table and the corresponding string table. */
	shdr = (Elf_Shdr *)((uint8_t *)start + ehdr->e_shoff);
	for (i = 1; i < ehdr->e_shnum; i++) {
		if (shdr[i].sh_type != SHT_SYMTAB)
			continue;
		if (shdr[i].sh_offset == 0)
			continue;
		symstart = (uint8_t *)start + shdr[i].sh_offset;
		symsize = shdr[i].sh_size;
		j = shdr[i].sh_link;
		if (shdr[j].sh_offset == 0)
			continue; /* Can this happen? */
		strstart = (uint8_t *)start + shdr[j].sh_offset;
		strsize = shdr[j].sh_size;
		break;
	}

	if (!ksyms_verify(symstart, strstart))
		return;
	addsymtab("netbsd", symstart, symsize, strstart, strsize,
	    &kernel_symtab, start);

#ifdef DEBUG
	printf("Loaded initial symtab at %p, strtab at %p, # entries %ld\n",
	    kernel_symtab.sd_symstart, kernel_symtab.sd_strstart,
	    (long)kernel_symtab.sd_symsize/sizeof(Elf_Sym));
#endif
}

/*
 * Setup the kernel symbol table stuff.
 * Use this when the address of the symbol and string tables are known;
 * otherwise use ksyms_init with an ELF image.
 * We need to pass a minimal ELF header which will later be completed by
 * ksyms_hdr_init and handed off to userland through /dev/ksyms.  We use
 * a void *rather than a pointer to avoid exposing the Elf_Ehdr type.
 */
void
ksyms_init_explicit(void *ehdr, void *symstart, size_t symsize,
		    void *strstart, size_t strsize)
{

	mutex_init(&ksyms_lock, MUTEX_DEFAULT, IPL_NONE);

	if (!ksyms_verify(symstart, strstart))
		return;

	ksyms_hdr_init(ehdr);
	addsymtab("netbsd", symstart, symsize, strstart, strsize,
	    &kernel_symtab, symstart);
}

/*
 * Get the value associated with a symbol.
 * "mod" is the module name, or null if any module.
 * "sym" is the symbol name.
 * "val" is a pointer to the corresponding value, if call succeeded.
 * Returns 0 if success or ENOENT if no such entry.
 *
 * Call with ksyms_lock, unless known that the symbol table can't change.
 */
int
ksyms_getval_unlocked(const char *mod, const char *sym, unsigned long *val,
		      int type)
{
	struct ksyms_symtab *st;
	Elf_Sym *es;

	if (!ksyms_initted)
		return ENOENT;

#ifdef KSYMS_DEBUG
	if (ksyms_debug & FOLLOW_CALLS)
		printf("ksyms_getval_unlocked: mod %s sym %s valp %p\n",
		    mod, sym, val);
#endif

	TAILQ_FOREACH(st, &ksyms_symtabs, sd_queue) {
		if (st->sd_gone)
			continue;
		if (mod && strcmp(st->sd_name, mod))
			continue;
		if ((es = findsym(sym, st)) == NULL)
			continue;
		if (es->st_shndx == SHN_UNDEF)
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

int
ksyms_getval(const char *mod, const char *sym, unsigned long *val, int type)
{
	int rc;

	mutex_enter(&ksyms_lock);
	rc = ksyms_getval_unlocked(mod, sym, val, type);
	mutex_exit(&ksyms_lock);
	return rc;
}

/*
 * Get "mod" and "symbol" associated with an address.
 * Returns 0 if success or ENOENT if no such entry.
 *
 * Call with ksyms_lock, unless known that the symbol table can't change.
 */
int
ksyms_getname(const char **mod, const char **sym, vaddr_t v, int f)
{
	struct ksyms_symtab *st;
	Elf_Sym *les, *es = NULL;
	vaddr_t laddr = 0;
	const char *lmod = NULL;
	char *stable = NULL;
	int type, i, sz;

	if (!ksyms_initted)
		return ENOENT;

	TAILQ_FOREACH(st, &ksyms_symtabs, sd_queue) {
		if (st->sd_gone)
			continue;
		if (st->sd_minsym != NULL && v < st->sd_minsym->st_value)
			continue;
		if (st->sd_maxsym != NULL && v > st->sd_maxsym->st_value)
			continue;
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

/*
 * Add a symbol table from a loadable module.
 */
void
ksyms_modload(const char *name, void *symstart, vsize_t symsize,
	      char *strstart, vsize_t strsize)
{
	struct ksyms_symtab *st;

	st = kmem_zalloc(sizeof(*st), KM_SLEEP);
	mutex_enter(&ksyms_lock);
	addsymtab(name, symstart, symsize, strstart, strsize, st, symstart);
	mutex_exit(&ksyms_lock);
}

/*
 * Remove a symbol table from a loadable module.
 */
void
ksyms_modunload(const char *name)
{
	struct ksyms_symtab *st;

	mutex_enter(&ksyms_lock);
	TAILQ_FOREACH(st, &ksyms_symtabs, sd_queue) {
		if (st->sd_gone)
			continue;
		if (strcmp(name, st->sd_name) != 0)
			continue;
		st->sd_gone = true;
		if (!ksyms_isopen) {
			TAILQ_REMOVE(&ksyms_symtabs, st, sd_queue);
			ksyms_sizes_calc();
			kmem_free(st, sizeof(*st));
		}
		break;
	}
	mutex_exit(&ksyms_lock);
	KASSERT(st != NULL);
}

#ifdef DDB
/*
 * Keep sifting stuff here, to avoid export of ksyms internals.
 *
 * Systems is expected to be quiescent, so no locking done.
 */
int
ksyms_sift(char *mod, char *sym, int mode)
{
	struct ksyms_symtab *st;
	char *sb;
	int i, sz;

	if (!ksyms_initted)
		return ENOENT;

	TAILQ_FOREACH(st, &ksyms_symtabs, sd_queue) {
		if (st->sd_gone)
			continue;
		if (mod && strcmp(mod, st->sd_name))
			continue;
		sb = st->sd_strstart - st->sd_usroffset;

		sz = st->sd_symsize/sizeof(Elf_Sym);
		for (i = 0; i < sz; i++) {
			Elf_Sym *les = st->sd_symstart + i;
			char c;

			if (strstr(sb + les->st_name, sym) == NULL)
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
				db_printf("%s%c ", sb + les->st_name, c);
			} else
				db_printf("%s ", sb + les->st_name);
		}
	}
	return ENOENT;
}
#endif /* DDB */

/*
 * In case we exposing the symbol table to the userland using the pseudo-
 * device /dev/ksyms, it is easier to provide all the tables as one.
 * However, it means we have to change all the st_name fields for the
 * symbols so they match the ELF image that the userland will read
 * through the device.
 *
 * The actual (correct) value of st_name is preserved through a global
 * offset stored in the symbol table structure.
 *
 * Call with ksyms_lock held.
 */
static void
ksyms_sizes_calc(void)
{
        struct ksyms_symtab *st;
	int i, delta;

        ksyms_symsz = ksyms_strsz = 0;
        TAILQ_FOREACH(st, &ksyms_symtabs, sd_queue) {
		delta = ksyms_strsz - st->sd_usroffset;
		if (delta != 0) {
			for (i = 0; i < st->sd_symsize/sizeof(Elf_Sym); i++)
				st->sd_symstart[i].st_name += delta;
			st->sd_usroffset = ksyms_strsz;
		}
                ksyms_symsz += st->sd_symsize;
                ksyms_strsz += st->sd_strsize;
        }
}

static void
ksyms_hdr_init(void *hdraddr)
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

	/* Text */
	ksyms_hdr.kh_phdr[0].p_type = PT_LOAD;
	ksyms_hdr.kh_phdr[0].p_memsz = (unsigned long)-1L;
	ksyms_hdr.kh_phdr[0].p_flags = PF_R | PF_X;

	/* Data */
	ksyms_hdr.kh_phdr[1].p_type = PT_LOAD;
	ksyms_hdr.kh_phdr[1].p_memsz = (unsigned long)-1L;
	ksyms_hdr.kh_phdr[1].p_flags = PF_R | PF_W | PF_X;

	/* First section is null */

	/* Second section header; ".symtab" */
	ksyms_hdr.kh_shdr[SYMTAB].sh_name = 1; /* Section 3 offset */
	ksyms_hdr.kh_shdr[SYMTAB].sh_type = SHT_SYMTAB;
	ksyms_hdr.kh_shdr[SYMTAB].sh_offset = sizeof(struct ksyms_hdr);
/*	ksyms_hdr.kh_shdr[SYMTAB].sh_size = filled in at open */
	ksyms_hdr.kh_shdr[SYMTAB].sh_link = 2; /* Corresponding strtab */
	ksyms_hdr.kh_shdr[SYMTAB].sh_addralign = sizeof(long);
	ksyms_hdr.kh_shdr[SYMTAB].sh_entsize = sizeof(Elf_Sym);

	/* Third section header; ".strtab" */
	ksyms_hdr.kh_shdr[STRTAB].sh_name = 9; /* Section 3 offset */
	ksyms_hdr.kh_shdr[STRTAB].sh_type = SHT_STRTAB;
/*	ksyms_hdr.kh_shdr[STRTAB].sh_offset = filled in at open */
/*	ksyms_hdr.kh_shdr[STRTAB].sh_size = filled in at open */
	ksyms_hdr.kh_shdr[STRTAB].sh_addralign = sizeof(char);

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
}

static int
ksymsopen(dev_t dev, int oflags, int devtype, struct lwp *l)
{

	if (minor(dev) != 0 || !ksyms_initted)
		return ENXIO;

	/*
	 * Create a "snapshot" of the kernel symbol table.  Setting
	 * ksyms_isopen will prevent symbol tables from being freed.
	 */
	mutex_enter(&ksyms_lock);
	ksyms_hdr.kh_shdr[SYMTAB].sh_size = ksyms_symsz;
	ksyms_hdr.kh_shdr[SYMTAB].sh_info = ksyms_symsz / sizeof(Elf_Sym);
	ksyms_hdr.kh_shdr[STRTAB].sh_offset = ksyms_symsz +
	    ksyms_hdr.kh_shdr[SYMTAB].sh_offset;
	ksyms_hdr.kh_shdr[STRTAB].sh_size = ksyms_strsz;
	ksyms_isopen = true;
	mutex_exit(&ksyms_lock);

	return 0;
}

static int
ksymsclose(dev_t dev, int oflags, int devtype, struct lwp *l)
{
	struct ksyms_symtab *st, *next;
	bool resize;

	/* Discard refernces to symbol tables. */
	mutex_enter(&ksyms_lock);
	ksyms_isopen = false;
	resize = false;
	for (st = TAILQ_FIRST(&ksyms_symtabs); st != NULL; st = next) {
		next = TAILQ_NEXT(st, sd_queue);
		if (st->sd_gone) {
			TAILQ_REMOVE(&ksyms_symtabs, st, sd_queue);
			kmem_free(st, sizeof(*st));
			resize = true;
		}
	}
	if (resize)
		ksyms_sizes_calc();
	mutex_exit(&ksyms_lock);

	return 0;
}

static int
ksymsread(dev_t dev, struct uio *uio, int ioflag)
{
	struct ksyms_symtab *st;
	size_t filepos, inpos, off;
	int error;

	/*
	 * First: Copy out the ELF header.   XXX Lose if ksymsopen()
	 * occurs during read of the header.
	 */
	off = uio->uio_offset;
	if (off < sizeof(struct ksyms_hdr)) {
		error = uiomove((char *)&ksyms_hdr + off,
		    sizeof(struct ksyms_hdr) - off, uio);
		if (error != 0)
			return error;
	}

	/*
	 * Copy out the symbol table.
	 */
	filepos = sizeof(struct ksyms_hdr);
	TAILQ_FOREACH(st, &ksyms_symtabs, sd_queue) {
		if (uio->uio_resid == 0)
			return 0;
		if (uio->uio_offset <= st->sd_symsize + filepos) {
			inpos = uio->uio_offset - filepos;
			error = uiomove((char *)st->sd_symstart + inpos,
			   st->sd_symsize - inpos, uio);
			if (error != 0)
				return error;
		}
		filepos += st->sd_symsize;
	}

	/*
	 * Copy out the string table
	 */
	KASSERT(filepos == sizeof(struct ksyms_hdr) +
	    ksyms_hdr.kh_shdr[SYMTAB].sh_size);
	TAILQ_FOREACH(st, &ksyms_symtabs, sd_queue) {
		if (uio->uio_resid == 0)
			return 0;
		if (uio->uio_offset <= st->sd_strsize + filepos) {
			inpos = uio->uio_offset - filepos;
			error = uiomove((char *)st->sd_strstart + inpos,
			   st->sd_strsize - inpos, uio);
			if (error != 0)
				return error;
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
ksymsioctl(dev_t dev, u_long cmd, void *data, int fflag, struct lwp *l)
{
	struct ksyms_gsymbol *kg = (struct ksyms_gsymbol *)data;
	struct ksyms_symtab *st;
	Elf_Sym *sym = NULL, copy;
	unsigned long val;
	int error = 0;
	char *str = NULL;
	int len;

	/* Read ksyms_maxlen only once while not holding the lock. */
	len = ksyms_maxlen;

	if (cmd == KIOCGVALUE || cmd == KIOCGSYMBOL) {
		str = kmem_alloc(len, KM_SLEEP);
		if ((error = copyinstr(kg->kg_name, str, len, NULL)) != 0) {
			kmem_free(str, len);
			return error;
		}
	}

	switch (cmd) {
	case KIOCGVALUE:
		/*
		 * Use the in-kernel symbol lookup code for fast
		 * retreival of a value.
		 */
		error = ksyms_getval(NULL, str, &val, KSYMS_EXTERN);
		if (error == 0)
			error = copyout(&val, kg->kg_value, sizeof(long));
		kmem_free(str, len);
		break;

	case KIOCGSYMBOL:
		/*
		 * Use the in-kernel symbol lookup code for fast
		 * retreival of a symbol.
		 */
		mutex_enter(&ksyms_lock);
		TAILQ_FOREACH(st, &ksyms_symtabs, sd_queue) {
			if (st->sd_gone)
				continue;
			if ((sym = findsym(str, st)) == NULL)
				continue;
#ifdef notdef
			/* Skip if bad binding */
			if (ELF_ST_BIND(sym->st_info) != STB_GLOBAL) {
				sym = NULL;
				continue;
			}
#endif
			break;
		}
		if (sym != NULL) {
			memcpy(&copy, sym, sizeof(copy));
			mutex_exit(&ksyms_lock);
			error = copyout(&copy, kg->kg_sym, sizeof(Elf_Sym));
		} else {
			mutex_exit(&ksyms_lock);
			error = ENOENT;
		}
		kmem_free(str, len);
		break;

	case KIOCGSIZE:
		/*
		 * Get total size of symbol table.
		 */
		mutex_enter(&ksyms_lock);
		*(int *)data = ksyms_strsz + ksyms_symsz +
		    sizeof(struct ksyms_hdr);
		mutex_exit(&ksyms_lock);
		break;

	default:
		error = ENOTTY;
		break;
	}

	return error;
}

const struct cdevsw ksyms_cdevsw = {
	ksymsopen, ksymsclose, ksymsread, ksymswrite, ksymsioctl,
	nullstop, notty, nopoll, nommap, nullkqfilter, D_OTHER | D_MPSAFE
};
