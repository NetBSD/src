/* basic_blocks.c  -  Basic-block level related code: reading/writing
   of basic-block info to/from gmon.out; computing and formatting of
   basic-block related statistics.

   Copyright (C) 1999-2026 Free Software Foundation, Inc.

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


#include "config.h"
#include "util.h"
#include "Elf.h" /* link to bfd.h fixes bfd* */

#include "gp-gmon.h"

#include "basic_blocks.h"
#include "gmon_io.h"
#include "gmon_out.h"
#include "symtab.h"

/* Skip over variable length string.  */

static void
fskip_string (FILE *fp)
{
  int ch;

  while ((ch = fgetc (fp)) != EOF)
    {
      if (ch == '\0')
	break;
    }
}

/* Read a basic-block record from file IFP.  FILENAME is the name
   of file IFP and is provided for formatting error-messages only.  */

void
bb_read_rec (FILE *ifp, const char *filename,
	     bool line_granularity, const char *whoami)
{
  unsigned int nblocks, b;
  bfd_vma addr, ncalls;
  Sym *sym;
  Sym_Table *symtab;

  if (gmon_io_read_32 (ifp, &nblocks))
    {
      fprintf (stderr, "%s: %s: unexpected end of file\n",
	       whoami, filename);
      done (1);
    }

  symtab = get_symtab (whoami);

  nblocks = bfd_get_32 (core_bfd, (bfd_byte *) & nblocks);
  if (gmon_file_version == 0)
    fskip_string (ifp);

  for (b = 0; b < nblocks; ++b)
    {
      if (gmon_file_version == 0)
	{
	  int line_num;

	  /* Version 0 had lots of extra stuff that we don't
	     care about anymore.  */
	  if ((fread (&ncalls, sizeof (ncalls), 1, ifp) != 1)
	      || (fread (&addr, sizeof (addr), 1, ifp) != 1)
	      || (fskip_string (ifp), false)
	      || (fskip_string (ifp), false)
	      || (fread (&line_num, sizeof (line_num), 1, ifp) != 1))
	    {
	      perror (filename);
	      done (1);
	    }
	}
      else if (gmon_io_read_vma (ifp, &addr, whoami)
	       || gmon_io_read_vma (ifp, &ncalls, whoami))
	{
	  perror (filename);
	  done (1);
	}

      /* Basic-block execution counts are meaningful only if we're
	 profiling at the line-by-line level:  */
      if (line_granularity)
	{
	  sym = sym_lookup (symtab, addr);

	  if (sym)
	    {
	      int i;

	      DBG (BBDEBUG,
		   printf ("[bb_read_rec] 0x%lx->0x%lx (%s:%d) cnt=%lu\n",
			   (unsigned long) addr, (unsigned long) sym->addr,
			   sym->name, sym->line_num, (unsigned long) ncalls));

	      for (i = 0; i < NBBS; i++)
		{
		  if (! sym->bb_addr[i] || sym->bb_addr[i] == addr)
		    {
		      sym->bb_addr[i] = addr;
		      sym->bb_calls[i] += ncalls;
		      break;
		    }
		}
	    }
	}
      else
	{
	  static bool user_warned = false;

	  if (!user_warned)
	    {
	      user_warned = true;
	      fprintf (stderr,
		       "%s: warning: ignoring basic-block exec counts (use -l or --line)\n",
		       whoami);
	    }
	}
    }
  return;
}
