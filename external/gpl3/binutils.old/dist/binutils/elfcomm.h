/* elfcomm.h -- include file of common code for ELF format file.
   Copyright (C) 2010-2016 Free Software Foundation, Inc.

   Originally developed by Eric Youngdale <eric@andante.jic.com>
   Modifications by Nick Clifton <nickc@redhat.com>

   This file is part of GNU Binutils.

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
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#ifndef _ELFCOMM_H
#define _ELFCOMM_H

#include "aout/ar.h"

void error (const char *, ...) ATTRIBUTE_PRINTF_1;
void warn (const char *, ...) ATTRIBUTE_PRINTF_1;

#if defined HAVE_LONG_LONG && SIZEOF_LONG_LONG > SIZEOF_LONG
/* We can't use any bfd types here since readelf may define BFD64 and
   objdump may not.  */
#define HOST_WIDEST_INT	long long
#else
#define HOST_WIDEST_INT long
#endif
typedef unsigned HOST_WIDEST_INT elf_vma;

extern void (*byte_put) (unsigned char *, elf_vma, int);
extern void byte_put_little_endian (unsigned char *, elf_vma, int);
extern void byte_put_big_endian (unsigned char *, elf_vma, int);

extern elf_vma (*byte_get) (unsigned char *, int);
extern elf_vma byte_get_signed (unsigned char *, int);
extern elf_vma byte_get_little_endian (unsigned char *, int);
extern elf_vma byte_get_big_endian (unsigned char *, int);
extern void byte_get_64 (unsigned char *, elf_vma *, elf_vma *);

#define BYTE_PUT(field, val)	byte_put (field, val, sizeof (field))
#define BYTE_GET(field)		byte_get (field, sizeof (field))
#define BYTE_GET_SIGNED(field)	byte_get_signed (field, sizeof (field))

/* This is just a bit of syntatic sugar.  */
#define streq(a,b)	  (strcmp ((a), (b)) == 0)
#define strneq(a,b,n)	  (strncmp ((a), (b), (n)) == 0)
#define const_strneq(a,b) (strncmp ((a), (b), sizeof (b) - 1) == 0)

/* Structure to hold information about an archive file.  */

struct archive_info
{
  char * file_name;                     /* Archive file name.  */
  FILE * file;                          /* Open file descriptor.  */
  elf_vma index_num;                    /* Number of symbols in table.  */
  elf_vma * index_array;                /* The array of member offsets.  */
  char * sym_table;                     /* The symbol table.  */
  unsigned long sym_size;               /* Size of the symbol table.  */
  char * longnames;                     /* The long file names table.  */
  unsigned long longnames_size;         /* Size of the long file names table.  */
  unsigned long nested_member_origin;   /* Origin in the nested archive of the current member.  */
  unsigned long next_arhdr_offset;      /* Offset of the next archive header.  */
  bfd_boolean is_thin_archive;          /* TRUE if this is a thin archive.  */
  bfd_boolean uses_64bit_indicies;      /* TRUE if the index table uses 64bit entries.  */
  struct ar_hdr arhdr;                  /* Current archive header.  */
};

/* Return the path name for a proxy entry in a thin archive.  */
extern char *adjust_relative_path (const char *, const char *, unsigned long);

/* Read the symbol table and long-name table from an archive.  */
extern int setup_archive (struct archive_info *, const char *, FILE *,
			  bfd_boolean, bfd_boolean);

/* Open and setup a nested archive, if not already open.  */
extern int setup_nested_archive (struct archive_info *, const char *);

/* Release the memory used for the archive information.  */
extern void release_archive (struct archive_info *);

/* Get the name of an archive member from the current archive header.  */

extern char *get_archive_member_name (struct archive_info *,
				      struct archive_info *);

/* Get the name of an archive member at a given offset within an
   archive.  */

extern char *get_archive_member_name_at (struct archive_info *,
					 unsigned long,
					 struct archive_info *);

/* Construct a string showing the name of the archive member, qualified
   with the name of the containing archive file.  */

extern char *make_qualified_name (struct archive_info *,
				  struct archive_info *,
				  const char *);

#endif /* _ELFCOMM_H */
