/* Partial symbol tables.

   Copyright (C) 2009-2020 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "objfiles.h"
#include "psympriv.h"
#include "block.h"
#include "filenames.h"
#include "source.h"
#include "addrmap.h"
#include "gdbtypes.h"
#include "ui-out.h"
#include "command.h"
#include "readline/tilde.h"
#include "gdb_regex.h"
#include "dictionary.h"
#include "language.h"
#include "cp-support.h"
#include "gdbcmd.h"
#include <algorithm>
#include <set>

static struct partial_symbol *lookup_partial_symbol (struct objfile *,
						     struct partial_symtab *,
						     const lookup_name_info &,
						     int,
						     domain_enum);

static const char *psymtab_to_fullname (struct partial_symtab *ps);

static struct partial_symbol *find_pc_sect_psymbol (struct objfile *,
						    struct partial_symtab *,
						    CORE_ADDR,
						    struct obj_section *);

static struct compunit_symtab *psymtab_to_symtab (struct objfile *objfile,
						  struct partial_symtab *pst);



static unsigned long psymbol_hash (const void *addr, int length);
static int psymbol_compare (const void *addr1, const void *addr2, int length);

psymtab_storage::psymtab_storage ()
  : psymbol_cache (psymbol_hash, psymbol_compare)
{
}

psymtab_storage::~psymtab_storage ()
{
  partial_symtab *iter = psymtabs;
  while (iter != nullptr)
    {
      partial_symtab *next = iter->next;
      delete iter;
      iter = next;
    }
}

/* See psymtab.h.  */

void
psymtab_storage::install_psymtab (partial_symtab *pst)
{
  pst->next = psymtabs;
  psymtabs = pst;
}



/* See psymtab.h.  */

psymtab_storage::partial_symtab_range
require_partial_symbols (struct objfile *objfile, bool verbose)
{
  if ((objfile->flags & OBJF_PSYMTABS_READ) == 0)
    {
      objfile->flags |= OBJF_PSYMTABS_READ;

      if (objfile->sf->sym_read_psymbols)
	{
	  if (verbose)
	    printf_filtered (_("Reading symbols from %s...\n"),
			     objfile_name (objfile));
	  (*objfile->sf->sym_read_psymbols) (objfile);

	  /* Partial symbols list are not expected to changed after this
	     point.  */
	  objfile->partial_symtabs->global_psymbols.shrink_to_fit ();
	  objfile->partial_symtabs->static_psymbols.shrink_to_fit ();

	  if (verbose && !objfile_has_symbols (objfile))
	    printf_filtered (_("(No debugging symbols found in %s)\n"),
			     objfile_name (objfile));
	}
    }

  return objfile->psymtabs ();
}

/* Helper function for psym_map_symtabs_matching_filename that
   expands the symtabs and calls the iterator.  */

static bool
partial_map_expand_apply (struct objfile *objfile,
			  const char *name,
			  const char *real_path,
			  struct partial_symtab *pst,
			  gdb::function_view<bool (symtab *)> callback)
{
  struct compunit_symtab *last_made = objfile->compunit_symtabs;

  /* Shared psymtabs should never be seen here.  Instead they should
     be handled properly by the caller.  */
  gdb_assert (pst->user == NULL);

  /* Don't visit already-expanded psymtabs.  */
  if (pst->readin_p (objfile))
    return 0;

  /* This may expand more than one symtab, and we want to iterate over
     all of them.  */
  psymtab_to_symtab (objfile, pst);

  return iterate_over_some_symtabs (name, real_path, objfile->compunit_symtabs,
				    last_made, callback);
}

/*  Psymtab version of map_symtabs_matching_filename.  See its definition in
    the definition of quick_symbol_functions in symfile.h.  */

static bool
psym_map_symtabs_matching_filename
  (struct objfile *objfile,
   const char *name,
   const char *real_path,
   gdb::function_view<bool (symtab *)> callback)
{
  const char *name_basename = lbasename (name);

  for (partial_symtab *pst : require_partial_symbols (objfile, true))
    {
      /* Anonymous psymtabs don't have a file name.  */
      if (pst->anonymous)
	continue;

      if (compare_filenames_for_search (pst->filename, name))
	{
	  while (pst->user)
	    pst = pst->user;

	  if (partial_map_expand_apply (objfile, name, real_path,
					pst, callback))
	    return true;
	  continue;
	}

      /* Before we invoke realpath, which can get expensive when many
	 files are involved, do a quick comparison of the basenames.  */
      if (! basenames_may_differ
	  && FILENAME_CMP (name_basename, lbasename (pst->filename)) != 0)
	continue;

      if (compare_filenames_for_search (psymtab_to_fullname (pst), name))
	{
	  if (partial_map_expand_apply (objfile, name, real_path,
					pst, callback))
	    return true;
	  continue;
	}

      /* If the user gave us an absolute path, try to find the file in
	 this symtab and use its absolute path.  */
      if (real_path != NULL)
	{
	  gdb_assert (IS_ABSOLUTE_PATH (real_path));
	  gdb_assert (IS_ABSOLUTE_PATH (name));
	  if (filename_cmp (psymtab_to_fullname (pst), real_path) == 0)
	    {
	      if (partial_map_expand_apply (objfile, name, real_path,
					    pst, callback))
		return true;
	      continue;
	    }
	}
    }

  return false;
}

/* Find which partial symtab contains PC and SECTION starting at psymtab PST.
   We may find a different psymtab than PST.  See FIND_PC_SECT_PSYMTAB.  */

static struct partial_symtab *
find_pc_sect_psymtab_closer (struct objfile *objfile,
			     CORE_ADDR pc, struct obj_section *section,
			     struct partial_symtab *pst,
			     struct bound_minimal_symbol msymbol)
{
  struct partial_symtab *tpst;
  struct partial_symtab *best_pst = pst;
  CORE_ADDR best_addr = pst->text_low (objfile);

  gdb_assert (!pst->psymtabs_addrmap_supported);

  /* An objfile that has its functions reordered might have
     many partial symbol tables containing the PC, but
     we want the partial symbol table that contains the
     function containing the PC.  */
  if (!(objfile->flags & OBJF_REORDERED)
      && section == NULL)  /* Can't validate section this way.  */
    return pst;

  if (msymbol.minsym == NULL)
    return pst;

  /* The code range of partial symtabs sometimes overlap, so, in
     the loop below, we need to check all partial symtabs and
     find the one that fits better for the given PC address.  We
     select the partial symtab that contains a symbol whose
     address is closest to the PC address.  By closest we mean
     that find_pc_sect_symbol returns the symbol with address
     that is closest and still less than the given PC.  */
  for (tpst = pst; tpst != NULL; tpst = tpst->next)
    {
      if (pc >= tpst->text_low (objfile) && pc < tpst->text_high (objfile))
	{
	  struct partial_symbol *p;
	  CORE_ADDR this_addr;

	  /* NOTE: This assumes that every psymbol has a
	     corresponding msymbol, which is not necessarily
	     true; the debug info might be much richer than the
	     object's symbol table.  */
	  p = find_pc_sect_psymbol (objfile, tpst, pc, section);
	  if (p != NULL
	      && (p->address (objfile) == BMSYMBOL_VALUE_ADDRESS (msymbol)))
	    return tpst;

	  /* Also accept the textlow value of a psymtab as a
	     "symbol", to provide some support for partial
	     symbol tables with line information but no debug
	     symbols (e.g. those produced by an assembler).  */
	  if (p != NULL)
	    this_addr = p->address (objfile);
	  else
	    this_addr = tpst->text_low (objfile);

	  /* Check whether it is closer than our current
	     BEST_ADDR.  Since this symbol address is
	     necessarily lower or equal to PC, the symbol closer
	     to PC is the symbol which address is the highest.
	     This way we return the psymtab which contains such
	     best match symbol.  This can help in cases where the
	     symbol information/debuginfo is not complete, like
	     for instance on IRIX6 with gcc, where no debug info
	     is emitted for statics.  (See also the nodebug.exp
	     testcase.)  */
	  if (this_addr > best_addr)
	    {
	      best_addr = this_addr;
	      best_pst = tpst;
	    }
	}
    }
  return best_pst;
}

/* Find which partial symtab contains PC and SECTION.  Return NULL if
   none.  We return the psymtab that contains a symbol whose address
   exactly matches PC, or, if we cannot find an exact match, the
   psymtab that contains a symbol whose address is closest to PC.  */

static struct partial_symtab *
find_pc_sect_psymtab (struct objfile *objfile, CORE_ADDR pc,
		      struct obj_section *section,
		      struct bound_minimal_symbol msymbol)
{
  /* Try just the PSYMTABS_ADDRMAP mapping first as it has better
     granularity than the later used TEXTLOW/TEXTHIGH one.  However, we need
     to take care as the PSYMTABS_ADDRMAP can hold things other than partial
     symtabs in some cases.

     This function should only be called for objfiles that are using partial
     symtabs, not for objfiles that are using indexes (.gdb_index or
     .debug_names), however 'maintenance print psymbols' calls this function
     directly for all objfiles.  If we assume that PSYMTABS_ADDRMAP contains
     partial symtabs then we will end up returning a pointer to an object
     that is not a partial_symtab, which doesn't end well.  */

  if (objfile->partial_symtabs->psymtabs != NULL
      && objfile->partial_symtabs->psymtabs_addrmap != NULL)
    {
      CORE_ADDR baseaddr = objfile->text_section_offset ();

      struct partial_symtab *pst
	= ((struct partial_symtab *)
	   addrmap_find (objfile->partial_symtabs->psymtabs_addrmap,
			 pc - baseaddr));
      if (pst != NULL)
	{
	  /* FIXME: addrmaps currently do not handle overlayed sections,
	     so fall back to the non-addrmap case if we're debugging
	     overlays and the addrmap returned the wrong section.  */
	  if (overlay_debugging && msymbol.minsym != NULL && section != NULL)
	    {
	      struct partial_symbol *p;

	      /* NOTE: This assumes that every psymbol has a
		 corresponding msymbol, which is not necessarily
		 true; the debug info might be much richer than the
		 object's symbol table.  */
	      p = find_pc_sect_psymbol (objfile, pst, pc, section);
	      if (p == NULL
		  || (p->address (objfile)
		      != BMSYMBOL_VALUE_ADDRESS (msymbol)))
		goto next;
	    }

	  /* We do not try to call FIND_PC_SECT_PSYMTAB_CLOSER as
	     PSYMTABS_ADDRMAP we used has already the best 1-byte
	     granularity and FIND_PC_SECT_PSYMTAB_CLOSER may mislead us into
	     a worse chosen section due to the TEXTLOW/TEXTHIGH ranges
	     overlap.  */

	  return pst;
	}
    }

 next:

  /* Existing PSYMTABS_ADDRMAP mapping is present even for PARTIAL_SYMTABs
     which still have no corresponding full SYMTABs read.  But it is not
     present for non-DWARF2 debug infos not supporting PSYMTABS_ADDRMAP in GDB
     so far.  */

  /* Check even OBJFILE with non-zero PSYMTABS_ADDRMAP as only several of
     its CUs may be missing in PSYMTABS_ADDRMAP as they may be varying
     debug info type in single OBJFILE.  */

  for (partial_symtab *pst : require_partial_symbols (objfile, true))
    if (!pst->psymtabs_addrmap_supported
	&& pc >= pst->text_low (objfile) && pc < pst->text_high (objfile))
      {
	struct partial_symtab *best_pst;

	best_pst = find_pc_sect_psymtab_closer (objfile, pc, section, pst,
						msymbol);
	if (best_pst != NULL)
	  return best_pst;
      }

  return NULL;
}

/* Psymtab version of find_pc_sect_compunit_symtab.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

static struct compunit_symtab *
psym_find_pc_sect_compunit_symtab (struct objfile *objfile,
				   struct bound_minimal_symbol msymbol,
				   CORE_ADDR pc,
				   struct obj_section *section,
				   int warn_if_readin)
{
  struct partial_symtab *ps = find_pc_sect_psymtab (objfile, pc, section,
						    msymbol);
  if (ps != NULL)
    {
      if (warn_if_readin && ps->readin_p (objfile))
	/* Might want to error() here (in case symtab is corrupt and
	   will cause a core dump), but maybe we can successfully
	   continue, so let's not.  */
	warning (_("\
(Internal error: pc %s in read in psymtab, but not in symtab.)\n"),
		 paddress (objfile->arch (), pc));
      psymtab_to_symtab (objfile, ps);
      return ps->get_compunit_symtab (objfile);
    }
  return NULL;
}

/* Find which partial symbol within a psymtab matches PC and SECTION.
   Return NULL if none.  */

static struct partial_symbol *
find_pc_sect_psymbol (struct objfile *objfile,
		      struct partial_symtab *psymtab, CORE_ADDR pc,
		      struct obj_section *section)
{
  struct partial_symbol *best = NULL;
  CORE_ADDR best_pc;
  const CORE_ADDR textlow = psymtab->text_low (objfile);

  gdb_assert (psymtab != NULL);

  /* Cope with programs that start at address 0.  */
  best_pc = (textlow != 0) ? textlow - 1 : 0;

  /* Search the global symbols as well as the static symbols, so that
     find_pc_partial_function doesn't use a minimal symbol and thus
     cache a bad endaddr.  */
  for (int i = 0; i < psymtab->n_global_syms; i++)
    {
      partial_symbol *p
	= objfile->partial_symtabs->global_psymbols[psymtab->globals_offset
						    + i];

      if (p->domain == VAR_DOMAIN
	  && p->aclass == LOC_BLOCK
	  && pc >= p->address (objfile)
	  && (p->address (objfile) > best_pc
	      || (psymtab->text_low (objfile) == 0
		  && best_pc == 0 && p->address (objfile) == 0)))
	{
	  if (section != NULL)  /* Match on a specific section.  */
	    {
	      if (!matching_obj_sections (p->obj_section (objfile),
					  section))
		continue;
	    }
	  best_pc = p->address (objfile);
	  best = p;
	}
    }

  for (int i = 0; i < psymtab->n_static_syms; i++)
    {
      partial_symbol *p
	= objfile->partial_symtabs->static_psymbols[psymtab->statics_offset
						    + i];

      if (p->domain == VAR_DOMAIN
	  && p->aclass == LOC_BLOCK
	  && pc >= p->address (objfile)
	  && (p->address (objfile) > best_pc
	      || (psymtab->text_low (objfile) == 0
		  && best_pc == 0 && p->address (objfile) == 0)))
	{
	  if (section != NULL)  /* Match on a specific section.  */
	    {
	      if (!matching_obj_sections (p->obj_section (objfile),
					  section))
		continue;
	    }
	  best_pc = p->address (objfile);
	  best = p;
	}
    }

  return best;
}

/* Psymtab version of lookup_symbol.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

static struct compunit_symtab *
psym_lookup_symbol (struct objfile *objfile,
		    block_enum block_index, const char *name,
		    const domain_enum domain)
{
  const int psymtab_index = (block_index == GLOBAL_BLOCK ? 1 : 0);
  struct compunit_symtab *stab_best = NULL;

  lookup_name_info lookup_name (name, symbol_name_match_type::FULL);

  lookup_name_info psym_lookup_name = lookup_name.make_ignore_params ();

  for (partial_symtab *ps : require_partial_symbols (objfile, true))
    {
      if (!ps->readin_p (objfile)
	  && lookup_partial_symbol (objfile, ps, psym_lookup_name,
				    psymtab_index, domain))
	{
	  struct symbol *sym, *with_opaque = NULL;
	  struct compunit_symtab *stab = psymtab_to_symtab (objfile, ps);
	  /* Note: While psymtab_to_symtab can return NULL if the
	     partial symtab is empty, we can assume it won't here
	     because lookup_partial_symbol succeeded.  */
	  const struct blockvector *bv = COMPUNIT_BLOCKVECTOR (stab);
	  const struct block *block = BLOCKVECTOR_BLOCK (bv, block_index);

	  sym = block_find_symbol (block, name, domain,
				   block_find_non_opaque_type_preferred,
				   &with_opaque);

	  /* Some caution must be observed with overloaded functions
	     and methods, since the index will not contain any overload
	     information (but NAME might contain it).  */

	  if (sym != NULL
	      && SYMBOL_MATCHES_SEARCH_NAME (sym, lookup_name))
	    return stab;
	  if (with_opaque != NULL
	      && SYMBOL_MATCHES_SEARCH_NAME (with_opaque, lookup_name))
	    stab_best = stab;

	  /* Keep looking through other psymtabs.  */
	}
    }

  return stab_best;
}

/* Psymtab version of lookup_global_symbol_language.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

static enum language
psym_lookup_global_symbol_language (struct objfile *objfile, const char *name,
				    domain_enum domain, bool *symbol_found_p)
{
  *symbol_found_p = false;
  if (objfile->sf == NULL)
    return language_unknown;

  lookup_name_info lookup_name (name, symbol_name_match_type::FULL);

  for (partial_symtab *ps : require_partial_symbols (objfile, true))
    {
      struct partial_symbol *psym;
      if (ps->readin_p (objfile))
	continue;

      psym = lookup_partial_symbol (objfile, ps, lookup_name, 1, domain);
      if (psym)
	{
	  *symbol_found_p = true;
	  return psym->ginfo.language ();
	}
    }

  return language_unknown;
}

/* Returns true if PSYM matches LOOKUP_NAME.  */

static bool
psymbol_name_matches (partial_symbol *psym,
		      const lookup_name_info &lookup_name)
{
  const language_defn *lang = language_def (psym->ginfo.language ());
  symbol_name_matcher_ftype *name_match
    = lang->get_symbol_name_matcher (lookup_name);
  return name_match (psym->ginfo.search_name (), lookup_name, NULL);
}

/* Look in PST for a symbol in DOMAIN whose name matches NAME.  Search
   the global block of PST if GLOBAL, and otherwise the static block.
   MATCH is the comparison operation that returns true iff MATCH (s,
   NAME), where s is a SYMBOL_SEARCH_NAME.  If ORDERED_COMPARE is
   non-null, the symbols in the block are assumed to be ordered
   according to it (allowing binary search).  It must be compatible
   with MATCH.  Returns the symbol, if found, and otherwise NULL.  */

static struct partial_symbol *
match_partial_symbol (struct objfile *objfile,
		      struct partial_symtab *pst, int global,
		      const lookup_name_info &name, domain_enum domain,
		      symbol_compare_ftype *ordered_compare)
{
  struct partial_symbol **start, **psym;
  struct partial_symbol **top, **real_top, **bottom, **center;
  int length = (global ? pst->n_global_syms : pst->n_static_syms);
  int do_linear_search = 1;

  if (length == 0)
    return NULL;

  start = (global ?
	   &objfile->partial_symtabs->global_psymbols[pst->globals_offset] :
	   &objfile->partial_symtabs->static_psymbols[pst->statics_offset]);

  if (global && ordered_compare)  /* Can use a binary search.  */
    {
      do_linear_search = 0;

      /* Binary search.  This search is guaranteed to end with center
         pointing at the earliest partial symbol whose name might be
         correct.  At that point *all* partial symbols with an
         appropriate name will be checked against the correct
         domain.  */

      bottom = start;
      top = start + length - 1;
      real_top = top;
      while (top > bottom)
	{
	  center = bottom + (top - bottom) / 2;
	  gdb_assert (center < top);

	  enum language lang = (*center)->ginfo.language ();
	  const char *lang_ln = name.language_lookup_name (lang);

	  if (ordered_compare ((*center)->ginfo.search_name (),
			       lang_ln) >= 0)
	    top = center;
	  else
	    bottom = center + 1;
	}
      gdb_assert (top == bottom);

      while (top <= real_top
	     && psymbol_name_matches (*top, name))
	{
	  if (symbol_matches_domain ((*top)->ginfo.language (),
				     (*top)->domain, domain))
	    return *top;
	  top++;
	}
    }

  /* Can't use a binary search or else we found during the binary search that
     we should also do a linear search.  */

  if (do_linear_search)
    {
      for (psym = start; psym < start + length; psym++)
	{
	  if (symbol_matches_domain ((*psym)->ginfo.language (),
				     (*psym)->domain, domain)
	      && psymbol_name_matches (*psym, name))
	    return *psym;
	}
    }

  return NULL;
}

/* Look, in partial_symtab PST, for symbol whose natural name is
   LOOKUP_NAME.  Check the global symbols if GLOBAL, the static
   symbols if not.  */

static struct partial_symbol *
lookup_partial_symbol (struct objfile *objfile,
		       struct partial_symtab *pst,
		       const lookup_name_info &lookup_name,
		       int global, domain_enum domain)
{
  struct partial_symbol **start, **psym;
  struct partial_symbol **top, **real_top, **bottom, **center;
  int length = (global ? pst->n_global_syms : pst->n_static_syms);
  int do_linear_search = 1;

  if (length == 0)
    return NULL;

  start = (global ?
	   &objfile->partial_symtabs->global_psymbols[pst->globals_offset] :
	   &objfile->partial_symtabs->static_psymbols[pst->statics_offset]);

  if (global)			/* This means we can use a binary search.  */
    {
      do_linear_search = 0;

      /* Binary search.  This search is guaranteed to end with center
         pointing at the earliest partial symbol whose name might be
         correct.  At that point *all* partial symbols with an
         appropriate name will be checked against the correct
         domain.  */

      bottom = start;
      top = start + length - 1;
      real_top = top;
      while (top > bottom)
	{
	  center = bottom + (top - bottom) / 2;

	  gdb_assert (center < top);

	  if (strcmp_iw_ordered ((*center)->ginfo.search_name (),
				 lookup_name.c_str ()) >= 0)
	    {
	      top = center;
	    }
	  else
	    {
	      bottom = center + 1;
	    }
	}

      gdb_assert (top == bottom);

      /* For `case_sensitivity == case_sensitive_off' strcmp_iw_ordered will
	 search more exactly than what matches SYMBOL_MATCHES_SEARCH_NAME.  */
      while (top >= start && symbol_matches_search_name (&(*top)->ginfo,
							 lookup_name))
	top--;

      /* Fixup to have a symbol which matches SYMBOL_MATCHES_SEARCH_NAME.  */
      top++;

      while (top <= real_top && symbol_matches_search_name (&(*top)->ginfo,
							    lookup_name))
	{
	  if (symbol_matches_domain ((*top)->ginfo.language (),
				     (*top)->domain, domain))
	    return *top;
	  top++;
	}
    }

  /* Can't use a binary search or else we found during the binary search that
     we should also do a linear search.  */

  if (do_linear_search)
    {
      for (psym = start; psym < start + length; psym++)
	{
	  if (symbol_matches_domain ((*psym)->ginfo.language (),
				     (*psym)->domain, domain)
	      && symbol_matches_search_name (&(*psym)->ginfo, lookup_name))
	    return *psym;
	}
    }

  return NULL;
}

/* Get the symbol table that corresponds to a partial_symtab.
   This is fast after the first time you do it.
   The result will be NULL if the primary symtab has no symbols,
   which can happen.  Otherwise the result is the primary symtab
   that contains PST.  */

static struct compunit_symtab *
psymtab_to_symtab (struct objfile *objfile, struct partial_symtab *pst)
{
  /* If it is a shared psymtab, find an unshared psymtab that includes
     it.  Any such psymtab will do.  */
  while (pst->user != NULL)
    pst = pst->user;

  /* If it's been looked up before, return it.  */
  if (pst->get_compunit_symtab (objfile))
    return pst->get_compunit_symtab (objfile);

  /* If it has not yet been read in, read it.  */
  if (!pst->readin_p (objfile))
    {
      scoped_restore decrementer = increment_reading_symtab ();

      if (info_verbose)
	{
	  printf_filtered (_("Reading in symbols for %s...\n"),
			   pst->filename);
	  gdb_flush (gdb_stdout);
	}

      pst->read_symtab (objfile);
    }

  return pst->get_compunit_symtab (objfile);
}

/* Psymtab version of find_last_source_symtab.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

static struct symtab *
psym_find_last_source_symtab (struct objfile *ofp)
{
  struct partial_symtab *cs_pst = NULL;

  for (partial_symtab *ps : require_partial_symbols (ofp, true))
    {
      const char *name = ps->filename;
      int len = strlen (name);

      if (!(len > 2 && (strcmp (&name[len - 2], ".h") == 0
			|| strcmp (name, "<<C++-namespaces>>") == 0)))
	cs_pst = ps;
    }

  if (cs_pst)
    {
      if (cs_pst->readin_p (ofp))
	{
	  internal_error (__FILE__, __LINE__,
			  _("select_source_symtab: "
			  "readin pst found and no symtabs."));
	}
      else
	{
	  struct compunit_symtab *cust = psymtab_to_symtab (ofp, cs_pst);

	  if (cust == NULL)
	    return NULL;
	  return compunit_primary_filetab (cust);
	}
    }
  return NULL;
}

/* Psymtab version of forget_cached_source_info.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

static void
psym_forget_cached_source_info (struct objfile *objfile)
{
  for (partial_symtab *pst : require_partial_symbols (objfile, true))
    {
      if (pst->fullname != NULL)
	{
	  xfree (pst->fullname);
	  pst->fullname = NULL;
	}
    }
}

static void
print_partial_symbols (struct gdbarch *gdbarch, struct objfile *objfile,
		       struct partial_symbol **p, int count, const char *what,
		       struct ui_file *outfile)
{
  fprintf_filtered (outfile, "  %s partial symbols:\n", what);
  while (count-- > 0)
    {
      QUIT;
      fprintf_filtered (outfile, "    `%s'", (*p)->ginfo.linkage_name ());
      if ((*p)->ginfo.demangled_name () != NULL)
	{
	  fprintf_filtered (outfile, "  `%s'",
			    (*p)->ginfo.demangled_name ());
	}
      fputs_filtered (", ", outfile);
      switch ((*p)->domain)
	{
	case UNDEF_DOMAIN:
	  fputs_filtered ("undefined domain, ", outfile);
	  break;
	case VAR_DOMAIN:
	  /* This is the usual thing -- don't print it.  */
	  break;
	case STRUCT_DOMAIN:
	  fputs_filtered ("struct domain, ", outfile);
	  break;
	case MODULE_DOMAIN:
	  fputs_filtered ("module domain, ", outfile);
	  break;
	case LABEL_DOMAIN:
	  fputs_filtered ("label domain, ", outfile);
	  break;
	case COMMON_BLOCK_DOMAIN:
	  fputs_filtered ("common block domain, ", outfile);
	  break;
	default:
	  fputs_filtered ("<invalid domain>, ", outfile);
	  break;
	}
      switch ((*p)->aclass)
	{
	case LOC_UNDEF:
	  fputs_filtered ("undefined", outfile);
	  break;
	case LOC_CONST:
	  fputs_filtered ("constant int", outfile);
	  break;
	case LOC_STATIC:
	  fputs_filtered ("static", outfile);
	  break;
	case LOC_REGISTER:
	  fputs_filtered ("register", outfile);
	  break;
	case LOC_ARG:
	  fputs_filtered ("pass by value", outfile);
	  break;
	case LOC_REF_ARG:
	  fputs_filtered ("pass by reference", outfile);
	  break;
	case LOC_REGPARM_ADDR:
	  fputs_filtered ("register address parameter", outfile);
	  break;
	case LOC_LOCAL:
	  fputs_filtered ("stack parameter", outfile);
	  break;
	case LOC_TYPEDEF:
	  fputs_filtered ("type", outfile);
	  break;
	case LOC_LABEL:
	  fputs_filtered ("label", outfile);
	  break;
	case LOC_BLOCK:
	  fputs_filtered ("function", outfile);
	  break;
	case LOC_CONST_BYTES:
	  fputs_filtered ("constant bytes", outfile);
	  break;
	case LOC_UNRESOLVED:
	  fputs_filtered ("unresolved", outfile);
	  break;
	case LOC_OPTIMIZED_OUT:
	  fputs_filtered ("optimized out", outfile);
	  break;
	case LOC_COMPUTED:
	  fputs_filtered ("computed at runtime", outfile);
	  break;
	default:
	  fputs_filtered ("<invalid location>", outfile);
	  break;
	}
      fputs_filtered (", ", outfile);
      fputs_filtered (paddress (gdbarch, (*p)->unrelocated_address ()), outfile);
      fprintf_filtered (outfile, "\n");
      p++;
    }
}

static void
dump_psymtab (struct objfile *objfile, struct partial_symtab *psymtab,
	      struct ui_file *outfile)
{
  struct gdbarch *gdbarch = objfile->arch ();
  int i;

  if (psymtab->anonymous)
    {
      fprintf_filtered (outfile, "\nAnonymous partial symtab (%s) ",
			psymtab->filename);
    }
  else
    {
      fprintf_filtered (outfile, "\nPartial symtab for source file %s ",
			psymtab->filename);
    }
  fprintf_filtered (outfile, "(object ");
  gdb_print_host_address (psymtab, outfile);
  fprintf_filtered (outfile, ")\n\n");
  fprintf_filtered (outfile, "  Read from object file %s (",
		    objfile_name (objfile));
  gdb_print_host_address (objfile, outfile);
  fprintf_filtered (outfile, ")\n");

  if (psymtab->readin_p (objfile))
    {
      fprintf_filtered (outfile,
			"  Full symtab was read (at ");
      gdb_print_host_address (psymtab->get_compunit_symtab (objfile), outfile);
      fprintf_filtered (outfile, ")\n");
    }

  fprintf_filtered (outfile, "  Symbols cover text addresses ");
  fputs_filtered (paddress (gdbarch, psymtab->text_low (objfile)), outfile);
  fprintf_filtered (outfile, "-");
  fputs_filtered (paddress (gdbarch, psymtab->text_high (objfile)), outfile);
  fprintf_filtered (outfile, "\n");
  fprintf_filtered (outfile, "  Address map supported - %s.\n",
		    psymtab->psymtabs_addrmap_supported ? "yes" : "no");
  fprintf_filtered (outfile, "  Depends on %d other partial symtabs.\n",
		    psymtab->number_of_dependencies);
  for (i = 0; i < psymtab->number_of_dependencies; i++)
    {
      fprintf_filtered (outfile, "    %d ", i);
      gdb_print_host_address (psymtab->dependencies[i], outfile);
      fprintf_filtered (outfile, " %s\n",
			psymtab->dependencies[i]->filename);
    }
  if (psymtab->user != NULL)
    {
      fprintf_filtered (outfile, "  Shared partial symtab with user ");
      gdb_print_host_address (psymtab->user, outfile);
      fprintf_filtered (outfile, "\n");
    }
  if (psymtab->n_global_syms > 0)
    {
      print_partial_symbols
	(gdbarch, objfile,
	 &objfile->partial_symtabs->global_psymbols[psymtab->globals_offset],
	 psymtab->n_global_syms, "Global", outfile);
    }
  if (psymtab->n_static_syms > 0)
    {
      print_partial_symbols
	(gdbarch, objfile,
	 &objfile->partial_symtabs->static_psymbols[psymtab->statics_offset],
	 psymtab->n_static_syms, "Static", outfile);
    }
  fprintf_filtered (outfile, "\n");
}

/* Psymtab version of print_stats.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

static void
psym_print_stats (struct objfile *objfile)
{
  int i;

  i = 0;
  for (partial_symtab *ps : require_partial_symbols (objfile, true))
    {
      if (!ps->readin_p (objfile))
	i++;
    }
  printf_filtered (_("  Number of psym tables (not yet expanded): %d\n"), i);
}

/* Psymtab version of dump.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

static void
psym_dump (struct objfile *objfile)
{
  struct partial_symtab *psymtab;

  if (objfile->partial_symtabs->psymtabs)
    {
      printf_filtered ("Psymtabs:\n");
      for (psymtab = objfile->partial_symtabs->psymtabs;
	   psymtab != NULL;
	   psymtab = psymtab->next)
	{
	  printf_filtered ("%s at ",
			   psymtab->filename);
	  gdb_print_host_address (psymtab, gdb_stdout);
	  printf_filtered (", ");
	  wrap_here ("  ");
	}
      printf_filtered ("\n\n");
    }
}

/* Psymtab version of expand_symtabs_for_function.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

static void
psym_expand_symtabs_for_function (struct objfile *objfile,
				  const char *func_name)
{
  lookup_name_info base_lookup (func_name, symbol_name_match_type::FULL);
  lookup_name_info lookup_name = base_lookup.make_ignore_params ();

  for (partial_symtab *ps : require_partial_symbols (objfile, true))
    {
      if (ps->readin_p (objfile))
	continue;

      if ((lookup_partial_symbol (objfile, ps, lookup_name, 1, VAR_DOMAIN)
	   != NULL)
	  || (lookup_partial_symbol (objfile, ps, lookup_name, 0, VAR_DOMAIN)
	      != NULL))
	psymtab_to_symtab (objfile, ps);
    }
}

/* Psymtab version of expand_all_symtabs.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

static void
psym_expand_all_symtabs (struct objfile *objfile)
{
  for (partial_symtab *psymtab : require_partial_symbols (objfile, true))
    psymtab_to_symtab (objfile, psymtab);
}

/* Psymtab version of expand_symtabs_with_fullname.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

static void
psym_expand_symtabs_with_fullname (struct objfile *objfile,
				   const char *fullname)
{
  for (partial_symtab *p : require_partial_symbols (objfile, true))
    {
      /* Anonymous psymtabs don't have a name of a source file.  */
      if (p->anonymous)
	continue;

      /* psymtab_to_fullname tries to open the file which is slow.
	 Don't call it if we know the basenames don't match.  */
      if ((basenames_may_differ
	   || filename_cmp (lbasename (fullname), lbasename (p->filename)) == 0)
	  && filename_cmp (fullname, psymtab_to_fullname (p)) == 0)
	psymtab_to_symtab (objfile, p);
    }
}

/* Psymtab version of map_symbol_filenames.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

static void
psym_map_symbol_filenames (struct objfile *objfile,
			   symbol_filename_ftype *fun, void *data,
			   int need_fullname)
{
  for (partial_symtab *ps : require_partial_symbols (objfile, true))
    {
      const char *fullname;

      if (ps->readin_p (objfile))
	continue;

      /* We can skip shared psymtabs here, because any file name will be
	 attached to the unshared psymtab.  */
      if (ps->user != NULL)
	continue;

      /* Anonymous psymtabs don't have a file name.  */
      if (ps->anonymous)
	continue;

      QUIT;
      if (need_fullname)
	fullname = psymtab_to_fullname (ps);
      else
	fullname = NULL;
      (*fun) (ps->filename, fullname, data);
    }
}

/* Finds the fullname that a partial_symtab represents.

   If this functions finds the fullname, it will save it in ps->fullname
   and it will also return the value.

   If this function fails to find the file that this partial_symtab represents,
   NULL will be returned and ps->fullname will be set to NULL.  */

static const char *
psymtab_to_fullname (struct partial_symtab *ps)
{
  gdb_assert (!ps->anonymous);

  /* Use cached copy if we have it.
     We rely on forget_cached_source_info being called appropriately
     to handle cases like the file being moved.  */
  if (ps->fullname == NULL)
    {
      gdb::unique_xmalloc_ptr<char> fullname;
      scoped_fd fd = find_and_open_source (ps->filename, ps->dirname,
					   &fullname);
      ps->fullname = fullname.release ();

      if (fd.get () < 0)
	{
	  /* rewrite_source_path would be applied by find_and_open_source, we
	     should report the pathname where GDB tried to find the file.  */

	  if (ps->dirname == NULL || IS_ABSOLUTE_PATH (ps->filename))
	    fullname.reset (xstrdup (ps->filename));
	  else
	    fullname.reset (concat (ps->dirname, SLASH_STRING,
				    ps->filename, (char *) NULL));

	  ps->fullname = rewrite_source_path (fullname.get ()).release ();
	  if (ps->fullname == NULL)
	    ps->fullname = fullname.release ();
	}
    }

  return ps->fullname;
}

/* Psymtab version of map_matching_symbols.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

static void
psym_map_matching_symbols
  (struct objfile *objfile,
   const lookup_name_info &name, domain_enum domain,
   int global,
   gdb::function_view<symbol_found_callback_ftype> callback,
   symbol_compare_ftype *ordered_compare)
{
  const int block_kind = global ? GLOBAL_BLOCK : STATIC_BLOCK;

  for (partial_symtab *ps : require_partial_symbols (objfile, true))
    {
      QUIT;
      if (ps->readin_p (objfile)
	  || match_partial_symbol (objfile, ps, global, name, domain,
				   ordered_compare))
	{
	  struct compunit_symtab *cust = psymtab_to_symtab (objfile, ps);
	  const struct block *block;

	  if (cust == NULL)
	    continue;
	  block = BLOCKVECTOR_BLOCK (COMPUNIT_BLOCKVECTOR (cust), block_kind);
	  if (!iterate_over_symbols_terminated (block, name,
						domain, callback))
	    return;
	}
    }
}

/* A helper for psym_expand_symtabs_matching that handles searching
   included psymtabs.  This returns true if a symbol is found, and
   false otherwise.  It also updates the 'searched_flag' on the
   various psymtabs that it searches.  */

static bool
recursively_search_psymtabs
  (struct partial_symtab *ps,
   struct objfile *objfile,
   enum search_domain domain,
   const lookup_name_info &lookup_name,
   gdb::function_view<expand_symtabs_symbol_matcher_ftype> sym_matcher)
{
  int keep_going = 1;
  enum psymtab_search_status result = PST_SEARCHED_AND_NOT_FOUND;
  int i;

  if (ps->searched_flag != PST_NOT_SEARCHED)
    return ps->searched_flag == PST_SEARCHED_AND_FOUND;

  /* Recurse into shared psymtabs first, because they may have already
     been searched, and this could save some time.  */
  for (i = 0; i < ps->number_of_dependencies; ++i)
    {
      int r;

      /* Skip non-shared dependencies, these are handled elsewhere.  */
      if (ps->dependencies[i]->user == NULL)
	continue;

      r = recursively_search_psymtabs (ps->dependencies[i],
				       objfile, domain, lookup_name,
				       sym_matcher);
      if (r != 0)
	{
	  ps->searched_flag = PST_SEARCHED_AND_FOUND;
	  return true;
	}
    }

  partial_symbol **gbound
    = (objfile->partial_symtabs->global_psymbols.data ()
       + ps->globals_offset + ps->n_global_syms);
  partial_symbol **sbound
    = (objfile->partial_symtabs->static_psymbols.data ()
       + ps->statics_offset + ps->n_static_syms);
  partial_symbol **bound = gbound;

  /* Go through all of the symbols stored in a partial
     symtab in one loop.  */
  partial_symbol **psym = (objfile->partial_symtabs->global_psymbols.data ()
			   + ps->globals_offset);
  while (keep_going)
    {
      if (psym >= bound)
	{
	  if (bound == gbound && ps->n_static_syms != 0)
	    {
	      psym = (objfile->partial_symtabs->static_psymbols.data ()
		      + ps->statics_offset);
	      bound = sbound;
	    }
	  else
	    keep_going = 0;
	  continue;
	}
      else
	{
	  QUIT;

	  if ((domain == ALL_DOMAIN
	       || (domain == MODULES_DOMAIN
		   && (*psym)->domain == MODULE_DOMAIN)
	       || (domain == VARIABLES_DOMAIN
		   && (*psym)->aclass != LOC_TYPEDEF
		   && (*psym)->aclass != LOC_BLOCK)
	       || (domain == FUNCTIONS_DOMAIN
		   && (*psym)->aclass == LOC_BLOCK)
	       || (domain == TYPES_DOMAIN
		   && (*psym)->aclass == LOC_TYPEDEF))
	      && psymbol_name_matches (*psym, lookup_name)
	      && (sym_matcher == NULL
		  || sym_matcher ((*psym)->ginfo.search_name ())))
	    {
	      /* Found a match, so notify our caller.  */
	      result = PST_SEARCHED_AND_FOUND;
	      keep_going = 0;
	    }
	}
      psym++;
    }

  ps->searched_flag = result;
  return result == PST_SEARCHED_AND_FOUND;
}

/* Psymtab version of expand_symtabs_matching.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

static void
psym_expand_symtabs_matching
  (struct objfile *objfile,
   gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
   const lookup_name_info *lookup_name,
   gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
   gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
   enum search_domain domain)
{
  /* Clear the search flags.  */
  for (partial_symtab *ps : require_partial_symbols (objfile, true))
    ps->searched_flag = PST_NOT_SEARCHED;

  for (partial_symtab *ps : objfile->psymtabs ())
    {
      QUIT;

      if (ps->readin_p (objfile))
	continue;

      /* We skip shared psymtabs because file-matching doesn't apply
	 to them; but we search them later in the loop.  */
      if (ps->user != NULL)
	continue;

      if (file_matcher)
	{
	  bool match;

	  if (ps->anonymous)
	    continue;

	  match = file_matcher (ps->filename, false);
	  if (!match)
	    {
	      /* Before we invoke realpath, which can get expensive when many
		 files are involved, do a quick comparison of the basenames.  */
	      if (basenames_may_differ
		  || file_matcher (lbasename (ps->filename), true))
		match = file_matcher (psymtab_to_fullname (ps), false);
	    }
	  if (!match)
	    continue;
	}

      if ((symbol_matcher == NULL && lookup_name == NULL)
	  || recursively_search_psymtabs (ps, objfile, domain,
					  lookup_name->make_ignore_params (),
					  symbol_matcher))
	{
	  struct compunit_symtab *symtab =
	    psymtab_to_symtab (objfile, ps);

	  if (expansion_notify != NULL)
	    expansion_notify (symtab);
	}
    }
}

/* Psymtab version of has_symbols.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

static int
psym_has_symbols (struct objfile *objfile)
{
  return objfile->partial_symtabs->psymtabs != NULL;
}

/* Helper function for psym_find_compunit_symtab_by_address that fills
   in psymbol_map for a given range of psymbols.  */

static void
psym_fill_psymbol_map (struct objfile *objfile,
		       struct partial_symtab *psymtab,
		       std::set<CORE_ADDR> *seen_addrs,
		       const std::vector<partial_symbol *> &symbols,
		       int start,
		       int length)
{
  for (int i = 0; i < length; ++i)
    {
      struct partial_symbol *psym = symbols[start + i];

      if (psym->aclass == LOC_STATIC)
	{
	  CORE_ADDR addr = psym->address (objfile);
	  if (seen_addrs->find (addr) == seen_addrs->end ())
	    {
	      seen_addrs->insert (addr);
	      objfile->psymbol_map.emplace_back (addr, psymtab);
	    }
	}
    }
}

/* See find_compunit_symtab_by_address in quick_symbol_functions, in
   symfile.h.  */

static compunit_symtab *
psym_find_compunit_symtab_by_address (struct objfile *objfile,
				      CORE_ADDR address)
{
  if (objfile->psymbol_map.empty ())
    {
      std::set<CORE_ADDR> seen_addrs;

      for (partial_symtab *pst : require_partial_symbols (objfile, true))
	{
	  psym_fill_psymbol_map (objfile, pst,
				 &seen_addrs,
				 objfile->partial_symtabs->global_psymbols,
				 pst->globals_offset,
				 pst->n_global_syms);
	  psym_fill_psymbol_map (objfile, pst,
				 &seen_addrs,
				 objfile->partial_symtabs->static_psymbols,
				 pst->statics_offset,
				 pst->n_static_syms);
	}

      objfile->psymbol_map.shrink_to_fit ();

      std::sort (objfile->psymbol_map.begin (), objfile->psymbol_map.end (),
		 [] (const std::pair<CORE_ADDR, partial_symtab *> &a,
		     const std::pair<CORE_ADDR, partial_symtab *> &b)
		 {
		   return a.first < b.first;
		 });
    }

  auto iter = std::lower_bound
    (objfile->psymbol_map.begin (), objfile->psymbol_map.end (), address,
     [] (const std::pair<CORE_ADDR, partial_symtab *> &a,
	 CORE_ADDR b)
     {
       return a.first < b;
     });

  if (iter == objfile->psymbol_map.end () || iter->first != address)
    return NULL;

  return psymtab_to_symtab (objfile, iter->second);
}

const struct quick_symbol_functions psym_functions =
{
  psym_has_symbols,
  psym_find_last_source_symtab,
  psym_forget_cached_source_info,
  psym_map_symtabs_matching_filename,
  psym_lookup_symbol,
  psym_lookup_global_symbol_language,
  psym_print_stats,
  psym_dump,
  psym_expand_symtabs_for_function,
  psym_expand_all_symtabs,
  psym_expand_symtabs_with_fullname,
  psym_map_matching_symbols,
  psym_expand_symtabs_matching,
  psym_find_pc_sect_compunit_symtab,
  psym_find_compunit_symtab_by_address,
  psym_map_symbol_filenames
};



static void
sort_pst_symbols (struct objfile *objfile, struct partial_symtab *pst)
{
  /* Sort the global list; don't sort the static list.  */
  auto begin = objfile->partial_symtabs->global_psymbols.begin ();
  std::advance (begin, pst->globals_offset);

  /* The psymbols for this partial_symtab are currently at the end of the
     vector.  */
  auto end = objfile->partial_symtabs->global_psymbols.end ();

  std::sort (begin, end, [] (partial_symbol *s1, partial_symbol *s2)
    {
      return strcmp_iw_ordered (s1->ginfo.search_name (),
				s2->ginfo.search_name ()) < 0;
    });
}

/* Partially fill a partial symtab.  It will be completely filled at
   the end of the symbol list.  */

partial_symtab::partial_symtab (const char *filename,
				struct objfile *objfile,
				CORE_ADDR textlow)
  : partial_symtab (filename, objfile)
{
  set_text_low (textlow);
  set_text_high (raw_text_low ()); /* default */

  auto *v1 = new std::vector<partial_symbol *>;
  objfile->partial_symtabs->current_global_psymbols.push_back (v1);
  auto *v2 = new std::vector<partial_symbol *>;
  objfile->partial_symtabs->current_static_psymbols.push_back (v2);
}

/* Concat vectors V1 and V2.  */

static void
concat (std::vector<partial_symbol *> *v1, std::vector<partial_symbol *> *v2)
{
  v1->insert (v1->end (), v2->begin (), v2->end ());
  v2->clear ();
}

/* Perform "finishing up" operations of a partial symtab.  */

void
end_psymtab_common (struct objfile *objfile, struct partial_symtab *pst)
{
  pst->globals_offset = objfile->partial_symtabs->global_psymbols.size ();
  pst->statics_offset = objfile->partial_symtabs->static_psymbols.size ();

  auto *current_global_psymbols
    = objfile->partial_symtabs->current_global_psymbols.back ();
  auto *current_static_psymbols
    = objfile->partial_symtabs->current_static_psymbols.back ();
  objfile->partial_symtabs->current_global_psymbols.pop_back ();
  objfile->partial_symtabs->current_static_psymbols.pop_back ();

  pst->n_global_syms
    = current_global_psymbols->size ();
  pst->n_static_syms
    = current_static_psymbols->size ();

  concat (&objfile->partial_symtabs->global_psymbols, current_global_psymbols);
  concat (&objfile->partial_symtabs->static_psymbols, current_static_psymbols);

  delete current_global_psymbols;
  delete current_static_psymbols;

  sort_pst_symbols (objfile, pst);
}

/* Calculate a hash code for the given partial symbol.  The hash is
   calculated using the symbol's value, language, domain, class
   and name.  These are the values which are set by
   add_psymbol_to_bcache.  */

static unsigned long
psymbol_hash (const void *addr, int length)
{
  unsigned long h = 0;
  struct partial_symbol *psymbol = (struct partial_symbol *) addr;
  unsigned int lang = psymbol->ginfo.language ();
  unsigned int domain = psymbol->domain;
  unsigned int theclass = psymbol->aclass;

  h = fast_hash (&psymbol->ginfo.value, sizeof (psymbol->ginfo.value), h);
  h = fast_hash (&lang, sizeof (unsigned int), h);
  h = fast_hash (&domain, sizeof (unsigned int), h);
  h = fast_hash (&theclass, sizeof (unsigned int), h);
  /* Note that psymbol names are interned via compute_and_set_names, so
     there's no need to hash the contents of the name here.  */
  h = fast_hash (&psymbol->ginfo.m_name, sizeof (psymbol->ginfo.m_name), h);

  return h;
}

/* Returns true if the symbol at addr1 equals the symbol at addr2.
   For the comparison this function uses a symbols value,
   language, domain, class and name.  */

static int
psymbol_compare (const void *addr1, const void *addr2, int length)
{
  struct partial_symbol *sym1 = (struct partial_symbol *) addr1;
  struct partial_symbol *sym2 = (struct partial_symbol *) addr2;

  return (memcmp (&sym1->ginfo.value, &sym2->ginfo.value,
                  sizeof (sym1->ginfo.value)) == 0
	  && sym1->ginfo.language () == sym2->ginfo.language ()
          && sym1->domain == sym2->domain
          && sym1->aclass == sym2->aclass
	  /* Note that psymbol names are interned via
	     compute_and_set_names, so there's no need to compare the
	     contents of the name here.  */
          && sym1->ginfo.linkage_name () == sym2->ginfo.linkage_name ());
}

/* Helper function, initialises partial symbol structure and stashes
   it into objfile's bcache.  Note that our caching mechanism will
   use all fields of struct partial_symbol to determine hash value of the
   structure.  In other words, having two symbols with the same name but
   different domain (or address) is possible and correct.  */

static struct partial_symbol *
add_psymbol_to_bcache (const partial_symbol &psymbol, struct objfile *objfile,
		       bool *added)
{
  /* Stash the partial symbol away in the cache.  */
  return ((struct partial_symbol *)
	  objfile->partial_symtabs->psymbol_cache.insert
	  (&psymbol, sizeof (struct partial_symbol), added));
}

/* Helper function, adds partial symbol to the given partial symbol list.  */

static void
append_psymbol_to_list (std::vector<partial_symbol *> *list,
			struct partial_symbol *psym,
			struct objfile *objfile)
{
  list->push_back (psym);
  OBJSTAT (objfile, n_psyms++);
}

/* See psympriv.h.  */

void
add_psymbol_to_list (const partial_symbol &psymbol,
		     psymbol_placement where,
		     struct objfile *objfile)
{
  bool added;

  /* Stash the partial symbol away in the cache.  */
  partial_symbol *psym = add_psymbol_to_bcache (psymbol, objfile, &added);

  /* Do not duplicate global partial symbols.  */
  if (where == psymbol_placement::GLOBAL && !added)
    return;

  /* Save pointer to partial symbol in psymtab, growing symtab if needed.  */
  std::vector<partial_symbol *> *list
    = (where == psymbol_placement::STATIC
       ? objfile->partial_symtabs->current_static_psymbols.back ()
       : objfile->partial_symtabs->current_global_psymbols.back ());
  append_psymbol_to_list (list, psym, objfile);
}

/* See psympriv.h.  */

void
add_psymbol_to_list (gdb::string_view name, bool copy_name,
		     domain_enum domain,
		     enum address_class theclass,
		     short section,
		     psymbol_placement where,
		     CORE_ADDR coreaddr,
		     enum language language, struct objfile *objfile)
{
  struct partial_symbol psymbol;
  memset (&psymbol, 0, sizeof (psymbol));

  psymbol.set_unrelocated_address (coreaddr);
  psymbol.ginfo.section = section;
  psymbol.domain = domain;
  psymbol.aclass = theclass;
  psymbol.ginfo.set_language (language, objfile->partial_symtabs->obstack ());
  psymbol.ginfo.compute_and_set_names (name, copy_name, objfile->per_bfd);

  add_psymbol_to_list (psymbol, where, objfile);
}

/* See psympriv.h.  */

void
init_psymbol_list (struct objfile *objfile, int total_symbols)
{
  if (objfile->partial_symtabs->global_psymbols.capacity () == 0
      && objfile->partial_symtabs->static_psymbols.capacity () == 0)
    {
      /* Current best guess is that approximately a twentieth of the
	 total symbols (in a debugging file) are global or static
	 oriented symbols, then multiply that by slop factor of
	 two.  */
      objfile->partial_symtabs->global_psymbols.reserve (total_symbols / 10);
      objfile->partial_symtabs->static_psymbols.reserve (total_symbols / 10);
    }
}

/* See psympriv.h.  */

partial_symtab::partial_symtab (const char *filename_, struct objfile *objfile)
  : searched_flag (PST_NOT_SEARCHED),
    text_low_valid (0),
    text_high_valid (0)
{
  objfile->partial_symtabs->install_psymtab (this);

  filename = objfile->intern (filename_);

  if (symtab_create_debug)
    {
      /* Be a bit clever with debugging messages, and don't print objfile
	 every time, only when it changes.  */
      static char *last_objfile_name = NULL;

      if (last_objfile_name == NULL
	  || strcmp (last_objfile_name, objfile_name (objfile)) != 0)
	{
	  xfree (last_objfile_name);
	  last_objfile_name = xstrdup (objfile_name (objfile));
	  fprintf_filtered (gdb_stdlog,
			    "Creating one or more psymtabs for objfile %s ...\n",
			    last_objfile_name);
	}
      fprintf_filtered (gdb_stdlog,
			"Created psymtab %s for module %s.\n",
			host_address_to_string (this), filename);
    }
}

/* See psympriv.h.  */

void
partial_symtab::expand_dependencies (struct objfile *objfile)
{
  for (int i = 0; i < number_of_dependencies; ++i)
    {
      if (!dependencies[i]->readin_p (objfile)
	  && dependencies[i]->user == NULL)
	{
	  /* Inform about additional files to be read in.  */
	  if (info_verbose)
	    {
	      fputs_filtered (" ", gdb_stdout);
	      wrap_here ("");
	      fputs_filtered ("and ", gdb_stdout);
	      wrap_here ("");
	      printf_filtered ("%s...", dependencies[i]->filename);
	      wrap_here ("");	/* Flush output */
	      gdb_flush (gdb_stdout);
	    }
	  dependencies[i]->expand_psymtab (objfile);
	}
    }
}


void
psymtab_storage::discard_psymtab (struct partial_symtab *pst)
{
  struct partial_symtab **prev_pst;

  /* From dbxread.c:
     Empty psymtabs happen as a result of header files which don't
     have any symbols in them.  There can be a lot of them.  But this
     check is wrong, in that a psymtab with N_SLINE entries but
     nothing else is not empty, but we don't realize that.  Fixing
     that without slowing things down might be tricky.  */

  /* First, snip it out of the psymtab chain.  */

  prev_pst = &psymtabs;
  while ((*prev_pst) != pst)
    prev_pst = &((*prev_pst)->next);
  (*prev_pst) = pst->next;
  delete pst;
}



/* We need to pass a couple of items to the addrmap_foreach function,
   so use a struct.  */

struct dump_psymtab_addrmap_data
{
  struct objfile *objfile;
  struct partial_symtab *psymtab;
  struct ui_file *outfile;

  /* Non-zero if the previously printed addrmap entry was for PSYMTAB.
     If so, we want to print the next one as well (since the next addrmap
     entry defines the end of the range).  */
  int previous_matched;
};

/* Helper function for dump_psymtab_addrmap to print an addrmap entry.  */

static int
dump_psymtab_addrmap_1 (void *datap, CORE_ADDR start_addr, void *obj)
{
  struct dump_psymtab_addrmap_data *data
    = (struct dump_psymtab_addrmap_data *) datap;
  struct gdbarch *gdbarch = data->objfile->arch ();
  struct partial_symtab *addrmap_psymtab = (struct partial_symtab *) obj;
  const char *psymtab_address_or_end = NULL;

  QUIT;

  if (data->psymtab == NULL
      || data->psymtab == addrmap_psymtab)
    psymtab_address_or_end = host_address_to_string (addrmap_psymtab);
  else if (data->previous_matched)
    psymtab_address_or_end = "<ends here>";

  if (data->psymtab == NULL
      || data->psymtab == addrmap_psymtab
      || data->previous_matched)
    {
      fprintf_filtered (data->outfile, "  %s%s %s\n",
			data->psymtab != NULL ? "  " : "",
			paddress (gdbarch, start_addr),
			psymtab_address_or_end);
    }

  data->previous_matched = (data->psymtab == NULL
			    || data->psymtab == addrmap_psymtab);

  return 0;
}

/* Helper function for maintenance_print_psymbols to print the addrmap
   of PSYMTAB.  If PSYMTAB is NULL print the entire addrmap.  */

static void
dump_psymtab_addrmap (struct objfile *objfile, struct partial_symtab *psymtab,
		      struct ui_file *outfile)
{
  struct dump_psymtab_addrmap_data addrmap_dump_data;

  if ((psymtab == NULL
       || psymtab->psymtabs_addrmap_supported)
      && objfile->partial_symtabs->psymtabs_addrmap != NULL)
    {
      addrmap_dump_data.objfile = objfile;
      addrmap_dump_data.psymtab = psymtab;
      addrmap_dump_data.outfile = outfile;
      addrmap_dump_data.previous_matched = 0;
      fprintf_filtered (outfile, "%sddress map:\n",
			psymtab == NULL ? "Entire a" : "  A");
      addrmap_foreach (objfile->partial_symtabs->psymtabs_addrmap,
		       dump_psymtab_addrmap_1, &addrmap_dump_data);
    }
}

static void
maintenance_print_psymbols (const char *args, int from_tty)
{
  struct ui_file *outfile = gdb_stdout;
  char *address_arg = NULL, *source_arg = NULL, *objfile_arg = NULL;
  int i, outfile_idx, found;
  CORE_ADDR pc = 0;
  struct obj_section *section = NULL;

  dont_repeat ();

  gdb_argv argv (args);

  for (i = 0; argv != NULL && argv[i] != NULL; ++i)
    {
      if (strcmp (argv[i], "-pc") == 0)
	{
	  if (argv[i + 1] == NULL)
	    error (_("Missing pc value"));
	  address_arg = argv[++i];
	}
      else if (strcmp (argv[i], "-source") == 0)
	{
	  if (argv[i + 1] == NULL)
	    error (_("Missing source file"));
	  source_arg = argv[++i];
	}
      else if (strcmp (argv[i], "-objfile") == 0)
	{
	  if (argv[i + 1] == NULL)
	    error (_("Missing objfile name"));
	  objfile_arg = argv[++i];
	}
      else if (strcmp (argv[i], "--") == 0)
	{
	  /* End of options.  */
	  ++i;
	  break;
	}
      else if (argv[i][0] == '-')
	{
	  /* Future proofing: Don't allow OUTFILE to begin with "-".  */
	  error (_("Unknown option: %s"), argv[i]);
	}
      else
	break;
    }
  outfile_idx = i;

  if (address_arg != NULL && source_arg != NULL)
    error (_("Must specify at most one of -pc and -source"));

  stdio_file arg_outfile;

  if (argv != NULL && argv[outfile_idx] != NULL)
    {
      if (argv[outfile_idx + 1] != NULL)
	error (_("Junk at end of command"));
      gdb::unique_xmalloc_ptr<char> outfile_name
	(tilde_expand (argv[outfile_idx]));
      if (!arg_outfile.open (outfile_name.get (), FOPEN_WT))
	perror_with_name (outfile_name.get ());
      outfile = &arg_outfile;
    }

  if (address_arg != NULL)
    {
      pc = parse_and_eval_address (address_arg);
      /* If we fail to find a section, that's ok, try the lookup anyway.  */
      section = find_pc_section (pc);
    }

  found = 0;
  for (objfile *objfile : current_program_space->objfiles ())
    {
      int printed_objfile_header = 0;
      int print_for_objfile = 1;

      QUIT;
      if (objfile_arg != NULL)
	print_for_objfile
	  = compare_filenames_for_search (objfile_name (objfile),
					  objfile_arg);
      if (!print_for_objfile)
	continue;

      if (address_arg != NULL)
	{
	  struct bound_minimal_symbol msymbol = { NULL, NULL };

	  /* We don't assume each pc has a unique objfile (this is for
	     debugging).  */
	  struct partial_symtab *ps = find_pc_sect_psymtab (objfile, pc,
							    section, msymbol);
	  if (ps != NULL)
	    {
	      if (!printed_objfile_header)
		{
		  outfile->printf ("\nPartial symtabs for objfile %s\n",
				  objfile_name (objfile));
		  printed_objfile_header = 1;
		}
	      dump_psymtab (objfile, ps, outfile);
	      dump_psymtab_addrmap (objfile, ps, outfile);
	      found = 1;
	    }
	}
      else
	{
	  for (partial_symtab *ps : require_partial_symbols (objfile, true))
	    {
	      int print_for_source = 0;

	      QUIT;
	      if (source_arg != NULL)
		{
		  print_for_source
		    = compare_filenames_for_search (ps->filename, source_arg);
		  found = 1;
		}
	      if (source_arg == NULL
		  || print_for_source)
		{
		  if (!printed_objfile_header)
		    {
		      outfile->printf ("\nPartial symtabs for objfile %s\n",
				       objfile_name (objfile));
		      printed_objfile_header = 1;
		    }
		  dump_psymtab (objfile, ps, outfile);
		  dump_psymtab_addrmap (objfile, ps, outfile);
		}
	    }
	}

      /* If we're printing all the objfile's symbols dump the full addrmap.  */

      if (address_arg == NULL
	  && source_arg == NULL
	  && objfile->partial_symtabs->psymtabs_addrmap != NULL)
	{
	  outfile->puts ("\n");
	  dump_psymtab_addrmap (objfile, NULL, outfile);
	}
    }

  if (!found)
    {
      if (address_arg != NULL)
	error (_("No partial symtab for address: %s"), address_arg);
      if (source_arg != NULL)
	error (_("No partial symtab for source file: %s"), source_arg);
    }
}

/* List all the partial symbol tables whose names match REGEXP (optional).  */

static void
maintenance_info_psymtabs (const char *regexp, int from_tty)
{
  if (regexp)
    re_comp (regexp);

  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      {
	struct gdbarch *gdbarch = objfile->arch ();

	/* We don't want to print anything for this objfile until we
	   actually find a symtab whose name matches.  */
	int printed_objfile_start = 0;

	for (partial_symtab *psymtab : require_partial_symbols (objfile, true))
	  {
	    QUIT;

	    if (! regexp
		|| re_exec (psymtab->filename))
	      {
		if (! printed_objfile_start)
		  {
		    printf_filtered ("{ objfile %s ", objfile_name (objfile));
		    wrap_here ("  ");
		    printf_filtered ("((struct objfile *) %s)\n",
				     host_address_to_string (objfile));
		    printed_objfile_start = 1;
		  }

		printf_filtered ("  { psymtab %s ", psymtab->filename);
		wrap_here ("    ");
		printf_filtered ("((struct partial_symtab *) %s)\n",
				 host_address_to_string (psymtab));

		printf_filtered ("    readin %s\n",
				 psymtab->readin_p (objfile) ? "yes" : "no");
		printf_filtered ("    fullname %s\n",
				 psymtab->fullname
				 ? psymtab->fullname : "(null)");
		printf_filtered ("    text addresses ");
		fputs_filtered (paddress (gdbarch,
					  psymtab->text_low (objfile)),
				gdb_stdout);
		printf_filtered (" -- ");
		fputs_filtered (paddress (gdbarch,
					  psymtab->text_high (objfile)),
				gdb_stdout);
		printf_filtered ("\n");
		printf_filtered ("    psymtabs_addrmap_supported %s\n",
				 (psymtab->psymtabs_addrmap_supported
				  ? "yes" : "no"));
		printf_filtered ("    globals ");
		if (psymtab->n_global_syms)
		  {
		    auto p = &(objfile->partial_symtabs
			       ->global_psymbols[psymtab->globals_offset]);

		    printf_filtered
		      ("(* (struct partial_symbol **) %s @ %d)\n",
		       host_address_to_string (p),
		       psymtab->n_global_syms);
		  }
		else
		  printf_filtered ("(none)\n");
		printf_filtered ("    statics ");
		if (psymtab->n_static_syms)
		  {
		    auto p = &(objfile->partial_symtabs
			       ->static_psymbols[psymtab->statics_offset]);

		    printf_filtered
		      ("(* (struct partial_symbol **) %s @ %d)\n",
		       host_address_to_string (p),
		       psymtab->n_static_syms);
		  }
		else
		  printf_filtered ("(none)\n");
		if (psymtab->user)
		  printf_filtered ("    user %s "
				   "((struct partial_symtab *) %s)\n",
				   psymtab->user->filename,
				   host_address_to_string (psymtab->user));
		printf_filtered ("    dependencies ");
		if (psymtab->number_of_dependencies)
		  {
		    int i;

		    printf_filtered ("{\n");
		    for (i = 0; i < psymtab->number_of_dependencies; i++)
		      {
			struct partial_symtab *dep = psymtab->dependencies[i];

			/* Note the string concatenation there --- no
			   comma.  */
			printf_filtered ("      psymtab %s "
					 "((struct partial_symtab *) %s)\n",
					 dep->filename,
					 host_address_to_string (dep));
		      }
		    printf_filtered ("    }\n");
		  }
		else
		  printf_filtered ("(none)\n");
		printf_filtered ("  }\n");
	      }
	  }

	if (printed_objfile_start)
	  printf_filtered ("}\n");
      }
}

/* Check consistency of currently expanded psymtabs vs symtabs.  */

static void
maintenance_check_psymtabs (const char *ignore, int from_tty)
{
  struct symbol *sym;
  struct compunit_symtab *cust = NULL;
  const struct blockvector *bv;
  const struct block *b;
  int i;

  for (objfile *objfile : current_program_space->objfiles ())
    for (partial_symtab *ps : require_partial_symbols (objfile, true))
      {
	struct gdbarch *gdbarch = objfile->arch ();

	/* We don't call psymtab_to_symtab here because that may cause symtab
	   expansion.  When debugging a problem it helps if checkers leave
	   things unchanged.  */
	cust = ps->get_compunit_symtab (objfile);

	/* First do some checks that don't require the associated symtab.  */
	if (ps->text_high (objfile) < ps->text_low (objfile))
	  {
	    printf_filtered ("Psymtab ");
	    puts_filtered (ps->filename);
	    printf_filtered (" covers bad range ");
	    fputs_filtered (paddress (gdbarch, ps->text_low (objfile)),
			    gdb_stdout);
	    printf_filtered (" - ");
	    fputs_filtered (paddress (gdbarch, ps->text_high (objfile)),
			    gdb_stdout);
	    printf_filtered ("\n");
	    continue;
	  }

	/* Now do checks requiring the associated symtab.  */
	if (cust == NULL)
	  continue;
	bv = COMPUNIT_BLOCKVECTOR (cust);
	b = BLOCKVECTOR_BLOCK (bv, STATIC_BLOCK);
	partial_symbol **psym
	  = &objfile->partial_symtabs->static_psymbols[ps->statics_offset];
	for (i = 0; i < ps->n_static_syms; psym++, i++)
	  {
	    /* Skip symbols for inlined functions without address.  These may
	       or may not have a match in the full symtab.  */
	    if ((*psym)->aclass == LOC_BLOCK
		&& (*psym)->ginfo.value.address == 0)
	      continue;

	    sym = block_lookup_symbol (b, (*psym)->ginfo.search_name (),
				       symbol_name_match_type::SEARCH_NAME,
				       (*psym)->domain);
	    if (!sym)
	      {
		printf_filtered ("Static symbol `");
		puts_filtered ((*psym)->ginfo.linkage_name ());
		printf_filtered ("' only found in ");
		puts_filtered (ps->filename);
		printf_filtered (" psymtab\n");
	      }
	  }
	b = BLOCKVECTOR_BLOCK (bv, GLOBAL_BLOCK);
	psym = &objfile->partial_symtabs->global_psymbols[ps->globals_offset];
	for (i = 0; i < ps->n_global_syms; psym++, i++)
	  {
	    sym = block_lookup_symbol (b, (*psym)->ginfo.search_name (),
				       symbol_name_match_type::SEARCH_NAME,
				       (*psym)->domain);
	    if (!sym)
	      {
		printf_filtered ("Global symbol `");
		puts_filtered ((*psym)->ginfo.linkage_name ());
		printf_filtered ("' only found in ");
		puts_filtered (ps->filename);
		printf_filtered (" psymtab\n");
	      }
	  }
	if (ps->raw_text_high () != 0
	    && (ps->text_low (objfile) < BLOCK_START (b)
		|| ps->text_high (objfile) > BLOCK_END (b)))
	  {
	    printf_filtered ("Psymtab ");
	    puts_filtered (ps->filename);
	    printf_filtered (" covers ");
	    fputs_filtered (paddress (gdbarch, ps->text_low (objfile)),
			    gdb_stdout);
	    printf_filtered (" - ");
	    fputs_filtered (paddress (gdbarch, ps->text_high (objfile)),
			    gdb_stdout);
	    printf_filtered (" but symtab covers only ");
	    fputs_filtered (paddress (gdbarch, BLOCK_START (b)), gdb_stdout);
	    printf_filtered (" - ");
	    fputs_filtered (paddress (gdbarch, BLOCK_END (b)), gdb_stdout);
	    printf_filtered ("\n");
	  }
      }
}

void _initialize_psymtab ();
void
_initialize_psymtab ()
{
  add_cmd ("psymbols", class_maintenance, maintenance_print_psymbols, _("\
Print dump of current partial symbol definitions.\n\
Usage: mt print psymbols [-objfile OBJFILE] [-pc ADDRESS] [--] [OUTFILE]\n\
       mt print psymbols [-objfile OBJFILE] [-source SOURCE] [--] [OUTFILE]\n\
Entries in the partial symbol table are dumped to file OUTFILE,\n\
or the terminal if OUTFILE is unspecified.\n\
If ADDRESS is provided, dump only the file for that address.\n\
If SOURCE is provided, dump only that file's symbols.\n\
If OBJFILE is provided, dump only that file's minimal symbols."),
	   &maintenanceprintlist);

  add_cmd ("psymtabs", class_maintenance, maintenance_info_psymtabs, _("\
List the partial symbol tables for all object files.\n\
This does not include information about individual partial symbols,\n\
just the symbol table structures themselves."),
	   &maintenanceinfolist);

  add_cmd ("check-psymtabs", class_maintenance, maintenance_check_psymtabs,
	   _("\
Check consistency of currently expanded psymtabs versus symtabs."),
	   &maintenancelist);
}
