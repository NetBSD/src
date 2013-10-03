/* MI Command Set for GDB, the GNU debugger.
   Copyright (C) 2000-2013 Free Software Foundation, Inc.

   Contributed by Cygnus Solutions (a Red Hat company).

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
#include "top.h"
#include "mi-cmds.h"
#include "gdb_string.h"
#include "mi-main.h"

extern void _initialize_mi_cmds (void);

struct mi_cmd;
static struct mi_cmd **lookup_table (const char *command);
static void build_table (struct mi_cmd *commands);

static struct mi_cmd mi_cmds[] =
{
/* Define a MI command of NAME, and its corresponding CLI command is
   CLI_NAME.  */
#define DEF_MI_CMD_CLI_1(NAME, CLI_NAME, ARGS_P, CALLED)	\
  { NAME, { CLI_NAME, ARGS_P}, NULL, CALLED }
#define DEF_MI_CMD_CLI(NAME, CLI_NAME, ARGS_P) \
  DEF_MI_CMD_CLI_1(NAME, CLI_NAME, ARGS_P, NULL)

/* Define a MI command of NAME, and implemented by function MI_FUNC.  */
#define DEF_MI_CMD_MI_1(NAME, MI_FUNC, CALLED) \
  { NAME, {NULL, 0}, MI_FUNC, CALLED }
#define DEF_MI_CMD_MI(NAME, MI_FUNC) DEF_MI_CMD_MI_1(NAME, MI_FUNC, NULL)

  DEF_MI_CMD_MI ("ada-task-info", mi_cmd_ada_task_info),
  DEF_MI_CMD_MI ("add-inferior", mi_cmd_add_inferior),
  DEF_MI_CMD_CLI_1 ("break-after", "ignore", 1,
		    &mi_suppress_notification.breakpoint),
  DEF_MI_CMD_CLI_1 ("break-condition","cond", 1,
		  &mi_suppress_notification.breakpoint),
  DEF_MI_CMD_MI_1 ("break-commands", mi_cmd_break_commands,
		   &mi_suppress_notification.breakpoint),
  DEF_MI_CMD_CLI_1 ("break-delete", "delete breakpoint", 1,
		    &mi_suppress_notification.breakpoint),
  DEF_MI_CMD_CLI_1 ("break-disable", "disable breakpoint", 1,
		    &mi_suppress_notification.breakpoint),
  DEF_MI_CMD_CLI_1 ("break-enable", "enable breakpoint", 1,
		     &mi_suppress_notification.breakpoint),
  DEF_MI_CMD_CLI ("break-info", "info break", 1),
  DEF_MI_CMD_MI_1 ("break-insert", mi_cmd_break_insert,
		   &mi_suppress_notification.breakpoint),
  DEF_MI_CMD_CLI ("break-list", "info break", 0),
  DEF_MI_CMD_MI_1 ("break-passcount", mi_cmd_break_passcount,
		   &mi_suppress_notification.breakpoint),
  DEF_MI_CMD_MI_1 ("break-watch", mi_cmd_break_watch,
		   &mi_suppress_notification.breakpoint),
  DEF_MI_CMD_MI_1 ("catch-load", mi_cmd_catch_load,
                   &mi_suppress_notification.breakpoint),
  DEF_MI_CMD_MI_1 ("catch-unload", mi_cmd_catch_unload,
                   &mi_suppress_notification.breakpoint),
  DEF_MI_CMD_MI ("data-disassemble", mi_cmd_disassemble),
  DEF_MI_CMD_MI ("data-evaluate-expression", mi_cmd_data_evaluate_expression),
  DEF_MI_CMD_MI ("data-list-changed-registers",
		 mi_cmd_data_list_changed_registers),
  DEF_MI_CMD_MI ("data-list-register-names", mi_cmd_data_list_register_names),
  DEF_MI_CMD_MI ("data-list-register-values", mi_cmd_data_list_register_values),
  DEF_MI_CMD_MI ("data-read-memory", mi_cmd_data_read_memory),
  DEF_MI_CMD_MI ("data-read-memory-bytes", mi_cmd_data_read_memory_bytes),
  DEF_MI_CMD_MI_1 ("data-write-memory", mi_cmd_data_write_memory,
		   &mi_suppress_notification.memory),
  DEF_MI_CMD_MI_1 ("data-write-memory-bytes", mi_cmd_data_write_memory_bytes,
		   &mi_suppress_notification.memory),
  DEF_MI_CMD_MI ("data-write-register-values",
		 mi_cmd_data_write_register_values),
  DEF_MI_CMD_MI ("enable-timings", mi_cmd_enable_timings),
  DEF_MI_CMD_MI ("enable-pretty-printing", mi_cmd_enable_pretty_printing),
  DEF_MI_CMD_MI ("environment-cd", mi_cmd_env_cd),
  DEF_MI_CMD_MI ("environment-directory", mi_cmd_env_dir),
  DEF_MI_CMD_MI ("environment-path", mi_cmd_env_path),
  DEF_MI_CMD_MI ("environment-pwd", mi_cmd_env_pwd),
  DEF_MI_CMD_CLI ("exec-arguments", "set args", 1),
  DEF_MI_CMD_MI ("exec-continue", mi_cmd_exec_continue),
  DEF_MI_CMD_MI ("exec-finish", mi_cmd_exec_finish),
  DEF_MI_CMD_MI ("exec-jump", mi_cmd_exec_jump),
  DEF_MI_CMD_MI ("exec-interrupt", mi_cmd_exec_interrupt),
  DEF_MI_CMD_MI ("exec-next", mi_cmd_exec_next),
  DEF_MI_CMD_MI ("exec-next-instruction", mi_cmd_exec_next_instruction),
  DEF_MI_CMD_MI ("exec-return", mi_cmd_exec_return),
  DEF_MI_CMD_MI ("exec-run", mi_cmd_exec_run),
  DEF_MI_CMD_MI ("exec-step", mi_cmd_exec_step),
  DEF_MI_CMD_MI ("exec-step-instruction", mi_cmd_exec_step_instruction),
  DEF_MI_CMD_CLI ("exec-until", "until", 1),
  DEF_MI_CMD_CLI ("file-exec-and-symbols", "file", 1),
  DEF_MI_CMD_CLI ("file-exec-file", "exec-file", 1),
  DEF_MI_CMD_MI ("file-list-exec-source-file",
		 mi_cmd_file_list_exec_source_file),
  DEF_MI_CMD_MI ("file-list-exec-source-files",
		 mi_cmd_file_list_exec_source_files),
  DEF_MI_CMD_CLI ("file-symbol-file", "symbol-file", 1),
  DEF_MI_CMD_MI ("gdb-exit", mi_cmd_gdb_exit),
  DEF_MI_CMD_CLI_1 ("gdb-set", "set", 1,
		    &mi_suppress_notification.cmd_param_changed),
  DEF_MI_CMD_CLI ("gdb-show", "show", 1),
  DEF_MI_CMD_CLI ("gdb-version", "show version", 0),
  DEF_MI_CMD_MI ("inferior-tty-set", mi_cmd_inferior_tty_set),
  DEF_MI_CMD_MI ("inferior-tty-show", mi_cmd_inferior_tty_show),
  DEF_MI_CMD_MI ("info-os", mi_cmd_info_os),
  DEF_MI_CMD_MI ("interpreter-exec", mi_cmd_interpreter_exec),
  DEF_MI_CMD_MI ("list-features", mi_cmd_list_features),
  DEF_MI_CMD_MI ("list-target-features", mi_cmd_list_target_features),
  DEF_MI_CMD_MI ("list-thread-groups", mi_cmd_list_thread_groups),
  DEF_MI_CMD_MI ("remove-inferior", mi_cmd_remove_inferior),
  DEF_MI_CMD_MI ("stack-info-depth", mi_cmd_stack_info_depth),
  DEF_MI_CMD_MI ("stack-info-frame", mi_cmd_stack_info_frame),
  DEF_MI_CMD_MI ("stack-list-arguments", mi_cmd_stack_list_args),
  DEF_MI_CMD_MI ("stack-list-frames", mi_cmd_stack_list_frames),
  DEF_MI_CMD_MI ("stack-list-locals", mi_cmd_stack_list_locals),
  DEF_MI_CMD_MI ("stack-list-variables", mi_cmd_stack_list_variables),
  DEF_MI_CMD_MI ("stack-select-frame", mi_cmd_stack_select_frame),
  DEF_MI_CMD_MI ("symbol-list-lines", mi_cmd_symbol_list_lines),
  DEF_MI_CMD_CLI ("target-attach", "attach", 1),
  DEF_MI_CMD_MI ("target-detach", mi_cmd_target_detach),
  DEF_MI_CMD_CLI ("target-disconnect", "disconnect", 0),
  DEF_MI_CMD_CLI ("target-download", "load", 1),
  DEF_MI_CMD_MI ("target-file-delete", mi_cmd_target_file_delete),
  DEF_MI_CMD_MI ("target-file-get", mi_cmd_target_file_get),
  DEF_MI_CMD_MI ("target-file-put", mi_cmd_target_file_put),
  DEF_MI_CMD_CLI ("target-select", "target", 1),
  DEF_MI_CMD_MI ("thread-info", mi_cmd_thread_info),
  DEF_MI_CMD_MI ("thread-list-ids", mi_cmd_thread_list_ids),
  DEF_MI_CMD_MI ("thread-select", mi_cmd_thread_select),
  DEF_MI_CMD_MI ("trace-define-variable", mi_cmd_trace_define_variable),
  DEF_MI_CMD_MI_1 ("trace-find", mi_cmd_trace_find,
		   &mi_suppress_notification.traceframe),
  DEF_MI_CMD_MI ("trace-list-variables", mi_cmd_trace_list_variables),
  DEF_MI_CMD_MI ("trace-save", mi_cmd_trace_save),
  DEF_MI_CMD_MI ("trace-start", mi_cmd_trace_start),
  DEF_MI_CMD_MI ("trace-status", mi_cmd_trace_status),
  DEF_MI_CMD_MI ("trace-stop", mi_cmd_trace_stop),
  DEF_MI_CMD_MI ("var-assign", mi_cmd_var_assign),
  DEF_MI_CMD_MI ("var-create", mi_cmd_var_create),
  DEF_MI_CMD_MI ("var-delete", mi_cmd_var_delete),
  DEF_MI_CMD_MI ("var-evaluate-expression", mi_cmd_var_evaluate_expression),
  DEF_MI_CMD_MI ("var-info-path-expression", mi_cmd_var_info_path_expression),
  DEF_MI_CMD_MI ("var-info-expression", mi_cmd_var_info_expression),
  DEF_MI_CMD_MI ("var-info-num-children", mi_cmd_var_info_num_children),
  DEF_MI_CMD_MI ("var-info-type", mi_cmd_var_info_type),
  DEF_MI_CMD_MI ("var-list-children", mi_cmd_var_list_children),
  DEF_MI_CMD_MI ("var-set-format", mi_cmd_var_set_format),
  DEF_MI_CMD_MI ("var-set-frozen", mi_cmd_var_set_frozen),
  DEF_MI_CMD_MI ("var-set-update-range", mi_cmd_var_set_update_range),
  DEF_MI_CMD_MI ("var-set-visualizer", mi_cmd_var_set_visualizer),
  DEF_MI_CMD_MI ("var-show-attributes", mi_cmd_var_show_attributes),
  DEF_MI_CMD_MI ("var-show-format", mi_cmd_var_show_format),
  DEF_MI_CMD_MI ("var-update", mi_cmd_var_update),
  { NULL, }
};

/* Pointer to the mi command table (built at run time). */

static struct mi_cmd **mi_table;

/* A prime large enough to accomodate the entire command table.  */
enum
  {
    MI_TABLE_SIZE = 227
  };

/* Exported function used to obtain info from the table.  */
struct mi_cmd *
mi_lookup (const char *command)
{
  return *lookup_table (command);
}

/* Used for collecting hash hit/miss statistics.  */

struct mi_cmd_stats
{
  int hit;
  int miss;
  int rehash;
};
struct mi_cmd_stats stats;

/* Look up a command.  */

static struct mi_cmd **
lookup_table (const char *command)
{
  const char *chp;
  unsigned int index = 0;

  /* Compute our hash.  */
  for (chp = command; *chp; chp++)
    {
      /* We use a somewhat arbitrary formula.  */
      index = ((index << 6) + (unsigned int) *chp) % MI_TABLE_SIZE;
    }

  while (1)
    {
      struct mi_cmd **entry = &mi_table[index];
      if ((*entry) == 0)
	{
	  /* not found, return pointer to next free. */
	  stats.miss++;
	  return entry;
	}
      if (strcmp (command, (*entry)->name) == 0)
	{
	  stats.hit++;
	  return entry;		/* found */
	}
      index = (index + 1) % MI_TABLE_SIZE;
      stats.rehash++;
    }
}

static void
build_table (struct mi_cmd *commands)
{
  int nr_rehash = 0;
  int nr_entries = 0;
  struct mi_cmd *command;
  int sizeof_table = sizeof (struct mi_cmd **) * MI_TABLE_SIZE;

  mi_table = xmalloc (sizeof_table);
  memset (mi_table, 0, sizeof_table);
  for (command = commands; command->name != 0; command++)
    {
      struct mi_cmd **entry = lookup_table (command->name);

      if (*entry)
	internal_error (__FILE__, __LINE__,
			_("command `%s' appears to be duplicated"),
			command->name);
      *entry = command;
      /* FIXME lose these prints */
      if (0)
	{
	  fprintf_unfiltered (gdb_stdlog, "%-30s %2d\n",
			      command->name, stats.rehash - nr_rehash);
	}
      nr_entries++;
      nr_rehash = stats.rehash;
    }
  if (0)
    {
      fprintf_filtered (gdb_stdlog, "Average %3.1f\n",
			(double) nr_rehash / (double) nr_entries);
    }
}

void
_initialize_mi_cmds (void)
{
  build_table (mi_cmds);
  memset (&stats, 0, sizeof (stats));
}
