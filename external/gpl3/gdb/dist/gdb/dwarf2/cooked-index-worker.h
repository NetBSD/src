/* DWARF index storage

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

#ifndef GDB_DWARF2_COOKED_INDEX_WORKER_H
#define GDB_DWARF2_COOKED_INDEX_WORKER_H

#include "dwarf2/abbrev-table-cache.h"
#include "dwarf2/cooked-index-entry.h"
#include "dwarf2/cooked-index-shard.h"
#include "dwarf2/types.h"
#include "dwarf2/read.h"
#include "maint.h"
#include "run-on-main-thread.h"

#if CXX_STD_THREAD
#include <mutex>
#include <condition_variable>
#endif /* CXX_STD_THREAD */

using cutu_reader_up = std::unique_ptr<cutu_reader>;

/* An instance of this is created when scanning DWARF to create a
   cooked index.  This class is the result of a single task to store
   results while working -- that is, it is an implementation detail of
   the threads managed by cooked_index_worker.  Once scanning is done,
   selected parts of the state here are stored into the shard, and
   then these temporary objects are destroyed.  */

class cooked_index_worker_result
{
public:

  cooked_index_worker_result ();
  DISABLE_COPY_AND_ASSIGN (cooked_index_worker_result);

  cooked_index_worker_result (cooked_index_worker_result &&) = default;
  cooked_index_worker_result &operator= (cooked_index_worker_result &&)
    = default;

  /* Return the current abbrev table_cache.  */
  const abbrev_table_cache &get_abbrev_table_cache () const
  { return m_abbrev_table_cache; }

  /* Return the DIE reader corresponding to PER_CU.  If no such reader
     has been registered, return NULL.  */
  cutu_reader *get_reader (dwarf2_per_cu *per_cu);

  /* Preserve READER by storing it in the local hash table.  */
  cutu_reader *preserve (cutu_reader_up reader);

  /* Add an entry to the index.  The arguments describe the entry; see
     cooked-index.h.  The new entry is returned.  */
  cooked_index_entry *add (sect_offset die_offset, enum dwarf_tag tag,
			   cooked_index_flag flags,
			   const char *name,
			   cooked_index_entry_ref parent_entry,
			   dwarf2_per_cu *per_cu)
  {
    return m_shard->add (die_offset, tag, flags, per_cu->lang (),
			 name, parent_entry, per_cu);
  }

  /* Overload that allows the language to be specified.  */
  cooked_index_entry *add (sect_offset die_offset, enum dwarf_tag tag,
			   cooked_index_flag flags, enum language lang,
			   const char *name,
			   cooked_index_entry_ref parent_entry,
			   dwarf2_per_cu *per_cu)
  {
    return m_shard->add (die_offset, tag, flags, lang,
			 name, parent_entry, per_cu);
  }

  /* Install the current addrmap into the shard being constructed,
     then transfer ownership of the index to the caller.  */
  cooked_index_shard_up release_shard ()
  {
    m_shard->install_addrmap (&m_addrmap);
    /* This isn't needed any more.  */
    m_addrmap.clear ();
    return std::move (m_shard);
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

  /* Catch exceptions from calling F (), and add them to the list of caught
     exceptions.  These are passed forward and printed by the main thread.  */
  template <typename F>
  void
  catch_error (F &&f)
  {
    try
      {
	f ();
      }
    catch (gdb_exception &ex)
      {
	m_exceptions.push_back (std::move (ex));
      }
  }

  /* Called when the thread using this object is done with its work.
     This stores any complaints for later emission, and it clears some
     data that won't be needed again.  */
  void done_reading (complaint_collection &&complaints)
  {
    /* Hang on to the complaints.  */
    m_complaints = std::move (complaints);
    /* Discard things that are no longer needed.  */
    m_reader_hash.clear ();
  }

  /* Called to emit any stored complaints or exceptions.  This can
     only be called on the main thread.  */
  void emit_complaints_and_exceptions
       (gdb::unordered_set<gdb_exception> &seen_exceptions);

private:
  /* The abbrev table cache used by this indexer.  */
  abbrev_table_cache m_abbrev_table_cache;

  /* Hash function for a cutu_reader.  */
  struct cutu_reader_hash
  {
    using is_transparent = void;

    std::uint64_t operator() (const cutu_reader_up &reader) const noexcept;
    std::uint64_t operator() (const dwarf2_per_cu &per_cu) const noexcept;
  };

  /* Equality function for cutu_reader.  */
  struct cutu_reader_eq
  {
    using is_transparent = void;

    bool operator() (const cutu_reader_up &a,
		     const cutu_reader_up &b) const noexcept;

    bool operator() (const dwarf2_per_cu &per_cu,
		     const cutu_reader_up &reader) const noexcept;
  };

  /* A hash table of cutu_reader objects.  */
  gdb::unordered_set<cutu_reader_up, cutu_reader_hash, cutu_reader_eq>
    m_reader_hash;

  /* The index shard that is being constructed.  */
  cooked_index_shard_up m_shard;

  /* Parent map for each CU that is read.  */
  parent_map m_parent_map;

  /* A writeable addrmap being constructed by this scanner.  */
  addrmap_mutable m_addrmap;

  /* The issued complaints.  Only set after done_reading is
     called.  */
  complaint_collection m_complaints;

  /* Exceptions that we're storing to emit later.  */
  std::vector<gdb_exception> m_exceptions;
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
      m_cache_store (global_index_cache, per_objfile->per_bfd),
      m_per_command_time (per_command_time)
  {
    /* Make sure we capture per_command_time from the main thread.  */
    gdb_assert (is_main_thread ());
  }
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

  /* Release all shards from the results.  */
  std::vector<cooked_index_shard_up> release_shards ()
  {
    std::vector<cooked_index_shard_up> result;
    for (auto &one_result : m_results)
      result.push_back (one_result.release_shard ());
    result.shrink_to_fit ();
    return result;
  }

  /* Return the object holding all the parent maps.  */
  const parent_map_map *get_parent_map_map () const
  {
    return &m_all_parents_map;
  }

protected:

  /* Let cooked_index call the 'set' and 'write_to_cache' methods.  */
  friend class cooked_index;

  /* Set the current state.  */
  void set (cooked_state desired_state);

  /* Write to the index cache.  */
  void write_to_cache (const cooked_index *idx);

  /* Helper function that does the work of reading.  This must be able
     to be run in a worker thread without problems.  */
  virtual void do_reading () = 0;

  /* Helper function that should be called when done reading.  This
     assumes that m_results is filled in, and will initialize
     m_all_parents_map and end by calling
     cooked_index::set_contents.  */
  virtual void done_reading ();

  /* A callback that can print stats, if needed.  This is called when
     transitioning to the 'MAIN_AVAILABLE' state.  */
  virtual void print_stats ()
  { }

  /* The per-objfile object.  */
  dwarf2_per_objfile *m_per_objfile;
  /* Result of each worker task.  */
  std::vector<cooked_index_worker_result> m_results;
  /* Any warnings emitted.  For the time being at least, this only
     needed in do_reading, not in every worker.  Note that
     deferred_warnings uses gdb_stderr in its constructor, and this
     should only be done from the main thread.  This is enforced in
     the cooked_index_worker constructor.  */
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

  /* Captured value of per_command_time.  */
  bool m_per_command_time;
};

using cooked_index_worker_up = std::unique_ptr<cooked_index_worker>;

#endif /* GDB_DWARF2_COOKED_INDEX_WORKER_H */
