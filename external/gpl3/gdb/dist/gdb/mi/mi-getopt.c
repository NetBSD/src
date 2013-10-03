/* MI Command Set - MI Option Parser.
   Copyright (C) 2000-2013 Free Software Foundation, Inc.
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

#include "defs.h"
#include "mi-getopt.h"
#include "gdb_string.h"

int
mi_getopt (const char *prefix,
	   int argc, char **argv,
	   const struct mi_opt *opts,
	   int *oind, char **oarg)
{
  char *arg;
  const struct mi_opt *opt;

  /* We assume that argv/argc are ok.  */
  if (*oind > argc || *oind < 0)
    internal_error (__FILE__, __LINE__,
		    _("mi_getopt_long: oind out of bounds"));
  if (*oind == argc)
    return -1;
  arg = argv[*oind];
  /* ``--''? */
  if (strcmp (arg, "--") == 0)
    {
      *oind += 1;
      *oarg = NULL;
      return -1;
    }
  /* End of option list.  */
  if (arg[0] != '-')
    {
      *oarg = NULL;
      return -1;
    }
  /* Look the option up.  */
  for (opt = opts; opt->name != NULL; opt++)
    {
      if (strcmp (opt->name, arg + 1) != 0)
	continue;
      if (opt->arg_p)
	{
	  /* A non-simple oarg option.  */
	  if (argc < *oind + 2)
	    error (_("%s: Option %s requires an argument"), prefix, arg);
	  *oarg = argv[(*oind) + 1];
	  *oind = (*oind) + 2;
	  return opt->index;
	}
      else
	{
	  *oarg = NULL;
	  *oind = (*oind) + 1;
	  return opt->index;
	}
    }
  error (_("%s: Unknown option ``%s''"), prefix, arg + 1);
}

int 
mi_valid_noargs (const char *prefix, int argc, char **argv) 
{
  int oind = 0;
  char *oarg;
  static const struct mi_opt opts[] =
    {
      { 0, 0, 0 }
    };

  if (mi_getopt (prefix, argc, argv, opts, &oind, &oarg) == -1)
    return 1;
  else
    return 0;
}
