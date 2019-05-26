/* GNU/Linux native-dependent code for debugging multiple forks.

   Copyright (C) 2005-2017 Free Software Foundation, Inc.

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

struct fork_info;
extern struct fork_info *add_fork (pid_t);
extern struct fork_info *find_fork_pid (pid_t);
extern void linux_fork_killall (void);
extern void linux_fork_mourn_inferior (void);
extern void linux_fork_detach (const char *, int);
extern int forks_exist_p (void);
extern int linux_fork_checkpointing_p (int);
