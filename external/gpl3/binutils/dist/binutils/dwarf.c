/* dwarf.c -- display DWARF contents of a BFD binary file
   Copyright 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012
   Free Software Foundation, Inc.

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
#include "bfd.h"
#include "bfd_stdint.h"
#include "bucomm.h"
#include "elfcomm.h"
#include "elf/common.h"
#include "dwarf2.h"
#include "dwarf.h"
#include "gdb/gdb-index.h"

static const char *regname (unsigned int regno, int row);

static int have_frame_base;
static int need_base_address;

static unsigned int last_pointer_size = 0;
static int warned_about_missing_comp_units = FALSE;

static unsigned int num_debug_info_entries = 0;
static debug_info *debug_information = NULL;
/* Special value for num_debug_info_entries to indicate
   that the .debug_info section could not be loaded/parsed.  */
#define DEBUG_INFO_UNAVAILABLE  (unsigned int) -1

int eh_addr_size;

int do_debug_info;
int do_debug_abbrevs;
int do_debug_lines;
int do_debug_pubnames;
int do_debug_pubtypes;
int do_debug_aranges;
int do_debug_ranges;
int do_debug_frames;
int do_debug_frames_interp;
int do_debug_macinfo;
int do_debug_str;
int do_debug_loc;
int do_gdb_index;
int do_trace_info;
int do_trace_abbrevs;
int do_trace_aranges;
int do_wide;

int dwarf_cutoff_level = -1;
unsigned long dwarf_start_die;

int dwarf_check = 0;

/* Values for do_debug_lines.  */
#define FLAG_DEBUG_LINES_RAW	 1
#define FLAG_DEBUG_LINES_DECODED 2

static int
size_of_encoded_value (int encoding)
{
  switch (encoding & 0x7)
    {
    default:	/* ??? */
    case 0:	return eh_addr_size;
    case 2:	return 2;
    case 3:	return 4;
    case 4:	return 8;
    }
}

static dwarf_vma
get_encoded_value (unsigned char *data,
		   int encoding,
		   struct dwarf_section *section)
{
  int size = size_of_encoded_value (encoding);
  dwarf_vma val;

  if (encoding & DW_EH_PE_signed)
    val = byte_get_signed (data, size);
  else
    val = byte_get (data, size);

  if ((encoding & 0x70) == DW_EH_PE_pcrel)
    val += section->address + (data - section->start);
  return val;
}

/* Print a dwarf_vma value (typically an address, offset or length) in
   hexadecimal format, followed by a space.  The length of the value (and
   hence the precision displayed) is determined by the byte_size parameter.  */

static void
print_dwarf_vma (dwarf_vma val, unsigned byte_size)
{
  static char buff[18];
  int offset = 0;

  /* Printf does not have a way of specifiying a maximum field width for an
     integer value, so we print the full value into a buffer and then select
     the precision we need.  */
#if __STDC_VERSION__ >= 199901L || (defined(__GNUC__) && __GNUC__ >= 2)
#ifndef __MINGW32__
  snprintf (buff, sizeof (buff), "%16.16llx ", val);
#else
  snprintf (buff, sizeof (buff), "%016I64x ", val);
#endif
#else
  snprintf (buff, sizeof (buff), "%16.16lx ", val);
#endif

  if (byte_size != 0)
    {
      if (byte_size > 0 && byte_size <= 8)
	offset = 16 - 2 * byte_size;
      else
	error (_("Wrong size in print_dwarf_vma"));
    }

  fputs (buff + offset, stdout);
}

#if __STDC_VERSION__ >= 199901L || (defined(__GNUC__) && __GNUC__ >= 2)
#ifndef __MINGW32__
#define  DWARF_VMA_FMT "ll"
#else
#define  DWARF_VMA_FMT "I64"
#endif
#else
#define  DWARF_VMA_FMT "l"
#endif

static const char *
dwarf_vmatoa (const char *fmtch, dwarf_vma value)
{
  /* As dwarf_vmatoa is used more then once in a printf call
     for output, we are cycling through an fixed array of pointers
     for return address.  */
  static int buf_pos = 0;
  static struct dwarf_vmatoa_buf
  {
    char place[64];
  } buf[16];
  char fmt[32];
  char *ret;

  sprintf (fmt, "%%%s%s", DWARF_VMA_FMT, fmtch);

  ret = buf[buf_pos++].place;
  buf_pos %= ARRAY_SIZE (buf);

  snprintf (ret, sizeof (buf[0].place), fmt, value);

  return ret;
}

/* Format a 64-bit value, given as two 32-bit values, in hex.
   For reentrancy, this uses a buffer provided by the caller.  */

static const char *
dwarf_vmatoa64 (dwarf_vma hvalue, dwarf_vma lvalue, char *buf,
		unsigned int buf_len)
{
  int len = 0;

  if (hvalue == 0)
    snprintf (buf, buf_len, "%" DWARF_VMA_FMT "x", lvalue);
  else
    {
      len = snprintf (buf, buf_len, "%" DWARF_VMA_FMT "x", hvalue);
      snprintf (buf + len, buf_len - len,
		"%08" DWARF_VMA_FMT "x", lvalue);
    }

  return buf;
}

dwarf_vma
read_leb128 (unsigned char *data, unsigned int *length_return, int sign)
{
  dwarf_vma result = 0;
  unsigned int num_read = 0;
  unsigned int shift = 0;
  unsigned char byte;

  do
    {
      byte = *data++;
      num_read++;

      result |= ((dwarf_vma) (byte & 0x7f)) << shift;

      shift += 7;

    }
  while (byte & 0x80);

  if (length_return != NULL)
    *length_return = num_read;

  if (sign && (shift < 8 * sizeof (result)) && (byte & 0x40))
    result |= -1L << shift;

  return result;
}

/* Create a signed version to avoid painful typecasts.  */
static dwarf_signed_vma
read_sleb128 (unsigned char *data, unsigned int *length_return)
{
  return (dwarf_signed_vma) read_leb128 (data, length_return, 1);
}

typedef struct State_Machine_Registers
{
  dwarf_vma address;
  unsigned int file;
  unsigned int line;
  unsigned int column;
  int is_stmt;
  int basic_block;
  unsigned char op_index;
  unsigned char end_sequence;
/* This variable hold the number of the last entry seen
   in the File Table.  */
  unsigned int last_file_entry;
} SMR;

static SMR state_machine_regs;

static void
reset_state_machine (int is_stmt)
{
  state_machine_regs.address = 0;
  state_machine_regs.op_index = 0;
  state_machine_regs.file = 1;
  state_machine_regs.line = 1;
  state_machine_regs.column = 0;
  state_machine_regs.is_stmt = is_stmt;
  state_machine_regs.basic_block = 0;
  state_machine_regs.end_sequence = 0;
  state_machine_regs.last_file_entry = 0;
}

/* Handled an extend line op.
   Returns the number of bytes read.  */

static int
process_extended_line_op (unsigned char *data, int is_stmt)
{
  unsigned char op_code;
  unsigned int bytes_read;
  unsigned int len;
  unsigned char *name;
  dwarf_vma adr;
  unsigned char *orig_data = data;

  len = read_leb128 (data, & bytes_read, 0);
  data += bytes_read;

  if (len == 0)
    {
      warn (_("badly formed extended line op encountered!\n"));
      return bytes_read;
    }

  len += bytes_read;
  op_code = *data++;

  printf (_("  Extended opcode %d: "), op_code);

  switch (op_code)
    {
    case DW_LNE_end_sequence:
      printf (_("End of Sequence\n\n"));
      reset_state_machine (is_stmt);
      break;

    case DW_LNE_set_address:
      adr = byte_get (data, len - bytes_read - 1);
      printf (_("set Address to 0x%s\n"), dwarf_vmatoa ("x", adr));
      state_machine_regs.address = adr;
      state_machine_regs.op_index = 0;
      break;

    case DW_LNE_define_file:
      printf (_("define new File Table entry\n"));
      printf (_("  Entry\tDir\tTime\tSize\tName\n"));

      printf ("   %d\t", ++state_machine_regs.last_file_entry);
      name = data;
      data += strlen ((char *) data) + 1;
      printf ("%s\t", dwarf_vmatoa ("u", read_leb128 (data, & bytes_read, 0)));
      data += bytes_read;
      printf ("%s\t", dwarf_vmatoa ("u", read_leb128 (data, & bytes_read, 0)));
      data += bytes_read;
      printf ("%s\t", dwarf_vmatoa ("u", read_leb128 (data, & bytes_read, 0)));
      data += bytes_read;
      printf ("%s", name);
      if ((unsigned int) (data - orig_data) != len)
        printf (_(" [Bad opcode length]"));
      printf ("\n\n");
      break;

    case DW_LNE_set_discriminator:
      printf (_("set Discriminator to %s\n"),
	      dwarf_vmatoa ("u", read_leb128 (data, & bytes_read, 0)));
      break;

    /* HP extensions.  */
    case DW_LNE_HP_negate_is_UV_update:
      printf ("DW_LNE_HP_negate_is_UV_update\n");
      break;
    case DW_LNE_HP_push_context:
      printf ("DW_LNE_HP_push_context\n");
      break;
    case DW_LNE_HP_pop_context:
      printf ("DW_LNE_HP_pop_context\n");
      break;
    case DW_LNE_HP_set_file_line_column:
      printf ("DW_LNE_HP_set_file_line_column\n");
      break;
    case DW_LNE_HP_set_routine_name:
      printf ("DW_LNE_HP_set_routine_name\n");
      break;
    case DW_LNE_HP_set_sequence:
      printf ("DW_LNE_HP_set_sequence\n");
      break;
    case DW_LNE_HP_negate_post_semantics:
      printf ("DW_LNE_HP_negate_post_semantics\n");
      break;
    case DW_LNE_HP_negate_function_exit:
      printf ("DW_LNE_HP_negate_function_exit\n");
      break;
    case DW_LNE_HP_negate_front_end_logical:
      printf ("DW_LNE_HP_negate_front_end_logical\n");
      break;
    case DW_LNE_HP_define_proc:
      printf ("DW_LNE_HP_define_proc\n");
      break;
    case DW_LNE_HP_source_file_correlation:
      {
        unsigned char *edata = data + len - bytes_read - 1;

        printf ("DW_LNE_HP_source_file_correlation\n");

        while (data < edata)
          {
            unsigned int opc;

            opc = read_leb128 (data, & bytes_read, 0);
            data += bytes_read;

            switch (opc)
              {
              case DW_LNE_HP_SFC_formfeed:
                printf ("    DW_LNE_HP_SFC_formfeed\n");
                break;
              case DW_LNE_HP_SFC_set_listing_line:
                printf ("    DW_LNE_HP_SFC_set_listing_line (%s)\n",
                        dwarf_vmatoa ("u",
                                      read_leb128 (data, & bytes_read, 0)));
                data += bytes_read;
                break;
              case DW_LNE_HP_SFC_associate:
                printf ("    DW_LNE_HP_SFC_associate ");
                printf ("(%s",
                        dwarf_vmatoa ("u",
                                      read_leb128 (data, & bytes_read, 0)));
                data += bytes_read;
                printf (",%s",
                        dwarf_vmatoa ("u",
                                      read_leb128 (data, & bytes_read, 0)));
                data += bytes_read;
                printf (",%s)\n",
                        dwarf_vmatoa ("u",
                                      read_leb128 (data, & bytes_read, 0)));
                data += bytes_read;
                break;
              default:
                printf (_("    UNKNOWN DW_LNE_HP_SFC opcode (%u)\n"), opc);
                data = edata;
                break;
              }
          }
      }
      break;

    default:
      {
        unsigned int rlen = len - bytes_read - 1;

        if (op_code >= DW_LNE_lo_user
            /* The test against DW_LNW_hi_user is redundant due to
               the limited range of the unsigned char data type used
               for op_code.  */
            /*&& op_code <= DW_LNE_hi_user*/)
          printf (_("user defined: "));
        else
          printf (_("UNKNOWN: "));
        printf (_("length %d ["), rlen);
        for (; rlen; rlen--)
          printf (" %02x", *data++);
        printf ("]\n");
      }
      break;
    }

  return len;
}

static const char *
fetch_indirect_string (dwarf_vma offset)
{
  struct dwarf_section *section = &debug_displays [str].section;

  if (section->start == NULL)
    return _("<no .debug_str section>");

  /* DWARF sections under Mach-O have non-zero addresses.  */
  offset -= section->address;
  if (offset > section->size)
    {
      warn (_("DW_FORM_strp offset too big: %s\n"),
	    dwarf_vmatoa ("x", offset));
      return _("<offset is too big>");
    }

  return (const char *) section->start + offset;
}

static const char *
fetch_indexed_string (dwarf_vma idx, dwarf_vma offset_size, int dwo)
{
  enum dwarf_section_display_enum str_sec_idx = dwo ? str_dwo : str;
  enum dwarf_section_display_enum idx_sec_idx = dwo ? str_index_dwo : str_index;
  struct dwarf_section *index_section = &debug_displays [idx_sec_idx].section;
  struct dwarf_section *str_section = &debug_displays [str_sec_idx].section;
  dwarf_vma index_offset = idx * offset_size;
  dwarf_vma str_offset;

  if (index_section->start == NULL)
    return (dwo ? _("<no .debug_str_offsets.dwo section>")
		: _("<no .debug_str_offsets section>"));

  /* DWARF sections under Mach-O have non-zero addresses.  */
  index_offset -= index_section->address;
  if (index_offset > index_section->size)
    {
      warn (_("DW_FORM_GNU_str_index offset too big: %s\n"),
	    dwarf_vmatoa ("x", index_offset));
      return _("<index offset is too big>");
    }

  if (str_section->start == NULL)
    return (dwo ? _("<no .debug_str.dwo section>")
		: _("<no .debug_str section>"));

  str_offset = byte_get (index_section->start + index_offset, offset_size);
  str_offset -= str_section->address;
  if (str_offset > str_section->size)
    {
      warn (_("DW_FORM_GNU_str_index indirect offset too big: %s\n"),
	    dwarf_vmatoa ("x", str_offset));
      return _("<indirect index offset is too big>");
    }

  return (const char *) str_section->start + str_offset;
}

static const char *
fetch_indexed_value (dwarf_vma offset, dwarf_vma bytes)
{
  struct dwarf_section *section = &debug_displays [debug_addr].section;

  if (section->start == NULL)
    return (_("<no .debug_addr section>"));

  if (offset + bytes > section->size)
    {
      warn (_("Offset into section %s too big: %s\n"),
            section->name, dwarf_vmatoa ("x", offset));
      return "<offset too big>";
    }

  return dwarf_vmatoa ("x", byte_get (section->start + offset, bytes));
}


/* FIXME:  There are better and more efficient ways to handle
   these structures.  For now though, I just want something that
   is simple to implement.  */
typedef struct abbrev_attr
{
  unsigned long attribute;
  unsigned long form;
  struct abbrev_attr *next;
}
abbrev_attr;

typedef struct abbrev_entry
{
  unsigned long entry;
  unsigned long tag;
  int children;
  struct abbrev_attr *first_attr;
  struct abbrev_attr *last_attr;
  struct abbrev_entry *next;
}
abbrev_entry;

static abbrev_entry *first_abbrev = NULL;
static abbrev_entry *last_abbrev = NULL;

static void
free_abbrevs (void)
{
  abbrev_entry *abbrv;

  for (abbrv = first_abbrev; abbrv;)
    {
      abbrev_entry *next_abbrev = abbrv->next;
      abbrev_attr *attr;

      for (attr = abbrv->first_attr; attr;)
	{
	  abbrev_attr *next_attr = attr->next;

	  free (attr);
	  attr = next_attr;
	}

      free (abbrv);
      abbrv = next_abbrev;
    }

  last_abbrev = first_abbrev = NULL;
}

static void
add_abbrev (unsigned long number, unsigned long tag, int children)
{
  abbrev_entry *entry;

  entry = (abbrev_entry *) malloc (sizeof (*entry));
  if (entry == NULL)
    /* ugg */
    return;

  entry->entry      = number;
  entry->tag        = tag;
  entry->children   = children;
  entry->first_attr = NULL;
  entry->last_attr  = NULL;
  entry->next       = NULL;

  if (first_abbrev == NULL)
    first_abbrev = entry;
  else
    last_abbrev->next = entry;

  last_abbrev = entry;
}

static void
add_abbrev_attr (unsigned long attribute, unsigned long form)
{
  abbrev_attr *attr;

  attr = (abbrev_attr *) malloc (sizeof (*attr));
  if (attr == NULL)
    /* ugg */
    return;

  attr->attribute = attribute;
  attr->form      = form;
  attr->next      = NULL;

  if (last_abbrev->first_attr == NULL)
    last_abbrev->first_attr = attr;
  else
    last_abbrev->last_attr->next = attr;

  last_abbrev->last_attr = attr;
}

/* Processes the (partial) contents of a .debug_abbrev section.
   Returns NULL if the end of the section was encountered.
   Returns the address after the last byte read if the end of
   an abbreviation set was found.  */

static unsigned char *
process_abbrev_section (unsigned char *start, unsigned char *end)
{
  if (first_abbrev != NULL)
    return NULL;

  while (start < end)
    {
      unsigned int bytes_read;
      unsigned long entry;
      unsigned long tag;
      unsigned long attribute;
      int children;

      entry = read_leb128 (start, & bytes_read, 0);
      start += bytes_read;

      /* A single zero is supposed to end the section according
	 to the standard.  If there's more, then signal that to
	 the caller.  */
      if (entry == 0)
	return start == end ? NULL : start;

      tag = read_leb128 (start, & bytes_read, 0);
      start += bytes_read;

      children = *start++;

      add_abbrev (entry, tag, children);

      do
	{
	  unsigned long form;

	  attribute = read_leb128 (start, & bytes_read, 0);
	  start += bytes_read;

	  form = read_leb128 (start, & bytes_read, 0);
	  start += bytes_read;

	  if (attribute != 0)
	    add_abbrev_attr (attribute, form);
	}
      while (attribute != 0);
    }

  return NULL;
}

static const char *
get_TAG_name (unsigned long tag)
{
  const char *name = get_DW_TAG_name ((unsigned int)tag);

  if (name == NULL)
    {
      static char buffer[100];

      snprintf (buffer, sizeof (buffer), _("Unknown TAG value: %lx"), tag);
      return buffer;
    }

  return name;
}

static const char *
get_FORM_name (unsigned long form)
{
  const char *name = get_DW_FORM_name (form);

  if (name == NULL)
    {
      static char buffer[100];

      snprintf (buffer, sizeof (buffer), _("Unknown FORM value: %lx"), form);
      return buffer;
    }

  return name;
}

static unsigned char *
display_block (unsigned char *data, dwarf_vma length)
{
  printf (_(" %s byte block: "), dwarf_vmatoa ("u", length));

  while (length --)
    printf ("%lx ", (unsigned long) byte_get (data++, 1));

  return data;
}

static int
decode_location_expression (unsigned char * data,
			    unsigned int pointer_size,
			    unsigned int offset_size,
			    int dwarf_version,
			    dwarf_vma length,
			    dwarf_vma cu_offset,
			    struct dwarf_section * section)
{
  unsigned op;
  unsigned int bytes_read;
  dwarf_vma uvalue;
  unsigned char *end = data + length;
  int need_frame_base = 0;

  while (data < end)
    {
      op = *data++;

      switch (op)
	{
	case DW_OP_addr:
         printf ("DW_OP_addr: %s",
                 dwarf_vmatoa ("x", byte_get (data, pointer_size)));
	  data += pointer_size;
	  break;
	case DW_OP_deref:
	  printf ("DW_OP_deref");
	  break;
	case DW_OP_const1u:
	  printf ("DW_OP_const1u: %lu", (unsigned long) byte_get (data++, 1));
	  break;
	case DW_OP_const1s:
	  printf ("DW_OP_const1s: %ld", (long) byte_get_signed (data++, 1));
	  break;
	case DW_OP_const2u:
	  printf ("DW_OP_const2u: %lu", (unsigned long) byte_get (data, 2));
	  data += 2;
	  break;
	case DW_OP_const2s:
	  printf ("DW_OP_const2s: %ld", (long) byte_get_signed (data, 2));
	  data += 2;
	  break;
	case DW_OP_const4u:
	  printf ("DW_OP_const4u: %lu", (unsigned long) byte_get (data, 4));
	  data += 4;
	  break;
	case DW_OP_const4s:
	  printf ("DW_OP_const4s: %ld", (long) byte_get_signed (data, 4));
	  data += 4;
	  break;
	case DW_OP_const8u:
	  printf ("DW_OP_const8u: %lu %lu", (unsigned long) byte_get (data, 4),
		  (unsigned long) byte_get (data + 4, 4));
	  data += 8;
	  break;
	case DW_OP_const8s:
	  printf ("DW_OP_const8s: %ld %ld", (long) byte_get (data, 4),
		  (long) byte_get (data + 4, 4));
	  data += 8;
	  break;
	case DW_OP_constu:
	  printf ("DW_OP_constu: %s",
		  dwarf_vmatoa ("u", read_leb128 (data, &bytes_read, 0)));
	  data += bytes_read;
	  break;
	case DW_OP_consts:
	  printf ("DW_OP_consts: %s",
		  dwarf_vmatoa ("d", read_sleb128 (data, &bytes_read)));
	  data += bytes_read;
	  break;
	case DW_OP_dup:
	  printf ("DW_OP_dup");
	  break;
	case DW_OP_drop:
	  printf ("DW_OP_drop");
	  break;
	case DW_OP_over:
	  printf ("DW_OP_over");
	  break;
	case DW_OP_pick:
	  printf ("DW_OP_pick: %ld", (unsigned long) byte_get (data++, 1));
	  break;
	case DW_OP_swap:
	  printf ("DW_OP_swap");
	  break;
	case DW_OP_rot:
	  printf ("DW_OP_rot");
	  break;
	case DW_OP_xderef:
	  printf ("DW_OP_xderef");
	  break;
	case DW_OP_abs:
	  printf ("DW_OP_abs");
	  break;
	case DW_OP_and:
	  printf ("DW_OP_and");
	  break;
	case DW_OP_div:
	  printf ("DW_OP_div");
	  break;
	case DW_OP_minus:
	  printf ("DW_OP_minus");
	  break;
	case DW_OP_mod:
	  printf ("DW_OP_mod");
	  break;
	case DW_OP_mul:
	  printf ("DW_OP_mul");
	  break;
	case DW_OP_neg:
	  printf ("DW_OP_neg");
	  break;
	case DW_OP_not:
	  printf ("DW_OP_not");
	  break;
	case DW_OP_or:
	  printf ("DW_OP_or");
	  break;
	case DW_OP_plus:
	  printf ("DW_OP_plus");
	  break;
	case DW_OP_plus_uconst:
	  printf ("DW_OP_plus_uconst: %s",
		  dwarf_vmatoa ("u", read_leb128 (data, &bytes_read, 0)));
	  data += bytes_read;
	  break;
	case DW_OP_shl:
	  printf ("DW_OP_shl");
	  break;
	case DW_OP_shr:
	  printf ("DW_OP_shr");
	  break;
	case DW_OP_shra:
	  printf ("DW_OP_shra");
	  break;
	case DW_OP_xor:
	  printf ("DW_OP_xor");
	  break;
	case DW_OP_bra:
	  printf ("DW_OP_bra: %ld", (long) byte_get_signed (data, 2));
	  data += 2;
	  break;
	case DW_OP_eq:
	  printf ("DW_OP_eq");
	  break;
	case DW_OP_ge:
	  printf ("DW_OP_ge");
	  break;
	case DW_OP_gt:
	  printf ("DW_OP_gt");
	  break;
	case DW_OP_le:
	  printf ("DW_OP_le");
	  break;
	case DW_OP_lt:
	  printf ("DW_OP_lt");
	  break;
	case DW_OP_ne:
	  printf ("DW_OP_ne");
	  break;
	case DW_OP_skip:
	  printf ("DW_OP_skip: %ld", (long) byte_get_signed (data, 2));
	  data += 2;
	  break;

	case DW_OP_lit0:
	case DW_OP_lit1:
	case DW_OP_lit2:
	case DW_OP_lit3:
	case DW_OP_lit4:
	case DW_OP_lit5:
	case DW_OP_lit6:
	case DW_OP_lit7:
	case DW_OP_lit8:
	case DW_OP_lit9:
	case DW_OP_lit10:
	case DW_OP_lit11:
	case DW_OP_lit12:
	case DW_OP_lit13:
	case DW_OP_lit14:
	case DW_OP_lit15:
	case DW_OP_lit16:
	case DW_OP_lit17:
	case DW_OP_lit18:
	case DW_OP_lit19:
	case DW_OP_lit20:
	case DW_OP_lit21:
	case DW_OP_lit22:
	case DW_OP_lit23:
	case DW_OP_lit24:
	case DW_OP_lit25:
	case DW_OP_lit26:
	case DW_OP_lit27:
	case DW_OP_lit28:
	case DW_OP_lit29:
	case DW_OP_lit30:
	case DW_OP_lit31:
	  printf ("DW_OP_lit%d", op - DW_OP_lit0);
	  break;

	case DW_OP_reg0:
	case DW_OP_reg1:
	case DW_OP_reg2:
	case DW_OP_reg3:
	case DW_OP_reg4:
	case DW_OP_reg5:
	case DW_OP_reg6:
	case DW_OP_reg7:
	case DW_OP_reg8:
	case DW_OP_reg9:
	case DW_OP_reg10:
	case DW_OP_reg11:
	case DW_OP_reg12:
	case DW_OP_reg13:
	case DW_OP_reg14:
	case DW_OP_reg15:
	case DW_OP_reg16:
	case DW_OP_reg17:
	case DW_OP_reg18:
	case DW_OP_reg19:
	case DW_OP_reg20:
	case DW_OP_reg21:
	case DW_OP_reg22:
	case DW_OP_reg23:
	case DW_OP_reg24:
	case DW_OP_reg25:
	case DW_OP_reg26:
	case DW_OP_reg27:
	case DW_OP_reg28:
	case DW_OP_reg29:
	case DW_OP_reg30:
	case DW_OP_reg31:
	  printf ("DW_OP_reg%d (%s)", op - DW_OP_reg0,
		  regname (op - DW_OP_reg0, 1));
	  break;

	case DW_OP_breg0:
	case DW_OP_breg1:
	case DW_OP_breg2:
	case DW_OP_breg3:
	case DW_OP_breg4:
	case DW_OP_breg5:
	case DW_OP_breg6:
	case DW_OP_breg7:
	case DW_OP_breg8:
	case DW_OP_breg9:
	case DW_OP_breg10:
	case DW_OP_breg11:
	case DW_OP_breg12:
	case DW_OP_breg13:
	case DW_OP_breg14:
	case DW_OP_breg15:
	case DW_OP_breg16:
	case DW_OP_breg17:
	case DW_OP_breg18:
	case DW_OP_breg19:
	case DW_OP_breg20:
	case DW_OP_breg21:
	case DW_OP_breg22:
	case DW_OP_breg23:
	case DW_OP_breg24:
	case DW_OP_breg25:
	case DW_OP_breg26:
	case DW_OP_breg27:
	case DW_OP_breg28:
	case DW_OP_breg29:
	case DW_OP_breg30:
	case DW_OP_breg31:
	  printf ("DW_OP_breg%d (%s): %s",
		  op - DW_OP_breg0,
		  regname (op - DW_OP_breg0, 1),
		  dwarf_vmatoa ("d", (dwarf_signed_vma)
		    read_leb128 (data, &bytes_read, 1)));
	  data += bytes_read;
	  break;

	case DW_OP_regx:
	  uvalue = read_leb128 (data, &bytes_read, 0);
	  data += bytes_read;
	  printf ("DW_OP_regx: %s (%s)",
		  dwarf_vmatoa ("u", uvalue), regname (uvalue, 1));
	  break;
	case DW_OP_fbreg:
	  need_frame_base = 1;
	  printf ("DW_OP_fbreg: %s",
		  dwarf_vmatoa ("d", read_sleb128 (data, &bytes_read)));
	  data += bytes_read;
	  break;
	case DW_OP_bregx:
	  uvalue = read_leb128 (data, &bytes_read, 0);
	  data += bytes_read;
	  printf ("DW_OP_bregx: %s (%s) %s",
		  dwarf_vmatoa ("u", uvalue), regname (uvalue, 1),
		  dwarf_vmatoa ("d", read_sleb128 (data, &bytes_read)));
	  data += bytes_read;
	  break;
	case DW_OP_piece:
	  printf ("DW_OP_piece: %s",
		  dwarf_vmatoa ("u", read_leb128 (data, &bytes_read, 0)));
	  data += bytes_read;
	  break;
	case DW_OP_deref_size:
	  printf ("DW_OP_deref_size: %ld", (long) byte_get (data++, 1));
	  break;
	case DW_OP_xderef_size:
	  printf ("DW_OP_xderef_size: %ld", (long) byte_get (data++, 1));
	  break;
	case DW_OP_nop:
	  printf ("DW_OP_nop");
	  break;

	  /* DWARF 3 extensions.  */
	case DW_OP_push_object_address:
	  printf ("DW_OP_push_object_address");
	  break;
	case DW_OP_call2:
	  /* XXX: Strictly speaking for 64-bit DWARF3 files
	     this ought to be an 8-byte wide computation.  */
	  printf ("DW_OP_call2: <0x%s>",
		  dwarf_vmatoa ("x", (dwarf_signed_vma) byte_get (data, 2)
				     + cu_offset));
	  data += 2;
	  break;
	case DW_OP_call4:
	  /* XXX: Strictly speaking for 64-bit DWARF3 files
	     this ought to be an 8-byte wide computation.  */
	  printf ("DW_OP_call4: <0x%s>",
		  dwarf_vmatoa ("x", (dwarf_signed_vma) byte_get (data, 4)
				     + cu_offset));
	  data += 4;
	  break;
	case DW_OP_call_ref:
	  /* XXX: Strictly speaking for 64-bit DWARF3 files
	     this ought to be an 8-byte wide computation.  */
	  if (dwarf_version == -1)
	    {
	      printf (_("(DW_OP_call_ref in frame info)"));
	      /* No way to tell where the next op is, so just bail.  */
	      return need_frame_base;
	    }
	  if (dwarf_version == 2)
	    {
	      printf ("DW_OP_call_ref: <0x%s>",
		      dwarf_vmatoa ("x", byte_get (data, pointer_size)));
	      data += pointer_size;
	    }
	  else
	    {
	      printf ("DW_OP_call_ref: <0x%s>",
		      dwarf_vmatoa ("x", byte_get (data, offset_size)));
	      data += offset_size;
	    }
	  break;
	case DW_OP_form_tls_address:
	  printf ("DW_OP_form_tls_address");
	  break;
	case DW_OP_call_frame_cfa:
	  printf ("DW_OP_call_frame_cfa");
	  break;
	case DW_OP_bit_piece:
	  printf ("DW_OP_bit_piece: ");
	  printf (_("size: %s "),
		  dwarf_vmatoa ("u", read_leb128 (data, &bytes_read, 0)));
	  data += bytes_read;
	  printf (_("offset: %s "),
		  dwarf_vmatoa ("u", read_leb128 (data, &bytes_read, 0)));
	  data += bytes_read;
	  break;

	  /* DWARF 4 extensions.  */
	case DW_OP_stack_value:
	  printf ("DW_OP_stack_value");
	  break;

	case DW_OP_implicit_value:
	  printf ("DW_OP_implicit_value");
	  uvalue = read_leb128 (data, &bytes_read, 0);
	  data += bytes_read;
	  display_block (data, uvalue);
	  data += uvalue;
	  break;

	  /* GNU extensions.  */
	case DW_OP_GNU_push_tls_address:
	  printf (_("DW_OP_GNU_push_tls_address or DW_OP_HP_unknown"));
	  break;
	case DW_OP_GNU_uninit:
	  printf ("DW_OP_GNU_uninit");
	  /* FIXME: Is there data associated with this OP ?  */
	  break;
	case DW_OP_GNU_encoded_addr:
	  {
	    int encoding;
	    dwarf_vma addr;

	    encoding = *data++;
	    addr = get_encoded_value (data, encoding, section);
	    data += size_of_encoded_value (encoding);

	    printf ("DW_OP_GNU_encoded_addr: fmt:%02x addr:", encoding);
	    print_dwarf_vma (addr, pointer_size);
	  }
	  break;
	case DW_OP_GNU_implicit_pointer:
	  /* XXX: Strictly speaking for 64-bit DWARF3 files
	     this ought to be an 8-byte wide computation.  */
	  if (dwarf_version == -1)
	    {
	      printf (_("(DW_OP_GNU_implicit_pointer in frame info)"));
	      /* No way to tell where the next op is, so just bail.  */
	      return need_frame_base;
	    }
	  if (dwarf_version == 2)
	    {
	      printf ("DW_OP_GNU_implicit_pointer: <0x%s> %s",
		      dwarf_vmatoa ("x", byte_get (data, pointer_size)),
		      dwarf_vmatoa ("d", read_sleb128 (data + pointer_size,
				     &bytes_read)));
	      data += pointer_size + bytes_read;
	    }
	  else
	    {
	      printf ("DW_OP_GNU_implicit_pointer: <0x%s> %s",
		      dwarf_vmatoa ("x", byte_get (data, offset_size)),
		      dwarf_vmatoa ("d", read_sleb128 (data + offset_size,
				     &bytes_read)));
	      data += offset_size + bytes_read;
	    }
	  break;
	case DW_OP_GNU_entry_value:
	  uvalue = read_leb128 (data, &bytes_read, 0);
	  data += bytes_read;
	  printf ("DW_OP_GNU_entry_value: (");
	  if (decode_location_expression (data, pointer_size, offset_size,
					  dwarf_version, uvalue,
					  cu_offset, section))
	    need_frame_base = 1;
	  putchar (')');
	  data += uvalue;
	  break;
	case DW_OP_GNU_const_type:
	  uvalue = read_leb128 (data, &bytes_read, 0);
	  data += bytes_read;
	  printf ("DW_OP_GNU_const_type: <0x%s> ",
		  dwarf_vmatoa ("x", cu_offset + uvalue));
	  uvalue = byte_get (data++, 1);
	  display_block (data, uvalue);
	  data += uvalue;
	  break;
	case DW_OP_GNU_regval_type:
	  uvalue = read_leb128 (data, &bytes_read, 0);
	  data += bytes_read;
	  printf ("DW_OP_GNU_regval_type: %s (%s)",
		  dwarf_vmatoa ("u", uvalue), regname (uvalue, 1));
	  uvalue = read_leb128 (data, &bytes_read, 0);
	  data += bytes_read;
	  printf (" <0x%s>", dwarf_vmatoa ("x", cu_offset + uvalue));
	  break;
	case DW_OP_GNU_deref_type:
	  printf ("DW_OP_GNU_deref_type: %ld", (long) byte_get (data++, 1));
	  uvalue = read_leb128 (data, &bytes_read, 0);
	  data += bytes_read;
	  printf (" <0x%s>", dwarf_vmatoa ("x", cu_offset + uvalue));
	  break;
	case DW_OP_GNU_convert:
	  uvalue = read_leb128 (data, &bytes_read, 0);
	  data += bytes_read;
	  printf ("DW_OP_GNU_convert <0x%s>",
		  dwarf_vmatoa ("x", uvalue ? cu_offset + uvalue : 0));
	  break;
	case DW_OP_GNU_reinterpret:
	  uvalue = read_leb128 (data, &bytes_read, 0);
	  data += bytes_read;
	  printf ("DW_OP_GNU_reinterpret <0x%s>",
		  dwarf_vmatoa ("x", uvalue ? cu_offset + uvalue : 0));
	  break;
	case DW_OP_GNU_parameter_ref:
	  printf ("DW_OP_GNU_parameter_ref: <0x%s>",
		  dwarf_vmatoa ("x", cu_offset + byte_get (data, 4)));
	  data += 4;
	  break;
        case DW_OP_GNU_addr_index:
          uvalue = read_leb128 (data, &bytes_read, 0);
          data += bytes_read;
          printf ("DW_OP_GNU_addr_index <0x%s>", dwarf_vmatoa ("x", uvalue));
          break;

	  /* HP extensions.  */
	case DW_OP_HP_is_value:
	  printf ("DW_OP_HP_is_value");
	  /* FIXME: Is there data associated with this OP ?  */
	  break;
	case DW_OP_HP_fltconst4:
	  printf ("DW_OP_HP_fltconst4");
	  /* FIXME: Is there data associated with this OP ?  */
	  break;
	case DW_OP_HP_fltconst8:
	  printf ("DW_OP_HP_fltconst8");
	  /* FIXME: Is there data associated with this OP ?  */
	  break;
	case DW_OP_HP_mod_range:
	  printf ("DW_OP_HP_mod_range");
	  /* FIXME: Is there data associated with this OP ?  */
	  break;
	case DW_OP_HP_unmod_range:
	  printf ("DW_OP_HP_unmod_range");
	  /* FIXME: Is there data associated with this OP ?  */
	  break;
	case DW_OP_HP_tls:
	  printf ("DW_OP_HP_tls");
	  /* FIXME: Is there data associated with this OP ?  */
	  break;

	  /* PGI (STMicroelectronics) extensions.  */
	case DW_OP_PGI_omp_thread_num:
	  /* Pushes the thread number for the current thread as it would be
	     returned by the standard OpenMP library function:
	     omp_get_thread_num().  The "current thread" is the thread for
	     which the expression is being evaluated.  */
	  printf ("DW_OP_PGI_omp_thread_num");
	  break;

	default:
	  if (op >= DW_OP_lo_user
	      && op <= DW_OP_hi_user)
	    printf (_("(User defined location op)"));
	  else
	    printf (_("(Unknown location op)"));
	  /* No way to tell where the next op is, so just bail.  */
	  return need_frame_base;
	}

      /* Separate the ops.  */
      if (data < end)
	printf ("; ");
    }

  return need_frame_base;
}

static unsigned char *
read_and_display_attr_value (unsigned long attribute,
			     unsigned long form,
			     unsigned char * data,
			     dwarf_vma cu_offset,
			     dwarf_vma pointer_size,
			     dwarf_vma offset_size,
			     int dwarf_version,
			     debug_info * debug_info_p,
			     int do_loc,
			     struct dwarf_section * section)
{
  dwarf_vma uvalue = 0;
  unsigned char *block_start = NULL;
  unsigned char * orig_data = data;
  unsigned int bytes_read;

  switch (form)
    {
    default:
      break;

    case DW_FORM_ref_addr:
      if (dwarf_version == 2)
	{
	  uvalue = byte_get (data, pointer_size);
	  data += pointer_size;
	}
      else if (dwarf_version == 3 || dwarf_version == 4)
	{
	  uvalue = byte_get (data, offset_size);
	  data += offset_size;
	}
      else
	error (_("Internal error: DWARF version is not 2, 3 or 4.\n"));

      break;

    case DW_FORM_addr:
      uvalue = byte_get (data, pointer_size);
      data += pointer_size;
      break;

    case DW_FORM_strp:
    case DW_FORM_sec_offset:
    case DW_FORM_GNU_ref_alt:
    case DW_FORM_GNU_strp_alt:
      uvalue = byte_get (data, offset_size);
      data += offset_size;
      break;

    case DW_FORM_flag_present:
      uvalue = 1;
      break;

    case DW_FORM_ref1:
    case DW_FORM_flag:
    case DW_FORM_data1:
      uvalue = byte_get (data++, 1);
      break;

    case DW_FORM_ref2:
    case DW_FORM_data2:
      uvalue = byte_get (data, 2);
      data += 2;
      break;

    case DW_FORM_ref4:
    case DW_FORM_data4:
      uvalue = byte_get (data, 4);
      data += 4;
      break;

    case DW_FORM_sdata:
      uvalue = read_leb128 (data, & bytes_read, 1);
      data += bytes_read;
      break;

    case DW_FORM_GNU_str_index:
      uvalue = read_leb128 (data, & bytes_read, 0);
      data += bytes_read;
      break;

    case DW_FORM_ref_udata:
    case DW_FORM_udata:
      uvalue = read_leb128 (data, & bytes_read, 0);
      data += bytes_read;
      break;

    case DW_FORM_indirect:
      form = read_leb128 (data, & bytes_read, 0);
      data += bytes_read;
      if (!do_loc)
	printf (" %s", get_FORM_name (form));
      return read_and_display_attr_value (attribute, form, data,
					  cu_offset, pointer_size,
					  offset_size, dwarf_version,
					  debug_info_p, do_loc,
					  section);
    case DW_FORM_GNU_addr_index:
      uvalue = read_leb128 (data, & bytes_read, 0);
      data += bytes_read;
      break;
    }

  switch (form)
    {
    case DW_FORM_ref_addr:
      if (!do_loc)
	printf (" <0x%s>", dwarf_vmatoa ("x",uvalue));
      break;

    case DW_FORM_GNU_ref_alt:
      if (!do_loc)
	printf (" <alt 0x%s>", dwarf_vmatoa ("x",uvalue));
      break;

    case DW_FORM_ref1:
    case DW_FORM_ref2:
    case DW_FORM_ref4:
    case DW_FORM_ref_udata:
      if (!do_loc)
	printf (" <0x%s>", dwarf_vmatoa ("x", uvalue + cu_offset));
      break;

    case DW_FORM_data4:
    case DW_FORM_addr:
    case DW_FORM_sec_offset:
      if (!do_loc)
	printf (" 0x%s", dwarf_vmatoa ("x", uvalue));
      break;

    case DW_FORM_flag_present:
    case DW_FORM_flag:
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_sdata:
    case DW_FORM_udata:
      if (!do_loc)
	printf (" %s", dwarf_vmatoa ("d", uvalue));
      break;

    case DW_FORM_ref8:
    case DW_FORM_data8:
      if (!do_loc)
	{
	  dwarf_vma high_bits;
	  char buf[64];

	  byte_get_64 (data, &high_bits, &uvalue);
	  printf (" 0x%s",
		  dwarf_vmatoa64 (high_bits, uvalue, buf, sizeof (buf)));
	}
      if ((do_loc || do_debug_loc || do_debug_ranges)
	  && num_debug_info_entries == 0)
	{
	  if (sizeof (uvalue) == 8)
	    uvalue = byte_get (data, 8);
	  else
	    error (_("DW_FORM_data8 is unsupported when sizeof (dwarf_vma) != 8\n"));
	}
      data += 8;
      break;

    case DW_FORM_string:
      if (!do_loc)
	printf (" %s", data);
      data += strlen ((char *) data) + 1;
      break;

    case DW_FORM_block:
    case DW_FORM_exprloc:
      uvalue = read_leb128 (data, & bytes_read, 0);
      block_start = data + bytes_read;
      if (do_loc)
	data = block_start + uvalue;
      else
	data = display_block (block_start, uvalue);
      break;

    case DW_FORM_block1:
      uvalue = byte_get (data, 1);
      block_start = data + 1;
      if (do_loc)
	data = block_start + uvalue;
      else
	data = display_block (block_start, uvalue);
      break;

    case DW_FORM_block2:
      uvalue = byte_get (data, 2);
      block_start = data + 2;
      if (do_loc)
	data = block_start + uvalue;
      else
	data = display_block (block_start, uvalue);
      break;

    case DW_FORM_block4:
      uvalue = byte_get (data, 4);
      block_start = data + 4;
      if (do_loc)
	data = block_start + uvalue;
      else
	data = display_block (block_start, uvalue);
      break;

    case DW_FORM_strp:
      if (!do_loc)
	printf (_(" (indirect string, offset: 0x%s): %s"),
		dwarf_vmatoa ("x", uvalue),
		fetch_indirect_string (uvalue));
      break;

    case DW_FORM_GNU_str_index:
      if (!do_loc)
        {
          const char *suffix = strrchr (section->name, '.');
          int dwo = (suffix && strcmp (suffix, ".dwo") == 0) ? 1 : 0;

          printf (_(" (indexed string: 0x%s): %s"),
                  dwarf_vmatoa ("x", uvalue),
                  fetch_indexed_string (uvalue, offset_size, dwo));
        }
      break;

    case DW_FORM_GNU_strp_alt:
      if (!do_loc)
	printf (_(" (alt indirect string, offset: 0x%s)"),
		dwarf_vmatoa ("x", uvalue));
      break;

    case DW_FORM_indirect:
      /* Handled above.  */
      break;

    case DW_FORM_ref_sig8:
      if (!do_loc)
	{
	  dwarf_vma high_bits;
	  char buf[64];

	  byte_get_64 (data, &high_bits, &uvalue);
	  printf (" signature: 0x%s",
		  dwarf_vmatoa64 (high_bits, uvalue, buf, sizeof (buf)));
	}
      data += 8;
      break;

    case DW_FORM_GNU_addr_index:
      if (!do_loc)
        printf (_(" (addr_index: 0x%s): %s"),
                dwarf_vmatoa ("x", uvalue),
                fetch_indexed_value (uvalue * pointer_size, pointer_size));
      break;

    default:
      warn (_("Unrecognized form: %lu\n"), form);
      break;
    }

  if ((do_loc || do_debug_loc || do_debug_ranges)
      && num_debug_info_entries == 0
      && debug_info_p != NULL)
    {
      switch (attribute)
	{
	case DW_AT_frame_base:
	  have_frame_base = 1;
	case DW_AT_location:
	case DW_AT_string_length:
	case DW_AT_return_addr:
	case DW_AT_data_member_location:
	case DW_AT_vtable_elem_location:
	case DW_AT_segment:
	case DW_AT_static_link:
	case DW_AT_use_location:
	case DW_AT_GNU_call_site_value:
	case DW_AT_GNU_call_site_data_value:
	case DW_AT_GNU_call_site_target:
	case DW_AT_GNU_call_site_target_clobbered:
    	  if ((dwarf_version < 4
	       && (form == DW_FORM_data4 || form == DW_FORM_data8))
	      || form == DW_FORM_sec_offset)
	    {
	      /* Process location list.  */
	      unsigned int lmax = debug_info_p->max_loc_offsets;
	      unsigned int num = debug_info_p->num_loc_offsets;

	      if (lmax == 0 || num >= lmax)
		{
		  lmax += 1024;
		  debug_info_p->loc_offsets = (dwarf_vma *)
                      xcrealloc (debug_info_p->loc_offsets,
				 lmax, sizeof (*debug_info_p->loc_offsets));
		  debug_info_p->have_frame_base = (int *)
                      xcrealloc (debug_info_p->have_frame_base,
				 lmax, sizeof (*debug_info_p->have_frame_base));
		  debug_info_p->max_loc_offsets = lmax;
		}
	      debug_info_p->loc_offsets [num] = uvalue;
	      debug_info_p->have_frame_base [num] = have_frame_base;
	      debug_info_p->num_loc_offsets++;
	    }
	  break;

	case DW_AT_low_pc:
	  if (need_base_address)
	    debug_info_p->base_address = uvalue;
	  break;

	case DW_AT_GNU_addr_base:
          debug_info_p->addr_base = uvalue;
	  break;

	case DW_AT_GNU_ranges_base:
          debug_info_p->ranges_base = uvalue;
	  break;

	case DW_AT_ranges:
    	  if ((dwarf_version < 4
	       && (form == DW_FORM_data4 || form == DW_FORM_data8))
	      || form == DW_FORM_sec_offset)
	    {
	      /* Process range list.  */
	      unsigned int lmax = debug_info_p->max_range_lists;
	      unsigned int num = debug_info_p->num_range_lists;

	      if (lmax == 0 || num >= lmax)
		{
		  lmax += 1024;
		  debug_info_p->range_lists = (dwarf_vma *)
                      xcrealloc (debug_info_p->range_lists,
				 lmax, sizeof (*debug_info_p->range_lists));
		  debug_info_p->max_range_lists = lmax;
		}
	      debug_info_p->range_lists [num] = uvalue;
	      debug_info_p->num_range_lists++;
	    }
	  break;

	default:
	  break;
	}
    }

  if (do_loc || attribute == 0)
    return data;

  /* For some attributes we can display further information.  */
  printf ("\t");

  switch (attribute)
    {
    case DW_AT_inline:
      switch (uvalue)
	{
	case DW_INL_not_inlined:
	  printf (_("(not inlined)"));
	  break;
	case DW_INL_inlined:
	  printf (_("(inlined)"));
	  break;
	case DW_INL_declared_not_inlined:
	  printf (_("(declared as inline but ignored)"));
	  break;
	case DW_INL_declared_inlined:
	  printf (_("(declared as inline and inlined)"));
	  break;
	default:
	  printf (_("  (Unknown inline attribute value: %s)"),
		  dwarf_vmatoa ("x", uvalue));
	  break;
	}
      break;

    case DW_AT_language:
      switch (uvalue)
	{
	  /* Ordered by the numeric value of these constants.  */
	case DW_LANG_C89:		printf ("(ANSI C)"); break;
	case DW_LANG_C:			printf ("(non-ANSI C)"); break;
	case DW_LANG_Ada83:		printf ("(Ada)"); break;
	case DW_LANG_C_plus_plus:	printf ("(C++)"); break;
	case DW_LANG_Cobol74:		printf ("(Cobol 74)"); break;
	case DW_LANG_Cobol85:		printf ("(Cobol 85)"); break;
	case DW_LANG_Fortran77:		printf ("(FORTRAN 77)"); break;
	case DW_LANG_Fortran90:		printf ("(Fortran 90)"); break;
	case DW_LANG_Pascal83:		printf ("(ANSI Pascal)"); break;
	case DW_LANG_Modula2:		printf ("(Modula 2)"); break;
	  /* DWARF 2.1 values.	*/
	case DW_LANG_Java:		printf ("(Java)"); break;
	case DW_LANG_C99:		printf ("(ANSI C99)"); break;
	case DW_LANG_Ada95:		printf ("(ADA 95)"); break;
	case DW_LANG_Fortran95:		printf ("(Fortran 95)"); break;
	  /* DWARF 3 values.  */
	case DW_LANG_PLI:		printf ("(PLI)"); break;
	case DW_LANG_ObjC:		printf ("(Objective C)"); break;
	case DW_LANG_ObjC_plus_plus:	printf ("(Objective C++)"); break;
	case DW_LANG_UPC:		printf ("(Unified Parallel C)"); break;
	case DW_LANG_D:			printf ("(D)"); break;
	  /* DWARF 4 values.  */
	case DW_LANG_Python:		printf ("(Python)"); break;
	  /* DWARF 5 values.  */
	case DW_LANG_Go:		printf ("(Go)"); break;
	  /* MIPS extension.  */
	case DW_LANG_Mips_Assembler:	printf ("(MIPS assembler)"); break;
	  /* UPC extension.  */
	case DW_LANG_Upc:		printf ("(Unified Parallel C)"); break;
	default:
	  if (uvalue >= DW_LANG_lo_user && uvalue <= DW_LANG_hi_user)
	    printf (_("(implementation defined: %s)"),
		    dwarf_vmatoa ("x", uvalue));
	  else
	    printf (_("(Unknown: %s)"), dwarf_vmatoa ("x", uvalue));
	  break;
	}
      break;

    case DW_AT_encoding:
      switch (uvalue)
	{
	case DW_ATE_void:		printf ("(void)"); break;
	case DW_ATE_address:		printf ("(machine address)"); break;
	case DW_ATE_boolean:		printf ("(boolean)"); break;
	case DW_ATE_complex_float:	printf ("(complex float)"); break;
	case DW_ATE_float:		printf ("(float)"); break;
	case DW_ATE_signed:		printf ("(signed)"); break;
	case DW_ATE_signed_char:	printf ("(signed char)"); break;
	case DW_ATE_unsigned:		printf ("(unsigned)"); break;
	case DW_ATE_unsigned_char:	printf ("(unsigned char)"); break;
	  /* DWARF 2.1 values:  */
	case DW_ATE_imaginary_float:	printf ("(imaginary float)"); break;
	case DW_ATE_decimal_float:	printf ("(decimal float)"); break;
	  /* DWARF 3 values:  */
	case DW_ATE_packed_decimal:	printf ("(packed_decimal)"); break;
	case DW_ATE_numeric_string:	printf ("(numeric_string)"); break;
	case DW_ATE_edited:		printf ("(edited)"); break;
	case DW_ATE_signed_fixed:	printf ("(signed_fixed)"); break;
	case DW_ATE_unsigned_fixed:	printf ("(unsigned_fixed)"); break;
	  /* HP extensions:  */
	case DW_ATE_HP_float80:		printf ("(HP_float80)"); break;
	case DW_ATE_HP_complex_float80:	printf ("(HP_complex_float80)"); break;
	case DW_ATE_HP_float128:	printf ("(HP_float128)"); break;
	case DW_ATE_HP_complex_float128:printf ("(HP_complex_float128)"); break;
	case DW_ATE_HP_floathpintel:	printf ("(HP_floathpintel)"); break;
	case DW_ATE_HP_imaginary_float80:	printf ("(HP_imaginary_float80)"); break;
	case DW_ATE_HP_imaginary_float128:	printf ("(HP_imaginary_float128)"); break;

	default:
	  if (uvalue >= DW_ATE_lo_user
	      && uvalue <= DW_ATE_hi_user)
	    printf (_("(user defined type)"));
	  else
	    printf (_("(unknown type)"));
	  break;
	}
      break;

    case DW_AT_accessibility:
      switch (uvalue)
	{
	case DW_ACCESS_public:		printf ("(public)"); break;
	case DW_ACCESS_protected:	printf ("(protected)"); break;
	case DW_ACCESS_private:		printf ("(private)"); break;
	default:
	  printf (_("(unknown accessibility)"));
	  break;
	}
      break;

    case DW_AT_visibility:
      switch (uvalue)
	{
	case DW_VIS_local:		printf ("(local)"); break;
	case DW_VIS_exported:		printf ("(exported)"); break;
	case DW_VIS_qualified:		printf ("(qualified)"); break;
	default:			printf (_("(unknown visibility)")); break;
	}
      break;

    case DW_AT_virtuality:
      switch (uvalue)
	{
	case DW_VIRTUALITY_none:	printf ("(none)"); break;
	case DW_VIRTUALITY_virtual:	printf ("(virtual)"); break;
	case DW_VIRTUALITY_pure_virtual:printf ("(pure_virtual)"); break;
	default:			printf (_("(unknown virtuality)")); break;
	}
      break;

    case DW_AT_identifier_case:
      switch (uvalue)
	{
	case DW_ID_case_sensitive:	printf ("(case_sensitive)"); break;
	case DW_ID_up_case:		printf ("(up_case)"); break;
	case DW_ID_down_case:		printf ("(down_case)"); break;
	case DW_ID_case_insensitive:	printf ("(case_insensitive)"); break;
	default:			printf (_("(unknown case)")); break;
	}
      break;

    case DW_AT_calling_convention:
      switch (uvalue)
	{
	case DW_CC_normal:	printf ("(normal)"); break;
	case DW_CC_program:	printf ("(program)"); break;
	case DW_CC_nocall:	printf ("(nocall)"); break;
	default:
	  if (uvalue >= DW_CC_lo_user
	      && uvalue <= DW_CC_hi_user)
	    printf (_("(user defined)"));
	  else
	    printf (_("(unknown convention)"));
	}
      break;

    case DW_AT_ordering:
      switch (uvalue)
	{
	case -1: printf (_("(undefined)")); break;
	case 0:  printf ("(row major)"); break;
	case 1:  printf ("(column major)"); break;
	}
      break;

    case DW_AT_frame_base:
      have_frame_base = 1;
    case DW_AT_location:
    case DW_AT_string_length:
    case DW_AT_return_addr:
    case DW_AT_data_member_location:
    case DW_AT_vtable_elem_location:
    case DW_AT_segment:
    case DW_AT_static_link:
    case DW_AT_use_location:
    case DW_AT_GNU_call_site_value:
    case DW_AT_GNU_call_site_data_value:
    case DW_AT_GNU_call_site_target:
    case DW_AT_GNU_call_site_target_clobbered:
      if ((dwarf_version < 4
           && (form == DW_FORM_data4 || form == DW_FORM_data8))
	  || form == DW_FORM_sec_offset)
	printf (_("(location list)"));
      /* Fall through.  */
    case DW_AT_allocated:
    case DW_AT_associated:
    case DW_AT_data_location:
    case DW_AT_stride:
    case DW_AT_upper_bound:
    case DW_AT_lower_bound:
      if (block_start)
	{
	  int need_frame_base;

	  printf ("(");
	  need_frame_base = decode_location_expression (block_start,
							pointer_size,
							offset_size,
							dwarf_version,
							uvalue,
							cu_offset, section);
	  printf (")");
	  if (need_frame_base && !have_frame_base)
	    printf (_(" [without DW_AT_frame_base]"));
	}
      break;

    case DW_AT_import:
      {
	if (form == DW_FORM_ref_sig8
	    || form == DW_FORM_GNU_ref_alt)
          break;

	if (form == DW_FORM_ref1
	    || form == DW_FORM_ref2
	    || form == DW_FORM_ref4
	    || form == DW_FORM_ref_udata)
	  uvalue += cu_offset;

	if (uvalue >= section->size)
	  warn (_("Offset %s used as value for DW_AT_import attribute of DIE at offset %lx is too big.\n"),
		dwarf_vmatoa ("x", uvalue),
		(unsigned long) (orig_data - section->start));
	else
	  {
	    unsigned long abbrev_number;
	    abbrev_entry * entry;

	    abbrev_number = read_leb128 (section->start + uvalue, NULL, 0);

	    printf (_("[Abbrev Number: %ld"), abbrev_number);
	    /* Don't look up abbrev for DW_FORM_ref_addr, as it very often will
	       use different abbrev table, and we don't track .debug_info chunks
	       yet.  */
	    if (form != DW_FORM_ref_addr)
	      {
		for (entry = first_abbrev; entry != NULL; entry = entry->next)
		  if (entry->entry == abbrev_number)
		    break;
		if (entry != NULL)
		  printf (" (%s)", get_TAG_name (entry->tag));
	      }
	    printf ("]");
	  }
      }
      break;

    default:
      break;
    }

  return data;
}

static const char *
get_AT_name (unsigned long attribute)
{
  const char *name;

  /* One value is shared by the MIPS and HP extensions:  */
  if (attribute == DW_AT_MIPS_fde)
    return "DW_AT_MIPS_fde or DW_AT_HP_unmodifiable";

  name = get_DW_AT_name (attribute);

  if (name == NULL)
    {
      static char buffer[100];

      snprintf (buffer, sizeof (buffer), _("Unknown AT value: %lx"),
		attribute);
      return buffer;
    }

  return name;
}

static unsigned char *
read_and_display_attr (unsigned long attribute,
		       unsigned long form,
		       unsigned char * data,
		       dwarf_vma cu_offset,
		       dwarf_vma pointer_size,
		       dwarf_vma offset_size,
		       int dwarf_version,
		       debug_info * debug_info_p,
		       int do_loc,
		       struct dwarf_section * section)
{
  if (!do_loc)
    printf ("   %-18s:", get_AT_name (attribute));
  data = read_and_display_attr_value (attribute, form, data, cu_offset,
				      pointer_size, offset_size,
				      dwarf_version, debug_info_p,
				      do_loc, section);
  if (!do_loc)
    printf ("\n");
  return data;
}


/* Process the contents of a .debug_info section.  If do_loc is non-zero
   then we are scanning for location lists and we do not want to display
   anything to the user.  If do_types is non-zero, we are processing
   a .debug_types section instead of a .debug_info section.  */

static int
process_debug_info (struct dwarf_section *section,
		    void *file,
                    enum dwarf_section_display_enum abbrev_sec,
		    int do_loc,
		    int do_types)
{
  unsigned char *start = section->start;
  unsigned char *end = start + section->size;
  unsigned char *section_begin;
  unsigned int unit;
  unsigned int num_units = 0;

  if ((do_loc || do_debug_loc || do_debug_ranges)
      && num_debug_info_entries == 0
      && ! do_types)
    {
      dwarf_vma length;

      /* First scan the section to get the number of comp units.  */
      for (section_begin = start, num_units = 0; section_begin < end;
	   num_units ++)
	{
	  /* Read the first 4 bytes.  For a 32-bit DWARF section, this
	     will be the length.  For a 64-bit DWARF section, it'll be
	     the escape code 0xffffffff followed by an 8 byte length.  */
	  length = byte_get (section_begin, 4);

	  if (length == 0xffffffff)
	    {
	      length = byte_get (section_begin + 4, 8);
	      section_begin += length + 12;
	    }
	  else if (length >= 0xfffffff0 && length < 0xffffffff)
	    {
	      warn (_("Reserved length value (0x%s) found in section %s\n"),
		    dwarf_vmatoa ("x", length), section->name);
	      return 0;
	    }
	  else
	    section_begin += length + 4;

	  /* Negative values are illegal, they may even cause infinite
	     looping.  This can happen if we can't accurately apply
	     relocations to an object file.  */
	  if ((signed long) length <= 0)
	    {
	      warn (_("Corrupt unit length (0x%s) found in section %s\n"),
		    dwarf_vmatoa ("x", length), section->name);
	      return 0;
	    }
	}

      if (num_units == 0)
	{
	  error (_("No comp units in %s section ?"), section->name);
	  return 0;
	}

      /* Then allocate an array to hold the information.  */
      debug_information = (debug_info *) cmalloc (num_units,
                                                  sizeof (* debug_information));
      if (debug_information == NULL)
	{
	  error (_("Not enough memory for a debug info array of %u entries"),
		 num_units);
	  return 0;
	}
    }

  if (!do_loc)
    {
      if (dwarf_start_die == 0)
	printf (_("Contents of the %s section:\n\n"), section->name);

      load_debug_section (str, file);
      load_debug_section (str_dwo, file);
      load_debug_section (str_index, file);
      load_debug_section (str_index_dwo, file);
      load_debug_section (debug_addr, file);
    }

  load_debug_section (abbrev_sec, file);
  if (debug_displays [abbrev_sec].section.start == NULL)
    {
      warn (_("Unable to locate %s section!\n"),
	    debug_displays [abbrev_sec].section.name);
      return 0;
    }

  for (section_begin = start, unit = 0; start < end; unit++)
    {
      DWARF2_Internal_CompUnit compunit;
      unsigned char *hdrptr;
      unsigned char *tags;
      int level, last_level, saved_level;
      dwarf_vma cu_offset;
      int offset_size;
      int initial_length_size;
      dwarf_vma signature_high = 0;
      dwarf_vma signature_low = 0;
      dwarf_vma type_offset = 0;

      hdrptr = start;

      compunit.cu_length = byte_get (hdrptr, 4);
      hdrptr += 4;

      if (compunit.cu_length == 0xffffffff)
	{
	  compunit.cu_length = byte_get (hdrptr, 8);
	  hdrptr += 8;
	  offset_size = 8;
	  initial_length_size = 12;
	}
      else
	{
	  offset_size = 4;
	  initial_length_size = 4;
	}

      compunit.cu_version = byte_get (hdrptr, 2);
      hdrptr += 2;

      cu_offset = start - section_begin;

      compunit.cu_abbrev_offset = byte_get (hdrptr, offset_size);
      hdrptr += offset_size;

      compunit.cu_pointer_size = byte_get (hdrptr, 1);
      hdrptr += 1;

      if (do_types)
        {
          byte_get_64 (hdrptr, &signature_high, &signature_low);
          hdrptr += 8;
          type_offset = byte_get (hdrptr, offset_size);
          hdrptr += offset_size;
        }

      if ((do_loc || do_debug_loc || do_debug_ranges)
	  && num_debug_info_entries == 0
	  && ! do_types)
	{
	  debug_information [unit].cu_offset = cu_offset;
	  debug_information [unit].pointer_size
	    = compunit.cu_pointer_size;
	  debug_information [unit].offset_size = offset_size;
	  debug_information [unit].dwarf_version = compunit.cu_version;
	  debug_information [unit].base_address = 0;
	  debug_information [unit].addr_base = DEBUG_INFO_UNAVAILABLE;
	  debug_information [unit].ranges_base = DEBUG_INFO_UNAVAILABLE;
	  debug_information [unit].loc_offsets = NULL;
	  debug_information [unit].have_frame_base = NULL;
	  debug_information [unit].max_loc_offsets = 0;
	  debug_information [unit].num_loc_offsets = 0;
	  debug_information [unit].range_lists = NULL;
	  debug_information [unit].max_range_lists= 0;
	  debug_information [unit].num_range_lists = 0;
	}

      if (!do_loc && dwarf_start_die == 0)
	{
	  printf (_("  Compilation Unit @ offset 0x%s:\n"),
		  dwarf_vmatoa ("x", cu_offset));
	  printf (_("   Length:        0x%s (%s)\n"),
		  dwarf_vmatoa ("x", compunit.cu_length),
		  offset_size == 8 ? "64-bit" : "32-bit");
	  printf (_("   Version:       %d\n"), compunit.cu_version);
	  printf (_("   Abbrev Offset: 0x%s\n"),
		  dwarf_vmatoa ("x", compunit.cu_abbrev_offset));
	  printf (_("   Pointer Size:  %d\n"), compunit.cu_pointer_size);
	  if (do_types)
	    {
	      char buf[64];

	      printf (_("   Signature:     0x%s\n"),
		      dwarf_vmatoa64 (signature_high, signature_low,
				      buf, sizeof (buf)));
	      printf (_("   Type Offset:   0x%s\n"),
		      dwarf_vmatoa ("x", type_offset));
	    }
	}

      if (cu_offset + compunit.cu_length + initial_length_size
	  > section->size)
	{
	  warn (_("Debug info is corrupted, length of CU at %s"
	  	  " extends beyond end of section (length = %s)\n"),
		dwarf_vmatoa ("x", cu_offset),
		dwarf_vmatoa ("x", compunit.cu_length));
	  break;
	}
      tags = hdrptr;
      start += compunit.cu_length + initial_length_size;

      if (compunit.cu_version != 2
	  && compunit.cu_version != 3
	  && compunit.cu_version != 4)
	{
	  warn (_("CU at offset %s contains corrupt or "
		  "unsupported version number: %d.\n"),
		dwarf_vmatoa ("x", cu_offset), compunit.cu_version);
	  continue;
	}

      free_abbrevs ();

      /* Process the abbrevs used by this compilation unit. DWARF
	 sections under Mach-O have non-zero addresses.  */
      if (compunit.cu_abbrev_offset >= debug_displays [abbrev_sec].section.size)
	warn (_("Debug info is corrupted, abbrev offset (%lx) is larger than abbrev section size (%lx)\n"),
	      (unsigned long) compunit.cu_abbrev_offset,
	      (unsigned long) debug_displays [abbrev_sec].section.size);
      else
	process_abbrev_section
	  ((unsigned char *) debug_displays [abbrev_sec].section.start
	   + compunit.cu_abbrev_offset,
	   (unsigned char *) debug_displays [abbrev_sec].section.start
	   + debug_displays [abbrev_sec].section.size);

      level = 0;
      last_level = level;
      saved_level = -1;
      while (tags < start)
	{
	  unsigned int bytes_read;
	  unsigned long abbrev_number;
	  unsigned long die_offset;
	  abbrev_entry *entry;
	  abbrev_attr *attr;
	  int do_printing = 1;

	  die_offset = tags - section_begin;

	  abbrev_number = read_leb128 (tags, & bytes_read, 0);
	  tags += bytes_read;

	  /* A null DIE marks the end of a list of siblings or it may also be
	     a section padding.  */
	  if (abbrev_number == 0)
	    {
	      /* Check if it can be a section padding for the last CU.  */
	      if (level == 0 && start == end)
		{
		  unsigned char *chk;

		  for (chk = tags; chk < start; chk++)
		    if (*chk != 0)
		      break;
		  if (chk == start)
		    break;
		}

	      --level;
	      if (level < 0)
		{
		  static unsigned num_bogus_warns = 0;

		  if (num_bogus_warns < 3)
		    {
		      warn (_("Bogus end-of-siblings marker detected at offset %lx in %s section\n"),
			    die_offset, section->name);
		      num_bogus_warns ++;
		      if (num_bogus_warns == 3)
			warn (_("Further warnings about bogus end-of-sibling markers suppressed\n"));
		    }
		}
	      if (dwarf_start_die != 0 && level < saved_level)
		return 1;
	      continue;
	    }

	  if (!do_loc)
	    {
	      if (dwarf_start_die != 0 && die_offset < dwarf_start_die)
		do_printing = 0;
	      else
		{
		  if (dwarf_start_die != 0 && die_offset == dwarf_start_die)
		    saved_level = level;
		  do_printing = (dwarf_cutoff_level == -1
				 || level < dwarf_cutoff_level);
		  if (do_printing)
		    printf (_(" <%d><%lx>: Abbrev Number: %lu"),
			    level, die_offset, abbrev_number);
		  else if (dwarf_cutoff_level == -1
			   || last_level < dwarf_cutoff_level)
		    printf (_(" <%d><%lx>: ...\n"), level, die_offset);
		  last_level = level;
		}
	    }

	  /* Scan through the abbreviation list until we reach the
	     correct entry.  */
	  for (entry = first_abbrev;
	       entry && entry->entry != abbrev_number;
	       entry = entry->next)
	    continue;

	  if (entry == NULL)
	    {
	      if (!do_loc && do_printing)
		{
		  printf ("\n");
		  fflush (stdout);
		}
	      warn (_("DIE at offset %lx refers to abbreviation number %lu which does not exist\n"),
		    die_offset, abbrev_number);
	      return 0;
	    }

	  if (!do_loc && do_printing)
	    printf (" (%s)\n", get_TAG_name (entry->tag));

	  switch (entry->tag)
	    {
	    default:
	      need_base_address = 0;
	      break;
	    case DW_TAG_compile_unit:
	      need_base_address = 1;
	      break;
	    case DW_TAG_entry_point:
	    case DW_TAG_subprogram:
	      need_base_address = 0;
	      /* Assuming that there is no DW_AT_frame_base.  */
	      have_frame_base = 0;
	      break;
	    }

	  for (attr = entry->first_attr; attr; attr = attr->next)
	    {
	      debug_info *arg;

	      if (! do_loc && do_printing)
		/* Show the offset from where the tag was extracted.  */
		printf ("    <%lx>", (unsigned long)(tags - section_begin));

	      arg = debug_information;
	      if (debug_information)
		arg += unit;

	      tags = read_and_display_attr (attr->attribute,
					    attr->form,
					    tags, cu_offset,
					    compunit.cu_pointer_size,
					    offset_size,
					    compunit.cu_version,
					    arg,
					    do_loc || ! do_printing, section);
	    }

 	  if (entry->children)
 	    ++level;
 	}
    }

  /* Set num_debug_info_entries here so that it can be used to check if
     we need to process .debug_loc and .debug_ranges sections.  */
  if ((do_loc || do_debug_loc || do_debug_ranges)
      && num_debug_info_entries == 0
      && ! do_types)
    num_debug_info_entries = num_units;

  if (!do_loc)
    printf ("\n");

  return 1;
}

/* Locate and scan the .debug_info section in the file and record the pointer
   sizes and offsets for the compilation units in it.  Usually an executable
   will have just one pointer size, but this is not guaranteed, and so we try
   not to make any assumptions.  Returns zero upon failure, or the number of
   compilation units upon success.  */

static unsigned int
load_debug_info (void * file)
{
  /* Reset the last pointer size so that we can issue correct error
     messages if we are displaying the contents of more than one section.  */
  last_pointer_size = 0;
  warned_about_missing_comp_units = FALSE;

  /* If we have already tried and failed to load the .debug_info
     section then do not bother to repear the task.  */
  if (num_debug_info_entries == DEBUG_INFO_UNAVAILABLE)
    return 0;

  /* If we already have the information there is nothing else to do.  */
  if (num_debug_info_entries > 0)
    return num_debug_info_entries;

  if (load_debug_section (info, file)
      && process_debug_info (&debug_displays [info].section, file, abbrev, 1, 0))
    return num_debug_info_entries;
  else if (load_debug_section (info_dwo, file)
           && process_debug_info (&debug_displays [info_dwo].section, file,
                                  abbrev_dwo, 1, 0))
    return num_debug_info_entries;

  num_debug_info_entries = DEBUG_INFO_UNAVAILABLE;
  return 0;
}

static int
display_debug_lines_raw (struct dwarf_section *section,
			 unsigned char *data,
                         unsigned char *end)
{
  unsigned char *start = section->start;

  printf (_("Raw dump of debug contents of section %s:\n\n"),
          section->name);

  while (data < end)
    {
      DWARF2_Internal_LineInfo linfo;
      unsigned char *standard_opcodes;
      unsigned char *end_of_sequence;
      unsigned char *hdrptr;
      unsigned long hdroff;
      int initial_length_size;
      int offset_size;
      int i;

      hdrptr = data;
      hdroff = hdrptr - start;

      /* Check the length of the block.  */
      linfo.li_length = byte_get (hdrptr, 4);
      hdrptr += 4;

      if (linfo.li_length == 0xffffffff)
	{
	  /* This section is 64-bit DWARF 3.  */
	  linfo.li_length = byte_get (hdrptr, 8);
	  hdrptr += 8;
	  offset_size = 8;
	  initial_length_size = 12;
	}
      else
	{
	  offset_size = 4;
	  initial_length_size = 4;
	}

      if (linfo.li_length + initial_length_size > section->size)
	{
	  warn
	    (_("The information in section %s appears to be corrupt - the section is too small\n"),
	     section->name);
	  return 0;
	}

      /* Check its version number.  */
      linfo.li_version = byte_get (hdrptr, 2);
      hdrptr += 2;
      if (linfo.li_version != 2
	  && linfo.li_version != 3
	  && linfo.li_version != 4)
	{
	  warn (_("Only DWARF version 2, 3 and 4 line info is currently supported.\n"));
	  return 0;
	}

      linfo.li_prologue_length = byte_get (hdrptr, offset_size);
      hdrptr += offset_size;
      linfo.li_min_insn_length = byte_get (hdrptr, 1);
      hdrptr++;
      if (linfo.li_version >= 4)
	{
	  linfo.li_max_ops_per_insn = byte_get (hdrptr, 1);
	  hdrptr++;
	  if (linfo.li_max_ops_per_insn == 0)
	    {
	      warn (_("Invalid maximum operations per insn.\n"));
	      return 0;
	    }
	}
      else
	linfo.li_max_ops_per_insn = 1;
      linfo.li_default_is_stmt = byte_get (hdrptr, 1);
      hdrptr++;
      linfo.li_line_base = byte_get (hdrptr, 1);
      hdrptr++;
      linfo.li_line_range = byte_get (hdrptr, 1);
      hdrptr++;
      linfo.li_opcode_base = byte_get (hdrptr, 1);
      hdrptr++;

      /* Sign extend the line base field.  */
      linfo.li_line_base <<= 24;
      linfo.li_line_base >>= 24;

      printf (_("  Offset:                      0x%lx\n"), hdroff);
      printf (_("  Length:                      %ld\n"), (long) linfo.li_length);
      printf (_("  DWARF Version:               %d\n"), linfo.li_version);
      printf (_("  Prologue Length:             %d\n"), linfo.li_prologue_length);
      printf (_("  Minimum Instruction Length:  %d\n"), linfo.li_min_insn_length);
      if (linfo.li_version >= 4)
	printf (_("  Maximum Ops per Instruction: %d\n"), linfo.li_max_ops_per_insn);
      printf (_("  Initial value of 'is_stmt':  %d\n"), linfo.li_default_is_stmt);
      printf (_("  Line Base:                   %d\n"), linfo.li_line_base);
      printf (_("  Line Range:                  %d\n"), linfo.li_line_range);
      printf (_("  Opcode Base:                 %d\n"), linfo.li_opcode_base);

      end_of_sequence = data + linfo.li_length + initial_length_size;

      reset_state_machine (linfo.li_default_is_stmt);

      /* Display the contents of the Opcodes table.  */
      standard_opcodes = hdrptr;

      printf (_("\n Opcodes:\n"));

      for (i = 1; i < linfo.li_opcode_base; i++)
	printf (_("  Opcode %d has %d args\n"), i, standard_opcodes[i - 1]);

      /* Display the contents of the Directory table.  */
      data = standard_opcodes + linfo.li_opcode_base - 1;

      if (*data == 0)
	printf (_("\n The Directory Table is empty.\n"));
      else
	{
	  printf (_("\n The Directory Table:\n"));

	  while (*data != 0)
	    {
	      printf ("  %s\n", data);

	      data += strlen ((char *) data) + 1;
	    }
	}

      /* Skip the NUL at the end of the table.  */
      data++;

      /* Display the contents of the File Name table.  */
      if (*data == 0)
	printf (_("\n The File Name Table is empty.\n"));
      else
	{
	  printf (_("\n The File Name Table:\n"));
	  printf (_("  Entry\tDir\tTime\tSize\tName\n"));

	  while (*data != 0)
	    {
	      unsigned char *name;
	      unsigned int bytes_read;

	      printf ("  %d\t", ++state_machine_regs.last_file_entry);
	      name = data;

	      data += strlen ((char *) data) + 1;

	      printf ("%s\t",
		      dwarf_vmatoa ("u", read_leb128 (data, & bytes_read, 0)));
	      data += bytes_read;
	      printf ("%s\t",
		      dwarf_vmatoa ("u", read_leb128 (data, & bytes_read, 0)));
	      data += bytes_read;
	      printf ("%s\t",
		      dwarf_vmatoa ("u", read_leb128 (data, & bytes_read, 0)));
	      data += bytes_read;
	      printf ("%s\n", name);
	    }
	}

      /* Skip the NUL at the end of the table.  */
      data++;

      /* Now display the statements.  */
      printf (_("\n Line Number Statements:\n"));

      while (data < end_of_sequence)
	{
	  unsigned char op_code;
	  dwarf_signed_vma adv;
	  dwarf_vma uladv;
	  unsigned int bytes_read;

	  op_code = *data++;

	  if (op_code >= linfo.li_opcode_base)
	    {
	      op_code -= linfo.li_opcode_base;
	      uladv = (op_code / linfo.li_line_range);
	      if (linfo.li_max_ops_per_insn == 1)
		{
		  uladv *= linfo.li_min_insn_length;
		  state_machine_regs.address += uladv;
		  printf (_("  Special opcode %d: "
			    "advance Address by %s to 0x%s"),
			  op_code, dwarf_vmatoa ("u", uladv),
			  dwarf_vmatoa ("x", state_machine_regs.address));
		}
	      else
		{
		  state_machine_regs.address
		    += ((state_machine_regs.op_index + uladv)
			/ linfo.li_max_ops_per_insn)
		       * linfo.li_min_insn_length;
		  state_machine_regs.op_index
		    = (state_machine_regs.op_index + uladv)
		      % linfo.li_max_ops_per_insn;
		  printf (_("  Special opcode %d: "
			    "advance Address by %s to 0x%s[%d]"),
			  op_code, dwarf_vmatoa ("u", uladv),
			  dwarf_vmatoa ("x", state_machine_regs.address),
			  state_machine_regs.op_index);
		}
	      adv = (op_code % linfo.li_line_range) + linfo.li_line_base;
	      state_machine_regs.line += adv;
	      printf (_(" and Line by %s to %d\n"),
		      dwarf_vmatoa ("d", adv), state_machine_regs.line);
	    }
	  else switch (op_code)
	    {
	    case DW_LNS_extended_op:
	      data += process_extended_line_op (data, linfo.li_default_is_stmt);
	      break;

	    case DW_LNS_copy:
	      printf (_("  Copy\n"));
	      break;

	    case DW_LNS_advance_pc:
	      uladv = read_leb128 (data, & bytes_read, 0);
	      data += bytes_read;
	      if (linfo.li_max_ops_per_insn == 1)
		{
		  uladv *= linfo.li_min_insn_length;
		  state_machine_regs.address += uladv;
		  printf (_("  Advance PC by %s to 0x%s\n"),
			  dwarf_vmatoa ("u", uladv),
			  dwarf_vmatoa ("x", state_machine_regs.address));
		}
	      else
		{
		  state_machine_regs.address
		    += ((state_machine_regs.op_index + uladv)
			/ linfo.li_max_ops_per_insn)
		       * linfo.li_min_insn_length;
		  state_machine_regs.op_index
		    = (state_machine_regs.op_index + uladv)
		      % linfo.li_max_ops_per_insn;
		  printf (_("  Advance PC by %s to 0x%s[%d]\n"),
			  dwarf_vmatoa ("u", uladv),
			  dwarf_vmatoa ("x", state_machine_regs.address),
			  state_machine_regs.op_index);
		}
	      break;

	    case DW_LNS_advance_line:
	      adv = read_sleb128 (data, & bytes_read);
	      data += bytes_read;
	      state_machine_regs.line += adv;
	      printf (_("  Advance Line by %s to %d\n"),
		        dwarf_vmatoa ("d", adv),
			state_machine_regs.line);
	      break;

	    case DW_LNS_set_file:
	      adv = read_leb128 (data, & bytes_read, 0);
	      data += bytes_read;
	      printf (_("  Set File Name to entry %s in the File Name Table\n"),
		      dwarf_vmatoa ("d", adv));
	      state_machine_regs.file = adv;
	      break;

	    case DW_LNS_set_column:
	      uladv = read_leb128 (data, & bytes_read, 0);
	      data += bytes_read;
	      printf (_("  Set column to %s\n"),
		      dwarf_vmatoa ("u", uladv));
	      state_machine_regs.column = uladv;
	      break;

	    case DW_LNS_negate_stmt:
	      adv = state_machine_regs.is_stmt;
	      adv = ! adv;
	      printf (_("  Set is_stmt to %s\n"), dwarf_vmatoa ("d", adv));
	      state_machine_regs.is_stmt = adv;
	      break;

	    case DW_LNS_set_basic_block:
	      printf (_("  Set basic block\n"));
	      state_machine_regs.basic_block = 1;
	      break;

	    case DW_LNS_const_add_pc:
	      uladv = ((255 - linfo.li_opcode_base) / linfo.li_line_range);
	      if (linfo.li_max_ops_per_insn)
		{
		  uladv *= linfo.li_min_insn_length;
		  state_machine_regs.address += uladv;
		  printf (_("  Advance PC by constant %s to 0x%s\n"),
			  dwarf_vmatoa ("u", uladv),
			  dwarf_vmatoa ("x", state_machine_regs.address));
		}
	      else
		{
		  state_machine_regs.address
		    += ((state_machine_regs.op_index + uladv)
			/ linfo.li_max_ops_per_insn)
		       * linfo.li_min_insn_length;
		  state_machine_regs.op_index
		    = (state_machine_regs.op_index + uladv)
		      % linfo.li_max_ops_per_insn;
		  printf (_("  Advance PC by constant %s to 0x%s[%d]\n"),
			  dwarf_vmatoa ("u", uladv),
			  dwarf_vmatoa ("x", state_machine_regs.address),
			  state_machine_regs.op_index);
		}
	      break;

	    case DW_LNS_fixed_advance_pc:
	      uladv = byte_get (data, 2);
	      data += 2;
	      state_machine_regs.address += uladv;
	      state_machine_regs.op_index = 0;
	      printf (_("  Advance PC by fixed size amount %s to 0x%s\n"),
		      dwarf_vmatoa ("u", uladv),
		      dwarf_vmatoa ("x", state_machine_regs.address));
	      break;

	    case DW_LNS_set_prologue_end:
	      printf (_("  Set prologue_end to true\n"));
	      break;

	    case DW_LNS_set_epilogue_begin:
	      printf (_("  Set epilogue_begin to true\n"));
	      break;

	    case DW_LNS_set_isa:
	      uladv = read_leb128 (data, & bytes_read, 0);
	      data += bytes_read;
	      printf (_("  Set ISA to %s\n"), dwarf_vmatoa ("u", uladv));
	      break;

	    default:
	      printf (_("  Unknown opcode %d with operands: "), op_code);

	      for (i = standard_opcodes[op_code - 1]; i > 0 ; --i)
		{
		  printf ("0x%s%s", dwarf_vmatoa ("x", read_leb128 (data,
							 &bytes_read, 0)),
			  i == 1 ? "" : ", ");
		  data += bytes_read;
		}
	      putchar ('\n');
	      break;
	    }
	}
      putchar ('\n');
    }

  return 1;
}

typedef struct
{
  unsigned char *name;
  unsigned int directory_index;
  unsigned int modification_date;
  unsigned int length;
} File_Entry;

/* Output a decoded representation of the .debug_line section.  */

static int
display_debug_lines_decoded (struct dwarf_section *section,
			     unsigned char *data,
			     unsigned char *end)
{
  printf (_("Decoded dump of debug contents of section %s:\n\n"),
          section->name);

  while (data < end)
    {
      /* This loop amounts to one iteration per compilation unit.  */
      DWARF2_Internal_LineInfo linfo;
      unsigned char *standard_opcodes;
      unsigned char *end_of_sequence;
      unsigned char *hdrptr;
      int initial_length_size;
      int offset_size;
      int i;
      File_Entry *file_table = NULL;
      unsigned int n_files = 0;
      unsigned char **directory_table = NULL;
      unsigned int n_directories = 0;

      hdrptr = data;

      /* Extract information from the Line Number Program Header.
        (section 6.2.4 in the Dwarf3 doc).  */

      /* Get the length of this CU's line number information block.  */
      linfo.li_length = byte_get (hdrptr, 4);
      hdrptr += 4;

      if (linfo.li_length == 0xffffffff)
        {
          /* This section is 64-bit DWARF 3.  */
          linfo.li_length = byte_get (hdrptr, 8);
          hdrptr += 8;
          offset_size = 8;
          initial_length_size = 12;
        }
      else
        {
          offset_size = 4;
          initial_length_size = 4;
        }

      if (linfo.li_length + initial_length_size > section->size)
        {
          warn (_("The line info appears to be corrupt - "
                  "the section is too small\n"));
          return 0;
        }

      /* Get this CU's Line Number Block version number.  */
      linfo.li_version = byte_get (hdrptr, 2);
      hdrptr += 2;
      if (linfo.li_version != 2
	  && linfo.li_version != 3
	  && linfo.li_version != 4)
        {
          warn (_("Only DWARF version 2, 3 and 4 line info is currently "
                "supported.\n"));
          return 0;
        }

      linfo.li_prologue_length = byte_get (hdrptr, offset_size);
      hdrptr += offset_size;
      linfo.li_min_insn_length = byte_get (hdrptr, 1);
      hdrptr++;
      if (linfo.li_version >= 4)
	{
	  linfo.li_max_ops_per_insn = byte_get (hdrptr, 1);
	  hdrptr++;
	  if (linfo.li_max_ops_per_insn == 0)
	    {
	      warn (_("Invalid maximum operations per insn.\n"));
	      return 0;
	    }
	}
      else
	linfo.li_max_ops_per_insn = 1;
      linfo.li_default_is_stmt = byte_get (hdrptr, 1);
      hdrptr++;
      linfo.li_line_base = byte_get (hdrptr, 1);
      hdrptr++;
      linfo.li_line_range = byte_get (hdrptr, 1);
      hdrptr++;
      linfo.li_opcode_base = byte_get (hdrptr, 1);
      hdrptr++;

      /* Sign extend the line base field.  */
      linfo.li_line_base <<= 24;
      linfo.li_line_base >>= 24;

      /* Find the end of this CU's Line Number Information Block.  */
      end_of_sequence = data + linfo.li_length + initial_length_size;

      reset_state_machine (linfo.li_default_is_stmt);

      /* Save a pointer to the contents of the Opcodes table.  */
      standard_opcodes = hdrptr;

      /* Traverse the Directory table just to count entries.  */
      data = standard_opcodes + linfo.li_opcode_base - 1;
      if (*data != 0)
        {
          unsigned char *ptr_directory_table = data;

	  while (*data != 0)
	    {
	      data += strlen ((char *) data) + 1;
	      n_directories++;
	    }

          /* Go through the directory table again to save the directories.  */
          directory_table = (unsigned char **)
              xmalloc (n_directories * sizeof (unsigned char *));

          i = 0;
          while (*ptr_directory_table != 0)
            {
              directory_table[i] = ptr_directory_table;
              ptr_directory_table += strlen ((char *) ptr_directory_table) + 1;
              i++;
            }
        }
      /* Skip the NUL at the end of the table.  */
      data++;

      /* Traverse the File Name table just to count the entries.  */
      if (*data != 0)
        {
          unsigned char *ptr_file_name_table = data;

          while (*data != 0)
            {
	      unsigned int bytes_read;

              /* Skip Name, directory index, last modification time and length
                 of file.  */
              data += strlen ((char *) data) + 1;
              read_leb128 (data, & bytes_read, 0);
              data += bytes_read;
              read_leb128 (data, & bytes_read, 0);
              data += bytes_read;
              read_leb128 (data, & bytes_read, 0);
              data += bytes_read;

              n_files++;
            }

          /* Go through the file table again to save the strings.  */
          file_table = (File_Entry *) xmalloc (n_files * sizeof (File_Entry));

          i = 0;
          while (*ptr_file_name_table != 0)
            {
              unsigned int bytes_read;

              file_table[i].name = ptr_file_name_table;
              ptr_file_name_table += strlen ((char *) ptr_file_name_table) + 1;

              /* We are not interested in directory, time or size.  */
              file_table[i].directory_index = read_leb128 (ptr_file_name_table,
                                                           & bytes_read, 0);
              ptr_file_name_table += bytes_read;
              file_table[i].modification_date = read_leb128 (ptr_file_name_table,
							     & bytes_read, 0);
              ptr_file_name_table += bytes_read;
              file_table[i].length = read_leb128 (ptr_file_name_table, & bytes_read, 0);
              ptr_file_name_table += bytes_read;
              i++;
            }
          i = 0;

          /* Print the Compilation Unit's name and a header.  */
          if (directory_table == NULL)
            {
              printf (_("CU: %s:\n"), file_table[0].name);
              printf (_("File name                            Line number    Starting address\n"));
            }
          else
            {
              unsigned int ix = file_table[0].directory_index;
              const char *directory = ix ? (char *)directory_table[ix - 1] : ".";
              if (do_wide || strlen (directory) < 76)
		printf (_("CU: %s/%s:\n"), directory, file_table[0].name);
              else
		printf ("%s:\n", file_table[0].name);

              printf (_("File name                            Line number    Starting address\n"));
            }
        }

      /* Skip the NUL at the end of the table.  */
      data++;

      /* This loop iterates through the Dwarf Line Number Program.  */
      while (data < end_of_sequence)
        {
	  unsigned char op_code;
          int adv;
          unsigned long int uladv;
          unsigned int bytes_read;
          int is_special_opcode = 0;

          op_code = *data++;

          if (op_code >= linfo.li_opcode_base)
	    {
	      op_code -= linfo.li_opcode_base;
	      uladv = (op_code / linfo.li_line_range);
	      if (linfo.li_max_ops_per_insn == 1)
		{
		  uladv *= linfo.li_min_insn_length;
		  state_machine_regs.address += uladv;
		}
	      else
		{
		  state_machine_regs.address
		    += ((state_machine_regs.op_index + uladv)
			/ linfo.li_max_ops_per_insn)
		       * linfo.li_min_insn_length;
		  state_machine_regs.op_index
		    = (state_machine_regs.op_index + uladv)
		      % linfo.li_max_ops_per_insn;
		}

              adv = (op_code % linfo.li_line_range) + linfo.li_line_base;
              state_machine_regs.line += adv;
              is_special_opcode = 1;
            }
          else switch (op_code)
            {
            case DW_LNS_extended_op:
              {
                unsigned int ext_op_code_len;
                unsigned char ext_op_code;
                unsigned char *op_code_data = data;

                ext_op_code_len = read_leb128 (op_code_data, &bytes_read, 0);
                op_code_data += bytes_read;

                if (ext_op_code_len == 0)
                  {
                    warn (_("badly formed extended line op encountered!\n"));
                    break;
                  }
                ext_op_code_len += bytes_read;
                ext_op_code = *op_code_data++;

                switch (ext_op_code)
                  {
                  case DW_LNE_end_sequence:
                    reset_state_machine (linfo.li_default_is_stmt);
                    break;
                  case DW_LNE_set_address:
                    state_machine_regs.address =
                    byte_get (op_code_data, ext_op_code_len - bytes_read - 1);
		    state_machine_regs.op_index = 0;
                    break;
                  case DW_LNE_define_file:
                    {
                      file_table = (File_Entry *) xrealloc
                        (file_table, (n_files + 1) * sizeof (File_Entry));

                      ++state_machine_regs.last_file_entry;
                      /* Source file name.  */
                      file_table[n_files].name = op_code_data;
                      op_code_data += strlen ((char *) op_code_data) + 1;
                      /* Directory index.  */
                      file_table[n_files].directory_index =
                        read_leb128 (op_code_data, & bytes_read, 0);
                      op_code_data += bytes_read;
                      /* Last modification time.  */
                      file_table[n_files].modification_date =
                        read_leb128 (op_code_data, & bytes_read, 0);
                      op_code_data += bytes_read;
                      /* File length.  */
                      file_table[n_files].length =
                        read_leb128 (op_code_data, & bytes_read, 0);

                      n_files++;
                      break;
                    }
                  case DW_LNE_set_discriminator:
                  case DW_LNE_HP_set_sequence:
                    /* Simply ignored.  */
                    break;

                  default:
                    printf (_("UNKNOWN (%u): length %d\n"),
                            ext_op_code, ext_op_code_len - bytes_read);
                    break;
                  }
                data += ext_op_code_len;
                break;
              }
            case DW_LNS_copy:
              break;

            case DW_LNS_advance_pc:
              uladv = read_leb128 (data, & bytes_read, 0);
              data += bytes_read;
	      if (linfo.li_max_ops_per_insn == 1)
		{
		  uladv *= linfo.li_min_insn_length;
		  state_machine_regs.address += uladv;
		}
	      else
		{
		  state_machine_regs.address
		    += ((state_machine_regs.op_index + uladv)
			/ linfo.li_max_ops_per_insn)
		       * linfo.li_min_insn_length;
		  state_machine_regs.op_index
		    = (state_machine_regs.op_index + uladv)
		      % linfo.li_max_ops_per_insn;
		}
              break;

            case DW_LNS_advance_line:
              adv = read_sleb128 (data, & bytes_read);
              data += bytes_read;
              state_machine_regs.line += adv;
              break;

            case DW_LNS_set_file:
              adv = read_leb128 (data, & bytes_read, 0);
              data += bytes_read;
              state_machine_regs.file = adv;
              if (file_table[state_machine_regs.file - 1].directory_index == 0)
                {
                  /* If directory index is 0, that means current directory.  */
                  printf ("\n./%s:[++]\n",
                          file_table[state_machine_regs.file - 1].name);
                }
              else
                {
                  /* The directory index starts counting at 1.  */
                  printf ("\n%s/%s:\n",
                          directory_table[file_table[state_machine_regs.file - 1].directory_index - 1],
                          file_table[state_machine_regs.file - 1].name);
                }
              break;

            case DW_LNS_set_column:
              uladv = read_leb128 (data, & bytes_read, 0);
              data += bytes_read;
              state_machine_regs.column = uladv;
              break;

            case DW_LNS_negate_stmt:
              adv = state_machine_regs.is_stmt;
              adv = ! adv;
              state_machine_regs.is_stmt = adv;
              break;

            case DW_LNS_set_basic_block:
              state_machine_regs.basic_block = 1;
              break;

            case DW_LNS_const_add_pc:
	      uladv = ((255 - linfo.li_opcode_base) / linfo.li_line_range);
	      if (linfo.li_max_ops_per_insn == 1)
		{
		  uladv *= linfo.li_min_insn_length;
		  state_machine_regs.address += uladv;
		}
	      else
		{
		  state_machine_regs.address
		    += ((state_machine_regs.op_index + uladv)
			/ linfo.li_max_ops_per_insn)
		       * linfo.li_min_insn_length;
		  state_machine_regs.op_index
		    = (state_machine_regs.op_index + uladv)
		      % linfo.li_max_ops_per_insn;
		}
              break;

            case DW_LNS_fixed_advance_pc:
              uladv = byte_get (data, 2);
              data += 2;
              state_machine_regs.address += uladv;
	      state_machine_regs.op_index = 0;
              break;

            case DW_LNS_set_prologue_end:
              break;

            case DW_LNS_set_epilogue_begin:
              break;

            case DW_LNS_set_isa:
              uladv = read_leb128 (data, & bytes_read, 0);
              data += bytes_read;
              printf (_("  Set ISA to %lu\n"), uladv);
              break;

            default:
              printf (_("  Unknown opcode %d with operands: "), op_code);

              for (i = standard_opcodes[op_code - 1]; i > 0 ; --i)
                {
                  printf ("0x%s%s", dwarf_vmatoa ("x", read_leb128 (data,
							 &bytes_read, 0)),
                          i == 1 ? "" : ", ");
                  data += bytes_read;
                }
              putchar ('\n');
              break;
            }

          /* Only Special opcodes, DW_LNS_copy and DW_LNE_end_sequence adds a row
             to the DWARF address/line matrix.  */
          if ((is_special_opcode) || (op_code == DW_LNE_end_sequence)
	      || (op_code == DW_LNS_copy))
            {
              const unsigned int MAX_FILENAME_LENGTH = 35;
              char *fileName = (char *)file_table[state_machine_regs.file - 1].name;
              char *newFileName = NULL;
              size_t fileNameLength = strlen (fileName);

              if ((fileNameLength > MAX_FILENAME_LENGTH) && (!do_wide))
                {
                  newFileName = (char *) xmalloc (MAX_FILENAME_LENGTH + 1);
                  /* Truncate file name */
                  strncpy (newFileName,
                           fileName + fileNameLength - MAX_FILENAME_LENGTH,
                           MAX_FILENAME_LENGTH + 1);
                }
              else
                {
                  newFileName = (char *) xmalloc (fileNameLength + 1);
                  strncpy (newFileName, fileName, fileNameLength + 1);
                }

              if (!do_wide || (fileNameLength <= MAX_FILENAME_LENGTH))
                {
		  if (linfo.li_max_ops_per_insn == 1)
		    printf ("%-35s  %11d  %#18" DWARF_VMA_FMT "x\n",
			    newFileName, state_machine_regs.line,
			    state_machine_regs.address);
		  else
		    printf ("%-35s  %11d  %#18" DWARF_VMA_FMT "x[%d]\n",
			    newFileName, state_machine_regs.line,
			    state_machine_regs.address,
			    state_machine_regs.op_index);
                }
              else
                {
		  if (linfo.li_max_ops_per_insn == 1)
		    printf ("%s  %11d  %#18" DWARF_VMA_FMT "x\n",
			    newFileName, state_machine_regs.line,
			    state_machine_regs.address);
		  else
		    printf ("%s  %11d  %#18" DWARF_VMA_FMT "x[%d]\n",
			    newFileName, state_machine_regs.line,
			    state_machine_regs.address,
			    state_machine_regs.op_index);
                }

              if (op_code == DW_LNE_end_sequence)
		printf ("\n");

              free (newFileName);
            }
        }
      free (file_table);
      file_table = NULL;
      free (directory_table);
      directory_table = NULL;
      putchar ('\n');
    }

  return 1;
}

static int
display_debug_lines (struct dwarf_section *section, void *file ATTRIBUTE_UNUSED)
{
  unsigned char *data = section->start;
  unsigned char *end = data + section->size;
  int retValRaw = 1;
  int retValDecoded = 1;

  if (do_debug_lines == 0)
    do_debug_lines |= FLAG_DEBUG_LINES_RAW;

  if (do_debug_lines & FLAG_DEBUG_LINES_RAW)
    retValRaw = display_debug_lines_raw (section, data, end);

  if (do_debug_lines & FLAG_DEBUG_LINES_DECODED)
    retValDecoded = display_debug_lines_decoded (section, data, end);

  if (!retValRaw || !retValDecoded)
    return 0;

  return 1;
}

static debug_info *
find_debug_info_for_offset (unsigned long offset)
{
  unsigned int i;

  if (num_debug_info_entries == DEBUG_INFO_UNAVAILABLE)
    return NULL;

  for (i = 0; i < num_debug_info_entries; i++)
    if (debug_information[i].cu_offset == offset)
      return debug_information + i;

  return NULL;
}

static int
display_debug_pubnames (struct dwarf_section *section,
			void *file ATTRIBUTE_UNUSED)
{
  DWARF2_Internal_PubNames names;
  unsigned char *start = section->start;
  unsigned char *end = start + section->size;

  /* It does not matter if this load fails,
     we test for that later on.  */
  load_debug_info (file);

  printf (_("Contents of the %s section:\n\n"), section->name);

  while (start < end)
    {
      unsigned char *data;
      unsigned long offset;
      int offset_size, initial_length_size;

      data = start;

      names.pn_length = byte_get (data, 4);
      data += 4;
      if (names.pn_length == 0xffffffff)
	{
	  names.pn_length = byte_get (data, 8);
	  data += 8;
	  offset_size = 8;
	  initial_length_size = 12;
	}
      else
	{
	  offset_size = 4;
	  initial_length_size = 4;
	}

      names.pn_version = byte_get (data, 2);
      data += 2;

      names.pn_offset = byte_get (data, offset_size);
      data += offset_size;

      if (num_debug_info_entries != DEBUG_INFO_UNAVAILABLE
	  && num_debug_info_entries > 0
	  && find_debug_info_for_offset (names.pn_offset) == NULL)
	warn (_(".debug_info offset of 0x%lx in %s section does not point to a CU header.\n"),
	      (unsigned long) names.pn_offset, section->name);

      names.pn_size = byte_get (data, offset_size);
      data += offset_size;

      start += names.pn_length + initial_length_size;

      if (names.pn_version != 2 && names.pn_version != 3)
	{
	  static int warned = 0;

	  if (! warned)
	    {
	      warn (_("Only DWARF 2 and 3 pubnames are currently supported\n"));
	      warned = 1;
	    }

	  continue;
	}

      printf (_("  Length:                              %ld\n"),
	      (long) names.pn_length);
      printf (_("  Version:                             %d\n"),
	      names.pn_version);
      printf (_("  Offset into .debug_info section:     0x%lx\n"),
	      (unsigned long) names.pn_offset);
      printf (_("  Size of area in .debug_info section: %ld\n"),
	      (long) names.pn_size);

      printf (_("\n    Offset\tName\n"));

      do
	{
	  offset = byte_get (data, offset_size);

	  if (offset != 0)
	    {
	      data += offset_size;
	      printf ("    %-6lx\t%s\n", offset, data);
	      data += strlen ((char *) data) + 1;
	    }
	}
      while (offset != 0);
    }

  printf ("\n");
  return 1;
}

static int
display_debug_macinfo (struct dwarf_section *section,
		       void *file ATTRIBUTE_UNUSED)
{
  unsigned char *start = section->start;
  unsigned char *end = start + section->size;
  unsigned char *curr = start;
  unsigned int bytes_read;
  enum dwarf_macinfo_record_type op;

  printf (_("Contents of the %s section:\n\n"), section->name);

  while (curr < end)
    {
      unsigned int lineno;
      const char *string;

      op = (enum dwarf_macinfo_record_type) *curr;
      curr++;

      switch (op)
	{
	case DW_MACINFO_start_file:
	  {
	    unsigned int filenum;

	    lineno = read_leb128 (curr, & bytes_read, 0);
	    curr += bytes_read;
	    filenum = read_leb128 (curr, & bytes_read, 0);
	    curr += bytes_read;

	    printf (_(" DW_MACINFO_start_file - lineno: %d filenum: %d\n"),
		    lineno, filenum);
	  }
	  break;

	case DW_MACINFO_end_file:
	  printf (_(" DW_MACINFO_end_file\n"));
	  break;

	case DW_MACINFO_define:
	  lineno = read_leb128 (curr, & bytes_read, 0);
	  curr += bytes_read;
	  string = (char *) curr;
	  curr += strlen (string) + 1;
	  printf (_(" DW_MACINFO_define - lineno : %d macro : %s\n"),
		  lineno, string);
	  break;

	case DW_MACINFO_undef:
	  lineno = read_leb128 (curr, & bytes_read, 0);
	  curr += bytes_read;
	  string = (char *) curr;
	  curr += strlen (string) + 1;
	  printf (_(" DW_MACINFO_undef - lineno : %d macro : %s\n"),
		  lineno, string);
	  break;

	case DW_MACINFO_vendor_ext:
	  {
	    unsigned int constant;

	    constant = read_leb128 (curr, & bytes_read, 0);
	    curr += bytes_read;
	    string = (char *) curr;
	    curr += strlen (string) + 1;
	    printf (_(" DW_MACINFO_vendor_ext - constant : %d string : %s\n"),
		    constant, string);
	  }
	  break;
	}
    }

  return 1;
}

/* Given LINE_OFFSET into the .debug_line section, attempt to return
   filename and dirname corresponding to file name table entry with index
   FILEIDX.  Return NULL on failure.  */

static unsigned char *
get_line_filename_and_dirname (dwarf_vma line_offset, dwarf_vma fileidx,
			       unsigned char **dir_name)
{
  struct dwarf_section *section = &debug_displays [line].section;
  unsigned char *hdrptr, *dirtable, *file_name;
  unsigned int offset_size, initial_length_size;
  unsigned int version, opcode_base, bytes_read;
  dwarf_vma length, diridx;

  *dir_name = NULL;
  if (section->start == NULL
      || line_offset >= section->size
      || fileidx == 0)
    return NULL;

  hdrptr = section->start + line_offset;
  length = byte_get (hdrptr, 4);
  hdrptr += 4;
  if (length == 0xffffffff)
    {
      /* This section is 64-bit DWARF 3.  */
      length = byte_get (hdrptr, 8);
      hdrptr += 8;
      offset_size = 8;
      initial_length_size = 12;
    }
  else
    {
      offset_size = 4;
      initial_length_size = 4;
    }
  if (length + initial_length_size > section->size)
    return NULL;
  version = byte_get (hdrptr, 2);
  hdrptr += 2;
  if (version != 2 && version != 3 && version != 4)
    return NULL;
  hdrptr += offset_size + 1;/* Skip prologue_length and min_insn_length.  */
  if (version >= 4)
    hdrptr++;		    /* Skip max_ops_per_insn.  */
  hdrptr += 3;		    /* Skip default_is_stmt, line_base, line_range.  */
  opcode_base = byte_get (hdrptr, 1);
  if (opcode_base == 0)
    return NULL;
  hdrptr++;
  hdrptr += opcode_base - 1;
  dirtable = hdrptr;
  /* Skip over dirname table.  */
  while (*hdrptr != '\0')
    hdrptr += strlen ((char *) hdrptr) + 1;
  hdrptr++;		    /* Skip the NUL at the end of the table.  */
  /* Now skip over preceding filename table entries.  */
  for (; *hdrptr != '\0' && fileidx > 1; fileidx--)
    {
      hdrptr += strlen ((char *) hdrptr) + 1;
      read_leb128 (hdrptr, &bytes_read, 0);
      hdrptr += bytes_read;
      read_leb128 (hdrptr, &bytes_read, 0);
      hdrptr += bytes_read;
      read_leb128 (hdrptr, &bytes_read, 0);
      hdrptr += bytes_read;
    }
  if (*hdrptr == '\0')
    return NULL;
  file_name = hdrptr;
  hdrptr += strlen ((char *) hdrptr) + 1;
  diridx = read_leb128 (hdrptr, &bytes_read, 0);
  if (diridx == 0)
    return file_name;
  for (; *dirtable != '\0' && diridx > 1; diridx--)
    dirtable += strlen ((char *) dirtable) + 1;
  if (*dirtable == '\0')
    return NULL;
  *dir_name = dirtable;
  return file_name;
}

static int
display_debug_macro (struct dwarf_section *section,
		     void *file)
{
  unsigned char *start = section->start;
  unsigned char *end = start + section->size;
  unsigned char *curr = start;
  unsigned char *extended_op_buf[256];
  unsigned int bytes_read;

  load_debug_section (str, file);
  load_debug_section (line, file);

  printf (_("Contents of the %s section:\n\n"), section->name);

  while (curr < end)
    {
      unsigned int lineno, version, flags;
      unsigned int offset_size = 4;
      const char *string;
      dwarf_vma line_offset = 0, sec_offset = curr - start, offset;
      unsigned char **extended_ops = NULL;

      version = byte_get (curr, 2);
      curr += 2;

      if (version != 4)
	{
	  error (_("Only GNU extension to DWARF 4 of %s is currently supported.\n"),
		 section->name);
	  return 0;
	}

      flags = byte_get (curr++, 1);
      if (flags & 1)
	offset_size = 8;
      printf (_("  Offset:                      0x%lx\n"),
	      (unsigned long) sec_offset);
      printf (_("  Version:                     %d\n"), version);
      printf (_("  Offset size:                 %d\n"), offset_size);
      if (flags & 2)
	{
	  line_offset = byte_get (curr, offset_size);
	  curr += offset_size;
	  printf (_("  Offset into .debug_line:     0x%lx\n"),
		  (unsigned long) line_offset);
	}
      if (flags & 4)
	{
	  unsigned int i, count = byte_get (curr++, 1), op;
	  dwarf_vma nargs, n;
	  memset (extended_op_buf, 0, sizeof (extended_op_buf));
	  extended_ops = extended_op_buf;
	  if (count)
	    {
	      printf (_("  Extension opcode arguments:\n"));
	      for (i = 0; i < count; i++)
		{
		  op = byte_get (curr++, 1);
		  extended_ops[op] = curr;
		  nargs = read_leb128 (curr, &bytes_read, 0);
		  curr += bytes_read;
		  if (nargs == 0)
		    printf (_("    DW_MACRO_GNU_%02x has no arguments\n"), op);
		  else
		    {
		      printf (_("    DW_MACRO_GNU_%02x arguments: "), op);
		      for (n = 0; n < nargs; n++)
			{
			  unsigned int form = byte_get (curr++, 1);
			  printf ("%s%s", get_FORM_name (form),
				  n == nargs - 1 ? "\n" : ", ");
			  switch (form)
			    {
			    case DW_FORM_data1:
			    case DW_FORM_data2:
			    case DW_FORM_data4:
			    case DW_FORM_data8:
			    case DW_FORM_sdata:
			    case DW_FORM_udata:
			    case DW_FORM_block:
			    case DW_FORM_block1:
			    case DW_FORM_block2:
			    case DW_FORM_block4:
			    case DW_FORM_flag:
			    case DW_FORM_string:
			    case DW_FORM_strp:
			    case DW_FORM_sec_offset:
			      break;
			    default:
			      error (_("Invalid extension opcode form %s\n"),
				     get_FORM_name (form));
			      return 0;
			    }
			}
		    }
		}
	    }
	}
      printf ("\n");

      while (1)
	{
	  unsigned int op;

	  if (curr >= end)
	    {
	      error (_(".debug_macro section not zero terminated\n"));
	      return 0;
	    }

	  op = byte_get (curr++, 1);
	  if (op == 0)
	    break;

	  switch (op)
	    {
	    case DW_MACRO_GNU_start_file:
	      {
		unsigned int filenum;
		unsigned char *file_name = NULL, *dir_name = NULL;

		lineno = read_leb128 (curr, &bytes_read, 0);
		curr += bytes_read;
		filenum = read_leb128 (curr, &bytes_read, 0);
		curr += bytes_read;

		if ((flags & 2) == 0)
		  error (_("DW_MACRO_GNU_start_file used, but no .debug_line offset provided.\n"));
		else
		  file_name
		    = get_line_filename_and_dirname (line_offset, filenum,
						     &dir_name);
		if (file_name == NULL)
		  printf (_(" DW_MACRO_GNU_start_file - lineno: %d filenum: %d\n"),
			  lineno, filenum);
		else
		  printf (_(" DW_MACRO_GNU_start_file - lineno: %d filenum: %d filename: %s%s%s\n"),
			  lineno, filenum,
			  dir_name != NULL ? (const char *) dir_name : "",
			  dir_name != NULL ? "/" : "", file_name);
	      }
	      break;

	    case DW_MACRO_GNU_end_file:
	      printf (_(" DW_MACRO_GNU_end_file\n"));
	      break;

	    case DW_MACRO_GNU_define:
	      lineno = read_leb128 (curr, &bytes_read, 0);
	      curr += bytes_read;
	      string = (char *) curr;
	      curr += strlen (string) + 1;
	      printf (_(" DW_MACRO_GNU_define - lineno : %d macro : %s\n"),
		      lineno, string);
	      break;

	    case DW_MACRO_GNU_undef:
	      lineno = read_leb128 (curr, &bytes_read, 0);
	      curr += bytes_read;
	      string = (char *) curr;
	      curr += strlen (string) + 1;
	      printf (_(" DW_MACRO_GNU_undef - lineno : %d macro : %s\n"),
		      lineno, string);
	      break;

	    case DW_MACRO_GNU_define_indirect:
	      lineno = read_leb128 (curr, &bytes_read, 0);
	      curr += bytes_read;
	      offset = byte_get (curr, offset_size);
	      curr += offset_size;
	      string = fetch_indirect_string (offset);
	      printf (_(" DW_MACRO_GNU_define_indirect - lineno : %d macro : %s\n"),
		      lineno, string);
	      break;

	    case DW_MACRO_GNU_undef_indirect:
	      lineno = read_leb128 (curr, &bytes_read, 0);
	      curr += bytes_read;
	      offset = byte_get (curr, offset_size);
	      curr += offset_size;
	      string = fetch_indirect_string (offset);
	      printf (_(" DW_MACRO_GNU_undef_indirect - lineno : %d macro : %s\n"),
		      lineno, string);
	      break;

	    case DW_MACRO_GNU_transparent_include:
	      offset = byte_get (curr, offset_size);
	      curr += offset_size;
	      printf (_(" DW_MACRO_GNU_transparent_include - offset : 0x%lx\n"),
		      (unsigned long) offset);
	      break;

	    case DW_MACRO_GNU_define_indirect_alt:
	      lineno = read_leb128 (curr, &bytes_read, 0);
	      curr += bytes_read;
	      offset = byte_get (curr, offset_size);
	      curr += offset_size;
	      printf (_(" DW_MACRO_GNU_define_indirect_alt - lineno : %d macro offset : 0x%lx\n"),
		      lineno, (unsigned long) offset);
	      break;

	    case DW_MACRO_GNU_undef_indirect_alt:
	      lineno = read_leb128 (curr, &bytes_read, 0);
	      curr += bytes_read;
	      offset = byte_get (curr, offset_size);
	      curr += offset_size;
	      printf (_(" DW_MACRO_GNU_undef_indirect_alt - lineno : %d macro offset : 0x%lx\n"),
		      lineno, (unsigned long) offset);
	      break;

	    case DW_MACRO_GNU_transparent_include_alt:
	      offset = byte_get (curr, offset_size);
	      curr += offset_size;
	      printf (_(" DW_MACRO_GNU_transparent_include_alt - offset : 0x%lx\n"),
		      (unsigned long) offset);
	      break;

	    default:
	      if (extended_ops == NULL || extended_ops[op] == NULL)
		{
		  error (_(" Unknown macro opcode %02x seen\n"), op);
		  return 0;
		}
	      else
		{
		  /* Skip over unhandled opcodes.  */
		  dwarf_vma nargs, n;
		  unsigned char *desc = extended_ops[op];
		  nargs = read_leb128 (desc, &bytes_read, 0);
		  desc += bytes_read;
		  if (nargs == 0)
		    {
		      printf (_(" DW_MACRO_GNU_%02x\n"), op);
		      break;
		    }
		  printf (_(" DW_MACRO_GNU_%02x -"), op);
		  for (n = 0; n < nargs; n++)
		    {
		      curr
			= read_and_display_attr_value (0, byte_get (desc++, 1),
						       curr, 0, 0, offset_size,
						       version, NULL, 0, NULL);
		      if (n != nargs - 1)
			printf (",");
		    }
		  printf ("\n");
		}
	      break;
	    }
	}

      printf ("\n");
    }	

  return 1;
}

static int
display_debug_abbrev (struct dwarf_section *section,
		      void *file ATTRIBUTE_UNUSED)
{
  abbrev_entry *entry;
  unsigned char *start = section->start;
  unsigned char *end = start + section->size;

  printf (_("Contents of the %s section:\n\n"), section->name);

  do
    {
      unsigned char *last;

      free_abbrevs ();

      last = start;
      start = process_abbrev_section (start, end);

      if (first_abbrev == NULL)
	continue;

      printf (_("  Number TAG (0x%lx)\n"), (long) (last - section->start));

      for (entry = first_abbrev; entry; entry = entry->next)
	{
	  abbrev_attr *attr;

	  printf ("   %ld      %s    [%s]\n",
		  entry->entry,
		  get_TAG_name (entry->tag),
		  entry->children ? _("has children") : _("no children"));

	  for (attr = entry->first_attr; attr; attr = attr->next)
	    printf ("    %-18s %s\n",
		    get_AT_name (attr->attribute),
		    get_FORM_name (attr->form));
	}
    }
  while (start);

  printf ("\n");

  return 1;
}

/* Display a location list from a normal (ie, non-dwo) .debug_loc section.  */

static void
display_loc_list (struct dwarf_section *section,
                  unsigned char **start_ptr,
                  int debug_info_entry,
                  unsigned long offset,
                  unsigned long base_address,
                  int has_frame_base)
{
  unsigned char *start = *start_ptr;
  unsigned char *section_end = section->start + section->size;
  unsigned long cu_offset = debug_information [debug_info_entry].cu_offset;
  unsigned int pointer_size = debug_information [debug_info_entry].pointer_size;
  unsigned int offset_size = debug_information [debug_info_entry].offset_size;
  int dwarf_version = debug_information [debug_info_entry].dwarf_version;

  dwarf_vma begin;
  dwarf_vma end;
  unsigned short length;
  int need_frame_base;

  while (1)
    {
      if (start + 2 * pointer_size > section_end)
        {
          warn (_("Location list starting at offset 0x%lx is not terminated.\n"),
                offset);
          break;
        }

      /* Note: we use sign extension here in order to be sure that we can detect
         the -1 escape value.  Sign extension into the top 32 bits of a 32-bit
         address will not affect the values that we display since we always show
         hex values, and always the bottom 32-bits.  */
      begin = byte_get_signed (start, pointer_size);
      start += pointer_size;
      end = byte_get_signed (start, pointer_size);
      start += pointer_size;

      printf ("    %8.8lx ", offset);

      if (begin == 0 && end == 0)
        {
          printf (_("<End of list>\n"));
          break;
        }

      /* Check base address specifiers.  */
      if (begin == (dwarf_vma) -1 && end != (dwarf_vma) -1)
        {
          base_address = end;
          print_dwarf_vma (begin, pointer_size);
          print_dwarf_vma (end, pointer_size);
          printf (_("(base address)\n"));
          continue;
        }

      if (start + 2 > section_end)
        {
          warn (_("Location list starting at offset 0x%lx is not terminated.\n"),
                offset);
          break;
        }

      length = byte_get (start, 2);
      start += 2;

      if (start + length > section_end)
        {
          warn (_("Location list starting at offset 0x%lx is not terminated.\n"),
                offset);
          break;
        }

      print_dwarf_vma (begin + base_address, pointer_size);
      print_dwarf_vma (end + base_address, pointer_size);

      putchar ('(');
      need_frame_base = decode_location_expression (start,
                                                    pointer_size,
                                                    offset_size,
                                                    dwarf_version,
                                                    length,
                                                    cu_offset, section);
      putchar (')');

      if (need_frame_base && !has_frame_base)
        printf (_(" [without DW_AT_frame_base]"));

      if (begin == end)
        fputs (_(" (start == end)"), stdout);
      else if (begin > end)
        fputs (_(" (start > end)"), stdout);

      putchar ('\n');

      start += length;
    }

  *start_ptr = start;
}

/* Display a location list from a .dwo section. It uses address indexes rather
   than embedded addresses.  This code closely follows display_loc_list, but the
   two are sufficiently different that combining things is very ugly.  */

static void
display_loc_list_dwo (struct dwarf_section *section,
                      unsigned char **start_ptr,
                      int debug_info_entry,
                      unsigned long offset,
                      int has_frame_base)
{
  unsigned char *start = *start_ptr;
  unsigned char *section_end = section->start + section->size;
  unsigned long cu_offset = debug_information [debug_info_entry].cu_offset;
  unsigned int pointer_size = debug_information [debug_info_entry].pointer_size;
  unsigned int offset_size = debug_information [debug_info_entry].offset_size;
  int dwarf_version = debug_information [debug_info_entry].dwarf_version;
  int entry_type;
  unsigned short length;
  int need_frame_base;
  dwarf_vma idx;
  unsigned int bytes_read;

  while (1)
    {
      printf ("    %8.8lx ", offset);

      if (start + 2 > section_end)
        {
          warn (_("Location list starting at offset 0x%lx is not terminated.\n"),
                offset);
          break;
        }

      entry_type = byte_get (start, 1);
      start++;
      switch (entry_type)
        {
          case 0: /* A terminating entry.  */
            idx = byte_get (start, 1);
            start++;
            *start_ptr = start;
            if (idx == 0)
              printf (_("<End of list>\n"));
            else
              warn (_("Location list starting at offset 0x%lx is not terminated.\n"),
                    offset);
            return;
          case 1: /* A base-address entry.  */
            idx = read_leb128 (start, &bytes_read, 0);
            start += bytes_read;
            print_dwarf_vma (idx, pointer_size);
            printf (_("(base address index)\n"));
            continue;
          case 2: /* A normal entry.  */
            idx = read_leb128 (start, &bytes_read, 0);
            start += bytes_read;
            print_dwarf_vma (idx, pointer_size);
            idx = read_leb128 (start, &bytes_read, 0);
            start += bytes_read;
            print_dwarf_vma (idx, pointer_size);
            break;
          default:
            warn (_("Unknown location-list type 0x%x.\n"), entry_type);
            *start_ptr = start;
            return;
        }

      if (start + 2 > section_end)
        {
          warn (_("Location list starting at offset 0x%lx is not terminated.\n"),
                offset);
          break;
        }

      length = byte_get (start, 2);
      start += 2;

      if (start + length > section_end)
        {
          warn (_("Location list starting at offset 0x%lx is not terminated.\n"),
                offset);
          break;
        }

      putchar ('(');
      need_frame_base = decode_location_expression (start,
                                                    pointer_size,
                                                    offset_size,
                                                    dwarf_version,
                                                    length,
                                                    cu_offset, section);
      putchar (')');

      if (need_frame_base && !has_frame_base)
        printf (_(" [without DW_AT_frame_base]"));

      putchar ('\n');

      start += length;
    }

  *start_ptr = start;
}

/* Sort array of indexes in ascending order of loc_offsets[idx].  */

static dwarf_vma *loc_offsets;

static int
loc_offsets_compar (const void *ap, const void *bp)
{
  dwarf_vma a = loc_offsets[*(const unsigned int *) ap];
  dwarf_vma b = loc_offsets[*(const unsigned int *) bp];

  return (a > b) - (b > a);
}

static int
display_debug_loc (struct dwarf_section *section, void *file)
{
  unsigned char *start = section->start;
  unsigned long bytes;
  unsigned char *section_begin = start;
  unsigned int num_loc_list = 0;
  unsigned long last_offset = 0;
  unsigned int first = 0;
  unsigned int i;
  unsigned int j;
  unsigned int k;
  int seen_first_offset = 0;
  int locs_sorted = 1;
  unsigned char *next;
  unsigned int *array = NULL;
  const char *suffix = strrchr (section->name, '.');
  int is_dwo = 0;

  if (suffix && strcmp (suffix, ".dwo") == 0)
    is_dwo = 1;

  bytes = section->size;

  if (bytes == 0)
    {
      printf (_("\nThe %s section is empty.\n"), section->name);
      return 0;
    }

  if (load_debug_info (file) == 0)
    {
      warn (_("Unable to load/parse the .debug_info section, so cannot interpret the %s section.\n"),
	    section->name);
      return 0;
    }

  /* Check the order of location list in .debug_info section. If
     offsets of location lists are in the ascending order, we can
     use `debug_information' directly.  */
  for (i = 0; i < num_debug_info_entries; i++)
    {
      unsigned int num;

      num = debug_information [i].num_loc_offsets;
      if (num > num_loc_list)
	num_loc_list = num;

      /* Check if we can use `debug_information' directly.  */
      if (locs_sorted && num != 0)
	{
	  if (!seen_first_offset)
	    {
	      /* This is the first location list.  */
	      last_offset = debug_information [i].loc_offsets [0];
	      first = i;
	      seen_first_offset = 1;
	      j = 1;
	    }
	  else
	    j = 0;

	  for (; j < num; j++)
	    {
	      if (last_offset >
		  debug_information [i].loc_offsets [j])
		{
		  locs_sorted = 0;
		  break;
		}
	      last_offset = debug_information [i].loc_offsets [j];
	    }
	}
    }

  if (!seen_first_offset)
    error (_("No location lists in .debug_info section!\n"));

  /* DWARF sections under Mach-O have non-zero addresses.  */
  if (debug_information [first].num_loc_offsets > 0
      && debug_information [first].loc_offsets [0] != section->address)
    warn (_("Location lists in %s section start at 0x%s\n"),
	  section->name,
	  dwarf_vmatoa ("x", debug_information [first].loc_offsets [0]));

  if (!locs_sorted)
    array = (unsigned int *) xcmalloc (num_loc_list, sizeof (unsigned int));
  printf (_("Contents of the %s section:\n\n"), section->name);
  if (!is_dwo)
    printf (_("    Offset   Begin    End      Expression\n"));
  else
    printf (_("    Offset   Begin idx End idx  Expression\n"));

  seen_first_offset = 0;
  for (i = first; i < num_debug_info_entries; i++)
    {
      unsigned long offset;
      unsigned long base_address;
      int has_frame_base;

      if (!locs_sorted)
	{
	  for (k = 0; k < debug_information [i].num_loc_offsets; k++)
	    array[k] = k;
	  loc_offsets = debug_information [i].loc_offsets;
	  qsort (array, debug_information [i].num_loc_offsets,
		 sizeof (*array), loc_offsets_compar);
	}

      for (k = 0; k < debug_information [i].num_loc_offsets; k++)
	{
	  j = locs_sorted ? k : array[k];
	  if (k
	      && debug_information [i].loc_offsets [locs_sorted
						    ? k - 1 : array [k - 1]]
		 == debug_information [i].loc_offsets [j])
	    continue;
	  has_frame_base = debug_information [i].have_frame_base [j];
	  /* DWARF sections under Mach-O have non-zero addresses.  */
	  offset = debug_information [i].loc_offsets [j] - section->address;
	  next = section_begin + offset;
	  base_address = debug_information [i].base_address;

	  if (!seen_first_offset)
	    seen_first_offset = 1;
	  else
	    {
	      if (start < next)
		warn (_("There is a hole [0x%lx - 0x%lx] in .debug_loc section.\n"),
		      (unsigned long) (start - section_begin),
		      (unsigned long) (next - section_begin));
	      else if (start > next)
		warn (_("There is an overlap [0x%lx - 0x%lx] in .debug_loc section.\n"),
		      (unsigned long) (start - section_begin),
		      (unsigned long) (next - section_begin));
	    }
	  start = next;

	  if (offset >= bytes)
	    {
	      warn (_("Offset 0x%lx is bigger than .debug_loc section size.\n"),
		    offset);
	      continue;
	    }

          if (is_dwo)
            display_loc_list_dwo (section, &start, i, offset, has_frame_base);
          else
            display_loc_list (section, &start, i, offset, base_address,
                              has_frame_base);
	}
    }

  if (start < section->start + section->size)
    warn (_("There are %ld unused bytes at the end of section %s\n"),
	  (long) (section->start + section->size - start), section->name);
  putchar ('\n');
  free (array);
  return 1;
}

static int
display_debug_str (struct dwarf_section *section,
		   void *file ATTRIBUTE_UNUSED)
{
  unsigned char *start = section->start;
  unsigned long bytes = section->size;
  dwarf_vma addr = section->address;

  if (bytes == 0)
    {
      printf (_("\nThe %s section is empty.\n"), section->name);
      return 0;
    }

  printf (_("Contents of the %s section:\n\n"), section->name);

  while (bytes)
    {
      int j;
      int k;
      int lbytes;

      lbytes = (bytes > 16 ? 16 : bytes);

      printf ("  0x%8.8lx ", (unsigned long) addr);

      for (j = 0; j < 16; j++)
	{
	  if (j < lbytes)
	    printf ("%2.2x", start[j]);
	  else
	    printf ("  ");

	  if ((j & 3) == 3)
	    printf (" ");
	}

      for (j = 0; j < lbytes; j++)
	{
	  k = start[j];
	  if (k >= ' ' && k < 0x80)
	    printf ("%c", k);
	  else
	    printf (".");
	}

      putchar ('\n');

      start += lbytes;
      addr  += lbytes;
      bytes -= lbytes;
    }

  putchar ('\n');

  return 1;
}

static int
display_debug_info (struct dwarf_section *section, void *file)
{
  return process_debug_info (section, file, section->abbrev_sec, 0, 0);
}

static int
display_debug_types (struct dwarf_section *section, void *file)
{
  return process_debug_info (section, file, section->abbrev_sec, 0, 1);
}

static int
display_trace_info (struct dwarf_section *section, void *file)
{
  return process_debug_info (section, file, section->abbrev_sec, 0, 0);
}

static int
display_debug_aranges (struct dwarf_section *section,
		       void *file ATTRIBUTE_UNUSED)
{
  unsigned char *start = section->start;
  unsigned char *end = start + section->size;

  printf (_("Contents of the %s section:\n\n"), section->name);

  /* It does not matter if this load fails,
     we test for that later on.  */
  load_debug_info (file);

  while (start < end)
    {
      unsigned char *hdrptr;
      DWARF2_Internal_ARange arange;
      unsigned char *addr_ranges;
      dwarf_vma length;
      dwarf_vma address;
      unsigned char address_size;
      int excess;
      int offset_size;
      int initial_length_size;

      hdrptr = start;

      arange.ar_length = byte_get (hdrptr, 4);
      hdrptr += 4;

      if (arange.ar_length == 0xffffffff)
	{
	  arange.ar_length = byte_get (hdrptr, 8);
	  hdrptr += 8;
	  offset_size = 8;
	  initial_length_size = 12;
	}
      else
	{
	  offset_size = 4;
	  initial_length_size = 4;
	}

      arange.ar_version = byte_get (hdrptr, 2);
      hdrptr += 2;

      arange.ar_info_offset = byte_get (hdrptr, offset_size);
      hdrptr += offset_size;

      if (num_debug_info_entries != DEBUG_INFO_UNAVAILABLE
	  && num_debug_info_entries > 0
	  && find_debug_info_for_offset (arange.ar_info_offset) == NULL)
	warn (_(".debug_info offset of 0x%lx in %s section does not point to a CU header.\n"),
	      (unsigned long) arange.ar_info_offset, section->name);

      arange.ar_pointer_size = byte_get (hdrptr, 1);
      hdrptr += 1;

      arange.ar_segment_size = byte_get (hdrptr, 1);
      hdrptr += 1;

      if (arange.ar_version != 2 && arange.ar_version != 3)
	{
	  warn (_("Only DWARF 2 and 3 aranges are currently supported.\n"));
	  break;
	}

      printf (_("  Length:                   %ld\n"),
	      (long) arange.ar_length);
      printf (_("  Version:                  %d\n"), arange.ar_version);
      printf (_("  Offset into .debug_info:  0x%lx\n"),
	      (unsigned long) arange.ar_info_offset);
      printf (_("  Pointer Size:             %d\n"), arange.ar_pointer_size);
      printf (_("  Segment Size:             %d\n"), arange.ar_segment_size);

      address_size = arange.ar_pointer_size + arange.ar_segment_size;

      if (address_size == 0)
	{
	  error (_("Invalid address size in %s section!\n"),
		 section->name);
	  break;
	}

      /* The DWARF spec does not require that the address size be a power
	 of two, but we do.  This will have to change if we ever encounter
	 an uneven architecture.  */
      if ((address_size & (address_size - 1)) != 0)
	{
	  warn (_("Pointer size + Segment size is not a power of two.\n"));
	  break;
	}

      if (address_size > 4)
	printf (_("\n    Address            Length\n"));
      else
	printf (_("\n    Address    Length\n"));

      addr_ranges = hdrptr;

      /* Must pad to an alignment boundary that is twice the address size.  */
      excess = (hdrptr - start) % (2 * address_size);
      if (excess)
	addr_ranges += (2 * address_size) - excess;

      start += arange.ar_length + initial_length_size;

      while (addr_ranges + 2 * address_size <= start)
	{
	  address = byte_get (addr_ranges, address_size);

	  addr_ranges += address_size;

	  length  = byte_get (addr_ranges, address_size);

	  addr_ranges += address_size;

	  printf ("    ");
	  print_dwarf_vma (address, address_size);
	  print_dwarf_vma (length, address_size);
	  putchar ('\n');
	}
    }

  printf ("\n");

  return 1;
}

/* Comparison function for qsort.  */
static int
comp_addr_base (const void * v0, const void * v1)
{
  debug_info * info0 = (debug_info *) v0;
  debug_info * info1 = (debug_info *) v1;
  return info0->addr_base - info1->addr_base;
}

/* Display the debug_addr section.  */
static int
display_debug_addr (struct dwarf_section *section,
                    void *file)
{
  debug_info **debug_addr_info;
  unsigned char *entry;
  unsigned char *end;
  unsigned int i;
  unsigned int count;

  if (section->size == 0)
    {
      printf (_("\nThe %s section is empty.\n"), section->name);
      return 0;
    }

  if (load_debug_info (file) == 0)
    {
      warn (_("Unable to load/parse the .debug_info section, so cannot interpret the %s section.\n"),
	    section->name);
      return 0;
    }

  printf (_("Contents of the %s section:\n\n"), section->name);

  debug_addr_info = (debug_info **) xmalloc (num_debug_info_entries + 1
                                             * sizeof (debug_info *));

  count = 0;
  for (i = 0; i < num_debug_info_entries; i++)
    {
      if (debug_information [i].addr_base != DEBUG_INFO_UNAVAILABLE)
        debug_addr_info [count++] = &debug_information [i];
    }

  /* Add a sentinel to make iteration convenient.  */
  debug_addr_info [count] = (debug_info *) xmalloc (sizeof (debug_info));
  debug_addr_info [count]->addr_base = section->size;

  qsort (debug_addr_info, count, sizeof (debug_info *), comp_addr_base);
  for (i = 0; i < count; i++)
    {
      unsigned int idx;

      printf (_("  For compilation unit at offset 0x%s:\n"),
              dwarf_vmatoa ("x", debug_addr_info [i]->cu_offset));

      printf (_("\tIndex\tOffset\n"));
      entry = section->start + debug_addr_info [i]->addr_base;
      end = section->start + debug_addr_info [i + 1]->addr_base;
      idx = 0;
      while (entry < end)
        {
          dwarf_vma base = byte_get (entry, debug_addr_info [i]->pointer_size);
          printf (_("\t%d:\t%s\n"), idx, dwarf_vmatoa ("x", base));
          entry += debug_addr_info [i]->pointer_size;
          idx++;
        }
    }
  printf ("\n");

  free (debug_addr_info);
  return 1;
}

/* Display the .debug_str_offsets and .debug_str_offsets.dwo sections.  */
static int
display_debug_str_offsets (struct dwarf_section *section,
                           void *file ATTRIBUTE_UNUSED)
{
  if (section->size == 0)
    {
      printf (_("\nThe %s section is empty.\n"), section->name);
      return 0;
    }
  /* TODO: Dump the contents.  This is made somewhat difficult by not knowing
     what the offset size is for this section.  */
  return 1;
}

/* Each debug_information[x].range_lists[y] gets this representation for
   sorting purposes.  */

struct range_entry
{
  /* The debug_information[x].range_lists[y] value.  */
  unsigned long ranges_offset;

  /* Original debug_information to find parameters of the data.  */
  debug_info *debug_info_p;
};

/* Sort struct range_entry in ascending order of its RANGES_OFFSET.  */

static int
range_entry_compar (const void *ap, const void *bp)
{
  const struct range_entry *a_re = (const struct range_entry *) ap;
  const struct range_entry *b_re = (const struct range_entry *) bp;
  const unsigned long a = a_re->ranges_offset;
  const unsigned long b = b_re->ranges_offset;

  return (a > b) - (b > a);
}

static int
display_debug_ranges (struct dwarf_section *section,
		      void *file ATTRIBUTE_UNUSED)
{
  unsigned char *start = section->start;
  unsigned char *last_start = start;
  unsigned long bytes;
  unsigned char *section_begin = start;
  unsigned int num_range_list, i;
  struct range_entry *range_entries, *range_entry_fill;

  bytes = section->size;

  if (bytes == 0)
    {
      printf (_("\nThe %s section is empty.\n"), section->name);
      return 0;
    }

  if (load_debug_info (file) == 0)
    {
      warn (_("Unable to load/parse the .debug_info section, so cannot interpret the %s section.\n"),
	    section->name);
      return 0;
    }

  num_range_list = 0;
  for (i = 0; i < num_debug_info_entries; i++)
    num_range_list += debug_information [i].num_range_lists;

  if (num_range_list == 0)
    {
      /* This can happen when the file was compiled with -gsplit-debug
         which removes references to range lists from the primary .o file.  */
      printf (_("No range lists in .debug_info section.\n"));
      return 1;
    }

  range_entries = (struct range_entry *)
      xmalloc (sizeof (*range_entries) * num_range_list);
  range_entry_fill = range_entries;

  for (i = 0; i < num_debug_info_entries; i++)
    {
      debug_info *debug_info_p = &debug_information[i];
      unsigned int j;

      for (j = 0; j < debug_info_p->num_range_lists; j++)
	{
	  range_entry_fill->ranges_offset = debug_info_p->range_lists[j];
	  range_entry_fill->debug_info_p = debug_info_p;
	  range_entry_fill++;
	}
    }

  qsort (range_entries, num_range_list, sizeof (*range_entries),
	 range_entry_compar);

  /* DWARF sections under Mach-O have non-zero addresses.  */
  if (dwarf_check != 0 && range_entries[0].ranges_offset != section->address)
    warn (_("Range lists in %s section start at 0x%lx\n"),
	  section->name, range_entries[0].ranges_offset);

  printf (_("Contents of the %s section:\n\n"), section->name);
  printf (_("    Offset   Begin    End\n"));

  for (i = 0; i < num_range_list; i++)
    {
      struct range_entry *range_entry = &range_entries[i];
      debug_info *debug_info_p = range_entry->debug_info_p;
      unsigned int pointer_size;
      unsigned long offset;
      unsigned char *next;
      unsigned long base_address;

      pointer_size = debug_info_p->pointer_size;

      /* DWARF sections under Mach-O have non-zero addresses.  */
      offset = range_entry->ranges_offset - section->address;
      next = section_begin + offset;
      base_address = debug_info_p->base_address;

      if (dwarf_check != 0 && i > 0)
	{
	  if (start < next)
	    warn (_("There is a hole [0x%lx - 0x%lx] in %s section.\n"),
		  (unsigned long) (start - section_begin),
		  (unsigned long) (next - section_begin), section->name);
	  else if (start > next)
	    {
	      if (next == last_start)
		continue;
	      warn (_("There is an overlap [0x%lx - 0x%lx] in %s section.\n"),
		    (unsigned long) (start - section_begin),
		    (unsigned long) (next - section_begin), section->name);
	    }
	}
      start = next;
      last_start = next;

      while (1)
	{
	  dwarf_vma begin;
	  dwarf_vma end;

	  /* Note: we use sign extension here in order to be sure that
	     we can detect the -1 escape value.  Sign extension into the
	     top 32 bits of a 32-bit address will not affect the values
	     that we display since we always show hex values, and always
	     the bottom 32-bits.  */
	  begin = byte_get_signed (start, pointer_size);
	  start += pointer_size;
	  end = byte_get_signed (start, pointer_size);
	  start += pointer_size;

	  printf ("    %8.8lx ", offset);

	  if (begin == 0 && end == 0)
	    {
	      printf (_("<End of list>\n"));
	      break;
	    }

	  /* Check base address specifiers.  */
	  if (begin == (dwarf_vma) -1 && end != (dwarf_vma) -1)
	    {
	      base_address = end;
	      print_dwarf_vma (begin, pointer_size);
	      print_dwarf_vma (end, pointer_size);
	      printf ("(base address)\n");
	      continue;
	    }

	  print_dwarf_vma (begin + base_address, pointer_size);
	  print_dwarf_vma (end + base_address, pointer_size);

	  if (begin == end)
	    fputs (_("(start == end)"), stdout);
	  else if (begin > end)
	    fputs (_("(start > end)"), stdout);

	  putchar ('\n');
	}
    }
  putchar ('\n');

  free (range_entries);

  return 1;
}

typedef struct Frame_Chunk
{
  struct Frame_Chunk *next;
  unsigned char *chunk_start;
  int ncols;
  /* DW_CFA_{undefined,same_value,offset,register,unreferenced}  */
  short int *col_type;
  int *col_offset;
  char *augmentation;
  unsigned int code_factor;
  int data_factor;
  unsigned long pc_begin;
  unsigned long pc_range;
  int cfa_reg;
  int cfa_offset;
  int ra;
  unsigned char fde_encoding;
  unsigned char cfa_exp;
  unsigned char ptr_size;
  unsigned char segment_size;
}
Frame_Chunk;

static const char *const *dwarf_regnames;
static unsigned int dwarf_regnames_count;

/* A marker for a col_type that means this column was never referenced
   in the frame info.  */
#define DW_CFA_unreferenced (-1)

/* Return 0 if not more space is needed, 1 if more space is needed,
   -1 for invalid reg.  */

static int
frame_need_space (Frame_Chunk *fc, unsigned int reg)
{
  int prev = fc->ncols;

  if (reg < (unsigned int) fc->ncols)
    return 0;

  if (dwarf_regnames_count
      && reg > dwarf_regnames_count)
    return -1;

  fc->ncols = reg + 1;
  fc->col_type = (short int *) xcrealloc (fc->col_type, fc->ncols,
                                          sizeof (short int));
  fc->col_offset = (int *) xcrealloc (fc->col_offset, fc->ncols, sizeof (int));

  while (prev < fc->ncols)
    {
      fc->col_type[prev] = DW_CFA_unreferenced;
      fc->col_offset[prev] = 0;
      prev++;
    }
  return 1;
}

static const char *const dwarf_regnames_i386[] =
{
  "eax", "ecx", "edx", "ebx",
  "esp", "ebp", "esi", "edi",
  "eip", "eflags", NULL,
  "st0", "st1", "st2", "st3",
  "st4", "st5", "st6", "st7",
  NULL, NULL,
  "xmm0", "xmm1", "xmm2", "xmm3",
  "xmm4", "xmm5", "xmm6", "xmm7",
  "mm0", "mm1", "mm2", "mm3",
  "mm4", "mm5", "mm6", "mm7",
  "fcw", "fsw", "mxcsr",
  "es", "cs", "ss", "ds", "fs", "gs", NULL, NULL,
  "tr", "ldtr"
};

void
init_dwarf_regnames_i386 (void)
{
  dwarf_regnames = dwarf_regnames_i386;
  dwarf_regnames_count = ARRAY_SIZE (dwarf_regnames_i386);
}

static const char *const dwarf_regnames_x86_64[] =
{
  "rax", "rdx", "rcx", "rbx",
  "rsi", "rdi", "rbp", "rsp",
  "r8",  "r9",  "r10", "r11",
  "r12", "r13", "r14", "r15",
  "rip",
  "xmm0",  "xmm1",  "xmm2",  "xmm3",
  "xmm4",  "xmm5",  "xmm6",  "xmm7",
  "xmm8",  "xmm9",  "xmm10", "xmm11",
  "xmm12", "xmm13", "xmm14", "xmm15",
  "st0", "st1", "st2", "st3",
  "st4", "st5", "st6", "st7",
  "mm0", "mm1", "mm2", "mm3",
  "mm4", "mm5", "mm6", "mm7",
  "rflags",
  "es", "cs", "ss", "ds", "fs", "gs", NULL, NULL,
  "fs.base", "gs.base", NULL, NULL,
  "tr", "ldtr",
  "mxcsr", "fcw", "fsw"
};

void
init_dwarf_regnames_x86_64 (void)
{
  dwarf_regnames = dwarf_regnames_x86_64;
  dwarf_regnames_count = ARRAY_SIZE (dwarf_regnames_x86_64);
}

void
init_dwarf_regnames (unsigned int e_machine)
{
  switch (e_machine)
    {
    case EM_386:
    case EM_486:
      init_dwarf_regnames_i386 ();
      break;

    case EM_X86_64:
    case EM_L1OM:
    case EM_K1OM:
      init_dwarf_regnames_x86_64 ();
      break;

    default:
      break;
    }
}

static const char *
regname (unsigned int regno, int row)
{
  static char reg[64];
  if (dwarf_regnames
      && regno < dwarf_regnames_count
      && dwarf_regnames [regno] != NULL)
    {
      if (row)
	return dwarf_regnames [regno];
      snprintf (reg, sizeof (reg), "r%d (%s)", regno,
		dwarf_regnames [regno]);
    }
  else
    snprintf (reg, sizeof (reg), "r%d", regno);
  return reg;
}

static void
frame_display_row (Frame_Chunk *fc, int *need_col_headers, int *max_regs)
{
  int r;
  char tmp[100];

  if (*max_regs < fc->ncols)
    *max_regs = fc->ncols;

  if (*need_col_headers)
    {
      static const char *sloc = "   LOC";

      *need_col_headers = 0;

      printf ("%-*s CFA      ", eh_addr_size * 2, sloc);

      for (r = 0; r < *max_regs; r++)
	if (fc->col_type[r] != DW_CFA_unreferenced)
	  {
	    if (r == fc->ra)
	      printf ("ra      ");
	    else
	      printf ("%-5s ", regname (r, 1));
	  }

      printf ("\n");
    }

  printf ("%0*lx ", eh_addr_size * 2, fc->pc_begin);
  if (fc->cfa_exp)
    strcpy (tmp, "exp");
  else
    sprintf (tmp, "%s%+d", regname (fc->cfa_reg, 1), fc->cfa_offset);
  printf ("%-8s ", tmp);

  for (r = 0; r < fc->ncols; r++)
    {
      if (fc->col_type[r] != DW_CFA_unreferenced)
	{
	  switch (fc->col_type[r])
	    {
	    case DW_CFA_undefined:
	      strcpy (tmp, "u");
	      break;
	    case DW_CFA_same_value:
	      strcpy (tmp, "s");
	      break;
	    case DW_CFA_offset:
	      sprintf (tmp, "c%+d", fc->col_offset[r]);
	      break;
	    case DW_CFA_val_offset:
	      sprintf (tmp, "v%+d", fc->col_offset[r]);
	      break;
	    case DW_CFA_register:
	      sprintf (tmp, "%s", regname (fc->col_offset[r], 0));
	      break;
	    case DW_CFA_expression:
	      strcpy (tmp, "exp");
	      break;
	    case DW_CFA_val_expression:
	      strcpy (tmp, "vexp");
	      break;
	    default:
	      strcpy (tmp, "n/a");
	      break;
	    }
	  printf ("%-5s ", tmp);
	}
    }
  printf ("\n");
}

#define GET(N)	byte_get (start, N); start += N
#define LEB()	read_leb128 (start, & length_return, 0); start += length_return
#define SLEB()	read_sleb128 (start, & length_return); start += length_return

static int
display_debug_frames (struct dwarf_section *section,
		      void *file ATTRIBUTE_UNUSED)
{
  unsigned char *start = section->start;
  unsigned char *end = start + section->size;
  unsigned char *section_start = start;
  Frame_Chunk *chunks = 0;
  Frame_Chunk *remembered_state = 0;
  Frame_Chunk *rs;
  int is_eh = strcmp (section->name, ".eh_frame") == 0;
  unsigned int length_return;
  int max_regs = 0;
  const char *bad_reg = _("bad register: ");
  int saved_eh_addr_size = eh_addr_size;

  printf (_("Contents of the %s section:\n"), section->name);

  while (start < end)
    {
      unsigned char *saved_start;
      unsigned char *block_end;
      unsigned long length;
      unsigned long cie_id;
      Frame_Chunk *fc;
      Frame_Chunk *cie;
      int need_col_headers = 1;
      unsigned char *augmentation_data = NULL;
      unsigned long augmentation_data_len = 0;
      int encoded_ptr_size = saved_eh_addr_size;
      int offset_size;
      int initial_length_size;

      saved_start = start;
      length = byte_get (start, 4); start += 4;

      if (length == 0)
	{
	  printf ("\n%08lx ZERO terminator\n\n",
		    (unsigned long)(saved_start - section_start));
	  continue;
	}

      if (length == 0xffffffff)
	{
	  length = byte_get (start, 8);
	  start += 8;
	  offset_size = 8;
	  initial_length_size = 12;
	}
      else
	{
	  offset_size = 4;
	  initial_length_size = 4;
	}

      block_end = saved_start + length + initial_length_size;
      if (block_end > end)
	{
	  warn ("Invalid length %#08lx in FDE at %#08lx\n",
		length, (unsigned long)(saved_start - section_start));
	  block_end = end;
	}
      cie_id = byte_get (start, offset_size); start += offset_size;

      if (is_eh ? (cie_id == 0) : (cie_id == DW_CIE_ID))
	{
	  int version;

	  fc = (Frame_Chunk *) xmalloc (sizeof (Frame_Chunk));
	  memset (fc, 0, sizeof (Frame_Chunk));

	  fc->next = chunks;
	  chunks = fc;
	  fc->chunk_start = saved_start;
	  fc->ncols = 0;
	  fc->col_type = (short int *) xmalloc (sizeof (short int));
	  fc->col_offset = (int *) xmalloc (sizeof (int));
	  frame_need_space (fc, max_regs - 1);

	  version = *start++;

	  fc->augmentation = (char *) start;
	  start = (unsigned char *) strchr ((char *) start, '\0') + 1;

	  if (strcmp (fc->augmentation, "eh") == 0)
	    start += eh_addr_size;

	  if (version >= 4)
	    {
	      fc->ptr_size = GET (1);
	      fc->segment_size = GET (1);
	      eh_addr_size = fc->ptr_size;
	    }
	  else
	    {
	      fc->ptr_size = eh_addr_size;
	      fc->segment_size = 0;
	    }
	  fc->code_factor = LEB ();
	  fc->data_factor = SLEB ();
	  if (version == 1)
	    {
	      fc->ra = GET (1);
	    }
	  else
	    {
	      fc->ra = LEB ();
	    }

	  if (fc->augmentation[0] == 'z')
	    {
	      augmentation_data_len = LEB ();
	      augmentation_data = start;
	      start += augmentation_data_len;
	    }
	  cie = fc;

	  if (do_debug_frames_interp)
	    printf ("\n%08lx %08lx %08lx CIE \"%s\" cf=%d df=%d ra=%d\n",
		    (unsigned long)(saved_start - section_start), length, cie_id,
		    fc->augmentation, fc->code_factor, fc->data_factor,
		    fc->ra);
	  else
	    {
	      printf ("\n%08lx %08lx %08lx CIE\n",
		      (unsigned long)(saved_start - section_start), length, cie_id);
	      printf ("  Version:               %d\n", version);
	      printf ("  Augmentation:          \"%s\"\n", fc->augmentation);
	      if (version >= 4)
		{
		  printf ("  Pointer Size:          %u\n", fc->ptr_size);
		  printf ("  Segment Size:          %u\n", fc->segment_size);
		}
	      printf ("  Code alignment factor: %u\n", fc->code_factor);
	      printf ("  Data alignment factor: %d\n", fc->data_factor);
	      printf ("  Return address column: %d\n", fc->ra);

	      if (augmentation_data_len)
		{
		  unsigned long i;
		  printf ("  Augmentation data:    ");
		  for (i = 0; i < augmentation_data_len; ++i)
		    printf (" %02x", augmentation_data[i]);
		  putchar ('\n');
		}
	      putchar ('\n');
	    }

	  if (augmentation_data_len)
	    {
	      unsigned char *p, *q;
	      p = (unsigned char *) fc->augmentation + 1;
	      q = augmentation_data;

	      while (1)
		{
		  if (*p == 'L')
		    q++;
		  else if (*p == 'P')
		    q += 1 + size_of_encoded_value (*q);
		  else if (*p == 'R')
		    fc->fde_encoding = *q++;
		  else if (*p == 'S')
		    ;
		  else
		    break;
		  p++;
		}

	      if (fc->fde_encoding)
		encoded_ptr_size = size_of_encoded_value (fc->fde_encoding);
	    }

	  frame_need_space (fc, fc->ra);
	}
      else
	{
	  unsigned char *look_for;
	  static Frame_Chunk fde_fc;
	  unsigned long segment_selector;

	  fc = & fde_fc;
	  memset (fc, 0, sizeof (Frame_Chunk));

	  look_for = is_eh ? start - 4 - cie_id : section_start + cie_id;

	  for (cie = chunks; cie ; cie = cie->next)
	    if (cie->chunk_start == look_for)
	      break;

	  if (!cie)
	    {
	      warn ("Invalid CIE pointer %#08lx in FDE at %#08lx\n",
		    cie_id, (unsigned long)(saved_start - section_start));
	      fc->ncols = 0;
	      fc->col_type = (short int *) xmalloc (sizeof (short int));
	      fc->col_offset = (int *) xmalloc (sizeof (int));
	      frame_need_space (fc, max_regs - 1);
	      cie = fc;
	      fc->augmentation = "";
	      fc->fde_encoding = 0;
	      fc->ptr_size = eh_addr_size;
	      fc->segment_size = 0;
	    }
	  else
	    {
	      fc->ncols = cie->ncols;
	      fc->col_type = (short int *) xcmalloc (fc->ncols, sizeof (short int));
	      fc->col_offset =  (int *) xcmalloc (fc->ncols, sizeof (int));
	      memcpy (fc->col_type, cie->col_type, fc->ncols * sizeof (short int));
	      memcpy (fc->col_offset, cie->col_offset, fc->ncols * sizeof (int));
	      fc->augmentation = cie->augmentation;
	      fc->ptr_size = cie->ptr_size;
	      eh_addr_size = cie->ptr_size;
	      fc->segment_size = cie->segment_size;
	      fc->code_factor = cie->code_factor;
	      fc->data_factor = cie->data_factor;
	      fc->cfa_reg = cie->cfa_reg;
	      fc->cfa_offset = cie->cfa_offset;
	      fc->ra = cie->ra;
	      frame_need_space (fc, max_regs - 1);
	      fc->fde_encoding = cie->fde_encoding;
	    }

	  if (fc->fde_encoding)
	    encoded_ptr_size = size_of_encoded_value (fc->fde_encoding);

	  segment_selector = 0;
	  if (fc->segment_size)
	    {
	      segment_selector = byte_get (start, fc->segment_size);
	      start += fc->segment_size;
	    }
	  fc->pc_begin = get_encoded_value (start, fc->fde_encoding, section);
	  start += encoded_ptr_size;
	  fc->pc_range = byte_get (start, encoded_ptr_size);
	  start += encoded_ptr_size;

	  if (cie->augmentation[0] == 'z')
	    {
	      augmentation_data_len = LEB ();
	      augmentation_data = start;
	      start += augmentation_data_len;
	    }

	  printf ("\n%08lx %08lx %08lx FDE cie=%08lx pc=",
		  (unsigned long)(saved_start - section_start), length, cie_id,
		  (unsigned long)(cie->chunk_start - section_start));
	  if (fc->segment_size)
	    printf ("%04lx:", segment_selector);
	  printf ("%08lx..%08lx\n", fc->pc_begin, fc->pc_begin + fc->pc_range);
	  if (! do_debug_frames_interp && augmentation_data_len)
	    {
	      unsigned long i;

	      printf ("  Augmentation data:    ");
	      for (i = 0; i < augmentation_data_len; ++i)
		printf (" %02x", augmentation_data[i]);
	      putchar ('\n');
	      putchar ('\n');
	    }
	}

      /* At this point, fc is the current chunk, cie (if any) is set, and
	 we're about to interpret instructions for the chunk.  */
      /* ??? At present we need to do this always, since this sizes the
	 fc->col_type and fc->col_offset arrays, which we write into always.
	 We should probably split the interpreted and non-interpreted bits
	 into two different routines, since there's so much that doesn't
	 really overlap between them.  */
      if (1 || do_debug_frames_interp)
	{
	  /* Start by making a pass over the chunk, allocating storage
	     and taking note of what registers are used.  */
	  unsigned char *tmp = start;

	  while (start < block_end)
	    {
	      unsigned op, opa;
	      unsigned long reg, temp;

	      op = *start++;
	      opa = op & 0x3f;
	      if (op & 0xc0)
		op &= 0xc0;

	      /* Warning: if you add any more cases to this switch, be
		 sure to add them to the corresponding switch below.  */
	      switch (op)
		{
		case DW_CFA_advance_loc:
		  break;
		case DW_CFA_offset:
		  LEB ();
		  if (frame_need_space (fc, opa) >= 0)
		    fc->col_type[opa] = DW_CFA_undefined;
		  break;
		case DW_CFA_restore:
		  if (frame_need_space (fc, opa) >= 0)
		    fc->col_type[opa] = DW_CFA_undefined;
		  break;
		case DW_CFA_set_loc:
		  start += encoded_ptr_size;
		  break;
		case DW_CFA_advance_loc1:
		  start += 1;
		  break;
		case DW_CFA_advance_loc2:
		  start += 2;
		  break;
		case DW_CFA_advance_loc4:
		  start += 4;
		  break;
		case DW_CFA_offset_extended:
		case DW_CFA_val_offset:
		  reg = LEB (); LEB ();
		  if (frame_need_space (fc, reg) >= 0)
		    fc->col_type[reg] = DW_CFA_undefined;
		  break;
		case DW_CFA_restore_extended:
		  reg = LEB ();
		  frame_need_space (fc, reg);
		  if (frame_need_space (fc, reg) >= 0)
		    fc->col_type[reg] = DW_CFA_undefined;
		  break;
		case DW_CFA_undefined:
		  reg = LEB ();
		  if (frame_need_space (fc, reg) >= 0)
		    fc->col_type[reg] = DW_CFA_undefined;
		  break;
		case DW_CFA_same_value:
		  reg = LEB ();
		  if (frame_need_space (fc, reg) >= 0)
		    fc->col_type[reg] = DW_CFA_undefined;
		  break;
		case DW_CFA_register:
		  reg = LEB (); LEB ();
		  if (frame_need_space (fc, reg) >= 0)
		    fc->col_type[reg] = DW_CFA_undefined;
		  break;
		case DW_CFA_def_cfa:
		  LEB (); LEB ();
		  break;
		case DW_CFA_def_cfa_register:
		  LEB ();
		  break;
		case DW_CFA_def_cfa_offset:
		  LEB ();
		  break;
		case DW_CFA_def_cfa_expression:
		  temp = LEB ();
		  start += temp;
		  break;
		case DW_CFA_expression:
		case DW_CFA_val_expression:
		  reg = LEB ();
		  temp = LEB ();
		  start += temp;
		  if (frame_need_space (fc, reg) >= 0)
		    fc->col_type[reg] = DW_CFA_undefined;
		  break;
		case DW_CFA_offset_extended_sf:
		case DW_CFA_val_offset_sf:
		  reg = LEB (); SLEB ();
		  if (frame_need_space (fc, reg) >= 0)
		    fc->col_type[reg] = DW_CFA_undefined;
		  break;
		case DW_CFA_def_cfa_sf:
		  LEB (); SLEB ();
		  break;
		case DW_CFA_def_cfa_offset_sf:
		  SLEB ();
		  break;
		case DW_CFA_MIPS_advance_loc8:
		  start += 8;
		  break;
		case DW_CFA_GNU_args_size:
		  LEB ();
		  break;
		case DW_CFA_GNU_negative_offset_extended:
		  reg = LEB (); LEB ();
		  if (frame_need_space (fc, reg) >= 0)
		    fc->col_type[reg] = DW_CFA_undefined;
		  break;
		default:
		  break;
		}
	    }
	  start = tmp;
	}

      /* Now we know what registers are used, make a second pass over
	 the chunk, this time actually printing out the info.  */

      while (start < block_end)
	{
	  unsigned op, opa;
	  unsigned long ul, reg, roffs;
	  long l, ofs;
	  dwarf_vma vma;
	  const char *reg_prefix = "";

	  op = *start++;
	  opa = op & 0x3f;
	  if (op & 0xc0)
	    op &= 0xc0;

	  /* Warning: if you add any more cases to this switch, be
	     sure to add them to the corresponding switch above.  */
	  switch (op)
	    {
	    case DW_CFA_advance_loc:
	      if (do_debug_frames_interp)
		frame_display_row (fc, &need_col_headers, &max_regs);
	      else
		printf ("  DW_CFA_advance_loc: %d to %08lx\n",
			opa * fc->code_factor,
			fc->pc_begin + opa * fc->code_factor);
	      fc->pc_begin += opa * fc->code_factor;
	      break;

	    case DW_CFA_offset:
	      roffs = LEB ();
	      if (opa >= (unsigned int) fc->ncols)
		reg_prefix = bad_reg;
	      if (! do_debug_frames_interp || *reg_prefix != '\0')
		printf ("  DW_CFA_offset: %s%s at cfa%+ld\n",
			reg_prefix, regname (opa, 0),
			roffs * fc->data_factor);
	      if (*reg_prefix == '\0')
		{
		  fc->col_type[opa] = DW_CFA_offset;
		  fc->col_offset[opa] = roffs * fc->data_factor;
		}
	      break;

	    case DW_CFA_restore:
	      if (opa >= (unsigned int) cie->ncols
		  || opa >= (unsigned int) fc->ncols)
		reg_prefix = bad_reg;
	      if (! do_debug_frames_interp || *reg_prefix != '\0')
		printf ("  DW_CFA_restore: %s%s\n",
			reg_prefix, regname (opa, 0));
	      if (*reg_prefix == '\0')
		{
		  fc->col_type[opa] = cie->col_type[opa];
		  fc->col_offset[opa] = cie->col_offset[opa];
		  if (do_debug_frames_interp
		      && fc->col_type[opa] == DW_CFA_unreferenced)
		    fc->col_type[opa] = DW_CFA_undefined;
		}
	      break;

	    case DW_CFA_set_loc:
	      vma = get_encoded_value (start, fc->fde_encoding, section);
	      start += encoded_ptr_size;
	      if (do_debug_frames_interp)
		frame_display_row (fc, &need_col_headers, &max_regs);
	      else
		printf ("  DW_CFA_set_loc: %08lx\n", (unsigned long)vma);
	      fc->pc_begin = vma;
	      break;

	    case DW_CFA_advance_loc1:
	      ofs = byte_get (start, 1); start += 1;
	      if (do_debug_frames_interp)
		frame_display_row (fc, &need_col_headers, &max_regs);
	      else
		printf ("  DW_CFA_advance_loc1: %ld to %08lx\n",
			ofs * fc->code_factor,
			fc->pc_begin + ofs * fc->code_factor);
	      fc->pc_begin += ofs * fc->code_factor;
	      break;

	    case DW_CFA_advance_loc2:
	      ofs = byte_get (start, 2); start += 2;
	      if (do_debug_frames_interp)
		frame_display_row (fc, &need_col_headers, &max_regs);
	      else
		printf ("  DW_CFA_advance_loc2: %ld to %08lx\n",
			ofs * fc->code_factor,
			fc->pc_begin + ofs * fc->code_factor);
	      fc->pc_begin += ofs * fc->code_factor;
	      break;

	    case DW_CFA_advance_loc4:
	      ofs = byte_get (start, 4); start += 4;
	      if (do_debug_frames_interp)
		frame_display_row (fc, &need_col_headers, &max_regs);
	      else
		printf ("  DW_CFA_advance_loc4: %ld to %08lx\n",
			ofs * fc->code_factor,
			fc->pc_begin + ofs * fc->code_factor);
	      fc->pc_begin += ofs * fc->code_factor;
	      break;

	    case DW_CFA_offset_extended:
	      reg = LEB ();
	      roffs = LEB ();
	      if (reg >= (unsigned int) fc->ncols)
		reg_prefix = bad_reg;
	      if (! do_debug_frames_interp || *reg_prefix != '\0')
		printf ("  DW_CFA_offset_extended: %s%s at cfa%+ld\n",
			reg_prefix, regname (reg, 0),
			roffs * fc->data_factor);
	      if (*reg_prefix == '\0')
		{
		  fc->col_type[reg] = DW_CFA_offset;
		  fc->col_offset[reg] = roffs * fc->data_factor;
		}
	      break;

	    case DW_CFA_val_offset:
	      reg = LEB ();
	      roffs = LEB ();
	      if (reg >= (unsigned int) fc->ncols)
		reg_prefix = bad_reg;
	      if (! do_debug_frames_interp || *reg_prefix != '\0')
		printf ("  DW_CFA_val_offset: %s%s at cfa%+ld\n",
			reg_prefix, regname (reg, 0),
			roffs * fc->data_factor);
	      if (*reg_prefix == '\0')
		{
		  fc->col_type[reg] = DW_CFA_val_offset;
		  fc->col_offset[reg] = roffs * fc->data_factor;
		}
	      break;

	    case DW_CFA_restore_extended:
	      reg = LEB ();
	      if (reg >= (unsigned int) cie->ncols
		  || reg >= (unsigned int) fc->ncols)
		reg_prefix = bad_reg;
	      if (! do_debug_frames_interp || *reg_prefix != '\0')
		printf ("  DW_CFA_restore_extended: %s%s\n",
			reg_prefix, regname (reg, 0));
	      if (*reg_prefix == '\0')
		{
		  fc->col_type[reg] = cie->col_type[reg];
		  fc->col_offset[reg] = cie->col_offset[reg];
		}
	      break;

	    case DW_CFA_undefined:
	      reg = LEB ();
	      if (reg >= (unsigned int) fc->ncols)
		reg_prefix = bad_reg;
	      if (! do_debug_frames_interp || *reg_prefix != '\0')
		printf ("  DW_CFA_undefined: %s%s\n",
			reg_prefix, regname (reg, 0));
	      if (*reg_prefix == '\0')
		{
		  fc->col_type[reg] = DW_CFA_undefined;
		  fc->col_offset[reg] = 0;
		}
	      break;

	    case DW_CFA_same_value:
	      reg = LEB ();
	      if (reg >= (unsigned int) fc->ncols)
		reg_prefix = bad_reg;
	      if (! do_debug_frames_interp || *reg_prefix != '\0')
		printf ("  DW_CFA_same_value: %s%s\n",
			reg_prefix, regname (reg, 0));
	      if (*reg_prefix == '\0')
		{
		  fc->col_type[reg] = DW_CFA_same_value;
		  fc->col_offset[reg] = 0;
		}
	      break;

	    case DW_CFA_register:
	      reg = LEB ();
	      roffs = LEB ();
	      if (reg >= (unsigned int) fc->ncols)
		reg_prefix = bad_reg;
	      if (! do_debug_frames_interp || *reg_prefix != '\0')
		{
		  printf ("  DW_CFA_register: %s%s in ",
			  reg_prefix, regname (reg, 0));
		  puts (regname (roffs, 0));
		}
	      if (*reg_prefix == '\0')
		{
		  fc->col_type[reg] = DW_CFA_register;
		  fc->col_offset[reg] = roffs;
		}
	      break;

	    case DW_CFA_remember_state:
	      if (! do_debug_frames_interp)
		printf ("  DW_CFA_remember_state\n");
	      rs = (Frame_Chunk *) xmalloc (sizeof (Frame_Chunk));
	      rs->ncols = fc->ncols;
	      rs->col_type = (short int *) xcmalloc (rs->ncols,
                                                     sizeof (short int));
	      rs->col_offset = (int *) xcmalloc (rs->ncols, sizeof (int));
	      memcpy (rs->col_type, fc->col_type, rs->ncols);
	      memcpy (rs->col_offset, fc->col_offset, rs->ncols * sizeof (int));
	      rs->next = remembered_state;
	      remembered_state = rs;
	      break;

	    case DW_CFA_restore_state:
	      if (! do_debug_frames_interp)
		printf ("  DW_CFA_restore_state\n");
	      rs = remembered_state;
	      if (rs)
		{
		  remembered_state = rs->next;
		  frame_need_space (fc, rs->ncols - 1);
		  memcpy (fc->col_type, rs->col_type, rs->ncols);
		  memcpy (fc->col_offset, rs->col_offset,
			  rs->ncols * sizeof (int));
		  free (rs->col_type);
		  free (rs->col_offset);
		  free (rs);
		}
	      else if (do_debug_frames_interp)
		printf ("Mismatched DW_CFA_restore_state\n");
	      break;

	    case DW_CFA_def_cfa:
	      fc->cfa_reg = LEB ();
	      fc->cfa_offset = LEB ();
	      fc->cfa_exp = 0;
	      if (! do_debug_frames_interp)
		printf ("  DW_CFA_def_cfa: %s ofs %d\n",
			regname (fc->cfa_reg, 0), fc->cfa_offset);
	      break;

	    case DW_CFA_def_cfa_register:
	      fc->cfa_reg = LEB ();
	      fc->cfa_exp = 0;
	      if (! do_debug_frames_interp)
		printf ("  DW_CFA_def_cfa_register: %s\n",
			regname (fc->cfa_reg, 0));
	      break;

	    case DW_CFA_def_cfa_offset:
	      fc->cfa_offset = LEB ();
	      if (! do_debug_frames_interp)
		printf ("  DW_CFA_def_cfa_offset: %d\n", fc->cfa_offset);
	      break;

	    case DW_CFA_nop:
	      if (! do_debug_frames_interp)
		printf ("  DW_CFA_nop\n");
	      break;

	    case DW_CFA_def_cfa_expression:
	      ul = LEB ();
	      if (! do_debug_frames_interp)
		{
		  printf ("  DW_CFA_def_cfa_expression (");
		  decode_location_expression (start, eh_addr_size, 0, -1,
					      ul, 0, section);
		  printf (")\n");
		}
	      fc->cfa_exp = 1;
	      start += ul;
	      break;

	    case DW_CFA_expression:
	      reg = LEB ();
	      ul = LEB ();
	      if (reg >= (unsigned int) fc->ncols)
		reg_prefix = bad_reg;
	      if (! do_debug_frames_interp || *reg_prefix != '\0')
		{
		  printf ("  DW_CFA_expression: %s%s (",
			  reg_prefix, regname (reg, 0));
		  decode_location_expression (start, eh_addr_size, 0, -1,
					      ul, 0, section);
		  printf (")\n");
		}
	      if (*reg_prefix == '\0')
		fc->col_type[reg] = DW_CFA_expression;
	      start += ul;
	      break;

	    case DW_CFA_val_expression:
	      reg = LEB ();
	      ul = LEB ();
	      if (reg >= (unsigned int) fc->ncols)
		reg_prefix = bad_reg;
	      if (! do_debug_frames_interp || *reg_prefix != '\0')
		{
		  printf ("  DW_CFA_val_expression: %s%s (",
			  reg_prefix, regname (reg, 0));
		  decode_location_expression (start, eh_addr_size, 0, -1,
					      ul, 0, section);
		  printf (")\n");
		}
	      if (*reg_prefix == '\0')
		fc->col_type[reg] = DW_CFA_val_expression;
	      start += ul;
	      break;

	    case DW_CFA_offset_extended_sf:
	      reg = LEB ();
	      l = SLEB ();
	      if (frame_need_space (fc, reg) < 0)
		reg_prefix = bad_reg;
	      if (! do_debug_frames_interp || *reg_prefix != '\0')
		printf ("  DW_CFA_offset_extended_sf: %s%s at cfa%+ld\n",
			reg_prefix, regname (reg, 0),
			l * fc->data_factor);
	      if (*reg_prefix == '\0')
		{
		  fc->col_type[reg] = DW_CFA_offset;
		  fc->col_offset[reg] = l * fc->data_factor;
		}
	      break;

	    case DW_CFA_val_offset_sf:
	      reg = LEB ();
	      l = SLEB ();
	      if (frame_need_space (fc, reg) < 0)
		reg_prefix = bad_reg;
	      if (! do_debug_frames_interp || *reg_prefix != '\0')
		printf ("  DW_CFA_val_offset_sf: %s%s at cfa%+ld\n",
			reg_prefix, regname (reg, 0),
			l * fc->data_factor);
	      if (*reg_prefix == '\0')
		{
		  fc->col_type[reg] = DW_CFA_val_offset;
		  fc->col_offset[reg] = l * fc->data_factor;
		}
	      break;

	    case DW_CFA_def_cfa_sf:
	      fc->cfa_reg = LEB ();
	      fc->cfa_offset = SLEB ();
	      fc->cfa_offset = fc->cfa_offset * fc->data_factor;
	      fc->cfa_exp = 0;
	      if (! do_debug_frames_interp)
		printf ("  DW_CFA_def_cfa_sf: %s ofs %d\n",
			regname (fc->cfa_reg, 0), fc->cfa_offset);
	      break;

	    case DW_CFA_def_cfa_offset_sf:
	      fc->cfa_offset = SLEB ();
	      fc->cfa_offset = fc->cfa_offset * fc->data_factor;
	      if (! do_debug_frames_interp)
		printf ("  DW_CFA_def_cfa_offset_sf: %d\n", fc->cfa_offset);
	      break;

	    case DW_CFA_MIPS_advance_loc8:
	      ofs = byte_get (start, 8); start += 8;
	      if (do_debug_frames_interp)
		frame_display_row (fc, &need_col_headers, &max_regs);
	      else
		printf ("  DW_CFA_MIPS_advance_loc8: %ld to %08lx\n",
			ofs * fc->code_factor,
			fc->pc_begin + ofs * fc->code_factor);
	      fc->pc_begin += ofs * fc->code_factor;
	      break;

	    case DW_CFA_GNU_window_save:
	      if (! do_debug_frames_interp)
		printf ("  DW_CFA_GNU_window_save\n");
	      break;

	    case DW_CFA_GNU_args_size:
	      ul = LEB ();
	      if (! do_debug_frames_interp)
		printf ("  DW_CFA_GNU_args_size: %ld\n", ul);
	      break;

	    case DW_CFA_GNU_negative_offset_extended:
	      reg = LEB ();
	      l = - LEB ();
	      if (frame_need_space (fc, reg) < 0)
		reg_prefix = bad_reg;
	      if (! do_debug_frames_interp || *reg_prefix != '\0')
		printf ("  DW_CFA_GNU_negative_offset_extended: %s%s at cfa%+ld\n",
			reg_prefix, regname (reg, 0),
			l * fc->data_factor);
	      if (*reg_prefix == '\0')
		{
		  fc->col_type[reg] = DW_CFA_offset;
		  fc->col_offset[reg] = l * fc->data_factor;
		}
	      break;

	    default:
	      if (op >= DW_CFA_lo_user && op <= DW_CFA_hi_user)
		printf (_("  DW_CFA_??? (User defined call frame op: %#x)\n"), op);
	      else
		warn (_("unsupported or unknown Dwarf Call Frame Instruction number: %#x\n"), op);
	      start = block_end;
	    }
	}

      if (do_debug_frames_interp)
	frame_display_row (fc, &need_col_headers, &max_regs);

      start = block_end;
      eh_addr_size = saved_eh_addr_size;
    }

  printf ("\n");

  return 1;
}

#undef GET
#undef LEB
#undef SLEB

static int
display_gdb_index (struct dwarf_section *section,
		   void *file ATTRIBUTE_UNUSED)
{
  unsigned char *start = section->start;
  uint32_t version;
  uint32_t cu_list_offset, tu_list_offset;
  uint32_t address_table_offset, symbol_table_offset, constant_pool_offset;
  unsigned int cu_list_elements, tu_list_elements;
  unsigned int address_table_size, symbol_table_slots;
  unsigned char *cu_list, *tu_list;
  unsigned char *address_table, *symbol_table, *constant_pool;
  unsigned int i;

  /* The documentation for the format of this file is in gdb/dwarf2read.c.  */

  printf (_("Contents of the %s section:\n"), section->name);

  if (section->size < 6 * sizeof (uint32_t))
    {
      warn (_("Truncated header in the %s section.\n"), section->name);
      return 0;
    }

  version = byte_get_little_endian (start, 4);
  printf (_("Version %ld\n"), (long) version);

  /* Prior versions are obsolete, and future versions may not be
     backwards compatible.  */
  if (version < 3 || version > 7)
    {
      warn (_("Unsupported version %lu.\n"), (unsigned long) version);
      return 0;
    }
  if (version < 4)
    warn (_("The address table data in version 3 may be wrong.\n"));
  if (version < 5)
    warn (_("Version 4 does not support case insensitive lookups.\n"));
  if (version < 6)
    warn (_("Version 5 does not include inlined functions.\n"));
  if (version < 7)
      warn (_("Version 6 does not include symbol attributes.\n"));

  cu_list_offset = byte_get_little_endian (start + 4, 4);
  tu_list_offset = byte_get_little_endian (start + 8, 4);
  address_table_offset = byte_get_little_endian (start + 12, 4);
  symbol_table_offset = byte_get_little_endian (start + 16, 4);
  constant_pool_offset = byte_get_little_endian (start + 20, 4);

  if (cu_list_offset > section->size
      || tu_list_offset > section->size
      || address_table_offset > section->size
      || symbol_table_offset > section->size
      || constant_pool_offset > section->size)
    {
      warn (_("Corrupt header in the %s section.\n"), section->name);
      return 0;
    }

  cu_list_elements = (tu_list_offset - cu_list_offset) / 8;
  tu_list_elements = (address_table_offset - tu_list_offset) / 8;
  address_table_size = symbol_table_offset - address_table_offset;
  symbol_table_slots = (constant_pool_offset - symbol_table_offset) / 8;

  cu_list = start + cu_list_offset;
  tu_list = start + tu_list_offset;
  address_table = start + address_table_offset;
  symbol_table = start + symbol_table_offset;
  constant_pool = start + constant_pool_offset;

  printf (_("\nCU table:\n"));
  for (i = 0; i < cu_list_elements; i += 2)
    {
      uint64_t cu_offset = byte_get_little_endian (cu_list + i * 8, 8);
      uint64_t cu_length = byte_get_little_endian (cu_list + i * 8 + 8, 8);

      printf (_("[%3u] 0x%lx - 0x%lx\n"), i / 2,
	      (unsigned long) cu_offset,
	      (unsigned long) (cu_offset + cu_length - 1));
    }

  printf (_("\nTU table:\n"));
  for (i = 0; i < tu_list_elements; i += 3)
    {
      uint64_t tu_offset = byte_get_little_endian (tu_list + i * 8, 8);
      uint64_t type_offset = byte_get_little_endian (tu_list + i * 8 + 8, 8);
      uint64_t signature = byte_get_little_endian (tu_list + i * 8 + 16, 8);

      printf (_("[%3u] 0x%lx 0x%lx "), i / 3,
	      (unsigned long) tu_offset,
	      (unsigned long) type_offset);
      print_dwarf_vma (signature, 8);
      printf ("\n");
    }

  printf (_("\nAddress table:\n"));
  for (i = 0; i < address_table_size; i += 2 * 8 + 4)
    {
      uint64_t low = byte_get_little_endian (address_table + i, 8);
      uint64_t high = byte_get_little_endian (address_table + i + 8, 8);
      uint32_t cu_index = byte_get_little_endian (address_table + i + 16, 4);

      print_dwarf_vma (low, 8);
      print_dwarf_vma (high, 8);
      printf (_("%lu\n"), (unsigned long) cu_index);
    }

  printf (_("\nSymbol table:\n"));
  for (i = 0; i < symbol_table_slots; ++i)
    {
      uint32_t name_offset = byte_get_little_endian (symbol_table + i * 8, 4);
      uint32_t cu_vector_offset = byte_get_little_endian (symbol_table + i * 8 + 4, 4);
      uint32_t num_cus, cu;

      if (name_offset != 0
	  || cu_vector_offset != 0)
	{
	  unsigned int j;

	  printf ("[%3u] %s:", i, constant_pool + name_offset);
	  num_cus = byte_get_little_endian (constant_pool + cu_vector_offset, 4);
	  if (num_cus > 1)
	    printf ("\n");
	  for (j = 0; j < num_cus; ++j)
	    {
	      gdb_index_symbol_kind kind;

	      cu = byte_get_little_endian (constant_pool + cu_vector_offset + 4 + j * 4, 4);
	      kind = GDB_INDEX_SYMBOL_KIND_VALUE (cu);
	      cu = GDB_INDEX_CU_VALUE (cu);
	      /* Convert to TU number if it's for a type unit.  */
	      if (cu >= cu_list_elements / 2)
		printf ("%cT%lu", num_cus > 1 ? '\t' : ' ',
			(unsigned long) (cu - cu_list_elements / 2));
	      else
		printf ("%c%lu", num_cus > 1 ? '\t' : ' ', (unsigned long) cu);

	      switch (kind)
		{
		case GDB_INDEX_SYMBOL_KIND_NONE:
		  printf (_(" [no symbol information]"));
		  break;
		case GDB_INDEX_SYMBOL_KIND_TYPE:
		  printf (_(" [type]"));
		  break;
		case GDB_INDEX_SYMBOL_KIND_VARIABLE:
		  printf (_(" [variable]"));
		  break;
		case GDB_INDEX_SYMBOL_KIND_FUNCTION:
		  printf (_(" [function]"));
		  break;
		case GDB_INDEX_SYMBOL_KIND_OTHER:
		  printf (_(" [other]"));
		  break;
		default:
		  printf (_(" [unknown: %d]"), kind);
		  break;
		}
	      if (num_cus > 1)
		printf ("\n");
	    }
	  if (num_cus <= 1)
	    printf ("\n");
	}
    }

  return 1;
}

static int
display_debug_not_supported (struct dwarf_section *section,
			     void *file ATTRIBUTE_UNUSED)
{
  printf (_("Displaying the debug contents of section %s is not yet supported.\n"),
	    section->name);

  return 1;
}

void *
cmalloc (size_t nmemb, size_t size)
{
  /* Check for overflow.  */
  if (nmemb >= ~(size_t) 0 / size)
    return NULL;
  else
    return malloc (nmemb * size);
}

void *
xcmalloc (size_t nmemb, size_t size)
{
  /* Check for overflow.  */
  if (nmemb >= ~(size_t) 0 / size)
    return NULL;
  else
    return xmalloc (nmemb * size);
}

void *
xcrealloc (void *ptr, size_t nmemb, size_t size)
{
  /* Check for overflow.  */
  if (nmemb >= ~(size_t) 0 / size)
    return NULL;
  else
    return xrealloc (ptr, nmemb * size);
}

void
free_debug_memory (void)
{
  unsigned int i;

  free_abbrevs ();

  for (i = 0; i < max; i++)
    free_debug_section ((enum dwarf_section_display_enum) i);

  if (debug_information != NULL)
    {
      if (num_debug_info_entries != DEBUG_INFO_UNAVAILABLE)
	{
	  for (i = 0; i < num_debug_info_entries; i++)
	    {
	      if (!debug_information [i].max_loc_offsets)
		{
		  free (debug_information [i].loc_offsets);
		  free (debug_information [i].have_frame_base);
		}
	      if (!debug_information [i].max_range_lists)
		free (debug_information [i].range_lists);
	    }
	}

      free (debug_information);
      debug_information = NULL;
      num_debug_info_entries = 0;
    }
}

void
dwarf_select_sections_by_names (const char *names)
{
  typedef struct
  {
    const char * option;
    int *        variable;
    int          val;
  }
  debug_dump_long_opts;

  static const debug_dump_long_opts opts_table [] =
    {
      /* Please keep this table alpha- sorted.  */
      { "Ranges", & do_debug_ranges, 1 },
      { "abbrev", & do_debug_abbrevs, 1 },
      { "aranges", & do_debug_aranges, 1 },
      { "frames", & do_debug_frames, 1 },
      { "frames-interp", & do_debug_frames_interp, 1 },
      { "info", & do_debug_info, 1 },
      { "line", & do_debug_lines, FLAG_DEBUG_LINES_RAW }, /* For backwards compatibility.  */
      { "rawline", & do_debug_lines, FLAG_DEBUG_LINES_RAW },
      { "decodedline", & do_debug_lines, FLAG_DEBUG_LINES_DECODED },
      { "loc",  & do_debug_loc, 1 },
      { "macro", & do_debug_macinfo, 1 },
      { "pubnames", & do_debug_pubnames, 1 },
      { "pubtypes", & do_debug_pubtypes, 1 },
      /* This entry is for compatability
	 with earlier versions of readelf.  */
      { "ranges", & do_debug_aranges, 1 },
      { "str", & do_debug_str, 1 },
      /* The special .gdb_index section.  */
      { "gdb_index", & do_gdb_index, 1 },
      /* These trace_* sections are used by Itanium VMS.  */
      { "trace_abbrev", & do_trace_abbrevs, 1 },
      { "trace_aranges", & do_trace_aranges, 1 },
      { "trace_info", & do_trace_info, 1 },
      { NULL, NULL, 0 }
    };

  const char *p;

  p = names;
  while (*p)
    {
      const debug_dump_long_opts * entry;

      for (entry = opts_table; entry->option; entry++)
	{
	  size_t len = strlen (entry->option);

	  if (strncmp (p, entry->option, len) == 0
	      && (p[len] == ',' || p[len] == '\0'))
	    {
	      * entry->variable |= entry->val;

	      /* The --debug-dump=frames-interp option also
		 enables the --debug-dump=frames option.  */
	      if (do_debug_frames_interp)
		do_debug_frames = 1;

	      p += len;
	      break;
	    }
	}

      if (entry->option == NULL)
	{
	  warn (_("Unrecognized debug option '%s'\n"), p);
	  p = strchr (p, ',');
	  if (p == NULL)
	    break;
	}

      if (*p == ',')
	p++;
    }
}

void
dwarf_select_sections_by_letters (const char *letters)
{
  unsigned int lindex = 0;

  while (letters[lindex])
    switch (letters[lindex++])
      {
      case 'i':
	do_debug_info = 1;
	break;

      case 'a':
	do_debug_abbrevs = 1;
	break;

      case 'l':
	do_debug_lines |= FLAG_DEBUG_LINES_RAW;
	break;

      case 'L':
	do_debug_lines |= FLAG_DEBUG_LINES_DECODED;
	break;

      case 'p':
	do_debug_pubnames = 1;
	break;

      case 't':
	do_debug_pubtypes = 1;
	break;

      case 'r':
	do_debug_aranges = 1;
	break;

      case 'R':
	do_debug_ranges = 1;
	break;

      case 'F':
	do_debug_frames_interp = 1;
      case 'f':
	do_debug_frames = 1;
	break;

      case 'm':
	do_debug_macinfo = 1;
	break;

      case 's':
	do_debug_str = 1;
	break;

      case 'o':
	do_debug_loc = 1;
	break;

      default:
	warn (_("Unrecognized debug option '%s'\n"), optarg);
	break;
      }
}

void
dwarf_select_sections_all (void)
{
  do_debug_info = 1;
  do_debug_abbrevs = 1;
  do_debug_lines = FLAG_DEBUG_LINES_RAW;
  do_debug_pubnames = 1;
  do_debug_pubtypes = 1;
  do_debug_aranges = 1;
  do_debug_ranges = 1;
  do_debug_frames = 1;
  do_debug_macinfo = 1;
  do_debug_str = 1;
  do_debug_loc = 1;
  do_gdb_index = 1;
  do_trace_info = 1;
  do_trace_abbrevs = 1;
  do_trace_aranges = 1;
}

struct dwarf_section_display debug_displays[] =
{
  { { ".debug_abbrev",	    ".zdebug_abbrev",	NULL, NULL, 0, 0, abbrev },
    display_debug_abbrev,   &do_debug_abbrevs,	0 },
  { { ".debug_aranges",	    ".zdebug_aranges",	NULL, NULL, 0, 0, abbrev },
    display_debug_aranges,  &do_debug_aranges,	1 },
  { { ".debug_frame",       ".zdebug_frame",	NULL, NULL, 0, 0, abbrev },
    display_debug_frames,   &do_debug_frames,	1 },
  { { ".debug_info",	    ".zdebug_info",	NULL, NULL, 0, 0, abbrev },
    display_debug_info,	    &do_debug_info,	1 },
  { { ".debug_line",	    ".zdebug_line",	NULL, NULL, 0, 0, abbrev },
    display_debug_lines,    &do_debug_lines,	1 },
  { { ".debug_pubnames",    ".zdebug_pubnames",	NULL, NULL, 0, 0, abbrev },
    display_debug_pubnames, &do_debug_pubnames,	0 },
  { { ".eh_frame",	    "",			NULL, NULL, 0, 0, abbrev },
    display_debug_frames,   &do_debug_frames,	1 },
  { { ".debug_macinfo",	    ".zdebug_macinfo",	NULL, NULL, 0, 0, abbrev },
    display_debug_macinfo,  &do_debug_macinfo,	0 },
  { { ".debug_macro",	    ".zdebug_macro",	NULL, NULL, 0, 0, abbrev },
    display_debug_macro,    &do_debug_macinfo,	1 },
  { { ".debug_str",	    ".zdebug_str",	NULL, NULL, 0, 0, abbrev },
    display_debug_str,	    &do_debug_str,	0 },
  { { ".debug_loc",	    ".zdebug_loc",	NULL, NULL, 0, 0, abbrev },
    display_debug_loc,	    &do_debug_loc,	1 },
  { { ".debug_pubtypes",    ".zdebug_pubtypes",	NULL, NULL, 0, 0, abbrev },
    display_debug_pubnames, &do_debug_pubtypes,	0 },
  { { ".debug_ranges",	    ".zdebug_ranges",	NULL, NULL, 0, 0, abbrev },
    display_debug_ranges,   &do_debug_ranges,	1 },
  { { ".debug_static_func", ".zdebug_static_func", NULL, NULL, 0, 0, abbrev },
    display_debug_not_supported, NULL,		0 },
  { { ".debug_static_vars", ".zdebug_static_vars", NULL, NULL, 0, 0, abbrev },
    display_debug_not_supported, NULL,		0 },
  { { ".debug_types",	    ".zdebug_types",	NULL, NULL, 0, 0, abbrev },
    display_debug_types,    &do_debug_info,	1 },
  { { ".debug_weaknames",   ".zdebug_weaknames", NULL, NULL, 0, 0, abbrev },
    display_debug_not_supported, NULL,		0 },
  { { ".gdb_index",	    "",	                NULL, NULL, 0, 0, abbrev },
    display_gdb_index,			&do_gdb_index,		0 },
  { { ".trace_info",	    "",			NULL, NULL, 0, 0, trace_abbrev },
    display_trace_info,			&do_trace_info,		1 },
  { { ".trace_abbrev",	    "",			NULL, NULL, 0, 0, abbrev },
    display_debug_abbrev,		&do_trace_abbrevs,	0 },
  { { ".trace_aranges",	    "",			NULL, NULL, 0, 0, abbrev },
    display_debug_aranges,		&do_trace_aranges,	0 },
  { { ".debug_info.dwo",    ".zdebug_info.dwo",	NULL, NULL, 0, 0, abbrev_dwo },
    display_debug_info,			&do_debug_info,		1 },
  { { ".debug_abbrev.dwo",  ".zdebug_abbrev.dwo", NULL, NULL, 0, 0, abbrev_dwo },
    display_debug_abbrev,		&do_debug_abbrevs,	0 },
  { { ".debug_types.dwo",   ".zdebug_types.dwo", NULL, NULL, 0, 0, abbrev_dwo },
    display_debug_types,		&do_debug_info,		1 },
  { { ".debug_line.dwo",   ".zdebug_line.dwo", NULL, NULL, 0, 0, abbrev_dwo },
    display_debug_lines,   &do_debug_lines,	1 },
  { { ".debug_loc.dwo",	    ".zdebug_loc.dwo",	NULL, NULL, 0, 0, abbrev_dwo },
    display_debug_loc,	    &do_debug_loc,	1 },
  { { ".debug_macro.dwo",   ".zdebug_macro.dwo",NULL, NULL, 0, 0, abbrev },
    display_debug_macro,    &do_debug_macinfo,	1 },
  { { ".debug_macinfo.dwo", ".zdebug_macinfo.dwo",NULL, NULL, 0, 0, abbrev },
    display_debug_macinfo,  &do_debug_macinfo,	0 },
  { { ".debug_str.dwo",   ".zdebug_str.dwo", NULL, NULL, 0, 0, str_dwo },
    display_debug_str,     &do_debug_str,	        1 },
  { { ".debug_str_offsets",".zdebug_str_offsets", NULL, NULL, 0, 0, abbrev },
    display_debug_str_offsets, NULL,		0 },
  { { ".debug_str_offsets.dwo",".zdebug_str_offsets.dwo", NULL, NULL, 0, 0,
      abbrev },
    display_debug_str_offsets, NULL,		0 },
  { { ".debug_addr",".zdebug_addr",             NULL, NULL, 0, 0, debug_addr },
    display_debug_addr, NULL,		1 },
};
