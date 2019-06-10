/* Copyright (C) 1992-2017 Free Software Foundation, Inc.

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
#include "observer.h"
#include "gdbcmd.h"
#include "target.h"
#include "ada-lang.h"
#include "gdbcore.h"
#include "inferior.h"
#include "gdbthread.h"
#include "progspace.h"
#include "objfiles.h"

/* The name of the array in the GNAT runtime where the Ada Task Control
   Block of each task is stored.  */
#define KNOWN_TASKS_NAME "system__tasking__debug__known_tasks"

/* The maximum number of tasks known to the Ada runtime.  */
static const int MAX_NUMBER_OF_KNOWN_TASKS = 1000;

/* The name of the variable in the GNAT runtime where the head of a task
   chain is saved.  This is an alternate mechanism to find the list of known
   tasks.  */
#define KNOWN_TASKS_LIST "system__tasking__debug__first_task"

enum task_states
{
  Unactivated,
  Runnable,
  Terminated,
  Activator_Sleep,
  Acceptor_Sleep,
  Entry_Caller_Sleep,
  Async_Select_Sleep,
  Delay_Sleep,
  Master_Completion_Sleep,
  Master_Phase_2_Sleep,
  Interrupt_Server_Idle_Sleep,
  Interrupt_Server_Blocked_Interrupt_Sleep,
  Timer_Server_Sleep,
  AST_Server_Sleep,
  Asynchronous_Hold,
  Interrupt_Server_Blocked_On_Event_Flag,
  Activating,
  Acceptor_Delay_Sleep
};

/* A short description corresponding to each possible task state.  */
static const char *task_states[] = {
  N_("Unactivated"),
  N_("Runnable"),
  N_("Terminated"),
  N_("Child Activation Wait"),
  N_("Accept or Select Term"),
  N_("Waiting on entry call"),
  N_("Async Select Wait"),
  N_("Delay Sleep"),
  N_("Child Termination Wait"),
  N_("Wait Child in Term Alt"),
  "",
  "",
  "",
  "",
  N_("Asynchronous Hold"),
  "",
  N_("Activating"),
  N_("Selective Wait")
};

/* A longer description corresponding to each possible task state.  */
static const char *long_task_states[] = {
  N_("Unactivated"),
  N_("Runnable"),
  N_("Terminated"),
  N_("Waiting for child activation"),
  N_("Blocked in accept or select with terminate"),
  N_("Waiting on entry call"),
  N_("Asynchronous Selective Wait"),
  N_("Delay Sleep"),
  N_("Waiting for children termination"),
  N_("Waiting for children in terminate alternative"),
  "",
  "",
  "",
  "",
  N_("Asynchronous Hold"),
  "",
  N_("Activating"),
  N_("Blocked in selective wait statement")
};

/* The index of certain important fields in the Ada Task Control Block
   record and sub-records.  */

struct atcb_fieldnos
{
  /* Fields in record Ada_Task_Control_Block.  */
  int common;
  int entry_calls;
  int atc_nesting_level;

  /* Fields in record Common_ATCB.  */
  int state;
  int parent;
  int priority;
  int image;
  int image_len;     /* This field may be missing.  */
  int activation_link;
  int call;
  int ll;

  /* Fields in Task_Primitives.Private_Data.  */
  int ll_thread;
  int ll_lwp;        /* This field may be missing.  */

  /* Fields in Common_ATCB.Call.all.  */
  int call_self;
};

/* This module's per-program-space data.  */

struct ada_tasks_pspace_data
{
  /* Nonzero if the data has been initialized.  If set to zero,
     it means that the data has either not been initialized, or
     has potentially become stale.  */
  int initialized_p;

  /* The ATCB record type.  */
  struct type *atcb_type;

  /* The ATCB "Common" component type.  */
  struct type *atcb_common_type;

  /* The type of the "ll" field, from the atcb_common_type.  */
  struct type *atcb_ll_type;

  /* The type of the "call" field, from the atcb_common_type.  */
  struct type *atcb_call_type;

  /* The index of various fields in the ATCB record and sub-records.  */
  struct atcb_fieldnos atcb_fieldno;
};

/* Key to our per-program-space data.  */
static const struct program_space_data *ada_tasks_pspace_data_handle;

typedef struct ada_task_info ada_task_info_s;
DEF_VEC_O(ada_task_info_s);

/* The kind of data structure used by the runtime to store the list
   of Ada tasks.  */

enum ada_known_tasks_kind
{
  /* Use this value when we haven't determined which kind of structure
     is being used, or when we need to recompute it.

     We set the value of this enumerate to zero on purpose: This allows
     us to use this enumerate in a structure where setting all fields
     to zero will result in this kind being set to unknown.  */
  ADA_TASKS_UNKNOWN = 0,

  /* This value means that we did not find any task list.  Unless
     there is a bug somewhere, this means that the inferior does not
     use tasking.  */
  ADA_TASKS_NOT_FOUND,

  /* This value means that the task list is stored as an array.
     This is the usual method, as it causes very little overhead.
     But this method is not always used, as it does use a certain
     amount of memory, which might be scarse in certain environments.  */
  ADA_TASKS_ARRAY,

  /* This value means that the task list is stored as a linked list.
     This has more runtime overhead than the array approach, but
     also require less memory when the number of tasks is small.  */
  ADA_TASKS_LIST,
};

/* This module's per-inferior data.  */

struct ada_tasks_inferior_data
{
  /* The type of data structure used by the runtime to store
     the list of Ada tasks.  The value of this field influences
     the interpretation of the known_tasks_addr field below:
       - ADA_TASKS_UNKNOWN: The value of known_tasks_addr hasn't
         been determined yet;
       - ADA_TASKS_NOT_FOUND: The program probably does not use tasking
         and the known_tasks_addr is irrelevant;
       - ADA_TASKS_ARRAY: The known_tasks is an array;
       - ADA_TASKS_LIST: The known_tasks is a list.  */
  enum ada_known_tasks_kind known_tasks_kind;

  /* The address of the known_tasks structure.  This is where
     the runtime stores the information for all Ada tasks.
     The interpretation of this field depends on KNOWN_TASKS_KIND
     above.  */
  CORE_ADDR known_tasks_addr;

  /* Type of elements of the known task.  Usually a pointer.  */
  struct type *known_tasks_element;

  /* Number of elements in the known tasks array.  */
  unsigned int known_tasks_length;

  /* When nonzero, this flag indicates that the task_list field
     below is up to date.  When set to zero, the list has either
     not been initialized, or has potentially become stale.  */
  int task_list_valid_p;

  /* The list of Ada tasks.

     Note: To each task we associate a number that the user can use to
     reference it - this number is printed beside each task in the tasks
     info listing displayed by "info tasks".  This number is equal to
     its index in the vector + 1.  Reciprocally, to compute the index
     of a task in the vector, we need to substract 1 from its number.  */
  VEC(ada_task_info_s) *task_list;
};

/* Key to our per-inferior data.  */
static const struct inferior_data *ada_tasks_inferior_data_handle;

/* Return the ada-tasks module's data for the given program space (PSPACE).
   If none is found, add a zero'ed one now.

   This function always returns a valid object.  */

static struct ada_tasks_pspace_data *
get_ada_tasks_pspace_data (struct program_space *pspace)
{
  struct ada_tasks_pspace_data *data;

  data = ((struct ada_tasks_pspace_data *)
	  program_space_data (pspace, ada_tasks_pspace_data_handle));
  if (data == NULL)
    {
      data = XCNEW (struct ada_tasks_pspace_data);
      set_program_space_data (pspace, ada_tasks_pspace_data_handle, data);
    }

  return data;
}

/* Return the ada-tasks module's data for the given inferior (INF).
   If none is found, add a zero'ed one now.

   This function always returns a valid object.

   Note that we could use an observer of the inferior-created event
   to make sure that the ada-tasks per-inferior data always exists.
   But we prefered this approach, as it avoids this entirely as long
   as the user does not use any of the tasking features.  This is
   quite possible, particularly in the case where the inferior does
   not use tasking.  */

static struct ada_tasks_inferior_data *
get_ada_tasks_inferior_data (struct inferior *inf)
{
  struct ada_tasks_inferior_data *data;

  data = ((struct ada_tasks_inferior_data *)
	  inferior_data (inf, ada_tasks_inferior_data_handle));
  if (data == NULL)
    {
      data = XCNEW (struct ada_tasks_inferior_data);
      set_inferior_data (inf, ada_tasks_inferior_data_handle, data);
    }

  return data;
}

/* Return the task number of the task whose ptid is PTID, or zero
   if the task could not be found.  */

int
ada_get_task_number (ptid_t ptid)
{
  int i;
  struct inferior *inf = find_inferior_ptid (ptid);
  struct ada_tasks_inferior_data *data;

  gdb_assert (inf != NULL);
  data = get_ada_tasks_inferior_data (inf);

  for (i = 0; i < VEC_length (ada_task_info_s, data->task_list); i++)
    if (ptid_equal (VEC_index (ada_task_info_s, data->task_list, i)->ptid,
		    ptid))
      return i + 1;

  return 0;  /* No matching task found.  */
}

/* Return the task number of the task running in inferior INF which
   matches TASK_ID , or zero if the task could not be found.  */
 
static int
get_task_number_from_id (CORE_ADDR task_id, struct inferior *inf)
{
  struct ada_tasks_inferior_data *data = get_ada_tasks_inferior_data (inf);
  int i;

  for (i = 0; i < VEC_length (ada_task_info_s, data->task_list); i++)
    {
      struct ada_task_info *task_info =
        VEC_index (ada_task_info_s, data->task_list, i);

      if (task_info->task_id == task_id)
        return i + 1;
    }

  /* Task not found.  Return 0.  */
  return 0;
}

/* Return non-zero if TASK_NUM is a valid task number.  */

int
valid_task_id (int task_num)
{
  struct ada_tasks_inferior_data *data;

  ada_build_task_list ();
  data = get_ada_tasks_inferior_data (current_inferior ());
  return (task_num > 0
          && task_num <= VEC_length (ada_task_info_s, data->task_list));
}

/* Return non-zero iff the task STATE corresponds to a non-terminated
   task state.  */

static int
ada_task_is_alive (struct ada_task_info *task_info)
{
  return (task_info->state != Terminated);
}

/* Call the ITERATOR function once for each Ada task that hasn't been
   terminated yet.  */

void
iterate_over_live_ada_tasks (ada_task_list_iterator_ftype *iterator)
{
  int i, nb_tasks;
  struct ada_task_info *task;
  struct ada_tasks_inferior_data *data;

  ada_build_task_list ();
  data = get_ada_tasks_inferior_data (current_inferior ());
  nb_tasks = VEC_length (ada_task_info_s, data->task_list);

  for (i = 0; i < nb_tasks; i++)
    {
      task = VEC_index (ada_task_info_s, data->task_list, i);
      if (!ada_task_is_alive (task))
        continue;
      iterator (task);
    }
}

/* Extract the contents of the value as a string whose length is LENGTH,
   and store the result in DEST.  */

static void
value_as_string (char *dest, struct value *val, int length)
{
  memcpy (dest, value_contents (val), length);
  dest[length] = '\0';
}

/* Extract the string image from the fat string corresponding to VAL,
   and store it in DEST.  If the string length is greater than MAX_LEN,
   then truncate the result to the first MAX_LEN characters of the fat
   string.  */

static void
read_fat_string_value (char *dest, struct value *val, int max_len)
{
  struct value *array_val;
  struct value *bounds_val;
  int len;

  /* The following variables are made static to avoid recomputing them
     each time this function is called.  */
  static int initialize_fieldnos = 1;
  static int array_fieldno;
  static int bounds_fieldno;
  static int upper_bound_fieldno;

  /* Get the index of the fields that we will need to read in order
     to extract the string from the fat string.  */
  if (initialize_fieldnos)
    {
      struct type *type = value_type (val);
      struct type *bounds_type;

      array_fieldno = ada_get_field_index (type, "P_ARRAY", 0);
      bounds_fieldno = ada_get_field_index (type, "P_BOUNDS", 0);

      bounds_type = TYPE_FIELD_TYPE (type, bounds_fieldno);
      if (TYPE_CODE (bounds_type) == TYPE_CODE_PTR)
        bounds_type = TYPE_TARGET_TYPE (bounds_type);
      if (TYPE_CODE (bounds_type) != TYPE_CODE_STRUCT)
        error (_("Unknown task name format. Aborting"));
      upper_bound_fieldno = ada_get_field_index (bounds_type, "UB0", 0);

      initialize_fieldnos = 0;
    }

  /* Get the size of the task image by checking the value of the bounds.
     The lower bound is always 1, so we only need to read the upper bound.  */
  bounds_val = value_ind (value_field (val, bounds_fieldno));
  len = value_as_long (value_field (bounds_val, upper_bound_fieldno));

  /* Make sure that we do not read more than max_len characters...  */
  if (len > max_len)
    len = max_len;

  /* Extract LEN characters from the fat string.  */
  array_val = value_ind (value_field (val, array_fieldno));
  read_memory (value_address (array_val), (gdb_byte *) dest, len);

  /* Add the NUL character to close the string.  */
  dest[len] = '\0';
}

/* Get from the debugging information the type description of all types
   related to the Ada Task Control Block that will be needed in order to
   read the list of known tasks in the Ada runtime.  Also return the
   associated ATCB_FIELDNOS.

   Error handling:  Any data missing from the debugging info will cause
   an error to be raised, and none of the return values to be set.
   Users of this function can depend on the fact that all or none of the
   return values will be set.  */

static void
get_tcb_types_info (void)
{
  struct type *type;
  struct type *common_type;
  struct type *ll_type;
  struct type *call_type;
  struct atcb_fieldnos fieldnos;
  struct ada_tasks_pspace_data *pspace_data;

  const char *atcb_name = "system__tasking__ada_task_control_block___XVE";
  const char *atcb_name_fixed = "system__tasking__ada_task_control_block";
  const char *common_atcb_name = "system__tasking__common_atcb";
  const char *private_data_name = "system__task_primitives__private_data";
  const char *entry_call_record_name = "system__tasking__entry_call_record";

  /* ATCB symbols may be found in several compilation units.  As we
     are only interested in one instance, use standard (literal,
     C-like) lookups to get the first match.  */

  struct symbol *atcb_sym =
    lookup_symbol_in_language (atcb_name, NULL, STRUCT_DOMAIN,
			       language_c, NULL).symbol;
  const struct symbol *common_atcb_sym =
    lookup_symbol_in_language (common_atcb_name, NULL, STRUCT_DOMAIN,
			       language_c, NULL).symbol;
  const struct symbol *private_data_sym =
    lookup_symbol_in_language (private_data_name, NULL, STRUCT_DOMAIN,
			       language_c, NULL).symbol;
  const struct symbol *entry_call_record_sym =
    lookup_symbol_in_language (entry_call_record_name, NULL, STRUCT_DOMAIN,
			       language_c, NULL).symbol;

  if (atcb_sym == NULL || atcb_sym->type == NULL)
    {
      /* In Ravenscar run-time libs, the  ATCB does not have a dynamic
         size, so the symbol name differs.  */
      atcb_sym = lookup_symbol_in_language (atcb_name_fixed, NULL,
					    STRUCT_DOMAIN, language_c,
					    NULL).symbol;

      if (atcb_sym == NULL || atcb_sym->type == NULL)
        error (_("Cannot find Ada_Task_Control_Block type. Aborting"));

      type = atcb_sym->type;
    }
  else
    {
      /* Get a static representation of the type record
         Ada_Task_Control_Block.  */
      type = atcb_sym->type;
      type = ada_template_to_fixed_record_type_1 (type, NULL, 0, NULL, 0);
    }

  if (common_atcb_sym == NULL || common_atcb_sym->type == NULL)
    error (_("Cannot find Common_ATCB type. Aborting"));
  if (private_data_sym == NULL || private_data_sym->type == NULL)
    error (_("Cannot find Private_Data type. Aborting"));
  if (entry_call_record_sym == NULL || entry_call_record_sym->type == NULL)
    error (_("Cannot find Entry_Call_Record type. Aborting"));

  /* Get the type for Ada_Task_Control_Block.Common.  */
  common_type = common_atcb_sym->type;

  /* Get the type for Ada_Task_Control_Bloc.Common.Call.LL.  */
  ll_type = private_data_sym->type;

  /* Get the type for Common_ATCB.Call.all.  */
  call_type = entry_call_record_sym->type;

  /* Get the field indices.  */
  fieldnos.common = ada_get_field_index (type, "common", 0);
  fieldnos.entry_calls = ada_get_field_index (type, "entry_calls", 1);
  fieldnos.atc_nesting_level =
    ada_get_field_index (type, "atc_nesting_level", 1);
  fieldnos.state = ada_get_field_index (common_type, "state", 0);
  fieldnos.parent = ada_get_field_index (common_type, "parent", 1);
  fieldnos.priority = ada_get_field_index (common_type, "base_priority", 0);
  fieldnos.image = ada_get_field_index (common_type, "task_image", 1);
  fieldnos.image_len = ada_get_field_index (common_type, "task_image_len", 1);
  fieldnos.activation_link = ada_get_field_index (common_type,
                                                  "activation_link", 1);
  fieldnos.call = ada_get_field_index (common_type, "call", 1);
  fieldnos.ll = ada_get_field_index (common_type, "ll", 0);
  fieldnos.ll_thread = ada_get_field_index (ll_type, "thread", 0);
  fieldnos.ll_lwp = ada_get_field_index (ll_type, "lwp", 1);
  fieldnos.call_self = ada_get_field_index (call_type, "self", 0);

  /* On certain platforms such as x86-windows, the "lwp" field has been
     named "thread_id".  This field will likely be renamed in the future,
     but we need to support both possibilities to avoid an unnecessary
     dependency on a recent compiler.  We therefore try locating the
     "thread_id" field in place of the "lwp" field if we did not find
     the latter.  */
  if (fieldnos.ll_lwp < 0)
    fieldnos.ll_lwp = ada_get_field_index (ll_type, "thread_id", 1);

  /* Set all the out parameters all at once, now that we are certain
     that there are no potential error() anymore.  */
  pspace_data = get_ada_tasks_pspace_data (current_program_space);
  pspace_data->initialized_p = 1;
  pspace_data->atcb_type = type;
  pspace_data->atcb_common_type = common_type;
  pspace_data->atcb_ll_type = ll_type;
  pspace_data->atcb_call_type = call_type;
  pspace_data->atcb_fieldno = fieldnos;
}

/* Build the PTID of the task from its COMMON_VALUE, which is the "Common"
   component of its ATCB record.  This PTID needs to match the PTID used
   by the thread layer.  */

static ptid_t
ptid_from_atcb_common (struct value *common_value)
{
  long thread = 0;
  CORE_ADDR lwp = 0;
  struct value *ll_value;
  ptid_t ptid;
  const struct ada_tasks_pspace_data *pspace_data
    = get_ada_tasks_pspace_data (current_program_space);

  ll_value = value_field (common_value, pspace_data->atcb_fieldno.ll);

  if (pspace_data->atcb_fieldno.ll_lwp >= 0)
    lwp = value_as_address (value_field (ll_value,
					 pspace_data->atcb_fieldno.ll_lwp));
  thread = value_as_long (value_field (ll_value,
				       pspace_data->atcb_fieldno.ll_thread));

  ptid = target_get_ada_task_ptid (lwp, thread);

  return ptid;
}

/* Read the ATCB data of a given task given its TASK_ID (which is in practice
   the address of its assocated ATCB record), and store the result inside
   TASK_INFO.  */

static void
read_atcb (CORE_ADDR task_id, struct ada_task_info *task_info)
{
  struct value *tcb_value;
  struct value *common_value;
  struct value *atc_nesting_level_value;
  struct value *entry_calls_value;
  struct value *entry_calls_value_element;
  int called_task_fieldno = -1;
  static const char ravenscar_task_name[] = "Ravenscar task";
  const struct ada_tasks_pspace_data *pspace_data
    = get_ada_tasks_pspace_data (current_program_space);

  if (!pspace_data->initialized_p)
    get_tcb_types_info ();

  tcb_value = value_from_contents_and_address (pspace_data->atcb_type,
					       NULL, task_id);
  common_value = value_field (tcb_value, pspace_data->atcb_fieldno.common);

  /* Fill in the task_id.  */

  task_info->task_id = task_id;

  /* Compute the name of the task.

     Depending on the GNAT version used, the task image is either a fat
     string, or a thin array of characters.  Older versions of GNAT used
     to use fat strings, and therefore did not need an extra field in
     the ATCB to store the string length.  For efficiency reasons, newer
     versions of GNAT replaced the fat string by a static buffer, but this
     also required the addition of a new field named "Image_Len" containing
     the length of the task name.  The method used to extract the task name
     is selected depending on the existence of this field.

     In some run-time libs (e.g. Ravenscar), the name is not in the ATCB;
     we may want to get it from the first user frame of the stack.  For now,
     we just give a dummy name.  */

  if (pspace_data->atcb_fieldno.image_len == -1)
    {
      if (pspace_data->atcb_fieldno.image >= 0)
        read_fat_string_value (task_info->name,
                               value_field (common_value,
					    pspace_data->atcb_fieldno.image),
                               sizeof (task_info->name) - 1);
      else
	{
	  struct bound_minimal_symbol msym;

	  msym = lookup_minimal_symbol_by_pc (task_id);
	  if (msym.minsym)
	    {
	      const char *full_name = MSYMBOL_LINKAGE_NAME (msym.minsym);
	      const char *task_name = full_name;
	      const char *p;

	      /* Strip the prefix.  */
	      for (p = full_name; *p; p++)
		if (p[0] == '_' && p[1] == '_')
		  task_name = p + 2;

	      /* Copy the task name.  */
	      strncpy (task_info->name, task_name, sizeof (task_info->name));
	      task_info->name[sizeof (task_info->name) - 1] = 0;
	    }
	  else
	    {
	      /* No symbol found.  Use a default name.  */
	      strcpy (task_info->name, ravenscar_task_name);
	    }
	}
    }
  else
    {
      int len = value_as_long
		  (value_field (common_value,
				pspace_data->atcb_fieldno.image_len));

      value_as_string (task_info->name,
                       value_field (common_value,
				    pspace_data->atcb_fieldno.image),
		       len);
    }

  /* Compute the task state and priority.  */

  task_info->state =
    value_as_long (value_field (common_value,
				pspace_data->atcb_fieldno.state));
  task_info->priority =
    value_as_long (value_field (common_value,
				pspace_data->atcb_fieldno.priority));

  /* If the ATCB contains some information about the parent task,
     then compute it as well.  Otherwise, zero.  */

  if (pspace_data->atcb_fieldno.parent >= 0)
    task_info->parent =
      value_as_address (value_field (common_value,
				     pspace_data->atcb_fieldno.parent));
  else
    task_info->parent = 0;
  

  /* If the ATCB contains some information about entry calls, then
     compute the "called_task" as well.  Otherwise, zero.  */

  if (pspace_data->atcb_fieldno.atc_nesting_level > 0
      && pspace_data->atcb_fieldno.entry_calls > 0)
    {
      /* Let My_ATCB be the Ada task control block of a task calling the
         entry of another task; then the Task_Id of the called task is
         in My_ATCB.Entry_Calls (My_ATCB.ATC_Nesting_Level).Called_Task.  */
      atc_nesting_level_value =
        value_field (tcb_value, pspace_data->atcb_fieldno.atc_nesting_level);
      entry_calls_value =
        ada_coerce_to_simple_array_ptr
	  (value_field (tcb_value, pspace_data->atcb_fieldno.entry_calls));
      entry_calls_value_element =
        value_subscript (entry_calls_value,
			 value_as_long (atc_nesting_level_value));
      called_task_fieldno =
        ada_get_field_index (value_type (entry_calls_value_element),
                             "called_task", 0);
      task_info->called_task =
        value_as_address (value_field (entry_calls_value_element,
                                       called_task_fieldno));
    }
  else
    {
      task_info->called_task = 0;
    }

  /* If the ATCB cotnains some information about RV callers,
     then compute the "caller_task".  Otherwise, zero.  */

  task_info->caller_task = 0;
  if (pspace_data->atcb_fieldno.call >= 0)
    {
      /* Get the ID of the caller task from Common_ATCB.Call.all.Self.
         If Common_ATCB.Call is null, then there is no caller.  */
      const CORE_ADDR call =
        value_as_address (value_field (common_value,
				       pspace_data->atcb_fieldno.call));
      struct value *call_val;

      if (call != 0)
        {
          call_val =
            value_from_contents_and_address (pspace_data->atcb_call_type,
					     NULL, call);
          task_info->caller_task =
            value_as_address
	      (value_field (call_val, pspace_data->atcb_fieldno.call_self));
        }
    }

  /* And finally, compute the task ptid.  Note that there are situations
     where this cannot be determined:
       - The task is no longer alive - the ptid is irrelevant;
       - We are debugging a core file - the thread is not always
         completely preserved for us to link back a task to its
         underlying thread.  Since we do not support task switching
         when debugging core files anyway, we don't need to compute
         that task ptid.
     In either case, we don't need that ptid, and it is just good enough
     to set it to null_ptid.  */

  if (target_has_execution && ada_task_is_alive (task_info))
    task_info->ptid = ptid_from_atcb_common (common_value);
  else
    task_info->ptid = null_ptid;
}

/* Read the ATCB info of the given task (identified by TASK_ID), and
   add the result to the given inferior's TASK_LIST.  */

static void
add_ada_task (CORE_ADDR task_id, struct inferior *inf)
{
  struct ada_task_info task_info;
  struct ada_tasks_inferior_data *data = get_ada_tasks_inferior_data (inf);

  read_atcb (task_id, &task_info);
  VEC_safe_push (ada_task_info_s, data->task_list, &task_info);
}

/* Read the Known_Tasks array from the inferior memory, and store
   it in the current inferior's TASK_LIST.  Return non-zero upon success.  */

static int
read_known_tasks_array (struct ada_tasks_inferior_data *data)
{
  const int target_ptr_byte = TYPE_LENGTH (data->known_tasks_element);
  const int known_tasks_size = target_ptr_byte * data->known_tasks_length;
  gdb_byte *known_tasks = (gdb_byte *) alloca (known_tasks_size);
  int i;

  /* Build a new list by reading the ATCBs from the Known_Tasks array
     in the Ada runtime.  */
  read_memory (data->known_tasks_addr, known_tasks, known_tasks_size);
  for (i = 0; i < data->known_tasks_length; i++)
    {
      CORE_ADDR task_id =
        extract_typed_address (known_tasks + i * target_ptr_byte,
			       data->known_tasks_element);

      if (task_id != 0)
        add_ada_task (task_id, current_inferior ());
    }

  return 1;
}

/* Read the known tasks from the inferior memory, and store it in
   the current inferior's TASK_LIST.  Return non-zero upon success.  */

static int
read_known_tasks_list (struct ada_tasks_inferior_data *data)
{
  const int target_ptr_byte = TYPE_LENGTH (data->known_tasks_element);
  gdb_byte *known_tasks = (gdb_byte *) alloca (target_ptr_byte);
  CORE_ADDR task_id;
  const struct ada_tasks_pspace_data *pspace_data
    = get_ada_tasks_pspace_data (current_program_space);

  /* Sanity check.  */
  if (pspace_data->atcb_fieldno.activation_link < 0)
    return 0;

  /* Build a new list by reading the ATCBs.  Read head of the list.  */
  read_memory (data->known_tasks_addr, known_tasks, target_ptr_byte);
  task_id = extract_typed_address (known_tasks, data->known_tasks_element);
  while (task_id != 0)
    {
      struct value *tcb_value;
      struct value *common_value;

      add_ada_task (task_id, current_inferior ());

      /* Read the chain.  */
      tcb_value = value_from_contents_and_address (pspace_data->atcb_type,
						   NULL, task_id);
      common_value = value_field (tcb_value, pspace_data->atcb_fieldno.common);
      task_id = value_as_address
		  (value_field (common_value,
                                pspace_data->atcb_fieldno.activation_link));
    }

  return 1;
}

/* Set all fields of the current inferior ada-tasks data pointed by DATA.
   Do nothing if those fields are already set and still up to date.  */

static void
ada_tasks_inferior_data_sniffer (struct ada_tasks_inferior_data *data)
{
  struct bound_minimal_symbol msym;
  struct symbol *sym;

  /* Return now if already set.  */
  if (data->known_tasks_kind != ADA_TASKS_UNKNOWN)
    return;

  /* Try array.  */

  msym = lookup_minimal_symbol (KNOWN_TASKS_NAME, NULL, NULL);
  if (msym.minsym != NULL)
    {
      data->known_tasks_kind = ADA_TASKS_ARRAY;
      data->known_tasks_addr = BMSYMBOL_VALUE_ADDRESS (msym);

      /* Try to get pointer type and array length from the symtab.  */
      sym = lookup_symbol_in_language (KNOWN_TASKS_NAME, NULL, VAR_DOMAIN,
				       language_c, NULL).symbol;
      if (sym != NULL)
	{
	  /* Validate.  */
	  struct type *type = check_typedef (SYMBOL_TYPE (sym));
	  struct type *eltype = NULL;
	  struct type *idxtype = NULL;

	  if (TYPE_CODE (type) == TYPE_CODE_ARRAY)
	    eltype = check_typedef (TYPE_TARGET_TYPE (type));
	  if (eltype != NULL
	      && TYPE_CODE (eltype) == TYPE_CODE_PTR)
	    idxtype = check_typedef (TYPE_INDEX_TYPE (type));
	  if (idxtype != NULL
	      && !TYPE_LOW_BOUND_UNDEFINED (idxtype)
	      && !TYPE_HIGH_BOUND_UNDEFINED (idxtype))
	    {
	      data->known_tasks_element = eltype;
	      data->known_tasks_length =
		TYPE_HIGH_BOUND (idxtype) - TYPE_LOW_BOUND (idxtype) + 1;
	      return;
	    }
	}

      /* Fallback to default values.  The runtime may have been stripped (as
	 in some distributions), but it is likely that the executable still
	 contains debug information on the task type (due to implicit with of
	 Ada.Tasking).  */
      data->known_tasks_element =
	builtin_type (target_gdbarch ())->builtin_data_ptr;
      data->known_tasks_length = MAX_NUMBER_OF_KNOWN_TASKS;
      return;
    }


  /* Try list.  */

  msym = lookup_minimal_symbol (KNOWN_TASKS_LIST, NULL, NULL);
  if (msym.minsym != NULL)
    {
      data->known_tasks_kind = ADA_TASKS_LIST;
      data->known_tasks_addr = BMSYMBOL_VALUE_ADDRESS (msym);
      data->known_tasks_length = 1;

      sym = lookup_symbol_in_language (KNOWN_TASKS_LIST, NULL, VAR_DOMAIN,
				       language_c, NULL).symbol;
      if (sym != NULL && SYMBOL_VALUE_ADDRESS (sym) != 0)
	{
	  /* Validate.  */
	  struct type *type = check_typedef (SYMBOL_TYPE (sym));

	  if (TYPE_CODE (type) == TYPE_CODE_PTR)
	    {
	      data->known_tasks_element = type;
	      return;
	    }
	}

      /* Fallback to default values.  */
      data->known_tasks_element =
	builtin_type (target_gdbarch ())->builtin_data_ptr;
      data->known_tasks_length = 1;
      return;
    }

  /* Can't find tasks.  */

  data->known_tasks_kind = ADA_TASKS_NOT_FOUND;
  data->known_tasks_addr = 0;
}

/* Read the known tasks from the current inferior's memory, and store it
   in the current inferior's data TASK_LIST.
   Return non-zero upon success.  */

static int
read_known_tasks (void)
{
  struct ada_tasks_inferior_data *data =
    get_ada_tasks_inferior_data (current_inferior ());

  /* Step 1: Clear the current list, if necessary.  */
  VEC_truncate (ada_task_info_s, data->task_list, 0);

  /* Step 2: do the real work.
     If the application does not use task, then no more needs to be done.
     It is important to have the task list cleared (see above) before we
     return, as we don't want a stale task list to be used...  This can
     happen for instance when debugging a non-multitasking program after
     having debugged a multitasking one.  */
  ada_tasks_inferior_data_sniffer (data);
  gdb_assert (data->known_tasks_kind != ADA_TASKS_UNKNOWN);

  switch (data->known_tasks_kind)
    {
      case ADA_TASKS_NOT_FOUND: /* Tasking not in use in inferior.  */
        return 0;
      case ADA_TASKS_ARRAY:
        return read_known_tasks_array (data);
      case ADA_TASKS_LIST:
        return read_known_tasks_list (data);
    }

  /* Step 3: Set task_list_valid_p, to avoid re-reading the Known_Tasks
     array unless needed.  Then report a success.  */
  data->task_list_valid_p = 1;

  return 1;
}

/* Build the task_list by reading the Known_Tasks array from
   the inferior, and return the number of tasks in that list
   (zero means that the program is not using tasking at all).  */

int
ada_build_task_list (void)
{
  struct ada_tasks_inferior_data *data;

  if (!target_has_stack)
    error (_("Cannot inspect Ada tasks when program is not running"));

  data = get_ada_tasks_inferior_data (current_inferior ());
  if (!data->task_list_valid_p)
    read_known_tasks ();

  return VEC_length (ada_task_info_s, data->task_list);
}

/* Print a table providing a short description of all Ada tasks
   running inside inferior INF.  If ARG_STR is set, it will be
   interpreted as a task number, and the table will be limited to
   that task only.  */

void
print_ada_task_info (struct ui_out *uiout,
		     char *arg_str,
		     struct inferior *inf)
{
  struct ada_tasks_inferior_data *data;
  int taskno, nb_tasks;
  int taskno_arg = 0;
  struct cleanup *old_chain;
  int nb_columns;

  if (ada_build_task_list () == 0)
    {
      uiout->message (_("Your application does not use any Ada tasks.\n"));
      return;
    }

  if (arg_str != NULL && arg_str[0] != '\0')
    taskno_arg = value_as_long (parse_and_eval (arg_str));

  if (uiout->is_mi_like_p ())
    /* In GDB/MI mode, we want to provide the thread ID corresponding
       to each task.  This allows clients to quickly find the thread
       associated to any task, which is helpful for commands that
       take a --thread argument.  However, in order to be able to
       provide that thread ID, the thread list must be up to date
       first.  */
    target_update_thread_list ();

  data = get_ada_tasks_inferior_data (inf);

  /* Compute the number of tasks that are going to be displayed
     in the output.  If an argument was given, there will be
     at most 1 entry.  Otherwise, there will be as many entries
     as we have tasks.  */
  if (taskno_arg)
    {
      if (taskno_arg > 0
	  && taskno_arg <= VEC_length (ada_task_info_s, data->task_list))
	nb_tasks = 1;
      else
	nb_tasks = 0;
    }
  else
    nb_tasks = VEC_length (ada_task_info_s, data->task_list);

  nb_columns = uiout->is_mi_like_p () ? 8 : 7;
  old_chain = make_cleanup_ui_out_table_begin_end (uiout, nb_columns,
						   nb_tasks, "tasks");
  uiout->table_header (1, ui_left, "current", "");
  uiout->table_header (3, ui_right, "id", "ID");
  uiout->table_header (9, ui_right, "task-id", "TID");
  /* The following column is provided in GDB/MI mode only because
     it is only really useful in that mode, and also because it
     allows us to keep the CLI output shorter and more compact.  */
  if (uiout->is_mi_like_p ())
    uiout->table_header (4, ui_right, "thread-id", "");
  uiout->table_header (4, ui_right, "parent-id", "P-ID");
  uiout->table_header (3, ui_right, "priority", "Pri");
  uiout->table_header (22, ui_left, "state", "State");
  /* Use ui_noalign for the last column, to prevent the CLI uiout
     from printing an extra space at the end of each row.  This
     is a bit of a hack, but does get the job done.  */
  uiout->table_header (1, ui_noalign, "name", "Name");
  uiout->table_body ();

  for (taskno = 1;
       taskno <= VEC_length (ada_task_info_s, data->task_list);
       taskno++)
    {
      const struct ada_task_info *const task_info =
	VEC_index (ada_task_info_s, data->task_list, taskno - 1);
      int parent_id;
      struct cleanup *chain2;

      gdb_assert (task_info != NULL);

      /* If the user asked for the output to be restricted
	 to one task only, and this is not the task, skip
	 to the next one.  */
      if (taskno_arg && taskno != taskno_arg)
        continue;

      chain2 = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);

      /* Print a star if this task is the current task (or the task
         currently selected).  */
      if (ptid_equal (task_info->ptid, inferior_ptid))
	uiout->field_string ("current", "*");
      else
	uiout->field_skip ("current");

      /* Print the task number.  */
      uiout->field_int ("id", taskno);

      /* Print the Task ID.  */
      uiout->field_fmt ("task-id", "%9lx", (long) task_info->task_id);

      /* Print the associated Thread ID.  */
      if (uiout->is_mi_like_p ())
        {
	  const int thread_id = ptid_to_global_thread_id (task_info->ptid);

	  if (thread_id != 0)
	    uiout->field_int ("thread-id", thread_id);
	  else
	    /* This should never happen unless there is a bug somewhere,
	       but be resilient when that happens.  */
	    uiout->field_skip ("thread-id");
	}

      /* Print the ID of the parent task.  */
      parent_id = get_task_number_from_id (task_info->parent, inf);
      if (parent_id)
        uiout->field_int ("parent-id", parent_id);
      else
        uiout->field_skip ("parent-id");

      /* Print the base priority of the task.  */
      uiout->field_int ("priority", task_info->priority);

      /* Print the task current state.  */
      if (task_info->caller_task)
	uiout->field_fmt ("state",
			  _("Accepting RV with %-4d"),
			  get_task_number_from_id (task_info->caller_task,
						   inf));
      else if (task_info->state == Entry_Caller_Sleep
	       && task_info->called_task)
	uiout->field_fmt ("state",
			  _("Waiting on RV with %-3d"),
			  get_task_number_from_id (task_info->called_task,
						   inf));
      else
	uiout->field_string ("state", task_states[task_info->state]);

      /* Finally, print the task name.  */
      uiout->field_fmt ("name",
			"%s",
			task_info->name[0] != '\0' ? task_info->name
						   : _("<no name>"));

      uiout->text ("\n");
      do_cleanups (chain2);
    }

  do_cleanups (old_chain);
}

/* Print a detailed description of the Ada task whose ID is TASKNO_STR
   for the given inferior (INF).  */

static void
info_task (struct ui_out *uiout, char *taskno_str, struct inferior *inf)
{
  const int taskno = value_as_long (parse_and_eval (taskno_str));
  struct ada_task_info *task_info;
  int parent_taskno = 0;
  struct ada_tasks_inferior_data *data = get_ada_tasks_inferior_data (inf);

  if (ada_build_task_list () == 0)
    {
      uiout->message (_("Your application does not use any Ada tasks.\n"));
      return;
    }

  if (taskno <= 0 || taskno > VEC_length (ada_task_info_s, data->task_list))
    error (_("Task ID %d not known.  Use the \"info tasks\" command to\n"
             "see the IDs of currently known tasks"), taskno);
  task_info = VEC_index (ada_task_info_s, data->task_list, taskno - 1);

  /* Print the Ada task ID.  */
  printf_filtered (_("Ada Task: %s\n"),
		   paddress (target_gdbarch (), task_info->task_id));

  /* Print the name of the task.  */
  if (task_info->name[0] != '\0')
    printf_filtered (_("Name: %s\n"), task_info->name);
  else
    printf_filtered (_("<no name>\n"));

  /* Print the TID and LWP.  */
  printf_filtered (_("Thread: %#lx\n"), ptid_get_tid (task_info->ptid));
  printf_filtered (_("LWP: %#lx\n"), ptid_get_lwp (task_info->ptid));

  /* Print who is the parent (if any).  */
  if (task_info->parent != 0)
    parent_taskno = get_task_number_from_id (task_info->parent, inf);
  if (parent_taskno)
    {
      struct ada_task_info *parent =
        VEC_index (ada_task_info_s, data->task_list, parent_taskno - 1);

      printf_filtered (_("Parent: %d"), parent_taskno);
      if (parent->name[0] != '\0')
        printf_filtered (" (%s)", parent->name);
      printf_filtered ("\n");
    }
  else
    printf_filtered (_("No parent\n"));

  /* Print the base priority.  */
  printf_filtered (_("Base Priority: %d\n"), task_info->priority);

  /* print the task current state.  */
  {
    int target_taskno = 0;

    if (task_info->caller_task)
      {
        target_taskno = get_task_number_from_id (task_info->caller_task, inf);
        printf_filtered (_("State: Accepting rendezvous with %d"),
                         target_taskno);
      }
    else if (task_info->state == Entry_Caller_Sleep && task_info->called_task)
      {
        target_taskno = get_task_number_from_id (task_info->called_task, inf);
        printf_filtered (_("State: Waiting on task %d's entry"),
                         target_taskno);
      }
    else
      printf_filtered (_("State: %s"), _(long_task_states[task_info->state]));

    if (target_taskno)
      {
        struct ada_task_info *target_task_info =
          VEC_index (ada_task_info_s, data->task_list, target_taskno - 1);

        if (target_task_info->name[0] != '\0')
          printf_filtered (" (%s)", target_task_info->name);
      }

    printf_filtered ("\n");
  }
}

/* If ARG is empty or null, then print a list of all Ada tasks.
   Otherwise, print detailed information about the task whose ID
   is ARG.
   
   Does nothing if the program doesn't use Ada tasking.  */

static void
info_tasks_command (char *arg, int from_tty)
{
  struct ui_out *uiout = current_uiout;

  if (arg == NULL || *arg == '\0')
    print_ada_task_info (uiout, NULL, current_inferior ());
  else
    info_task (uiout, arg, current_inferior ());
}

/* Print a message telling the user id of the current task.
   This function assumes that tasking is in use in the inferior.  */

static void
display_current_task_id (void)
{
  const int current_task = ada_get_task_number (inferior_ptid);

  if (current_task == 0)
    printf_filtered (_("[Current task is unknown]\n"));
  else
    printf_filtered (_("[Current task is %d]\n"), current_task);
}

/* Parse and evaluate TIDSTR into a task id, and try to switch to
   that task.  Print an error message if the task switch failed.  */

static void
task_command_1 (char *taskno_str, int from_tty, struct inferior *inf)
{
  const int taskno = value_as_long (parse_and_eval (taskno_str));
  struct ada_task_info *task_info;
  struct ada_tasks_inferior_data *data = get_ada_tasks_inferior_data (inf);

  if (taskno <= 0 || taskno > VEC_length (ada_task_info_s, data->task_list))
    error (_("Task ID %d not known.  Use the \"info tasks\" command to\n"
             "see the IDs of currently known tasks"), taskno);
  task_info = VEC_index (ada_task_info_s, data->task_list, taskno - 1);

  if (!ada_task_is_alive (task_info))
    error (_("Cannot switch to task %d: Task is no longer running"), taskno);
   
  /* On some platforms, the thread list is not updated until the user
     performs a thread-related operation (by using the "info threads"
     command, for instance).  So this thread list may not be up to date
     when the user attempts this task switch.  Since we cannot switch
     to the thread associated to our task if GDB does not know about
     that thread, we need to make sure that any new threads gets added
     to the thread list.  */
  target_update_thread_list ();

  /* Verify that the ptid of the task we want to switch to is valid
     (in other words, a ptid that GDB knows about).  Otherwise, we will
     cause an assertion failure later on, when we try to determine
     the ptid associated thread_info data.  We should normally never
     encounter such an error, but the wrong ptid can actually easily be
     computed if target_get_ada_task_ptid has not been implemented for
     our target (yet).  Rather than cause an assertion error in that case,
     it's nicer for the user to just refuse to perform the task switch.  */
  if (!find_thread_ptid (task_info->ptid))
    error (_("Unable to compute thread ID for task %d.\n"
             "Cannot switch to this task."),
           taskno);

  switch_to_thread (task_info->ptid);
  ada_find_printable_frame (get_selected_frame (NULL));
  printf_filtered (_("[Switching to task %d]\n"), taskno);
  print_stack_frame (get_selected_frame (NULL),
                     frame_relative_level (get_selected_frame (NULL)),
		     SRC_AND_LOC, 1);
}


/* Print the ID of the current task if TASKNO_STR is empty or NULL.
   Otherwise, switch to the task indicated by TASKNO_STR.  */

static void
task_command (char *taskno_str, int from_tty)
{
  struct ui_out *uiout = current_uiout;

  if (ada_build_task_list () == 0)
    {
      uiout->message (_("Your application does not use any Ada tasks.\n"));
      return;
    }

  if (taskno_str == NULL || taskno_str[0] == '\0')
    display_current_task_id ();
  else
    {
      /* Task switching in core files doesn't work, either because:
           1. Thread support is not implemented with core files
           2. Thread support is implemented, but the thread IDs created
              after having read the core file are not the same as the ones
              that were used during the program life, before the crash.
              As a consequence, there is no longer a way for the debugger
              to find the associated thead ID of any given Ada task.
         So, instead of attempting a task switch without giving the user
         any clue as to what might have happened, just error-out with
         a message explaining that this feature is not supported.  */
      if (!target_has_execution)
        error (_("\
Task switching not supported when debugging from core files\n\
(use thread support instead)"));
      task_command_1 (taskno_str, from_tty, current_inferior ());
    }
}

/* Indicate that the given inferior's task list may have changed,
   so invalidate the cache.  */

static void
ada_task_list_changed (struct inferior *inf)
{
  struct ada_tasks_inferior_data *data = get_ada_tasks_inferior_data (inf);

  data->task_list_valid_p = 0;
}

/* Invalidate the per-program-space data.  */

static void
ada_tasks_invalidate_pspace_data (struct program_space *pspace)
{
  get_ada_tasks_pspace_data (pspace)->initialized_p = 0;
}

/* Invalidate the per-inferior data.  */

static void
ada_tasks_invalidate_inferior_data (struct inferior *inf)
{
  struct ada_tasks_inferior_data *data = get_ada_tasks_inferior_data (inf);

  data->known_tasks_kind = ADA_TASKS_UNKNOWN;
  data->task_list_valid_p = 0;
}

/* The 'normal_stop' observer notification callback.  */

static void
ada_tasks_normal_stop_observer (struct bpstats *unused_args, int unused_args2)
{
  /* The inferior has been resumed, and just stopped. This means that
     our task_list needs to be recomputed before it can be used again.  */
  ada_task_list_changed (current_inferior ());
}

/* A routine to be called when the objfiles have changed.  */

static void
ada_tasks_new_objfile_observer (struct objfile *objfile)
{
  struct inferior *inf;

  /* Invalidate the relevant data in our program-space data.  */

  if (objfile == NULL)
    {
      /* All objfiles are being cleared, so we should clear all
	 our caches for all program spaces.  */
      struct program_space *pspace;

      for (pspace = program_spaces; pspace != NULL; pspace = pspace->next)
        ada_tasks_invalidate_pspace_data (pspace);
    }
  else
    {
      /* The associated program-space data might have changed after
	 this objfile was added.  Invalidate all cached data.  */
      ada_tasks_invalidate_pspace_data (objfile->pspace);
    }

  /* Invalidate the per-inferior cache for all inferiors using
     this objfile (or, in other words, for all inferiors who have
     the same program-space as the objfile's program space).
     If all objfiles are being cleared (OBJFILE is NULL), then
     clear the caches for all inferiors.  */

  for (inf = inferior_list; inf != NULL; inf = inf->next)
    if (objfile == NULL || inf->pspace == objfile->pspace)
      ada_tasks_invalidate_inferior_data (inf);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_tasks;

void
_initialize_tasks (void)
{
  ada_tasks_pspace_data_handle = register_program_space_data ();
  ada_tasks_inferior_data_handle = register_inferior_data ();

  /* Attach various observers.  */
  observer_attach_normal_stop (ada_tasks_normal_stop_observer);
  observer_attach_new_objfile (ada_tasks_new_objfile_observer);

  /* Some new commands provided by this module.  */
  add_info ("tasks", info_tasks_command,
            _("Provide information about all known Ada tasks"));
  add_cmd ("task", class_run, task_command,
           _("Use this command to switch between Ada tasks.\n\
Without argument, this command simply prints the current task ID"),
           &cmdlist);
}
