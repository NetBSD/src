/* elfcomm.c -- common code for ELF format file.
   Copyright (C) 2010-2020 Free Software Foundation, Inc.

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

#include "sysdep.h"
#include "libiberty.h"
#include "filenames.h"
#include "bfd.h"
#include "aout/ar.h"
#include "bucomm.h"
#include "elfcomm.h"
#include <assert.h>

void
error (const char *message, ...)
{
  va_list args;

  /* Try to keep error messages in sync with the program's normal output.  */
  fflush (stdout);

  va_start (args, message);
  fprintf (stderr, _("%s: Error: "), program_name);
  vfprintf (stderr, message, args);
  va_end (args);
}

void
warn (const char *message, ...)
{
  va_list args;

  /* Try to keep warning messages in sync with the program's normal output.  */
  fflush (stdout);

  va_start (args, message);
  fprintf (stderr, _("%s: Warning: "), program_name);
  vfprintf (stderr, message, args);
  va_end (args);
}

void (*byte_put) (unsigned char *, elf_vma, int);

void
byte_put_little_endian (unsigned char * field, elf_vma value, int size)
{
  switch (size)
    {
    case 8:
      field[7] = (((value >> 24) >> 24) >> 8) & 0xff;
      field[6] = ((value >> 24) >> 24) & 0xff;
      field[5] = ((value >> 24) >> 16) & 0xff;
      field[4] = ((value >> 24) >> 8) & 0xff;
      /* Fall through.  */
    case 4:
      field[3] = (value >> 24) & 0xff;
      /* Fall through.  */
    case 3:
      field[2] = (value >> 16) & 0xff;
      /* Fall through.  */
    case 2:
      field[1] = (value >> 8) & 0xff;
      /* Fall through.  */
    case 1:
      field[0] = value & 0xff;
      break;

    default:
      error (_("Unhandled data length: %d\n"), size);
      abort ();
    }
}

void
byte_put_big_endian (unsigned char * field, elf_vma value, int size)
{
  switch (size)
    {
    case 8:
      field[7] = value & 0xff;
      field[6] = (value >> 8) & 0xff;
      field[5] = (value >> 16) & 0xff;
      field[4] = (value >> 24) & 0xff;
      value >>= 16;
      value >>= 16;
      /* Fall through.  */
    case 4:
      field[3] = value & 0xff;
      value >>= 8;
      /* Fall through.  */
    case 3:
      field[2] = value & 0xff;
      value >>= 8;
      /* Fall through.  */
    case 2:
      field[1] = value & 0xff;
      value >>= 8;
      /* Fall through.  */
    case 1:
      field[0] = value & 0xff;
      break;

    default:
      error (_("Unhandled data length: %d\n"), size);
      abort ();
    }
}

elf_vma (*byte_get) (const unsigned char *, int);

elf_vma
byte_get_little_endian (const unsigned char *field, int size)
{
  switch (size)
    {
    case 1:
      return *field;

    case 2:
      return  ((unsigned int) (field[0]))
	|    (((unsigned int) (field[1])) << 8);

    case 3:
      return  ((unsigned long) (field[0]))
	|    (((unsigned long) (field[1])) << 8)
	|    (((unsigned long) (field[2])) << 16);

    case 4:
      return  ((unsigned long) (field[0]))
	|    (((unsigned long) (field[1])) << 8)
	|    (((unsigned long) (field[2])) << 16)
	|    (((unsigned long) (field[3])) << 24);

    case 5:
      if (sizeof (elf_vma) == 8)
	return  ((elf_vma) (field[0]))
	  |    (((elf_vma) (field[1])) << 8)
	  |    (((elf_vma) (field[2])) << 16)
	  |    (((elf_vma) (field[3])) << 24)
	  |    (((elf_vma) (field[4])) << 32);
      else if (sizeof (elf_vma) == 4)
	/* We want to extract data from an 8 byte wide field and
	   place it into a 4 byte wide field.  Since this is a little
	   endian source we can just use the 4 byte extraction code.  */
	return  ((unsigned long) (field[0]))
	  |    (((unsigned long) (field[1])) << 8)
	  |    (((unsigned long) (field[2])) << 16)
	  |    (((unsigned long) (field[3])) << 24);
      /* Fall through.  */

    case 6:
      if (sizeof (elf_vma) == 8)
	return  ((elf_vma) (field[0]))
	  |    (((elf_vma) (field[1])) << 8)
	  |    (((elf_vma) (field[2])) << 16)
	  |    (((elf_vma) (field[3])) << 24)
	  |    (((elf_vma) (field[4])) << 32)
	  |    (((elf_vma) (field[5])) << 40);
      else if (sizeof (elf_vma) == 4)
	/* We want to extract data from an 8 byte wide field and
	   place it into a 4 byte wide field.  Since this is a little
	   endian source we can just use the 4 byte extraction code.  */
	return  ((unsigned long) (field[0]))
	  |    (((unsigned long) (field[1])) << 8)
	  |    (((unsigned long) (field[2])) << 16)
	  |    (((unsigned long) (field[3])) << 24);
      /* Fall through.  */

    case 7:
      if (sizeof (elf_vma) == 8)
	return  ((elf_vma) (field[0]))
	  |    (((elf_vma) (field[1])) << 8)
	  |    (((elf_vma) (field[2])) << 16)
	  |    (((elf_vma) (field[3])) << 24)
	  |    (((elf_vma) (field[4])) << 32)
	  |    (((elf_vma) (field[5])) << 40)
	  |    (((elf_vma) (field[6])) << 48);
      else if (sizeof (elf_vma) == 4)
	/* We want to extract data from an 8 byte wide field and
	   place it into a 4 byte wide field.  Since this is a little
	   endian source we can just use the 4 byte extraction code.  */
	return  ((unsigned long) (field[0]))
	  |    (((unsigned long) (field[1])) << 8)
	  |    (((unsigned long) (field[2])) << 16)
	  |    (((unsigned long) (field[3])) << 24);
      /* Fall through.  */

    case 8:
      if (sizeof (elf_vma) == 8)
	return  ((elf_vma) (field[0]))
	  |    (((elf_vma) (field[1])) << 8)
	  |    (((elf_vma) (field[2])) << 16)
	  |    (((elf_vma) (field[3])) << 24)
	  |    (((elf_vma) (field[4])) << 32)
	  |    (((elf_vma) (field[5])) << 40)
	  |    (((elf_vma) (field[6])) << 48)
	  |    (((elf_vma) (field[7])) << 56);
      else if (sizeof (elf_vma) == 4)
	/* We want to extract data from an 8 byte wide field and
	   place it into a 4 byte wide field.  Since this is a little
	   endian source we can just use the 4 byte extraction code.  */
	return  ((unsigned long) (field[0]))
	  |    (((unsigned long) (field[1])) << 8)
	  |    (((unsigned long) (field[2])) << 16)
	  |    (((unsigned long) (field[3])) << 24);
      /* Fall through.  */

    default:
      error (_("Unhandled data length: %d\n"), size);
      abort ();
    }
}

elf_vma
byte_get_big_endian (const unsigned char *field, int size)
{
  switch (size)
    {
    case 1:
      return *field;

    case 2:
      return ((unsigned int) (field[1])) | (((int) (field[0])) << 8);

    case 3:
      return ((unsigned long) (field[2]))
	|   (((unsigned long) (field[1])) << 8)
	|   (((unsigned long) (field[0])) << 16);

    case 4:
      return ((unsigned long) (field[3]))
	|   (((unsigned long) (field[2])) << 8)
	|   (((unsigned long) (field[1])) << 16)
	|   (((unsigned long) (field[0])) << 24);

    case 5:
      if (sizeof (elf_vma) == 8)
	return ((elf_vma) (field[4]))
	  |   (((elf_vma) (field[3])) << 8)
	  |   (((elf_vma) (field[2])) << 16)
	  |   (((elf_vma) (field[1])) << 24)
	  |   (((elf_vma) (field[0])) << 32);
      else if (sizeof (elf_vma) == 4)
	{
	  /* Although we are extracting data from an 8 byte wide field,
	     we are returning only 4 bytes of data.  */
	  field += 1;
	  return ((unsigned long) (field[3]))
	    |   (((unsigned long) (field[2])) << 8)
	    |   (((unsigned long) (field[1])) << 16)
	    |   (((unsigned long) (field[0])) << 24);
	}
      /* Fall through.  */

    case 6:
      if (sizeof (elf_vma) == 8)
	return ((elf_vma) (field[5]))
	  |   (((elf_vma) (field[4])) << 8)
	  |   (((elf_vma) (field[3])) << 16)
	  |   (((elf_vma) (field[2])) << 24)
	  |   (((elf_vma) (field[1])) << 32)
	  |   (((elf_vma) (field[0])) << 40);
      else if (sizeof (elf_vma) == 4)
	{
	  /* Although we are extracting data from an 8 byte wide field,
	     we are returning only 4 bytes of data.  */
	  field += 2;
	  return ((unsigned long) (field[3]))
	    |   (((unsigned long) (field[2])) << 8)
	    |   (((unsigned long) (field[1])) << 16)
	    |   (((unsigned long) (field[0])) << 24);
	}
      /* Fall through.  */

    case 7:
      if (sizeof (elf_vma) == 8)
	return ((elf_vma) (field[6]))
	  |   (((elf_vma) (field[5])) << 8)
	  |   (((elf_vma) (field[4])) << 16)
	  |   (((elf_vma) (field[3])) << 24)
	  |   (((elf_vma) (field[2])) << 32)
	  |   (((elf_vma) (field[1])) << 40)
	  |   (((elf_vma) (field[0])) << 48);
      else if (sizeof (elf_vma) == 4)
	{
	  /* Although we are extracting data from an 8 byte wide field,
	     we are returning only 4 bytes of data.  */
	  field += 3;
	  return ((unsigned long) (field[3]))
	    |   (((unsigned long) (field[2])) << 8)
	    |   (((unsigned long) (field[1])) << 16)
	    |   (((unsigned long) (field[0])) << 24);
	}
      /* Fall through.  */

    case 8:
      if (sizeof (elf_vma) == 8)
	return ((elf_vma) (field[7]))
	  |   (((elf_vma) (field[6])) << 8)
	  |   (((elf_vma) (field[5])) << 16)
	  |   (((elf_vma) (field[4])) << 24)
	  |   (((elf_vma) (field[3])) << 32)
	  |   (((elf_vma) (field[2])) << 40)
	  |   (((elf_vma) (field[1])) << 48)
	  |   (((elf_vma) (field[0])) << 56);
      else if (sizeof (elf_vma) == 4)
	{
	  /* Although we are extracting data from an 8 byte wide field,
	     we are returning only 4 bytes of data.  */
	  field += 4;
	  return ((unsigned long) (field[3]))
	    |   (((unsigned long) (field[2])) << 8)
	    |   (((unsigned long) (field[1])) << 16)
	    |   (((unsigned long) (field[0])) << 24);
	}
      /* Fall through.  */

    default:
      error (_("Unhandled data length: %d\n"), size);
      abort ();
    }
}

elf_vma
byte_get_signed (const unsigned char *field, int size)
{
  elf_vma x = byte_get (field, size);

  switch (size)
    {
    case 1:
      return (x ^ 0x80) - 0x80;
    case 2:
      return (x ^ 0x8000) - 0x8000;
    case 3:
      return (x ^ 0x800000) - 0x800000;
    case 4:
      return (x ^ 0x80000000) - 0x80000000;
    case 5:
    case 6:
    case 7:
    case 8:
      /* Reads of 5-, 6-, and 7-byte numbers are the result of
         trying to read past the end of a buffer, and will therefore
         not have meaningful values, so we don't try to deal with
         the sign in these cases.  */
      return x;
    default:
      abort ();
    }
}

/* Return the high-order 32-bits and the low-order 32-bits
   of an 8-byte value separately.  */

void
byte_get_64 (const unsigned char *field, elf_vma *high, elf_vma *low)
{
  if (byte_get == byte_get_big_endian)
    {
      *high = byte_get_big_endian (field, 4);
      *low = byte_get_big_endian (field + 4, 4);
    }
  else
    {
      *high = byte_get_little_endian (field + 4, 4);
      *low = byte_get_little_endian (field, 4);
    }
  return;
}

/* Return the path name for a proxy entry in a thin archive, adjusted
   relative to the path name of the thin archive itself if necessary.
   Always returns a pointer to malloc'ed memory.  */

char *
adjust_relative_path (const char *file_name, const char *name,
		      unsigned long name_len)
{
  char * member_file_name;
  const char * base_name = lbasename (file_name);
  size_t amt;

  /* This is a proxy entry for a thin archive member.
     If the extended name table contains an absolute path
     name, or if the archive is in the current directory,
     use the path name as given.  Otherwise, we need to
     find the member relative to the directory where the
     archive is located.  */
  if (IS_ABSOLUTE_PATH (name) || base_name == file_name)
    {
      amt = name_len + 1;
      if (amt == 0)
	return NULL;
      member_file_name = (char *) malloc (amt);
      if (member_file_name == NULL)
        {
          error (_("Out of memory\n"));
          return NULL;
        }
      memcpy (member_file_name, name, name_len);
      member_file_name[name_len] = '\0';
    }
  else
    {
      /* Concatenate the path components of the archive file name
         to the relative path name from the extended name table.  */
      size_t prefix_len = base_name - file_name;

      amt = prefix_len + name_len + 1;
      /* PR 17531: file: 2896dc8b
	 Catch wraparound.  */
      if (amt < prefix_len || amt < name_len)
	{
	  error (_("Abnormal length of thin archive member name: %lx\n"),
		 name_len);
	  return NULL;
	}

      member_file_name = (char *) malloc (amt);
      if (member_file_name == NULL)
        {
          error (_("Out of memory\n"));
          return NULL;
        }
      memcpy (member_file_name, file_name, prefix_len);
      memcpy (member_file_name + prefix_len, name, name_len);
      member_file_name[prefix_len + name_len] = '\0';
    }
  return member_file_name;
}

/* Processes the archive index table and symbol table in ARCH.
   Entries in the index table are SIZEOF_AR_INDEX bytes long.
   Fills in ARCH->next_arhdr_offset and ARCH->arhdr.
   If READ_SYMBOLS is true then fills in ARCH->index_num, ARCH->index_array,
    ARCH->sym_size and ARCH->sym_table.
   It is the caller's responsibility to free ARCH->index_array and
    ARCH->sym_table.
   Returns TRUE upon success, FALSE otherwise.
   If failure occurs an error message is printed.  */

static bfd_boolean
process_archive_index_and_symbols (struct archive_info *  arch,
				   unsigned int           sizeof_ar_index,
				   bfd_boolean            read_symbols)
{
  size_t got;
  unsigned long size;
  char fmag_save;

  fmag_save = arch->arhdr.ar_fmag[0];
  arch->arhdr.ar_fmag[0] = 0;
  size = strtoul (arch->arhdr.ar_size, NULL, 10);
  arch->arhdr.ar_fmag[0] = fmag_save;
  /* PR 17531: file: 912bd7de.  */
  if ((signed long) size < 0)
    {
      error (_("%s: invalid archive header size: %ld\n"),
	     arch->file_name, size);
      return FALSE;
    }

  size = size + (size & 1);

  arch->next_arhdr_offset += sizeof arch->arhdr + size;

  if (! read_symbols)
    {
      if (fseek (arch->file, size, SEEK_CUR) != 0)
	{
	  error (_("%s: failed to skip archive symbol table\n"),
		 arch->file_name);
	  return FALSE;
	}
    }
  else
    {
      unsigned long i;
      /* A buffer used to hold numbers read in from an archive index.
	 These are always SIZEOF_AR_INDEX bytes long and stored in
	 big-endian format.  */
      unsigned char integer_buffer[sizeof arch->index_num];
      unsigned char * index_buffer;

      assert (sizeof_ar_index <= sizeof integer_buffer);

      /* Check the size of the archive index.  */
      if (size < sizeof_ar_index)
	{
	  error (_("%s: the archive index is empty\n"), arch->file_name);
	  return FALSE;
	}

      /* Read the number of entries in the archive index.  */
      got = fread (integer_buffer, 1, sizeof_ar_index, arch->file);
      if (got != sizeof_ar_index)
	{
	  error (_("%s: failed to read archive index\n"), arch->file_name);
	  return FALSE;
	}

      arch->index_num = byte_get_big_endian (integer_buffer, sizeof_ar_index);
      size -= sizeof_ar_index;

      if (size < arch->index_num * sizeof_ar_index
	  /* PR 17531: file: 585515d1.  */
	  || size < arch->index_num)
	{
	  error (_("%s: the archive index is supposed to have 0x%lx entries of %d bytes, but the size is only 0x%lx\n"),
		 arch->file_name, (long) arch->index_num, sizeof_ar_index, size);
	  return FALSE;
	}

      /* Read in the archive index.  */
      index_buffer = (unsigned char *)
	malloc (arch->index_num * sizeof_ar_index);
      if (index_buffer == NULL)
	{
	  error (_("Out of memory whilst trying to read archive symbol index\n"));
	  return FALSE;
	}

      got = fread (index_buffer, sizeof_ar_index, arch->index_num, arch->file);
      if (got != arch->index_num)
	{
	  free (index_buffer);
	  error (_("%s: failed to read archive index\n"), arch->file_name);
	  return FALSE;
	}

      size -= arch->index_num * sizeof_ar_index;

      /* Convert the index numbers into the host's numeric format.  */
      arch->index_array = (elf_vma *)
	malloc (arch->index_num * sizeof (* arch->index_array));
      if (arch->index_array == NULL)
	{
	  free (index_buffer);
	  error (_("Out of memory whilst trying to convert the archive symbol index\n"));
	  return FALSE;
	}

      for (i = 0; i < arch->index_num; i++)
	arch->index_array[i] =
	  byte_get_big_endian ((unsigned char *) (index_buffer + (i * sizeof_ar_index)),
			       sizeof_ar_index);
      free (index_buffer);

      /* The remaining space in the header is taken up by the symbol table.  */
      if (size < 1)
	{
	  error (_("%s: the archive has an index but no symbols\n"),
		 arch->file_name);
	  return FALSE;
	}

      arch->sym_table = (char *) malloc (size);
      if (arch->sym_table == NULL)
	{
	  error (_("Out of memory whilst trying to read archive index symbol table\n"));
	  return FALSE;
	}

      arch->sym_size = size;
      got = fread (arch->sym_table, 1, size, arch->file);
      if (got != size)
	{
	  error (_("%s: failed to read archive index symbol table\n"),
		 arch->file_name);
	  return FALSE;
	}
    }

  /* Read the next archive header.  */
  got = fread (&arch->arhdr, 1, sizeof arch->arhdr, arch->file);
  if (got != sizeof arch->arhdr && got != 0)
    {
      error (_("%s: failed to read archive header following archive index\n"),
	     arch->file_name);
      return FALSE;
    }

  return TRUE;
}

/* Read the symbol table and long-name table from an archive.  */

int
setup_archive (struct archive_info *arch, const char *file_name,
	       FILE *file, bfd_boolean is_thin_archive,
	       bfd_boolean read_symbols)
{
  size_t got;

  arch->file_name = strdup (file_name);
  arch->file = file;
  arch->index_num = 0;
  arch->index_array = NULL;
  arch->sym_table = NULL;
  arch->sym_size = 0;
  arch->longnames = NULL;
  arch->longnames_size = 0;
  arch->nested_member_origin = 0;
  arch->is_thin_archive = is_thin_archive;
  arch->uses_64bit_indices = FALSE;
  arch->next_arhdr_offset = SARMAG;

  /* Read the first archive member header.  */
  if (fseek (file, SARMAG, SEEK_SET) != 0)
    {
      error (_("%s: failed to seek to first archive header\n"), file_name);
      return 1;
    }
  got = fread (&arch->arhdr, 1, sizeof arch->arhdr, file);
  if (got != sizeof arch->arhdr)
    {
      if (got == 0)
	return 0;

      error (_("%s: failed to read archive header\n"), file_name);
      return 1;
    }

  /* See if this is the archive symbol table.  */
  if (const_strneq (arch->arhdr.ar_name, "/               "))
    {
      if (! process_archive_index_and_symbols (arch, 4, read_symbols))
	return 1;
    }
  else if (const_strneq (arch->arhdr.ar_name, "/SYM64/         "))
    {
      arch->uses_64bit_indices = TRUE;
      if (! process_archive_index_and_symbols (arch, 8, read_symbols))
	return 1;
    }
  else if (read_symbols)
    printf (_("%s has no archive index\n"), file_name);

  if (const_strneq (arch->arhdr.ar_name, "//              "))
    {
      /* This is the archive string table holding long member names.  */
      char fmag_save = arch->arhdr.ar_fmag[0];
      arch->arhdr.ar_fmag[0] = 0;
      arch->longnames_size = strtoul (arch->arhdr.ar_size, NULL, 10);
      arch->arhdr.ar_fmag[0] = fmag_save;
      /* PR 17531: file: 01068045.  */
      if (arch->longnames_size < 8)
	{
	  error (_("%s: long name table is too small, (size = %ld)\n"),
		 file_name, arch->longnames_size);
	  return 1;
	}
      /* PR 17531: file: 639d6a26.  */
      if ((signed long) arch->longnames_size < 0)
	{
	  error (_("%s: long name table is too big, (size = 0x%lx)\n"),
		 file_name, arch->longnames_size);
	  return 1;
	}

      arch->next_arhdr_offset += sizeof arch->arhdr + arch->longnames_size;

      /* Plus one to allow for a string terminator.  */
      arch->longnames = (char *) malloc (arch->longnames_size + 1);
      if (arch->longnames == NULL)
	{
	  error (_("Out of memory reading long symbol names in archive\n"));
	  return 1;
	}

      if (fread (arch->longnames, arch->longnames_size, 1, file) != 1)
	{
	  free (arch->longnames);
	  arch->longnames = NULL;
	  error (_("%s: failed to read long symbol name string table\n"),
		 file_name);
	  return 1;
	}

      if ((arch->longnames_size & 1) != 0)
	getc (file);

      arch->longnames[arch->longnames_size] = 0;
    }

  return 0;
}

/* Open and setup a nested archive, if not already open.  */

int
setup_nested_archive (struct archive_info *nested_arch,
		      const char *member_file_name)
{
  FILE * member_file;

  /* Have we already setup this archive?  */
  if (nested_arch->file_name != NULL
      && streq (nested_arch->file_name, member_file_name))
    return 0;

  /* Close previous file and discard cached information.  */
  if (nested_arch->file != NULL)
    fclose (nested_arch->file);
  release_archive (nested_arch);

  member_file = fopen (member_file_name, "rb");
  if (member_file == NULL)
    return 1;
  return setup_archive (nested_arch, member_file_name, member_file,
			FALSE, FALSE);
}

/* Release the memory used for the archive information.  */

void
release_archive (struct archive_info * arch)
{
  if (arch->file_name != NULL)
    free (arch->file_name);
  if (arch->index_array != NULL)
    free (arch->index_array);
  if (arch->sym_table != NULL)
    free (arch->sym_table);
  if (arch->longnames != NULL)
    free (arch->longnames);
}

/* Get the name of an archive member from the current archive header.
   For simple names, this will modify the ar_name field of the current
   archive header.  For long names, it will return a pointer to the
   longnames table.  For nested archives, it will open the nested archive
   and get the name recursively.  NESTED_ARCH is a single-entry cache so
   we don't keep rereading the same information from a nested archive.  */

char *
get_archive_member_name (struct archive_info *arch,
                         struct archive_info *nested_arch)
{
  unsigned long j, k;

  if (arch->arhdr.ar_name[0] == '/')
    {
      /* We have a long name.  */
      char *endp;
      char *member_file_name;
      char *member_name;
      char fmag_save;

      if (arch->longnames == NULL || arch->longnames_size == 0)
	{
	  error (_("Archive member uses long names, but no longname table found\n"));
	  return NULL;
	}

      arch->nested_member_origin = 0;
      fmag_save = arch->arhdr.ar_fmag[0];
      arch->arhdr.ar_fmag[0] = 0;
      k = j = strtoul (arch->arhdr.ar_name + 1, &endp, 10);
      if (arch->is_thin_archive && endp != NULL && * endp == ':')
        arch->nested_member_origin = strtoul (endp + 1, NULL, 10);
      arch->arhdr.ar_fmag[0] = fmag_save;

      if (j > arch->longnames_size)
	{
	  error (_("Found long name index (%ld) beyond end of long name table\n"),j);
	  return NULL;
	}
      while ((j < arch->longnames_size)
             && (arch->longnames[j] != '\n')
             && (arch->longnames[j] != '\0'))
        j++;
      if (j > 0 && arch->longnames[j-1] == '/')
        j--;
      if (j > arch->longnames_size)
	j = arch->longnames_size;
      arch->longnames[j] = '\0';

      if (!arch->is_thin_archive || arch->nested_member_origin == 0)
        return arch->longnames + k;

      /* PR 17531: file: 2896dc8b.  */
      if (k >= j)
	{
	  error (_("Invalid Thin archive member name\n"));
	  return NULL;
	}

      /* This is a proxy for a member of a nested archive.
         Find the name of the member in that archive.  */
      member_file_name = adjust_relative_path (arch->file_name,
					       arch->longnames + k, j - k);
      if (member_file_name != NULL
          && setup_nested_archive (nested_arch, member_file_name) == 0)
	{
          member_name = get_archive_member_name_at (nested_arch,
						    arch->nested_member_origin,
						    NULL);
	  if (member_name != NULL)
	    {
	      free (member_file_name);
	      return member_name;
	    }
	}
      free (member_file_name);

      /* Last resort: just return the name of the nested archive.  */
      return arch->longnames + k;
    }

  /* We have a normal (short) name.  */
  for (j = 0; j < sizeof (arch->arhdr.ar_name); j++)
    if (arch->arhdr.ar_name[j] == '/')
      {
	arch->arhdr.ar_name[j] = '\0';
	return arch->arhdr.ar_name;
      }

  /* The full ar_name field is used.  Don't rely on ar_date starting
     with a zero byte.  */
  {
    char *name = xmalloc (sizeof (arch->arhdr.ar_name) + 1);
    memcpy (name, arch->arhdr.ar_name, sizeof (arch->arhdr.ar_name));
    name[sizeof (arch->arhdr.ar_name)] = '\0';
    return name;
  }
}

/* Get the name of an archive member at a given OFFSET within an archive
   ARCH.  */

char *
get_archive_member_name_at (struct archive_info *arch,
                            unsigned long offset,
			    struct archive_info *nested_arch)
{
  size_t got;

  if (fseek (arch->file, offset, SEEK_SET) != 0)
    {
      error (_("%s: failed to seek to next file name\n"), arch->file_name);
      return NULL;
    }
  got = fread (&arch->arhdr, 1, sizeof arch->arhdr, arch->file);
  if (got != sizeof arch->arhdr)
    {
      error (_("%s: failed to read archive header\n"), arch->file_name);
      return NULL;
    }
  if (memcmp (arch->arhdr.ar_fmag, ARFMAG, 2) != 0)
    {
      error (_("%s: did not find a valid archive header\n"),
	     arch->file_name);
      return NULL;
    }

  return get_archive_member_name (arch, nested_arch);
}

/* Construct a string showing the name of the archive member, qualified
   with the name of the containing archive file.  For thin archives, we
   use square brackets to denote the indirection.  For nested archives,
   we show the qualified name of the external member inside the square
   brackets (e.g., "thin.a[normal.a(foo.o)]").  */

char *
make_qualified_name (struct archive_info * arch,
		     struct archive_info * nested_arch,
		     const char *member_name)
{
  const char * error_name = _("<corrupt>");
  size_t len;
  char * name;

  len = strlen (arch->file_name) + strlen (member_name) + 3;
  if (arch->is_thin_archive
      && arch->nested_member_origin != 0)
    {
      /* PR 15140: Allow for corrupt thin archives.  */
      if (nested_arch->file_name)
	len += strlen (nested_arch->file_name) + 2;
      else
	len += strlen (error_name) + 2;
    }

  name = (char *) malloc (len);
  if (name == NULL)
    {
      error (_("Out of memory\n"));
      return NULL;
    }

  if (arch->is_thin_archive
      && arch->nested_member_origin != 0)
    {
      if (nested_arch->file_name)
	snprintf (name, len, "%s[%s(%s)]", arch->file_name,
		  nested_arch->file_name, member_name);
      else
	snprintf (name, len, "%s[%s(%s)]", arch->file_name,
		  error_name, member_name);
    }
  else if (arch->is_thin_archive)
    snprintf (name, len, "%s[%s]", arch->file_name, member_name);
  else
    snprintf (name, len, "%s(%s)", arch->file_name, member_name);

  return name;
}
