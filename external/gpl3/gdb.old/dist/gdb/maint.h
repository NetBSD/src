/* Support for GDB maintenance commands.
   Copyright (C) 2013-2017 Free Software Foundation, Inc.

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

#ifndef MAINT_H
#define MAINT_H

#include "run-time-clock.h"
#include <chrono>

extern void set_per_command_time (int);

extern void set_per_command_space (int);

/* Records a run time and space usage to be used as a base for
   reporting elapsed time or change in space.  */

class scoped_command_stats
{
 public:

  explicit scoped_command_stats (bool msg_type);
  ~scoped_command_stats ();

 private:

  // No need for these.  They are intentionally not defined anywhere.
  scoped_command_stats &operator= (const scoped_command_stats &);
  scoped_command_stats (const scoped_command_stats &);

  /* Zero if the saved time is from the beginning of GDB execution.
     One if from the beginning of an individual command execution.  */
  bool m_msg_type;
  /* Track whether the stat was enabled at the start of the command
     so that we can avoid printing anything if it gets turned on by
     the current command.  */
  int m_time_enabled : 1;
  int m_space_enabled : 1;
  int m_symtab_enabled : 1;
  run_time_clock::time_point m_start_cpu_time;
  std::chrono::steady_clock::time_point m_start_wall_time;
  long m_start_space;
  /* Total number of symtabs (over all objfiles).  */
  int m_start_nr_symtabs;
  /* A count of the compunits.  */
  int m_start_nr_compunit_symtabs;
  /* Total number of blocks.  */
  int m_start_nr_blocks;
};

#endif /* MAINT_H */
