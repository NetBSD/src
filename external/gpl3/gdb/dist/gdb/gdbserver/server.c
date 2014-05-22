/* Main code for remote server for GDB.
   Copyright (C) 1989-2013 Free Software Foundation, Inc.

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

#include "server.h"
#include "gdbthread.h"
#include "agent.h"
#include "notif.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#include "gdb_wait.h"
#include "btrace-common.h"

/* The thread set with an `Hc' packet.  `Hc' is deprecated in favor of
   `vCont'.  Note the multi-process extensions made `vCont' a
   requirement, so `Hc pPID.TID' is pretty much undefined.  So
   CONT_THREAD can be null_ptid for no `Hc' thread, minus_one_ptid for
   resuming all threads of the process (again, `Hc' isn't used for
   multi-process), or a specific thread ptid_t.

   We also set this when handling a single-thread `vCont' resume, as
   some places in the backends check it to know when (and for which
   thread) single-thread scheduler-locking is in effect.  */
ptid_t cont_thread;

/* The thread set with an `Hg' packet.  */
ptid_t general_thread;

int server_waiting;

static int extended_protocol;
static int response_needed;
static int exit_requested;

/* --once: Exit after the first connection has closed.  */
int run_once;

int multi_process;
int non_stop;

/* Whether we should attempt to disable the operating system's address
   space randomization feature before starting an inferior.  */
int disable_randomization = 1;

static char **program_argv, **wrapper_argv;

/* Enable miscellaneous debugging output.  The name is historical - it
   was originally used to debug LinuxThreads support.  */
int debug_threads;

/* Enable debugging of h/w breakpoint/watchpoint support.  */
int debug_hw_points;

int pass_signals[GDB_SIGNAL_LAST];
int program_signals[GDB_SIGNAL_LAST];
int program_signals_p;

jmp_buf toplevel;

const char *gdbserver_xmltarget;

/* The PID of the originally created or attached inferior.  Used to
   send signals to the process when GDB sends us an asynchronous interrupt
   (user hitting Control-C in the client), and to wait for the child to exit
   when no longer debugging it.  */

unsigned long signal_pid;

#ifdef SIGTTOU
/* A file descriptor for the controlling terminal.  */
int terminal_fd;

/* TERMINAL_FD's original foreground group.  */
pid_t old_foreground_pgrp;

/* Hand back terminal ownership to the original foreground group.  */

static void
restore_old_foreground_pgrp (void)
{
  tcsetpgrp (terminal_fd, old_foreground_pgrp);
}
#endif

/* Set if you want to disable optional thread related packets support
   in gdbserver, for the sake of testing GDB against stubs that don't
   support them.  */
int disable_packet_vCont;
int disable_packet_Tthread;
int disable_packet_qC;
int disable_packet_qfThreadInfo;

/* Last status reported to GDB.  */
static struct target_waitstatus last_status;
static ptid_t last_ptid;

static char *own_buf;
static unsigned char *mem_buf;

/* A sub-class of 'struct notif_event' for stop, holding information
   relative to a single stop reply.  We keep a queue of these to
   push to GDB in non-stop mode.  */

struct vstop_notif
{
  struct notif_event base;

  /* Thread or process that got the event.  */
  ptid_t ptid;

  /* Event info.  */
  struct target_waitstatus status;
};

DEFINE_QUEUE_P (notif_event_p);

/* Put a stop reply to the stop reply queue.  */

static void
queue_stop_reply (ptid_t ptid, struct target_waitstatus *status)
{
  struct vstop_notif *new_notif = xmalloc (sizeof (*new_notif));

  new_notif->ptid = ptid;
  new_notif->status = *status;

  notif_event_enque (&notif_stop, (struct notif_event *) new_notif);
}

static int
remove_all_on_match_pid (QUEUE (notif_event_p) *q,
			    QUEUE_ITER (notif_event_p) *iter,
			    struct notif_event *event,
			    void *data)
{
  int *pid = data;

  if (*pid == -1
      || ptid_get_pid (((struct vstop_notif *) event)->ptid) == *pid)
    {
      if (q->free_func != NULL)
	q->free_func (event);

      QUEUE_remove_elem (notif_event_p, q, iter);
    }

  return 1;
}

/* Get rid of the currently pending stop replies for PID.  If PID is
   -1, then apply to all processes.  */

static void
discard_queued_stop_replies (int pid)
{
  QUEUE_iterate (notif_event_p, notif_stop.queue,
		 remove_all_on_match_pid, &pid);
}

static void
vstop_notif_reply (struct notif_event *event, char *own_buf)
{
  struct vstop_notif *vstop = (struct vstop_notif *) event;

  prepare_resume_reply (own_buf, vstop->ptid, &vstop->status);
}

struct notif_server notif_stop =
{
  "vStopped", "Stop", NULL, vstop_notif_reply,
};

static int
target_running (void)
{
  return all_threads.head != NULL;
}

static int
start_inferior (char **argv)
{
  char **new_argv = argv;

  if (wrapper_argv != NULL)
    {
      int i, count = 1;

      for (i = 0; wrapper_argv[i] != NULL; i++)
	count++;
      for (i = 0; argv[i] != NULL; i++)
	count++;
      new_argv = alloca (sizeof (char *) * count);
      count = 0;
      for (i = 0; wrapper_argv[i] != NULL; i++)
	new_argv[count++] = wrapper_argv[i];
      for (i = 0; argv[i] != NULL; i++)
	new_argv[count++] = argv[i];
      new_argv[count] = NULL;
    }

  if (debug_threads)
    {
      int i;
      for (i = 0; new_argv[i]; ++i)
	fprintf (stderr, "new_argv[%d] = \"%s\"\n", i, new_argv[i]);
      fflush (stderr);
    }

#ifdef SIGTTOU
  signal (SIGTTOU, SIG_DFL);
  signal (SIGTTIN, SIG_DFL);
#endif

  /* Clear this so the backend doesn't get confused, thinking
     CONT_THREAD died, and it needs to resume all threads.  */
  cont_thread = null_ptid;

  signal_pid = create_inferior (new_argv[0], new_argv);

  /* FIXME: we don't actually know at this point that the create
     actually succeeded.  We won't know that until we wait.  */
  fprintf (stderr, "Process %s created; pid = %ld\n", argv[0],
	   signal_pid);
  fflush (stderr);

#ifdef SIGTTOU
  signal (SIGTTOU, SIG_IGN);
  signal (SIGTTIN, SIG_IGN);
  terminal_fd = fileno (stderr);
  old_foreground_pgrp = tcgetpgrp (terminal_fd);
  tcsetpgrp (terminal_fd, signal_pid);
  atexit (restore_old_foreground_pgrp);
#endif

  if (wrapper_argv != NULL)
    {
      struct thread_resume resume_info;

      resume_info.thread = pid_to_ptid (signal_pid);
      resume_info.kind = resume_continue;
      resume_info.sig = 0;

      last_ptid = mywait (pid_to_ptid (signal_pid), &last_status, 0, 0);

      if (last_status.kind != TARGET_WAITKIND_STOPPED)
	return signal_pid;

      do
	{
	  (*the_target->resume) (&resume_info, 1);

 	  last_ptid = mywait (pid_to_ptid (signal_pid), &last_status, 0, 0);
	  if (last_status.kind != TARGET_WAITKIND_STOPPED)
	    return signal_pid;

	  current_inferior->last_resume_kind = resume_stop;
	  current_inferior->last_status = last_status;
	}
      while (last_status.value.sig != GDB_SIGNAL_TRAP);

      return signal_pid;
    }

  /* Wait till we are at 1st instruction in program, return new pid
     (assuming success).  */
  last_ptid = mywait (pid_to_ptid (signal_pid), &last_status, 0, 0);

  if (last_status.kind != TARGET_WAITKIND_EXITED
      && last_status.kind != TARGET_WAITKIND_SIGNALLED)
    {
      current_inferior->last_resume_kind = resume_stop;
      current_inferior->last_status = last_status;
    }

  return signal_pid;
}

static int
attach_inferior (int pid)
{
  /* myattach should return -1 if attaching is unsupported,
     0 if it succeeded, and call error() otherwise.  */

  if (myattach (pid) != 0)
    return -1;

  fprintf (stderr, "Attached; pid = %d\n", pid);
  fflush (stderr);

  /* FIXME - It may be that we should get the SIGNAL_PID from the
     attach function, so that it can be the main thread instead of
     whichever we were told to attach to.  */
  signal_pid = pid;

  /* Clear this so the backend doesn't get confused, thinking
     CONT_THREAD died, and it needs to resume all threads.  */
  cont_thread = null_ptid;

  if (!non_stop)
    {
      last_ptid = mywait (pid_to_ptid (pid), &last_status, 0, 0);

      /* GDB knows to ignore the first SIGSTOP after attaching to a running
	 process using the "attach" command, but this is different; it's
	 just using "target remote".  Pretend it's just starting up.  */
      if (last_status.kind == TARGET_WAITKIND_STOPPED
	  && last_status.value.sig == GDB_SIGNAL_STOP)
	last_status.value.sig = GDB_SIGNAL_TRAP;

      current_inferior->last_resume_kind = resume_stop;
      current_inferior->last_status = last_status;
    }

  return 0;
}

extern int remote_debug;

/* Decode a qXfer read request.  Return 0 if everything looks OK,
   or -1 otherwise.  */

static int
decode_xfer_read (char *buf, CORE_ADDR *ofs, unsigned int *len)
{
  /* After the read marker and annex, qXfer looks like a
     traditional 'm' packet.  */
  decode_m_packet (buf, ofs, len);

  return 0;
}

static int
decode_xfer (char *buf, char **object, char **rw, char **annex, char **offset)
{
  /* Extract and NUL-terminate the object.  */
  *object = buf;
  while (*buf && *buf != ':')
    buf++;
  if (*buf == '\0')
    return -1;
  *buf++ = 0;

  /* Extract and NUL-terminate the read/write action.  */
  *rw = buf;
  while (*buf && *buf != ':')
    buf++;
  if (*buf == '\0')
    return -1;
  *buf++ = 0;

  /* Extract and NUL-terminate the annex.  */
  *annex = buf;
  while (*buf && *buf != ':')
    buf++;
  if (*buf == '\0')
    return -1;
  *buf++ = 0;

  *offset = buf;
  return 0;
}

/* Write the response to a successful qXfer read.  Returns the
   length of the (binary) data stored in BUF, corresponding
   to as much of DATA/LEN as we could fit.  IS_MORE controls
   the first character of the response.  */
static int
write_qxfer_response (char *buf, const void *data, int len, int is_more)
{
  int out_len;

  if (is_more)
    buf[0] = 'm';
  else
    buf[0] = 'l';

  return remote_escape_output (data, len, (unsigned char *) buf + 1, &out_len,
			       PBUFSIZ - 2) + 1;
}

/* Handle btrace enabling.  */

static const char *
handle_btrace_enable (struct thread_info *thread)
{
  if (thread->btrace != NULL)
    return "E.Btrace already enabled.";

  thread->btrace = target_enable_btrace (thread->entry.id);
  if (thread->btrace == NULL)
    return "E.Could not enable btrace.";

  return NULL;
}

/* Handle btrace disabling.  */

static const char *
handle_btrace_disable (struct thread_info *thread)
{

  if (thread->btrace == NULL)
    return "E.Branch tracing not enabled.";

  if (target_disable_btrace (thread->btrace) != 0)
    return "E.Could not disable branch tracing.";

  thread->btrace = NULL;
  return NULL;
}

/* Handle the "Qbtrace" packet.  */

static int
handle_btrace_general_set (char *own_buf)
{
  struct thread_info *thread;
  const char *err;
  char *op;

  if (strncmp ("Qbtrace:", own_buf, strlen ("Qbtrace:")) != 0)
    return 0;

  op = own_buf + strlen ("Qbtrace:");

  if (!target_supports_btrace ())
    {
      strcpy (own_buf, "E.Target does not support branch tracing.");
      return -1;
    }

  if (ptid_equal (general_thread, null_ptid)
      || ptid_equal (general_thread, minus_one_ptid))
    {
      strcpy (own_buf, "E.Must select a single thread.");
      return -1;
    }

  thread = find_thread_ptid (general_thread);
  if (thread == NULL)
    {
      strcpy (own_buf, "E.No such thread.");
      return -1;
    }

  err = NULL;

  if (strcmp (op, "bts") == 0)
    err = handle_btrace_enable (thread);
  else if (strcmp (op, "off") == 0)
    err = handle_btrace_disable (thread);
  else
    err = "E.Bad Qbtrace operation. Use bts or off.";

  if (err != 0)
    strcpy (own_buf, err);
  else
    write_ok (own_buf);

  return 1;
}

/* Handle all of the extended 'Q' packets.  */

static void
handle_general_set (char *own_buf)
{
  if (strncmp ("QPassSignals:", own_buf, strlen ("QPassSignals:")) == 0)
    {
      int numsigs = (int) GDB_SIGNAL_LAST, i;
      const char *p = own_buf + strlen ("QPassSignals:");
      CORE_ADDR cursig;

      p = decode_address_to_semicolon (&cursig, p);
      for (i = 0; i < numsigs; i++)
	{
	  if (i == cursig)
	    {
	      pass_signals[i] = 1;
	      if (*p == '\0')
		/* Keep looping, to clear the remaining signals.  */
		cursig = -1;
	      else
		p = decode_address_to_semicolon (&cursig, p);
	    }
	  else
	    pass_signals[i] = 0;
	}
      strcpy (own_buf, "OK");
      return;
    }

  if (strncmp ("QProgramSignals:", own_buf, strlen ("QProgramSignals:")) == 0)
    {
      int numsigs = (int) GDB_SIGNAL_LAST, i;
      const char *p = own_buf + strlen ("QProgramSignals:");
      CORE_ADDR cursig;

      program_signals_p = 1;

      p = decode_address_to_semicolon (&cursig, p);
      for (i = 0; i < numsigs; i++)
	{
	  if (i == cursig)
	    {
	      program_signals[i] = 1;
	      if (*p == '\0')
		/* Keep looping, to clear the remaining signals.  */
		cursig = -1;
	      else
		p = decode_address_to_semicolon (&cursig, p);
	    }
	  else
	    program_signals[i] = 0;
	}
      strcpy (own_buf, "OK");
      return;
    }

  if (strcmp (own_buf, "QStartNoAckMode") == 0)
    {
      if (remote_debug)
	{
	  fprintf (stderr, "[noack mode enabled]\n");
	  fflush (stderr);
	}

      noack_mode = 1;
      write_ok (own_buf);
      return;
    }

  if (strncmp (own_buf, "QNonStop:", 9) == 0)
    {
      char *mode = own_buf + 9;
      int req = -1;
      char *req_str;

      if (strcmp (mode, "0") == 0)
	req = 0;
      else if (strcmp (mode, "1") == 0)
	req = 1;
      else
	{
	  /* We don't know what this mode is, so complain to
	     GDB.  */
	  fprintf (stderr, "Unknown non-stop mode requested: %s\n",
		   own_buf);
	  write_enn (own_buf);
	  return;
	}

      req_str = req ? "non-stop" : "all-stop";
      if (start_non_stop (req) != 0)
	{
	  fprintf (stderr, "Setting %s mode failed\n", req_str);
	  write_enn (own_buf);
	  return;
	}

      non_stop = req;

      if (remote_debug)
	fprintf (stderr, "[%s mode enabled]\n", req_str);

      write_ok (own_buf);
      return;
    }

  if (strncmp ("QDisableRandomization:", own_buf,
	       strlen ("QDisableRandomization:")) == 0)
    {
      char *packet = own_buf + strlen ("QDisableRandomization:");
      ULONGEST setting;

      unpack_varlen_hex (packet, &setting);
      disable_randomization = setting;

      if (remote_debug)
	{
	  if (disable_randomization)
	    fprintf (stderr, "[address space randomization disabled]\n");
	  else
	    fprintf (stderr, "[address space randomization enabled]\n");
	}

      write_ok (own_buf);
      return;
    }

  if (target_supports_tracepoints ()
      && handle_tracepoint_general_set (own_buf))
    return;

  if (strncmp ("QAgent:", own_buf, strlen ("QAgent:")) == 0)
    {
      char *mode = own_buf + strlen ("QAgent:");
      int req = 0;

      if (strcmp (mode, "0") == 0)
	req = 0;
      else if (strcmp (mode, "1") == 0)
	req = 1;
      else
	{
	  /* We don't know what this value is, so complain to GDB.  */
	  sprintf (own_buf, "E.Unknown QAgent value");
	  return;
	}

      /* Update the flag.  */
      use_agent = req;
      if (remote_debug)
	fprintf (stderr, "[%s agent]\n", req ? "Enable" : "Disable");
      write_ok (own_buf);
      return;
    }

  if (handle_btrace_general_set (own_buf))
    return;

  /* Otherwise we didn't know what packet it was.  Say we didn't
     understand it.  */
  own_buf[0] = 0;
}

static const char *
get_features_xml (const char *annex)
{
  /* gdbserver_xmltarget defines what to return when looking
     for the "target.xml" file.  Its contents can either be
     verbatim XML code (prefixed with a '@') or else the name
     of the actual XML file to be used in place of "target.xml".

     This variable is set up from the auto-generated
     init_registers_... routine for the current target.  */

  if (gdbserver_xmltarget
      && strcmp (annex, "target.xml") == 0)
    {
      if (*gdbserver_xmltarget == '@')
	return gdbserver_xmltarget + 1;
      else
	annex = gdbserver_xmltarget;
    }

#ifdef USE_XML
  {
    extern const char *const xml_builtin[][2];
    int i;

    /* Look for the annex.  */
    for (i = 0; xml_builtin[i][0] != NULL; i++)
      if (strcmp (annex, xml_builtin[i][0]) == 0)
	break;

    if (xml_builtin[i][0] != NULL)
      return xml_builtin[i][1];
  }
#endif

  return NULL;
}

void
monitor_show_help (void)
{
  monitor_output ("The following monitor commands are supported:\n");
  monitor_output ("  set debug <0|1>\n");
  monitor_output ("    Enable general debugging messages\n");
  monitor_output ("  set debug-hw-points <0|1>\n");
  monitor_output ("    Enable h/w breakpoint/watchpoint debugging messages\n");
  monitor_output ("  set remote-debug <0|1>\n");
  monitor_output ("    Enable remote protocol debugging messages\n");
  monitor_output ("  exit\n");
  monitor_output ("    Quit GDBserver\n");
}

/* Read trace frame or inferior memory.  Returns the number of bytes
   actually read, zero when no further transfer is possible, and -1 on
   error.  Return of a positive value smaller than LEN does not
   indicate there's no more to be read, only the end of the transfer.
   E.g., when GDB reads memory from a traceframe, a first request may
   be served from a memory block that does not cover the whole request
   length.  A following request gets the rest served from either
   another block (of the same traceframe) or from the read-only
   regions.  */

static int
gdb_read_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len)
{
  int res;

  if (current_traceframe >= 0)
    {
      ULONGEST nbytes;
      ULONGEST length = len;

      if (traceframe_read_mem (current_traceframe,
			       memaddr, myaddr, len, &nbytes))
	return EIO;
      /* Data read from trace buffer, we're done.  */
      if (nbytes > 0)
	return nbytes;
      if (!in_readonly_region (memaddr, length))
	return -1;
      /* Otherwise we have a valid readonly case, fall through.  */
      /* (assume no half-trace half-real blocks for now) */
    }

  res = prepare_to_access_memory ();
  if (res == 0)
    {
      res = read_inferior_memory (memaddr, myaddr, len);
      done_accessing_memory ();

      return res == 0 ? len : -1;
    }
  else
    return -1;
}

/* Write trace frame or inferior memory.  Actually, writing to trace
   frames is forbidden.  */

static int
gdb_write_memory (CORE_ADDR memaddr, const unsigned char *myaddr, int len)
{
  if (current_traceframe >= 0)
    return EIO;
  else
    {
      int ret;

      ret = prepare_to_access_memory ();
      if (ret == 0)
	{
	  ret = write_inferior_memory (memaddr, myaddr, len);
	  done_accessing_memory ();
	}
      return ret;
    }
}

/* Subroutine of handle_search_memory to simplify it.  */

static int
handle_search_memory_1 (CORE_ADDR start_addr, CORE_ADDR search_space_len,
			gdb_byte *pattern, unsigned pattern_len,
			gdb_byte *search_buf,
			unsigned chunk_size, unsigned search_buf_size,
			CORE_ADDR *found_addrp)
{
  /* Prime the search buffer.  */

  if (gdb_read_memory (start_addr, search_buf, search_buf_size)
      != search_buf_size)
    {
      warning ("Unable to access %ld bytes of target "
	       "memory at 0x%lx, halting search.",
	       (long) search_buf_size, (long) start_addr);
      return -1;
    }

  /* Perform the search.

     The loop is kept simple by allocating [N + pattern-length - 1] bytes.
     When we've scanned N bytes we copy the trailing bytes to the start and
     read in another N bytes.  */

  while (search_space_len >= pattern_len)
    {
      gdb_byte *found_ptr;
      unsigned nr_search_bytes = (search_space_len < search_buf_size
				  ? search_space_len
				  : search_buf_size);

      found_ptr = memmem (search_buf, nr_search_bytes, pattern, pattern_len);

      if (found_ptr != NULL)
	{
	  CORE_ADDR found_addr = start_addr + (found_ptr - search_buf);
	  *found_addrp = found_addr;
	  return 1;
	}

      /* Not found in this chunk, skip to next chunk.  */

      /* Don't let search_space_len wrap here, it's unsigned.  */
      if (search_space_len >= chunk_size)
	search_space_len -= chunk_size;
      else
	search_space_len = 0;

      if (search_space_len >= pattern_len)
	{
	  unsigned keep_len = search_buf_size - chunk_size;
	  CORE_ADDR read_addr = start_addr + chunk_size + keep_len;
	  int nr_to_read;

	  /* Copy the trailing part of the previous iteration to the front
	     of the buffer for the next iteration.  */
	  memcpy (search_buf, search_buf + chunk_size, keep_len);

	  nr_to_read = (search_space_len - keep_len < chunk_size
			? search_space_len - keep_len
			: chunk_size);

	  if (gdb_read_memory (read_addr, search_buf + keep_len,
			       nr_to_read) != search_buf_size)
	    {
	      warning ("Unable to access %ld bytes of target memory "
		       "at 0x%lx, halting search.",
		       (long) nr_to_read, (long) read_addr);
	      return -1;
	    }

	  start_addr += chunk_size;
	}
    }

  /* Not found.  */

  return 0;
}

/* Handle qSearch:memory packets.  */

static void
handle_search_memory (char *own_buf, int packet_len)
{
  CORE_ADDR start_addr;
  CORE_ADDR search_space_len;
  gdb_byte *pattern;
  unsigned int pattern_len;
  /* NOTE: also defined in find.c testcase.  */
#define SEARCH_CHUNK_SIZE 16000
  const unsigned chunk_size = SEARCH_CHUNK_SIZE;
  /* Buffer to hold memory contents for searching.  */
  gdb_byte *search_buf;
  unsigned search_buf_size;
  int found;
  CORE_ADDR found_addr;
  int cmd_name_len = sizeof ("qSearch:memory:") - 1;

  pattern = malloc (packet_len);
  if (pattern == NULL)
    {
      error ("Unable to allocate memory to perform the search");
      strcpy (own_buf, "E00");
      return;
    }
  if (decode_search_memory_packet (own_buf + cmd_name_len,
				   packet_len - cmd_name_len,
				   &start_addr, &search_space_len,
				   pattern, &pattern_len) < 0)
    {
      free (pattern);
      error ("Error in parsing qSearch:memory packet");
      strcpy (own_buf, "E00");
      return;
    }

  search_buf_size = chunk_size + pattern_len - 1;

  /* No point in trying to allocate a buffer larger than the search space.  */
  if (search_space_len < search_buf_size)
    search_buf_size = search_space_len;

  search_buf = malloc (search_buf_size);
  if (search_buf == NULL)
    {
      free (pattern);
      error ("Unable to allocate memory to perform the search");
      strcpy (own_buf, "E00");
      return;
    }

  found = handle_search_memory_1 (start_addr, search_space_len,
				  pattern, pattern_len,
				  search_buf, chunk_size, search_buf_size,
				  &found_addr);

  if (found > 0)
    sprintf (own_buf, "1,%lx", (long) found_addr);
  else if (found == 0)
    strcpy (own_buf, "0");
  else
    strcpy (own_buf, "E00");

  free (search_buf);
  free (pattern);
}

#define require_running(BUF)			\
  if (!target_running ())			\
    {						\
      write_enn (BUF);				\
      return;					\
    }

/* Handle monitor commands not handled by target-specific handlers.  */

static void
handle_monitor_command (char *mon, char *own_buf)
{
  if (strcmp (mon, "set debug 1") == 0)
    {
      debug_threads = 1;
      monitor_output ("Debug output enabled.\n");
    }
  else if (strcmp (mon, "set debug 0") == 0)
    {
      debug_threads = 0;
      monitor_output ("Debug output disabled.\n");
    }
  else if (strcmp (mon, "set debug-hw-points 1") == 0)
    {
      debug_hw_points = 1;
      monitor_output ("H/W point debugging output enabled.\n");
    }
  else if (strcmp (mon, "set debug-hw-points 0") == 0)
    {
      debug_hw_points = 0;
      monitor_output ("H/W point debugging output disabled.\n");
    }
  else if (strcmp (mon, "set remote-debug 1") == 0)
    {
      remote_debug = 1;
      monitor_output ("Protocol debug output enabled.\n");
    }
  else if (strcmp (mon, "set remote-debug 0") == 0)
    {
      remote_debug = 0;
      monitor_output ("Protocol debug output disabled.\n");
    }
  else if (strcmp (mon, "help") == 0)
    monitor_show_help ();
  else if (strcmp (mon, "exit") == 0)
    exit_requested = 1;
  else
    {
      monitor_output ("Unknown monitor command.\n\n");
      monitor_show_help ();
      write_enn (own_buf);
    }
}

/* Associates a callback with each supported qXfer'able object.  */

struct qxfer
{
  /* The object this handler handles.  */
  const char *object;

  /* Request that the target transfer up to LEN 8-bit bytes of the
     target's OBJECT.  The OFFSET, for a seekable object, specifies
     the starting point.  The ANNEX can be used to provide additional
     data-specific information to the target.

     Return the number of bytes actually transfered, zero when no
     further transfer is possible, -1 on error, -2 when the transfer
     is not supported, and -3 on a verbose error message that should
     be preserved.  Return of a positive value smaller than LEN does
     not indicate the end of the object, only the end of the transfer.

     One, and only one, of readbuf or writebuf must be non-NULL.  */
  int (*xfer) (const char *annex,
	       gdb_byte *readbuf, const gdb_byte *writebuf,
	       ULONGEST offset, LONGEST len);
};

/* Handle qXfer:auxv:read.  */

static int
handle_qxfer_auxv (const char *annex,
		   gdb_byte *readbuf, const gdb_byte *writebuf,
		   ULONGEST offset, LONGEST len)
{
  if (the_target->read_auxv == NULL || writebuf != NULL)
    return -2;

  if (annex[0] != '\0' || !target_running ())
    return -1;

  return (*the_target->read_auxv) (offset, readbuf, len);
}

/* Handle qXfer:features:read.  */

static int
handle_qxfer_features (const char *annex,
		       gdb_byte *readbuf, const gdb_byte *writebuf,
		       ULONGEST offset, LONGEST len)
{
  const char *document;
  size_t total_len;

  if (writebuf != NULL)
    return -2;

  if (!target_running ())
    return -1;

  /* Grab the correct annex.  */
  document = get_features_xml (annex);
  if (document == NULL)
    return -1;

  total_len = strlen (document);

  if (offset > total_len)
    return -1;

  if (offset + len > total_len)
    len = total_len - offset;

  memcpy (readbuf, document + offset, len);
  return len;
}

/* Handle qXfer:libraries:read.  */

static int
handle_qxfer_libraries (const char *annex,
			gdb_byte *readbuf, const gdb_byte *writebuf,
			ULONGEST offset, LONGEST len)
{
  unsigned int total_len;
  char *document, *p;
  struct inferior_list_entry *dll_ptr;

  if (writebuf != NULL)
    return -2;

  if (annex[0] != '\0' || !target_running ())
    return -1;

  /* Over-estimate the necessary memory.  Assume that every character
     in the library name must be escaped.  */
  total_len = 64;
  for (dll_ptr = all_dlls.head; dll_ptr != NULL; dll_ptr = dll_ptr->next)
    total_len += 128 + 6 * strlen (((struct dll_info *) dll_ptr)->name);

  document = malloc (total_len);
  if (document == NULL)
    return -1;

  strcpy (document, "<library-list>\n");
  p = document + strlen (document);

  for (dll_ptr = all_dlls.head; dll_ptr != NULL; dll_ptr = dll_ptr->next)
    {
      struct dll_info *dll = (struct dll_info *) dll_ptr;
      char *name;

      strcpy (p, "  <library name=\"");
      p = p + strlen (p);
      name = xml_escape_text (dll->name);
      strcpy (p, name);
      free (name);
      p = p + strlen (p);
      strcpy (p, "\"><segment address=\"");
      p = p + strlen (p);
      sprintf (p, "0x%lx", (long) dll->base_addr);
      p = p + strlen (p);
      strcpy (p, "\"/></library>\n");
      p = p + strlen (p);
    }

  strcpy (p, "</library-list>\n");

  total_len = strlen (document);

  if (offset > total_len)
    {
      free (document);
      return -1;
    }

  if (offset + len > total_len)
    len = total_len - offset;

  memcpy (readbuf, document + offset, len);
  free (document);
  return len;
}

/* Handle qXfer:libraries-svr4:read.  */

static int
handle_qxfer_libraries_svr4 (const char *annex,
			     gdb_byte *readbuf, const gdb_byte *writebuf,
			     ULONGEST offset, LONGEST len)
{
  if (writebuf != NULL)
    return -2;

  if (annex[0] != '\0' || !target_running ()
      || the_target->qxfer_libraries_svr4 == NULL)
    return -1;

  return the_target->qxfer_libraries_svr4 (annex, readbuf, writebuf, offset, len);
}

/* Handle qXfer:osadata:read.  */

static int
handle_qxfer_osdata (const char *annex,
		     gdb_byte *readbuf, const gdb_byte *writebuf,
		     ULONGEST offset, LONGEST len)
{
  if (the_target->qxfer_osdata == NULL || writebuf != NULL)
    return -2;

  return (*the_target->qxfer_osdata) (annex, readbuf, NULL, offset, len);
}

/* Handle qXfer:siginfo:read and qXfer:siginfo:write.  */

static int
handle_qxfer_siginfo (const char *annex,
		      gdb_byte *readbuf, const gdb_byte *writebuf,
		      ULONGEST offset, LONGEST len)
{
  if (the_target->qxfer_siginfo == NULL)
    return -2;

  if (annex[0] != '\0' || !target_running ())
    return -1;

  return (*the_target->qxfer_siginfo) (annex, readbuf, writebuf, offset, len);
}

/* Handle qXfer:spu:read and qXfer:spu:write.  */

static int
handle_qxfer_spu (const char *annex,
		  gdb_byte *readbuf, const gdb_byte *writebuf,
		  ULONGEST offset, LONGEST len)
{
  if (the_target->qxfer_spu == NULL)
    return -2;

  if (!target_running ())
    return -1;

  return (*the_target->qxfer_spu) (annex, readbuf, writebuf, offset, len);
}

/* Handle qXfer:statictrace:read.  */

static int
handle_qxfer_statictrace (const char *annex,
			  gdb_byte *readbuf, const gdb_byte *writebuf,
			  ULONGEST offset, LONGEST len)
{
  ULONGEST nbytes;

  if (writebuf != NULL)
    return -2;

  if (annex[0] != '\0' || !target_running () || current_traceframe == -1)
    return -1;

  if (traceframe_read_sdata (current_traceframe, offset,
			     readbuf, len, &nbytes))
    return -1;
  return nbytes;
}

/* Helper for handle_qxfer_threads.  */

static void
handle_qxfer_threads_proper (struct buffer *buffer)
{
  struct inferior_list_entry *thread;

  buffer_grow_str (buffer, "<threads>\n");

  for (thread = all_threads.head; thread; thread = thread->next)
    {
      ptid_t ptid = thread_to_gdb_id ((struct thread_info *)thread);
      char ptid_s[100];
      int core = target_core_of_thread (ptid);
      char core_s[21];

      write_ptid (ptid_s, ptid);

      if (core != -1)
	{
	  sprintf (core_s, "%d", core);
	  buffer_xml_printf (buffer, "<thread id=\"%s\" core=\"%s\"/>\n",
			     ptid_s, core_s);
	}
      else
	{
	  buffer_xml_printf (buffer, "<thread id=\"%s\"/>\n",
			     ptid_s);
	}
    }

  buffer_grow_str0 (buffer, "</threads>\n");
}

/* Handle qXfer:threads:read.  */

static int
handle_qxfer_threads (const char *annex,
		      gdb_byte *readbuf, const gdb_byte *writebuf,
		      ULONGEST offset, LONGEST len)
{
  static char *result = 0;
  static unsigned int result_length = 0;

  if (writebuf != NULL)
    return -2;

  if (!target_running () || annex[0] != '\0')
    return -1;

  if (offset == 0)
    {
      struct buffer buffer;
      /* When asked for data at offset 0, generate everything and store into
	 'result'.  Successive reads will be served off 'result'.  */
      if (result)
	free (result);

      buffer_init (&buffer);

      handle_qxfer_threads_proper (&buffer);

      result = buffer_finish (&buffer);
      result_length = strlen (result);
      buffer_free (&buffer);
    }

  if (offset >= result_length)
    {
      /* We're out of data.  */
      free (result);
      result = NULL;
      result_length = 0;
      return 0;
    }

  if (len > result_length - offset)
    len = result_length - offset;

  memcpy (readbuf, result + offset, len);

  return len;
}

/* Handle qXfer:traceframe-info:read.  */

static int
handle_qxfer_traceframe_info (const char *annex,
			      gdb_byte *readbuf, const gdb_byte *writebuf,
			      ULONGEST offset, LONGEST len)
{
  static char *result = 0;
  static unsigned int result_length = 0;

  if (writebuf != NULL)
    return -2;

  if (!target_running () || annex[0] != '\0' || current_traceframe == -1)
    return -1;

  if (offset == 0)
    {
      struct buffer buffer;

      /* When asked for data at offset 0, generate everything and
	 store into 'result'.  Successive reads will be served off
	 'result'.  */
      free (result);

      buffer_init (&buffer);

      traceframe_read_info (current_traceframe, &buffer);

      result = buffer_finish (&buffer);
      result_length = strlen (result);
      buffer_free (&buffer);
    }

  if (offset >= result_length)
    {
      /* We're out of data.  */
      free (result);
      result = NULL;
      result_length = 0;
      return 0;
    }

  if (len > result_length - offset)
    len = result_length - offset;

  memcpy (readbuf, result + offset, len);
  return len;
}

/* Handle qXfer:fdpic:read.  */

static int
handle_qxfer_fdpic (const char *annex, gdb_byte *readbuf,
		    const gdb_byte *writebuf, ULONGEST offset, LONGEST len)
{
  if (the_target->read_loadmap == NULL)
    return -2;

  if (!target_running ())
    return -1;

  return (*the_target->read_loadmap) (annex, offset, readbuf, len);
}

/* Handle qXfer:btrace:read.  */

static int
handle_qxfer_btrace (const char *annex,
		     gdb_byte *readbuf, const gdb_byte *writebuf,
		     ULONGEST offset, LONGEST len)
{
  static struct buffer cache;
  struct thread_info *thread;
  int type;

  if (the_target->read_btrace == NULL || writebuf != NULL)
    return -2;

  if (!target_running ())
    return -1;

  if (ptid_equal (general_thread, null_ptid)
      || ptid_equal (general_thread, minus_one_ptid))
    {
      strcpy (own_buf, "E.Must select a single thread.");
      return -3;
    }

  thread = find_thread_ptid (general_thread);
  if (thread == NULL)
    {
      strcpy (own_buf, "E.No such thread.");
      return -3;
    }

  if (thread->btrace == NULL)
    {
      strcpy (own_buf, "E.Btrace not enabled.");
      return -3;
    }

  if (strcmp (annex, "all") == 0)
    type = btrace_read_all;
  else if (strcmp (annex, "new") == 0)
    type = btrace_read_new;
  else
    {
      strcpy (own_buf, "E.Bad annex.");
      return -3;
    }

  if (offset == 0)
    {
      buffer_free (&cache);

      target_read_btrace (thread->btrace, &cache, type);
    }
  else if (offset > cache.used_size)
    {
      buffer_free (&cache);
      return -3;
    }

  if (len > cache.used_size - offset)
    len = cache.used_size - offset;

  memcpy (readbuf, cache.buffer + offset, len);

  return len;
}

static const struct qxfer qxfer_packets[] =
  {
    { "auxv", handle_qxfer_auxv },
    { "btrace", handle_qxfer_btrace },
    { "fdpic", handle_qxfer_fdpic},
    { "features", handle_qxfer_features },
    { "libraries", handle_qxfer_libraries },
    { "libraries-svr4", handle_qxfer_libraries_svr4 },
    { "osdata", handle_qxfer_osdata },
    { "siginfo", handle_qxfer_siginfo },
    { "spu", handle_qxfer_spu },
    { "statictrace", handle_qxfer_statictrace },
    { "threads", handle_qxfer_threads },
    { "traceframe-info", handle_qxfer_traceframe_info },
  };

static int
handle_qxfer (char *own_buf, int packet_len, int *new_packet_len_p)
{
  int i;
  char *object;
  char *rw;
  char *annex;
  char *offset;

  if (strncmp (own_buf, "qXfer:", 6) != 0)
    return 0;

  /* Grab the object, r/w and annex.  */
  if (decode_xfer (own_buf + 6, &object, &rw, &annex, &offset) < 0)
    {
      write_enn (own_buf);
      return 1;
    }

  for (i = 0;
       i < sizeof (qxfer_packets) / sizeof (qxfer_packets[0]);
       i++)
    {
      const struct qxfer *q = &qxfer_packets[i];

      if (strcmp (object, q->object) == 0)
	{
	  if (strcmp (rw, "read") == 0)
	    {
	      unsigned char *data;
	      int n;
	      CORE_ADDR ofs;
	      unsigned int len;

	      /* Grab the offset and length.  */
	      if (decode_xfer_read (offset, &ofs, &len) < 0)
		{
		  write_enn (own_buf);
		  return 1;
		}

	      /* Read one extra byte, as an indicator of whether there is
		 more.  */
	      if (len > PBUFSIZ - 2)
		len = PBUFSIZ - 2;
	      data = malloc (len + 1);
	      if (data == NULL)
		{
		  write_enn (own_buf);
		  return 1;
		}
	      n = (*q->xfer) (annex, data, NULL, ofs, len + 1);
	      if (n == -2)
		{
		  free (data);
		  return 0;
		}
	      else if (n == -3)
		{
		  /* Preserve error message.  */
		}
	      else if (n < 0)
		write_enn (own_buf);
	      else if (n > len)
		*new_packet_len_p = write_qxfer_response (own_buf, data, len, 1);
	      else
		*new_packet_len_p = write_qxfer_response (own_buf, data, n, 0);

	      free (data);
	      return 1;
	    }
	  else if (strcmp (rw, "write") == 0)
	    {
	      int n;
	      unsigned int len;
	      CORE_ADDR ofs;
	      unsigned char *data;

	      strcpy (own_buf, "E00");
	      data = malloc (packet_len - (offset - own_buf));
	      if (data == NULL)
		{
		  write_enn (own_buf);
		  return 1;
		}
	      if (decode_xfer_write (offset, packet_len - (offset - own_buf),
				     &ofs, &len, data) < 0)
		{
		  free (data);
		  write_enn (own_buf);
		  return 1;
		}

	      n = (*q->xfer) (annex, NULL, data, ofs, len);
	      if (n == -2)
		{
		  free (data);
		  return 0;
		}
	      else if (n == -3)
		{
		  /* Preserve error message.  */
		}
	      else if (n < 0)
		write_enn (own_buf);
	      else
		sprintf (own_buf, "%x", n);

	      free (data);
	      return 1;
	    }

	  return 0;
	}
    }

  return 0;
}

/* Table used by the crc32 function to calcuate the checksum.  */

static unsigned int crc32_table[256] =
{0, 0};

/* Compute 32 bit CRC from inferior memory.

   On success, return 32 bit CRC.
   On failure, return (unsigned long long) -1.  */

static unsigned long long
crc32 (CORE_ADDR base, int len, unsigned int crc)
{
  if (!crc32_table[1])
    {
      /* Initialize the CRC table and the decoding table.  */
      int i, j;
      unsigned int c;

      for (i = 0; i < 256; i++)
	{
	  for (c = i << 24, j = 8; j > 0; --j)
	    c = c & 0x80000000 ? (c << 1) ^ 0x04c11db7 : (c << 1);
	  crc32_table[i] = c;
	}
    }

  while (len--)
    {
      unsigned char byte = 0;

      /* Return failure if memory read fails.  */
      if (read_inferior_memory (base, &byte, 1) != 0)
	return (unsigned long long) -1;

      crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ byte) & 255];
      base++;
    }
  return (unsigned long long) crc;
}

/* Handle all of the extended 'q' packets.  */

void
handle_query (char *own_buf, int packet_len, int *new_packet_len_p)
{
  static struct inferior_list_entry *thread_ptr;

  /* Reply the current thread id.  */
  if (strcmp ("qC", own_buf) == 0 && !disable_packet_qC)
    {
      ptid_t gdb_id;
      require_running (own_buf);

      if (!ptid_equal (general_thread, null_ptid)
	  && !ptid_equal (general_thread, minus_one_ptid))
	gdb_id = general_thread;
      else
	{
	  thread_ptr = all_threads.head;
	  gdb_id = thread_to_gdb_id ((struct thread_info *)thread_ptr);
	}

      sprintf (own_buf, "QC");
      own_buf += 2;
      write_ptid (own_buf, gdb_id);
      return;
    }

  if (strcmp ("qSymbol::", own_buf) == 0)
    {
      /* GDB is suggesting new symbols have been loaded.  This may
	 mean a new shared library has been detected as loaded, so
	 take the opportunity to check if breakpoints we think are
	 inserted, still are.  Note that it isn't guaranteed that
	 we'll see this when a shared library is loaded, and nor will
	 we see this for unloads (although breakpoints in unloaded
	 libraries shouldn't trigger), as GDB may not find symbols for
	 the library at all.  We also re-validate breakpoints when we
	 see a second GDB breakpoint for the same address, and or when
	 we access breakpoint shadows.  */
      validate_breakpoints ();

      if (target_supports_tracepoints ())
	tracepoint_look_up_symbols ();

      if (target_running () && the_target->look_up_symbols != NULL)
	(*the_target->look_up_symbols) ();

      strcpy (own_buf, "OK");
      return;
    }

  if (!disable_packet_qfThreadInfo)
    {
      if (strcmp ("qfThreadInfo", own_buf) == 0)
	{
	  ptid_t gdb_id;

	  require_running (own_buf);
	  thread_ptr = all_threads.head;

	  *own_buf++ = 'm';
	  gdb_id = thread_to_gdb_id ((struct thread_info *)thread_ptr);
	  write_ptid (own_buf, gdb_id);
	  thread_ptr = thread_ptr->next;
	  return;
	}

      if (strcmp ("qsThreadInfo", own_buf) == 0)
	{
	  ptid_t gdb_id;

	  require_running (own_buf);
	  if (thread_ptr != NULL)
	    {
	      *own_buf++ = 'm';
	      gdb_id = thread_to_gdb_id ((struct thread_info *)thread_ptr);
	      write_ptid (own_buf, gdb_id);
	      thread_ptr = thread_ptr->next;
	      return;
	    }
	  else
	    {
	      sprintf (own_buf, "l");
	      return;
	    }
	}
    }

  if (the_target->read_offsets != NULL
      && strcmp ("qOffsets", own_buf) == 0)
    {
      CORE_ADDR text, data;

      require_running (own_buf);
      if (the_target->read_offsets (&text, &data))
	sprintf (own_buf, "Text=%lX;Data=%lX;Bss=%lX",
		 (long)text, (long)data, (long)data);
      else
	write_enn (own_buf);

      return;
    }

  /* Protocol features query.  */
  if (strncmp ("qSupported", own_buf, 10) == 0
      && (own_buf[10] == ':' || own_buf[10] == '\0'))
    {
      char *p = &own_buf[10];
      int gdb_supports_qRelocInsn = 0;

      /* Start processing qSupported packet.  */
      target_process_qsupported (NULL);

      /* Process each feature being provided by GDB.  The first
	 feature will follow a ':', and latter features will follow
	 ';'.  */
      if (*p == ':')
	{
	  char **qsupported = NULL;
	  int count = 0;
	  int i;

	  /* Two passes, to avoid nested strtok calls in
	     target_process_qsupported.  */
	  for (p = strtok (p + 1, ";");
	       p != NULL;
	       p = strtok (NULL, ";"))
	    {
	      count++;
	      qsupported = xrealloc (qsupported, count * sizeof (char *));
	      qsupported[count - 1] = xstrdup (p);
	    }

	  for (i = 0; i < count; i++)
	    {
	      p = qsupported[i];
	      if (strcmp (p, "multiprocess+") == 0)
		{
		  /* GDB supports and wants multi-process support if
		     possible.  */
		  if (target_supports_multi_process ())
		    multi_process = 1;
		}
	      else if (strcmp (p, "qRelocInsn+") == 0)
		{
		  /* GDB supports relocate instruction requests.  */
		  gdb_supports_qRelocInsn = 1;
		}
	      else
		target_process_qsupported (p);

	      free (p);
	    }

	  free (qsupported);
	}

      sprintf (own_buf,
	       "PacketSize=%x;QPassSignals+;QProgramSignals+",
	       PBUFSIZ - 1);

      if (the_target->qxfer_libraries_svr4 != NULL)
	strcat (own_buf, ";qXfer:libraries-svr4:read+");
      else
	{
	  /* We do not have any hook to indicate whether the non-SVR4 target
	     backend supports qXfer:libraries:read, so always report it.  */
	  strcat (own_buf, ";qXfer:libraries:read+");
	}

      if (the_target->read_auxv != NULL)
	strcat (own_buf, ";qXfer:auxv:read+");

      if (the_target->qxfer_spu != NULL)
	strcat (own_buf, ";qXfer:spu:read+;qXfer:spu:write+");

      if (the_target->qxfer_siginfo != NULL)
	strcat (own_buf, ";qXfer:siginfo:read+;qXfer:siginfo:write+");

      if (the_target->read_loadmap != NULL)
	strcat (own_buf, ";qXfer:fdpic:read+");

      /* We always report qXfer:features:read, as targets may
	 install XML files on a subsequent call to arch_setup.
	 If we reported to GDB on startup that we don't support
	 qXfer:feature:read at all, we will never be re-queried.  */
      strcat (own_buf, ";qXfer:features:read+");

      if (transport_is_reliable)
	strcat (own_buf, ";QStartNoAckMode+");

      if (the_target->qxfer_osdata != NULL)
	strcat (own_buf, ";qXfer:osdata:read+");

      if (target_supports_multi_process ())
	strcat (own_buf, ";multiprocess+");

      if (target_supports_non_stop ())
	strcat (own_buf, ";QNonStop+");

      if (target_supports_disable_randomization ())
	strcat (own_buf, ";QDisableRandomization+");

      strcat (own_buf, ";qXfer:threads:read+");

      if (target_supports_tracepoints ())
	{
	  strcat (own_buf, ";ConditionalTracepoints+");
	  strcat (own_buf, ";TraceStateVariables+");
	  strcat (own_buf, ";TracepointSource+");
	  strcat (own_buf, ";DisconnectedTracing+");
	  if (gdb_supports_qRelocInsn && target_supports_fast_tracepoints ())
	    strcat (own_buf, ";FastTracepoints+");
	  strcat (own_buf, ";StaticTracepoints+");
	  strcat (own_buf, ";InstallInTrace+");
	  strcat (own_buf, ";qXfer:statictrace:read+");
	  strcat (own_buf, ";qXfer:traceframe-info:read+");
	  strcat (own_buf, ";EnableDisableTracepoints+");
	  strcat (own_buf, ";QTBuffer:size+");
	  strcat (own_buf, ";tracenz+");
	}

      /* Support target-side breakpoint conditions and commands.  */
      strcat (own_buf, ";ConditionalBreakpoints+");
      strcat (own_buf, ";BreakpointCommands+");

      if (target_supports_agent ())
	strcat (own_buf, ";QAgent+");

      if (target_supports_btrace ())
	{
	  strcat (own_buf, ";Qbtrace:bts+");
	  strcat (own_buf, ";Qbtrace:off+");
	  strcat (own_buf, ";qXfer:btrace:read+");
	}

      return;
    }

  /* Thread-local storage support.  */
  if (the_target->get_tls_address != NULL
      && strncmp ("qGetTLSAddr:", own_buf, 12) == 0)
    {
      char *p = own_buf + 12;
      CORE_ADDR parts[2], address = 0;
      int i, err;
      ptid_t ptid = null_ptid;

      require_running (own_buf);

      for (i = 0; i < 3; i++)
	{
	  char *p2;
	  int len;

	  if (p == NULL)
	    break;

	  p2 = strchr (p, ',');
	  if (p2)
	    {
	      len = p2 - p;
	      p2++;
	    }
	  else
	    {
	      len = strlen (p);
	      p2 = NULL;
	    }

	  if (i == 0)
	    ptid = read_ptid (p, NULL);
	  else
	    decode_address (&parts[i - 1], p, len);
	  p = p2;
	}

      if (p != NULL || i < 3)
	err = 1;
      else
	{
	  struct thread_info *thread = find_thread_ptid (ptid);

	  if (thread == NULL)
	    err = 2;
	  else
	    err = the_target->get_tls_address (thread, parts[0], parts[1],
					       &address);
	}

      if (err == 0)
	{
	  strcpy (own_buf, paddress(address));
	  return;
	}
      else if (err > 0)
	{
	  write_enn (own_buf);
	  return;
	}

      /* Otherwise, pretend we do not understand this packet.  */
    }

  /* Windows OS Thread Information Block address support.  */
  if (the_target->get_tib_address != NULL
      && strncmp ("qGetTIBAddr:", own_buf, 12) == 0)
    {
      char *annex;
      int n;
      CORE_ADDR tlb;
      ptid_t ptid = read_ptid (own_buf + 12, &annex);

      n = (*the_target->get_tib_address) (ptid, &tlb);
      if (n == 1)
	{
	  strcpy (own_buf, paddress(tlb));
	  return;
	}
      else if (n == 0)
	{
	  write_enn (own_buf);
	  return;
	}
      return;
    }

  /* Handle "monitor" commands.  */
  if (strncmp ("qRcmd,", own_buf, 6) == 0)
    {
      char *mon = malloc (PBUFSIZ);
      int len = strlen (own_buf + 6);

      if (mon == NULL)
	{
	  write_enn (own_buf);
	  return;
	}

      if ((len % 2) != 0 || unhexify (mon, own_buf + 6, len / 2) != len / 2)
	{
	  write_enn (own_buf);
	  free (mon);
	  return;
	}
      mon[len / 2] = '\0';

      write_ok (own_buf);

      if (the_target->handle_monitor_command == NULL
	  || (*the_target->handle_monitor_command) (mon) == 0)
	/* Default processing.  */
	handle_monitor_command (mon, own_buf);

      free (mon);
      return;
    }

  if (strncmp ("qSearch:memory:", own_buf,
	       sizeof ("qSearch:memory:") - 1) == 0)
    {
      require_running (own_buf);
      handle_search_memory (own_buf, packet_len);
      return;
    }

  if (strcmp (own_buf, "qAttached") == 0
      || strncmp (own_buf, "qAttached:", sizeof ("qAttached:") - 1) == 0)
    {
      struct process_info *process;

      if (own_buf[sizeof ("qAttached") - 1])
	{
	  int pid = strtoul (own_buf + sizeof ("qAttached:") - 1, NULL, 16);
	  process = (struct process_info *)
	    find_inferior_id (&all_processes, pid_to_ptid (pid));
	}
      else
	{
	  require_running (own_buf);
	  process = current_process ();
	}

      if (process == NULL)
	{
	  write_enn (own_buf);
	  return;
	}

      strcpy (own_buf, process->attached ? "1" : "0");
      return;
    }

  if (strncmp ("qCRC:", own_buf, 5) == 0)
    {
      /* CRC check (compare-section).  */
      char *comma;
      ULONGEST base;
      int len;
      unsigned long long crc;

      require_running (own_buf);
      comma = unpack_varlen_hex (own_buf + 5, &base);
      if (*comma++ != ',')
	{
	  write_enn (own_buf);
	  return;
	}
      len = strtoul (comma, NULL, 16);
      crc = crc32 (base, len, 0xffffffff);
      /* Check for memory failure.  */
      if (crc == (unsigned long long) -1)
	{
	  write_enn (own_buf);
	  return;
	}
      sprintf (own_buf, "C%lx", (unsigned long) crc);
      return;
    }

  if (handle_qxfer (own_buf, packet_len, new_packet_len_p))
    return;

  if (target_supports_tracepoints () && handle_tracepoint_query (own_buf))
    return;

  /* Otherwise we didn't know what packet it was.  Say we didn't
     understand it.  */
  own_buf[0] = 0;
}

static void gdb_wants_all_threads_stopped (void);

/* Parse vCont packets.  */
void
handle_v_cont (char *own_buf)
{
  char *p, *q;
  int n = 0, i = 0;
  struct thread_resume *resume_info;
  struct thread_resume default_action = {{0}};

  /* Count the number of semicolons in the packet.  There should be one
     for every action.  */
  p = &own_buf[5];
  while (p)
    {
      n++;
      p++;
      p = strchr (p, ';');
    }

  resume_info = malloc (n * sizeof (resume_info[0]));
  if (resume_info == NULL)
    goto err;

  p = &own_buf[5];
  while (*p)
    {
      p++;

      if (p[0] == 's' || p[0] == 'S')
	resume_info[i].kind = resume_step;
      else if (p[0] == 'c' || p[0] == 'C')
	resume_info[i].kind = resume_continue;
      else if (p[0] == 't')
	resume_info[i].kind = resume_stop;
      else
	goto err;

      if (p[0] == 'S' || p[0] == 'C')
	{
	  int sig;
	  sig = strtol (p + 1, &q, 16);
	  if (p == q)
	    goto err;
	  p = q;

	  if (!gdb_signal_to_host_p (sig))
	    goto err;
	  resume_info[i].sig = gdb_signal_to_host (sig);
	}
      else
	{
	  resume_info[i].sig = 0;
	  p = p + 1;
	}

      if (p[0] == 0)
	{
	  resume_info[i].thread = minus_one_ptid;
	  default_action = resume_info[i];

	  /* Note: we don't increment i here, we'll overwrite this entry
	     the next time through.  */
	}
      else if (p[0] == ':')
	{
	  ptid_t ptid = read_ptid (p + 1, &q);

	  if (p == q)
	    goto err;
	  p = q;
	  if (p[0] != ';' && p[0] != 0)
	    goto err;

	  resume_info[i].thread = ptid;

	  i++;
	}
    }

  if (i < n)
    resume_info[i] = default_action;

  /* `cont_thread' is still used in occasional places in the backend,
     to implement single-thread scheduler-locking.  Doesn't make sense
     to set it if we see a stop request, or a wildcard action (one
     with '-1' (all threads), or 'pPID.-1' (all threads of PID)).  */
  if (n == 1
      && !(ptid_equal (resume_info[0].thread, minus_one_ptid)
	   || ptid_get_lwp (resume_info[0].thread) == -1)
      && resume_info[0].kind != resume_stop)
    cont_thread = resume_info[0].thread;
  else
    cont_thread = minus_one_ptid;
  set_desired_inferior (0);

  if (!non_stop)
    enable_async_io ();

  (*the_target->resume) (resume_info, n);

  free (resume_info);

  if (non_stop)
    write_ok (own_buf);
  else
    {
      last_ptid = mywait (minus_one_ptid, &last_status, 0, 1);

      if (last_status.kind != TARGET_WAITKIND_EXITED
          && last_status.kind != TARGET_WAITKIND_SIGNALLED)
	current_inferior->last_status = last_status;

      /* From the client's perspective, all-stop mode always stops all
	 threads implicitly (and the target backend has already done
	 so by now).  Tag all threads as "want-stopped", so we don't
	 resume them implicitly without the client telling us to.  */
      gdb_wants_all_threads_stopped ();
      prepare_resume_reply (own_buf, last_ptid, &last_status);
      disable_async_io ();

      if (last_status.kind == TARGET_WAITKIND_EXITED
          || last_status.kind == TARGET_WAITKIND_SIGNALLED)
        mourn_inferior (find_process_pid (ptid_get_pid (last_ptid)));
    }
  return;

err:
  write_enn (own_buf);
  free (resume_info);
  return;
}

/* Attach to a new program.  Return 1 if successful, 0 if failure.  */
int
handle_v_attach (char *own_buf)
{
  int pid;

  pid = strtol (own_buf + 8, NULL, 16);
  if (pid != 0 && attach_inferior (pid) == 0)
    {
      /* Don't report shared library events after attaching, even if
	 some libraries are preloaded.  GDB will always poll the
	 library list.  Avoids the "stopped by shared library event"
	 notice on the GDB side.  */
      dlls_changed = 0;

      if (non_stop)
	{
	  /* In non-stop, we don't send a resume reply.  Stop events
	     will follow up using the normal notification
	     mechanism.  */
	  write_ok (own_buf);
	}
      else
	prepare_resume_reply (own_buf, last_ptid, &last_status);

      return 1;
    }
  else
    {
      write_enn (own_buf);
      return 0;
    }
}

/* Run a new program.  Return 1 if successful, 0 if failure.  */
static int
handle_v_run (char *own_buf)
{
  char *p, *next_p, **new_argv;
  int i, new_argc;

  new_argc = 0;
  for (p = own_buf + strlen ("vRun;"); p && *p; p = strchr (p, ';'))
    {
      p++;
      new_argc++;
    }

  new_argv = calloc (new_argc + 2, sizeof (char *));
  if (new_argv == NULL)
    {
      write_enn (own_buf);
      return 0;
    }

  i = 0;
  for (p = own_buf + strlen ("vRun;"); *p; p = next_p)
    {
      next_p = strchr (p, ';');
      if (next_p == NULL)
	next_p = p + strlen (p);

      if (i == 0 && p == next_p)
	new_argv[i] = NULL;
      else
	{
	  /* FIXME: Fail request if out of memory instead of dying.  */
	  new_argv[i] = xmalloc (1 + (next_p - p) / 2);
	  unhexify (new_argv[i], p, (next_p - p) / 2);
	  new_argv[i][(next_p - p) / 2] = '\0';
	}

      if (*next_p)
	next_p++;
      i++;
    }
  new_argv[i] = NULL;

  if (new_argv[0] == NULL)
    {
      /* GDB didn't specify a program to run.  Use the program from the
	 last run with the new argument list.  */

      if (program_argv == NULL)
	{
	  write_enn (own_buf);
	  freeargv (new_argv);
	  return 0;
	}

      new_argv[0] = strdup (program_argv[0]);
      if (new_argv[0] == NULL)
	{
	  write_enn (own_buf);
	  freeargv (new_argv);
	  return 0;
	}
    }

  /* Free the old argv and install the new one.  */
  freeargv (program_argv);
  program_argv = new_argv;

  start_inferior (program_argv);
  if (last_status.kind == TARGET_WAITKIND_STOPPED)
    {
      prepare_resume_reply (own_buf, last_ptid, &last_status);

      /* In non-stop, sending a resume reply doesn't set the general
	 thread, but GDB assumes a vRun sets it (this is so GDB can
	 query which is the main thread of the new inferior.  */
      if (non_stop)
	general_thread = last_ptid;

      return 1;
    }
  else
    {
      write_enn (own_buf);
      return 0;
    }
}

/* Kill process.  Return 1 if successful, 0 if failure.  */
int
handle_v_kill (char *own_buf)
{
  int pid;
  char *p = &own_buf[6];
  if (multi_process)
    pid = strtol (p, NULL, 16);
  else
    pid = signal_pid;
  if (pid != 0 && kill_inferior (pid) == 0)
    {
      last_status.kind = TARGET_WAITKIND_SIGNALLED;
      last_status.value.sig = GDB_SIGNAL_KILL;
      last_ptid = pid_to_ptid (pid);
      discard_queued_stop_replies (pid);
      write_ok (own_buf);
      return 1;
    }
  else
    {
      write_enn (own_buf);
      return 0;
    }
}

/* Handle all of the extended 'v' packets.  */
void
handle_v_requests (char *own_buf, int packet_len, int *new_packet_len)
{
  if (!disable_packet_vCont)
    {
      if (strncmp (own_buf, "vCont;", 6) == 0)
	{
	  require_running (own_buf);
	  handle_v_cont (own_buf);
	  return;
	}

      if (strncmp (own_buf, "vCont?", 6) == 0)
	{
	  strcpy (own_buf, "vCont;c;C;s;S;t");
	  return;
	}
    }

  if (strncmp (own_buf, "vFile:", 6) == 0
      && handle_vFile (own_buf, packet_len, new_packet_len))
    return;

  if (strncmp (own_buf, "vAttach;", 8) == 0)
    {
      if ((!extended_protocol || !multi_process) && target_running ())
	{
	  fprintf (stderr, "Already debugging a process\n");
	  write_enn (own_buf);
	  return;
	}
      handle_v_attach (own_buf);
      return;
    }

  if (strncmp (own_buf, "vRun;", 5) == 0)
    {
      if ((!extended_protocol || !multi_process) && target_running ())
	{
	  fprintf (stderr, "Already debugging a process\n");
	  write_enn (own_buf);
	  return;
	}
      handle_v_run (own_buf);
      return;
    }

  if (strncmp (own_buf, "vKill;", 6) == 0)
    {
      if (!target_running ())
	{
	  fprintf (stderr, "No process to kill\n");
	  write_enn (own_buf);
	  return;
	}
      handle_v_kill (own_buf);
      return;
    }

  if (handle_notif_ack (own_buf, packet_len))
    return;

  /* Otherwise we didn't know what packet it was.  Say we didn't
     understand it.  */
  own_buf[0] = 0;
  return;
}

/* Resume inferior and wait for another event.  In non-stop mode,
   don't really wait here, but return immediatelly to the event
   loop.  */
static void
myresume (char *own_buf, int step, int sig)
{
  struct thread_resume resume_info[2];
  int n = 0;
  int valid_cont_thread;

  set_desired_inferior (0);

  valid_cont_thread = (!ptid_equal (cont_thread, null_ptid)
			 && !ptid_equal (cont_thread, minus_one_ptid));

  if (step || sig || valid_cont_thread)
    {
      resume_info[0].thread = current_ptid;
      if (step)
	resume_info[0].kind = resume_step;
      else
	resume_info[0].kind = resume_continue;
      resume_info[0].sig = sig;
      n++;
    }

  if (!valid_cont_thread)
    {
      resume_info[n].thread = minus_one_ptid;
      resume_info[n].kind = resume_continue;
      resume_info[n].sig = 0;
      n++;
    }

  if (!non_stop)
    enable_async_io ();

  (*the_target->resume) (resume_info, n);

  if (non_stop)
    write_ok (own_buf);
  else
    {
      last_ptid = mywait (minus_one_ptid, &last_status, 0, 1);

      if (last_status.kind != TARGET_WAITKIND_EXITED
          && last_status.kind != TARGET_WAITKIND_SIGNALLED)
	{
	  current_inferior->last_resume_kind = resume_stop;
	  current_inferior->last_status = last_status;
	}

      prepare_resume_reply (own_buf, last_ptid, &last_status);
      disable_async_io ();

      if (last_status.kind == TARGET_WAITKIND_EXITED
          || last_status.kind == TARGET_WAITKIND_SIGNALLED)
        mourn_inferior (find_process_pid (ptid_get_pid (last_ptid)));
    }
}

/* Callback for for_each_inferior.  Make a new stop reply for each
   stopped thread.  */

static int
queue_stop_reply_callback (struct inferior_list_entry *entry, void *arg)
{
  struct thread_info *thread = (struct thread_info *) entry;

  /* For now, assume targets that don't have this callback also don't
     manage the thread's last_status field.  */
  if (the_target->thread_stopped == NULL)
    {
      struct vstop_notif *new_notif = xmalloc (sizeof (*new_notif));

      new_notif->ptid = entry->id;
      new_notif->status = thread->last_status;
      /* Pass the last stop reply back to GDB, but don't notify
	 yet.  */
      notif_event_enque (&notif_stop,
			 (struct notif_event *) new_notif);
    }
  else
    {
      if (thread_stopped (thread))
	{
	  if (debug_threads)
	    fprintf (stderr,
		     "Reporting thread %s as already stopped with %s\n",
		     target_pid_to_str (entry->id),
		     target_waitstatus_to_string (&thread->last_status));

	  gdb_assert (thread->last_status.kind != TARGET_WAITKIND_IGNORE);

	  /* Pass the last stop reply back to GDB, but don't notify
	     yet.  */
	  queue_stop_reply (entry->id, &thread->last_status);
	}
    }

  return 0;
}

/* Set this inferior threads's state as "want-stopped".  We won't
   resume this thread until the client gives us another action for
   it.  */

static void
gdb_wants_thread_stopped (struct inferior_list_entry *entry)
{
  struct thread_info *thread = (struct thread_info *) entry;

  thread->last_resume_kind = resume_stop;

  if (thread->last_status.kind == TARGET_WAITKIND_IGNORE)
    {
      /* Most threads are stopped implicitly (all-stop); tag that with
	 signal 0.  */
      thread->last_status.kind = TARGET_WAITKIND_STOPPED;
      thread->last_status.value.sig = GDB_SIGNAL_0;
    }
}

/* Set all threads' states as "want-stopped".  */

static void
gdb_wants_all_threads_stopped (void)
{
  for_each_inferior (&all_threads, gdb_wants_thread_stopped);
}

/* Clear the gdb_detached flag of every process.  */

static void
gdb_reattached_process (struct inferior_list_entry *entry)
{
  struct process_info *process = (struct process_info *) entry;

  process->gdb_detached = 0;
}

/* Status handler for the '?' packet.  */

static void
handle_status (char *own_buf)
{
  /* GDB is connected, don't forward events to the target anymore.  */
  for_each_inferior (&all_processes, gdb_reattached_process);

  /* In non-stop mode, we must send a stop reply for each stopped
     thread.  In all-stop mode, just send one for the first stopped
     thread we find.  */

  if (non_stop)
    {
      discard_queued_stop_replies (-1);
      find_inferior (&all_threads, queue_stop_reply_callback, NULL);

      /* The first is sent immediatly.  OK is sent if there is no
	 stopped thread, which is the same handling of the vStopped
	 packet (by design).  */
      notif_write_event (&notif_stop, own_buf);
    }
  else
    {
      pause_all (0);
      stabilize_threads ();
      gdb_wants_all_threads_stopped ();

      if (all_threads.head)
	{
	  struct target_waitstatus status;

	  status.kind = TARGET_WAITKIND_STOPPED;
	  status.value.sig = GDB_SIGNAL_TRAP;
	  prepare_resume_reply (own_buf,
				all_threads.head->id, &status);
	}
      else
	strcpy (own_buf, "W00");
    }
}

static void
gdbserver_version (void)
{
  printf ("GNU gdbserver %s%s\n"
	  "Copyright (C) 2013 Free Software Foundation, Inc.\n"
	  "gdbserver is free software, covered by the "
	  "GNU General Public License.\n"
	  "This gdbserver was configured as \"%s\"\n",
	  PKGVERSION, version, host_name);
}

static void
gdbserver_usage (FILE *stream)
{
  fprintf (stream, "Usage:\tgdbserver [OPTIONS] COMM PROG [ARGS ...]\n"
	   "\tgdbserver [OPTIONS] --attach COMM PID\n"
	   "\tgdbserver [OPTIONS] --multi COMM\n"
	   "\n"
	   "COMM may either be a tty device (for serial debugging), or \n"
	   "HOST:PORT to listen for a TCP connection.\n"
	   "\n"
	   "Options:\n"
	   "  --debug               Enable general debugging output.\n"
	   "  --remote-debug        Enable remote protocol debugging output.\n"
	   "  --version             Display version information and exit.\n"
	   "  --wrapper WRAPPER --  Run WRAPPER to start new programs.\n"
	   "  --once                Exit after the first connection has "
								  "closed.\n");
  if (REPORT_BUGS_TO[0] && stream == stdout)
    fprintf (stream, "Report bugs to \"%s\".\n", REPORT_BUGS_TO);
}

static void
gdbserver_show_disableable (FILE *stream)
{
  fprintf (stream, "Disableable packets:\n"
	   "  vCont       \tAll vCont packets\n"
	   "  qC          \tQuerying the current thread\n"
	   "  qfThreadInfo\tThread listing\n"
	   "  Tthread     \tPassing the thread specifier in the "
	   "T stop reply packet\n"
	   "  threads     \tAll of the above\n");
}


#undef require_running
#define require_running(BUF)			\
  if (!target_running ())			\
    {						\
      write_enn (BUF);				\
      break;					\
    }

static int
first_thread_of (struct inferior_list_entry *entry, void *args)
{
  int pid = * (int *) args;

  if (ptid_get_pid (entry->id) == pid)
    return 1;

  return 0;
}

static void
kill_inferior_callback (struct inferior_list_entry *entry)
{
  struct process_info *process = (struct process_info *) entry;
  int pid = ptid_get_pid (process->head.id);

  kill_inferior (pid);
  discard_queued_stop_replies (pid);
}

/* Callback for for_each_inferior to detach or kill the inferior,
   depending on whether we attached to it or not.
   We inform the user whether we're detaching or killing the process
   as this is only called when gdbserver is about to exit.  */

static void
detach_or_kill_inferior_callback (struct inferior_list_entry *entry)
{
  struct process_info *process = (struct process_info *) entry;
  int pid = ptid_get_pid (process->head.id);

  if (process->attached)
    detach_inferior (pid);
  else
    kill_inferior (pid);

  discard_queued_stop_replies (pid);
}

/* for_each_inferior callback for detach_or_kill_for_exit to print
   the pids of started inferiors.  */

static void
print_started_pid (struct inferior_list_entry *entry)
{
  struct process_info *process = (struct process_info *) entry;

  if (! process->attached)
    {
      int pid = ptid_get_pid (process->head.id);
      fprintf (stderr, " %d", pid);
    }
}

/* for_each_inferior callback for detach_or_kill_for_exit to print
   the pids of attached inferiors.  */

static void
print_attached_pid (struct inferior_list_entry *entry)
{
  struct process_info *process = (struct process_info *) entry;

  if (process->attached)
    {
      int pid = ptid_get_pid (process->head.id);
      fprintf (stderr, " %d", pid);
    }
}

/* Call this when exiting gdbserver with possible inferiors that need
   to be killed or detached from.  */

static void
detach_or_kill_for_exit (void)
{
  /* First print a list of the inferiors we will be killing/detaching.
     This is to assist the user, for example, in case the inferior unexpectedly
     dies after we exit: did we screw up or did the inferior exit on its own?
     Having this info will save some head-scratching.  */

  if (have_started_inferiors_p ())
    {
      fprintf (stderr, "Killing process(es):");
      for_each_inferior (&all_processes, print_started_pid);
      fprintf (stderr, "\n");
    }
  if (have_attached_inferiors_p ())
    {
      fprintf (stderr, "Detaching process(es):");
      for_each_inferior (&all_processes, print_attached_pid);
      fprintf (stderr, "\n");
    }

  /* Now we can kill or detach the inferiors.  */

  for_each_inferior (&all_processes, detach_or_kill_inferior_callback);
}

int
main (int argc, char *argv[])
{
  int bad_attach;
  int pid;
  char *arg_end, *port;
  char **next_arg = &argv[1];
  volatile int multi_mode = 0;
  volatile int attach = 0;
  int was_running;

  while (*next_arg != NULL && **next_arg == '-')
    {
      if (strcmp (*next_arg, "--version") == 0)
	{
	  gdbserver_version ();
	  exit (0);
	}
      else if (strcmp (*next_arg, "--help") == 0)
	{
	  gdbserver_usage (stdout);
	  exit (0);
	}
      else if (strcmp (*next_arg, "--attach") == 0)
	attach = 1;
      else if (strcmp (*next_arg, "--multi") == 0)
	multi_mode = 1;
      else if (strcmp (*next_arg, "--wrapper") == 0)
	{
	  next_arg++;

	  wrapper_argv = next_arg;
	  while (*next_arg != NULL && strcmp (*next_arg, "--") != 0)
	    next_arg++;

	  if (next_arg == wrapper_argv || *next_arg == NULL)
	    {
	      gdbserver_usage (stderr);
	      exit (1);
	    }

	  /* Consume the "--".  */
	  *next_arg = NULL;
	}
      else if (strcmp (*next_arg, "--debug") == 0)
	debug_threads = 1;
      else if (strcmp (*next_arg, "--remote-debug") == 0)
	remote_debug = 1;
      else if (strcmp (*next_arg, "--disable-packet") == 0)
	{
	  gdbserver_show_disableable (stdout);
	  exit (0);
	}
      else if (strncmp (*next_arg,
			"--disable-packet=",
			sizeof ("--disable-packet=") - 1) == 0)
	{
	  char *packets, *tok;

	  packets = *next_arg += sizeof ("--disable-packet=") - 1;
	  for (tok = strtok (packets, ",");
	       tok != NULL;
	       tok = strtok (NULL, ","))
	    {
	      if (strcmp ("vCont", tok) == 0)
		disable_packet_vCont = 1;
	      else if (strcmp ("Tthread", tok) == 0)
		disable_packet_Tthread = 1;
	      else if (strcmp ("qC", tok) == 0)
		disable_packet_qC = 1;
	      else if (strcmp ("qfThreadInfo", tok) == 0)
		disable_packet_qfThreadInfo = 1;
	      else if (strcmp ("threads", tok) == 0)
		{
		  disable_packet_vCont = 1;
		  disable_packet_Tthread = 1;
		  disable_packet_qC = 1;
		  disable_packet_qfThreadInfo = 1;
		}
	      else
		{
		  fprintf (stderr, "Don't know how to disable \"%s\".\n\n",
			   tok);
		  gdbserver_show_disableable (stderr);
		  exit (1);
		}
	    }
	}
      else if (strcmp (*next_arg, "-") == 0)
	{
	  /* "-" specifies a stdio connection and is a form of port
	     specification.  */
	  *next_arg = STDIO_CONNECTION_NAME;
	  break;
	}
      else if (strcmp (*next_arg, "--disable-randomization") == 0)
	disable_randomization = 1;
      else if (strcmp (*next_arg, "--no-disable-randomization") == 0)
	disable_randomization = 0;
      else if (strcmp (*next_arg, "--once") == 0)
	run_once = 1;
      else
	{
	  fprintf (stderr, "Unknown argument: %s\n", *next_arg);
	  exit (1);
	}

      next_arg++;
      continue;
    }

  if (setjmp (toplevel))
    {
      fprintf (stderr, "Exiting\n");
      exit (1);
    }

  port = *next_arg;
  next_arg++;
  if (port == NULL || (!attach && !multi_mode && *next_arg == NULL))
    {
      gdbserver_usage (stderr);
      exit (1);
    }

  /* We need to know whether the remote connection is stdio before
     starting the inferior.  Inferiors created in this scenario have
     stdin,stdout redirected.  So do this here before we call
     start_inferior.  */
  remote_prepare (port);

  bad_attach = 0;
  pid = 0;

  /* --attach used to come after PORT, so allow it there for
       compatibility.  */
  if (*next_arg != NULL && strcmp (*next_arg, "--attach") == 0)
    {
      attach = 1;
      next_arg++;
    }

  if (attach
      && (*next_arg == NULL
	  || (*next_arg)[0] == '\0'
	  || (pid = strtoul (*next_arg, &arg_end, 0)) == 0
	  || *arg_end != '\0'
	  || next_arg[1] != NULL))
    bad_attach = 1;

  if (bad_attach)
    {
      gdbserver_usage (stderr);
      exit (1);
    }

  initialize_async_io ();
  initialize_low ();
  initialize_event_loop ();
  if (target_supports_tracepoints ())
    initialize_tracepoint ();

  own_buf = xmalloc (PBUFSIZ + 1);
  mem_buf = xmalloc (PBUFSIZ);

  if (pid == 0 && *next_arg != NULL)
    {
      int i, n;

      n = argc - (next_arg - argv);
      program_argv = xmalloc (sizeof (char *) * (n + 1));
      for (i = 0; i < n; i++)
	program_argv[i] = xstrdup (next_arg[i]);
      program_argv[i] = NULL;

      /* Wait till we are at first instruction in program.  */
      start_inferior (program_argv);

      /* We are now (hopefully) stopped at the first instruction of
	 the target process.  This assumes that the target process was
	 successfully created.  */
    }
  else if (pid != 0)
    {
      if (attach_inferior (pid) == -1)
	error ("Attaching not supported on this target");

      /* Otherwise succeeded.  */
    }
  else
    {
      last_status.kind = TARGET_WAITKIND_EXITED;
      last_status.value.integer = 0;
      last_ptid = minus_one_ptid;
    }

  initialize_notif ();

  /* Don't report shared library events on the initial connection,
     even if some libraries are preloaded.  Avoids the "stopped by
     shared library event" notice on gdb side.  */
  dlls_changed = 0;

  if (setjmp (toplevel))
    {
      /* If something fails and longjmps while detaching or killing
	 inferiors, we'd end up here again, stuck in an infinite loop
	 trap.  Be sure that if that happens, we exit immediately
	 instead.  */
      if (setjmp (toplevel) == 0)
	detach_or_kill_for_exit ();
      else
	fprintf (stderr, "Detach or kill failed.  Exiting\n");
      exit (1);
    }

  if (last_status.kind == TARGET_WAITKIND_EXITED
      || last_status.kind == TARGET_WAITKIND_SIGNALLED)
    was_running = 0;
  else
    was_running = 1;

  if (!was_running && !multi_mode)
    {
      fprintf (stderr, "No program to debug.  GDBserver exiting.\n");
      exit (1);
    }

  while (1)
    {
      noack_mode = 0;
      multi_process = 0;
      /* Be sure we're out of tfind mode.  */
      current_traceframe = -1;

      remote_open (port);

      if (setjmp (toplevel) != 0)
	{
	  /* An error occurred.  */
	  if (response_needed)
	    {
	      write_enn (own_buf);
	      putpkt (own_buf);
	    }
	}

      /* Wait for events.  This will return when all event sources are
	 removed from the event loop.  */
      start_event_loop ();

      /* If an exit was requested (using the "monitor exit" command),
	 terminate now.  The only other way to get here is for
	 getpkt to fail; close the connection and reopen it at the
	 top of the loop.  */

      if (exit_requested || run_once)
	{
	  /* If something fails and longjmps while detaching or
	     killing inferiors, we'd end up here again, stuck in an
	     infinite loop trap.  Be sure that if that happens, we
	     exit immediately instead.  */
	  if (setjmp (toplevel) == 0)
	    {
	      detach_or_kill_for_exit ();
	      exit (0);
	    }
	  else
	    {
	      fprintf (stderr, "Detach or kill failed.  Exiting\n");
	      exit (1);
	    }
	}

      fprintf (stderr,
	       "Remote side has terminated connection.  "
	       "GDBserver will reopen the connection.\n");

      if (tracing)
	{
	  if (disconnected_tracing)
	    {
	      /* Try to enable non-stop/async mode, so we we can both
		 wait for an async socket accept, and handle async
		 target events simultaneously.  There's also no point
		 either in having the target always stop all threads,
		 when we're going to pass signals down without
		 informing GDB.  */
	      if (!non_stop)
		{
		  if (start_non_stop (1))
		    non_stop = 1;

		  /* Detaching implicitly resumes all threads; simply
		     disconnecting does not.  */
		}
	    }
	  else
	    {
	      fprintf (stderr,
		       "Disconnected tracing disabled; stopping trace run.\n");
	      stop_tracing ();
	    }
	}
    }
}

/* Process options coming from Z packets for *point at address
   POINT_ADDR.  PACKET is the packet buffer.  *PACKET is updated
   to point to the first char after the last processed option.  */

static void
process_point_options (CORE_ADDR point_addr, char **packet)
{
  char *dataptr = *packet;
  int persist;

  /* Check if data has the correct format.  */
  if (*dataptr != ';')
    return;

  dataptr++;

  while (*dataptr)
    {
      if (*dataptr == ';')
	++dataptr;

      if (*dataptr == 'X')
	{
	  /* Conditional expression.  */
	  if (debug_threads)
	    fprintf (stderr, "Found breakpoint condition.\n");
	  add_breakpoint_condition (point_addr, &dataptr);
	}
      else if (strncmp (dataptr, "cmds:", strlen ("cmds:")) == 0)
	{
	  dataptr += strlen ("cmds:");
	  if (debug_threads)
	    fprintf (stderr, "Found breakpoint commands %s.\n", dataptr);
	  persist = (*dataptr == '1');
	  dataptr += 2;
	  add_breakpoint_commands (point_addr, &dataptr, persist);
	}
      else
	{
	  fprintf (stderr, "Unknown token %c, ignoring.\n",
		   *dataptr);
	  /* Skip tokens until we find one that we recognize.  */
	  while (*dataptr && *dataptr != ';')
	    dataptr++;
	}
    }
  *packet = dataptr;
}

/* Event loop callback that handles a serial event.  The first byte in
   the serial buffer gets us here.  We expect characters to arrive at
   a brisk pace, so we read the rest of the packet with a blocking
   getpkt call.  */

static int
process_serial_event (void)
{
  char ch;
  int i = 0;
  int signal;
  unsigned int len;
  int res;
  CORE_ADDR mem_addr;
  int pid;
  unsigned char sig;
  int packet_len;
  int new_packet_len = -1;

  /* Used to decide when gdbserver should exit in
     multi-mode/remote.  */
  static int have_ran = 0;

  if (!have_ran)
    have_ran = target_running ();

  disable_async_io ();

  response_needed = 0;
  packet_len = getpkt (own_buf);
  if (packet_len <= 0)
    {
      remote_close ();
      /* Force an event loop break.  */
      return -1;
    }
  response_needed = 1;

  i = 0;
  ch = own_buf[i++];
  switch (ch)
    {
    case 'q':
      handle_query (own_buf, packet_len, &new_packet_len);
      break;
    case 'Q':
      handle_general_set (own_buf);
      break;
    case 'D':
      require_running (own_buf);

      if (multi_process)
	{
	  i++; /* skip ';' */
	  pid = strtol (&own_buf[i], NULL, 16);
	}
      else
	pid = ptid_get_pid (current_ptid);

      if ((tracing && disconnected_tracing) || any_persistent_commands ())
	{
	  struct thread_resume resume_info;
	  struct process_info *process = find_process_pid (pid);

	  if (process == NULL)
	    {
	      write_enn (own_buf);
	      break;
	    }

	  if (tracing && disconnected_tracing)
	    fprintf (stderr,
		     "Disconnected tracing in effect, "
		     "leaving gdbserver attached to the process\n");

	  if (any_persistent_commands ())
	    fprintf (stderr,
		     "Persistent commands are present, "
		     "leaving gdbserver attached to the process\n");

	  /* Make sure we're in non-stop/async mode, so we we can both
	     wait for an async socket accept, and handle async target
	     events simultaneously.  There's also no point either in
	     having the target stop all threads, when we're going to
	     pass signals down without informing GDB.  */
	  if (!non_stop)
	    {
	      if (debug_threads)
		fprintf (stderr, "Forcing non-stop mode\n");

	      non_stop = 1;
	      start_non_stop (1);
	    }

	  process->gdb_detached = 1;

	  /* Detaching implicitly resumes all threads.  */
	  resume_info.thread = minus_one_ptid;
	  resume_info.kind = resume_continue;
	  resume_info.sig = 0;
	  (*the_target->resume) (&resume_info, 1);

	  write_ok (own_buf);
	  break; /* from switch/case */
	}

      fprintf (stderr, "Detaching from process %d\n", pid);
      stop_tracing ();
      if (detach_inferior (pid) != 0)
	write_enn (own_buf);
      else
	{
	  discard_queued_stop_replies (pid);
	  write_ok (own_buf);

	  if (extended_protocol)
	    {
	      /* Treat this like a normal program exit.  */
	      last_status.kind = TARGET_WAITKIND_EXITED;
	      last_status.value.integer = 0;
	      last_ptid = pid_to_ptid (pid);

	      current_inferior = NULL;
	    }
	  else
	    {
	      putpkt (own_buf);
	      remote_close ();

	      /* If we are attached, then we can exit.  Otherwise, we
		 need to hang around doing nothing, until the child is
		 gone.  */
	      join_inferior (pid);
	      exit (0);
	    }
	}
      break;
    case '!':
      extended_protocol = 1;
      write_ok (own_buf);
      break;
    case '?':
      handle_status (own_buf);
      break;
    case 'H':
      if (own_buf[1] == 'c' || own_buf[1] == 'g' || own_buf[1] == 's')
	{
	  ptid_t gdb_id, thread_id;
	  int pid;

	  require_running (own_buf);

	  gdb_id = read_ptid (&own_buf[2], NULL);

	  pid = ptid_get_pid (gdb_id);

	  if (ptid_equal (gdb_id, null_ptid)
	      || ptid_equal (gdb_id, minus_one_ptid))
	    thread_id = null_ptid;
	  else if (pid != 0
		   && ptid_equal (pid_to_ptid (pid),
				  gdb_id))
	    {
	      struct thread_info *thread =
		(struct thread_info *) find_inferior (&all_threads,
						      first_thread_of,
						      &pid);
	      if (!thread)
		{
		  write_enn (own_buf);
		  break;
		}

	      thread_id = ((struct inferior_list_entry *)thread)->id;
	    }
	  else
	    {
	      thread_id = gdb_id_to_thread_id (gdb_id);
	      if (ptid_equal (thread_id, null_ptid))
		{
		  write_enn (own_buf);
		  break;
		}
	    }

	  if (own_buf[1] == 'g')
	    {
	      if (ptid_equal (thread_id, null_ptid))
		{
		  /* GDB is telling us to choose any thread.  Check if
		     the currently selected thread is still valid. If
		     it is not, select the first available.  */
		  struct thread_info *thread =
		    (struct thread_info *) find_inferior_id (&all_threads,
							     general_thread);
		  if (thread == NULL)
		    thread_id = all_threads.head->id;
		}

	      general_thread = thread_id;
	      set_desired_inferior (1);
	    }
	  else if (own_buf[1] == 'c')
	    cont_thread = thread_id;

	  write_ok (own_buf);
	}
      else
	{
	  /* Silently ignore it so that gdb can extend the protocol
	     without compatibility headaches.  */
	  own_buf[0] = '\0';
	}
      break;
    case 'g':
      require_running (own_buf);
      if (current_traceframe >= 0)
	{
	  struct regcache *regcache = new_register_cache ();

	  if (fetch_traceframe_registers (current_traceframe,
					  regcache, -1) == 0)
	    registers_to_string (regcache, own_buf);
	  else
	    write_enn (own_buf);
	  free_register_cache (regcache);
	}
      else
	{
	  struct regcache *regcache;

	  set_desired_inferior (1);
	  regcache = get_thread_regcache (current_inferior, 1);
	  registers_to_string (regcache, own_buf);
	}
      break;
    case 'G':
      require_running (own_buf);
      if (current_traceframe >= 0)
	write_enn (own_buf);
      else
	{
	  struct regcache *regcache;

	  set_desired_inferior (1);
	  regcache = get_thread_regcache (current_inferior, 1);
	  registers_from_string (regcache, &own_buf[1]);
	  write_ok (own_buf);
	}
      break;
    case 'm':
      require_running (own_buf);
      decode_m_packet (&own_buf[1], &mem_addr, &len);
      res = gdb_read_memory (mem_addr, mem_buf, len);
      if (res < 0)
	write_enn (own_buf);
      else
	convert_int_to_ascii (mem_buf, own_buf, res);
      break;
    case 'M':
      require_running (own_buf);
      decode_M_packet (&own_buf[1], &mem_addr, &len, &mem_buf);
      if (gdb_write_memory (mem_addr, mem_buf, len) == 0)
	write_ok (own_buf);
      else
	write_enn (own_buf);
      break;
    case 'X':
      require_running (own_buf);
      if (decode_X_packet (&own_buf[1], packet_len - 1,
			   &mem_addr, &len, &mem_buf) < 0
	  || gdb_write_memory (mem_addr, mem_buf, len) != 0)
	write_enn (own_buf);
      else
	write_ok (own_buf);
      break;
    case 'C':
      require_running (own_buf);
      convert_ascii_to_int (own_buf + 1, &sig, 1);
      if (gdb_signal_to_host_p (sig))
	signal = gdb_signal_to_host (sig);
      else
	signal = 0;
      myresume (own_buf, 0, signal);
      break;
    case 'S':
      require_running (own_buf);
      convert_ascii_to_int (own_buf + 1, &sig, 1);
      if (gdb_signal_to_host_p (sig))
	signal = gdb_signal_to_host (sig);
      else
	signal = 0;
      myresume (own_buf, 1, signal);
      break;
    case 'c':
      require_running (own_buf);
      signal = 0;
      myresume (own_buf, 0, signal);
      break;
    case 's':
      require_running (own_buf);
      signal = 0;
      myresume (own_buf, 1, signal);
      break;
    case 'Z':  /* insert_ ... */
      /* Fallthrough.  */
    case 'z':  /* remove_ ... */
      {
	char *dataptr;
	ULONGEST addr;
	int len;
	char type = own_buf[1];
	int res;
	const int insert = ch == 'Z';
	char *p = &own_buf[3];

	p = unpack_varlen_hex (p, &addr);
	len = strtol (p + 1, &dataptr, 16);

	/* Default to unrecognized/unsupported.  */
	res = 1;
	switch (type)
	  {
	  case '0': /* software-breakpoint */
	  case '1': /* hardware-breakpoint */
	  case '2': /* write watchpoint */
	  case '3': /* read watchpoint */
	  case '4': /* access watchpoint */
	    require_running (own_buf);
	    if (insert && the_target->insert_point != NULL)
	      {
		/* Insert the breakpoint.  If it is already inserted, nothing
		   will take place.  */
		res = (*the_target->insert_point) (type, addr, len);

		/* GDB may have sent us a list of *point parameters to be
		   evaluated on the target's side.  Read such list here.  If we
		   already have a list of parameters, GDB is telling us to drop
		   that list and use this one instead.  */
		if (!res && (type == '0' || type == '1'))
		  {
		    /* Remove previous conditions.  */
		    clear_gdb_breakpoint_conditions (addr);
		    process_point_options (addr, &dataptr);
		  }
	      }
	    else if (!insert && the_target->remove_point != NULL)
	      res = (*the_target->remove_point) (type, addr, len);
	    break;
	  default:
	    break;
	  }

	if (res == 0)
	  write_ok (own_buf);
	else if (res == 1)
	  /* Unsupported.  */
	  own_buf[0] = '\0';
	else
	  write_enn (own_buf);
	break;
      }
    case 'k':
      response_needed = 0;
      if (!target_running ())
	/* The packet we received doesn't make sense - but we can't
	   reply to it, either.  */
	return 0;

      fprintf (stderr, "Killing all inferiors\n");
      for_each_inferior (&all_processes, kill_inferior_callback);

      /* When using the extended protocol, we wait with no program
	 running.  The traditional protocol will exit instead.  */
      if (extended_protocol)
	{
	  last_status.kind = TARGET_WAITKIND_EXITED;
	  last_status.value.sig = GDB_SIGNAL_KILL;
	  return 0;
	}
      else
	exit (0);

    case 'T':
      {
	ptid_t gdb_id, thread_id;

	require_running (own_buf);

	gdb_id = read_ptid (&own_buf[1], NULL);
	thread_id = gdb_id_to_thread_id (gdb_id);
	if (ptid_equal (thread_id, null_ptid))
	  {
	    write_enn (own_buf);
	    break;
	  }

	if (mythread_alive (thread_id))
	  write_ok (own_buf);
	else
	  write_enn (own_buf);
      }
      break;
    case 'R':
      response_needed = 0;

      /* Restarting the inferior is only supported in the extended
	 protocol.  */
      if (extended_protocol)
	{
	  if (target_running ())
	    for_each_inferior (&all_processes,
			       kill_inferior_callback);
	  fprintf (stderr, "GDBserver restarting\n");

	  /* Wait till we are at 1st instruction in prog.  */
	  if (program_argv != NULL)
	    start_inferior (program_argv);
	  else
	    {
	      last_status.kind = TARGET_WAITKIND_EXITED;
	      last_status.value.sig = GDB_SIGNAL_KILL;
	    }
	  return 0;
	}
      else
	{
	  /* It is a request we don't understand.  Respond with an
	     empty packet so that gdb knows that we don't support this
	     request.  */
	  own_buf[0] = '\0';
	  break;
	}
    case 'v':
      /* Extended (long) request.  */
      handle_v_requests (own_buf, packet_len, &new_packet_len);
      break;

    default:
      /* It is a request we don't understand.  Respond with an empty
	 packet so that gdb knows that we don't support this
	 request.  */
      own_buf[0] = '\0';
      break;
    }

  if (new_packet_len != -1)
    putpkt_binary (own_buf, new_packet_len);
  else
    putpkt (own_buf);

  response_needed = 0;

  if (!extended_protocol && have_ran && !target_running ())
    {
      /* In non-stop, defer exiting until GDB had a chance to query
	 the whole vStopped list (until it gets an OK).  */
      if (QUEUE_is_empty (notif_event_p, notif_stop.queue))
	{
	  fprintf (stderr, "GDBserver exiting\n");
	  remote_close ();
	  exit (0);
	}
    }

  if (exit_requested)
    return -1;

  return 0;
}

/* Event-loop callback for serial events.  */

int
handle_serial_event (int err, gdb_client_data client_data)
{
  if (debug_threads)
    fprintf (stderr, "handling possible serial event\n");

  /* Really handle it.  */
  if (process_serial_event () < 0)
    return -1;

  /* Be sure to not change the selected inferior behind GDB's back.
     Important in the non-stop mode asynchronous protocol.  */
  set_desired_inferior (1);

  return 0;
}

/* Event-loop callback for target events.  */

int
handle_target_event (int err, gdb_client_data client_data)
{
  if (debug_threads)
    fprintf (stderr, "handling possible target event\n");

  last_ptid = mywait (minus_one_ptid, &last_status,
		      TARGET_WNOHANG, 1);

  if (last_status.kind != TARGET_WAITKIND_IGNORE)
    {
      int pid = ptid_get_pid (last_ptid);
      struct process_info *process = find_process_pid (pid);
      int forward_event = !gdb_connected () || process->gdb_detached;

      if (last_status.kind == TARGET_WAITKIND_EXITED
	  || last_status.kind == TARGET_WAITKIND_SIGNALLED)
	{
	  mark_breakpoints_out (process);
	  mourn_inferior (process);
	}
      else
	{
	  /* We're reporting this thread as stopped.  Update its
	     "want-stopped" state to what the client wants, until it
	     gets a new resume action.  */
	  current_inferior->last_resume_kind = resume_stop;
	  current_inferior->last_status = last_status;
	}

      if (forward_event)
	{
	  if (!target_running ())
	    {
	      /* The last process exited.  We're done.  */
	      exit (0);
	    }

	  if (last_status.kind == TARGET_WAITKIND_STOPPED)
	    {
	      /* A thread stopped with a signal, but gdb isn't
		 connected to handle it.  Pass it down to the
		 inferior, as if it wasn't being traced.  */
	      struct thread_resume resume_info;

	      if (debug_threads)
		fprintf (stderr,
			 "GDB not connected; forwarding event %d for [%s]\n",
			 (int) last_status.kind,
			 target_pid_to_str (last_ptid));

	      resume_info.thread = last_ptid;
	      resume_info.kind = resume_continue;
	      resume_info.sig = gdb_signal_to_host (last_status.value.sig);
	      (*the_target->resume) (&resume_info, 1);
	    }
	  else if (debug_threads)
	    fprintf (stderr, "GDB not connected; ignoring event %d for [%s]\n",
		     (int) last_status.kind,
		     target_pid_to_str (last_ptid));
	}
      else
	{
	  struct vstop_notif *vstop_notif
	    = xmalloc (sizeof (struct vstop_notif));

	  vstop_notif->status = last_status;
	  vstop_notif->ptid = last_ptid;
	  /* Push Stop notification.  */
	  notif_push (&notif_stop,
		      (struct notif_event *) vstop_notif);
	}
    }

  /* Be sure to not change the selected inferior behind GDB's back.
     Important in the non-stop mode asynchronous protocol.  */
  set_desired_inferior (1);

  return 0;
}
