/* Copyright (C) 1992, 1993, 1994, 1997, 1998, 1999, 2000, 2003, 2004, 2005,
   2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.

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

/* The name of the array in the GNAT runtime where the Ada Task Control
   Block of each task is stored.  */
#define KNOWN_TASKS_NAME "system__tasking__debug__known_tasks"

/* The maximum number of tasks known to the Ada runtime.  */
static const int MAX_NUMBER_OF_KNOWN_TASKS = 1000;

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

struct tcb_fieldnos
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
  int call;
  int ll;

  /* Fields in Task_Primitives.Private_Data.  */
  int ll_thread;
  int ll_lwp;        /* This field may be missing.  */

  /* Fields in Common_ATCB.Call.all.  */
  int call_self;
};

/* The type description for the ATCB record and subrecords, and
   the associated tcb_fieldnos.  For efficiency reasons, these are made
   static globals so that we can compute them only once the first time
   and reuse them later.  Set to NULL if the types haven't been computed
   yet, or if they may be obsolete (for instance after having loaded
   a new binary).  */

static struct type *atcb_type = NULL;
static struct type *atcb_common_type = NULL;
static struct type *atcb_ll_type = NULL;
static struct type *atcb_call_type = NULL;
static struct tcb_fieldnos fieldno;

/* Set to 1 when the cached address of System.Tasking.Debug.Known_Tasks
   might be stale and so needs to be recomputed.  */
static int ada_tasks_check_symbol_table = 1;

/* The list of Ada tasks.
 
   Note: To each task we associate a number that the user can use to
   reference it - this number is printed beside each task in the tasks
   info listing displayed by "info tasks".  This number is equal to
   its index in the vector + 1.  Reciprocally, to compute the index
   of a task in the vector, we need to substract 1 from its number.  */
typedef struct ada_task_info ada_task_info_s;
DEF_VEC_O(ada_task_info_s);
static VEC(ada_task_info_s) *task_list = NULL;

/* When non-zero, this flag indicates that the current task_list
   is obsolete, and should be recomputed before it is accessed.  */
static int stale_task_list_p = 1;

/* Return the task number of the task whose ptid is PTID, or zero
   if the task could not be found.  */

int
ada_get_task_number (ptid_t ptid)
{
  int i;

  for (i=0; i < VEC_length (ada_task_info_s, task_list); i++)
    if (ptid_equal (VEC_index (ada_task_info_s, task_list, i)->ptid, ptid))
      return i + 1;

  return 0;  /* No matching task found.  */
}

/* Return the task number of the task that matches TASK_ID, or zero
   if the task could not be found.  */
 
static int
get_task_number_from_id (CORE_ADDR task_id)
{
  int i;

  for (i = 0; i < VEC_length (ada_task_info_s, task_list); i++)
    {
      struct ada_task_info *task_info =
        VEC_index (ada_task_info_s, task_list, i);

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
  ada_build_task_list (0);
  return (task_num > 0
          && task_num <= VEC_length (ada_task_info_s, task_list));
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

  ada_build_task_list (0);
  nb_tasks = VEC_length (ada_task_info_s, task_list);

  for (i = 0; i < nb_tasks; i++)
    {
      task = VEC_index (ada_task_info_s, task_list, i);
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
  read_memory (value_address (array_val), dest, len);

  /* Add the NUL character to close the string.  */
  dest[len] = '\0';
}

/* Return the address of the Known_Tasks array maintained in
   the Ada Runtime.  Return zero if the array could not be found,
   meaning that the inferior program probably does not use tasking.

   In order to provide a fast response time, this function caches
   the Known_Tasks array address after the lookup during the first
   call.  Subsequent calls will simply return this cached address.  */

static CORE_ADDR
get_known_tasks_addr (void)
{
  static CORE_ADDR known_tasks_addr = 0;

  if (ada_tasks_check_symbol_table)
    {
      struct minimal_symbol *msym;

      msym = lookup_minimal_symbol (KNOWN_TASKS_NAME, NULL, NULL);
      if (msym == NULL)
        return 0;
      known_tasks_addr = SYMBOL_VALUE_ADDRESS (msym);

      /* FIXME: brobecker 2003-03-05: Here would be a much better place
         to attach the ada-tasks observers, instead of doing this
         unconditionaly in _initialize_tasks.  This would avoid an
         unecessary notification when the inferior does not use tasking
         or as long as the user does not use the ada-tasks commands.
         Unfortunately, this is not possible for the moment: the current
         code resets ada__tasks_check_symbol_table back to 1 whenever
         symbols for a new program are being loaded.  If we place the
         observers intialization here, we will end up adding new observers
         everytime we do the check for Ada tasking-related symbols
         above.  This would currently have benign effects, but is still
         undesirable.  The cleanest approach is probably to create a new
         observer to notify us when the user is debugging a new program.
         We would then reset ada__tasks_check_symbol_table back to 1
         during the notification, but also detach all observers.
         BTW: observers are probably not reentrant, so detaching during
         a notification may not be the safest thing to do...  Sigh...
         But creating the new observer would be a good idea in any case,
         since this allow us to make ada__tasks_check_symbol_table
         static, which is a good bonus.  */
      ada_tasks_check_symbol_table = 0;
    }

  return known_tasks_addr;
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
get_tcb_types_info (struct type **atcb_type,
                    struct type **atcb_common_type,
                    struct type **atcb_ll_type,
                    struct type **atcb_call_type,
                    struct tcb_fieldnos *atcb_fieldnos)
{
  struct type *type;
  struct type *common_type;
  struct type *ll_type;
  struct type *call_type;
  struct tcb_fieldnos fieldnos;

  const char *atcb_name = "system__tasking__ada_task_control_block___XVE";
  const char *atcb_name_fixed = "system__tasking__ada_task_control_block";
  const char *common_atcb_name = "system__tasking__common_atcb";
  const char *private_data_name = "system__task_primitives__private_data";
  const char *entry_call_record_name = "system__tasking__entry_call_record";

  /* ATCB symbols may be found in several compilation units.  As we
     are only interested in one instance, use standard (literal,
     C-like) lookups to get the first match.  */

  struct symbol *atcb_sym =
    lookup_symbol_in_language (atcb_name, NULL, VAR_DOMAIN,
			       language_c, NULL);
  const struct symbol *common_atcb_sym =
    lookup_symbol_in_language (common_atcb_name, NULL, VAR_DOMAIN,
			       language_c, NULL);
  const struct symbol *private_data_sym =
    lookup_symbol_in_language (private_data_name, NULL, VAR_DOMAIN,
			       language_c, NULL);
  const struct symbol *entry_call_record_sym =
    lookup_symbol_in_language (entry_call_record_name, NULL, VAR_DOMAIN,
			       language_c, NULL);

  if (atcb_sym == NULL || atcb_sym->type == NULL)
    {
      /* In Ravenscar run-time libs, the  ATCB does not have a dynamic
         size, so the symbol name differs.  */
      atcb_sym = lookup_symbol_in_language (atcb_name_fixed, NULL, VAR_DOMAIN,
					    language_c, NULL);

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
  *atcb_type = type;
  *atcb_common_type = common_type;
  *atcb_ll_type = ll_type;
  *atcb_call_type = call_type;
  *atcb_fieldnos = fieldnos;
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

  ll_value = value_field (common_value, fieldno.ll);

  if (fieldno.ll_lwp >= 0)
    lwp = value_as_address (value_field (ll_value, fieldno.ll_lwp));
  thread = value_as_long (value_field (ll_value, fieldno.ll_thread));

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
  const char ravenscar_task_name[] = "Ravenscar task";

  if (atcb_type == NULL)
    get_tcb_types_info (&atcb_type, &atcb_common_type, &atcb_ll_type,
                        &atcb_call_type, &fieldno);

  tcb_value = value_from_contents_and_address (atcb_type, NULL, task_id);
  common_value = value_field (tcb_value, fieldno.common);

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

  if (fieldno.image_len == -1)
    {
      if (fieldno.image >= 0)
        read_fat_string_value (task_info->name,
                               value_field (common_value, fieldno.image),
                               sizeof (task_info->name) - 1);
      else
        strcpy (task_info->name, ravenscar_task_name);
    }
  else
    {
      int len = value_as_long (value_field (common_value, fieldno.image_len));

      value_as_string (task_info->name,
                       value_field (common_value, fieldno.image), len);
    }

  /* Compute the task state and priority.  */

  task_info->state = value_as_long (value_field (common_value, fieldno.state));
  task_info->priority =
    value_as_long (value_field (common_value, fieldno.priority));

  /* If the ATCB contains some information about the parent task,
     then compute it as well.  Otherwise, zero.  */

  if (fieldno.parent >= 0)
    task_info->parent =
      value_as_address (value_field (common_value, fieldno.parent));
  else
    task_info->parent = 0;
  

  /* If the ATCB contains some information about entry calls, then
     compute the "called_task" as well.  Otherwise, zero.  */

  if (fieldno.atc_nesting_level > 0 && fieldno.entry_calls > 0) 
    {
      /* Let My_ATCB be the Ada task control block of a task calling the
         entry of another task; then the Task_Id of the called task is
         in My_ATCB.Entry_Calls (My_ATCB.ATC_Nesting_Level).Called_Task.  */
      atc_nesting_level_value = value_field (tcb_value,
                                             fieldno.atc_nesting_level);
      entry_calls_value =
        ada_coerce_to_simple_array_ptr (value_field (tcb_value,
                                                     fieldno.entry_calls));
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
  if (fieldno.call >= 0)
    {
      /* Get the ID of the caller task from Common_ATCB.Call.all.Self.
         If Common_ATCB.Call is null, then there is no caller.  */
      const CORE_ADDR call =
        value_as_address (value_field (common_value, fieldno.call));
      struct value *call_val;

      if (call != 0)
        {
          call_val =
            value_from_contents_and_address (atcb_call_type, NULL, call);
          task_info->caller_task =
            value_as_address (value_field (call_val, fieldno.call_self));
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
   add the result to the TASK_LIST.  */

static void
add_ada_task (CORE_ADDR task_id)
{
  struct ada_task_info task_info;

  read_atcb (task_id, &task_info);
  VEC_safe_push (ada_task_info_s, task_list, &task_info);
}

/* Read the Known_Tasks array from the inferior memory, and store
   it in TASK_LIST.  Return non-zero upon success.  */

static int
read_known_tasks_array (void)
{
  const int target_ptr_byte =
    gdbarch_ptr_bit (target_gdbarch) / TARGET_CHAR_BIT;
  const CORE_ADDR known_tasks_addr = get_known_tasks_addr ();
  const int known_tasks_size = target_ptr_byte * MAX_NUMBER_OF_KNOWN_TASKS;
  gdb_byte *known_tasks = alloca (known_tasks_size);
  int i;

  /* Step 1: Clear the current list, if necessary.  */
  VEC_truncate (ada_task_info_s, task_list, 0);

  /* If the application does not use task, then no more needs to be done.
     It is important to have the task list cleared (see above) before we
     return, as we don't want a stale task list to be used...  This can
     happen for instance when debugging a non-multitasking program after
     having debugged a multitasking one.  */
  if (known_tasks_addr == 0)
    return 0;

  /* Step 2: Build a new list by reading the ATCBs from the Known_Tasks
     array in the Ada runtime.  */
  read_memory (known_tasks_addr, known_tasks, known_tasks_size);
  for (i = 0; i < MAX_NUMBER_OF_KNOWN_TASKS; i++)
    {
      struct type *data_ptr_type =
        builtin_type (target_gdbarch)->builtin_data_ptr;
      CORE_ADDR task_id =
        extract_typed_address (known_tasks + i * target_ptr_byte,
			       data_ptr_type);

      if (task_id != 0)
        add_ada_task (task_id);
    }

  /* Step 3: Unset stale_task_list_p, to avoid re-reading the Known_Tasks
     array unless needed.  Then report a success.  */
  stale_task_list_p = 0;

  return 1;
}

/* Builds the task_list by reading the Known_Tasks array from
   the inferior.  Prints an appropriate message and returns non-zero
   if it failed to build this list.  */

int
ada_build_task_list (int warn_if_null)
{
  if (!target_has_stack)
    error (_("Cannot inspect Ada tasks when program is not running"));

  if (stale_task_list_p)
    read_known_tasks_array ();

  if (task_list == NULL)
    {
      if (warn_if_null)
        printf_filtered (_("Your application does not use any Ada tasks.\n"));
      return 0;
    }

  return 1;
}

/* Print a one-line description of the task whose number is TASKNO.
   The formatting should fit the "info tasks" array.  */

static void
short_task_info (int taskno)
{
  const struct ada_task_info *const task_info =
    VEC_index (ada_task_info_s, task_list, taskno - 1);
  int active_task_p;

  gdb_assert (task_info != NULL);

  /* Print a star if this task is the current task (or the task currently
     selected).  */

  active_task_p = ptid_equal (task_info->ptid, inferior_ptid);
  if (active_task_p)
    printf_filtered ("*");
  else
    printf_filtered (" ");

  /* Print the task number.  */
  printf_filtered ("%3d", taskno);

  /* Print the Task ID.  */
  printf_filtered (" %9lx", (long) task_info->task_id);

  /* Print the Task ID of the task parent.  */
  printf_filtered (" %4d", get_task_number_from_id (task_info->parent));

  /* Print the base priority of the task.  */
  printf_filtered (" %3d", task_info->priority);

  /* Print the task current state.  */
  if (task_info->caller_task)
    printf_filtered (_(" Accepting RV with %-4d"),
                     get_task_number_from_id (task_info->caller_task));
  else if (task_info->state == Entry_Caller_Sleep && task_info->called_task)
    printf_filtered (_(" Waiting on RV with %-3d"),
                     get_task_number_from_id (task_info->called_task));
  else
    printf_filtered (" %-22s", _(task_states[task_info->state]));

  /* Finally, print the task name.  */
  if (task_info->name[0] != '\0')
    printf_filtered (" %s\n", task_info->name);
  else
    printf_filtered (_(" <no name>\n"));
}

/* Print a list containing a short description of all Ada tasks.  */
/* FIXME: Shouldn't we be using ui_out???  */

static void
info_tasks (int from_tty)
{
  int taskno;
  const int nb_tasks = VEC_length (ada_task_info_s, task_list);

  printf_filtered (_("  ID       TID P-ID Pri State                  Name\n"));
  
  for (taskno = 1; taskno <= nb_tasks; taskno++)
    short_task_info (taskno);
}

/* Print a detailed description of the Ada task whose ID is TASKNO_STR.  */

static void
info_task (char *taskno_str, int from_tty)
{
  const int taskno = value_as_long (parse_and_eval (taskno_str));
  struct ada_task_info *task_info;
  int parent_taskno = 0;

  if (taskno <= 0 || taskno > VEC_length (ada_task_info_s, task_list))
    error (_("Task ID %d not known.  Use the \"info tasks\" command to\n"
             "see the IDs of currently known tasks"), taskno);
  task_info = VEC_index (ada_task_info_s, task_list, taskno - 1);

  /* Print the Ada task ID.  */
  printf_filtered (_("Ada Task: %s\n"),
		   paddress (target_gdbarch, task_info->task_id));

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
    parent_taskno = get_task_number_from_id (task_info->parent);
  if (parent_taskno)
    {
      struct ada_task_info *parent =
        VEC_index (ada_task_info_s, task_list, parent_taskno - 1);

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
        target_taskno = get_task_number_from_id (task_info->caller_task);
        printf_filtered (_("State: Accepting rendezvous with %d"),
                         target_taskno);
      }
    else if (task_info->state == Entry_Caller_Sleep && task_info->called_task)
      {
        target_taskno = get_task_number_from_id (task_info->called_task);
        printf_filtered (_("State: Waiting on task %d's entry"),
                         target_taskno);
      }
    else
      printf_filtered (_("State: %s"), _(long_task_states[task_info->state]));

    if (target_taskno)
      {
        struct ada_task_info *target_task_info =
          VEC_index (ada_task_info_s, task_list, target_taskno - 1);

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
  const int task_list_built = ada_build_task_list (1);

  if (!task_list_built)
    return;

  if (arg == NULL || *arg == '\0')
    info_tasks (from_tty);
  else
    info_task (arg, from_tty);
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
task_command_1 (char *taskno_str, int from_tty)
{
  const int taskno = value_as_long (parse_and_eval (taskno_str));
  struct ada_task_info *task_info;

  if (taskno <= 0 || taskno > VEC_length (ada_task_info_s, task_list))
    error (_("Task ID %d not known.  Use the \"info tasks\" command to\n"
             "see the IDs of currently known tasks"), taskno);
  task_info = VEC_index (ada_task_info_s, task_list, taskno - 1);

  if (!ada_task_is_alive (task_info))
    error (_("Cannot switch to task %d: Task is no longer running"), taskno);
   
  /* On some platforms, the thread list is not updated until the user
     performs a thread-related operation (by using the "info threads"
     command, for instance).  So this thread list may not be up to date
     when the user attempts this task switch.  Since we cannot switch
     to the thread associated to our task if GDB does not know about
     that thread, we need to make sure that any new threads gets added
     to the thread list.  */
  target_find_new_threads ();

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
                     frame_relative_level (get_selected_frame (NULL)), 1);
}


/* Print the ID of the current task if TASKNO_STR is empty or NULL.
   Otherwise, switch to the task indicated by TASKNO_STR.  */

static void
task_command (char *taskno_str, int from_tty)
{
  const int task_list_built = ada_build_task_list (1);

  if (!task_list_built)
    return;

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
      task_command_1 (taskno_str, from_tty);
    }
}

/* Indicate that the task list may have changed, so invalidate the cache.  */

static void
ada_task_list_changed (void)
{
  stale_task_list_p = 1;  
}

/* The 'normal_stop' observer notification callback.  */

static void
ada_normal_stop_observer (struct bpstats *unused_args, int unused_args2)
{
  /* The inferior has been resumed, and just stopped. This means that
     our task_list needs to be recomputed before it can be used again.  */
  ada_task_list_changed ();
}

/* A routine to be called when the objfiles have changed.  */

static void
ada_new_objfile_observer (struct objfile *objfile)
{
  /* Invalidate all cached data that were extracted from an objfile.  */

  atcb_type = NULL;
  atcb_common_type = NULL;
  atcb_ll_type = NULL;
  atcb_call_type = NULL;

  ada_tasks_check_symbol_table = 1;
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_tasks;

void
_initialize_tasks (void)
{
  /* Attach various observers.  */
  observer_attach_normal_stop (ada_normal_stop_observer);
  observer_attach_new_objfile (ada_new_objfile_observer);

  /* Some new commands provided by this module.  */
  add_info ("tasks", info_tasks_command,
            _("Provide information about all known Ada tasks"));
  add_cmd ("task", class_run, task_command,
           _("Use this command to switch between Ada tasks.\n\
Without argument, this command simply prints the current task ID"),
           &cmdlist);
}

