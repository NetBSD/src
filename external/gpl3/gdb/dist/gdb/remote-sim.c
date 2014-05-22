/* Generic remote debugging interface for simulators.

   Copyright (C) 1993-2013 Free Software Foundation, Inc.

   Contributed by Cygnus Support.
   Steve Chamberlain (sac@cygnus.com).

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
#include "inferior.h"
#include "value.h"
#include "gdb_string.h"
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include "terminal.h"
#include "target.h"
#include "gdbcore.h"
#include "gdb/callback.h"
#include "gdb/remote-sim.h"
#include "command.h"
#include "regcache.h"
#include "gdb_assert.h"
#include "sim-regno.h"
#include "arch-utils.h"
#include "readline/readline.h"
#include "gdbthread.h"

/* Prototypes */

extern void _initialize_remote_sim (void);

static void dump_mem (char *buf, int len);

static void init_callbacks (void);

static void end_callbacks (void);

static int gdb_os_write_stdout (host_callback *, const char *, int);

static void gdb_os_flush_stdout (host_callback *);

static int gdb_os_write_stderr (host_callback *, const char *, int);

static void gdb_os_flush_stderr (host_callback *);

static int gdb_os_poll_quit (host_callback *);

/* printf_filtered is depreciated.  */
static void gdb_os_printf_filtered (host_callback *, const char *, ...);

static void gdb_os_vprintf_filtered (host_callback *, const char *, va_list);

static void gdb_os_evprintf_filtered (host_callback *, const char *, va_list);

static void gdb_os_error (host_callback *, const char *, ...)
     ATTRIBUTE_NORETURN;

static void gdbsim_kill (struct target_ops *);

static void gdbsim_load (char *prog, int fromtty);

static void gdbsim_open (char *args, int from_tty);

static void gdbsim_close (int quitting);

static void gdbsim_detach (struct target_ops *ops, char *args, int from_tty);

static void gdbsim_prepare_to_store (struct regcache *regcache);

static void gdbsim_files_info (struct target_ops *target);

static void gdbsim_mourn_inferior (struct target_ops *target);

static void gdbsim_stop (ptid_t ptid);

void simulator_command (char *args, int from_tty);

/* Naming convention:

   sim_* are the interface to the simulator (see remote-sim.h).
   gdbsim_* are stuff which is internal to gdb.  */

/* Forward data declarations */
extern struct target_ops gdbsim_ops;

static const struct inferior_data *sim_inferior_data_key;

/* Simulator-specific, per-inferior state.  */
struct sim_inferior_data {
  /* Flag which indicates whether or not the program has been loaded.  */
  int program_loaded;

  /* Simulator descriptor for this inferior.  */
  SIM_DESC gdbsim_desc;

  /* This is the ptid we use for this particular simulator instance.  Its
     value is somewhat arbitrary, as the simulator target don't have a
     notion of tasks or threads, but we need something non-null to place
     in inferior_ptid.  For simulators which permit multiple instances,
     we also need a unique identifier to use for each inferior.  */
  ptid_t remote_sim_ptid;

  /* Signal with which to resume.  */
  enum gdb_signal resume_siggnal;

  /* Flag which indicates whether resume should step or not.  */
  int resume_step;
};

/* Flag indicating the "open" status of this module.  It's set to 1
   in gdbsim_open() and 0 in gdbsim_close().  */
static int gdbsim_is_open = 0;

/* Value of the next pid to allocate for an inferior.  As indicated
   elsewhere, its initial value is somewhat arbitrary; it's critical
   though that it's not zero or negative.  */
static int next_pid;
#define INITIAL_PID 42000

/* Argument list to pass to sim_open().  It is allocated in gdbsim_open()
   and deallocated in gdbsim_close().  The lifetime needs to extend beyond
   the call to gdbsim_open() due to the fact that other sim instances other
   than the first will be allocated after the gdbsim_open() call.  */
static char **sim_argv = NULL;

/* OS-level callback functions for write, flush, etc.  */
static host_callback gdb_callback;
static int callbacks_initialized = 0;

/* Callback for iterate_over_inferiors.  It checks to see if the sim
   descriptor passed via ARG is the same as that for the inferior
   designated by INF.  Return true if so; false otherwise.  */

static int
check_for_duplicate_sim_descriptor (struct inferior *inf, void *arg)
{
  struct sim_inferior_data *sim_data;
  SIM_DESC new_sim_desc = arg;

  sim_data = inferior_data (inf, sim_inferior_data_key);

  return (sim_data != NULL && sim_data->gdbsim_desc == new_sim_desc);
}

/* Flags indicating whether or not a sim instance is needed.  One of these
   flags should be passed to get_sim_inferior_data().  */

enum {SIM_INSTANCE_NOT_NEEDED = 0, SIM_INSTANCE_NEEDED = 1};

/* Obtain pointer to per-inferior simulator data, allocating it if necessary.
   Attempt to open the sim if SIM_INSTANCE_NEEDED is true.  */

static struct sim_inferior_data *
get_sim_inferior_data (struct inferior *inf, int sim_instance_needed)
{
  SIM_DESC sim_desc = NULL;
  struct sim_inferior_data *sim_data
    = inferior_data (inf, sim_inferior_data_key);

  /* Try to allocate a new sim instance, if needed.  We do this ahead of
     a potential allocation of a sim_inferior_data struct in order to
     avoid needlessly allocating that struct in the event that the sim
     instance allocation fails.  */
  if (sim_instance_needed == SIM_INSTANCE_NEEDED 
      && (sim_data == NULL || sim_data->gdbsim_desc == NULL))
    {
      struct inferior *idup;
      sim_desc = sim_open (SIM_OPEN_DEBUG, &gdb_callback, exec_bfd, sim_argv);
      if (sim_desc == NULL)
	error (_("Unable to create simulator instance for inferior %d."),
	       inf->num);

      idup = iterate_over_inferiors (check_for_duplicate_sim_descriptor,
                                     sim_desc);
      if (idup != NULL)
	{
	  /* We don't close the descriptor due to the fact that it's
	     shared with some other inferior.  If we were to close it,
	     that might needlessly muck up the other inferior.  Of
	     course, it's possible that the damage has already been
	     done...  Note that it *will* ultimately be closed during
	     cleanup of the other inferior.  */
	  sim_desc = NULL;
	  error (
 _("Inferior %d and inferior %d would have identical simulator state.\n"
   "(This simulator does not support the running of more than one inferior.)"),
		 inf->num, idup->num); 
        }
    }

  if (sim_data == NULL)
    {
      sim_data = XZALLOC(struct sim_inferior_data);
      set_inferior_data (inf, sim_inferior_data_key, sim_data);

      /* Allocate a ptid for this inferior.  */
      sim_data->remote_sim_ptid = ptid_build (next_pid, 0, next_pid);
      next_pid++;

      /* Initialize the other instance variables.  */
      sim_data->program_loaded = 0;
      sim_data->gdbsim_desc = sim_desc;
      sim_data->resume_siggnal = GDB_SIGNAL_0;
      sim_data->resume_step = 0;
    }
  else if (sim_desc)
    {
      /* This handles the case where sim_data was allocated prior to
         needing a sim instance.  */
      sim_data->gdbsim_desc = sim_desc;
    }


  return sim_data;
}

/* Return pointer to per-inferior simulator data using PTID to find the
   inferior in question.  Return NULL when no inferior is found or
   when ptid has a zero or negative pid component.  */

static struct sim_inferior_data *
get_sim_inferior_data_by_ptid (ptid_t ptid, int sim_instance_needed)
{
  struct inferior *inf;
  int pid = ptid_get_pid (ptid);

  if (pid <= 0)
    return NULL;
  
  inf = find_inferior_pid (pid);

  if (inf)
    return get_sim_inferior_data (inf, sim_instance_needed);
  else
    return NULL;
}

/* Free the per-inferior simulator data.  */

static void
sim_inferior_data_cleanup (struct inferior *inf, void *data)
{
  struct sim_inferior_data *sim_data = data;

  if (sim_data != NULL)
    {
      if (sim_data->gdbsim_desc)
	{
	  sim_close (sim_data->gdbsim_desc, 0);
	  sim_data->gdbsim_desc = NULL;
	}
      xfree (sim_data);
    }
}

static void
dump_mem (char *buf, int len)
{
  printf_filtered ("\t");

  if (len == 8 || len == 4)
    {
      uint32_t l[2];

      memcpy (l, buf, len);
      printf_filtered ("0x%08x", l[0]);
      if (len == 8)
	printf_filtered (" 0x%08x", l[1]);
    }
  else
    {
      int i;

      for (i = 0; i < len; i++)
	printf_filtered ("0x%02x ", buf[i]);
    }

  printf_filtered ("\n");
}

/* Initialize gdb_callback.  */

static void
init_callbacks (void)
{
  if (!callbacks_initialized)
    {
      gdb_callback = default_callback;
      gdb_callback.init (&gdb_callback);
      gdb_callback.write_stdout = gdb_os_write_stdout;
      gdb_callback.flush_stdout = gdb_os_flush_stdout;
      gdb_callback.write_stderr = gdb_os_write_stderr;
      gdb_callback.flush_stderr = gdb_os_flush_stderr;
      gdb_callback.printf_filtered = gdb_os_printf_filtered;
      gdb_callback.vprintf_filtered = gdb_os_vprintf_filtered;
      gdb_callback.evprintf_filtered = gdb_os_evprintf_filtered;
      gdb_callback.error = gdb_os_error;
      gdb_callback.poll_quit = gdb_os_poll_quit;
      gdb_callback.magic = HOST_CALLBACK_MAGIC;
      callbacks_initialized = 1;
    }
}

/* Release callbacks (free resources used by them).  */

static void
end_callbacks (void)
{
  if (callbacks_initialized)
    {
      gdb_callback.shutdown (&gdb_callback);
      callbacks_initialized = 0;
    }
}

/* GDB version of os_write_stdout callback.  */

static int
gdb_os_write_stdout (host_callback *p, const char *buf, int len)
{
  int i;
  char b[2];

  ui_file_write (gdb_stdtarg, buf, len);
  return len;
}

/* GDB version of os_flush_stdout callback.  */

static void
gdb_os_flush_stdout (host_callback *p)
{
  gdb_flush (gdb_stdtarg);
}

/* GDB version of os_write_stderr callback.  */

static int
gdb_os_write_stderr (host_callback *p, const char *buf, int len)
{
  int i;
  char b[2];

  for (i = 0; i < len; i++)
    {
      b[0] = buf[i];
      b[1] = 0;
      fputs_unfiltered (b, gdb_stdtargerr);
    }
  return len;
}

/* GDB version of os_flush_stderr callback.  */

static void
gdb_os_flush_stderr (host_callback *p)
{
  gdb_flush (gdb_stdtargerr);
}

/* GDB version of printf_filtered callback.  */

static void
gdb_os_printf_filtered (host_callback * p, const char *format,...)
{
  va_list args;

  va_start (args, format);
  vfprintf_filtered (gdb_stdout, format, args);
  va_end (args);
}

/* GDB version of error vprintf_filtered.  */

static void
gdb_os_vprintf_filtered (host_callback * p, const char *format, va_list ap)
{
  vfprintf_filtered (gdb_stdout, format, ap);
}

/* GDB version of error evprintf_filtered.  */

static void
gdb_os_evprintf_filtered (host_callback * p, const char *format, va_list ap)
{
  vfprintf_filtered (gdb_stderr, format, ap);
}

/* GDB version of error callback.  */

static void
gdb_os_error (host_callback * p, const char *format, ...)
{
  va_list args;

  va_start (args, format);
  verror (format, args);
  va_end (args);
}

int
one2one_register_sim_regno (struct gdbarch *gdbarch, int regnum)
{
  /* Only makes sense to supply raw registers.  */
  gdb_assert (regnum >= 0 && regnum < gdbarch_num_regs (gdbarch));
  return regnum;
}

static void
gdbsim_fetch_register (struct target_ops *ops,
		       struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NEEDED);

  if (regno == -1)
    {
      for (regno = 0; regno < gdbarch_num_regs (gdbarch); regno++)
	gdbsim_fetch_register (ops, regcache, regno);
      return;
    }

  switch (gdbarch_register_sim_regno (gdbarch, regno))
    {
    case LEGACY_SIM_REGNO_IGNORE:
      break;
    case SIM_REGNO_DOES_NOT_EXIST:
      {
	/* For moment treat a `does not exist' register the same way
           as an ``unavailable'' register.  */
	gdb_byte buf[MAX_REGISTER_SIZE];
	int nr_bytes;

	memset (buf, 0, MAX_REGISTER_SIZE);
	regcache_raw_supply (regcache, regno, buf);
	break;
      }
      
    default:
      {
	static int warn_user = 1;
	gdb_byte buf[MAX_REGISTER_SIZE];
	int nr_bytes;

	gdb_assert (regno >= 0 && regno < gdbarch_num_regs (gdbarch));
	memset (buf, 0, MAX_REGISTER_SIZE);
	nr_bytes = sim_fetch_register (sim_data->gdbsim_desc,
				       gdbarch_register_sim_regno
					 (gdbarch, regno),
				       buf,
				       register_size (gdbarch, regno));
	if (nr_bytes > 0
	    && nr_bytes != register_size (gdbarch, regno) && warn_user)
	  {
	    fprintf_unfiltered (gdb_stderr,
				"Size of register %s (%d/%d) "
				"incorrect (%d instead of %d))",
				gdbarch_register_name (gdbarch, regno),
				regno,
				gdbarch_register_sim_regno
				  (gdbarch, regno),
				nr_bytes, register_size (gdbarch, regno));
	    warn_user = 0;
	  }
	/* FIXME: cagney/2002-05-27: Should check `nr_bytes == 0'
	   indicating that GDB and the SIM have different ideas about
	   which registers are fetchable.  */
	/* Else if (nr_bytes < 0): an old simulator, that doesn't
	   think to return the register size.  Just assume all is ok.  */
	regcache_raw_supply (regcache, regno, buf);
	if (remote_debug)
	  {
	    printf_filtered ("gdbsim_fetch_register: %d", regno);
	    /* FIXME: We could print something more intelligible.  */
	    dump_mem (buf, register_size (gdbarch, regno));
	  }
	break;
      }
    }
}


static void
gdbsim_store_register (struct target_ops *ops,
		       struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NEEDED);

  if (regno == -1)
    {
      for (regno = 0; regno < gdbarch_num_regs (gdbarch); regno++)
	gdbsim_store_register (ops, regcache, regno);
      return;
    }
  else if (gdbarch_register_sim_regno (gdbarch, regno) >= 0)
    {
      char tmp[MAX_REGISTER_SIZE];
      int nr_bytes;

      regcache_cooked_read (regcache, regno, tmp);
      nr_bytes = sim_store_register (sim_data->gdbsim_desc,
				     gdbarch_register_sim_regno
				       (gdbarch, regno),
				     tmp, register_size (gdbarch, regno));
      if (nr_bytes > 0 && nr_bytes != register_size (gdbarch, regno))
	internal_error (__FILE__, __LINE__,
			_("Register size different to expected"));
      if (nr_bytes < 0)
        internal_error (__FILE__, __LINE__,
 			_("Register %d not updated"), regno);
      if (nr_bytes == 0)
        warning (_("Register %s not updated"),
                 gdbarch_register_name (gdbarch, regno));

      if (remote_debug)
	{
	  printf_filtered ("gdbsim_store_register: %d", regno);
	  /* FIXME: We could print something more intelligible.  */
	  dump_mem (tmp, register_size (gdbarch, regno));
	}
    }
}

/* Kill the running program.  This may involve closing any open files
   and releasing other resources acquired by the simulated program.  */

static void
gdbsim_kill (struct target_ops *ops)
{
  if (remote_debug)
    printf_filtered ("gdbsim_kill\n");

  /* There is no need to `kill' running simulator - the simulator is
     not running.  Mourning it is enough.  */
  target_mourn_inferior ();
}

/* Load an executable file into the target process.  This is expected to
   not only bring new code into the target process, but also to update
   GDB's symbol tables to match.  */

static void
gdbsim_load (char *args, int fromtty)
{
  char **argv;
  char *prog;
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NEEDED);

  if (args == NULL)
      error_no_arg (_("program to load"));

  argv = gdb_buildargv (args);
  make_cleanup_freeargv (argv);

  prog = tilde_expand (argv[0]);

  if (argv[1] != NULL)
    error (_("GDB sim does not yet support a load offset."));

  if (remote_debug)
    printf_filtered ("gdbsim_load: prog \"%s\"\n", prog);

  /* FIXME: We will print two messages on error.
     Need error to either not print anything if passed NULL or need
     another routine that doesn't take any arguments.  */
  if (sim_load (sim_data->gdbsim_desc, prog, NULL, fromtty) == SIM_RC_FAIL)
    error (_("unable to load program"));

  /* FIXME: If a load command should reset the targets registers then
     a call to sim_create_inferior() should go here.  */

  sim_data->program_loaded = 1;
}


/* Start an inferior process and set inferior_ptid to its pid.
   EXEC_FILE is the file to run.
   ARGS is a string containing the arguments to the program.
   ENV is the environment vector to pass.  Errors reported with error().
   On VxWorks and various standalone systems, we ignore exec_file.  */
/* This is called not only when we first attach, but also when the
   user types "run" after having attached.  */

static void
gdbsim_create_inferior (struct target_ops *target, char *exec_file, char *args,
			char **env, int from_tty)
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NEEDED);
  int len;
  char *arg_buf, **argv;

  if (exec_file == 0 || exec_bfd == 0)
    warning (_("No executable file specified."));
  if (!sim_data->program_loaded)
    warning (_("No program loaded."));

  if (remote_debug)
    printf_filtered ("gdbsim_create_inferior: exec_file \"%s\", args \"%s\"\n",
		     (exec_file ? exec_file : "(NULL)"),
		     args);

  if (ptid_equal (inferior_ptid, sim_data->remote_sim_ptid))
    gdbsim_kill (target);
  remove_breakpoints ();
  init_wait_for_inferior ();

  if (exec_file != NULL)
    {
      len = strlen (exec_file) + 1 + strlen (args) + 1 + /*slop */ 10;
      arg_buf = (char *) alloca (len);
      arg_buf[0] = '\0';
      strcat (arg_buf, exec_file);
      strcat (arg_buf, " ");
      strcat (arg_buf, args);
      argv = gdb_buildargv (arg_buf);
      make_cleanup_freeargv (argv);
    }
  else
    argv = NULL;

  if (!have_inferiors ())
    init_thread_list ();

  if (sim_create_inferior (sim_data->gdbsim_desc, exec_bfd, argv, env)
      != SIM_RC_OK)
    error (_("Unable to create sim inferior."));

  inferior_ptid = sim_data->remote_sim_ptid;
  inferior_appeared (current_inferior (), ptid_get_pid (inferior_ptid));
  add_thread_silent (inferior_ptid);

  insert_breakpoints ();	/* Needed to get correct instruction
				   in cache.  */

  clear_proceed_status ();
}

/* The open routine takes the rest of the parameters from the command,
   and (if successful) pushes a new target onto the stack.
   Targets should supply this routine, if only to provide an error message.  */
/* Called when selecting the simulator.  E.g. (gdb) target sim name.  */

static void
gdbsim_open (char *args, int from_tty)
{
  int len;
  char *arg_buf;
  struct sim_inferior_data *sim_data;
  SIM_DESC gdbsim_desc;

  if (remote_debug)
    printf_filtered ("gdbsim_open: args \"%s\"\n", args ? args : "(null)");

  /* Ensure that the sim target is not on the target stack.  This is
     necessary, because if it is on the target stack, the call to
     push_target below will invoke sim_close(), thus freeing various
     state (including a sim instance) that we allocate prior to
     invoking push_target().  We want to delay the push_target()
     operation until after we complete those operations which could
     error out.  */
  if (gdbsim_is_open)
    unpush_target (&gdbsim_ops);

  len = (7 + 1			/* gdbsim */
	 + strlen (" -E little")
	 + strlen (" --architecture=xxxxxxxxxx")
	 + strlen (" --sysroot=") + strlen (gdb_sysroot) +
	 + (args ? strlen (args) : 0)
	 + 50) /* slack */ ;
  arg_buf = (char *) alloca (len);
  strcpy (arg_buf, "gdbsim");	/* 7 */
  /* Specify the byte order for the target when it is explicitly
     specified by the user (not auto detected).  */
  switch (selected_byte_order ())
    {
    case BFD_ENDIAN_BIG:
      strcat (arg_buf, " -E big");
      break;
    case BFD_ENDIAN_LITTLE:
      strcat (arg_buf, " -E little");
      break;
    case BFD_ENDIAN_UNKNOWN:
      break;
    }
  /* Specify the architecture of the target when it has been
     explicitly specified */
  if (selected_architecture_name () != NULL)
    {
      strcat (arg_buf, " --architecture=");
      strcat (arg_buf, selected_architecture_name ());
    }
  /* Pass along gdb's concept of the sysroot.  */
  strcat (arg_buf, " --sysroot=");
  strcat (arg_buf, gdb_sysroot);
  /* finally, any explicit args */
  if (args)
    {
      strcat (arg_buf, " ");	/* 1 */
      strcat (arg_buf, args);
    }
  sim_argv = gdb_buildargv (arg_buf);

  init_callbacks ();
  gdbsim_desc = sim_open (SIM_OPEN_DEBUG, &gdb_callback, exec_bfd, sim_argv);

  if (gdbsim_desc == 0)
    {
      freeargv (sim_argv);
      sim_argv = NULL;
      error (_("unable to create simulator instance"));
    }

  /* Reset the pid numberings for this batch of sim instances.  */
  next_pid = INITIAL_PID;

  /* Allocate the inferior data, but do not allocate a sim instance
     since we've already just done that.  */
  sim_data = get_sim_inferior_data (current_inferior (),
				    SIM_INSTANCE_NOT_NEEDED);

  sim_data->gdbsim_desc = gdbsim_desc;

  push_target (&gdbsim_ops);
  printf_filtered ("Connected to the simulator.\n");

  /* There's nothing running after "target sim" or "load"; not until
     "run".  */
  inferior_ptid = null_ptid;

  gdbsim_is_open = 1;
}

/* Callback for iterate_over_inferiors.  Called (indirectly) by
   gdbsim_close().  */

static int
gdbsim_close_inferior (struct inferior *inf, void *arg)
{
  struct sim_inferior_data *sim_data = inferior_data (inf,
                                                      sim_inferior_data_key);
  if (sim_data != NULL)
    {
      ptid_t ptid = sim_data->remote_sim_ptid;

      sim_inferior_data_cleanup (inf, sim_data);
      set_inferior_data (inf, sim_inferior_data_key, NULL);

      /* Having a ptid allocated and stored in remote_sim_ptid does
	 not mean that a corresponding inferior was ever created.
	 Thus we need to verify the existence of an inferior using the
	 pid in question before setting inferior_ptid via
	 switch_to_thread() or mourning the inferior.  */
      if (find_inferior_pid (ptid_get_pid (ptid)) != NULL)
	{
	  switch_to_thread (ptid);
	  generic_mourn_inferior ();
	}
    }

  return 0;
}

/* Does whatever cleanup is required for a target that we are no longer
   going to be calling.  Argument says whether we are quitting gdb and
   should not get hung in case of errors, or whether we want a clean
   termination even if it takes a while.  This routine is automatically
   always called just before a routine is popped off the target stack.
   Closing file descriptors and freeing memory are typical things it should
   do.  */
/* Close out all files and local state before this target loses control.  */

static void
gdbsim_close (int quitting)
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NOT_NEEDED);

  if (remote_debug)
    printf_filtered ("gdbsim_close: quitting %d\n", quitting);

  iterate_over_inferiors (gdbsim_close_inferior, NULL);

  if (sim_argv != NULL)
    {
      freeargv (sim_argv);
      sim_argv = NULL;
    }

  end_callbacks ();

  gdbsim_is_open = 0;
}

/* Takes a program previously attached to and detaches it.
   The program may resume execution (some targets do, some don't) and will
   no longer stop on signals, etc.  We better not have left any breakpoints
   in the program or it'll die when it hits one.  ARGS is arguments
   typed by the user (e.g. a signal to send the process).  FROM_TTY
   says whether to be verbose or not.  */
/* Terminate the open connection to the remote debugger.
   Use this when you want to detach and do something else with your gdb.  */

static void
gdbsim_detach (struct target_ops *ops, char *args, int from_tty)
{
  if (remote_debug)
    printf_filtered ("gdbsim_detach: args \"%s\"\n", args);

  pop_target ();		/* calls gdbsim_close to do the real work */
  if (from_tty)
    printf_filtered ("Ending simulator %s debugging\n", target_shortname);
}

/* Resume execution of the target process.  STEP says whether to single-step
   or to run free; SIGGNAL is the signal value (e.g. SIGINT) to be given
   to the target, or zero for no signal.  */

struct resume_data
{
  enum gdb_signal siggnal;
  int step;
};

static int
gdbsim_resume_inferior (struct inferior *inf, void *arg)
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (inf, SIM_INSTANCE_NOT_NEEDED);
  struct resume_data *rd = arg;

  if (sim_data)
    {
      sim_data->resume_siggnal = rd->siggnal;
      sim_data->resume_step = rd->step;

      if (remote_debug)
	printf_filtered (_("gdbsim_resume: pid %d, step %d, signal %d\n"),
	                 inf->pid, rd->step, rd->siggnal);
    }

  /* When called from iterate_over_inferiors, a zero return causes the
     iteration process to proceed until there are no more inferiors to
     consider.  */
  return 0;
}

static void
gdbsim_resume (struct target_ops *ops,
	       ptid_t ptid, int step, enum gdb_signal siggnal)
{
  struct resume_data rd;
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data_by_ptid (ptid, SIM_INSTANCE_NOT_NEEDED);

  rd.siggnal = siggnal;
  rd.step = step;

  /* We don't access any sim_data members within this function.
     What's of interest is whether or not the call to
     get_sim_inferior_data_by_ptid(), above, is able to obtain a
     non-NULL pointer.  If it managed to obtain a non-NULL pointer, we
     know we have a single inferior to consider.  If it's NULL, we
     either have multiple inferiors to resume or an error condition.  */

  if (sim_data)
    gdbsim_resume_inferior (find_inferior_pid (ptid_get_pid (ptid)), &rd);
  else if (ptid_equal (ptid, minus_one_ptid))
    iterate_over_inferiors (gdbsim_resume_inferior, &rd);
  else
    error (_("The program is not being run."));
}

/* Notify the simulator of an asynchronous request to stop.

   The simulator shall ensure that the stop request is eventually
   delivered to the simulator.  If the call is made while the
   simulator is not running then the stop request is processed when
   the simulator is next resumed.

   For simulators that do not support this operation, just abort.  */

static int
gdbsim_stop_inferior (struct inferior *inf, void *arg)
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (inf, SIM_INSTANCE_NEEDED);

  if (sim_data)
    {
      if (!sim_stop (sim_data->gdbsim_desc))
	{
	  quit ();
	}
    }

  /* When called from iterate_over_inferiors, a zero return causes the
     iteration process to proceed until there are no more inferiors to
     consider.  */
  return 0;
}

static void
gdbsim_stop (ptid_t ptid)
{
  struct sim_inferior_data *sim_data;

  if (ptid_equal (ptid, minus_one_ptid))
    {
      iterate_over_inferiors (gdbsim_stop_inferior, NULL);
    }
  else
    {
      struct inferior *inf = find_inferior_pid (ptid_get_pid (ptid));

      if (inf == NULL)
	error (_("Can't stop pid %d.  No inferior found."),
	       ptid_get_pid (ptid));

      gdbsim_stop_inferior (inf, NULL);
    }
}

/* GDB version of os_poll_quit callback.
   Taken from gdb/util.c - should be in a library.  */

static int
gdb_os_poll_quit (host_callback *p)
{
  if (deprecated_ui_loop_hook != NULL)
    deprecated_ui_loop_hook (0);

  if (check_quit_flag ())	/* gdb's idea of quit */
    {
      clear_quit_flag ();	/* we've stolen it */
      return 1;
    }
  return 0;
}

/* Wait for inferior process to do something.  Return pid of child,
   or -1 in case of error; store status through argument pointer STATUS,
   just as `wait' would.  */

static void
gdbsim_cntrl_c (int signo)
{
  gdbsim_stop (minus_one_ptid);
}

static ptid_t
gdbsim_wait (struct target_ops *ops,
	     ptid_t ptid, struct target_waitstatus *status, int options)
{
  struct sim_inferior_data *sim_data;
  static RETSIGTYPE (*prev_sigint) ();
  int sigrc = 0;
  enum sim_stop reason = sim_running;

  /* This target isn't able to (yet) resume more than one inferior at a time.
     When ptid is minus_one_ptid, just use the current inferior.  If we're
     given an explicit pid, we'll try to find it and use that instead.  */
  if (ptid_equal (ptid, minus_one_ptid))
    sim_data = get_sim_inferior_data (current_inferior (),
				      SIM_INSTANCE_NEEDED);
  else
    {
      sim_data = get_sim_inferior_data_by_ptid (ptid, SIM_INSTANCE_NEEDED);
      if (sim_data == NULL)
	error (_("Unable to wait for pid %d.  Inferior not found."),
	       ptid_get_pid (ptid));
      inferior_ptid = ptid;
    }

  if (remote_debug)
    printf_filtered ("gdbsim_wait\n");

#if defined (HAVE_SIGACTION) && defined (SA_RESTART)
  {
    struct sigaction sa, osa;
    sa.sa_handler = gdbsim_cntrl_c;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction (SIGINT, &sa, &osa);
    prev_sigint = osa.sa_handler;
  }
#else
  prev_sigint = signal (SIGINT, gdbsim_cntrl_c);
#endif
  sim_resume (sim_data->gdbsim_desc, sim_data->resume_step,
              sim_data->resume_siggnal);

  signal (SIGINT, prev_sigint);
  sim_data->resume_step = 0;

  sim_stop_reason (sim_data->gdbsim_desc, &reason, &sigrc);

  switch (reason)
    {
    case sim_exited:
      status->kind = TARGET_WAITKIND_EXITED;
      status->value.integer = sigrc;
      break;
    case sim_stopped:
      switch (sigrc)
	{
	case GDB_SIGNAL_ABRT:
	  quit ();
	  break;
	case GDB_SIGNAL_INT:
	case GDB_SIGNAL_TRAP:
	default:
	  status->kind = TARGET_WAITKIND_STOPPED;
	  status->value.sig = sigrc;
	  break;
	}
      break;
    case sim_signalled:
      status->kind = TARGET_WAITKIND_SIGNALLED;
      status->value.sig = sigrc;
      break;
    case sim_running:
    case sim_polling:
      /* FIXME: Is this correct?  */
      break;
    }

  return inferior_ptid;
}

/* Get ready to modify the registers array.  On machines which store
   individual registers, this doesn't need to do anything.  On machines
   which store all the registers in one fell swoop, this makes sure
   that registers contains all the registers from the program being
   debugged.  */

static void
gdbsim_prepare_to_store (struct regcache *regcache)
{
  /* Do nothing, since we can store individual regs.  */
}

/* Transfer LEN bytes between GDB address MYADDR and target address
   MEMADDR.  If WRITE is non-zero, transfer them to the target,
   otherwise transfer them from the target.  TARGET is unused.

   Returns the number of bytes transferred.  */

static int
gdbsim_xfer_inferior_memory (CORE_ADDR memaddr, gdb_byte *myaddr, int len,
			     int write, struct mem_attrib *attrib,
			     struct target_ops *target)
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NOT_NEEDED);

  /* If this target doesn't have memory yet, return 0 causing the
     request to be passed to a lower target, hopefully an exec
     file.  */
  if (!target->to_has_memory (target))
    return 0;

  if (!sim_data->program_loaded)
    error (_("No program loaded."));

  /* Note that we obtained the sim_data pointer above using
     SIM_INSTANCE_NOT_NEEDED.  We do this so that we don't needlessly
     allocate a sim instance prior to loading a program.   If we
     get to this point in the code though, gdbsim_desc should be
     non-NULL.  (Note that a sim instance is needed in order to load
     the program...)  */
  gdb_assert (sim_data->gdbsim_desc != NULL);

  if (remote_debug)
    {
      /* FIXME: Send to something other than STDOUT?  */
      printf_filtered ("gdbsim_xfer_inferior_memory: myaddr 0x");
      gdb_print_host_address (myaddr, gdb_stdout);
      printf_filtered (", memaddr %s, len %d, write %d\n",
		       paddress (target_gdbarch (), memaddr), len, write);
      if (remote_debug && write)
	dump_mem (myaddr, len);
    }

  if (write)
    {
      len = sim_write (sim_data->gdbsim_desc, memaddr, myaddr, len);
    }
  else
    {
      len = sim_read (sim_data->gdbsim_desc, memaddr, myaddr, len);
      if (remote_debug && len > 0)
	dump_mem (myaddr, len);
    }
  return len;
}

static void
gdbsim_files_info (struct target_ops *target)
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NEEDED);
  const char *file = "nothing";

  if (exec_bfd)
    file = bfd_get_filename (exec_bfd);

  if (remote_debug)
    printf_filtered ("gdbsim_files_info: file \"%s\"\n", file);

  if (exec_bfd)
    {
      printf_filtered ("\tAttached to %s running program %s\n",
		       target_shortname, file);
      sim_info (sim_data->gdbsim_desc, 0);
    }
}

/* Clear the simulator's notion of what the break points are.  */

static void
gdbsim_mourn_inferior (struct target_ops *target)
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NOT_NEEDED);

  if (remote_debug)
    printf_filtered ("gdbsim_mourn_inferior:\n");

  remove_breakpoints ();
  generic_mourn_inferior ();
  delete_thread_silent (sim_data->remote_sim_ptid);
}

/* Pass the command argument through to the simulator verbatim.  The
   simulator must do any command interpretation work.  */

void
simulator_command (char *args, int from_tty)
{
  struct sim_inferior_data *sim_data;

  /* We use inferior_data() instead of get_sim_inferior_data() here in
     order to avoid attaching a sim_inferior_data struct to an
     inferior unnecessarily.  The reason we take such care here is due
     to the fact that this function, simulator_command(), may be called
     even when the sim target is not active.  If we were to use
     get_sim_inferior_data() here, it is possible that this call would
     be made either prior to gdbsim_open() or after gdbsim_close(),
     thus allocating memory that would not be garbage collected until
     the ultimate destruction of the associated inferior.  */

  sim_data  = inferior_data (current_inferior (), sim_inferior_data_key);
  if (sim_data == NULL || sim_data->gdbsim_desc == NULL)
    {

      /* PREVIOUSLY: The user may give a command before the simulator
         is opened. [...] (??? assuming of course one wishes to
         continue to allow commands to be sent to unopened simulators,
         which isn't entirely unreasonable).  */

      /* The simulator is a builtin abstraction of a remote target.
         Consistent with that model, access to the simulator, via sim
         commands, is restricted to the period when the channel to the
         simulator is open.  */

      error (_("Not connected to the simulator target"));
    }

  sim_do_command (sim_data->gdbsim_desc, args);

  /* Invalidate the register cache, in case the simulator command does
     something funny.  */
  registers_changed ();
}

static VEC (char_ptr) *
sim_command_completer (struct cmd_list_element *ignore, char *text, char *word)
{
  struct sim_inferior_data *sim_data;
  char **tmp;
  int i;
  VEC (char_ptr) *result = NULL;

  sim_data = inferior_data (current_inferior (), sim_inferior_data_key);
  if (sim_data == NULL || sim_data->gdbsim_desc == NULL)
    return NULL;

  tmp = sim_complete_command (sim_data->gdbsim_desc, text, word);
  if (tmp == NULL)
    return NULL;

  /* Transform the array into a VEC, and then free the array.  */
  for (i = 0; tmp[i] != NULL; i++)
    VEC_safe_push (char_ptr, result, tmp[i]);
  xfree (tmp);

  return result;
}

/* Check to see if a thread is still alive.  */

static int
gdbsim_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data_by_ptid (ptid, SIM_INSTANCE_NOT_NEEDED);

  if (sim_data == NULL)
    return 0;

  if (ptid_equal (ptid, sim_data->remote_sim_ptid))
    /* The simulators' task is always alive.  */
    return 1;

  return 0;
}

/* Convert a thread ID to a string.  Returns the string in a static
   buffer.  */

static char *
gdbsim_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  return normal_pid_to_str (ptid);
}

/* Simulator memory may be accessed after the program has been loaded.  */

static int
gdbsim_has_all_memory (struct target_ops *ops)
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NOT_NEEDED);

  if (!sim_data->program_loaded)
    return 0;

  return 1;
}

static int
gdbsim_has_memory (struct target_ops *ops)
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NOT_NEEDED);

  if (!sim_data->program_loaded)
    return 0;

  return 1;
}

/* Define the target subroutine names.  */

struct target_ops gdbsim_ops;

static void
init_gdbsim_ops (void)
{
  gdbsim_ops.to_shortname = "sim";
  gdbsim_ops.to_longname = "simulator";
  gdbsim_ops.to_doc = "Use the compiled-in simulator.";
  gdbsim_ops.to_open = gdbsim_open;
  gdbsim_ops.to_close = gdbsim_close;
  gdbsim_ops.to_detach = gdbsim_detach;
  gdbsim_ops.to_resume = gdbsim_resume;
  gdbsim_ops.to_wait = gdbsim_wait;
  gdbsim_ops.to_fetch_registers = gdbsim_fetch_register;
  gdbsim_ops.to_store_registers = gdbsim_store_register;
  gdbsim_ops.to_prepare_to_store = gdbsim_prepare_to_store;
  gdbsim_ops.deprecated_xfer_memory = gdbsim_xfer_inferior_memory;
  gdbsim_ops.to_files_info = gdbsim_files_info;
  gdbsim_ops.to_insert_breakpoint = memory_insert_breakpoint;
  gdbsim_ops.to_remove_breakpoint = memory_remove_breakpoint;
  gdbsim_ops.to_kill = gdbsim_kill;
  gdbsim_ops.to_load = gdbsim_load;
  gdbsim_ops.to_create_inferior = gdbsim_create_inferior;
  gdbsim_ops.to_mourn_inferior = gdbsim_mourn_inferior;
  gdbsim_ops.to_stop = gdbsim_stop;
  gdbsim_ops.to_thread_alive = gdbsim_thread_alive;
  gdbsim_ops.to_pid_to_str = gdbsim_pid_to_str;
  gdbsim_ops.to_stratum = process_stratum;
  gdbsim_ops.to_has_all_memory = gdbsim_has_all_memory;
  gdbsim_ops.to_has_memory = gdbsim_has_memory;
  gdbsim_ops.to_has_stack = default_child_has_stack;
  gdbsim_ops.to_has_registers = default_child_has_registers;
  gdbsim_ops.to_has_execution = default_child_has_execution;
  gdbsim_ops.to_magic = OPS_MAGIC;
}

void
_initialize_remote_sim (void)
{
  struct cmd_list_element *c;

  init_gdbsim_ops ();
  add_target (&gdbsim_ops);

  c = add_com ("sim", class_obscure, simulator_command,
	       _("Send a command to the simulator."));
  set_cmd_completer (c, sim_command_completer);

  sim_inferior_data_key
    = register_inferior_data_with_cleanup (NULL, sim_inferior_data_cleanup);
}
