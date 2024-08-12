/* Definitions for complaint handling during symbol reading in GDB.

   Copyright (C) 1990-2024 Free Software Foundation, Inc.

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

/* Type of collected complaints.  */

typedef std::unordered_set<std::string> complaint_collection;

/* A class that can handle calls to complaint from multiple threads.
   When this is instantiated, it hooks into the complaint mechanism,
   so the 'complaint' macro can continue to be used.  It collects all
   the complaints (and warnings) emitted while it is installed, and
   these can be retrieved before the object is destroyed.  This is
   intended for use from worker threads (though it will also work on
   the main thread).  Messages can be re-emitted on the main thread
   using re_emit_complaints, see below.  */

class complaint_interceptor final : public warning_hook_handler_type
{
public:

  complaint_interceptor ();
  ~complaint_interceptor () = default;

  DISABLE_COPY_AND_ASSIGN (complaint_interceptor);

  complaint_collection &&release ()
  {
    return std::move (m_complaints);
  }

private:

  /* The issued complaints.  */
  complaint_collection m_complaints;

  /* The saved value of g_complaint_interceptor.  */
  scoped_restore_tmpl<complaint_interceptor *> m_saved_complaint_interceptor;

  /* A helper function that is used by the 'complaint' implementation
     to issue a complaint.  */
  void warn (const char *, va_list) override
    ATTRIBUTE_PRINTF (2, 0);

  /* This object.  Used by the static callback function.  */
  static thread_local complaint_interceptor *g_complaint_interceptor;

  /* Object to initialise the warning hook.  */
  scoped_restore_warning_hook m_saved_warning_hook;
};

/* Re-emit complaints that were collected by complaint_interceptor.
   This can only be called on the main thread.  */

extern void re_emit_complaints (const complaint_collection &);

#endif /* !defined (COMPLAINTS_H) */
