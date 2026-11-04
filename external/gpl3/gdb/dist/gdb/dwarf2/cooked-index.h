/* DIE indexing 

   Copyright (C) 2022-2025 Free Software Foundation, Inc.

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

#ifndef GDB_DWARF2_COOKED_INDEX_H
#define GDB_DWARF2_COOKED_INDEX_H

#include "dwarf2/cooked-index-entry.h"
#include "symtab.h"
#include "quick-symbol.h"
#include "addrmap.h"
#include "dwarf2/mapped-index.h"
#include "dwarf2/read.h"
#include "dwarf2/parent-map.h"
#include "gdbsupport/range-chain.h"
#include "dwarf2/cooked-index-shard.h"
#include "dwarf2/cooked-index-worker.h"

/* The main index of DIEs.

   The index is created by multiple threads.  The overall process is
   somewhat complicated, so here's a diagram to help sort it out.

   The basic idea behind this design is (1) to do as much work as
   possible in worker threads, and (2) to start the work as early as
   possible.  This combination should help hide the effort from the
   user to the maximum possible degree.

   There are a number of different objects involved in this process.
   Most of them are temporary -- they are created to handle different
   phases of scanning, then discarded when possible.  The "steady
   state" objects are index itself (cooked_index, below), which holds
   the entries (cooked_index_entry), and the implementation of the
   "quick" API (e.g., cooked_index_functions, though there are
   other variants).

   . Main Thread                |       Worker Threads
   ============================================================
   . dwarf2_initialize_objfile
   . 	      |
   .          v
   .     cooked index ------------> cooked_index_worker::start
   .          |                           / | \
   .          v                          /  |  \
   .       install                      /   |	\
   .  cooked_index_functions        scan CUs in workers
   .          |               create cooked_index_shard objects
   .          |                           \ | /
   .          v                            \|/
   .    return to caller                    v
   .                                 initial scan is done
   .                                state = MAIN_AVAILABLE
   .                              "main" name now available
   .                                        |
   .                                        |
   .   if main thread calls...              v
   .   compute_main_name         cooked_index::set_contents
   .          |                           / | \
   .          v                          /  |  \
   .   wait (MAIN_AVAILABLE)          finalization
   .          |                          \  |  /
   .          v                           \ | /        
   .        done                      state = FINALIZED
   .                                        |
   .                                        v
   .                              maybe write to index cache
   .                                  state = CACHE_DONE
   .                                 ~cooked_index_worker
   .
   .
   .   if main thread calls...
   .   any other "quick" API
   .          |
   .          v
   .   wait (FINALIZED)
   .          |
   .          v
   .    use the index
*/

class cooked_index : public dwarf_scanner_base
{
public:
  cooked_index (cooked_index_worker_up &&worker);
  ~cooked_index () override;

  DISABLE_COPY_AND_ASSIGN (cooked_index);

  /* Start reading the DWARF.  */
  void start_reading () override;

  /* Called by cooked_index_worker to set the contents of this index
     and transition to the MAIN_AVAILABLE state.  */
  void set_contents ();

  /* A range over a vector of subranges.  */
  using range = range_chain<cooked_index_shard::range>;

  /* Look up an entry by name.  Returns a range of all matching
     results.  If COMPLETING is true, then a larger range, suitable
     for completion, will be returned.  */
  range find (const std::string &name, bool completing);

  /* Return a range of all the entries.  */
  range all_entries ()
  {
    wait (cooked_state::FINALIZED, true);
    std::vector<cooked_index_shard::range> result_range;
    result_range.reserve (m_shards.size ());
    for (auto &shard : m_shards)
      result_range.push_back (shard->all_entries ());
    return range (std::move (result_range));
  }

  /* Look up ADDR in the address map, and return either the
     corresponding CU, or nullptr if the address could not be
     found.  */
  dwarf2_per_cu *lookup (unrelocated_addr addr) override;

  /* Return a new vector of all the addrmaps used by all the indexes
     held by this object.

     Elements of the vector may be nullptr.  */
  std::vector<const addrmap *> get_addrmaps ();

  /* Return the entry that is believed to represent the program's
     "main".  This will return NULL if no such entry is available.  */
  const cooked_index_entry *get_main () const;

  const char *get_main_name (struct obstack *obstack, enum language *lang)
    const;

  cooked_index *index_for_writing () override
  {
    wait (cooked_state::FINALIZED, true);
    return this;
  }

  quick_symbol_functions_up make_quick_functions () const override;

  /* Dump a human-readable form of the contents of the index.  */
  void dump (gdbarch *arch);

  /* Wait until this object reaches the desired state.  Note that
     DESIRED_STATE may not be INITIAL -- it does not make sense to
     wait for this.  If ALLOW_QUIT is true, timed waits will be done
     and the quit flag will be checked in a loop.  This may normally
     only be called from the main thread; however, it is ok to call
     from a worker as long as the desired state has already been
     attained.  (This property is needed by the index cache
     writer.)  */
  void wait (cooked_state desired_state, bool allow_quit = false);

  void wait_completely () override
  { wait (cooked_state::CACHE_DONE); }

private:

  /* The vector of cooked_index objects.  This is stored because the
     entries are stored on the obstacks in those objects.  */
  std::vector<cooked_index_shard_up> m_shards;

  /* This tracks the current state.  When this is nullptr, it means
     that the state is CACHE_DONE -- it's important to note that only
     the main thread may change the value of this pointer.  */
  cooked_index_worker_up m_state;
};

/* An implementation of quick_symbol_functions for the cooked DWARF
   index.  */

struct cooked_index_functions : public dwarf2_base_index_functions
{
  cooked_index *wait (struct objfile *objfile, bool allow_quit)
  {
    dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
    cooked_index *table
      = (gdb::checked_static_cast<cooked_index *>
	 (per_objfile->per_bfd->index_table.get ()));
    table->wait (cooked_state::MAIN_AVAILABLE, allow_quit);
    return table;
  }

  struct compunit_symtab *find_compunit_symtab_by_address
    (struct objfile *objfile, CORE_ADDR address) override;

  bool has_unexpanded_symtabs (struct objfile *objfile) override
  {
    wait (objfile, true);
    return dwarf2_base_index_functions::has_unexpanded_symtabs (objfile);
  }

  struct symtab *find_last_source_symtab (struct objfile *objfile) override
  {
    wait (objfile, true);
    return dwarf2_base_index_functions::find_last_source_symtab (objfile);
  }

  void forget_cached_source_info (struct objfile *objfile) override
  {
    wait (objfile, true);
    dwarf2_base_index_functions::forget_cached_source_info (objfile);
  }

  void print_stats (struct objfile *objfile, bool print_bcache) override
  {
    wait (objfile, true);
    dwarf2_base_index_functions::print_stats (objfile, print_bcache);
  }

  void dump (struct objfile *objfile) override
  {
    cooked_index *index = wait (objfile, true);
    gdb_printf ("Cooked index in use:\n");
    gdb_printf ("\n");
    index->dump (objfile->arch ());
  }

  void expand_all_symtabs (struct objfile *objfile) override
  {
    wait (objfile, true);
    dwarf2_base_index_functions::expand_all_symtabs (objfile);
  }

  bool expand_symtabs_matching
    (struct objfile *objfile,
     expand_symtabs_file_matcher file_matcher,
     const lookup_name_info *lookup_name,
     expand_symtabs_symbol_matcher symbol_matcher,
     expand_symtabs_expansion_listener expansion_notify,
     block_search_flags search_flags,
     domain_search_flags domain,
     expand_symtabs_lang_matcher lang_matcher) override;

  struct compunit_symtab *find_pc_sect_compunit_symtab
    (struct objfile *objfile, bound_minimal_symbol msymbol,
     CORE_ADDR pc, struct obj_section *section, int warn_if_readin) override
  {
    wait (objfile, true);
    return (dwarf2_base_index_functions::find_pc_sect_compunit_symtab
	    (objfile, msymbol, pc, section, warn_if_readin));
  }

  void map_symbol_filenames (objfile *objfile, symbol_filename_listener fun,
			     bool need_fullname) override
  {
    wait (objfile, true);
    return (dwarf2_base_index_functions::map_symbol_filenames
	    (objfile, fun, need_fullname));
  }

  void compute_main_name (struct objfile *objfile) override
  {
    wait (objfile, false);
  }
};

#endif /* GDB_DWARF2_COOKED_INDEX_H */
