/* DWARF 2 debugging format support for GDB.

   Copyright (C) 1994-2020 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "dwarf2/comp-unit.h"
#include "dwarf2/leb.h"
#include "dwarf2/line-header.h"
#include "dwarf2/read.h"
#include "complaints.h"
#include "filenames.h"

void
line_header::add_include_dir (const char *include_dir)
{
  if (dwarf_line_debug >= 2)
    {
      size_t new_size;
      if (version >= 5)
        new_size = m_include_dirs.size ();
      else
        new_size = m_include_dirs.size () + 1;
      fprintf_unfiltered (gdb_stdlog, "Adding dir %zu: %s\n",
			  new_size, include_dir);
    }
  m_include_dirs.push_back (include_dir);
}

void
line_header::add_file_name (const char *name,
			    dir_index d_index,
			    unsigned int mod_time,
			    unsigned int length)
{
  if (dwarf_line_debug >= 2)
    {
      size_t new_size;
      if (version >= 5)
        new_size = file_names_size ();
      else
        new_size = file_names_size () + 1;
      fprintf_unfiltered (gdb_stdlog, "Adding file %zu: %s\n",
			  new_size, name);
    }
  m_file_names.emplace_back (name, d_index, mod_time, length);
}

gdb::unique_xmalloc_ptr<char>
line_header::file_file_name (int file) const
{
  /* Is the file number a valid index into the line header's file name
     table?  Remember that file numbers start with one, not zero.  */
  if (is_valid_file_index (file))
    {
      const file_entry *fe = file_name_at (file);

      if (!IS_ABSOLUTE_PATH (fe->name))
	{
	  const char *dir = fe->include_dir (this);
	  if (dir != NULL)
	    return gdb::unique_xmalloc_ptr<char> (concat (dir, SLASH_STRING,
							  fe->name,
							  (char *) NULL));
	}
      return make_unique_xstrdup (fe->name);
    }
  else
    {
      /* The compiler produced a bogus file number.  We can at least
         record the macro definitions made in the file, even if we
         won't be able to find the file by name.  */
      char fake_name[80];

      xsnprintf (fake_name, sizeof (fake_name),
		 "<bad macro file number %d>", file);

      complaint (_("bad file number in macro information (%d)"),
                 file);

      return make_unique_xstrdup (fake_name);
    }
}

gdb::unique_xmalloc_ptr<char>
line_header::file_full_name (int file, const char *comp_dir) const
{
  /* Is the file number a valid index into the line header's file name
     table?  Remember that file numbers start with one, not zero.  */
  if (is_valid_file_index (file))
    {
      gdb::unique_xmalloc_ptr<char> relative = file_file_name (file);

      if (IS_ABSOLUTE_PATH (relative.get ()) || comp_dir == NULL)
	return relative;
      return gdb::unique_xmalloc_ptr<char> (concat (comp_dir, SLASH_STRING,
						    relative.get (),
						    (char *) NULL));
    }
  else
    return file_file_name (file);
}

static void
dwarf2_statement_list_fits_in_line_number_section_complaint (void)
{
  complaint (_("statement list doesn't fit in .debug_line section"));
}

/* Cover function for read_initial_length.
   Returns the length of the object at BUF, and stores the size of the
   initial length in *BYTES_READ and stores the size that offsets will be in
   *OFFSET_SIZE.
   If the initial length size is not equivalent to that specified in
   CU_HEADER then issue a complaint.
   This is useful when reading non-comp-unit headers.  */

static LONGEST
read_checked_initial_length_and_offset (bfd *abfd, const gdb_byte *buf,
					const struct comp_unit_head *cu_header,
					unsigned int *bytes_read,
					unsigned int *offset_size)
{
  LONGEST length = read_initial_length (abfd, buf, bytes_read);

  gdb_assert (cu_header->initial_length_size == 4
	      || cu_header->initial_length_size == 8
	      || cu_header->initial_length_size == 12);

  if (cu_header->initial_length_size != *bytes_read)
    complaint (_("intermixed 32-bit and 64-bit DWARF sections"));

  *offset_size = (*bytes_read == 4) ? 4 : 8;
  return length;
}

/* Read directory or file name entry format, starting with byte of
   format count entries, ULEB128 pairs of entry formats, ULEB128 of
   entries count and the entries themselves in the described entry
   format.  */

static void
read_formatted_entries (dwarf2_per_objfile *per_objfile, bfd *abfd,
			const gdb_byte **bufp, struct line_header *lh,
			const struct comp_unit_head *cu_header,
			void (*callback) (struct line_header *lh,
					  const char *name,
					  dir_index d_index,
					  unsigned int mod_time,
					  unsigned int length))
{
  gdb_byte format_count, formati;
  ULONGEST data_count, datai;
  const gdb_byte *buf = *bufp;
  const gdb_byte *format_header_data;
  unsigned int bytes_read;

  format_count = read_1_byte (abfd, buf);
  buf += 1;
  format_header_data = buf;
  for (formati = 0; formati < format_count; formati++)
    {
      read_unsigned_leb128 (abfd, buf, &bytes_read);
      buf += bytes_read;
      read_unsigned_leb128 (abfd, buf, &bytes_read);
      buf += bytes_read;
    }

  data_count = read_unsigned_leb128 (abfd, buf, &bytes_read);
  buf += bytes_read;
  for (datai = 0; datai < data_count; datai++)
    {
      const gdb_byte *format = format_header_data;
      struct file_entry fe;

      for (formati = 0; formati < format_count; formati++)
	{
	  ULONGEST content_type = read_unsigned_leb128 (abfd, format, &bytes_read);
	  format += bytes_read;

	  ULONGEST form  = read_unsigned_leb128 (abfd, format, &bytes_read);
	  format += bytes_read;

	  gdb::optional<const char *> string;
	  gdb::optional<unsigned int> uint;

	  switch (form)
	    {
	    case DW_FORM_string:
	      string.emplace (read_direct_string (abfd, buf, &bytes_read));
	      buf += bytes_read;
	      break;

	    case DW_FORM_line_strp:
	      string.emplace
		(per_objfile->read_line_string (buf, cu_header, &bytes_read));
	      buf += bytes_read;
	      break;

	    case DW_FORM_data1:
	      uint.emplace (read_1_byte (abfd, buf));
	      buf += 1;
	      break;

	    case DW_FORM_data2:
	      uint.emplace (read_2_bytes (abfd, buf));
	      buf += 2;
	      break;

	    case DW_FORM_data4:
	      uint.emplace (read_4_bytes (abfd, buf));
	      buf += 4;
	      break;

	    case DW_FORM_data8:
	      uint.emplace (read_8_bytes (abfd, buf));
	      buf += 8;
	      break;

	    case DW_FORM_data16:
	      /*  This is used for MD5, but file_entry does not record MD5s. */
	      buf += 16;
	      break;

	    case DW_FORM_udata:
	      uint.emplace (read_unsigned_leb128 (abfd, buf, &bytes_read));
	      buf += bytes_read;
	      break;

	    case DW_FORM_block:
	      /* It is valid only for DW_LNCT_timestamp which is ignored by
		 current GDB.  */
	      break;
	    }

	  switch (content_type)
	    {
	    case DW_LNCT_path:
	      if (string.has_value ())
		fe.name = *string;
	      break;
	    case DW_LNCT_directory_index:
	      if (uint.has_value ())
		fe.d_index = (dir_index) *uint;
	      break;
	    case DW_LNCT_timestamp:
	      if (uint.has_value ())
		fe.mod_time = *uint;
	      break;
	    case DW_LNCT_size:
	      if (uint.has_value ())
		fe.length = *uint;
	      break;
	    case DW_LNCT_MD5:
	      break;
	    default:
	      complaint (_("Unknown format content type %s"),
			 pulongest (content_type));
	    }
	}

      callback (lh, fe.name, fe.d_index, fe.mod_time, fe.length);
    }

  *bufp = buf;
}

/* See line-header.h.  */

line_header_up
dwarf_decode_line_header  (sect_offset sect_off, bool is_dwz,
			   dwarf2_per_objfile *per_objfile,
			   struct dwarf2_section_info *section,
			   const struct comp_unit_head *cu_header)
{
  const gdb_byte *line_ptr;
  unsigned int bytes_read, offset_size;
  int i;
  const char *cur_dir, *cur_file;

  bfd *abfd = section->get_bfd_owner ();

  /* Make sure that at least there's room for the total_length field.
     That could be 12 bytes long, but we're just going to fudge that.  */
  if (to_underlying (sect_off) + 4 >= section->size)
    {
      dwarf2_statement_list_fits_in_line_number_section_complaint ();
      return 0;
    }

  line_header_up lh (new line_header ());

  lh->sect_off = sect_off;
  lh->offset_in_dwz = is_dwz;

  line_ptr = section->buffer + to_underlying (sect_off);

  /* Read in the header.  */
  lh->total_length =
    read_checked_initial_length_and_offset (abfd, line_ptr, cu_header,
					    &bytes_read, &offset_size);
  line_ptr += bytes_read;

  const gdb_byte *start_here = line_ptr;

  if (line_ptr + lh->total_length > (section->buffer + section->size))
    {
      dwarf2_statement_list_fits_in_line_number_section_complaint ();
      return 0;
    }
  lh->statement_program_end = start_here + lh->total_length;
  lh->version = read_2_bytes (abfd, line_ptr);
  line_ptr += 2;
  if (lh->version > 5)
    {
      /* This is a version we don't understand.  The format could have
	 changed in ways we don't handle properly so just punt.  */
      complaint (_("unsupported version in .debug_line section"));
      return NULL;
    }
  if (lh->version >= 5)
    {
      gdb_byte segment_selector_size;

      /* Skip address size.  */
      read_1_byte (abfd, line_ptr);
      line_ptr += 1;

      segment_selector_size = read_1_byte (abfd, line_ptr);
      line_ptr += 1;
      if (segment_selector_size != 0)
	{
	  complaint (_("unsupported segment selector size %u "
		       "in .debug_line section"),
		     segment_selector_size);
	  return NULL;
	}
    }
  lh->header_length = read_offset (abfd, line_ptr, offset_size);
  line_ptr += offset_size;
  lh->statement_program_start = line_ptr + lh->header_length;
  lh->minimum_instruction_length = read_1_byte (abfd, line_ptr);
  line_ptr += 1;
  if (lh->version >= 4)
    {
      lh->maximum_ops_per_instruction = read_1_byte (abfd, line_ptr);
      line_ptr += 1;
    }
  else
    lh->maximum_ops_per_instruction = 1;

  if (lh->maximum_ops_per_instruction == 0)
    {
      lh->maximum_ops_per_instruction = 1;
      complaint (_("invalid maximum_ops_per_instruction "
		   "in `.debug_line' section"));
    }

  lh->default_is_stmt = read_1_byte (abfd, line_ptr);
  line_ptr += 1;
  lh->line_base = read_1_signed_byte (abfd, line_ptr);
  line_ptr += 1;
  lh->line_range = read_1_byte (abfd, line_ptr);
  line_ptr += 1;
  lh->opcode_base = read_1_byte (abfd, line_ptr);
  line_ptr += 1;
  lh->standard_opcode_lengths.reset (new unsigned char[lh->opcode_base]);

  lh->standard_opcode_lengths[0] = 1;  /* This should never be used anyway.  */
  for (i = 1; i < lh->opcode_base; ++i)
    {
      lh->standard_opcode_lengths[i] = read_1_byte (abfd, line_ptr);
      line_ptr += 1;
    }

  if (lh->version >= 5)
    {
      /* Read directory table.  */
      read_formatted_entries (per_objfile, abfd, &line_ptr, lh.get (),
			      cu_header,
			      [] (struct line_header *header, const char *name,
				  dir_index d_index, unsigned int mod_time,
				  unsigned int length)
	{
	  header->add_include_dir (name);
	});

      /* Read file name table.  */
      read_formatted_entries (per_objfile, abfd, &line_ptr, lh.get (),
			      cu_header,
			      [] (struct line_header *header, const char *name,
				  dir_index d_index, unsigned int mod_time,
				  unsigned int length)
	{
	  header->add_file_name (name, d_index, mod_time, length);
	});
    }
  else
    {
      /* Read directory table.  */
      while ((cur_dir = read_direct_string (abfd, line_ptr, &bytes_read)) != NULL)
	{
	  line_ptr += bytes_read;
	  lh->add_include_dir (cur_dir);
	}
      line_ptr += bytes_read;

      /* Read file name table.  */
      while ((cur_file = read_direct_string (abfd, line_ptr, &bytes_read)) != NULL)
	{
	  unsigned int mod_time, length;
	  dir_index d_index;

	  line_ptr += bytes_read;
	  d_index = (dir_index) read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
	  line_ptr += bytes_read;
	  mod_time = read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
	  line_ptr += bytes_read;
	  length = read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
	  line_ptr += bytes_read;

	  lh->add_file_name (cur_file, d_index, mod_time, length);
	}
      line_ptr += bytes_read;
    }

  if (line_ptr > (section->buffer + section->size))
    complaint (_("line number info header doesn't "
		 "fit in `.debug_line' section"));

  return lh;
}
