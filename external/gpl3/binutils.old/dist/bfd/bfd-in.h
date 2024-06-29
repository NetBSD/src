/* Main header file for the bfd library -- portable access to object files.

   Copyright (C) 1990-2022 Free Software Foundation, Inc.

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
#include <stdint.h>
#include <stdbool.h>
#include "diagnostics.h"
#include <stdarg.h>
#include <string.h>
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

#define BFD_SUPPORTS_PLUGINS @supports_plugins@

/* The word size used by BFD on the host.  This may be 64 with a 32
   bit target if the host is 64 bit, or if other 64 bit targets have
   been selected with --enable-targets, or if --enable-64-bit-bfd.  */
#define BFD_ARCH_SIZE @wordsize@

/* The word size of the default bfd target.  */
#define BFD_DEFAULT_TARGET_SIZE @bfd_default_target_size@

#include <inttypes.h>

#if BFD_ARCH_SIZE >= 64
#define BFD64
#endif

/* Forward declaration.  */
typedef struct bfd bfd;

/* Boolean type used in bfd.
   General rule: Functions which are bfd_boolean return TRUE on
   success and FALSE on failure (unless they're a predicate).  */

#ifdef POISON_BFD_BOOLEAN
# pragma GCC poison bfd_boolean
#else
# define bfd_boolean bool
# undef FALSE
# undef TRUE
# define FALSE 0
# define TRUE 1
#endif

/* Silence "applying zero offset to null pointer" UBSAN warnings.  */
#define PTR_ADD(P,A) ((A) != 0 ? (P) + (A) : (P))
/* Also prevent non-zero offsets from being applied to a null pointer.  */
#define NPTR_ADD(P,A) ((P) != NULL ? (P) + (A) : (P))

#ifdef BFD64

/* Represent a target address.  Also used as a generic unsigned type
   which is guaranteed to be big enough to hold any arithmetic types
   we need to deal with.  */
typedef uint64_t bfd_vma;

/* A generic signed type which is guaranteed to be big enough to hold any
   arithmetic types we need to deal with.  Can be assumed to be compatible
   with bfd_vma in the same way that signed and unsigned ints are compatible
   (as parameters, in assignment, etc).  */
typedef int64_t bfd_signed_vma;

typedef uint64_t bfd_size_type;
typedef uint64_t symvalue;

#define BFD_VMA_FMT @BFD_INT64_FMT@

#define fprintf_vma(f,x) fprintf (f, "%016" BFD_VMA_FMT "x", x)
#define sprintf_vma(s,x) sprintf (s, "%016" BFD_VMA_FMT "x", x)

#else /* not BFD64  */

typedef unsigned long bfd_vma;
typedef long bfd_signed_vma;
typedef unsigned long symvalue;
typedef unsigned long bfd_size_type;

#define BFD_VMA_FMT "l"

#define fprintf_vma(s,x) fprintf (s, "%08" BFD_VMA_FMT "x", x)
#define sprintf_vma(s,x) sprintf (s, "%08" BFD_VMA_FMT "x", x)

#endif /* not BFD64  */

#define HALF_BFD_SIZE_TYPE \
  (((bfd_size_type) 1) << (8 * sizeof (bfd_size_type) / 2))

/* An offset into a file.  BFD always uses the largest possible offset
   based on the build time availability of fseek, fseeko, or fseeko64.  */
typedef @bfd_file_ptr@ file_ptr;
typedef @bfd_ufile_ptr@ ufile_ptr;

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

#define BFD_NO_MORE_SYMBOLS ((symindex) ~0)

/* A canonical archive symbol.  */
/* This is a type pun with struct ranlib on purpose!  */
typedef struct carsym
{
  const char *name;
  file_ptr file_offset;	/* Look here to find the file.  */
}
carsym;			/* To make these you call a carsymogen.  */

/* Used in generating armaps (archive tables of contents).
   Perhaps just a forward definition would do?  */
struct orl		/* Output ranlib.  */
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
    bfd_vma offset;		/* Offset into section.  */
  } u;
}
alent;

/* Object and core file sections.  */
typedef struct bfd_section *sec_ptr;

#define	align_power(addr, align)	\
  (((addr) + ((bfd_vma) 1 << (align)) - 1) & (-((bfd_vma) 1 << (align))))

/* Align an address upward to a boundary, expressed as a number of bytes.
   E.g. align to an 8-byte boundary with argument of 8.  Take care never
   to wrap around if the address is within boundary-1 of the end of the
   address space.  */
#define BFD_ALIGN(this, boundary)					  \
  ((((bfd_vma) (this) + (boundary) - 1) >= (bfd_vma) (this))		  \
   ? (((bfd_vma) (this) + ((boundary) - 1)) & ~ (bfd_vma) ((boundary)-1)) \
   : ~ (bfd_vma) 0)

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
  const char *name;		/* Symbol name.  */
  unsigned char stab_type;	/* Stab type.  */
  char stab_other;		/* Stab other.  */
  short stab_desc;		/* Stab desc.  */
  const char *stab_name;	/* String for stab type.  */
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
extern bool bfd_hash_table_init
  (struct bfd_hash_table *,
   struct bfd_hash_entry *(*) (struct bfd_hash_entry *,
			       struct bfd_hash_table *,
			       const char *),
   unsigned int);

/* Initialize a hash table specifying a size.  */
extern bool bfd_hash_table_init_n
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
  (struct bfd_hash_table *, const char *, bool create, bool copy);

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
   bool (*) (struct bfd_hash_entry *, void *),
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
  (_bfd_warn_deprecated ("bfd_read", __FILE__, __LINE__, __FUNCTION__),	\
   bfd_bread ((BUF), (ELTSIZE) * (NITEMS), (ABFD)))
#define bfd_write(BUF, ELTSIZE, NITEMS, ABFD)				\
  (_bfd_warn_deprecated ("bfd_write", __FILE__, __LINE__, __FUNCTION__), \
   bfd_bwrite ((BUF), (ELTSIZE) * (NITEMS), (ABFD)))
#else
#define bfd_read(BUF, ELTSIZE, NITEMS, ABFD)				\
  (_bfd_warn_deprecated ("bfd_read", (const char *) 0, 0, (const char *) 0), \
   bfd_bread ((BUF), (ELTSIZE) * (NITEMS), (ABFD)))
#define bfd_write(BUF, ELTSIZE, NITEMS, ABFD)				\
  (_bfd_warn_deprecated ("bfd_write", (const char *) 0, 0, (const char *) 0),\
   bfd_bwrite ((BUF), (ELTSIZE) * (NITEMS), (ABFD)))
#endif
extern void _bfd_warn_deprecated (const char *, const char *, int, const char *);

extern bool bfd_cache_close
  (bfd *abfd);
/* NB: This declaration should match the autogenerated one in libbfd.h.  */

extern bool bfd_cache_close_all (void);

extern bool bfd_record_phdr
  (bfd *, unsigned long, bool, flagword, bool, bfd_vma,
   bool, bool, unsigned int, struct bfd_section **);

/* Byte swapping routines.  */

uint64_t bfd_getb64 (const void *);
uint64_t bfd_getl64 (const void *);
int64_t bfd_getb_signed_64 (const void *);
int64_t bfd_getl_signed_64 (const void *);
bfd_vma bfd_getb32 (const void *);
bfd_vma bfd_getl32 (const void *);
bfd_signed_vma bfd_getb_signed_32 (const void *);
bfd_signed_vma bfd_getl_signed_32 (const void *);
bfd_vma bfd_getb16 (const void *);
bfd_vma bfd_getl16 (const void *);
bfd_signed_vma bfd_getb_signed_16 (const void *);
bfd_signed_vma bfd_getl_signed_16 (const void *);
void bfd_putb64 (uint64_t, void *);
void bfd_putl64 (uint64_t, void *);
void bfd_putb32 (bfd_vma, void *);
void bfd_putl32 (bfd_vma, void *);
void bfd_putb24 (bfd_vma, void *);
void bfd_putl24 (bfd_vma, void *);
void bfd_putb16 (bfd_vma, void *);
void bfd_putl16 (bfd_vma, void *);

/* Byte swapping routines which take size and endiannes as arguments.  */

uint64_t bfd_get_bits (const void *, int, bool);
void bfd_put_bits (uint64_t, void *, int, bool);


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
extern bool bfd_get_file_window
  (bfd *, file_ptr, bfd_size_type, bfd_window *, bool);

/* Externally visible ELF routines.  */

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

/* Forward declarations.  */
struct ecoff_debug_info;
struct ecoff_debug_swap;
struct ecoff_extr;
struct bfd_link_info;
struct bfd_link_hash_entry;

/* Return TRUE if the start of STR matches PREFIX, FALSE otherwise.  */

static inline bool
startswith (const char *str, const char *prefix)
{
  return strncmp (str, prefix, strlen (prefix)) == 0;
}
