/* GNU/Linux native-dependent code for debugging multiple forks.

   Copyright 2005 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

struct fork_info;
extern struct fork_info *add_fork (pid_t);
extern struct fork_info *find_fork_pid (pid_t);
extern void fork_save_infrun_state (struct fork_info *, int);
extern void linux_fork_killall (void);
extern void linux_fork_mourn_inferior (void);
extern int  forks_exist_p (void);

struct fork_info *fork_list;

extern int detach_fork;

