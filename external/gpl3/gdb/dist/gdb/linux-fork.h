/* GNU/Linux native-dependent code for debugging multiple forks.

   Copyright (C) 2005-2025 Free Software Foundation, Inc.

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

#ifndef GDB_LINUX_FORK_H
#define GDB_LINUX_FORK_H

struct fork_info;
struct lwp_info;
class inferior;
extern void add_fork (pid_t, inferior *inf);
extern std::pair<fork_info *, inferior *> find_fork_pid (pid_t);
extern void linux_fork_killall (inferior *inf);
extern void linux_fork_mourn_inferior ();
extern void linux_fork_detach (int, lwp_info *, inferior *inf);
extern bool forks_exist_p (inferior *inf);
extern bool linux_fork_checkpointing_p (int);

#endif /* GDB_LINUX_FORK_H */
