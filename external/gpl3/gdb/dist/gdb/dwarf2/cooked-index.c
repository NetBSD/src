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

#include "dwarf2/cooked-index.h"
#include "dwarf2/read.h"
#include "dwarf2/stringify.h"
#include "event-top.h"
#include "maint.h"
#include "observable.h"
#include "run-on-main-thread.h"
#include "gdbsupport/task-group.h"
#include "cli/cli-cmds.h"

/* We don't want gdb to exit while it is in the process of writing to
   the index cache.  So, all live cooked index vectors are stored
   here, and then these are all waited for before exit proceeds.  */
static gdb::unordered_set<cooked_index *> active_vectors;

/* Return true if LANG requires canonicalization.  This is used
   primarily to work around an issue computing the name of "main".
   This function must be kept in sync with
   cooked_index_shard::finalize.  */

static bool
language_requires_canonicalization (enum language lang)
{
  return (lang == language_ada
	  || lang == language_c
	  || lang == language_cplus);
}

cooked_index::cooked_index (cooked_index_worker_up &&worker)
  : m_state (std::move (worker))
{
  /* ACTIVE_VECTORS is not locked, and this assert ensures that this
     will be caught if ever moved to the background.  */
  gdb_assert (is_main_thread ());
  active_vectors.insert (this);
}

void
cooked_index::start_reading ()
{
  m_state->start ();
}

void
cooked_index::wait (cooked_state desired_state, bool allow_quit)
{
  gdb_assert (desired_state != cooked_state::INITIAL);

  /* If the state object has been deleted, then that means waiting is
     completely done.  */
  if (m_state == nullptr)
    return;

  if (m_state->wait (desired_state, allow_quit))
    {
      /* Only the main thread can modify this.  */
      gdb_assert (is_main_thread ());
      m_state.reset (nullptr);
    }
}

void
cooked_index::set_contents ()
{
  gdb_assert (m_shards.empty ());
  m_shards = m_state->release_shards ();

  m_state->set (cooked_state::MAIN_AVAILABLE);

  /* This is run after finalization is done -- but not before.  If
     this task were submitted earlier, it would have to wait for
     finalization.  However, that would take a slot in the global
     thread pool, and if enough such tasks were submitted at once, it
     would cause a livelock.  */
  gdb::task_group finalizers ([this] ()
  {
    m_state->set (cooked_state::FINALIZED);
    m_state->write_to_cache (index_for_writing ());
    m_state->set (cooked_state::CACHE_DONE);
  });

  for (auto &shard : m_shards)
    {
      auto this_shard = shard.get ();
      const parent_map_map *parent_maps = m_state->get_parent_map_map ();
      finalizers.add_task ([=] ()
	{
	  scoped_time_it time_it ("DWARF finalize worker",
				  m_state->m_per_command_time);
	  this_shard->finalize (parent_maps);
	});
    }

  finalizers.start ();
}

cooked_index::~cooked_index ()
{
  /* Wait for index-creation to be done, though this one must also
     waited for by the per-BFD object to ensure the required data
     remains live.  */
  wait (cooked_state::CACHE_DONE);

  /* Remove our entry from the global list.  See the assert in the
     constructor to understand this.  */
  gdb_assert (is_main_thread ());
  active_vectors.erase (this);
}

/* See cooked-index.h.  */

dwarf2_per_cu *
cooked_index::lookup (unrelocated_addr addr)
{
  /* Ensure that the address maps are ready.  */
  wait (cooked_state::MAIN_AVAILABLE, true);
  for (const auto &shard : m_shards)
    {
      dwarf2_per_cu *result = shard->lookup (addr);
      if (result != nullptr)
	return result;
    }
  return nullptr;
}

/* See cooked-index.h.  */

std::vector<const addrmap *>
cooked_index::get_addrmaps ()
{
  /* Ensure that the address maps are ready.  */
  wait (cooked_state::MAIN_AVAILABLE, true);
  std::vector<const addrmap *> result;
  for (const auto &shard : m_shards)
    result.push_back (shard->m_addrmap);
  return result;
}

/* See cooked-index.h.  */

cooked_index::range
cooked_index::find (const std::string &name, bool completing)
{
  wait (cooked_state::FINALIZED, true);
  std::vector<cooked_index_shard::range> result_range;
  result_range.reserve (m_shards.size ());
  for (auto &shard : m_shards)
    result_range.push_back (shard->find (name, completing));
  return range (std::move (result_range));
}

/* See cooked-index.h.  */

const char *
cooked_index::get_main_name (struct obstack *obstack, enum language *lang)
  const
{
  const cooked_index_entry *entry = get_main ();
  if (entry == nullptr)
    return nullptr;

  *lang = entry->lang;
  return entry->full_name (obstack, FOR_MAIN);
}

/* See cooked_index.h.  */

const cooked_index_entry *
cooked_index::get_main () const
{
  const cooked_index_entry *best_entry = nullptr;
  for (const auto &shard : m_shards)
    {
      const cooked_index_entry *entry = shard->get_main ();
      /* Choose the first "main" we see.  We only do this for names
	 not requiring canonicalization.  At this point in the process
	 names might not have been canonicalized.  However, currently,
	 languages that require this step also do not use
	 DW_AT_main_subprogram.  An assert is appropriate here because
	 this filtering is done in get_main.  */
      if (entry != nullptr)
	{
	  if ((entry->flags & IS_MAIN) != 0)
	    {
	      if (!language_requires_canonicalization (entry->lang))
		{
		  /* There won't be one better than this.  */
		  return entry;
		}
	    }
	  else
	    {
	      /* This is one that is named "main".  Here we don't care
		 if the language requires canonicalization, due to how
		 the entry is detected.  Entries like this have worse
		 priority than IS_MAIN entries.  */
	      if (best_entry == nullptr)
		best_entry = entry;
	    }
	}
    }

  return best_entry;
}

quick_symbol_functions_up
cooked_index::make_quick_functions () const
{
  return quick_symbol_functions_up (new cooked_index_functions);
}

/* See cooked-index.h.  */

void
cooked_index::dump (gdbarch *arch)
{
  auto_obstack temp_storage;

  gdb_printf ("  entries:\n");
  gdb_printf ("\n");

  size_t i = 0;
  for (const cooked_index_entry *entry : this->all_entries ())
    {
      QUIT;

      gdb_printf ("    [%zu] ((cooked_index_entry *) %p)\n", i++, entry);
      gdb_printf ("    name:       %s\n", entry->name);
      gdb_printf ("    canonical:  %s\n", entry->canonical);
      gdb_printf ("    qualified:  %s\n",
		  entry->full_name (&temp_storage, 0, "::"));
      gdb_printf ("    DWARF tag:  %s\n", dwarf_tag_name (entry->tag));
      gdb_printf ("    flags:      %s\n", to_string (entry->flags).c_str ());
      gdb_printf ("    DIE offset: %s\n", sect_offset_str (entry->die_offset));

      if ((entry->flags & IS_PARENT_DEFERRED) != 0)
	gdb_printf ("    parent:     deferred (%" PRIx64 ")\n",
		    entry->get_deferred_parent ());
      else if (entry->get_parent () != nullptr)
	gdb_printf ("    parent:     ((cooked_index_entry *) %p) [%s]\n",
		    entry->get_parent (), entry->get_parent ()->name);
      else
	gdb_printf ("    parent:     ((cooked_index_entry *) 0)\n");

      gdb_printf ("\n");
    }

  const cooked_index_entry *main_entry = this->get_main ();
  if (main_entry != nullptr)
    gdb_printf ("  main: ((cooked_index_entry *) %p) [%s]\n", main_entry,
		  main_entry->name);
  else
    gdb_printf ("  main: ((cooked_index_entry *) 0)\n");

  gdb_printf ("\n");
  gdb_printf ("  address maps:\n");
  gdb_printf ("\n");

  std::vector<const addrmap *> addrmaps = this->get_addrmaps ();
  for (i = 0; i < addrmaps.size (); ++i)
    {
      const addrmap *addrmap = addrmaps[i];

      gdb_printf ("    [%zu] ((addrmap *) %p)\n", i, addrmap);
      gdb_printf ("\n");

      if (addrmap == nullptr)
	continue;

      addrmap->foreach ([arch] (CORE_ADDR start_addr, const void *obj)
	{
	  QUIT;

	  const char *start_addr_str = paddress (arch, start_addr);

	  if (obj != nullptr)
	    {
	      const dwarf2_per_cu *per_cu
		= static_cast<const dwarf2_per_cu *> (obj);
	      gdb_printf ("      [%s] ((dwarf2_per_cu *) %p)\n",
			  start_addr_str, per_cu);
	    }
	  else
	    gdb_printf ("      [%s] ((dwarf2_per_cu *) 0)\n", start_addr_str);

	  return 0;
	});

      gdb_printf ("\n");
    }
}

/* Wait for all the index cache entries to be written before gdb
   exits.  */
static void
wait_for_index_cache (int)
{
  gdb_assert (is_main_thread ());
  for (cooked_index *item : active_vectors)
    item->wait_completely ();
}

/* A maint command to wait for the cache.  */

static void
maintenance_wait_for_index_cache (const char *args, int from_tty)
{
  wait_for_index_cache (0);
}

INIT_GDB_FILE (cooked_index)
{
  add_cmd ("wait-for-index-cache", class_maintenance,
	   maintenance_wait_for_index_cache, _("\
Wait until all pending writes to the index cache have completed.\n\
Usage: maintenance wait-for-index-cache"),
	   &maintenancelist);

  gdb::observers::gdb_exiting.attach (wait_for_index_cache, "cooked-index");
}
