/* Read MiniDebugInfo data from an objfile.

   Copyright (C) 2012-2017 Free Software Foundation, Inc.

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
#include "gdb_bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbcore.h"
#include <algorithm>

#ifdef HAVE_LIBLZMA

#include <lzma.h>

/* Allocator function for LZMA.  */

static void *
alloc_lzma (void *opaque, size_t nmemb, size_t size)
{
  return xmalloc (nmemb * size);
}

/* Free function for LZMA.  */

static void
free_lzma (void *opaque, void *ptr)
{
  xfree (ptr);
}

/* The allocator object for LZMA.  Note that 'gdb_lzma_allocator'
   cannot be const due to the lzma library function prototypes.  */

static lzma_allocator gdb_lzma_allocator = { alloc_lzma, free_lzma, NULL };

/* Custom bfd_openr_iovec implementation to read compressed data from
   a section.  This keeps only the last decompressed block in memory
   to allow larger data without using to much memory.  */

struct gdb_lzma_stream
{
  /* Section of input BFD from which we are decoding data.  */
  asection *section;

  /* lzma library decompression state.  */
  lzma_index *index;

  /* Currently decoded block.  */
  bfd_size_type data_start;
  bfd_size_type data_end;
  gdb_byte *data;
};

/* bfd_openr_iovec OPEN_P implementation for
   find_separate_debug_file_in_section.  OPEN_CLOSURE is 'asection *'
   of the section to decompress.

   Return 'struct gdb_lzma_stream *' must be freed by caller by xfree,
   together with its INDEX lzma data.  */

static void *
lzma_open (struct bfd *nbfd, void *open_closure)
{
  asection *section = (asection *) open_closure;
  bfd_size_type size, offset;
  lzma_stream_flags options;
  gdb_byte footer[LZMA_STREAM_HEADER_SIZE];
  gdb_byte *indexdata;
  lzma_index *index;
  int ret;
  uint64_t memlimit = UINT64_MAX;
  struct gdb_lzma_stream *lstream;
  size_t pos;

  size = bfd_get_section_size (section);
  offset = section->filepos + size - LZMA_STREAM_HEADER_SIZE;
  if (size < LZMA_STREAM_HEADER_SIZE
      || bfd_seek (section->owner, offset, SEEK_SET) != 0
      || bfd_bread (footer, LZMA_STREAM_HEADER_SIZE, section->owner)
         != LZMA_STREAM_HEADER_SIZE
      || lzma_stream_footer_decode (&options, footer) != LZMA_OK
      || offset < options.backward_size)
    {
      bfd_set_error (bfd_error_wrong_format);
      return NULL;
    }

  offset -= options.backward_size;
  indexdata = (gdb_byte *) xmalloc (options.backward_size);
  index = NULL;
  pos = 0;
  if (bfd_seek (section->owner, offset, SEEK_SET) != 0
      || bfd_bread (indexdata, options.backward_size, section->owner)
         != options.backward_size
      || lzma_index_buffer_decode (&index, &memlimit, &gdb_lzma_allocator,
				   indexdata, &pos, options.backward_size)
         != LZMA_OK
      || lzma_index_size (index) != options.backward_size)
    {
      xfree (indexdata);
      bfd_set_error (bfd_error_wrong_format);
      return NULL;
    }
  xfree (indexdata);

  lstream = XCNEW (struct gdb_lzma_stream);
  lstream->section = section;
  lstream->index = index;

  return lstream;
}

/* bfd_openr_iovec PREAD_P implementation for
   find_separate_debug_file_in_section.  Passed STREAM
   is 'struct gdb_lzma_stream *'.  */

static file_ptr
lzma_pread (struct bfd *nbfd, void *stream, void *buf, file_ptr nbytes,
	    file_ptr offset)
{
  struct gdb_lzma_stream *lstream = (struct gdb_lzma_stream *) stream;
  bfd_size_type chunk_size;
  lzma_index_iter iter;
  gdb_byte *compressed, *uncompressed;
  file_ptr block_offset;
  lzma_filter filters[LZMA_FILTERS_MAX + 1];
  lzma_block block;
  size_t compressed_pos, uncompressed_pos;
  file_ptr res;

  res = 0;
  while (nbytes > 0)
    {
      if (lstream->data == NULL
	  || lstream->data_start > offset || offset >= lstream->data_end)
	{
	  asection *section = lstream->section;

	  lzma_index_iter_init (&iter, lstream->index);
	  if (lzma_index_iter_locate (&iter, offset))
	    break;

	  compressed = (gdb_byte *) xmalloc (iter.block.total_size);
	  block_offset = section->filepos + iter.block.compressed_file_offset;
	  if (bfd_seek (section->owner, block_offset, SEEK_SET) != 0
	      || bfd_bread (compressed, iter.block.total_size, section->owner)
		 != iter.block.total_size)
	    {
	      xfree (compressed);
	      break;
	    }

	  uncompressed = (gdb_byte *) xmalloc (iter.block.uncompressed_size);

	  memset (&block, 0, sizeof (block));
	  block.filters = filters;
	  block.header_size = lzma_block_header_size_decode (compressed[0]);
	  if (lzma_block_header_decode (&block, &gdb_lzma_allocator, compressed)
	      != LZMA_OK)
	    {
	      xfree (compressed);
	      xfree (uncompressed);
	      break;
	    }

	  compressed_pos = block.header_size;
	  uncompressed_pos = 0;
	  if (lzma_block_buffer_decode (&block, &gdb_lzma_allocator,
					compressed, &compressed_pos,
					iter.block.total_size,
					uncompressed, &uncompressed_pos,
					iter.block.uncompressed_size)
	      != LZMA_OK)
	    {
	      xfree (compressed);
	      xfree (uncompressed);
	      break;
	    }

	  xfree (compressed);

	  xfree (lstream->data);
	  lstream->data = uncompressed;
	  lstream->data_start = iter.block.uncompressed_file_offset;
	  lstream->data_end = (iter.block.uncompressed_file_offset
			       + iter.block.uncompressed_size);
	}

      chunk_size = std::min (nbytes, (file_ptr) lstream->data_end - offset);
      memcpy (buf, lstream->data + offset - lstream->data_start, chunk_size);
      buf = (gdb_byte *) buf + chunk_size;
      offset += chunk_size;
      nbytes -= chunk_size;
      res += chunk_size;
    }

  return res;
}

/* bfd_openr_iovec CLOSE_P implementation for
   find_separate_debug_file_in_section.  Passed STREAM
   is 'struct gdb_lzma_stream *'.  */

static int
lzma_close (struct bfd *nbfd,
	    void *stream)
{
  struct gdb_lzma_stream *lstream = (struct gdb_lzma_stream *) stream;

  lzma_index_end (lstream->index, &gdb_lzma_allocator);
  xfree (lstream->data);
  xfree (lstream);

  /* Zero means success.  */
  return 0;
}

/* bfd_openr_iovec STAT_P implementation for
   find_separate_debug_file_in_section.  Passed STREAM
   is 'struct gdb_lzma_stream *'.  */

static int
lzma_stat (struct bfd *abfd,
	   void *stream,
	   struct stat *sb)
{
  struct gdb_lzma_stream *lstream = (struct gdb_lzma_stream *) stream;

  memset (sb, 0, sizeof (struct stat));
  sb->st_size = lzma_index_uncompressed_size (lstream->index);
  return 0;
}

#endif /* HAVE_LIBLZMA  */

/* This looks for a xz compressed separate debug info object file embedded
   in a section called .gnu_debugdata.  See
   http://fedoraproject.org/wiki/Features/MiniDebugInfo
   or the "Separate Debug Sections" of the manual for details.
   If we find one we create a iovec based bfd that decompresses the
   object data on demand.  If we don't find one, return NULL.  */

gdb_bfd_ref_ptr
find_separate_debug_file_in_section (struct objfile *objfile)
{
  asection *section;
  gdb_bfd_ref_ptr abfd;

  if (objfile->obfd == NULL)
    return NULL;

  section = bfd_get_section_by_name (objfile->obfd, ".gnu_debugdata");
  if (section == NULL)
    return NULL;

#ifdef HAVE_LIBLZMA
  abfd = gdb_bfd_openr_iovec (objfile_name (objfile), gnutarget, lzma_open,
			      section, lzma_pread, lzma_close, lzma_stat);
  if (abfd == NULL)
    return NULL;

  if (!bfd_check_format (abfd.get (), bfd_object))
    {
      warning (_("Cannot parse .gnu_debugdata section; not a BFD object"));
      return NULL;
    }
#else
  warning (_("Cannot parse .gnu_debugdata section; LZMA support was "
	     "disabled at compile time"));
#endif /* !HAVE_LIBLZMA */

  return abfd;
}
