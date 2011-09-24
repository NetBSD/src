/* MI Command Set - target commands.
   Copyright (C) 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.

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

#include "defs.h"
#include "mi-cmds.h"
#include "mi-getopt.h"
#include "remote.h"

/* Get a file from the target.  */

void
mi_cmd_target_file_get (char *command, char **argv, int argc)
{
  int optind = 0;
  char *optarg;
  const char *remote_file, *local_file;
  static struct mi_opt opts[] =
  {
    { 0, 0, 0 }
  };
  static const char *prefix = "-target-file-get";

  if (mi_getopt (prefix, argc, argv, opts, &optind, &optarg) != -1
      || optind != argc - 2)
    error (_("-target-file-get: Usage: REMOTE_FILE LOCAL_FILE"));

  remote_file = argv[optind];
  local_file = argv[optind + 1];

  remote_file_get (remote_file, local_file, 0);
}

/* Send a file to the target.  */

void
mi_cmd_target_file_put (char *command, char **argv, int argc)
{
  int optind = 0;
  char *optarg;
  const char *remote_file, *local_file;
  static struct mi_opt opts[] =
  {
    { 0, 0, 0 }
  };
  static const char *prefix = "-target-file-put";

  if (mi_getopt (prefix, argc, argv, opts, &optind, &optarg) != -1
      || optind != argc - 2)
    error (_("-target-file-put: Usage: LOCAL_FILE REMOTE_FILE"));

  local_file = argv[optind];
  remote_file = argv[optind + 1];

  remote_file_put (local_file, remote_file, 0);
}

/* Delete a file on the target.  */

void
mi_cmd_target_file_delete (char *command, char **argv, int argc)
{
  int optind = 0;
  char *optarg;
  const char *remote_file;
  static struct mi_opt opts[] =
  {
    { 0, 0, 0 }
  };
  static const char *prefix = "-target-file-delete";

  if (mi_getopt (prefix, argc, argv, opts, &optind, &optarg) != -1
      || optind != argc - 1)
    error (_("-target-file-delete: Usage: REMOTE_FILE"));

  remote_file = argv[optind];

  remote_file_delete (remote_file, 0);
}

