/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016 Free Software Foundation, Inc.

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

#include <unistd.h>
#include <stdint.h>
#include <pthread.h>

struct jit_code_entry
{
  struct jit_code_entry *next_entry;
  struct jit_code_entry *prev_entry;
  const char *symfile_addr;
  uint64_t symfile_size;
};

struct jit_descriptor
{
  uint32_t version;
  /* This type should be jit_actions_t, but we use uint32_t
     to be explicit about the bitwidth.  */
  uint32_t action_flag;
  struct jit_code_entry *relevant_entry;
  struct jit_code_entry *first_entry;
};

struct jit_descriptor __jit_debug_descriptor = { 1, 0, 0, 0 };

void __jit_debug_register_code()
{
}

static void *
thread_proc (void *arg)
{
  sleep (60);
  return arg;
}

int
main (void)
{
  pthread_t thread;

  pthread_create (&thread, NULL, thread_proc, 0);
  pthread_join (thread, NULL);
  return 0;
}
