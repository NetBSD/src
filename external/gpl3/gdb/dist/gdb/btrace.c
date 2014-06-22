/* Branch trace support for GDB, the GNU debugger.

   Copyright (C) 2013-2014 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>

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

#include "btrace.h"
#include "gdbthread.h"
#include "exceptions.h"
#include "inferior.h"
#include "target.h"
#include "record.h"
#include "symtab.h"
#include "disasm.h"
#include "source.h"
#include "filenames.h"
#include "xml-support.h"

/* Print a record debug message.  Use do ... while (0) to avoid ambiguities
   when used in if statements.  */

#define DEBUG(msg, args...)						\
  do									\
    {									\
      if (record_debug != 0)						\
        fprintf_unfiltered (gdb_stdlog,					\
			    "[btrace] " msg "\n", ##args);		\
    }									\
  while (0)

#define DEBUG_FTRACE(msg, args...) DEBUG ("[ftrace] " msg, ##args)

/* Initialize the instruction iterator.  */

static void
btrace_init_insn_iterator (struct btrace_thread_info *btinfo)
{
  DEBUG ("init insn iterator");

  btinfo->insn_iterator.begin = 1;
  btinfo->insn_iterator.end = 0;
}

/* Initialize the function iterator.  */

static void
btrace_init_func_iterator (struct btrace_thread_info *btinfo)
{
  DEBUG ("init func iterator");

  btinfo->func_iterator.begin = 1;
  btinfo->func_iterator.end = 0;
}

/* Compute the instruction trace from the block trace.  */

static VEC (btrace_inst_s) *
compute_itrace (VEC (btrace_block_s) *btrace)
{
  VEC (btrace_inst_s) *itrace;
  struct gdbarch *gdbarch;
  unsigned int b;

  DEBUG ("compute itrace");

  itrace = NULL;
  gdbarch = target_gdbarch ();
  b = VEC_length (btrace_block_s, btrace);

  while (b-- != 0)
    {
      btrace_block_s *block;
      CORE_ADDR pc;

      block = VEC_index (btrace_block_s, btrace, b);
      pc = block->begin;

      /* Add instructions for this block.  */
      for (;;)
	{
	  btrace_inst_s *inst;
	  int size;

	  /* We should hit the end of the block.  Warn if we went too far.  */
	  if (block->end < pc)
	    {
	      warning (_("Recorded trace may be corrupted."));
	      break;
	    }

	  inst = VEC_safe_push (btrace_inst_s, itrace, NULL);
	  inst->pc = pc;

	  /* We're done once we pushed the instruction at the end.  */
	  if (block->end == pc)
	    break;

	  size = gdb_insn_length (gdbarch, pc);

	  /* Make sure we terminate if we fail to compute the size.  */
	  if (size <= 0)
	    {
	      warning (_("Recorded trace may be incomplete."));
	      break;
	    }

	  pc += size;
	}
    }

  return itrace;
}

/* Return the function name of a recorded function segment for printing.
   This function never returns NULL.  */

static const char *
ftrace_print_function_name (struct btrace_func *bfun)
{
  struct minimal_symbol *msym;
  struct symbol *sym;

  msym = bfun->msym;
  sym = bfun->sym;

  if (sym != NULL)
    return SYMBOL_PRINT_NAME (sym);

  if (msym != NULL)
    return SYMBOL_PRINT_NAME (msym);

  return "<unknown>";
}

/* Return the file name of a recorded function segment for printing.
   This function never returns NULL.  */

static const char *
ftrace_print_filename (struct btrace_func *bfun)
{
  struct symbol *sym;
  const char *filename;

  sym = bfun->sym;

  if (sym != NULL)
    filename = symtab_to_filename_for_display (sym->symtab);
  else
    filename = "<unknown>";

  return filename;
}

/* Print an ftrace debug status message.  */

static void
ftrace_debug (struct btrace_func *bfun, const char *prefix)
{
  DEBUG_FTRACE ("%s: fun = %s, file = %s, lines = [%d; %d], insn = [%u; %u]",
		prefix, ftrace_print_function_name (bfun),
		ftrace_print_filename (bfun), bfun->lbegin, bfun->lend,
		bfun->ibegin, bfun->iend);
}

/* Initialize a recorded function segment.  */

static void
ftrace_init_func (struct btrace_func *bfun, struct minimal_symbol *mfun,
		  struct symbol *fun, unsigned int idx)
{
  bfun->msym = mfun;
  bfun->sym = fun;
  bfun->lbegin = INT_MAX;
  bfun->lend = 0;
  bfun->ibegin = idx;
  bfun->iend = idx;
}

/* Check whether the function has changed.  */

static int
ftrace_function_switched (struct btrace_func *bfun,
			  struct minimal_symbol *mfun, struct symbol *fun)
{
  struct minimal_symbol *msym;
  struct symbol *sym;

  /* The function changed if we did not have one before.  */
  if (bfun == NULL)
    return 1;

  msym = bfun->msym;
  sym = bfun->sym;

  /* If the minimal symbol changed, we certainly switched functions.  */
  if (mfun != NULL && msym != NULL
      && strcmp (SYMBOL_LINKAGE_NAME (mfun), SYMBOL_LINKAGE_NAME (msym)) != 0)
    return 1;

  /* If the symbol changed, we certainly switched functions.  */
  if (fun != NULL && sym != NULL)
    {
      const char *bfname, *fname;

      /* Check the function name.  */
      if (strcmp (SYMBOL_LINKAGE_NAME (fun), SYMBOL_LINKAGE_NAME (sym)) != 0)
	return 1;

      /* Check the location of those functions, as well.  */
      bfname = symtab_to_fullname (sym->symtab);
      fname = symtab_to_fullname (fun->symtab);
      if (filename_cmp (fname, bfname) != 0)
	return 1;
    }

  return 0;
}

/* Check if we should skip this file when generating the function call
   history.  We would want to do that if, say, a macro that is defined
   in another file is expanded in this function.  */

static int
ftrace_skip_file (struct btrace_func *bfun, const char *filename)
{
  struct symbol *sym;
  const char *bfile;

  sym = bfun->sym;

  if (sym != NULL)
    bfile = symtab_to_fullname (sym->symtab);
  else
    bfile = "";

  if (filename == NULL)
    filename = "";

  return (filename_cmp (bfile, filename) != 0);
}

/* Compute the function trace from the instruction trace.  */

static VEC (btrace_func_s) *
compute_ftrace (VEC (btrace_inst_s) *itrace)
{
  VEC (btrace_func_s) *ftrace;
  struct btrace_inst *binst;
  struct btrace_func *bfun;
  unsigned int idx;

  DEBUG ("compute ftrace");

  ftrace = NULL;
  bfun = NULL;

  for (idx = 0; VEC_iterate (btrace_inst_s, itrace, idx, binst); ++idx)
    {
      struct symtab_and_line sal;
      struct bound_minimal_symbol mfun;
      struct symbol *fun;
      const char *filename;
      CORE_ADDR pc;

      pc = binst->pc;

      /* Try to determine the function we're in.  We use both types of symbols
	 to avoid surprises when we sometimes get a full symbol and sometimes
	 only a minimal symbol.  */
      fun = find_pc_function (pc);
      mfun = lookup_minimal_symbol_by_pc (pc);

      if (fun == NULL && mfun.minsym == NULL)
	{
	  DEBUG_FTRACE ("no symbol at %u, pc=%s", idx,
			core_addr_to_string_nz (pc));
	  continue;
	}

      /* If we're switching functions, we start over.  */
      if (ftrace_function_switched (bfun, mfun.minsym, fun))
	{
	  bfun = VEC_safe_push (btrace_func_s, ftrace, NULL);

	  ftrace_init_func (bfun, mfun.minsym, fun, idx);
	  ftrace_debug (bfun, "init");
	}

      /* Update the instruction range.  */
      bfun->iend = idx;
      ftrace_debug (bfun, "update insns");

      /* Let's see if we have source correlation, as well.  */
      sal = find_pc_line (pc, 0);
      if (sal.symtab == NULL || sal.line == 0)
	{
	  DEBUG_FTRACE ("no lines at %u, pc=%s", idx,
			core_addr_to_string_nz (pc));
	  continue;
	}

      /* Check if we switched files.  This could happen if, say, a macro that
	 is defined in another file is expanded here.  */
      filename = symtab_to_fullname (sal.symtab);
      if (ftrace_skip_file (bfun, filename))
	{
	  DEBUG_FTRACE ("ignoring file at %u, pc=%s, file=%s", idx,
			core_addr_to_string_nz (pc), filename);
	  continue;
	}

      /* Update the line range.  */
      bfun->lbegin = min (bfun->lbegin, sal.line);
      bfun->lend = max (bfun->lend, sal.line);
      ftrace_debug (bfun, "update lines");
    }

  return ftrace;
}

/* See btrace.h.  */

void
btrace_enable (struct thread_info *tp)
{
  if (tp->btrace.target != NULL)
    return;

  if (!target_supports_btrace ())
    error (_("Target does not support branch tracing."));

  DEBUG ("enable thread %d (%s)", tp->num, target_pid_to_str (tp->ptid));

  tp->btrace.target = target_enable_btrace (tp->ptid);
}

/* See btrace.h.  */

void
btrace_disable (struct thread_info *tp)
{
  struct btrace_thread_info *btp = &tp->btrace;
  int errcode = 0;

  if (btp->target == NULL)
    return;

  DEBUG ("disable thread %d (%s)", tp->num, target_pid_to_str (tp->ptid));

  target_disable_btrace (btp->target);
  btp->target = NULL;

  btrace_clear (tp);
}

/* See btrace.h.  */

void
btrace_teardown (struct thread_info *tp)
{
  struct btrace_thread_info *btp = &tp->btrace;
  int errcode = 0;

  if (btp->target == NULL)
    return;

  DEBUG ("teardown thread %d (%s)", tp->num, target_pid_to_str (tp->ptid));

  target_teardown_btrace (btp->target);
  btp->target = NULL;

  btrace_clear (tp);
}

/* See btrace.h.  */

void
btrace_fetch (struct thread_info *tp)
{
  struct btrace_thread_info *btinfo;
  VEC (btrace_block_s) *btrace;

  DEBUG ("fetch thread %d (%s)", tp->num, target_pid_to_str (tp->ptid));

  btinfo = &tp->btrace;
  if (btinfo->target == NULL)
    return;

  btrace = target_read_btrace (btinfo->target, btrace_read_new);
  if (VEC_empty (btrace_block_s, btrace))
    return;

  btrace_clear (tp);

  btinfo->btrace = btrace;
  btinfo->itrace = compute_itrace (btinfo->btrace);
  btinfo->ftrace = compute_ftrace (btinfo->itrace);

  /* Initialize branch trace iterators.  */
  btrace_init_insn_iterator (btinfo);
  btrace_init_func_iterator (btinfo);
}

/* See btrace.h.  */

void
btrace_clear (struct thread_info *tp)
{
  struct btrace_thread_info *btinfo;

  DEBUG ("clear thread %d (%s)", tp->num, target_pid_to_str (tp->ptid));

  btinfo = &tp->btrace;

  VEC_free (btrace_block_s, btinfo->btrace);
  VEC_free (btrace_inst_s, btinfo->itrace);
  VEC_free (btrace_func_s, btinfo->ftrace);

  btinfo->btrace = NULL;
  btinfo->itrace = NULL;
  btinfo->ftrace = NULL;
}

/* See btrace.h.  */

void
btrace_free_objfile (struct objfile *objfile)
{
  struct thread_info *tp;

  DEBUG ("free objfile");

  ALL_THREADS (tp)
    btrace_clear (tp);
}

#if defined (HAVE_LIBEXPAT)

/* Check the btrace document version.  */

static void
check_xml_btrace_version (struct gdb_xml_parser *parser,
			  const struct gdb_xml_element *element,
			  void *user_data, VEC (gdb_xml_value_s) *attributes)
{
  const char *version = xml_find_attribute (attributes, "version")->value;

  if (strcmp (version, "1.0") != 0)
    gdb_xml_error (parser, _("Unsupported btrace version: \"%s\""), version);
}

/* Parse a btrace "block" xml record.  */

static void
parse_xml_btrace_block (struct gdb_xml_parser *parser,
			const struct gdb_xml_element *element,
			void *user_data, VEC (gdb_xml_value_s) *attributes)
{
  VEC (btrace_block_s) **btrace;
  struct btrace_block *block;
  ULONGEST *begin, *end;

  btrace = user_data;
  block = VEC_safe_push (btrace_block_s, *btrace, NULL);

  begin = xml_find_attribute (attributes, "begin")->value;
  end = xml_find_attribute (attributes, "end")->value;

  block->begin = *begin;
  block->end = *end;
}

static const struct gdb_xml_attribute block_attributes[] = {
  { "begin", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "end", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute btrace_attributes[] = {
  { "version", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element btrace_children[] = {
  { "block", block_attributes, NULL,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL, parse_xml_btrace_block, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_element btrace_elements[] = {
  { "btrace", btrace_attributes, btrace_children, GDB_XML_EF_NONE,
    check_xml_btrace_version, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

#endif /* defined (HAVE_LIBEXPAT) */

/* See btrace.h.  */

VEC (btrace_block_s) *
parse_xml_btrace (const char *buffer)
{
  VEC (btrace_block_s) *btrace = NULL;
  struct cleanup *cleanup;
  int errcode;

#if defined (HAVE_LIBEXPAT)

  cleanup = make_cleanup (VEC_cleanup (btrace_block_s), &btrace);
  errcode = gdb_xml_parse_quick (_("btrace"), "btrace.dtd", btrace_elements,
				 buffer, &btrace);
  if (errcode != 0)
    {
      do_cleanups (cleanup);
      return NULL;
    }

  /* Keep parse results.  */
  discard_cleanups (cleanup);

#else  /* !defined (HAVE_LIBEXPAT) */

  error (_("Cannot process branch trace.  XML parsing is not supported."));

#endif  /* !defined (HAVE_LIBEXPAT) */

  return btrace;
}
