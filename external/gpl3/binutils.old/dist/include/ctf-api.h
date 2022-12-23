/* Public API to libctf.
   Copyright (C) 2019-2020 Free Software Foundation, Inc.

   This file is part of libctf.

   libctf is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not see
   <http://www.gnu.org/licenses/>.  */

/* This header file defines the interfaces available from the CTF debugger
   library, libctf.  This API can be used by a debugger to operate on data in
   the Compact ANSI-C Type Format (CTF).  */

#ifndef	_CTF_API_H
#define	_CTF_API_H

#include <sys/types.h>
#include <ctf.h>
#include <zlib.h>

#ifdef	__cplusplus
extern "C"
  {
#endif

/* Clients can open one or more CTF containers and obtain a pointer to an
   opaque ctf_file_t.  Types are identified by an opaque ctf_id_t token.
   They can also open or create read-only archives of CTF containers in a
   ctf_archive_t.

   These opaque definitions allow libctf to evolve without breaking clients.  */

typedef struct ctf_file ctf_file_t;
typedef struct ctf_archive_internal ctf_archive_t;
typedef unsigned long ctf_id_t;

/* This opaque definition allows libctf to accept BFD data structures without
   importing all the BFD noise into users' namespaces.  */

struct bfd;

/* If the debugger needs to provide the CTF library with a set of raw buffers
   for use as the CTF data, symbol table, and string table, it can do so by
   filling in ctf_sect_t structures and passing them to ctf_bufopen().

   The contents of this structure must always be in native endianness (no
   byteswapping is performed).  */

typedef struct ctf_sect
{
  const char *cts_name;		  /* Section name (if any).  */
  const void *cts_data;		  /* Pointer to section data.  */
  size_t cts_size;		  /* Size of data in bytes.  */
  size_t cts_entsize;		  /* Size of each section entry (symtab only).  */
} ctf_sect_t;

/* A minimal symbol extracted from a linker's internal symbol table
   representation.  */

typedef struct ctf_link_sym
{
  /* The st_name will not be accessed outside the call to
     ctf_link_shuffle_syms().  */

  const char *st_name;
  uint32_t st_shndx;
  uint32_t st_type;
  uint32_t st_value;
} ctf_link_sym_t;

/* Indication of how to share types when linking.  */

/* Share all types thare are not in conflict.  The default.  */
#define CTF_LINK_SHARE_UNCONFLICTED 0x0

/* Share only types that are used by multiple inputs.  Not implemented yet.  */
#define CTF_LINK_SHARE_DUPLICATED 0x1

/* Symbolic names for CTF sections.  */

typedef enum ctf_sect_names
  {
   CTF_SECT_HEADER,
   CTF_SECT_LABEL,
   CTF_SECT_OBJT,
   CTF_SECT_FUNC,
   CTF_SECT_VAR,
   CTF_SECT_TYPE,
   CTF_SECT_STR
  } ctf_sect_names_t;

/* Encoding information for integers, floating-point values, and certain other
   intrinsics can be obtained by calling ctf_type_encoding(), below.  The flags
   field will contain values appropriate for the type defined in <ctf.h>.  */

typedef struct ctf_encoding
{
  uint32_t cte_format;		 /* Data format (CTF_INT_* or CTF_FP_* flags).  */
  uint32_t cte_offset;		 /* Offset of value in bits.  */
  uint32_t cte_bits;		 /* Size of storage in bits.  */
} ctf_encoding_t;

typedef struct ctf_membinfo
{
  ctf_id_t ctm_type;		/* Type of struct or union member.  */
  unsigned long ctm_offset;	/* Offset of member in bits.  */
} ctf_membinfo_t;

typedef struct ctf_arinfo
{
  ctf_id_t ctr_contents;	/* Type of array contents.  */
  ctf_id_t ctr_index;		/* Type of array index.  */
  uint32_t ctr_nelems;		/* Number of elements.  */
} ctf_arinfo_t;

typedef struct ctf_funcinfo
{
  ctf_id_t ctc_return;		/* Function return type.  */
  uint32_t ctc_argc;		/* Number of typed arguments to function.  */
  uint32_t ctc_flags;		/* Function attributes (see below).  */
} ctf_funcinfo_t;

typedef struct ctf_lblinfo
{
  ctf_id_t ctb_type;		/* Last type associated with the label.  */
} ctf_lblinfo_t;

typedef struct ctf_snapshot_id
{
  unsigned long dtd_id;		/* Highest DTD ID at time of snapshot.  */
  unsigned long snapshot_id;	/* Snapshot id at time of snapshot.  */
} ctf_snapshot_id_t;

#define	CTF_FUNC_VARARG	0x1	/* Function arguments end with varargs.  */

/* Functions that return a ctf_id_t use the following value to indicate failure.
   ctf_errno() can be used to obtain an error code.  Functions that return
   a straight integral -1 also use ctf_errno().  */
#define	CTF_ERR	((ctf_id_t) -1L)

#define	ECTF_BASE	1000	/* Base value for libctf errnos.  */


enum
  {
   ECTF_FMT = ECTF_BASE,	/* File is not in CTF or ELF format.  */
   ECTF_BFDERR,			/* BFD error.  */
   ECTF_CTFVERS,		/* CTF version is more recent than libctf.  */
   ECTF_BFD_AMBIGUOUS,		/* Ambiguous BFD target.  */
   ECTF_SYMTAB,			/* Symbol table uses invalid entry size.  */
   ECTF_SYMBAD,			/* Symbol table data buffer invalid.  */
   ECTF_STRBAD,			/* String table data buffer invalid.  */
   ECTF_CORRUPT,		/* File data corruption detected.  */
   ECTF_NOCTFDATA,		/* ELF file does not contain CTF data.  */
   ECTF_NOCTFBUF,		/* Buffer does not contain CTF data.  */
   ECTF_NOSYMTAB,		/* Symbol table data is not available.  */
   ECTF_NOPARENT,		/* Parent CTF container is not available.  */
   ECTF_DMODEL,			/* Data model mismatch.  */
   ECTF_LINKADDEDLATE,		/* File added to link too late.  */
   ECTF_ZALLOC,			/* Failed to allocate (de)compression buffer.  */
   ECTF_DECOMPRESS,		/* Failed to decompress CTF data.  */
   ECTF_STRTAB,			/* String table for this string is missing.  */
   ECTF_BADNAME,		/* String offset is corrupt w.r.t. strtab.  */
   ECTF_BADID,			/* Invalid type ID number.  */
   ECTF_NOTSOU,			/* Type is not a struct or union.  */
   ECTF_NOTENUM,		/* Type is not an enum.  */
   ECTF_NOTSUE,			/* Type is not a struct, union, or enum.  */
   ECTF_NOTINTFP,		/* Type is not an integer, float, or enum.  */
   ECTF_NOTARRAY,		/* Type is not an array.  */
   ECTF_NOTREF,			/* Type does not reference another type.  */
   ECTF_NAMELEN,		/* Buffer is too small to hold type name.  */
   ECTF_NOTYPE,			/* No type found corresponding to name.  */
   ECTF_SYNTAX,			/* Syntax error in type name.  */
   ECTF_NOTFUNC,		/* Symbol entry or type is not a function.  */
   ECTF_NOFUNCDAT,		/* No func info available for function.  */
   ECTF_NOTDATA,		/* Symtab entry does not refer to a data obj.  */
   ECTF_NOTYPEDAT,		/* No type info available for object.  */
   ECTF_NOLABEL,		/* No label found corresponding to name.  */
   ECTF_NOLABELDATA,		/* File does not contain any labels.  */
   ECTF_NOTSUP,			/* Feature not supported.  */
   ECTF_NOENUMNAM,		/* Enum element name not found.  */
   ECTF_NOMEMBNAM,		/* Member name not found.  */
   ECTF_RDONLY,			/* CTF container is read-only.  */
   ECTF_DTFULL,			/* CTF type is full (no more members allowed).  */
   ECTF_FULL,			/* CTF container is full.  */
   ECTF_DUPLICATE,		/* Duplicate member or variable name.  */
   ECTF_CONFLICT,		/* Conflicting type definition present.  */
   ECTF_OVERROLLBACK,		/* Attempt to roll back past a ctf_update.  */
   ECTF_COMPRESS,		/* Failed to compress CTF data.  */
   ECTF_ARCREATE,		/* Error creating CTF archive.  */
   ECTF_ARNNAME,		/* Name not found in CTF archive.  */
   ECTF_SLICEOVERFLOW,		/* Overflow of type bitness or offset in slice.  */
   ECTF_DUMPSECTUNKNOWN,	/* Unknown section number in dump.  */
   ECTF_DUMPSECTCHANGED,	/* Section changed in middle of dump.  */
   ECTF_NOTYET,			/* Feature not yet implemented.  */
   ECTF_INTERNAL,		/* Internal error in link.  */
   ECTF_NONREPRESENTABLE	/* Type not representable in CTF.  */
  };

/* The CTF data model is inferred to be the caller's data model or the data
   model of the given object, unless ctf_setmodel() is explicitly called.  */
#define	CTF_MODEL_ILP32 1	/* Object data model is ILP32.  */
#define	CTF_MODEL_LP64  2	/* Object data model is LP64.  */
#ifdef _LP64
# define CTF_MODEL_NATIVE CTF_MODEL_LP64
#else
# define CTF_MODEL_NATIVE CTF_MODEL_ILP32
#endif

/* Dynamic CTF containers can be created using ctf_create().  The ctf_add_*
   routines can be used to add new definitions to the dynamic container.
   New types are labeled as root or non-root to determine whether they are
   visible at the top-level program scope when subsequently doing a lookup.  */

#define	CTF_ADD_NONROOT	0	/* Type only visible in nested scope.  */
#define	CTF_ADD_ROOT	1	/* Type visible at top-level scope.  */

/* These typedefs are used to define the signature for callback functions
   that can be used with the iteration and visit functions below.  */

typedef int ctf_visit_f (const char *name, ctf_id_t type, unsigned long offset,
			 int depth, void *arg);
typedef int ctf_member_f (const char *name, ctf_id_t membtype,
			  unsigned long offset, void *arg);
typedef int ctf_enum_f (const char *name, int val, void *arg);
typedef int ctf_variable_f (const char *name, ctf_id_t type, void *arg);
typedef int ctf_type_f (ctf_id_t type, void *arg);
typedef int ctf_type_all_f (ctf_id_t type, int flag, void *arg);
typedef int ctf_label_f (const char *name, const ctf_lblinfo_t *info,
			 void *arg);
typedef int ctf_archive_member_f (ctf_file_t *fp, const char *name, void *arg);
typedef int ctf_archive_raw_member_f (const char *name, const void *content,
				      size_t len, void *arg);
typedef char *ctf_dump_decorate_f (ctf_sect_names_t sect,
				   char *line, void *arg);

typedef struct ctf_dump_state ctf_dump_state_t;

/* Opening.  These mostly return an abstraction over both CTF files and CTF
   archives: so they can be used to open both.  CTF files will appear to be an
   archive with one member named '.ctf'.  The low-level functions
   ctf_simple_open() and ctf_bufopen() return ctf_file_t's directly, and cannot
   be used on CTF archives.  */

extern ctf_archive_t *ctf_bfdopen (struct bfd *, int *);
extern ctf_archive_t *ctf_bfdopen_ctfsect (struct bfd *, const ctf_sect_t *,
					   int *);
extern ctf_archive_t *ctf_fdopen (int fd, const char *filename,
				  const char *target, int *errp);
extern ctf_archive_t *ctf_open (const char *filename,
				const char *target, int *errp);
extern void ctf_close (ctf_archive_t *);
extern ctf_sect_t ctf_getdatasect (const ctf_file_t *);
extern ctf_archive_t *ctf_get_arc (const ctf_file_t *);
extern ctf_archive_t *ctf_arc_open (const char *, int *);
extern void ctf_arc_close (ctf_archive_t *);
extern ctf_file_t *ctf_arc_open_by_name (const ctf_archive_t *,
					 const char *, int *);
extern ctf_file_t *ctf_arc_open_by_name_sections (const ctf_archive_t *,
						  const ctf_sect_t *,
						  const ctf_sect_t *,
						  const char *, int *);

/* The next functions return or close real CTF files, or write out CTF archives,
   not opaque containers around either.  */

extern ctf_file_t *ctf_simple_open (const char *, size_t, const char *, size_t,
				   size_t, const char *, size_t, int *);
extern ctf_file_t *ctf_bufopen (const ctf_sect_t *, const ctf_sect_t *,
				const ctf_sect_t *, int *);
extern void ctf_file_close (ctf_file_t *);

extern int ctf_arc_write (const char *, ctf_file_t **, size_t,
			  const char **, size_t);
extern int ctf_arc_write_fd (int, ctf_file_t **, size_t, const char **,
			     size_t);

extern const char *ctf_cuname (ctf_file_t *);
extern int ctf_cuname_set (ctf_file_t *, const char *);
extern ctf_file_t *ctf_parent_file (ctf_file_t *);
extern const char *ctf_parent_name (ctf_file_t *);
extern int ctf_parent_name_set (ctf_file_t *, const char *);
extern int ctf_type_isparent (ctf_file_t *, ctf_id_t);
extern int ctf_type_ischild (ctf_file_t *, ctf_id_t);

extern int ctf_import (ctf_file_t *, ctf_file_t *);
extern int ctf_setmodel (ctf_file_t *, int);
extern int ctf_getmodel (ctf_file_t *);

extern void ctf_setspecific (ctf_file_t *, void *);
extern void *ctf_getspecific (ctf_file_t *);

extern int ctf_errno (ctf_file_t *);
extern const char *ctf_errmsg (int);
extern int ctf_version (int);

extern int ctf_func_info (ctf_file_t *, unsigned long, ctf_funcinfo_t *);
extern int ctf_func_args (ctf_file_t *, unsigned long, uint32_t, ctf_id_t *);
extern int ctf_func_type_info (ctf_file_t *, ctf_id_t, ctf_funcinfo_t *);
extern int ctf_func_type_args (ctf_file_t *, ctf_id_t, uint32_t, ctf_id_t *);

extern ctf_id_t ctf_lookup_by_name (ctf_file_t *, const char *);
extern ctf_id_t ctf_lookup_by_symbol (ctf_file_t *, unsigned long);
extern ctf_id_t ctf_lookup_variable (ctf_file_t *, const char *);

extern ctf_id_t ctf_type_resolve (ctf_file_t *, ctf_id_t);
extern char *ctf_type_aname (ctf_file_t *, ctf_id_t);
extern char *ctf_type_aname_raw (ctf_file_t *, ctf_id_t);
extern ssize_t ctf_type_lname (ctf_file_t *, ctf_id_t, char *, size_t);
extern char *ctf_type_name (ctf_file_t *, ctf_id_t, char *, size_t);
extern ssize_t ctf_type_size (ctf_file_t *, ctf_id_t);
extern ssize_t ctf_type_align (ctf_file_t *, ctf_id_t);
extern int ctf_type_kind (ctf_file_t *, ctf_id_t);
extern ctf_id_t ctf_type_reference (ctf_file_t *, ctf_id_t);
extern ctf_id_t ctf_type_pointer (ctf_file_t *, ctf_id_t);
extern int ctf_type_encoding (ctf_file_t *, ctf_id_t, ctf_encoding_t *);
extern int ctf_type_visit (ctf_file_t *, ctf_id_t, ctf_visit_f *, void *);
extern int ctf_type_cmp (ctf_file_t *, ctf_id_t, ctf_file_t *, ctf_id_t);
extern int ctf_type_compat (ctf_file_t *, ctf_id_t, ctf_file_t *, ctf_id_t);

extern int ctf_member_info (ctf_file_t *, ctf_id_t, const char *,
			    ctf_membinfo_t *);
extern int ctf_array_info (ctf_file_t *, ctf_id_t, ctf_arinfo_t *);

extern const char *ctf_enum_name (ctf_file_t *, ctf_id_t, int);
extern int ctf_enum_value (ctf_file_t *, ctf_id_t, const char *, int *);

extern void ctf_label_set (ctf_file_t *, const char *);
extern const char *ctf_label_get (ctf_file_t *);

extern const char *ctf_label_topmost (ctf_file_t *);
extern int ctf_label_info (ctf_file_t *, const char *, ctf_lblinfo_t *);

extern int ctf_member_iter (ctf_file_t *, ctf_id_t, ctf_member_f *, void *);
extern int ctf_enum_iter (ctf_file_t *, ctf_id_t, ctf_enum_f *, void *);
extern int ctf_type_iter (ctf_file_t *, ctf_type_f *, void *);
extern int ctf_type_iter_all (ctf_file_t *, ctf_type_all_f *, void *);
extern int ctf_label_iter (ctf_file_t *, ctf_label_f *, void *);
extern int ctf_variable_iter (ctf_file_t *, ctf_variable_f *, void *);
extern int ctf_archive_iter (const ctf_archive_t *, ctf_archive_member_f *,
			     void *);
/* This function alone does not currently operate on CTF files masquerading
   as archives, and returns -EINVAL: the raw data is no longer available.  It is
   expected to be used only by archiving tools, in any case, which have no need
   to deal with non-archives at all.  */
extern int ctf_archive_raw_iter (const ctf_archive_t *,
				 ctf_archive_raw_member_f *, void *);
extern char *ctf_dump (ctf_file_t *, ctf_dump_state_t **state,
		       ctf_sect_names_t sect, ctf_dump_decorate_f *,
		       void *arg);

extern ctf_id_t ctf_add_array (ctf_file_t *, uint32_t,
			       const ctf_arinfo_t *);
extern ctf_id_t ctf_add_const (ctf_file_t *, uint32_t, ctf_id_t);
extern ctf_id_t ctf_add_enum_encoded (ctf_file_t *, uint32_t, const char *,
				      const ctf_encoding_t *);
extern ctf_id_t ctf_add_enum (ctf_file_t *, uint32_t, const char *);
extern ctf_id_t ctf_add_float (ctf_file_t *, uint32_t,
			       const char *, const ctf_encoding_t *);
extern ctf_id_t ctf_add_forward (ctf_file_t *, uint32_t, const char *,
				 uint32_t);
extern ctf_id_t ctf_add_function (ctf_file_t *, uint32_t,
				  const ctf_funcinfo_t *, const ctf_id_t *);
extern ctf_id_t ctf_add_integer (ctf_file_t *, uint32_t, const char *,
				 const ctf_encoding_t *);
extern ctf_id_t ctf_add_slice (ctf_file_t *, uint32_t, ctf_id_t, const ctf_encoding_t *);
extern ctf_id_t ctf_add_pointer (ctf_file_t *, uint32_t, ctf_id_t);
extern ctf_id_t ctf_add_type (ctf_file_t *, ctf_file_t *, ctf_id_t);
extern ctf_id_t ctf_add_typedef (ctf_file_t *, uint32_t, const char *,
				 ctf_id_t);
extern ctf_id_t ctf_add_restrict (ctf_file_t *, uint32_t, ctf_id_t);
extern ctf_id_t ctf_add_struct (ctf_file_t *, uint32_t, const char *);
extern ctf_id_t ctf_add_union (ctf_file_t *, uint32_t, const char *);
extern ctf_id_t ctf_add_struct_sized (ctf_file_t *, uint32_t, const char *,
				      size_t);
extern ctf_id_t ctf_add_union_sized (ctf_file_t *, uint32_t, const char *,
				     size_t);
extern ctf_id_t ctf_add_volatile (ctf_file_t *, uint32_t, ctf_id_t);

extern int ctf_add_enumerator (ctf_file_t *, ctf_id_t, const char *, int);
extern int ctf_add_member (ctf_file_t *, ctf_id_t, const char *, ctf_id_t);
extern int ctf_add_member_offset (ctf_file_t *, ctf_id_t, const char *,
				  ctf_id_t, unsigned long);
extern int ctf_add_member_encoded (ctf_file_t *, ctf_id_t, const char *,
				   ctf_id_t, unsigned long,
				   const ctf_encoding_t);

extern int ctf_add_variable (ctf_file_t *, const char *, ctf_id_t);

extern int ctf_set_array (ctf_file_t *, ctf_id_t, const ctf_arinfo_t *);

extern ctf_file_t *ctf_create (int *);
extern int ctf_update (ctf_file_t *);
extern ctf_snapshot_id_t ctf_snapshot (ctf_file_t *);
extern int ctf_rollback (ctf_file_t *, ctf_snapshot_id_t);
extern int ctf_discard (ctf_file_t *);
extern int ctf_write (ctf_file_t *, int);
extern int ctf_gzwrite (ctf_file_t *fp, gzFile fd);
extern int ctf_compress_write (ctf_file_t * fp, int fd);
extern unsigned char *ctf_write_mem (ctf_file_t *, size_t *, size_t threshold);

/* The ctf_link interfaces are not stable yet.  No guarantees!  */

extern int ctf_link_add_ctf (ctf_file_t *, ctf_archive_t *, const char *);
extern int ctf_link (ctf_file_t *, int share_mode);
typedef const char *ctf_link_strtab_string_f (uint32_t *offset, void *arg);
extern int ctf_link_add_strtab (ctf_file_t *, ctf_link_strtab_string_f *,
				void *);
typedef ctf_link_sym_t *ctf_link_iter_symbol_f (ctf_link_sym_t *dest,
						void *arg);
extern int ctf_link_shuffle_syms (ctf_file_t *, ctf_link_iter_symbol_f *,
				  void *);
extern unsigned char *ctf_link_write (ctf_file_t *, size_t *size,
				      size_t threshold);

/* Specialist linker functions.  These functions are not used by ld, but can be
   used by other prgorams making use of the linker machinery for other purposes
   to customize its output.  */
extern int ctf_link_add_cu_mapping (ctf_file_t *, const char *from,
				    const char *to);
typedef char *ctf_link_memb_name_changer_f (ctf_file_t *,
					    const char *, void *);
extern void ctf_link_set_memb_name_changer
  (ctf_file_t *, ctf_link_memb_name_changer_f *, void *);

extern void ctf_setdebug (int debug);
extern int ctf_getdebug (void);

#ifdef	__cplusplus
}
#endif

#endif				/* _CTF_API_H */
