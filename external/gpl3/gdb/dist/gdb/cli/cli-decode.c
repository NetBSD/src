/* Handle lists of commands, their decoding and documentation, for GDB.

   Copyright (C) 1986-2020 Free Software Foundation, Inc.

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
#include "symtab.h"
#include <ctype.h>
#include "gdb_regex.h"
#include "completer.h"
#include "ui-out.h"
#include "cli/cli-cmds.h"
#include "cli/cli-decode.h"
#include "cli/cli-style.h"
#include "gdbsupport/gdb_optional.h"

/* Prototypes for local functions.  */

static void undef_cmd_error (const char *, const char *);

static struct cmd_list_element *delete_cmd (const char *name,
					    struct cmd_list_element **list,
					    struct cmd_list_element **prehook,
					    struct cmd_list_element **prehookee,
					    struct cmd_list_element **posthook,
					    struct cmd_list_element **posthookee);

static struct cmd_list_element *find_cmd (const char *command,
					  int len,
					  struct cmd_list_element *clist,
					  int ignore_help_classes,
					  int *nfound);

static void help_cmd_list (struct cmd_list_element *list,
			   enum command_class theclass,
			   bool recurse,
			   struct ui_file *stream);

static void help_all (struct ui_file *stream);

/* Look up a command whose 'prefixlist' is KEY.  Return the command if found,
   otherwise return NULL.  */

static struct cmd_list_element *
lookup_cmd_for_prefixlist (struct cmd_list_element **key,
			   struct cmd_list_element *list)
{
  struct cmd_list_element *p = NULL;

  for (p = list; p != NULL; p = p->next)
    {
      struct cmd_list_element *q;

      if (p->prefixlist == NULL)
	continue;
      else if (p->prefixlist == key)
	{
	  /* If we found an alias, we must return the aliased
	     command.  */
	  return p->cmd_pointer ? p->cmd_pointer : p;
	}

      q = lookup_cmd_for_prefixlist (key, *(p->prefixlist));
      if (q != NULL)
	return q;
    }

  return NULL;
}

static void
print_help_for_command (struct cmd_list_element *c,
			bool recurse, struct ui_file *stream);


/* Set the callback function for the specified command.  For each both
   the commands callback and func() are set.  The latter set to a
   bounce function (unless cfunc / sfunc is NULL that is).  */

static void
do_const_cfunc (struct cmd_list_element *c, const char *args, int from_tty)
{
  c->function.const_cfunc (args, from_tty);
}

static void
set_cmd_cfunc (struct cmd_list_element *cmd, cmd_const_cfunc_ftype *cfunc)
{
  if (cfunc == NULL)
    cmd->func = NULL;
  else
    cmd->func = do_const_cfunc;
  cmd->function.const_cfunc = cfunc;
}

static void
do_sfunc (struct cmd_list_element *c, const char *args, int from_tty)
{
  c->function.sfunc (args, from_tty, c);
}

void
set_cmd_sfunc (struct cmd_list_element *cmd, cmd_const_sfunc_ftype *sfunc)
{
  if (sfunc == NULL)
    cmd->func = NULL;
  else
    cmd->func = do_sfunc;
  cmd->function.sfunc = sfunc;
}

int
cmd_cfunc_eq (struct cmd_list_element *cmd, cmd_const_cfunc_ftype *cfunc)
{
  return cmd->func == do_const_cfunc && cmd->function.const_cfunc == cfunc;
}

void
set_cmd_context (struct cmd_list_element *cmd, void *context)
{
  cmd->context = context;
}

void *
get_cmd_context (struct cmd_list_element *cmd)
{
  return cmd->context;
}

void
set_cmd_completer (struct cmd_list_element *cmd, completer_ftype *completer)
{
  cmd->completer = completer; /* Ok.  */
}

/* See definition in commands.h.  */

void
set_cmd_completer_handle_brkchars (struct cmd_list_element *cmd,
				   completer_handle_brkchars_ftype *func)
{
  cmd->completer_handle_brkchars = func;
}

/* Add element named NAME.
   Space for NAME and DOC must be allocated by the caller.
   CLASS is the top level category into which commands are broken down
   for "help" purposes.
   FUN should be the function to execute the command;
   it will get a character string as argument, with leading
   and trailing blanks already eliminated.

   DOC is a documentation string for the command.
   Its first line should be a complete sentence.
   It should start with ? for a command that is an abbreviation
   or with * for a command that most users don't need to know about.

   Add this command to command list *LIST.

   Returns a pointer to the added command (not necessarily the head 
   of *LIST).  */

static struct cmd_list_element *
do_add_cmd (const char *name, enum command_class theclass,
	    const char *doc, struct cmd_list_element **list)
{
  struct cmd_list_element *c = new struct cmd_list_element (name, theclass,
							    doc);
  struct cmd_list_element *p, *iter;

  /* Turn each alias of the old command into an alias of the new
     command.  */
  c->aliases = delete_cmd (name, list, &c->hook_pre, &c->hookee_pre,
			   &c->hook_post, &c->hookee_post);
  for (iter = c->aliases; iter; iter = iter->alias_chain)
    iter->cmd_pointer = c;
  if (c->hook_pre)
    c->hook_pre->hookee_pre = c;
  if (c->hookee_pre)
    c->hookee_pre->hook_pre = c;
  if (c->hook_post)
    c->hook_post->hookee_post = c;
  if (c->hookee_post)
    c->hookee_post->hook_post = c;

  if (*list == NULL || strcmp ((*list)->name, name) >= 0)
    {
      c->next = *list;
      *list = c;
    }
  else
    {
      p = *list;
      while (p->next && strcmp (p->next->name, name) <= 0)
	{
	  p = p->next;
	}
      c->next = p->next;
      p->next = c;
    }

  /* Search the prefix cmd of C, and assigns it to C->prefix.
     See also add_prefix_cmd and update_prefix_field_of_prefixed_commands.  */
  struct cmd_list_element *prefixcmd = lookup_cmd_for_prefixlist (list,
								  cmdlist);
  c->prefix = prefixcmd;


  return c;
}

struct cmd_list_element *
add_cmd (const char *name, enum command_class theclass,
	 const char *doc, struct cmd_list_element **list)
{
  cmd_list_element *result = do_add_cmd (name, theclass, doc, list);
  result->func = NULL;
  result->function.const_cfunc = NULL;
  return result;
}

struct cmd_list_element *
add_cmd (const char *name, enum command_class theclass,
	 cmd_const_cfunc_ftype *fun,
	 const char *doc, struct cmd_list_element **list)
{
  cmd_list_element *result = do_add_cmd (name, theclass, doc, list);
  set_cmd_cfunc (result, fun);
  return result;
}

/* Add an element with a suppress notification to the LIST of commands.  */

struct cmd_list_element *
add_cmd_suppress_notification (const char *name, enum command_class theclass,
			       cmd_const_cfunc_ftype *fun, const char *doc,
			       struct cmd_list_element **list,
			       int *suppress_notification)
{
  struct cmd_list_element *element;

  element = add_cmd (name, theclass, fun, doc, list);
  element->suppress_notification = suppress_notification;

  return element;
}


/* Deprecates a command CMD.
   REPLACEMENT is the name of the command which should be used in
   place of this command, or NULL if no such command exists.

   This function does not check to see if command REPLACEMENT exists
   since gdb may not have gotten around to adding REPLACEMENT when
   this function is called.

   Returns a pointer to the deprecated command.  */

struct cmd_list_element *
deprecate_cmd (struct cmd_list_element *cmd, const char *replacement)
{
  cmd->cmd_deprecated = 1;
  cmd->deprecated_warn_user = 1;

  if (replacement != NULL)
    cmd->replacement = replacement;
  else
    cmd->replacement = NULL;

  return cmd;
}

struct cmd_list_element *
add_alias_cmd (const char *name, cmd_list_element *old,
	       enum command_class theclass, int abbrev_flag,
	       struct cmd_list_element **list)
{
  if (old == 0)
    {
      struct cmd_list_element *prehook, *prehookee, *posthook, *posthookee;
      struct cmd_list_element *aliases = delete_cmd (name, list,
						     &prehook, &prehookee,
						     &posthook, &posthookee);

      /* If this happens, it means a programmer error somewhere.  */
      gdb_assert (!aliases && !prehook && !prehookee
		  && !posthook && ! posthookee);
      return 0;
    }

  struct cmd_list_element *c = add_cmd (name, theclass, old->doc, list);

  /* If OLD->DOC can be freed, we should make another copy.  */
  if (old->doc_allocated)
    {
      c->doc = xstrdup (old->doc);
      c->doc_allocated = 1;
    }
  /* NOTE: Both FUNC and all the FUNCTIONs need to be copied.  */
  c->func = old->func;
  c->function = old->function;
  c->prefixlist = old->prefixlist;
  c->prefixname = old->prefixname;
  c->allow_unknown = old->allow_unknown;
  c->abbrev_flag = abbrev_flag;
  c->cmd_pointer = old;
  c->alias_chain = old->aliases;
  old->aliases = c;

  return c;
}

struct cmd_list_element *
add_alias_cmd (const char *name, const char *oldname,
	       enum command_class theclass, int abbrev_flag,
	       struct cmd_list_element **list)
{
  const char *tmp;
  struct cmd_list_element *old;

  tmp = oldname;
  old = lookup_cmd (&tmp, *list, "", NULL, 1, 1);

  return add_alias_cmd (name, old, theclass, abbrev_flag, list);
}


/* Update the prefix field of all sub-commands of the prefix command C.
   We must do this when a prefix command is defined as the GDB init sequence
   does not guarantee that a prefix command is created before its sub-commands.
   For example, break-catch-sig.c initialization runs before breakpoint.c
   initialization, but it is breakpoint.c that creates the "catch" command used
   by the "catch signal" command created by break-catch-sig.c.  */

static void
update_prefix_field_of_prefixed_commands (struct cmd_list_element *c)
{
  for (cmd_list_element *p = *c->prefixlist; p != NULL; p = p->next)
    {
      p->prefix = c;

      /* We must recursively update the prefix field to cover
	 e.g.  'info auto-load libthread-db' where the creation
	 order was:
           libthread-db
           auto-load
           info
	 In such a case, when 'auto-load' was created by do_add_cmd,
         the 'libthread-db' prefix field could not be updated, as the
	 'auto-load' command was not yet reachable by
	    lookup_cmd_for_prefixlist (list, cmdlist)
	    that searches from the top level 'cmdlist'.  */
      if (p->prefixlist != nullptr)
	update_prefix_field_of_prefixed_commands (p);
    }
}


/* Like add_cmd but adds an element for a command prefix: a name that
   should be followed by a subcommand to be looked up in another
   command list.  PREFIXLIST should be the address of the variable
   containing that list.  */

struct cmd_list_element *
add_prefix_cmd (const char *name, enum command_class theclass,
		cmd_const_cfunc_ftype *fun,
		const char *doc, struct cmd_list_element **prefixlist,
		const char *prefixname, int allow_unknown,
		struct cmd_list_element **list)
{
  struct cmd_list_element *c = add_cmd (name, theclass, fun, doc, list);

  c->prefixlist = prefixlist;
  c->prefixname = prefixname;
  c->allow_unknown = allow_unknown;

  /* Now that prefix command C is defined, we need to set the prefix field
     of all prefixed commands that were defined before C itself was defined.  */
  update_prefix_field_of_prefixed_commands (c);

  return c;
}

/* A helper function for add_basic_prefix_cmd.  This is a command
   function that just forwards to help_list.  */

static void
do_prefix_cmd (const char *args, int from_tty, struct cmd_list_element *c)
{
  /* Look past all aliases.  */
  while (c->cmd_pointer != nullptr)
    c = c->cmd_pointer;

  help_list (*c->prefixlist, c->prefixname, all_commands, gdb_stdout);
}

/* See command.h.  */

struct cmd_list_element *
add_basic_prefix_cmd (const char *name, enum command_class theclass,
		      const char *doc, struct cmd_list_element **prefixlist,
		      const char *prefixname, int allow_unknown,
		      struct cmd_list_element **list)
{
  struct cmd_list_element *cmd = add_prefix_cmd (name, theclass, nullptr,
						 doc, prefixlist, prefixname,
						 allow_unknown, list);
  set_cmd_sfunc (cmd, do_prefix_cmd);
  return cmd;
}

/* A helper function for add_show_prefix_cmd.  This is a command
   function that just forwards to cmd_show_list.  */

static void
do_show_prefix_cmd (const char *args, int from_tty, struct cmd_list_element *c)
{
  cmd_show_list (*c->prefixlist, from_tty);
}

/* See command.h.  */

struct cmd_list_element *
add_show_prefix_cmd (const char *name, enum command_class theclass,
		     const char *doc, struct cmd_list_element **prefixlist,
		     const char *prefixname, int allow_unknown,
		     struct cmd_list_element **list)
{
  struct cmd_list_element *cmd = add_prefix_cmd (name, theclass, nullptr,
						 doc, prefixlist, prefixname,
						 allow_unknown, list);
  set_cmd_sfunc (cmd, do_show_prefix_cmd);
  return cmd;
}

/* Like ADD_PREFIX_CMD but sets the suppress_notification pointer on the
   new command list element.  */

struct cmd_list_element *
add_prefix_cmd_suppress_notification
               (const char *name, enum command_class theclass,
		cmd_const_cfunc_ftype *fun,
		const char *doc, struct cmd_list_element **prefixlist,
		const char *prefixname, int allow_unknown,
		struct cmd_list_element **list,
		int *suppress_notification)
{
  struct cmd_list_element *element
    = add_prefix_cmd (name, theclass, fun, doc, prefixlist,
		      prefixname, allow_unknown, list);
  element->suppress_notification = suppress_notification;
  return element;
}

/* Like add_prefix_cmd but sets the abbrev_flag on the new command.  */

struct cmd_list_element *
add_abbrev_prefix_cmd (const char *name, enum command_class theclass,
		       cmd_const_cfunc_ftype *fun, const char *doc,
		       struct cmd_list_element **prefixlist,
		       const char *prefixname,
		       int allow_unknown, struct cmd_list_element **list)
{
  struct cmd_list_element *c = add_cmd (name, theclass, fun, doc, list);

  c->prefixlist = prefixlist;
  c->prefixname = prefixname;
  c->allow_unknown = allow_unknown;
  c->abbrev_flag = 1;
  return c;
}

/* This is an empty "cfunc".  */
void
not_just_help_class_command (const char *args, int from_tty)
{
}

/* This is an empty "sfunc".  */

static void
empty_sfunc (const char *args, int from_tty, struct cmd_list_element *c)
{
}

/* Add element named NAME to command list LIST (the list for set/show
   or some sublist thereof).
   TYPE is set_cmd or show_cmd.
   CLASS is as in add_cmd.
   VAR_TYPE is the kind of thing we are setting.
   VAR is address of the variable being controlled by this command.
   DOC is the documentation string.  */

static struct cmd_list_element *
add_set_or_show_cmd (const char *name,
		     enum cmd_types type,
		     enum command_class theclass,
		     var_types var_type,
		     void *var,
		     const char *doc,
		     struct cmd_list_element **list)
{
  struct cmd_list_element *c = add_cmd (name, theclass, doc, list);

  gdb_assert (type == set_cmd || type == show_cmd);
  c->type = type;
  c->var_type = var_type;
  c->var = var;
  /* This needs to be something besides NULL so that this isn't
     treated as a help class.  */
  set_cmd_sfunc (c, empty_sfunc);
  return c;
}

/* Add element named NAME to both the command SET_LIST and SHOW_LIST.
   CLASS is as in add_cmd.  VAR_TYPE is the kind of thing we are
   setting.  VAR is address of the variable being controlled by this
   command.  SET_FUNC and SHOW_FUNC are the callback functions (if
   non-NULL).  SET_DOC, SHOW_DOC and HELP_DOC are the documentation
   strings.  PRINT the format string to print the value.  SET_RESULT
   and SHOW_RESULT, if not NULL, are set to the resulting command
   structures.  */

static void
add_setshow_cmd_full (const char *name,
		      enum command_class theclass,
		      var_types var_type, void *var,
		      const char *set_doc, const char *show_doc,
		      const char *help_doc,
		      cmd_const_sfunc_ftype *set_func,
		      show_value_ftype *show_func,
		      struct cmd_list_element **set_list,
		      struct cmd_list_element **show_list,
		      struct cmd_list_element **set_result,
		      struct cmd_list_element **show_result)
{
  struct cmd_list_element *set;
  struct cmd_list_element *show;
  char *full_set_doc;
  char *full_show_doc;

  if (help_doc != NULL)
    {
      full_set_doc = xstrprintf ("%s\n%s", set_doc, help_doc);
      full_show_doc = xstrprintf ("%s\n%s", show_doc, help_doc);
    }
  else
    {
      full_set_doc = xstrdup (set_doc);
      full_show_doc = xstrdup (show_doc);
    }
  set = add_set_or_show_cmd (name, set_cmd, theclass, var_type, var,
			     full_set_doc, set_list);
  set->doc_allocated = 1;

  if (set_func != NULL)
    set_cmd_sfunc (set, set_func);

  show = add_set_or_show_cmd (name, show_cmd, theclass, var_type, var,
			      full_show_doc, show_list);
  show->doc_allocated = 1;
  show->show_value_func = show_func;
  /* Disable the default symbol completer.  Doesn't make much sense
     for the "show" command to complete on anything.  */
  set_cmd_completer (show, nullptr);

  if (set_result != NULL)
    *set_result = set;
  if (show_result != NULL)
    *show_result = show;
}

/* Add element named NAME to command list LIST (the list for set or
   some sublist thereof).  CLASS is as in add_cmd.  ENUMLIST is a list
   of strings which may follow NAME.  VAR is address of the variable
   which will contain the matching string (from ENUMLIST).  */

void
add_setshow_enum_cmd (const char *name,
		      enum command_class theclass,
		      const char *const *enumlist,
		      const char **var,
		      const char *set_doc,
		      const char *show_doc,
		      const char *help_doc,
		      cmd_const_sfunc_ftype *set_func,
		      show_value_ftype *show_func,
		      struct cmd_list_element **set_list,
		      struct cmd_list_element **show_list,
		      void *context)
{
  struct cmd_list_element *c, *show;

  add_setshow_cmd_full (name, theclass, var_enum, var,
			set_doc, show_doc, help_doc,
			set_func, show_func,
			set_list, show_list,
			&c, &show);
  c->enums = enumlist;

  set_cmd_context (c, context);
  set_cmd_context (show, context);
}

/* See cli-decode.h.  */
const char * const auto_boolean_enums[] = { "on", "off", "auto", NULL };

/* Add an auto-boolean command named NAME to both the set and show
   command list lists.  CLASS is as in add_cmd.  VAR is address of the
   variable which will contain the value.  DOC is the documentation
   string.  FUNC is the corresponding callback.  */
void
add_setshow_auto_boolean_cmd (const char *name,
			      enum command_class theclass,
			      enum auto_boolean *var,
			      const char *set_doc, const char *show_doc,
			      const char *help_doc,
			      cmd_const_sfunc_ftype *set_func,
			      show_value_ftype *show_func,
			      struct cmd_list_element **set_list,
			      struct cmd_list_element **show_list)
{
  struct cmd_list_element *c;

  add_setshow_cmd_full (name, theclass, var_auto_boolean, var,
			set_doc, show_doc, help_doc,
			set_func, show_func,
			set_list, show_list,
			&c, NULL);
  c->enums = auto_boolean_enums;
}

/* See cli-decode.h.  */
const char * const boolean_enums[] = { "on", "off", NULL };

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  CLASS is as in
   add_cmd.  VAR is address of the variable which will contain the
   value.  SET_DOC and SHOW_DOC are the documentation strings.
   Returns the new command element.  */

cmd_list_element *
add_setshow_boolean_cmd (const char *name, enum command_class theclass, bool *var,
			 const char *set_doc, const char *show_doc,
			 const char *help_doc,
			 cmd_const_sfunc_ftype *set_func,
			 show_value_ftype *show_func,
			 struct cmd_list_element **set_list,
			 struct cmd_list_element **show_list)
{
  struct cmd_list_element *c;

  add_setshow_cmd_full (name, theclass, var_boolean, var,
			set_doc, show_doc, help_doc,
			set_func, show_func,
			set_list, show_list,
			&c, NULL);
  c->enums = boolean_enums;

  return c;
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  */
void
add_setshow_filename_cmd (const char *name, enum command_class theclass,
			  char **var,
			  const char *set_doc, const char *show_doc,
			  const char *help_doc,
			  cmd_const_sfunc_ftype *set_func,
			  show_value_ftype *show_func,
			  struct cmd_list_element **set_list,
			  struct cmd_list_element **show_list)
{
  struct cmd_list_element *set_result;

  add_setshow_cmd_full (name, theclass, var_filename, var,
			set_doc, show_doc, help_doc,
			set_func, show_func,
			set_list, show_list,
			&set_result, NULL);
  set_cmd_completer (set_result, filename_completer);
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  */
void
add_setshow_string_cmd (const char *name, enum command_class theclass,
			char **var,
			const char *set_doc, const char *show_doc,
			const char *help_doc,
			cmd_const_sfunc_ftype *set_func,
			show_value_ftype *show_func,
			struct cmd_list_element **set_list,
			struct cmd_list_element **show_list)
{
  cmd_list_element *set_cmd;

  add_setshow_cmd_full (name, theclass, var_string, var,
			set_doc, show_doc, help_doc,
			set_func, show_func,
			set_list, show_list,
			&set_cmd, NULL);

  /* Disable the default symbol completer.  */
  set_cmd_completer (set_cmd, nullptr);
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  */
struct cmd_list_element *
add_setshow_string_noescape_cmd (const char *name, enum command_class theclass,
				 char **var,
				 const char *set_doc, const char *show_doc,
				 const char *help_doc,
				 cmd_const_sfunc_ftype *set_func,
				 show_value_ftype *show_func,
				 struct cmd_list_element **set_list,
				 struct cmd_list_element **show_list)
{
  struct cmd_list_element *set_cmd;

  add_setshow_cmd_full (name, theclass, var_string_noescape, var,
			set_doc, show_doc, help_doc,
			set_func, show_func,
			set_list, show_list,
			&set_cmd, NULL);

  /* Disable the default symbol completer.  */
  set_cmd_completer (set_cmd, nullptr);

  return set_cmd;
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  */
void
add_setshow_optional_filename_cmd (const char *name, enum command_class theclass,
				   char **var,
				   const char *set_doc, const char *show_doc,
				   const char *help_doc,
				   cmd_const_sfunc_ftype *set_func,
				   show_value_ftype *show_func,
				   struct cmd_list_element **set_list,
				   struct cmd_list_element **show_list)
{
  struct cmd_list_element *set_result;
 
  add_setshow_cmd_full (name, theclass, var_optional_filename, var,
			set_doc, show_doc, help_doc,
			set_func, show_func,
			set_list, show_list,
			&set_result, NULL);
		
  set_cmd_completer (set_result, filename_completer);

}

/* Completes on literal "unlimited".  Used by integer commands that
   support a special "unlimited" value.  */

static void
integer_unlimited_completer (struct cmd_list_element *ignore,
			     completion_tracker &tracker,
			     const char *text, const char *word)
{
  static const char * const keywords[] =
    {
      "unlimited",
      NULL,
    };

  complete_on_enum (tracker, keywords, text, word);
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  CLASS is as in
   add_cmd.  VAR is address of the variable which will contain the
   value.  SET_DOC and SHOW_DOC are the documentation strings.  This
   function is only used in Python API.  Please don't use it elsewhere.  */
void
add_setshow_integer_cmd (const char *name, enum command_class theclass,
			 int *var,
			 const char *set_doc, const char *show_doc,
			 const char *help_doc,
			 cmd_const_sfunc_ftype *set_func,
			 show_value_ftype *show_func,
			 struct cmd_list_element **set_list,
			 struct cmd_list_element **show_list)
{
  struct cmd_list_element *set;

  add_setshow_cmd_full (name, theclass, var_integer, var,
			set_doc, show_doc, help_doc,
			set_func, show_func,
			set_list, show_list,
			&set, NULL);

  set_cmd_completer (set, integer_unlimited_completer);
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  CLASS is as in
   add_cmd.  VAR is address of the variable which will contain the
   value.  SET_DOC and SHOW_DOC are the documentation strings.  */
void
add_setshow_uinteger_cmd (const char *name, enum command_class theclass,
			  unsigned int *var,
			  const char *set_doc, const char *show_doc,
			  const char *help_doc,
			  cmd_const_sfunc_ftype *set_func,
			  show_value_ftype *show_func,
			  struct cmd_list_element **set_list,
			  struct cmd_list_element **show_list)
{
  struct cmd_list_element *set;

  add_setshow_cmd_full (name, theclass, var_uinteger, var,
			set_doc, show_doc, help_doc,
			set_func, show_func,
			set_list, show_list,
			&set, NULL);

  set_cmd_completer (set, integer_unlimited_completer);
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  CLASS is as in
   add_cmd.  VAR is address of the variable which will contain the
   value.  SET_DOC and SHOW_DOC are the documentation strings.  */
void
add_setshow_zinteger_cmd (const char *name, enum command_class theclass,
			  int *var,
			  const char *set_doc, const char *show_doc,
			  const char *help_doc,
			  cmd_const_sfunc_ftype *set_func,
			  show_value_ftype *show_func,
			  struct cmd_list_element **set_list,
			  struct cmd_list_element **show_list)
{
  add_setshow_cmd_full (name, theclass, var_zinteger, var,
			set_doc, show_doc, help_doc,
			set_func, show_func,
			set_list, show_list,
			NULL, NULL);
}

void
add_setshow_zuinteger_unlimited_cmd (const char *name,
				     enum command_class theclass,
				     int *var,
				     const char *set_doc,
				     const char *show_doc,
				     const char *help_doc,
				     cmd_const_sfunc_ftype *set_func,
				     show_value_ftype *show_func,
				     struct cmd_list_element **set_list,
				     struct cmd_list_element **show_list)
{
  struct cmd_list_element *set;

  add_setshow_cmd_full (name, theclass, var_zuinteger_unlimited, var,
			set_doc, show_doc, help_doc,
			set_func, show_func,
			set_list, show_list,
			&set, NULL);

  set_cmd_completer (set, integer_unlimited_completer);
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  CLASS is as in
   add_cmd.  VAR is address of the variable which will contain the
   value.  SET_DOC and SHOW_DOC are the documentation strings.  */
void
add_setshow_zuinteger_cmd (const char *name, enum command_class theclass,
			   unsigned int *var,
			   const char *set_doc, const char *show_doc,
			   const char *help_doc,
			   cmd_const_sfunc_ftype *set_func,
			   show_value_ftype *show_func,
			   struct cmd_list_element **set_list,
			   struct cmd_list_element **show_list)
{
  add_setshow_cmd_full (name, theclass, var_zuinteger, var,
			set_doc, show_doc, help_doc,
			set_func, show_func,
			set_list, show_list,
			NULL, NULL);
}

/* Remove the command named NAME from the command list.  Return the
   list commands which were aliased to the deleted command.  If the
   command had no aliases, return NULL.  The various *HOOKs are set to
   the pre- and post-hook commands for the deleted command.  If the
   command does not have a hook, the corresponding out parameter is
   set to NULL.  */

static struct cmd_list_element *
delete_cmd (const char *name, struct cmd_list_element **list,
	    struct cmd_list_element **prehook,
	    struct cmd_list_element **prehookee,
	    struct cmd_list_element **posthook,
	    struct cmd_list_element **posthookee)
{
  struct cmd_list_element *iter;
  struct cmd_list_element **previous_chain_ptr;
  struct cmd_list_element *aliases = NULL;

  *prehook = NULL;
  *prehookee = NULL;
  *posthook = NULL;
  *posthookee = NULL;
  previous_chain_ptr = list;

  for (iter = *previous_chain_ptr; iter; iter = *previous_chain_ptr)
    {
      if (strcmp (iter->name, name) == 0)
	{
	  if (iter->destroyer)
	    iter->destroyer (iter, iter->context);
	  if (iter->hookee_pre)
	    iter->hookee_pre->hook_pre = 0;
	  *prehook = iter->hook_pre;
	  *prehookee = iter->hookee_pre;
	  if (iter->hookee_post)
	    iter->hookee_post->hook_post = 0;
	  *posthook = iter->hook_post;
	  *posthookee = iter->hookee_post;

	  /* Update the link.  */
	  *previous_chain_ptr = iter->next;

	  aliases = iter->aliases;

	  /* If this command was an alias, remove it from the list of
	     aliases.  */
	  if (iter->cmd_pointer)
	    {
	      struct cmd_list_element **prevp = &iter->cmd_pointer->aliases;
	      struct cmd_list_element *a = *prevp;

	      while (a != iter)
		{
		  prevp = &a->alias_chain;
		  a = *prevp;
		}
	      *prevp = iter->alias_chain;
	    }

	  delete iter;

	  /* We won't see another command with the same name.  */
	  break;
	}
      else
	previous_chain_ptr = &iter->next;
    }

  return aliases;
}

/* Shorthands to the commands above.  */

/* Add an element to the list of info subcommands.  */

struct cmd_list_element *
add_info (const char *name, cmd_const_cfunc_ftype *fun, const char *doc)
{
  return add_cmd (name, class_info, fun, doc, &infolist);
}

/* Add an alias to the list of info subcommands.  */

struct cmd_list_element *
add_info_alias (const char *name, const char *oldname, int abbrev_flag)
{
  return add_alias_cmd (name, oldname, class_run, abbrev_flag, &infolist);
}

/* Add an element to the list of commands.  */

struct cmd_list_element *
add_com (const char *name, enum command_class theclass,
	 cmd_const_cfunc_ftype *fun,
	 const char *doc)
{
  return add_cmd (name, theclass, fun, doc, &cmdlist);
}

/* Add an alias or abbreviation command to the list of commands.
   For aliases predefined by GDB (such as bt), THECLASS must be
   different of class_alias, as class_alias is used to identify
   user defined aliases.  */

struct cmd_list_element *
add_com_alias (const char *name, const char *oldname, enum command_class theclass,
	       int abbrev_flag)
{
  return add_alias_cmd (name, oldname, theclass, abbrev_flag, &cmdlist);
}

/* Add an element with a suppress notification to the list of commands.  */

struct cmd_list_element *
add_com_suppress_notification (const char *name, enum command_class theclass,
			       cmd_const_cfunc_ftype *fun, const char *doc,
			       int *suppress_notification)
{
  return add_cmd_suppress_notification (name, theclass, fun, doc,
					&cmdlist, suppress_notification);
}

/* Print the prefix of C followed by name of C in title style.  */

static void
fput_command_name_styled (struct cmd_list_element *c, struct ui_file *stream)
{
  const char *prefixname
    = c->prefix == nullptr ? "" : c->prefix->prefixname;

  fprintf_styled (stream, title_style.style (), "%s%s", prefixname, c->name);
}

/* Print the definition of alias C using title style for alias
   and aliased command.  */

static void
fput_alias_definition_styled (struct cmd_list_element *c,
			      struct ui_file *stream)
{
  gdb_assert (c->cmd_pointer != nullptr);
  fputs_filtered ("  alias ", stream);
  fput_command_name_styled (c, stream);
  fprintf_filtered (stream, " = ");
  fput_command_name_styled (c->cmd_pointer, stream);
  fprintf_filtered (stream, " %s\n", c->default_args.c_str ());
}

/* Print the definition of the aliases of CMD that have default args.  */

static void
fput_aliases_definition_styled (struct cmd_list_element *cmd,
				struct ui_file *stream)
{
  if (cmd->aliases != nullptr)
    {
      for (cmd_list_element *iter = cmd->aliases;
	   iter;
	   iter = iter->alias_chain)
	{
	  if (!iter->default_args.empty ())
	    fput_alias_definition_styled (iter, stream);
	}
    }
}


/* If C has one or more aliases, style print the name of C and
   the name of its aliases, separated by commas.
   If ALWAYS_FPUT_C_NAME, print the name of C even if it has no aliases.
   If one or more names are printed, POSTFIX is printed after the last name.
*/

static void
fput_command_names_styled (struct cmd_list_element *c,
			   bool always_fput_c_name, const char *postfix,
			   struct ui_file *stream)
{
  if (always_fput_c_name ||  c->aliases != nullptr)
    fput_command_name_styled (c, stream);
  if (c->aliases != nullptr)
    {
      for (cmd_list_element *iter = c->aliases; iter; iter = iter->alias_chain)
	{
	  fputs_filtered (", ", stream);
	  wrap_here ("   ");
	  fput_command_name_styled (iter, stream);
	}
    }
  if (always_fput_c_name ||  c->aliases != nullptr)
    fputs_filtered (postfix, stream);
}

/* If VERBOSE, print the full help for command C and highlight the
   documentation parts matching HIGHLIGHT,
   otherwise print only one-line help for command C.  */

static void
print_doc_of_command (struct cmd_list_element *c, const char *prefix,
		      bool verbose, compiled_regex &highlight,
		      struct ui_file *stream)
{
  /* When printing the full documentation, add a line to separate
     this documentation from the previous command help, in the likely
     case that apropos finds several commands.  */
  if (verbose)
    fputs_filtered ("\n", stream);

  fput_command_names_styled (c, true,
			     verbose ? "" : " -- ", stream);
  if (verbose)
    {
      fputs_filtered ("\n", stream);
      fput_aliases_definition_styled (c, stream);
      fputs_highlighted (c->doc, highlight, stream);
      fputs_filtered ("\n", stream);
    }
  else
    {
      print_doc_line (stream, c->doc, false);
      fputs_filtered ("\n", stream);
      fput_aliases_definition_styled (c, stream);
    }
}

/* Recursively walk the commandlist structures, and print out the
   documentation of commands that match our regex in either their
   name, or their documentation.
   If VERBOSE, prints the complete documentation and highlight the
   documentation parts matching REGEX, otherwise prints only
   the first line.
*/
void
apropos_cmd (struct ui_file *stream,
	     struct cmd_list_element *commandlist,
	     bool verbose, compiled_regex &regex, const char *prefix)
{
  struct cmd_list_element *c;
  int returnvalue;

  /* Walk through the commands.  */
  for (c=commandlist;c;c=c->next)
    {
      if (c->cmd_pointer != nullptr)
	{
	  /* Command aliases/abbreviations are skipped to ensure we print the
	     doc of a command only once, when encountering the aliased
	     command.  */
	  continue;
	}

      returnvalue = -1; /* Needed to avoid double printing.  */
      if (c->name != NULL)
	{
	  size_t name_len = strlen (c->name);

	  /* Try to match against the name.  */
	  returnvalue = regex.search (c->name, name_len, 0, name_len, NULL);
	  if (returnvalue >= 0)
	    print_doc_of_command (c, prefix, verbose, regex, stream);

	  /* Try to match against the name of the aliases.  */
	  for (cmd_list_element *iter = c->aliases;
	       returnvalue < 0 && iter;
	       iter = iter->alias_chain)
	    {
	      name_len = strlen (iter->name);
	      returnvalue = regex.search (iter->name, name_len, 0, name_len, NULL);
	      if (returnvalue >= 0)
		print_doc_of_command (c, prefix, verbose, regex, stream);
	    }
	}
      if (c->doc != NULL && returnvalue < 0)
	{
	  size_t doc_len = strlen (c->doc);

	  /* Try to match against documentation.  */
	  if (regex.search (c->doc, doc_len, 0, doc_len, NULL) >= 0)
	    print_doc_of_command (c, prefix, verbose, regex, stream);
	}
      /* Check if this command has subcommands.  */
      if (c->prefixlist != NULL)
	{
	  /* Recursively call ourselves on the subcommand list,
	     passing the right prefix in.  */
	  apropos_cmd (stream, *c->prefixlist, verbose, regex, c->prefixname);
	}
    }
}

/* This command really has to deal with two things:
   1) I want documentation on *this string* (usually called by
      "help commandname").

   2) I want documentation on *this list* (usually called by giving a
      command that requires subcommands.  Also called by saying just
      "help".)

   I am going to split this into two separate commands, help_cmd and
   help_list.  */

void
help_cmd (const char *command, struct ui_file *stream)
{
  struct cmd_list_element *c, *alias, *prefix_cmd, *c_cmd;

  if (!command)
    {
      help_list (cmdlist, "", all_classes, stream);
      return;
    }

  if (strcmp (command, "all") == 0)
    {
      help_all (stream);
      return;
    }

  const char *orig_command = command;
  c = lookup_cmd (&command, cmdlist, "", NULL, 0, 0);

  if (c == 0)
    return;

  lookup_cmd_composition (orig_command, &alias, &prefix_cmd, &c_cmd);

  /* There are three cases here.
     If c->prefixlist is nonzero, we have a prefix command.
     Print its documentation, then list its subcommands.

     If c->func is non NULL, we really have a command.  Print its
     documentation and return.

     If c->func is NULL, we have a class name.  Print its
     documentation (as if it were a command) and then set class to the
     number of this class so that the commands in the class will be
     listed.  */

  /* If the user asked 'help somecommand' and there is no alias,
     the false indicates to not output the (single) command name.  */
  fput_command_names_styled (c, false, "\n", stream);
  fput_aliases_definition_styled (c, stream);
  fputs_filtered (c->doc, stream);
  fputs_filtered ("\n", stream);

  if (c->prefixlist == 0 && c->func != NULL)
    return;
  fprintf_filtered (stream, "\n");

  /* If this is a prefix command, print it's subcommands.  */
  if (c->prefixlist)
    help_list (*c->prefixlist, c->prefixname, all_commands, stream);

  /* If this is a class name, print all of the commands in the class.  */
  if (c->func == NULL)
    help_list (cmdlist, "", c->theclass, stream);

  if (c->hook_pre || c->hook_post)
    fprintf_filtered (stream,
                      "\nThis command has a hook (or hooks) defined:\n");

  if (c->hook_pre)
    fprintf_filtered (stream,
                      "\tThis command is run after  : %s (pre hook)\n",
                    c->hook_pre->name);
  if (c->hook_post)
    fprintf_filtered (stream,
                      "\tThis command is run before : %s (post hook)\n",
                    c->hook_post->name);
}

/*
 * Get a specific kind of help on a command list.
 *
 * LIST is the list.
 * CMDTYPE is the prefix to use in the title string.
 * CLASS is the class with which to list the nodes of this list (see
 * documentation for help_cmd_list below),  As usual, ALL_COMMANDS for
 * everything, ALL_CLASSES for just classes, and non-negative for only things
 * in a specific class.
 * and STREAM is the output stream on which to print things.
 * If you call this routine with a class >= 0, it recurses.
 */
void
help_list (struct cmd_list_element *list, const char *cmdtype,
	   enum command_class theclass, struct ui_file *stream)
{
  int len;
  char *cmdtype1, *cmdtype2;

  /* If CMDTYPE is "foo ", CMDTYPE1 gets " foo" and CMDTYPE2 gets "foo sub".
   */
  len = strlen (cmdtype);
  cmdtype1 = (char *) alloca (len + 1);
  cmdtype1[0] = 0;
  cmdtype2 = (char *) alloca (len + 4);
  cmdtype2[0] = 0;
  if (len)
    {
      cmdtype1[0] = ' ';
      memcpy (cmdtype1 + 1, cmdtype, len - 1);
      cmdtype1[len] = 0;
      memcpy (cmdtype2, cmdtype, len - 1);
      strcpy (cmdtype2 + len - 1, " sub");
    }

  if (theclass == all_classes)
    fprintf_filtered (stream, "List of classes of %scommands:\n\n", cmdtype2);
  else
    fprintf_filtered (stream, "List of %scommands:\n\n", cmdtype2);

  help_cmd_list (list, theclass, theclass >= 0, stream);

  if (theclass == all_classes)
    {
      fprintf_filtered (stream, "\n\
Type \"help%s\" followed by a class name for a list of commands in ",
			cmdtype1);
      wrap_here ("");
      fprintf_filtered (stream, "that class.");

      fprintf_filtered (stream, "\n\
Type \"help all\" for the list of all commands.");
    }

  fprintf_filtered (stream, "\nType \"help%s\" followed by %scommand name ",
		    cmdtype1, cmdtype2);
  wrap_here ("");
  fputs_filtered ("for ", stream);
  wrap_here ("");
  fputs_filtered ("full ", stream);
  wrap_here ("");
  fputs_filtered ("documentation.\n", stream);
  fputs_filtered ("Type \"apropos word\" to search "
		  "for commands related to \"word\".\n", stream);
  fputs_filtered ("Type \"apropos -v word\" for full documentation", stream);
  wrap_here ("");
  fputs_filtered (" of commands related to \"word\".\n", stream);
  fputs_filtered ("Command name abbreviations are allowed if unambiguous.\n",
		  stream);
}

static void
help_all (struct ui_file *stream)
{
  struct cmd_list_element *c;
  int seen_unclassified = 0;

  for (c = cmdlist; c; c = c->next)
    {
      if (c->abbrev_flag)
        continue;
      /* If this is a class name, print all of the commands in the
	 class.  */

      if (c->func == NULL)
	{
	  fprintf_filtered (stream, "\nCommand class: %s\n\n", c->name);
	  help_cmd_list (cmdlist, c->theclass, true, stream);
	}
    }

  /* While it's expected that all commands are in some class,
     as a safety measure, we'll print commands outside of any
     class at the end.  */

  for (c = cmdlist; c; c = c->next)
    {
      if (c->abbrev_flag)
        continue;

      if (c->theclass == no_class)
	{
	  if (!seen_unclassified)
	    {
	      fprintf_filtered (stream, "\nUnclassified commands\n\n");
	      seen_unclassified = 1;
	    }
	  print_help_for_command (c, true, stream);
	}
    }

}

/* See cli-decode.h.  */

void
print_doc_line (struct ui_file *stream, const char *str,
		bool for_value_prefix)
{
  static char *line_buffer = 0;
  static int line_size;
  const char *p;

  if (!line_buffer)
    {
      line_size = 80;
      line_buffer = (char *) xmalloc (line_size);
    }

  /* Searches for the first end of line or the end of STR.  */
  p = str;
  while (*p && *p != '\n')
    p++;
  if (p - str > line_size - 1)
    {
      line_size = p - str + 1;
      xfree (line_buffer);
      line_buffer = (char *) xmalloc (line_size);
    }
  strncpy (line_buffer, str, p - str);
  if (for_value_prefix)
    {
      if (islower (line_buffer[0]))
	line_buffer[0] = toupper (line_buffer[0]);
      gdb_assert (p > str);
      if (line_buffer[p - str - 1] == '.')
	line_buffer[p - str - 1] = '\0';
      else
	line_buffer[p - str] = '\0';
    }
  else
    line_buffer[p - str] = '\0';
  fputs_filtered (line_buffer, stream);
}

/* Print one-line help for command C.
   If RECURSE is non-zero, also print one-line descriptions
   of all prefixed subcommands.  */
static void
print_help_for_command (struct cmd_list_element *c,
			bool recurse, struct ui_file *stream)
{
  fput_command_names_styled (c, true, " -- ", stream);
  print_doc_line (stream, c->doc, false);
  fputs_filtered ("\n", stream);
  if (!c->default_args.empty ())
    fput_alias_definition_styled (c, stream);
  fput_aliases_definition_styled (c, stream);

  if (recurse
      && c->prefixlist != 0
      && c->abbrev_flag == 0)
    /* Subcommands of a prefix command typically have 'all_commands'
       as class.  If we pass CLASS to recursive invocation,
       most often we won't see anything.  */
    help_cmd_list (*c->prefixlist, all_commands, true, stream);
}

/*
 * Implement a help command on command list LIST.
 * RECURSE should be non-zero if this should be done recursively on
 * all sublists of LIST.
 * STREAM is the stream upon which the output should be written.
 * THECLASS should be:
 *      A non-negative class number to list only commands in that
 *      ALL_COMMANDS to list all commands in list.
 *      ALL_CLASSES  to list all classes in list.
 *
 *   Note that aliases are only shown when THECLASS is class_alias.
 *   In the other cases, the aliases will be shown together with their
 *   aliased command.
 *
 *   Note that RECURSE will be active on *all* sublists, not just the
 * ones selected by the criteria above (ie. the selection mechanism
 * is at the low level, not the high-level).
 */

static void
help_cmd_list (struct cmd_list_element *list, enum command_class theclass,
	       bool recurse, struct ui_file *stream)
{
  struct cmd_list_element *c;

  for (c = list; c; c = c->next)
    {
      if (c->abbrev_flag == 1 || c->cmd_deprecated)
	{
	  /* Do not show abbreviations or deprecated commands.  */
	  continue;
	}

      if (c->cmd_pointer != nullptr && theclass != class_alias)
	{
	  /* Do not show an alias, unless specifically showing the
	     list of aliases:  for all other classes, an alias is
	     shown (if needed) together with its aliased command.  */
	  continue;
	}

      if (theclass == all_commands
	  || (theclass == all_classes && c->func == NULL)
	  || (theclass == c->theclass && c->func != NULL))
	{
	  /* show C when
             - showing all commands
	     - showing all classes and C is a help class
	     - showing commands of THECLASS and C is not the help class  */

	  /* If we show the class_alias and C is an alias, do not recurse,
	     as this would show the (possibly very long) not very useful
	     list of sub-commands of the aliased command.  */
	  print_help_for_command
	    (c,
	     recurse && (theclass != class_alias || c->cmd_pointer == nullptr),
	     stream);
	  continue;
	}

      if (recurse
	  && (theclass == class_user || theclass == class_alias)
	  && c->prefixlist != NULL)
	{
	  /* User-defined commands or aliases may be subcommands.  */
	  help_cmd_list (*c->prefixlist, theclass, recurse, stream);
	  continue;
	}

      /* Do not show C or recurse on C, e.g. because C does not belong to
	 THECLASS or because C is a help class.  */
    }
}


/* Search the input clist for 'command'.  Return the command if
   found (or NULL if not), and return the number of commands
   found in nfound.  */

static struct cmd_list_element *
find_cmd (const char *command, int len, struct cmd_list_element *clist,
	  int ignore_help_classes, int *nfound)
{
  struct cmd_list_element *found, *c;

  found = NULL;
  *nfound = 0;
  for (c = clist; c; c = c->next)
    if (!strncmp (command, c->name, len)
	&& (!ignore_help_classes || c->func))
      {
	found = c;
	(*nfound)++;
	if (c->name[len] == '\0')
	  {
	    *nfound = 1;
	    break;
	  }
      }
  return found;
}

/* Return the length of command name in TEXT.  */

int
find_command_name_length (const char *text)
{
  const char *p = text;

  /* Treating underscores as part of command words is important
     so that "set args_foo()" doesn't get interpreted as
     "set args _foo()".  */
  /* Some characters are only used for TUI specific commands.
     However, they are always allowed for the sake of consistency.

     Note that this is larger than the character set allowed when
     creating user-defined commands.  */

  /* Recognize the single character commands so that, e.g., "!ls"
     works as expected.  */
  if (*p == '!' || *p == '|')
    return 1;

  while (valid_cmd_char_p (*p)
	 /* Characters used by TUI specific commands.  */
	 || *p == '+' || *p == '<' || *p == '>' || *p == '$')
    p++;

  return p - text;
}

/* See command.h.  */

bool
valid_cmd_char_p (int c)
{
  /* Alas "42" is a legitimate user-defined command.
     In the interests of not breaking anything we preserve that.  */

  return isalnum (c) || c == '-' || c == '_' || c == '.';
}

/* See command.h.  */

bool
valid_user_defined_cmd_name_p (const char *name)
{
  const char *p;

  if (*name == '\0')
    return false;

  for (p = name; *p != '\0'; ++p)
    {
      if (valid_cmd_char_p (*p))
	; /* Ok.  */
      else
	return false;
    }

  return true;
}

/* This routine takes a line of TEXT and a CLIST in which to start the
   lookup.  When it returns it will have incremented the text pointer past
   the section of text it matched, set *RESULT_LIST to point to the list in
   which the last word was matched, and will return a pointer to the cmd
   list element which the text matches.  It will return NULL if no match at
   all was possible.  It will return -1 (cast appropriately, ick) if ambigous
   matches are possible; in this case *RESULT_LIST will be set to point to
   the list in which there are ambiguous choices (and *TEXT will be set to
   the ambiguous text string).

   if DEFAULT_ARGS is not null, *DEFAULT_ARGS is set to the found command
   default args (possibly empty).

   If the located command was an abbreviation, this routine returns the base
   command of the abbreviation.  Note that *DEFAULT_ARGS will contain the
   default args defined for the alias.

   It does no error reporting whatsoever; control will always return
   to the superior routine.

   In the case of an ambiguous return (-1), *RESULT_LIST will be set to point
   at the prefix_command (ie. the best match) *or* (special case) will be NULL
   if no prefix command was ever found.  For example, in the case of "info a",
   "info" matches without ambiguity, but "a" could be "args" or "address", so
   *RESULT_LIST is set to the cmd_list_element for "info".  So in this case
   RESULT_LIST should not be interpreted as a pointer to the beginning of a
   list; it simply points to a specific command.  In the case of an ambiguous
   return *TEXT is advanced past the last non-ambiguous prefix (e.g.
   "info t" can be "info types" or "info target"; upon return *TEXT has been
   advanced past "info ").

   If RESULT_LIST is NULL, don't set *RESULT_LIST (but don't otherwise
   affect the operation).

   This routine does *not* modify the text pointed to by TEXT.

   If IGNORE_HELP_CLASSES is nonzero, ignore any command list elements which
   are actually help classes rather than commands (i.e. the function field of
   the struct cmd_list_element is NULL).  */

struct cmd_list_element *
lookup_cmd_1 (const char **text, struct cmd_list_element *clist,
	      struct cmd_list_element **result_list, std::string *default_args,
	      int ignore_help_classes)
{
  char *command;
  int len, nfound;
  struct cmd_list_element *found, *c;
  bool found_alias = false;
  const char *line = *text;

  while (**text == ' ' || **text == '\t')
    (*text)++;

  /* Identify the name of the command.  */
  len = find_command_name_length (*text);

  /* If nothing but whitespace, return 0.  */
  if (len == 0)
    return 0;

  /* *text and p now bracket the first command word to lookup (and
     it's length is len).  We copy this into a local temporary.  */


  command = (char *) alloca (len + 1);
  memcpy (command, *text, len);
  command[len] = '\0';

  /* Look it up.  */
  found = 0;
  nfound = 0;
  found = find_cmd (command, len, clist, ignore_help_classes, &nfound);

  /* If nothing matches, we have a simple failure.  */
  if (nfound == 0)
    return 0;

  if (nfound > 1)
    {
      if (result_list != nullptr)
	/* Will be modified in calling routine
	   if we know what the prefix command is.  */
	*result_list = 0;
      if (default_args != nullptr)
	*default_args = std::string ();
      return CMD_LIST_AMBIGUOUS;	/* Ambiguous.  */
    }

  /* We've matched something on this list.  Move text pointer forward.  */

  *text += len;

  if (found->cmd_pointer)
    {
      /* We drop the alias (abbreviation) in favor of the command it
       is pointing to.  If the alias is deprecated, though, we need to
       warn the user about it before we drop it.  Note that while we
       are warning about the alias, we may also warn about the command
       itself and we will adjust the appropriate DEPRECATED_WARN_USER
       flags.  */

      if (found->deprecated_warn_user)
	deprecated_cmd_warning (line);

      /* Return the default_args of the alias, not the default_args
	 of the command it is pointing to.  */
      if (default_args != nullptr)
	*default_args = found->default_args;
      found = found->cmd_pointer;
      found_alias = true;
    }
  /* If we found a prefix command, keep looking.  */

  if (found->prefixlist)
    {
      c = lookup_cmd_1 (text, *found->prefixlist, result_list,
			default_args, ignore_help_classes);
      if (!c)
	{
	  /* Didn't find anything; this is as far as we got.  */
	  if (result_list != nullptr)
	    *result_list = clist;
	  if (!found_alias && default_args != nullptr)
	    *default_args = found->default_args;
	  return found;
	}
      else if (c == CMD_LIST_AMBIGUOUS)
	{
	  /* We've gotten this far properly, but the next step is
	     ambiguous.  We need to set the result list to the best
	     we've found (if an inferior hasn't already set it).  */
	  if (result_list != nullptr)
	    if (!*result_list)
	      /* This used to say *result_list = *found->prefixlist.
	         If that was correct, need to modify the documentation
	         at the top of this function to clarify what is
	         supposed to be going on.  */
	      *result_list = found;
	  /* For ambiguous commands, do not return any default_args args.  */
	  if (default_args != nullptr)
	    *default_args = std::string ();
	  return c;
	}
      else
	{
	  /* We matched!  */
	  return c;
	}
    }
  else
    {
      if (result_list != nullptr)
	*result_list = clist;
      if (!found_alias && default_args != nullptr)
	*default_args = found->default_args;
      return found;
    }
}

/* All this hair to move the space to the front of cmdtype */

static void
undef_cmd_error (const char *cmdtype, const char *q)
{
  error (_("Undefined %scommand: \"%s\".  Try \"help%s%.*s\"."),
	 cmdtype,
	 q,
	 *cmdtype ? " " : "",
	 (int) strlen (cmdtype) - 1,
	 cmdtype);
}

/* Look up the contents of *LINE as a command in the command list LIST.
   LIST is a chain of struct cmd_list_element's.
   If it is found, return the struct cmd_list_element for that command,
   update *LINE to point after the command name, at the first argument
   and update *DEFAULT_ARGS (if DEFAULT_ARGS is not null) to the default
   args to prepend to the user provided args when running the command.
   Note that if the found cmd_list_element is found via an alias,
   the default args of the alias are returned.

   If not found, call error if ALLOW_UNKNOWN is zero
   otherwise (or if error returns) return zero.
   Call error if specified command is ambiguous,
   unless ALLOW_UNKNOWN is negative.
   CMDTYPE precedes the word "command" in the error message.

   If IGNORE_HELP_CLASSES is nonzero, ignore any command list
   elements which are actually help classes rather than commands (i.e.
   the function field of the struct cmd_list_element is 0).  */

struct cmd_list_element *
lookup_cmd (const char **line, struct cmd_list_element *list,
	    const char *cmdtype,
	    std::string *default_args,
	    int allow_unknown, int ignore_help_classes)
{
  struct cmd_list_element *last_list = 0;
  struct cmd_list_element *c;

  /* Note: Do not remove trailing whitespace here because this
     would be wrong for complete_command.  Jim Kingdon  */

  if (!*line)
    error (_("Lack of needed %scommand"), cmdtype);

  c = lookup_cmd_1 (line, list, &last_list, default_args, ignore_help_classes);

  if (!c)
    {
      if (!allow_unknown)
	{
	  char *q;
	  int len = find_command_name_length (*line);

	  q = (char *) alloca (len + 1);
	  strncpy (q, *line, len);
	  q[len] = '\0';
	  undef_cmd_error (cmdtype, q);
	}
      else
	return 0;
    }
  else if (c == CMD_LIST_AMBIGUOUS)
    {
      /* Ambigous.  Local values should be off prefixlist or called
         values.  */
      int local_allow_unknown = (last_list ? last_list->allow_unknown :
				 allow_unknown);
      const char *local_cmdtype = last_list ? last_list->prefixname : cmdtype;
      struct cmd_list_element *local_list =
	(last_list ? *(last_list->prefixlist) : list);

      if (local_allow_unknown < 0)
	{
	  if (last_list)
	    return last_list;	/* Found something.  */
	  else
	    return 0;		/* Found nothing.  */
	}
      else
	{
	  /* Report as error.  */
	  int amb_len;
	  char ambbuf[100];

	  for (amb_len = 0;
	       ((*line)[amb_len] && (*line)[amb_len] != ' '
		&& (*line)[amb_len] != '\t');
	       amb_len++)
	    ;

	  ambbuf[0] = 0;
	  for (c = local_list; c; c = c->next)
	    if (!strncmp (*line, c->name, amb_len))
	      {
		if (strlen (ambbuf) + strlen (c->name) + 6
		    < (int) sizeof ambbuf)
		  {
		    if (strlen (ambbuf))
		      strcat (ambbuf, ", ");
		    strcat (ambbuf, c->name);
		  }
		else
		  {
		    strcat (ambbuf, "..");
		    break;
		  }
	      }
	  error (_("Ambiguous %scommand \"%s\": %s."), local_cmdtype,
		 *line, ambbuf);
	}
    }
  else
    {
      if (c->type == set_cmd && **line != '\0' && !isspace (**line))
        error (_("Argument must be preceded by space."));

      /* We've got something.  It may still not be what the caller
         wants (if this command *needs* a subcommand).  */
      while (**line == ' ' || **line == '\t')
	(*line)++;

      if (c->prefixlist && **line && !c->allow_unknown)
	undef_cmd_error (c->prefixname, *line);

      /* Seems to be what he wants.  Return it.  */
      return c;
    }
  return 0;
}

/* We are here presumably because an alias or command in TEXT is
   deprecated and a warning message should be generated.  This
   function decodes TEXT and potentially generates a warning message
   as outlined below.
   
   Example for 'set endian big' which has a fictitious alias 'seb'.
   
   If alias wasn't used in TEXT, and the command is deprecated:
   "warning: 'set endian big' is deprecated." 
   
   If alias was used, and only the alias is deprecated:
   "warning: 'seb' an alias for the command 'set endian big' is deprecated."
   
   If alias was used and command is deprecated (regardless of whether
   the alias itself is deprecated:
   
   "warning: 'set endian big' (seb) is deprecated."

   After the message has been sent, clear the appropriate flags in the
   command and/or the alias so the user is no longer bothered.
   
*/
void
deprecated_cmd_warning (const char *text)
{
  struct cmd_list_element *alias = NULL;
  struct cmd_list_element *prefix_cmd = NULL;
  struct cmd_list_element *cmd = NULL;

  if (!lookup_cmd_composition (text, &alias, &prefix_cmd, &cmd))
    /* Return if text doesn't evaluate to a command.  */
    return;

  if (!((alias ? alias->deprecated_warn_user : 0)
      || cmd->deprecated_warn_user) ) 
    /* Return if nothing is deprecated.  */
    return;
  
  printf_filtered ("Warning:");
  
  if (alias && !cmd->cmd_deprecated)
    printf_filtered (" '%s', an alias for the", alias->name);
    
  printf_filtered (" command '");
  
  if (prefix_cmd)
    printf_filtered ("%s", prefix_cmd->prefixname);
  
  printf_filtered ("%s", cmd->name);

  if (alias && cmd->cmd_deprecated)
    printf_filtered ("' (%s) is deprecated.\n", alias->name);
  else
    printf_filtered ("' is deprecated.\n"); 
  

  /* If it is only the alias that is deprecated, we want to indicate
     the new alias, otherwise we'll indicate the new command.  */

  if (alias && !cmd->cmd_deprecated)
    {
      if (alias->replacement)
	printf_filtered ("Use '%s'.\n\n", alias->replacement);
      else
	printf_filtered ("No alternative known.\n\n");
     }  
  else
    {
      if (cmd->replacement)
	printf_filtered ("Use '%s'.\n\n", cmd->replacement);
      else
	printf_filtered ("No alternative known.\n\n");
    }

  /* We've warned you, now we'll keep quiet.  */
  if (alias)
    alias->deprecated_warn_user = 0;
  
  cmd->deprecated_warn_user = 0;
}


/* Look up the contents of TEXT as a command in the command list 'cmdlist'.
   Return 1 on success, 0 on failure.

   If TEXT refers to an alias, *ALIAS will point to that alias.

   If TEXT is a subcommand (i.e. one that is preceded by a prefix
   command) set *PREFIX_CMD.

   Set *CMD to point to the command TEXT indicates.

   If any of *ALIAS, *PREFIX_CMD, or *CMD cannot be determined or do not
   exist, they are NULL when we return.

*/
int
lookup_cmd_composition (const char *text,
			struct cmd_list_element **alias,
			struct cmd_list_element **prefix_cmd,
			struct cmd_list_element **cmd)
{
  char *command;
  int len, nfound;
  struct cmd_list_element *cur_list;
  struct cmd_list_element *prev_cmd;

  *alias = NULL;
  *prefix_cmd = NULL;
  *cmd = NULL;

  cur_list = cmdlist;

  text = skip_spaces (text);

  while (1)
    {
      /* Go through as many command lists as we need to,
	 to find the command TEXT refers to.  */

      prev_cmd = *cmd;

      /* Identify the name of the command.  */
      len = find_command_name_length (text);

      /* If nothing but whitespace, return.  */
      if (len == 0)
	return 0;

      /* TEXT is the start of the first command word to lookup (and
	 it's length is LEN).  We copy this into a local temporary.  */

      command = (char *) alloca (len + 1);
      memcpy (command, text, len);
      command[len] = '\0';

      /* Look it up.  */
      *cmd = 0;
      nfound = 0;
      *cmd = find_cmd (command, len, cur_list, 1, &nfound);

      if (*cmd == CMD_LIST_AMBIGUOUS)
	{
	  return 0;              /* ambiguous */
	}

      if (*cmd == NULL)
	return 0;                /* nothing found */
      else
	{
	  if ((*cmd)->cmd_pointer)
	    {
	      /* cmd was actually an alias, we note that an alias was
		 used (by assigning *ALIAS) and we set *CMD.  */
	      *alias = *cmd;
	      *cmd = (*cmd)->cmd_pointer;
	    }
	  *prefix_cmd = prev_cmd;
	}

      text += len;
      text = skip_spaces (text);

      if ((*cmd)->prefixlist && *text != '\0')
	cur_list = *(*cmd)->prefixlist;
      else
	return 1;
    }
}

/* Helper function for SYMBOL_COMPLETION_FUNCTION.  */

/* Return a vector of char pointers which point to the different
   possible completions in LIST of TEXT.

   WORD points in the same buffer as TEXT, and completions should be
   returned relative to this position.  For example, suppose TEXT is
   "foo" and we want to complete to "foobar".  If WORD is "oo", return
   "oobar"; if WORD is "baz/foo", return "baz/foobar".  */

void
complete_on_cmdlist (struct cmd_list_element *list,
		     completion_tracker &tracker,
		     const char *text, const char *word,
		     int ignore_help_classes)
{
  struct cmd_list_element *ptr;
  int textlen = strlen (text);
  int pass;
  int saw_deprecated_match = 0;

  /* We do one or two passes.  In the first pass, we skip deprecated
     commands.  If we see no matching commands in the first pass, and
     if we did happen to see a matching deprecated command, we do
     another loop to collect those.  */
  for (pass = 0; pass < 2; ++pass)
    {
      bool got_matches = false;

      for (ptr = list; ptr; ptr = ptr->next)
	if (!strncmp (ptr->name, text, textlen)
	    && !ptr->abbrev_flag
	    && (!ignore_help_classes || ptr->func
		|| ptr->prefixlist))
	  {
	    if (pass == 0)
	      {
		if (ptr->cmd_deprecated)
		  {
		    saw_deprecated_match = 1;
		    continue;
		  }
	      }

	    tracker.add_completion
	      (make_completion_match_str (ptr->name, text, word));
	    got_matches = true;
	  }

      if (got_matches)
	break;

      /* If we saw no matching deprecated commands in the first pass,
	 just bail out.  */
      if (!saw_deprecated_match)
	break;
    }
}

/* Helper function for SYMBOL_COMPLETION_FUNCTION.  */

/* Add the different possible completions in ENUMLIST of TEXT.

   WORD points in the same buffer as TEXT, and completions should be
   returned relative to this position.  For example, suppose TEXT is "foo"
   and we want to complete to "foobar".  If WORD is "oo", return
   "oobar"; if WORD is "baz/foo", return "baz/foobar".  */

void
complete_on_enum (completion_tracker &tracker,
		  const char *const *enumlist,
		  const char *text, const char *word)
{
  int textlen = strlen (text);
  int i;
  const char *name;

  for (i = 0; (name = enumlist[i]) != NULL; i++)
    if (strncmp (name, text, textlen) == 0)
      tracker.add_completion (make_completion_match_str (name, text, word));
}


/* Check function pointer.  */
int
cmd_func_p (struct cmd_list_element *cmd)
{
  return (cmd->func != NULL);
}


/* Call the command function.  */
void
cmd_func (struct cmd_list_element *cmd, const char *args, int from_tty)
{
  if (cmd_func_p (cmd))
    {
      gdb::optional<scoped_restore_tmpl<int>> restore_suppress;

      if (cmd->suppress_notification != NULL)
	restore_suppress.emplace (cmd->suppress_notification, 1);

      (*cmd->func) (cmd, args, from_tty);
    }
  else
    error (_("Invalid command"));
}

int
cli_user_command_p (struct cmd_list_element *cmd)
{
  return (cmd->theclass == class_user
	  && (cmd->func == do_const_cfunc || cmd->func == do_sfunc));
}
