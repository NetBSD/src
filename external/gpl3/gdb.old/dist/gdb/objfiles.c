/* GDB routines for manipulating objfiles.

   Copyright (C) 1992-2020 Free Software Foundation, Inc.

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
   destroying objfile structures.  */

#include "defs.h"
#include "bfd.h"		/* Binary File Description */
#include "symtab.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdb-stabs.h"
#include "target.h"
#include "bcache.h"
#include "expression.h"
#include "parser-defs.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "gdb_obstack.h"
#include "hashtab.h"

#include "breakpoint.h"
#include "block.h"
#include "dictionary.h"
#include "source.h"
#include "addrmap.h"
#include "arch-utils.h"
#include "exec.h"
#include "observable.h"
#include "complaints.h"
#include "psymtab.h"
#include "solist.h"
#include "gdb_bfd.h"
#include "btrace.h"
#include "gdbsupport/pathstuff.h"

#include <algorithm>
#include <vector>

/* Keep a registry of per-objfile data-pointers required by other GDB
   modules.  */

DEFINE_REGISTRY (objfile, REGISTRY_ACCESS_FIELD)

/* Externally visible variables that are owned by this module.
   See declarations in objfile.h for more info.  */

struct objfile_pspace_info
{
  objfile_pspace_info () = default;
  ~objfile_pspace_info ();

  struct obj_section **sections = nullptr;
  int num_sections = 0;

  /* Nonzero if object files have been added since the section map
     was last updated.  */
  int new_objfiles_available = 0;

  /* Nonzero if the section map MUST be updated before use.  */
  int section_map_dirty = 0;

  /* Nonzero if section map updates should be inhibited if possible.  */
  int inhibit_updates = 0;
};

/* Per-program-space data key.  */
static const struct program_space_key<objfile_pspace_info>
  objfiles_pspace_data;

objfile_pspace_info::~objfile_pspace_info ()
{
  xfree (sections);
}

/* Get the current svr4 data.  If none is found yet, add it now.  This
   function always returns a valid object.  */

static struct objfile_pspace_info *
get_objfile_pspace_data (struct program_space *pspace)
{
  struct objfile_pspace_info *info;

  info = objfiles_pspace_data.get (pspace);
  if (info == NULL)
    info = objfiles_pspace_data.emplace (pspace);

  return info;
}



/* Per-BFD data key.  */

static const struct bfd_key<objfile_per_bfd_storage> objfiles_bfd_data;

objfile_per_bfd_storage::~objfile_per_bfd_storage ()
{
}

/* Create the per-BFD storage object for OBJFILE.  If ABFD is not
   NULL, and it already has a per-BFD storage object, use that.
   Otherwise, allocate a new per-BFD storage object.  Note that it is
   not safe to call this multiple times for a given OBJFILE -- it can
   only be called when allocating or re-initializing OBJFILE.  */

static struct objfile_per_bfd_storage *
get_objfile_bfd_data (struct objfile *objfile, struct bfd *abfd)
{
  struct objfile_per_bfd_storage *storage = NULL;

  if (abfd != NULL)
    storage = objfiles_bfd_data.get (abfd);

  if (storage == NULL)
    {
      storage = new objfile_per_bfd_storage;
      /* If the object requires gdb to do relocations, we simply fall
	 back to not sharing data across users.  These cases are rare
	 enough that this seems reasonable.  */
      if (abfd != NULL && !gdb_bfd_requires_relocations (abfd))
	objfiles_bfd_data.set (abfd, storage);

      /* Look up the gdbarch associated with the BFD.  */
      if (abfd != NULL)
	storage->gdbarch = gdbarch_from_bfd (abfd);
    }

  return storage;
}

/* See objfiles.h.  */

void
set_objfile_per_bfd (struct objfile *objfile)
{
  objfile->per_bfd = get_objfile_bfd_data (objfile, objfile->obfd);
}

/* Set the objfile's per-BFD notion of the "main" name and
   language.  */

void
set_objfile_main_name (struct objfile *objfile,
		       const char *name, enum language lang)
{
  if (objfile->per_bfd->name_of_main == NULL
      || strcmp (objfile->per_bfd->name_of_main, name) != 0)
    objfile->per_bfd->name_of_main
      = obstack_strdup (&objfile->per_bfd->storage_obstack, name);
  objfile->per_bfd->language_of_main = lang;
}

/* Helper structure to map blocks to static link properties in hash tables.  */

struct static_link_htab_entry
{
  const struct block *block;
  const struct dynamic_prop *static_link;
};

/* Return a hash code for struct static_link_htab_entry *P.  */

static hashval_t
static_link_htab_entry_hash (const void *p)
{
  const struct static_link_htab_entry *e
    = (const struct static_link_htab_entry *) p;

  return htab_hash_pointer (e->block);
}

/* Return whether P1 an P2 (pointers to struct static_link_htab_entry) are
   mappings for the same block.  */

static int
static_link_htab_entry_eq (const void *p1, const void *p2)
{
  const struct static_link_htab_entry *e1
    = (const struct static_link_htab_entry *) p1;
  const struct static_link_htab_entry *e2
    = (const struct static_link_htab_entry *) p2;

  return e1->block == e2->block;
}

/* Register STATIC_LINK as the static link for BLOCK, which is part of OBJFILE.
   Must not be called more than once for each BLOCK.  */

void
objfile_register_static_link (struct objfile *objfile,
			      const struct block *block,
			      const struct dynamic_prop *static_link)
{
  void **slot;
  struct static_link_htab_entry lookup_entry;
  struct static_link_htab_entry *entry;

  if (objfile->static_links == NULL)
    objfile->static_links.reset (htab_create_alloc
      (1, &static_link_htab_entry_hash, static_link_htab_entry_eq, NULL,
       xcalloc, xfree));

  /* Create a slot for the mapping, make sure it's the first mapping for this
     block and then create the mapping itself.  */
  lookup_entry.block = block;
  slot = htab_find_slot (objfile->static_links.get (), &lookup_entry, INSERT);
  gdb_assert (*slot == NULL);

  entry = XOBNEW (&objfile->objfile_obstack, static_link_htab_entry);
  entry->block = block;
  entry->static_link = static_link;
  *slot = (void *) entry;
}

/* Look for a static link for BLOCK, which is part of OBJFILE.  Return NULL if
   none was found.  */

const struct dynamic_prop *
objfile_lookup_static_link (struct objfile *objfile,
			    const struct block *block)
{
  struct static_link_htab_entry *entry;
  struct static_link_htab_entry lookup_entry;

  if (objfile->static_links == NULL)
    return NULL;
  lookup_entry.block = block;
  entry = ((struct static_link_htab_entry *)
	   htab_find (objfile->static_links.get (), &lookup_entry));
  if (entry == NULL)
    return NULL;

  gdb_assert (entry->block == block);
  return entry->static_link;
}



/* Called via bfd_map_over_sections to build up the section table that
   the objfile references.  The objfile contains pointers to the start
   of the table (objfile->sections) and to the first location after
   the end of the table (objfile->sections_end).  */

static void
add_to_objfile_sections_full (struct bfd *abfd, struct bfd_section *asect,
			      struct objfile *objfile, int force)
{
  struct obj_section *section;

  if (!force)
    {
      flagword aflag;

      aflag = bfd_section_flags (asect);
      if (!(aflag & SEC_ALLOC))
	return;
    }

  section = &objfile->sections[gdb_bfd_section_index (abfd, asect)];
  section->objfile = objfile;
  section->the_bfd_section = asect;
  section->ovly_mapped = 0;
}

static void
add_to_objfile_sections (struct bfd *abfd, struct bfd_section *asect,
			 void *objfilep)
{
  add_to_objfile_sections_full (abfd, asect, (struct objfile *) objfilep, 0);
}

/* Builds a section table for OBJFILE.

   Note that the OFFSET and OVLY_MAPPED in each table entry are
   initialized to zero.  */

void
build_objfile_section_table (struct objfile *objfile)
{
  int count = gdb_bfd_count_sections (objfile->obfd);

  objfile->sections = OBSTACK_CALLOC (&objfile->objfile_obstack,
				      count,
				      struct obj_section);
  objfile->sections_end = (objfile->sections + count);
  bfd_map_over_sections (objfile->obfd,
			 add_to_objfile_sections, (void *) objfile);

  /* See gdb_bfd_section_index.  */
  add_to_objfile_sections_full (objfile->obfd, bfd_com_section_ptr, objfile, 1);
  add_to_objfile_sections_full (objfile->obfd, bfd_und_section_ptr, objfile, 1);
  add_to_objfile_sections_full (objfile->obfd, bfd_abs_section_ptr, objfile, 1);
  add_to_objfile_sections_full (objfile->obfd, bfd_ind_section_ptr, objfile, 1);
}

/* Given a pointer to an initialized bfd (ABFD) and some flag bits,
   initialize the new objfile as best we can and link it into the list
   of all known objfiles.

   NAME should contain original non-canonicalized filename or other
   identifier as entered by user.  If there is no better source use
   bfd_get_filename (ABFD).  NAME may be NULL only if ABFD is NULL.
   NAME content is copied into returned objfile.

   The FLAGS word contains various bits (OBJF_*) that can be taken as
   requests for specific operations.  Other bits like OBJF_SHARED are
   simply copied through to the new objfile flags member.  */

objfile::objfile (bfd *abfd, const char *name, objfile_flags flags_)
  : flags (flags_),
    pspace (current_program_space),
    partial_symtabs (new psymtab_storage ()),
    obfd (abfd)
{
  const char *expanded_name;

  /* We could use obstack_specify_allocation here instead, but
     gdb_obstack.h specifies the alloc/dealloc functions.  */
  obstack_init (&objfile_obstack);

  objfile_alloc_data (this);

  gdb::unique_xmalloc_ptr<char> name_holder;
  if (name == NULL)
    {
      gdb_assert (abfd == NULL);
      gdb_assert ((flags & OBJF_NOT_FILENAME) != 0);
      expanded_name = "<<anonymous objfile>>";
    }
  else if ((flags & OBJF_NOT_FILENAME) != 0
	   || is_target_filename (name))
    expanded_name = name;
  else
    {
      name_holder = gdb_abspath (name);
      expanded_name = name_holder.get ();
    }
  original_name = obstack_strdup (&objfile_obstack, expanded_name);

  /* Update the per-objfile information that comes from the bfd, ensuring
     that any data that is reference is saved in the per-objfile data
     region.  */

  gdb_bfd_ref (abfd);
  if (abfd != NULL)
    {
      mtime = bfd_get_mtime (abfd);

      /* Build section table.  */
      build_objfile_section_table (this);
    }

  per_bfd = get_objfile_bfd_data (this, abfd);
}

/* If there is a valid and known entry point, function fills *ENTRY_P with it
   and returns non-zero; otherwise it returns zero.  */

int
entry_point_address_query (CORE_ADDR *entry_p)
{
  if (symfile_objfile == NULL || !symfile_objfile->per_bfd->ei.entry_point_p)
    return 0;

  int idx = symfile_objfile->per_bfd->ei.the_bfd_section_index;
  *entry_p = (symfile_objfile->per_bfd->ei.entry_point
	      + symfile_objfile->section_offsets[idx]);

  return 1;
}

/* Get current entry point address.  Call error if it is not known.  */

CORE_ADDR
entry_point_address (void)
{
  CORE_ADDR retval;

  if (!entry_point_address_query (&retval))
    error (_("Entry point address is not known."));

  return retval;
}

separate_debug_iterator &
separate_debug_iterator::operator++ ()
{
  gdb_assert (m_objfile != nullptr);

  struct objfile *res;

  /* If any, return the first child.  */
  res = m_objfile->separate_debug_objfile;
  if (res != nullptr)
    {
      m_objfile = res;
      return *this;
    }

  /* Common case where there is no separate debug objfile.  */
  if (m_objfile == m_parent)
    {
      m_objfile = nullptr;
      return *this;
    }

  /* Return the brother if any.  Note that we don't iterate on brothers of
     the parents.  */
  res = m_objfile->separate_debug_objfile_link;
  if (res != nullptr)
    {
      m_objfile = res;
      return *this;
    }

  for (res = m_objfile->separate_debug_objfile_backlink;
       res != m_parent;
       res = res->separate_debug_objfile_backlink)
    {
      gdb_assert (res != nullptr);
      if (res->separate_debug_objfile_link != nullptr)
	{
	  m_objfile = res->separate_debug_objfile_link;
	  return *this;
	}
    }
  m_objfile = nullptr;
  return *this;
}

/* Add OBJFILE as a separate debug objfile of PARENT.  */

static void
add_separate_debug_objfile (struct objfile *objfile, struct objfile *parent)
{
  gdb_assert (objfile && parent);

  /* Must not be already in a list.  */
  gdb_assert (objfile->separate_debug_objfile_backlink == NULL);
  gdb_assert (objfile->separate_debug_objfile_link == NULL);
  gdb_assert (objfile->separate_debug_objfile == NULL);
  gdb_assert (parent->separate_debug_objfile_backlink == NULL);
  gdb_assert (parent->separate_debug_objfile_link == NULL);

  objfile->separate_debug_objfile_backlink = parent;
  objfile->separate_debug_objfile_link = parent->separate_debug_objfile;
  parent->separate_debug_objfile = objfile;
}

/* See objfiles.h.  */

objfile *
objfile::make (bfd *bfd_, const char *name_, objfile_flags flags_,
	       objfile *parent)
{
  objfile *result = new objfile (bfd_, name_, flags_);
  if (parent != nullptr)
    add_separate_debug_objfile (result, parent);

  /* Using std::make_shared might be a bit nicer here, but that would
     require making the constructor public.  */
  current_program_space->add_objfile (std::shared_ptr<objfile> (result),
				      parent);

  /* Rebuild section map next time we need it.  */
  get_objfile_pspace_data (current_program_space)->new_objfiles_available = 1;

  return result;
}

/* See objfiles.h.  */

void
objfile::unlink ()
{
  current_program_space->remove_objfile (this);
}

/* Free all separate debug objfile of OBJFILE, but don't free OBJFILE
   itself.  */

void
free_objfile_separate_debug (struct objfile *objfile)
{
  struct objfile *child;

  for (child = objfile->separate_debug_objfile; child;)
    {
      struct objfile *next_child = child->separate_debug_objfile_link;
      child->unlink ();
      child = next_child;
    }
}

/* Destroy an objfile and all the symtabs and psymtabs under it.  */

objfile::~objfile ()
{
  /* First notify observers that this objfile is about to be freed.  */
  gdb::observers::free_objfile.notify (this);

  /* Free all separate debug objfiles.  */
  free_objfile_separate_debug (this);

  if (separate_debug_objfile_backlink)
    {
      /* We freed the separate debug file, make sure the base objfile
	 doesn't reference it.  */
      struct objfile *child;

      child = separate_debug_objfile_backlink->separate_debug_objfile;

      if (child == this)
        {
          /* THIS is the first child.  */
          separate_debug_objfile_backlink->separate_debug_objfile =
            separate_debug_objfile_link;
        }
      else
        {
          /* Find THIS in the list.  */
          while (1)
            {
              if (child->separate_debug_objfile_link == this)
                {
                  child->separate_debug_objfile_link =
                    separate_debug_objfile_link;
                  break;
                }
              child = child->separate_debug_objfile_link;
              gdb_assert (child);
            }
        }
    }

  /* Remove any references to this objfile in the global value
     lists.  */
  preserve_values (this);

  /* It still may reference data modules have associated with the objfile and
     the symbol file data.  */
  forget_cached_source_info_for_objfile (this);

  breakpoint_free_objfile (this);
  btrace_free_objfile (this);

  /* First do any symbol file specific actions required when we are
     finished with a particular symbol file.  Note that if the objfile
     is using reusable symbol information (via mmalloc) then each of
     these routines is responsible for doing the correct thing, either
     freeing things which are valid only during this particular gdb
     execution, or leaving them to be reused during the next one.  */

  if (sf != NULL)
    (*sf->sym_finish) (this);

  /* Discard any data modules have associated with the objfile.  The function
     still may reference obfd.  */
  objfile_free_data (this);

  if (obfd)
    gdb_bfd_unref (obfd);
  else
    delete per_bfd;

  /* Before the symbol table code was redone to make it easier to
     selectively load and remove information particular to a specific
     linkage unit, gdb used to do these things whenever the monolithic
     symbol table was blown away.  How much still needs to be done
     is unknown, but we play it safe for now and keep each action until
     it is shown to be no longer needed.  */

  /* Not all our callers call clear_symtab_users (objfile_purge_solibs,
     for example), so we need to call this here.  */
  clear_pc_function_cache ();

  /* Check to see if the current_source_symtab belongs to this objfile,
     and if so, call clear_current_source_symtab_and_line.  */

  {
    struct symtab_and_line cursal = get_current_source_symtab_and_line ();

    if (cursal.symtab && SYMTAB_OBJFILE (cursal.symtab) == this)
      clear_current_source_symtab_and_line ();
  }

  /* Free the obstacks for non-reusable objfiles.  */
  obstack_free (&objfile_obstack, 0);

  /* Rebuild section map next time we need it.  */
  get_objfile_pspace_data (pspace)->section_map_dirty = 1;
}


/* A helper function for objfile_relocate1 that relocates a single
   symbol.  */

static void
relocate_one_symbol (struct symbol *sym, struct objfile *objfile,
		     const section_offsets &delta)
{
  fixup_symbol_section (sym, objfile);

  /* The RS6000 code from which this was taken skipped
     any symbols in STRUCT_DOMAIN or UNDEF_DOMAIN.
     But I'm leaving out that test, on the theory that
     they can't possibly pass the tests below.  */
  if ((SYMBOL_CLASS (sym) == LOC_LABEL
       || SYMBOL_CLASS (sym) == LOC_STATIC)
      && SYMBOL_SECTION (sym) >= 0)
    {
      SET_SYMBOL_VALUE_ADDRESS (sym,
				SYMBOL_VALUE_ADDRESS (sym)
				+ delta[SYMBOL_SECTION (sym)]);
    }
}

/* Relocate OBJFILE to NEW_OFFSETS.  There should be OBJFILE->NUM_SECTIONS
   entries in new_offsets.  SEPARATE_DEBUG_OBJFILE is not touched here.
   Return non-zero iff any change happened.  */

static int
objfile_relocate1 (struct objfile *objfile, 
		   const section_offsets &new_offsets)
{
  section_offsets delta (objfile->section_offsets.size ());

  int something_changed = 0;

  for (int i = 0; i < objfile->section_offsets.size (); ++i)
    {
      delta[i] = new_offsets[i] - objfile->section_offsets[i];
      if (delta[i] != 0)
	something_changed = 1;
    }
  if (!something_changed)
    return 0;

  /* OK, get all the symtabs.  */
  {
    for (compunit_symtab *cust : objfile->compunits ())
      {
	for (symtab *s : compunit_filetabs (cust))
	  {
	    struct linetable *l;

	    /* First the line table.  */
	    l = SYMTAB_LINETABLE (s);
	    if (l)
	      {
		for (int i = 0; i < l->nitems; ++i)
		  l->item[i].pc += delta[COMPUNIT_BLOCK_LINE_SECTION (cust)];
	      }
	  }
      }

    for (compunit_symtab *cust : objfile->compunits ())
      {
	const struct blockvector *bv = COMPUNIT_BLOCKVECTOR (cust);
	int block_line_section = COMPUNIT_BLOCK_LINE_SECTION (cust);

	if (BLOCKVECTOR_MAP (bv))
	  addrmap_relocate (BLOCKVECTOR_MAP (bv), delta[block_line_section]);

	for (int i = 0; i < BLOCKVECTOR_NBLOCKS (bv); ++i)
	  {
	    struct block *b;
	    struct symbol *sym;
	    struct mdict_iterator miter;

	    b = BLOCKVECTOR_BLOCK (bv, i);
	    BLOCK_START (b) += delta[block_line_section];
	    BLOCK_END (b) += delta[block_line_section];

	    if (BLOCK_RANGES (b) != nullptr)
	      for (int j = 0; j < BLOCK_NRANGES (b); j++)
		{
		  BLOCK_RANGE_START (b, j) += delta[block_line_section];
		  BLOCK_RANGE_END (b, j) += delta[block_line_section];
		}

	    /* We only want to iterate over the local symbols, not any
	       symbols in included symtabs.  */
	    ALL_DICT_SYMBOLS (BLOCK_MULTIDICT (b), miter, sym)
	      {
		relocate_one_symbol (sym, objfile, delta);
	      }
	  }
      }
  }

  /* This stores relocated addresses and so must be cleared.  This
     will cause it to be recreated on demand.  */
  objfile->psymbol_map.clear ();

  /* Relocate isolated symbols.  */
  {
    struct symbol *iter;

    for (iter = objfile->template_symbols; iter; iter = iter->hash_next)
      relocate_one_symbol (iter, objfile, delta);
  }

  {
    int i;

    for (i = 0; i < objfile->section_offsets.size (); ++i)
      objfile->section_offsets[i] = new_offsets[i];
  }

  /* Rebuild section map next time we need it.  */
  get_objfile_pspace_data (objfile->pspace)->section_map_dirty = 1;

  /* Update the table in exec_ops, used to read memory.  */
  struct obj_section *s;
  ALL_OBJFILE_OSECTIONS (objfile, s)
    {
      int idx = s - objfile->sections;

      exec_set_section_address (bfd_get_filename (objfile->obfd), idx,
				obj_section_addr (s));
    }

  /* Data changed.  */
  return 1;
}

/* Relocate OBJFILE to NEW_OFFSETS.  There should be OBJFILE->NUM_SECTIONS
   entries in new_offsets.  Process also OBJFILE's SEPARATE_DEBUG_OBJFILEs.

   The number and ordering of sections does differ between the two objfiles.
   Only their names match.  Also the file offsets will differ (objfile being
   possibly prelinked but separate_debug_objfile is probably not prelinked) but
   the in-memory absolute address as specified by NEW_OFFSETS must match both
   files.  */

void
objfile_relocate (struct objfile *objfile,
		  const section_offsets &new_offsets)
{
  int changed = 0;

  changed |= objfile_relocate1 (objfile, new_offsets);

  for (::objfile *debug_objfile : objfile->separate_debug_objfiles ())
    {
      if (debug_objfile == objfile)
	continue;

      section_addr_info objfile_addrs
	= build_section_addr_info_from_objfile (objfile);

      /* Here OBJFILE_ADDRS contain the correct absolute addresses, the
	 relative ones must be already created according to debug_objfile.  */

      addr_info_make_relative (&objfile_addrs, debug_objfile->obfd);

      gdb_assert (debug_objfile->section_offsets.size ()
		  == gdb_bfd_count_sections (debug_objfile->obfd));
      section_offsets new_debug_offsets
	(debug_objfile->section_offsets.size ());
      relative_addr_info_to_section_offsets (new_debug_offsets, objfile_addrs);

      changed |= objfile_relocate1 (debug_objfile, new_debug_offsets);
    }

  /* Relocate breakpoints as necessary, after things are relocated.  */
  if (changed)
    breakpoint_re_set ();
}

/* Rebase (add to the offsets) OBJFILE by SLIDE.  SEPARATE_DEBUG_OBJFILE is
   not touched here.
   Return non-zero iff any change happened.  */

static int
objfile_rebase1 (struct objfile *objfile, CORE_ADDR slide)
{
  section_offsets new_offsets (objfile->section_offsets.size (), slide);
  return objfile_relocate1 (objfile, new_offsets);
}

/* Rebase (add to the offsets) OBJFILE by SLIDE.  Process also OBJFILE's
   SEPARATE_DEBUG_OBJFILEs.  */

void
objfile_rebase (struct objfile *objfile, CORE_ADDR slide)
{
  int changed = 0;

  for (::objfile *debug_objfile : objfile->separate_debug_objfiles ())
    changed |= objfile_rebase1 (debug_objfile, slide);

  /* Relocate breakpoints as necessary, after things are relocated.  */
  if (changed)
    breakpoint_re_set ();
}

/* Return non-zero if OBJFILE has partial symbols.  */

int
objfile_has_partial_symbols (struct objfile *objfile)
{
  if (!objfile->sf)
    return 0;

  /* If we have not read psymbols, but we have a function capable of reading
     them, then that is an indication that they are in fact available.  Without
     this function the symbols may have been already read in but they also may
     not be present in this objfile.  */
  if ((objfile->flags & OBJF_PSYMTABS_READ) == 0
      && objfile->sf->sym_read_psymbols != NULL)
    return 1;

  return objfile->sf->qf->has_symbols (objfile);
}

/* Return non-zero if OBJFILE has full symbols.  */

int
objfile_has_full_symbols (struct objfile *objfile)
{
  return objfile->compunit_symtabs != NULL;
}

/* Return non-zero if OBJFILE has full or partial symbols, either directly
   or through a separate debug file.  */

int
objfile_has_symbols (struct objfile *objfile)
{
  for (::objfile *o : objfile->separate_debug_objfiles ())
    if (objfile_has_partial_symbols (o) || objfile_has_full_symbols (o))
      return 1;
  return 0;
}


/* Many places in gdb want to test just to see if we have any partial
   symbols available.  This function returns zero if none are currently
   available, nonzero otherwise.  */

int
have_partial_symbols (void)
{
  for (objfile *ofp : current_program_space->objfiles ())
    {
      if (objfile_has_partial_symbols (ofp))
	return 1;
    }
  return 0;
}

/* Many places in gdb want to test just to see if we have any full
   symbols available.  This function returns zero if none are currently
   available, nonzero otherwise.  */

int
have_full_symbols (void)
{
  for (objfile *ofp : current_program_space->objfiles ())
    {
      if (objfile_has_full_symbols (ofp))
	return 1;
    }
  return 0;
}


/* This operations deletes all objfile entries that represent solibs that
   weren't explicitly loaded by the user, via e.g., the add-symbol-file
   command.  */

void
objfile_purge_solibs (void)
{
  for (objfile *objf : current_program_space->objfiles_safe ())
    {
      /* We assume that the solib package has been purged already, or will
	 be soon.  */

      if (!(objf->flags & OBJF_USERLOADED) && (objf->flags & OBJF_SHARED))
	objf->unlink ();
    }
}


/* Many places in gdb want to test just to see if we have any minimal
   symbols available.  This function returns zero if none are currently
   available, nonzero otherwise.  */

int
have_minimal_symbols (void)
{
  for (objfile *ofp : current_program_space->objfiles ())
    {
      if (ofp->per_bfd->minimal_symbol_count > 0)
	{
	  return 1;
	}
    }
  return 0;
}

/* Qsort comparison function.  */

static bool
sort_cmp (const struct obj_section *sect1, const obj_section *sect2)
{
  const CORE_ADDR sect1_addr = obj_section_addr (sect1);
  const CORE_ADDR sect2_addr = obj_section_addr (sect2);

  if (sect1_addr < sect2_addr)
    return true;
  else if (sect1_addr > sect2_addr)
    return false;
  else
    {
      /* Sections are at the same address.  This could happen if
	 A) we have an objfile and a separate debuginfo.
	 B) we are confused, and have added sections without proper relocation,
	 or something like that.  */

      const struct objfile *const objfile1 = sect1->objfile;
      const struct objfile *const objfile2 = sect2->objfile;

      if (objfile1->separate_debug_objfile == objfile2
	  || objfile2->separate_debug_objfile == objfile1)
	{
	  /* Case A.  The ordering doesn't matter: separate debuginfo files
	     will be filtered out later.  */

	  return false;
	}

      /* Case B.  Maintain stable sort order, so bugs in GDB are easier to
	 triage.  This section could be slow (since we iterate over all
	 objfiles in each call to sort_cmp), but this shouldn't happen
	 very often (GDB is already in a confused state; one hopes this
	 doesn't happen at all).  If you discover that significant time is
	 spent in the loops below, do 'set complaints 100' and examine the
	 resulting complaints.  */
      if (objfile1 == objfile2)
	{
	  /* Both sections came from the same objfile.  We are really
	     confused.  Sort on sequence order of sections within the
	     objfile.  The order of checks is important here, if we find a
	     match on SECT2 first then either SECT2 is before SECT1, or,
	     SECT2 == SECT1, in both cases we should return false.  The
	     second case shouldn't occur during normal use, but std::sort
	     does check that '!(a < a)' when compiled in debug mode.  */

	  const struct obj_section *osect;

	  ALL_OBJFILE_OSECTIONS (objfile1, osect)
	    if (osect == sect2)
	      return false;
	    else if (osect == sect1)
	      return true;

	  /* We should have found one of the sections before getting here.  */
	  gdb_assert_not_reached ("section not found");
	}
      else
	{
	  /* Sort on sequence number of the objfile in the chain.  */

	  for (objfile *objfile : current_program_space->objfiles ())
	    if (objfile == objfile1)
	      return true;
	    else if (objfile == objfile2)
	      return false;

	  /* We should have found one of the objfiles before getting here.  */
	  gdb_assert_not_reached ("objfile not found");
	}
    }

  /* Unreachable.  */
  gdb_assert_not_reached ("unexpected code path");
  return false;
}

/* Select "better" obj_section to keep.  We prefer the one that came from
   the real object, rather than the one from separate debuginfo.
   Most of the time the two sections are exactly identical, but with
   prelinking the .rel.dyn section in the real object may have different
   size.  */

static struct obj_section *
preferred_obj_section (struct obj_section *a, struct obj_section *b)
{
  gdb_assert (obj_section_addr (a) == obj_section_addr (b));
  gdb_assert ((a->objfile->separate_debug_objfile == b->objfile)
	      || (b->objfile->separate_debug_objfile == a->objfile));
  gdb_assert ((a->objfile->separate_debug_objfile_backlink == b->objfile)
	      || (b->objfile->separate_debug_objfile_backlink == a->objfile));

  if (a->objfile->separate_debug_objfile != NULL)
    return a;
  return b;
}

/* Return 1 if SECTION should be inserted into the section map.
   We want to insert only non-overlay and non-TLS section.  */

static int
insert_section_p (const struct bfd *abfd,
		  const struct bfd_section *section)
{
  const bfd_vma lma = bfd_section_lma (section);

  if (overlay_debugging && lma != 0 && lma != bfd_section_vma (section)
      && (bfd_get_file_flags (abfd) & BFD_IN_MEMORY) == 0)
    /* This is an overlay section.  IN_MEMORY check is needed to avoid
       discarding sections from the "system supplied DSO" (aka vdso)
       on some Linux systems (e.g. Fedora 11).  */
    return 0;
  if ((bfd_section_flags (section) & SEC_THREAD_LOCAL) != 0)
    /* This is a TLS section.  */
    return 0;

  return 1;
}

/* Filter out overlapping sections where one section came from the real
   objfile, and the other from a separate debuginfo file.
   Return the size of table after redundant sections have been eliminated.  */

static int
filter_debuginfo_sections (struct obj_section **map, int map_size)
{
  int i, j;

  for (i = 0, j = 0; i < map_size - 1; i++)
    {
      struct obj_section *const sect1 = map[i];
      struct obj_section *const sect2 = map[i + 1];
      const struct objfile *const objfile1 = sect1->objfile;
      const struct objfile *const objfile2 = sect2->objfile;
      const CORE_ADDR sect1_addr = obj_section_addr (sect1);
      const CORE_ADDR sect2_addr = obj_section_addr (sect2);

      if (sect1_addr == sect2_addr
	  && (objfile1->separate_debug_objfile == objfile2
	      || objfile2->separate_debug_objfile == objfile1))
	{
	  map[j++] = preferred_obj_section (sect1, sect2);
	  ++i;
	}
      else
	map[j++] = sect1;
    }

  if (i < map_size)
    {
      gdb_assert (i == map_size - 1);
      map[j++] = map[i];
    }

  /* The map should not have shrunk to less than half the original size.  */
  gdb_assert (map_size / 2 <= j);

  return j;
}

/* Filter out overlapping sections, issuing a warning if any are found.
   Overlapping sections could really be overlay sections which we didn't
   classify as such in insert_section_p, or we could be dealing with a
   corrupt binary.  */

static int
filter_overlapping_sections (struct obj_section **map, int map_size)
{
  int i, j;

  for (i = 0, j = 0; i < map_size - 1; )
    {
      int k;

      map[j++] = map[i];
      for (k = i + 1; k < map_size; k++)
	{
	  struct obj_section *const sect1 = map[i];
	  struct obj_section *const sect2 = map[k];
	  const CORE_ADDR sect1_addr = obj_section_addr (sect1);
	  const CORE_ADDR sect2_addr = obj_section_addr (sect2);
	  const CORE_ADDR sect1_endaddr = obj_section_endaddr (sect1);

	  gdb_assert (sect1_addr <= sect2_addr);

	  if (sect1_endaddr <= sect2_addr)
	    break;
	  else
	    {
	      /* We have an overlap.  Report it.  */

	      struct objfile *const objf1 = sect1->objfile;
	      struct objfile *const objf2 = sect2->objfile;

	      const struct bfd_section *const bfds1 = sect1->the_bfd_section;
	      const struct bfd_section *const bfds2 = sect2->the_bfd_section;

	      const CORE_ADDR sect2_endaddr = obj_section_endaddr (sect2);

	      struct gdbarch *const gdbarch = objf1->arch ();

	      complaint (_("unexpected overlap between:\n"
			   " (A) section `%s' from `%s' [%s, %s)\n"
			   " (B) section `%s' from `%s' [%s, %s).\n"
			   "Will ignore section B"),
			 bfd_section_name (bfds1), objfile_name (objf1),
			 paddress (gdbarch, sect1_addr),
			 paddress (gdbarch, sect1_endaddr),
			 bfd_section_name (bfds2), objfile_name (objf2),
			 paddress (gdbarch, sect2_addr),
			 paddress (gdbarch, sect2_endaddr));
	    }
	}
      i = k;
    }

  if (i < map_size)
    {
      gdb_assert (i == map_size - 1);
      map[j++] = map[i];
    }

  return j;
}


/* Update PMAP, PMAP_SIZE with sections from all objfiles, excluding any
   TLS, overlay and overlapping sections.  */

static void
update_section_map (struct program_space *pspace,
		    struct obj_section ***pmap, int *pmap_size)
{
  struct objfile_pspace_info *pspace_info;
  int alloc_size, map_size, i;
  struct obj_section *s, **map;

  pspace_info = get_objfile_pspace_data (pspace);
  gdb_assert (pspace_info->section_map_dirty != 0
	      || pspace_info->new_objfiles_available != 0);

  map = *pmap;
  xfree (map);

  alloc_size = 0;
  for (objfile *objfile : pspace->objfiles ())
    ALL_OBJFILE_OSECTIONS (objfile, s)
      if (insert_section_p (objfile->obfd, s->the_bfd_section))
	alloc_size += 1;

  /* This happens on detach/attach (e.g. in gdb.base/attach.exp).  */
  if (alloc_size == 0)
    {
      *pmap = NULL;
      *pmap_size = 0;
      return;
    }

  map = XNEWVEC (struct obj_section *, alloc_size);

  i = 0;
  for (objfile *objfile : pspace->objfiles ())
    ALL_OBJFILE_OSECTIONS (objfile, s)
      if (insert_section_p (objfile->obfd, s->the_bfd_section))
	map[i++] = s;

  std::sort (map, map + alloc_size, sort_cmp);
  map_size = filter_debuginfo_sections(map, alloc_size);
  map_size = filter_overlapping_sections(map, map_size);

  if (map_size < alloc_size)
    /* Some sections were eliminated.  Trim excess space.  */
    map = XRESIZEVEC (struct obj_section *, map, map_size);
  else
    gdb_assert (alloc_size == map_size);

  *pmap = map;
  *pmap_size = map_size;
}

/* Bsearch comparison function.  */

static int
bsearch_cmp (const void *key, const void *elt)
{
  const CORE_ADDR pc = *(CORE_ADDR *) key;
  const struct obj_section *section = *(const struct obj_section **) elt;

  if (pc < obj_section_addr (section))
    return -1;
  if (pc < obj_section_endaddr (section))
    return 0;
  return 1;
}

/* Returns a section whose range includes PC or NULL if none found.   */

struct obj_section *
find_pc_section (CORE_ADDR pc)
{
  struct objfile_pspace_info *pspace_info;
  struct obj_section *s, **sp;

  /* Check for mapped overlay section first.  */
  s = find_pc_mapped_section (pc);
  if (s)
    return s;

  pspace_info = get_objfile_pspace_data (current_program_space);
  if (pspace_info->section_map_dirty
      || (pspace_info->new_objfiles_available
	  && !pspace_info->inhibit_updates))
    {
      update_section_map (current_program_space,
			  &pspace_info->sections,
			  &pspace_info->num_sections);

      /* Don't need updates to section map until objfiles are added,
         removed or relocated.  */
      pspace_info->new_objfiles_available = 0;
      pspace_info->section_map_dirty = 0;
    }

  /* The C standard (ISO/IEC 9899:TC2) requires the BASE argument to
     bsearch be non-NULL.  */
  if (pspace_info->sections == NULL)
    {
      gdb_assert (pspace_info->num_sections == 0);
      return NULL;
    }

  sp = (struct obj_section **) bsearch (&pc,
					pspace_info->sections,
					pspace_info->num_sections,
					sizeof (*pspace_info->sections),
					bsearch_cmp);
  if (sp != NULL)
    return *sp;
  return NULL;
}


/* Return non-zero if PC is in a section called NAME.  */

int
pc_in_section (CORE_ADDR pc, const char *name)
{
  struct obj_section *s;
  int retval = 0;

  s = find_pc_section (pc);

  retval = (s != NULL
	    && s->the_bfd_section->name != NULL
	    && strcmp (s->the_bfd_section->name, name) == 0);
  return (retval);
}


/* Set section_map_dirty so section map will be rebuilt next time it
   is used.  Called by reread_symbols.  */

void
objfiles_changed (void)
{
  /* Rebuild section map next time we need it.  */
  get_objfile_pspace_data (current_program_space)->section_map_dirty = 1;
}

/* See comments in objfiles.h.  */

scoped_restore_tmpl<int>
inhibit_section_map_updates (struct program_space *pspace)
{
  return scoped_restore_tmpl<int>
    (&get_objfile_pspace_data (pspace)->inhibit_updates, 1);
}

/* See objfiles.h.  */

bool
is_addr_in_objfile (CORE_ADDR addr, const struct objfile *objfile)
{
  struct obj_section *osect;

  if (objfile == NULL)
    return false;

  ALL_OBJFILE_OSECTIONS (objfile, osect)
    {
      if (section_is_overlay (osect) && !section_is_mapped (osect))
	continue;

      if (obj_section_addr (osect) <= addr
	  && addr < obj_section_endaddr (osect))
	return true;
    }
  return false;
}

/* See objfiles.h.  */

bool
shared_objfile_contains_address_p (struct program_space *pspace,
				   CORE_ADDR address)
{
  for (objfile *objfile : pspace->objfiles ())
    {
      if ((objfile->flags & OBJF_SHARED) != 0
	  && is_addr_in_objfile (address, objfile))
	return true;
    }

  return false;
}

/* The default implementation for the "iterate_over_objfiles_in_search_order"
   gdbarch method.  It is equivalent to use the objfiles iterable,
   searching the objfiles in the order they are stored internally,
   ignoring CURRENT_OBJFILE.

   On most platforms, it should be close enough to doing the best
   we can without some knowledge specific to the architecture.  */

void
default_iterate_over_objfiles_in_search_order
  (struct gdbarch *gdbarch,
   iterate_over_objfiles_in_search_order_cb_ftype *cb,
   void *cb_data, struct objfile *current_objfile)
{
  int stop = 0;

  for (objfile *objfile : current_program_space->objfiles ())
    {
       stop = cb (objfile, cb_data);
       if (stop)
	 return;
    }
}

/* See objfiles.h.  */

const char *
objfile_name (const struct objfile *objfile)
{
  if (objfile->obfd != NULL)
    return bfd_get_filename (objfile->obfd);

  return objfile->original_name;
}

/* See objfiles.h.  */

const char *
objfile_filename (const struct objfile *objfile)
{
  if (objfile->obfd != NULL)
    return bfd_get_filename (objfile->obfd);

  return NULL;
}

/* See objfiles.h.  */

const char *
objfile_debug_name (const struct objfile *objfile)
{
  return lbasename (objfile->original_name);
}

/* See objfiles.h.  */

const char *
objfile_flavour_name (struct objfile *objfile)
{
  if (objfile->obfd != NULL)
    return bfd_flavour_name (bfd_get_flavour (objfile->obfd));
  return NULL;
}
