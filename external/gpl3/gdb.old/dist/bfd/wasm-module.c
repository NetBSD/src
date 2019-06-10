/* BFD back-end for WebAssembly modules.
   Copyright (C) 2017 Free Software Foundation, Inc.

   Based on srec.c, mmo.c, and binary.c

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
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

/* The WebAssembly module format is a simple object file format
   including up to 11 numbered sections, plus any number of named
   "custom" sections. It is described at:
   https://github.com/WebAssembly/design/blob/master/BinaryEncoding.md. */

#include "sysdep.h"
#include "alloca-conf.h"
#include "bfd.h"
#include "sysdep.h"
#include <limits.h>
#include "bfd_stdint.h"
#include "libiberty.h"
#include "libbfd.h"
#include "wasm-module.h"

typedef struct
{
  asymbol *      symbols;
  bfd_size_type  symcount;
} tdata_type;

static const char * const wasm_numbered_sections[] =
{
  NULL, /* Custom section, different layout.  */
  WASM_SECTION ( 1, "type"),
  WASM_SECTION ( 2, "import"),
  WASM_SECTION ( 3, "function"),
  WASM_SECTION ( 4, "table"),
  WASM_SECTION ( 5, "memory"),
  WASM_SECTION ( 6, "global"),
  WASM_SECTION ( 7, "export"),
  WASM_SECTION ( 8, "start"),
  WASM_SECTION ( 9, "element"),
  WASM_SECTION (10, "code"),
  WASM_SECTION (11, "data"),
};

#define WASM_NUMBERED_SECTIONS ARRAY_SIZE (wasm_numbered_sections)

/* Resolve SECTION_CODE to a section name if there is one, NULL
   otherwise.  */

static const char *
wasm_section_code_to_name (bfd_byte section_code)
{
  if (section_code < WASM_NUMBERED_SECTIONS)
    return wasm_numbered_sections[section_code];

  return NULL;
}

/* Translate section name NAME to a section code, or 0 if it's a
   custom name.  */

static unsigned int
wasm_section_name_to_code (const char *name)
{
  unsigned i;

  for (i = 1; i < WASM_NUMBERED_SECTIONS; i++)
    if (strcmp (name, wasm_numbered_sections[i]) == 0)
      return i;

  return 0;
}

/* WebAssembly LEB128 integers are sufficiently like DWARF LEB128
   integers that we use _bfd_safe_read_leb128, but there are two
   points of difference:

   - WebAssembly requires a 32-bit value to be encoded in at most 5
     bytes, etc.
   - _bfd_safe_read_leb128 accepts incomplete LEB128 encodings at the
     end of the buffer, while these are invalid in WebAssembly.

   Those differences mean that we will accept some files that are
   invalid WebAssembly.  */

/* Read an LEB128-encoded integer from ABFD's I/O stream, reading one
   byte at a time.  Set ERROR_RETURN if no complete integer could be
   read, LENGTH_RETURN to the number of bytes read (including bytes in
   incomplete numbers).  SIGN means interpret the number as SLEB128. */

static bfd_vma
wasm_read_leb128 (bfd *           abfd,
                  bfd_boolean *   error_return,
                  unsigned int *  length_return,
                  bfd_boolean     sign)
{
  bfd_vma result = 0;
  unsigned int num_read = 0;
  unsigned int shift = 0;
  unsigned char byte = 0;
  bfd_boolean success = FALSE;

  while (bfd_bread (&byte, 1, abfd) == 1)
    {
      num_read++;

      result |= ((bfd_vma) (byte & 0x7f)) << shift;

      shift += 7;
      if ((byte & 0x80) == 0)
        {
          success = TRUE;
          break;
        }
    }

  if (length_return != NULL)
    *length_return = num_read;
  if (error_return != NULL)
    *error_return = ! success;

  if (sign && (shift < 8 * sizeof (result)) && (byte & 0x40))
    result |= -((bfd_vma) 1 << shift);

  return result;
}

/* Encode an integer V as LEB128 and write it to ABFD, return TRUE on
   success.  */

static bfd_boolean
wasm_write_uleb128 (bfd *abfd, bfd_vma v)
{
  do
    {
      bfd_byte c = v & 0x7f;
      v >>= 7;

      if (v)
        c |= 0x80;

      if (bfd_bwrite (&c, 1, abfd) != 1)
        return FALSE;
    }
  while (v);

  return TRUE;
}

/* Read the LEB128 integer at P, saving it to X; at end of buffer,
   jump to error_return.  */
#define READ_LEB128(x, p, end)                                          \
  do                                                                    \
    {                                                                   \
      unsigned int length_read;                                         \
      (x) = _bfd_safe_read_leb128 (abfd, (p), &length_read,             \
                                   FALSE, (end));                       \
      (p) += length_read;                                               \
      if (length_read == 0)                                             \
        goto error_return;                                              \
    }                                                                   \
  while (0)

/* Verify the magic number at the beginning of a WebAssembly module
   ABFD, setting ERRORPTR if there's a mismatch.  */

static bfd_boolean
wasm_read_magic (bfd *abfd, bfd_boolean *errorptr)
{
  bfd_byte magic_const[SIZEOF_WASM_MAGIC] = WASM_MAGIC;
  bfd_byte magic[SIZEOF_WASM_MAGIC];

  if (bfd_bread (magic, sizeof (magic), abfd) == sizeof (magic)
      && memcmp (magic, magic_const, sizeof (magic)) == 0)
    return TRUE;

  *errorptr = TRUE;
  return FALSE;
}

/* Read the version number from ABFD, returning TRUE if it's a supported
   version. Set ERRORPTR otherwise.  */

static bfd_boolean
wasm_read_version (bfd *abfd, bfd_boolean *errorptr)
{
  bfd_byte vers_const[SIZEOF_WASM_VERSION] = WASM_VERSION;
  bfd_byte vers[SIZEOF_WASM_VERSION];

  if (bfd_bread (vers, sizeof (vers), abfd) == sizeof (vers)
      /* Don't attempt to parse newer versions, which are likely to
	 require code changes.  */
      && memcmp (vers, vers_const, sizeof (vers)) == 0)
    return TRUE;

  *errorptr = TRUE;
  return FALSE;
}

/* Read the WebAssembly header (magic number plus version number) from
   ABFD, setting ERRORPTR to TRUE if there is a mismatch.  */

static bfd_boolean
wasm_read_header (bfd *abfd, bfd_boolean *errorptr)
{
  if (! wasm_read_magic (abfd, errorptr))
    return FALSE;

  if (! wasm_read_version (abfd, errorptr))
    return FALSE;

  return TRUE;
}

/* Scan the "function" subsection of the "name" section ASECT in the
   wasm module ABFD. Create symbols. Return TRUE on success.  */

static bfd_boolean
wasm_scan_name_function_section (bfd *abfd, sec_ptr asect)
{
  bfd_byte *p;
  bfd_byte *end;
  bfd_vma payload_size;
  bfd_vma symcount = 0;
  tdata_type *tdata = abfd->tdata.any;
  asymbol *symbols = NULL;
  sec_ptr space_function_index;

  if (! asect)
    return FALSE;

  if (strcmp (asect->name, WASM_NAME_SECTION) != 0)
    return FALSE;

  p = asect->contents;
  end = asect->contents + asect->size;

  if (! p)
    return FALSE;

  while (p < end)
    {
      bfd_byte subsection_code = *p++;
      if (subsection_code == WASM_FUNCTION_SUBSECTION)
        break;

      /* subsection_code is documented to be a varuint7, meaning that
         it has to be a single byte in the 0 - 127 range.  If it isn't,
         the spec must have changed underneath us, so give up.  */
      if (subsection_code & 0x80)
        return FALSE;

      READ_LEB128 (payload_size, p, end);

      if (p > p + payload_size)
        return FALSE;

      p += payload_size;
    }

  if (p >= end)
    return FALSE;

  READ_LEB128 (payload_size, p, end);

  if (p > p + payload_size)
    return FALSE;

  if (p + payload_size > end)
    return FALSE;

  end = p + payload_size;

  READ_LEB128 (symcount, p, end);

  /* Sanity check: each symbol has at least two bytes.  */
  if (symcount > payload_size/2)
    return FALSE;

  tdata->symcount = symcount;

  space_function_index = bfd_make_section_with_flags
    (abfd, WASM_SECTION_FUNCTION_INDEX, SEC_READONLY | SEC_CODE);

  if (! space_function_index)
    space_function_index = bfd_get_section_by_name (abfd, WASM_SECTION_FUNCTION_INDEX);

  if (! space_function_index)
    return FALSE;

  symbols = bfd_zalloc (abfd, tdata->symcount * sizeof (asymbol));
  if (! symbols)
    return FALSE;

  for (symcount = 0; p < end && symcount < tdata->symcount; symcount++)
    {
      bfd_vma index;
      bfd_vma len;
      char *name;
      asymbol *sym;

      READ_LEB128 (index, p, end);
      READ_LEB128 (len, p, end);

      if (p + len < p || p + len > end)
        goto error_return;

      name = bfd_zalloc (abfd, len + 1);
      if (! name)
        goto error_return;

      memcpy (name, p, len);
      p += len;

      sym = &symbols[symcount];
      sym->the_bfd = abfd;
      sym->name = name;
      sym->value = index;
      sym->flags = BSF_GLOBAL | BSF_FUNCTION;
      sym->section = space_function_index;
      sym->udata.p = NULL;
    }

  if (symcount < tdata->symcount)
    goto error_return;

  tdata->symbols = symbols;
  abfd->symcount = symcount;

  return TRUE;

 error_return:
  while (symcount)
    bfd_release (abfd, (void *)symbols[--symcount].name);
  bfd_release (abfd, symbols);
  return FALSE;
}

/* Read a byte from ABFD and return it, or EOF for EOF or error.
   Set ERRORPTR on non-EOF error.  */

static int
wasm_read_byte (bfd *abfd, bfd_boolean *errorptr)
{
  bfd_byte byte;

  if (bfd_bread (&byte, (bfd_size_type) 1, abfd) != 1)
    {
      if (bfd_get_error () != bfd_error_file_truncated)
        *errorptr = TRUE;
      return EOF;
    }

  return byte;
}

/* Scan the wasm module ABFD, creating sections and symbols.
   Return TRUE on success.  */

static bfd_boolean
wasm_scan (bfd *abfd)
{
  bfd_boolean error = FALSE;
  /* Fake VMAs for now. Choose 0x80000000 as base to avoid clashes
     with actual data addresses.  */
  bfd_vma vma = 0x80000000;
  int section_code;
  unsigned int bytes_read;
  char *name = NULL;
  asection *bfdsec;

  if (bfd_seek (abfd, (file_ptr) 0, SEEK_SET) != 0)
    goto error_return;

  if (! wasm_read_header (abfd, &error))
    goto error_return;

  while ((section_code = wasm_read_byte (abfd, &error)) != EOF)
    {
      if (section_code != 0)
        {
          const char *sname = wasm_section_code_to_name (section_code);

          if (! sname)
            goto error_return;

          name = strdup (sname);
          bfdsec = bfd_make_section_anyway_with_flags (abfd, name, SEC_HAS_CONTENTS);
          if (bfdsec == NULL)
            goto error_return;
          name = NULL;

          bfdsec->vma = vma;
          bfdsec->lma = vma;
          bfdsec->size = wasm_read_leb128 (abfd, &error, &bytes_read, FALSE);
          if (error)
            goto error_return;
          bfdsec->filepos = bfd_tell (abfd);
          bfdsec->alignment_power = 0;
        }
      else
        {
          bfd_vma payload_len;
          file_ptr section_start;
          bfd_vma namelen;
          char *prefix = WASM_SECTION_PREFIX;
          char *p;
          int ret;

          payload_len = wasm_read_leb128 (abfd, &error, &bytes_read, FALSE);
          if (error)
            goto error_return;
          section_start = bfd_tell (abfd);
          namelen = wasm_read_leb128 (abfd, &error, &bytes_read, FALSE);
          if (error || namelen > payload_len)
            goto error_return;
          name = bfd_zmalloc (namelen + strlen (prefix) + 1);
          if (! name)
            goto error_return;
          p = name;
          ret = sprintf (p, "%s", prefix);
          if (ret < 0 || (bfd_vma) ret != strlen (prefix))
            goto error_return;
          p += ret;
          if (bfd_bread (p, namelen, abfd) != namelen)
	    goto error_return;

          bfdsec = bfd_make_section_anyway_with_flags (abfd, name, SEC_HAS_CONTENTS);
          if (bfdsec == NULL)
            goto error_return;
          name = NULL;

          bfdsec->vma = vma;
          bfdsec->lma = vma;
          bfdsec->filepos = bfd_tell (abfd);
          bfdsec->size = section_start + payload_len - bfdsec->filepos;
          bfdsec->alignment_power = 0;
        }

      if (bfdsec->size != 0)
        {
          bfdsec->contents = bfd_zalloc (abfd, bfdsec->size);
          if (! bfdsec->contents)
            goto error_return;

          if (bfd_bread (bfdsec->contents, bfdsec->size, abfd) != bfdsec->size)
	    goto error_return;
        }

      vma += bfdsec->size;
    }

  /* Make sure we're at actual EOF.  There's no indication in the
     WebAssembly format of how long the file is supposed to be.  */
  if (error)
    goto error_return;

  return TRUE;

 error_return:
  if (name)
    free (name);

  for (bfdsec = abfd->sections; bfdsec; bfdsec = bfdsec->next)
    free ((void *) bfdsec->name);

  return FALSE;
}

/* Put a numbered section ASECT of ABFD into the table of numbered
   sections pointed to by FSARG.  */

static void
wasm_register_section (bfd *abfd ATTRIBUTE_UNUSED,
                       asection *asect,
		       void *fsarg)
{
  sec_ptr *numbered_sections = fsarg;
  int index = wasm_section_name_to_code (asect->name);

  if (index == 0)
    return;

  numbered_sections[index] = asect;
}

struct compute_section_arg
{
  bfd_vma pos;
  bfd_boolean failed;
};

/* Compute the file position of ABFD's section ASECT.  FSARG is a
   pointer to the current file position.

   We allow section names of the form .wasm.id to encode the numbered
   section with ID id, if it exists; otherwise, a custom section with
   ID "id" is produced.  Arbitrary section names are for sections that
   are assumed already to contain a section header; those are appended
   to the WebAssembly module verbatim.  */

static void
wasm_compute_custom_section_file_position (bfd *abfd,
					   sec_ptr asect,
                                           void *fsarg)
{
  struct compute_section_arg *fs = fsarg;
  int index;

  if (fs->failed)
    return;

  index = wasm_section_name_to_code (asect->name);

  if (index != 0)
    return;

  if (CONST_STRNEQ (asect->name, WASM_SECTION_PREFIX))
    {
      const char *name = asect->name + strlen (WASM_SECTION_PREFIX);
      bfd_size_type payload_len = asect->size;
      bfd_size_type name_len = strlen (name);
      bfd_size_type nl = name_len;

      payload_len += name_len;

      do
        {
          payload_len++;
          nl >>= 7;
        }
      while (nl);

      bfd_seek (abfd, fs->pos, SEEK_SET);
      if (! wasm_write_uleb128 (abfd, 0)
          || ! wasm_write_uleb128 (abfd, payload_len)
          || ! wasm_write_uleb128 (abfd, name_len)
          || bfd_bwrite (name, name_len, abfd) != name_len)
        goto error_return;
      fs->pos = asect->filepos = bfd_tell (abfd);
    }
  else
    {
      asect->filepos = fs->pos;
    }


  fs->pos += asect->size;
  return;

 error_return:
  fs->failed = TRUE;
}

/* Compute the file positions for the sections of ABFD.  Currently,
   this writes all numbered sections first, in order, then all custom
   sections, in section order.

   The spec says that the numbered sections must appear in order of
   their ids, but custom sections can appear in any position and any
   order, and more than once. FIXME: support that.  */

static bfd_boolean
wasm_compute_section_file_positions (bfd *abfd)
{
  bfd_byte magic[SIZEOF_WASM_MAGIC] = WASM_MAGIC;
  bfd_byte vers[SIZEOF_WASM_VERSION] = WASM_VERSION;
  sec_ptr numbered_sections[WASM_NUMBERED_SECTIONS];
  struct compute_section_arg fs;
  unsigned int i;

  bfd_seek (abfd, (bfd_vma) 0, SEEK_SET);

  if (bfd_bwrite (magic, sizeof (magic), abfd) != (sizeof magic)
      || bfd_bwrite (vers, sizeof (vers), abfd) != sizeof (vers))
    return FALSE;

  for (i = 0; i < WASM_NUMBERED_SECTIONS; i++)
    numbered_sections[i] = NULL;

  bfd_map_over_sections (abfd, wasm_register_section, numbered_sections);

  fs.pos = bfd_tell (abfd);
  for (i = 0; i < WASM_NUMBERED_SECTIONS; i++)
    {
      sec_ptr sec = numbered_sections[i];
      bfd_size_type size;

      if (! sec)
        continue;
      size = sec->size;
      if (bfd_seek (abfd, fs.pos, SEEK_SET) != 0)
        return FALSE;
      if (! wasm_write_uleb128 (abfd, i)
          || ! wasm_write_uleb128 (abfd, size))
        return FALSE;
      fs.pos = sec->filepos = bfd_tell (abfd);
      fs.pos += size;
    }

  fs.failed = FALSE;

  bfd_map_over_sections (abfd, wasm_compute_custom_section_file_position, &fs);

  if (fs.failed)
    return FALSE;

  abfd->output_has_begun = TRUE;

  return TRUE;
}

static bfd_boolean
wasm_set_section_contents (bfd *abfd,
                           sec_ptr section,
                           const void *location,
                           file_ptr offset,
                           bfd_size_type count)
{
  if (count == 0)
    return TRUE;

  if (! abfd->output_has_begun
      && ! wasm_compute_section_file_positions (abfd))
    return FALSE;

  if (bfd_seek (abfd, section->filepos + offset, SEEK_SET) != 0
      || bfd_bwrite (location, count, abfd) != count)
    return FALSE;

  return TRUE;
}

static bfd_boolean
wasm_write_object_contents (bfd* abfd)
{
  bfd_byte magic[] = WASM_MAGIC;
  bfd_byte vers[] = WASM_VERSION;

  if (bfd_seek (abfd, 0, SEEK_SET) != 0)
    return FALSE;

  if (bfd_bwrite (magic, sizeof (magic), abfd) != sizeof (magic)
      || bfd_bwrite (vers, sizeof (vers), abfd) != sizeof (vers))
    return FALSE;

  return TRUE;
}

static bfd_boolean
wasm_mkobject (bfd *abfd)
{
  tdata_type *tdata = (tdata_type *) bfd_alloc (abfd, sizeof (tdata_type));

  if (! tdata)
    return FALSE;

  tdata->symbols = NULL;
  tdata->symcount = 0;

  abfd->tdata.any = tdata;

  return TRUE;
}

static long
wasm_get_symtab_upper_bound (bfd *abfd)
{
  tdata_type *tdata = abfd->tdata.any;

  return (tdata->symcount + 1) * (sizeof (asymbol *));
}

static long
wasm_canonicalize_symtab (bfd *abfd, asymbol **alocation)
{
  tdata_type *tdata = abfd->tdata.any;
  size_t i;

  for (i = 0; i < tdata->symcount; i++)
    alocation[i] = &tdata->symbols[i];
  alocation[i] = NULL;

  return tdata->symcount;
}

static asymbol *
wasm_make_empty_symbol (bfd *abfd)
{
  bfd_size_type amt = sizeof (asymbol);
  asymbol *new_symbol = (asymbol *) bfd_zalloc (abfd, amt);

  if (! new_symbol)
    return NULL;
  new_symbol->the_bfd = abfd;
  return new_symbol;
}

static void
wasm_print_symbol (bfd *abfd,
                   void * filep,
                   asymbol *symbol,
                   bfd_print_symbol_type how)
{
  FILE *file = (FILE *) filep;

  switch (how)
    {
    case bfd_print_symbol_name:
      fprintf (file, "%s", symbol->name);
      break;

    default:
      bfd_print_symbol_vandf (abfd, filep, symbol);
      fprintf (file, " %-5s %s", symbol->section->name, symbol->name);
    }
}

static void
wasm_get_symbol_info (bfd *abfd ATTRIBUTE_UNUSED,
                      asymbol *symbol,
                      symbol_info *ret)
{
  bfd_symbol_info (symbol, ret);
}

/* Check whether ABFD is a WebAssembly module; if so, scan it.  */

static const bfd_target *
wasm_object_p (bfd *abfd)
{
  bfd_boolean error;

  if (bfd_seek (abfd, (file_ptr) 0, SEEK_SET) != 0)
    return NULL;

  if (! wasm_read_header (abfd, &error))
    {
      bfd_set_error (bfd_error_wrong_format);
      return NULL;
    }

  if (! wasm_mkobject (abfd) || ! wasm_scan (abfd))
    return NULL;

  if (! bfd_default_set_arch_mach (abfd, bfd_arch_wasm32, 0))
    return NULL;

  if (wasm_scan_name_function_section (abfd, bfd_get_section_by_name (abfd, WASM_NAME_SECTION)))
    abfd->flags |= HAS_SYMS;

  return abfd->xvec;
}

/* BFD_JUMP_TABLE_WRITE */
#define wasm_set_arch_mach                _bfd_generic_set_arch_mach

/* BFD_JUMP_TABLE_SYMBOLS */
#define wasm_get_symbol_version_string    _bfd_nosymbols_get_symbol_version_string
#define wasm_bfd_is_local_label_name       bfd_generic_is_local_label_name
#define wasm_bfd_is_target_special_symbol ((bfd_boolean (*) (bfd *, asymbol *)) bfd_false)
#define wasm_get_lineno                   _bfd_nosymbols_get_lineno
#define wasm_find_nearest_line            _bfd_nosymbols_find_nearest_line
#define wasm_find_line                    _bfd_nosymbols_find_line
#define wasm_find_inliner_info            _bfd_nosymbols_find_inliner_info
#define wasm_bfd_make_debug_symbol        _bfd_nosymbols_bfd_make_debug_symbol
#define wasm_read_minisymbols             _bfd_generic_read_minisymbols
#define wasm_minisymbol_to_symbol         _bfd_generic_minisymbol_to_symbol

const bfd_target wasm_vec =
{
  "wasm",               	/* Name.  */
  bfd_target_unknown_flavour,
  BFD_ENDIAN_LITTLE,
  BFD_ENDIAN_LITTLE,
  (HAS_SYMS | WP_TEXT),		/* Object flags.  */
  (SEC_CODE | SEC_DATA | SEC_HAS_CONTENTS), /* Section flags.  */
  0,                    	/* Leading underscore.  */
  ' ',                  	/* AR_pad_char.  */
  255,                  	/* AR_max_namelen.  */
  0,				/* Match priority.  */
  /* Routines to byte-swap various sized integers from the data sections.  */
  bfd_getl64, bfd_getl_signed_64, bfd_putl64,
  bfd_getl32, bfd_getl_signed_32, bfd_putl32,
  bfd_getl16, bfd_getl_signed_16, bfd_putl16,

  /* Routines to byte-swap various sized integers from the file headers.  */
  bfd_getl64, bfd_getl_signed_64, bfd_putl64,
  bfd_getl32, bfd_getl_signed_32, bfd_putl32,
  bfd_getl16, bfd_getl_signed_16, bfd_putl16,

  {
    _bfd_dummy_target,
    wasm_object_p,		/* bfd_check_format.  */
    _bfd_dummy_target,
    _bfd_dummy_target,
  },
  {
    bfd_false,
    wasm_mkobject,
    _bfd_generic_mkarchive,
    bfd_false,
  },
  {				/* bfd_write_contents.  */
    bfd_false,
    wasm_write_object_contents,
    _bfd_write_archive_contents,
    bfd_false,
  },

  BFD_JUMP_TABLE_GENERIC (_bfd_generic),
  BFD_JUMP_TABLE_COPY (_bfd_generic),
  BFD_JUMP_TABLE_CORE (_bfd_nocore),
  BFD_JUMP_TABLE_ARCHIVE (_bfd_noarchive),
  BFD_JUMP_TABLE_SYMBOLS (wasm),
  BFD_JUMP_TABLE_RELOCS (_bfd_norelocs),
  BFD_JUMP_TABLE_WRITE (wasm),
  BFD_JUMP_TABLE_LINK (_bfd_nolink),
  BFD_JUMP_TABLE_DYNAMIC (_bfd_nodynamic),

  NULL,

  NULL,
};
