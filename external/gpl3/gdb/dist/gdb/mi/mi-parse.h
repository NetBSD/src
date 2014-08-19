/* MI Command Set - MI Command Parser.
   Copyright (C) 2000-2014 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions (a Red Hat company).

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

#ifndef MI_PARSE_H
#define MI_PARSE_H

#include <sys/time.h>

/* MI parser */

/* Timestamps for current command and last asynchronous command.  */
struct mi_timestamp {
  struct timeval wallclock;
  struct timeval utime;
  struct timeval stime;
};

enum mi_command_type
  {
    MI_COMMAND, CLI_COMMAND
  };

struct mi_parse
  {
    enum mi_command_type op;
    char *command;
    char *token;
    const struct mi_cmd *cmd;
    struct mi_timestamp *cmd_start;
    char *args;
    char **argv;
    int argc;
    int all;
    int thread_group; /* At present, the same as inferior number.  */
    int thread;
    int frame;

    /* The language that should be used to evaluate the MI command.
       Ignored if set to language_unknown.  */
    enum language language;
  };

/* Attempts to parse CMD returning a ``struct mi_parse''.  If CMD is
   invalid, an exception is thrown.  For an MI_COMMAND COMMAND, ARGS
   and OP are initialized.  Un-initialized fields are zero.  *TOKEN is
   set to the token, even if an exception is thrown.  It is allocated
   with xmalloc; it must either be freed with xfree, or assigned to
   the TOKEN field of the resultant mi_parse object, to be freed by
   mi_parse_free.  */

extern struct mi_parse *mi_parse (const char *cmd, char **token);

/* Free a command returned by mi_parse_command.  */

extern void mi_parse_free (struct mi_parse *cmd);

/* Parse a string argument into a print_values value.  */

enum print_values mi_parse_print_values (const char *name);

#endif
