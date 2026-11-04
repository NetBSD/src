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

#include "dwarf2/cooked-index-worker.h"
#include "dwarf2/cooked-index.h"
#include "gdbsupport/thread-pool.h"
#include "maint.h"
#include "run-on-main-thread.h"
#include "event-top.h"
#include "exceptions.h"

/* See cooked-index-worker.h.  */

cooked_index_worker_result::cooked_index_worker_result ()
  : m_shard (new cooked_index_shard)
{
}

/* See cooked-index-worker.h.  */

cutu_reader *
cooked_index_worker_result::get_reader (dwarf2_per_cu *per_cu)
{
  auto it = m_reader_hash.find (*per_cu);
  return it != m_reader_hash.end () ? it->get () : nullptr;
}

/* See cooked-index-worker.h.  */

cutu_reader *
cooked_index_worker_result::preserve (cutu_reader_up reader)
{
  m_abbrev_table_cache.add (reader->release_abbrev_table ());

  auto [it, inserted] = m_reader_hash.insert (std::move (reader));
  gdb_assert (inserted);

  return it->get();
}

/* See cooked-index-worker.h.  */

std::uint64_t
cooked_index_worker_result::cutu_reader_hash::operator()
  (const cutu_reader_up &reader) const noexcept
{
  return (*this) (*reader->cu ()->per_cu);
}

/* See cooked-index-worker.h.  */

std::uint64_t
cooked_index_worker_result::cutu_reader_hash::operator() (const dwarf2_per_cu &per_cu)
  const noexcept
{
  return per_cu.index;
}

/* See cooked-index-worker.h.  */

bool
cooked_index_worker_result::cutu_reader_eq::operator() (const cutu_reader_up &a,
						  const cutu_reader_up &b) const noexcept
{
  return (*this) (*a->cu ()->per_cu, b);
}

/* See cooked-index-worker.h.  */

bool cooked_index_worker_result::cutu_reader_eq::operator()
  (const dwarf2_per_cu &per_cu, const cutu_reader_up &reader) const noexcept
{
  return per_cu.index == reader->cu ()->per_cu->index;
}

/* See cooked-index-worker.h.  */

void
cooked_index_worker_result::emit_complaints_and_exceptions
     (gdb::unordered_set<gdb_exception> &seen_exceptions)
{
  gdb_assert (is_main_thread ());

  re_emit_complaints (m_complaints);

  /* Only show a given exception a single time.  */
  for (auto &one_exc : m_exceptions)
    if (seen_exceptions.insert (one_exc).second)
      exception_print (gdb_stderr, one_exc);
}

/* See cooked-index-worker.h.  */

void
cooked_index_worker::start ()
{
  gdb::thread_pool::g_thread_pool->post_task ([this] ()
  {
    try
      {
	do_reading ();
      }
    catch (const gdb_exception &exc)
      {
	m_failed = exc;
	set (cooked_state::CACHE_DONE);
      }

    bfd_thread_cleanup ();
  });
}

/* See cooked-index-worker.h.  */

bool
cooked_index_worker::wait (cooked_state desired_state, bool allow_quit)
{
  bool done;
#if CXX_STD_THREAD
  {
    std::unique_lock<std::mutex> lock (m_mutex);

    /* This may be called from a non-main thread -- this functionality
       is needed for the index cache -- but in this case we require
       that the desired state already have been attained.  */
    gdb_assert (is_main_thread () || desired_state <= m_state);

    while (desired_state > m_state)
      {
	if (allow_quit)
	  {
	    std::chrono::milliseconds duration { 15 };
	    if (m_cond.wait_for (lock, duration) == std::cv_status::timeout)
	      QUIT;
	  }
	else
	  m_cond.wait (lock);
      }
    done = m_state == cooked_state::CACHE_DONE;
  }
#else
  /* Without threads, all the work is done immediately on the main
     thread, and there is never anything to wait for.  */
  done = desired_state == cooked_state::CACHE_DONE;
#endif /* CXX_STD_THREAD */

  /* Only the main thread is allowed to report complaints and the
     like.  */
  if (!is_main_thread ())
    return false;

  if (m_reported)
    return done;
  m_reported = true;

  /* Emit warnings first, maybe they were emitted before an exception
     (if any) was thrown.  */
  m_warnings.emit ();

  if (m_failed.has_value ())
    {
      /* do_reading failed -- report it.  */
      exception_print (gdb_stderr, *m_failed);
      m_failed.reset ();
      return done;
    }

  /* Only show a given exception a single time.  */
  gdb::unordered_set<gdb_exception> seen_exceptions;
  for (auto &one_result : m_results)
    one_result.emit_complaints_and_exceptions (seen_exceptions);

  print_stats ();

  struct objfile *objfile = m_per_objfile->objfile;
  dwarf2_per_bfd *per_bfd = m_per_objfile->per_bfd;
  cooked_index *table
    = (gdb::checked_static_cast<cooked_index *>
       (per_bfd->index_table.get ()));

  auto_obstack temp_storage;
  enum language lang = language_unknown;
  const char *main_name = table->get_main_name (&temp_storage, &lang);
  if (main_name != nullptr)
    set_objfile_main_name (objfile, main_name, lang);

  /* dwarf_read_debug_printf ("Done building psymtabs of %s", */
  /* 			   objfile_name (objfile)); */

  return done;
}

/* See cooked-index-worker.h.  */

void
cooked_index_worker::set (cooked_state desired_state)
{
  gdb_assert (desired_state != cooked_state::INITIAL);

#if CXX_STD_THREAD
  std::lock_guard<std::mutex> guard (m_mutex);
  gdb_assert (desired_state > m_state);
  m_state = desired_state;
  m_cond.notify_one ();
#else
  /* Without threads, all the work is done immediately on the main
     thread, and there is never anything to do.  */
#endif /* CXX_STD_THREAD */
}

/* See cooked-index-worker.h.  */

void
cooked_index_worker::write_to_cache (const cooked_index *idx)
{
  if (idx != nullptr)
    {
      /* Writing to the index cache may cause a warning to be emitted.
	 See PR symtab/30837.  This arranges to capture all such
	 warnings.  This is safe because we know the deferred_warnings
	 object isn't in use by any other thread at this point.  */
      scoped_restore_warning_hook defer (&m_warnings);
      m_cache_store.store ();
    }
}

/* See cooked-index-worker.h.  */

void
cooked_index_worker::done_reading ()
{
  {
    scoped_time_it time_it ("DWARF add parent map", m_per_command_time);

    for (auto &one_result : m_results)
      m_all_parents_map.add_map (*one_result.get_parent_map ());
  }

  dwarf2_per_bfd *per_bfd = m_per_objfile->per_bfd;
  cooked_index *table
    = (gdb::checked_static_cast<cooked_index *>
       (per_bfd->index_table.get ()));
  table->set_contents ();
}
