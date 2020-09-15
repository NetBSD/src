/* Everything about syscall catchpoints, for GDB.

   Copyright (C) 2009-2020 Free Software Foundation, Inc.

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
#include <ctype.h>
#include "breakpoint.h"
#include "gdbcmd.h"
#include "inferior.h"
#include "cli/cli-utils.h"
#include "annotate.h"
#include "mi/mi-common.h"
#include "valprint.h"
#include "arch-utils.h"
#include "observable.h"
#include "xml-syscall.h"
#include "cli/cli-style.h"

/* An instance of this type is used to represent a syscall catchpoint.
   A breakpoint is really of this type iff its ops pointer points to
   CATCH_SYSCALL_BREAKPOINT_OPS.  */

struct syscall_catchpoint : public breakpoint
{
  /* Syscall numbers used for the 'catch syscall' feature.  If no
     syscall has been specified for filtering, it is empty.
     Otherwise, it holds a list of all syscalls to be caught.  */
  std::vector<int> syscalls_to_be_caught;
};

struct catch_syscall_inferior_data
{
  /* We keep a count of the number of times the user has requested a
     particular syscall to be tracked, and pass this information to the
     target.  This lets capable targets implement filtering directly.  */

  /* Number of times that "any" syscall is requested.  */
  int any_syscall_count;

  /* Count of each system call.  */
  std::vector<int> syscalls_counts;

  /* This counts all syscall catch requests, so we can readily determine
     if any catching is necessary.  */
  int total_syscalls_count;
};

static const struct inferior_key<struct catch_syscall_inferior_data>
  catch_syscall_inferior_data;

static struct catch_syscall_inferior_data *
get_catch_syscall_inferior_data (struct inferior *inf)
{
  struct catch_syscall_inferior_data *inf_data;

  inf_data = catch_syscall_inferior_data.get (inf);
  if (inf_data == NULL)
    inf_data = catch_syscall_inferior_data.emplace (inf);

  return inf_data;
}

/* Implement the "insert" breakpoint_ops method for syscall
   catchpoints.  */

static int
insert_catch_syscall (struct bp_location *bl)
{
  struct syscall_catchpoint *c = (struct syscall_catchpoint *) bl->owner;
  struct inferior *inf = current_inferior ();
  struct catch_syscall_inferior_data *inf_data
    = get_catch_syscall_inferior_data (inf);

  ++inf_data->total_syscalls_count;
  if (c->syscalls_to_be_caught.empty ())
    ++inf_data->any_syscall_count;
  else
    {
      for (int iter : c->syscalls_to_be_caught)
	{
	  if (iter >= inf_data->syscalls_counts.size ())
	    inf_data->syscalls_counts.resize (iter + 1);
	  ++inf_data->syscalls_counts[iter];
	}
    }

  return target_set_syscall_catchpoint (inferior_ptid.pid (),
					inf_data->total_syscalls_count != 0,
					inf_data->any_syscall_count,
					inf_data->syscalls_counts);
}

/* Implement the "remove" breakpoint_ops method for syscall
   catchpoints.  */

static int
remove_catch_syscall (struct bp_location *bl, enum remove_bp_reason reason)
{
  struct syscall_catchpoint *c = (struct syscall_catchpoint *) bl->owner;
  struct inferior *inf = current_inferior ();
  struct catch_syscall_inferior_data *inf_data
    = get_catch_syscall_inferior_data (inf);

  --inf_data->total_syscalls_count;
  if (c->syscalls_to_be_caught.empty ())
    --inf_data->any_syscall_count;
  else
    {
      for (int iter : c->syscalls_to_be_caught)
	{
	  if (iter >= inf_data->syscalls_counts.size ())
	    /* Shouldn't happen.  */
	    continue;
          --inf_data->syscalls_counts[iter];
        }
    }

  return target_set_syscall_catchpoint (inferior_ptid.pid (),
					inf_data->total_syscalls_count != 0,
					inf_data->any_syscall_count,
					inf_data->syscalls_counts);
}

/* Implement the "breakpoint_hit" breakpoint_ops method for syscall
   catchpoints.  */

static int
breakpoint_hit_catch_syscall (const struct bp_location *bl,
			      const address_space *aspace, CORE_ADDR bp_addr,
			      const struct target_waitstatus *ws)
{
  /* We must check if we are catching specific syscalls in this
     breakpoint.  If we are, then we must guarantee that the called
     syscall is the same syscall we are catching.  */
  int syscall_number = 0;
  const struct syscall_catchpoint *c
    = (const struct syscall_catchpoint *) bl->owner;

  if (ws->kind != TARGET_WAITKIND_SYSCALL_ENTRY
      && ws->kind != TARGET_WAITKIND_SYSCALL_RETURN)
    return 0;

  syscall_number = ws->value.syscall_number;

  /* Now, checking if the syscall is the same.  */
  if (!c->syscalls_to_be_caught.empty ())
    {
      for (int iter : c->syscalls_to_be_caught)
	if (syscall_number == iter)
	  return 1;

      return 0;
    }

  return 1;
}

/* Implement the "print_it" breakpoint_ops method for syscall
   catchpoints.  */

static enum print_stop_action
print_it_catch_syscall (bpstat bs)
{
  struct ui_out *uiout = current_uiout;
  struct breakpoint *b = bs->breakpoint_at;
  /* These are needed because we want to know in which state a
     syscall is.  It can be in the TARGET_WAITKIND_SYSCALL_ENTRY
     or TARGET_WAITKIND_SYSCALL_RETURN, and depending on it we
     must print "called syscall" or "returned from syscall".  */
  struct target_waitstatus last;
  struct syscall s;
  struct gdbarch *gdbarch = bs->bp_location_at->gdbarch;

  get_last_target_status (nullptr, nullptr, &last);

  get_syscall_by_number (gdbarch, last.value.syscall_number, &s);

  annotate_catchpoint (b->number);
  maybe_print_thread_hit_breakpoint (uiout);

  if (b->disposition == disp_del)
    uiout->text ("Temporary catchpoint ");
  else
    uiout->text ("Catchpoint ");
  if (uiout->is_mi_like_p ())
    {
      uiout->field_string ("reason",
			   async_reason_lookup (last.kind == TARGET_WAITKIND_SYSCALL_ENTRY
						? EXEC_ASYNC_SYSCALL_ENTRY
						: EXEC_ASYNC_SYSCALL_RETURN));
      uiout->field_string ("disp", bpdisp_text (b->disposition));
    }
  uiout->field_signed ("bkptno", b->number);

  if (last.kind == TARGET_WAITKIND_SYSCALL_ENTRY)
    uiout->text (" (call to syscall ");
  else
    uiout->text (" (returned from syscall ");

  if (s.name == NULL || uiout->is_mi_like_p ())
    uiout->field_signed ("syscall-number", last.value.syscall_number);
  if (s.name != NULL)
    uiout->field_string ("syscall-name", s.name);

  uiout->text ("), ");

  return PRINT_SRC_AND_LOC;
}

/* Implement the "print_one" breakpoint_ops method for syscall
   catchpoints.  */

static void
print_one_catch_syscall (struct breakpoint *b,
			 struct bp_location **last_loc)
{
  struct syscall_catchpoint *c = (struct syscall_catchpoint *) b;
  struct value_print_options opts;
  struct ui_out *uiout = current_uiout;
  struct gdbarch *gdbarch = b->loc->gdbarch;

  get_user_print_options (&opts);
  /* Field 4, the address, is omitted (which makes the columns not
     line up too nicely with the headers, but the effect is relatively
     readable).  */
  if (opts.addressprint)
    uiout->field_skip ("addr");
  annotate_field (5);

  if (c->syscalls_to_be_caught.size () > 1)
    uiout->text ("syscalls \"");
  else
    uiout->text ("syscall \"");

  if (!c->syscalls_to_be_caught.empty ())
    {
      char *text = xstrprintf ("%s", "");

      for (int iter : c->syscalls_to_be_caught)
        {
          char *previous_text = text;
          struct syscall s;
          get_syscall_by_number (gdbarch, iter, &s);

          if (s.name != NULL)
            text = xstrprintf ("%s%s, ", text, s.name);
          else
            text = xstrprintf ("%s%d, ", text, iter);

          /* We have to xfree previous_text because xstrprintf dynamically
	     allocates new space for text on every call.  */
	  xfree (previous_text);
        }
      /* Remove the last comma.  */
      text[strlen (text) - 2] = '\0';
      uiout->field_string ("what", text);
      /* xfree last text.  */
      xfree (text);
    }
  else
    uiout->field_string ("what", "<any syscall>", metadata_style.style ());
  uiout->text ("\" ");

  if (uiout->is_mi_like_p ())
    uiout->field_string ("catch-type", "syscall");
}

/* Implement the "print_mention" breakpoint_ops method for syscall
   catchpoints.  */

static void
print_mention_catch_syscall (struct breakpoint *b)
{
  struct syscall_catchpoint *c = (struct syscall_catchpoint *) b;
  struct gdbarch *gdbarch = b->loc->gdbarch;

  if (!c->syscalls_to_be_caught.empty ())
    {
      if (c->syscalls_to_be_caught.size () > 1)
        printf_filtered (_("Catchpoint %d (syscalls"), b->number);
      else
        printf_filtered (_("Catchpoint %d (syscall"), b->number);

      for (int iter : c->syscalls_to_be_caught)
        {
          struct syscall s;
          get_syscall_by_number (gdbarch, iter, &s);

          if (s.name != NULL)
            printf_filtered (" '%s' [%d]", s.name, s.number);
          else
            printf_filtered (" %d", s.number);
        }
      printf_filtered (")");
    }
  else
    printf_filtered (_("Catchpoint %d (any syscall)"),
                     b->number);
}

/* Implement the "print_recreate" breakpoint_ops method for syscall
   catchpoints.  */

static void
print_recreate_catch_syscall (struct breakpoint *b, struct ui_file *fp)
{
  struct syscall_catchpoint *c = (struct syscall_catchpoint *) b;
  struct gdbarch *gdbarch = b->loc->gdbarch;

  fprintf_unfiltered (fp, "catch syscall");

  for (int iter : c->syscalls_to_be_caught)
    {
      struct syscall s;

      get_syscall_by_number (gdbarch, iter, &s);
      if (s.name != NULL)
	fprintf_unfiltered (fp, " %s", s.name);
      else
	fprintf_unfiltered (fp, " %d", s.number);
    }

  print_recreate_thread (b, fp);
}

/* The breakpoint_ops structure to be used in syscall catchpoints.  */

static struct breakpoint_ops catch_syscall_breakpoint_ops;

/* Returns non-zero if 'b' is a syscall catchpoint.  */

static int
syscall_catchpoint_p (struct breakpoint *b)
{
  return (b->ops == &catch_syscall_breakpoint_ops);
}

static void
create_syscall_event_catchpoint (int tempflag, std::vector<int> &&filter,
                                 const struct breakpoint_ops *ops)
{
  struct gdbarch *gdbarch = get_current_arch ();

  std::unique_ptr<syscall_catchpoint> c (new syscall_catchpoint ());
  init_catchpoint (c.get (), gdbarch, tempflag, NULL, ops);
  c->syscalls_to_be_caught = std::move (filter);

  install_breakpoint (0, std::move (c), 1);
}

/* Splits the argument using space as delimiter.  */

static std::vector<int>
catch_syscall_split_args (const char *arg)
{
  std::vector<int> result;
  struct gdbarch *gdbarch = target_gdbarch ();

  while (*arg != '\0')
    {
      int i, syscall_number;
      char *endptr;
      char cur_name[128];
      struct syscall s;

      /* Skip whitespace.  */
      arg = skip_spaces (arg);

      for (i = 0; i < 127 && arg[i] && !isspace (arg[i]); ++i)
	cur_name[i] = arg[i];
      cur_name[i] = '\0';
      arg += i;

      /* Check if the user provided a syscall name, group, or a number.  */
      syscall_number = (int) strtol (cur_name, &endptr, 0);
      if (*endptr == '\0')
	{
	  get_syscall_by_number (gdbarch, syscall_number, &s);
	  result.push_back (s.number);
	}
      else if (startswith (cur_name, "g:")
	       || startswith (cur_name, "group:"))
	{
	  /* We have a syscall group.  Let's expand it into a syscall
	     list before inserting.  */
	  const char *group_name;

	  /* Skip over "g:" and "group:" prefix strings.  */
	  group_name = strchr (cur_name, ':') + 1;

	  if (!get_syscalls_by_group (gdbarch, group_name, &result))
	    error (_("Unknown syscall group '%s'."), group_name);
	}
      else
	{
	  /* We have a name.  Let's check if it's valid and fetch a
	     list of matching numbers.  */
	  if (!get_syscalls_by_name (gdbarch, cur_name, &result))
	    /* Here we have to issue an error instead of a warning,
	       because GDB cannot do anything useful if there's no
	       syscall number to be caught.  */
	    error (_("Unknown syscall name '%s'."), cur_name);
	}
    }

  return result;
}

/* Implement the "catch syscall" command.  */

static void
catch_syscall_command_1 (const char *arg, int from_tty, 
			 struct cmd_list_element *command)
{
  int tempflag;
  std::vector<int> filter;
  struct syscall s;
  struct gdbarch *gdbarch = get_current_arch ();

  /* Checking if the feature if supported.  */
  if (gdbarch_get_syscall_number_p (gdbarch) == 0)
    error (_("The feature 'catch syscall' is not supported on \
this architecture yet."));

  tempflag = get_cmd_context (command) == CATCH_TEMPORARY;

  arg = skip_spaces (arg);

  /* We need to do this first "dummy" translation in order
     to get the syscall XML file loaded or, most important,
     to display a warning to the user if there's no XML file
     for his/her architecture.  */
  get_syscall_by_number (gdbarch, 0, &s);

  /* The allowed syntax is:
     catch syscall
     catch syscall <name | number> [<name | number> ... <name | number>]

     Let's check if there's a syscall name.  */

  if (arg != NULL)
    filter = catch_syscall_split_args (arg);

  create_syscall_event_catchpoint (tempflag, std::move (filter),
				   &catch_syscall_breakpoint_ops);
}


/* Returns 0 if 'bp' is NOT a syscall catchpoint,
   non-zero otherwise.  */
static int
is_syscall_catchpoint_enabled (struct breakpoint *bp)
{
  if (syscall_catchpoint_p (bp)
      && bp->enable_state != bp_disabled
      && bp->enable_state != bp_call_disabled)
    return 1;
  else
    return 0;
}

int
catch_syscall_enabled (void)
{
  struct catch_syscall_inferior_data *inf_data
    = get_catch_syscall_inferior_data (current_inferior ());

  return inf_data->total_syscalls_count != 0;
}

/* Helper function for catching_syscall_number.  If B is a syscall
   catchpoint for SYSCALL_NUMBER, return 1 (which will make
   'breakpoint_find_if' return).  Otherwise, return 0.  */

static int
catching_syscall_number_1 (struct breakpoint *b,
			   void *data)
{
  int syscall_number = (int) (uintptr_t) data;

  if (is_syscall_catchpoint_enabled (b))
    {
      struct syscall_catchpoint *c = (struct syscall_catchpoint *) b;

      if (!c->syscalls_to_be_caught.empty ())
	{
	  for (int iter : c->syscalls_to_be_caught)
	    if (syscall_number == iter)
	      return 1;
	}
      else
	return 1;
    }

  return 0;
}

int
catching_syscall_number (int syscall_number)
{
  struct breakpoint *b = breakpoint_find_if (catching_syscall_number_1,
					 (void *) (uintptr_t) syscall_number);

  return b != NULL;
}

/* Complete syscall names.  Used by "catch syscall".  */

static void
catch_syscall_completer (struct cmd_list_element *cmd,
			 completion_tracker &tracker,
                         const char *text, const char *word)
{
  struct gdbarch *gdbarch = get_current_arch ();
  gdb::unique_xmalloc_ptr<const char *> group_list;
  const char *prefix;

  /* Completion considers ':' to be a word separator, so we use this to
     verify whether the previous word was a group prefix.  If so, we
     build the completion list using group names only.  */
  for (prefix = word; prefix != text && prefix[-1] != ' '; prefix--)
    ;

  if (startswith (prefix, "g:") || startswith (prefix, "group:"))
    {
      /* Perform completion inside 'group:' namespace only.  */
      group_list.reset (get_syscall_group_names (gdbarch));
      if (group_list != NULL)
	complete_on_enum (tracker, group_list.get (), word, word);
    }
  else
    {
      /* Complete with both, syscall names and groups.  */
      gdb::unique_xmalloc_ptr<const char *> syscall_list
	(get_syscall_names (gdbarch));
      group_list.reset (get_syscall_group_names (gdbarch));

      const char **group_ptr = group_list.get ();

      /* Hold on to strings while we're using them.  */
      std::vector<std::string> holders;

      /* Append "group:" prefix to syscall groups.  */
      for (int i = 0; group_ptr[i] != NULL; i++)
	holders.push_back (string_printf ("group:%s", group_ptr[i]));

      for (int i = 0; group_ptr[i] != NULL; i++)
	group_ptr[i] = holders[i].c_str ();

      if (syscall_list != NULL)
	complete_on_enum (tracker, syscall_list.get (), word, word);
      if (group_list != NULL)
	complete_on_enum (tracker, group_ptr, word, word);
    }
}

static void
clear_syscall_counts (struct inferior *inf)
{
  struct catch_syscall_inferior_data *inf_data
    = get_catch_syscall_inferior_data (inf);

  inf_data->total_syscalls_count = 0;
  inf_data->any_syscall_count = 0;
  inf_data->syscalls_counts.clear ();
}

static void
initialize_syscall_catchpoint_ops (void)
{
  struct breakpoint_ops *ops;

  initialize_breakpoint_ops ();

  /* Syscall catchpoints.  */
  ops = &catch_syscall_breakpoint_ops;
  *ops = base_breakpoint_ops;
  ops->insert_location = insert_catch_syscall;
  ops->remove_location = remove_catch_syscall;
  ops->breakpoint_hit = breakpoint_hit_catch_syscall;
  ops->print_it = print_it_catch_syscall;
  ops->print_one = print_one_catch_syscall;
  ops->print_mention = print_mention_catch_syscall;
  ops->print_recreate = print_recreate_catch_syscall;
}

void _initialize_break_catch_syscall ();
void
_initialize_break_catch_syscall ()
{
  initialize_syscall_catchpoint_ops ();

  gdb::observers::inferior_exit.attach (clear_syscall_counts);

  add_catch_command ("syscall", _("\
Catch system calls by their names, groups and/or numbers.\n\
Arguments say which system calls to catch.  If no arguments are given,\n\
every system call will be caught.  Arguments, if given, should be one\n\
or more system call names (if your system supports that), system call\n\
groups or system call numbers."),
		     catch_syscall_command_1,
		     catch_syscall_completer,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
}
