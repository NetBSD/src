/* Everything about syscall catchpoints, for GDB.

   Copyright (C) 2009-2017 Free Software Foundation, Inc.

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
#include "observer.h"
#include "xml-syscall.h"

/* An instance of this type is used to represent a syscall catchpoint.
   It includes a "struct breakpoint" as a kind of base class; users
   downcast to "struct breakpoint *" when needed.  A breakpoint is
   really of this type iff its ops pointer points to
   CATCH_SYSCALL_BREAKPOINT_OPS.  */

struct syscall_catchpoint
{
  /* The base class.  */
  struct breakpoint base;

  /* Syscall numbers used for the 'catch syscall' feature.  If no
     syscall has been specified for filtering, its value is NULL.
     Otherwise, it holds a list of all syscalls to be caught.  The
     list elements are allocated with xmalloc.  */
  VEC(int) *syscalls_to_be_caught;
};

/* Implement the "dtor" breakpoint_ops method for syscall
   catchpoints.  */

static void
dtor_catch_syscall (struct breakpoint *b)
{
  struct syscall_catchpoint *c = (struct syscall_catchpoint *) b;

  VEC_free (int, c->syscalls_to_be_caught);

  base_breakpoint_ops.dtor (b);
}

static const struct inferior_data *catch_syscall_inferior_data = NULL;

struct catch_syscall_inferior_data
{
  /* We keep a count of the number of times the user has requested a
     particular syscall to be tracked, and pass this information to the
     target.  This lets capable targets implement filtering directly.  */

  /* Number of times that "any" syscall is requested.  */
  int any_syscall_count;

  /* Count of each system call.  */
  VEC(int) *syscalls_counts;

  /* This counts all syscall catch requests, so we can readily determine
     if any catching is necessary.  */
  int total_syscalls_count;
};

static struct catch_syscall_inferior_data*
get_catch_syscall_inferior_data (struct inferior *inf)
{
  struct catch_syscall_inferior_data *inf_data;

  inf_data = ((struct catch_syscall_inferior_data *)
	      inferior_data (inf, catch_syscall_inferior_data));
  if (inf_data == NULL)
    {
      inf_data = XCNEW (struct catch_syscall_inferior_data);
      set_inferior_data (inf, catch_syscall_inferior_data, inf_data);
    }

  return inf_data;
}

static void
catch_syscall_inferior_data_cleanup (struct inferior *inf, void *arg)
{
  xfree (arg);
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
  if (!c->syscalls_to_be_caught)
    ++inf_data->any_syscall_count;
  else
    {
      int i, iter;

      for (i = 0;
           VEC_iterate (int, c->syscalls_to_be_caught, i, iter);
           i++)
	{
          int elem;

	  if (iter >= VEC_length (int, inf_data->syscalls_counts))
	    {
              int old_size = VEC_length (int, inf_data->syscalls_counts);
              uintptr_t vec_addr_offset
		= old_size * ((uintptr_t) sizeof (int));
              uintptr_t vec_addr;
              VEC_safe_grow (int, inf_data->syscalls_counts, iter + 1);
              vec_addr = ((uintptr_t) VEC_address (int,
						  inf_data->syscalls_counts)
			  + vec_addr_offset);
              memset ((void *) vec_addr, 0,
                      (iter + 1 - old_size) * sizeof (int));
	    }
          elem = VEC_index (int, inf_data->syscalls_counts, iter);
          VEC_replace (int, inf_data->syscalls_counts, iter, ++elem);
	}
    }

  return target_set_syscall_catchpoint (ptid_get_pid (inferior_ptid),
					inf_data->total_syscalls_count != 0,
					inf_data->any_syscall_count,
					VEC_length (int,
						    inf_data->syscalls_counts),
					VEC_address (int,
						     inf_data->syscalls_counts));
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
  if (!c->syscalls_to_be_caught)
    --inf_data->any_syscall_count;
  else
    {
      int i, iter;

      for (i = 0;
           VEC_iterate (int, c->syscalls_to_be_caught, i, iter);
           i++)
	{
          int elem;
	  if (iter >= VEC_length (int, inf_data->syscalls_counts))
	    /* Shouldn't happen.  */
	    continue;
          elem = VEC_index (int, inf_data->syscalls_counts, iter);
          VEC_replace (int, inf_data->syscalls_counts, iter, --elem);
        }
    }

  return target_set_syscall_catchpoint (ptid_get_pid (inferior_ptid),
					inf_data->total_syscalls_count != 0,
					inf_data->any_syscall_count,
					VEC_length (int,
						    inf_data->syscalls_counts),
					VEC_address (int,
						     inf_data->syscalls_counts));
}

/* Implement the "breakpoint_hit" breakpoint_ops method for syscall
   catchpoints.  */

static int
breakpoint_hit_catch_syscall (const struct bp_location *bl,
			      struct address_space *aspace, CORE_ADDR bp_addr,
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
  if (c->syscalls_to_be_caught)
    {
      int i, iter;

      for (i = 0;
           VEC_iterate (int, c->syscalls_to_be_caught, i, iter);
           i++)
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
  ptid_t ptid;
  struct target_waitstatus last;
  struct syscall s;
  struct gdbarch *gdbarch = bs->bp_location_at->gdbarch;

  get_last_target_status (&ptid, &last);

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
  uiout->field_int ("bkptno", b->number);

  if (last.kind == TARGET_WAITKIND_SYSCALL_ENTRY)
    uiout->text (" (call to syscall ");
  else
    uiout->text (" (returned from syscall ");

  if (s.name == NULL || uiout->is_mi_like_p ())
    uiout->field_int ("syscall-number", last.value.syscall_number);
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

  if (c->syscalls_to_be_caught
      && VEC_length (int, c->syscalls_to_be_caught) > 1)
    uiout->text ("syscalls \"");
  else
    uiout->text ("syscall \"");

  if (c->syscalls_to_be_caught)
    {
      int i, iter;
      char *text = xstrprintf ("%s", "");

      for (i = 0;
           VEC_iterate (int, c->syscalls_to_be_caught, i, iter);
           i++)
        {
          char *x = text;
          struct syscall s;
          get_syscall_by_number (gdbarch, iter, &s);

          if (s.name != NULL)
            text = xstrprintf ("%s%s, ", text, s.name);
          else
            text = xstrprintf ("%s%d, ", text, iter);

          /* We have to xfree the last 'text' (now stored at 'x')
             because xstrprintf dynamically allocates new space for it
             on every call.  */
	  xfree (x);
        }
      /* Remove the last comma.  */
      text[strlen (text) - 2] = '\0';
      uiout->field_string ("what", text);
    }
  else
    uiout->field_string ("what", "<any syscall>");
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

  if (c->syscalls_to_be_caught)
    {
      int i, iter;

      if (VEC_length (int, c->syscalls_to_be_caught) > 1)
        printf_filtered (_("Catchpoint %d (syscalls"), b->number);
      else
        printf_filtered (_("Catchpoint %d (syscall"), b->number);

      for (i = 0;
           VEC_iterate (int, c->syscalls_to_be_caught, i, iter);
           i++)
        {
          struct syscall s;
          get_syscall_by_number (gdbarch, iter, &s);

          if (s.name)
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

  if (c->syscalls_to_be_caught)
    {
      int i, iter;

      for (i = 0;
           VEC_iterate (int, c->syscalls_to_be_caught, i, iter);
           i++)
        {
          struct syscall s;

          get_syscall_by_number (gdbarch, iter, &s);
          if (s.name)
            fprintf_unfiltered (fp, " %s", s.name);
          else
            fprintf_unfiltered (fp, " %d", s.number);
        }
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
create_syscall_event_catchpoint (int tempflag, VEC(int) *filter,
                                 const struct breakpoint_ops *ops)
{
  struct syscall_catchpoint *c;
  struct gdbarch *gdbarch = get_current_arch ();

  c = new syscall_catchpoint ();
  init_catchpoint (&c->base, gdbarch, tempflag, NULL, ops);
  c->syscalls_to_be_caught = filter;

  install_breakpoint (0, &c->base, 1);
}

/* Splits the argument using space as delimiter.  Returns an xmalloc'd
   filter list, or NULL if no filtering is required.  */
static VEC(int) *
catch_syscall_split_args (char *arg)
{
  VEC(int) *result = NULL;
  struct cleanup *cleanup = make_cleanup (VEC_cleanup (int), &result);
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
	  VEC_safe_push (int, result, s.number);
	}
      else if (startswith (cur_name, "g:")
	       || startswith (cur_name, "group:"))
	{
	  /* We have a syscall group.  Let's expand it into a syscall
	     list before inserting.  */
	  struct syscall *syscall_list;
	  const char *group_name;

	  /* Skip over "g:" and "group:" prefix strings.  */
	  group_name = strchr (cur_name, ':') + 1;

	  syscall_list = get_syscalls_by_group (gdbarch, group_name);

	  if (syscall_list == NULL)
	    error (_("Unknown syscall group '%s'."), group_name);

	  for (i = 0; syscall_list[i].name != NULL; i++)
	    {
	      /* Insert each syscall that are part of the group.  No
		 need to check if it is valid.  */
	      VEC_safe_push (int, result, syscall_list[i].number);
	    }

	  xfree (syscall_list);
	}
      else
	{
	  /* We have a name.  Let's check if it's valid and convert it
	     to a number.  */
	  get_syscall_by_name (gdbarch, cur_name, &s);

	  if (s.number == UNKNOWN_SYSCALL)
	    /* Here we have to issue an error instead of a warning,
	       because GDB cannot do anything useful if there's no
	       syscall number to be caught.  */
	    error (_("Unknown syscall name '%s'."), cur_name);

	  /* Ok, it's valid.  */
	  VEC_safe_push (int, result, s.number);
	}
    }

  discard_cleanups (cleanup);
  return result;
}

/* Implement the "catch syscall" command.  */

static void
catch_syscall_command_1 (char *arg, int from_tty, 
			 struct cmd_list_element *command)
{
  int tempflag;
  VEC(int) *filter;
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
  else
    filter = NULL;

  create_syscall_event_catchpoint (tempflag, filter,
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

      if (c->syscalls_to_be_caught)
	{
	  int i, iter;
	  for (i = 0;
	       VEC_iterate (int, c->syscalls_to_be_caught, i, iter);
	       i++)
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
static VEC (char_ptr) *
catch_syscall_completer (struct cmd_list_element *cmd,
                         const char *text, const char *word)
{
  struct gdbarch *gdbarch = get_current_arch ();
  struct cleanup *cleanups = make_cleanup (null_cleanup, NULL);
  VEC (char_ptr) *group_retlist = NULL;
  VEC (char_ptr) *syscall_retlist = NULL;
  VEC (char_ptr) *retlist = NULL;
  const char **group_list = NULL;
  const char **syscall_list = NULL;
  const char *prefix;
  int i;

  /* Completion considers ':' to be a word separator, so we use this to
     verify whether the previous word was a group prefix.  If so, we
     build the completion list using group names only.  */
  for (prefix = word; prefix != text && prefix[-1] != ' '; prefix--)
    ;

  if (startswith (prefix, "g:") || startswith (prefix, "group:"))
    {
      /* Perform completion inside 'group:' namespace only.  */
      group_list = get_syscall_group_names (gdbarch);
      retlist = (group_list == NULL
		 ? NULL : complete_on_enum (group_list, word, word));
    }
  else
    {
      /* Complete with both, syscall names and groups.  */
      syscall_list = get_syscall_names (gdbarch);
      group_list = get_syscall_group_names (gdbarch);

      /* Append "group:" prefix to syscall groups.  */
      for (i = 0; group_list[i] != NULL; i++)
	{
	  char *prefixed_group = xstrprintf ("group:%s", group_list[i]);

	  group_list[i] = prefixed_group;
	  make_cleanup (xfree, prefixed_group);
	}

      syscall_retlist = ((syscall_list == NULL)
			 ? NULL : complete_on_enum (syscall_list, word, word));
      group_retlist = ((group_list == NULL)
		       ? NULL : complete_on_enum (group_list, word, word));

      retlist = VEC_merge (char_ptr, syscall_retlist, group_retlist);
    }

  VEC_free (char_ptr, syscall_retlist);
  VEC_free (char_ptr, group_retlist);
  xfree (syscall_list);
  xfree (group_list);
  do_cleanups (cleanups);

  return retlist;
}

static void
clear_syscall_counts (struct inferior *inf)
{
  struct catch_syscall_inferior_data *inf_data
    = get_catch_syscall_inferior_data (inf);

  inf_data->total_syscalls_count = 0;
  inf_data->any_syscall_count = 0;
  VEC_free (int, inf_data->syscalls_counts);
}

static void
initialize_syscall_catchpoint_ops (void)
{
  struct breakpoint_ops *ops;

  initialize_breakpoint_ops ();

  /* Syscall catchpoints.  */
  ops = &catch_syscall_breakpoint_ops;
  *ops = base_breakpoint_ops;
  ops->dtor = dtor_catch_syscall;
  ops->insert_location = insert_catch_syscall;
  ops->remove_location = remove_catch_syscall;
  ops->breakpoint_hit = breakpoint_hit_catch_syscall;
  ops->print_it = print_it_catch_syscall;
  ops->print_one = print_one_catch_syscall;
  ops->print_mention = print_mention_catch_syscall;
  ops->print_recreate = print_recreate_catch_syscall;
}

initialize_file_ftype _initialize_break_catch_syscall;

void
_initialize_break_catch_syscall (void)
{
  initialize_syscall_catchpoint_ops ();

  observer_attach_inferior_exit (clear_syscall_counts);
  catch_syscall_inferior_data
    = register_inferior_data_with_cleanup (NULL,
					   catch_syscall_inferior_data_cleanup);

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
