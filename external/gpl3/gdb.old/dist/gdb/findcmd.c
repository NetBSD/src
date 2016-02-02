/* The find command.

   Copyright (C) 2008-2015 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include <ctype.h>
#include "gdbcmd.h"
#include "value.h"
#include "target.h"
#include "cli/cli-utils.h"

/* Copied from bfd_put_bits.  */

static void
put_bits (bfd_uint64_t data, gdb_byte *buf, int bits, bfd_boolean big_p)
{
  int i;
  int bytes;

  gdb_assert (bits % 8 == 0);

  bytes = bits / 8;
  for (i = 0; i < bytes; i++)
    {
      int index = big_p ? bytes - i - 1 : i;

      buf[index] = data & 0xff;
      data >>= 8;
    }
}

/* Subroutine of find_command to simplify it.
   Parse the arguments of the "find" command.  */

static void
parse_find_args (char *args, ULONGEST *max_countp,
		 gdb_byte **pattern_bufp, ULONGEST *pattern_lenp,
		 CORE_ADDR *start_addrp, ULONGEST *search_space_lenp,
		 bfd_boolean big_p)
{
  /* Default to using the specified type.  */
  char size = '\0';
  ULONGEST max_count = ~(ULONGEST) 0;
  /* Buffer to hold the search pattern.  */
  gdb_byte *pattern_buf;
  /* Current size of search pattern buffer.
     We realloc space as needed.  */
#define INITIAL_PATTERN_BUF_SIZE 100
  ULONGEST pattern_buf_size = INITIAL_PATTERN_BUF_SIZE;
  /* Pointer to one past the last in-use part of pattern_buf.  */
  gdb_byte *pattern_buf_end;
  ULONGEST pattern_len;
  CORE_ADDR start_addr;
  ULONGEST search_space_len;
  const char *s = args;
  struct cleanup *old_cleanups;
  struct value *v;

  if (args == NULL)
    error (_("Missing search parameters."));

  pattern_buf = xmalloc (pattern_buf_size);
  pattern_buf_end = pattern_buf;
  old_cleanups = make_cleanup (free_current_contents, &pattern_buf);

  /* Get search granularity and/or max count if specified.
     They may be specified in either order, together or separately.  */

  while (*s == '/')
    {
      ++s;

      while (*s != '\0' && *s != '/' && !isspace (*s))
	{
	  if (isdigit (*s))
	    {
	      max_count = atoi (s);
	      while (isdigit (*s))
		++s;
	      continue;
	    }

	  switch (*s)
	    {
	    case 'b':
	    case 'h':
	    case 'w':
	    case 'g':
	      size = *s++;
	      break;
	    default:
	      error (_("Invalid size granularity."));
	    }
	}

      s = skip_spaces_const (s);
    }

  /* Get the search range.  */

  v = parse_to_comma_and_eval (&s);
  start_addr = value_as_address (v);

  if (*s == ',')
    ++s;
  s = skip_spaces_const (s);

  if (*s == '+')
    {
      LONGEST len;

      ++s;
      v = parse_to_comma_and_eval (&s);
      len = value_as_long (v);
      if (len == 0)
	{
	  do_cleanups (old_cleanups);
	  printf_filtered (_("Empty search range.\n"));
	  return;
	}
      if (len < 0)
	error (_("Invalid length."));
      /* Watch for overflows.  */
      if (len > CORE_ADDR_MAX
	  || (start_addr + len - 1) < start_addr)
	error (_("Search space too large."));
      search_space_len = len;
    }
  else
    {
      CORE_ADDR end_addr;

      v = parse_to_comma_and_eval (&s);
      end_addr = value_as_address (v);
      if (start_addr > end_addr)
	error (_("Invalid search space, end precedes start."));
      search_space_len = end_addr - start_addr + 1;
      /* We don't support searching all of memory
	 (i.e. start=0, end = 0xff..ff).
	 Bail to avoid overflows later on.  */
      if (search_space_len == 0)
	error (_("Overflow in address range "
		 "computation, choose smaller range."));
    }

  if (*s == ',')
    ++s;

  /* Fetch the search string.  */

  while (*s != '\0')
    {
      LONGEST x;
      struct type *t;
      ULONGEST pattern_buf_size_need;

      s = skip_spaces_const (s);

      v = parse_to_comma_and_eval (&s);
      t = value_type (v);

      /* Keep it simple and assume size == 'g' when watching for when we
	 need to grow the pattern buf.  */
      pattern_buf_size_need = (pattern_buf_end - pattern_buf
			       + max (TYPE_LENGTH (t), sizeof (int64_t)));
      if (pattern_buf_size_need > pattern_buf_size)
	{
	  size_t current_offset = pattern_buf_end - pattern_buf;

	  pattern_buf_size = pattern_buf_size_need * 2;
	  pattern_buf = xrealloc (pattern_buf, pattern_buf_size);
	  pattern_buf_end = pattern_buf + current_offset;
	}

      if (size != '\0')
	{
	  x = value_as_long (v);
	  switch (size)
	    {
	    case 'b':
	      *pattern_buf_end++ = x;
	      break;
	    case 'h':
	      put_bits (x, pattern_buf_end, 16, big_p);
	      pattern_buf_end += sizeof (int16_t);
	      break;
	    case 'w':
	      put_bits (x, pattern_buf_end, 32, big_p);
	      pattern_buf_end += sizeof (int32_t);
	      break;
	    case 'g':
	      put_bits (x, pattern_buf_end, 64, big_p);
	      pattern_buf_end += sizeof (int64_t);
	      break;
	    }
	}
      else
	{
	  memcpy (pattern_buf_end, value_contents (v), TYPE_LENGTH (t));
	  pattern_buf_end += TYPE_LENGTH (t);
	}

      if (*s == ',')
	++s;
      s = skip_spaces_const (s);
    }

  if (pattern_buf_end == pattern_buf)
    error (_("Missing search pattern."));

  pattern_len = pattern_buf_end - pattern_buf;

  if (search_space_len < pattern_len)
    error (_("Search space too small to contain pattern."));

  *max_countp = max_count;
  *pattern_bufp = pattern_buf;
  *pattern_lenp = pattern_len;
  *start_addrp = start_addr;
  *search_space_lenp = search_space_len;

  /* We successfully parsed the arguments, leave the freeing of PATTERN_BUF
     to the caller now.  */
  discard_cleanups (old_cleanups);
}

static void
find_command (char *args, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();
  bfd_boolean big_p = gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG;
  /* Command line parameters.
     These are initialized to avoid uninitialized warnings from -Wall.  */
  ULONGEST max_count = 0;
  gdb_byte *pattern_buf = 0;
  ULONGEST pattern_len = 0;
  CORE_ADDR start_addr = 0;
  ULONGEST search_space_len = 0;
  /* End of command line parameters.  */
  unsigned int found_count;
  CORE_ADDR last_found_addr;
  struct cleanup *old_cleanups;

  parse_find_args (args, &max_count, &pattern_buf, &pattern_len, 
		   &start_addr, &search_space_len, big_p);

  old_cleanups = make_cleanup (free_current_contents, &pattern_buf);

  /* Perform the search.  */

  found_count = 0;
  last_found_addr = 0;

  while (search_space_len >= pattern_len
	 && found_count < max_count)
    {
      /* Offset from start of this iteration to the next iteration.  */
      ULONGEST next_iter_incr;
      CORE_ADDR found_addr;
      int found = target_search_memory (start_addr, search_space_len,
					pattern_buf, pattern_len, &found_addr);

      if (found <= 0)
	break;

      print_address (gdbarch, found_addr, gdb_stdout);
      printf_filtered ("\n");
      ++found_count;
      last_found_addr = found_addr;

      /* Begin next iteration at one byte past this match.  */
      next_iter_incr = (found_addr - start_addr) + 1;

      /* For robustness, we don't let search_space_len go -ve here.  */
      if (search_space_len >= next_iter_incr)
	search_space_len -= next_iter_incr;
      else
	search_space_len = 0;
      start_addr += next_iter_incr;
    }

  /* Record and print the results.  */

  set_internalvar_integer (lookup_internalvar ("numfound"), found_count);
  if (found_count > 0)
    {
      struct type *ptr_type = builtin_type (gdbarch)->builtin_data_ptr;

      set_internalvar (lookup_internalvar ("_"),
		       value_from_pointer (ptr_type, last_found_addr));
    }

  if (found_count == 0)
    printf_filtered ("Pattern not found.\n");
  else
    printf_filtered ("%d pattern%s found.\n", found_count,
		     found_count > 1 ? "s" : "");

  do_cleanups (old_cleanups);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_mem_search;

void
_initialize_mem_search (void)
{
  add_cmd ("find", class_vars, find_command, _("\
Search memory for a sequence of bytes.\n\
Usage:\nfind \
[/size-char] [/max-count] start-address, end-address, expr1 [, expr2 ...]\n\
find [/size-char] [/max-count] start-address, +length, expr1 [, expr2 ...]\n\
size-char is one of b,h,w,g for 8,16,32,64 bit values respectively,\n\
and if not specified the size is taken from the type of the expression\n\
in the current language.\n\
Note that this means for example that in the case of C-like languages\n\
a search for an untyped 0x42 will search for \"(int) 0x42\"\n\
which is typically four bytes.\n\
\n\
The address of the last match is stored as the value of \"$_\".\n\
Convenience variable \"$numfound\" is set to the number of matches."),
	   &cmdlist);
}
