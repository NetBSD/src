/* DIE indexing 

   Copyright (C) 2022-2024 Free Software Foundation, Inc.

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

#include "dwarf2.h"
#include "dwarf2/types.h"
#include "symtab.h"
#include "hashtab.h"
#include "dwarf2/index-common.h"
#include <string_view>
#include "quick-symbol.h"
#include "gdbsupport/gdb_obstack.h"
#include "addrmap.h"
#include "gdbsupport/iterator-range.h"
#include "gdbsupport/thread-pool.h"
#include "dwarf2/mapped-index.h"
#include "dwarf2/read.h"
#include "dwarf2/tag.h"
#include "dwarf2/abbrev-cache.h"
#include "dwarf2/parent-map.h"
#include "gdbsupport/range-chain.h"
#include "gdbsupport/task-group.h"
#include "complaints.h"
#include "run-on-main-thread.h"

#if CXX_STD_THREAD
#include <mutex>
#include <condition_variable>
#endif /* CXX_STD_THREAD */

struct dwarf2_per_cu_data;
struct dwarf2_per_bfd;
struct index_cache_store_context;
struct cooked_index_entry;

/* Flags that describe an entry in the index.  */
enum cooked_index_flag_enum : unsigned char
{
  /* True if this entry is the program's "main".  */
  IS_MAIN = 1,
  /* True if this entry represents a "static" object.  */
  IS_STATIC = 2,
  /* True if this entry uses the linkage name.  */
  IS_LINKAGE = 4,
  /* True if this entry is just for the declaration of a type, not the
     definition.  */
  IS_TYPE_DECLARATION = 8,
  /* True is parent_entry.deferred has a value rather than parent_entry
     .resolved.  */
  IS_PARENT_DEFERRED = 16,
};
DEF_ENUM_FLAGS_TYPE (enum cooked_index_flag_enum, cooked_index_flag);

/* Type representing either a resolved or deferred cooked_index_entry.  */

union cooked_index_entry_ref
{
  cooked_index_entry_ref (parent_map::addr_type deferred_)
  {
    deferred = deferred_;
  }

  cooked_index_entry_ref (const cooked_index_entry *resolved_)
  {
    resolved = resolved_;
  }

  const cooked_index_entry *resolved;
  parent_map::addr_type deferred;
};

/* Return a string representation of FLAGS.  */

std::string to_string (cooked_index_flag flags);

/* Return true if LANG requires canonicalization.  This is used
   primarily to work around an issue computing the name of "main".
   This function must be kept in sync with
   cooked_index_shard::finalize.  */

extern bool language_requires_canonicalization (enum language lang);

/* A cooked_index_entry represents a single item in the index.  Note
   that two entries can be created for the same DIE -- one using the
   name, and another one using the linkage name, if any.

   This is an "open" class and the members are all directly
   accessible.  It is read-only after the index has been fully read
   and processed.  */
struct cooked_index_entry : public allocate_on_obstack<cooked_index_entry>
{
  cooked_index_entry (sect_offset die_offset_, enum dwarf_tag tag_,
		      cooked_index_flag flags_,
		      enum language lang_, const char *name_,
		      cooked_index_entry_ref parent_entry_,
		      dwarf2_per_cu_data *per_cu_)
    : name (name_),
      tag (tag_),
      flags (flags_),
      lang (lang_),
      die_offset (die_offset_),
      per_cu (per_cu_),
      m_parent_entry (parent_entry_)
  {
  }

  /* Return true if this entry matches SEARCH_FLAGS.  */
  bool matches (block_search_flags search_flags) const
  {
    /* Just reject type declarations.  */
    if ((flags & IS_TYPE_DECLARATION) != 0)
      return false;

    if ((search_flags & SEARCH_STATIC_BLOCK) != 0
	&& (flags & IS_STATIC) != 0)
      return true;
    if ((search_flags & SEARCH_GLOBAL_BLOCK) != 0
	&& (flags & IS_STATIC) == 0)
      return true;
    return false;
  }

  /* Return true if this entry matches KIND.  */
  bool matches (domain_search_flags kind) const;

  /* Construct the fully-qualified name of this entry and return a
     pointer to it.  If allocation is needed, it will be done on
     STORAGE.  FOR_MAIN is true if we are computing the name of the
     "main" entry -- one marked DW_AT_main_subprogram.  This matters
     for avoiding name canonicalization and also a related race (if
     "main" computation is done during finalization).  */
  const char *full_name (struct obstack *storage, bool for_main = false) const;

  /* Comparison modes for the 'compare' function.  See the function
     for a description.  */
  enum comparison_mode
  {
    MATCH,
    SORT,
    COMPLETE,
  };

  /* Compare two strings, case-insensitively.  Return -1 if STRA is
     less than STRB, 0 if they are equal, and 1 if STRA is greater.

     When comparing, '<' is considered to be less than all other
     printable characters.  This ensures that "t<x>" sorts before
     "t1", which is necessary when looking up "t".  This '<' handling
     is to ensure that certain C++ lookups work correctly.  It is
     inexact, and applied regardless of the search language, but this
     is ok because callers of this code do more precise filtering
     according to their needs.  This is also why using a
     case-insensitive comparison works even for languages that are
     case sensitive.

     MODE controls how the comparison proceeds.

     MODE==SORT is used when sorting and the only special '<' handling
     that it does is to ensure that '<' sorts before all other
     printable characters.  This ensures that the resulting ordering
     will be binary-searchable.

     MODE==MATCH is used when searching for a symbol.  In this case,
     STRB must always be the search name, and STRA must be the name in
     the index that is under consideration.  In compare mode, early
     termination of STRB may match STRA -- for example, "t<int>" and
     "t" will be considered to be equal.  (However, if A=="t" and
     B=="t<int>", then this will not consider them as equal.)

     MODE==COMPLETE is used when searching for a symbol for
     completion.  In this case, STRB must always be the search name,
     and STRA must be the name in the index that is under
     consideration.  In completion mode, early termination of STRB
     always results in a match.  */
  static int compare (const char *stra, const char *strb,
		      comparison_mode mode);

  /* Compare two entries by canonical name.  */
  bool operator< (const cooked_index_entry &other) const
  {
    return compare (canonical, other.canonical, SORT) < 0;
  }

  /* Set parent entry to PARENT.  */
  void set_parent (const cooked_index_entry *parent)
  {
    gdb_assert ((flags & IS_PARENT_DEFERRED) == 0);
    m_parent_entry.resolved = parent;
  }

  /* Resolve deferred parent entry to PARENT.  */
  void resolve_parent (const cooked_index_entry *parent)
  {
    gdb_assert ((flags & IS_PARENT_DEFERRED) != 0);
    flags = flags & ~IS_PARENT_DEFERRED;
    m_parent_entry.resolved = parent;
  }

  /* Return parent entry.  */
  const cooked_index_entry *get_parent () const
  {
    gdb_assert ((flags & IS_PARENT_DEFERRED) == 0);
    return m_parent_entry.resolved;
  }

  /* Return deferred parent entry.  */
  parent_map::addr_type get_deferred_parent () const
  {
    gdb_assert ((flags & IS_PARENT_DEFERRED) != 0);
    return m_parent_entry.deferred;
  }

  /* The name as it appears in DWARF.  This always points into one of
     the mapped DWARF sections.  Note that this may be the name or the
     linkage name -- two entries are created for DIEs which have both
     attributes.  */
  const char *name;
  /* The canonical name.  For C++ names, this may differ from NAME.
     In all other cases, this is equal to NAME.  */
  const char *canonical = nullptr;
  /* The DWARF tag.  */
  enum dwarf_tag tag;
  /* Any flags attached to this entry.  */
  cooked_index_flag flags;
  /* The language of this symbol.  */
  ENUM_BITFIELD (language) lang : LANGUAGE_BITS;
  /* The offset of this DIE.  */
  sect_offset die_offset;
  /* The CU from which this entry originates.  */
  dwarf2_per_cu_data *per_cu;

private:

  /* A helper method for full_name.  Emits the full scope of this
     object, followed by the separator, to STORAGE.  If this entry has
     a parent, its write_scope method is called first.  */
  void write_scope (struct obstack *storage, const char *sep,
		    bool for_name) const;

  /* The parent entry.  This is NULL for top-level entries.
     Otherwise, it points to the parent entry, such as a namespace or
     class.  */
  cooked_index_entry_ref m_parent_entry;
};

class cooked_index;

/* An index of interesting DIEs.  This is "cooked", in contrast to a
   mapped .debug_names or .gdb_index, which are "raw".  An entry in
   the index is of type cooked_index_entry.

   Operations on the index are described below.  They are chosen to
   make it relatively simple to implement the symtab "quick"
   methods.  */
class cooked_index_shard
{
public:
  cooked_index_shard () = default;
  DISABLE_COPY_AND_ASSIGN (cooked_index_shard);

  /* Create a new cooked_index_entry and register it with this object.
     Entries are owned by this object.  The new item is returned.  */
  cooked_index_entry *add (sect_offset die_offset, enum dwarf_tag tag,
			   cooked_index_flag flags, enum language lang,
			   const char *name,
			   cooked_index_entry_ref parent_entry,
			   dwarf2_per_cu_data *per_cu);

  /* Install a new fixed addrmap from the given mutable addrmap.  */
  void install_addrmap (addrmap_mutable *map)
  {
    gdb_assert (m_addrmap == nullptr);
    m_addrmap = new (&m_storage) addrmap_fixed (&m_storage, map);
  }

  friend class cooked_index;

  /* A simple range over part of m_entries.  */
  typedef iterator_range<std::vector<cooked_index_entry *>::const_iterator>
       range;

  /* Return a range of all the entries.  */
  range all_entries () const
  {
    return { m_entries.cbegin (), m_entries.cend () };
  }

  /* Look up an entry by name.  Returns a range of all matching
     results.  If COMPLETING is true, then a larger range, suitable
     for completion, will be returned.  */
  range find (const std::string &name, bool completing) const;

private:

  /* Return the entry that is believed to represent the program's
     "main".  This will return NULL if no such entry is available.  */
  const cooked_index_entry *get_main () const
  {
    return m_main;
  }

  /* Look up ADDR in the address map, and return either the
     corresponding CU, or nullptr if the address could not be
     found.  */
  dwarf2_per_cu_data *lookup (unrelocated_addr addr)
  {
    return (static_cast<dwarf2_per_cu_data *>
	    (m_addrmap->find ((CORE_ADDR) addr)));
  }

  /* Create a new cooked_index_entry and register it with this object.
     Entries are owned by this object.  The new item is returned.  */
  cooked_index_entry *create (sect_offset die_offset,
			      enum dwarf_tag tag,
			      cooked_index_flag flags,
			      enum language lang,
			      const char *name,
			      cooked_index_entry_ref parent_entry,
			      dwarf2_per_cu_data *per_cu)
  {
    return new (&m_storage) cooked_index_entry (die_offset, tag, flags,
						lang, name, parent_entry,
						per_cu);
  }

  /* GNAT only emits mangled ("encoded") names in the DWARF, and does
     not emit the module structure.  However, we need this structure
     to do lookups.  This function recreates that structure for an
     existing entry.  It returns the base name (last element) of the
     full decoded name.  */
  gdb::unique_xmalloc_ptr<char> handle_gnat_encoded_entry
       (cooked_index_entry *entry, htab_t gnat_entries);

  /* Finalize the index.  This should be called a single time, when
     the index has been fully populated.  It enters all the entries
     into the internal table and fixes up all missing parent links.
     This may be invoked in a worker thread.  */
  void finalize (const parent_map_map *parent_maps);

  /* Storage for the entries.  */
  auto_obstack m_storage;
  /* List of all entries.  */
  std::vector<cooked_index_entry *> m_entries;
  /* If we found an entry with 'is_main' set, store it here.  */
  cooked_index_entry *m_main = nullptr;
  /* The addrmap.  This maps address ranges to dwarf2_per_cu_data
     objects.  */
  addrmap_fixed *m_addrmap = nullptr;
  /* Storage for canonical names.  */
  std::vector<gdb::unique_xmalloc_ptr<char>> m_names;
};

class cutu_reader;

/* An instance of this is created when scanning DWARF to create a
   cooked index.  */

class cooked_index_storage
{
public:

  cooked_index_storage ();
  DISABLE_COPY_AND_ASSIGN (cooked_index_storage);

  /* Return the current abbrev cache.  */
  abbrev_cache *get_abbrev_cache ()
  {
    return &m_abbrev_cache;
  }

  /* Return the DIE reader corresponding to PER_CU.  If no such reader
     has been registered, return NULL.  */
  cutu_reader *get_reader (dwarf2_per_cu_data *per_cu);

  /* Preserve READER by storing it in the local hash table.  */
  cutu_reader *preserve (std::unique_ptr<cutu_reader> reader);

  /* Add an entry to the index.  The arguments describe the entry; see
     cooked-index.h.  The new entry is returned.  */
  cooked_index_entry *add (sect_offset die_offset, enum dwarf_tag tag,
			   cooked_index_flag flags,
			   const char *name,
			   cooked_index_entry_ref parent_entry,
			   dwarf2_per_cu_data *per_cu)
  {
    return m_index->add (die_offset, tag, flags, per_cu->lang (),
			 name, parent_entry, per_cu);
  }

  /* Install the current addrmap into the shard being constructed,
     then transfer ownership of the index to the caller.  */
  std::unique_ptr<cooked_index_shard> release ()
  {
    m_index->install_addrmap (&m_addrmap);
    return std::move (m_index);
  }

  /* Return the mutable addrmap that is currently being created.  */
  addrmap_mutable *get_addrmap ()
  {
    return &m_addrmap;
  }

  /* Return the parent_map that is currently being created.  */
  parent_map *get_parent_map ()
  {
    return &m_parent_map;
  }

  /* Return the parent_map that is currently being created.  Ownership
     is passed to the caller.  */
  parent_map release_parent_map ()
  {
    return std::move (m_parent_map);
  }

private:

  /* Hash function for a cutu_reader.  */
  static hashval_t hash_cutu_reader (const void *a);

  /* Equality function for cutu_reader.  */
  static int eq_cutu_reader (const void *a, const void *b);

  /* The abbrev cache used by this indexer.  */
  abbrev_cache m_abbrev_cache;
  /* A hash table of cutu_reader objects.  */
  htab_up m_reader_hash;
  /* The index shard that is being constructed.  */
  std::unique_ptr<cooked_index_shard> m_index;

  /* Parent map for each CU that is read.  */
  parent_map m_parent_map;

  /* A writeable addrmap being constructed by this scanner.  */
  addrmap_mutable m_addrmap;
};

/* The possible states of the index.  See the explanatory comment
   before cooked_index for more details.  */
enum class cooked_state
{
  /* The default state.  This is not a valid argument to 'wait'.  */
  INITIAL,
  /* The initial scan has completed.  The name of "main" is now
     available (if known).  The addrmaps are usable now.
     Finalization has started but is not complete.  */
  MAIN_AVAILABLE,
  /* Finalization has completed.  This means the index is fully
     available for queries.  */
  FINALIZED,
  /* Writing to the index cache has finished.  */
  CACHE_DONE,
};

/* An object of this type controls the scanning of the DWARF.  It
   schedules the worker tasks and tracks the current state.  Once
   scanning is done, this object is discarded.
   
   This is an abstract base class that defines the basic behavior of
   scanners.  Separate concrete implementations exist for scanning
   .debug_names and .debug_info.  */

class cooked_index_worker
{
public:

  explicit cooked_index_worker (dwarf2_per_objfile *per_objfile)
    : m_per_objfile (per_objfile),
      m_cache_store (global_index_cache, per_objfile->per_bfd)
  { }
  virtual ~cooked_index_worker ()
  { }
  DISABLE_COPY_AND_ASSIGN (cooked_index_worker);

  /* Start reading.  */
  void start ();

  /* Wait for a particular state to be achieved.  If ALLOW_QUIT is
     true, then the loop will check the QUIT flag.  Normally this
     method may only be called from the main thread; however, it can
     be called from a worker thread provided that the desired state
     has already been attained.  (This oddity is used by the index
     cache writer.)  */
  bool wait (cooked_state desired_state, bool allow_quit);

protected:

  /* Let cooked_index call the 'set' and 'write_to_cache' methods.  */
  friend class cooked_index;

  /* Set the current state.  */
  void set (cooked_state desired_state);

  /* Write to the index cache.  */
  void write_to_cache (const cooked_index *idx,
		       deferred_warnings *warn) const;

  /* Helper function that does the work of reading.  This must be able
     to be run in a worker thread without problems.  */
  virtual void do_reading () = 0;

  /* A callback that can print stats, if needed.  This is called when
     transitioning to the 'MAIN_AVAILABLE' state.  */
  virtual void print_stats ()
  { }

  /* Each thread returns a tuple holding a cooked index, any collected
     complaints, a vector of errors that should be printed, and a
     vector of parent maps.

     The errors are retained because GDB's I/O system is not
     thread-safe.  run_on_main_thread could be used, but that would
     mean the messages are printed after the prompt, which looks
     weird.  */
  using result_type = std::tuple<std::unique_ptr<cooked_index_shard>,
				 complaint_collection,
				 std::vector<gdb_exception>,
				 parent_map>;

  /* The per-objfile object.  */
  dwarf2_per_objfile *m_per_objfile;
  /* Result of each worker task.  */
  std::vector<result_type> m_results;
  /* Any warnings emitted.  This is not in 'result_type' because (for
     the time being at least), it's only needed in do_reading, not in
     every worker.  Note that deferred_warnings uses gdb_stderr in its
     constructor, and this should only be done from the main thread.
     This is enforced in the cooked_index_worker constructor.  */
  deferred_warnings m_warnings;

  /* A map of all parent maps.  Used during finalization to fix up
     parent relationships.  */
  parent_map_map m_all_parents_map;

#if CXX_STD_THREAD
  /* Current state of this object.  */
  cooked_state m_state = cooked_state::INITIAL;
  /* Mutex and condition variable used to synchronize.  */
  std::mutex m_mutex;
  std::condition_variable m_cond;
#endif /* CXX_STD_THREAD */
  /* This flag indicates whether any complaints or exceptions that
     arose during scanning have been reported by 'wait'.  This may
     only be modified on the main thread.  */
  bool m_reported = false;
  /* If set, an exception occurred during reading; in this case the
     scanning is stopped and this exception will later be reported by
     the 'wait' method.  */
  std::optional<gdb_exception> m_failed;
  /* An object used to write to the index cache.  */
  index_cache_store_context m_cache_store;
};

/* The main index of DIEs.

   The index is created by multiple threads.  The overall process is
   somewhat complicated, so here's a diagram to help sort it out.

   The basic idea behind this design is (1) to do as much work as
   possible in worker threads, and (2) to start the work as early as
   possible.  This combination should help hide the effort from the
   user to the maximum possible degree.

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

  /* A convenience typedef for the vector that is contained in this
     object.  */
  using vec_type = std::vector<std::unique_ptr<cooked_index_shard>>;

  cooked_index (dwarf2_per_objfile *per_objfile,
		std::unique_ptr<cooked_index_worker> &&worker);
  ~cooked_index () override;

  DISABLE_COPY_AND_ASSIGN (cooked_index);

  /* Start reading the DWARF.  */
  void start_reading ();

  /* Called by cooked_index_worker to set the contents of this index
     and transition to the MAIN_AVAILABLE state.  WARN is used to
     collect any warnings that may arise when writing to the cache.
     PARENT_MAPS is used when resolving pending parent links.
     PARENT_MAPS may be NULL if there are no IS_PARENT_DEFERRED
     entries in VEC.  */
  void set_contents (vec_type &&vec, deferred_warnings *warn,
		     const parent_map_map *parent_maps);

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
    result_range.reserve (m_vector.size ());
    for (auto &entry : m_vector)
      result_range.push_back (entry->all_entries ());
    return range (std::move (result_range));
  }

  /* Look up ADDR in the address map, and return either the
     corresponding CU, or nullptr if the address could not be
     found.  */
  dwarf2_per_cu_data *lookup (unrelocated_addr addr);

  /* Return a new vector of all the addrmaps used by all the indexes
     held by this object.  */
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
  vec_type m_vector;

  /* This tracks the current state.  When this is nullptr, it means
     that the state is CACHE_DONE -- it's important to note that only
     the main thread may change the value of this pointer.  */
  std::unique_ptr<cooked_index_worker> m_state;

  dwarf2_per_bfd *m_per_bfd;
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

  dwarf2_per_cu_data *find_per_cu (dwarf2_per_bfd *per_bfd,
				   unrelocated_addr adjusted_pc) override;

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
     gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
     const lookup_name_info *lookup_name,
     gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
     gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
     block_search_flags search_flags,
     domain_search_flags domain) override;

  struct compunit_symtab *find_pc_sect_compunit_symtab
    (struct objfile *objfile, struct bound_minimal_symbol msymbol,
     CORE_ADDR pc, struct obj_section *section, int warn_if_readin) override
  {
    wait (objfile, true);
    return (dwarf2_base_index_functions::find_pc_sect_compunit_symtab
	    (objfile, msymbol, pc, section, warn_if_readin));
  }

  void map_symbol_filenames
       (struct objfile *objfile,
	gdb::function_view<symbol_filename_ftype> fun,
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
