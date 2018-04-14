# This shell script emits a C file. -*- C -*-
# It does some substitutions.
fragment <<EOF
/* Copyright (C) 1991-2016 Free Software Foundation, Inc.

   This file is part of the GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */


/* Emulate the Intel's port of gld.  */

#include "sysdep.h"
#include "bfd.h"
#include "libiberty.h"
#include "safe-ctype.h"
#include "bfdlink.h"

#include "ld.h"
#include "ldmisc.h"
#include "ldmain.h"

#include "ldexp.h"
#include "ldlang.h"
#include "ldfile.h"
#include "ldemul.h"

static void gld960_before_parse (void)
{
  char *env ;
  env =  getenv("G960LIB");
  if (env) {
    ldfile_add_library_path(env, FALSE);
  }
  env = getenv("G960BASE");
  if (env)
    ldfile_add_library_path (concat (env, "/lib", (const char *) NULL),
			     FALSE);
  ldfile_output_architecture = bfd_arch_i960;
}

static void
gld960_set_output_arch (void)
{
  if (ldfile_output_machine_name != NULL
      && *ldfile_output_machine_name != '\0')
    {
      char *s, *s1;

      s = concat ("i960:", ldfile_output_machine_name, (char *) NULL);
      for (s1 = s; *s1 != '\0'; s1++)
	*s1 = TOLOWER (*s1);
      ldfile_set_output_arch (s, bfd_arch_unknown);
      free (s);
    }

  set_output_arch_default ();
}

static char *
gld960_choose_target (int argc ATTRIBUTE_UNUSED, char **argv ATTRIBUTE_UNUSED)
{
  char *from_outside = getenv(TARGET_ENVIRON);
  output_filename = "b.out";

  if (from_outside != (char *)NULL)
    return from_outside;

  return "coff-Intel-little";
}

static char *
gld960_get_script (int *isfile)
EOF

if test x"$COMPILE_IN" = xyes
then
# Scripts compiled in.

# sed commands to quote an ld script as a C string.
sc="-f stringify.sed"

fragment <<EOF
{
  *isfile = 0;

  if (bfd_link_relocatable (&link_info) && config.build_constructors)
    return
EOF
sed $sc ldscripts/${EMULATION_NAME}.xu                 >> e${EMULATION_NAME}.c
echo '  ; else if (bfd_link_relocatable (&link_info)) return' >> e${EMULATION_NAME}.c
sed $sc ldscripts/${EMULATION_NAME}.xr                 >> e${EMULATION_NAME}.c
echo '  ; else if (!config.text_read_only) return'     >> e${EMULATION_NAME}.c
sed $sc ldscripts/${EMULATION_NAME}.xbn                >> e${EMULATION_NAME}.c
echo '  ; else if (!config.magic_demand_paged) return' >> e${EMULATION_NAME}.c
sed $sc ldscripts/${EMULATION_NAME}.xn                 >> e${EMULATION_NAME}.c
echo '  ; else return'                                 >> e${EMULATION_NAME}.c
sed $sc ldscripts/${EMULATION_NAME}.x                  >> e${EMULATION_NAME}.c
echo '; }'                                             >> e${EMULATION_NAME}.c

else
# Scripts read from the filesystem.

fragment <<EOF
{
  *isfile = 1;

  if (bfd_link_relocatable (&link_info) && config.build_constructors)
    return "ldscripts/${EMULATION_NAME}.xu";
  else if (bfd_link_relocatable (&link_info))
    return "ldscripts/${EMULATION_NAME}.xr";
  else if (!config.text_read_only)
    return "ldscripts/${EMULATION_NAME}.xbn";
  else if (!config.magic_demand_paged)
    return "ldscripts/${EMULATION_NAME}.xn";
  else
    return "ldscripts/${EMULATION_NAME}.x";
}
EOF

fi

fragment <<EOF

struct ld_emulation_xfer_struct ld_gld960coff_emulation =
{
  gld960_before_parse,
  syslib_default,
  hll_default,
  after_parse_default,
  after_open_default,
  after_allocation_default,
  gld960_set_output_arch,
  gld960_choose_target,
  before_allocation_default,
  gld960_get_script,
  "960coff",
  "",
  finish_default,
  NULL,	/* create output section statements */
  NULL,	/* open dynamic archive */
  NULL,	/* place orphan */
  NULL,	/* set symbols */
  NULL,	/* parse args */
  NULL,	/* add_options */
  NULL,	/* handle_option */
  NULL,	/* unrecognized file */
  NULL,	/* list options */
  NULL,	/* recognized file */
  NULL,	/* find_potential_libraries */
  NULL,	/* new_vers_pattern */
  NULL	/* extra_map_file_text */
};
EOF
