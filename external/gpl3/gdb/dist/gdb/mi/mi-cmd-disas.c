/* MI Command Set - disassemble commands.
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

#include "defs.h"
#include "arch-utils.h"
#include "target.h"
#include "value.h"
#include "mi-cmds.h"
#include "mi-getopt.h"
#include <string.h>
#include "ui-out.h"
#include "disasm.h"

/* The arguments to be passed on the command line and parsed here are
   either:

   START-ADDRESS: address to start the disassembly at.
   END-ADDRESS: address to end the disassembly at.

   or:

   FILENAME: The name of the file where we want disassemble from.
   LINE: The line around which we want to disassemble. It will
   disassemble the function that contins that line.
   HOW_MANY: Number of disassembly lines to display. With source, it
   is the number of disassembly lines only, not counting the source
   lines.  

   always required:

   MODE: 0 -- disassembly.
         1 -- disassembly and source.
         2 -- disassembly and opcodes.
         3 -- disassembly, source and opcodes.
*/

void
mi_cmd_disassemble (char *command, char **argv, int argc)
{
  struct gdbarch *gdbarch = get_current_arch ();
  struct ui_out *uiout = current_uiout;
  CORE_ADDR start;

  int mode, disasm_flags;
  struct symtab *s;

  /* Which options have we processed ... */
  int file_seen = 0;
  int line_seen = 0;
  int num_seen = 0;
  int start_seen = 0;
  int end_seen = 0;

  /* ... and their corresponding value. */
  char *file_string = NULL;
  int line_num = -1;
  int how_many = -1;
  CORE_ADDR low = 0;
  CORE_ADDR high = 0;
  struct cleanup *cleanups = make_cleanup (null_cleanup, NULL);

  /* Options processing stuff.  */
  int oind = 0;
  char *oarg;
  enum opt
  {
    FILE_OPT, LINE_OPT, NUM_OPT, START_OPT, END_OPT
  };
  static const struct mi_opt opts[] =
    {
      {"f", FILE_OPT, 1},
      {"l", LINE_OPT, 1},
      {"n", NUM_OPT, 1},
      {"s", START_OPT, 1},
      {"e", END_OPT, 1},
      { 0, 0, 0 }
    };

  /* Get the options with their arguments. Keep track of what we
     encountered.  */
  while (1)
    {
      int opt = mi_getopt ("-data-disassemble", argc, argv, opts,
			   &oind, &oarg);
      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case FILE_OPT:
	  file_string = xstrdup (oarg);
	  file_seen = 1;
	  make_cleanup (xfree, file_string);
	  break;
	case LINE_OPT:
	  line_num = atoi (oarg);
	  line_seen = 1;
	  break;
	case NUM_OPT:
	  how_many = atoi (oarg);
	  num_seen = 1;
	  break;
	case START_OPT:
	  low = parse_and_eval_address (oarg);
	  start_seen = 1;
	  break;
	case END_OPT:
	  high = parse_and_eval_address (oarg);
	  end_seen = 1;
	  break;
	}
    }
  argv += oind;
  argc -= oind;

  /* Allow only filename + linenum (with how_many which is not
     required) OR start_addr + end_addr.  */

  if (!((line_seen && file_seen && num_seen && !start_seen && !end_seen)
	|| (line_seen && file_seen && !num_seen && !start_seen && !end_seen)
	|| (!line_seen && !file_seen && !num_seen && start_seen && end_seen)))
    error (_("-data-disassemble: Usage: ( [-f filename -l linenum [-n "
	     "howmany]] | [-s startaddr -e endaddr]) [--] mode."));

  if (argc != 1)
    error (_("-data-disassemble: Usage: [-f filename -l linenum "
	     "[-n howmany]] [-s startaddr -e endaddr] [--] mode."));

  mode = atoi (argv[0]);
  if (mode < 0 || mode > 3)
    error (_("-data-disassemble: Mode argument must be 0, 1, 2, or 3."));

  /* Convert the mode into a set of disassembly flags.  */

  disasm_flags = 0;
  if (mode & 0x1)
    disasm_flags |= DISASSEMBLY_SOURCE;
  if (mode & 0x2)
    disasm_flags |= DISASSEMBLY_RAW_INSN;

  /* We must get the function beginning and end where line_num is
     contained.  */

  if (line_seen && file_seen)
    {
      s = lookup_symtab (file_string);
      if (s == NULL)
	error (_("-data-disassemble: Invalid filename."));
      if (!find_line_pc (s, line_num, &start))
	error (_("-data-disassemble: Invalid line number"));
      if (find_pc_partial_function (start, NULL, &low, &high) == 0)
	error (_("-data-disassemble: "
		 "No function contains specified address"));
    }

  gdb_disassembly (gdbarch, uiout,
  		   file_string,
  		   disasm_flags,
		   how_many, low, high);

  do_cleanups (cleanups);
}
