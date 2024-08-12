/* Definitions for complaint handling during symbol reading in GDB.

   Copyright (C) 1990-2023 Free Software Foundation, Inc.

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


#if !defined (COMPLAINTS_H)
#define COMPLAINTS_H

#include <unordered_set>

/* Helper for complaint.  */
extern void complaint_internal (const char *fmt, ...)
  ATTRIBUTE_PRINTF (1, 2);

/* This controls whether complaints are emitted.  */

extern int stop_whining;

/* Return true if complaints are enabled.  This can be used to guard code
   that is used only to decide whether to issue a complaint.  */

static inline bool
have_complaint ()
{
  return stop_whining > 0;
}

/* Register a complaint.  This is a macro around complaint_internal to
   avoid computing complaint's arguments when complaints are disabled.
   Running FMT via gettext [i.e., _(FMT)] can be quite expensive, for
   example.  */
#define complaint(FMT, ...)					\
  do								\
    {								\
      if (have_complaint ())					\
	complaint_internal (FMT, ##__VA_ARGS__);		\
    }								\
  while (0)

/* Clear out / initialize all complaint counters that have ever been
   incremented.  */

extern void clear_complaints ();

/* A class that can handle calls to complaint from multiple threads.
   When this is instantiated, it hooks into the complaint mechanism,
   so the 'complaint' macro can continue to be used.  When it is
   destroyed, it issues all the complaints that have been stored.  It
   should only be instantiated in the main thread.  */

class complaint_interceptor
{
public:

  complaint_interceptor ();
  ~complaint_interceptor ();

  DISABLE_COPY_AND_ASSIGN (complaint_interceptor);

private:

  /* The issued complaints.  */
  std::unordered_set<std::string> m_complaints;

  /* The saved value of deprecated_warning_hook.  */
  void (*m_saved_warning_hook) (const char *, va_list)
    ATTRIBUTE_FPTR_PRINTF (1,0);

  /* A helper function that is used by the 'complaint' implementation
     to issue a complaint.  */
  static void issue_complaint (const char *, va_list)
    ATTRIBUTE_PRINTF (1, 0);

  /* This object.  Used by the static callback function.  */
  static complaint_interceptor *g_complaint_interceptor;
};

#endif /* !defined (COMPLAINTS_H) */
