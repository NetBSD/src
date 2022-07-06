/*	$NetBSD: kern_ksyms.c,v 1.106 2022/07/06 01:12:46 riastradh Exp $	*/

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
 *	Add support for mmap, poll.
 *	Constify tables.
 *	Constify db_symtab and move it to .rodata.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_ksyms.c,v 1.106 2022/07/06 01:12:46 riastradh Exp $");

#if defined(_KERNEL) && defined(_KERNEL_OPT)
#include "opt_copy_symtab.h"
#include "opt_ddb.h"
#include "opt_dtrace.h"
#endif

#define _KSYMS_PRIVATE

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kauth.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/atomic.h>
#include <sys/ksyms.h>
#include <sys/kernel.h>
#include <sys/intr.h>
#include <sys/pserialize.h>
#include <sys/stat.h>

#include <uvm/uvm_extern.h>

#ifdef DDB
#include <ddb/db_output.h>
#endif

#include "ksyms.h"
#if NKSYMS > 0
#include "ioconf.h"
#endif

struct ksyms_snapshot {
	uint64_t		ks_refcnt;
	uint64_t		ks_gen;
	struct uvm_object	*ks_uobj;
	size_t			ks_size;
	dev_t			ks_dev;
	int			ks_maxlen;
};

#define KSYMS_MAX_ID	98304
#ifdef KDTRACE_HOOKS
static uint32_t ksyms_nmap[KSYMS_MAX_ID];	/* sorted symbol table map */
#else
static uint32_t *ksyms_nmap = NULL;
#endif

static int ksyms_maxlen;
static bool ksyms_initted;
static bool ksyms_loaded;
static kmutex_t ksyms_lock __cacheline_aligned;
static struct ksyms_symtab kernel_symtab;
static kcondvar_t ksyms_cv;
static struct lwp *ksyms_snapshotting;
static struct ksyms_snapshot *ksyms_snapshot;
static uint64_t ksyms_snapshot_gen;
static pserialize_t ksyms_psz __read_mostly;

static void ksyms_hdr_init(const void *);
static void ksyms_sizes_calc(void);
static struct ksyms_snapshot *ksyms_snapshot_alloc(int, size_t, dev_t,
    uint64_t);
static void ksyms_snapshot_release(struct ksyms_snapshot *);

#ifdef KSYMS_DEBUG
#define	FOLLOW_CALLS		1
#define	FOLLOW_MORE_CALLS	2
#define	FOLLOW_DEVKSYMS		4
static int ksyms_debug;
#endif

#define		SYMTAB_FILLER	"|This is the symbol table!"

#ifdef makeoptions_COPY_SYMTAB
extern char db_symtab[];
extern int db_symtabsize;
#endif

/*
 * used by savecore(8) so non-static
 */
struct ksyms_hdr ksyms_hdr;
int ksyms_symsz;
int ksyms_strsz;
int ksyms_ctfsz;	/* this is not currently used by savecore(8) */
TAILQ_HEAD(ksyms_symtab_queue, ksyms_symtab) ksyms_symtabs =
    TAILQ_HEAD_INITIALIZER(ksyms_symtabs);
static struct pslist_head ksyms_symtabs_psz = PSLIST_INITIALIZER;

static int
ksyms_verify(const void *symstart, const void *strstart)
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
	return 1;
}

/*
 * Finds a certain symbol name in a certain symbol table.
 */
static Elf_Sym *
findsym(const char *name, struct ksyms_symtab *table, int type)
{
	Elf_Sym *sym, *maxsym;
	int low, mid, high, nglob;
	char *str, *cmp;

	sym = table->sd_symstart;
	str = table->sd_strstart - table->sd_usroffset;
	nglob = table->sd_nglob;
	low = 0;
	high = nglob;

	/*
	 * Start with a binary search of all global symbols in this table.
	 * Global symbols must have unique names.
	 */
	while (low < high) {
		mid = (low + high) >> 1;
		cmp = sym[mid].st_name + str;
		if (cmp[0] < name[0] || strcmp(cmp, name) < 0) {
			low = mid + 1;
		} else {
			high = mid;
		}
	}
	KASSERT(low == high);
	if (__predict_true(low < nglob &&
	    strcmp(sym[low].st_name + str, name) == 0)) {
		KASSERT(ELF_ST_BIND(sym[low].st_info) == STB_GLOBAL);
		return &sym[low];
	}

	/*
	 * Perform a linear search of local symbols (rare).  Many local
	 * symbols with the same name can exist so are not included in
	 * the binary search.
	 */
	if (type != KSYMS_EXTERN) {
		maxsym = sym + table->sd_symsize / sizeof(Elf_Sym);
		for (sym += nglob; sym < maxsym; sym++) {
			if (strcmp(name, sym->st_name + str) == 0) {
				return sym;
			}
		}
	}
	return NULL;
}

/*
 * The "attach" is in reality done in ksyms_init().
 */
#if NKSYMS > 0
/*
 * ksyms can be loaded even if the kernel has a missing "pseudo-device ksyms"
 * statement because ddb and modules require it. Fixing it properly requires
 * fixing config to warn about required, but missing preudo-devices. For now,
 * if we don't have the pseudo-device we don't need the attach function; this
 * is fine, as it does nothing.
 */
void
ksymsattach(int arg)
{
}
#endif

void
ksyms_init(void)
{

#ifdef makeoptions_COPY_SYMTAB
	if (!ksyms_loaded &&
	    strncmp(db_symtab, SYMTAB_FILLER, sizeof(SYMTAB_FILLER))) {
		ksyms_addsyms_elf(db_symtabsize, db_symtab,
		    db_symtab + db_symtabsize);
	}
#endif

	if (!ksyms_initted) {
		mutex_init(&ksyms_lock, MUTEX_DEFAULT, IPL_NONE);
		cv_init(&ksyms_cv, "ksyms");
		ksyms_psz = pserialize_create();
		ksyms_initted = true;
	}
}

/*
 * Are any symbols available?
 */
bool
ksyms_available(void)
{

	return ksyms_loaded;
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
static const char *addsymtab_strstart;

static int
addsymtab_compar(const void *a, const void *b)
{
	const Elf_Sym *sa, *sb;

	sa = a;
	sb = b;

	/*
	 * Split the symbol table into two, with globals at the start
	 * and locals at the end.
	 */
	if (ELF_ST_BIND(sa->st_info) != ELF_ST_BIND(sb->st_info)) {
		if (ELF_ST_BIND(sa->st_info) == STB_GLOBAL) {
			return -1;
		}
		if (ELF_ST_BIND(sb->st_info) == STB_GLOBAL) {
			return 1;
		}
	}

	/* Within each band, sort by name. */
	return strcmp(sa->st_name + addsymtab_strstart,
	    sb->st_name + addsymtab_strstart);
}

static void
addsymtab(const char *name, void *symstart, size_t symsize,
	  void *strstart, size_t strsize, struct ksyms_symtab *tab,
	  void *newstart, void *ctfstart, size_t ctfsize, uint32_t *nmap)
{
	Elf_Sym *sym, *nsym, ts;
	int i, j, n, nglob;
	char *str;
	int nsyms = symsize / sizeof(Elf_Sym);
	int s;

	/* Sanity check for pre-allocated map table used during startup. */
	if ((nmap == ksyms_nmap) && (nsyms >= KSYMS_MAX_ID)) {
		printf("kern_ksyms: ERROR %d > %d, increase KSYMS_MAX_ID\n",
		    nsyms, KSYMS_MAX_ID);

		/* truncate for now */
		nsyms = KSYMS_MAX_ID - 1;
	}

	tab->sd_symstart = symstart;
	tab->sd_symsize = symsize;
	tab->sd_strstart = strstart;
	tab->sd_strsize = strsize;
	tab->sd_name = name;
	tab->sd_minsym = UINTPTR_MAX;
	tab->sd_maxsym = 0;
	tab->sd_usroffset = 0;
	tab->sd_ctfstart = ctfstart;
	tab->sd_ctfsize = ctfsize;
	tab->sd_nmap = nmap;
	tab->sd_nmapsize = nsyms;
#ifdef KSYMS_DEBUG
	printf("newstart %p sym %p ksyms_symsz %zu str %p strsz %zu send %p\n",
	    newstart, symstart, symsize, strstart, strsize,
	    tab->sd_strstart + tab->sd_strsize);
#endif

	if (nmap) {
		memset(nmap, 0, nsyms * sizeof(uint32_t));
	}

	/* Pack symbol table by removing all file name references. */
	sym = tab->sd_symstart;
	nsym = (Elf_Sym *)newstart;
	str = tab->sd_strstart;
	nglob = 0;
	for (i = n = 0; i < nsyms; i++) {

		/*
		 * This breaks CTF mapping, so don't do it when
		 * DTrace is enabled.
		 */
#ifndef KDTRACE_HOOKS
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
#endif

		/* Save symbol. Set it as an absolute offset */
		nsym[n] = sym[i];

#ifdef KDTRACE_HOOKS
		if (nmap != NULL) {
			/*
			 * Save the size, replace it with the symbol id so
			 * the mapping can be done after the cleanup and sort.
			 */
			nmap[i] = nsym[n].st_size;
			nsym[n].st_size = i + 1;	/* zero is reserved */
		}
#endif

		if (sym[i].st_shndx != SHN_ABS) {
			nsym[n].st_shndx = SHBSS;
		} else {
			/* SHN_ABS is a magic value, don't overwrite it */
		}

		j = strlen(nsym[n].st_name + str) + 1;
		if (j > ksyms_maxlen)
			ksyms_maxlen = j;
		nglob += (ELF_ST_BIND(nsym[n].st_info) == STB_GLOBAL);

		/* Compute min and max symbols. */
		if (strcmp(str + sym[i].st_name, "*ABS*") != 0
		    && ELF_ST_TYPE(nsym[n].st_info) != STT_NOTYPE) {
			if (nsym[n].st_value < tab->sd_minsym) {
				tab->sd_minsym = nsym[n].st_value;
			}
			if (nsym[n].st_value > tab->sd_maxsym) {
				tab->sd_maxsym = nsym[n].st_value;
			}
		}
		n++;
	}

	/* Fill the rest of the record, and sort the symbols. */
	tab->sd_symstart = nsym;
	tab->sd_symsize = n * sizeof(Elf_Sym);
	tab->sd_nglob = nglob;

	addsymtab_strstart = str;
	if (kheapsort(nsym, n, sizeof(Elf_Sym), addsymtab_compar, &ts) != 0)
		panic("addsymtab");

#ifdef KDTRACE_HOOKS
	/*
	 * Build the mapping from original symbol id to new symbol table.
	 * Deleted symbols will have a zero map, indices will be one based
	 * instead of zero based.
	 * Resulting map is sd_nmap[original_index] = new_index + 1
	 */
	if (nmap != NULL) {
		int new;
		for (new = 0; new < n; new++) {
			uint32_t orig = nsym[new].st_size - 1;
			uint32_t size = nmap[orig];

			nmap[orig] = new + 1;

			/* restore the size */
			nsym[new].st_size = size;
		}
	}
#endif

	KASSERT(strcmp(name, "netbsd") == 0 || mutex_owned(&ksyms_lock));
	KASSERT(cold || mutex_owned(&ksyms_lock));

	/*
	 * Publish the symtab.  Do this at splhigh to ensure ddb never
	 * witnesses an inconsistent state of the queue, unless memory
	 * is so corrupt that we crash in PSLIST_WRITER_INSERT_AFTER or
	 * TAILQ_INSERT_TAIL.
	 */
	PSLIST_ENTRY_INIT(tab, sd_pslist);
	s = splhigh();
	if (TAILQ_EMPTY(&ksyms_symtabs)) {
		PSLIST_WRITER_INSERT_HEAD(&ksyms_symtabs_psz, tab, sd_pslist);
	} else {
		struct ksyms_symtab *last;

		last = TAILQ_LAST(&ksyms_symtabs, ksyms_symtab_queue);
		PSLIST_WRITER_INSERT_AFTER(last, tab, sd_pslist);
	}
	TAILQ_INSERT_TAIL(&ksyms_symtabs, tab, sd_queue);
	splx(s);

	ksyms_sizes_calc();
	ksyms_loaded = true;
}

/*
 * Setup the kernel symbol table stuff.
 */
void
ksyms_addsyms_elf(int symsize, void *start, void *end)
{
	int i, j;
	Elf_Shdr *shdr;
	char *symstart = NULL, *strstart = NULL;
	size_t strsize = 0;
	Elf_Ehdr *ehdr;
	char *ctfstart = NULL;
	size_t ctfsize = 0;

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

#ifdef KDTRACE_HOOKS
	/* Find the CTF section */
	shdr = (Elf_Shdr *)((uint8_t *)start + ehdr->e_shoff);
	if (ehdr->e_shstrndx != 0) {
		char *shstr = (uint8_t *)start +
		    shdr[ehdr->e_shstrndx].sh_offset;
		for (i = 1; i < ehdr->e_shnum; i++) {
#ifdef DEBUG
			printf("ksyms: checking %s\n", &shstr[shdr[i].sh_name]);
#endif
			if (shdr[i].sh_type != SHT_PROGBITS)
				continue;
			if (strncmp(".SUNW_ctf", &shstr[shdr[i].sh_name], 10)
			    != 0)
				continue;
			ctfstart = (uint8_t *)start + shdr[i].sh_offset;
			ctfsize = shdr[i].sh_size;
			ksyms_ctfsz = ctfsize;
#ifdef DEBUG
			aprint_normal("Found CTF at %p, size 0x%zx\n",
			    ctfstart, ctfsize);
#endif
			break;
		}
#ifdef DEBUG
	} else {
		printf("ksyms: e_shstrndx == 0\n");
#endif
	}
#endif

	if (!ksyms_verify(symstart, strstart))
		return;

	addsymtab("netbsd", symstart, symsize, strstart, strsize,
	    &kernel_symtab, symstart, ctfstart, ctfsize, ksyms_nmap);

#ifdef DEBUG
	aprint_normal("Loaded initial symtab at %p, strtab at %p, # entries %ld\n",
	    kernel_symtab.sd_symstart, kernel_symtab.sd_strstart,
	    (long)kernel_symtab.sd_symsize/sizeof(Elf_Sym));
#endif

	/* Should be no snapshot to invalidate yet.  */
	KASSERT(ksyms_snapshot == NULL);
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
ksyms_addsyms_explicit(void *ehdr, void *symstart, size_t symsize,
    void *strstart, size_t strsize)
{
	if (!ksyms_verify(symstart, strstart))
		return;

	ksyms_hdr_init(ehdr);
	addsymtab("netbsd", symstart, symsize, strstart, strsize,
	    &kernel_symtab, symstart, NULL, 0, ksyms_nmap);

	/* Should be no snapshot to invalidate yet.  */
	KASSERT(ksyms_snapshot == NULL);
}

/*
 * Get the value associated with a symbol.
 * "mod" is the module name, or null if any module.
 * "sym" is the symbol name.
 * "val" is a pointer to the corresponding value, if call succeeded.
 * Returns 0 if success or ENOENT if no such entry.
 *
 * If symp is nonnull, caller must hold ksyms_lock or module_lock, have
 * ksyms_opencnt nonzero, be in a pserialize read section, be in ddb
 * with all other CPUs quiescent.
 */
int
ksyms_getval_unlocked(const char *mod, const char *sym, Elf_Sym **symp,
    unsigned long *val, int type)
{
	struct ksyms_symtab *st;
	Elf_Sym *es;
	int s, error = ENOENT;

#ifdef KSYMS_DEBUG
	if (ksyms_debug & FOLLOW_CALLS)
		printf("%s: mod %s sym %s valp %p\n", __func__, mod, sym, val);
#endif

	s = pserialize_read_enter();
	PSLIST_READER_FOREACH(st, &ksyms_symtabs_psz, struct ksyms_symtab,
	    sd_pslist) {
		if (mod != NULL && strcmp(st->sd_name, mod))
			continue;
		if ((es = findsym(sym, st, type)) != NULL) {
			*val = es->st_value;
			if (symp)
				*symp = es;
			error = 0;
			break;
		}
	}
	pserialize_read_exit(s);
	return error;
}

int
ksyms_getval(const char *mod, const char *sym, unsigned long *val, int type)
{

	if (!ksyms_loaded)
		return ENOENT;

	/* No locking needed -- we read the table pserialized.  */
	return ksyms_getval_unlocked(mod, sym, NULL, val, type);
}

/*
 * ksyms_get_mod(mod)
 *
 * Return the symtab for the given module name.  Caller must ensure
 * that the module cannot be unloaded until after this returns.
 */
struct ksyms_symtab *
ksyms_get_mod(const char *mod)
{
	struct ksyms_symtab *st;
	int s;

	s = pserialize_read_enter();
	PSLIST_READER_FOREACH(st, &ksyms_symtabs_psz, struct ksyms_symtab,
	    sd_pslist) {
		if (mod != NULL && strcmp(st->sd_name, mod))
			continue;
		break;
	}
	pserialize_read_exit(s);

	return st;
}


/*
 * ksyms_mod_foreach()
 *
 * Iterate over the symbol table of the specified module, calling the callback
 * handler for each symbol. Stop iterating if the handler return is non-zero.
 *
 */

int
ksyms_mod_foreach(const char *mod, ksyms_callback_t callback, void *opaque)
{
	struct ksyms_symtab *st;
	Elf_Sym *sym, *maxsym;
	char *str;
	int symindx;

	if (!ksyms_loaded)
		return ENOENT;

	mutex_enter(&ksyms_lock);

	/* find the module */
	TAILQ_FOREACH(st, &ksyms_symtabs, sd_queue) {
		if (mod != NULL && strcmp(st->sd_name, mod))
			continue;

		sym = st->sd_symstart;
		str = st->sd_strstart - st->sd_usroffset;

		/* now iterate through the symbols */
		maxsym = sym + st->sd_symsize / sizeof(Elf_Sym);
		for (symindx = 0; sym < maxsym; sym++, symindx++) {
			if (callback(str + sym->st_name, symindx,
			    (void *)sym->st_value,
			    sym->st_size,
			    sym->st_info,
			    opaque) != 0) {
				break;
			}
		}
	}
	mutex_exit(&ksyms_lock);

	return 0;
}

/*
 * Get "mod" and "symbol" associated with an address.
 * Returns 0 if success or ENOENT if no such entry.
 *
 * Caller must hold ksyms_lock or module_lock, have ksyms_opencnt
 * nonzero, be in a pserialize read section, or be in ddb with all
 * other CPUs quiescent.
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

	if (!ksyms_loaded)
		return ENOENT;

	PSLIST_READER_FOREACH(st, &ksyms_symtabs_psz, struct ksyms_symtab,
	    sd_pslist) {
		if (v < st->sd_minsym || v > st->sd_maxsym)
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
	struct ksyms_snapshot *ks;
	void *nmap;

	st = kmem_zalloc(sizeof(*st), KM_SLEEP);
	nmap = kmem_zalloc(symsize / sizeof(Elf_Sym) * sizeof (uint32_t),
			   KM_SLEEP);
	mutex_enter(&ksyms_lock);
	addsymtab(name, symstart, symsize, strstart, strsize, st, symstart,
	    NULL, 0, nmap);
	ks = ksyms_snapshot;
	ksyms_snapshot = NULL;
	mutex_exit(&ksyms_lock);

	if (ks)
		ksyms_snapshot_release(ks);
}

/*
 * Remove a symbol table from a loadable module.
 */
void
ksyms_modunload(const char *name)
{
	struct ksyms_symtab *st;
	struct ksyms_snapshot *ks;
	int s;

	mutex_enter(&ksyms_lock);
	TAILQ_FOREACH(st, &ksyms_symtabs, sd_queue) {
		if (strcmp(name, st->sd_name) != 0)
			continue;
		break;
	}
	KASSERT(st != NULL);

	/* Wait for any snapshot in progress to complete.  */
	while (ksyms_snapshotting)
		cv_wait(&ksyms_cv, &ksyms_lock);

	/*
	 * Remove the symtab.  Do this at splhigh to ensure ddb never
	 * witnesses an inconsistent state of the queue, unless memory
	 * is so corrupt that we crash in TAILQ_REMOVE or
	 * PSLIST_WRITER_REMOVE.
	 */
	s = splhigh();
	TAILQ_REMOVE(&ksyms_symtabs, st, sd_queue);
	PSLIST_WRITER_REMOVE(st, sd_pslist);
	splx(s);

	/*
	 * And wait a grace period, in case there are any pserialized
	 * readers in flight.
	 */
	pserialize_perform(ksyms_psz);
	PSLIST_ENTRY_DESTROY(st, sd_pslist);

	/* Recompute the ksyms sizes now that we've removed st.  */
	ksyms_sizes_calc();

	/* Invalidate the global ksyms snapshot.  */
	ks = ksyms_snapshot;
	ksyms_snapshot = NULL;
	mutex_exit(&ksyms_lock);

	/*
	 * No more references are possible.  Free the name map and the
	 * symtab itself, which we had allocated in ksyms_modload.
	 */
	kmem_free(st->sd_nmap, st->sd_nmapsize * sizeof(uint32_t));
	kmem_free(st, sizeof(*st));

	/* Release the formerly global ksyms snapshot, if any.  */
	if (ks)
		ksyms_snapshot_release(ks);
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

	if (!ksyms_loaded)
		return ENOENT;

	TAILQ_FOREACH(st, &ksyms_symtabs, sd_queue) {
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

	KASSERT(cold || mutex_owned(&ksyms_lock));

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
ksyms_fill_note(void)
{
	int32_t *note = ksyms_hdr.kh_note;
	note[0] = ELF_NOTE_NETBSD_NAMESZ;
	note[1] = ELF_NOTE_NETBSD_DESCSZ;
	note[2] = ELF_NOTE_TYPE_NETBSD_TAG;
	memcpy(&note[3],  "NetBSD\0", 8);
	note[5] = __NetBSD_Version__;
}

static void
ksyms_hdr_init(const void *hdraddr)
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
	ksyms_hdr.kh_ehdr.e_shstrndx = SHSTRTAB;

	/* Text/data - fake */
	ksyms_hdr.kh_phdr[0].p_type = PT_LOAD;
	ksyms_hdr.kh_phdr[0].p_memsz = (unsigned long)-1L;
	ksyms_hdr.kh_phdr[0].p_flags = PF_R | PF_X | PF_W;

#define SHTCOPY(name)  strlcpy(&ksyms_hdr.kh_strtab[offs], (name), \
    sizeof(ksyms_hdr.kh_strtab) - offs), offs += sizeof(name)

	uint32_t offs = 1;
	/* First section header ".note.netbsd.ident" */
	ksyms_hdr.kh_shdr[SHNOTE].sh_name = offs;
	ksyms_hdr.kh_shdr[SHNOTE].sh_type = SHT_NOTE;
	ksyms_hdr.kh_shdr[SHNOTE].sh_offset = 
	    offsetof(struct ksyms_hdr, kh_note[0]);
	ksyms_hdr.kh_shdr[SHNOTE].sh_size = sizeof(ksyms_hdr.kh_note);
	ksyms_hdr.kh_shdr[SHNOTE].sh_addralign = sizeof(int);
	SHTCOPY(".note.netbsd.ident");
	ksyms_fill_note();

	/* Second section header; ".symtab" */
	ksyms_hdr.kh_shdr[SYMTAB].sh_name = offs;
	ksyms_hdr.kh_shdr[SYMTAB].sh_type = SHT_SYMTAB;
	ksyms_hdr.kh_shdr[SYMTAB].sh_offset = sizeof(struct ksyms_hdr);
/*	ksyms_hdr.kh_shdr[SYMTAB].sh_size = filled in at open */
	ksyms_hdr.kh_shdr[SYMTAB].sh_link = STRTAB; /* Corresponding strtab */
	ksyms_hdr.kh_shdr[SYMTAB].sh_addralign = sizeof(long);
	ksyms_hdr.kh_shdr[SYMTAB].sh_entsize = sizeof(Elf_Sym);
	SHTCOPY(".symtab");

	/* Third section header; ".strtab" */
	ksyms_hdr.kh_shdr[STRTAB].sh_name = offs;
	ksyms_hdr.kh_shdr[STRTAB].sh_type = SHT_STRTAB;
/*	ksyms_hdr.kh_shdr[STRTAB].sh_offset = filled in at open */
/*	ksyms_hdr.kh_shdr[STRTAB].sh_size = filled in at open */
	ksyms_hdr.kh_shdr[STRTAB].sh_addralign = sizeof(char);
	SHTCOPY(".strtab");

	/* Fourth section, ".shstrtab" */
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_name = offs;
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_type = SHT_STRTAB;
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_offset =
	    offsetof(struct ksyms_hdr, kh_strtab);
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_size = SHSTRSIZ;
	ksyms_hdr.kh_shdr[SHSTRTAB].sh_addralign = sizeof(char);
	SHTCOPY(".shstrtab");

	/* Fifth section, ".bss". All symbols reside here. */
	ksyms_hdr.kh_shdr[SHBSS].sh_name = offs;
	ksyms_hdr.kh_shdr[SHBSS].sh_type = SHT_NOBITS;
	ksyms_hdr.kh_shdr[SHBSS].sh_offset = 0;
	ksyms_hdr.kh_shdr[SHBSS].sh_size = (unsigned long)-1L;
	ksyms_hdr.kh_shdr[SHBSS].sh_addralign = PAGE_SIZE;
	ksyms_hdr.kh_shdr[SHBSS].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
	SHTCOPY(".bss");

	/* Sixth section header; ".SUNW_ctf" */
	ksyms_hdr.kh_shdr[SHCTF].sh_name = offs;
	ksyms_hdr.kh_shdr[SHCTF].sh_type = SHT_PROGBITS;
/*	ksyms_hdr.kh_shdr[SHCTF].sh_offset = filled in at open */
/*	ksyms_hdr.kh_shdr[SHCTF].sh_size = filled in at open */
	ksyms_hdr.kh_shdr[SHCTF].sh_link = SYMTAB; /* Corresponding symtab */
	ksyms_hdr.kh_shdr[SHCTF].sh_addralign = sizeof(char);
	SHTCOPY(".SUNW_ctf");
}

static struct ksyms_snapshot *
ksyms_snapshot_alloc(int maxlen, size_t size, dev_t dev, uint64_t gen)
{
	struct ksyms_snapshot *ks;

	ks = kmem_zalloc(sizeof(*ks), KM_SLEEP);
	ks->ks_refcnt = 1;
	ks->ks_gen = gen;
	ks->ks_uobj = uao_create(size, 0);
	ks->ks_size = size;
	ks->ks_dev = dev;
	ks->ks_maxlen = maxlen;

	return ks;
}

static void
ksyms_snapshot_release(struct ksyms_snapshot *ks)
{
	uint64_t refcnt;

	mutex_enter(&ksyms_lock);
	refcnt = --ks->ks_refcnt;
	mutex_exit(&ksyms_lock);

	if (refcnt)
		return;

	uao_detach(ks->ks_uobj);
	kmem_free(ks, sizeof(*ks));
}

static int
ubc_copyfrombuf(struct uvm_object *uobj, struct uio *uio, const void *buf,
    size_t n)
{
	struct iovec iov = { .iov_base = __UNCONST(buf), .iov_len = n };

	uio->uio_iov = &iov;
	uio->uio_iovcnt = 1;
	uio->uio_resid = n;

	return ubc_uiomove(uobj, uio, n, UVM_ADV_SEQUENTIAL, UBC_WRITE);
}

static int
ksyms_take_snapshot(struct ksyms_snapshot *ks, struct ksyms_symtab *last)
{
	struct uvm_object *uobj = ks->ks_uobj;
	struct uio uio;
	struct ksyms_symtab *st;
	int error;

	/* Caller must have initiated snapshotting.  */
	KASSERT(ksyms_snapshotting == curlwp);

	/* Start a uio transfer to reuse incrementally.  */
	uio.uio_offset = 0;
	uio.uio_rw = UIO_WRITE; /* write from buffer to uobj */
	UIO_SETUP_SYSSPACE(&uio);

	/*
	 * First: Copy out the ELF header.
	 */
	error = ubc_copyfrombuf(uobj, &uio, &ksyms_hdr, sizeof(ksyms_hdr));
	if (error)
		return error;

	/*
	 * Copy out the symbol table.  The list of symtabs is
	 * guaranteed to be nonempty because we always have an entry
	 * for the main kernel.  We stop at last, not at the end of the
	 * tailq or NULL, because entries beyond last are not included
	 * in this snapshot (and may not be fully initialized memory as
	 * we witness it).
	 */
	KASSERT(uio.uio_offset == sizeof(struct ksyms_hdr));
	for (st = TAILQ_FIRST(&ksyms_symtabs);
	     ;
	     st = TAILQ_NEXT(st, sd_queue)) {
		error = ubc_copyfrombuf(uobj, &uio, st->sd_symstart,
		    st->sd_symsize);
		if (error)
			return error;
		if (st == last)
			break;
	}

	/*
	 * Copy out the string table
	 */
	KASSERT(uio.uio_offset == sizeof(struct ksyms_hdr) +
	    ksyms_hdr.kh_shdr[SYMTAB].sh_size);
	for (st = TAILQ_FIRST(&ksyms_symtabs);
	     ;
	     st = TAILQ_NEXT(st, sd_queue)) {
		error = ubc_copyfrombuf(uobj, &uio, st->sd_strstart,
		    st->sd_strsize);
		if (error)
			return error;
		if (st == last)
			break;
	}

	/*
	 * Copy out the CTF table.
	 */
	KASSERT(uio.uio_offset == sizeof(struct ksyms_hdr) +
	    ksyms_hdr.kh_shdr[SYMTAB].sh_size +
	    ksyms_hdr.kh_shdr[STRTAB].sh_size);
	st = TAILQ_FIRST(&ksyms_symtabs);
	if (st->sd_ctfstart != NULL) {
		error = ubc_copyfrombuf(uobj, &uio, st->sd_ctfstart,
		    st->sd_ctfsize);
		if (error)
			return error;
	}

	KASSERT(uio.uio_offset == sizeof(struct ksyms_hdr) +
	    ksyms_hdr.kh_shdr[SYMTAB].sh_size +
	    ksyms_hdr.kh_shdr[STRTAB].sh_size +
	    ksyms_hdr.kh_shdr[SHCTF].sh_size);
	KASSERT(uio.uio_offset == ks->ks_size);

	return 0;
}

static const struct fileops ksyms_fileops;

static int
ksymsopen(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct file *fp = NULL;
	int fd = -1;
	struct ksyms_snapshot *ks = NULL;
	size_t size;
	struct ksyms_symtab *last;
	int maxlen;
	uint64_t gen;
	int error;

	if (minor(dev) != 0 || !ksyms_loaded)
		return ENXIO;

	/* Allocate a private file.  */
	error = fd_allocfile(&fp, &fd);
	if (error)
		return error;

	mutex_enter(&ksyms_lock);

	/*
	 * Wait until we have a snapshot, or until there is no snapshot
	 * being taken right now so we can take one.
	 */
	while ((ks = ksyms_snapshot) == NULL && ksyms_snapshotting) {
		error = cv_wait_sig(&ksyms_cv, &ksyms_lock);
		if (error)
			goto out;
	}

	/*
	 * If there's a usable snapshot, increment its reference count
	 * (can't overflow, 64-bit) and just reuse it.
	 */
	if (ks) {
		ks->ks_refcnt++;
		goto out;
	}

	/* Find the current length of the symtab object. */
	size = sizeof(struct ksyms_hdr);
	size += ksyms_strsz;
	size += ksyms_symsz;
	size += ksyms_ctfsz;

	/* Start a new snapshot.  */
	ksyms_hdr.kh_shdr[SYMTAB].sh_size = ksyms_symsz;
	ksyms_hdr.kh_shdr[SYMTAB].sh_info = ksyms_symsz / sizeof(Elf_Sym);
	ksyms_hdr.kh_shdr[STRTAB].sh_offset = ksyms_symsz +
	    ksyms_hdr.kh_shdr[SYMTAB].sh_offset;
	ksyms_hdr.kh_shdr[STRTAB].sh_size = ksyms_strsz;
	ksyms_hdr.kh_shdr[SHCTF].sh_offset = ksyms_strsz +
	    ksyms_hdr.kh_shdr[STRTAB].sh_offset;
	ksyms_hdr.kh_shdr[SHCTF].sh_size = ksyms_ctfsz;
	last = TAILQ_LAST(&ksyms_symtabs, ksyms_symtab_queue);
	maxlen = ksyms_maxlen;
	gen = ksyms_snapshot_gen++;

	/*
	 * Prevent ksyms entries from being removed while we take the
	 * snapshot.
	 */
	KASSERT(ksyms_snapshotting == NULL);
	ksyms_snapshotting = curlwp;
	mutex_exit(&ksyms_lock);

	/* Create a snapshot and write the symtab to it.  */
	ks = ksyms_snapshot_alloc(maxlen, size, dev, gen);
	error = ksyms_take_snapshot(ks, last);

	/*
	 * Snapshot creation is done.  Wake up anyone waiting to remove
	 * entries (module unload).
	 */
	mutex_enter(&ksyms_lock);
	KASSERTMSG(ksyms_snapshotting == curlwp, "lwp %p stole snapshot",
	    ksyms_snapshotting);
	ksyms_snapshotting = NULL;
	cv_broadcast(&ksyms_cv);

	/* If we failed, give up.  */
	if (error)
		goto out;

	/* Cache the snapshot for the next reader.  */
	KASSERT(ksyms_snapshot == NULL);
	ksyms_snapshot = ks;
	ks->ks_refcnt++;
	KASSERT(ks->ks_refcnt == 2);

out:	mutex_exit(&ksyms_lock);
	if (error) {
		if (fp)
			fd_abort(curproc, fp, fd);
		if (ks)
			ksyms_snapshot_release(ks);
	} else {
		KASSERT(fp);
		KASSERT(ks);
		error = fd_clone(fp, fd, flags, &ksyms_fileops, ks);
		KASSERTMSG(error == EMOVEFD, "error=%d", error);
	}
	return error;
}

static int
ksymsclose(struct file *fp)
{
	struct ksyms_snapshot *ks = fp->f_data;

	ksyms_snapshot_release(ks);

	return 0;
}

static int
ksymsread(struct file *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	const struct ksyms_snapshot *ks = fp->f_data;
	size_t count;
	int error;

	/*
	 * Since we don't have a per-object lock, we might as well use
	 * the struct file lock to serialize access to fp->f_offset --
	 * but if the caller isn't relying on or updating fp->f_offset,
	 * there's no need to do even that.  We could use ksyms_lock,
	 * but why bother with a global lock if not needed?  Either
	 * way, the lock we use here must agree with what ksymsseek
	 * takes (nothing else in ksyms uses fp->f_offset).
	 */
	if (offp == &fp->f_offset)
		mutex_enter(&fp->f_lock);

	/* Refuse negative offsets.  */
	if (*offp < 0) {
		error = EINVAL;
		goto out;
	}

	/* Return nothing at or past end of file.  */
	if (*offp >= ks->ks_size) {
		error = 0;
		goto out;
	}

	/*
	 * 1. Set up the uio to transfer from offset *offp.
	 * 2. Transfer as many bytes as we can (at most uio->uio_resid
	 *    or what's left in the ksyms).
	 * 3. If requested, update *offp to reflect the number of bytes
	 *    transferred.
	 */
	uio->uio_offset = *offp;
	count = uio->uio_resid;
	error = ubc_uiomove(ks->ks_uobj, uio, MIN(count, ks->ks_size - *offp),
	    UVM_ADV_SEQUENTIAL, UBC_READ|UBC_PARTIALOK);
	if (flags & FOF_UPDATE_OFFSET)
		*offp += count - uio->uio_resid;

out:	if (offp == &fp->f_offset)
		mutex_exit(&fp->f_lock);
	return error;
}

static int
ksymsstat(struct file *fp, struct stat *st)
{
	const struct ksyms_snapshot *ks = fp->f_data;

	memset(st, 0, sizeof(*st));

	st->st_dev = NODEV;
	st->st_ino = 0;
	st->st_mode = S_IFCHR;
	st->st_nlink = 1;
	st->st_uid = kauth_cred_geteuid(fp->f_cred);
	st->st_gid = kauth_cred_getegid(fp->f_cred);
	st->st_rdev = ks->ks_dev;
	st->st_size = ks->ks_size;
	/* zero time */
	st->st_blksize = MAXPHYS; /* XXX arbitrary */
	st->st_blocks = 0;
	st->st_gen = ks->ks_gen;

	return 0;
}

static int
ksymsmmap(struct file *fp, off_t *offp, size_t nbytes, int prot, int *flagsp,
    int *advicep, struct uvm_object **uobjp, int *maxprotp)
{
	const struct ksyms_snapshot *ks = fp->f_data;

	/* uvm_mmap guarantees page-aligned offset and size.  */
	KASSERT(*offp == round_page(*offp));
	KASSERT(nbytes == round_page(nbytes));
	KASSERT(nbytes > 0);

	/* Refuse negative offsets.  */
	if (*offp < 0)
		return EINVAL;

	/* Refuse mappings that pass the end of file.  */
	if (nbytes > round_page(ks->ks_size) ||
	    *offp > round_page(ks->ks_size) - nbytes)
		return EINVAL;	/* XXX ??? */

	/* Success!  */
	uao_reference(ks->ks_uobj);
	*advicep = UVM_ADV_SEQUENTIAL;
	*uobjp = ks->ks_uobj;
	*maxprotp = prot & VM_PROT_READ;
	return 0;
}

static int
ksymsseek(struct file *fp, off_t delta, int whence, off_t *newoffp, int flags)
{
	struct ksyms_snapshot *ks = fp->f_data;
	off_t base, newoff;
	int error;

	mutex_enter(&fp->f_lock);

	switch (whence) {
	case SEEK_CUR:
		base = fp->f_offset;
		break;
	case SEEK_END:
		base = ks->ks_size;
		break;
	case SEEK_SET:
		base = 0;
		break;
	default:
		error = EINVAL;
		goto out;
	}

	/* Compute the new offset and validate it.  */
	newoff = base + delta;	/* XXX arithmetic overflow */
	if (newoff < 0) {
		error = EINVAL;
		goto out;
	}

	/* Success!  */
	if (newoffp)
		*newoffp = newoff;
	if (flags & FOF_UPDATE_OFFSET)
		fp->f_offset = newoff;
	error = 0;

out:	mutex_exit(&fp->f_lock);
	return error;
}

__CTASSERT(offsetof(struct ksyms_ogsymbol, kg_name) == offsetof(struct ksyms_gsymbol, kg_name));
__CTASSERT(offsetof(struct ksyms_gvalue, kv_name) == offsetof(struct ksyms_gsymbol, kg_name));

static int
ksymsioctl(struct file *fp, u_long cmd, void *data)
{
	struct ksyms_snapshot *ks = fp->f_data;
	struct ksyms_ogsymbol *okg = (struct ksyms_ogsymbol *)data;
	struct ksyms_gsymbol *kg = (struct ksyms_gsymbol *)data;
	struct ksyms_gvalue *kv = (struct ksyms_gvalue *)data;
	struct ksyms_symtab *st;
	Elf_Sym *sym = NULL, copy;
	unsigned long val;
	int error = 0;
	char *str = NULL;
	int len, s;

	/* Read cached ksyms_maxlen.  */
	len = ks->ks_maxlen;

	if (cmd == OKIOCGVALUE || cmd == OKIOCGSYMBOL ||
	    cmd == KIOCGVALUE || cmd == KIOCGSYMBOL) {
		str = kmem_alloc(len, KM_SLEEP);
		if ((error = copyinstr(kg->kg_name, str, len, NULL)) != 0) {
			kmem_free(str, len);
			return error;
		}
	}

	switch (cmd) {
	case OKIOCGVALUE:
		/*
		 * Use the in-kernel symbol lookup code for fast
		 * retreival of a value.
		 */
		error = ksyms_getval(NULL, str, &val, KSYMS_EXTERN);
		if (error == 0)
			error = copyout(&val, okg->kg_value, sizeof(long));
		kmem_free(str, len);
		break;

	case OKIOCGSYMBOL:
		/*
		 * Use the in-kernel symbol lookup code for fast
		 * retreival of a symbol.
		 */
		s = pserialize_read_enter();
		PSLIST_READER_FOREACH(st, &ksyms_symtabs_psz,
		    struct ksyms_symtab, sd_pslist) {
			if ((sym = findsym(str, st, KSYMS_ANY)) == NULL)
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
			pserialize_read_exit(s);
			error = copyout(&copy, okg->kg_sym, sizeof(Elf_Sym));
		} else {
			pserialize_read_exit(s);
			error = ENOENT;
		}
		kmem_free(str, len);
		break;

	case KIOCGVALUE:
		/*
		 * Use the in-kernel symbol lookup code for fast
		 * retreival of a value.
		 */
		error = ksyms_getval(NULL, str, &val, KSYMS_EXTERN);
		if (error == 0)
			kv->kv_value = val;
		kmem_free(str, len);
		break;

	case KIOCGSYMBOL:
		/*
		 * Use the in-kernel symbol lookup code for fast
		 * retreival of a symbol.
		 */
		s = pserialize_read_enter();
		PSLIST_READER_FOREACH(st, &ksyms_symtabs_psz,
		    struct ksyms_symtab, sd_pslist) {
			if ((sym = findsym(str, st, KSYMS_ANY)) == NULL)
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
			kg->kg_sym = *sym;
		} else {
			error = ENOENT;
		}
		pserialize_read_exit(s);
		kmem_free(str, len);
		break;

	case KIOCGSIZE:
		/*
		 * Get total size of symbol table.
		 */
		*(int *)data = ks->ks_size;
		break;

	default:
		error = ENOTTY;
		break;
	}

	return error;
}

const struct cdevsw ksyms_cdevsw = {
	.d_open = ksymsopen,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

static const struct fileops ksyms_fileops = {
	.fo_name = "ksyms",
	.fo_read = ksymsread,
	.fo_write = fbadop_write,
	.fo_ioctl = ksymsioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = fnullop_poll,
	.fo_stat = ksymsstat,
	.fo_close = ksymsclose,
	.fo_kqfilter = fnullop_kqfilter,
	.fo_restart = fnullop_restart,
	.fo_mmap = ksymsmmap,
	.fo_seek = ksymsseek,
};
