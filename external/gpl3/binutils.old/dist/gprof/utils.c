/*
 * Copyright (c) 1983, 1993, 2001
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "gprof.h"
#include "demangle.h"
#include "search_list.h"
#include "source.h"
#include "symtab.h"
#include "cg_arcs.h"
#include "utils.h"
#include "corefile.h"


/*
 * Print name of symbol.  Return number of characters printed.
 */
int
print_name_only (Sym *self)
{
  const char *name = self->name;
  char *demangled = 0;
  int size = 0;

  if (name)
    {
      if (!bsd_style_output && demangle)
	{
	  demangled = bfd_demangle (core_bfd, name, DMGL_ANSI | DMGL_PARAMS);
	  if (demangled)
	    name = demangled;
	}
      printf ("%s", name);
      size = strlen (name);
      if ((line_granularity || inline_file_names) && self->file)
	{
	  const char *filename = self->file->name;
	  char *buf;

	  if (!print_path)
	    {
	      filename = strrchr (filename, '/');
	      if (filename)
		{
		  ++filename;
		}
	      else
		{
		  filename = self->file->name;
		}
	    }
	  buf = xmalloc (strlen (filename) + 8 + 20 + 16);
	  if (line_granularity)
	    {
	      sprintf (buf, " (%s:%d @ %lx)", filename, self->line_num,
		       (unsigned long) self->addr);
	    }
	  else
	    {
	      sprintf (buf, " (%s:%d)", filename, self->line_num);
	    }
	  printf ("%s", buf);
	  size += strlen (buf);
	  free (buf);
	}
      free (demangled);
      DBG (DFNDEBUG, printf ("{%d} ", self->cg.top_order));
      DBG (PROPDEBUG, printf ("%4.0f%% ", 100.0 * self->cg.prop.fract));
    }
  return size;
}


void
print_name (Sym *self)
{
  print_name_only (self);

  if (self->cg.cyc.num != 0)
    {
      printf (_(" <cycle %d>"), self->cg.cyc.num);
    }
  if (self->cg.index != 0)
    {
      if (self->cg.print_flag)
	{
	  printf (" [%d]", self->cg.index);
	}
      else
	{
	  printf (" (%d)", self->cg.index);
	}
    }
}
