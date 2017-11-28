/* Disable address space randomization based on inferior personality.

   Copyright (C) 2008-2017 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include "nat/linux-personality.h"

#ifdef HAVE_PERSONALITY
# include <sys/personality.h>
# if !HAVE_DECL_ADDR_NO_RANDOMIZE
#  define ADDR_NO_RANDOMIZE 0x0040000
# endif /* ! HAVE_DECL_ADDR_NO_RANDOMIZE */
#endif /* HAVE_PERSONALITY */

#ifdef HAVE_PERSONALITY

/* Restore address space randomization of the inferior.  ARG is the
   original inferior's personality value before the address space
   randomization was disabled.  */

static void
restore_personality (void *arg)
{
  int personality_orig = (int) (uintptr_t) arg;

  errno = 0;
  personality (personality_orig);
  if (errno != 0)
    warning (_("Error restoring address space randomization: %s"),
	     safe_strerror (errno));
}
#endif /* HAVE_PERSONALITY */

/* Return a cleanup responsible for restoring the inferior's
   personality (and restoring the inferior's address space
   randomization) if HAVE_PERSONALITY and if PERSONALITY_SET is not
   zero; return a null cleanup if not HAVE_PERSONALITY or if
   PERSONALITY_SET is zero.  */

static struct cleanup *
make_disable_asr_cleanup (int personality_set, int personality_orig)
{
#ifdef HAVE_PERSONALITY
  if (personality_set != 0)
    return make_cleanup (restore_personality,
			 (void *) (uintptr_t) personality_orig);
#endif /* HAVE_PERSONALITY */

  return make_cleanup (null_cleanup, NULL);
}

/* See comment on nat/linux-personality.h.  */

struct cleanup *
maybe_disable_address_space_randomization (int disable_randomization)
{
  int personality_orig = 0;
  int personality_set = 0;

#ifdef HAVE_PERSONALITY
  if (disable_randomization)
    {
      errno = 0;
      personality_orig = personality (0xffffffff);
      if (errno == 0 && !(personality_orig & ADDR_NO_RANDOMIZE))
	{
	  personality_set = 1;
	  personality (personality_orig | ADDR_NO_RANDOMIZE);
	}
      if (errno != 0 || (personality_set
			 && !(personality (0xffffffff) & ADDR_NO_RANDOMIZE)))
	warning (_("Error disabling address space randomization: %s"),
		 safe_strerror (errno));
    }
#endif /* HAVE_PERSONALITY */

  return make_disable_asr_cleanup (personality_set,
				   personality_orig);
}
