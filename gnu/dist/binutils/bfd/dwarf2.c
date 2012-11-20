/* DWARF 2 support.
   Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
   2004, 2005 Free Software Foundation, Inc.

   Adapted from gdb/dwarf2read.c by Gavin Koch of Cygnus Solutions
   (gavin@cygnus.com).

   From the dwarf2read.c header:
   Adapted by Gary Funck (gary@intrepid.com), Intrepid Technology,
   Inc.  with support from Florida State University (under contract
   with the Ada Joint Program Office), and Silicon Graphics, Inc.
   Initial contribution by Brent Benson, Harris Computer Systems, Inc.,
   based on Fred Fish's (Cygnus Support) implementation of DWARF 1
   support in dwarfread.c

   This file is part of BFD.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "bfd.h"
#include "sysdep.h"
#include "libiberty.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/dwarf2.h"

/* The data in the .debug_line statement prologue looks like this.  */

struct line_head
{
  bfd_vma total_length;
  unsigned short version;
  bfd_vma prologue_length;
  unsigned char minimum_instruction_length;
  unsigned char default_is_stmt;
  int line_base;
  unsigned char line_range;
  unsigned char opcode_base;
  unsigned char *standard_opcode_lengths;
};

/* Attributes have a name and a value.  */

struct attribute
{
  enum dwarf_attribute name;
  enum dwarf_form form;
  union
  {
    char *str;
    struct dwarf_block *blk;
    bfd_uint64_t val;
    bfd_int64_t sval;
  }
  u;
};

/* Blocks are a bunch of untyped bytes.  */
struct dwarf_block
{
  unsigned int size;
  bfd_byte *data;
};

struct dwarf2_debug
{
  /* A list of all previously read comp_units.  */
  struct comp_unit *all_comp_units;

  /* The next unread compilation unit within the .debug_info section.
     Zero indicates that the .debug_info section has not been loaded
     into a buffer yet.  */
  bfd_byte *info_ptr;

  /* Pointer to the end of the .debug_info section memory buffer.  */
  bfd_byte *info_ptr_end;

  /* Pointer to the section and address of the beginning of the
     section.  */
  asection *sec;
  bfd_byte *sec_info_ptr;

  /* Pointer to the symbol table.  */
  asymbol **syms;

  /* Pointer to the .debug_abbrev section loaded into memory.  */
  bfd_byte *dwarf_abbrev_buffer;

  /* Length of the loaded .debug_abbrev section.  */
  unsigned long dwarf_abbrev_size;

  /* Buffer for decode_line_info.  */
  bfd_byte *dwarf_line_buffer;

  /* Length of the loaded .debug_line section.  */
  unsigned long dwarf_line_size;

  /* Pointer to the .debug_str section loaded into memory.  */
  bfd_byte *dwarf_str_buffer;

  /* Length of the loaded .debug_str section.  */
  unsigned long dwarf_str_size;
};

struct arange
{
  struct arange *next;
  bfd_vma low;
  bfd_vma high;
};

/* A minimal decoding of DWARF2 compilation units.  We only decode
   what's needed to get to the line number information.  */

struct comp_unit
{
  /* Chain the previously read compilation units.  */
  struct comp_unit *next_unit;

  /* Keep the bdf convenient (for memory allocation).  */
  bfd *abfd;

  /* The lowest and higest addresses contained in this compilation
     unit as specified in the compilation unit header.  */
  struct arange arange;

  /* The DW_AT_name attribute (for error messages).  */
  char *name;

  /* The abbrev hash table.  */
  struct abbrev_info **abbrevs;

  /* Note that an error was found by comp_unit_find_nearest_line.  */
  int error;

  /* The DW_AT_comp_dir attribute.  */
  char *comp_dir;

  /* TRUE if there is a line number table associated with this comp. unit.  */
  int stmtlist;

  /* Pointer to the current comp_unit so that we can find a given entry
     by its reference.  */
  bfd_byte *info_ptr_unit;

  /* The offset into .debug_line of the line number table.  */
  unsigned long line_offset;

  /* Pointer to the first child die for the comp unit.  */
  bfd_byte *first_child_die_ptr;

  /* The end of the comp unit.  */
  bfd_byte *end_ptr;

  /* The decoded line number, NULL if not yet decoded.  */
  struct line_info_table *line_table;

  /* A list of the functions found in this comp. unit.  */
  struct funcinfo *function_table;

  /* Pointer to dwarf2_debug structure.  */
  struct dwarf2_debug *stash;

  /* Address size for this unit - from unit header.  */
  unsigned char addr_size;

  /* Offset size for this unit - from unit header.  */
  unsigned char offset_size;
};

/* This data structure holds the information of an abbrev.  */
struct abbrev_info
{
  unsigned int number;		/* Number identifying abbrev.  */
  enum dwarf_tag tag;		/* DWARF tag.  */
  int has_children;		/* Boolean.  */
  unsigned int num_attrs;	/* Number of attributes.  */
  struct attr_abbrev *attrs;	/* An array of attribute descriptions.  */
  struct abbrev_info *next;	/* Next in chain.  */
};

struct attr_abbrev
{
  enum dwarf_attribute name;
  enum dwarf_form form;
};

#ifndef ABBREV_HASH_SIZE
#define ABBREV_HASH_SIZE 121
#endif
#ifndef ATTR_ALLOC_CHUNK
#define ATTR_ALLOC_CHUNK 4
#endif

/* VERBATIM
   The following function up to the END VERBATIM mark are
   copied directly from dwarf2read.c.  */

/* Read dwarf information from a buffer.  */

static unsigned int
read_1_byte (bfd *abfd ATTRIBUTE_UNUSED, bfd_byte *buf)
{
  return bfd_get_8 (abfd, buf);
}

static int
read_1_signed_byte (bfd *abfd ATTRIBUTE_UNUSED, bfd_byte *buf)
{
  return bfd_get_signed_8 (abfd, buf);
}

static unsigned int
read_2_bytes (bfd *abfd, bfd_byte *buf)
{
  return bfd_get_16 (abfd, buf);
}

static unsigned int
read_4_bytes (bfd *abfd, bfd_byte *buf)
{
  return bfd_get_32 (abfd, buf);
}

static bfd_uint64_t
read_8_bytes (bfd *abfd, bfd_byte *buf)
{
  return bfd_get_64 (abfd, buf);
}

static bfd_byte *
read_n_bytes (bfd *abfd ATTRIBUTE_UNUSED,
	      bfd_byte *buf,
	      unsigned int size ATTRIBUTE_UNUSED)
{
  /* If the size of a host char is 8 bits, we can return a pointer
     to the buffer, otherwise we have to copy the data to a buffer
     allocated on the temporary obstack.  */
  return buf;
}

static char *
read_string (bfd *abfd ATTRIBUTE_UNUSED,
	     bfd_byte *buf,
	     unsigned int *bytes_read_ptr)
{
  /* Return a pointer to the embedded string.  */
  char *str = (char *) buf;
  if (*str == '\0')
    {
      *bytes_read_ptr = 1;
      return NULL;
    }

  *bytes_read_ptr = strlen (str) + 1;
  return str;
}

static char *
read_indirect_string (struct comp_unit* unit,
		      bfd_byte *buf,
		      unsigned int *bytes_read_ptr)
{
  bfd_uint64_t offset;
  struct dwarf2_debug *stash = unit->stash;
  char *str;

  if (unit->offset_size == 4)
    offset = read_4_bytes (unit->abfd, buf);
  else
    offset = read_8_bytes (unit->abfd, buf);
  *bytes_read_ptr = unit->offset_size;

  if (! stash->dwarf_str_buffer)
    {
      asection *msec;
      bfd *abfd = unit->abfd;
      bfd_size_type sz;

      msec = bfd_get_section_by_name (abfd, ".debug_str");
      if (! msec)
	{
	  (*_bfd_error_handler)
	    (_("Dwarf Error: Can't find .debug_str section."));
	  bfd_set_error (bfd_error_bad_value);
	  return NULL;
	}

      sz = msec->rawsize ? msec->rawsize : msec->size;
      stash->dwarf_str_size = sz;
      stash->dwarf_str_buffer = bfd_alloc (abfd, sz);
      if (! stash->dwarf_abbrev_buffer)
	return NULL;

      if (! bfd_get_section_contents (abfd, msec, stash->dwarf_str_buffer,
				      0, sz))
	return NULL;
    }

  if (offset >= stash->dwarf_str_size)
    {
      (*_bfd_error_handler) (_("Dwarf Error: DW_FORM_strp offset (%lu) greater than or equal to .debug_str size (%lu)."),
			     (unsigned long) offset, stash->dwarf_str_size);
      bfd_set_error (bfd_error_bad_value);
      return NULL;
    }

  str = (char *) stash->dwarf_str_buffer + offset;
  if (*str == '\0')
    return NULL;
  return str;
}

/* END VERBATIM */

static bfd_uint64_t
read_address (struct comp_unit *unit, bfd_byte *buf)
{
  switch (unit->addr_size)
    {
    case 8:
      return bfd_get_64 (unit->abfd, buf);
    case 4:
      return bfd_get_32 (unit->abfd, buf);
    case 2:
      return bfd_get_16 (unit->abfd, buf);
    default:
      abort ();
    }
}

/* Lookup an abbrev_info structure in the abbrev hash table.  */

static struct abbrev_info *
lookup_abbrev (unsigned int number, struct abbrev_info **abbrevs)
{
  unsigned int hash_number;
  struct abbrev_info *abbrev;

  hash_number = number % ABBREV_HASH_SIZE;
  abbrev = abbrevs[hash_number];

  while (abbrev)
    {
      if (abbrev->number == number)
	return abbrev;
      else
	abbrev = abbrev->next;
    }

  return NULL;
}

/* In DWARF version 2, the description of the debugging information is
   stored in a separate .debug_abbrev section.  Before we read any
   dies from a section we read in all abbreviations and install them
   in a hash table.  */

static struct abbrev_info**
read_abbrevs (bfd *abfd, bfd_uint64_t offset, struct dwarf2_debug *stash)
{
  struct abbrev_info **abbrevs;
  bfd_byte *abbrev_ptr;
  struct abbrev_info *cur_abbrev;
  unsigned int abbrev_number, bytes_read, abbrev_name;
  unsigned int abbrev_form, hash_number;
  bfd_size_type amt;

  if (! stash->dwarf_abbrev_buffer)
    {
      asection *msec;

      msec = bfd_get_section_by_name (abfd, ".debug_abbrev");
      if (! msec)
	{
	  (*_bfd_error_handler) (_("Dwarf Error: Can't find .debug_abbrev section."));
	  bfd_set_error (bfd_error_bad_value);
	  return 0;
	}

      stash->dwarf_abbrev_size = msec->size;
      stash->dwarf_abbrev_buffer
	= bfd_simple_get_relocated_section_contents (abfd, msec, NULL,
						     stash->syms);
      if (! stash->dwarf_abbrev_buffer)
	  return 0;
    }

  if (offset >= stash->dwarf_abbrev_size)
    {
      (*_bfd_error_handler) (_("Dwarf Error: Abbrev offset (%lu) greater than or equal to .debug_abbrev size (%lu)."),
			     (unsigned long) offset, stash->dwarf_abbrev_size);
      bfd_set_error (bfd_error_bad_value);
      return 0;
    }

  amt = sizeof (struct abbrev_info*) * ABBREV_HASH_SIZE;
  abbrevs = bfd_zalloc (abfd, amt);

  abbrev_ptr = stash->dwarf_abbrev_buffer + offset;
  abbrev_number = read_unsigned_leb128 (abfd, abbrev_ptr, &bytes_read);
  abbrev_ptr += bytes_read;

  /* Loop until we reach an abbrev number of 0.  */
  while (abbrev_number)
    {
      amt = sizeof (struct abbrev_info);
      cur_abbrev = bfd_zalloc (abfd, amt);

      /* Read in abbrev header.  */
      cur_abbrev->number = abbrev_number;
      cur_abbrev->tag = (enum dwarf_tag)
	read_unsigned_leb128 (abfd, abbrev_ptr, &bytes_read);
      abbrev_ptr += bytes_read;
      cur_abbrev->has_children = read_1_byte (abfd, abbrev_ptr);
      abbrev_ptr += 1;

      /* Now read in declarations.  */
      abbrev_name = read_unsigned_leb128 (abfd, abbrev_ptr, &bytes_read);
      abbrev_ptr += bytes_read;
      abbrev_form = read_unsigned_leb128 (abfd, abbrev_ptr, &bytes_read);
      abbrev_ptr += bytes_read;

      while (abbrev_name)
	{
	  if ((cur_abbrev->num_attrs % ATTR_ALLOC_CHUNK) == 0)
	    {
	      amt = cur_abbrev->num_attrs + ATTR_ALLOC_CHUNK;
	      amt *= sizeof (struct attr_abbrev);
	      cur_abbrev->attrs = bfd_realloc (cur_abbrev->attrs, amt);
	      if (! cur_abbrev->attrs)
		return 0;
	    }

	  cur_abbrev->attrs[cur_abbrev->num_attrs].name
	    = (enum dwarf_attribute) abbrev_name;
	  cur_abbrev->attrs[cur_abbrev->num_attrs++].form
	    = (enum dwarf_form) abbrev_form;
	  abbrev_name = read_unsigned_leb128 (abfd, abbrev_ptr, &bytes_read);
	  abbrev_ptr += bytes_read;
	  abbrev_form = read_unsigned_leb128 (abfd, abbrev_ptr, &bytes_read);
	  abbrev_ptr += bytes_read;
	}

      hash_number = abbrev_number % ABBREV_HASH_SIZE;
      cur_abbrev->next = abbrevs[hash_number];
      abbrevs[hash_number] = cur_abbrev;

      /* Get next abbreviation.
	 Under Irix6 the abbreviations for a compilation unit are not
	 always properly terminated with an abbrev number of 0.
	 Exit loop if we encounter an abbreviation which we have
	 already read (which means we are about to read the abbreviations
	 for the next compile unit) or if the end of the abbreviation
	 table is reached.  */
      if ((unsigned int) (abbrev_ptr - stash->dwarf_abbrev_buffer)
	    >= stash->dwarf_abbrev_size)
	break;
      abbrev_number = read_unsigned_leb128 (abfd, abbrev_ptr, &bytes_read);
      abbrev_ptr += bytes_read;
      if (lookup_abbrev (abbrev_number,abbrevs) != NULL)
	break;
    }

  return abbrevs;
}

/* Read an attribute value described by an attribute form.  */

static bfd_byte *
read_attribute_value (struct attribute *attr,
		      unsigned form,
		      struct comp_unit *unit,
		      bfd_byte *info_ptr)
{
  bfd *abfd = unit->abfd;
  unsigned int bytes_read;
  struct dwarf_block *blk;
  bfd_size_type amt;

  attr->form = (enum dwarf_form) form;

  switch (form)
    {
    case DW_FORM_addr:
      /* FIXME: DWARF3 draft says DW_FORM_ref_addr is offset_size.  */
    case DW_FORM_ref_addr:
      attr->u.val = read_address (unit, info_ptr);
      info_ptr += unit->addr_size;
      break;
    case DW_FORM_block2:
      amt = sizeof (struct dwarf_block);
      blk = bfd_alloc (abfd, amt);
      blk->size = read_2_bytes (abfd, info_ptr);
      info_ptr += 2;
      blk->data = read_n_bytes (abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      attr->u.blk = blk;
      break;
    case DW_FORM_block4:
      amt = sizeof (struct dwarf_block);
      blk = bfd_alloc (abfd, amt);
      blk->size = read_4_bytes (abfd, info_ptr);
      info_ptr += 4;
      blk->data = read_n_bytes (abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      attr->u.blk = blk;
      break;
    case DW_FORM_data2:
      attr->u.val = read_2_bytes (abfd, info_ptr);
      info_ptr += 2;
      break;
    case DW_FORM_data4:
      attr->u.val = read_4_bytes (abfd, info_ptr);
      info_ptr += 4;
      break;
    case DW_FORM_data8:
      attr->u.val = read_8_bytes (abfd, info_ptr);
      info_ptr += 8;
      break;
    case DW_FORM_string:
      attr->u.str = read_string (abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_strp:
      attr->u.str = read_indirect_string (unit, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_block:
      amt = sizeof (struct dwarf_block);
      blk = bfd_alloc (abfd, amt);
      blk->size = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      blk->data = read_n_bytes (abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      attr->u.blk = blk;
      break;
    case DW_FORM_block1:
      amt = sizeof (struct dwarf_block);
      blk = bfd_alloc (abfd, amt);
      blk->size = read_1_byte (abfd, info_ptr);
      info_ptr += 1;
      blk->data = read_n_bytes (abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      attr->u.blk = blk;
      break;
    case DW_FORM_data1:
      attr->u.val = read_1_byte (abfd, info_ptr);
      info_ptr += 1;
      break;
    case DW_FORM_flag:
      attr->u.val = read_1_byte (abfd, info_ptr);
      info_ptr += 1;
      break;
    case DW_FORM_sdata:
      attr->u.sval = read_signed_leb128 (abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_udata:
      attr->u.val = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_ref1:
      attr->u.val = read_1_byte (abfd, info_ptr);
      info_ptr += 1;
      break;
    case DW_FORM_ref2:
      attr->u.val = read_2_bytes (abfd, info_ptr);
      info_ptr += 2;
      break;
    case DW_FORM_ref4:
      attr->u.val = read_4_bytes (abfd, info_ptr);
      info_ptr += 4;
      break;
    case DW_FORM_ref8:
      attr->u.val = read_8_bytes (abfd, info_ptr);
      info_ptr += 8;
      break;
    case DW_FORM_ref_udata:
      attr->u.val = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_indirect:
      form = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      info_ptr = read_attribute_value (attr, form, unit, info_ptr);
      break;
    default:
      (*_bfd_error_handler) (_("Dwarf Error: Invalid or unhandled FORM value: %u."),
			     form);
      bfd_set_error (bfd_error_bad_value);
    }
  return info_ptr;
}

/* Read an attribute described by an abbreviated attribute.  */

static bfd_byte *
read_attribute (struct attribute *attr,
		struct attr_abbrev *abbrev,
		struct comp_unit *unit,
		bfd_byte *info_ptr)
{
  attr->name = abbrev->name;
  info_ptr = read_attribute_value (attr, abbrev->form, unit, info_ptr);
  return info_ptr;
}

/* Source line information table routines.  */

#define FILE_ALLOC_CHUNK 5
#define DIR_ALLOC_CHUNK 5

struct line_info
{
  struct line_info* prev_line;
  bfd_vma address;
  char *filename;
  unsigned int line;
  unsigned int column;
  int end_sequence;		/* End of (sequential) code sequence.  */
};

struct fileinfo
{
  char *name;
  unsigned int dir;
  unsigned int time;
  unsigned int size;
};

struct line_info_table
{
  bfd* abfd;
  unsigned int num_files;
  unsigned int num_dirs;
  char *comp_dir;
  char **dirs;
  struct fileinfo* files;
  struct line_info* last_line;  /* largest VMA */
  struct line_info* lcl_head;   /* local head; used in 'add_line_info' */
};

struct funcinfo
{
  struct funcinfo *prev_func;
  char *name;
  bfd_vma low;
  bfd_vma high;
};

/* Adds a new entry to the line_info list in the line_info_table, ensuring
   that the list is sorted.  Note that the line_info list is sorted from
   highest to lowest VMA (with possible duplicates); that is,
   line_info->prev_line always accesses an equal or smaller VMA.  */

static void
add_line_info (struct line_info_table *table,
	       bfd_vma address,
	       char *filename,
	       unsigned int line,
	       unsigned int column,
	       int end_sequence)
{
  bfd_size_type amt = sizeof (struct line_info);
  struct line_info* info = bfd_alloc (table->abfd, amt);

  /* Find the correct location for 'info'.  Normally we will receive
     new line_info data 1) in order and 2) with increasing VMAs.
     However some compilers break the rules (cf. decode_line_info) and
     so we include some heuristics for quickly finding the correct
     location for 'info'. In particular, these heuristics optimize for
     the common case in which the VMA sequence that we receive is a
     list of locally sorted VMAs such as
       p...z a...j  (where a < j < p < z)

     Note: table->lcl_head is used to head an *actual* or *possible*
     sequence within the list (such as a...j) that is not directly
     headed by table->last_line

     Note: we may receive duplicate entries from 'decode_line_info'.  */

  while (1)
    if (!table->last_line
	|| address >= table->last_line->address)
      {
	/* Normal case: add 'info' to the beginning of the list */
	info->prev_line = table->last_line;
	table->last_line = info;

	/* lcl_head: initialize to head a *possible* sequence at the end.  */
	if (!table->lcl_head)
	  table->lcl_head = info;
	break;
      }
    else if (!table->lcl_head->prev_line
	     && table->lcl_head->address > address)
      {
	/* Abnormal but easy: lcl_head is 1) at the *end* of the line
	   list and 2) the head of 'info'.  */
	info->prev_line = NULL;
	table->lcl_head->prev_line = info;
	break;
      }
    else if (table->lcl_head->prev_line
	     && table->lcl_head->address > address
	     && address >= table->lcl_head->prev_line->address)
      {
	/* Abnormal but easy: lcl_head is 1) in the *middle* of the line
	   list and 2) the head of 'info'.  */
	info->prev_line = table->lcl_head->prev_line;
	table->lcl_head->prev_line = info;
	break;
      }
    else
      {
	/* Abnormal and hard: Neither 'last_line' nor 'lcl_head' are valid
	   heads for 'info'.  Reset 'lcl_head' and repeat.  */
	struct line_info* li2 = table->last_line; /* always non-NULL */
	struct line_info* li1 = li2->prev_line;

	while (li1)
	  {
	    if (li2->address > address && address >= li1->address)
	      break;

	    li2 = li1; /* always non-NULL */
	    li1 = li1->prev_line;
	  }
	table->lcl_head = li2;
      }

  /* Set member data of 'info'.  */
  info->address = address;
  info->line = line;
  info->column = column;
  info->end_sequence = end_sequence;

  if (filename && filename[0])
    {
      info->filename = bfd_alloc (table->abfd, strlen (filename) + 1);
      if (info->filename)
	strcpy (info->filename, filename);
    }
  else
    info->filename = NULL;
}

/* Extract a fully qualified filename from a line info table.
   The returned string has been malloc'ed and it is the caller's
   responsibility to free it.  */

static char *
concat_filename (struct line_info_table *table, unsigned int file)
{
  char *filename;

  if (file - 1 >= table->num_files)
    {
      (*_bfd_error_handler)
	(_("Dwarf Error: mangled line number section (bad file number)."));
      return strdup ("<unknown>");
    }

  filename = table->files[file - 1].name;

  if (! IS_ABSOLUTE_PATH (filename))
    {
      char *dirname = (table->files[file - 1].dir
		       ? table->dirs[table->files[file - 1].dir - 1]
		       : table->comp_dir);

      /* Not all tools set DW_AT_comp_dir, so dirname may be unknown.
	 The best we can do is return the filename part.  */
      if (dirname != NULL)
	{
	  unsigned int len = strlen (dirname) + strlen (filename) + 2;
	  char * name;

	  name = bfd_malloc (len);
	  if (name)
	    sprintf (name, "%s/%s", dirname, filename);
	  return name;
	}
    }

  return strdup (filename);
}

static void
arange_add (struct comp_unit *unit, bfd_vma low_pc, bfd_vma high_pc)
{
  struct arange *arange;

  /* First see if we can cheaply extend an existing range.  */
  arange = &unit->arange;

  do
    {
      if (low_pc == arange->high)
	{
	  arange->high = high_pc;
	  return;
	}
      if (high_pc == arange->low)
	{
	  arange->low = low_pc;
	  return;
	}
      arange = arange->next;
    }
  while (arange);

  if (unit->arange.high == 0)
    {
      /* This is the first address range: store it in unit->arange.  */
      unit->arange.next = 0;
      unit->arange.low = low_pc;
      unit->arange.high = high_pc;
      return;
    }

  /* Need to allocate a new arange and insert it into the arange list.  */
  arange = bfd_zalloc (unit->abfd, sizeof (*arange));
  arange->low = low_pc;
  arange->high = high_pc;

  arange->next = unit->arange.next;
  unit->arange.next = arange;
}

/* Decode the line number information for UNIT.  */

static struct line_info_table*
decode_line_info (struct comp_unit *unit, struct dwarf2_debug *stash)
{
  bfd *abfd = unit->abfd;
  struct line_info_table* table;
  bfd_byte *line_ptr;
  bfd_byte *line_end;
  struct line_head lh;
  unsigned int i, bytes_read, offset_size;
  char *cur_file, *cur_dir;
  unsigned char op_code, extended_op, adj_opcode;
  bfd_size_type amt;

  if (! stash->dwarf_line_buffer)
    {
      asection *msec;

      msec = bfd_get_section_by_name (abfd, ".debug_line");
      if (! msec)
	{
	  (*_bfd_error_handler) (_("Dwarf Error: Can't find .debug_line section."));
	  bfd_set_error (bfd_error_bad_value);
	  return 0;
	}

      stash->dwarf_line_size = msec->size;
      stash->dwarf_line_buffer
	= bfd_simple_get_relocated_section_contents (abfd, msec, NULL,
						     stash->syms);
      if (! stash->dwarf_line_buffer)
	return 0;
    }

  /* It is possible to get a bad value for the line_offset.  Validate
     it here so that we won't get a segfault below.  */
  if (unit->line_offset >= stash->dwarf_line_size)
    {
      (*_bfd_error_handler) (_("Dwarf Error: Line offset (%lu) greater than or equal to .debug_line size (%lu)."),
			     unit->line_offset, stash->dwarf_line_size);
      bfd_set_error (bfd_error_bad_value);
      return 0;
    }

  amt = sizeof (struct line_info_table);
  table = bfd_alloc (abfd, amt);
  table->abfd = abfd;
  table->comp_dir = unit->comp_dir;

  table->num_files = 0;
  table->files = NULL;

  table->num_dirs = 0;
  table->dirs = NULL;

  table->files = NULL;
  table->last_line = NULL;
  table->lcl_head = NULL;

  line_ptr = stash->dwarf_line_buffer + unit->line_offset;

  /* Read in the prologue.  */
  lh.total_length = read_4_bytes (abfd, line_ptr);
  line_ptr += 4;
  offset_size = 4;
  if (lh.total_length == 0xffffffff)
    {
      lh.total_length = read_8_bytes (abfd, line_ptr);
      line_ptr += 8;
      offset_size = 8;
    }
  else if (lh.total_length == 0 && unit->addr_size == 8)
    {
      /* Handle (non-standard) 64-bit DWARF2 formats.  */
      lh.total_length = read_4_bytes (abfd, line_ptr);
      line_ptr += 4;
      offset_size = 8;
    }
  line_end = line_ptr + lh.total_length;
  lh.version = read_2_bytes (abfd, line_ptr);
  line_ptr += 2;
  if (offset_size == 4)
    lh.prologue_length = read_4_bytes (abfd, line_ptr);
  else
    lh.prologue_length = read_8_bytes (abfd, line_ptr);
  line_ptr += offset_size;
  lh.minimum_instruction_length = read_1_byte (abfd, line_ptr);
  line_ptr += 1;
  lh.default_is_stmt = read_1_byte (abfd, line_ptr);
  line_ptr += 1;
  lh.line_base = read_1_signed_byte (abfd, line_ptr);
  line_ptr += 1;
  lh.line_range = read_1_byte (abfd, line_ptr);
  line_ptr += 1;
  lh.opcode_base = read_1_byte (abfd, line_ptr);
  line_ptr += 1;
  amt = lh.opcode_base * sizeof (unsigned char);
  lh.standard_opcode_lengths = bfd_alloc (abfd, amt);

  lh.standard_opcode_lengths[0] = 1;

  for (i = 1; i < lh.opcode_base; ++i)
    {
      lh.standard_opcode_lengths[i] = read_1_byte (abfd, line_ptr);
      line_ptr += 1;
    }

  /* Read directory table.  */
  while ((cur_dir = read_string (abfd, line_ptr, &bytes_read)) != NULL)
    {
      line_ptr += bytes_read;

      if ((table->num_dirs % DIR_ALLOC_CHUNK) == 0)
	{
	  amt = table->num_dirs + DIR_ALLOC_CHUNK;
	  amt *= sizeof (char *);
	  table->dirs = bfd_realloc (table->dirs, amt);
	  if (! table->dirs)
	    return 0;
	}

      table->dirs[table->num_dirs++] = cur_dir;
    }

  line_ptr += bytes_read;

  /* Read file name table.  */
  while ((cur_file = read_string (abfd, line_ptr, &bytes_read)) != NULL)
    {
      line_ptr += bytes_read;

      if ((table->num_files % FILE_ALLOC_CHUNK) == 0)
	{
	  amt = table->num_files + FILE_ALLOC_CHUNK;
	  amt *= sizeof (struct fileinfo);
	  table->files = bfd_realloc (table->files, amt);
	  if (! table->files)
	    return 0;
	}

      table->files[table->num_files].name = cur_file;
      table->files[table->num_files].dir =
	read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
      line_ptr += bytes_read;
      table->files[table->num_files].time =
	read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
      line_ptr += bytes_read;
      table->files[table->num_files].size =
	read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
      line_ptr += bytes_read;
      table->num_files++;
    }

  line_ptr += bytes_read;

  /* Read the statement sequences until there's nothing left.  */
  while (line_ptr < line_end)
    {
      /* State machine registers.  */
      bfd_vma address = 0;
      char * filename = table->num_files ? concat_filename (table, 1) : NULL;
      unsigned int line = 1;
      unsigned int column = 0;
      int is_stmt = lh.default_is_stmt;
      int basic_block = 0;
      int end_sequence = 0;
      /* eraxxon@alumni.rice.edu: Against the DWARF2 specs, some
	 compilers generate address sequences that are wildly out of
	 order using DW_LNE_set_address (e.g. Intel C++ 6.0 compiler
	 for ia64-Linux).  Thus, to determine the low and high
	 address, we must compare on every DW_LNS_copy, etc.  */
      bfd_vma low_pc  = 0;
      bfd_vma high_pc = 0;

      /* Decode the table.  */
      while (! end_sequence)
	{
	  op_code = read_1_byte (abfd, line_ptr);
	  line_ptr += 1;

	  if (op_code >= lh.opcode_base)
	    {
	      /* Special operand.  */
	      adj_opcode = op_code - lh.opcode_base;
	      address += (adj_opcode / lh.line_range)
		* lh.minimum_instruction_length;
	      line += lh.line_base + (adj_opcode % lh.line_range);
	      /* Append row to matrix using current values.  */
	      add_line_info (table, address, filename, line, column, 0);
	      basic_block = 1;
	      if (low_pc == 0 || address < low_pc)
		low_pc = address;
	      if (address > high_pc)
		high_pc = address;
	    }
	  else switch (op_code)
	    {
	    case DW_LNS_extended_op:
	      /* Ignore length.  */
	      line_ptr += 1;
	      extended_op = read_1_byte (abfd, line_ptr);
	      line_ptr += 1;

	      switch (extended_op)
		{
		case DW_LNE_end_sequence:
		  end_sequence = 1;
		  add_line_info (table, address, filename, line, column,
				 end_sequence);
		  if (low_pc == 0 || address < low_pc)
		    low_pc = address;
		  if (address > high_pc)
		    high_pc = address;
		  arange_add (unit, low_pc, high_pc);
		  break;
		case DW_LNE_set_address:
		  address = read_address (unit, line_ptr);
		  line_ptr += unit->addr_size;
		  break;
		case DW_LNE_define_file:
		  cur_file = read_string (abfd, line_ptr, &bytes_read);
		  line_ptr += bytes_read;
		  if ((table->num_files % FILE_ALLOC_CHUNK) == 0)
		    {
		      amt = table->num_files + FILE_ALLOC_CHUNK;
		      amt *= sizeof (struct fileinfo);
		      table->files = bfd_realloc (table->files, amt);
		      if (! table->files)
			return 0;
		    }
		  table->files[table->num_files].name = cur_file;
		  table->files[table->num_files].dir =
		    read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		  line_ptr += bytes_read;
		  table->files[table->num_files].time =
		    read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		  line_ptr += bytes_read;
		  table->files[table->num_files].size =
		    read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		  line_ptr += bytes_read;
		  table->num_files++;
		  break;
		default:
		  (*_bfd_error_handler) (_("Dwarf Error: mangled line number section."));
		  bfd_set_error (bfd_error_bad_value);
		  return 0;
		}
	      break;
	    case DW_LNS_copy:
	      add_line_info (table, address, filename, line, column, 0);
	      basic_block = 0;
	      if (low_pc == 0 || address < low_pc)
		low_pc = address;
	      if (address > high_pc)
		high_pc = address;
	      break;
	    case DW_LNS_advance_pc:
	      address += lh.minimum_instruction_length
		* read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
	      line_ptr += bytes_read;
	      break;
	    case DW_LNS_advance_line:
	      line += read_signed_leb128 (abfd, line_ptr, &bytes_read);
	      line_ptr += bytes_read;
	      break;
	    case DW_LNS_set_file:
	      {
		unsigned int file;

		/* The file and directory tables are 0
		   based, the references are 1 based.  */
		file = read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		line_ptr += bytes_read;
		if (filename)
		  free (filename);
		filename = concat_filename (table, file);
		break;
	      }
	    case DW_LNS_set_column:
	      column = read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
	      line_ptr += bytes_read;
	      break;
	    case DW_LNS_negate_stmt:
	      is_stmt = (!is_stmt);
	      break;
	    case DW_LNS_set_basic_block:
	      basic_block = 1;
	      break;
	    case DW_LNS_const_add_pc:
	      address += lh.minimum_instruction_length
		      * ((255 - lh.opcode_base) / lh.line_range);
	      break;
	    case DW_LNS_fixed_advance_pc:
	      address += read_2_bytes (abfd, line_ptr);
	      line_ptr += 2;
	      break;
	    default:
	      {
		int i;

		/* Unknown standard opcode, ignore it.  */
		for (i = 0; i < lh.standard_opcode_lengths[op_code]; i++)
		  {
		    (void) read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		    line_ptr += bytes_read;
		  }
	      }
	    }
	}

      if (filename)
	free (filename);
    }

  return table;
}

/* If ADDR is within TABLE set the output parameters and return TRUE,
   otherwise return FALSE.  The output parameters, FILENAME_PTR and
   LINENUMBER_PTR, are pointers to the objects to be filled in.  */

static bfd_boolean
lookup_address_in_line_info_table (struct line_info_table *table,
				   bfd_vma addr,
				   struct funcinfo *function,
				   const char **filename_ptr,
				   unsigned int *linenumber_ptr)
{
  /* Note: table->last_line should be a descendingly sorted list. */
  struct line_info* next_line = table->last_line;
  struct line_info* each_line = NULL;
  *filename_ptr = NULL;

  if (!next_line)
    return FALSE;

  each_line = next_line->prev_line;

  /* Check for large addresses */
  if (addr > next_line->address)
    each_line = NULL; /* ensure we skip over the normal case */

  /* Normal case: search the list; save  */
  while (each_line && next_line)
    {
      /* If we have an address match, save this info.  This allows us
	 to return as good as results as possible for strange debugging
	 info.  */
      bfd_boolean addr_match = FALSE;
      if (each_line->address <= addr && addr <= next_line->address)
	{
	  addr_match = TRUE;

	  /* If this line appears to span functions, and addr is in the
	     later function, return the first line of that function instead
	     of the last line of the earlier one.  This check is for GCC
	     2.95, which emits the first line number for a function late.  */
	  if (function != NULL
	      && each_line->address < function->low
	      && next_line->address > function->low)
	    {
	      *filename_ptr = next_line->filename;
	      *linenumber_ptr = next_line->line;
	    }
	  else
	    {
	      *filename_ptr = each_line->filename;
	      *linenumber_ptr = each_line->line;
	    }
	}

      if (addr_match && !each_line->end_sequence)
	return TRUE; /* we have definitely found what we want */

      next_line = each_line;
      each_line = each_line->prev_line;
    }

  /* At this point each_line is NULL but next_line is not.  If we found
     a candidate end-of-sequence point in the loop above, we can return
     that (compatibility with a bug in the Intel compiler); otherwise,
     assuming that we found the containing function for this address in
     this compilation unit, return the first line we have a number for
     (compatibility with GCC 2.95).  */
  if (*filename_ptr == NULL && function != NULL)
    {
      *filename_ptr = next_line->filename;
      *linenumber_ptr = next_line->line;
      return TRUE;
    }

  return FALSE;
}

/* Function table functions.  */

/* If ADDR is within TABLE, set FUNCTIONNAME_PTR, and return TRUE.  */

static bfd_boolean
lookup_address_in_function_table (struct funcinfo *table,
				  bfd_vma addr,
				  struct funcinfo **function_ptr,
				  const char **functionname_ptr)
{
  struct funcinfo* each_func;

  for (each_func = table;
       each_func;
       each_func = each_func->prev_func)
    {
      if (addr >= each_func->low && addr < each_func->high)
	{
	  *functionname_ptr = each_func->name;
	  *function_ptr = each_func;
	  return TRUE;
	}
    }

  return FALSE;
}

static char *
find_abstract_instance_name (struct comp_unit *unit, bfd_uint64_t die_ref)
{
  bfd *abfd = unit->abfd;
  bfd_byte *info_ptr;
  unsigned int abbrev_number, bytes_read, i;
  struct abbrev_info *abbrev;
  struct attribute attr;
  char *name = 0;

  info_ptr = unit->info_ptr_unit + die_ref;
  abbrev_number = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
  info_ptr += bytes_read;

  if (abbrev_number)
    {
      abbrev = lookup_abbrev (abbrev_number, unit->abbrevs);
      if (! abbrev)
	{
	  (*_bfd_error_handler) (_("Dwarf Error: Could not find abbrev number %u."),
				 abbrev_number);
	  bfd_set_error (bfd_error_bad_value);
	}
      else
	{
	  for (i = 0; i < abbrev->num_attrs && !name; ++i)
	    {
	      info_ptr = read_attribute (&attr, &abbrev->attrs[i], unit, info_ptr);
	      switch (attr.name)
		{
		case DW_AT_name:
		  name = attr.u.str;
		  break;
		case DW_AT_specification:
		  name = find_abstract_instance_name (unit, attr.u.val);
		  break;
		default:
		  break;
		}
	    }
	}
    }
  return (name);
}

/* DWARF2 Compilation unit functions.  */

/* Scan over each die in a comp. unit looking for functions to add
   to the function table.  */

static bfd_boolean
scan_unit_for_functions (struct comp_unit *unit)
{
  bfd *abfd = unit->abfd;
  bfd_byte *info_ptr = unit->first_child_die_ptr;
  int nesting_level = 1;

  while (nesting_level)
    {
      unsigned int abbrev_number, bytes_read, i;
      struct abbrev_info *abbrev;
      struct attribute attr;
      struct funcinfo *func;
      char *name = 0;

      abbrev_number = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;

      if (! abbrev_number)
	{
	  nesting_level--;
	  continue;
	}

      abbrev = lookup_abbrev (abbrev_number,unit->abbrevs);
      if (! abbrev)
	{
	  (*_bfd_error_handler) (_("Dwarf Error: Could not find abbrev number %u."),
			     abbrev_number);
	  bfd_set_error (bfd_error_bad_value);
	  return FALSE;
	}

      if (abbrev->tag == DW_TAG_subprogram
	  || abbrev->tag == DW_TAG_inlined_subroutine)
	{
	  bfd_size_type amt = sizeof (struct funcinfo);
	  func = bfd_zalloc (abfd, amt);
	  func->prev_func = unit->function_table;
	  unit->function_table = func;
	}
      else
	func = NULL;

      for (i = 0; i < abbrev->num_attrs; ++i)
	{
	  info_ptr = read_attribute (&attr, &abbrev->attrs[i], unit, info_ptr);

	  if (func)
	    {
	      switch (attr.name)
		{
		case DW_AT_abstract_origin:
		  func->name = find_abstract_instance_name (unit, attr.u.val);
		  break;

		case DW_AT_name:

		  name = attr.u.str;

		  /* Prefer DW_AT_MIPS_linkage_name over DW_AT_name.  */
		  if (func->name == NULL)
		    func->name = attr.u.str;
		  break;

		case DW_AT_MIPS_linkage_name:
		  func->name = attr.u.str;
		  break;

		case DW_AT_low_pc:
		  func->low = attr.u.val;
		  break;

		case DW_AT_high_pc:
		  func->high = attr.u.val;
		  break;

		default:
		  break;
		}
	    }
	  else
	    {
	      switch (attr.name)
		{
		case DW_AT_name:
		  name = attr.u.str;
		  break;

		default:
		  break;
		}
	    }
	}

      if (abbrev->has_children)
	nesting_level++;
    }

  return TRUE;
}

/* Parse a DWARF2 compilation unit starting at INFO_PTR.  This
   includes the compilation unit header that proceeds the DIE's, but
   does not include the length field that precedes each compilation
   unit header.  END_PTR points one past the end of this comp unit.
   OFFSET_SIZE is the size of DWARF2 offsets (either 4 or 8 bytes).

   This routine does not read the whole compilation unit; only enough
   to get to the line number information for the compilation unit.  */

static struct comp_unit *
parse_comp_unit (bfd *abfd,
		 struct dwarf2_debug *stash,
		 bfd_vma unit_length,
		 bfd_byte *info_ptr_unit,
		 unsigned int offset_size)
{
  struct comp_unit* unit;
  unsigned int version;
  bfd_uint64_t abbrev_offset = 0;
  unsigned int addr_size;
  struct abbrev_info** abbrevs;
  unsigned int abbrev_number, bytes_read, i;
  struct abbrev_info *abbrev;
  struct attribute attr;
  bfd_byte *info_ptr = stash->info_ptr;
  bfd_byte *end_ptr = info_ptr + unit_length;
  bfd_size_type amt;

  version = read_2_bytes (abfd, info_ptr);
  info_ptr += 2;
  BFD_ASSERT (offset_size == 4 || offset_size == 8);
  if (offset_size == 4)
    abbrev_offset = read_4_bytes (abfd, info_ptr);
  else
    abbrev_offset = read_8_bytes (abfd, info_ptr);
  info_ptr += offset_size;
  addr_size = read_1_byte (abfd, info_ptr);
  info_ptr += 1;

  if (version != 2)
    {
      (*_bfd_error_handler) (_("Dwarf Error: found dwarf version '%u', this reader only handles version 2 information."), version);
      bfd_set_error (bfd_error_bad_value);
      return 0;
    }

  if (addr_size > sizeof (bfd_vma))
    {
      (*_bfd_error_handler) (_("Dwarf Error: found address size '%u', this reader can not handle sizes greater than '%u'."),
			 addr_size,
			 (unsigned int) sizeof (bfd_vma));
      bfd_set_error (bfd_error_bad_value);
      return 0;
    }

  if (addr_size != 2 && addr_size != 4 && addr_size != 8)
    {
      (*_bfd_error_handler) ("Dwarf Error: found address size '%u', this reader can only handle address sizes '2', '4' and '8'.", addr_size);
      bfd_set_error (bfd_error_bad_value);
      return 0;
    }

  /* Read the abbrevs for this compilation unit into a table.  */
  abbrevs = read_abbrevs (abfd, abbrev_offset, stash);
  if (! abbrevs)
      return 0;

  abbrev_number = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
  info_ptr += bytes_read;
  if (! abbrev_number)
    {
      (*_bfd_error_handler) (_("Dwarf Error: Bad abbrev number: %u."),
			 abbrev_number);
      bfd_set_error (bfd_error_bad_value);
      return 0;
    }

  abbrev = lookup_abbrev (abbrev_number, abbrevs);
  if (! abbrev)
    {
      (*_bfd_error_handler) (_("Dwarf Error: Could not find abbrev number %u."),
			 abbrev_number);
      bfd_set_error (bfd_error_bad_value);
      return 0;
    }

  amt = sizeof (struct comp_unit);
  unit = bfd_zalloc (abfd, amt);
  unit->abfd = abfd;
  unit->addr_size = addr_size;
  unit->offset_size = offset_size;
  unit->abbrevs = abbrevs;
  unit->end_ptr = end_ptr;
  unit->stash = stash;
  unit->info_ptr_unit = info_ptr_unit;

  for (i = 0; i < abbrev->num_attrs; ++i)
    {
      info_ptr = read_attribute (&attr, &abbrev->attrs[i], unit, info_ptr);

      /* Store the data if it is of an attribute we want to keep in a
	 partial symbol table.  */
      switch (attr.name)
	{
	case DW_AT_stmt_list:
	  unit->stmtlist = 1;
	  unit->line_offset = attr.u.val;
	  break;

	case DW_AT_name:
	  unit->name = attr.u.str;
	  break;

	case DW_AT_low_pc:
	  unit->arange.low = attr.u.val;
	  break;

	case DW_AT_high_pc:
	  unit->arange.high = attr.u.val;
	  break;

	case DW_AT_comp_dir:
	  {
	    char *comp_dir = attr.u.str;
	    if (comp_dir)
	      {
		/* Irix 6.2 native cc prepends <machine>.: to the compilation
		   directory, get rid of it.  */
		char *cp = strchr (comp_dir, ':');

		if (cp && cp != comp_dir && cp[-1] == '.' && cp[1] == '/')
		  comp_dir = cp + 1;
	      }
	    unit->comp_dir = comp_dir;
	    break;
	  }

	default:
	  break;
	}
    }

  unit->first_child_die_ptr = info_ptr;
  return unit;
}

/* Return TRUE if UNIT contains the address given by ADDR.  */

static bfd_boolean
comp_unit_contains_address (struct comp_unit *unit, bfd_vma addr)
{
  struct arange *arange;

  if (unit->error)
    return FALSE;

  arange = &unit->arange;
  do
    {
      if (addr >= arange->low && addr < arange->high)
	return TRUE;
      arange = arange->next;
    }
  while (arange);

  return FALSE;
}

/* If UNIT contains ADDR, set the output parameters to the values for
   the line containing ADDR.  The output parameters, FILENAME_PTR,
   FUNCTIONNAME_PTR, and LINENUMBER_PTR, are pointers to the objects
   to be filled in.

   Return TRUE if UNIT contains ADDR, and no errors were encountered;
   FALSE otherwise.  */

static bfd_boolean
comp_unit_find_nearest_line (struct comp_unit *unit,
			     bfd_vma addr,
			     const char **filename_ptr,
			     const char **functionname_ptr,
			     unsigned int *linenumber_ptr,
			     struct dwarf2_debug *stash)
{
  bfd_boolean line_p;
  bfd_boolean func_p;
  struct funcinfo *function;

  if (unit->error)
    return FALSE;

  if (! unit->line_table)
    {
      if (! unit->stmtlist)
	{
	  unit->error = 1;
	  return FALSE;
	}

      unit->line_table = decode_line_info (unit, stash);

      if (! unit->line_table)
	{
	  unit->error = 1;
	  return FALSE;
	}

      if (unit->first_child_die_ptr < unit->end_ptr
	  && ! scan_unit_for_functions (unit))
	{
	  unit->error = 1;
	  return FALSE;
	}
    }

  function = NULL;
  func_p = lookup_address_in_function_table (unit->function_table, addr,
					     &function, functionname_ptr);
  line_p = lookup_address_in_line_info_table (unit->line_table, addr,
					      function, filename_ptr,
					      linenumber_ptr);
  return line_p || func_p;
}

/* Locate a section in a BFD containing debugging info.  The search starts
   from the section after AFTER_SEC, or from the first section in the BFD if
   AFTER_SEC is NULL.  The search works by examining the names of the
   sections.  There are two permissiable names.  The first is .debug_info.
   This is the standard DWARF2 name.  The second is a prefix .gnu.linkonce.wi.
   This is a variation on the .debug_info section which has a checksum
   describing the contents appended onto the name.  This allows the linker to
   identify and discard duplicate debugging sections for different
   compilation units.  */
#define DWARF2_DEBUG_INFO ".debug_info"
#define GNU_LINKONCE_INFO ".gnu.linkonce.wi."

static asection *
find_debug_info (bfd *abfd, asection *after_sec)
{
  asection * msec;

  if (after_sec)
    msec = after_sec->next;
  else
    msec = abfd->sections;

  while (msec)
    {
      if (strcmp (msec->name, DWARF2_DEBUG_INFO) == 0)
	return msec;

      if (strncmp (msec->name, GNU_LINKONCE_INFO, strlen (GNU_LINKONCE_INFO)) == 0)
	return msec;

      msec = msec->next;
    }

  return NULL;
}

/* The DWARF2 version of find_nearest_line.  Return TRUE if the line
   is found without error.  ADDR_SIZE is the number of bytes in the
   initial .debug_info length field and in the abbreviation offset.
   You may use zero to indicate that the default value should be
   used.  */

bfd_boolean
_bfd_dwarf2_find_nearest_line (bfd *abfd,
			       asection *section,
			       asymbol **symbols,
			       bfd_vma offset,
			       const char **filename_ptr,
			       const char **functionname_ptr,
			       unsigned int *linenumber_ptr,
			       unsigned int addr_size,
			       void **pinfo)
{
  /* Read each compilation unit from the section .debug_info, and check
     to see if it contains the address we are searching for.  If yes,
     lookup the address, and return the line number info.  If no, go
     on to the next compilation unit.

     We keep a list of all the previously read compilation units, and
     a pointer to the next un-read compilation unit.  Check the
     previously read units before reading more.  */
  struct dwarf2_debug *stash;

  /* What address are we looking for?  */
  bfd_vma addr;

  struct comp_unit* each;

  stash = *pinfo;
  addr = offset;
  if (section->output_section)
    addr += section->output_section->vma + section->output_offset;
  else
    addr += section->vma;
  *filename_ptr = NULL;
  *functionname_ptr = NULL;
  *linenumber_ptr = 0;

  /* The DWARF2 spec says that the initial length field, and the
     offset of the abbreviation table, should both be 4-byte values.
     However, some compilers do things differently.  */
  if (addr_size == 0)
    addr_size = 4;
  BFD_ASSERT (addr_size == 4 || addr_size == 8);

  if (! stash)
    {
      bfd_size_type total_size;
      asection *msec;
      bfd_size_type amt = sizeof (struct dwarf2_debug);

      stash = bfd_zalloc (abfd, amt);
      if (! stash)
	return FALSE;

      *pinfo = stash;

      msec = find_debug_info (abfd, NULL);
      if (! msec)
	/* No dwarf2 info.  Note that at this point the stash
	   has been allocated, but contains zeros, this lets
	   future calls to this function fail quicker.  */
	 return FALSE;

      /* There can be more than one DWARF2 info section in a BFD these days.
	 Read them all in and produce one large stash.  We do this in two
	 passes - in the first pass we just accumulate the section sizes.
	 In the second pass we read in the section's contents.  The allows
	 us to avoid reallocing the data as we add sections to the stash.  */
      for (total_size = 0; msec; msec = find_debug_info (abfd, msec))
	total_size += msec->size;

      stash->info_ptr = bfd_alloc (abfd, total_size);
      if (stash->info_ptr == NULL)
	return FALSE;

      stash->info_ptr_end = stash->info_ptr;

      for (msec = find_debug_info (abfd, NULL);
	   msec;
	   msec = find_debug_info (abfd, msec))
	{
	  bfd_size_type size;
	  bfd_size_type start;

	  size = msec->size;
	  if (size == 0)
	    continue;

	  start = stash->info_ptr_end - stash->info_ptr;

	  if ((bfd_simple_get_relocated_section_contents
	       (abfd, msec, stash->info_ptr + start, symbols)) == NULL)
	    continue;

	  stash->info_ptr_end = stash->info_ptr + start + size;
	}

      BFD_ASSERT (stash->info_ptr_end == stash->info_ptr + total_size);

      stash->sec = find_debug_info (abfd, NULL);
      stash->sec_info_ptr = stash->info_ptr;
      stash->syms = symbols;
    }

  /* A null info_ptr indicates that there is no dwarf2 info
     (or that an error occured while setting up the stash).  */
  if (! stash->info_ptr)
    return FALSE;

  /* Check the previously read comp. units first.  */
  for (each = stash->all_comp_units; each; each = each->next_unit)
    if (comp_unit_contains_address (each, addr))
      return comp_unit_find_nearest_line (each, addr, filename_ptr,
					  functionname_ptr, linenumber_ptr,
					  stash);

  /* Read each remaining comp. units checking each as they are read.  */
  while (stash->info_ptr < stash->info_ptr_end)
    {
      bfd_vma length;
      bfd_boolean found;
      unsigned int offset_size = addr_size;
      bfd_byte *info_ptr_unit = stash->info_ptr;

      length = read_4_bytes (abfd, stash->info_ptr);
      /* A 0xffffff length is the DWARF3 way of indicating we use
	 64-bit offsets, instead of 32-bit offsets.  */
      if (length == 0xffffffff)
	{
	  offset_size = 8;
	  length = read_8_bytes (abfd, stash->info_ptr + 4);
	  stash->info_ptr += 12;
	}
      /* A zero length is the IRIX way of indicating 64-bit offsets,
	 mostly because the 64-bit length will generally fit in 32
	 bits, and the endianness helps.  */
      else if (length == 0)
	{
	  offset_size = 8;
	  length = read_4_bytes (abfd, stash->info_ptr + 4);
	  stash->info_ptr += 8;
	}
      /* In the absence of the hints above, we assume addr_size-sized
	 offsets, for backward-compatibility with pre-DWARF3 64-bit
	 platforms.  */
      else if (addr_size == 8)
	{
	  length = read_8_bytes (abfd, stash->info_ptr);
	  stash->info_ptr += 8;
	}
      else
	stash->info_ptr += 4;

      if (length > 0)
	{
	  each = parse_comp_unit (abfd, stash, length, info_ptr_unit,
				  offset_size);
	  stash->info_ptr += length;

	  if ((bfd_vma) (stash->info_ptr - stash->sec_info_ptr)
	      == stash->sec->size)
	    {
	      stash->sec = find_debug_info (abfd, stash->sec);
	      stash->sec_info_ptr = stash->info_ptr;
	    }

	  if (each)
	    {
	      each->next_unit = stash->all_comp_units;
	      stash->all_comp_units = each;

	      /* DW_AT_low_pc and DW_AT_high_pc are optional for
		 compilation units.  If we don't have them (i.e.,
		 unit->high == 0), we need to consult the line info
		 table to see if a compilation unit contains the given
		 address.  */
	      if (each->arange.high > 0)
		{
		  if (comp_unit_contains_address (each, addr))
		    return comp_unit_find_nearest_line (each, addr,
							filename_ptr,
							functionname_ptr,
							linenumber_ptr,
							stash);
		}
	      else
		{
		  found = comp_unit_find_nearest_line (each, addr,
						       filename_ptr,
						       functionname_ptr,
						       linenumber_ptr,
						       stash);
		  if (found)
		    return TRUE;
		}
	    }
	}
    }

  return FALSE;
}
