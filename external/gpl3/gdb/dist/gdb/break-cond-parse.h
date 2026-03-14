/* Copyright (C) 2023-2025 Free Software Foundation, Inc.

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

#ifndef GDB_BREAK_COND_PARSE_H
#define GDB_BREAK_COND_PARSE_H

/* Given TOK, a string possibly containing a condition, thread, inferior,
   task and force-condition flag, as accepted by the 'break' command,
   extract the condition string, thread, inferior, task number, and the
   force_condition flag, then set *COND_STRING, *THREAD, *INFERIOR, *TASK,
   and *FORCE.

   As TOK is parsed, if an unknown keyword is encountered before the 'if'
   keyword then everything starting from the unknown keyword is placed into
   *REST.

   Both *COND and *REST are initialized to nullptr.  If no 'if' keyword is
   found then *COND will be returned as nullptr.  If no unknown content is
   found then *REST is returned as nullptr.

   If no thread is found, *THREAD is set to -1.  If no inferior is found,
   *INFERIOR is set to -1.  If no task is found, *TASK is set to -1.  If
   the -force-condition flag is not found then *FORCE is set to false.

   Due to the free-form nature that the string TOK might take (a 'thread'
   keyword can appear before or after an 'if' condition) then we end up
   having to check for keywords from both the start of TOK and the end of
   TOK.

   If TOK is nullptr, or TOK is the empty string, then the output variables
   are all given their default values.  */

extern void create_breakpoint_parse_arg_string
  (const char *tok, gdb::unique_xmalloc_ptr<char> *cond_string,
   int *thread, int *inferior, int *task,
   gdb::unique_xmalloc_ptr<char> *rest, bool *force);

#endif /* GDB_BREAK_COND_PARSE_H */
