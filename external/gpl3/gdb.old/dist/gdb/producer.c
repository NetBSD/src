/* Producer string parsers for GDB.

   Copyright (C) 2012-2019 Free Software Foundation, Inc.

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
#include "producer.h"
#include "common/selftest.h"

/* See producer.h.  */

int
producer_is_gcc_ge_4 (const char *producer)
{
  int major, minor;

  if (! producer_is_gcc (producer, &major, &minor))
    return -1;
  if (major < 4)
    return -1;
  if (major > 4)
    return INT_MAX;
  return minor;
}

/* See producer.h.  */

int
producer_is_gcc (const char *producer, int *major, int *minor)
{
  const char *cs;

  if (producer != NULL && startswith (producer, "GNU "))
    {
      int maj, min;

      if (major == NULL)
	major = &maj;
      if (minor == NULL)
	minor = &min;

      /* Skip any identifier after "GNU " - such as "C11" "C++" or "Java".
	 A full producer string might look like:
	 "GNU C 4.7.2"
	 "GNU Fortran 4.8.2 20140120 (Red Hat 4.8.2-16) -mtune=generic ..."
	 "GNU C++14 5.0.0 20150123 (experimental)"
      */
      cs = &producer[strlen ("GNU ")];
      while (*cs && !isspace (*cs))
        cs++;
      if (*cs && isspace (*cs))
        cs++;
      if (sscanf (cs, "%d.%d", major, minor) == 2)
	return 1;
    }

  /* Not recognized as GCC.  */
  return 0;
}


/* See producer.h.  */

bool
producer_is_icc (const char *producer, int *major, int *minor)
{
  if (producer == NULL || !startswith (producer, "Intel(R)"))
    return false;

  /* Prepare the used fields.  */
  int maj, min;
  if (major == NULL)
    major = &maj;
  if (minor == NULL)
    minor = &min;

  *minor = 0;
  *major = 0;

  /* Consumes the string till a "Version" is found.  */
  const char *cs = strstr (producer, "Version");
  if (cs != NULL)
    {
      cs = skip_to_space (cs);

      int intermediate = 0;
      int nof = sscanf (cs, "%d.%d.%d.%*d", major, &intermediate, minor);

      /* Internal versions are represented only as MAJOR.MINOR, where
	 minor is usually 0.
	 Public versions have 3 fields as described with the command
	 above.  */
      if (nof == 3)
	return true;

      if (nof == 2)
	{
	  *minor = intermediate;
	  return true;
	}
    }

  static bool warning_printed = false;
  /* Not recognized as Intel, let the user know.  */
  if (!warning_printed)
    {
      warning (_("Could not recognize version of Intel Compiler in: \"%s\""),
	       producer);
      warning_printed = true;
    }
  return false;
}

#if defined GDB_SELF_TEST
namespace selftests {
namespace producer {

static void
producer_parsing_tests ()
{
  {
    /* Check that we don't crash if "Version" is not found in what
       looks like an ICC producer string.  */
    static const char icc_no_version[] = "Intel(R) foo bar";

    int major = 0, minor = 0;
    SELF_CHECK (!producer_is_icc (icc_no_version, &major, &minor));
    SELF_CHECK (!producer_is_gcc (icc_no_version, &major, &minor));
  }

  {
    static const char extern_f_14_1[] = "\
Intel(R) Fortran Intel(R) 64 Compiler XE for applications running on \
Intel(R) 64, \
Version 14.0.1.074 Build 20130716";

    int major = 0, minor = 0;
    SELF_CHECK (producer_is_icc (extern_f_14_1, &major, &minor)
		&& major == 14 && minor == 1);
    SELF_CHECK (!producer_is_gcc (extern_f_14_1, &major, &minor));
  }

  {
    static const char intern_f_14[] = "\
Intel(R) Fortran Intel(R) 64 Compiler XE for applications running on \
Intel(R) 64, \
Version 14.0";

    int major = 0, minor = 0;
    SELF_CHECK (producer_is_icc (intern_f_14, &major, &minor)
		&& major == 14 && minor == 0);
    SELF_CHECK (!producer_is_gcc (intern_f_14, &major, &minor));
  }

  {
    static const char intern_c_14[] = "\
Intel(R) C++ Intel(R) 64 Compiler XE for applications running on \
Intel(R) 64, \
Version 14.0";
    int major = 0, minor = 0;
    SELF_CHECK (producer_is_icc (intern_c_14, &major, &minor)
		&& major == 14 && minor == 0);
    SELF_CHECK (!producer_is_gcc (intern_c_14, &major, &minor));
  }

  {
    static const char intern_c_18[] = "\
Intel(R) C++ Intel(R) 64 Compiler for applications running on \
Intel(R) 64, \
Version 18.0 Beta";
    int major = 0, minor = 0;
    SELF_CHECK (producer_is_icc (intern_c_18, &major, &minor)
		&& major == 18 && minor == 0);
  }

  {
    static const char gnu[] = "GNU C 4.7.2";
    SELF_CHECK (!producer_is_icc (gnu, NULL, NULL));

    int major = 0, minor = 0;
    SELF_CHECK (producer_is_gcc (gnu, &major, &minor)
		&& major == 4 && minor == 7);
  }

  {
    static const char gnu_exp[] = "GNU C++14 5.0.0 20150123 (experimental)";
    int major = 0, minor = 0;
    SELF_CHECK (!producer_is_icc (gnu_exp, NULL, NULL));
    SELF_CHECK (producer_is_gcc (gnu_exp, &major, &minor)
		&& major == 5 && minor == 0);
  }
}
}
}
#endif

void
_initialize_producer ()
{
#if defined GDB_SELF_TEST
  selftests::register_test
    ("producer-parser", selftests::producer::producer_parsing_tests);
#endif
}
