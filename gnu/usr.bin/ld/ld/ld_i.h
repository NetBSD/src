/*	$NetBSD: ld_i.h,v 1.1.8.1 2000/12/26 01:20:01 jhawk Exp $	*/
/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*-
 * This code is derived from software copyrighted by the Free Software
 * Foundation.
 *
 * Modified 1991 by Donn Seeley at UUNET Technologies, Inc.
 * 
 * Linker `ld' for GNU
 - Copyright (C) 1988 Free Software Foundation, Inc.

 - This program is free software; you can redistribute it and/or modify
 - it under the terms of the GNU General Public License as published by
 - the Free Software Foundation; either version 1, or (at your option)
 - any later version.

 - This program is distributed in the hope that it will be useful,
 - but WITHOUT ANY WARRANTY; without even the implied warranty of
 - MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 - GNU General Public License for more details.

 - You should have received a copy of the GNU General Public License
 - along with this program; if not, write to the Free Software
 - Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * ld(1) implementation definitions.
 */

#include <ranlib.h>

/* Macro to control the number of undefined references printed */
#define MAX_UREFS_PRINTED	10

/* Define this to specify the default executable format.  */
#ifndef DEFAULT_MAGIC
#ifdef __FreeBSD__
#define DEFAULT_MAGIC QMAGIC
extern int	netzmagic;
#else
#define DEFAULT_MAGIC ZMAGIC
#endif
#endif

/*
 * Internal representation of relocation types
 */
#define RELTYPE_EXTERN		1
#define RELTYPE_JMPSLOT		2
#define RELTYPE_BASEREL		4
#define RELTYPE_RELATIVE	8
#define RELTYPE_COPY		16

/*
 * Local (wrt a compilation unit) symbol information.
 */
typedef struct localsymbol {
	struct nzlist		nzlist;		/* n[z]list from file */
	struct glosym		*symbol;	/* Corresponding global symbol,
						   if any */
	struct localsymbol	*next;		/* List of definitions */
	struct file_entry	*entry;		/* Backpointer to file */
	long			gotslot_offset;	/* Position in GOT, if any */
	int			symbolnum;	/* Position in output nlist */
	int			flags;
#define LS_L_SYMBOL		1	/* Local symbol starts with an `L' */
#define LS_WRITE		2	/* Symbol goes in output symtable */
#define LS_RENAME		4	/* xlat name to `<file>.<name>' */
#define LS_HASGOTSLOT		8	/* This symbol has a GOT entry */
#define LS_WARN			16	/* Second part of a N_WARN duo */
} localsymbol_t;

/* Symbol table */

/*
 * Global symbol data is recorded in these structures, one for each global
 * symbol. They are found via hashing in 'symtab', which points to a vector
 * of buckets. Each bucket is a chain of these structures through the link
 * field.
 */

typedef struct glosym {
	struct glosym	*link;	/* Next symbol hash bucket. */
	char		*name;	/* Name of this symbol.  */
	long		value;	/* Value of this symbol */
	localsymbol_t	*refs;	/* Chain of local symbols from object
				   files pertaining to this global
				   symbol */
	localsymbol_t	*sorefs;/* Same for local symbols from shared
				   object files. */

	char	*warning;	/* message, from N_WARNING nlists */
	int	common_size;	/* Common size */
	int	symbolnum;	/* Symbol index in output symbol table */
	int	rrs_symbolnum;	/* Symbol index in RRS symbol table */

	localsymbol_t	*def_lsp;	/* The local symbol that gave this
					   global symbol its definition */

	char	defined;	/* Definition of this symbol */
	char	so_defined;	/* Definition of this symbol in a shared
				   object. These go into the RRS symbol table */
	u_char	undef_refs;	/* Count of number of "undefined"
				   messages printed for this symbol */
	u_char	mult_defs;	/* Same for "multiply defined" symbols */
	struct glosym	*alias;	/* For symbols of type N_INDR, this
				   points at the real symbol. */
	int	setv_count;	/* Number of elements in N_SETV symbols */
	int	size;		/* Size of this symbol (either from N_SIZE
				   symbols or a from shared object's RRS */
	int	aux;		/* Auxiliary type information conveyed in
				   the `n_other' field of nlists */

	/* The offset into one of the RRS tables, -1 if not used */
	long	jmpslot_offset;
	long	gotslot_offset;

	long			flags;

#define GS_DEFINED		0x1	/* Symbol has definition (notyetused)*/
#define GS_REFERENCED		0x2	/* Symbol is referred to by something
					   interesting */
#define GS_TRACE		0x4	/* Symbol will be traced */
#define GS_HASJMPSLOT		0x8	/*				 */
#define GS_HASGOTSLOT		0x10	/* Some state bits concerning    */
#define GS_CPYRELOCRESERVED	0x20	/* entries in GOT and PLT tables */
#define GS_CPYRELOCCLAIMED	0x40	/*				 */
#define GS_WEAK			0x80	/* Symbol is weakly defined */

} symbol;

/* Number of buckets in symbol hash table */
#define	SYMTABSIZE	1009

/* The symbol hash table: a vector of SYMTABSIZE pointers to struct glosym. */
extern symbol *symtab[];
#define FOR_EACH_SYMBOL(i,sp) {					\
	int i;							\
	for (i = 0; i < SYMTABSIZE; i++) {				\
		register symbol *sp;				\
		for (sp = symtab[i]; sp; sp = sp->link)

#define END_EACH_SYMBOL	}}

/* # of global symbols referenced and not defined.  */
extern int	undefined_global_sym_count;

/* # of weak symbols referenced and not defined.  */
extern int	undefined_weak_sym_count;

/* # of undefined symbols referenced by shared objects */
extern int	undefined_shobj_sym_count;

/* # of multiply defined symbols. */
extern int	multiple_def_count;

/* # of common symbols. */
extern int	common_defined_global_count;

/* # of warning symbols encountered. */
extern int	warn_sym_count;
extern int	list_warning_symbols;

/*
 * Define a linked list of strings which define symbols which should be
 * treated as set elements even though they aren't.  Any symbol with a prefix
 * matching one of these should be treated as a set element.
 * 
 * This is to make up for deficiencies in many assemblers which aren't willing
 * to pass any stabs through to the loader which they don't understand.
 */
struct string_list_element {
	char *str;
	struct string_list_element *next;
};

extern symbol	*entry_symbol;		/* the entry symbol, if any */
extern symbol	*edata_symbol;		/* the symbol _edata */
extern symbol	*etext_symbol;		/* the symbol _etext */
extern symbol	*end_symbol;		/* the symbol _end */
extern symbol	*got_symbol;		/* the symbol __GLOBAL_OFFSET_TABLE_ */
extern symbol	*plt_symbol;		/* the symbol __PROCEDURE_LINKAGE_TABLE_ */
extern symbol	*dynamic_symbol;	/* the symbol __DYNAMIC */

/*
 * Each input file, and each library member ("subfile") being loaded, has a
 * `file_entry' structure for it.
 * 
 * For files specified by command args, these are contained in the vector which
 * `file_table' points to.
 * 
 * For library members, they are dynamically allocated, and chained through the
 * `chain' field. The chain is found in the `subfiles' field of the
 * `file_entry'. The `file_entry' objects for the members have `superfile'
 * fields pointing to the one for the library.
 */

struct file_entry {
	char	*filename;	/* Name of this file.  */
	/*
	 * Name to use for the symbol giving address of text start Usually
	 * the same as filename, but for a file spec'd with -l this is the -l
	 * switch itself rather than the filename.
	 */
	char		*local_sym_name;
	struct exec	header;	/* The file's a.out header.  */
	localsymbol_t	*symbols;	/* Symbol table of the file. */
	int		nsymbols;	/* Number of symbols in above array. */
	int		string_size;	/* Size in bytes of string table. */
	char		*strings;	/* Pointer to the string table when
					   in core, NULL otherwise */
	int		strings_offset;	/* Offset of string table,
					   (normally N_STROFF() + 4) */
	/*
	 * Next two used only if `relocatable_output' or if needed for
	 * output of undefined reference line numbers.
	 */
	struct relocation_info	*textrel;	/* Text relocations */
	int			ntextrel;	/* # of text relocations */
	struct relocation_info	*datarel;	/* Data relocations */
	int			ndatarel;	/* # of data relocations */

	/*
	 * Relation of this file's segments to the output file.
	 */
	int 	text_start_address;	/* Start of this file's text segment
					   in the output file core image. */
	int	data_start_address;	/* Start of this file's data segment
					   in the output file core image. */
	int	bss_start_address;	/* Start of this file's bss segment
					   in the output file core image. */
	struct file_entry *subfiles;	/* For a library, points to chain of
					   entries for the library members. */
	struct file_entry *superfile;	/* For library member, points to the
					   library's own entry.  */
	struct file_entry *chain;	/* For library member, points to next
					   entry for next member.  */
	int	starting_offset;	/* For a library member, offset of the
					   member within the archive. Zero for
					   files that are not library members.*/
	int	total_size;		/* Size of contents of this file,
					   if library member. */
#ifdef SUN_COMPAT
	struct file_entry *silly_archive;/* For shared libraries which have
					    a .sa companion */
#endif
	int	lib_major, lib_minor;	/* Version numbers of a shared object */

	int	flags;
#define E_IS_LIBRARY		0x001	/* File is a an archive */
#define E_HEADER_VALID		0x002	/* File's header has been read */
#define E_SEARCH_DIRS		0x004	/* Search directories for file */
#define E_SEARCH_DYNAMIC	0x008	/* Search for shared libs allowed */
#define E_JUST_SYMS		0x010	/* File is used for incremental load */
#define E_DYNAMIC		0x020	/* File is a shared object */
#define E_SCRAPPED		0x040	/* Ignore this file */
#define E_SYMBOLS_USED		0x080	/* Symbols from this entry were used */
#define E_SECONDCLASS		0x100	/* Shared object is a subsidiary */
#define	E_FORCE_ARCHIVE		0x200	/* Include all library symbols */
};

/*
 * Section start addresses.
 */
extern int	text_size;		/* total size of text. */
extern int	text_start;		/* start of text */
extern int	text_pad;		/* clear space between text and data */
extern int	data_size;		/* total size of data. */
extern int	data_start;		/* start of data */
extern int	data_pad;		/* part of bss segment within data */

extern int	bss_size;		/* total size of bss. */
extern int	bss_start;		/* start of bss */

extern int	text_reloc_size;	/* total size of text relocation. */
extern int	data_reloc_size;	/* total size of data relocation. */

/*
 * Runtime Relocation Section (RRS).
 * This describes the data structures that go into the output text and data
 * segments to support the run-time linker. The RRS can be empty (plain old
 * static linking), or can just exist of GOT and PLT entries (in case of
 * statically linked PIC code).
 */
extern int		rrs_section_type;	/* What's in the RRS section */
#define RRS_NONE	0
#define RRS_PARTIAL	1
#define RRS_FULL	2
extern int		rrs_text_size;		/* Size of RRS text additions */
extern int		rrs_text_start;		/* Location of above */
extern int		rrs_data_size;		/* Size of RRS data additions */
extern int		rrs_data_start;		/* Location of above */
extern char		*rrs_search_paths;	/* `-L' RT paths */

/* Version number to put in __DYNAMIC (set by -V) */
extern int	soversion;
#ifndef DEFAULT_SOVERSION
#define DEFAULT_SOVERSION	LD_VERSION_BSD
#endif

extern int		pc_relocation;		/* Current PC reloc value */

extern int		number_of_shobjs;	/* # of shared objects linked in */

/* Current link mode */
extern int		link_mode;
#define DYNAMIC		1		/* Consider shared libraries */
#define SYMBOLIC	2		/* Force symbolic resolution */
#define FORCEARCHIVE	4		/* Force inclusion of all members
					   of archives */
#define SHAREABLE	8		/* Build a shared object */
#define SILLYARCHIVE	16		/* Process .sa companions, if any */

extern FILE		*outstream;	/* Output file. */
extern struct exec	outheader;	/* Output file header. */
extern int		magic;		/* Output file magic. */
extern int		oldmagic;
extern int		relocatable_output;
extern int		pic_type;
#define PIC_TYPE_NONE	0
#define PIC_TYPE_SMALL	1
#define PIC_TYPE_LARGE	2

/* Size of a page. */
extern int	page_size;

extern int	write_map;	/* write a load map (`-M') */

/* In ld.c: */
void	read_header __P((int, struct file_entry *));
void	read_entry_symbols __P((int, struct file_entry *));
void	read_entry_strings __P((int, struct file_entry *));
void	read_entry_relocation __P((int, struct file_entry *));
void	enter_file_symbols __P((struct file_entry *));
void	read_file_symbols __P((struct file_entry *));
int	set_element_prefixed_p __P((char *));
int	text_offset __P((struct file_entry *));
int	file_open __P((struct file_entry *));
void	each_file __P((void (*) __P((struct file_entry *, void *)), void *));
void	each_full_file __P((void (*)__P((struct file_entry *, void *)), void *));
unsigned long	check_each_file __P((unsigned long (*)__P((struct file_entry *, void *)), void *));
void	mywrite __P((void *, int, int, FILE *));
void	padfile __P((int, FILE *));

/* In warnings.c: */
void	perror_name __P((char *));
void	perror_file __P((struct file_entry *));
void	print_symbols __P((FILE *));
char	*get_file_name __P((struct file_entry *));
void	print_file_name __P((struct file_entry *, FILE *));
void	prline_file_name __P((struct file_entry *, FILE *));
int	do_warnings __P((FILE *));

/* In symbol.c: */
void	symtab_init __P((int));
symbol	*getsym __P((char *)), *getsym_soft __P((char *));

/* In lib.c: */
void	search_library __P((int, struct file_entry *));
void	read_shared_object __P((int, struct file_entry *));
int	findlib __P((struct file_entry *));

/* In rrs.c: */
void	init_rrs __P((void));
int	rrs_add_shobj __P((struct file_entry *));
void	alloc_rrs_reloc __P((struct file_entry *, symbol *));
void	alloc_rrs_segment_reloc __P((struct file_entry *,
				     struct relocation_info  *));
void	alloc_rrs_jmpslot __P((struct file_entry *, symbol *));
void	alloc_rrs_gotslot __P((struct file_entry *, struct relocation_info *,
			       localsymbol_t *));
void	alloc_rrs_cpy_reloc __P((struct file_entry *, symbol *));

int	claim_rrs_reloc __P((struct file_entry *, struct relocation_info *,
			     symbol *, long *));
long	claim_rrs_jmpslot __P((struct file_entry *, struct relocation_info *,
			       symbol *, long));
long	claim_rrs_gotslot __P((struct file_entry *, struct relocation_info *,
			       struct localsymbol *, long));
long	claim_rrs_internal_gotslot __P((struct file_entry *,
					struct relocation_info *,
					struct localsymbol *, long));
void	claim_rrs_cpy_reloc __P((struct file_entry *,
				 struct relocation_info *, symbol *));
void	claim_rrs_segment_reloc __P((struct file_entry *,
				     struct relocation_info *));
void	consider_rrs_section_lengths __P((void));
void	relocate_rrs_addresses __P((void));
void	write_rrs __P((void));

void	swap_longs	__P((long *, int));

void	md_swapin_exec_hdr	__P((struct exec *));
void	md_swapout_exec_hdr	__P((struct exec *));
void	md_swapin_reloc		__P((struct relocation_info *, int));
void	md_swapout_reloc	__P((struct relocation_info *, int));
void	md_swapout_jmpslot	__P((jmpslot_t *, int));

/* In xbits.c: */
void	swap_symbols	__P((struct nlist *, int));
void	swap_zsymbols	__P((struct nzlist *, int));
void	swap_ranlib_hdr	__P((struct ranlib *, int));
void	swap__dynamic	__P((struct _dynamic *));
void	swap_section_dispatch_table __P((struct section_dispatch_table *));
void	swap_so_debug	__P((struct so_debug *));
void	swapin_sod	__P((struct sod *, int));
void	swapout_sod	__P((struct sod *, int));
void	swap_rrs_hash	__P((struct rrs_hash *, int));
/*void	swapout_fshash	__P((struct fshash *, int));*/
