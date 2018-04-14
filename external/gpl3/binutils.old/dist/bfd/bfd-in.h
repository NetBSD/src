/* Main header file for the bfd library -- portable access to object files.

   Copyright (C) 1990-2016 Free Software Foundation, Inc.

   Contributed by Cygnus Support.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef __BFD_H_SEEN__
#define __BFD_H_SEEN__

/* PR 14072: Ensure that config.h is included first.  */
#if !defined PACKAGE && !defined PACKAGE_VERSION
#error config.h must be included before this header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ansidecl.h"
#include "symcat.h"
#include <sys/stat.h>

#if defined (__STDC__) || defined (ALMOST_STDC) || defined (HAVE_STRINGIZE)
#ifndef SABER
/* This hack is to avoid a problem with some strict ANSI C preprocessors.
   The problem is, "32_" is not a valid preprocessing token, and we don't
   want extra underscores (e.g., "nlm_32_").  The XCONCAT2 macro will
   cause the inner CONCAT2 macros to be evaluated first, producing
   still-valid pp-tokens.  Then the final concatenation can be done.  */
#undef CONCAT4
#define CONCAT4(a,b,c,d) XCONCAT2(CONCAT2(a,b),CONCAT2(c,d))
#endif
#endif

/* This is a utility macro to handle the situation where the code
   wants to place a constant string into the code, followed by a
   comma and then the length of the string.  Doing this by hand
   is error prone, so using this macro is safer.  */
#define STRING_COMMA_LEN(STR) (STR), (sizeof (STR) - 1)
/* Unfortunately it is not possible to use the STRING_COMMA_LEN macro
   to create the arguments to another macro, since the preprocessor
   will mis-count the number of arguments to the outer macro (by not
   evaluating STRING_COMMA_LEN and so missing the comma).  This is a
   problem for example when trying to use STRING_COMMA_LEN to build
   the arguments to the strncmp() macro.  Hence this alternative
   definition of strncmp is provided here.

   Note - these macros do NOT work if STR2 is not a constant string.  */
#define CONST_STRNEQ(STR1,STR2) (strncmp ((STR1), (STR2), sizeof (STR2) - 1) == 0)
  /* strcpy() can have a similar problem, but since we know we are
     copying a constant string, we can use memcpy which will be faster
     since there is no need to check for a NUL byte inside STR.  We
     can also save time if we do not need to copy the terminating NUL.  */
#define LITMEMCPY(DEST,STR2) memcpy ((DEST), (STR2), sizeof (STR2) - 1)
#define LITSTRCPY(DEST,STR2) memcpy ((DEST), (STR2), sizeof (STR2))


#define BFD_SUPPORTS_PLUGINS @supports_plugins@

/* The word size used by BFD on the host.  This may be 64 with a 32
   bit target if the host is 64 bit, or if other 64 bit targets have
   been selected with --enable-targets, or if --enable-64-bit-bfd.  */
#define BFD_ARCH_SIZE @wordsize@

/* The word size of the default bfd target.  */
#define BFD_DEFAULT_TARGET_SIZE @bfd_default_target_size@

#define BFD_HOST_64BIT_LONG @BFD_HOST_64BIT_LONG@
#define BFD_HOST_64BIT_LONG_LONG @BFD_HOST_64BIT_LONG_LONG@
#if @BFD_HOST_64_BIT_DEFINED@
#define BFD_HOST_64_BIT @BFD_HOST_64_BIT@
#define BFD_HOST_U_64_BIT @BFD_HOST_U_64_BIT@
typedef BFD_HOST_64_BIT bfd_int64_t;
typedef BFD_HOST_U_64_BIT bfd_uint64_t;
#endif

#if BFD_ARCH_SIZE >= 64
#define BFD64
#endif

#ifndef INLINE
#if __GNUC__ >= 2
#define INLINE __inline__
#else
#define INLINE
#endif
#endif

/* Declaring a type wide enough to hold a host long and a host pointer.  */
#define BFD_HOSTPTR_T	@BFD_HOSTPTR_T@
typedef BFD_HOSTPTR_T bfd_hostptr_t;

/* Forward declaration.  */
typedef struct bfd bfd;

/* Boolean type used in bfd.  Too many systems define their own
   versions of "boolean" for us to safely typedef a "boolean" of
   our own.  Using an enum for "bfd_boolean" has its own set of
   problems, with strange looking casts required to avoid warnings
   on some older compilers.  Thus we just use an int.

   General rule: Functions which are bfd_boolean return TRUE on
   success and FALSE on failure (unless they're a predicate).  */

typedef int bfd_boolean;
#undef FALSE
#undef TRUE
#define FALSE 0
#define TRUE 1

#ifdef BFD64

#ifndef BFD_HOST_64_BIT
 #error No 64 bit integer type available
#endif /* ! defined (BFD_HOST_64_BIT) */

typedef BFD_HOST_U_64_BIT bfd_vma;
typedef BFD_HOST_64_BIT bfd_signed_vma;
typedef BFD_HOST_U_64_BIT bfd_size_type;
typedef BFD_HOST_U_64_BIT symvalue;

#if BFD_HOST_64BIT_LONG
#define BFD_VMA_FMT "l"
#elif defined (__MSVCRT__)
#define BFD_VMA_FMT "I64"
#else
#define BFD_VMA_FMT "ll"
#endif

#ifndef fprintf_vma
#define sprintf_vma(s,x) sprintf (s, "%016" BFD_VMA_FMT "x", x)
#define fprintf_vma(f,x) fprintf (f, "%016" BFD_VMA_FMT "x", x)
#endif

#else /* not BFD64  */

/* Represent a target address.  Also used as a generic unsigned type
   which is guaranteed to be big enough to hold any arithmetic types
   we need to deal with.  */
typedef unsigned long bfd_vma;

/* A generic signed type which is guaranteed to be big enough to hold any
   arithmetic types we need to deal with.  Can be assumed to be compatible
   with bfd_vma in the same way that signed and unsigned ints are compatible
   (as parameters, in assignment, etc).  */
typedef long bfd_signed_vma;

typedef unsigned long symvalue;
typedef unsigned long bfd_size_type;

/* Print a bfd_vma x on stream s.  */
#define BFD_VMA_FMT "l"
#define fprintf_vma(s,x) fprintf (s, "%08" BFD_VMA_FMT "x", x)
#define sprintf_vma(s,x) sprintf (s, "%08" BFD_VMA_FMT "x", x)

#endif /* not BFD64  */

#define HALF_BFD_SIZE_TYPE \
  (((bfd_size_type) 1) << (8 * sizeof (bfd_size_type) / 2))

#ifndef BFD_HOST_64_BIT
/* Fall back on a 32 bit type.  The idea is to make these types always
   available for function return types, but in the case that
   BFD_HOST_64_BIT is undefined such a function should abort or
   otherwise signal an error.  */
typedef bfd_signed_vma bfd_int64_t;
typedef bfd_vma bfd_uint64_t;
#endif

/* An offset into a file.  BFD always uses the largest possible offset
   based on the build time availability of fseek, fseeko, or fseeko64.  */
typedef @bfd_file_ptr@ file_ptr;
typedef unsigned @bfd_file_ptr@ ufile_ptr;

extern void bfd_sprintf_vma (bfd *, char *, bfd_vma);
extern void bfd_fprintf_vma (bfd *, void *, bfd_vma);

#define printf_vma(x) fprintf_vma(stdout,x)
#define bfd_printf_vma(abfd,x) bfd_fprintf_vma (abfd,stdout,x)

typedef unsigned int flagword;	/* 32 bits of flags */
typedef unsigned char bfd_byte;

/* File formats.  */

typedef enum bfd_format
{
  bfd_unknown = 0,	/* File format is unknown.  */
  bfd_object,		/* Linker/assembler/compiler output.  */
  bfd_archive,		/* Object archive file.  */
  bfd_core,		/* Core dump.  */
  bfd_type_end		/* Marks the end; don't use it!  */
}
bfd_format;

/* Symbols and relocation.  */

/* A count of carsyms (canonical archive symbols).  */
typedef unsigned long symindex;

/* How to perform a relocation.  */
typedef const struct reloc_howto_struct reloc_howto_type;

#define BFD_NO_MORE_SYMBOLS ((symindex) ~0)

/* General purpose part of a symbol X;
   target specific parts are in libcoff.h, libaout.h, etc.  */

#define bfd_get_section(x) ((x)->section)
#define bfd_get_output_section(x) ((x)->section->output_section)
#define bfd_set_section(x,y) ((x)->section) = (y)
#define bfd_asymbol_base(x) ((x)->section->vma)
#define bfd_asymbol_value(x) (bfd_asymbol_base(x) + (x)->value)
#define bfd_asymbol_name(x) ((x)->name)
/*Perhaps future: #define bfd_asymbol_bfd(x) ((x)->section->owner)*/
#define bfd_asymbol_bfd(x) ((x)->the_bfd)
#define bfd_asymbol_flavour(x)			\
  (((x)->flags & BSF_SYNTHETIC) != 0		\
   ? bfd_target_unknown_flavour			\
   : bfd_asymbol_bfd (x)->xvec->flavour)

/* A canonical archive symbol.  */
/* This is a type pun with struct ranlib on purpose!  */
typedef struct carsym
{
  char *name;
  file_ptr file_offset;	/* Look here to find the file.  */
}
carsym;			/* To make these you call a carsymogen.  */

/* Used in generating armaps (archive tables of contents).
   Perhaps just a forward definition would do?  */
struct orl 			/* Output ranlib.  */
{
  char **name;		/* Symbol name.  */
  union
  {
    file_ptr pos;
    bfd *abfd;
  } u;			/* bfd* or file position.  */
  int namidx;		/* Index into string table.  */
};

/* Linenumber stuff.  */
typedef struct lineno_cache_entry
{
  unsigned int line_number;	/* Linenumber from start of function.  */
  union
  {
    struct bfd_symbol *sym;	/* Function name.  */
    bfd_vma offset;	    		/* Offset into section.  */
  } u;
}
alent;

/* Object and core file sections.  */

#define	align_power(addr, align)	\
  (((addr) + ((bfd_vma) 1 << (align)) - 1) & (-((bfd_vma) 1 << (align))))

typedef struct bfd_section *sec_ptr;

#define bfd_get_section_name(bfd, ptr) ((void) bfd, (ptr)->name)
#define bfd_get_section_vma(bfd, ptr) ((void) bfd, (ptr)->vma)
#define bfd_get_section_lma(bfd, ptr) ((void) bfd, (ptr)->lma)
#define bfd_get_section_alignment(bfd, ptr) ((void) bfd, \
					     (ptr)->alignment_power)
#define bfd_section_name(bfd, ptr) ((ptr)->name)
#define bfd_section_size(bfd, ptr) ((ptr)->size)
#define bfd_get_section_size(ptr) ((ptr)->size)
#define bfd_section_vma(bfd, ptr) ((ptr)->vma)
#define bfd_section_lma(bfd, ptr) ((ptr)->lma)
#define bfd_section_alignment(bfd, ptr) ((ptr)->alignment_power)
#define bfd_get_section_flags(bfd, ptr) ((void) bfd, (ptr)->flags)
#define bfd_get_section_userdata(bfd, ptr) ((void) bfd, (ptr)->userdata)

#define bfd_is_com_section(ptr) (((ptr)->flags & SEC_IS_COMMON) != 0)

#define bfd_get_section_limit_octets(bfd, sec)			\
  ((bfd)->direction != write_direction && (sec)->rawsize != 0	\
   ? (sec)->rawsize : (sec)->size)

/* Find the address one past the end of SEC.  */
#define bfd_get_section_limit(bfd, sec) \
  (bfd_get_section_limit_octets(bfd, sec) / bfd_octets_per_byte (bfd))

/* Return TRUE if input section SEC has been discarded.  */
#define discarded_section(sec)				\
  (!bfd_is_abs_section (sec)					\
   && bfd_is_abs_section ((sec)->output_section)		\
   && (sec)->sec_info_type != SEC_INFO_TYPE_MERGE		\
   && (sec)->sec_info_type != SEC_INFO_TYPE_JUST_SYMS)

typedef enum bfd_print_symbol
{
  bfd_print_symbol_name,
  bfd_print_symbol_more,
  bfd_print_symbol_all
} bfd_print_symbol_type;

/* Information about a symbol that nm needs.  */

typedef struct _symbol_info
{
  symvalue value;
  char type;
  const char *name;            /* Symbol name.  */
  unsigned char stab_type;     /* Stab type.  */
  char stab_other;             /* Stab other.  */
  short stab_desc;             /* Stab desc.  */
  const char *stab_name;       /* String for stab type.  */
} symbol_info;

/* Get the name of a stabs type code.  */

extern const char *bfd_get_stab_name (int);

/* Hash table routines.  There is no way to free up a hash table.  */

/* An element in the hash table.  Most uses will actually use a larger
   structure, and an instance of this will be the first field.  */

struct bfd_hash_entry
{
  /* Next entry for this hash code.  */
  struct bfd_hash_entry *next;
  /* String being hashed.  */
  const char *string;
  /* Hash code.  This is the full hash code, not the index into the
     table.  */
  unsigned long hash;
};

/* A hash table.  */

struct bfd_hash_table
{
  /* The hash array.  */
  struct bfd_hash_entry **table;
  /* A function used to create new elements in the hash table.  The
     first entry is itself a pointer to an element.  When this
     function is first invoked, this pointer will be NULL.  However,
     having the pointer permits a hierarchy of method functions to be
     built each of which calls the function in the superclass.  Thus
     each function should be written to allocate a new block of memory
     only if the argument is NULL.  */
  struct bfd_hash_entry *(*newfunc)
    (struct bfd_hash_entry *, struct bfd_hash_table *, const char *);
   /* An objalloc for this hash table.  This is a struct objalloc *,
     but we use void * to avoid requiring the inclusion of objalloc.h.  */
  void *memory;
  /* The number of slots in the hash table.  */
  unsigned int size;
  /* The number of entries in the hash table.  */
  unsigned int count;
  /* The size of elements.  */
  unsigned int entsize;
  /* If non-zero, don't grow the hash table.  */
  unsigned int frozen:1;
};

/* Initialize a hash table.  */
extern bfd_boolean bfd_hash_table_init
  (struct bfd_hash_table *,
   struct bfd_hash_entry *(*) (struct bfd_hash_entry *,
			       struct bfd_hash_table *,
			       const char *),
   unsigned int);

/* Initialize a hash table specifying a size.  */
extern bfd_boolean bfd_hash_table_init_n
  (struct bfd_hash_table *,
   struct bfd_hash_entry *(*) (struct bfd_hash_entry *,
			       struct bfd_hash_table *,
			       const char *),
   unsigned int, unsigned int);

/* Free up a hash table.  */
extern void bfd_hash_table_free
  (struct bfd_hash_table *);

/* Look up a string in a hash table.  If CREATE is TRUE, a new entry
   will be created for this string if one does not already exist.  The
   COPY argument must be TRUE if this routine should copy the string
   into newly allocated memory when adding an entry.  */
extern struct bfd_hash_entry *bfd_hash_lookup
  (struct bfd_hash_table *, const char *, bfd_boolean create,
   bfd_boolean copy);

/* Insert an entry in a hash table.  */
extern struct bfd_hash_entry *bfd_hash_insert
  (struct bfd_hash_table *, const char *, unsigned long);

/* Rename an entry in a hash table.  */
extern void bfd_hash_rename
  (struct bfd_hash_table *, const char *, struct bfd_hash_entry *);

/* Replace an entry in a hash table.  */
extern void bfd_hash_replace
  (struct bfd_hash_table *, struct bfd_hash_entry *old,
   struct bfd_hash_entry *nw);

/* Base method for creating a hash table entry.  */
extern struct bfd_hash_entry *bfd_hash_newfunc
  (struct bfd_hash_entry *, struct bfd_hash_table *, const char *);

/* Grab some space for a hash table entry.  */
extern void *bfd_hash_allocate
  (struct bfd_hash_table *, unsigned int);

/* Traverse a hash table in a random order, calling a function on each
   element.  If the function returns FALSE, the traversal stops.  The
   INFO argument is passed to the function.  */
extern void bfd_hash_traverse
  (struct bfd_hash_table *,
   bfd_boolean (*) (struct bfd_hash_entry *, void *),
   void *info);

/* Allows the default size of a hash table to be configured. New hash
   tables allocated using bfd_hash_table_init will be created with
   this size.  */
extern unsigned long bfd_hash_set_default_size (unsigned long);

/* Types of compressed DWARF debug sections.  We currently support
   zlib.  */
enum compressed_debug_section_type
{
  COMPRESS_DEBUG_NONE = 0,
  COMPRESS_DEBUG = 1 << 0,
  COMPRESS_DEBUG_GNU_ZLIB = COMPRESS_DEBUG | 1 << 1,
  COMPRESS_DEBUG_GABI_ZLIB = COMPRESS_DEBUG | 1 << 2
};

/* This structure is used to keep track of stabs in sections
   information while linking.  */

struct stab_info
{
  /* A hash table used to hold stabs strings.  */
  struct bfd_strtab_hash *strings;
  /* The header file hash table.  */
  struct bfd_hash_table includes;
  /* The first .stabstr section.  */
  struct bfd_section *stabstr;
};

#define COFF_SWAP_TABLE (void *) &bfd_coff_std_swap_table

/* User program access to BFD facilities.  */

/* Direct I/O routines, for programs which know more about the object
   file than BFD does.  Use higher level routines if possible.  */

extern bfd_size_type bfd_bread (void *, bfd_size_type, bfd *);
extern bfd_size_type bfd_bwrite (const void *, bfd_size_type, bfd *);
extern int bfd_seek (bfd *, file_ptr, int);
extern file_ptr bfd_tell (bfd *);
extern int bfd_flush (bfd *);
extern int bfd_stat (bfd *, struct stat *);

/* Deprecated old routines.  */
#if __GNUC__
#define bfd_read(BUF, ELTSIZE, NITEMS, ABFD)				\
  (warn_deprecated ("bfd_read", __FILE__, __LINE__, __FUNCTION__),	\
   bfd_bread ((BUF), (ELTSIZE) * (NITEMS), (ABFD)))
#define bfd_write(BUF, ELTSIZE, NITEMS, ABFD)				\
  (warn_deprecated ("bfd_write", __FILE__, __LINE__, __FUNCTION__),	\
   bfd_bwrite ((BUF), (ELTSIZE) * (NITEMS), (ABFD)))
#else
#define bfd_read(BUF, ELTSIZE, NITEMS, ABFD)				\
  (warn_deprecated ("bfd_read", (const char *) 0, 0, (const char *) 0), \
   bfd_bread ((BUF), (ELTSIZE) * (NITEMS), (ABFD)))
#define bfd_write(BUF, ELTSIZE, NITEMS, ABFD)				\
  (warn_deprecated ("bfd_write", (const char *) 0, 0, (const char *) 0),\
   bfd_bwrite ((BUF), (ELTSIZE) * (NITEMS), (ABFD)))
#endif
extern void warn_deprecated (const char *, const char *, int, const char *);

/* Cast from const char * to char * so that caller can assign to
   a char * without a warning.  */
#define bfd_get_filename(abfd) ((char *) (abfd)->filename)
#define bfd_get_cacheable(abfd) ((abfd)->cacheable)
#define bfd_get_format(abfd) ((abfd)->format)
#define bfd_get_target(abfd) ((abfd)->xvec->name)
#define bfd_get_flavour(abfd) ((abfd)->xvec->flavour)
#define bfd_family_coff(abfd) \
  (bfd_get_flavour (abfd) == bfd_target_coff_flavour || \
   bfd_get_flavour (abfd) == bfd_target_xcoff_flavour)
#define bfd_big_endian(abfd) ((abfd)->xvec->byteorder == BFD_ENDIAN_BIG)
#define bfd_little_endian(abfd) ((abfd)->xvec->byteorder == BFD_ENDIAN_LITTLE)
#define bfd_header_big_endian(abfd) \
  ((abfd)->xvec->header_byteorder == BFD_ENDIAN_BIG)
#define bfd_header_little_endian(abfd) \
  ((abfd)->xvec->header_byteorder == BFD_ENDIAN_LITTLE)
#define bfd_get_file_flags(abfd) ((abfd)->flags)
#define bfd_applicable_file_flags(abfd) ((abfd)->xvec->object_flags)
#define bfd_applicable_section_flags(abfd) ((abfd)->xvec->section_flags)
#define bfd_has_map(abfd) ((abfd)->has_armap)
#define bfd_is_thin_archive(abfd) ((abfd)->is_thin_archive)

#define bfd_valid_reloc_types(abfd) ((abfd)->xvec->valid_reloc_types)
#define bfd_usrdata(abfd) ((abfd)->usrdata)

#define bfd_get_start_address(abfd) ((abfd)->start_address)
#define bfd_get_symcount(abfd) ((abfd)->symcount)
#define bfd_get_outsymbols(abfd) ((abfd)->outsymbols)
#define bfd_count_sections(abfd) ((abfd)->section_count)

#define bfd_get_dynamic_symcount(abfd) ((abfd)->dynsymcount)

#define bfd_get_symbol_leading_char(abfd) ((abfd)->xvec->symbol_leading_char)

extern bfd_boolean bfd_cache_close
  (bfd *abfd);
/* NB: This declaration should match the autogenerated one in libbfd.h.  */

extern bfd_boolean bfd_cache_close_all (void);

extern bfd_boolean bfd_record_phdr
  (bfd *, unsigned long, bfd_boolean, flagword, bfd_boolean, bfd_vma,
   bfd_boolean, bfd_boolean, unsigned int, struct bfd_section **);

/* Byte swapping routines.  */

bfd_uint64_t bfd_getb64 (const void *);
bfd_uint64_t bfd_getl64 (const void *);
bfd_int64_t bfd_getb_signed_64 (const void *);
bfd_int64_t bfd_getl_signed_64 (const void *);
bfd_vma bfd_getb32 (const void *);
bfd_vma bfd_getl32 (const void *);
bfd_signed_vma bfd_getb_signed_32 (const void *);
bfd_signed_vma bfd_getl_signed_32 (const void *);
bfd_vma bfd_getb16 (const void *);
bfd_vma bfd_getl16 (const void *);
bfd_signed_vma bfd_getb_signed_16 (const void *);
bfd_signed_vma bfd_getl_signed_16 (const void *);
void bfd_putb64 (bfd_uint64_t, void *);
void bfd_putl64 (bfd_uint64_t, void *);
void bfd_putb32 (bfd_vma, void *);
void bfd_putl32 (bfd_vma, void *);
void bfd_putb16 (bfd_vma, void *);
void bfd_putl16 (bfd_vma, void *);

/* Byte swapping routines which take size and endiannes as arguments.  */

bfd_uint64_t bfd_get_bits (const void *, int, bfd_boolean);
void bfd_put_bits (bfd_uint64_t, void *, int, bfd_boolean);

#if defined(__STDC__) || defined(ALMOST_STDC)
struct ecoff_debug_info;
struct ecoff_debug_swap;
struct ecoff_extr;
struct bfd_symbol;
struct bfd_link_info;
struct bfd_link_hash_entry;
struct bfd_section_already_linked;
struct bfd_elf_version_tree;
#endif

extern bfd_boolean bfd_section_already_linked_table_init (void);
extern void bfd_section_already_linked_table_free (void);
extern bfd_boolean _bfd_handle_already_linked
  (struct bfd_section *, struct bfd_section_already_linked *,
   struct bfd_link_info *);

/* Externally visible ECOFF routines.  */

extern bfd_vma bfd_ecoff_get_gp_value
  (bfd * abfd);
extern bfd_boolean bfd_ecoff_set_gp_value
  (bfd *abfd, bfd_vma gp_value);
extern bfd_boolean bfd_ecoff_set_regmasks
  (bfd *abfd, unsigned long gprmask, unsigned long fprmask,
   unsigned long *cprmask);
extern void *bfd_ecoff_debug_init
  (bfd *output_bfd, struct ecoff_debug_info *output_debug,
   const struct ecoff_debug_swap *output_swap, struct bfd_link_info *);
extern void bfd_ecoff_debug_free
  (void *handle, bfd *output_bfd, struct ecoff_debug_info *output_debug,
   const struct ecoff_debug_swap *output_swap, struct bfd_link_info *);
extern bfd_boolean bfd_ecoff_debug_accumulate
  (void *handle, bfd *output_bfd, struct ecoff_debug_info *output_debug,
   const struct ecoff_debug_swap *output_swap, bfd *input_bfd,
   struct ecoff_debug_info *input_debug,
   const struct ecoff_debug_swap *input_swap, struct bfd_link_info *);
extern bfd_boolean bfd_ecoff_debug_accumulate_other
  (void *handle, bfd *output_bfd, struct ecoff_debug_info *output_debug,
   const struct ecoff_debug_swap *output_swap, bfd *input_bfd,
   struct bfd_link_info *);
extern bfd_boolean bfd_ecoff_debug_externals
  (bfd *abfd, struct ecoff_debug_info *debug,
   const struct ecoff_debug_swap *swap, bfd_boolean relocatable,
   bfd_boolean (*get_extr) (struct bfd_symbol *, struct ecoff_extr *),
   void (*set_index) (struct bfd_symbol *, bfd_size_type));
extern bfd_boolean bfd_ecoff_debug_one_external
  (bfd *abfd, struct ecoff_debug_info *debug,
   const struct ecoff_debug_swap *swap, const char *name,
   struct ecoff_extr *esym);
extern bfd_size_type bfd_ecoff_debug_size
  (bfd *abfd, struct ecoff_debug_info *debug,
   const struct ecoff_debug_swap *swap);
extern bfd_boolean bfd_ecoff_write_debug
  (bfd *abfd, struct ecoff_debug_info *debug,
   const struct ecoff_debug_swap *swap, file_ptr where);
extern bfd_boolean bfd_ecoff_write_accumulated_debug
  (void *handle, bfd *abfd, struct ecoff_debug_info *debug,
   const struct ecoff_debug_swap *swap,
   struct bfd_link_info *info, file_ptr where);

/* Externally visible ELF routines.  */

struct bfd_link_needed_list
{
  struct bfd_link_needed_list *next;
  bfd *by;
  const char *name;
};

enum dynamic_lib_link_class {
  DYN_NORMAL = 0,
  DYN_AS_NEEDED = 1,
  DYN_DT_NEEDED = 2,
  DYN_NO_ADD_NEEDED = 4,
  DYN_NO_NEEDED = 8
};

enum notice_asneeded_action {
  notice_as_needed,
  notice_not_needed,
  notice_needed
};

extern bfd_boolean bfd_elf_record_link_assignment
  (bfd *, struct bfd_link_info *, const char *, bfd_boolean,
   bfd_boolean);
extern struct bfd_link_needed_list *bfd_elf_get_needed_list
  (bfd *, struct bfd_link_info *);
extern bfd_boolean bfd_elf_get_bfd_needed_list
  (bfd *, struct bfd_link_needed_list **);
extern bfd_boolean bfd_elf_stack_segment_size (bfd *, struct bfd_link_info *,
					       const char *, bfd_vma);
extern bfd_boolean bfd_elf_size_dynamic_sections
  (bfd *, const char *, const char *, const char *, const char *, const char *,
   const char * const *, struct bfd_link_info *, struct bfd_section **);
extern bfd_boolean bfd_elf_size_dynsym_hash_dynstr
  (bfd *, struct bfd_link_info *);
extern void bfd_elf_set_dt_needed_name
  (bfd *, const char *);
extern const char *bfd_elf_get_dt_soname
  (bfd *);
extern void bfd_elf_set_dyn_lib_class
  (bfd *, enum dynamic_lib_link_class);
extern int bfd_elf_get_dyn_lib_class
  (bfd *);
extern struct bfd_link_needed_list *bfd_elf_get_runpath_list
  (bfd *, struct bfd_link_info *);
extern int bfd_elf_discard_info
  (bfd *, struct bfd_link_info *);
extern unsigned int _bfd_elf_default_action_discarded
  (struct bfd_section *);

/* Return an upper bound on the number of bytes required to store a
   copy of ABFD's program header table entries.  Return -1 if an error
   occurs; bfd_get_error will return an appropriate code.  */
extern long bfd_get_elf_phdr_upper_bound
  (bfd *abfd);

/* Copy ABFD's program header table entries to *PHDRS.  The entries
   will be stored as an array of Elf_Internal_Phdr structures, as
   defined in include/elf/internal.h.  To find out how large the
   buffer needs to be, call bfd_get_elf_phdr_upper_bound.

   Return the number of program header table entries read, or -1 if an
   error occurs; bfd_get_error will return an appropriate code.  */
extern int bfd_get_elf_phdrs
  (bfd *abfd, void *phdrs);

/* Create a new BFD as if by bfd_openr.  Rather than opening a file,
   reconstruct an ELF file by reading the segments out of remote
   memory based on the ELF file header at EHDR_VMA and the ELF program
   headers it points to.  If non-zero, SIZE is the known extent of the
   object.  If not null, *LOADBASEP is filled in with the difference
   between the VMAs from which the segments were read, and the VMAs
   the file headers (and hence BFD's idea of each section's VMA) put
   them at.

   The function TARGET_READ_MEMORY is called to copy LEN bytes from
   the remote memory at target address VMA into the local buffer at
   MYADDR; it should return zero on success or an `errno' code on
   failure.  TEMPL must be a BFD for a target with the word size and
   byte order found in the remote memory.  */
extern bfd *bfd_elf_bfd_from_remote_memory
  (bfd *templ, bfd_vma ehdr_vma, bfd_size_type size, bfd_vma *loadbasep,
   int (*target_read_memory) (bfd_vma vma, bfd_byte *myaddr,
			      bfd_size_type len));

extern struct bfd_section *_bfd_elf_tls_setup
  (bfd *, struct bfd_link_info *);

extern struct bfd_section *
_bfd_nearby_section (bfd *, struct bfd_section *, bfd_vma);

extern void _bfd_fix_excluded_sec_syms
  (bfd *, struct bfd_link_info *);

extern unsigned bfd_m68k_mach_to_features (int);

extern int bfd_m68k_features_to_mach (unsigned);

extern bfd_boolean bfd_m68k_elf32_create_embedded_relocs
  (bfd *, struct bfd_link_info *, struct bfd_section *, struct bfd_section *,
   char **);

extern void bfd_elf_m68k_set_target_options (struct bfd_link_info *, int);

extern bfd_boolean bfd_bfin_elf32_create_embedded_relocs
  (bfd *, struct bfd_link_info *, struct bfd_section *, struct bfd_section *,
   char **);

extern bfd_boolean bfd_cr16_elf32_create_embedded_relocs
  (bfd *, struct bfd_link_info *, struct bfd_section *, struct bfd_section *,
   char **);

/* SunOS shared library support routines for the linker.  */

extern struct bfd_link_needed_list *bfd_sunos_get_needed_list
  (bfd *, struct bfd_link_info *);
extern bfd_boolean bfd_sunos_record_link_assignment
  (bfd *, struct bfd_link_info *, const char *);
extern bfd_boolean bfd_sunos_size_dynamic_sections
  (bfd *, struct bfd_link_info *, struct bfd_section **,
   struct bfd_section **, struct bfd_section **);

/* Linux shared library support routines for the linker.  */

extern bfd_boolean bfd_i386linux_size_dynamic_sections
  (bfd *, struct bfd_link_info *);
extern bfd_boolean bfd_m68klinux_size_dynamic_sections
  (bfd *, struct bfd_link_info *);
extern bfd_boolean bfd_sparclinux_size_dynamic_sections
  (bfd *, struct bfd_link_info *);

/* mmap hacks */

struct _bfd_window_internal;
typedef struct _bfd_window_internal bfd_window_internal;

typedef struct _bfd_window
{
  /* What the user asked for.  */
  void *data;
  bfd_size_type size;
  /* The actual window used by BFD.  Small user-requested read-only
     regions sharing a page may share a single window into the object
     file.  Read-write versions shouldn't until I've fixed things to
     keep track of which portions have been claimed by the
     application; don't want to give the same region back when the
     application wants two writable copies!  */
  struct _bfd_window_internal *i;
}
bfd_window;

extern void bfd_init_window
  (bfd_window *);
extern void bfd_free_window
  (bfd_window *);
extern bfd_boolean bfd_get_file_window
  (bfd *, file_ptr, bfd_size_type, bfd_window *, bfd_boolean);

/* XCOFF support routines for the linker.  */

extern bfd_boolean bfd_xcoff_split_import_path
  (bfd *, const char *, const char **, const char **);
extern bfd_boolean bfd_xcoff_set_archive_import_path
  (struct bfd_link_info *, bfd *, const char *);
extern bfd_boolean bfd_xcoff_link_record_set
  (bfd *, struct bfd_link_info *, struct bfd_link_hash_entry *, bfd_size_type);
extern bfd_boolean bfd_xcoff_import_symbol
  (bfd *, struct bfd_link_info *, struct bfd_link_hash_entry *, bfd_vma,
   const char *, const char *, const char *, unsigned int);
extern bfd_boolean bfd_xcoff_export_symbol
  (bfd *, struct bfd_link_info *, struct bfd_link_hash_entry *);
extern bfd_boolean bfd_xcoff_link_count_reloc
  (bfd *, struct bfd_link_info *, const char *);
extern bfd_boolean bfd_xcoff_record_link_assignment
  (bfd *, struct bfd_link_info *, const char *);
extern bfd_boolean bfd_xcoff_size_dynamic_sections
  (bfd *, struct bfd_link_info *, const char *, const char *,
   unsigned long, unsigned long, unsigned long, bfd_boolean,
   int, bfd_boolean, unsigned int, struct bfd_section **, bfd_boolean);
extern bfd_boolean bfd_xcoff_link_generate_rtinit
  (bfd *, const char *, const char *, bfd_boolean);

/* XCOFF support routines for ar.  */
extern bfd_boolean bfd_xcoff_ar_archive_set_magic
  (bfd *, char *);

/* Externally visible COFF routines.  */

#if defined(__STDC__) || defined(ALMOST_STDC)
struct internal_syment;
union internal_auxent;
#endif

extern bfd_boolean bfd_coff_set_symbol_class
  (bfd *, struct bfd_symbol *, unsigned int);

extern bfd_boolean bfd_m68k_coff_create_embedded_relocs
  (bfd *, struct bfd_link_info *, struct bfd_section *, struct bfd_section *, char **);

/* ARM VFP11 erratum workaround support.  */
typedef enum
{
  BFD_ARM_VFP11_FIX_DEFAULT,
  BFD_ARM_VFP11_FIX_NONE,
  BFD_ARM_VFP11_FIX_SCALAR,
  BFD_ARM_VFP11_FIX_VECTOR
} bfd_arm_vfp11_fix;

extern void bfd_elf32_arm_init_maps
  (bfd *);

extern void bfd_elf32_arm_set_vfp11_fix
  (bfd *, struct bfd_link_info *);

extern void bfd_elf32_arm_set_cortex_a8_fix
  (bfd *, struct bfd_link_info *);

extern bfd_boolean bfd_elf32_arm_vfp11_erratum_scan
  (bfd *, struct bfd_link_info *);

extern void bfd_elf32_arm_vfp11_fix_veneer_locations
  (bfd *, struct bfd_link_info *);

/* ARM STM STM32L4XX erratum workaround support.  */
typedef enum
{
  BFD_ARM_STM32L4XX_FIX_NONE,
  BFD_ARM_STM32L4XX_FIX_DEFAULT,
  BFD_ARM_STM32L4XX_FIX_ALL
} bfd_arm_stm32l4xx_fix;

extern void bfd_elf32_arm_set_stm32l4xx_fix
  (bfd *, struct bfd_link_info *);

extern bfd_boolean bfd_elf32_arm_stm32l4xx_erratum_scan
  (bfd *, struct bfd_link_info *);

extern void bfd_elf32_arm_stm32l4xx_fix_veneer_locations
  (bfd *, struct bfd_link_info *);

/* ARM Interworking support.  Called from linker.  */
extern bfd_boolean bfd_arm_allocate_interworking_sections
  (struct bfd_link_info *);

extern bfd_boolean bfd_arm_process_before_allocation
  (bfd *, struct bfd_link_info *, int);

extern bfd_boolean bfd_arm_get_bfd_for_interworking
  (bfd *, struct bfd_link_info *);

/* PE ARM Interworking support.  Called from linker.  */
extern bfd_boolean bfd_arm_pe_allocate_interworking_sections
  (struct bfd_link_info *);

extern bfd_boolean bfd_arm_pe_process_before_allocation
  (bfd *, struct bfd_link_info *, int);

extern bfd_boolean bfd_arm_pe_get_bfd_for_interworking
  (bfd *, struct bfd_link_info *);

/* ELF ARM Interworking support.  Called from linker.  */
extern bfd_boolean bfd_elf32_arm_allocate_interworking_sections
  (struct bfd_link_info *);

extern bfd_boolean bfd_elf32_arm_process_before_allocation
  (bfd *, struct bfd_link_info *);

void bfd_elf32_arm_set_target_relocs
  (bfd *, struct bfd_link_info *, int, char *, int, int, bfd_arm_vfp11_fix,
   bfd_arm_stm32l4xx_fix, int, int, int, int, int);

extern bfd_boolean bfd_elf32_arm_get_bfd_for_interworking
  (bfd *, struct bfd_link_info *);

extern bfd_boolean bfd_elf32_arm_add_glue_sections_to_bfd
  (bfd *, struct bfd_link_info *);

extern void bfd_elf32_arm_keep_private_stub_output_sections
  (struct bfd_link_info *);

/* ELF ARM mapping symbol support.  */
#define BFD_ARM_SPECIAL_SYM_TYPE_MAP	(1 << 0)
#define BFD_ARM_SPECIAL_SYM_TYPE_TAG	(1 << 1)
#define BFD_ARM_SPECIAL_SYM_TYPE_OTHER  (1 << 2)
#define BFD_ARM_SPECIAL_SYM_TYPE_ANY	(~0)

extern bfd_boolean bfd_is_arm_special_symbol_name
  (const char *, int);

extern void bfd_elf32_arm_set_byteswap_code
  (struct bfd_link_info *, int);

extern void bfd_elf32_arm_use_long_plt (void);

/* ARM Note section processing.  */
extern bfd_boolean bfd_arm_merge_machines
  (bfd *, bfd *);

extern bfd_boolean bfd_arm_update_notes
  (bfd *, const char *);

extern unsigned int bfd_arm_get_mach_from_notes
  (bfd *, const char *);

/* ARM stub generation support.  Called from the linker.  */
extern int elf32_arm_setup_section_lists
  (bfd *, struct bfd_link_info *);
extern void elf32_arm_next_input_section
  (struct bfd_link_info *, struct bfd_section *);
extern bfd_boolean elf32_arm_size_stubs
  (bfd *, bfd *, struct bfd_link_info *, bfd_signed_vma,
   struct bfd_section * (*) (const char *, struct bfd_section *,
			     struct bfd_section *, unsigned int),
   void (*) (void));
extern bfd_boolean elf32_arm_build_stubs
  (struct bfd_link_info *);

/* ARM unwind section editing support.  */
extern bfd_boolean elf32_arm_fix_exidx_coverage
(struct bfd_section **, unsigned int, struct bfd_link_info *, bfd_boolean);

/* C6x unwind section editing support.  */
extern bfd_boolean elf32_tic6x_fix_exidx_coverage
(struct bfd_section **, unsigned int, struct bfd_link_info *, bfd_boolean);

extern void bfd_elf64_aarch64_init_maps
  (bfd *);

extern void bfd_elf32_aarch64_init_maps
  (bfd *);

extern void bfd_elf64_aarch64_set_options
  (bfd *, struct bfd_link_info *, int, int, int, int, int, int);

extern void bfd_elf32_aarch64_set_options
  (bfd *, struct bfd_link_info *, int, int, int, int, int, int);

/* ELF AArch64 mapping symbol support.  */
#define BFD_AARCH64_SPECIAL_SYM_TYPE_MAP	(1 << 0)
#define BFD_AARCH64_SPECIAL_SYM_TYPE_TAG	(1 << 1)
#define BFD_AARCH64_SPECIAL_SYM_TYPE_OTHER	(1 << 2)
#define BFD_AARCH64_SPECIAL_SYM_TYPE_ANY	(~0)
extern bfd_boolean bfd_is_aarch64_special_symbol_name
  (const char * name, int type);

/* AArch64 stub generation support for ELF64.  Called from the linker.  */
extern int elf64_aarch64_setup_section_lists
  (bfd *, struct bfd_link_info *);
extern void elf64_aarch64_next_input_section
  (struct bfd_link_info *, struct bfd_section *);
extern bfd_boolean elf64_aarch64_size_stubs
  (bfd *, bfd *, struct bfd_link_info *, bfd_signed_vma,
   struct bfd_section * (*) (const char *, struct bfd_section *),
   void (*) (void));
extern bfd_boolean elf64_aarch64_build_stubs
  (struct bfd_link_info *);
/* AArch64 stub generation support for ELF32.  Called from the linker.  */
extern int elf32_aarch64_setup_section_lists
  (bfd *, struct bfd_link_info *);
extern void elf32_aarch64_next_input_section
  (struct bfd_link_info *, struct bfd_section *);
extern bfd_boolean elf32_aarch64_size_stubs
  (bfd *, bfd *, struct bfd_link_info *, bfd_signed_vma,
   struct bfd_section * (*) (const char *, struct bfd_section *),
   void (*) (void));
extern bfd_boolean elf32_aarch64_build_stubs
  (struct bfd_link_info *);


/* TI COFF load page support.  */
extern void bfd_ticoff_set_section_load_page
  (struct bfd_section *, int);

extern int bfd_ticoff_get_section_load_page
  (struct bfd_section *);

/* H8/300 functions.  */
extern bfd_vma bfd_h8300_pad_address
  (bfd *, bfd_vma);

/* IA64 Itanium code generation.  Called from linker.  */
extern void bfd_elf32_ia64_after_parse
  (int);

extern void bfd_elf64_ia64_after_parse
  (int);

/* V850 Note manipulation routines.  */
extern bfd_boolean v850_elf_create_sections
  (struct bfd_link_info *);

extern bfd_boolean v850_elf_set_note
  (bfd *, unsigned int, unsigned int);
