/* Header file for GDB command decoding library.

   Copyright (C) 2000-2016 Free Software Foundation, Inc.

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

#if !defined (CLI_DECODE_H)
#define CLI_DECODE_H 1

/* This file defines the private interfaces for any code implementing
   command internals.  */

/* Include the public interfaces.  */
#include "command.h"

struct re_pattern_buffer;

#if 0
/* FIXME: cagney/2002-03-17: Once cmd_type() has been removed, ``enum
   cmd_types'' can be moved from "command.h" to "cli-decode.h".  */
/* Not a set/show command.  Note that some commands which begin with
   "set" or "show" might be in this category, if their syntax does
   not fall into one of the following categories.  */
typedef enum cmd_types
  {
    not_set_cmd,
    set_cmd,
    show_cmd
  }
cmd_types;
#endif

/* This structure records one command'd definition.  */


struct cmd_list_element
  {
    /* Points to next command in this list.  */
    struct cmd_list_element *next;

    /* Name of this command.  */
    const char *name;

    /* Command class; class values are chosen by application program.  */
    enum command_class theclass;

    /* When 1 indicated that this command is deprecated.  It may be
       removed from gdb's command set in the future.  */

    unsigned int cmd_deprecated : 1;

    /* The user needs to be warned that this is a deprecated command.
       The user should only be warned the first time a command is
       used.  */
        
    unsigned int deprecated_warn_user : 1;

    /* When functions are deprecated at compile time (this is the way
       it should, in general, be done) the memory containing the
       replacement string is statically allocated.  In some cases it
       makes sense to deprecate commands at runtime (the testsuite is
       one example).  In this case the memory for replacement is
       malloc'ed.  When a command is undeprecated or re-deprecated at
       runtime we don't want to risk calling free on statically
       allocated memory, so we check this flag.  */

    unsigned int malloced_replacement : 1;

    /* Set if the doc field should be xfree'd.  */

    unsigned int doc_allocated : 1;

    /* Flag that specifies if this command is already running its hook.  */
    /* Prevents the possibility of hook recursion.  */
    unsigned int hook_in : 1;

    /* For prefix commands only:
       nonzero means do not get an error if subcommand is not
       recognized; call the prefix's own function in that case.  */
    unsigned int allow_unknown : 1;

    /* Nonzero says this is an abbreviation, and should not
       be mentioned in lists of commands.
       This allows "br<tab>" to complete to "break", which it
       otherwise wouldn't.  */
    unsigned int abbrev_flag : 1;

    /* Type of "set" or "show" command (or SET_NOT_SET if not "set"
       or "show").  */
    ENUM_BITFIELD (cmd_types) type : 2;

    /* What kind of variable is *VAR?  */
    ENUM_BITFIELD (var_types) var_type : 4;

    /* Function definition of this command.  NULL for command class
       names and for help topics that are not really commands.  NOTE:
       cagney/2002-02-02: This function signature is evolving.  For
       the moment suggest sticking with either set_cmd_cfunc() or
       set_cmd_sfunc().  */
    void (*func) (struct cmd_list_element *c, char *args, int from_tty);
    /* The command's real callback.  At present func() bounces through
       to one of the below.  */
    union
      {
	/* If type is not_set_cmd, call it like this: */
	cmd_cfunc_ftype *cfunc;
	/* If type is set_cmd or show_cmd, first set the variables,
	   and then call this: */
	cmd_sfunc_ftype *sfunc;
      }
    function;

    /* Local state (context) for this command.  This can be anything.  */
    void *context;

    /* Documentation of this command (or help topic).
       First line is brief documentation; remaining lines form, with it,
       the full documentation.  First line should end with a period.
       Entire string should also end with a period, not a newline.  */
    const char *doc;

    /* For set/show commands.  A method for printing the output to the
       specified stream.  */
    show_value_ftype *show_value_func;

    /* If this command is deprecated, this is the replacement name.  */
    const char *replacement;

    /* If this command represents a show command, then this function
       is called before the variable's value is examined.  */
    void (*pre_show_hook) (struct cmd_list_element *c);

    /* Hook for another command to be executed before this command.  */
    struct cmd_list_element *hook_pre;

    /* Hook for another command to be executed after this command.  */
    struct cmd_list_element *hook_post;

    /* Nonzero identifies a prefix command.  For them, the address
       of the variable containing the list of subcommands.  */
    struct cmd_list_element **prefixlist;

    /* For prefix commands only:
       String containing prefix commands to get here: this one
       plus any others needed to get to it.  Should end in a space.
       It is used before the word "command" in describing the
       commands reached through this prefix.  */
    const char *prefixname;

    /* The prefix command of this command.  */
    struct cmd_list_element *prefix;

    /* Completion routine for this command.  TEXT is the text beyond
       what was matched for the command itself (leading whitespace is
       skipped).  It stops where we are supposed to stop completing
       (rl_point) and is '\0' terminated.

       Return value is a malloc'd vector of pointers to possible
       completions terminated with NULL.  If there are no completions,
       returning a pointer to a NULL would work but returning NULL
       itself is also valid.  WORD points in the same buffer as TEXT,
       and completions should be returned relative to this position.
       For example, suppose TEXT is "foo" and we want to complete to
       "foobar".  If WORD is "oo", return "oobar"; if WORD is
       "baz/foo", return "baz/foobar".  */
    completer_ftype *completer;

    /* Handle the word break characters for this completer.  Usually
       this function need not be defined, but for some types of
       completers (e.g., Python completers declared as methods inside
       a class) the word break chars may need to be redefined
       depending on the completer type (e.g., for filename
       completers).  */

    completer_ftype_void *completer_handle_brkchars;

    /* Destruction routine for this command.  If non-NULL, this is
       called when this command instance is destroyed.  This may be
       used to finalize the CONTEXT field, if needed.  */
    void (*destroyer) (struct cmd_list_element *self, void *context);

    /* Pointer to variable affected by "set" and "show".  Doesn't
       matter if type is not_set.  */
    void *var;

    /* Pointer to NULL terminated list of enumerated values (like
       argv).  */
    const char *const *enums;

    /* Pointer to command strings of user-defined commands */
    struct command_line *user_commands;

    /* Pointer to command that is hooked by this one, (by hook_pre)
       so the hook can be removed when this one is deleted.  */
    struct cmd_list_element *hookee_pre;

    /* Pointer to command that is hooked by this one, (by hook_post)
       so the hook can be removed when this one is deleted.  */
    struct cmd_list_element *hookee_post;

    /* Pointer to command that is aliased by this one, so the
       aliased command can be located in case it has been hooked.  */
    struct cmd_list_element *cmd_pointer;

    /* Start of a linked list of all aliases of this command.  */
    struct cmd_list_element *aliases;

    /* Link pointer for aliases on an alias list.  */
    struct cmd_list_element *alias_chain;

    /* If non-null, the pointer to a field in 'struct
       cli_suppress_notification', which will be set to true in cmd_func
       when this command is being executed.  It will be set back to false
       when the command has been executed.  */
    int *suppress_notification;
  };

extern void help_cmd_list (struct cmd_list_element *, enum command_class,
			   const char *, int, struct ui_file *);

/* Functions that implement commands about CLI commands.  */

extern void help_cmd (const char *, struct ui_file *);

extern void apropos_cmd (struct ui_file *, struct cmd_list_element *,
                         struct re_pattern_buffer *, const char *);

/* Used to mark commands that don't do anything.  If we just leave the
   function field NULL, the command is interpreted as a help topic, or
   as a class of commands.  */

extern void not_just_help_class_command (char *arg, int from_tty);

/* Exported to cli/cli-setshow.c */

extern void print_doc_line (struct ui_file *, const char *);

extern const char * const auto_boolean_enums[];

/* Verify whether a given cmd_list_element is a user-defined command.
   Return 1 if it is user-defined.  Return 0 otherwise.  */

extern int cli_user_command_p (struct cmd_list_element *);

#endif /* !defined (CLI_DECODE_H) */
