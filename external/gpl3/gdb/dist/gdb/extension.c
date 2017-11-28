/* Interface between gdb and its extension languages.

   Copyright (C) 2014-2017 Free Software Foundation, Inc.

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

/* Note: With few exceptions, external functions and variables in this file
   have "ext_lang" in the name, and no other symbol in gdb does.  */

#include "defs.h"
#include <signal.h>
#include "target.h"
#include "auto-load.h"
#include "breakpoint.h"
#include "event-top.h"
#include "extension.h"
#include "extension-priv.h"
#include "observer.h"
#include "cli/cli-script.h"
#include "python/python.h"
#include "guile/guile.h"

/* Iterate over all external extension languages, regardless of whether the
   support has been compiled in or not.
   This does not include GDB's own scripting language.  */

#define ALL_EXTENSION_LANGUAGES(i, extlang) \
  for (/*int*/ i = 0, extlang = extension_languages[0]; \
       extlang != NULL; \
       extlang = extension_languages[++i])

/* Iterate over all external extension languages that are supported.
   This does not include GDB's own scripting language.  */

#define ALL_ENABLED_EXTENSION_LANGUAGES(i, extlang) \
  for (/*int*/ i = 0, extlang = extension_languages[0]; \
       extlang != NULL; \
       extlang = extension_languages[++i]) \
    if (extlang->ops != NULL)

static script_sourcer_func source_gdb_script;
static objfile_script_sourcer_func source_gdb_objfile_script;

/* GDB's own scripting language.
   This exists, in part, to support auto-loading ${prog}-gdb.gdb scripts.  */

static const struct extension_language_script_ops
  extension_language_gdb_script_ops =
{
  source_gdb_script,
  source_gdb_objfile_script,
  NULL, /* objfile_script_executor */
  auto_load_gdb_scripts_enabled
};

const struct extension_language_defn extension_language_gdb =
{
  EXT_LANG_GDB,
  "gdb",
  "GDB",

  /* We fall back to interpreting a script as a GDB script if it doesn't
     match the other scripting languages, but for consistency's sake
     give it a formal suffix.  */
  ".gdb",
  "-gdb.gdb",

  /* cli_control_type: This is never used: GDB's own scripting language
     has a variety of control types (if, while, etc.).  */
  commands_control,

  &extension_language_gdb_script_ops,

  /* The rest of the extension language interface isn't supported by GDB's own
     extension/scripting language.  */
  NULL
};

/* NULL-terminated table of all external (non-native) extension languages.

   The order of appearance in the table is important.
   When multiple extension languages provide the same feature, for example
   a pretty-printer for a particular type, which one gets used?
   The algorithm employed here is "the first one wins".  For example, in
   the case of pretty-printers this means the first one to provide a
   pretty-printed value is the one that is used.  This algorithm is employed
   throughout.  */

static const struct extension_language_defn * const extension_languages[] =
{
  /* To preserve existing behaviour, python should always appear first.  */
  &extension_language_python,
  &extension_language_guile,
  NULL
};

/* Return a pointer to the struct extension_language_defn object of
   extension language LANG.
   This always returns a non-NULL pointer, even if support for the language
   is not compiled into this copy of GDB.  */

const struct extension_language_defn *
get_ext_lang_defn (enum extension_language lang)
{
  int i;
  const struct extension_language_defn *extlang;

  gdb_assert (lang != EXT_LANG_NONE);

  if (lang == EXT_LANG_GDB)
    return &extension_language_gdb;

  ALL_EXTENSION_LANGUAGES (i, extlang)
    {
      if (extlang->language == lang)
	return extlang;
    }

  gdb_assert_not_reached ("unable to find extension_language_defn");
}

/* Return TRUE if FILE has extension EXTENSION.  */

static int
has_extension (const char *file, const char *extension)
{
  int file_len = strlen (file);
  int extension_len = strlen (extension);

  return (file_len > extension_len
	  && strcmp (&file[file_len - extension_len], extension) == 0);
}

/* Return the extension language of FILE, or NULL if
   the extension language of FILE is not recognized.
   This is done by looking at the file's suffix.  */

const struct extension_language_defn *
get_ext_lang_of_file (const char *file)
{
  int i;
  const struct extension_language_defn *extlang;

  ALL_EXTENSION_LANGUAGES (i, extlang)
    {
      if (has_extension (file, extlang->suffix))
	return extlang;
    }

  return NULL;
}

/* Return non-zero if support for the specified extension language
   is compiled in.  */

int
ext_lang_present_p (const struct extension_language_defn *extlang)
{
  return extlang->script_ops != NULL;
}

/* Return non-zero if the specified extension language has successfully
   initialized.  */

int
ext_lang_initialized_p (const struct extension_language_defn *extlang)
{
  if (extlang->ops != NULL)
    {
      /* This method is required.  */
      gdb_assert (extlang->ops->initialized != NULL);
      return extlang->ops->initialized (extlang);
    }

  return 0;
}

/* Throw an error indicating EXTLANG is not supported in this copy of GDB.  */

void
throw_ext_lang_unsupported (const struct extension_language_defn *extlang)
{
  error (_("Scripting in the \"%s\" language is not supported"
	   " in this copy of GDB."),
	 ext_lang_capitalized_name (extlang));
}

/* Methods for GDB's own extension/scripting language.  */

/* The extension_language_script_ops.script_sourcer "method".  */

static void
source_gdb_script (const struct extension_language_defn *extlang,
		   FILE *stream, const char *file)
{
  script_from_file (stream, file);
}

/* The extension_language_script_ops.objfile_script_sourcer "method".  */

static void
source_gdb_objfile_script (const struct extension_language_defn *extlang,
			   struct objfile *objfile,
			   FILE *stream, const char *file)
{
  script_from_file (stream, file);
}

/* Accessors for "public" attributes of struct extension_language.  */

/* Return the "name" field of EXTLANG.  */

const char *
ext_lang_name (const struct extension_language_defn *extlang)
{
  return extlang->name;
}

/* Return the "capitalized_name" field of EXTLANG.  */

const char *
ext_lang_capitalized_name (const struct extension_language_defn *extlang)
{
  return extlang->capitalized_name;
}

/* Return the "suffix" field of EXTLANG.  */

const char *
ext_lang_suffix (const struct extension_language_defn *extlang)
{
  return extlang->suffix;
}

/* Return the "auto_load_suffix" field of EXTLANG.  */

const char *
ext_lang_auto_load_suffix (const struct extension_language_defn *extlang)
{
  return extlang->auto_load_suffix;
}

/* extension_language_script_ops wrappers.  */

/* Return the script "sourcer" function for EXTLANG.
   This is the function that loads and processes a script.
   If support for this language isn't compiled in, NULL is returned.  */

script_sourcer_func *
ext_lang_script_sourcer (const struct extension_language_defn *extlang)
{
  if (extlang->script_ops == NULL)
    return NULL;

  /* The extension language is required to implement this function.  */
  gdb_assert (extlang->script_ops->script_sourcer != NULL);

  return extlang->script_ops->script_sourcer;
}

/* Return the objfile script "sourcer" function for EXTLANG.
   This is the function that loads and processes a script for a particular
   objfile.
   If support for this language isn't compiled in, NULL is returned.  */

objfile_script_sourcer_func *
ext_lang_objfile_script_sourcer (const struct extension_language_defn *extlang)
{
  if (extlang->script_ops == NULL)
    return NULL;

  /* The extension language is required to implement this function.  */
  gdb_assert (extlang->script_ops->objfile_script_sourcer != NULL);

  return extlang->script_ops->objfile_script_sourcer;
}

/* Return the objfile script "executor" function for EXTLANG.
   This is the function that executes a script for a particular objfile.
   If support for this language isn't compiled in, NULL is returned.
   The extension language is not required to implement this function.  */

objfile_script_executor_func *
ext_lang_objfile_script_executor
  (const struct extension_language_defn *extlang)
{
  if (extlang->script_ops == NULL)
    return NULL;

  return extlang->script_ops->objfile_script_executor;
}

/* Return non-zero if auto-loading of EXTLANG scripts is enabled.
   Zero is returned if support for this language isn't compiled in.  */

int
ext_lang_auto_load_enabled (const struct extension_language_defn *extlang)
{
  if (extlang->script_ops == NULL)
    return 0;

  /* The extension language is required to implement this function.  */
  gdb_assert (extlang->script_ops->auto_load_enabled != NULL);

  return extlang->script_ops->auto_load_enabled (extlang);
}

/* Functions that iterate over all extension languages.
   These only iterate over external extension languages, not including
   GDB's own extension/scripting language, unless otherwise indicated.  */

/* Wrapper to call the extension_language_ops.finish_initialization "method"
   for each compiled-in extension language.  */

void
finish_ext_lang_initialization (void)
{
  int i;
  const struct extension_language_defn *extlang;

  ALL_ENABLED_EXTENSION_LANGUAGES (i, extlang)
    {
      if (extlang->ops->finish_initialization != NULL)
	extlang->ops->finish_initialization (extlang);
    }
}

/* Invoke the appropriate extension_language_ops.eval_from_control_command
   method to perform CMD, which is a list of commands in an extension language.

   This function is what implements, for example:

   python
   print 42
   end

   in a GDB script.  */

void
eval_ext_lang_from_control_command (struct command_line *cmd)
{
  int i;
  const struct extension_language_defn *extlang;

  ALL_EXTENSION_LANGUAGES (i, extlang)
    {
      if (extlang->cli_control_type == cmd->control_type)
	{
	  if (extlang->ops != NULL
	      && extlang->ops->eval_from_control_command != NULL)
	    {
	      extlang->ops->eval_from_control_command (extlang, cmd);
	      return;
	    }
	  /* The requested extension language is not supported in this GDB.  */
	  throw_ext_lang_unsupported (extlang);
	}
    }

  gdb_assert_not_reached ("unknown extension language in command_line");
}

/* Search for and load scripts for OBJFILE written in extension languages.
   This includes GDB's own scripting language.

   This function is what implements the loading of OBJFILE-gdb.py and
   OBJFILE-gdb.gdb.  */

void
auto_load_ext_lang_scripts_for_objfile (struct objfile *objfile)
{
  int i;
  const struct extension_language_defn *extlang;

  extlang = &extension_language_gdb;
  if (ext_lang_auto_load_enabled (extlang))
    auto_load_objfile_script (objfile, extlang);

  ALL_ENABLED_EXTENSION_LANGUAGES (i, extlang)
    {
      if (ext_lang_auto_load_enabled (extlang))
	auto_load_objfile_script (objfile, extlang);
    }
}

/* Interface to type pretty-printers implemented in an extension language.  */

/* Call this at the start when preparing to pretty-print a type.
   The result is a pointer to an opaque object (to the caller) to be passed
   to apply_ext_lang_type_printers and free_ext_lang_type_printers.

   We don't know in advance which extension language will provide a
   pretty-printer for the type, so all are initialized.  */

struct ext_lang_type_printers *
start_ext_lang_type_printers (void)
{
  struct ext_lang_type_printers *printers
    = XCNEW (struct ext_lang_type_printers);
  int i;
  const struct extension_language_defn *extlang;

  ALL_ENABLED_EXTENSION_LANGUAGES (i, extlang)
    {
      if (extlang->ops->start_type_printers != NULL)
	extlang->ops->start_type_printers (extlang, printers);
    }

  return printers;
}

/* Iteratively try the type pretty-printers specified by PRINTERS
   according to the standard search order (specified by extension_languages),
   returning the result of the first one that succeeds.
   If there was an error, or if no printer succeeds, then NULL is returned.  */

char *
apply_ext_lang_type_printers (struct ext_lang_type_printers *printers,
			      struct type *type)
{
  int i;
  const struct extension_language_defn *extlang;

  ALL_ENABLED_EXTENSION_LANGUAGES (i, extlang)
    {
      char *result = NULL;
      enum ext_lang_rc rc;

      if (extlang->ops->apply_type_printers == NULL)
	continue;
      rc = extlang->ops->apply_type_printers (extlang, printers, type,
					      &result);
      switch (rc)
	{
	case EXT_LANG_RC_OK:
	  gdb_assert (result != NULL);
	  return result;
	case EXT_LANG_RC_ERROR:
	  return NULL;
	case EXT_LANG_RC_NOP:
	  break;
	default:
	  gdb_assert_not_reached ("bad return from apply_type_printers");
	}
    }

  return NULL;
}

/* Call this after pretty-printing a type to release all memory held
   by PRINTERS.  */

void
free_ext_lang_type_printers (struct ext_lang_type_printers *printers)
{
  int i;
  const struct extension_language_defn *extlang;

  ALL_ENABLED_EXTENSION_LANGUAGES (i, extlang)
    {
      if (extlang->ops->free_type_printers != NULL)
	extlang->ops->free_type_printers (extlang, printers);
    }

  xfree (printers);
}

/* Try to pretty-print a value of type TYPE located at VAL's contents
   buffer + EMBEDDED_OFFSET, which came from the inferior at address
   ADDRESS + EMBEDDED_OFFSET, onto stdio stream STREAM according to
   OPTIONS.
   VAL is the whole object that came from ADDRESS.
   Returns non-zero if the value was successfully pretty-printed.

   Extension languages are tried in the order specified by
   extension_languages.  The first one to provide a pretty-printed
   value "wins".

   If an error is encountered in a pretty-printer, no further extension
   languages are tried.
   Note: This is different than encountering a memory error trying to read a
   value for pretty-printing.  Here we're referring to, e.g., programming
   errors that trigger an exception in the extension language.  */

int
apply_ext_lang_val_pretty_printer (struct type *type,
				   LONGEST embedded_offset, CORE_ADDR address,
				   struct ui_file *stream, int recurse,
				   struct value *val,
				   const struct value_print_options *options,
				   const struct language_defn *language)
{
  int i;
  const struct extension_language_defn *extlang;

  ALL_ENABLED_EXTENSION_LANGUAGES (i, extlang)
    {
      enum ext_lang_rc rc;

      if (extlang->ops->apply_val_pretty_printer == NULL)
	continue;
      rc = extlang->ops->apply_val_pretty_printer (extlang, type,
						   embedded_offset, address,
						   stream, recurse, val,
						   options, language);
      switch (rc)
	{
	case EXT_LANG_RC_OK:
	  return 1;
	case EXT_LANG_RC_ERROR:
	  return 0;
	case EXT_LANG_RC_NOP:
	  break;
	default:
	  gdb_assert_not_reached ("bad return from apply_val_pretty_printer");
	}
    }

  return 0;
}

/* GDB access to the "frame filter" feature.
   FRAME is the source frame to start frame-filter invocation.  FLAGS is an
   integer holding the flags for printing.  The following elements of
   the FRAME_FILTER_FLAGS enum denotes the make-up of FLAGS:
   PRINT_LEVEL is a flag indicating whether to print the frame's
   relative level in the output.  PRINT_FRAME_INFO is a flag that
   indicates whether this function should print the frame
   information, PRINT_ARGS is a flag that indicates whether to print
   frame arguments, and PRINT_LOCALS, likewise, with frame local
   variables.  ARGS_TYPE is an enumerator describing the argument
   format, OUT is the output stream to print.  FRAME_LOW is the
   beginning of the slice of frames to print, and FRAME_HIGH is the
   upper limit of the frames to count.  Returns EXT_LANG_BT_ERROR on error,
   or EXT_LANG_BT_COMPLETED on success.

   Extension languages are tried in the order specified by
   extension_languages.  The first one to provide a filter "wins".
   If there is an error (EXT_LANG_BT_ERROR) it is reported immediately
   rather than trying filters in other extension languages.  */

enum ext_lang_bt_status
apply_ext_lang_frame_filter (struct frame_info *frame, int flags,
			     enum ext_lang_frame_args args_type,
			     struct ui_out *out,
			     int frame_low, int frame_high)
{
  int i;
  const struct extension_language_defn *extlang;

  ALL_ENABLED_EXTENSION_LANGUAGES (i, extlang)
    {
      enum ext_lang_bt_status status;

      if (extlang->ops->apply_frame_filter == NULL)
	continue;
      status = extlang->ops->apply_frame_filter (extlang, frame, flags,
					       args_type, out,
					       frame_low, frame_high);
      /* We use the filters from the first extension language that has
	 applicable filters.  Also, an error is reported immediately
	 rather than continue trying.  */
      if (status != EXT_LANG_BT_NO_FILTERS)
	return status;
    }

  return EXT_LANG_BT_NO_FILTERS;
}

/* Update values held by the extension language when OBJFILE is discarded.
   New global types must be created for every such value, which must then be
   updated to use the new types.
   The function typically just iterates over all appropriate values and
   calls preserve_one_value for each one.
   COPIED_TYPES is used to prevent cycles / duplicates and is passed to
   preserve_one_value.  */

void
preserve_ext_lang_values (struct objfile *objfile, htab_t copied_types)
{
  int i;
  const struct extension_language_defn *extlang;

  ALL_ENABLED_EXTENSION_LANGUAGES (i, extlang)
    {
      if (extlang->ops->preserve_values != NULL)
	extlang->ops->preserve_values (extlang, objfile, copied_types);
    }
}

/* If there is a stop condition implemented in an extension language for
   breakpoint B, return a pointer to the extension language's definition.
   Otherwise return NULL.
   If SKIP_LANG is not EXT_LANG_NONE, skip checking this language.
   This is for the case where we're setting a new condition: Only one
   condition is allowed, so when setting a condition for any particular
   extension language, we need to check if any other extension language
   already has a condition set.  */

const struct extension_language_defn *
get_breakpoint_cond_ext_lang (struct breakpoint *b,
			      enum extension_language skip_lang)
{
  int i;
  const struct extension_language_defn *extlang;

  ALL_ENABLED_EXTENSION_LANGUAGES (i, extlang)
    {
      if (extlang->language != skip_lang
	  && extlang->ops->breakpoint_has_cond != NULL
	  && extlang->ops->breakpoint_has_cond (extlang, b))
	return extlang;
    }

  return NULL;
}

/* Return whether a stop condition for breakpoint B says to stop.
   True is also returned if there is no stop condition for B.  */

int
breakpoint_ext_lang_cond_says_stop (struct breakpoint *b)
{
  int i;
  const struct extension_language_defn *extlang;
  enum ext_lang_bp_stop stop = EXT_LANG_BP_STOP_UNSET;

  ALL_ENABLED_EXTENSION_LANGUAGES (i, extlang)
    {
      /* There is a rule that a breakpoint can have at most one of any of a
	 CLI or extension language condition.  However, Python hacks in "finish
	 breakpoints" on top of the "stop" check, so we have to call this for
	 every language, even if we could first determine whether a "stop"
	 method exists.  */
      if (extlang->ops->breakpoint_cond_says_stop != NULL)
	{
	  enum ext_lang_bp_stop this_stop
	    = extlang->ops->breakpoint_cond_says_stop (extlang, b);

	  if (this_stop != EXT_LANG_BP_STOP_UNSET)
	    {
	      /* Even though we have to check every extension language, only
		 one of them can return yes/no (because only one of them
		 can have a "stop" condition).  */
	      gdb_assert (stop == EXT_LANG_BP_STOP_UNSET);
	      stop = this_stop;
	    }
	}
    }

  return stop == EXT_LANG_BP_STOP_NO ? 0 : 1;
}

/* ^C/SIGINT support.
   This requires cooperation with the extension languages so the support
   is defined here.  */

/* This flag tracks quit requests when we haven't called out to an
   extension language.  it also holds quit requests when we transition to
   an extension language that doesn't have cooperative SIGINT handling.  */
static int quit_flag;

/* The current extension language we've called out to, or
   extension_language_gdb if there isn't one.
   This must be set everytime we call out to an extension language, and reset
   to the previous value when it returns.  Note that the previous value may
   be a different (or the same) extension language.  */
static const struct extension_language_defn *active_ext_lang
  = &extension_language_gdb;

/* Return the currently active extension language.  */

const struct extension_language_defn *
get_active_ext_lang (void)
{
  return active_ext_lang;
}

/* Install a SIGINT handler.  */

static void
install_sigint_handler (const struct signal_handler *handler_state)
{
  gdb_assert (handler_state->handler_saved);

  signal (SIGINT, handler_state->handler);
}

/* Install GDB's SIGINT handler, storing the previous version in *PREVIOUS.
   As a simple optimization, if the previous version was GDB's SIGINT handler
   then mark the previous handler as not having been saved, and thus it won't
   be restored.  */

static void
install_gdb_sigint_handler (struct signal_handler *previous)
{
  /* Save here to simplify comparison.  */
  sighandler_t handle_sigint_for_compare = handle_sigint;

  previous->handler = signal (SIGINT, handle_sigint);
  if (previous->handler != handle_sigint_for_compare)
    previous->handler_saved = 1;
  else
    previous->handler_saved = 0;
}

/* Set the currently active extension language to NOW_ACTIVE.
   The result is a pointer to a malloc'd block of memory to pass to
   restore_active_ext_lang.

   N.B. This function must be called every time we call out to an extension
   language, and the result must be passed to restore_active_ext_lang
   afterwards.

   If there is a pending SIGINT it is "moved" to the now active extension
   language, if it supports cooperative SIGINT handling (i.e., it provides
   {clear,set,check}_quit_flag methods).  If the extension language does not
   support cooperative SIGINT handling, then the SIGINT is left queued and
   we require the non-cooperative extension language to call check_quit_flag
   at appropriate times.
   It is important for the extension language to call check_quit_flag if it
   installs its own SIGINT handler to prevent the situation where a SIGINT
   is queued on entry, extension language code runs for a "long" time possibly
   serving one or more SIGINTs, and then returns.  Upon return, if
   check_quit_flag is not called, the original SIGINT will be thrown.
   Non-cooperative extension languages are free to install their own SIGINT
   handler but the original must be restored upon return, either itself
   or via restore_active_ext_lang.  */

struct active_ext_lang_state *
set_active_ext_lang (const struct extension_language_defn *now_active)
{
  struct active_ext_lang_state *previous
    = XCNEW (struct active_ext_lang_state);

  previous->ext_lang = active_ext_lang;
  previous->sigint_handler.handler_saved = 0;
  active_ext_lang = now_active;

  if (target_terminal_is_ours ())
    {
      /* If the newly active extension language uses cooperative SIGINT
	 handling then ensure GDB's SIGINT handler is installed.  */
      if (now_active->language == EXT_LANG_GDB
	  || now_active->ops->check_quit_flag != NULL)
	install_gdb_sigint_handler (&previous->sigint_handler);

      /* If there's a SIGINT recorded in the cooperative extension languages,
	 move it to the new language, or save it in GDB's global flag if the
	 newly active extension language doesn't use cooperative SIGINT
	 handling.  */
      if (check_quit_flag ())
	set_quit_flag ();
    }

  return previous;
}

/* Restore active extension language from PREVIOUS.  */

void
restore_active_ext_lang (struct active_ext_lang_state *previous)
{
  active_ext_lang = previous->ext_lang;

  if (target_terminal_is_ours ())
    {
      /* Restore the previous SIGINT handler if one was saved.  */
      if (previous->sigint_handler.handler_saved)
	install_sigint_handler (&previous->sigint_handler);

      /* If there's a SIGINT recorded in the cooperative extension languages,
	 move it to the new language, or save it in GDB's global flag if the
	 newly active extension language doesn't use cooperative SIGINT
	 handling.  */
      if (check_quit_flag ())
	set_quit_flag ();
    }
  xfree (previous);
}

/* Set the quit flag.
   This only sets the flag in the currently active extension language.
   If the currently active extension language does not have cooperative
   SIGINT handling, then GDB's global flag is set, and it is up to the
   extension language to call check_quit_flag.  The extension language
   is free to install its own SIGINT handler, but we still need to handle
   the transition.  */

void
set_quit_flag (void)
{
  if (active_ext_lang->ops != NULL
      && active_ext_lang->ops->set_quit_flag != NULL)
    active_ext_lang->ops->set_quit_flag (active_ext_lang);
  else
    {
      quit_flag = 1;

      /* Now wake up the event loop, or any interruptible_select.  Do
	 this after setting the flag, because signals on Windows
	 actually run on a separate thread, and thus otherwise the
	 main code could be woken up and find quit_flag still
	 clear.  */
      quit_serial_event_set ();
    }
}

/* Return true if the quit flag has been set, false otherwise.
   Note: The flag is cleared as a side-effect.
   The flag is checked in all extension languages that support cooperative
   SIGINT handling, not just the current one.  This simplifies transitions.  */

int
check_quit_flag (void)
{
  int i, result = 0;
  const struct extension_language_defn *extlang;

  ALL_ENABLED_EXTENSION_LANGUAGES (i, extlang)
    {
      if (extlang->ops->check_quit_flag != NULL)
	if (extlang->ops->check_quit_flag (extlang) != 0)
	  result = 1;
    }

  /* This is written in a particular way to avoid races.  */
  if (quit_flag)
    {
      /* No longer need to wake up the event loop or any
	 interruptible_select.  The caller handles the quit
	 request.  */
      quit_serial_event_clear ();
      quit_flag = 0;
      result = 1;
    }

  return result;
}

/* xmethod support.  */

/* The xmethod API routines do not have "ext_lang" in the name because
   the name "xmethod" implies that this routine deals with extension
   languages.  Plus some of the methods take a xmethod_foo * "self/this"
   arg, not an extension_language_defn * arg.  */

/* Returns a new xmethod_worker with EXTLANG and DATA.  Space for the
   result must be freed with free_xmethod_worker.  */

struct xmethod_worker *
new_xmethod_worker (const struct extension_language_defn *extlang, void *data)
{
  struct xmethod_worker *worker = XCNEW (struct xmethod_worker);

  worker->extlang = extlang;
  worker->data = data;
  worker->value = NULL;

  return worker;
}

/* Clones WORKER and returns a new but identical worker.
   The function get_matching_xmethod_workers (see below), returns a
   vector of matching workers.  If a particular worker is selected by GDB
   to invoke a method, then this function can help in cloning the
   selected worker and freeing up the vector via a cleanup.

   Space for the result must be freed with free_xmethod_worker.  */

struct xmethod_worker *
clone_xmethod_worker (struct xmethod_worker *worker)
{
  struct xmethod_worker *new_worker;
  const struct extension_language_defn *extlang = worker->extlang;

  gdb_assert (extlang->ops->clone_xmethod_worker_data != NULL);

  new_worker = new_xmethod_worker
    (extlang,
     extlang->ops->clone_xmethod_worker_data (extlang, worker->data));

  return new_worker;
}

/* If a method with name METHOD_NAME is to be invoked on an object of type
   TYPE, then all entension languages are searched for implementations of
   methods with name METHOD.  All matches found are returned as a vector
   of 'xmethod_worker_ptr' objects.  If no matching methods are
   found, NULL is returned.  */

VEC (xmethod_worker_ptr) *
get_matching_xmethod_workers (struct type *type, const char *method_name)
{
  VEC (xmethod_worker_ptr) *workers = NULL;
  int i;
  const struct extension_language_defn *extlang;

  ALL_ENABLED_EXTENSION_LANGUAGES (i, extlang)
    {
      VEC (xmethod_worker_ptr) *lang_workers, *new_vec;
      enum ext_lang_rc rc;

      /* If an extension language does not support xmethods, ignore
	 it.  */
      if (extlang->ops->get_matching_xmethod_workers == NULL)
	continue;

      rc = extlang->ops->get_matching_xmethod_workers (extlang,
						       type, method_name,
						       &lang_workers);
      if (rc == EXT_LANG_RC_ERROR)
	{
	  free_xmethod_worker_vec (workers);
	  error (_("Error while looking for matching xmethod workers "
		   "defined in %s."), extlang->capitalized_name);
	}

      new_vec = VEC_merge (xmethod_worker_ptr, workers, lang_workers);
      /* Free only the vectors and not the elements as NEW_VEC still
	 contains them.  */
      VEC_free (xmethod_worker_ptr, workers);
      VEC_free (xmethod_worker_ptr, lang_workers);
      workers = new_vec;
    }

  return workers;
}

/* Return the arg types of the xmethod encapsulated in WORKER.
   An array of arg types is returned.  The length of the array is returned in
   NARGS.  The type of the 'this' object is returned as the first element of
   array.  */

struct type **
get_xmethod_arg_types (struct xmethod_worker *worker, int *nargs)
{
  enum ext_lang_rc rc;
  struct type **type_array = NULL;
  const struct extension_language_defn *extlang = worker->extlang;

  gdb_assert (extlang->ops->get_xmethod_arg_types != NULL);

  rc = extlang->ops->get_xmethod_arg_types (extlang, worker, nargs,
					    &type_array);
  if (rc == EXT_LANG_RC_ERROR)
    {
      error (_("Error while looking for arg types of a xmethod worker "
	       "defined in %s."), extlang->capitalized_name);
    }

  return type_array;
}

/* Return the type of the result of the xmethod encapsulated in WORKER.
   OBJECT, ARGS, NARGS are the same as for invoke_xmethod.  */

struct type *
get_xmethod_result_type (struct xmethod_worker *worker,
			 struct value *object, struct value **args, int nargs)
{
  enum ext_lang_rc rc;
  struct type *result_type;
  const struct extension_language_defn *extlang = worker->extlang;

  gdb_assert (extlang->ops->get_xmethod_arg_types != NULL);

  rc = extlang->ops->get_xmethod_result_type (extlang, worker,
					      object, args, nargs,
					      &result_type);
  if (rc == EXT_LANG_RC_ERROR)
    {
      error (_("Error while fetching result type of an xmethod worker "
	       "defined in %s."), extlang->capitalized_name);
    }

  return result_type;
}

/* Invokes the xmethod encapsulated in WORKER and returns the result.
   The method is invoked on OBJ with arguments in the ARGS array.  NARGS is
   the length of the this array.  */

struct value *
invoke_xmethod (struct xmethod_worker *worker, struct value *obj,
		     struct value **args, int nargs)
{
  gdb_assert (worker->extlang->ops->invoke_xmethod != NULL);

  return worker->extlang->ops->invoke_xmethod (worker->extlang, worker,
					       obj, args, nargs);
}

/* Frees the xmethod worker WORKER.  */

void
free_xmethod_worker (struct xmethod_worker *worker)
{
  gdb_assert (worker->extlang->ops->free_xmethod_worker_data != NULL);
  worker->extlang->ops->free_xmethod_worker_data (worker->extlang,
						  worker->data);
  xfree (worker);
}

/* Frees a vector of xmethod_workers VEC.  */

void
free_xmethod_worker_vec (void *vec)
{
  int i;
  struct xmethod_worker *worker;
  VEC (xmethod_worker_ptr) *v = (VEC (xmethod_worker_ptr) *) vec;

  for (i = 0; VEC_iterate (xmethod_worker_ptr, v, i, worker); i++)
    free_xmethod_worker (worker);

  VEC_free (xmethod_worker_ptr, v);
}

/* Called via an observer before gdb prints its prompt.
   Iterate over the extension languages giving them a chance to
   change the prompt.  The first one to change the prompt wins,
   and no further languages are tried.  */

static void
ext_lang_before_prompt (const char *current_gdb_prompt)
{
  int i;
  const struct extension_language_defn *extlang;

  ALL_ENABLED_EXTENSION_LANGUAGES (i, extlang)
    {
      enum ext_lang_rc rc;

      if (extlang->ops->before_prompt == NULL)
	continue;
      rc = extlang->ops->before_prompt (extlang, current_gdb_prompt);
      switch (rc)
	{
	case EXT_LANG_RC_OK:
	case EXT_LANG_RC_ERROR:
	  return;
	case EXT_LANG_RC_NOP:
	  break;
	default:
	  gdb_assert_not_reached ("bad return from before_prompt");
	}
    }
}

extern initialize_file_ftype _initialize_extension;

void
_initialize_extension (void)
{
  observer_attach_before_prompt (ext_lang_before_prompt);
}
