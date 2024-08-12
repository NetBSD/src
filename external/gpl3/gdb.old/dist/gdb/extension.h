/* Interface between gdb and its extension languages.

   Copyright (C) 2014-2023 Free Software Foundation, Inc.

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

#ifndef EXTENSION_H
#define EXTENSION_H

#include "mi/mi-cmds.h" /* For PRINT_NO_VALUES, etc.  */
#include "gdbsupport/array-view.h"
#include "gdbsupport/gdb_optional.h"

struct breakpoint;
struct command_line;
class frame_info_ptr;
struct language_defn;
struct objfile;
struct extension_language_defn;
struct type;
struct ui_file;
struct ui_out;
struct value;
struct value_print_options;

/* A function to load and process a script file.
   The file has been opened and is ready to be read from the beginning.
   Any exceptions are not caught, and are passed to the caller.  */
typedef void script_sourcer_func (const struct extension_language_defn *,
				  FILE *stream, const char *filename);

/* A function to load and process a script for an objfile.
   The file has been opened and is ready to be read from the beginning.
   Any exceptions are not caught, and are passed to the caller.  */
typedef void objfile_script_sourcer_func
  (const struct extension_language_defn *,
   struct objfile *, FILE *stream, const char *filename);

/* A function to execute a script for an objfile.
   Any exceptions are not caught, and are passed to the caller.  */
typedef void objfile_script_executor_func
  (const struct extension_language_defn *,
   struct objfile *, const char *name, const char *script);

/* Enum of each extension(/scripting) language.  */

enum extension_language
  {
    EXT_LANG_NONE,
    EXT_LANG_GDB,
    EXT_LANG_PYTHON,
    EXT_LANG_GUILE
  };

/* Extension language frame-filter status return values.  */

enum ext_lang_bt_status
  {
    /* Return when an error has occurred in processing frame filters,
       or when printing the stack.  */
    EXT_LANG_BT_ERROR = -1,

    /* Return from internal routines to indicate that the function
       succeeded.  */
    EXT_LANG_BT_OK = 1,

    /* Return when the frame filter process is complete, but there
       were no filter registered and enabled to process.  */
    EXT_LANG_BT_NO_FILTERS = 2
  };

/* Flags to pass to apply_extlang_frame_filter.  */

enum frame_filter_flag
  {
    /* Set this flag if frame level is to be printed.  */
    PRINT_LEVEL = 1 << 0,

    /* Set this flag if frame information is to be printed.  */
    PRINT_FRAME_INFO = 1 << 1,

    /* Set this flag if frame arguments are to be printed.  */
    PRINT_ARGS = 1 << 2,

    /* Set this flag if frame locals are to be printed.  */
    PRINT_LOCALS = 1 << 3,

    /* Set this flag if a "More frames" message is to be printed.  */
    PRINT_MORE_FRAMES = 1 << 4,

    /* Set this flag if elided frames should not be printed.  */
    PRINT_HIDE = 1 << 5,
  };

DEF_ENUM_FLAGS_TYPE (enum frame_filter_flag, frame_filter_flags);

/* A choice of the different frame argument printing strategies that
   can occur in different cases of frame filter instantiation.  */

enum ext_lang_frame_args
  {
    /* Print no values for arguments when invoked from the MI. */
    NO_VALUES = PRINT_NO_VALUES,

    MI_PRINT_ALL_VALUES = PRINT_ALL_VALUES,

    /* Print only simple values (what MI defines as "simple") for
       arguments when invoked from the MI. */
    MI_PRINT_SIMPLE_VALUES = PRINT_SIMPLE_VALUES,

    /* Print only scalar values for arguments when invoked from the CLI. */
    CLI_SCALAR_VALUES,

    /* Print all values for arguments when invoked from the CLI. */
    CLI_ALL_VALUES,

    /* Only indicate the presence of arguments when invoked from the CLI.  */
    CLI_PRESENCE
  };

/* The possible results of
   extension_language_ops.breakpoint_cond_says_stop.  */

enum ext_lang_bp_stop
  {
    /* No "stop" condition is set.  */
    EXT_LANG_BP_STOP_UNSET,

    /* A "stop" condition is set, and it says "don't stop".  */
    EXT_LANG_BP_STOP_NO,

    /* A "stop" condition is set, and it says "stop".  */
    EXT_LANG_BP_STOP_YES
  };

/* Table of type printers associated with the global typedef table.  */

struct ext_lang_type_printers
{
  ext_lang_type_printers ();
  ~ext_lang_type_printers ();

  DISABLE_COPY_AND_ASSIGN (ext_lang_type_printers);

  /* Type-printers from Python.  */
  void *py_type_printers = nullptr;
};

/* The return code for some API calls.  */

enum ext_lang_rc
{
  /* The operation completed successfully.  */
  EXT_LANG_RC_OK,

  /* The operation was not performed (e.g., no pretty-printer).  */
  EXT_LANG_RC_NOP,

  /* There was an error (e.g., Python error while printing a value).
     When an error occurs no further extension languages are tried.
     This is to preserve existing behaviour, and because it's convenient
     for Python developers.
     Note: This is different than encountering a memory error trying to read
     a value for pretty-printing.  Here we're referring to, e.g., programming
     errors that trigger an exception in the extension language.  */
  EXT_LANG_RC_ERROR
};

/* A type which holds its extension language specific xmethod worker data.  */

struct xmethod_worker
{
  xmethod_worker (const extension_language_defn *extlang)
  : m_extlang (extlang)
  {}

  virtual ~xmethod_worker () = default;

  /* Invoke the xmethod encapsulated in this worker and return the result.
     The method is invoked on OBJ with arguments in the ARGS array.  */

  virtual value *invoke (value *obj, gdb::array_view<value *> args) = 0;

  /* Return the arg types of the xmethod encapsulated in this worker.
     The type of the 'this' object is returned as the first element of
     the vector.  */

  std::vector<type *> get_arg_types ();

  /* Return the type of the result of the xmethod encapsulated in this worker.
     OBJECT and ARGS are the same as for invoke.  */

  type *get_result_type (value *object, gdb::array_view<value *> args);

private:

  /* Return the types of the arguments the method takes.  The types
     are returned in TYPE_ARGS, one per argument.  */

  virtual enum ext_lang_rc do_get_arg_types
    (std::vector<type *> *type_args) = 0;

  /* Fetch the type of the result of the method implemented by this
     worker.  OBJECT and ARGS are the same as for the invoked method.
     The result type is stored in *RESULT_TYPE.  */

  virtual enum ext_lang_rc do_get_result_type
    (struct value *obj, gdb::array_view<value *> args,
     struct type **result_type_ptr) = 0;

  /* The language the xmethod worker is implemented in.  */

  const extension_language_defn *m_extlang;
};

typedef std::unique_ptr<xmethod_worker> xmethod_worker_up;

/* The interface for gdb's own extension(/scripting) language.  */
extern const struct extension_language_defn extension_language_gdb;

extern const struct extension_language_defn *get_ext_lang_defn
  (enum extension_language lang);

extern const struct extension_language_defn *get_ext_lang_of_file
  (const char *file);

extern int ext_lang_present_p (const struct extension_language_defn *);

extern int ext_lang_initialized_p (const struct extension_language_defn *);

extern void throw_ext_lang_unsupported
  (const struct extension_language_defn *);

/* Accessors for "public" attributes of the extension language definition.  */

extern enum extension_language ext_lang_kind
  (const struct extension_language_defn *);

extern const char *ext_lang_name (const struct extension_language_defn *);

extern const char *ext_lang_capitalized_name
  (const struct extension_language_defn *);

extern const char *ext_lang_suffix (const struct extension_language_defn *);

extern const char *ext_lang_auto_load_suffix
  (const struct extension_language_defn *);

extern script_sourcer_func *ext_lang_script_sourcer
  (const struct extension_language_defn *);

extern objfile_script_sourcer_func *ext_lang_objfile_script_sourcer
  (const struct extension_language_defn *);

extern objfile_script_executor_func *ext_lang_objfile_script_executor
  (const struct extension_language_defn *);

/* Return true if auto-loading of EXTLANG scripts is enabled.
   False is returned if support for this language isn't compiled in.  */

extern bool ext_lang_auto_load_enabled (const struct extension_language_defn *);

/* Wrappers for each extension language API function that iterate over all
   extension languages.  */

extern void ext_lang_initialization (void);

extern void eval_ext_lang_from_control_command (struct command_line *cmd);

extern void auto_load_ext_lang_scripts_for_objfile (struct objfile *);

extern char *apply_ext_lang_type_printers (struct ext_lang_type_printers *,
					   struct type *);

extern int apply_ext_lang_val_pretty_printer
  (struct value *value, struct ui_file *stream, int recurse,
   const struct value_print_options *options,
   const struct language_defn *language);

extern enum ext_lang_bt_status apply_ext_lang_frame_filter
  (frame_info_ptr frame, frame_filter_flags flags,
   enum ext_lang_frame_args args_type,
   struct ui_out *out, int frame_low, int frame_high);

extern void preserve_ext_lang_values (struct objfile *, htab_t copied_types);

extern const struct extension_language_defn *get_breakpoint_cond_ext_lang
  (struct breakpoint *b, enum extension_language skip_lang);

extern int breakpoint_ext_lang_cond_says_stop (struct breakpoint *);

/* If a method with name METHOD_NAME is to be invoked on an object of type
   TYPE, then all extension languages are searched for implementations of
   methods with name METHOD_NAME.  All matches found are appended to the WORKERS
   vector.  */

extern void get_matching_xmethod_workers
  (struct type *type, const char *method_name,
   std::vector<xmethod_worker_up> *workers);

/* Try to colorize some source code.  FILENAME is the name of the file
   holding the code.  CONTENTS is the source code itself.  This will
   either a colorized (using ANSI terminal escapes) version of the
   source code, or an empty value if colorizing could not be done.  */

extern gdb::optional<std::string> ext_lang_colorize
  (const std::string &filename, const std::string &contents);

/* Try to colorize a single line of disassembler output, CONTENT for
   GDBARCH.  This will return either a colorized (using ANSI terminal
   escapes) version of CONTENT, or an empty value if colorizing could not
   be done.  */

extern gdb::optional<std::string> ext_lang_colorize_disasm
  (const std::string &content, gdbarch *gdbarch);

/* Calls extension_language_ops::print_insn for each extension language,
   returning the result from the first extension language that returns a
   non-empty result (any further extension languages are not then called).

   All arguments are forwarded to extension_language_ops::print_insn, see
   that function for a full description.  */

extern gdb::optional<int> ext_lang_print_insn
  (struct gdbarch *gdbarch, CORE_ADDR address, struct disassemble_info *info);

#if GDB_SELF_TEST
namespace selftests {
extern void (*hook_set_active_ext_lang) ();
}
#endif

#endif /* EXTENSION_H */
