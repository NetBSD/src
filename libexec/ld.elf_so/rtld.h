/*	$NetBSD: rtld.h,v 1.25.4.1 2000/07/26 23:45:22 mycroft Exp $	 */

/*
 * Copyright 1996 John D. Polstra.
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by John Polstra.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef RTLD_H
#define RTLD_H

#include <dlfcn.h>
#include <stddef.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/exec_elf.h>
#include "rtldenv.h"
#include "link.h"

#if defined(_RTLD_SOURCE)

#define	RTLD_DEFAULT_LIBRARY_PATH	"/usr/lib"
#define _PATH_LD_HINTS			"/etc/ld.so.conf"

#if 0
#define SVR4_LIBDIR	"/usr/lib"
#endif

#define LIBDIRLEN	(sizeof LIBDIR - 1)
#define SVR4_LIBDIRLEN	(sizeof SVR4_LIBDIR - 1)

#ifndef	PAGESIZE
# ifdef VARPSZ
extern int _rtld_pagesz;
#  ifdef RTLD_DEBUG
#   define PAGESIZE	(assert(_rtld_pagesz), _rtld_pagesz)
#  else
#   define PAGESIZE	_rtld_pagesz
#  endif
# else
#  ifndef __sparc__
#   define PAGESIZE	NBPG
#  else
   #error "Sparc has a variable page size"
#  endif
# endif
#endif

#define round_down(x)	((x) & ~(PAGESIZE-1))
#define round_up(x)	round_down((x) + PAGESIZE - 1)

#define NEW(type)	((type *) xmalloc(sizeof(type)))
#define CNEW(type)	((type *) xcalloc(sizeof(type)))

#endif /* _RTLD_SOURCE */

/*
 * C++ has mandated the use of the following keywords for its new boolean
 * type.  We might as well follow their lead.
 */
typedef enum {
	false = 0,
	true = 1
} bool;

struct Struct_Obj_Entry;

typedef struct Struct_Objlist_Entry {
	SIMPLEQ_ENTRY(Struct_Objlist_Entry) link;
	struct Struct_Obj_Entry *obj;
} Objlist_Entry;

typedef SIMPLEQ_HEAD(Struct_Objlist, Struct_Objlist_Entry) Objlist;

typedef struct Struct_Needed_Entry {
	struct Struct_Needed_Entry *next;
	struct Struct_Obj_Entry *obj;
	unsigned long   name;	/* Offset of name in string table */
}               Needed_Entry;

typedef struct _rtld_search_path_t {
	struct _rtld_search_path_t *sp_next;
	const char     *sp_path;
	size_t          sp_pathlen;
}               Search_Path;


#define RTLD_MAX_ENTRY 10
#define RTLD_MAX_LIBRARY 4
#define RTLD_MAX_CTL 2
typedef struct _rtld_library_xform_t {
	struct _rtld_library_xform_t *next;
	char *name;
	int ctl[RTLD_MAX_CTL];
	int ctltype[RTLD_MAX_CTL];
	int ctlmax;
	struct {
		char *value;
		char *library[RTLD_MAX_LIBRARY];
	} entry[RTLD_MAX_ENTRY];
} Library_Xform;

/*
 * Shared object descriptor.
 *
 * Items marked with "(%)" are dynamically allocated, and must be freed
 * when the structure is destroyed.
 */

#define RTLD_MAGIC	0xd550b87a
#define RTLD_VERSION	1

typedef struct Struct_Obj_Entry {
	Elf32_Word      magic;		/* Magic number (sanity check) */
	Elf32_Word      version;	/* Version number of struct format */

	struct Struct_Obj_Entry *next;
	char           *path;		/* Pathname of underlying file (%) */
	int             refcount;
	int             dl_refcount;	/* Number of times loaded by dlopen */

	/* These items are computed by map_object() or by digest_phdr(). */
	caddr_t         mapbase;	/* Base address of mapped region */
	size_t          mapsize;	/* Size of mapped region in bytes */
	size_t          textsize;	/* Size of text segment in bytes */
	Elf_Addr        vaddrbase;	/* Base address in shared object file */
	caddr_t         relocbase;	/* Reloc const = mapbase - *vaddrbase */
	Elf_Dyn        *dynamic;	/* Dynamic section */
	caddr_t         entry;		/* Entry point */
	const Elf_Phdr *phdr;		/* Program header if mapped, ow NULL */
	size_t          phsize;		/* Size of program header in bytes */

	/* Items from the dynamic section. */
	Elf_Addr       *pltgot;		/* PLTGOT table */
	const Elf_Rel  *rel;		/* Relocation entries */
	const Elf_Rel  *rellim;		/* Limit of Relocation entries */
	const Elf_RelA *rela;		/* Relocation entries */
	const Elf_RelA *relalim;	/* Limit of Relocation entries */
	const Elf_Rel  *pltrel;		/* PLT relocation entries */
	const Elf_Rel  *pltrellim;	/* Limit of PLT relocation entries */
	const Elf_RelA *pltrela;	/* PLT relocation entries */
	const Elf_RelA *pltrelalim;	/* Limit of PLT relocation entries */
	const Elf_Sym  *symtab;		/* Symbol table */
	const char     *strtab;		/* String table */
	unsigned long   strsize;	/* Size in bytes of string table */
#if defined(__mips__)
	Elf_Word        local_gotno;	/* Number of local GOT entries */
	Elf_Word        symtabno;	/* Number of dynamic symbols */
	Elf_Word        gotsym;		/* First dynamic symbol in GOT */
#endif

	const Elf_Word *buckets;	/* Hash table buckets array */
	unsigned long   nbuckets;	/* Number of buckets */
	const Elf_Word *chains;		/* Hash table chain array */
	unsigned long   nchains;	/* Number of chains */

	Search_Path    *rpaths;		/* Search path specified in object */
	Needed_Entry   *needed;		/* Shared objects needed by this (%) */

	void            (*init) 	/* Initialization function to call */
	    __P((void));
	void            (*fini)		/* Termination function to call */
	    __P((void));

	/* Entry points for dlopen() and friends. */
	void           *(*dlopen) __P((const char *, int));
	void           *(*dlsym) __P((void *, const char *));
	char           *(*dlerror) __P((void));
	int             (*dlclose) __P((void *));
	int             (*dladdr) __P((const void *, Dl_info *));

	int             mainprog:1;	/* True if this is the main program */
	int             rtld:1;		/* True if this is the dynamic linker */
	int             textrel:1;	/* True if there are relocations to
					 * text seg */
	int             symbolic:1;	/* True if generated with
					 * "-Bsymbolic" */
	int             printed:1;	/* True if ldd has printed it */

	struct link_map linkmap;	/* for GDB */

	/* These items are computed by map_object() or by digest_phdr(). */
	const char     *interp;	/* Pathname of the interpreter, if any */
	Objlist         dldags;	/* Object belongs to these dlopened DAGs (%) */
	Objlist         dagmembers;	/* DAG has these members (%) */
	dev_t           dev;		/* Object's filesystem's device */
	ino_t           ino;		/* Object's inode number */
	unsigned long   mark;		/* Set to "_rtld_curmark" to avoid
					   repeat visits */
} Obj_Entry;

#if defined(_RTLD_SOURCE)

extern struct r_debug _rtld_debug;
extern Search_Path *_rtld_default_paths;
extern Obj_Entry *_rtld_objlist;
extern Obj_Entry **_rtld_objtail;
extern Obj_Entry *_rtld_objmain;
extern Obj_Entry _rtld_objself;
extern Search_Path *_rtld_paths;
extern Library_Xform *_rtld_xforms;
extern bool _rtld_trust;
extern const char *_rtld_error_message;
extern unsigned long _rtld_curmark;
extern Objlist _rtld_list_global;
extern Objlist _rtld_list_main;
extern Elf_Sym _rtld_sym_zero;

/* rtld_start.S */
void _rtld_bind_start __P((void));

/* rtld.c */
void _rtld_error __P((const char *, ...));
void _rtld_die __P((void));
char *_rtld_dlerror __P((void));
void *_rtld_dlopen __P((const char *, int));
void *_rtld_objmain_sym __P((const char *));
void *_rtld_dlsym __P((void *, const char *));
int _rtld_dlclose __P((void *));
int _rtld_dladdr __P((const void *, Dl_info *));
void _rtld_debug_state __P((void));
void _rtld_linkmap_add __P((Obj_Entry *));
void _rtld_linkmap_delete __P((Obj_Entry *));

/* headers.c */
void _rtld_digest_dynamic __P((Obj_Entry *));
Obj_Entry *_rtld_digest_phdr __P((const Elf_Phdr *, int, caddr_t));

/* load.c */
Obj_Entry *_rtld_load_object __P((char *, bool));
int _rtld_load_needed_objects __P((Obj_Entry *, bool));
int _rtld_preload __P((const char *, bool));

/* path.c */
void _rtld_add_paths __P((Search_Path **, const char *, bool));
void _rtld_process_hints __P((Search_Path **, Library_Xform **, const char *,
    bool));

/* reloc.c */
int _rtld_do_copy_relocations __P((const Obj_Entry *, bool));
caddr_t _rtld_bind __P((Obj_Entry *, Elf_Word));
int _rtld_relocate_objects __P((Obj_Entry *, bool, bool));
int _rtld_relocate_nonplt_object __P((Obj_Entry *,
    const Elf_RelA *, bool));
int _rtld_relocate_plt_object __P((Obj_Entry *, const Elf_RelA *,
    caddr_t *, bool, bool));

/* search.c */
char *_rtld_find_library __P((const char *, const Obj_Entry *));

/* symbol.c */
unsigned long _rtld_elf_hash __P((const char *));
const Elf_Sym *_rtld_symlook_obj __P((const char *, unsigned long,
    const Obj_Entry *, bool));
const Elf_Sym *_rtld_find_symdef __P((const Obj_Entry *, Elf_Addr,
    const char *, Obj_Entry *, const Obj_Entry **, bool));
const Elf_Sym *_rtld_symlook_list(const char *, unsigned long,
  Objlist *, const Obj_Entry **, bool in_plt);

/* map_object.c */
Obj_Entry *_rtld_map_object __P((const char *, int, const struct stat *));
void _rtld_obj_free(Obj_Entry *);
Obj_Entry *_rtld_obj_new(void);

#if defined(__mips__)
/* mips_reloc.c */
void _rtld_relocate_mips_got __P((Obj_Entry *));
caddr_t _rtld_bind_mips __P((Elf_Word, Elf_Addr, Elf_Addr, Elf_Addr));
#endif

#if defined(__powerpc__)
/* ppc_reloc.c */
caddr_t _rtld_bind_powerpc __P((Obj_Entry *, Elf_Word));
int _rtld_reloc_powerpc_plt __P((Obj_Entry *, const Elf_RelA *, bool));
void _rtld_setup_powerpc_plt __P((const Obj_Entry *));
#endif

#endif /* _RTLD_SOURCE */

#endif /* RTLD_H */
