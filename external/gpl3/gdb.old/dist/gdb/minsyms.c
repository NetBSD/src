/* GDB routines for manipulating the minimal symbol tables.
   Copyright (C) 1992-2017 Free Software Foundation, Inc.
   Contributed by Cygnus Support, using pieces from other GDB modules.

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


/* This file contains support routines for creating, manipulating, and
   destroying minimal symbol tables.

   Minimal symbol tables are used to hold some very basic information about
   all defined global symbols (text, data, bss, abs, etc).  The only two
   required pieces of information are the symbol's name and the address
   associated with that symbol.

   In many cases, even if a file was compiled with no special options for
   debugging at all, as long as was not stripped it will contain sufficient
   information to build useful minimal symbol tables using this structure.

   Even when a file contains enough debugging information to build a full
   symbol table, these minimal symbols are still useful for quickly mapping
   between names and addresses, and vice versa.  They are also sometimes used
   to figure out what full symbol table entries need to be read in.  */


#include "defs.h"
#include <ctype.h>
#include "symtab.h"
#include "bfd.h"
#include "filenames.h"
#include "symfile.h"
#include "objfiles.h"
#include "demangle.h"
#include "value.h"
#include "cp-abi.h"
#include "target.h"
#include "cp-support.h"
#include "language.h"
#include "cli/cli-utils.h"
#include "symbol.h"

/* Accumulate the minimal symbols for each objfile in bunches of BUNCH_SIZE.
   At the end, copy them all into one newly allocated location on an objfile's
   per-BFD storage obstack.  */

#define BUNCH_SIZE 127

struct msym_bunch
  {
    struct msym_bunch *next;
    struct minimal_symbol contents[BUNCH_SIZE];
  };

/* See minsyms.h.  */

unsigned int
msymbol_hash_iw (const char *string)
{
  unsigned int hash = 0;

  while (*string && *string != '(')
    {
      string = skip_spaces_const (string);
      if (*string && *string != '(')
	{
	  hash = SYMBOL_HASH_NEXT (hash, *string);
	  ++string;
	}
    }
  return hash;
}

/* See minsyms.h.  */

unsigned int
msymbol_hash (const char *string)
{
  unsigned int hash = 0;

  for (; *string; ++string)
    hash = SYMBOL_HASH_NEXT (hash, *string);
  return hash;
}

/* Add the minimal symbol SYM to an objfile's minsym hash table, TABLE.  */
static void
add_minsym_to_hash_table (struct minimal_symbol *sym,
			  struct minimal_symbol **table)
{
  if (sym->hash_next == NULL)
    {
      unsigned int hash
	= msymbol_hash (MSYMBOL_LINKAGE_NAME (sym)) % MINIMAL_SYMBOL_HASH_SIZE;

      sym->hash_next = table[hash];
      table[hash] = sym;
    }
}

/* Add the minimal symbol SYM to an objfile's minsym demangled hash table,
   TABLE.  */
static void
add_minsym_to_demangled_hash_table (struct minimal_symbol *sym,
                                  struct minimal_symbol **table)
{
  if (sym->demangled_hash_next == NULL)
    {
      unsigned int hash = msymbol_hash_iw (MSYMBOL_SEARCH_NAME (sym))
	% MINIMAL_SYMBOL_HASH_SIZE;

      sym->demangled_hash_next = table[hash];
      table[hash] = sym;
    }
}

/* Look through all the current minimal symbol tables and find the
   first minimal symbol that matches NAME.  If OBJF is non-NULL, limit
   the search to that objfile.  If SFILE is non-NULL, the only file-scope
   symbols considered will be from that source file (global symbols are
   still preferred).  Returns a pointer to the minimal symbol that
   matches, or NULL if no match is found.

   Note:  One instance where there may be duplicate minimal symbols with
   the same name is when the symbol tables for a shared library and the
   symbol tables for an executable contain global symbols with the same
   names (the dynamic linker deals with the duplication).

   It's also possible to have minimal symbols with different mangled
   names, but identical demangled names.  For example, the GNU C++ v3
   ABI requires the generation of two (or perhaps three) copies of
   constructor functions --- "in-charge", "not-in-charge", and
   "allocate" copies; destructors may be duplicated as well.
   Obviously, there must be distinct mangled names for each of these,
   but the demangled names are all the same: S::S or S::~S.  */

struct bound_minimal_symbol
lookup_minimal_symbol (const char *name, const char *sfile,
		       struct objfile *objf)
{
  struct objfile *objfile;
  struct bound_minimal_symbol found_symbol = { NULL, NULL };
  struct bound_minimal_symbol found_file_symbol = { NULL, NULL };
  struct bound_minimal_symbol trampoline_symbol = { NULL, NULL };

  unsigned int hash = msymbol_hash (name) % MINIMAL_SYMBOL_HASH_SIZE;
  unsigned int dem_hash = msymbol_hash_iw (name) % MINIMAL_SYMBOL_HASH_SIZE;

  const char *modified_name = name;

  if (sfile != NULL)
    sfile = lbasename (sfile);

  /* For C++, canonicalize the input name.  */
  std::string modified_name_storage;
  if (current_language->la_language == language_cplus)
    {
      std::string cname = cp_canonicalize_string (name);
      if (!cname.empty ())
	{
	  std::swap (modified_name_storage, cname);
	  modified_name = modified_name_storage.c_str ();
	}
    }

  for (objfile = object_files;
       objfile != NULL && found_symbol.minsym == NULL;
       objfile = objfile->next)
    {
      struct minimal_symbol *msymbol;

      if (objf == NULL || objf == objfile
	  || objf == objfile->separate_debug_objfile_backlink)
	{
	  /* Do two passes: the first over the ordinary hash table,
	     and the second over the demangled hash table.  */
        int pass;

	if (symbol_lookup_debug)
	  {
	    fprintf_unfiltered (gdb_stdlog,
				"lookup_minimal_symbol (%s, %s, %s)\n",
				name, sfile != NULL ? sfile : "NULL",
				objfile_debug_name (objfile));
	  }

        for (pass = 1; pass <= 2 && found_symbol.minsym == NULL; pass++)
	    {
            /* Select hash list according to pass.  */
            if (pass == 1)
              msymbol = objfile->per_bfd->msymbol_hash[hash];
            else
              msymbol = objfile->per_bfd->msymbol_demangled_hash[dem_hash];

            while (msymbol != NULL && found_symbol.minsym == NULL)
		{
		  int match;

		  if (pass == 1)
		    {
		      int (*cmp) (const char *, const char *);

		      cmp = (case_sensitivity == case_sensitive_on
		             ? strcmp : strcasecmp);
		      match = cmp (MSYMBOL_LINKAGE_NAME (msymbol),
				   modified_name) == 0;
		    }
		  else
		    {
		      /* The function respects CASE_SENSITIVITY.  */
		      match = MSYMBOL_MATCHES_SEARCH_NAME (msymbol,
							  modified_name);
		    }

		  if (match)
		    {
                    switch (MSYMBOL_TYPE (msymbol))
                      {
                      case mst_file_text:
                      case mst_file_data:
                      case mst_file_bss:
                        if (sfile == NULL
			    || filename_cmp (msymbol->filename, sfile) == 0)
			  {
			    found_file_symbol.minsym = msymbol;
			    found_file_symbol.objfile = objfile;
			  }
                        break;

                      case mst_solib_trampoline:

                        /* If a trampoline symbol is found, we prefer to
                           keep looking for the *real* symbol.  If the
                           actual symbol is not found, then we'll use the
                           trampoline entry.  */
                        if (trampoline_symbol.minsym == NULL)
			  {
			    trampoline_symbol.minsym = msymbol;
			    trampoline_symbol.objfile = objfile;
			  }
                        break;

                      case mst_unknown:
                      default:
                        found_symbol.minsym = msymbol;
			found_symbol.objfile = objfile;
                        break;
                      }
		    }

                /* Find the next symbol on the hash chain.  */
                if (pass == 1)
                  msymbol = msymbol->hash_next;
                else
                  msymbol = msymbol->demangled_hash_next;
		}
	    }
	}
    }

  /* External symbols are best.  */
  if (found_symbol.minsym != NULL)
    {
      if (symbol_lookup_debug)
	{
	  fprintf_unfiltered (gdb_stdlog,
			      "lookup_minimal_symbol (...) = %s"
			      " (external)\n",
			      host_address_to_string (found_symbol.minsym));
	}
      return found_symbol;
    }

  /* File-local symbols are next best.  */
  if (found_file_symbol.minsym != NULL)
    {
      if (symbol_lookup_debug)
	{
	  fprintf_unfiltered (gdb_stdlog,
			      "lookup_minimal_symbol (...) = %s"
			      " (file-local)\n",
			      host_address_to_string
			        (found_file_symbol.minsym));
	}
      return found_file_symbol;
    }

  /* Symbols for shared library trampolines are next best.  */
  if (symbol_lookup_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "lookup_minimal_symbol (...) = %s%s\n",
			  trampoline_symbol.minsym != NULL
			  ? host_address_to_string (trampoline_symbol.minsym)
			  : "NULL",
			  trampoline_symbol.minsym != NULL
			  ? " (trampoline)" : "");
    }
  return trampoline_symbol;
}

/* See minsyms.h.  */

struct bound_minimal_symbol
lookup_bound_minimal_symbol (const char *name)
{
  return lookup_minimal_symbol (name, NULL, NULL);
}

/* See common/symbol.h.  */

int
find_minimal_symbol_address (const char *name, CORE_ADDR *addr,
			     struct objfile *objfile)
{
  struct bound_minimal_symbol sym
    = lookup_minimal_symbol (name, NULL, objfile);

  if (sym.minsym != NULL)
    *addr = BMSYMBOL_VALUE_ADDRESS (sym);

  return sym.minsym == NULL;
}

/* See minsyms.h.  */

void
iterate_over_minimal_symbols (struct objfile *objf, const char *name,
			      void (*callback) (struct minimal_symbol *,
						void *),
			      void *user_data)
{
  unsigned int hash;
  struct minimal_symbol *iter;
  int (*cmp) (const char *, const char *);

  /* The first pass is over the ordinary hash table.  */
  hash = msymbol_hash (name) % MINIMAL_SYMBOL_HASH_SIZE;
  iter = objf->per_bfd->msymbol_hash[hash];
  cmp = (case_sensitivity == case_sensitive_on ? strcmp : strcasecmp);
  while (iter)
    {
      if (cmp (MSYMBOL_LINKAGE_NAME (iter), name) == 0)
	(*callback) (iter, user_data);
      iter = iter->hash_next;
    }

  /* The second pass is over the demangled table.  */
  hash = msymbol_hash_iw (name) % MINIMAL_SYMBOL_HASH_SIZE;
  iter = objf->per_bfd->msymbol_demangled_hash[hash];
  while (iter)
    {
      if (MSYMBOL_MATCHES_SEARCH_NAME (iter, name))
	(*callback) (iter, user_data);
      iter = iter->demangled_hash_next;
    }
}

/* See minsyms.h.  */

struct bound_minimal_symbol
lookup_minimal_symbol_text (const char *name, struct objfile *objf)
{
  struct objfile *objfile;
  struct minimal_symbol *msymbol;
  struct bound_minimal_symbol found_symbol = { NULL, NULL };
  struct bound_minimal_symbol found_file_symbol = { NULL, NULL };

  unsigned int hash = msymbol_hash (name) % MINIMAL_SYMBOL_HASH_SIZE;

  for (objfile = object_files;
       objfile != NULL && found_symbol.minsym == NULL;
       objfile = objfile->next)
    {
      if (objf == NULL || objf == objfile
	  || objf == objfile->separate_debug_objfile_backlink)
	{
	  for (msymbol = objfile->per_bfd->msymbol_hash[hash];
	       msymbol != NULL && found_symbol.minsym == NULL;
	       msymbol = msymbol->hash_next)
	    {
	      if (strcmp (MSYMBOL_LINKAGE_NAME (msymbol), name) == 0 &&
		  (MSYMBOL_TYPE (msymbol) == mst_text
		   || MSYMBOL_TYPE (msymbol) == mst_text_gnu_ifunc
		   || MSYMBOL_TYPE (msymbol) == mst_file_text))
		{
		  switch (MSYMBOL_TYPE (msymbol))
		    {
		    case mst_file_text:
		      found_file_symbol.minsym = msymbol;
		      found_file_symbol.objfile = objfile;
		      break;
		    default:
		      found_symbol.minsym = msymbol;
		      found_symbol.objfile = objfile;
		      break;
		    }
		}
	    }
	}
    }
  /* External symbols are best.  */
  if (found_symbol.minsym)
    return found_symbol;

  /* File-local symbols are next best.  */
  return found_file_symbol;
}

/* See minsyms.h.  */

struct minimal_symbol *
lookup_minimal_symbol_by_pc_name (CORE_ADDR pc, const char *name,
				  struct objfile *objf)
{
  struct objfile *objfile;
  struct minimal_symbol *msymbol;

  unsigned int hash = msymbol_hash (name) % MINIMAL_SYMBOL_HASH_SIZE;

  for (objfile = object_files;
       objfile != NULL;
       objfile = objfile->next)
    {
      if (objf == NULL || objf == objfile
	  || objf == objfile->separate_debug_objfile_backlink)
	{
	  for (msymbol = objfile->per_bfd->msymbol_hash[hash];
	       msymbol != NULL;
	       msymbol = msymbol->hash_next)
	    {
	      if (MSYMBOL_VALUE_ADDRESS (objfile, msymbol) == pc
		  && strcmp (MSYMBOL_LINKAGE_NAME (msymbol), name) == 0)
		return msymbol;
	    }
	}
    }

  return NULL;
}

/* See minsyms.h.  */

struct bound_minimal_symbol
lookup_minimal_symbol_solib_trampoline (const char *name,
					struct objfile *objf)
{
  struct objfile *objfile;
  struct minimal_symbol *msymbol;
  struct bound_minimal_symbol found_symbol = { NULL, NULL };

  unsigned int hash = msymbol_hash (name) % MINIMAL_SYMBOL_HASH_SIZE;

  for (objfile = object_files;
       objfile != NULL;
       objfile = objfile->next)
    {
      if (objf == NULL || objf == objfile
	  || objf == objfile->separate_debug_objfile_backlink)
	{
	  for (msymbol = objfile->per_bfd->msymbol_hash[hash];
	       msymbol != NULL;
	       msymbol = msymbol->hash_next)
	    {
	      if (strcmp (MSYMBOL_LINKAGE_NAME (msymbol), name) == 0 &&
		  MSYMBOL_TYPE (msymbol) == mst_solib_trampoline)
		{
		  found_symbol.objfile = objfile;
		  found_symbol.minsym = msymbol;
		  return found_symbol;
		}
	    }
	}
    }

  return found_symbol;
}

/* A helper function that makes *PC section-relative.  This searches
   the sections of OBJFILE and if *PC is in a section, it subtracts
   the section offset and returns true.  Otherwise it returns
   false.  */

static int
frob_address (struct objfile *objfile, CORE_ADDR *pc)
{
  struct obj_section *iter;

  ALL_OBJFILE_OSECTIONS (objfile, iter)
    {
      if (*pc >= obj_section_addr (iter) && *pc < obj_section_endaddr (iter))
	{
	  *pc -= obj_section_offset (iter);
	  return 1;
	}
    }

  return 0;
}

/* Search through the minimal symbol table for each objfile and find
   the symbol whose address is the largest address that is still less
   than or equal to PC, and matches SECTION (which is not NULL).
   Returns a pointer to the minimal symbol if such a symbol is found,
   or NULL if PC is not in a suitable range.
   Note that we need to look through ALL the minimal symbol tables
   before deciding on the symbol that comes closest to the specified PC.
   This is because objfiles can overlap, for example objfile A has .text
   at 0x100 and .data at 0x40000 and objfile B has .text at 0x234 and
   .data at 0x40048.

   If WANT_TRAMPOLINE is set, prefer mst_solib_trampoline symbols when
   there are text and trampoline symbols at the same address.
   Otherwise prefer mst_text symbols.  */

static struct bound_minimal_symbol
lookup_minimal_symbol_by_pc_section_1 (CORE_ADDR pc_in,
				       struct obj_section *section,
				       int want_trampoline)
{
  int lo;
  int hi;
  int newobj;
  struct objfile *objfile;
  struct minimal_symbol *msymbol;
  struct minimal_symbol *best_symbol = NULL;
  struct objfile *best_objfile = NULL;
  struct bound_minimal_symbol result;
  enum minimal_symbol_type want_type, other_type;

  want_type = want_trampoline ? mst_solib_trampoline : mst_text;
  other_type = want_trampoline ? mst_text : mst_solib_trampoline;

  /* We can not require the symbol found to be in section, because
     e.g. IRIX 6.5 mdebug relies on this code returning an absolute
     symbol - but find_pc_section won't return an absolute section and
     hence the code below would skip over absolute symbols.  We can
     still take advantage of the call to find_pc_section, though - the
     object file still must match.  In case we have separate debug
     files, search both the file and its separate debug file.  There's
     no telling which one will have the minimal symbols.  */

  gdb_assert (section != NULL);

  for (objfile = section->objfile;
       objfile != NULL;
       objfile = objfile_separate_debug_iterate (section->objfile, objfile))
    {
      CORE_ADDR pc = pc_in;

      /* If this objfile has a minimal symbol table, go search it using
         a binary search.  Note that a minimal symbol table always consists
         of at least two symbols, a "real" symbol and the terminating
         "null symbol".  If there are no real symbols, then there is no
         minimal symbol table at all.  */

      if (objfile->per_bfd->minimal_symbol_count > 0)
	{
	  int best_zero_sized = -1;

          msymbol = objfile->per_bfd->msymbols;
	  lo = 0;
	  hi = objfile->per_bfd->minimal_symbol_count - 1;

	  /* This code assumes that the minimal symbols are sorted by
	     ascending address values.  If the pc value is greater than or
	     equal to the first symbol's address, then some symbol in this
	     minimal symbol table is a suitable candidate for being the
	     "best" symbol.  This includes the last real symbol, for cases
	     where the pc value is larger than any address in this vector.

	     By iterating until the address associated with the current
	     hi index (the endpoint of the test interval) is less than
	     or equal to the desired pc value, we accomplish two things:
	     (1) the case where the pc value is larger than any minimal
	     symbol address is trivially solved, (2) the address associated
	     with the hi index is always the one we want when the interation
	     terminates.  In essence, we are iterating the test interval
	     down until the pc value is pushed out of it from the high end.

	     Warning: this code is trickier than it would appear at first.  */

	  if (frob_address (objfile, &pc)
	      && pc >= MSYMBOL_VALUE_RAW_ADDRESS (&msymbol[lo]))
	    {
	      while (MSYMBOL_VALUE_RAW_ADDRESS (&msymbol[hi]) > pc)
		{
		  /* pc is still strictly less than highest address.  */
		  /* Note "new" will always be >= lo.  */
		  newobj = (lo + hi) / 2;
		  if ((MSYMBOL_VALUE_RAW_ADDRESS (&msymbol[newobj]) >= pc)
		      || (lo == newobj))
		    {
		      hi = newobj;
		    }
		  else
		    {
		      lo = newobj;
		    }
		}

	      /* If we have multiple symbols at the same address, we want
	         hi to point to the last one.  That way we can find the
	         right symbol if it has an index greater than hi.  */
	      while (hi < objfile->per_bfd->minimal_symbol_count - 1
		     && (MSYMBOL_VALUE_RAW_ADDRESS (&msymbol[hi])
			 == MSYMBOL_VALUE_RAW_ADDRESS (&msymbol[hi + 1])))
		hi++;

	      /* Skip various undesirable symbols.  */
	      while (hi >= 0)
		{
		  /* Skip any absolute symbols.  This is apparently
		     what adb and dbx do, and is needed for the CM-5.
		     There are two known possible problems: (1) on
		     ELF, apparently end, edata, etc. are absolute.
		     Not sure ignoring them here is a big deal, but if
		     we want to use them, the fix would go in
		     elfread.c.  (2) I think shared library entry
		     points on the NeXT are absolute.  If we want
		     special handling for this it probably should be
		     triggered by a special mst_abs_or_lib or some
		     such.  */

		  if (MSYMBOL_TYPE (&msymbol[hi]) == mst_abs)
		    {
		      hi--;
		      continue;
		    }

		  /* If SECTION was specified, skip any symbol from
		     wrong section.  */
		  if (section
		      /* Some types of debug info, such as COFF,
			 don't fill the bfd_section member, so don't
			 throw away symbols on those platforms.  */
		      && MSYMBOL_OBJ_SECTION (objfile, &msymbol[hi]) != NULL
		      && (!matching_obj_sections
			  (MSYMBOL_OBJ_SECTION (objfile, &msymbol[hi]),
			   section)))
		    {
		      hi--;
		      continue;
		    }

		  /* If we are looking for a trampoline and this is a
		     text symbol, or the other way around, check the
		     preceding symbol too.  If they are otherwise
		     identical prefer that one.  */
		  if (hi > 0
		      && MSYMBOL_TYPE (&msymbol[hi]) == other_type
		      && MSYMBOL_TYPE (&msymbol[hi - 1]) == want_type
		      && (MSYMBOL_SIZE (&msymbol[hi])
			  == MSYMBOL_SIZE (&msymbol[hi - 1]))
		      && (MSYMBOL_VALUE_RAW_ADDRESS (&msymbol[hi])
			  == MSYMBOL_VALUE_RAW_ADDRESS (&msymbol[hi - 1]))
		      && (MSYMBOL_OBJ_SECTION (objfile, &msymbol[hi])
			  == MSYMBOL_OBJ_SECTION (objfile, &msymbol[hi - 1])))
		    {
		      hi--;
		      continue;
		    }

		  /* If the minimal symbol has a zero size, save it
		     but keep scanning backwards looking for one with
		     a non-zero size.  A zero size may mean that the
		     symbol isn't an object or function (e.g. a
		     label), or it may just mean that the size was not
		     specified.  */
		  if (MSYMBOL_SIZE (&msymbol[hi]) == 0)
		    {
		      if (best_zero_sized == -1)
			best_zero_sized = hi;
		      hi--;
		      continue;
		    }

		  /* If we are past the end of the current symbol, try
		     the previous symbol if it has a larger overlapping
		     size.  This happens on i686-pc-linux-gnu with glibc;
		     the nocancel variants of system calls are inside
		     the cancellable variants, but both have sizes.  */
		  if (hi > 0
		      && MSYMBOL_SIZE (&msymbol[hi]) != 0
		      && pc >= (MSYMBOL_VALUE_RAW_ADDRESS (&msymbol[hi])
				+ MSYMBOL_SIZE (&msymbol[hi]))
		      && pc < (MSYMBOL_VALUE_RAW_ADDRESS (&msymbol[hi - 1])
			       + MSYMBOL_SIZE (&msymbol[hi - 1])))
		    {
		      hi--;
		      continue;
		    }

		  /* Otherwise, this symbol must be as good as we're going
		     to get.  */
		  break;
		}

	      /* If HI has a zero size, and best_zero_sized is set,
		 then we had two or more zero-sized symbols; prefer
		 the first one we found (which may have a higher
		 address).  Also, if we ran off the end, be sure
		 to back up.  */
	      if (best_zero_sized != -1
		  && (hi < 0 || MSYMBOL_SIZE (&msymbol[hi]) == 0))
		hi = best_zero_sized;

	      /* If the minimal symbol has a non-zero size, and this
		 PC appears to be outside the symbol's contents, then
		 refuse to use this symbol.  If we found a zero-sized
		 symbol with an address greater than this symbol's,
		 use that instead.  We assume that if symbols have
		 specified sizes, they do not overlap.  */

	      if (hi >= 0
		  && MSYMBOL_SIZE (&msymbol[hi]) != 0
		  && pc >= (MSYMBOL_VALUE_RAW_ADDRESS (&msymbol[hi])
			    + MSYMBOL_SIZE (&msymbol[hi])))
		{
		  if (best_zero_sized != -1)
		    hi = best_zero_sized;
		  else
		    /* Go on to the next object file.  */
		    continue;
		}

	      /* The minimal symbol indexed by hi now is the best one in this
	         objfile's minimal symbol table.  See if it is the best one
	         overall.  */

	      if (hi >= 0
		  && ((best_symbol == NULL) ||
		      (MSYMBOL_VALUE_RAW_ADDRESS (best_symbol) <
		       MSYMBOL_VALUE_RAW_ADDRESS (&msymbol[hi]))))
		{
		  best_symbol = &msymbol[hi];
		  best_objfile = objfile;
		}
	    }
	}
    }

  result.minsym = best_symbol;
  result.objfile = best_objfile;
  return result;
}

struct bound_minimal_symbol
lookup_minimal_symbol_by_pc_section (CORE_ADDR pc, struct obj_section *section)
{
  if (section == NULL)
    {
      /* NOTE: cagney/2004-01-27: This was using find_pc_mapped_section to
	 force the section but that (well unless you're doing overlay
	 debugging) always returns NULL making the call somewhat useless.  */
      section = find_pc_section (pc);
      if (section == NULL)
	{
	  struct bound_minimal_symbol result;

	  memset (&result, 0, sizeof (result));
	  return result;
	}
    }
  return lookup_minimal_symbol_by_pc_section_1 (pc, section, 0);
}

/* See minsyms.h.  */

struct bound_minimal_symbol
lookup_minimal_symbol_by_pc (CORE_ADDR pc)
{
  struct obj_section *section = find_pc_section (pc);

  if (section == NULL)
    {
      struct bound_minimal_symbol result;

      memset (&result, 0, sizeof (result));
      return result;
    }
  return lookup_minimal_symbol_by_pc_section_1 (pc, section, 0);
}

/* Return non-zero iff PC is in an STT_GNU_IFUNC function resolver.  */

int
in_gnu_ifunc_stub (CORE_ADDR pc)
{
  struct bound_minimal_symbol msymbol = lookup_minimal_symbol_by_pc (pc);

  return msymbol.minsym && MSYMBOL_TYPE (msymbol.minsym) == mst_text_gnu_ifunc;
}

/* See elf_gnu_ifunc_resolve_addr for its real implementation.  */

static CORE_ADDR
stub_gnu_ifunc_resolve_addr (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  error (_("GDB cannot resolve STT_GNU_IFUNC symbol at address %s without "
	   "the ELF support compiled in."),
	 paddress (gdbarch, pc));
}

/* See elf_gnu_ifunc_resolve_name for its real implementation.  */

static int
stub_gnu_ifunc_resolve_name (const char *function_name,
			     CORE_ADDR *function_address_p)
{
  error (_("GDB cannot resolve STT_GNU_IFUNC symbol \"%s\" without "
	   "the ELF support compiled in."),
	 function_name);
}

/* See elf_gnu_ifunc_resolver_stop for its real implementation.  */

static void
stub_gnu_ifunc_resolver_stop (struct breakpoint *b)
{
  internal_error (__FILE__, __LINE__,
		  _("elf_gnu_ifunc_resolver_stop cannot be reached."));
}

/* See elf_gnu_ifunc_resolver_return_stop for its real implementation.  */

static void
stub_gnu_ifunc_resolver_return_stop (struct breakpoint *b)
{
  internal_error (__FILE__, __LINE__,
		  _("elf_gnu_ifunc_resolver_return_stop cannot be reached."));
}

/* See elf_gnu_ifunc_fns for its real implementation.  */

static const struct gnu_ifunc_fns stub_gnu_ifunc_fns =
{
  stub_gnu_ifunc_resolve_addr,
  stub_gnu_ifunc_resolve_name,
  stub_gnu_ifunc_resolver_stop,
  stub_gnu_ifunc_resolver_return_stop,
};

/* A placeholder for &elf_gnu_ifunc_fns.  */

const struct gnu_ifunc_fns *gnu_ifunc_fns_p = &stub_gnu_ifunc_fns;

/* See minsyms.h.  */

struct bound_minimal_symbol
lookup_minimal_symbol_and_objfile (const char *name)
{
  struct bound_minimal_symbol result;
  struct objfile *objfile;
  unsigned int hash = msymbol_hash (name) % MINIMAL_SYMBOL_HASH_SIZE;

  ALL_OBJFILES (objfile)
    {
      struct minimal_symbol *msym;

      for (msym = objfile->per_bfd->msymbol_hash[hash];
	   msym != NULL;
	   msym = msym->hash_next)
	{
	  if (strcmp (MSYMBOL_LINKAGE_NAME (msym), name) == 0)
	    {
	      result.minsym = msym;
	      result.objfile = objfile;
	      return result;
	    }
	}
    }

  memset (&result, 0, sizeof (result));
  return result;
}


/* Return leading symbol character for a BFD.  If BFD is NULL,
   return the leading symbol character from the main objfile.  */

static int
get_symbol_leading_char (bfd *abfd)
{
  if (abfd != NULL)
    return bfd_get_symbol_leading_char (abfd);
  if (symfile_objfile != NULL && symfile_objfile->obfd != NULL)
    return bfd_get_symbol_leading_char (symfile_objfile->obfd);
  return 0;
}

/* See minsyms.h.  */

minimal_symbol_reader::minimal_symbol_reader (struct objfile *obj)
: m_objfile (obj),
  m_msym_bunch (NULL),
  /* Note that presetting m_msym_bunch_index to BUNCH_SIZE causes the
     first call to save a minimal symbol to allocate the memory for
     the first bunch.  */
  m_msym_bunch_index (BUNCH_SIZE),
  m_msym_count (0)
{
}

/* Discard the currently collected minimal symbols, if any.  If we wish
   to save them for later use, we must have already copied them somewhere
   else before calling this function.

   FIXME:  We could allocate the minimal symbol bunches on their own
   obstack and then simply blow the obstack away when we are done with
   it.  Is it worth the extra trouble though?  */

minimal_symbol_reader::~minimal_symbol_reader ()
{
  struct msym_bunch *next;

  while (m_msym_bunch != NULL)
    {
      next = m_msym_bunch->next;
      xfree (m_msym_bunch);
      m_msym_bunch = next;
    }
}

/* See minsyms.h.  */

void
minimal_symbol_reader::record (const char *name, CORE_ADDR address,
			       enum minimal_symbol_type ms_type)
{
  int section;

  switch (ms_type)
    {
    case mst_text:
    case mst_text_gnu_ifunc:
    case mst_file_text:
    case mst_solib_trampoline:
      section = SECT_OFF_TEXT (m_objfile);
      break;
    case mst_data:
    case mst_file_data:
      section = SECT_OFF_DATA (m_objfile);
      break;
    case mst_bss:
    case mst_file_bss:
      section = SECT_OFF_BSS (m_objfile);
      break;
    default:
      section = -1;
    }

  record_with_info (name, address, ms_type, section);
}

/* See minsyms.h.  */

struct minimal_symbol *
minimal_symbol_reader::record_full (const char *name, int name_len,
				    bool copy_name, CORE_ADDR address,
				    enum minimal_symbol_type ms_type,
				    int section)
{
  struct msym_bunch *newobj;
  struct minimal_symbol *msymbol;

  /* Don't put gcc_compiled, __gnu_compiled_cplus, and friends into
     the minimal symbols, because if there is also another symbol
     at the same address (e.g. the first function of the file),
     lookup_minimal_symbol_by_pc would have no way of getting the
     right one.  */
  if (ms_type == mst_file_text && name[0] == 'g'
      && (strcmp (name, GCC_COMPILED_FLAG_SYMBOL) == 0
	  || strcmp (name, GCC2_COMPILED_FLAG_SYMBOL) == 0))
    return (NULL);

  /* It's safe to strip the leading char here once, since the name
     is also stored stripped in the minimal symbol table.  */
  if (name[0] == get_symbol_leading_char (m_objfile->obfd))
    {
      ++name;
      --name_len;
    }

  if (ms_type == mst_file_text && startswith (name, "__gnu_compiled"))
    return (NULL);

  if (m_msym_bunch_index == BUNCH_SIZE)
    {
      newobj = XCNEW (struct msym_bunch);
      m_msym_bunch_index = 0;
      newobj->next = m_msym_bunch;
      m_msym_bunch = newobj;
    }
  msymbol = &m_msym_bunch->contents[m_msym_bunch_index];
  MSYMBOL_SET_LANGUAGE (msymbol, language_auto,
			&m_objfile->per_bfd->storage_obstack);
  MSYMBOL_SET_NAMES (msymbol, name, name_len, copy_name, m_objfile);

  SET_MSYMBOL_VALUE_ADDRESS (msymbol, address);
  MSYMBOL_SECTION (msymbol) = section;

  MSYMBOL_TYPE (msymbol) = ms_type;
  MSYMBOL_TARGET_FLAG_1 (msymbol) = 0;
  MSYMBOL_TARGET_FLAG_2 (msymbol) = 0;
  /* Do not use the SET_MSYMBOL_SIZE macro to initialize the size,
     as it would also set the has_size flag.  */
  msymbol->size = 0;

  /* The hash pointers must be cleared! If they're not,
     add_minsym_to_hash_table will NOT add this msymbol to the hash table.  */
  msymbol->hash_next = NULL;
  msymbol->demangled_hash_next = NULL;

  /* If we already read minimal symbols for this objfile, then don't
     ever allocate a new one.  */
  if (!m_objfile->per_bfd->minsyms_read)
    {
      m_msym_bunch_index++;
      m_objfile->per_bfd->n_minsyms++;
    }
  m_msym_count++;
  return msymbol;
}

/* Compare two minimal symbols by address and return a signed result based
   on unsigned comparisons, so that we sort into unsigned numeric order.
   Within groups with the same address, sort by name.  */

static int
compare_minimal_symbols (const void *fn1p, const void *fn2p)
{
  const struct minimal_symbol *fn1;
  const struct minimal_symbol *fn2;

  fn1 = (const struct minimal_symbol *) fn1p;
  fn2 = (const struct minimal_symbol *) fn2p;

  if (MSYMBOL_VALUE_RAW_ADDRESS (fn1) < MSYMBOL_VALUE_RAW_ADDRESS (fn2))
    {
      return (-1);		/* addr 1 is less than addr 2.  */
    }
  else if (MSYMBOL_VALUE_RAW_ADDRESS (fn1) > MSYMBOL_VALUE_RAW_ADDRESS (fn2))
    {
      return (1);		/* addr 1 is greater than addr 2.  */
    }
  else
    /* addrs are equal: sort by name */
    {
      const char *name1 = MSYMBOL_LINKAGE_NAME (fn1);
      const char *name2 = MSYMBOL_LINKAGE_NAME (fn2);

      if (name1 && name2)	/* both have names */
	return strcmp (name1, name2);
      else if (name2)
	return 1;		/* fn1 has no name, so it is "less".  */
      else if (name1)		/* fn2 has no name, so it is "less".  */
	return -1;
      else
	return (0);		/* Neither has a name, so they're equal.  */
    }
}

/* Compact duplicate entries out of a minimal symbol table by walking
   through the table and compacting out entries with duplicate addresses
   and matching names.  Return the number of entries remaining.

   On entry, the table resides between msymbol[0] and msymbol[mcount].
   On exit, it resides between msymbol[0] and msymbol[result_count].

   When files contain multiple sources of symbol information, it is
   possible for the minimal symbol table to contain many duplicate entries.
   As an example, SVR4 systems use ELF formatted object files, which
   usually contain at least two different types of symbol tables (a
   standard ELF one and a smaller dynamic linking table), as well as
   DWARF debugging information for files compiled with -g.

   Without compacting, the minimal symbol table for gdb itself contains
   over a 1000 duplicates, about a third of the total table size.  Aside
   from the potential trap of not noticing that two successive entries
   identify the same location, this duplication impacts the time required
   to linearly scan the table, which is done in a number of places.  So we
   just do one linear scan here and toss out the duplicates.

   Note that we are not concerned here about recovering the space that
   is potentially freed up, because the strings themselves are allocated
   on the storage_obstack, and will get automatically freed when the symbol
   table is freed.  The caller can free up the unused minimal symbols at
   the end of the compacted region if their allocation strategy allows it.

   Also note we only go up to the next to last entry within the loop
   and then copy the last entry explicitly after the loop terminates.

   Since the different sources of information for each symbol may
   have different levels of "completeness", we may have duplicates
   that have one entry with type "mst_unknown" and the other with a
   known type.  So if the one we are leaving alone has type mst_unknown,
   overwrite its type with the type from the one we are compacting out.  */

static int
compact_minimal_symbols (struct minimal_symbol *msymbol, int mcount,
			 struct objfile *objfile)
{
  struct minimal_symbol *copyfrom;
  struct minimal_symbol *copyto;

  if (mcount > 0)
    {
      copyfrom = copyto = msymbol;
      while (copyfrom < msymbol + mcount - 1)
	{
	  if (MSYMBOL_VALUE_RAW_ADDRESS (copyfrom)
	      == MSYMBOL_VALUE_RAW_ADDRESS ((copyfrom + 1))
	      && MSYMBOL_SECTION (copyfrom) == MSYMBOL_SECTION (copyfrom + 1)
	      && strcmp (MSYMBOL_LINKAGE_NAME (copyfrom),
			 MSYMBOL_LINKAGE_NAME ((copyfrom + 1))) == 0)
	    {
	      if (MSYMBOL_TYPE ((copyfrom + 1)) == mst_unknown)
		{
		  MSYMBOL_TYPE ((copyfrom + 1)) = MSYMBOL_TYPE (copyfrom);
		}
	      copyfrom++;
	    }
	  else
	    *copyto++ = *copyfrom++;
	}
      *copyto++ = *copyfrom++;
      mcount = copyto - msymbol;
    }
  return (mcount);
}

/* Build (or rebuild) the minimal symbol hash tables.  This is necessary
   after compacting or sorting the table since the entries move around
   thus causing the internal minimal_symbol pointers to become jumbled.  */
  
static void
build_minimal_symbol_hash_tables (struct objfile *objfile)
{
  int i;
  struct minimal_symbol *msym;

  /* Clear the hash tables.  */
  for (i = 0; i < MINIMAL_SYMBOL_HASH_SIZE; i++)
    {
      objfile->per_bfd->msymbol_hash[i] = 0;
      objfile->per_bfd->msymbol_demangled_hash[i] = 0;
    }

  /* Now, (re)insert the actual entries.  */
  for ((i = objfile->per_bfd->minimal_symbol_count,
	msym = objfile->per_bfd->msymbols);
       i > 0;
       i--, msym++)
    {
      msym->hash_next = 0;
      add_minsym_to_hash_table (msym, objfile->per_bfd->msymbol_hash);

      msym->demangled_hash_next = 0;
      if (MSYMBOL_SEARCH_NAME (msym) != MSYMBOL_LINKAGE_NAME (msym))
	add_minsym_to_demangled_hash_table (msym,
                                            objfile->per_bfd->msymbol_demangled_hash);
    }
}

/* Add the minimal symbols in the existing bunches to the objfile's official
   minimal symbol table.  In most cases there is no minimal symbol table yet
   for this objfile, and the existing bunches are used to create one.  Once
   in a while (for shared libraries for example), we add symbols (e.g. common
   symbols) to an existing objfile.

   Because of the way minimal symbols are collected, we generally have no way
   of knowing what source language applies to any particular minimal symbol.
   Specifically, we have no way of knowing if the minimal symbol comes from a
   C++ compilation unit or not.  So for the sake of supporting cached
   demangled C++ names, we have no choice but to try and demangle each new one
   that comes in.  If the demangling succeeds, then we assume it is a C++
   symbol and set the symbol's language and demangled name fields
   appropriately.  Note that in order to avoid unnecessary demanglings, and
   allocating obstack space that subsequently can't be freed for the demangled
   names, we mark all newly added symbols with language_auto.  After
   compaction of the minimal symbols, we go back and scan the entire minimal
   symbol table looking for these new symbols.  For each new symbol we attempt
   to demangle it, and if successful, record it as a language_cplus symbol
   and cache the demangled form on the symbol obstack.  Symbols which don't
   demangle are marked as language_unknown symbols, which inhibits future
   attempts to demangle them if we later add more minimal symbols.  */

void
minimal_symbol_reader::install ()
{
  int bindex;
  int mcount;
  struct msym_bunch *bunch;
  struct minimal_symbol *msymbols;
  int alloc_count;

  if (m_objfile->per_bfd->minsyms_read)
    return;

  if (m_msym_count > 0)
    {
      if (symtab_create_debug)
	{
	  fprintf_unfiltered (gdb_stdlog,
			      "Installing %d minimal symbols of objfile %s.\n",
			      m_msym_count, objfile_name (m_objfile));
	}

      /* Allocate enough space in the obstack, into which we will gather the
         bunches of new and existing minimal symbols, sort them, and then
         compact out the duplicate entries.  Once we have a final table,
         we will give back the excess space.  */

      alloc_count = m_msym_count + m_objfile->per_bfd->minimal_symbol_count + 1;
      obstack_blank (&m_objfile->per_bfd->storage_obstack,
		     alloc_count * sizeof (struct minimal_symbol));
      msymbols = (struct minimal_symbol *)
	obstack_base (&m_objfile->per_bfd->storage_obstack);

      /* Copy in the existing minimal symbols, if there are any.  */

      if (m_objfile->per_bfd->minimal_symbol_count)
	memcpy ((char *) msymbols, (char *) m_objfile->per_bfd->msymbols,
	    m_objfile->per_bfd->minimal_symbol_count * sizeof (struct minimal_symbol));

      /* Walk through the list of minimal symbol bunches, adding each symbol
         to the new contiguous array of symbols.  Note that we start with the
         current, possibly partially filled bunch (thus we use the current
         msym_bunch_index for the first bunch we copy over), and thereafter
         each bunch is full.  */

      mcount = m_objfile->per_bfd->minimal_symbol_count;

      for (bunch = m_msym_bunch; bunch != NULL; bunch = bunch->next)
	{
	  for (bindex = 0; bindex < m_msym_bunch_index; bindex++, mcount++)
	    msymbols[mcount] = bunch->contents[bindex];
	  m_msym_bunch_index = BUNCH_SIZE;
	}

      /* Sort the minimal symbols by address.  */

      qsort (msymbols, mcount, sizeof (struct minimal_symbol),
	     compare_minimal_symbols);

      /* Compact out any duplicates, and free up whatever space we are
         no longer using.  */

      mcount = compact_minimal_symbols (msymbols, mcount, m_objfile);

      obstack_blank_fast (&m_objfile->per_bfd->storage_obstack,
	       (mcount + 1 - alloc_count) * sizeof (struct minimal_symbol));
      msymbols = (struct minimal_symbol *)
	obstack_finish (&m_objfile->per_bfd->storage_obstack);

      /* We also terminate the minimal symbol table with a "null symbol",
         which is *not* included in the size of the table.  This makes it
         easier to find the end of the table when we are handed a pointer
         to some symbol in the middle of it.  Zero out the fields in the
         "null symbol" allocated at the end of the array.  Note that the
         symbol count does *not* include this null symbol, which is why it
         is indexed by mcount and not mcount-1.  */

      memset (&msymbols[mcount], 0, sizeof (struct minimal_symbol));

      /* Attach the minimal symbol table to the specified objfile.
         The strings themselves are also located in the storage_obstack
         of this objfile.  */

      m_objfile->per_bfd->minimal_symbol_count = mcount;
      m_objfile->per_bfd->msymbols = msymbols;

      /* Now build the hash tables; we can't do this incrementally
         at an earlier point since we weren't finished with the obstack
	 yet.  (And if the msymbol obstack gets moved, all the internal
	 pointers to other msymbols need to be adjusted.)  */
      build_minimal_symbol_hash_tables (m_objfile);
    }
}

/* See minsyms.h.  */

void
terminate_minimal_symbol_table (struct objfile *objfile)
{
  if (! objfile->per_bfd->msymbols)
    objfile->per_bfd->msymbols
      = ((struct minimal_symbol *)
	 obstack_alloc (&objfile->per_bfd->storage_obstack,
			sizeof (struct minimal_symbol)));

  {
    struct minimal_symbol *m
      = &objfile->per_bfd->msymbols[objfile->per_bfd->minimal_symbol_count];

    memset (m, 0, sizeof (*m));
    /* Don't rely on these enumeration values being 0's.  */
    MSYMBOL_TYPE (m) = mst_unknown;
    MSYMBOL_SET_LANGUAGE (m, language_unknown,
			  &objfile->per_bfd->storage_obstack);
  }
}

/* Check if PC is in a shared library trampoline code stub.
   Return minimal symbol for the trampoline entry or NULL if PC is not
   in a trampoline code stub.  */

static struct minimal_symbol *
lookup_solib_trampoline_symbol_by_pc (CORE_ADDR pc)
{
  struct obj_section *section = find_pc_section (pc);
  struct bound_minimal_symbol msymbol;

  if (section == NULL)
    return NULL;
  msymbol = lookup_minimal_symbol_by_pc_section_1 (pc, section, 1);

  if (msymbol.minsym != NULL
      && MSYMBOL_TYPE (msymbol.minsym) == mst_solib_trampoline)
    return msymbol.minsym;
  return NULL;
}

/* If PC is in a shared library trampoline code stub, return the
   address of the `real' function belonging to the stub.
   Return 0 if PC is not in a trampoline code stub or if the real
   function is not found in the minimal symbol table.

   We may fail to find the right function if a function with the
   same name is defined in more than one shared library, but this
   is considered bad programming style.  We could return 0 if we find
   a duplicate function in case this matters someday.  */

CORE_ADDR
find_solib_trampoline_target (struct frame_info *frame, CORE_ADDR pc)
{
  struct objfile *objfile;
  struct minimal_symbol *msymbol;
  struct minimal_symbol *tsymbol = lookup_solib_trampoline_symbol_by_pc (pc);

  if (tsymbol != NULL)
    {
      ALL_MSYMBOLS (objfile, msymbol)
      {
	if ((MSYMBOL_TYPE (msymbol) == mst_text
	    || MSYMBOL_TYPE (msymbol) == mst_text_gnu_ifunc)
	    && strcmp (MSYMBOL_LINKAGE_NAME (msymbol),
		       MSYMBOL_LINKAGE_NAME (tsymbol)) == 0)
	  return MSYMBOL_VALUE_ADDRESS (objfile, msymbol);

	/* Also handle minimal symbols pointing to function descriptors.  */
	if (MSYMBOL_TYPE (msymbol) == mst_data
	    && strcmp (MSYMBOL_LINKAGE_NAME (msymbol),
		       MSYMBOL_LINKAGE_NAME (tsymbol)) == 0)
	  {
	    CORE_ADDR func;

	    func = gdbarch_convert_from_func_ptr_addr
		    (get_objfile_arch (objfile),
		     MSYMBOL_VALUE_ADDRESS (objfile, msymbol),
		     &current_target);

	    /* Ignore data symbols that are not function descriptors.  */
	    if (func != MSYMBOL_VALUE_ADDRESS (objfile, msymbol))
	      return func;
	  }
      }
    }
  return 0;
}

/* See minsyms.h.  */

CORE_ADDR
minimal_symbol_upper_bound (struct bound_minimal_symbol minsym)
{
  int i;
  short section;
  struct obj_section *obj_section;
  CORE_ADDR result;
  struct minimal_symbol *msymbol;

  gdb_assert (minsym.minsym != NULL);

  /* If the minimal symbol has a size, use it.  Otherwise use the
     lesser of the next minimal symbol in the same section, or the end
     of the section, as the end of the function.  */

  if (MSYMBOL_SIZE (minsym.minsym) != 0)
    return BMSYMBOL_VALUE_ADDRESS (minsym) + MSYMBOL_SIZE (minsym.minsym);

  /* Step over other symbols at this same address, and symbols in
     other sections, to find the next symbol in this section with a
     different address.  */

  msymbol = minsym.minsym;
  section = MSYMBOL_SECTION (msymbol);
  for (i = 1; MSYMBOL_LINKAGE_NAME (msymbol + i) != NULL; i++)
    {
      if ((MSYMBOL_VALUE_RAW_ADDRESS (msymbol + i)
	   != MSYMBOL_VALUE_RAW_ADDRESS (msymbol))
	  && MSYMBOL_SECTION (msymbol + i) == section)
	break;
    }

  obj_section = MSYMBOL_OBJ_SECTION (minsym.objfile, minsym.minsym);
  if (MSYMBOL_LINKAGE_NAME (msymbol + i) != NULL
      && (MSYMBOL_VALUE_ADDRESS (minsym.objfile, msymbol + i)
	  < obj_section_endaddr (obj_section)))
    result = MSYMBOL_VALUE_ADDRESS (minsym.objfile, msymbol + i);
  else
    /* We got the start address from the last msymbol in the objfile.
       So the end address is the end of the section.  */
    result = obj_section_endaddr (obj_section);

  return result;
}
