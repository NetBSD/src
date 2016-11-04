/* Remote target communications for serial-line targets in custom GDB protocol

   Copyright (C) 1988-2016 Free Software Foundation, Inc.

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

/* See the GDB User Guide for details of the GDB remote protocol.  */

#include "defs.h"
#include <ctype.h>
#include <fcntl.h>
#include "inferior.h"
#include "infrun.h"
#include "bfd.h"
#include "symfile.h"
#include "target.h"
/*#include "terminal.h" */
#include "gdbcmd.h"
#include "objfiles.h"
#include "gdb-stabs.h"
#include "gdbthread.h"
#include "remote.h"
#include "remote-notif.h"
#include "regcache.h"
#include "value.h"
#include "observer.h"
#include "solib.h"
#include "cli/cli-decode.h"
#include "cli/cli-setshow.h"
#include "target-descriptions.h"
#include "gdb_bfd.h"
#include "filestuff.h"
#include "rsp-low.h"
#include "disasm.h"
#include "location.h"

#include "gdb_sys_time.h"

#include "event-loop.h"
#include "event-top.h"
#include "inf-loop.h"

#include <signal.h>
#include "serial.h"

#include "gdbcore.h" /* for exec_bfd */

#include "remote-fileio.h"
#include "gdb/fileio.h"
#include <sys/stat.h>
#include "xml-support.h"

#include "memory-map.h"

#include "tracepoint.h"
#include "ax.h"
#include "ax-gdb.h"
#include "agent.h"
#include "btrace.h"
#include "record-btrace.h"

/* Temp hacks for tracepoint encoding migration.  */
static char *target_buf;
static long target_buf_size;

/* Per-program-space data key.  */
static const struct program_space_data *remote_pspace_data;

/* The variable registered as the control variable used by the
   remote exec-file commands.  While the remote exec-file setting is
   per-program-space, the set/show machinery uses this as the 
   location of the remote exec-file value.  */
static char *remote_exec_file_var;

/* The size to align memory write packets, when practical.  The protocol
   does not guarantee any alignment, and gdb will generate short
   writes and unaligned writes, but even as a best-effort attempt this
   can improve bulk transfers.  For instance, if a write is misaligned
   relative to the target's data bus, the stub may need to make an extra
   round trip fetching data from the target.  This doesn't make a
   huge difference, but it's easy to do, so we try to be helpful.

   The alignment chosen is arbitrary; usually data bus width is
   important here, not the possibly larger cache line size.  */
enum { REMOTE_ALIGN_WRITES = 16 };

/* Prototypes for local functions.  */
static int getpkt_sane (char **buf, long *sizeof_buf, int forever);
static int getpkt_or_notif_sane (char **buf, long *sizeof_buf,
				 int forever, int *is_notif);

static void remote_files_info (struct target_ops *ignore);

static void remote_prepare_to_store (struct target_ops *self,
				     struct regcache *regcache);

static void remote_open_1 (const char *, int, struct target_ops *,
			   int extended_p);

static void remote_close (struct target_ops *self);

struct remote_state;

static int remote_vkill (int pid, struct remote_state *rs);

static void remote_kill_k (void);

static void remote_mourn (struct target_ops *ops);

static void extended_remote_restart (void);

static void remote_send (char **buf, long *sizeof_buf_p);

static int readchar (int timeout);

static void remote_serial_write (const char *str, int len);

static void remote_kill (struct target_ops *ops);

static int remote_can_async_p (struct target_ops *);

static int remote_is_async_p (struct target_ops *);

static void remote_async (struct target_ops *ops, int enable);

static void remote_thread_events (struct target_ops *ops, int enable);

static void interrupt_query (void);

static void set_general_thread (struct ptid ptid);
static void set_continue_thread (struct ptid ptid);

static void get_offsets (void);

static void skip_frame (void);

static long read_frame (char **buf_p, long *sizeof_buf);

static int hexnumlen (ULONGEST num);

static void init_remote_ops (void);

static void init_extended_remote_ops (void);

static void remote_stop (struct target_ops *self, ptid_t);

static int stubhex (int ch);

static int hexnumstr (char *, ULONGEST);

static int hexnumnstr (char *, ULONGEST, int);

static CORE_ADDR remote_address_masked (CORE_ADDR);

static void print_packet (const char *);

static void compare_sections_command (char *, int);

static void packet_command (char *, int);

static int stub_unpack_int (char *buff, int fieldlength);

static ptid_t remote_current_thread (ptid_t oldptid);

static int putpkt_binary (const char *buf, int cnt);

static void check_binary_download (CORE_ADDR addr);

struct packet_config;

static void show_packet_config_cmd (struct packet_config *config);

static void show_remote_protocol_packet_cmd (struct ui_file *file,
					     int from_tty,
					     struct cmd_list_element *c,
					     const char *value);

static char *write_ptid (char *buf, const char *endbuf, ptid_t ptid);
static ptid_t read_ptid (char *buf, char **obuf);

static void remote_set_permissions (struct target_ops *self);

static int remote_get_trace_status (struct target_ops *self,
				    struct trace_status *ts);

static int remote_upload_tracepoints (struct target_ops *self,
				      struct uploaded_tp **utpp);

static int remote_upload_trace_state_variables (struct target_ops *self,
						struct uploaded_tsv **utsvp);
  
static void remote_query_supported (void);

static void remote_check_symbols (void);

void _initialize_remote (void);

struct stop_reply;
static void stop_reply_xfree (struct stop_reply *);
static void remote_parse_stop_reply (char *, struct stop_reply *);
static void push_stop_reply (struct stop_reply *);
static void discard_pending_stop_replies_in_queue (struct remote_state *);
static int peek_stop_reply (ptid_t ptid);

struct threads_listing_context;
static void remove_new_fork_children (struct threads_listing_context *);

static void remote_async_inferior_event_handler (gdb_client_data);

static void remote_terminal_ours (struct target_ops *self);

static int remote_read_description_p (struct target_ops *target);

static void remote_console_output (char *msg);

static int remote_supports_cond_breakpoints (struct target_ops *self);

static int remote_can_run_breakpoint_commands (struct target_ops *self);

static void remote_btrace_reset (void);

static void remote_btrace_maybe_reopen (void);

static int stop_reply_queue_length (void);

static void readahead_cache_invalidate (void);

static void remote_unpush_and_throw (void);

/* For "remote".  */

static struct cmd_list_element *remote_cmdlist;

/* For "set remote" and "show remote".  */

static struct cmd_list_element *remote_set_cmdlist;
static struct cmd_list_element *remote_show_cmdlist;

/* Stub vCont actions support.

   Each field is a boolean flag indicating whether the stub reports
   support for the corresponding action.  */

struct vCont_action_support
{
  /* vCont;t */
  int t;

  /* vCont;r */
  int r;

  /* vCont;s */
  int s;

  /* vCont;S */
  int S;
};

/* Controls whether GDB is willing to use range stepping.  */

static int use_range_stepping = 1;

#define OPAQUETHREADBYTES 8

/* a 64 bit opaque identifier */
typedef unsigned char threadref[OPAQUETHREADBYTES];

/* About this many threadisds fit in a packet.  */

#define MAXTHREADLISTRESULTS 32

/* Data for the vFile:pread readahead cache.  */

struct readahead_cache
{
  /* The file descriptor for the file that is being cached.  -1 if the
     cache is invalid.  */
  int fd;

  /* The offset into the file that the cache buffer corresponds
     to.  */
  ULONGEST offset;

  /* The buffer holding the cache contents.  */
  gdb_byte *buf;
  /* The buffer's size.  We try to read as much as fits into a packet
     at a time.  */
  size_t bufsize;

  /* Cache hit and miss counters.  */
  ULONGEST hit_count;
  ULONGEST miss_count;
};

/* Description of the remote protocol state for the currently
   connected target.  This is per-target state, and independent of the
   selected architecture.  */

struct remote_state
{
  /* A buffer to use for incoming packets, and its current size.  The
     buffer is grown dynamically for larger incoming packets.
     Outgoing packets may also be constructed in this buffer.
     BUF_SIZE is always at least REMOTE_PACKET_SIZE;
     REMOTE_PACKET_SIZE should be used to limit the length of outgoing
     packets.  */
  char *buf;
  long buf_size;

  /* True if we're going through initial connection setup (finding out
     about the remote side's threads, relocating symbols, etc.).  */
  int starting_up;

  /* If we negotiated packet size explicitly (and thus can bypass
     heuristics for the largest packet size that will not overflow
     a buffer in the stub), this will be set to that packet size.
     Otherwise zero, meaning to use the guessed size.  */
  long explicit_packet_size;

  /* remote_wait is normally called when the target is running and
     waits for a stop reply packet.  But sometimes we need to call it
     when the target is already stopped.  We can send a "?" packet
     and have remote_wait read the response.  Or, if we already have
     the response, we can stash it in BUF and tell remote_wait to
     skip calling getpkt.  This flag is set when BUF contains a
     stop reply packet and the target is not waiting.  */
  int cached_wait_status;

  /* True, if in no ack mode.  That is, neither GDB nor the stub will
     expect acks from each other.  The connection is assumed to be
     reliable.  */
  int noack_mode;

  /* True if we're connected in extended remote mode.  */
  int extended;

  /* True if we resumed the target and we're waiting for the target to
     stop.  In the mean time, we can't start another command/query.
     The remote server wouldn't be ready to process it, so we'd
     timeout waiting for a reply that would never come and eventually
     we'd close the connection.  This can happen in asynchronous mode
     because we allow GDB commands while the target is running.  */
  int waiting_for_stop_reply;

  /* The status of the stub support for the various vCont actions.  */
  struct vCont_action_support supports_vCont;

  /* Nonzero if the user has pressed Ctrl-C, but the target hasn't
     responded to that.  */
  int ctrlc_pending_p;

  /* True if we saw a Ctrl-C while reading or writing from/to the
     remote descriptor.  At that point it is not safe to send a remote
     interrupt packet, so we instead remember we saw the Ctrl-C and
     process it once we're done with sending/receiving the current
     packet, which should be shortly.  If however that takes too long,
     and the user presses Ctrl-C again, we offer to disconnect.  */
  int got_ctrlc_during_io;

  /* Descriptor for I/O to remote machine.  Initialize it to NULL so that
     remote_open knows that we don't have a file open when the program
     starts.  */
  struct serial *remote_desc;

  /* These are the threads which we last sent to the remote system.  The
     TID member will be -1 for all or -2 for not sent yet.  */
  ptid_t general_thread;
  ptid_t continue_thread;

  /* This is the traceframe which we last selected on the remote system.
     It will be -1 if no traceframe is selected.  */
  int remote_traceframe_number;

  char *last_pass_packet;

  /* The last QProgramSignals packet sent to the target.  We bypass
     sending a new program signals list down to the target if the new
     packet is exactly the same as the last we sent.  IOW, we only let
     the target know about program signals list changes.  */
  char *last_program_signals_packet;

  enum gdb_signal last_sent_signal;

  int last_sent_step;

  /* The execution direction of the last resume we got.  */
  enum exec_direction_kind last_resume_exec_dir;

  char *finished_object;
  char *finished_annex;
  ULONGEST finished_offset;

  /* Should we try the 'ThreadInfo' query packet?

     This variable (NOT available to the user: auto-detect only!)
     determines whether GDB will use the new, simpler "ThreadInfo"
     query or the older, more complex syntax for thread queries.
     This is an auto-detect variable (set to true at each connect,
     and set to false when the target fails to recognize it).  */
  int use_threadinfo_query;
  int use_threadextra_query;

  threadref echo_nextthread;
  threadref nextthread;
  threadref resultthreadlist[MAXTHREADLISTRESULTS];

  /* The state of remote notification.  */
  struct remote_notif_state *notif_state;

  /* The branch trace configuration.  */
  struct btrace_config btrace_config;

  /* The argument to the last "vFile:setfs:" packet we sent, used
     to avoid sending repeated unnecessary "vFile:setfs:" packets.
     Initialized to -1 to indicate that no "vFile:setfs:" packet
     has yet been sent.  */
  int fs_pid;

  /* A readahead cache for vFile:pread.  Often, reading a binary
     involves a sequence of small reads.  E.g., when parsing an ELF
     file.  A readahead cache helps mostly the case of remote
     debugging on a connection with higher latency, due to the
     request/reply nature of the RSP.  We only cache data for a single
     file descriptor at a time.  */
  struct readahead_cache readahead_cache;
};

/* Private data that we'll store in (struct thread_info)->private.  */
struct private_thread_info
{
  char *extra;
  char *name;
  int core;

  /* Whether the target stopped for a breakpoint/watchpoint.  */
  enum target_stop_reason stop_reason;

  /* This is set to the data address of the access causing the target
     to stop for a watchpoint.  */
  CORE_ADDR watch_data_address;
};

static void
free_private_thread_info (struct private_thread_info *info)
{
  xfree (info->extra);
  xfree (info->name);
  xfree (info);
}

/* This data could be associated with a target, but we do not always
   have access to the current target when we need it, so for now it is
   static.  This will be fine for as long as only one target is in use
   at a time.  */
static struct remote_state *remote_state;

static struct remote_state *
get_remote_state_raw (void)
{
  return remote_state;
}

/* Allocate a new struct remote_state with xmalloc, initialize it, and
   return it.  */

static struct remote_state *
new_remote_state (void)
{
  struct remote_state *result = XCNEW (struct remote_state);

  /* The default buffer size is unimportant; it will be expanded
     whenever a larger buffer is needed. */
  result->buf_size = 400;
  result->buf = (char *) xmalloc (result->buf_size);
  result->remote_traceframe_number = -1;
  result->last_sent_signal = GDB_SIGNAL_0;
  result->last_resume_exec_dir = EXEC_FORWARD;
  result->fs_pid = -1;

  return result;
}

/* Description of the remote protocol for a given architecture.  */

struct packet_reg
{
  long offset; /* Offset into G packet.  */
  long regnum; /* GDB's internal register number.  */
  LONGEST pnum; /* Remote protocol register number.  */
  int in_g_packet; /* Always part of G packet.  */
  /* long size in bytes;  == register_size (target_gdbarch (), regnum);
     at present.  */
  /* char *name; == gdbarch_register_name (target_gdbarch (), regnum);
     at present.  */
};

struct remote_arch_state
{
  /* Description of the remote protocol registers.  */
  long sizeof_g_packet;

  /* Description of the remote protocol registers indexed by REGNUM
     (making an array gdbarch_num_regs in size).  */
  struct packet_reg *regs;

  /* This is the size (in chars) of the first response to the ``g''
     packet.  It is used as a heuristic when determining the maximum
     size of memory-read and memory-write packets.  A target will
     typically only reserve a buffer large enough to hold the ``g''
     packet.  The size does not include packet overhead (headers and
     trailers).  */
  long actual_register_packet_size;

  /* This is the maximum size (in chars) of a non read/write packet.
     It is also used as a cap on the size of read/write packets.  */
  long remote_packet_size;
};

/* Utility: generate error from an incoming stub packet.  */
static void
trace_error (char *buf)
{
  if (*buf++ != 'E')
    return;			/* not an error msg */
  switch (*buf)
    {
    case '1':			/* malformed packet error */
      if (*++buf == '0')	/*   general case: */
	error (_("remote.c: error in outgoing packet."));
      else
	error (_("remote.c: error in outgoing packet at field #%ld."),
	       strtol (buf, NULL, 16));
    default:
      error (_("Target returns error code '%s'."), buf);
    }
}

/* Utility: wait for reply from stub, while accepting "O" packets.  */
static char *
remote_get_noisy_reply (char **buf_p,
			long *sizeof_buf)
{
  do				/* Loop on reply from remote stub.  */
    {
      char *buf;

      QUIT;			/* Allow user to bail out with ^C.  */
      getpkt (buf_p, sizeof_buf, 0);
      buf = *buf_p;
      if (buf[0] == 'E')
	trace_error (buf);
      else if (startswith (buf, "qRelocInsn:"))
	{
	  ULONGEST ul;
	  CORE_ADDR from, to, org_to;
	  char *p, *pp;
	  int adjusted_size = 0;
	  int relocated = 0;

	  p = buf + strlen ("qRelocInsn:");
	  pp = unpack_varlen_hex (p, &ul);
	  if (*pp != ';')
	    error (_("invalid qRelocInsn packet: %s"), buf);
	  from = ul;

	  p = pp + 1;
	  unpack_varlen_hex (p, &ul);
	  to = ul;

	  org_to = to;

	  TRY
	    {
	      gdbarch_relocate_instruction (target_gdbarch (), &to, from);
	      relocated = 1;
	    }
	  CATCH (ex, RETURN_MASK_ALL)
	    {
	      if (ex.error == MEMORY_ERROR)
		{
		  /* Propagate memory errors silently back to the
		     target.  The stub may have limited the range of
		     addresses we can write to, for example.  */
		}
	      else
		{
		  /* Something unexpectedly bad happened.  Be verbose
		     so we can tell what, and propagate the error back
		     to the stub, so it doesn't get stuck waiting for
		     a response.  */
		  exception_fprintf (gdb_stderr, ex,
				     _("warning: relocating instruction: "));
		}
	      putpkt ("E01");
	    }
	  END_CATCH

	  if (relocated)
	    {
	      adjusted_size = to - org_to;

	      xsnprintf (buf, *sizeof_buf, "qRelocInsn:%x", adjusted_size);
	      putpkt (buf);
	    }
	}
      else if (buf[0] == 'O' && buf[1] != 'K')
	remote_console_output (buf + 1);	/* 'O' message from stub */
      else
	return buf;		/* Here's the actual reply.  */
    }
  while (1);
}

/* Handle for retreving the remote protocol data from gdbarch.  */
static struct gdbarch_data *remote_gdbarch_data_handle;

static struct remote_arch_state *
get_remote_arch_state (void)
{
  gdb_assert (target_gdbarch () != NULL);
  return ((struct remote_arch_state *)
	  gdbarch_data (target_gdbarch (), remote_gdbarch_data_handle));
}

/* Fetch the global remote target state.  */

static struct remote_state *
get_remote_state (void)
{
  /* Make sure that the remote architecture state has been
     initialized, because doing so might reallocate rs->buf.  Any
     function which calls getpkt also needs to be mindful of changes
     to rs->buf, but this call limits the number of places which run
     into trouble.  */
  get_remote_arch_state ();

  return get_remote_state_raw ();
}

/* Cleanup routine for the remote module's pspace data.  */

static void
remote_pspace_data_cleanup (struct program_space *pspace, void *arg)
{
  char *remote_exec_file = (char *) arg;

  xfree (remote_exec_file);
}

/* Fetch the remote exec-file from the current program space.  */

static const char *
get_remote_exec_file (void)
{
  char *remote_exec_file;

  remote_exec_file
    = (char *) program_space_data (current_program_space,
				   remote_pspace_data);
  if (remote_exec_file == NULL)
    return "";

  return remote_exec_file;
}

/* Set the remote exec file for PSPACE.  */

static void
set_pspace_remote_exec_file (struct program_space *pspace,
			char *remote_exec_file)
{
  char *old_file = (char *) program_space_data (pspace, remote_pspace_data);

  xfree (old_file);
  set_program_space_data (pspace, remote_pspace_data,
			  xstrdup (remote_exec_file));
}

/* The "set/show remote exec-file" set command hook.  */

static void
set_remote_exec_file (char *ignored, int from_tty,
		      struct cmd_list_element *c)
{
  gdb_assert (remote_exec_file_var != NULL);
  set_pspace_remote_exec_file (current_program_space, remote_exec_file_var);
}

/* The "set/show remote exec-file" show command hook.  */

static void
show_remote_exec_file (struct ui_file *file, int from_tty,
		       struct cmd_list_element *cmd, const char *value)
{
  fprintf_filtered (file, "%s\n", remote_exec_file_var);
}

static int
compare_pnums (const void *lhs_, const void *rhs_)
{
  const struct packet_reg * const *lhs
    = (const struct packet_reg * const *) lhs_;
  const struct packet_reg * const *rhs
    = (const struct packet_reg * const *) rhs_;

  if ((*lhs)->pnum < (*rhs)->pnum)
    return -1;
  else if ((*lhs)->pnum == (*rhs)->pnum)
    return 0;
  else
    return 1;
}

static int
map_regcache_remote_table (struct gdbarch *gdbarch, struct packet_reg *regs)
{
  int regnum, num_remote_regs, offset;
  struct packet_reg **remote_regs;

  for (regnum = 0; regnum < gdbarch_num_regs (gdbarch); regnum++)
    {
      struct packet_reg *r = &regs[regnum];

      if (register_size (gdbarch, regnum) == 0)
	/* Do not try to fetch zero-sized (placeholder) registers.  */
	r->pnum = -1;
      else
	r->pnum = gdbarch_remote_register_number (gdbarch, regnum);

      r->regnum = regnum;
    }

  /* Define the g/G packet format as the contents of each register
     with a remote protocol number, in order of ascending protocol
     number.  */

  remote_regs = XALLOCAVEC (struct packet_reg *, gdbarch_num_regs (gdbarch));
  for (num_remote_regs = 0, regnum = 0;
       regnum < gdbarch_num_regs (gdbarch);
       regnum++)
    if (regs[regnum].pnum != -1)
      remote_regs[num_remote_regs++] = &regs[regnum];

  qsort (remote_regs, num_remote_regs, sizeof (struct packet_reg *),
	 compare_pnums);

  for (regnum = 0, offset = 0; regnum < num_remote_regs; regnum++)
    {
      remote_regs[regnum]->in_g_packet = 1;
      remote_regs[regnum]->offset = offset;
      offset += register_size (gdbarch, remote_regs[regnum]->regnum);
    }

  return offset;
}

/* Given the architecture described by GDBARCH, return the remote
   protocol register's number and the register's offset in the g/G
   packets of GDB register REGNUM, in PNUM and POFFSET respectively.
   If the target does not have a mapping for REGNUM, return false,
   otherwise, return true.  */

int
remote_register_number_and_offset (struct gdbarch *gdbarch, int regnum,
				   int *pnum, int *poffset)
{
  struct packet_reg *regs;
  struct cleanup *old_chain;

  gdb_assert (regnum < gdbarch_num_regs (gdbarch));

  regs = XCNEWVEC (struct packet_reg, gdbarch_num_regs (gdbarch));
  old_chain = make_cleanup (xfree, regs);

  map_regcache_remote_table (gdbarch, regs);

  *pnum = regs[regnum].pnum;
  *poffset = regs[regnum].offset;

  do_cleanups (old_chain);

  return *pnum != -1;
}

static void *
init_remote_state (struct gdbarch *gdbarch)
{
  struct remote_state *rs = get_remote_state_raw ();
  struct remote_arch_state *rsa;

  rsa = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct remote_arch_state);

  /* Use the architecture to build a regnum<->pnum table, which will be
     1:1 unless a feature set specifies otherwise.  */
  rsa->regs = GDBARCH_OBSTACK_CALLOC (gdbarch,
				      gdbarch_num_regs (gdbarch),
				      struct packet_reg);

  /* Record the maximum possible size of the g packet - it may turn out
     to be smaller.  */
  rsa->sizeof_g_packet = map_regcache_remote_table (gdbarch, rsa->regs);

  /* Default maximum number of characters in a packet body.  Many
     remote stubs have a hardwired buffer size of 400 bytes
     (c.f. BUFMAX in m68k-stub.c and i386-stub.c).  BUFMAX-1 is used
     as the maximum packet-size to ensure that the packet and an extra
     NUL character can always fit in the buffer.  This stops GDB
     trashing stubs that try to squeeze an extra NUL into what is
     already a full buffer (As of 1999-12-04 that was most stubs).  */
  rsa->remote_packet_size = 400 - 1;

  /* This one is filled in when a ``g'' packet is received.  */
  rsa->actual_register_packet_size = 0;

  /* Should rsa->sizeof_g_packet needs more space than the
     default, adjust the size accordingly.  Remember that each byte is
     encoded as two characters.  32 is the overhead for the packet
     header / footer.  NOTE: cagney/1999-10-26: I suspect that 8
     (``$NN:G...#NN'') is a better guess, the below has been padded a
     little.  */
  if (rsa->sizeof_g_packet > ((rsa->remote_packet_size - 32) / 2))
    rsa->remote_packet_size = (rsa->sizeof_g_packet * 2 + 32);

  /* Make sure that the packet buffer is plenty big enough for
     this architecture.  */
  if (rs->buf_size < rsa->remote_packet_size)
    {
      rs->buf_size = 2 * rsa->remote_packet_size;
      rs->buf = (char *) xrealloc (rs->buf, rs->buf_size);
    }

  return rsa;
}

/* Return the current allowed size of a remote packet.  This is
   inferred from the current architecture, and should be used to
   limit the length of outgoing packets.  */
static long
get_remote_packet_size (void)
{
  struct remote_state *rs = get_remote_state ();
  struct remote_arch_state *rsa = get_remote_arch_state ();

  if (rs->explicit_packet_size)
    return rs->explicit_packet_size;

  return rsa->remote_packet_size;
}

static struct packet_reg *
packet_reg_from_regnum (struct remote_arch_state *rsa, long regnum)
{
  if (regnum < 0 && regnum >= gdbarch_num_regs (target_gdbarch ()))
    return NULL;
  else
    {
      struct packet_reg *r = &rsa->regs[regnum];

      gdb_assert (r->regnum == regnum);
      return r;
    }
}

static struct packet_reg *
packet_reg_from_pnum (struct remote_arch_state *rsa, LONGEST pnum)
{
  int i;

  for (i = 0; i < gdbarch_num_regs (target_gdbarch ()); i++)
    {
      struct packet_reg *r = &rsa->regs[i];

      if (r->pnum == pnum)
	return r;
    }
  return NULL;
}

static struct target_ops remote_ops;

static struct target_ops extended_remote_ops;

/* FIXME: cagney/1999-09-23: Even though getpkt was called with
   ``forever'' still use the normal timeout mechanism.  This is
   currently used by the ASYNC code to guarentee that target reads
   during the initial connect always time-out.  Once getpkt has been
   modified to return a timeout indication and, in turn
   remote_wait()/wait_for_inferior() have gained a timeout parameter
   this can go away.  */
static int wait_forever_enabled_p = 1;

/* Allow the user to specify what sequence to send to the remote
   when he requests a program interruption: Although ^C is usually
   what remote systems expect (this is the default, here), it is
   sometimes preferable to send a break.  On other systems such
   as the Linux kernel, a break followed by g, which is Magic SysRq g
   is required in order to interrupt the execution.  */
const char interrupt_sequence_control_c[] = "Ctrl-C";
const char interrupt_sequence_break[] = "BREAK";
const char interrupt_sequence_break_g[] = "BREAK-g";
static const char *const interrupt_sequence_modes[] =
  {
    interrupt_sequence_control_c,
    interrupt_sequence_break,
    interrupt_sequence_break_g,
    NULL
  };
static const char *interrupt_sequence_mode = interrupt_sequence_control_c;

static void
show_interrupt_sequence (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c,
			 const char *value)
{
  if (interrupt_sequence_mode == interrupt_sequence_control_c)
    fprintf_filtered (file,
		      _("Send the ASCII ETX character (Ctrl-c) "
			"to the remote target to interrupt the "
			"execution of the program.\n"));
  else if (interrupt_sequence_mode == interrupt_sequence_break)
    fprintf_filtered (file,
		      _("send a break signal to the remote target "
			"to interrupt the execution of the program.\n"));
  else if (interrupt_sequence_mode == interrupt_sequence_break_g)
    fprintf_filtered (file,
		      _("Send a break signal and 'g' a.k.a. Magic SysRq g to "
			"the remote target to interrupt the execution "
			"of Linux kernel.\n"));
  else
    internal_error (__FILE__, __LINE__,
		    _("Invalid value for interrupt_sequence_mode: %s."),
		    interrupt_sequence_mode);
}

/* This boolean variable specifies whether interrupt_sequence is sent
   to the remote target when gdb connects to it.
   This is mostly needed when you debug the Linux kernel: The Linux kernel
   expects BREAK g which is Magic SysRq g for connecting gdb.  */
static int interrupt_on_connect = 0;

/* This variable is used to implement the "set/show remotebreak" commands.
   Since these commands are now deprecated in favor of "set/show remote
   interrupt-sequence", it no longer has any effect on the code.  */
static int remote_break;

static void
set_remotebreak (char *args, int from_tty, struct cmd_list_element *c)
{
  if (remote_break)
    interrupt_sequence_mode = interrupt_sequence_break;
  else
    interrupt_sequence_mode = interrupt_sequence_control_c;
}

static void
show_remotebreak (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c,
		  const char *value)
{
}

/* This variable sets the number of bits in an address that are to be
   sent in a memory ("M" or "m") packet.  Normally, after stripping
   leading zeros, the entire address would be sent.  This variable
   restricts the address to REMOTE_ADDRESS_SIZE bits.  HISTORY: The
   initial implementation of remote.c restricted the address sent in
   memory packets to ``host::sizeof long'' bytes - (typically 32
   bits).  Consequently, for 64 bit targets, the upper 32 bits of an
   address was never sent.  Since fixing this bug may cause a break in
   some remote targets this variable is principly provided to
   facilitate backward compatibility.  */

static unsigned int remote_address_size;

/* Temporary to track who currently owns the terminal.  See
   remote_terminal_* for more details.  */

static int remote_async_terminal_ours_p;


/* User configurable variables for the number of characters in a
   memory read/write packet.  MIN (rsa->remote_packet_size,
   rsa->sizeof_g_packet) is the default.  Some targets need smaller
   values (fifo overruns, et.al.) and some users need larger values
   (speed up transfers).  The variables ``preferred_*'' (the user
   request), ``current_*'' (what was actually set) and ``forced_*''
   (Positive - a soft limit, negative - a hard limit).  */

struct memory_packet_config
{
  char *name;
  long size;
  int fixed_p;
};

/* The default max memory-write-packet-size.  The 16k is historical.
   (It came from older GDB's using alloca for buffers and the
   knowledge (folklore?) that some hosts don't cope very well with
   large alloca calls.)  */
#define DEFAULT_MAX_MEMORY_PACKET_SIZE 16384

/* The minimum remote packet size for memory transfers.  Ensures we
   can write at least one byte.  */
#define MIN_MEMORY_PACKET_SIZE 20

/* Compute the current size of a read/write packet.  Since this makes
   use of ``actual_register_packet_size'' the computation is dynamic.  */

static long
get_memory_packet_size (struct memory_packet_config *config)
{
  struct remote_state *rs = get_remote_state ();
  struct remote_arch_state *rsa = get_remote_arch_state ();

  long what_they_get;
  if (config->fixed_p)
    {
      if (config->size <= 0)
	what_they_get = DEFAULT_MAX_MEMORY_PACKET_SIZE;
      else
	what_they_get = config->size;
    }
  else
    {
      what_they_get = get_remote_packet_size ();
      /* Limit the packet to the size specified by the user.  */
      if (config->size > 0
	  && what_they_get > config->size)
	what_they_get = config->size;

      /* Limit it to the size of the targets ``g'' response unless we have
	 permission from the stub to use a larger packet size.  */
      if (rs->explicit_packet_size == 0
	  && rsa->actual_register_packet_size > 0
	  && what_they_get > rsa->actual_register_packet_size)
	what_they_get = rsa->actual_register_packet_size;
    }
  if (what_they_get < MIN_MEMORY_PACKET_SIZE)
    what_they_get = MIN_MEMORY_PACKET_SIZE;

  /* Make sure there is room in the global buffer for this packet
     (including its trailing NUL byte).  */
  if (rs->buf_size < what_they_get + 1)
    {
      rs->buf_size = 2 * what_they_get;
      rs->buf = (char *) xrealloc (rs->buf, 2 * what_they_get);
    }

  return what_they_get;
}

/* Update the size of a read/write packet.  If they user wants
   something really big then do a sanity check.  */

static void
set_memory_packet_size (char *args, struct memory_packet_config *config)
{
  int fixed_p = config->fixed_p;
  long size = config->size;

  if (args == NULL)
    error (_("Argument required (integer, `fixed' or `limited')."));
  else if (strcmp (args, "hard") == 0
      || strcmp (args, "fixed") == 0)
    fixed_p = 1;
  else if (strcmp (args, "soft") == 0
	   || strcmp (args, "limit") == 0)
    fixed_p = 0;
  else
    {
      char *end;

      size = strtoul (args, &end, 0);
      if (args == end)
	error (_("Invalid %s (bad syntax)."), config->name);

      /* Instead of explicitly capping the size of a packet to or
	 disallowing it, the user is allowed to set the size to
	 something arbitrarily large.  */
    }

  /* So that the query shows the correct value.  */
  if (size <= 0)
    size = DEFAULT_MAX_MEMORY_PACKET_SIZE;

  /* Extra checks?  */
  if (fixed_p && !config->fixed_p)
    {
      if (! query (_("The target may not be able to correctly handle a %s\n"
		   "of %ld bytes. Change the packet size? "),
		   config->name, size))
	error (_("Packet size not changed."));
    }
  /* Update the config.  */
  config->fixed_p = fixed_p;
  config->size = size;
}

static void
show_memory_packet_size (struct memory_packet_config *config)
{
  printf_filtered (_("The %s is %ld. "), config->name, config->size);
  if (config->fixed_p)
    printf_filtered (_("Packets are fixed at %ld bytes.\n"),
		     get_memory_packet_size (config));
  else
    printf_filtered (_("Packets are limited to %ld bytes.\n"),
		     get_memory_packet_size (config));
}

static struct memory_packet_config memory_write_packet_config =
{
  "memory-write-packet-size",
};

static void
set_memory_write_packet_size (char *args, int from_tty)
{
  set_memory_packet_size (args, &memory_write_packet_config);
}

static void
show_memory_write_packet_size (char *args, int from_tty)
{
  show_memory_packet_size (&memory_write_packet_config);
}

static long
get_memory_write_packet_size (void)
{
  return get_memory_packet_size (&memory_write_packet_config);
}

static struct memory_packet_config memory_read_packet_config =
{
  "memory-read-packet-size",
};

static void
set_memory_read_packet_size (char *args, int from_tty)
{
  set_memory_packet_size (args, &memory_read_packet_config);
}

static void
show_memory_read_packet_size (char *args, int from_tty)
{
  show_memory_packet_size (&memory_read_packet_config);
}

static long
get_memory_read_packet_size (void)
{
  long size = get_memory_packet_size (&memory_read_packet_config);

  /* FIXME: cagney/1999-11-07: Functions like getpkt() need to get an
     extra buffer size argument before the memory read size can be
     increased beyond this.  */
  if (size > get_remote_packet_size ())
    size = get_remote_packet_size ();
  return size;
}


/* Generic configuration support for packets the stub optionally
   supports.  Allows the user to specify the use of the packet as well
   as allowing GDB to auto-detect support in the remote stub.  */

enum packet_support
  {
    PACKET_SUPPORT_UNKNOWN = 0,
    PACKET_ENABLE,
    PACKET_DISABLE
  };

struct packet_config
  {
    const char *name;
    const char *title;

    /* If auto, GDB auto-detects support for this packet or feature,
       either through qSupported, or by trying the packet and looking
       at the response.  If true, GDB assumes the target supports this
       packet.  If false, the packet is disabled.  Configs that don't
       have an associated command always have this set to auto.  */
    enum auto_boolean detect;

    /* Does the target support this packet?  */
    enum packet_support support;
  };

/* Analyze a packet's return value and update the packet config
   accordingly.  */

enum packet_result
{
  PACKET_ERROR,
  PACKET_OK,
  PACKET_UNKNOWN
};

static enum packet_support packet_config_support (struct packet_config *config);
static enum packet_support packet_support (int packet);

static void
show_packet_config_cmd (struct packet_config *config)
{
  char *support = "internal-error";

  switch (packet_config_support (config))
    {
    case PACKET_ENABLE:
      support = "enabled";
      break;
    case PACKET_DISABLE:
      support = "disabled";
      break;
    case PACKET_SUPPORT_UNKNOWN:
      support = "unknown";
      break;
    }
  switch (config->detect)
    {
    case AUTO_BOOLEAN_AUTO:
      printf_filtered (_("Support for the `%s' packet "
			 "is auto-detected, currently %s.\n"),
		       config->name, support);
      break;
    case AUTO_BOOLEAN_TRUE:
    case AUTO_BOOLEAN_FALSE:
      printf_filtered (_("Support for the `%s' packet is currently %s.\n"),
		       config->name, support);
      break;
    }
}

static void
add_packet_config_cmd (struct packet_config *config, const char *name,
		       const char *title, int legacy)
{
  char *set_doc;
  char *show_doc;
  char *cmd_name;

  config->name = name;
  config->title = title;
  set_doc = xstrprintf ("Set use of remote protocol `%s' (%s) packet",
			name, title);
  show_doc = xstrprintf ("Show current use of remote "
			 "protocol `%s' (%s) packet",
			 name, title);
  /* set/show TITLE-packet {auto,on,off} */
  cmd_name = xstrprintf ("%s-packet", title);
  add_setshow_auto_boolean_cmd (cmd_name, class_obscure,
				&config->detect, set_doc,
				show_doc, NULL, /* help_doc */
				NULL,
				show_remote_protocol_packet_cmd,
				&remote_set_cmdlist, &remote_show_cmdlist);
  /* The command code copies the documentation strings.  */
  xfree (set_doc);
  xfree (show_doc);
  /* set/show remote NAME-packet {auto,on,off} -- legacy.  */
  if (legacy)
    {
      char *legacy_name;

      legacy_name = xstrprintf ("%s-packet", name);
      add_alias_cmd (legacy_name, cmd_name, class_obscure, 0,
		     &remote_set_cmdlist);
      add_alias_cmd (legacy_name, cmd_name, class_obscure, 0,
		     &remote_show_cmdlist);
    }
}

static enum packet_result
packet_check_result (const char *buf)
{
  if (buf[0] != '\0')
    {
      /* The stub recognized the packet request.  Check that the
	 operation succeeded.  */
      if (buf[0] == 'E'
	  && isxdigit (buf[1]) && isxdigit (buf[2])
	  && buf[3] == '\0')
	/* "Enn"  - definitly an error.  */
	return PACKET_ERROR;

      /* Always treat "E." as an error.  This will be used for
	 more verbose error messages, such as E.memtypes.  */
      if (buf[0] == 'E' && buf[1] == '.')
	return PACKET_ERROR;

      /* The packet may or may not be OK.  Just assume it is.  */
      return PACKET_OK;
    }
  else
    /* The stub does not support the packet.  */
    return PACKET_UNKNOWN;
}

static enum packet_result
packet_ok (const char *buf, struct packet_config *config)
{
  enum packet_result result;

  if (config->detect != AUTO_BOOLEAN_TRUE
      && config->support == PACKET_DISABLE)
    internal_error (__FILE__, __LINE__,
		    _("packet_ok: attempt to use a disabled packet"));

  result = packet_check_result (buf);
  switch (result)
    {
    case PACKET_OK:
    case PACKET_ERROR:
      /* The stub recognized the packet request.  */
      if (config->support == PACKET_SUPPORT_UNKNOWN)
	{
	  if (remote_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"Packet %s (%s) is supported\n",
				config->name, config->title);
	  config->support = PACKET_ENABLE;
	}
      break;
    case PACKET_UNKNOWN:
      /* The stub does not support the packet.  */
      if (config->detect == AUTO_BOOLEAN_AUTO
	  && config->support == PACKET_ENABLE)
	{
	  /* If the stub previously indicated that the packet was
	     supported then there is a protocol error.  */
	  error (_("Protocol error: %s (%s) conflicting enabled responses."),
		 config->name, config->title);
	}
      else if (config->detect == AUTO_BOOLEAN_TRUE)
	{
	  /* The user set it wrong.  */
	  error (_("Enabled packet %s (%s) not recognized by stub"),
		 config->name, config->title);
	}

      if (remote_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "Packet %s (%s) is NOT supported\n",
			    config->name, config->title);
      config->support = PACKET_DISABLE;
      break;
    }

  return result;
}

enum {
  PACKET_vCont = 0,
  PACKET_X,
  PACKET_qSymbol,
  PACKET_P,
  PACKET_p,
  PACKET_Z0,
  PACKET_Z1,
  PACKET_Z2,
  PACKET_Z3,
  PACKET_Z4,
  PACKET_vFile_setfs,
  PACKET_vFile_open,
  PACKET_vFile_pread,
  PACKET_vFile_pwrite,
  PACKET_vFile_close,
  PACKET_vFile_unlink,
  PACKET_vFile_readlink,
  PACKET_vFile_fstat,
  PACKET_qXfer_auxv,
  PACKET_qXfer_features,
  PACKET_qXfer_exec_file,
  PACKET_qXfer_libraries,
  PACKET_qXfer_libraries_svr4,
  PACKET_qXfer_memory_map,
  PACKET_qXfer_spu_read,
  PACKET_qXfer_spu_write,
  PACKET_qXfer_osdata,
  PACKET_qXfer_threads,
  PACKET_qXfer_statictrace_read,
  PACKET_qXfer_traceframe_info,
  PACKET_qXfer_uib,
  PACKET_qGetTIBAddr,
  PACKET_qGetTLSAddr,
  PACKET_qSupported,
  PACKET_qTStatus,
  PACKET_QPassSignals,
  PACKET_QCatchSyscalls,
  PACKET_QProgramSignals,
  PACKET_qCRC,
  PACKET_qSearch_memory,
  PACKET_vAttach,
  PACKET_vRun,
  PACKET_QStartNoAckMode,
  PACKET_vKill,
  PACKET_qXfer_siginfo_read,
  PACKET_qXfer_siginfo_write,
  PACKET_qAttached,

  /* Support for conditional tracepoints.  */
  PACKET_ConditionalTracepoints,

  /* Support for target-side breakpoint conditions.  */
  PACKET_ConditionalBreakpoints,

  /* Support for target-side breakpoint commands.  */
  PACKET_BreakpointCommands,

  /* Support for fast tracepoints.  */
  PACKET_FastTracepoints,

  /* Support for static tracepoints.  */
  PACKET_StaticTracepoints,

  /* Support for installing tracepoints while a trace experiment is
     running.  */
  PACKET_InstallInTrace,

  PACKET_bc,
  PACKET_bs,
  PACKET_TracepointSource,
  PACKET_QAllow,
  PACKET_qXfer_fdpic,
  PACKET_QDisableRandomization,
  PACKET_QAgent,
  PACKET_QTBuffer_size,
  PACKET_Qbtrace_off,
  PACKET_Qbtrace_bts,
  PACKET_Qbtrace_pt,
  PACKET_qXfer_btrace,

  /* Support for the QNonStop packet.  */
  PACKET_QNonStop,

  /* Support for the QThreadEvents packet.  */
  PACKET_QThreadEvents,

  /* Support for multi-process extensions.  */
  PACKET_multiprocess_feature,

  /* Support for enabling and disabling tracepoints while a trace
     experiment is running.  */
  PACKET_EnableDisableTracepoints_feature,

  /* Support for collecting strings using the tracenz bytecode.  */
  PACKET_tracenz_feature,

  /* Support for continuing to run a trace experiment while GDB is
     disconnected.  */
  PACKET_DisconnectedTracing_feature,

  /* Support for qXfer:libraries-svr4:read with a non-empty annex.  */
  PACKET_augmented_libraries_svr4_read_feature,

  /* Support for the qXfer:btrace-conf:read packet.  */
  PACKET_qXfer_btrace_conf,

  /* Support for the Qbtrace-conf:bts:size packet.  */
  PACKET_Qbtrace_conf_bts_size,

  /* Support for swbreak+ feature.  */
  PACKET_swbreak_feature,

  /* Support for hwbreak+ feature.  */
  PACKET_hwbreak_feature,

  /* Support for fork events.  */
  PACKET_fork_event_feature,

  /* Support for vfork events.  */
  PACKET_vfork_event_feature,

  /* Support for the Qbtrace-conf:pt:size packet.  */
  PACKET_Qbtrace_conf_pt_size,

  /* Support for exec events.  */
  PACKET_exec_event_feature,

  /* Support for query supported vCont actions.  */
  PACKET_vContSupported,

  /* Support remote CTRL-C.  */
  PACKET_vCtrlC,

  /* Support TARGET_WAITKIND_NO_RESUMED.  */
  PACKET_no_resumed,

  PACKET_MAX
};

static struct packet_config remote_protocol_packets[PACKET_MAX];

/* Returns the packet's corresponding "set remote foo-packet" command
   state.  See struct packet_config for more details.  */

static enum auto_boolean
packet_set_cmd_state (int packet)
{
  return remote_protocol_packets[packet].detect;
}

/* Returns whether a given packet or feature is supported.  This takes
   into account the state of the corresponding "set remote foo-packet"
   command, which may be used to bypass auto-detection.  */

static enum packet_support
packet_config_support (struct packet_config *config)
{
  switch (config->detect)
    {
    case AUTO_BOOLEAN_TRUE:
      return PACKET_ENABLE;
    case AUTO_BOOLEAN_FALSE:
      return PACKET_DISABLE;
    case AUTO_BOOLEAN_AUTO:
      return config->support;
    default:
      gdb_assert_not_reached (_("bad switch"));
    }
}

/* Same as packet_config_support, but takes the packet's enum value as
   argument.  */

static enum packet_support
packet_support (int packet)
{
  struct packet_config *config = &remote_protocol_packets[packet];

  return packet_config_support (config);
}

static void
show_remote_protocol_packet_cmd (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c,
				 const char *value)
{
  struct packet_config *packet;

  for (packet = remote_protocol_packets;
       packet < &remote_protocol_packets[PACKET_MAX];
       packet++)
    {
      if (&packet->detect == c->var)
	{
	  show_packet_config_cmd (packet);
	  return;
	}
    }
  internal_error (__FILE__, __LINE__, _("Could not find config for %s"),
		  c->name);
}

/* Should we try one of the 'Z' requests?  */

enum Z_packet_type
{
  Z_PACKET_SOFTWARE_BP,
  Z_PACKET_HARDWARE_BP,
  Z_PACKET_WRITE_WP,
  Z_PACKET_READ_WP,
  Z_PACKET_ACCESS_WP,
  NR_Z_PACKET_TYPES
};

/* For compatibility with older distributions.  Provide a ``set remote
   Z-packet ...'' command that updates all the Z packet types.  */

static enum auto_boolean remote_Z_packet_detect;

static void
set_remote_protocol_Z_packet_cmd (char *args, int from_tty,
				  struct cmd_list_element *c)
{
  int i;

  for (i = 0; i < NR_Z_PACKET_TYPES; i++)
    remote_protocol_packets[PACKET_Z0 + i].detect = remote_Z_packet_detect;
}

static void
show_remote_protocol_Z_packet_cmd (struct ui_file *file, int from_tty,
				   struct cmd_list_element *c,
				   const char *value)
{
  int i;

  for (i = 0; i < NR_Z_PACKET_TYPES; i++)
    {
      show_packet_config_cmd (&remote_protocol_packets[PACKET_Z0 + i]);
    }
}

/* Returns true if the multi-process extensions are in effect.  */

static int
remote_multi_process_p (struct remote_state *rs)
{
  return packet_support (PACKET_multiprocess_feature) == PACKET_ENABLE;
}

/* Returns true if fork events are supported.  */

static int
remote_fork_event_p (struct remote_state *rs)
{
  return packet_support (PACKET_fork_event_feature) == PACKET_ENABLE;
}

/* Returns true if vfork events are supported.  */

static int
remote_vfork_event_p (struct remote_state *rs)
{
  return packet_support (PACKET_vfork_event_feature) == PACKET_ENABLE;
}

/* Returns true if exec events are supported.  */

static int
remote_exec_event_p (struct remote_state *rs)
{
  return packet_support (PACKET_exec_event_feature) == PACKET_ENABLE;
}

/* Insert fork catchpoint target routine.  If fork events are enabled
   then return success, nothing more to do.  */

static int
remote_insert_fork_catchpoint (struct target_ops *ops, int pid)
{
  struct remote_state *rs = get_remote_state ();

  return !remote_fork_event_p (rs);
}

/* Remove fork catchpoint target routine.  Nothing to do, just
   return success.  */

static int
remote_remove_fork_catchpoint (struct target_ops *ops, int pid)
{
  return 0;
}

/* Insert vfork catchpoint target routine.  If vfork events are enabled
   then return success, nothing more to do.  */

static int
remote_insert_vfork_catchpoint (struct target_ops *ops, int pid)
{
  struct remote_state *rs = get_remote_state ();

  return !remote_vfork_event_p (rs);
}

/* Remove vfork catchpoint target routine.  Nothing to do, just
   return success.  */

static int
remote_remove_vfork_catchpoint (struct target_ops *ops, int pid)
{
  return 0;
}

/* Insert exec catchpoint target routine.  If exec events are
   enabled, just return success.  */

static int
remote_insert_exec_catchpoint (struct target_ops *ops, int pid)
{
  struct remote_state *rs = get_remote_state ();

  return !remote_exec_event_p (rs);
}

/* Remove exec catchpoint target routine.  Nothing to do, just
   return success.  */

static int
remote_remove_exec_catchpoint (struct target_ops *ops, int pid)
{
  return 0;
}


/* Asynchronous signal handle registered as event loop source for
   when we have pending events ready to be passed to the core.  */

static struct async_event_handler *remote_async_inferior_event_token;



static ptid_t magic_null_ptid;
static ptid_t not_sent_ptid;
static ptid_t any_thread_ptid;

/* Find out if the stub attached to PID (and hence GDB should offer to
   detach instead of killing it when bailing out).  */

static int
remote_query_attached (int pid)
{
  struct remote_state *rs = get_remote_state ();
  size_t size = get_remote_packet_size ();

  if (packet_support (PACKET_qAttached) == PACKET_DISABLE)
    return 0;

  if (remote_multi_process_p (rs))
    xsnprintf (rs->buf, size, "qAttached:%x", pid);
  else
    xsnprintf (rs->buf, size, "qAttached");

  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  switch (packet_ok (rs->buf,
		     &remote_protocol_packets[PACKET_qAttached]))
    {
    case PACKET_OK:
      if (strcmp (rs->buf, "1") == 0)
	return 1;
      break;
    case PACKET_ERROR:
      warning (_("Remote failure reply: %s"), rs->buf);
      break;
    case PACKET_UNKNOWN:
      break;
    }

  return 0;
}

/* Add PID to GDB's inferior table.  If FAKE_PID_P is true, then PID
   has been invented by GDB, instead of reported by the target.  Since
   we can be connected to a remote system before before knowing about
   any inferior, mark the target with execution when we find the first
   inferior.  If ATTACHED is 1, then we had just attached to this
   inferior.  If it is 0, then we just created this inferior.  If it
   is -1, then try querying the remote stub to find out if it had
   attached to the inferior or not.  If TRY_OPEN_EXEC is true then
   attempt to open this inferior's executable as the main executable
   if no main executable is open already.  */

static struct inferior *
remote_add_inferior (int fake_pid_p, int pid, int attached,
		     int try_open_exec)
{
  struct inferior *inf;

  /* Check whether this process we're learning about is to be
     considered attached, or if is to be considered to have been
     spawned by the stub.  */
  if (attached == -1)
    attached = remote_query_attached (pid);

  if (gdbarch_has_global_solist (target_gdbarch ()))
    {
      /* If the target shares code across all inferiors, then every
	 attach adds a new inferior.  */
      inf = add_inferior (pid);

      /* ... and every inferior is bound to the same program space.
	 However, each inferior may still have its own address
	 space.  */
      inf->aspace = maybe_new_address_space ();
      inf->pspace = current_program_space;
    }
  else
    {
      /* In the traditional debugging scenario, there's a 1-1 match
	 between program/address spaces.  We simply bind the inferior
	 to the program space's address space.  */
      inf = current_inferior ();
      inferior_appeared (inf, pid);
    }

  inf->attach_flag = attached;
  inf->fake_pid_p = fake_pid_p;

  /* If no main executable is currently open then attempt to
     open the file that was executed to create this inferior.  */
  if (try_open_exec && get_exec_file (0) == NULL)
    exec_file_locate_attach (pid, 0, 1);

  return inf;
}

/* Add thread PTID to GDB's thread list.  Tag it as executing/running
   according to RUNNING.  */

static void
remote_add_thread (ptid_t ptid, int running, int executing)
{
  struct remote_state *rs = get_remote_state ();

  /* GDB historically didn't pull threads in the initial connection
     setup.  If the remote target doesn't even have a concept of
     threads (e.g., a bare-metal target), even if internally we
     consider that a single-threaded target, mentioning a new thread
     might be confusing to the user.  Be silent then, preserving the
     age old behavior.  */
  if (rs->starting_up)
    add_thread_silent (ptid);
  else
    add_thread (ptid);

  set_executing (ptid, executing);
  set_running (ptid, running);
}

/* Come here when we learn about a thread id from the remote target.
   It may be the first time we hear about such thread, so take the
   opportunity to add it to GDB's thread list.  In case this is the
   first time we're noticing its corresponding inferior, add it to
   GDB's inferior list as well.  EXECUTING indicates whether the
   thread is (internally) executing or stopped.  */

static void
remote_notice_new_inferior (ptid_t currthread, int executing)
{
  /* In non-stop mode, we assume new found threads are (externally)
     running until proven otherwise with a stop reply.  In all-stop,
     we can only get here if all threads are stopped.  */
  int running = target_is_non_stop_p () ? 1 : 0;

  /* If this is a new thread, add it to GDB's thread list.
     If we leave it up to WFI to do this, bad things will happen.  */

  if (in_thread_list (currthread) && is_exited (currthread))
    {
      /* We're seeing an event on a thread id we knew had exited.
	 This has to be a new thread reusing the old id.  Add it.  */
      remote_add_thread (currthread, running, executing);
      return;
    }

  if (!in_thread_list (currthread))
    {
      struct inferior *inf = NULL;
      int pid = ptid_get_pid (currthread);

      if (ptid_is_pid (inferior_ptid)
	  && pid == ptid_get_pid (inferior_ptid))
	{
	  /* inferior_ptid has no thread member yet.  This can happen
	     with the vAttach -> remote_wait,"TAAthread:" path if the
	     stub doesn't support qC.  This is the first stop reported
	     after an attach, so this is the main thread.  Update the
	     ptid in the thread list.  */
	  if (in_thread_list (pid_to_ptid (pid)))
	    thread_change_ptid (inferior_ptid, currthread);
	  else
	    {
	      remote_add_thread (currthread, running, executing);
	      inferior_ptid = currthread;
	    }
	  return;
	}

      if (ptid_equal (magic_null_ptid, inferior_ptid))
	{
	  /* inferior_ptid is not set yet.  This can happen with the
	     vRun -> remote_wait,"TAAthread:" path if the stub
	     doesn't support qC.  This is the first stop reported
	     after an attach, so this is the main thread.  Update the
	     ptid in the thread list.  */
	  thread_change_ptid (inferior_ptid, currthread);
	  return;
	}

      /* When connecting to a target remote, or to a target
	 extended-remote which already was debugging an inferior, we
	 may not know about it yet.  Add it before adding its child
	 thread, so notifications are emitted in a sensible order.  */
      if (!in_inferior_list (ptid_get_pid (currthread)))
	{
	  struct remote_state *rs = get_remote_state ();
	  int fake_pid_p = !remote_multi_process_p (rs);

	  inf = remote_add_inferior (fake_pid_p,
				     ptid_get_pid (currthread), -1, 1);
	}

      /* This is really a new thread.  Add it.  */
      remote_add_thread (currthread, running, executing);

      /* If we found a new inferior, let the common code do whatever
	 it needs to with it (e.g., read shared libraries, insert
	 breakpoints), unless we're just setting up an all-stop
	 connection.  */
      if (inf != NULL)
	{
	  struct remote_state *rs = get_remote_state ();

	  if (!rs->starting_up)
	    notice_new_inferior (currthread, executing, 0);
	}
    }
}

/* Return the private thread data, creating it if necessary.  */

static struct private_thread_info *
demand_private_info (ptid_t ptid)
{
  struct thread_info *info = find_thread_ptid (ptid);

  gdb_assert (info);

  if (!info->priv)
    {
      info->priv = XNEW (struct private_thread_info);
      info->private_dtor = free_private_thread_info;
      info->priv->core = -1;
      info->priv->extra = NULL;
      info->priv->name = NULL;
    }

  return info->priv;
}

/* Call this function as a result of
   1) A halt indication (T packet) containing a thread id
   2) A direct query of currthread
   3) Successful execution of set thread */

static void
record_currthread (struct remote_state *rs, ptid_t currthread)
{
  rs->general_thread = currthread;
}

/* If 'QPassSignals' is supported, tell the remote stub what signals
   it can simply pass through to the inferior without reporting.  */

static void
remote_pass_signals (struct target_ops *self,
		     int numsigs, unsigned char *pass_signals)
{
  if (packet_support (PACKET_QPassSignals) != PACKET_DISABLE)
    {
      char *pass_packet, *p;
      int count = 0, i;
      struct remote_state *rs = get_remote_state ();

      gdb_assert (numsigs < 256);
      for (i = 0; i < numsigs; i++)
	{
	  if (pass_signals[i])
	    count++;
	}
      pass_packet = (char *) xmalloc (count * 3 + strlen ("QPassSignals:") + 1);
      strcpy (pass_packet, "QPassSignals:");
      p = pass_packet + strlen (pass_packet);
      for (i = 0; i < numsigs; i++)
	{
	  if (pass_signals[i])
	    {
	      if (i >= 16)
		*p++ = tohex (i >> 4);
	      *p++ = tohex (i & 15);
	      if (count)
		*p++ = ';';
	      else
		break;
	      count--;
	    }
	}
      *p = 0;
      if (!rs->last_pass_packet || strcmp (rs->last_pass_packet, pass_packet))
	{
	  putpkt (pass_packet);
	  getpkt (&rs->buf, &rs->buf_size, 0);
	  packet_ok (rs->buf, &remote_protocol_packets[PACKET_QPassSignals]);
	  if (rs->last_pass_packet)
	    xfree (rs->last_pass_packet);
	  rs->last_pass_packet = pass_packet;
	}
      else
	xfree (pass_packet);
    }
}

/* If 'QCatchSyscalls' is supported, tell the remote stub
   to report syscalls to GDB.  */

static int
remote_set_syscall_catchpoint (struct target_ops *self,
			       int pid, int needed, int any_count,
			       int table_size, int *table)
{
  char *catch_packet;
  enum packet_result result;
  int n_sysno = 0;

  if (packet_support (PACKET_QCatchSyscalls) == PACKET_DISABLE)
    {
      /* Not supported.  */
      return 1;
    }

  if (needed && !any_count)
    {
      int i;

      /* Count how many syscalls are to be caught (table[sysno] != 0).  */
      for (i = 0; i < table_size; i++)
	{
	  if (table[i] != 0)
	    n_sysno++;
	}
    }

  if (remote_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "remote_set_syscall_catchpoint "
			  "pid %d needed %d any_count %d n_sysno %d\n",
			  pid, needed, any_count, n_sysno);
    }

  if (needed)
    {
      /* Prepare a packet with the sysno list, assuming max 8+1
	 characters for a sysno.  If the resulting packet size is too
	 big, fallback on the non-selective packet.  */
      const int maxpktsz = strlen ("QCatchSyscalls:1") + n_sysno * 9 + 1;

      catch_packet = (char *) xmalloc (maxpktsz);
      strcpy (catch_packet, "QCatchSyscalls:1");
      if (!any_count)
	{
	  int i;
	  char *p;

	  p = catch_packet;
	  p += strlen (p);

	  /* Add in catch_packet each syscall to be caught (table[i] != 0).  */
	  for (i = 0; i < table_size; i++)
	    {
	      if (table[i] != 0)
		p += xsnprintf (p, catch_packet + maxpktsz - p, ";%x", i);
	    }
	}
      if (strlen (catch_packet) > get_remote_packet_size ())
	{
	  /* catch_packet too big.  Fallback to less efficient
	     non selective mode, with GDB doing the filtering.  */
	  catch_packet[sizeof ("QCatchSyscalls:1") - 1] = 0;
	}
    }
  else
    catch_packet = xstrdup ("QCatchSyscalls:0");

  {
    struct cleanup *old_chain = make_cleanup (xfree, catch_packet);
    struct remote_state *rs = get_remote_state ();

    putpkt (catch_packet);
    getpkt (&rs->buf, &rs->buf_size, 0);
    result = packet_ok (rs->buf, &remote_protocol_packets[PACKET_QCatchSyscalls]);
    do_cleanups (old_chain);
    if (result == PACKET_OK)
      return 0;
    else
      return -1;
  }
}

/* If 'QProgramSignals' is supported, tell the remote stub what
   signals it should pass through to the inferior when detaching.  */

static void
remote_program_signals (struct target_ops *self,
			int numsigs, unsigned char *signals)
{
  if (packet_support (PACKET_QProgramSignals) != PACKET_DISABLE)
    {
      char *packet, *p;
      int count = 0, i;
      struct remote_state *rs = get_remote_state ();

      gdb_assert (numsigs < 256);
      for (i = 0; i < numsigs; i++)
	{
	  if (signals[i])
	    count++;
	}
      packet = (char *) xmalloc (count * 3 + strlen ("QProgramSignals:") + 1);
      strcpy (packet, "QProgramSignals:");
      p = packet + strlen (packet);
      for (i = 0; i < numsigs; i++)
	{
	  if (signal_pass_state (i))
	    {
	      if (i >= 16)
		*p++ = tohex (i >> 4);
	      *p++ = tohex (i & 15);
	      if (count)
		*p++ = ';';
	      else
		break;
	      count--;
	    }
	}
      *p = 0;
      if (!rs->last_program_signals_packet
	  || strcmp (rs->last_program_signals_packet, packet) != 0)
	{
	  putpkt (packet);
	  getpkt (&rs->buf, &rs->buf_size, 0);
	  packet_ok (rs->buf, &remote_protocol_packets[PACKET_QProgramSignals]);
	  xfree (rs->last_program_signals_packet);
	  rs->last_program_signals_packet = packet;
	}
      else
	xfree (packet);
    }
}

/* If PTID is MAGIC_NULL_PTID, don't set any thread.  If PTID is
   MINUS_ONE_PTID, set the thread to -1, so the stub returns the
   thread.  If GEN is set, set the general thread, if not, then set
   the step/continue thread.  */
static void
set_thread (struct ptid ptid, int gen)
{
  struct remote_state *rs = get_remote_state ();
  ptid_t state = gen ? rs->general_thread : rs->continue_thread;
  char *buf = rs->buf;
  char *endbuf = rs->buf + get_remote_packet_size ();

  if (ptid_equal (state, ptid))
    return;

  *buf++ = 'H';
  *buf++ = gen ? 'g' : 'c';
  if (ptid_equal (ptid, magic_null_ptid))
    xsnprintf (buf, endbuf - buf, "0");
  else if (ptid_equal (ptid, any_thread_ptid))
    xsnprintf (buf, endbuf - buf, "0");
  else if (ptid_equal (ptid, minus_one_ptid))
    xsnprintf (buf, endbuf - buf, "-1");
  else
    write_ptid (buf, endbuf, ptid);
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);
  if (gen)
    rs->general_thread = ptid;
  else
    rs->continue_thread = ptid;
}

static void
set_general_thread (struct ptid ptid)
{
  set_thread (ptid, 1);
}

static void
set_continue_thread (struct ptid ptid)
{
  set_thread (ptid, 0);
}

/* Change the remote current process.  Which thread within the process
   ends up selected isn't important, as long as it is the same process
   as what INFERIOR_PTID points to.

   This comes from that fact that there is no explicit notion of
   "selected process" in the protocol.  The selected process for
   general operations is the process the selected general thread
   belongs to.  */

static void
set_general_process (void)
{
  struct remote_state *rs = get_remote_state ();

  /* If the remote can't handle multiple processes, don't bother.  */
  if (!remote_multi_process_p (rs))
    return;

  /* We only need to change the remote current thread if it's pointing
     at some other process.  */
  if (ptid_get_pid (rs->general_thread) != ptid_get_pid (inferior_ptid))
    set_general_thread (inferior_ptid);
}


/* Return nonzero if this is the main thread that we made up ourselves
   to model non-threaded targets as single-threaded.  */

static int
remote_thread_always_alive (struct target_ops *ops, ptid_t ptid)
{
  if (ptid_equal (ptid, magic_null_ptid))
    /* The main thread is always alive.  */
    return 1;

  if (ptid_get_pid (ptid) != 0 && ptid_get_lwp (ptid) == 0)
    /* The main thread is always alive.  This can happen after a
       vAttach, if the remote side doesn't support
       multi-threading.  */
    return 1;

  return 0;
}

/* Return nonzero if the thread PTID is still alive on the remote
   system.  */

static int
remote_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  struct remote_state *rs = get_remote_state ();
  char *p, *endp;

  /* Check if this is a thread that we made up ourselves to model
     non-threaded targets as single-threaded.  */
  if (remote_thread_always_alive (ops, ptid))
    return 1;

  p = rs->buf;
  endp = rs->buf + get_remote_packet_size ();

  *p++ = 'T';
  write_ptid (p, endp, ptid);

  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);
  return (rs->buf[0] == 'O' && rs->buf[1] == 'K');
}

/* Return a pointer to a thread name if we know it and NULL otherwise.
   The thread_info object owns the memory for the name.  */

static const char *
remote_thread_name (struct target_ops *ops, struct thread_info *info)
{
  if (info->priv != NULL)
    return info->priv->name;

  return NULL;
}

/* About these extended threadlist and threadinfo packets.  They are
   variable length packets but, the fields within them are often fixed
   length.  They are redundent enough to send over UDP as is the
   remote protocol in general.  There is a matching unit test module
   in libstub.  */

/* WARNING: This threadref data structure comes from the remote O.S.,
   libstub protocol encoding, and remote.c.  It is not particularly
   changable.  */

/* Right now, the internal structure is int. We want it to be bigger.
   Plan to fix this.  */

typedef int gdb_threadref;	/* Internal GDB thread reference.  */

/* gdb_ext_thread_info is an internal GDB data structure which is
   equivalent to the reply of the remote threadinfo packet.  */

struct gdb_ext_thread_info
  {
    threadref threadid;		/* External form of thread reference.  */
    int active;			/* Has state interesting to GDB?
				   regs, stack.  */
    char display[256];		/* Brief state display, name,
				   blocked/suspended.  */
    char shortname[32];		/* To be used to name threads.  */
    char more_display[256];	/* Long info, statistics, queue depth,
				   whatever.  */
  };

/* The volume of remote transfers can be limited by submitting
   a mask containing bits specifying the desired information.
   Use a union of these values as the 'selection' parameter to
   get_thread_info.  FIXME: Make these TAG names more thread specific.  */

#define TAG_THREADID 1
#define TAG_EXISTS 2
#define TAG_DISPLAY 4
#define TAG_THREADNAME 8
#define TAG_MOREDISPLAY 16

#define BUF_THREAD_ID_SIZE (OPAQUETHREADBYTES * 2)

static char *unpack_nibble (char *buf, int *val);

static char *unpack_byte (char *buf, int *value);

static char *pack_int (char *buf, int value);

static char *unpack_int (char *buf, int *value);

static char *unpack_string (char *src, char *dest, int length);

static char *pack_threadid (char *pkt, threadref *id);

static char *unpack_threadid (char *inbuf, threadref *id);

void int_to_threadref (threadref *id, int value);

static int threadref_to_int (threadref *ref);

static void copy_threadref (threadref *dest, threadref *src);

static int threadmatch (threadref *dest, threadref *src);

static char *pack_threadinfo_request (char *pkt, int mode,
				      threadref *id);

static int remote_unpack_thread_info_response (char *pkt,
					       threadref *expectedref,
					       struct gdb_ext_thread_info
					       *info);


static int remote_get_threadinfo (threadref *threadid,
				  int fieldset,	/*TAG mask */
				  struct gdb_ext_thread_info *info);

static char *pack_threadlist_request (char *pkt, int startflag,
				      int threadcount,
				      threadref *nextthread);

static int parse_threadlist_response (char *pkt,
				      int result_limit,
				      threadref *original_echo,
				      threadref *resultlist,
				      int *doneflag);

static int remote_get_threadlist (int startflag,
				  threadref *nextthread,
				  int result_limit,
				  int *done,
				  int *result_count,
				  threadref *threadlist);

typedef int (*rmt_thread_action) (threadref *ref, void *context);

static int remote_threadlist_iterator (rmt_thread_action stepfunction,
				       void *context, int looplimit);

static int remote_newthread_step (threadref *ref, void *context);


/* Write a PTID to BUF.  ENDBUF points to one-passed-the-end of the
   buffer we're allowed to write to.  Returns
   BUF+CHARACTERS_WRITTEN.  */

static char *
write_ptid (char *buf, const char *endbuf, ptid_t ptid)
{
  int pid, tid;
  struct remote_state *rs = get_remote_state ();

  if (remote_multi_process_p (rs))
    {
      pid = ptid_get_pid (ptid);
      if (pid < 0)
	buf += xsnprintf (buf, endbuf - buf, "p-%x.", -pid);
      else
	buf += xsnprintf (buf, endbuf - buf, "p%x.", pid);
    }
  tid = ptid_get_lwp (ptid);
  if (tid < 0)
    buf += xsnprintf (buf, endbuf - buf, "-%x", -tid);
  else
    buf += xsnprintf (buf, endbuf - buf, "%x", tid);

  return buf;
}

/* Extract a PTID from BUF.  If non-null, OBUF is set to the to one
   passed the last parsed char.  Returns null_ptid on error.  */

static ptid_t
read_ptid (char *buf, char **obuf)
{
  char *p = buf;
  char *pp;
  ULONGEST pid = 0, tid = 0;

  if (*p == 'p')
    {
      /* Multi-process ptid.  */
      pp = unpack_varlen_hex (p + 1, &pid);
      if (*pp != '.')
	error (_("invalid remote ptid: %s"), p);

      p = pp;
      pp = unpack_varlen_hex (p + 1, &tid);
      if (obuf)
	*obuf = pp;
      return ptid_build (pid, tid, 0);
    }

  /* No multi-process.  Just a tid.  */
  pp = unpack_varlen_hex (p, &tid);

  /* Return null_ptid when no thread id is found.  */
  if (p == pp)
    {
      if (obuf)
	*obuf = pp;
      return null_ptid;
    }

  /* Since the stub is not sending a process id, then default to
     what's in inferior_ptid, unless it's null at this point.  If so,
     then since there's no way to know the pid of the reported
     threads, use the magic number.  */
  if (ptid_equal (inferior_ptid, null_ptid))
    pid = ptid_get_pid (magic_null_ptid);
  else
    pid = ptid_get_pid (inferior_ptid);

  if (obuf)
    *obuf = pp;
  return ptid_build (pid, tid, 0);
}

static int
stubhex (int ch)
{
  if (ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;
  if (ch >= '0' && ch <= '9')
    return ch - '0';
  if (ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  return -1;
}

static int
stub_unpack_int (char *buff, int fieldlength)
{
  int nibble;
  int retval = 0;

  while (fieldlength)
    {
      nibble = stubhex (*buff++);
      retval |= nibble;
      fieldlength--;
      if (fieldlength)
	retval = retval << 4;
    }
  return retval;
}

static char *
unpack_nibble (char *buf, int *val)
{
  *val = fromhex (*buf++);
  return buf;
}

static char *
unpack_byte (char *buf, int *value)
{
  *value = stub_unpack_int (buf, 2);
  return buf + 2;
}

static char *
pack_int (char *buf, int value)
{
  buf = pack_hex_byte (buf, (value >> 24) & 0xff);
  buf = pack_hex_byte (buf, (value >> 16) & 0xff);
  buf = pack_hex_byte (buf, (value >> 8) & 0x0ff);
  buf = pack_hex_byte (buf, (value & 0xff));
  return buf;
}

static char *
unpack_int (char *buf, int *value)
{
  *value = stub_unpack_int (buf, 8);
  return buf + 8;
}

#if 0			/* Currently unused, uncomment when needed.  */
static char *pack_string (char *pkt, char *string);

static char *
pack_string (char *pkt, char *string)
{
  char ch;
  int len;

  len = strlen (string);
  if (len > 200)
    len = 200;		/* Bigger than most GDB packets, junk???  */
  pkt = pack_hex_byte (pkt, len);
  while (len-- > 0)
    {
      ch = *string++;
      if ((ch == '\0') || (ch == '#'))
	ch = '*';		/* Protect encapsulation.  */
      *pkt++ = ch;
    }
  return pkt;
}
#endif /* 0 (unused) */

static char *
unpack_string (char *src, char *dest, int length)
{
  while (length--)
    *dest++ = *src++;
  *dest = '\0';
  return src;
}

static char *
pack_threadid (char *pkt, threadref *id)
{
  char *limit;
  unsigned char *altid;

  altid = (unsigned char *) id;
  limit = pkt + BUF_THREAD_ID_SIZE;
  while (pkt < limit)
    pkt = pack_hex_byte (pkt, *altid++);
  return pkt;
}


static char *
unpack_threadid (char *inbuf, threadref *id)
{
  char *altref;
  char *limit = inbuf + BUF_THREAD_ID_SIZE;
  int x, y;

  altref = (char *) id;

  while (inbuf < limit)
    {
      x = stubhex (*inbuf++);
      y = stubhex (*inbuf++);
      *altref++ = (x << 4) | y;
    }
  return inbuf;
}

/* Externally, threadrefs are 64 bits but internally, they are still
   ints.  This is due to a mismatch of specifications.  We would like
   to use 64bit thread references internally.  This is an adapter
   function.  */

void
int_to_threadref (threadref *id, int value)
{
  unsigned char *scan;

  scan = (unsigned char *) id;
  {
    int i = 4;
    while (i--)
      *scan++ = 0;
  }
  *scan++ = (value >> 24) & 0xff;
  *scan++ = (value >> 16) & 0xff;
  *scan++ = (value >> 8) & 0xff;
  *scan++ = (value & 0xff);
}

static int
threadref_to_int (threadref *ref)
{
  int i, value = 0;
  unsigned char *scan;

  scan = *ref;
  scan += 4;
  i = 4;
  while (i-- > 0)
    value = (value << 8) | ((*scan++) & 0xff);
  return value;
}

static void
copy_threadref (threadref *dest, threadref *src)
{
  int i;
  unsigned char *csrc, *cdest;

  csrc = (unsigned char *) src;
  cdest = (unsigned char *) dest;
  i = 8;
  while (i--)
    *cdest++ = *csrc++;
}

static int
threadmatch (threadref *dest, threadref *src)
{
  /* Things are broken right now, so just assume we got a match.  */
#if 0
  unsigned char *srcp, *destp;
  int i, result;
  srcp = (char *) src;
  destp = (char *) dest;

  result = 1;
  while (i-- > 0)
    result &= (*srcp++ == *destp++) ? 1 : 0;
  return result;
#endif
  return 1;
}

/*
   threadid:1,        # always request threadid
   context_exists:2,
   display:4,
   unique_name:8,
   more_display:16
 */

/* Encoding:  'Q':8,'P':8,mask:32,threadid:64 */

static char *
pack_threadinfo_request (char *pkt, int mode, threadref *id)
{
  *pkt++ = 'q';				/* Info Query */
  *pkt++ = 'P';				/* process or thread info */
  pkt = pack_int (pkt, mode);		/* mode */
  pkt = pack_threadid (pkt, id);	/* threadid */
  *pkt = '\0';				/* terminate */
  return pkt;
}

/* These values tag the fields in a thread info response packet.  */
/* Tagging the fields allows us to request specific fields and to
   add more fields as time goes by.  */

#define TAG_THREADID 1		/* Echo the thread identifier.  */
#define TAG_EXISTS 2		/* Is this process defined enough to
				   fetch registers and its stack?  */
#define TAG_DISPLAY 4		/* A short thing maybe to put on a window */
#define TAG_THREADNAME 8	/* string, maps 1-to-1 with a thread is.  */
#define TAG_MOREDISPLAY 16	/* Whatever the kernel wants to say about
				   the process.  */

static int
remote_unpack_thread_info_response (char *pkt, threadref *expectedref,
				    struct gdb_ext_thread_info *info)
{
  struct remote_state *rs = get_remote_state ();
  int mask, length;
  int tag;
  threadref ref;
  char *limit = pkt + rs->buf_size; /* Plausible parsing limit.  */
  int retval = 1;

  /* info->threadid = 0; FIXME: implement zero_threadref.  */
  info->active = 0;
  info->display[0] = '\0';
  info->shortname[0] = '\0';
  info->more_display[0] = '\0';

  /* Assume the characters indicating the packet type have been
     stripped.  */
  pkt = unpack_int (pkt, &mask);	/* arg mask */
  pkt = unpack_threadid (pkt, &ref);

  if (mask == 0)
    warning (_("Incomplete response to threadinfo request."));
  if (!threadmatch (&ref, expectedref))
    {			/* This is an answer to a different request.  */
      warning (_("ERROR RMT Thread info mismatch."));
      return 0;
    }
  copy_threadref (&info->threadid, &ref);

  /* Loop on tagged fields , try to bail if somthing goes wrong.  */

  /* Packets are terminated with nulls.  */
  while ((pkt < limit) && mask && *pkt)
    {
      pkt = unpack_int (pkt, &tag);	/* tag */
      pkt = unpack_byte (pkt, &length);	/* length */
      if (!(tag & mask))		/* Tags out of synch with mask.  */
	{
	  warning (_("ERROR RMT: threadinfo tag mismatch."));
	  retval = 0;
	  break;
	}
      if (tag == TAG_THREADID)
	{
	  if (length != 16)
	    {
	      warning (_("ERROR RMT: length of threadid is not 16."));
	      retval = 0;
	      break;
	    }
	  pkt = unpack_threadid (pkt, &ref);
	  mask = mask & ~TAG_THREADID;
	  continue;
	}
      if (tag == TAG_EXISTS)
	{
	  info->active = stub_unpack_int (pkt, length);
	  pkt += length;
	  mask = mask & ~(TAG_EXISTS);
	  if (length > 8)
	    {
	      warning (_("ERROR RMT: 'exists' length too long."));
	      retval = 0;
	      break;
	    }
	  continue;
	}
      if (tag == TAG_THREADNAME)
	{
	  pkt = unpack_string (pkt, &info->shortname[0], length);
	  mask = mask & ~TAG_THREADNAME;
	  continue;
	}
      if (tag == TAG_DISPLAY)
	{
	  pkt = unpack_string (pkt, &info->display[0], length);
	  mask = mask & ~TAG_DISPLAY;
	  continue;
	}
      if (tag == TAG_MOREDISPLAY)
	{
	  pkt = unpack_string (pkt, &info->more_display[0], length);
	  mask = mask & ~TAG_MOREDISPLAY;
	  continue;
	}
      warning (_("ERROR RMT: unknown thread info tag."));
      break;			/* Not a tag we know about.  */
    }
  return retval;
}

static int
remote_get_threadinfo (threadref *threadid, int fieldset,	/* TAG mask */
		       struct gdb_ext_thread_info *info)
{
  struct remote_state *rs = get_remote_state ();
  int result;

  pack_threadinfo_request (rs->buf, fieldset, threadid);
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  if (rs->buf[0] == '\0')
    return 0;

  result = remote_unpack_thread_info_response (rs->buf + 2,
					       threadid, info);
  return result;
}

/*    Format: i'Q':8,i"L":8,initflag:8,batchsize:16,lastthreadid:32   */

static char *
pack_threadlist_request (char *pkt, int startflag, int threadcount,
			 threadref *nextthread)
{
  *pkt++ = 'q';			/* info query packet */
  *pkt++ = 'L';			/* Process LIST or threadLIST request */
  pkt = pack_nibble (pkt, startflag);		/* initflag 1 bytes */
  pkt = pack_hex_byte (pkt, threadcount);	/* threadcount 2 bytes */
  pkt = pack_threadid (pkt, nextthread);	/* 64 bit thread identifier */
  *pkt = '\0';
  return pkt;
}

/* Encoding:   'q':8,'M':8,count:16,done:8,argthreadid:64,(threadid:64)* */

static int
parse_threadlist_response (char *pkt, int result_limit,
			   threadref *original_echo, threadref *resultlist,
			   int *doneflag)
{
  struct remote_state *rs = get_remote_state ();
  char *limit;
  int count, resultcount, done;

  resultcount = 0;
  /* Assume the 'q' and 'M chars have been stripped.  */
  limit = pkt + (rs->buf_size - BUF_THREAD_ID_SIZE);
  /* done parse past here */
  pkt = unpack_byte (pkt, &count);	/* count field */
  pkt = unpack_nibble (pkt, &done);
  /* The first threadid is the argument threadid.  */
  pkt = unpack_threadid (pkt, original_echo);	/* should match query packet */
  while ((count-- > 0) && (pkt < limit))
    {
      pkt = unpack_threadid (pkt, resultlist++);
      if (resultcount++ >= result_limit)
	break;
    }
  if (doneflag)
    *doneflag = done;
  return resultcount;
}

/* Fetch the next batch of threads from the remote.  Returns -1 if the
   qL packet is not supported, 0 on error and 1 on success.  */

static int
remote_get_threadlist (int startflag, threadref *nextthread, int result_limit,
		       int *done, int *result_count, threadref *threadlist)
{
  struct remote_state *rs = get_remote_state ();
  int result = 1;

  /* Trancate result limit to be smaller than the packet size.  */
  if ((((result_limit + 1) * BUF_THREAD_ID_SIZE) + 10)
      >= get_remote_packet_size ())
    result_limit = (get_remote_packet_size () / BUF_THREAD_ID_SIZE) - 2;

  pack_threadlist_request (rs->buf, startflag, result_limit, nextthread);
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);
  if (*rs->buf == '\0')
    {
      /* Packet not supported.  */
      return -1;
    }

  *result_count =
    parse_threadlist_response (rs->buf + 2, result_limit,
			       &rs->echo_nextthread, threadlist, done);

  if (!threadmatch (&rs->echo_nextthread, nextthread))
    {
      /* FIXME: This is a good reason to drop the packet.  */
      /* Possably, there is a duplicate response.  */
      /* Possabilities :
         retransmit immediatly - race conditions
         retransmit after timeout - yes
         exit
         wait for packet, then exit
       */
      warning (_("HMM: threadlist did not echo arg thread, dropping it."));
      return 0;			/* I choose simply exiting.  */
    }
  if (*result_count <= 0)
    {
      if (*done != 1)
	{
	  warning (_("RMT ERROR : failed to get remote thread list."));
	  result = 0;
	}
      return result;		/* break; */
    }
  if (*result_count > result_limit)
    {
      *result_count = 0;
      warning (_("RMT ERROR: threadlist response longer than requested."));
      return 0;
    }
  return result;
}

/* Fetch the list of remote threads, with the qL packet, and call
   STEPFUNCTION for each thread found.  Stops iterating and returns 1
   if STEPFUNCTION returns true.  Stops iterating and returns 0 if the
   STEPFUNCTION returns false.  If the packet is not supported,
   returns -1.  */

static int
remote_threadlist_iterator (rmt_thread_action stepfunction, void *context,
			    int looplimit)
{
  struct remote_state *rs = get_remote_state ();
  int done, i, result_count;
  int startflag = 1;
  int result = 1;
  int loopcount = 0;

  done = 0;
  while (!done)
    {
      if (loopcount++ > looplimit)
	{
	  result = 0;
	  warning (_("Remote fetch threadlist -infinite loop-."));
	  break;
	}
      result = remote_get_threadlist (startflag, &rs->nextthread,
				      MAXTHREADLISTRESULTS,
				      &done, &result_count,
				      rs->resultthreadlist);
      if (result <= 0)
	break;
      /* Clear for later iterations.  */
      startflag = 0;
      /* Setup to resume next batch of thread references, set nextthread.  */
      if (result_count >= 1)
	copy_threadref (&rs->nextthread,
			&rs->resultthreadlist[result_count - 1]);
      i = 0;
      while (result_count--)
	{
	  if (!(*stepfunction) (&rs->resultthreadlist[i++], context))
	    {
	      result = 0;
	      break;
	    }
	}
    }
  return result;
}

/* A thread found on the remote target.  */

typedef struct thread_item
{
  /* The thread's PTID.  */
  ptid_t ptid;

  /* The thread's extra info.  May be NULL.  */
  char *extra;

  /* The thread's name.  May be NULL.  */
  char *name;

  /* The core the thread was running on.  -1 if not known.  */
  int core;
} thread_item_t;
DEF_VEC_O(thread_item_t);

/* Context passed around to the various methods listing remote
   threads.  As new threads are found, they're added to the ITEMS
   vector.  */

struct threads_listing_context
{
  /* The threads found on the remote target.  */
  VEC (thread_item_t) *items;
};

/* Discard the contents of the constructed thread listing context.  */

static void
clear_threads_listing_context (void *p)
{
  struct threads_listing_context *context
    = (struct threads_listing_context *) p;
  int i;
  struct thread_item *item;

  for (i = 0; VEC_iterate (thread_item_t, context->items, i, item); ++i)
    {
      xfree (item->extra);
      xfree (item->name);
    }

  VEC_free (thread_item_t, context->items);
}

/* Remove the thread specified as the related_pid field of WS
   from the CONTEXT list.  */

static void
threads_listing_context_remove (struct target_waitstatus *ws,
				struct threads_listing_context *context)
{
  struct thread_item *item;
  int i;
  ptid_t child_ptid = ws->value.related_pid;

  for (i = 0; VEC_iterate (thread_item_t, context->items, i, item); ++i)
    {
      if (ptid_equal (item->ptid, child_ptid))
	{
	  VEC_ordered_remove (thread_item_t, context->items, i);
	  break;
	}
    }
}

static int
remote_newthread_step (threadref *ref, void *data)
{
  struct threads_listing_context *context
    = (struct threads_listing_context *) data;
  struct thread_item item;
  int pid = ptid_get_pid (inferior_ptid);

  item.ptid = ptid_build (pid, threadref_to_int (ref), 0);
  item.core = -1;
  item.name = NULL;
  item.extra = NULL;

  VEC_safe_push (thread_item_t, context->items, &item);

  return 1;			/* continue iterator */
}

#define CRAZY_MAX_THREADS 1000

static ptid_t
remote_current_thread (ptid_t oldpid)
{
  struct remote_state *rs = get_remote_state ();

  putpkt ("qC");
  getpkt (&rs->buf, &rs->buf_size, 0);
  if (rs->buf[0] == 'Q' && rs->buf[1] == 'C')
    {
      char *obuf;
      ptid_t result;

      result = read_ptid (&rs->buf[2], &obuf);
      if (*obuf != '\0' && remote_debug)
        fprintf_unfiltered (gdb_stdlog,
	                    "warning: garbage in qC reply\n");

      return result;
    }
  else
    return oldpid;
}

/* List remote threads using the deprecated qL packet.  */

static int
remote_get_threads_with_ql (struct target_ops *ops,
			    struct threads_listing_context *context)
{
  if (remote_threadlist_iterator (remote_newthread_step, context,
				  CRAZY_MAX_THREADS) >= 0)
    return 1;

  return 0;
}

#if defined(HAVE_LIBEXPAT)

static void
start_thread (struct gdb_xml_parser *parser,
	      const struct gdb_xml_element *element,
	      void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct threads_listing_context *data
    = (struct threads_listing_context *) user_data;

  struct thread_item item;
  char *id;
  struct gdb_xml_value *attr;

  id = (char *) xml_find_attribute (attributes, "id")->value;
  item.ptid = read_ptid (id, NULL);

  attr = xml_find_attribute (attributes, "core");
  if (attr != NULL)
    item.core = *(ULONGEST *) attr->value;
  else
    item.core = -1;

  attr = xml_find_attribute (attributes, "name");
  item.name = attr != NULL ? xstrdup ((const char *) attr->value) : NULL;

  item.extra = 0;

  VEC_safe_push (thread_item_t, data->items, &item);
}

static void
end_thread (struct gdb_xml_parser *parser,
	    const struct gdb_xml_element *element,
	    void *user_data, const char *body_text)
{
  struct threads_listing_context *data
    = (struct threads_listing_context *) user_data;

  if (body_text && *body_text)
    VEC_last (thread_item_t, data->items)->extra = xstrdup (body_text);
}

const struct gdb_xml_attribute thread_attributes[] = {
  { "id", GDB_XML_AF_NONE, NULL, NULL },
  { "core", GDB_XML_AF_OPTIONAL, gdb_xml_parse_attr_ulongest, NULL },
  { "name", GDB_XML_AF_OPTIONAL, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

const struct gdb_xml_element thread_children[] = {
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

const struct gdb_xml_element threads_children[] = {
  { "thread", thread_attributes, thread_children,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    start_thread, end_thread },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

const struct gdb_xml_element threads_elements[] = {
  { "threads", NULL, threads_children,
    GDB_XML_EF_NONE, NULL, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

#endif

/* List remote threads using qXfer:threads:read.  */

static int
remote_get_threads_with_qxfer (struct target_ops *ops,
			       struct threads_listing_context *context)
{
#if defined(HAVE_LIBEXPAT)
  if (packet_support (PACKET_qXfer_threads) == PACKET_ENABLE)
    {
      char *xml = target_read_stralloc (ops, TARGET_OBJECT_THREADS, NULL);
      struct cleanup *back_to = make_cleanup (xfree, xml);

      if (xml != NULL && *xml != '\0')
	{
	  gdb_xml_parse_quick (_("threads"), "threads.dtd",
			       threads_elements, xml, context);
	}

      do_cleanups (back_to);
      return 1;
    }
#endif

  return 0;
}

/* List remote threads using qfThreadInfo/qsThreadInfo.  */

static int
remote_get_threads_with_qthreadinfo (struct target_ops *ops,
				     struct threads_listing_context *context)
{
  struct remote_state *rs = get_remote_state ();

  if (rs->use_threadinfo_query)
    {
      char *bufp;

      putpkt ("qfThreadInfo");
      getpkt (&rs->buf, &rs->buf_size, 0);
      bufp = rs->buf;
      if (bufp[0] != '\0')		/* q packet recognized */
	{
	  while (*bufp++ == 'm')	/* reply contains one or more TID */
	    {
	      do
		{
		  struct thread_item item;

		  item.ptid = read_ptid (bufp, &bufp);
		  item.core = -1;
		  item.name = NULL;
		  item.extra = NULL;

		  VEC_safe_push (thread_item_t, context->items, &item);
		}
	      while (*bufp++ == ',');	/* comma-separated list */
	      putpkt ("qsThreadInfo");
	      getpkt (&rs->buf, &rs->buf_size, 0);
	      bufp = rs->buf;
	    }
	  return 1;
	}
      else
	{
	  /* Packet not recognized.  */
	  rs->use_threadinfo_query = 0;
	}
    }

  return 0;
}

/* Implement the to_update_thread_list function for the remote
   targets.  */

static void
remote_update_thread_list (struct target_ops *ops)
{
  struct threads_listing_context context;
  struct cleanup *old_chain;
  int got_list = 0;

  context.items = NULL;
  old_chain = make_cleanup (clear_threads_listing_context, &context);

  /* We have a few different mechanisms to fetch the thread list.  Try
     them all, starting with the most preferred one first, falling
     back to older methods.  */
  if (remote_get_threads_with_qxfer (ops, &context)
      || remote_get_threads_with_qthreadinfo (ops, &context)
      || remote_get_threads_with_ql (ops, &context))
    {
      int i;
      struct thread_item *item;
      struct thread_info *tp, *tmp;

      got_list = 1;

      if (VEC_empty (thread_item_t, context.items)
	  && remote_thread_always_alive (ops, inferior_ptid))
	{
	  /* Some targets don't really support threads, but still
	     reply an (empty) thread list in response to the thread
	     listing packets, instead of replying "packet not
	     supported".  Exit early so we don't delete the main
	     thread.  */
	  do_cleanups (old_chain);
	  return;
	}

      /* CONTEXT now holds the current thread list on the remote
	 target end.  Delete GDB-side threads no longer found on the
	 target.  */
      ALL_THREADS_SAFE (tp, tmp)
	{
	  for (i = 0;
	       VEC_iterate (thread_item_t, context.items, i, item);
	       ++i)
	    {
	      if (ptid_equal (item->ptid, tp->ptid))
		break;
	    }

	  if (i == VEC_length (thread_item_t, context.items))
	    {
	      /* Not found.  */
	      delete_thread (tp->ptid);
	    }
	}

      /* Remove any unreported fork child threads from CONTEXT so
	 that we don't interfere with follow fork, which is where
	 creation of such threads is handled.  */
      remove_new_fork_children (&context);

      /* And now add threads we don't know about yet to our list.  */
      for (i = 0;
	   VEC_iterate (thread_item_t, context.items, i, item);
	   ++i)
	{
	  if (!ptid_equal (item->ptid, null_ptid))
	    {
	      struct private_thread_info *info;
	      /* In non-stop mode, we assume new found threads are
		 executing until proven otherwise with a stop reply.
		 In all-stop, we can only get here if all threads are
		 stopped.  */
	      int executing = target_is_non_stop_p () ? 1 : 0;

	      remote_notice_new_inferior (item->ptid, executing);

	      info = demand_private_info (item->ptid);
	      info->core = item->core;
	      info->extra = item->extra;
	      item->extra = NULL;
	      info->name = item->name;
	      item->name = NULL;
	    }
	}
    }

  if (!got_list)
    {
      /* If no thread listing method is supported, then query whether
	 each known thread is alive, one by one, with the T packet.
	 If the target doesn't support threads at all, then this is a
	 no-op.  See remote_thread_alive.  */
      prune_threads ();
    }

  do_cleanups (old_chain);
}

/*
 * Collect a descriptive string about the given thread.
 * The target may say anything it wants to about the thread
 * (typically info about its blocked / runnable state, name, etc.).
 * This string will appear in the info threads display.
 *
 * Optional: targets are not required to implement this function.
 */

static char *
remote_threads_extra_info (struct target_ops *self, struct thread_info *tp)
{
  struct remote_state *rs = get_remote_state ();
  int result;
  int set;
  threadref id;
  struct gdb_ext_thread_info threadinfo;
  static char display_buf[100];	/* arbitrary...  */
  int n = 0;                    /* position in display_buf */

  if (rs->remote_desc == 0)		/* paranoia */
    internal_error (__FILE__, __LINE__,
		    _("remote_threads_extra_info"));

  if (ptid_equal (tp->ptid, magic_null_ptid)
      || (ptid_get_pid (tp->ptid) != 0 && ptid_get_lwp (tp->ptid) == 0))
    /* This is the main thread which was added by GDB.  The remote
       server doesn't know about it.  */
    return NULL;

  if (packet_support (PACKET_qXfer_threads) == PACKET_ENABLE)
    {
      struct thread_info *info = find_thread_ptid (tp->ptid);

      if (info && info->priv)
	return info->priv->extra;
      else
	return NULL;
    }

  if (rs->use_threadextra_query)
    {
      char *b = rs->buf;
      char *endb = rs->buf + get_remote_packet_size ();

      xsnprintf (b, endb - b, "qThreadExtraInfo,");
      b += strlen (b);
      write_ptid (b, endb, tp->ptid);

      putpkt (rs->buf);
      getpkt (&rs->buf, &rs->buf_size, 0);
      if (rs->buf[0] != 0)
	{
	  n = min (strlen (rs->buf) / 2, sizeof (display_buf));
	  result = hex2bin (rs->buf, (gdb_byte *) display_buf, n);
	  display_buf [result] = '\0';
	  return display_buf;
	}
    }

  /* If the above query fails, fall back to the old method.  */
  rs->use_threadextra_query = 0;
  set = TAG_THREADID | TAG_EXISTS | TAG_THREADNAME
    | TAG_MOREDISPLAY | TAG_DISPLAY;
  int_to_threadref (&id, ptid_get_lwp (tp->ptid));
  if (remote_get_threadinfo (&id, set, &threadinfo))
    if (threadinfo.active)
      {
	if (*threadinfo.shortname)
	  n += xsnprintf (&display_buf[0], sizeof (display_buf) - n,
			  " Name: %s,", threadinfo.shortname);
	if (*threadinfo.display)
	  n += xsnprintf (&display_buf[n], sizeof (display_buf) - n,
			  " State: %s,", threadinfo.display);
	if (*threadinfo.more_display)
	  n += xsnprintf (&display_buf[n], sizeof (display_buf) - n,
			  " Priority: %s", threadinfo.more_display);

	if (n > 0)
	  {
	    /* For purely cosmetic reasons, clear up trailing commas.  */
	    if (',' == display_buf[n-1])
	      display_buf[n-1] = ' ';
	    return display_buf;
	  }
      }
  return NULL;
}


static int
remote_static_tracepoint_marker_at (struct target_ops *self, CORE_ADDR addr,
				    struct static_tracepoint_marker *marker)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf;

  xsnprintf (p, get_remote_packet_size (), "qTSTMat:");
  p += strlen (p);
  p += hexnumstr (p, addr);
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);
  p = rs->buf;

  if (*p == 'E')
    error (_("Remote failure reply: %s"), p);

  if (*p++ == 'm')
    {
      parse_static_tracepoint_marker_definition (p, &p, marker);
      return 1;
    }

  return 0;
}

static VEC(static_tracepoint_marker_p) *
remote_static_tracepoint_markers_by_strid (struct target_ops *self,
					   const char *strid)
{
  struct remote_state *rs = get_remote_state ();
  VEC(static_tracepoint_marker_p) *markers = NULL;
  struct static_tracepoint_marker *marker = NULL;
  struct cleanup *old_chain;
  char *p;

  /* Ask for a first packet of static tracepoint marker
     definition.  */
  putpkt ("qTfSTM");
  getpkt (&rs->buf, &rs->buf_size, 0);
  p = rs->buf;
  if (*p == 'E')
    error (_("Remote failure reply: %s"), p);

  old_chain = make_cleanup (free_current_marker, &marker);

  while (*p++ == 'm')
    {
      if (marker == NULL)
	marker = XCNEW (struct static_tracepoint_marker);

      do
	{
	  parse_static_tracepoint_marker_definition (p, &p, marker);

	  if (strid == NULL || strcmp (strid, marker->str_id) == 0)
	    {
	      VEC_safe_push (static_tracepoint_marker_p,
			     markers, marker);
	      marker = NULL;
	    }
	  else
	    {
	      release_static_tracepoint_marker (marker);
	      memset (marker, 0, sizeof (*marker));
	    }
	}
      while (*p++ == ',');	/* comma-separated list */
      /* Ask for another packet of static tracepoint definition.  */
      putpkt ("qTsSTM");
      getpkt (&rs->buf, &rs->buf_size, 0);
      p = rs->buf;
    }

  do_cleanups (old_chain);
  return markers;
}


/* Implement the to_get_ada_task_ptid function for the remote targets.  */

static ptid_t
remote_get_ada_task_ptid (struct target_ops *self, long lwp, long thread)
{
  return ptid_build (ptid_get_pid (inferior_ptid), lwp, 0);
}


/* Restart the remote side; this is an extended protocol operation.  */

static void
extended_remote_restart (void)
{
  struct remote_state *rs = get_remote_state ();

  /* Send the restart command; for reasons I don't understand the
     remote side really expects a number after the "R".  */
  xsnprintf (rs->buf, get_remote_packet_size (), "R%x", 0);
  putpkt (rs->buf);

  remote_fileio_reset ();
}

/* Clean up connection to a remote debugger.  */

static void
remote_close (struct target_ops *self)
{
  struct remote_state *rs = get_remote_state ();

  if (rs->remote_desc == NULL)
    return; /* already closed */

  /* Make sure we leave stdin registered in the event loop.  */
  remote_terminal_ours (self);

  serial_close (rs->remote_desc);
  rs->remote_desc = NULL;

  /* We don't have a connection to the remote stub anymore.  Get rid
     of all the inferiors and their threads we were controlling.
     Reset inferior_ptid to null_ptid first, as otherwise has_stack_frame
     will be unable to find the thread corresponding to (pid, 0, 0).  */
  inferior_ptid = null_ptid;
  discard_all_inferiors ();

  /* We are closing the remote target, so we should discard
     everything of this target.  */
  discard_pending_stop_replies_in_queue (rs);

  if (remote_async_inferior_event_token)
    delete_async_event_handler (&remote_async_inferior_event_token);

  remote_notif_state_xfree (rs->notif_state);

  trace_reset_local_state ();
}

/* Query the remote side for the text, data and bss offsets.  */

static void
get_offsets (void)
{
  struct remote_state *rs = get_remote_state ();
  char *buf;
  char *ptr;
  int lose, num_segments = 0, do_sections, do_segments;
  CORE_ADDR text_addr, data_addr, bss_addr, segments[2];
  struct section_offsets *offs;
  struct symfile_segment_data *data;

  if (symfile_objfile == NULL)
    return;

  putpkt ("qOffsets");
  getpkt (&rs->buf, &rs->buf_size, 0);
  buf = rs->buf;

  if (buf[0] == '\000')
    return;			/* Return silently.  Stub doesn't support
				   this command.  */
  if (buf[0] == 'E')
    {
      warning (_("Remote failure reply: %s"), buf);
      return;
    }

  /* Pick up each field in turn.  This used to be done with scanf, but
     scanf will make trouble if CORE_ADDR size doesn't match
     conversion directives correctly.  The following code will work
     with any size of CORE_ADDR.  */
  text_addr = data_addr = bss_addr = 0;
  ptr = buf;
  lose = 0;

  if (startswith (ptr, "Text="))
    {
      ptr += 5;
      /* Don't use strtol, could lose on big values.  */
      while (*ptr && *ptr != ';')
	text_addr = (text_addr << 4) + fromhex (*ptr++);

      if (startswith (ptr, ";Data="))
	{
	  ptr += 6;
	  while (*ptr && *ptr != ';')
	    data_addr = (data_addr << 4) + fromhex (*ptr++);
	}
      else
	lose = 1;

      if (!lose && startswith (ptr, ";Bss="))
	{
	  ptr += 5;
	  while (*ptr && *ptr != ';')
	    bss_addr = (bss_addr << 4) + fromhex (*ptr++);

	  if (bss_addr != data_addr)
	    warning (_("Target reported unsupported offsets: %s"), buf);
	}
      else
	lose = 1;
    }
  else if (startswith (ptr, "TextSeg="))
    {
      ptr += 8;
      /* Don't use strtol, could lose on big values.  */
      while (*ptr && *ptr != ';')
	text_addr = (text_addr << 4) + fromhex (*ptr++);
      num_segments = 1;

      if (startswith (ptr, ";DataSeg="))
	{
	  ptr += 9;
	  while (*ptr && *ptr != ';')
	    data_addr = (data_addr << 4) + fromhex (*ptr++);
	  num_segments++;
	}
    }
  else
    lose = 1;

  if (lose)
    error (_("Malformed response to offset query, %s"), buf);
  else if (*ptr != '\0')
    warning (_("Target reported unsupported offsets: %s"), buf);

  offs = ((struct section_offsets *)
	  alloca (SIZEOF_N_SECTION_OFFSETS (symfile_objfile->num_sections)));
  memcpy (offs, symfile_objfile->section_offsets,
	  SIZEOF_N_SECTION_OFFSETS (symfile_objfile->num_sections));

  data = get_symfile_segment_data (symfile_objfile->obfd);
  do_segments = (data != NULL);
  do_sections = num_segments == 0;

  if (num_segments > 0)
    {
      segments[0] = text_addr;
      segments[1] = data_addr;
    }
  /* If we have two segments, we can still try to relocate everything
     by assuming that the .text and .data offsets apply to the whole
     text and data segments.  Convert the offsets given in the packet
     to base addresses for symfile_map_offsets_to_segments.  */
  else if (data && data->num_segments == 2)
    {
      segments[0] = data->segment_bases[0] + text_addr;
      segments[1] = data->segment_bases[1] + data_addr;
      num_segments = 2;
    }
  /* If the object file has only one segment, assume that it is text
     rather than data; main programs with no writable data are rare,
     but programs with no code are useless.  Of course the code might
     have ended up in the data segment... to detect that we would need
     the permissions here.  */
  else if (data && data->num_segments == 1)
    {
      segments[0] = data->segment_bases[0] + text_addr;
      num_segments = 1;
    }
  /* There's no way to relocate by segment.  */
  else
    do_segments = 0;

  if (do_segments)
    {
      int ret = symfile_map_offsets_to_segments (symfile_objfile->obfd, data,
						 offs, num_segments, segments);

      if (ret == 0 && !do_sections)
	error (_("Can not handle qOffsets TextSeg "
		 "response with this symbol file"));

      if (ret > 0)
	do_sections = 0;
    }

  if (data)
    free_symfile_segment_data (data);

  if (do_sections)
    {
      offs->offsets[SECT_OFF_TEXT (symfile_objfile)] = text_addr;

      /* This is a temporary kludge to force data and bss to use the
	 same offsets because that's what nlmconv does now.  The real
	 solution requires changes to the stub and remote.c that I
	 don't have time to do right now.  */

      offs->offsets[SECT_OFF_DATA (symfile_objfile)] = data_addr;
      offs->offsets[SECT_OFF_BSS (symfile_objfile)] = data_addr;
    }

  objfile_relocate (symfile_objfile, offs);
}

/* Send interrupt_sequence to remote target.  */
static void
send_interrupt_sequence (void)
{
  struct remote_state *rs = get_remote_state ();

  if (interrupt_sequence_mode == interrupt_sequence_control_c)
    remote_serial_write ("\x03", 1);
  else if (interrupt_sequence_mode == interrupt_sequence_break)
    serial_send_break (rs->remote_desc);
  else if (interrupt_sequence_mode == interrupt_sequence_break_g)
    {
      serial_send_break (rs->remote_desc);
      remote_serial_write ("g", 1);
    }
  else
    internal_error (__FILE__, __LINE__,
		    _("Invalid value for interrupt_sequence_mode: %s."),
		    interrupt_sequence_mode);
}


/* If STOP_REPLY is a T stop reply, look for the "thread" register,
   and extract the PTID.  Returns NULL_PTID if not found.  */

static ptid_t
stop_reply_extract_thread (char *stop_reply)
{
  if (stop_reply[0] == 'T' && strlen (stop_reply) > 3)
    {
      char *p;

      /* Txx r:val ; r:val (...)  */
      p = &stop_reply[3];

      /* Look for "register" named "thread".  */
      while (*p != '\0')
	{
	  char *p1;

	  p1 = strchr (p, ':');
	  if (p1 == NULL)
	    return null_ptid;

	  if (strncmp (p, "thread", p1 - p) == 0)
	    return read_ptid (++p1, &p);

	  p1 = strchr (p, ';');
	  if (p1 == NULL)
	    return null_ptid;
	  p1++;

	  p = p1;
	}
    }

  return null_ptid;
}

/* Determine the remote side's current thread.  If we have a stop
   reply handy (in WAIT_STATUS), maybe it's a T stop reply with a
   "thread" register we can extract the current thread from.  If not,
   ask the remote which is the current thread with qC.  The former
   method avoids a roundtrip.  */

static ptid_t
get_current_thread (char *wait_status)
{
  ptid_t ptid = null_ptid;

  /* Note we don't use remote_parse_stop_reply as that makes use of
     the target architecture, which we haven't yet fully determined at
     this point.  */
  if (wait_status != NULL)
    ptid = stop_reply_extract_thread (wait_status);
  if (ptid_equal (ptid, null_ptid))
    ptid = remote_current_thread (inferior_ptid);

  return ptid;
}

/* Query the remote target for which is the current thread/process,
   add it to our tables, and update INFERIOR_PTID.  The caller is
   responsible for setting the state such that the remote end is ready
   to return the current thread.

   This function is called after handling the '?' or 'vRun' packets,
   whose response is a stop reply from which we can also try
   extracting the thread.  If the target doesn't support the explicit
   qC query, we infer the current thread from that stop reply, passed
   in in WAIT_STATUS, which may be NULL.  */

static void
add_current_inferior_and_thread (char *wait_status)
{
  struct remote_state *rs = get_remote_state ();
  int fake_pid_p = 0;
  ptid_t ptid;

  inferior_ptid = null_ptid;

  /* Now, if we have thread information, update inferior_ptid.  */
  ptid = get_current_thread (wait_status);

  if (!ptid_equal (ptid, null_ptid))
    {
      if (!remote_multi_process_p (rs))
	fake_pid_p = 1;

      inferior_ptid = ptid;
    }
  else
    {
      /* Without this, some commands which require an active target
	 (such as kill) won't work.  This variable serves (at least)
	 double duty as both the pid of the target process (if it has
	 such), and as a flag indicating that a target is active.  */
      inferior_ptid = magic_null_ptid;
      fake_pid_p = 1;
    }

  remote_add_inferior (fake_pid_p, ptid_get_pid (inferior_ptid), -1, 1);

  /* Add the main thread.  */
  add_thread_silent (inferior_ptid);
}

/* Print info about a thread that was found already stopped on
   connection.  */

static void
print_one_stopped_thread (struct thread_info *thread)
{
  struct target_waitstatus *ws = &thread->suspend.waitstatus;

  switch_to_thread (thread->ptid);
  stop_pc = get_frame_pc (get_current_frame ());
  set_current_sal_from_frame (get_current_frame ());

  thread->suspend.waitstatus_pending_p = 0;

  if (ws->kind == TARGET_WAITKIND_STOPPED)
    {
      enum gdb_signal sig = ws->value.sig;

      if (signal_print_state (sig))
	observer_notify_signal_received (sig);
    }
  observer_notify_normal_stop (NULL, 1);
}

/* Process all initial stop replies the remote side sent in response
   to the ? packet.  These indicate threads that were already stopped
   on initial connection.  We mark these threads as stopped and print
   their current frame before giving the user the prompt.  */

static void
process_initial_stop_replies (int from_tty)
{
  int pending_stop_replies = stop_reply_queue_length ();
  struct inferior *inf;
  struct thread_info *thread;
  struct thread_info *selected = NULL;
  struct thread_info *lowest_stopped = NULL;
  struct thread_info *first = NULL;

  /* Consume the initial pending events.  */
  while (pending_stop_replies-- > 0)
    {
      ptid_t waiton_ptid = minus_one_ptid;
      ptid_t event_ptid;
      struct target_waitstatus ws;
      int ignore_event = 0;
      struct thread_info *thread;

      memset (&ws, 0, sizeof (ws));
      event_ptid = target_wait (waiton_ptid, &ws, TARGET_WNOHANG);
      if (remote_debug)
	print_target_wait_results (waiton_ptid, event_ptid, &ws);

      switch (ws.kind)
	{
	case TARGET_WAITKIND_IGNORE:
	case TARGET_WAITKIND_NO_RESUMED:
	case TARGET_WAITKIND_SIGNALLED:
	case TARGET_WAITKIND_EXITED:
	  /* We shouldn't see these, but if we do, just ignore.  */
	  if (remote_debug)
	    fprintf_unfiltered (gdb_stdlog, "remote: event ignored\n");
	  ignore_event = 1;
	  break;

	case TARGET_WAITKIND_EXECD:
	  xfree (ws.value.execd_pathname);
	  break;
	default:
	  break;
	}

      if (ignore_event)
	continue;

      thread = find_thread_ptid (event_ptid);

      if (ws.kind == TARGET_WAITKIND_STOPPED)
	{
	  enum gdb_signal sig = ws.value.sig;

	  /* Stubs traditionally report SIGTRAP as initial signal,
	     instead of signal 0.  Suppress it.  */
	  if (sig == GDB_SIGNAL_TRAP)
	    sig = GDB_SIGNAL_0;
	  thread->suspend.stop_signal = sig;
	  ws.value.sig = sig;
	}

      thread->suspend.waitstatus = ws;

      if (ws.kind != TARGET_WAITKIND_STOPPED
	  || ws.value.sig != GDB_SIGNAL_0)
	thread->suspend.waitstatus_pending_p = 1;

      set_executing (event_ptid, 0);
      set_running (event_ptid, 0);
    }

  /* "Notice" the new inferiors before anything related to
     registers/memory.  */
  ALL_INFERIORS (inf)
    {
      if (inf->pid == 0)
	continue;

      inf->needs_setup = 1;

      if (non_stop)
	{
	  thread = any_live_thread_of_process (inf->pid);
	  notice_new_inferior (thread->ptid,
			       thread->state == THREAD_RUNNING,
			       from_tty);
	}
    }

  /* If all-stop on top of non-stop, pause all threads.  Note this
     records the threads' stop pc, so must be done after "noticing"
     the inferiors.  */
  if (!non_stop)
    {
      stop_all_threads ();

      /* If all threads of an inferior were already stopped, we
	 haven't setup the inferior yet.  */
      ALL_INFERIORS (inf)
	{
	  if (inf->pid == 0)
	    continue;

	  if (inf->needs_setup)
	    {
	      thread = any_live_thread_of_process (inf->pid);
	      switch_to_thread_no_regs (thread);
	      setup_inferior (0);
	    }
	}
    }

  /* Now go over all threads that are stopped, and print their current
     frame.  If all-stop, then if there's a signalled thread, pick
     that as current.  */
  ALL_NON_EXITED_THREADS (thread)
    {
      if (first == NULL)
	first = thread;

      if (!non_stop)
	set_running (thread->ptid, 0);
      else if (thread->state != THREAD_STOPPED)
	continue;

      if (selected == NULL
	  && thread->suspend.waitstatus_pending_p)
	selected = thread;

      if (lowest_stopped == NULL
	  || thread->inf->num < lowest_stopped->inf->num
	  || thread->per_inf_num < lowest_stopped->per_inf_num)
	lowest_stopped = thread;

      if (non_stop)
	print_one_stopped_thread (thread);
    }

  /* In all-stop, we only print the status of one thread, and leave
     others with their status pending.  */
  if (!non_stop)
    {
      thread = selected;
      if (thread == NULL)
	thread = lowest_stopped;
      if (thread == NULL)
	thread = first;

      print_one_stopped_thread (thread);
    }

  /* For "info program".  */
  thread = inferior_thread ();
  if (thread->state == THREAD_STOPPED)
    set_last_target_status (inferior_ptid, thread->suspend.waitstatus);
}

/* Start the remote connection and sync state.  */

static void
remote_start_remote (int from_tty, struct target_ops *target, int extended_p)
{
  struct remote_state *rs = get_remote_state ();
  struct packet_config *noack_config;
  char *wait_status = NULL;

  /* Signal other parts that we're going through the initial setup,
     and so things may not be stable yet.  E.g., we don't try to
     install tracepoints until we've relocated symbols.  Also, a
     Ctrl-C before we're connected and synced up can't interrupt the
     target.  Instead, it offers to drop the (potentially wedged)
     connection.  */
  rs->starting_up = 1;

  QUIT;

  if (interrupt_on_connect)
    send_interrupt_sequence ();

  /* Ack any packet which the remote side has already sent.  */
  remote_serial_write ("+", 1);

  /* The first packet we send to the target is the optional "supported
     packets" request.  If the target can answer this, it will tell us
     which later probes to skip.  */
  remote_query_supported ();

  /* If the stub wants to get a QAllow, compose one and send it.  */
  if (packet_support (PACKET_QAllow) != PACKET_DISABLE)
    remote_set_permissions (target);

  /* gdbserver < 7.7 (before its fix from 2013-12-11) did reply to any
     unknown 'v' packet with string "OK".  "OK" gets interpreted by GDB
     as a reply to known packet.  For packet "vFile:setfs:" it is an
     invalid reply and GDB would return error in
     remote_hostio_set_filesystem, making remote files access impossible.
     Disable "vFile:setfs:" in such case.  Do not disable other 'v' packets as
     other "vFile" packets get correctly detected even on gdbserver < 7.7.  */
  {
    const char v_mustreplyempty[] = "vMustReplyEmpty";

    putpkt (v_mustreplyempty);
    getpkt (&rs->buf, &rs->buf_size, 0);
    if (strcmp (rs->buf, "OK") == 0)
      remote_protocol_packets[PACKET_vFile_setfs].support = PACKET_DISABLE;
    else if (strcmp (rs->buf, "") != 0)
      error (_("Remote replied unexpectedly to '%s': %s"), v_mustreplyempty,
	     rs->buf);
  }

  /* Next, we possibly activate noack mode.

     If the QStartNoAckMode packet configuration is set to AUTO,
     enable noack mode if the stub reported a wish for it with
     qSupported.

     If set to TRUE, then enable noack mode even if the stub didn't
     report it in qSupported.  If the stub doesn't reply OK, the
     session ends with an error.

     If FALSE, then don't activate noack mode, regardless of what the
     stub claimed should be the default with qSupported.  */

  noack_config = &remote_protocol_packets[PACKET_QStartNoAckMode];
  if (packet_config_support (noack_config) != PACKET_DISABLE)
    {
      putpkt ("QStartNoAckMode");
      getpkt (&rs->buf, &rs->buf_size, 0);
      if (packet_ok (rs->buf, noack_config) == PACKET_OK)
	rs->noack_mode = 1;
    }

  if (extended_p)
    {
      /* Tell the remote that we are using the extended protocol.  */
      putpkt ("!");
      getpkt (&rs->buf, &rs->buf_size, 0);
    }

  /* Let the target know which signals it is allowed to pass down to
     the program.  */
  update_signals_program_target ();

  /* Next, if the target can specify a description, read it.  We do
     this before anything involving memory or registers.  */
  target_find_description ();

  /* Next, now that we know something about the target, update the
     address spaces in the program spaces.  */
  update_address_spaces ();

  /* On OSs where the list of libraries is global to all
     processes, we fetch them early.  */
  if (gdbarch_has_global_solist (target_gdbarch ()))
    solib_add (NULL, from_tty, target, auto_solib_add);

  if (target_is_non_stop_p ())
    {
      if (packet_support (PACKET_QNonStop) != PACKET_ENABLE)
	error (_("Non-stop mode requested, but remote "
		 "does not support non-stop"));

      putpkt ("QNonStop:1");
      getpkt (&rs->buf, &rs->buf_size, 0);

      if (strcmp (rs->buf, "OK") != 0)
	error (_("Remote refused setting non-stop mode with: %s"), rs->buf);

      /* Find about threads and processes the stub is already
	 controlling.  We default to adding them in the running state.
	 The '?' query below will then tell us about which threads are
	 stopped.  */
      remote_update_thread_list (target);
    }
  else if (packet_support (PACKET_QNonStop) == PACKET_ENABLE)
    {
      /* Don't assume that the stub can operate in all-stop mode.
	 Request it explicitly.  */
      putpkt ("QNonStop:0");
      getpkt (&rs->buf, &rs->buf_size, 0);

      if (strcmp (rs->buf, "OK") != 0)
	error (_("Remote refused setting all-stop mode with: %s"), rs->buf);
    }

  /* Upload TSVs regardless of whether the target is running or not.  The
     remote stub, such as GDBserver, may have some predefined or builtin
     TSVs, even if the target is not running.  */
  if (remote_get_trace_status (target, current_trace_status ()) != -1)
    {
      struct uploaded_tsv *uploaded_tsvs = NULL;

      remote_upload_trace_state_variables (target, &uploaded_tsvs);
      merge_uploaded_trace_state_variables (&uploaded_tsvs);
    }

  /* Check whether the target is running now.  */
  putpkt ("?");
  getpkt (&rs->buf, &rs->buf_size, 0);

  if (!target_is_non_stop_p ())
    {
      if (rs->buf[0] == 'W' || rs->buf[0] == 'X')
	{
	  if (!extended_p)
	    error (_("The target is not running (try extended-remote?)"));

	  /* We're connected, but not running.  Drop out before we
	     call start_remote.  */
	  rs->starting_up = 0;
	  return;
	}
      else
	{
	  /* Save the reply for later.  */
	  wait_status = (char *) alloca (strlen (rs->buf) + 1);
	  strcpy (wait_status, rs->buf);
	}

      /* Fetch thread list.  */
      target_update_thread_list ();

      /* Let the stub know that we want it to return the thread.  */
      set_continue_thread (minus_one_ptid);

      if (thread_count () == 0)
	{
	  /* Target has no concept of threads at all.  GDB treats
	     non-threaded target as single-threaded; add a main
	     thread.  */
	  add_current_inferior_and_thread (wait_status);
	}
      else
	{
	  /* We have thread information; select the thread the target
	     says should be current.  If we're reconnecting to a
	     multi-threaded program, this will ideally be the thread
	     that last reported an event before GDB disconnected.  */
	  inferior_ptid = get_current_thread (wait_status);
	  if (ptid_equal (inferior_ptid, null_ptid))
	    {
	      /* Odd... The target was able to list threads, but not
		 tell us which thread was current (no "thread"
		 register in T stop reply?).  Just pick the first
		 thread in the thread list then.  */
	      
	      if (remote_debug)
		fprintf_unfiltered (gdb_stdlog,
		                    "warning: couldn't determine remote "
				    "current thread; picking first in list.\n");

	      inferior_ptid = thread_list->ptid;
	    }
	}

      /* init_wait_for_inferior should be called before get_offsets in order
	 to manage `inserted' flag in bp loc in a correct state.
	 breakpoint_init_inferior, called from init_wait_for_inferior, set
	 `inserted' flag to 0, while before breakpoint_re_set, called from
	 start_remote, set `inserted' flag to 1.  In the initialization of
	 inferior, breakpoint_init_inferior should be called first, and then
	 breakpoint_re_set can be called.  If this order is broken, state of
	 `inserted' flag is wrong, and cause some problems on breakpoint
	 manipulation.  */
      init_wait_for_inferior ();

      get_offsets ();		/* Get text, data & bss offsets.  */

      /* If we could not find a description using qXfer, and we know
	 how to do it some other way, try again.  This is not
	 supported for non-stop; it could be, but it is tricky if
	 there are no stopped threads when we connect.  */
      if (remote_read_description_p (target)
	  && gdbarch_target_desc (target_gdbarch ()) == NULL)
	{
	  target_clear_description ();
	  target_find_description ();
	}

      /* Use the previously fetched status.  */
      gdb_assert (wait_status != NULL);
      strcpy (rs->buf, wait_status);
      rs->cached_wait_status = 1;

      start_remote (from_tty); /* Initialize gdb process mechanisms.  */
    }
  else
    {
      /* Clear WFI global state.  Do this before finding about new
	 threads and inferiors, and setting the current inferior.
	 Otherwise we would clear the proceed status of the current
	 inferior when we want its stop_soon state to be preserved
	 (see notice_new_inferior).  */
      init_wait_for_inferior ();

      /* In non-stop, we will either get an "OK", meaning that there
	 are no stopped threads at this time; or, a regular stop
	 reply.  In the latter case, there may be more than one thread
	 stopped --- we pull them all out using the vStopped
	 mechanism.  */
      if (strcmp (rs->buf, "OK") != 0)
	{
	  struct notif_client *notif = &notif_client_stop;

	  /* remote_notif_get_pending_replies acks this one, and gets
	     the rest out.  */
	  rs->notif_state->pending_event[notif_client_stop.id]
	    = remote_notif_parse (notif, rs->buf);
	  remote_notif_get_pending_events (notif);
	}

      if (thread_count () == 0)
	{
	  if (!extended_p)
	    error (_("The target is not running (try extended-remote?)"));

	  /* We're connected, but not running.  Drop out before we
	     call start_remote.  */
	  rs->starting_up = 0;
	  return;
	}

      /* In non-stop mode, any cached wait status will be stored in
	 the stop reply queue.  */
      gdb_assert (wait_status == NULL);

      /* Report all signals during attach/startup.  */
      remote_pass_signals (target, 0, NULL);

      /* If there are already stopped threads, mark them stopped and
	 report their stops before giving the prompt to the user.  */
      process_initial_stop_replies (from_tty);

      if (target_can_async_p ())
	target_async (1);
    }

  /* If we connected to a live target, do some additional setup.  */
  if (target_has_execution)
    {
      if (symfile_objfile) 	/* No use without a symbol-file.  */
	remote_check_symbols ();
    }

  /* Possibly the target has been engaged in a trace run started
     previously; find out where things are at.  */
  if (remote_get_trace_status (target, current_trace_status ()) != -1)
    {
      struct uploaded_tp *uploaded_tps = NULL;

      if (current_trace_status ()->running)
	printf_filtered (_("Trace is already running on the target.\n"));

      remote_upload_tracepoints (target, &uploaded_tps);

      merge_uploaded_tracepoints (&uploaded_tps);
    }

  /* Possibly the target has been engaged in a btrace record started
     previously; find out where things are at.  */
  remote_btrace_maybe_reopen ();

  /* The thread and inferior lists are now synchronized with the
     target, our symbols have been relocated, and we're merged the
     target's tracepoints with ours.  We're done with basic start
     up.  */
  rs->starting_up = 0;

  /* Maybe breakpoints are global and need to be inserted now.  */
  if (breakpoints_should_be_inserted_now ())
    insert_breakpoints ();
}

/* Open a connection to a remote debugger.
   NAME is the filename used for communication.  */

static void
remote_open (const char *name, int from_tty)
{
  remote_open_1 (name, from_tty, &remote_ops, 0);
}

/* Open a connection to a remote debugger using the extended
   remote gdb protocol.  NAME is the filename used for communication.  */

static void
extended_remote_open (const char *name, int from_tty)
{
  remote_open_1 (name, from_tty, &extended_remote_ops, 1 /*extended_p */);
}

/* Reset all packets back to "unknown support".  Called when opening a
   new connection to a remote target.  */

static void
reset_all_packet_configs_support (void)
{
  int i;

  for (i = 0; i < PACKET_MAX; i++)
    remote_protocol_packets[i].support = PACKET_SUPPORT_UNKNOWN;
}

/* Initialize all packet configs.  */

static void
init_all_packet_configs (void)
{
  int i;

  for (i = 0; i < PACKET_MAX; i++)
    {
      remote_protocol_packets[i].detect = AUTO_BOOLEAN_AUTO;
      remote_protocol_packets[i].support = PACKET_SUPPORT_UNKNOWN;
    }
}

/* Symbol look-up.  */

static void
remote_check_symbols (void)
{
  struct remote_state *rs = get_remote_state ();
  char *msg, *reply, *tmp;
  int end;
  long reply_size;
  struct cleanup *old_chain;

  /* The remote side has no concept of inferiors that aren't running
     yet, it only knows about running processes.  If we're connected
     but our current inferior is not running, we should not invite the
     remote target to request symbol lookups related to its
     (unrelated) current process.  */
  if (!target_has_execution)
    return;

  if (packet_support (PACKET_qSymbol) == PACKET_DISABLE)
    return;

  /* Make sure the remote is pointing at the right process.  Note
     there's no way to select "no process".  */
  set_general_process ();

  /* Allocate a message buffer.  We can't reuse the input buffer in RS,
     because we need both at the same time.  */
  msg = (char *) xmalloc (get_remote_packet_size ());
  old_chain = make_cleanup (xfree, msg);
  reply = (char *) xmalloc (get_remote_packet_size ());
  make_cleanup (free_current_contents, &reply);
  reply_size = get_remote_packet_size ();

  /* Invite target to request symbol lookups.  */

  putpkt ("qSymbol::");
  getpkt (&reply, &reply_size, 0);
  packet_ok (reply, &remote_protocol_packets[PACKET_qSymbol]);

  while (startswith (reply, "qSymbol:"))
    {
      struct bound_minimal_symbol sym;

      tmp = &reply[8];
      end = hex2bin (tmp, (gdb_byte *) msg, strlen (tmp) / 2);
      msg[end] = '\0';
      sym = lookup_minimal_symbol (msg, NULL, NULL);
      if (sym.minsym == NULL)
	xsnprintf (msg, get_remote_packet_size (), "qSymbol::%s", &reply[8]);
      else
	{
	  int addr_size = gdbarch_addr_bit (target_gdbarch ()) / 8;
	  CORE_ADDR sym_addr = BMSYMBOL_VALUE_ADDRESS (sym);

	  /* If this is a function address, return the start of code
	     instead of any data function descriptor.  */
	  sym_addr = gdbarch_convert_from_func_ptr_addr (target_gdbarch (),
							 sym_addr,
							 &current_target);

	  xsnprintf (msg, get_remote_packet_size (), "qSymbol:%s:%s",
		     phex_nz (sym_addr, addr_size), &reply[8]);
	}
  
      putpkt (msg);
      getpkt (&reply, &reply_size, 0);
    }

  do_cleanups (old_chain);
}

static struct serial *
remote_serial_open (const char *name)
{
  static int udp_warning = 0;

  /* FIXME: Parsing NAME here is a hack.  But we want to warn here instead
     of in ser-tcp.c, because it is the remote protocol assuming that the
     serial connection is reliable and not the serial connection promising
     to be.  */
  if (!udp_warning && startswith (name, "udp:"))
    {
      warning (_("The remote protocol may be unreliable over UDP.\n"
		 "Some events may be lost, rendering further debugging "
		 "impossible."));
      udp_warning = 1;
    }

  return serial_open (name);
}

/* Inform the target of our permission settings.  The permission flags
   work without this, but if the target knows the settings, it can do
   a couple things.  First, it can add its own check, to catch cases
   that somehow manage to get by the permissions checks in target
   methods.  Second, if the target is wired to disallow particular
   settings (for instance, a system in the field that is not set up to
   be able to stop at a breakpoint), it can object to any unavailable
   permissions.  */

void
remote_set_permissions (struct target_ops *self)
{
  struct remote_state *rs = get_remote_state ();

  xsnprintf (rs->buf, get_remote_packet_size (), "QAllow:"
	     "WriteReg:%x;WriteMem:%x;"
	     "InsertBreak:%x;InsertTrace:%x;"
	     "InsertFastTrace:%x;Stop:%x",
	     may_write_registers, may_write_memory,
	     may_insert_breakpoints, may_insert_tracepoints,
	     may_insert_fast_tracepoints, may_stop);
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  /* If the target didn't like the packet, warn the user.  Do not try
     to undo the user's settings, that would just be maddening.  */
  if (strcmp (rs->buf, "OK") != 0)
    warning (_("Remote refused setting permissions with: %s"), rs->buf);
}

/* This type describes each known response to the qSupported
   packet.  */
struct protocol_feature
{
  /* The name of this protocol feature.  */
  const char *name;

  /* The default for this protocol feature.  */
  enum packet_support default_support;

  /* The function to call when this feature is reported, or after
     qSupported processing if the feature is not supported.
     The first argument points to this structure.  The second
     argument indicates whether the packet requested support be
     enabled, disabled, or probed (or the default, if this function
     is being called at the end of processing and this feature was
     not reported).  The third argument may be NULL; if not NULL, it
     is a NUL-terminated string taken from the packet following
     this feature's name and an equals sign.  */
  void (*func) (const struct protocol_feature *, enum packet_support,
		const char *);

  /* The corresponding packet for this feature.  Only used if
     FUNC is remote_supported_packet.  */
  int packet;
};

static void
remote_supported_packet (const struct protocol_feature *feature,
			 enum packet_support support,
			 const char *argument)
{
  if (argument)
    {
      warning (_("Remote qSupported response supplied an unexpected value for"
		 " \"%s\"."), feature->name);
      return;
    }

  remote_protocol_packets[feature->packet].support = support;
}

static void
remote_packet_size (const struct protocol_feature *feature,
		    enum packet_support support, const char *value)
{
  struct remote_state *rs = get_remote_state ();

  int packet_size;
  char *value_end;

  if (support != PACKET_ENABLE)
    return;

  if (value == NULL || *value == '\0')
    {
      warning (_("Remote target reported \"%s\" without a size."),
	       feature->name);
      return;
    }

  errno = 0;
  packet_size = strtol (value, &value_end, 16);
  if (errno != 0 || *value_end != '\0' || packet_size < 0)
    {
      warning (_("Remote target reported \"%s\" with a bad size: \"%s\"."),
	       feature->name, value);
      return;
    }

  /* Record the new maximum packet size.  */
  rs->explicit_packet_size = packet_size;
}

static const struct protocol_feature remote_protocol_features[] = {
  { "PacketSize", PACKET_DISABLE, remote_packet_size, -1 },
  { "qXfer:auxv:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_auxv },
  { "qXfer:exec-file:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_exec_file },
  { "qXfer:features:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_features },
  { "qXfer:libraries:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_libraries },
  { "qXfer:libraries-svr4:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_libraries_svr4 },
  { "augmented-libraries-svr4-read", PACKET_DISABLE,
    remote_supported_packet, PACKET_augmented_libraries_svr4_read_feature },
  { "qXfer:memory-map:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_memory_map },
  { "qXfer:spu:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_spu_read },
  { "qXfer:spu:write", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_spu_write },
  { "qXfer:osdata:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_osdata },
  { "qXfer:threads:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_threads },
  { "qXfer:traceframe-info:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_traceframe_info },
  { "QPassSignals", PACKET_DISABLE, remote_supported_packet,
    PACKET_QPassSignals },
  { "QCatchSyscalls", PACKET_DISABLE, remote_supported_packet,
    PACKET_QCatchSyscalls },
  { "QProgramSignals", PACKET_DISABLE, remote_supported_packet,
    PACKET_QProgramSignals },
  { "QStartNoAckMode", PACKET_DISABLE, remote_supported_packet,
    PACKET_QStartNoAckMode },
  { "multiprocess", PACKET_DISABLE, remote_supported_packet,
    PACKET_multiprocess_feature },
  { "QNonStop", PACKET_DISABLE, remote_supported_packet, PACKET_QNonStop },
  { "qXfer:siginfo:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_siginfo_read },
  { "qXfer:siginfo:write", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_siginfo_write },
  { "ConditionalTracepoints", PACKET_DISABLE, remote_supported_packet,
    PACKET_ConditionalTracepoints },
  { "ConditionalBreakpoints", PACKET_DISABLE, remote_supported_packet,
    PACKET_ConditionalBreakpoints },
  { "BreakpointCommands", PACKET_DISABLE, remote_supported_packet,
    PACKET_BreakpointCommands },
  { "FastTracepoints", PACKET_DISABLE, remote_supported_packet,
    PACKET_FastTracepoints },
  { "StaticTracepoints", PACKET_DISABLE, remote_supported_packet,
    PACKET_StaticTracepoints },
  {"InstallInTrace", PACKET_DISABLE, remote_supported_packet,
   PACKET_InstallInTrace},
  { "DisconnectedTracing", PACKET_DISABLE, remote_supported_packet,
    PACKET_DisconnectedTracing_feature },
  { "ReverseContinue", PACKET_DISABLE, remote_supported_packet,
    PACKET_bc },
  { "ReverseStep", PACKET_DISABLE, remote_supported_packet,
    PACKET_bs },
  { "TracepointSource", PACKET_DISABLE, remote_supported_packet,
    PACKET_TracepointSource },
  { "QAllow", PACKET_DISABLE, remote_supported_packet,
    PACKET_QAllow },
  { "EnableDisableTracepoints", PACKET_DISABLE, remote_supported_packet,
    PACKET_EnableDisableTracepoints_feature },
  { "qXfer:fdpic:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_fdpic },
  { "qXfer:uib:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_uib },
  { "QDisableRandomization", PACKET_DISABLE, remote_supported_packet,
    PACKET_QDisableRandomization },
  { "QAgent", PACKET_DISABLE, remote_supported_packet, PACKET_QAgent},
  { "QTBuffer:size", PACKET_DISABLE,
    remote_supported_packet, PACKET_QTBuffer_size},
  { "tracenz", PACKET_DISABLE, remote_supported_packet, PACKET_tracenz_feature },
  { "Qbtrace:off", PACKET_DISABLE, remote_supported_packet, PACKET_Qbtrace_off },
  { "Qbtrace:bts", PACKET_DISABLE, remote_supported_packet, PACKET_Qbtrace_bts },
  { "Qbtrace:pt", PACKET_DISABLE, remote_supported_packet, PACKET_Qbtrace_pt },
  { "qXfer:btrace:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_btrace },
  { "qXfer:btrace-conf:read", PACKET_DISABLE, remote_supported_packet,
    PACKET_qXfer_btrace_conf },
  { "Qbtrace-conf:bts:size", PACKET_DISABLE, remote_supported_packet,
    PACKET_Qbtrace_conf_bts_size },
  { "swbreak", PACKET_DISABLE, remote_supported_packet, PACKET_swbreak_feature },
  { "hwbreak", PACKET_DISABLE, remote_supported_packet, PACKET_hwbreak_feature },
  { "fork-events", PACKET_DISABLE, remote_supported_packet,
    PACKET_fork_event_feature },
  { "vfork-events", PACKET_DISABLE, remote_supported_packet,
    PACKET_vfork_event_feature },
  { "exec-events", PACKET_DISABLE, remote_supported_packet,
    PACKET_exec_event_feature },
  { "Qbtrace-conf:pt:size", PACKET_DISABLE, remote_supported_packet,
    PACKET_Qbtrace_conf_pt_size },
  { "vContSupported", PACKET_DISABLE, remote_supported_packet, PACKET_vContSupported },
  { "QThreadEvents", PACKET_DISABLE, remote_supported_packet, PACKET_QThreadEvents },
  { "no-resumed", PACKET_DISABLE, remote_supported_packet, PACKET_no_resumed },
};

static char *remote_support_xml;

/* Register string appended to "xmlRegisters=" in qSupported query.  */

void
register_remote_support_xml (const char *xml)
{
#if defined(HAVE_LIBEXPAT)
  if (remote_support_xml == NULL)
    remote_support_xml = concat ("xmlRegisters=", xml, (char *) NULL);
  else
    {
      char *copy = xstrdup (remote_support_xml + 13);
      char *p = strtok (copy, ",");

      do
	{
	  if (strcmp (p, xml) == 0)
	    {
	      /* already there */
	      xfree (copy);
	      return;
	    }
	}
      while ((p = strtok (NULL, ",")) != NULL);
      xfree (copy);

      remote_support_xml = reconcat (remote_support_xml,
				     remote_support_xml, ",", xml,
				     (char *) NULL);
    }
#endif
}

static char *
remote_query_supported_append (char *msg, const char *append)
{
  if (msg)
    return reconcat (msg, msg, ";", append, (char *) NULL);
  else
    return xstrdup (append);
}

static void
remote_query_supported (void)
{
  struct remote_state *rs = get_remote_state ();
  char *next;
  int i;
  unsigned char seen [ARRAY_SIZE (remote_protocol_features)];

  /* The packet support flags are handled differently for this packet
     than for most others.  We treat an error, a disabled packet, and
     an empty response identically: any features which must be reported
     to be used will be automatically disabled.  An empty buffer
     accomplishes this, since that is also the representation for a list
     containing no features.  */

  rs->buf[0] = 0;
  if (packet_support (PACKET_qSupported) != PACKET_DISABLE)
    {
      char *q = NULL;
      struct cleanup *old_chain = make_cleanup (free_current_contents, &q);

      if (packet_set_cmd_state (PACKET_multiprocess_feature) != AUTO_BOOLEAN_FALSE)
	q = remote_query_supported_append (q, "multiprocess+");

      if (packet_set_cmd_state (PACKET_swbreak_feature) != AUTO_BOOLEAN_FALSE)
	q = remote_query_supported_append (q, "swbreak+");
      if (packet_set_cmd_state (PACKET_hwbreak_feature) != AUTO_BOOLEAN_FALSE)
	q = remote_query_supported_append (q, "hwbreak+");

      q = remote_query_supported_append (q, "qRelocInsn+");

      if (packet_set_cmd_state (PACKET_fork_event_feature)
	  != AUTO_BOOLEAN_FALSE)
	q = remote_query_supported_append (q, "fork-events+");
      if (packet_set_cmd_state (PACKET_vfork_event_feature)
	  != AUTO_BOOLEAN_FALSE)
	q = remote_query_supported_append (q, "vfork-events+");
      if (packet_set_cmd_state (PACKET_exec_event_feature)
	  != AUTO_BOOLEAN_FALSE)
	q = remote_query_supported_append (q, "exec-events+");

      if (packet_set_cmd_state (PACKET_vContSupported) != AUTO_BOOLEAN_FALSE)
	q = remote_query_supported_append (q, "vContSupported+");

      if (packet_set_cmd_state (PACKET_QThreadEvents) != AUTO_BOOLEAN_FALSE)
	q = remote_query_supported_append (q, "QThreadEvents+");

      if (packet_set_cmd_state (PACKET_no_resumed) != AUTO_BOOLEAN_FALSE)
	q = remote_query_supported_append (q, "no-resumed+");

      /* Keep this one last to work around a gdbserver <= 7.10 bug in
	 the qSupported:xmlRegisters=i386 handling.  */
      if (remote_support_xml != NULL)
	q = remote_query_supported_append (q, remote_support_xml);

      q = reconcat (q, "qSupported:", q, (char *) NULL);
      putpkt (q);

      do_cleanups (old_chain);

      getpkt (&rs->buf, &rs->buf_size, 0);

      /* If an error occured, warn, but do not return - just reset the
	 buffer to empty and go on to disable features.  */
      if (packet_ok (rs->buf, &remote_protocol_packets[PACKET_qSupported])
	  == PACKET_ERROR)
	{
	  warning (_("Remote failure reply: %s"), rs->buf);
	  rs->buf[0] = 0;
	}
    }

  memset (seen, 0, sizeof (seen));

  next = rs->buf;
  while (*next)
    {
      enum packet_support is_supported;
      char *p, *end, *name_end, *value;

      /* First separate out this item from the rest of the packet.  If
	 there's another item after this, we overwrite the separator
	 (terminated strings are much easier to work with).  */
      p = next;
      end = strchr (p, ';');
      if (end == NULL)
	{
	  end = p + strlen (p);
	  next = end;
	}
      else
	{
	  *end = '\0';
	  next = end + 1;

	  if (end == p)
	    {
	      warning (_("empty item in \"qSupported\" response"));
	      continue;
	    }
	}

      name_end = strchr (p, '=');
      if (name_end)
	{
	  /* This is a name=value entry.  */
	  is_supported = PACKET_ENABLE;
	  value = name_end + 1;
	  *name_end = '\0';
	}
      else
	{
	  value = NULL;
	  switch (end[-1])
	    {
	    case '+':
	      is_supported = PACKET_ENABLE;
	      break;

	    case '-':
	      is_supported = PACKET_DISABLE;
	      break;

	    case '?':
	      is_supported = PACKET_SUPPORT_UNKNOWN;
	      break;

	    default:
	      warning (_("unrecognized item \"%s\" "
			 "in \"qSupported\" response"), p);
	      continue;
	    }
	  end[-1] = '\0';
	}

      for (i = 0; i < ARRAY_SIZE (remote_protocol_features); i++)
	if (strcmp (remote_protocol_features[i].name, p) == 0)
	  {
	    const struct protocol_feature *feature;

	    seen[i] = 1;
	    feature = &remote_protocol_features[i];
	    feature->func (feature, is_supported, value);
	    break;
	  }
    }

  /* If we increased the packet size, make sure to increase the global
     buffer size also.  We delay this until after parsing the entire
     qSupported packet, because this is the same buffer we were
     parsing.  */
  if (rs->buf_size < rs->explicit_packet_size)
    {
      rs->buf_size = rs->explicit_packet_size;
      rs->buf = (char *) xrealloc (rs->buf, rs->buf_size);
    }

  /* Handle the defaults for unmentioned features.  */
  for (i = 0; i < ARRAY_SIZE (remote_protocol_features); i++)
    if (!seen[i])
      {
	const struct protocol_feature *feature;

	feature = &remote_protocol_features[i];
	feature->func (feature, feature->default_support, NULL);
      }
}

/* Serial QUIT handler for the remote serial descriptor.

   Defers handling a Ctrl-C until we're done with the current
   command/response packet sequence, unless:

   - We're setting up the connection.  Don't send a remote interrupt
     request, as we're not fully synced yet.  Quit immediately
     instead.

   - The target has been resumed in the foreground
     (target_terminal_is_ours is false) with a synchronous resume
     packet, and we're blocked waiting for the stop reply, thus a
     Ctrl-C should be immediately sent to the target.

   - We get a second Ctrl-C while still within the same serial read or
     write.  In that case the serial is seemingly wedged --- offer to
     quit/disconnect.

   - We see a second Ctrl-C without target response, after having
     previously interrupted the target.  In that case the target/stub
     is probably wedged --- offer to quit/disconnect.
*/

static void
remote_serial_quit_handler (void)
{
  struct remote_state *rs = get_remote_state ();

  if (check_quit_flag ())
    {
      /* If we're starting up, we're not fully synced yet.  Quit
	 immediately.  */
      if (rs->starting_up)
	quit ();
      else if (rs->got_ctrlc_during_io)
	{
	  if (query (_("The target is not responding to GDB commands.\n"
		       "Stop debugging it? ")))
	    remote_unpush_and_throw ();
	}
      /* If ^C has already been sent once, offer to disconnect.  */
      else if (!target_terminal_is_ours () && rs->ctrlc_pending_p)
	interrupt_query ();
      /* All-stop protocol, and blocked waiting for stop reply.  Send
	 an interrupt request.  */
      else if (!target_terminal_is_ours () && rs->waiting_for_stop_reply)
	target_interrupt (inferior_ptid);
      else
	rs->got_ctrlc_during_io = 1;
    }
}

/* Remove any of the remote.c targets from target stack.  Upper targets depend
   on it so remove them first.  */

static void
remote_unpush_target (void)
{
  pop_all_targets_at_and_above (process_stratum);
}

static void
remote_unpush_and_throw (void)
{
  remote_unpush_target ();
  throw_error (TARGET_CLOSE_ERROR, _("Disconnected from target."));
}

static void
remote_open_1 (const char *name, int from_tty,
	       struct target_ops *target, int extended_p)
{
  struct remote_state *rs = get_remote_state ();

  if (name == 0)
    error (_("To open a remote debug connection, you need to specify what\n"
	   "serial device is attached to the remote system\n"
	   "(e.g. /dev/ttyS0, /dev/ttya, COM1, etc.)."));

  /* See FIXME above.  */
  if (!target_async_permitted)
    wait_forever_enabled_p = 1;

  /* If we're connected to a running target, target_preopen will kill it.
     Ask this question first, before target_preopen has a chance to kill
     anything.  */
  if (rs->remote_desc != NULL && !have_inferiors ())
    {
      if (from_tty
	  && !query (_("Already connected to a remote target.  Disconnect? ")))
	error (_("Still connected."));
    }

  /* Here the possibly existing remote target gets unpushed.  */
  target_preopen (from_tty);

  /* Make sure we send the passed signals list the next time we resume.  */
  xfree (rs->last_pass_packet);
  rs->last_pass_packet = NULL;

  /* Make sure we send the program signals list the next time we
     resume.  */
  xfree (rs->last_program_signals_packet);
  rs->last_program_signals_packet = NULL;

  remote_fileio_reset ();
  reopen_exec_file ();
  reread_symbols ();

  rs->remote_desc = remote_serial_open (name);
  if (!rs->remote_desc)
    perror_with_name (name);

  if (baud_rate != -1)
    {
      if (serial_setbaudrate (rs->remote_desc, baud_rate))
	{
	  /* The requested speed could not be set.  Error out to
	     top level after closing remote_desc.  Take care to
	     set remote_desc to NULL to avoid closing remote_desc
	     more than once.  */
	  serial_close (rs->remote_desc);
	  rs->remote_desc = NULL;
	  perror_with_name (name);
	}
    }

  serial_setparity (rs->remote_desc, serial_parity);
  serial_raw (rs->remote_desc);

  /* If there is something sitting in the buffer we might take it as a
     response to a command, which would be bad.  */
  serial_flush_input (rs->remote_desc);

  if (from_tty)
    {
      puts_filtered ("Remote debugging using ");
      puts_filtered (name);
      puts_filtered ("\n");
    }
  push_target (target);		/* Switch to using remote target now.  */

  /* Register extra event sources in the event loop.  */
  remote_async_inferior_event_token
    = create_async_event_handler (remote_async_inferior_event_handler,
				  NULL);
  rs->notif_state = remote_notif_state_allocate ();

  /* Reset the target state; these things will be queried either by
     remote_query_supported or as they are needed.  */
  reset_all_packet_configs_support ();
  rs->cached_wait_status = 0;
  rs->explicit_packet_size = 0;
  rs->noack_mode = 0;
  rs->extended = extended_p;
  rs->waiting_for_stop_reply = 0;
  rs->ctrlc_pending_p = 0;
  rs->got_ctrlc_during_io = 0;

  rs->general_thread = not_sent_ptid;
  rs->continue_thread = not_sent_ptid;
  rs->remote_traceframe_number = -1;

  rs->last_resume_exec_dir = EXEC_FORWARD;

  /* Probe for ability to use "ThreadInfo" query, as required.  */
  rs->use_threadinfo_query = 1;
  rs->use_threadextra_query = 1;

  readahead_cache_invalidate ();

  /* Start out by owning the terminal.  */
  remote_async_terminal_ours_p = 1;

  if (target_async_permitted)
    {
      /* FIXME: cagney/1999-09-23: During the initial connection it is
	 assumed that the target is already ready and able to respond to
	 requests.  Unfortunately remote_start_remote() eventually calls
	 wait_for_inferior() with no timeout.  wait_forever_enabled_p gets
	 around this.  Eventually a mechanism that allows
	 wait_for_inferior() to expect/get timeouts will be
	 implemented.  */
      wait_forever_enabled_p = 0;
    }

  /* First delete any symbols previously loaded from shared libraries.  */
  no_shared_libraries (NULL, 0);

  /* Start afresh.  */
  init_thread_list ();

  /* Start the remote connection.  If error() or QUIT, discard this
     target (we'd otherwise be in an inconsistent state) and then
     propogate the error on up the exception chain.  This ensures that
     the caller doesn't stumble along blindly assuming that the
     function succeeded.  The CLI doesn't have this problem but other
     UI's, such as MI do.

     FIXME: cagney/2002-05-19: Instead of re-throwing the exception,
     this function should return an error indication letting the
     caller restore the previous state.  Unfortunately the command
     ``target remote'' is directly wired to this function making that
     impossible.  On a positive note, the CLI side of this problem has
     been fixed - the function set_cmd_context() makes it possible for
     all the ``target ....'' commands to share a common callback
     function.  See cli-dump.c.  */
  {

    TRY
      {
	remote_start_remote (from_tty, target, extended_p);
      }
    CATCH (ex, RETURN_MASK_ALL)
      {
	/* Pop the partially set up target - unless something else did
	   already before throwing the exception.  */
	if (rs->remote_desc != NULL)
	  remote_unpush_target ();
	if (target_async_permitted)
	  wait_forever_enabled_p = 1;
	throw_exception (ex);
      }
    END_CATCH
  }

  remote_btrace_reset ();

  if (target_async_permitted)
    wait_forever_enabled_p = 1;
}

/* Detach the specified process.  */

static void
remote_detach_pid (int pid)
{
  struct remote_state *rs = get_remote_state ();

  if (remote_multi_process_p (rs))
    xsnprintf (rs->buf, get_remote_packet_size (), "D;%x", pid);
  else
    strcpy (rs->buf, "D");

  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  if (rs->buf[0] == 'O' && rs->buf[1] == 'K')
    ;
  else if (rs->buf[0] == '\0')
    error (_("Remote doesn't know how to detach"));
  else
    error (_("Can't detach process."));
}

/* This detaches a program to which we previously attached, using
   inferior_ptid to identify the process.  After this is done, GDB
   can be used to debug some other program.  We better not have left
   any breakpoints in the target program or it'll die when it hits
   one.  */

static void
remote_detach_1 (const char *args, int from_tty)
{
  int pid = ptid_get_pid (inferior_ptid);
  struct remote_state *rs = get_remote_state ();
  struct thread_info *tp = find_thread_ptid (inferior_ptid);
  int is_fork_parent;

  if (args)
    error (_("Argument given to \"detach\" when remotely debugging."));

  if (!target_has_execution)
    error (_("No process to detach from."));

  target_announce_detach (from_tty);

  /* Tell the remote target to detach.  */
  remote_detach_pid (pid);

  /* Exit only if this is the only active inferior.  */
  if (from_tty && !rs->extended && number_of_live_inferiors () == 1)
    puts_filtered (_("Ending remote debugging.\n"));

  /* Check to see if we are detaching a fork parent.  Note that if we
     are detaching a fork child, tp == NULL.  */
  is_fork_parent = (tp != NULL
		    && tp->pending_follow.kind == TARGET_WAITKIND_FORKED);

  /* If doing detach-on-fork, we don't mourn, because that will delete
     breakpoints that should be available for the followed inferior.  */
  if (!is_fork_parent)
    target_mourn_inferior ();
  else
    {
      inferior_ptid = null_ptid;
      detach_inferior (pid);
    }
}

static void
remote_detach (struct target_ops *ops, const char *args, int from_tty)
{
  remote_detach_1 (args, from_tty);
}

static void
extended_remote_detach (struct target_ops *ops, const char *args, int from_tty)
{
  remote_detach_1 (args, from_tty);
}

/* Target follow-fork function for remote targets.  On entry, and
   at return, the current inferior is the fork parent.

   Note that although this is currently only used for extended-remote,
   it is named remote_follow_fork in anticipation of using it for the
   remote target as well.  */

static int
remote_follow_fork (struct target_ops *ops, int follow_child,
		    int detach_fork)
{
  struct remote_state *rs = get_remote_state ();
  enum target_waitkind kind = inferior_thread ()->pending_follow.kind;

  if ((kind == TARGET_WAITKIND_FORKED && remote_fork_event_p (rs))
      || (kind == TARGET_WAITKIND_VFORKED && remote_vfork_event_p (rs)))
    {
      /* When following the parent and detaching the child, we detach
	 the child here.  For the case of following the child and
	 detaching the parent, the detach is done in the target-
	 independent follow fork code in infrun.c.  We can't use
	 target_detach when detaching an unfollowed child because
	 the client side doesn't know anything about the child.  */
      if (detach_fork && !follow_child)
	{
	  /* Detach the fork child.  */
	  ptid_t child_ptid;
	  pid_t child_pid;

	  child_ptid = inferior_thread ()->pending_follow.value.related_pid;
	  child_pid = ptid_get_pid (child_ptid);

	  remote_detach_pid (child_pid);
	  detach_inferior (child_pid);
	}
    }
  return 0;
}

/* Target follow-exec function for remote targets.  Save EXECD_PATHNAME
   in the program space of the new inferior.  On entry and at return the
   current inferior is the exec'ing inferior.  INF is the new exec'd
   inferior, which may be the same as the exec'ing inferior unless
   follow-exec-mode is "new".  */

static void
remote_follow_exec (struct target_ops *ops,
		    struct inferior *inf, char *execd_pathname)
{
  /* We know that this is a target file name, so if it has the "target:"
     prefix we strip it off before saving it in the program space.  */
  if (is_target_filename (execd_pathname))
    execd_pathname += strlen (TARGET_SYSROOT_PREFIX);

  set_pspace_remote_exec_file (inf->pspace, execd_pathname);
}

/* Same as remote_detach, but don't send the "D" packet; just disconnect.  */

static void
remote_disconnect (struct target_ops *target, const char *args, int from_tty)
{
  if (args)
    error (_("Argument given to \"disconnect\" when remotely debugging."));

  /* Make sure we unpush even the extended remote targets.  Calling
     target_mourn_inferior won't unpush, and remote_mourn won't
     unpush if there is more than one inferior left.  */
  unpush_target (target);
  generic_mourn_inferior ();

  if (from_tty)
    puts_filtered ("Ending remote debugging.\n");
}

/* Attach to the process specified by ARGS.  If FROM_TTY is non-zero,
   be chatty about it.  */

static void
extended_remote_attach (struct target_ops *target, const char *args,
			int from_tty)
{
  struct remote_state *rs = get_remote_state ();
  int pid;
  char *wait_status = NULL;

  pid = parse_pid_to_attach (args);

  /* Remote PID can be freely equal to getpid, do not check it here the same
     way as in other targets.  */

  if (packet_support (PACKET_vAttach) == PACKET_DISABLE)
    error (_("This target does not support attaching to a process"));

  if (from_tty)
    {
      char *exec_file = get_exec_file (0);

      if (exec_file)
	printf_unfiltered (_("Attaching to program: %s, %s\n"), exec_file,
			   target_pid_to_str (pid_to_ptid (pid)));
      else
	printf_unfiltered (_("Attaching to %s\n"),
			   target_pid_to_str (pid_to_ptid (pid)));

      gdb_flush (gdb_stdout);
    }

  xsnprintf (rs->buf, get_remote_packet_size (), "vAttach;%x", pid);
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  switch (packet_ok (rs->buf,
		     &remote_protocol_packets[PACKET_vAttach]))
    {
    case PACKET_OK:
      if (!target_is_non_stop_p ())
	{
	  /* Save the reply for later.  */
	  wait_status = (char *) alloca (strlen (rs->buf) + 1);
	  strcpy (wait_status, rs->buf);
	}
      else if (strcmp (rs->buf, "OK") != 0)
	error (_("Attaching to %s failed with: %s"),
	       target_pid_to_str (pid_to_ptid (pid)),
	       rs->buf);
      break;
    case PACKET_UNKNOWN:
      error (_("This target does not support attaching to a process"));
    default:
      error (_("Attaching to %s failed"),
	     target_pid_to_str (pid_to_ptid (pid)));
    }

  set_current_inferior (remote_add_inferior (0, pid, 1, 0));

  inferior_ptid = pid_to_ptid (pid);

  if (target_is_non_stop_p ())
    {
      struct thread_info *thread;

      /* Get list of threads.  */
      remote_update_thread_list (target);

      thread = first_thread_of_process (pid);
      if (thread)
	inferior_ptid = thread->ptid;
      else
	inferior_ptid = pid_to_ptid (pid);

      /* Invalidate our notion of the remote current thread.  */
      record_currthread (rs, minus_one_ptid);
    }
  else
    {
      /* Now, if we have thread information, update inferior_ptid.  */
      inferior_ptid = remote_current_thread (inferior_ptid);

      /* Add the main thread to the thread list.  */
      add_thread_silent (inferior_ptid);
    }

  /* Next, if the target can specify a description, read it.  We do
     this before anything involving memory or registers.  */
  target_find_description ();

  if (!target_is_non_stop_p ())
    {
      /* Use the previously fetched status.  */
      gdb_assert (wait_status != NULL);

      if (target_can_async_p ())
	{
	  struct notif_event *reply
	    =  remote_notif_parse (&notif_client_stop, wait_status);

	  push_stop_reply ((struct stop_reply *) reply);

	  target_async (1);
	}
      else
	{
	  gdb_assert (wait_status != NULL);
	  strcpy (rs->buf, wait_status);
	  rs->cached_wait_status = 1;
	}
    }
  else
    gdb_assert (wait_status == NULL);
}

/* Implementation of the to_post_attach method.  */

static void
extended_remote_post_attach (struct target_ops *ops, int pid)
{
  /* Get text, data & bss offsets.  */
  get_offsets ();

  /* In certain cases GDB might not have had the chance to start
     symbol lookup up until now.  This could happen if the debugged
     binary is not using shared libraries, the vsyscall page is not
     present (on Linux) and the binary itself hadn't changed since the
     debugging process was started.  */
  if (symfile_objfile != NULL)
    remote_check_symbols();
}


/* Check for the availability of vCont.  This function should also check
   the response.  */

static void
remote_vcont_probe (struct remote_state *rs)
{
  char *buf;

  strcpy (rs->buf, "vCont?");
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);
  buf = rs->buf;

  /* Make sure that the features we assume are supported.  */
  if (startswith (buf, "vCont"))
    {
      char *p = &buf[5];
      int support_c, support_C;

      rs->supports_vCont.s = 0;
      rs->supports_vCont.S = 0;
      support_c = 0;
      support_C = 0;
      rs->supports_vCont.t = 0;
      rs->supports_vCont.r = 0;
      while (p && *p == ';')
	{
	  p++;
	  if (*p == 's' && (*(p + 1) == ';' || *(p + 1) == 0))
	    rs->supports_vCont.s = 1;
	  else if (*p == 'S' && (*(p + 1) == ';' || *(p + 1) == 0))
	    rs->supports_vCont.S = 1;
	  else if (*p == 'c' && (*(p + 1) == ';' || *(p + 1) == 0))
	    support_c = 1;
	  else if (*p == 'C' && (*(p + 1) == ';' || *(p + 1) == 0))
	    support_C = 1;
	  else if (*p == 't' && (*(p + 1) == ';' || *(p + 1) == 0))
	    rs->supports_vCont.t = 1;
	  else if (*p == 'r' && (*(p + 1) == ';' || *(p + 1) == 0))
	    rs->supports_vCont.r = 1;

	  p = strchr (p, ';');
	}

      /* If c, and C are not all supported, we can't use vCont.  Clearing
	 BUF will make packet_ok disable the packet.  */
      if (!support_c || !support_C)
	buf[0] = 0;
    }

  packet_ok (buf, &remote_protocol_packets[PACKET_vCont]);
}

/* Helper function for building "vCont" resumptions.  Write a
   resumption to P.  ENDP points to one-passed-the-end of the buffer
   we're allowed to write to.  Returns BUF+CHARACTERS_WRITTEN.  The
   thread to be resumed is PTID; STEP and SIGGNAL indicate whether the
   resumed thread should be single-stepped and/or signalled.  If PTID
   equals minus_one_ptid, then all threads are resumed; if PTID
   represents a process, then all threads of the process are resumed;
   the thread to be stepped and/or signalled is given in the global
   INFERIOR_PTID.  */

static char *
append_resumption (char *p, char *endp,
		   ptid_t ptid, int step, enum gdb_signal siggnal)
{
  struct remote_state *rs = get_remote_state ();

  if (step && siggnal != GDB_SIGNAL_0)
    p += xsnprintf (p, endp - p, ";S%02x", siggnal);
  else if (step
	   /* GDB is willing to range step.  */
	   && use_range_stepping
	   /* Target supports range stepping.  */
	   && rs->supports_vCont.r
	   /* We don't currently support range stepping multiple
	      threads with a wildcard (though the protocol allows it,
	      so stubs shouldn't make an active effort to forbid
	      it).  */
	   && !(remote_multi_process_p (rs) && ptid_is_pid (ptid)))
    {
      struct thread_info *tp;

      if (ptid_equal (ptid, minus_one_ptid))
	{
	  /* If we don't know about the target thread's tid, then
	     we're resuming magic_null_ptid (see caller).  */
	  tp = find_thread_ptid (magic_null_ptid);
	}
      else
	tp = find_thread_ptid (ptid);
      gdb_assert (tp != NULL);

      if (tp->control.may_range_step)
	{
	  int addr_size = gdbarch_addr_bit (target_gdbarch ()) / 8;

	  p += xsnprintf (p, endp - p, ";r%s,%s",
			  phex_nz (tp->control.step_range_start,
				   addr_size),
			  phex_nz (tp->control.step_range_end,
				   addr_size));
	}
      else
	p += xsnprintf (p, endp - p, ";s");
    }
  else if (step)
    p += xsnprintf (p, endp - p, ";s");
  else if (siggnal != GDB_SIGNAL_0)
    p += xsnprintf (p, endp - p, ";C%02x", siggnal);
  else
    p += xsnprintf (p, endp - p, ";c");

  if (remote_multi_process_p (rs) && ptid_is_pid (ptid))
    {
      ptid_t nptid;

      /* All (-1) threads of process.  */
      nptid = ptid_build (ptid_get_pid (ptid), -1, 0);

      p += xsnprintf (p, endp - p, ":");
      p = write_ptid (p, endp, nptid);
    }
  else if (!ptid_equal (ptid, minus_one_ptid))
    {
      p += xsnprintf (p, endp - p, ":");
      p = write_ptid (p, endp, ptid);
    }

  return p;
}

/* Clear the thread's private info on resume.  */

static void
resume_clear_thread_private_info (struct thread_info *thread)
{
  if (thread->priv != NULL)
    {
      thread->priv->stop_reason = TARGET_STOPPED_BY_NO_REASON;
      thread->priv->watch_data_address = 0;
    }
}

/* Append a vCont continue-with-signal action for threads that have a
   non-zero stop signal.  */

static char *
append_pending_thread_resumptions (char *p, char *endp, ptid_t ptid)
{
  struct thread_info *thread;

  ALL_NON_EXITED_THREADS (thread)
    if (ptid_match (thread->ptid, ptid)
	&& !ptid_equal (inferior_ptid, thread->ptid)
	&& thread->suspend.stop_signal != GDB_SIGNAL_0)
      {
	p = append_resumption (p, endp, thread->ptid,
			       0, thread->suspend.stop_signal);
	thread->suspend.stop_signal = GDB_SIGNAL_0;
	resume_clear_thread_private_info (thread);
      }

  return p;
}

/* Resume the remote inferior by using a "vCont" packet.  The thread
   to be resumed is PTID; STEP and SIGGNAL indicate whether the
   resumed thread should be single-stepped and/or signalled.  If PTID
   equals minus_one_ptid, then all threads are resumed; the thread to
   be stepped and/or signalled is given in the global INFERIOR_PTID.
   This function returns non-zero iff it resumes the inferior.

   This function issues a strict subset of all possible vCont commands at the
   moment.  */

static int
remote_vcont_resume (ptid_t ptid, int step, enum gdb_signal siggnal)
{
  struct remote_state *rs = get_remote_state ();
  char *p;
  char *endp;

  if (packet_support (PACKET_vCont) == PACKET_SUPPORT_UNKNOWN)
    remote_vcont_probe (rs);

  if (packet_support (PACKET_vCont) == PACKET_DISABLE)
    return 0;

  p = rs->buf;
  endp = rs->buf + get_remote_packet_size ();

  /* If we could generate a wider range of packets, we'd have to worry
     about overflowing BUF.  Should there be a generic
     "multi-part-packet" packet?  */

  p += xsnprintf (p, endp - p, "vCont");

  if (ptid_equal (ptid, magic_null_ptid))
    {
      /* MAGIC_NULL_PTID means that we don't have any active threads,
	 so we don't have any TID numbers the inferior will
	 understand.  Make sure to only send forms that do not specify
	 a TID.  */
      append_resumption (p, endp, minus_one_ptid, step, siggnal);
    }
  else if (ptid_equal (ptid, minus_one_ptid) || ptid_is_pid (ptid))
    {
      /* Resume all threads (of all processes, or of a single
	 process), with preference for INFERIOR_PTID.  This assumes
	 inferior_ptid belongs to the set of all threads we are about
	 to resume.  */
      if (step || siggnal != GDB_SIGNAL_0)
	{
	  /* Step inferior_ptid, with or without signal.  */
	  p = append_resumption (p, endp, inferior_ptid, step, siggnal);
	}

      /* Also pass down any pending signaled resumption for other
	 threads not the current.  */
      p = append_pending_thread_resumptions (p, endp, ptid);

      /* And continue others without a signal.  */
      append_resumption (p, endp, ptid, /*step=*/ 0, GDB_SIGNAL_0);
    }
  else
    {
      /* Scheduler locking; resume only PTID.  */
      append_resumption (p, endp, ptid, step, siggnal);
    }

  gdb_assert (strlen (rs->buf) < get_remote_packet_size ());
  putpkt (rs->buf);

  if (target_is_non_stop_p ())
    {
      /* In non-stop, the stub replies to vCont with "OK".  The stop
	 reply will be reported asynchronously by means of a `%Stop'
	 notification.  */
      getpkt (&rs->buf, &rs->buf_size, 0);
      if (strcmp (rs->buf, "OK") != 0)
	error (_("Unexpected vCont reply in non-stop mode: %s"), rs->buf);
    }

  return 1;
}

/* Tell the remote machine to resume.  */

static void
remote_resume (struct target_ops *ops,
	       ptid_t ptid, int step, enum gdb_signal siggnal)
{
  struct remote_state *rs = get_remote_state ();
  char *buf;
  struct thread_info *thread;

  /* In all-stop, we can't mark REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN
     (explained in remote-notif.c:handle_notification) so
     remote_notif_process is not called.  We need find a place where
     it is safe to start a 'vNotif' sequence.  It is good to do it
     before resuming inferior, because inferior was stopped and no RSP
     traffic at that moment.  */
  if (!target_is_non_stop_p ())
    remote_notif_process (rs->notif_state, &notif_client_stop);

  rs->last_sent_signal = siggnal;
  rs->last_sent_step = step;

  rs->last_resume_exec_dir = execution_direction;

  /* The vCont packet doesn't need to specify threads via Hc.  */
  /* No reverse support (yet) for vCont.  */
  if (execution_direction != EXEC_REVERSE)
    if (remote_vcont_resume (ptid, step, siggnal))
      goto done;

  /* All other supported resume packets do use Hc, so set the continue
     thread.  */
  if (ptid_equal (ptid, minus_one_ptid))
    set_continue_thread (any_thread_ptid);
  else
    set_continue_thread (ptid);

  ALL_NON_EXITED_THREADS (thread)
    resume_clear_thread_private_info (thread);

  buf = rs->buf;
  if (execution_direction == EXEC_REVERSE)
    {
      /* We don't pass signals to the target in reverse exec mode.  */
      if (info_verbose && siggnal != GDB_SIGNAL_0)
	warning (_(" - Can't pass signal %d to target in reverse: ignored."),
		 siggnal);

      if (step && packet_support (PACKET_bs) == PACKET_DISABLE)
	error (_("Remote reverse-step not supported."));
      if (!step && packet_support (PACKET_bc) == PACKET_DISABLE)
	error (_("Remote reverse-continue not supported."));

      strcpy (buf, step ? "bs" : "bc");
    }
  else if (siggnal != GDB_SIGNAL_0)
    {
      buf[0] = step ? 'S' : 'C';
      buf[1] = tohex (((int) siggnal >> 4) & 0xf);
      buf[2] = tohex (((int) siggnal) & 0xf);
      buf[3] = '\0';
    }
  else
    strcpy (buf, step ? "s" : "c");

  putpkt (buf);

 done:
  /* We are about to start executing the inferior, let's register it
     with the event loop.  NOTE: this is the one place where all the
     execution commands end up.  We could alternatively do this in each
     of the execution commands in infcmd.c.  */
  /* FIXME: ezannoni 1999-09-28: We may need to move this out of here
     into infcmd.c in order to allow inferior function calls to work
     NOT asynchronously.  */
  if (target_can_async_p ())
    target_async (1);

  /* We've just told the target to resume.  The remote server will
     wait for the inferior to stop, and then send a stop reply.  In
     the mean time, we can't start another command/query ourselves
     because the stub wouldn't be ready to process it.  This applies
     only to the base all-stop protocol, however.  In non-stop (which
     only supports vCont), the stub replies with an "OK", and is
     immediate able to process further serial input.  */
  if (!target_is_non_stop_p ())
    rs->waiting_for_stop_reply = 1;
}


/* Non-stop version of target_stop.  Uses `vCont;t' to stop a remote
   thread, all threads of a remote process, or all threads of all
   processes.  */

static void
remote_stop_ns (ptid_t ptid)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf;
  char *endp = rs->buf + get_remote_packet_size ();

  if (packet_support (PACKET_vCont) == PACKET_SUPPORT_UNKNOWN)
    remote_vcont_probe (rs);

  if (!rs->supports_vCont.t)
    error (_("Remote server does not support stopping threads"));

  if (ptid_equal (ptid, minus_one_ptid)
      || (!remote_multi_process_p (rs) && ptid_is_pid (ptid)))
    p += xsnprintf (p, endp - p, "vCont;t");
  else
    {
      ptid_t nptid;

      p += xsnprintf (p, endp - p, "vCont;t:");

      if (ptid_is_pid (ptid))
	  /* All (-1) threads of process.  */
	nptid = ptid_build (ptid_get_pid (ptid), -1, 0);
      else
	{
	  /* Small optimization: if we already have a stop reply for
	     this thread, no use in telling the stub we want this
	     stopped.  */
	  if (peek_stop_reply (ptid))
	    return;

	  nptid = ptid;
	}

      write_ptid (p, endp, nptid);
    }

  /* In non-stop, we get an immediate OK reply.  The stop reply will
     come in asynchronously by notification.  */
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);
  if (strcmp (rs->buf, "OK") != 0)
    error (_("Stopping %s failed: %s"), target_pid_to_str (ptid), rs->buf);
}

/* All-stop version of target_interrupt.  Sends a break or a ^C to
   interrupt the remote target.  It is undefined which thread of which
   process reports the interrupt.  */

static void
remote_interrupt_as (void)
{
  struct remote_state *rs = get_remote_state ();

  rs->ctrlc_pending_p = 1;

  /* If the inferior is stopped already, but the core didn't know
     about it yet, just ignore the request.  The cached wait status
     will be collected in remote_wait.  */
  if (rs->cached_wait_status)
    return;

  /* Send interrupt_sequence to remote target.  */
  send_interrupt_sequence ();
}

/* Non-stop version of target_interrupt.  Uses `vCtrlC' to interrupt
   the remote target.  It is undefined which thread of which process
   reports the interrupt.  Throws an error if the packet is not
   supported by the server.  */

static void
remote_interrupt_ns (void)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf;
  char *endp = rs->buf + get_remote_packet_size ();

  xsnprintf (p, endp - p, "vCtrlC");

  /* In non-stop, we get an immediate OK reply.  The stop reply will
     come in asynchronously by notification.  */
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  switch (packet_ok (rs->buf, &remote_protocol_packets[PACKET_vCtrlC]))
    {
    case PACKET_OK:
      break;
    case PACKET_UNKNOWN:
      error (_("No support for interrupting the remote target."));
    case PACKET_ERROR:
      error (_("Interrupting target failed: %s"), rs->buf);
    }
}

/* Implement the to_stop function for the remote targets.  */

static void
remote_stop (struct target_ops *self, ptid_t ptid)
{
  if (remote_debug)
    fprintf_unfiltered (gdb_stdlog, "remote_stop called\n");

  if (target_is_non_stop_p ())
    remote_stop_ns (ptid);
  else
    {
      /* We don't currently have a way to transparently pause the
	 remote target in all-stop mode.  Interrupt it instead.  */
      remote_interrupt_as ();
    }
}

/* Implement the to_interrupt function for the remote targets.  */

static void
remote_interrupt (struct target_ops *self, ptid_t ptid)
{
  struct remote_state *rs = get_remote_state ();

  if (remote_debug)
    fprintf_unfiltered (gdb_stdlog, "remote_interrupt called\n");

  if (target_is_non_stop_p ())
    remote_interrupt_ns ();
  else
    remote_interrupt_as ();
}

/* Implement the to_pass_ctrlc function for the remote targets.  */

static void
remote_pass_ctrlc (struct target_ops *self)
{
  struct remote_state *rs = get_remote_state ();

  if (remote_debug)
    fprintf_unfiltered (gdb_stdlog, "remote_pass_ctrlc called\n");

  /* If we're starting up, we're not fully synced yet.  Quit
     immediately.  */
  if (rs->starting_up)
    quit ();
  /* If ^C has already been sent once, offer to disconnect.  */
  else if (rs->ctrlc_pending_p)
    interrupt_query ();
  else
    target_interrupt (inferior_ptid);
}

/* Ask the user what to do when an interrupt is received.  */

static void
interrupt_query (void)
{
  struct remote_state *rs = get_remote_state ();

  if (rs->waiting_for_stop_reply && rs->ctrlc_pending_p)
    {
      if (query (_("The target is not responding to interrupt requests.\n"
		   "Stop debugging it? ")))
	{
	  remote_unpush_target ();
	  throw_error (TARGET_CLOSE_ERROR, _("Disconnected from target."));
	}
    }
  else
    {
      if (query (_("Interrupted while waiting for the program.\n"
		   "Give up waiting? ")))
	quit ();
    }
}

/* Enable/disable target terminal ownership.  Most targets can use
   terminal groups to control terminal ownership.  Remote targets are
   different in that explicit transfer of ownership to/from GDB/target
   is required.  */

static void
remote_terminal_inferior (struct target_ops *self)
{
  /* FIXME: cagney/1999-09-27: Make calls to target_terminal_*()
     idempotent.  The event-loop GDB talking to an asynchronous target
     with a synchronous command calls this function from both
     event-top.c and infrun.c/infcmd.c.  Once GDB stops trying to
     transfer the terminal to the target when it shouldn't this guard
     can go away.  */
  if (!remote_async_terminal_ours_p)
    return;
  remote_async_terminal_ours_p = 0;
  /* NOTE: At this point we could also register our selves as the
     recipient of all input.  Any characters typed could then be
     passed on down to the target.  */
}

static void
remote_terminal_ours (struct target_ops *self)
{
  /* See FIXME in remote_terminal_inferior.  */
  if (remote_async_terminal_ours_p)
    return;
  remote_async_terminal_ours_p = 1;
}

static void
remote_console_output (char *msg)
{
  char *p;

  for (p = msg; p[0] && p[1]; p += 2)
    {
      char tb[2];
      char c = fromhex (p[0]) * 16 + fromhex (p[1]);

      tb[0] = c;
      tb[1] = 0;
      fputs_unfiltered (tb, gdb_stdtarg);
    }
  gdb_flush (gdb_stdtarg);
}

typedef struct cached_reg
{
  int num;
  gdb_byte data[MAX_REGISTER_SIZE];
} cached_reg_t;

DEF_VEC_O(cached_reg_t);

typedef struct stop_reply
{
  struct notif_event base;

  /* The identifier of the thread about this event  */
  ptid_t ptid;

  /* The remote state this event is associated with.  When the remote
     connection, represented by a remote_state object, is closed,
     all the associated stop_reply events should be released.  */
  struct remote_state *rs;

  struct target_waitstatus ws;

  /* Expedited registers.  This makes remote debugging a bit more
     efficient for those targets that provide critical registers as
     part of their normal status mechanism (as another roundtrip to
     fetch them is avoided).  */
  VEC(cached_reg_t) *regcache;

  enum target_stop_reason stop_reason;

  CORE_ADDR watch_data_address;

  int core;
} *stop_reply_p;

DECLARE_QUEUE_P (stop_reply_p);
DEFINE_QUEUE_P (stop_reply_p);
/* The list of already fetched and acknowledged stop events.  This
   queue is used for notification Stop, and other notifications
   don't need queue for their events, because the notification events
   of Stop can't be consumed immediately, so that events should be
   queued first, and be consumed by remote_wait_{ns,as} one per
   time.  Other notifications can consume their events immediately,
   so queue is not needed for them.  */
static QUEUE (stop_reply_p) *stop_reply_queue;

static void
stop_reply_xfree (struct stop_reply *r)
{
  notif_event_xfree ((struct notif_event *) r);
}

/* Return the length of the stop reply queue.  */

static int
stop_reply_queue_length (void)
{
  return QUEUE_length (stop_reply_p, stop_reply_queue);
}

static void
remote_notif_stop_parse (struct notif_client *self, char *buf,
			 struct notif_event *event)
{
  remote_parse_stop_reply (buf, (struct stop_reply *) event);
}

static void
remote_notif_stop_ack (struct notif_client *self, char *buf,
		       struct notif_event *event)
{
  struct stop_reply *stop_reply = (struct stop_reply *) event;

  /* acknowledge */
  putpkt ((char *) self->ack_command);

  if (stop_reply->ws.kind == TARGET_WAITKIND_IGNORE)
      /* We got an unknown stop reply.  */
      error (_("Unknown stop reply"));

  push_stop_reply (stop_reply);
}

static int
remote_notif_stop_can_get_pending_events (struct notif_client *self)
{
  /* We can't get pending events in remote_notif_process for
     notification stop, and we have to do this in remote_wait_ns
     instead.  If we fetch all queued events from stub, remote stub
     may exit and we have no chance to process them back in
     remote_wait_ns.  */
  mark_async_event_handler (remote_async_inferior_event_token);
  return 0;
}

static void
stop_reply_dtr (struct notif_event *event)
{
  struct stop_reply *r = (struct stop_reply *) event;

  VEC_free (cached_reg_t, r->regcache);
}

static struct notif_event *
remote_notif_stop_alloc_reply (void)
{
  /* We cast to a pointer to the "base class".  */
  struct notif_event *r = (struct notif_event *) XNEW (struct stop_reply);

  r->dtr = stop_reply_dtr;

  return r;
}

/* A client of notification Stop.  */

struct notif_client notif_client_stop =
{
  "Stop",
  "vStopped",
  remote_notif_stop_parse,
  remote_notif_stop_ack,
  remote_notif_stop_can_get_pending_events,
  remote_notif_stop_alloc_reply,
  REMOTE_NOTIF_STOP,
};

/* A parameter to pass data in and out.  */

struct queue_iter_param
{
  void *input;
  struct stop_reply *output;
};

/* Determine if THREAD is a pending fork parent thread.  ARG contains
   the pid of the process that owns the threads we want to check, or
   -1 if we want to check all threads.  */

static int
is_pending_fork_parent (struct target_waitstatus *ws, int event_pid,
			ptid_t thread_ptid)
{
  if (ws->kind == TARGET_WAITKIND_FORKED
      || ws->kind == TARGET_WAITKIND_VFORKED)
    {
      if (event_pid == -1 || event_pid == ptid_get_pid (thread_ptid))
	return 1;
    }

  return 0;
}

/* Check whether EVENT is a fork event, and if it is, remove the
   fork child from the context list passed in DATA.  */

static int
remove_child_of_pending_fork (QUEUE (stop_reply_p) *q,
			      QUEUE_ITER (stop_reply_p) *iter,
			      stop_reply_p event,
			      void *data)
{
  struct queue_iter_param *param = (struct queue_iter_param *) data;
  struct threads_listing_context *context
    = (struct threads_listing_context *) param->input;

  if (event->ws.kind == TARGET_WAITKIND_FORKED
      || event->ws.kind == TARGET_WAITKIND_VFORKED
      || event->ws.kind == TARGET_WAITKIND_THREAD_EXITED)
    threads_listing_context_remove (&event->ws, context);

  return 1;
}

/* If CONTEXT contains any fork child threads that have not been
   reported yet, remove them from the CONTEXT list.  If such a
   thread exists it is because we are stopped at a fork catchpoint
   and have not yet called follow_fork, which will set up the
   host-side data structures for the new process.  */

static void
remove_new_fork_children (struct threads_listing_context *context)
{
  struct thread_info * thread;
  int pid = -1;
  struct notif_client *notif = &notif_client_stop;
  struct queue_iter_param param;

  /* For any threads stopped at a fork event, remove the corresponding
     fork child threads from the CONTEXT list.  */
  ALL_NON_EXITED_THREADS (thread)
    {
      struct target_waitstatus *ws;

      if (thread->suspend.waitstatus_pending_p)
	ws = &thread->suspend.waitstatus;
      else
	ws = &thread->pending_follow;

      if (is_pending_fork_parent (ws, pid, thread->ptid))
	{
	  threads_listing_context_remove (ws, context);
	}
    }

  /* Check for any pending fork events (not reported or processed yet)
     in process PID and remove those fork child threads from the
     CONTEXT list as well.  */
  remote_notif_get_pending_events (notif);
  param.input = context;
  param.output = NULL;
  QUEUE_iterate (stop_reply_p, stop_reply_queue,
		 remove_child_of_pending_fork, &param);
}

/* Remove stop replies in the queue if its pid is equal to the given
   inferior's pid.  */

static int
remove_stop_reply_for_inferior (QUEUE (stop_reply_p) *q,
				QUEUE_ITER (stop_reply_p) *iter,
				stop_reply_p event,
				void *data)
{
  struct queue_iter_param *param = (struct queue_iter_param *) data;
  struct inferior *inf = (struct inferior *) param->input;

  if (ptid_get_pid (event->ptid) == inf->pid)
    {
      stop_reply_xfree (event);
      QUEUE_remove_elem (stop_reply_p, q, iter);
    }

  return 1;
}

/* Discard all pending stop replies of inferior INF.  */

static void
discard_pending_stop_replies (struct inferior *inf)
{
  struct queue_iter_param param;
  struct stop_reply *reply;
  struct remote_state *rs = get_remote_state ();
  struct remote_notif_state *rns = rs->notif_state;

  /* This function can be notified when an inferior exists.  When the
     target is not remote, the notification state is NULL.  */
  if (rs->remote_desc == NULL)
    return;

  reply = (struct stop_reply *) rns->pending_event[notif_client_stop.id];

  /* Discard the in-flight notification.  */
  if (reply != NULL && ptid_get_pid (reply->ptid) == inf->pid)
    {
      stop_reply_xfree (reply);
      rns->pending_event[notif_client_stop.id] = NULL;
    }

  param.input = inf;
  param.output = NULL;
  /* Discard the stop replies we have already pulled with
     vStopped.  */
  QUEUE_iterate (stop_reply_p, stop_reply_queue,
		 remove_stop_reply_for_inferior, &param);
}

/* If its remote state is equal to the given remote state,
   remove EVENT from the stop reply queue.  */

static int
remove_stop_reply_of_remote_state (QUEUE (stop_reply_p) *q,
				   QUEUE_ITER (stop_reply_p) *iter,
				   stop_reply_p event,
				   void *data)
{
  struct queue_iter_param *param = (struct queue_iter_param *) data;
  struct remote_state *rs = (struct remote_state *) param->input;

  if (event->rs == rs)
    {
      stop_reply_xfree (event);
      QUEUE_remove_elem (stop_reply_p, q, iter);
    }

  return 1;
}

/* Discard the stop replies for RS in stop_reply_queue.  */

static void
discard_pending_stop_replies_in_queue (struct remote_state *rs)
{
  struct queue_iter_param param;

  param.input = rs;
  param.output = NULL;
  /* Discard the stop replies we have already pulled with
     vStopped.  */
  QUEUE_iterate (stop_reply_p, stop_reply_queue,
		 remove_stop_reply_of_remote_state, &param);
}

/* A parameter to pass data in and out.  */

static int
remote_notif_remove_once_on_match (QUEUE (stop_reply_p) *q,
				   QUEUE_ITER (stop_reply_p) *iter,
				   stop_reply_p event,
				   void *data)
{
  struct queue_iter_param *param = (struct queue_iter_param *) data;
  ptid_t *ptid = (ptid_t *) param->input;

  if (ptid_match (event->ptid, *ptid))
    {
      param->output = event;
      QUEUE_remove_elem (stop_reply_p, q, iter);
      return 0;
    }

  return 1;
}

/* Remove the first reply in 'stop_reply_queue' which matches
   PTID.  */

static struct stop_reply *
remote_notif_remove_queued_reply (ptid_t ptid)
{
  struct queue_iter_param param;

  param.input = &ptid;
  param.output = NULL;

  QUEUE_iterate (stop_reply_p, stop_reply_queue,
		 remote_notif_remove_once_on_match, &param);
  if (notif_debug)
    fprintf_unfiltered (gdb_stdlog,
			"notif: discard queued event: 'Stop' in %s\n",
			target_pid_to_str (ptid));

  return param.output;
}

/* Look for a queued stop reply belonging to PTID.  If one is found,
   remove it from the queue, and return it.  Returns NULL if none is
   found.  If there are still queued events left to process, tell the
   event loop to get back to target_wait soon.  */

static struct stop_reply *
queued_stop_reply (ptid_t ptid)
{
  struct stop_reply *r = remote_notif_remove_queued_reply (ptid);

  if (!QUEUE_is_empty (stop_reply_p, stop_reply_queue))
    /* There's still at least an event left.  */
    mark_async_event_handler (remote_async_inferior_event_token);

  return r;
}

/* Push a fully parsed stop reply in the stop reply queue.  Since we
   know that we now have at least one queued event left to pass to the
   core side, tell the event loop to get back to target_wait soon.  */

static void
push_stop_reply (struct stop_reply *new_event)
{
  QUEUE_enque (stop_reply_p, stop_reply_queue, new_event);

  if (notif_debug)
    fprintf_unfiltered (gdb_stdlog,
			"notif: push 'Stop' %s to queue %d\n",
			target_pid_to_str (new_event->ptid),
			QUEUE_length (stop_reply_p,
				      stop_reply_queue));

  mark_async_event_handler (remote_async_inferior_event_token);
}

static int
stop_reply_match_ptid_and_ws (QUEUE (stop_reply_p) *q,
			      QUEUE_ITER (stop_reply_p) *iter,
			      struct stop_reply *event,
			      void *data)
{
  ptid_t *ptid = (ptid_t *) data;

  return !(ptid_equal (*ptid, event->ptid)
	   && event->ws.kind == TARGET_WAITKIND_STOPPED);
}

/* Returns true if we have a stop reply for PTID.  */

static int
peek_stop_reply (ptid_t ptid)
{
  return !QUEUE_iterate (stop_reply_p, stop_reply_queue,
			 stop_reply_match_ptid_and_ws, &ptid);
}

/* Helper for remote_parse_stop_reply.  Return nonzero if the substring
   starting with P and ending with PEND matches PREFIX.  */

static int
strprefix (const char *p, const char *pend, const char *prefix)
{
  for ( ; p < pend; p++, prefix++)
    if (*p != *prefix)
      return 0;
  return *prefix == '\0';
}

/* Parse the stop reply in BUF.  Either the function succeeds, and the
   result is stored in EVENT, or throws an error.  */

static void
remote_parse_stop_reply (char *buf, struct stop_reply *event)
{
  struct remote_arch_state *rsa = get_remote_arch_state ();
  ULONGEST addr;
  char *p;
  int skipregs = 0;

  event->ptid = null_ptid;
  event->rs = get_remote_state ();
  event->ws.kind = TARGET_WAITKIND_IGNORE;
  event->ws.value.integer = 0;
  event->stop_reason = TARGET_STOPPED_BY_NO_REASON;
  event->regcache = NULL;
  event->core = -1;

  switch (buf[0])
    {
    case 'T':		/* Status with PC, SP, FP, ...	*/
      /* Expedited reply, containing Signal, {regno, reg} repeat.  */
      /*  format is:  'Tssn...:r...;n...:r...;n...:r...;#cc', where
	    ss = signal number
	    n... = register number
	    r... = register contents
      */

      p = &buf[3];	/* after Txx */
      while (*p)
	{
	  char *p1;
	  int fieldsize;

	  p1 = strchr (p, ':');
	  if (p1 == NULL)
	    error (_("Malformed packet(a) (missing colon): %s\n\
Packet: '%s'\n"),
		   p, buf);
	  if (p == p1)
	    error (_("Malformed packet(a) (missing register number): %s\n\
Packet: '%s'\n"),
		   p, buf);

	  /* Some "registers" are actually extended stop information.
	     Note if you're adding a new entry here: GDB 7.9 and
	     earlier assume that all register "numbers" that start
	     with an hex digit are real register numbers.  Make sure
	     the server only sends such a packet if it knows the
	     client understands it.  */

	  if (strprefix (p, p1, "thread"))
	    event->ptid = read_ptid (++p1, &p);
	  else if (strprefix (p, p1, "syscall_entry"))
	    {
	      ULONGEST sysno;

	      event->ws.kind = TARGET_WAITKIND_SYSCALL_ENTRY;
	      p = unpack_varlen_hex (++p1, &sysno);
	      event->ws.value.syscall_number = (int) sysno;
	    }
	  else if (strprefix (p, p1, "syscall_return"))
	    {
	      ULONGEST sysno;

	      event->ws.kind = TARGET_WAITKIND_SYSCALL_RETURN;
	      p = unpack_varlen_hex (++p1, &sysno);
	      event->ws.value.syscall_number = (int) sysno;
	    }
	  else if (strprefix (p, p1, "watch")
		   || strprefix (p, p1, "rwatch")
		   || strprefix (p, p1, "awatch"))
	    {
	      event->stop_reason = TARGET_STOPPED_BY_WATCHPOINT;
	      p = unpack_varlen_hex (++p1, &addr);
	      event->watch_data_address = (CORE_ADDR) addr;
	    }
	  else if (strprefix (p, p1, "swbreak"))
	    {
	      event->stop_reason = TARGET_STOPPED_BY_SW_BREAKPOINT;

	      /* Make sure the stub doesn't forget to indicate support
		 with qSupported.  */
	      if (packet_support (PACKET_swbreak_feature) != PACKET_ENABLE)
		error (_("Unexpected swbreak stop reason"));

	      /* The value part is documented as "must be empty",
		 though we ignore it, in case we ever decide to make
		 use of it in a backward compatible way.  */
	      p = strchrnul (p1 + 1, ';');
	    }
	  else if (strprefix (p, p1, "hwbreak"))
	    {
	      event->stop_reason = TARGET_STOPPED_BY_HW_BREAKPOINT;

	      /* Make sure the stub doesn't forget to indicate support
		 with qSupported.  */
	      if (packet_support (PACKET_hwbreak_feature) != PACKET_ENABLE)
		error (_("Unexpected hwbreak stop reason"));

	      /* See above.  */
	      p = strchrnul (p1 + 1, ';');
	    }
	  else if (strprefix (p, p1, "library"))
	    {
	      event->ws.kind = TARGET_WAITKIND_LOADED;
	      p = strchrnul (p1 + 1, ';');
	    }
	  else if (strprefix (p, p1, "replaylog"))
	    {
	      event->ws.kind = TARGET_WAITKIND_NO_HISTORY;
	      /* p1 will indicate "begin" or "end", but it makes
		 no difference for now, so ignore it.  */
	      p = strchrnul (p1 + 1, ';');
	    }
	  else if (strprefix (p, p1, "core"))
	    {
	      ULONGEST c;

	      p = unpack_varlen_hex (++p1, &c);
	      event->core = c;
	    }
	  else if (strprefix (p, p1, "fork"))
	    {
	      event->ws.value.related_pid = read_ptid (++p1, &p);
	      event->ws.kind = TARGET_WAITKIND_FORKED;
	    }
	  else if (strprefix (p, p1, "vfork"))
	    {
	      event->ws.value.related_pid = read_ptid (++p1, &p);
	      event->ws.kind = TARGET_WAITKIND_VFORKED;
	    }
	  else if (strprefix (p, p1, "vforkdone"))
	    {
	      event->ws.kind = TARGET_WAITKIND_VFORK_DONE;
	      p = strchrnul (p1 + 1, ';');
	    }
	  else if (strprefix (p, p1, "exec"))
	    {
	      ULONGEST ignored;
	      char pathname[PATH_MAX];
	      int pathlen;

	      /* Determine the length of the execd pathname.  */
	      p = unpack_varlen_hex (++p1, &ignored);
	      pathlen = (p - p1) / 2;

	      /* Save the pathname for event reporting and for
		 the next run command.  */
	      hex2bin (p1, (gdb_byte *) pathname, pathlen);
	      pathname[pathlen] = '\0';

	      /* This is freed during event handling.  */
	      event->ws.value.execd_pathname = xstrdup (pathname);
	      event->ws.kind = TARGET_WAITKIND_EXECD;

	      /* Skip the registers included in this packet, since
		 they may be for an architecture different from the
		 one used by the original program.  */
	      skipregs = 1;
	    }
	  else if (strprefix (p, p1, "create"))
	    {
	      event->ws.kind = TARGET_WAITKIND_THREAD_CREATED;
	      p = strchrnul (p1 + 1, ';');
	    }
	  else
	    {
	      ULONGEST pnum;
	      char *p_temp;

	      if (skipregs)
		{
		  p = strchrnul (p1 + 1, ';');
		  p++;
		  continue;
		}

	      /* Maybe a real ``P'' register number.  */
	      p_temp = unpack_varlen_hex (p, &pnum);
	      /* If the first invalid character is the colon, we got a
		 register number.  Otherwise, it's an unknown stop
		 reason.  */
	      if (p_temp == p1)
		{
		  struct packet_reg *reg = packet_reg_from_pnum (rsa, pnum);
		  cached_reg_t cached_reg;

		  if (reg == NULL)
		    error (_("Remote sent bad register number %s: %s\n\
Packet: '%s'\n"),
			   hex_string (pnum), p, buf);

		  cached_reg.num = reg->regnum;

		  p = p1 + 1;
		  fieldsize = hex2bin (p, cached_reg.data,
				       register_size (target_gdbarch (),
						      reg->regnum));
		  p += 2 * fieldsize;
		  if (fieldsize < register_size (target_gdbarch (),
						 reg->regnum))
		    warning (_("Remote reply is too short: %s"), buf);

		  VEC_safe_push (cached_reg_t, event->regcache, &cached_reg);
		}
	      else
		{
		  /* Not a number.  Silently skip unknown optional
		     info.  */
		  p = strchrnul (p1 + 1, ';');
		}
	    }

	  if (*p != ';')
	    error (_("Remote register badly formatted: %s\nhere: %s"),
		   buf, p);
	  ++p;
	}

      if (event->ws.kind != TARGET_WAITKIND_IGNORE)
	break;

      /* fall through */
    case 'S':		/* Old style status, just signal only.  */
      {
	int sig;

	event->ws.kind = TARGET_WAITKIND_STOPPED;
	sig = (fromhex (buf[1]) << 4) + fromhex (buf[2]);
	if (GDB_SIGNAL_FIRST <= sig && sig < GDB_SIGNAL_LAST)
	  event->ws.value.sig = (enum gdb_signal) sig;
	else
	  event->ws.value.sig = GDB_SIGNAL_UNKNOWN;
      }
      break;
    case 'w':		/* Thread exited.  */
      {
	char *p;
	ULONGEST value;

	event->ws.kind = TARGET_WAITKIND_THREAD_EXITED;
	p = unpack_varlen_hex (&buf[1], &value);
	event->ws.value.integer = value;
	if (*p != ';')
	  error (_("stop reply packet badly formatted: %s"), buf);
	event->ptid = read_ptid (++p, NULL);
	break;
      }
    case 'W':		/* Target exited.  */
    case 'X':
      {
	char *p;
	int pid;
	ULONGEST value;

	/* GDB used to accept only 2 hex chars here.  Stubs should
	   only send more if they detect GDB supports multi-process
	   support.  */
	p = unpack_varlen_hex (&buf[1], &value);

	if (buf[0] == 'W')
	  {
	    /* The remote process exited.  */
	    event->ws.kind = TARGET_WAITKIND_EXITED;
	    event->ws.value.integer = value;
	  }
	else
	  {
	    /* The remote process exited with a signal.  */
	    event->ws.kind = TARGET_WAITKIND_SIGNALLED;
	    if (GDB_SIGNAL_FIRST <= value && value < GDB_SIGNAL_LAST)
	      event->ws.value.sig = (enum gdb_signal) value;
	    else
	      event->ws.value.sig = GDB_SIGNAL_UNKNOWN;
	  }

	/* If no process is specified, assume inferior_ptid.  */
	pid = ptid_get_pid (inferior_ptid);
	if (*p == '\0')
	  ;
	else if (*p == ';')
	  {
	    p++;

	    if (*p == '\0')
	      ;
	    else if (startswith (p, "process:"))
	      {
		ULONGEST upid;

		p += sizeof ("process:") - 1;
		unpack_varlen_hex (p, &upid);
		pid = upid;
	      }
	    else
	      error (_("unknown stop reply packet: %s"), buf);
	  }
	else
	  error (_("unknown stop reply packet: %s"), buf);
	event->ptid = pid_to_ptid (pid);
      }
      break;
    case 'N':
      event->ws.kind = TARGET_WAITKIND_NO_RESUMED;
      event->ptid = minus_one_ptid;
      break;
    }

  if (target_is_non_stop_p () && ptid_equal (event->ptid, null_ptid))
    error (_("No process or thread specified in stop reply: %s"), buf);
}

/* When the stub wants to tell GDB about a new notification reply, it
   sends a notification (%Stop, for example).  Those can come it at
   any time, hence, we have to make sure that any pending
   putpkt/getpkt sequence we're making is finished, before querying
   the stub for more events with the corresponding ack command
   (vStopped, for example).  E.g., if we started a vStopped sequence
   immediately upon receiving the notification, something like this
   could happen:

    1.1) --> Hg 1
    1.2) <-- OK
    1.3) --> g
    1.4) <-- %Stop
    1.5) --> vStopped
    1.6) <-- (registers reply to step #1.3)

   Obviously, the reply in step #1.6 would be unexpected to a vStopped
   query.

   To solve this, whenever we parse a %Stop notification successfully,
   we mark the REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN, and carry on
   doing whatever we were doing:

    2.1) --> Hg 1
    2.2) <-- OK
    2.3) --> g
    2.4) <-- %Stop
      <GDB marks the REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN>
    2.5) <-- (registers reply to step #2.3)

   Eventualy after step #2.5, we return to the event loop, which
   notices there's an event on the
   REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN event and calls the
   associated callback --- the function below.  At this point, we're
   always safe to start a vStopped sequence. :

    2.6) --> vStopped
    2.7) <-- T05 thread:2
    2.8) --> vStopped
    2.9) --> OK
*/

void
remote_notif_get_pending_events (struct notif_client *nc)
{
  struct remote_state *rs = get_remote_state ();

  if (rs->notif_state->pending_event[nc->id] != NULL)
    {
      if (notif_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "notif: process: '%s' ack pending event\n",
			    nc->name);

      /* acknowledge */
      nc->ack (nc, rs->buf, rs->notif_state->pending_event[nc->id]);
      rs->notif_state->pending_event[nc->id] = NULL;

      while (1)
	{
	  getpkt (&rs->buf, &rs->buf_size, 0);
	  if (strcmp (rs->buf, "OK") == 0)
	    break;
	  else
	    remote_notif_ack (nc, rs->buf);
	}
    }
  else
    {
      if (notif_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "notif: process: '%s' no pending reply\n",
			    nc->name);
    }
}

/* Called when it is decided that STOP_REPLY holds the info of the
   event that is to be returned to the core.  This function always
   destroys STOP_REPLY.  */

static ptid_t
process_stop_reply (struct stop_reply *stop_reply,
		    struct target_waitstatus *status)
{
  ptid_t ptid;

  *status = stop_reply->ws;
  ptid = stop_reply->ptid;

  /* If no thread/process was reported by the stub, assume the current
     inferior.  */
  if (ptid_equal (ptid, null_ptid))
    ptid = inferior_ptid;

  if (status->kind != TARGET_WAITKIND_EXITED
      && status->kind != TARGET_WAITKIND_SIGNALLED
      && status->kind != TARGET_WAITKIND_NO_RESUMED)
    {
      struct private_thread_info *remote_thr;

      /* Expedited registers.  */
      if (stop_reply->regcache)
	{
	  struct regcache *regcache
	    = get_thread_arch_regcache (ptid, target_gdbarch ());
	  cached_reg_t *reg;
	  int ix;

	  for (ix = 0;
	       VEC_iterate(cached_reg_t, stop_reply->regcache, ix, reg);
	       ix++)
	    regcache_raw_supply (regcache, reg->num, reg->data);
	  VEC_free (cached_reg_t, stop_reply->regcache);
	}

      remote_notice_new_inferior (ptid, 0);
      remote_thr = demand_private_info (ptid);
      remote_thr->core = stop_reply->core;
      remote_thr->stop_reason = stop_reply->stop_reason;
      remote_thr->watch_data_address = stop_reply->watch_data_address;
    }

  stop_reply_xfree (stop_reply);
  return ptid;
}

/* The non-stop mode version of target_wait.  */

static ptid_t
remote_wait_ns (ptid_t ptid, struct target_waitstatus *status, int options)
{
  struct remote_state *rs = get_remote_state ();
  struct stop_reply *stop_reply;
  int ret;
  int is_notif = 0;

  /* If in non-stop mode, get out of getpkt even if a
     notification is received.	*/

  ret = getpkt_or_notif_sane (&rs->buf, &rs->buf_size,
			      0 /* forever */, &is_notif);
  while (1)
    {
      if (ret != -1 && !is_notif)
	switch (rs->buf[0])
	  {
	  case 'E':		/* Error of some sort.	*/
	    /* We're out of sync with the target now.  Did it continue
	       or not?  We can't tell which thread it was in non-stop,
	       so just ignore this.  */
	    warning (_("Remote failure reply: %s"), rs->buf);
	    break;
	  case 'O':		/* Console output.  */
	    remote_console_output (rs->buf + 1);
	    break;
	  default:
	    warning (_("Invalid remote reply: %s"), rs->buf);
	    break;
	  }

      /* Acknowledge a pending stop reply that may have arrived in the
	 mean time.  */
      if (rs->notif_state->pending_event[notif_client_stop.id] != NULL)
	remote_notif_get_pending_events (&notif_client_stop);

      /* If indeed we noticed a stop reply, we're done.  */
      stop_reply = queued_stop_reply (ptid);
      if (stop_reply != NULL)
	return process_stop_reply (stop_reply, status);

      /* Still no event.  If we're just polling for an event, then
	 return to the event loop.  */
      if (options & TARGET_WNOHANG)
	{
	  status->kind = TARGET_WAITKIND_IGNORE;
	  return minus_one_ptid;
	}

      /* Otherwise do a blocking wait.  */
      ret = getpkt_or_notif_sane (&rs->buf, &rs->buf_size,
				  1 /* forever */, &is_notif);
    }
}

/* Wait until the remote machine stops, then return, storing status in
   STATUS just as `wait' would.  */

static ptid_t
remote_wait_as (ptid_t ptid, struct target_waitstatus *status, int options)
{
  struct remote_state *rs = get_remote_state ();
  ptid_t event_ptid = null_ptid;
  char *buf;
  struct stop_reply *stop_reply;

 again:

  status->kind = TARGET_WAITKIND_IGNORE;
  status->value.integer = 0;

  stop_reply = queued_stop_reply (ptid);
  if (stop_reply != NULL)
    return process_stop_reply (stop_reply, status);

  if (rs->cached_wait_status)
    /* Use the cached wait status, but only once.  */
    rs->cached_wait_status = 0;
  else
    {
      int ret;
      int is_notif;
      int forever = ((options & TARGET_WNOHANG) == 0
		     && wait_forever_enabled_p);

      if (!rs->waiting_for_stop_reply)
	{
	  status->kind = TARGET_WAITKIND_NO_RESUMED;
	  return minus_one_ptid;
	}

      /* FIXME: cagney/1999-09-27: If we're in async mode we should
	 _never_ wait for ever -> test on target_is_async_p().
	 However, before we do that we need to ensure that the caller
	 knows how to take the target into/out of async mode.  */
      ret = getpkt_or_notif_sane (&rs->buf, &rs->buf_size,
				  forever, &is_notif);

      /* GDB gets a notification.  Return to core as this event is
	 not interesting.  */
      if (ret != -1 && is_notif)
	return minus_one_ptid;

      if (ret == -1 && (options & TARGET_WNOHANG) != 0)
	return minus_one_ptid;
    }

  buf = rs->buf;

  /* Assume that the target has acknowledged Ctrl-C unless we receive
     an 'F' or 'O' packet.  */
  if (buf[0] != 'F' && buf[0] != 'O')
    rs->ctrlc_pending_p = 0;

  switch (buf[0])
    {
    case 'E':		/* Error of some sort.	*/
      /* We're out of sync with the target now.  Did it continue or
	 not?  Not is more likely, so report a stop.  */
      rs->waiting_for_stop_reply = 0;

      warning (_("Remote failure reply: %s"), buf);
      status->kind = TARGET_WAITKIND_STOPPED;
      status->value.sig = GDB_SIGNAL_0;
      break;
    case 'F':		/* File-I/O request.  */
      /* GDB may access the inferior memory while handling the File-I/O
	 request, but we don't want GDB accessing memory while waiting
	 for a stop reply.  See the comments in putpkt_binary.  Set
	 waiting_for_stop_reply to 0 temporarily.  */
      rs->waiting_for_stop_reply = 0;
      remote_fileio_request (buf, rs->ctrlc_pending_p);
      rs->ctrlc_pending_p = 0;
      /* GDB handled the File-I/O request, and the target is running
	 again.  Keep waiting for events.  */
      rs->waiting_for_stop_reply = 1;
      break;
    case 'N': case 'T': case 'S': case 'X': case 'W':
      {
	struct stop_reply *stop_reply;

	/* There is a stop reply to handle.  */
	rs->waiting_for_stop_reply = 0;

	stop_reply
	  = (struct stop_reply *) remote_notif_parse (&notif_client_stop,
						      rs->buf);

	event_ptid = process_stop_reply (stop_reply, status);
	break;
      }
    case 'O':		/* Console output.  */
      remote_console_output (buf + 1);
      break;
    case '\0':
      if (rs->last_sent_signal != GDB_SIGNAL_0)
	{
	  /* Zero length reply means that we tried 'S' or 'C' and the
	     remote system doesn't support it.  */
	  target_terminal_ours_for_output ();
	  printf_filtered
	    ("Can't send signals to this remote system.  %s not sent.\n",
	     gdb_signal_to_name (rs->last_sent_signal));
	  rs->last_sent_signal = GDB_SIGNAL_0;
	  target_terminal_inferior ();

	  strcpy ((char *) buf, rs->last_sent_step ? "s" : "c");
	  putpkt ((char *) buf);
	  break;
	}
      /* else fallthrough */
    default:
      warning (_("Invalid remote reply: %s"), buf);
      break;
    }

  if (status->kind == TARGET_WAITKIND_NO_RESUMED)
    return minus_one_ptid;
  else if (status->kind == TARGET_WAITKIND_IGNORE)
    {
      /* Nothing interesting happened.  If we're doing a non-blocking
	 poll, we're done.  Otherwise, go back to waiting.  */
      if (options & TARGET_WNOHANG)
	return minus_one_ptid;
      else
	goto again;
    }
  else if (status->kind != TARGET_WAITKIND_EXITED
	   && status->kind != TARGET_WAITKIND_SIGNALLED)
    {
      if (!ptid_equal (event_ptid, null_ptid))
	record_currthread (rs, event_ptid);
      else
	event_ptid = inferior_ptid;
    }
  else
    /* A process exit.  Invalidate our notion of current thread.  */
    record_currthread (rs, minus_one_ptid);

  return event_ptid;
}

/* Wait until the remote machine stops, then return, storing status in
   STATUS just as `wait' would.  */

static ptid_t
remote_wait (struct target_ops *ops,
	     ptid_t ptid, struct target_waitstatus *status, int options)
{
  ptid_t event_ptid;

  if (target_is_non_stop_p ())
    event_ptid = remote_wait_ns (ptid, status, options);
  else
    event_ptid = remote_wait_as (ptid, status, options);

  if (target_is_async_p ())
    {
      /* If there are are events left in the queue tell the event loop
	 to return here.  */
      if (!QUEUE_is_empty (stop_reply_p, stop_reply_queue))
	mark_async_event_handler (remote_async_inferior_event_token);
    }

  return event_ptid;
}

/* Fetch a single register using a 'p' packet.  */

static int
fetch_register_using_p (struct regcache *regcache, struct packet_reg *reg)
{
  struct remote_state *rs = get_remote_state ();
  char *buf, *p;
  char regp[MAX_REGISTER_SIZE];
  int i;

  if (packet_support (PACKET_p) == PACKET_DISABLE)
    return 0;

  if (reg->pnum == -1)
    return 0;

  p = rs->buf;
  *p++ = 'p';
  p += hexnumstr (p, reg->pnum);
  *p++ = '\0';
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  buf = rs->buf;

  switch (packet_ok (buf, &remote_protocol_packets[PACKET_p]))
    {
    case PACKET_OK:
      break;
    case PACKET_UNKNOWN:
      return 0;
    case PACKET_ERROR:
      error (_("Could not fetch register \"%s\"; remote failure reply '%s'"),
	     gdbarch_register_name (get_regcache_arch (regcache), 
				    reg->regnum), 
	     buf);
    }

  /* If this register is unfetchable, tell the regcache.  */
  if (buf[0] == 'x')
    {
      regcache_raw_supply (regcache, reg->regnum, NULL);
      return 1;
    }

  /* Otherwise, parse and supply the value.  */
  p = buf;
  i = 0;
  while (p[0] != 0)
    {
      if (p[1] == 0)
	error (_("fetch_register_using_p: early buf termination"));

      regp[i++] = fromhex (p[0]) * 16 + fromhex (p[1]);
      p += 2;
    }
  regcache_raw_supply (regcache, reg->regnum, regp);
  return 1;
}

/* Fetch the registers included in the target's 'g' packet.  */

static int
send_g_packet (void)
{
  struct remote_state *rs = get_remote_state ();
  int buf_len;

  xsnprintf (rs->buf, get_remote_packet_size (), "g");
  remote_send (&rs->buf, &rs->buf_size);

  /* We can get out of synch in various cases.  If the first character
     in the buffer is not a hex character, assume that has happened
     and try to fetch another packet to read.  */
  while ((rs->buf[0] < '0' || rs->buf[0] > '9')
	 && (rs->buf[0] < 'A' || rs->buf[0] > 'F')
	 && (rs->buf[0] < 'a' || rs->buf[0] > 'f')
	 && rs->buf[0] != 'x')	/* New: unavailable register value.  */
    {
      if (remote_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "Bad register packet; fetching a new packet\n");
      getpkt (&rs->buf, &rs->buf_size, 0);
    }

  buf_len = strlen (rs->buf);

  /* Sanity check the received packet.  */
  if (buf_len % 2 != 0)
    error (_("Remote 'g' packet reply is of odd length: %s"), rs->buf);

  return buf_len / 2;
}

static void
process_g_packet (struct regcache *regcache)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct remote_state *rs = get_remote_state ();
  struct remote_arch_state *rsa = get_remote_arch_state ();
  int i, buf_len;
  char *p;
  char *regs;

  buf_len = strlen (rs->buf);

  /* Further sanity checks, with knowledge of the architecture.  */
  if (buf_len > 2 * rsa->sizeof_g_packet)
    error (_("Remote 'g' packet reply is too long: %s"), rs->buf);

  /* Save the size of the packet sent to us by the target.  It is used
     as a heuristic when determining the max size of packets that the
     target can safely receive.  */
  if (rsa->actual_register_packet_size == 0)
    rsa->actual_register_packet_size = buf_len;

  /* If this is smaller than we guessed the 'g' packet would be,
     update our records.  A 'g' reply that doesn't include a register's
     value implies either that the register is not available, or that
     the 'p' packet must be used.  */
  if (buf_len < 2 * rsa->sizeof_g_packet)
    {
      rsa->sizeof_g_packet = buf_len / 2;

      for (i = 0; i < gdbarch_num_regs (gdbarch); i++)
	{
	  if (rsa->regs[i].pnum == -1)
	    continue;

	  if (rsa->regs[i].offset >= rsa->sizeof_g_packet)
	    rsa->regs[i].in_g_packet = 0;
	  else
	    rsa->regs[i].in_g_packet = 1;
	}
    }

  regs = (char *) alloca (rsa->sizeof_g_packet);

  /* Unimplemented registers read as all bits zero.  */
  memset (regs, 0, rsa->sizeof_g_packet);

  /* Reply describes registers byte by byte, each byte encoded as two
     hex characters.  Suck them all up, then supply them to the
     register cacheing/storage mechanism.  */

  p = rs->buf;
  for (i = 0; i < rsa->sizeof_g_packet; i++)
    {
      if (p[0] == 0 || p[1] == 0)
	/* This shouldn't happen - we adjusted sizeof_g_packet above.  */
	internal_error (__FILE__, __LINE__,
			_("unexpected end of 'g' packet reply"));

      if (p[0] == 'x' && p[1] == 'x')
	regs[i] = 0;		/* 'x' */
      else
	regs[i] = fromhex (p[0]) * 16 + fromhex (p[1]);
      p += 2;
    }

  for (i = 0; i < gdbarch_num_regs (gdbarch); i++)
    {
      struct packet_reg *r = &rsa->regs[i];

      if (r->in_g_packet)
	{
	  if (r->offset * 2 >= strlen (rs->buf))
	    /* This shouldn't happen - we adjusted in_g_packet above.  */
	    internal_error (__FILE__, __LINE__,
			    _("unexpected end of 'g' packet reply"));
	  else if (rs->buf[r->offset * 2] == 'x')
	    {
	      gdb_assert (r->offset * 2 < strlen (rs->buf));
	      /* The register isn't available, mark it as such (at
		 the same time setting the value to zero).  */
	      regcache_raw_supply (regcache, r->regnum, NULL);
	    }
	  else
	    regcache_raw_supply (regcache, r->regnum,
				 regs + r->offset);
	}
    }
}

static void
fetch_registers_using_g (struct regcache *regcache)
{
  send_g_packet ();
  process_g_packet (regcache);
}

/* Make the remote selected traceframe match GDB's selected
   traceframe.  */

static void
set_remote_traceframe (void)
{
  int newnum;
  struct remote_state *rs = get_remote_state ();

  if (rs->remote_traceframe_number == get_traceframe_number ())
    return;

  /* Avoid recursion, remote_trace_find calls us again.  */
  rs->remote_traceframe_number = get_traceframe_number ();

  newnum = target_trace_find (tfind_number,
			      get_traceframe_number (), 0, 0, NULL);

  /* Should not happen.  If it does, all bets are off.  */
  if (newnum != get_traceframe_number ())
    warning (_("could not set remote traceframe"));
}

static void
remote_fetch_registers (struct target_ops *ops,
			struct regcache *regcache, int regnum)
{
  struct remote_arch_state *rsa = get_remote_arch_state ();
  int i;

  set_remote_traceframe ();
  set_general_thread (inferior_ptid);

  if (regnum >= 0)
    {
      struct packet_reg *reg = packet_reg_from_regnum (rsa, regnum);

      gdb_assert (reg != NULL);

      /* If this register might be in the 'g' packet, try that first -
	 we are likely to read more than one register.  If this is the
	 first 'g' packet, we might be overly optimistic about its
	 contents, so fall back to 'p'.  */
      if (reg->in_g_packet)
	{
	  fetch_registers_using_g (regcache);
	  if (reg->in_g_packet)
	    return;
	}

      if (fetch_register_using_p (regcache, reg))
	return;

      /* This register is not available.  */
      regcache_raw_supply (regcache, reg->regnum, NULL);

      return;
    }

  fetch_registers_using_g (regcache);

  for (i = 0; i < gdbarch_num_regs (get_regcache_arch (regcache)); i++)
    if (!rsa->regs[i].in_g_packet)
      if (!fetch_register_using_p (regcache, &rsa->regs[i]))
	{
	  /* This register is not available.  */
	  regcache_raw_supply (regcache, i, NULL);
	}
}

/* Prepare to store registers.  Since we may send them all (using a
   'G' request), we have to read out the ones we don't want to change
   first.  */

static void
remote_prepare_to_store (struct target_ops *self, struct regcache *regcache)
{
  struct remote_arch_state *rsa = get_remote_arch_state ();
  int i;
  gdb_byte buf[MAX_REGISTER_SIZE];

  /* Make sure the entire registers array is valid.  */
  switch (packet_support (PACKET_P))
    {
    case PACKET_DISABLE:
    case PACKET_SUPPORT_UNKNOWN:
      /* Make sure all the necessary registers are cached.  */
      for (i = 0; i < gdbarch_num_regs (get_regcache_arch (regcache)); i++)
	if (rsa->regs[i].in_g_packet)
	  regcache_raw_read (regcache, rsa->regs[i].regnum, buf);
      break;
    case PACKET_ENABLE:
      break;
    }
}

/* Helper: Attempt to store REGNUM using the P packet.  Return fail IFF
   packet was not recognized.  */

static int
store_register_using_P (const struct regcache *regcache, 
			struct packet_reg *reg)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct remote_state *rs = get_remote_state ();
  /* Try storing a single register.  */
  char *buf = rs->buf;
  gdb_byte regp[MAX_REGISTER_SIZE];
  char *p;

  if (packet_support (PACKET_P) == PACKET_DISABLE)
    return 0;

  if (reg->pnum == -1)
    return 0;

  xsnprintf (buf, get_remote_packet_size (), "P%s=", phex_nz (reg->pnum, 0));
  p = buf + strlen (buf);
  regcache_raw_collect (regcache, reg->regnum, regp);
  bin2hex (regp, p, register_size (gdbarch, reg->regnum));
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  switch (packet_ok (rs->buf, &remote_protocol_packets[PACKET_P]))
    {
    case PACKET_OK:
      return 1;
    case PACKET_ERROR:
      error (_("Could not write register \"%s\"; remote failure reply '%s'"),
	     gdbarch_register_name (gdbarch, reg->regnum), rs->buf);
    case PACKET_UNKNOWN:
      return 0;
    default:
      internal_error (__FILE__, __LINE__, _("Bad result from packet_ok"));
    }
}

/* Store register REGNUM, or all registers if REGNUM == -1, from the
   contents of the register cache buffer.  FIXME: ignores errors.  */

static void
store_registers_using_G (const struct regcache *regcache)
{
  struct remote_state *rs = get_remote_state ();
  struct remote_arch_state *rsa = get_remote_arch_state ();
  gdb_byte *regs;
  char *p;

  /* Extract all the registers in the regcache copying them into a
     local buffer.  */
  {
    int i;

    regs = (gdb_byte *) alloca (rsa->sizeof_g_packet);
    memset (regs, 0, rsa->sizeof_g_packet);
    for (i = 0; i < gdbarch_num_regs (get_regcache_arch (regcache)); i++)
      {
	struct packet_reg *r = &rsa->regs[i];

	if (r->in_g_packet)
	  regcache_raw_collect (regcache, r->regnum, regs + r->offset);
      }
  }

  /* Command describes registers byte by byte,
     each byte encoded as two hex characters.  */
  p = rs->buf;
  *p++ = 'G';
  /* remote_prepare_to_store insures that rsa->sizeof_g_packet gets
     updated.  */
  bin2hex (regs, p, rsa->sizeof_g_packet);
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);
  if (packet_check_result (rs->buf) == PACKET_ERROR)
    error (_("Could not write registers; remote failure reply '%s'"), 
	   rs->buf);
}

/* Store register REGNUM, or all registers if REGNUM == -1, from the contents
   of the register cache buffer.  FIXME: ignores errors.  */

static void
remote_store_registers (struct target_ops *ops,
			struct regcache *regcache, int regnum)
{
  struct remote_arch_state *rsa = get_remote_arch_state ();
  int i;

  set_remote_traceframe ();
  set_general_thread (inferior_ptid);

  if (regnum >= 0)
    {
      struct packet_reg *reg = packet_reg_from_regnum (rsa, regnum);

      gdb_assert (reg != NULL);

      /* Always prefer to store registers using the 'P' packet if
	 possible; we often change only a small number of registers.
	 Sometimes we change a larger number; we'd need help from a
	 higher layer to know to use 'G'.  */
      if (store_register_using_P (regcache, reg))
	return;

      /* For now, don't complain if we have no way to write the
	 register.  GDB loses track of unavailable registers too
	 easily.  Some day, this may be an error.  We don't have
	 any way to read the register, either...  */
      if (!reg->in_g_packet)
	return;

      store_registers_using_G (regcache);
      return;
    }

  store_registers_using_G (regcache);

  for (i = 0; i < gdbarch_num_regs (get_regcache_arch (regcache)); i++)
    if (!rsa->regs[i].in_g_packet)
      if (!store_register_using_P (regcache, &rsa->regs[i]))
	/* See above for why we do not issue an error here.  */
	continue;
}


/* Return the number of hex digits in num.  */

static int
hexnumlen (ULONGEST num)
{
  int i;

  for (i = 0; num != 0; i++)
    num >>= 4;

  return max (i, 1);
}

/* Set BUF to the minimum number of hex digits representing NUM.  */

static int
hexnumstr (char *buf, ULONGEST num)
{
  int len = hexnumlen (num);

  return hexnumnstr (buf, num, len);
}


/* Set BUF to the hex digits representing NUM, padded to WIDTH characters.  */

static int
hexnumnstr (char *buf, ULONGEST num, int width)
{
  int i;

  buf[width] = '\0';

  for (i = width - 1; i >= 0; i--)
    {
      buf[i] = "0123456789abcdef"[(num & 0xf)];
      num >>= 4;
    }

  return width;
}

/* Mask all but the least significant REMOTE_ADDRESS_SIZE bits.  */

static CORE_ADDR
remote_address_masked (CORE_ADDR addr)
{
  unsigned int address_size = remote_address_size;

  /* If "remoteaddresssize" was not set, default to target address size.  */
  if (!address_size)
    address_size = gdbarch_addr_bit (target_gdbarch ());

  if (address_size > 0
      && address_size < (sizeof (ULONGEST) * 8))
    {
      /* Only create a mask when that mask can safely be constructed
         in a ULONGEST variable.  */
      ULONGEST mask = 1;

      mask = (mask << address_size) - 1;
      addr &= mask;
    }
  return addr;
}

/* Determine whether the remote target supports binary downloading.
   This is accomplished by sending a no-op memory write of zero length
   to the target at the specified address. It does not suffice to send
   the whole packet, since many stubs strip the eighth bit and
   subsequently compute a wrong checksum, which causes real havoc with
   remote_write_bytes.

   NOTE: This can still lose if the serial line is not eight-bit
   clean.  In cases like this, the user should clear "remote
   X-packet".  */

static void
check_binary_download (CORE_ADDR addr)
{
  struct remote_state *rs = get_remote_state ();

  switch (packet_support (PACKET_X))
    {
    case PACKET_DISABLE:
      break;
    case PACKET_ENABLE:
      break;
    case PACKET_SUPPORT_UNKNOWN:
      {
	char *p;

	p = rs->buf;
	*p++ = 'X';
	p += hexnumstr (p, (ULONGEST) addr);
	*p++ = ',';
	p += hexnumstr (p, (ULONGEST) 0);
	*p++ = ':';
	*p = '\0';

	putpkt_binary (rs->buf, (int) (p - rs->buf));
	getpkt (&rs->buf, &rs->buf_size, 0);

	if (rs->buf[0] == '\0')
	  {
	    if (remote_debug)
	      fprintf_unfiltered (gdb_stdlog,
				  "binary downloading NOT "
				  "supported by target\n");
	    remote_protocol_packets[PACKET_X].support = PACKET_DISABLE;
	  }
	else
	  {
	    if (remote_debug)
	      fprintf_unfiltered (gdb_stdlog,
				  "binary downloading supported by target\n");
	    remote_protocol_packets[PACKET_X].support = PACKET_ENABLE;
	  }
	break;
      }
    }
}

/* Helper function to resize the payload in order to try to get a good
   alignment.  We try to write an amount of data such that the next write will
   start on an address aligned on REMOTE_ALIGN_WRITES.  */

static int
align_for_efficient_write (int todo, CORE_ADDR memaddr)
{
  return ((memaddr + todo) & ~(REMOTE_ALIGN_WRITES - 1)) - memaddr;
}

/* Write memory data directly to the remote machine.
   This does not inform the data cache; the data cache uses this.
   HEADER is the starting part of the packet.
   MEMADDR is the address in the remote memory space.
   MYADDR is the address of the buffer in our space.
   LEN_UNITS is the number of addressable units to write.
   UNIT_SIZE is the length in bytes of an addressable unit.
   PACKET_FORMAT should be either 'X' or 'M', and indicates if we
   should send data as binary ('X'), or hex-encoded ('M').

   The function creates packet of the form
       <HEADER><ADDRESS>,<LENGTH>:<DATA>

   where encoding of <DATA> is terminated by PACKET_FORMAT.

   If USE_LENGTH is 0, then the <LENGTH> field and the preceding comma
   are omitted.

   Return the transferred status, error or OK (an
   'enum target_xfer_status' value).  Save the number of addressable units
   transferred in *XFERED_LEN_UNITS.  Only transfer a single packet.

   On a platform with an addressable memory size of 2 bytes (UNIT_SIZE == 2), an
   exchange between gdb and the stub could look like (?? in place of the
   checksum):

   -> $m1000,4#??
   <- aaaabbbbccccdddd

   -> $M1000,3:eeeeffffeeee#??
   <- OK

   -> $m1000,4#??
   <- eeeeffffeeeedddd  */

static enum target_xfer_status
remote_write_bytes_aux (const char *header, CORE_ADDR memaddr,
			const gdb_byte *myaddr, ULONGEST len_units,
			int unit_size, ULONGEST *xfered_len_units,
			char packet_format, int use_length)
{
  struct remote_state *rs = get_remote_state ();
  char *p;
  char *plen = NULL;
  int plenlen = 0;
  int todo_units;
  int units_written;
  int payload_capacity_bytes;
  int payload_length_bytes;

  if (packet_format != 'X' && packet_format != 'M')
    internal_error (__FILE__, __LINE__,
		    _("remote_write_bytes_aux: bad packet format"));

  if (len_units == 0)
    return TARGET_XFER_EOF;

  payload_capacity_bytes = get_memory_write_packet_size ();

  /* The packet buffer will be large enough for the payload;
     get_memory_packet_size ensures this.  */
  rs->buf[0] = '\0';

  /* Compute the size of the actual payload by subtracting out the
     packet header and footer overhead: "$M<memaddr>,<len>:...#nn".  */

  payload_capacity_bytes -= strlen ("$,:#NN");
  if (!use_length)
    /* The comma won't be used.  */
    payload_capacity_bytes += 1;
  payload_capacity_bytes -= strlen (header);
  payload_capacity_bytes -= hexnumlen (memaddr);

  /* Construct the packet excluding the data: "<header><memaddr>,<len>:".  */

  strcat (rs->buf, header);
  p = rs->buf + strlen (header);

  /* Compute a best guess of the number of bytes actually transfered.  */
  if (packet_format == 'X')
    {
      /* Best guess at number of bytes that will fit.  */
      todo_units = min (len_units, payload_capacity_bytes / unit_size);
      if (use_length)
	payload_capacity_bytes -= hexnumlen (todo_units);
      todo_units = min (todo_units, payload_capacity_bytes / unit_size);
    }
  else
    {
      /* Number of bytes that will fit.  */
      todo_units = min (len_units, (payload_capacity_bytes / unit_size) / 2);
      if (use_length)
	payload_capacity_bytes -= hexnumlen (todo_units);
      todo_units = min (todo_units, (payload_capacity_bytes / unit_size) / 2);
    }

  if (todo_units <= 0)
    internal_error (__FILE__, __LINE__,
		    _("minimum packet size too small to write data"));

  /* If we already need another packet, then try to align the end
     of this packet to a useful boundary.  */
  if (todo_units > 2 * REMOTE_ALIGN_WRITES && todo_units < len_units)
    todo_units = align_for_efficient_write (todo_units, memaddr);

  /* Append "<memaddr>".  */
  memaddr = remote_address_masked (memaddr);
  p += hexnumstr (p, (ULONGEST) memaddr);

  if (use_length)
    {
      /* Append ",".  */
      *p++ = ',';

      /* Append the length and retain its location and size.  It may need to be
         adjusted once the packet body has been created.  */
      plen = p;
      plenlen = hexnumstr (p, (ULONGEST) todo_units);
      p += plenlen;
    }

  /* Append ":".  */
  *p++ = ':';
  *p = '\0';

  /* Append the packet body.  */
  if (packet_format == 'X')
    {
      /* Binary mode.  Send target system values byte by byte, in
	 increasing byte addresses.  Only escape certain critical
	 characters.  */
      payload_length_bytes =
	  remote_escape_output (myaddr, todo_units, unit_size, (gdb_byte *) p,
				&units_written, payload_capacity_bytes);

      /* If not all TODO units fit, then we'll need another packet.  Make
	 a second try to keep the end of the packet aligned.  Don't do
	 this if the packet is tiny.  */
      if (units_written < todo_units && units_written > 2 * REMOTE_ALIGN_WRITES)
	{
	  int new_todo_units;

	  new_todo_units = align_for_efficient_write (units_written, memaddr);

	  if (new_todo_units != units_written)
	    payload_length_bytes =
		remote_escape_output (myaddr, new_todo_units, unit_size,
				      (gdb_byte *) p, &units_written,
				      payload_capacity_bytes);
	}

      p += payload_length_bytes;
      if (use_length && units_written < todo_units)
	{
	  /* Escape chars have filled up the buffer prematurely,
	     and we have actually sent fewer units than planned.
	     Fix-up the length field of the packet.  Use the same
	     number of characters as before.  */
	  plen += hexnumnstr (plen, (ULONGEST) units_written,
			      plenlen);
	  *plen = ':';  /* overwrite \0 from hexnumnstr() */
	}
    }
  else
    {
      /* Normal mode: Send target system values byte by byte, in
	 increasing byte addresses.  Each byte is encoded as a two hex
	 value.  */
      p += 2 * bin2hex (myaddr, p, todo_units * unit_size);
      units_written = todo_units;
    }

  putpkt_binary (rs->buf, (int) (p - rs->buf));
  getpkt (&rs->buf, &rs->buf_size, 0);

  if (rs->buf[0] == 'E')
    return TARGET_XFER_E_IO;

  /* Return UNITS_WRITTEN, not TODO_UNITS, in case escape chars caused us to
     send fewer units than we'd planned.  */
  *xfered_len_units = (ULONGEST) units_written;
  return TARGET_XFER_OK;
}

/* Write memory data directly to the remote machine.
   This does not inform the data cache; the data cache uses this.
   MEMADDR is the address in the remote memory space.
   MYADDR is the address of the buffer in our space.
   LEN is the number of bytes.

   Return the transferred status, error or OK (an
   'enum target_xfer_status' value).  Save the number of bytes
   transferred in *XFERED_LEN.  Only transfer a single packet.  */

static enum target_xfer_status
remote_write_bytes (CORE_ADDR memaddr, const gdb_byte *myaddr, ULONGEST len,
		    int unit_size, ULONGEST *xfered_len)
{
  char *packet_format = 0;

  /* Check whether the target supports binary download.  */
  check_binary_download (memaddr);

  switch (packet_support (PACKET_X))
    {
    case PACKET_ENABLE:
      packet_format = "X";
      break;
    case PACKET_DISABLE:
      packet_format = "M";
      break;
    case PACKET_SUPPORT_UNKNOWN:
      internal_error (__FILE__, __LINE__,
		      _("remote_write_bytes: bad internal state"));
    default:
      internal_error (__FILE__, __LINE__, _("bad switch"));
    }

  return remote_write_bytes_aux (packet_format,
				 memaddr, myaddr, len, unit_size, xfered_len,
				 packet_format[0], 1);
}

/* Read memory data directly from the remote machine.
   This does not use the data cache; the data cache uses this.
   MEMADDR is the address in the remote memory space.
   MYADDR is the address of the buffer in our space.
   LEN_UNITS is the number of addressable memory units to read..
   UNIT_SIZE is the length in bytes of an addressable unit.

   Return the transferred status, error or OK (an
   'enum target_xfer_status' value).  Save the number of bytes
   transferred in *XFERED_LEN_UNITS.

   See the comment of remote_write_bytes_aux for an example of
   memory read/write exchange between gdb and the stub.  */

static enum target_xfer_status
remote_read_bytes_1 (CORE_ADDR memaddr, gdb_byte *myaddr, ULONGEST len_units,
		     int unit_size, ULONGEST *xfered_len_units)
{
  struct remote_state *rs = get_remote_state ();
  int buf_size_bytes;		/* Max size of packet output buffer.  */
  char *p;
  int todo_units;
  int decoded_bytes;

  buf_size_bytes = get_memory_read_packet_size ();
  /* The packet buffer will be large enough for the payload;
     get_memory_packet_size ensures this.  */

  /* Number of units that will fit.  */
  todo_units = min (len_units, (buf_size_bytes / unit_size) / 2);

  /* Construct "m"<memaddr>","<len>".  */
  memaddr = remote_address_masked (memaddr);
  p = rs->buf;
  *p++ = 'm';
  p += hexnumstr (p, (ULONGEST) memaddr);
  *p++ = ',';
  p += hexnumstr (p, (ULONGEST) todo_units);
  *p = '\0';
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);
  if (rs->buf[0] == 'E'
      && isxdigit (rs->buf[1]) && isxdigit (rs->buf[2])
      && rs->buf[3] == '\0')
    return TARGET_XFER_E_IO;
  /* Reply describes memory byte by byte, each byte encoded as two hex
     characters.  */
  p = rs->buf;
  decoded_bytes = hex2bin (p, myaddr, todo_units * unit_size);
  /* Return what we have.  Let higher layers handle partial reads.  */
  *xfered_len_units = (ULONGEST) (decoded_bytes / unit_size);
  return TARGET_XFER_OK;
}

/* Using the set of read-only target sections of remote, read live
   read-only memory.

   For interface/parameters/return description see target.h,
   to_xfer_partial.  */

static enum target_xfer_status
remote_xfer_live_readonly_partial (struct target_ops *ops, gdb_byte *readbuf,
				   ULONGEST memaddr, ULONGEST len,
				   int unit_size, ULONGEST *xfered_len)
{
  struct target_section *secp;
  struct target_section_table *table;

  secp = target_section_by_addr (ops, memaddr);
  if (secp != NULL
      && (bfd_get_section_flags (secp->the_bfd_section->owner,
				 secp->the_bfd_section)
	  & SEC_READONLY))
    {
      struct target_section *p;
      ULONGEST memend = memaddr + len;

      table = target_get_section_table (ops);

      for (p = table->sections; p < table->sections_end; p++)
	{
	  if (memaddr >= p->addr)
	    {
	      if (memend <= p->endaddr)
		{
		  /* Entire transfer is within this section.  */
		  return remote_read_bytes_1 (memaddr, readbuf, len, unit_size,
					      xfered_len);
		}
	      else if (memaddr >= p->endaddr)
		{
		  /* This section ends before the transfer starts.  */
		  continue;
		}
	      else
		{
		  /* This section overlaps the transfer.  Just do half.  */
		  len = p->endaddr - memaddr;
		  return remote_read_bytes_1 (memaddr, readbuf, len, unit_size,
					      xfered_len);
		}
	    }
	}
    }

  return TARGET_XFER_EOF;
}

/* Similar to remote_read_bytes_1, but it reads from the remote stub
   first if the requested memory is unavailable in traceframe.
   Otherwise, fall back to remote_read_bytes_1.  */

static enum target_xfer_status
remote_read_bytes (struct target_ops *ops, CORE_ADDR memaddr,
		   gdb_byte *myaddr, ULONGEST len, int unit_size,
		   ULONGEST *xfered_len)
{
  if (len == 0)
    return TARGET_XFER_EOF;

  if (get_traceframe_number () != -1)
    {
      VEC(mem_range_s) *available;

      /* If we fail to get the set of available memory, then the
	 target does not support querying traceframe info, and so we
	 attempt reading from the traceframe anyway (assuming the
	 target implements the old QTro packet then).  */
      if (traceframe_available_memory (&available, memaddr, len))
	{
	  struct cleanup *old_chain;

	  old_chain = make_cleanup (VEC_cleanup(mem_range_s), &available);

	  if (VEC_empty (mem_range_s, available)
	      || VEC_index (mem_range_s, available, 0)->start != memaddr)
	    {
	      enum target_xfer_status res;

	      /* Don't read into the traceframe's available
		 memory.  */
	      if (!VEC_empty (mem_range_s, available))
		{
		  LONGEST oldlen = len;

		  len = VEC_index (mem_range_s, available, 0)->start - memaddr;
		  gdb_assert (len <= oldlen);
		}

	      do_cleanups (old_chain);

	      /* This goes through the topmost target again.  */
	      res = remote_xfer_live_readonly_partial (ops, myaddr, memaddr,
						       len, unit_size, xfered_len);
	      if (res == TARGET_XFER_OK)
		return TARGET_XFER_OK;
	      else
		{
		  /* No use trying further, we know some memory starting
		     at MEMADDR isn't available.  */
		  *xfered_len = len;
		  return TARGET_XFER_UNAVAILABLE;
		}
	    }

	  /* Don't try to read more than how much is available, in
	     case the target implements the deprecated QTro packet to
	     cater for older GDBs (the target's knowledge of read-only
	     sections may be outdated by now).  */
	  len = VEC_index (mem_range_s, available, 0)->length;

	  do_cleanups (old_chain);
	}
    }

  return remote_read_bytes_1 (memaddr, myaddr, len, unit_size, xfered_len);
}



/* Sends a packet with content determined by the printf format string
   FORMAT and the remaining arguments, then gets the reply.  Returns
   whether the packet was a success, a failure, or unknown.  */

static enum packet_result remote_send_printf (const char *format, ...)
  ATTRIBUTE_PRINTF (1, 2);

static enum packet_result
remote_send_printf (const char *format, ...)
{
  struct remote_state *rs = get_remote_state ();
  int max_size = get_remote_packet_size ();
  va_list ap;

  va_start (ap, format);

  rs->buf[0] = '\0';
  if (vsnprintf (rs->buf, max_size, format, ap) >= max_size)
    internal_error (__FILE__, __LINE__, _("Too long remote packet."));

  if (putpkt (rs->buf) < 0)
    error (_("Communication problem with target."));

  rs->buf[0] = '\0';
  getpkt (&rs->buf, &rs->buf_size, 0);

  return packet_check_result (rs->buf);
}

static void
restore_remote_timeout (void *p)
{
  int value = *(int *)p;

  remote_timeout = value;
}

/* Flash writing can take quite some time.  We'll set
   effectively infinite timeout for flash operations.
   In future, we'll need to decide on a better approach.  */
static const int remote_flash_timeout = 1000;

static void
remote_flash_erase (struct target_ops *ops,
                    ULONGEST address, LONGEST length)
{
  int addr_size = gdbarch_addr_bit (target_gdbarch ()) / 8;
  int saved_remote_timeout = remote_timeout;
  enum packet_result ret;
  struct cleanup *back_to = make_cleanup (restore_remote_timeout,
                                          &saved_remote_timeout);

  remote_timeout = remote_flash_timeout;

  ret = remote_send_printf ("vFlashErase:%s,%s",
			    phex (address, addr_size),
			    phex (length, 4));
  switch (ret)
    {
    case PACKET_UNKNOWN:
      error (_("Remote target does not support flash erase"));
    case PACKET_ERROR:
      error (_("Error erasing flash with vFlashErase packet"));
    default:
      break;
    }

  do_cleanups (back_to);
}

static enum target_xfer_status
remote_flash_write (struct target_ops *ops, ULONGEST address,
		    ULONGEST length, ULONGEST *xfered_len,
		    const gdb_byte *data)
{
  int saved_remote_timeout = remote_timeout;
  enum target_xfer_status ret;
  struct cleanup *back_to = make_cleanup (restore_remote_timeout,
					  &saved_remote_timeout);

  remote_timeout = remote_flash_timeout;
  ret = remote_write_bytes_aux ("vFlashWrite:", address, data, length, 1,
				xfered_len,'X', 0);
  do_cleanups (back_to);

  return ret;
}

static void
remote_flash_done (struct target_ops *ops)
{
  int saved_remote_timeout = remote_timeout;
  int ret;
  struct cleanup *back_to = make_cleanup (restore_remote_timeout,
                                          &saved_remote_timeout);

  remote_timeout = remote_flash_timeout;
  ret = remote_send_printf ("vFlashDone");
  do_cleanups (back_to);

  switch (ret)
    {
    case PACKET_UNKNOWN:
      error (_("Remote target does not support vFlashDone"));
    case PACKET_ERROR:
      error (_("Error finishing flash operation"));
    default:
      break;
    }
}

static void
remote_files_info (struct target_ops *ignore)
{
  puts_filtered ("Debugging a target over a serial line.\n");
}

/* Stuff for dealing with the packets which are part of this protocol.
   See comment at top of file for details.  */

/* Close/unpush the remote target, and throw a TARGET_CLOSE_ERROR
   error to higher layers.  Called when a serial error is detected.
   The exception message is STRING, followed by a colon and a blank,
   the system error message for errno at function entry and final dot
   for output compatibility with throw_perror_with_name.  */

static void
unpush_and_perror (const char *string)
{
  int saved_errno = errno;

  remote_unpush_target ();
  throw_error (TARGET_CLOSE_ERROR, "%s: %s.", string,
	       safe_strerror (saved_errno));
}

/* Read a single character from the remote end.  The current quit
   handler is overridden to avoid quitting in the middle of packet
   sequence, as that would break communication with the remote server.
   See remote_serial_quit_handler for more detail.  */

static int
readchar (int timeout)
{
  int ch;
  struct remote_state *rs = get_remote_state ();
  struct cleanup *old_chain;

  old_chain = make_cleanup_override_quit_handler (remote_serial_quit_handler);

  rs->got_ctrlc_during_io = 0;

  ch = serial_readchar (rs->remote_desc, timeout);

  if (rs->got_ctrlc_during_io)
    set_quit_flag ();

  do_cleanups (old_chain);

  if (ch >= 0)
    return ch;

  switch ((enum serial_rc) ch)
    {
    case SERIAL_EOF:
      remote_unpush_target ();
      throw_error (TARGET_CLOSE_ERROR, _("Remote connection closed"));
      /* no return */
    case SERIAL_ERROR:
      unpush_and_perror (_("Remote communication error.  "
			   "Target disconnected."));
      /* no return */
    case SERIAL_TIMEOUT:
      break;
    }
  return ch;
}

/* Wrapper for serial_write that closes the target and throws if
   writing fails.  The current quit handler is overridden to avoid
   quitting in the middle of packet sequence, as that would break
   communication with the remote server.  See
   remote_serial_quit_handler for more detail.  */

static void
remote_serial_write (const char *str, int len)
{
  struct remote_state *rs = get_remote_state ();
  struct cleanup *old_chain;

  old_chain = make_cleanup_override_quit_handler (remote_serial_quit_handler);

  rs->got_ctrlc_during_io = 0;

  if (serial_write (rs->remote_desc, str, len))
    {
      unpush_and_perror (_("Remote communication error.  "
			   "Target disconnected."));
    }

  if (rs->got_ctrlc_during_io)
    set_quit_flag ();

  do_cleanups (old_chain);
}

/* Send the command in *BUF to the remote machine, and read the reply
   into *BUF.  Report an error if we get an error reply.  Resize
   *BUF using xrealloc if necessary to hold the result, and update
   *SIZEOF_BUF.  */

static void
remote_send (char **buf,
	     long *sizeof_buf)
{
  putpkt (*buf);
  getpkt (buf, sizeof_buf, 0);

  if ((*buf)[0] == 'E')
    error (_("Remote failure reply: %s"), *buf);
}

/* Return a pointer to an xmalloc'ed string representing an escaped
   version of BUF, of len N.  E.g. \n is converted to \\n, \t to \\t,
   etc.  The caller is responsible for releasing the returned
   memory.  */

static char *
escape_buffer (const char *buf, int n)
{
  struct cleanup *old_chain;
  struct ui_file *stb;
  char *str;

  stb = mem_fileopen ();
  old_chain = make_cleanup_ui_file_delete (stb);

  fputstrn_unfiltered (buf, n, '\\', stb);
  str = ui_file_xstrdup (stb, NULL);
  do_cleanups (old_chain);
  return str;
}

/* Display a null-terminated packet on stdout, for debugging, using C
   string notation.  */

static void
print_packet (const char *buf)
{
  puts_filtered ("\"");
  fputstr_filtered (buf, '"', gdb_stdout);
  puts_filtered ("\"");
}

int
putpkt (const char *buf)
{
  return putpkt_binary (buf, strlen (buf));
}

/* Send a packet to the remote machine, with error checking.  The data
   of the packet is in BUF.  The string in BUF can be at most
   get_remote_packet_size () - 5 to account for the $, # and checksum,
   and for a possible /0 if we are debugging (remote_debug) and want
   to print the sent packet as a string.  */

static int
putpkt_binary (const char *buf, int cnt)
{
  struct remote_state *rs = get_remote_state ();
  int i;
  unsigned char csum = 0;
  char *buf2 = (char *) xmalloc (cnt + 6);
  struct cleanup *old_chain = make_cleanup (xfree, buf2);

  int ch;
  int tcount = 0;
  char *p;

  /* Catch cases like trying to read memory or listing threads while
     we're waiting for a stop reply.  The remote server wouldn't be
     ready to handle this request, so we'd hang and timeout.  We don't
     have to worry about this in synchronous mode, because in that
     case it's not possible to issue a command while the target is
     running.  This is not a problem in non-stop mode, because in that
     case, the stub is always ready to process serial input.  */
  if (!target_is_non_stop_p ()
      && target_is_async_p ()
      && rs->waiting_for_stop_reply)
    {
      error (_("Cannot execute this command while the target is running.\n"
	       "Use the \"interrupt\" command to stop the target\n"
	       "and then try again."));
    }

  /* We're sending out a new packet.  Make sure we don't look at a
     stale cached response.  */
  rs->cached_wait_status = 0;

  /* Copy the packet into buffer BUF2, encapsulating it
     and giving it a checksum.  */

  p = buf2;
  *p++ = '$';

  for (i = 0; i < cnt; i++)
    {
      csum += buf[i];
      *p++ = buf[i];
    }
  *p++ = '#';
  *p++ = tohex ((csum >> 4) & 0xf);
  *p++ = tohex (csum & 0xf);

  /* Send it over and over until we get a positive ack.  */

  while (1)
    {
      int started_error_output = 0;

      if (remote_debug)
	{
	  struct cleanup *old_chain;
	  char *str;

	  *p = '\0';
	  str = escape_buffer (buf2, p - buf2);
	  old_chain = make_cleanup (xfree, str);
	  fprintf_unfiltered (gdb_stdlog, "Sending packet: %s...", str);
	  gdb_flush (gdb_stdlog);
	  do_cleanups (old_chain);
	}
      remote_serial_write (buf2, p - buf2);

      /* If this is a no acks version of the remote protocol, send the
	 packet and move on.  */
      if (rs->noack_mode)
        break;

      /* Read until either a timeout occurs (-2) or '+' is read.
	 Handle any notification that arrives in the mean time.  */
      while (1)
	{
	  ch = readchar (remote_timeout);

	  if (remote_debug)
	    {
	      switch (ch)
		{
		case '+':
		case '-':
		case SERIAL_TIMEOUT:
		case '$':
		case '%':
		  if (started_error_output)
		    {
		      putchar_unfiltered ('\n');
		      started_error_output = 0;
		    }
		}
	    }

	  switch (ch)
	    {
	    case '+':
	      if (remote_debug)
		fprintf_unfiltered (gdb_stdlog, "Ack\n");
	      do_cleanups (old_chain);
	      return 1;
	    case '-':
	      if (remote_debug)
		fprintf_unfiltered (gdb_stdlog, "Nak\n");
	      /* FALLTHROUGH */
	    case SERIAL_TIMEOUT:
	      tcount++;
	      if (tcount > 3)
		{
		  do_cleanups (old_chain);
		  return 0;
		}
	      break;		/* Retransmit buffer.  */
	    case '$':
	      {
	        if (remote_debug)
		  fprintf_unfiltered (gdb_stdlog,
				      "Packet instead of Ack, ignoring it\n");
		/* It's probably an old response sent because an ACK
		   was lost.  Gobble up the packet and ack it so it
		   doesn't get retransmitted when we resend this
		   packet.  */
		skip_frame ();
		remote_serial_write ("+", 1);
		continue;	/* Now, go look for +.  */
	      }

	    case '%':
	      {
		int val;

		/* If we got a notification, handle it, and go back to looking
		   for an ack.  */
		/* We've found the start of a notification.  Now
		   collect the data.  */
		val = read_frame (&rs->buf, &rs->buf_size);
		if (val >= 0)
		  {
		    if (remote_debug)
		      {
			struct cleanup *old_chain;
			char *str;

			str = escape_buffer (rs->buf, val);
			old_chain = make_cleanup (xfree, str);
			fprintf_unfiltered (gdb_stdlog,
					    "  Notification received: %s\n",
					    str);
			do_cleanups (old_chain);
		      }
		    handle_notification (rs->notif_state, rs->buf);
		    /* We're in sync now, rewait for the ack.  */
		    tcount = 0;
		  }
		else
		  {
		    if (remote_debug)
		      {
			if (!started_error_output)
			  {
			    started_error_output = 1;
			    fprintf_unfiltered (gdb_stdlog, "putpkt: Junk: ");
			  }
			fputc_unfiltered (ch & 0177, gdb_stdlog);
			fprintf_unfiltered (gdb_stdlog, "%s", rs->buf);
		      }
		  }
		continue;
	      }
	      /* fall-through */
	    default:
	      if (remote_debug)
		{
		  if (!started_error_output)
		    {
		      started_error_output = 1;
		      fprintf_unfiltered (gdb_stdlog, "putpkt: Junk: ");
		    }
		  fputc_unfiltered (ch & 0177, gdb_stdlog);
		}
	      continue;
	    }
	  break;		/* Here to retransmit.  */
	}

#if 0
      /* This is wrong.  If doing a long backtrace, the user should be
         able to get out next time we call QUIT, without anything as
         violent as interrupt_query.  If we want to provide a way out of
         here without getting to the next QUIT, it should be based on
         hitting ^C twice as in remote_wait.  */
      if (quit_flag)
	{
	  quit_flag = 0;
	  interrupt_query ();
	}
#endif
    }

  do_cleanups (old_chain);
  return 0;
}

/* Come here after finding the start of a frame when we expected an
   ack.  Do our best to discard the rest of this packet.  */

static void
skip_frame (void)
{
  int c;

  while (1)
    {
      c = readchar (remote_timeout);
      switch (c)
	{
	case SERIAL_TIMEOUT:
	  /* Nothing we can do.  */
	  return;
	case '#':
	  /* Discard the two bytes of checksum and stop.  */
	  c = readchar (remote_timeout);
	  if (c >= 0)
	    c = readchar (remote_timeout);

	  return;
	case '*':		/* Run length encoding.  */
	  /* Discard the repeat count.  */
	  c = readchar (remote_timeout);
	  if (c < 0)
	    return;
	  break;
	default:
	  /* A regular character.  */
	  break;
	}
    }
}

/* Come here after finding the start of the frame.  Collect the rest
   into *BUF, verifying the checksum, length, and handling run-length
   compression.  NUL terminate the buffer.  If there is not enough room,
   expand *BUF using xrealloc.

   Returns -1 on error, number of characters in buffer (ignoring the
   trailing NULL) on success. (could be extended to return one of the
   SERIAL status indications).  */

static long
read_frame (char **buf_p,
	    long *sizeof_buf)
{
  unsigned char csum;
  long bc;
  int c;
  char *buf = *buf_p;
  struct remote_state *rs = get_remote_state ();

  csum = 0;
  bc = 0;

  while (1)
    {
      c = readchar (remote_timeout);
      switch (c)
	{
	case SERIAL_TIMEOUT:
	  if (remote_debug)
	    fputs_filtered ("Timeout in mid-packet, retrying\n", gdb_stdlog);
	  return -1;
	case '$':
	  if (remote_debug)
	    fputs_filtered ("Saw new packet start in middle of old one\n",
			    gdb_stdlog);
	  return -1;		/* Start a new packet, count retries.  */
	case '#':
	  {
	    unsigned char pktcsum;
	    int check_0 = 0;
	    int check_1 = 0;

	    buf[bc] = '\0';

	    check_0 = readchar (remote_timeout);
	    if (check_0 >= 0)
	      check_1 = readchar (remote_timeout);

	    if (check_0 == SERIAL_TIMEOUT || check_1 == SERIAL_TIMEOUT)
	      {
		if (remote_debug)
		  fputs_filtered ("Timeout in checksum, retrying\n",
				  gdb_stdlog);
		return -1;
	      }
	    else if (check_0 < 0 || check_1 < 0)
	      {
		if (remote_debug)
		  fputs_filtered ("Communication error in checksum\n",
				  gdb_stdlog);
		return -1;
	      }

	    /* Don't recompute the checksum; with no ack packets we
	       don't have any way to indicate a packet retransmission
	       is necessary.  */
	    if (rs->noack_mode)
	      return bc;

	    pktcsum = (fromhex (check_0) << 4) | fromhex (check_1);
	    if (csum == pktcsum)
              return bc;

	    if (remote_debug)
	      {
		struct cleanup *old_chain;
		char *str;

		str = escape_buffer (buf, bc);
		old_chain = make_cleanup (xfree, str);
		fprintf_unfiltered (gdb_stdlog,
				    "Bad checksum, sentsum=0x%x, "
				    "csum=0x%x, buf=%s\n",
				    pktcsum, csum, str);
		do_cleanups (old_chain);
	      }
	    /* Number of characters in buffer ignoring trailing
               NULL.  */
	    return -1;
	  }
	case '*':		/* Run length encoding.  */
          {
	    int repeat;

 	    csum += c;
	    c = readchar (remote_timeout);
	    csum += c;
	    repeat = c - ' ' + 3;	/* Compute repeat count.  */

	    /* The character before ``*'' is repeated.  */

	    if (repeat > 0 && repeat <= 255 && bc > 0)
	      {
		if (bc + repeat - 1 >= *sizeof_buf - 1)
		  {
		    /* Make some more room in the buffer.  */
		    *sizeof_buf += repeat;
		    *buf_p = (char *) xrealloc (*buf_p, *sizeof_buf);
		    buf = *buf_p;
		  }

		memset (&buf[bc], buf[bc - 1], repeat);
		bc += repeat;
		continue;
	      }

	    buf[bc] = '\0';
	    printf_filtered (_("Invalid run length encoding: %s\n"), buf);
	    return -1;
	  }
	default:
	  if (bc >= *sizeof_buf - 1)
	    {
	      /* Make some more room in the buffer.  */
	      *sizeof_buf *= 2;
	      *buf_p = (char *) xrealloc (*buf_p, *sizeof_buf);
	      buf = *buf_p;
	    }

	  buf[bc++] = c;
	  csum += c;
	  continue;
	}
    }
}

/* Read a packet from the remote machine, with error checking, and
   store it in *BUF.  Resize *BUF using xrealloc if necessary to hold
   the result, and update *SIZEOF_BUF.  If FOREVER, wait forever
   rather than timing out; this is used (in synchronous mode) to wait
   for a target that is is executing user code to stop.  */
/* FIXME: ezannoni 2000-02-01 this wrapper is necessary so that we
   don't have to change all the calls to getpkt to deal with the
   return value, because at the moment I don't know what the right
   thing to do it for those.  */
void
getpkt (char **buf,
	long *sizeof_buf,
	int forever)
{
  getpkt_sane (buf, sizeof_buf, forever);
}


/* Read a packet from the remote machine, with error checking, and
   store it in *BUF.  Resize *BUF using xrealloc if necessary to hold
   the result, and update *SIZEOF_BUF.  If FOREVER, wait forever
   rather than timing out; this is used (in synchronous mode) to wait
   for a target that is is executing user code to stop.  If FOREVER ==
   0, this function is allowed to time out gracefully and return an
   indication of this to the caller.  Otherwise return the number of
   bytes read.  If EXPECTING_NOTIF, consider receiving a notification
   enough reason to return to the caller.  *IS_NOTIF is an output
   boolean that indicates whether *BUF holds a notification or not
   (a regular packet).  */

static int
getpkt_or_notif_sane_1 (char **buf, long *sizeof_buf, int forever,
			int expecting_notif, int *is_notif)
{
  struct remote_state *rs = get_remote_state ();
  int c;
  int tries;
  int timeout;
  int val = -1;

  /* We're reading a new response.  Make sure we don't look at a
     previously cached response.  */
  rs->cached_wait_status = 0;

  strcpy (*buf, "timeout");

  if (forever)
    timeout = watchdog > 0 ? watchdog : -1;
  else if (expecting_notif)
    timeout = 0; /* There should already be a char in the buffer.  If
		    not, bail out.  */
  else
    timeout = remote_timeout;

#define MAX_TRIES 3

  /* Process any number of notifications, and then return when
     we get a packet.  */
  for (;;)
    {
      /* If we get a timeout or bad checksum, retry up to MAX_TRIES
	 times.  */
      for (tries = 1; tries <= MAX_TRIES; tries++)
	{
	  /* This can loop forever if the remote side sends us
	     characters continuously, but if it pauses, we'll get
	     SERIAL_TIMEOUT from readchar because of timeout.  Then
	     we'll count that as a retry.

	     Note that even when forever is set, we will only wait
	     forever prior to the start of a packet.  After that, we
	     expect characters to arrive at a brisk pace.  They should
	     show up within remote_timeout intervals.  */
	  do
	    c = readchar (timeout);
	  while (c != SERIAL_TIMEOUT && c != '$' && c != '%');

	  if (c == SERIAL_TIMEOUT)
	    {
	      if (expecting_notif)
		return -1; /* Don't complain, it's normal to not get
			      anything in this case.  */

	      if (forever)	/* Watchdog went off?  Kill the target.  */
		{
		  remote_unpush_target ();
		  throw_error (TARGET_CLOSE_ERROR,
			       _("Watchdog timeout has expired.  "
				 "Target detached."));
		}
	      if (remote_debug)
		fputs_filtered ("Timed out.\n", gdb_stdlog);
	    }
	  else
	    {
	      /* We've found the start of a packet or notification.
		 Now collect the data.  */
	      val = read_frame (buf, sizeof_buf);
	      if (val >= 0)
		break;
	    }

	  remote_serial_write ("-", 1);
	}

      if (tries > MAX_TRIES)
	{
	  /* We have tried hard enough, and just can't receive the
	     packet/notification.  Give up.  */
	  printf_unfiltered (_("Ignoring packet error, continuing...\n"));

	  /* Skip the ack char if we're in no-ack mode.  */
	  if (!rs->noack_mode)
	    remote_serial_write ("+", 1);
	  return -1;
	}

      /* If we got an ordinary packet, return that to our caller.  */
      if (c == '$')
	{
	  if (remote_debug)
	    {
	     struct cleanup *old_chain;
	     char *str;

	     str = escape_buffer (*buf, val);
	     old_chain = make_cleanup (xfree, str);
	     fprintf_unfiltered (gdb_stdlog, "Packet received: %s\n", str);
	     do_cleanups (old_chain);
	    }

	  /* Skip the ack char if we're in no-ack mode.  */
	  if (!rs->noack_mode)
	    remote_serial_write ("+", 1);
	  if (is_notif != NULL)
	    *is_notif = 0;
	  return val;
	}

       /* If we got a notification, handle it, and go back to looking
	 for a packet.  */
      else
	{
	  gdb_assert (c == '%');

	  if (remote_debug)
	    {
	      struct cleanup *old_chain;
	      char *str;

	      str = escape_buffer (*buf, val);
	      old_chain = make_cleanup (xfree, str);
	      fprintf_unfiltered (gdb_stdlog,
				  "  Notification received: %s\n",
				  str);
	      do_cleanups (old_chain);
	    }
	  if (is_notif != NULL)
	    *is_notif = 1;

	  handle_notification (rs->notif_state, *buf);

	  /* Notifications require no acknowledgement.  */

	  if (expecting_notif)
	    return val;
	}
    }
}

static int
getpkt_sane (char **buf, long *sizeof_buf, int forever)
{
  return getpkt_or_notif_sane_1 (buf, sizeof_buf, forever, 0, NULL);
}

static int
getpkt_or_notif_sane (char **buf, long *sizeof_buf, int forever,
		      int *is_notif)
{
  return getpkt_or_notif_sane_1 (buf, sizeof_buf, forever, 1,
				 is_notif);
}

/* Check whether EVENT is a fork event for the process specified
   by the pid passed in DATA, and if it is, kill the fork child.  */

static int
kill_child_of_pending_fork (QUEUE (stop_reply_p) *q,
			    QUEUE_ITER (stop_reply_p) *iter,
			    stop_reply_p event,
			    void *data)
{
  struct queue_iter_param *param = (struct queue_iter_param *) data;
  int parent_pid = *(int *) param->input;

  if (is_pending_fork_parent (&event->ws, parent_pid, event->ptid))
    {
      struct remote_state *rs = get_remote_state ();
      int child_pid = ptid_get_pid (event->ws.value.related_pid);
      int res;

      res = remote_vkill (child_pid, rs);
      if (res != 0)
	error (_("Can't kill fork child process %d"), child_pid);
    }

  return 1;
}

/* Kill any new fork children of process PID that haven't been
   processed by follow_fork.  */

static void
kill_new_fork_children (int pid, struct remote_state *rs)
{
  struct thread_info *thread;
  struct notif_client *notif = &notif_client_stop;
  struct queue_iter_param param;

  /* Kill the fork child threads of any threads in process PID
     that are stopped at a fork event.  */
  ALL_NON_EXITED_THREADS (thread)
    {
      struct target_waitstatus *ws = &thread->pending_follow;

      if (is_pending_fork_parent (ws, pid, thread->ptid))
	{
	  struct remote_state *rs = get_remote_state ();
	  int child_pid = ptid_get_pid (ws->value.related_pid);
	  int res;

	  res = remote_vkill (child_pid, rs);
	  if (res != 0)
	    error (_("Can't kill fork child process %d"), child_pid);
	}
    }

  /* Check for any pending fork events (not reported or processed yet)
     in process PID and kill those fork child threads as well.  */
  remote_notif_get_pending_events (notif);
  param.input = &pid;
  param.output = NULL;
  QUEUE_iterate (stop_reply_p, stop_reply_queue,
		 kill_child_of_pending_fork, &param);
}


/* Target hook to kill the current inferior.  */

static void
remote_kill (struct target_ops *ops)
{
  int res = -1;
  int pid = ptid_get_pid (inferior_ptid);
  struct remote_state *rs = get_remote_state ();

  if (packet_support (PACKET_vKill) != PACKET_DISABLE)
    {
      /* If we're stopped while forking and we haven't followed yet,
	 kill the child task.  We need to do this before killing the
	 parent task because if this is a vfork then the parent will
	 be sleeping.  */
      kill_new_fork_children (pid, rs);

      res = remote_vkill (pid, rs);
      if (res == 0)
	{
	  target_mourn_inferior ();
	  return;
	}
    }

  /* If we are in 'target remote' mode and we are killing the only
     inferior, then we will tell gdbserver to exit and unpush the
     target.  */
  if (res == -1 && !remote_multi_process_p (rs)
      && number_of_live_inferiors () == 1)
    {
      remote_kill_k ();

      /* We've killed the remote end, we get to mourn it.  If we are
	 not in extended mode, mourning the inferior also unpushes
	 remote_ops from the target stack, which closes the remote
	 connection.  */
      target_mourn_inferior ();

      return;
    }

  error (_("Can't kill process"));
}

/* Send a kill request to the target using the 'vKill' packet.  */

static int
remote_vkill (int pid, struct remote_state *rs)
{
  if (packet_support (PACKET_vKill) == PACKET_DISABLE)
    return -1;

  /* Tell the remote target to detach.  */
  xsnprintf (rs->buf, get_remote_packet_size (), "vKill;%x", pid);
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  switch (packet_ok (rs->buf,
		     &remote_protocol_packets[PACKET_vKill]))
    {
    case PACKET_OK:
      return 0;
    case PACKET_ERROR:
      return 1;
    case PACKET_UNKNOWN:
      return -1;
    default:
      internal_error (__FILE__, __LINE__, _("Bad result from packet_ok"));
    }
}

/* Send a kill request to the target using the 'k' packet.  */

static void
remote_kill_k (void)
{
  /* Catch errors so the user can quit from gdb even when we
     aren't on speaking terms with the remote system.  */
  TRY
    {
      putpkt ("k");
    }
  CATCH (ex, RETURN_MASK_ERROR)
    {
      if (ex.error == TARGET_CLOSE_ERROR)
	{
	  /* If we got an (EOF) error that caused the target
	     to go away, then we're done, that's what we wanted.
	     "k" is susceptible to cause a premature EOF, given
	     that the remote server isn't actually required to
	     reply to "k", and it can happen that it doesn't
	     even get to reply ACK to the "k".  */
	  return;
	}

      /* Otherwise, something went wrong.  We didn't actually kill
	 the target.  Just propagate the exception, and let the
	 user or higher layers decide what to do.  */
      throw_exception (ex);
    }
  END_CATCH
}

static void
remote_mourn (struct target_ops *target)
{
  struct remote_state *rs = get_remote_state ();

  /* In 'target remote' mode with one inferior, we close the connection.  */
  if (!rs->extended && number_of_live_inferiors () <= 1)
    {
      unpush_target (target);

      /* remote_close takes care of doing most of the clean up.  */
      generic_mourn_inferior ();
      return;
    }

  /* In case we got here due to an error, but we're going to stay
     connected.  */
  rs->waiting_for_stop_reply = 0;

  /* If the current general thread belonged to the process we just
     detached from or has exited, the remote side current general
     thread becomes undefined.  Considering a case like this:

     - We just got here due to a detach.
     - The process that we're detaching from happens to immediately
       report a global breakpoint being hit in non-stop mode, in the
       same thread we had selected before.
     - GDB attaches to this process again.
     - This event happens to be the next event we handle.

     GDB would consider that the current general thread didn't need to
     be set on the stub side (with Hg), since for all it knew,
     GENERAL_THREAD hadn't changed.

     Notice that although in all-stop mode, the remote server always
     sets the current thread to the thread reporting the stop event,
     that doesn't happen in non-stop mode; in non-stop, the stub *must
     not* change the current thread when reporting a breakpoint hit,
     due to the decoupling of event reporting and event handling.

     To keep things simple, we always invalidate our notion of the
     current thread.  */
  record_currthread (rs, minus_one_ptid);

  /* Call common code to mark the inferior as not running.  */
  generic_mourn_inferior ();

  if (!have_inferiors ())
    {
      if (!remote_multi_process_p (rs))
	{
	  /* Check whether the target is running now - some remote stubs
	     automatically restart after kill.	*/
	  putpkt ("?");
	  getpkt (&rs->buf, &rs->buf_size, 0);

	  if (rs->buf[0] == 'S' || rs->buf[0] == 'T')
	    {
	      /* Assume that the target has been restarted.  Set
		 inferior_ptid so that bits of core GDB realizes
		 there's something here, e.g., so that the user can
		 say "kill" again.  */
	      inferior_ptid = magic_null_ptid;
	    }
	}
    }
}

static int
extended_remote_supports_disable_randomization (struct target_ops *self)
{
  return packet_support (PACKET_QDisableRandomization) == PACKET_ENABLE;
}

static void
extended_remote_disable_randomization (int val)
{
  struct remote_state *rs = get_remote_state ();
  char *reply;

  xsnprintf (rs->buf, get_remote_packet_size (), "QDisableRandomization:%x",
	     val);
  putpkt (rs->buf);
  reply = remote_get_noisy_reply (&target_buf, &target_buf_size);
  if (*reply == '\0')
    error (_("Target does not support QDisableRandomization."));
  if (strcmp (reply, "OK") != 0)
    error (_("Bogus QDisableRandomization reply from target: %s"), reply);
}

static int
extended_remote_run (char *args)
{
  struct remote_state *rs = get_remote_state ();
  int len;
  const char *remote_exec_file = get_remote_exec_file ();

  /* If the user has disabled vRun support, or we have detected that
     support is not available, do not try it.  */
  if (packet_support (PACKET_vRun) == PACKET_DISABLE)
    return -1;

  strcpy (rs->buf, "vRun;");
  len = strlen (rs->buf);

  if (strlen (remote_exec_file) * 2 + len >= get_remote_packet_size ())
    error (_("Remote file name too long for run packet"));
  len += 2 * bin2hex ((gdb_byte *) remote_exec_file, rs->buf + len,
		      strlen (remote_exec_file));

  gdb_assert (args != NULL);
  if (*args)
    {
      struct cleanup *back_to;
      int i;
      char **argv;

      argv = gdb_buildargv (args);
      back_to = make_cleanup_freeargv (argv);
      for (i = 0; argv[i] != NULL; i++)
	{
	  if (strlen (argv[i]) * 2 + 1 + len >= get_remote_packet_size ())
	    error (_("Argument list too long for run packet"));
	  rs->buf[len++] = ';';
	  len += 2 * bin2hex ((gdb_byte *) argv[i], rs->buf + len,
			      strlen (argv[i]));
	}
      do_cleanups (back_to);
    }

  rs->buf[len++] = '\0';

  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  switch (packet_ok (rs->buf, &remote_protocol_packets[PACKET_vRun]))
    {
    case PACKET_OK:
      /* We have a wait response.  All is well.  */
      return 0;
    case PACKET_UNKNOWN:
      return -1;
    case PACKET_ERROR:
      if (remote_exec_file[0] == '\0')
	error (_("Running the default executable on the remote target failed; "
		 "try \"set remote exec-file\"?"));
      else
	error (_("Running \"%s\" on the remote target failed"),
	       remote_exec_file);
    default:
      gdb_assert_not_reached (_("bad switch"));
    }
}

/* In the extended protocol we want to be able to do things like
   "run" and have them basically work as expected.  So we need
   a special create_inferior function.  We support changing the
   executable file and the command line arguments, but not the
   environment.  */

static void
extended_remote_create_inferior (struct target_ops *ops,
				 char *exec_file, char *args,
				 char **env, int from_tty)
{
  int run_worked;
  char *stop_reply;
  struct remote_state *rs = get_remote_state ();
  const char *remote_exec_file = get_remote_exec_file ();

  /* If running asynchronously, register the target file descriptor
     with the event loop.  */
  if (target_can_async_p ())
    target_async (1);

  /* Disable address space randomization if requested (and supported).  */
  if (extended_remote_supports_disable_randomization (ops))
    extended_remote_disable_randomization (disable_randomization);

  /* Now restart the remote server.  */
  run_worked = extended_remote_run (args) != -1;
  if (!run_worked)
    {
      /* vRun was not supported.  Fail if we need it to do what the
	 user requested.  */
      if (remote_exec_file[0])
	error (_("Remote target does not support \"set remote exec-file\""));
      if (args[0])
	error (_("Remote target does not support \"set args\" or run <ARGS>"));

      /* Fall back to "R".  */
      extended_remote_restart ();
    }

  if (!have_inferiors ())
    {
      /* Clean up from the last time we ran, before we mark the target
	 running again.  This will mark breakpoints uninserted, and
	 get_offsets may insert breakpoints.  */
      init_thread_list ();
      init_wait_for_inferior ();
    }

  /* vRun's success return is a stop reply.  */
  stop_reply = run_worked ? rs->buf : NULL;
  add_current_inferior_and_thread (stop_reply);

  /* Get updated offsets, if the stub uses qOffsets.  */
  get_offsets ();
}


/* Given a location's target info BP_TGT and the packet buffer BUF,  output
   the list of conditions (in agent expression bytecode format), if any, the
   target needs to evaluate.  The output is placed into the packet buffer
   started from BUF and ended at BUF_END.  */

static int
remote_add_target_side_condition (struct gdbarch *gdbarch,
				  struct bp_target_info *bp_tgt, char *buf,
				  char *buf_end)
{
  struct agent_expr *aexpr = NULL;
  int i, ix;

  if (VEC_empty (agent_expr_p, bp_tgt->conditions))
    return 0;

  buf += strlen (buf);
  xsnprintf (buf, buf_end - buf, "%s", ";");
  buf++;

  /* Send conditions to the target and free the vector.  */
  for (ix = 0;
       VEC_iterate (agent_expr_p, bp_tgt->conditions, ix, aexpr);
       ix++)
    {
      xsnprintf (buf, buf_end - buf, "X%x,", aexpr->len);
      buf += strlen (buf);
      for (i = 0; i < aexpr->len; ++i)
	buf = pack_hex_byte (buf, aexpr->buf[i]);
      *buf = '\0';
    }
  return 0;
}

static void
remote_add_target_side_commands (struct gdbarch *gdbarch,
				 struct bp_target_info *bp_tgt, char *buf)
{
  struct agent_expr *aexpr = NULL;
  int i, ix;

  if (VEC_empty (agent_expr_p, bp_tgt->tcommands))
    return;

  buf += strlen (buf);

  sprintf (buf, ";cmds:%x,", bp_tgt->persist);
  buf += strlen (buf);

  /* Concatenate all the agent expressions that are commands into the
     cmds parameter.  */
  for (ix = 0;
       VEC_iterate (agent_expr_p, bp_tgt->tcommands, ix, aexpr);
       ix++)
    {
      sprintf (buf, "X%x,", aexpr->len);
      buf += strlen (buf);
      for (i = 0; i < aexpr->len; ++i)
	buf = pack_hex_byte (buf, aexpr->buf[i]);
      *buf = '\0';
    }
}

/* Insert a breakpoint.  On targets that have software breakpoint
   support, we ask the remote target to do the work; on targets
   which don't, we insert a traditional memory breakpoint.  */

static int
remote_insert_breakpoint (struct target_ops *ops,
			  struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt)
{
  /* Try the "Z" s/w breakpoint packet if it is not already disabled.
     If it succeeds, then set the support to PACKET_ENABLE.  If it
     fails, and the user has explicitly requested the Z support then
     report an error, otherwise, mark it disabled and go on.  */

  if (packet_support (PACKET_Z0) != PACKET_DISABLE)
    {
      CORE_ADDR addr = bp_tgt->reqstd_address;
      struct remote_state *rs;
      char *p, *endbuf;
      int bpsize;

      /* Make sure the remote is pointing at the right process, if
	 necessary.  */
      if (!gdbarch_has_global_breakpoints (target_gdbarch ()))
	set_general_process ();

      gdbarch_remote_breakpoint_from_pc (gdbarch, &addr, &bpsize);

      rs = get_remote_state ();
      p = rs->buf;
      endbuf = rs->buf + get_remote_packet_size ();

      *(p++) = 'Z';
      *(p++) = '0';
      *(p++) = ',';
      addr = (ULONGEST) remote_address_masked (addr);
      p += hexnumstr (p, addr);
      xsnprintf (p, endbuf - p, ",%d", bpsize);

      if (remote_supports_cond_breakpoints (ops))
	remote_add_target_side_condition (gdbarch, bp_tgt, p, endbuf);

      if (remote_can_run_breakpoint_commands (ops))
	remote_add_target_side_commands (gdbarch, bp_tgt, p);

      putpkt (rs->buf);
      getpkt (&rs->buf, &rs->buf_size, 0);

      switch (packet_ok (rs->buf, &remote_protocol_packets[PACKET_Z0]))
	{
	case PACKET_ERROR:
	  return -1;
	case PACKET_OK:
	  bp_tgt->placed_address = addr;
	  bp_tgt->placed_size = bpsize;
	  return 0;
	case PACKET_UNKNOWN:
	  break;
	}
    }

  /* If this breakpoint has target-side commands but this stub doesn't
     support Z0 packets, throw error.  */
  if (!VEC_empty (agent_expr_p, bp_tgt->tcommands))
    throw_error (NOT_SUPPORTED_ERROR, _("\
Target doesn't support breakpoints that have target side commands."));

  return memory_insert_breakpoint (ops, gdbarch, bp_tgt);
}

static int
remote_remove_breakpoint (struct target_ops *ops,
			  struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt,
			  enum remove_bp_reason reason)
{
  CORE_ADDR addr = bp_tgt->placed_address;
  struct remote_state *rs = get_remote_state ();

  if (packet_support (PACKET_Z0) != PACKET_DISABLE)
    {
      char *p = rs->buf;
      char *endbuf = rs->buf + get_remote_packet_size ();

      /* Make sure the remote is pointing at the right process, if
	 necessary.  */
      if (!gdbarch_has_global_breakpoints (target_gdbarch ()))
	set_general_process ();

      *(p++) = 'z';
      *(p++) = '0';
      *(p++) = ',';

      addr = (ULONGEST) remote_address_masked (bp_tgt->placed_address);
      p += hexnumstr (p, addr);
      xsnprintf (p, endbuf - p, ",%d", bp_tgt->placed_size);

      putpkt (rs->buf);
      getpkt (&rs->buf, &rs->buf_size, 0);

      return (rs->buf[0] == 'E');
    }

  return memory_remove_breakpoint (ops, gdbarch, bp_tgt, reason);
}

static enum Z_packet_type
watchpoint_to_Z_packet (int type)
{
  switch (type)
    {
    case hw_write:
      return Z_PACKET_WRITE_WP;
      break;
    case hw_read:
      return Z_PACKET_READ_WP;
      break;
    case hw_access:
      return Z_PACKET_ACCESS_WP;
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("hw_bp_to_z: bad watchpoint type %d"), type);
    }
}

static int
remote_insert_watchpoint (struct target_ops *self, CORE_ADDR addr, int len,
			  enum target_hw_bp_type type, struct expression *cond)
{
  struct remote_state *rs = get_remote_state ();
  char *endbuf = rs->buf + get_remote_packet_size ();
  char *p;
  enum Z_packet_type packet = watchpoint_to_Z_packet (type);

  if (packet_support (PACKET_Z0 + packet) == PACKET_DISABLE)
    return 1;

  /* Make sure the remote is pointing at the right process, if
     necessary.  */
  if (!gdbarch_has_global_breakpoints (target_gdbarch ()))
    set_general_process ();

  xsnprintf (rs->buf, endbuf - rs->buf, "Z%x,", packet);
  p = strchr (rs->buf, '\0');
  addr = remote_address_masked (addr);
  p += hexnumstr (p, (ULONGEST) addr);
  xsnprintf (p, endbuf - p, ",%x", len);

  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  switch (packet_ok (rs->buf, &remote_protocol_packets[PACKET_Z0 + packet]))
    {
    case PACKET_ERROR:
      return -1;
    case PACKET_UNKNOWN:
      return 1;
    case PACKET_OK:
      return 0;
    }
  internal_error (__FILE__, __LINE__,
		  _("remote_insert_watchpoint: reached end of function"));
}

static int
remote_watchpoint_addr_within_range (struct target_ops *target, CORE_ADDR addr,
				     CORE_ADDR start, int length)
{
  CORE_ADDR diff = remote_address_masked (addr - start);

  return diff < length;
}


static int
remote_remove_watchpoint (struct target_ops *self, CORE_ADDR addr, int len,
			  enum target_hw_bp_type type, struct expression *cond)
{
  struct remote_state *rs = get_remote_state ();
  char *endbuf = rs->buf + get_remote_packet_size ();
  char *p;
  enum Z_packet_type packet = watchpoint_to_Z_packet (type);

  if (packet_support (PACKET_Z0 + packet) == PACKET_DISABLE)
    return -1;

  /* Make sure the remote is pointing at the right process, if
     necessary.  */
  if (!gdbarch_has_global_breakpoints (target_gdbarch ()))
    set_general_process ();

  xsnprintf (rs->buf, endbuf - rs->buf, "z%x,", packet);
  p = strchr (rs->buf, '\0');
  addr = remote_address_masked (addr);
  p += hexnumstr (p, (ULONGEST) addr);
  xsnprintf (p, endbuf - p, ",%x", len);
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  switch (packet_ok (rs->buf, &remote_protocol_packets[PACKET_Z0 + packet]))
    {
    case PACKET_ERROR:
    case PACKET_UNKNOWN:
      return -1;
    case PACKET_OK:
      return 0;
    }
  internal_error (__FILE__, __LINE__,
		  _("remote_remove_watchpoint: reached end of function"));
}


int remote_hw_watchpoint_limit = -1;
int remote_hw_watchpoint_length_limit = -1;
int remote_hw_breakpoint_limit = -1;

static int
remote_region_ok_for_hw_watchpoint (struct target_ops *self,
				    CORE_ADDR addr, int len)
{
  if (remote_hw_watchpoint_length_limit == 0)
    return 0;
  else if (remote_hw_watchpoint_length_limit < 0)
    return 1;
  else if (len <= remote_hw_watchpoint_length_limit)
    return 1;
  else
    return 0;
}

static int
remote_check_watch_resources (struct target_ops *self,
			      enum bptype type, int cnt, int ot)
{
  if (type == bp_hardware_breakpoint)
    {
      if (remote_hw_breakpoint_limit == 0)
	return 0;
      else if (remote_hw_breakpoint_limit < 0)
	return 1;
      else if (cnt <= remote_hw_breakpoint_limit)
	return 1;
    }
  else
    {
      if (remote_hw_watchpoint_limit == 0)
	return 0;
      else if (remote_hw_watchpoint_limit < 0)
	return 1;
      else if (ot)
	return -1;
      else if (cnt <= remote_hw_watchpoint_limit)
	return 1;
    }
  return -1;
}

/* The to_stopped_by_sw_breakpoint method of target remote.  */

static int
remote_stopped_by_sw_breakpoint (struct target_ops *ops)
{
  struct thread_info *thread = inferior_thread ();

  return (thread->priv != NULL
	  && thread->priv->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT);
}

/* The to_supports_stopped_by_sw_breakpoint method of target
   remote.  */

static int
remote_supports_stopped_by_sw_breakpoint (struct target_ops *ops)
{
  return (packet_support (PACKET_swbreak_feature) == PACKET_ENABLE);
}

/* The to_stopped_by_hw_breakpoint method of target remote.  */

static int
remote_stopped_by_hw_breakpoint (struct target_ops *ops)
{
  struct thread_info *thread = inferior_thread ();

  return (thread->priv != NULL
	  && thread->priv->stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT);
}

/* The to_supports_stopped_by_hw_breakpoint method of target
   remote.  */

static int
remote_supports_stopped_by_hw_breakpoint (struct target_ops *ops)
{
  return (packet_support (PACKET_hwbreak_feature) == PACKET_ENABLE);
}

static int
remote_stopped_by_watchpoint (struct target_ops *ops)
{
  struct thread_info *thread = inferior_thread ();

  return (thread->priv != NULL
	  && thread->priv->stop_reason == TARGET_STOPPED_BY_WATCHPOINT);
}

static int
remote_stopped_data_address (struct target_ops *target, CORE_ADDR *addr_p)
{
  struct thread_info *thread = inferior_thread ();

  if (thread->priv != NULL
      && thread->priv->stop_reason == TARGET_STOPPED_BY_WATCHPOINT)
    {
      *addr_p = thread->priv->watch_data_address;
      return 1;
    }

  return 0;
}


static int
remote_insert_hw_breakpoint (struct target_ops *self, struct gdbarch *gdbarch,
			     struct bp_target_info *bp_tgt)
{
  CORE_ADDR addr = bp_tgt->reqstd_address;
  struct remote_state *rs;
  char *p, *endbuf;
  char *message;
  int bpsize;

  /* The length field should be set to the size of a breakpoint
     instruction, even though we aren't inserting one ourselves.  */

  gdbarch_remote_breakpoint_from_pc (gdbarch, &addr, &bpsize);

  if (packet_support (PACKET_Z1) == PACKET_DISABLE)
    return -1;

  /* Make sure the remote is pointing at the right process, if
     necessary.  */
  if (!gdbarch_has_global_breakpoints (target_gdbarch ()))
    set_general_process ();

  rs = get_remote_state ();
  p = rs->buf;
  endbuf = rs->buf + get_remote_packet_size ();

  *(p++) = 'Z';
  *(p++) = '1';
  *(p++) = ',';

  addr = remote_address_masked (addr);
  p += hexnumstr (p, (ULONGEST) addr);
  xsnprintf (p, endbuf - p, ",%x", bpsize);

  if (remote_supports_cond_breakpoints (self))
    remote_add_target_side_condition (gdbarch, bp_tgt, p, endbuf);

  if (remote_can_run_breakpoint_commands (self))
    remote_add_target_side_commands (gdbarch, bp_tgt, p);

  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  switch (packet_ok (rs->buf, &remote_protocol_packets[PACKET_Z1]))
    {
    case PACKET_ERROR:
      if (rs->buf[1] == '.')
        {
          message = strchr (rs->buf + 2, '.');
          if (message)
            error (_("Remote failure reply: %s"), message + 1);
        }
      return -1;
    case PACKET_UNKNOWN:
      return -1;
    case PACKET_OK:
      bp_tgt->placed_address = addr;
      bp_tgt->placed_size = bpsize;
      return 0;
    }
  internal_error (__FILE__, __LINE__,
		  _("remote_insert_hw_breakpoint: reached end of function"));
}


static int
remote_remove_hw_breakpoint (struct target_ops *self, struct gdbarch *gdbarch,
			     struct bp_target_info *bp_tgt)
{
  CORE_ADDR addr;
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf;
  char *endbuf = rs->buf + get_remote_packet_size ();

  if (packet_support (PACKET_Z1) == PACKET_DISABLE)
    return -1;

  /* Make sure the remote is pointing at the right process, if
     necessary.  */
  if (!gdbarch_has_global_breakpoints (target_gdbarch ()))
    set_general_process ();

  *(p++) = 'z';
  *(p++) = '1';
  *(p++) = ',';

  addr = remote_address_masked (bp_tgt->placed_address);
  p += hexnumstr (p, (ULONGEST) addr);
  xsnprintf (p, endbuf  - p, ",%x", bp_tgt->placed_size);

  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  switch (packet_ok (rs->buf, &remote_protocol_packets[PACKET_Z1]))
    {
    case PACKET_ERROR:
    case PACKET_UNKNOWN:
      return -1;
    case PACKET_OK:
      return 0;
    }
  internal_error (__FILE__, __LINE__,
		  _("remote_remove_hw_breakpoint: reached end of function"));
}

/* Verify memory using the "qCRC:" request.  */

static int
remote_verify_memory (struct target_ops *ops,
		      const gdb_byte *data, CORE_ADDR lma, ULONGEST size)
{
  struct remote_state *rs = get_remote_state ();
  unsigned long host_crc, target_crc;
  char *tmp;

  /* It doesn't make sense to use qCRC if the remote target is
     connected but not running.  */
  if (target_has_execution && packet_support (PACKET_qCRC) != PACKET_DISABLE)
    {
      enum packet_result result;

      /* Make sure the remote is pointing at the right process.  */
      set_general_process ();

      /* FIXME: assumes lma can fit into long.  */
      xsnprintf (rs->buf, get_remote_packet_size (), "qCRC:%lx,%lx",
		 (long) lma, (long) size);
      putpkt (rs->buf);

      /* Be clever; compute the host_crc before waiting for target
	 reply.  */
      host_crc = xcrc32 (data, size, 0xffffffff);

      getpkt (&rs->buf, &rs->buf_size, 0);

      result = packet_ok (rs->buf,
			  &remote_protocol_packets[PACKET_qCRC]);
      if (result == PACKET_ERROR)
	return -1;
      else if (result == PACKET_OK)
	{
	  for (target_crc = 0, tmp = &rs->buf[1]; *tmp; tmp++)
	    target_crc = target_crc * 16 + fromhex (*tmp);

	  return (host_crc == target_crc);
	}
    }

  return simple_verify_memory (ops, data, lma, size);
}

/* compare-sections command

   With no arguments, compares each loadable section in the exec bfd
   with the same memory range on the target, and reports mismatches.
   Useful for verifying the image on the target against the exec file.  */

static void
compare_sections_command (char *args, int from_tty)
{
  asection *s;
  struct cleanup *old_chain;
  gdb_byte *sectdata;
  const char *sectname;
  bfd_size_type size;
  bfd_vma lma;
  int matched = 0;
  int mismatched = 0;
  int res;
  int read_only = 0;

  if (!exec_bfd)
    error (_("command cannot be used without an exec file"));

  /* Make sure the remote is pointing at the right process.  */
  set_general_process ();

  if (args != NULL && strcmp (args, "-r") == 0)
    {
      read_only = 1;
      args = NULL;
    }

  for (s = exec_bfd->sections; s; s = s->next)
    {
      if (!(s->flags & SEC_LOAD))
	continue;		/* Skip non-loadable section.  */

      if (read_only && (s->flags & SEC_READONLY) == 0)
	continue;		/* Skip writeable sections */

      size = bfd_get_section_size (s);
      if (size == 0)
	continue;		/* Skip zero-length section.  */

      sectname = bfd_get_section_name (exec_bfd, s);
      if (args && strcmp (args, sectname) != 0)
	continue;		/* Not the section selected by user.  */

      matched = 1;		/* Do this section.  */
      lma = s->lma;

      sectdata = (gdb_byte *) xmalloc (size);
      old_chain = make_cleanup (xfree, sectdata);
      bfd_get_section_contents (exec_bfd, s, sectdata, 0, size);

      res = target_verify_memory (sectdata, lma, size);

      if (res == -1)
	error (_("target memory fault, section %s, range %s -- %s"), sectname,
	       paddress (target_gdbarch (), lma),
	       paddress (target_gdbarch (), lma + size));

      printf_filtered ("Section %s, range %s -- %s: ", sectname,
		       paddress (target_gdbarch (), lma),
		       paddress (target_gdbarch (), lma + size));
      if (res)
	printf_filtered ("matched.\n");
      else
	{
	  printf_filtered ("MIS-MATCHED!\n");
	  mismatched++;
	}

      do_cleanups (old_chain);
    }
  if (mismatched > 0)
    warning (_("One or more sections of the target image does not match\n\
the loaded file\n"));
  if (args && !matched)
    printf_filtered (_("No loaded section named '%s'.\n"), args);
}

/* Write LEN bytes from WRITEBUF into OBJECT_NAME/ANNEX at OFFSET
   into remote target.  The number of bytes written to the remote
   target is returned, or -1 for error.  */

static enum target_xfer_status
remote_write_qxfer (struct target_ops *ops, const char *object_name,
                    const char *annex, const gdb_byte *writebuf, 
                    ULONGEST offset, LONGEST len, ULONGEST *xfered_len,
                    struct packet_config *packet)
{
  int i, buf_len;
  ULONGEST n;
  struct remote_state *rs = get_remote_state ();
  int max_size = get_memory_write_packet_size (); 

  if (packet->support == PACKET_DISABLE)
    return TARGET_XFER_E_IO;

  /* Insert header.  */
  i = snprintf (rs->buf, max_size, 
		"qXfer:%s:write:%s:%s:",
		object_name, annex ? annex : "",
		phex_nz (offset, sizeof offset));
  max_size -= (i + 1);

  /* Escape as much data as fits into rs->buf.  */
  buf_len = remote_escape_output 
    (writebuf, len, 1, (gdb_byte *) rs->buf + i, &max_size, max_size);

  if (putpkt_binary (rs->buf, i + buf_len) < 0
      || getpkt_sane (&rs->buf, &rs->buf_size, 0) < 0
      || packet_ok (rs->buf, packet) != PACKET_OK)
    return TARGET_XFER_E_IO;

  unpack_varlen_hex (rs->buf, &n);

  *xfered_len = n;
  return TARGET_XFER_OK;
}

/* Read OBJECT_NAME/ANNEX from the remote target using a qXfer packet.
   Data at OFFSET, of up to LEN bytes, is read into READBUF; the
   number of bytes read is returned, or 0 for EOF, or -1 for error.
   The number of bytes read may be less than LEN without indicating an
   EOF.  PACKET is checked and updated to indicate whether the remote
   target supports this object.  */

static enum target_xfer_status
remote_read_qxfer (struct target_ops *ops, const char *object_name,
		   const char *annex,
		   gdb_byte *readbuf, ULONGEST offset, LONGEST len,
		   ULONGEST *xfered_len,
		   struct packet_config *packet)
{
  struct remote_state *rs = get_remote_state ();
  LONGEST i, n, packet_len;

  if (packet->support == PACKET_DISABLE)
    return TARGET_XFER_E_IO;

  /* Check whether we've cached an end-of-object packet that matches
     this request.  */
  if (rs->finished_object)
    {
      if (strcmp (object_name, rs->finished_object) == 0
	  && strcmp (annex ? annex : "", rs->finished_annex) == 0
	  && offset == rs->finished_offset)
	return TARGET_XFER_EOF;


      /* Otherwise, we're now reading something different.  Discard
	 the cache.  */
      xfree (rs->finished_object);
      xfree (rs->finished_annex);
      rs->finished_object = NULL;
      rs->finished_annex = NULL;
    }

  /* Request only enough to fit in a single packet.  The actual data
     may not, since we don't know how much of it will need to be escaped;
     the target is free to respond with slightly less data.  We subtract
     five to account for the response type and the protocol frame.  */
  n = min (get_remote_packet_size () - 5, len);
  snprintf (rs->buf, get_remote_packet_size () - 4, "qXfer:%s:read:%s:%s,%s",
	    object_name, annex ? annex : "",
	    phex_nz (offset, sizeof offset),
	    phex_nz (n, sizeof n));
  i = putpkt (rs->buf);
  if (i < 0)
    return TARGET_XFER_E_IO;

  rs->buf[0] = '\0';
  packet_len = getpkt_sane (&rs->buf, &rs->buf_size, 0);
  if (packet_len < 0 || packet_ok (rs->buf, packet) != PACKET_OK)
    return TARGET_XFER_E_IO;

  if (rs->buf[0] != 'l' && rs->buf[0] != 'm')
    error (_("Unknown remote qXfer reply: %s"), rs->buf);

  /* 'm' means there is (or at least might be) more data after this
     batch.  That does not make sense unless there's at least one byte
     of data in this reply.  */
  if (rs->buf[0] == 'm' && packet_len == 1)
    error (_("Remote qXfer reply contained no data."));

  /* Got some data.  */
  i = remote_unescape_input ((gdb_byte *) rs->buf + 1,
			     packet_len - 1, readbuf, n);

  /* 'l' is an EOF marker, possibly including a final block of data,
     or possibly empty.  If we have the final block of a non-empty
     object, record this fact to bypass a subsequent partial read.  */
  if (rs->buf[0] == 'l' && offset + i > 0)
    {
      rs->finished_object = xstrdup (object_name);
      rs->finished_annex = xstrdup (annex ? annex : "");
      rs->finished_offset = offset + i;
    }

  if (i == 0)
    return TARGET_XFER_EOF;
  else
    {
      *xfered_len = i;
      return TARGET_XFER_OK;
    }
}

static enum target_xfer_status
remote_xfer_partial (struct target_ops *ops, enum target_object object,
		     const char *annex, gdb_byte *readbuf,
		     const gdb_byte *writebuf, ULONGEST offset, ULONGEST len,
		     ULONGEST *xfered_len)
{
  struct remote_state *rs;
  int i;
  char *p2;
  char query_type;
  int unit_size = gdbarch_addressable_memory_unit_size (target_gdbarch ());

  set_remote_traceframe ();
  set_general_thread (inferior_ptid);

  rs = get_remote_state ();

  /* Handle memory using the standard memory routines.  */
  if (object == TARGET_OBJECT_MEMORY)
    {
      /* If the remote target is connected but not running, we should
	 pass this request down to a lower stratum (e.g. the executable
	 file).  */
      if (!target_has_execution)
	return TARGET_XFER_EOF;

      if (writebuf != NULL)
	return remote_write_bytes (offset, writebuf, len, unit_size,
				   xfered_len);
      else
	return remote_read_bytes (ops, offset, readbuf, len, unit_size,
				  xfered_len);
    }

  /* Handle SPU memory using qxfer packets.  */
  if (object == TARGET_OBJECT_SPU)
    {
      if (readbuf)
	return remote_read_qxfer (ops, "spu", annex, readbuf, offset, len,
				  xfered_len, &remote_protocol_packets
				  [PACKET_qXfer_spu_read]);
      else
	return remote_write_qxfer (ops, "spu", annex, writebuf, offset, len,
				   xfered_len, &remote_protocol_packets
				   [PACKET_qXfer_spu_write]);
    }

  /* Handle extra signal info using qxfer packets.  */
  if (object == TARGET_OBJECT_SIGNAL_INFO)
    {
      if (readbuf)
	return remote_read_qxfer (ops, "siginfo", annex, readbuf, offset, len,
				  xfered_len, &remote_protocol_packets
				  [PACKET_qXfer_siginfo_read]);
      else
	return remote_write_qxfer (ops, "siginfo", annex,
				   writebuf, offset, len, xfered_len,
				   &remote_protocol_packets
				   [PACKET_qXfer_siginfo_write]);
    }

  if (object == TARGET_OBJECT_STATIC_TRACE_DATA)
    {
      if (readbuf)
	return remote_read_qxfer (ops, "statictrace", annex,
				  readbuf, offset, len, xfered_len,
				  &remote_protocol_packets
				  [PACKET_qXfer_statictrace_read]);
      else
	return TARGET_XFER_E_IO;
    }

  /* Only handle flash writes.  */
  if (writebuf != NULL)
    {
      switch (object)
	{
	case TARGET_OBJECT_FLASH:
	  return remote_flash_write (ops, offset, len, xfered_len,
				     writebuf);

	default:
	  return TARGET_XFER_E_IO;
	}
    }

  /* Map pre-existing objects onto letters.  DO NOT do this for new
     objects!!!  Instead specify new query packets.  */
  switch (object)
    {
    case TARGET_OBJECT_AVR:
      query_type = 'R';
      break;

    case TARGET_OBJECT_AUXV:
      gdb_assert (annex == NULL);
      return remote_read_qxfer (ops, "auxv", annex, readbuf, offset, len,
				xfered_len,
				&remote_protocol_packets[PACKET_qXfer_auxv]);

    case TARGET_OBJECT_AVAILABLE_FEATURES:
      return remote_read_qxfer
	(ops, "features", annex, readbuf, offset, len, xfered_len,
	 &remote_protocol_packets[PACKET_qXfer_features]);

    case TARGET_OBJECT_LIBRARIES:
      return remote_read_qxfer
	(ops, "libraries", annex, readbuf, offset, len, xfered_len,
	 &remote_protocol_packets[PACKET_qXfer_libraries]);

    case TARGET_OBJECT_LIBRARIES_SVR4:
      return remote_read_qxfer
	(ops, "libraries-svr4", annex, readbuf, offset, len, xfered_len,
	 &remote_protocol_packets[PACKET_qXfer_libraries_svr4]);

    case TARGET_OBJECT_MEMORY_MAP:
      gdb_assert (annex == NULL);
      return remote_read_qxfer (ops, "memory-map", annex, readbuf, offset, len,
				 xfered_len,
				&remote_protocol_packets[PACKET_qXfer_memory_map]);

    case TARGET_OBJECT_OSDATA:
      /* Should only get here if we're connected.  */
      gdb_assert (rs->remote_desc);
      return remote_read_qxfer
	(ops, "osdata", annex, readbuf, offset, len, xfered_len,
        &remote_protocol_packets[PACKET_qXfer_osdata]);

    case TARGET_OBJECT_THREADS:
      gdb_assert (annex == NULL);
      return remote_read_qxfer (ops, "threads", annex, readbuf, offset, len,
				xfered_len,
				&remote_protocol_packets[PACKET_qXfer_threads]);

    case TARGET_OBJECT_TRACEFRAME_INFO:
      gdb_assert (annex == NULL);
      return remote_read_qxfer
	(ops, "traceframe-info", annex, readbuf, offset, len, xfered_len,
	 &remote_protocol_packets[PACKET_qXfer_traceframe_info]);

    case TARGET_OBJECT_FDPIC:
      return remote_read_qxfer (ops, "fdpic", annex, readbuf, offset, len,
				xfered_len,
				&remote_protocol_packets[PACKET_qXfer_fdpic]);

    case TARGET_OBJECT_OPENVMS_UIB:
      return remote_read_qxfer (ops, "uib", annex, readbuf, offset, len,
				xfered_len,
				&remote_protocol_packets[PACKET_qXfer_uib]);

    case TARGET_OBJECT_BTRACE:
      return remote_read_qxfer (ops, "btrace", annex, readbuf, offset, len,
				xfered_len,
        &remote_protocol_packets[PACKET_qXfer_btrace]);

    case TARGET_OBJECT_BTRACE_CONF:
      return remote_read_qxfer (ops, "btrace-conf", annex, readbuf, offset,
				len, xfered_len,
	&remote_protocol_packets[PACKET_qXfer_btrace_conf]);

    case TARGET_OBJECT_EXEC_FILE:
      return remote_read_qxfer (ops, "exec-file", annex, readbuf, offset,
				len, xfered_len,
	&remote_protocol_packets[PACKET_qXfer_exec_file]);

    default:
      return TARGET_XFER_E_IO;
    }

  /* Minimum outbuf size is get_remote_packet_size ().  If LEN is not
     large enough let the caller deal with it.  */
  if (len < get_remote_packet_size ())
    return TARGET_XFER_E_IO;
  len = get_remote_packet_size ();

  /* Except for querying the minimum buffer size, target must be open.  */
  if (!rs->remote_desc)
    error (_("remote query is only available after target open"));

  gdb_assert (annex != NULL);
  gdb_assert (readbuf != NULL);

  p2 = rs->buf;
  *p2++ = 'q';
  *p2++ = query_type;

  /* We used one buffer char for the remote protocol q command and
     another for the query type.  As the remote protocol encapsulation
     uses 4 chars plus one extra in case we are debugging
     (remote_debug), we have PBUFZIZ - 7 left to pack the query
     string.  */
  i = 0;
  while (annex[i] && (i < (get_remote_packet_size () - 8)))
    {
      /* Bad caller may have sent forbidden characters.  */
      gdb_assert (isprint (annex[i]) && annex[i] != '$' && annex[i] != '#');
      *p2++ = annex[i];
      i++;
    }
  *p2 = '\0';
  gdb_assert (annex[i] == '\0');

  i = putpkt (rs->buf);
  if (i < 0)
    return TARGET_XFER_E_IO;

  getpkt (&rs->buf, &rs->buf_size, 0);
  strcpy ((char *) readbuf, rs->buf);

  *xfered_len = strlen ((char *) readbuf);
  return TARGET_XFER_OK;
}

/* Implementation of to_get_memory_xfer_limit.  */

static ULONGEST
remote_get_memory_xfer_limit (struct target_ops *ops)
{
  return get_memory_write_packet_size ();
}

static int
remote_search_memory (struct target_ops* ops,
		      CORE_ADDR start_addr, ULONGEST search_space_len,
		      const gdb_byte *pattern, ULONGEST pattern_len,
		      CORE_ADDR *found_addrp)
{
  int addr_size = gdbarch_addr_bit (target_gdbarch ()) / 8;
  struct remote_state *rs = get_remote_state ();
  int max_size = get_memory_write_packet_size ();
  struct packet_config *packet =
    &remote_protocol_packets[PACKET_qSearch_memory];
  /* Number of packet bytes used to encode the pattern;
     this could be more than PATTERN_LEN due to escape characters.  */
  int escaped_pattern_len;
  /* Amount of pattern that was encodable in the packet.  */
  int used_pattern_len;
  int i;
  int found;
  ULONGEST found_addr;

  /* Don't go to the target if we don't have to.
     This is done before checking packet->support to avoid the possibility that
     a success for this edge case means the facility works in general.  */
  if (pattern_len > search_space_len)
    return 0;
  if (pattern_len == 0)
    {
      *found_addrp = start_addr;
      return 1;
    }

  /* If we already know the packet isn't supported, fall back to the simple
     way of searching memory.  */

  if (packet_config_support (packet) == PACKET_DISABLE)
    {
      /* Target doesn't provided special support, fall back and use the
	 standard support (copy memory and do the search here).  */
      return simple_search_memory (ops, start_addr, search_space_len,
				   pattern, pattern_len, found_addrp);
    }

  /* Make sure the remote is pointing at the right process.  */
  set_general_process ();

  /* Insert header.  */
  i = snprintf (rs->buf, max_size, 
		"qSearch:memory:%s;%s;",
		phex_nz (start_addr, addr_size),
		phex_nz (search_space_len, sizeof (search_space_len)));
  max_size -= (i + 1);

  /* Escape as much data as fits into rs->buf.  */
  escaped_pattern_len =
    remote_escape_output (pattern, pattern_len, 1, (gdb_byte *) rs->buf + i,
			  &used_pattern_len, max_size);

  /* Bail if the pattern is too large.  */
  if (used_pattern_len != pattern_len)
    error (_("Pattern is too large to transmit to remote target."));

  if (putpkt_binary (rs->buf, i + escaped_pattern_len) < 0
      || getpkt_sane (&rs->buf, &rs->buf_size, 0) < 0
      || packet_ok (rs->buf, packet) != PACKET_OK)
    {
      /* The request may not have worked because the command is not
	 supported.  If so, fall back to the simple way.  */
      if (packet->support == PACKET_DISABLE)
	{
	  return simple_search_memory (ops, start_addr, search_space_len,
				       pattern, pattern_len, found_addrp);
	}
      return -1;
    }

  if (rs->buf[0] == '0')
    found = 0;
  else if (rs->buf[0] == '1')
    {
      found = 1;
      if (rs->buf[1] != ',')
	error (_("Unknown qSearch:memory reply: %s"), rs->buf);
      unpack_varlen_hex (rs->buf + 2, &found_addr);
      *found_addrp = found_addr;
    }
  else
    error (_("Unknown qSearch:memory reply: %s"), rs->buf);

  return found;
}

static void
remote_rcmd (struct target_ops *self, const char *command,
	     struct ui_file *outbuf)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf;

  if (!rs->remote_desc)
    error (_("remote rcmd is only available after target open"));

  /* Send a NULL command across as an empty command.  */
  if (command == NULL)
    command = "";

  /* The query prefix.  */
  strcpy (rs->buf, "qRcmd,");
  p = strchr (rs->buf, '\0');

  if ((strlen (rs->buf) + strlen (command) * 2 + 8/*misc*/)
      > get_remote_packet_size ())
    error (_("\"monitor\" command ``%s'' is too long."), command);

  /* Encode the actual command.  */
  bin2hex ((const gdb_byte *) command, p, strlen (command));

  if (putpkt (rs->buf) < 0)
    error (_("Communication problem with target."));

  /* get/display the response */
  while (1)
    {
      char *buf;

      /* XXX - see also remote_get_noisy_reply().  */
      QUIT;			/* Allow user to bail out with ^C.  */
      rs->buf[0] = '\0';
      if (getpkt_sane (&rs->buf, &rs->buf_size, 0) == -1)
        { 
          /* Timeout.  Continue to (try to) read responses.
             This is better than stopping with an error, assuming the stub
             is still executing the (long) monitor command.
             If needed, the user can interrupt gdb using C-c, obtaining
             an effect similar to stop on timeout.  */
          continue;
        }
      buf = rs->buf;
      if (buf[0] == '\0')
	error (_("Target does not support this command."));
      if (buf[0] == 'O' && buf[1] != 'K')
	{
	  remote_console_output (buf + 1); /* 'O' message from stub.  */
	  continue;
	}
      if (strcmp (buf, "OK") == 0)
	break;
      if (strlen (buf) == 3 && buf[0] == 'E'
	  && isdigit (buf[1]) && isdigit (buf[2]))
	{
	  error (_("Protocol error with Rcmd"));
	}
      for (p = buf; p[0] != '\0' && p[1] != '\0'; p += 2)
	{
	  char c = (fromhex (p[0]) << 4) + fromhex (p[1]);

	  fputc_unfiltered (c, outbuf);
	}
      break;
    }
}

static VEC(mem_region_s) *
remote_memory_map (struct target_ops *ops)
{
  VEC(mem_region_s) *result = NULL;
  char *text = target_read_stralloc (&current_target,
				     TARGET_OBJECT_MEMORY_MAP, NULL);

  if (text)
    {
      struct cleanup *back_to = make_cleanup (xfree, text);

      result = parse_memory_map (text);
      do_cleanups (back_to);
    }

  return result;
}

static void
packet_command (char *args, int from_tty)
{
  struct remote_state *rs = get_remote_state ();

  if (!rs->remote_desc)
    error (_("command can only be used with remote target"));

  if (!args)
    error (_("remote-packet command requires packet text as argument"));

  puts_filtered ("sending: ");
  print_packet (args);
  puts_filtered ("\n");
  putpkt (args);

  getpkt (&rs->buf, &rs->buf_size, 0);
  puts_filtered ("received: ");
  print_packet (rs->buf);
  puts_filtered ("\n");
}

#if 0
/* --------- UNIT_TEST for THREAD oriented PACKETS ------------------- */

static void display_thread_info (struct gdb_ext_thread_info *info);

static void threadset_test_cmd (char *cmd, int tty);

static void threadalive_test (char *cmd, int tty);

static void threadlist_test_cmd (char *cmd, int tty);

int get_and_display_threadinfo (threadref *ref);

static void threadinfo_test_cmd (char *cmd, int tty);

static int thread_display_step (threadref *ref, void *context);

static void threadlist_update_test_cmd (char *cmd, int tty);

static void init_remote_threadtests (void);

#define SAMPLE_THREAD  0x05060708	/* Truncated 64 bit threadid.  */

static void
threadset_test_cmd (char *cmd, int tty)
{
  int sample_thread = SAMPLE_THREAD;

  printf_filtered (_("Remote threadset test\n"));
  set_general_thread (sample_thread);
}


static void
threadalive_test (char *cmd, int tty)
{
  int sample_thread = SAMPLE_THREAD;
  int pid = ptid_get_pid (inferior_ptid);
  ptid_t ptid = ptid_build (pid, sample_thread, 0);

  if (remote_thread_alive (ptid))
    printf_filtered ("PASS: Thread alive test\n");
  else
    printf_filtered ("FAIL: Thread alive test\n");
}

void output_threadid (char *title, threadref *ref);

void
output_threadid (char *title, threadref *ref)
{
  char hexid[20];

  pack_threadid (&hexid[0], ref);	/* Convert threead id into hex.  */
  hexid[16] = 0;
  printf_filtered ("%s  %s\n", title, (&hexid[0]));
}

static void
threadlist_test_cmd (char *cmd, int tty)
{
  int startflag = 1;
  threadref nextthread;
  int done, result_count;
  threadref threadlist[3];

  printf_filtered ("Remote Threadlist test\n");
  if (!remote_get_threadlist (startflag, &nextthread, 3, &done,
			      &result_count, &threadlist[0]))
    printf_filtered ("FAIL: threadlist test\n");
  else
    {
      threadref *scan = threadlist;
      threadref *limit = scan + result_count;

      while (scan < limit)
	output_threadid (" thread ", scan++);
    }
}

void
display_thread_info (struct gdb_ext_thread_info *info)
{
  output_threadid ("Threadid: ", &info->threadid);
  printf_filtered ("Name: %s\n ", info->shortname);
  printf_filtered ("State: %s\n", info->display);
  printf_filtered ("other: %s\n\n", info->more_display);
}

int
get_and_display_threadinfo (threadref *ref)
{
  int result;
  int set;
  struct gdb_ext_thread_info threadinfo;

  set = TAG_THREADID | TAG_EXISTS | TAG_THREADNAME
    | TAG_MOREDISPLAY | TAG_DISPLAY;
  if (0 != (result = remote_get_threadinfo (ref, set, &threadinfo)))
    display_thread_info (&threadinfo);
  return result;
}

static void
threadinfo_test_cmd (char *cmd, int tty)
{
  int athread = SAMPLE_THREAD;
  threadref thread;
  int set;

  int_to_threadref (&thread, athread);
  printf_filtered ("Remote Threadinfo test\n");
  if (!get_and_display_threadinfo (&thread))
    printf_filtered ("FAIL cannot get thread info\n");
}

static int
thread_display_step (threadref *ref, void *context)
{
  /* output_threadid(" threadstep ",ref); *//* simple test */
  return get_and_display_threadinfo (ref);
}

static void
threadlist_update_test_cmd (char *cmd, int tty)
{
  printf_filtered ("Remote Threadlist update test\n");
  remote_threadlist_iterator (thread_display_step, 0, CRAZY_MAX_THREADS);
}

static void
init_remote_threadtests (void)
{
  add_com ("tlist", class_obscure, threadlist_test_cmd,
	   _("Fetch and print the remote list of "
	     "thread identifiers, one pkt only"));
  add_com ("tinfo", class_obscure, threadinfo_test_cmd,
	   _("Fetch and display info about one thread"));
  add_com ("tset", class_obscure, threadset_test_cmd,
	   _("Test setting to a different thread"));
  add_com ("tupd", class_obscure, threadlist_update_test_cmd,
	   _("Iterate through updating all remote thread info"));
  add_com ("talive", class_obscure, threadalive_test,
	   _(" Remote thread alive test "));
}

#endif /* 0 */

/* Convert a thread ID to a string.  Returns the string in a static
   buffer.  */

static char *
remote_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  static char buf[64];
  struct remote_state *rs = get_remote_state ();

  if (ptid_equal (ptid, null_ptid))
    return normal_pid_to_str (ptid);
  else if (ptid_is_pid (ptid))
    {
      /* Printing an inferior target id.  */

      /* When multi-process extensions are off, there's no way in the
	 remote protocol to know the remote process id, if there's any
	 at all.  There's one exception --- when we're connected with
	 target extended-remote, and we manually attached to a process
	 with "attach PID".  We don't record anywhere a flag that
	 allows us to distinguish that case from the case of
	 connecting with extended-remote and the stub already being
	 attached to a process, and reporting yes to qAttached, hence
	 no smart special casing here.  */
      if (!remote_multi_process_p (rs))
	{
	  xsnprintf (buf, sizeof buf, "Remote target");
	  return buf;
	}

      return normal_pid_to_str (ptid);
    }
  else
    {
      if (ptid_equal (magic_null_ptid, ptid))
	xsnprintf (buf, sizeof buf, "Thread <main>");
      else if (remote_multi_process_p (rs))
	if (ptid_get_lwp (ptid) == 0)
	  return normal_pid_to_str (ptid);
	else
	  xsnprintf (buf, sizeof buf, "Thread %d.%ld",
		     ptid_get_pid (ptid), ptid_get_lwp (ptid));
      else
	xsnprintf (buf, sizeof buf, "Thread %ld",
		   ptid_get_lwp (ptid));
      return buf;
    }
}

/* Get the address of the thread local variable in OBJFILE which is
   stored at OFFSET within the thread local storage for thread PTID.  */

static CORE_ADDR
remote_get_thread_local_address (struct target_ops *ops,
				 ptid_t ptid, CORE_ADDR lm, CORE_ADDR offset)
{
  if (packet_support (PACKET_qGetTLSAddr) != PACKET_DISABLE)
    {
      struct remote_state *rs = get_remote_state ();
      char *p = rs->buf;
      char *endp = rs->buf + get_remote_packet_size ();
      enum packet_result result;

      strcpy (p, "qGetTLSAddr:");
      p += strlen (p);
      p = write_ptid (p, endp, ptid);
      *p++ = ',';
      p += hexnumstr (p, offset);
      *p++ = ',';
      p += hexnumstr (p, lm);
      *p++ = '\0';

      putpkt (rs->buf);
      getpkt (&rs->buf, &rs->buf_size, 0);
      result = packet_ok (rs->buf,
			  &remote_protocol_packets[PACKET_qGetTLSAddr]);
      if (result == PACKET_OK)
	{
	  ULONGEST result;

	  unpack_varlen_hex (rs->buf, &result);
	  return result;
	}
      else if (result == PACKET_UNKNOWN)
	throw_error (TLS_GENERIC_ERROR,
		     _("Remote target doesn't support qGetTLSAddr packet"));
      else
	throw_error (TLS_GENERIC_ERROR,
		     _("Remote target failed to process qGetTLSAddr request"));
    }
  else
    throw_error (TLS_GENERIC_ERROR,
		 _("TLS not supported or disabled on this target"));
  /* Not reached.  */
  return 0;
}

/* Provide thread local base, i.e. Thread Information Block address.
   Returns 1 if ptid is found and thread_local_base is non zero.  */

static int
remote_get_tib_address (struct target_ops *self, ptid_t ptid, CORE_ADDR *addr)
{
  if (packet_support (PACKET_qGetTIBAddr) != PACKET_DISABLE)
    {
      struct remote_state *rs = get_remote_state ();
      char *p = rs->buf;
      char *endp = rs->buf + get_remote_packet_size ();
      enum packet_result result;

      strcpy (p, "qGetTIBAddr:");
      p += strlen (p);
      p = write_ptid (p, endp, ptid);
      *p++ = '\0';

      putpkt (rs->buf);
      getpkt (&rs->buf, &rs->buf_size, 0);
      result = packet_ok (rs->buf,
			  &remote_protocol_packets[PACKET_qGetTIBAddr]);
      if (result == PACKET_OK)
	{
	  ULONGEST result;

	  unpack_varlen_hex (rs->buf, &result);
	  if (addr)
	    *addr = (CORE_ADDR) result;
	  return 1;
	}
      else if (result == PACKET_UNKNOWN)
	error (_("Remote target doesn't support qGetTIBAddr packet"));
      else
	error (_("Remote target failed to process qGetTIBAddr request"));
    }
  else
    error (_("qGetTIBAddr not supported or disabled on this target"));
  /* Not reached.  */
  return 0;
}

/* Support for inferring a target description based on the current
   architecture and the size of a 'g' packet.  While the 'g' packet
   can have any size (since optional registers can be left off the
   end), some sizes are easily recognizable given knowledge of the
   approximate architecture.  */

struct remote_g_packet_guess
{
  int bytes;
  const struct target_desc *tdesc;
};
typedef struct remote_g_packet_guess remote_g_packet_guess_s;
DEF_VEC_O(remote_g_packet_guess_s);

struct remote_g_packet_data
{
  VEC(remote_g_packet_guess_s) *guesses;
};

static struct gdbarch_data *remote_g_packet_data_handle;

static void *
remote_g_packet_data_init (struct obstack *obstack)
{
  return OBSTACK_ZALLOC (obstack, struct remote_g_packet_data);
}

void
register_remote_g_packet_guess (struct gdbarch *gdbarch, int bytes,
				const struct target_desc *tdesc)
{
  struct remote_g_packet_data *data
    = ((struct remote_g_packet_data *)
       gdbarch_data (gdbarch, remote_g_packet_data_handle));
  struct remote_g_packet_guess new_guess, *guess;
  int ix;

  gdb_assert (tdesc != NULL);

  for (ix = 0;
       VEC_iterate (remote_g_packet_guess_s, data->guesses, ix, guess);
       ix++)
    if (guess->bytes == bytes)
      internal_error (__FILE__, __LINE__,
		      _("Duplicate g packet description added for size %d"),
		      bytes);

  new_guess.bytes = bytes;
  new_guess.tdesc = tdesc;
  VEC_safe_push (remote_g_packet_guess_s, data->guesses, &new_guess);
}

/* Return 1 if remote_read_description would do anything on this target
   and architecture, 0 otherwise.  */

static int
remote_read_description_p (struct target_ops *target)
{
  struct remote_g_packet_data *data
    = ((struct remote_g_packet_data *)
       gdbarch_data (target_gdbarch (), remote_g_packet_data_handle));

  if (!VEC_empty (remote_g_packet_guess_s, data->guesses))
    return 1;

  return 0;
}

static const struct target_desc *
remote_read_description (struct target_ops *target)
{
  struct remote_g_packet_data *data
    = ((struct remote_g_packet_data *)
       gdbarch_data (target_gdbarch (), remote_g_packet_data_handle));

  /* Do not try this during initial connection, when we do not know
     whether there is a running but stopped thread.  */
  if (!target_has_execution || ptid_equal (inferior_ptid, null_ptid))
    return target->beneath->to_read_description (target->beneath);

  if (!VEC_empty (remote_g_packet_guess_s, data->guesses))
    {
      struct remote_g_packet_guess *guess;
      int ix;
      int bytes = send_g_packet ();

      for (ix = 0;
	   VEC_iterate (remote_g_packet_guess_s, data->guesses, ix, guess);
	   ix++)
	if (guess->bytes == bytes)
	  return guess->tdesc;

      /* We discard the g packet.  A minor optimization would be to
	 hold on to it, and fill the register cache once we have selected
	 an architecture, but it's too tricky to do safely.  */
    }

  return target->beneath->to_read_description (target->beneath);
}

/* Remote file transfer support.  This is host-initiated I/O, not
   target-initiated; for target-initiated, see remote-fileio.c.  */

/* If *LEFT is at least the length of STRING, copy STRING to
   *BUFFER, update *BUFFER to point to the new end of the buffer, and
   decrease *LEFT.  Otherwise raise an error.  */

static void
remote_buffer_add_string (char **buffer, int *left, char *string)
{
  int len = strlen (string);

  if (len > *left)
    error (_("Packet too long for target."));

  memcpy (*buffer, string, len);
  *buffer += len;
  *left -= len;

  /* NUL-terminate the buffer as a convenience, if there is
     room.  */
  if (*left)
    **buffer = '\0';
}

/* If *LEFT is large enough, hex encode LEN bytes from BYTES into
   *BUFFER, update *BUFFER to point to the new end of the buffer, and
   decrease *LEFT.  Otherwise raise an error.  */

static void
remote_buffer_add_bytes (char **buffer, int *left, const gdb_byte *bytes,
			 int len)
{
  if (2 * len > *left)
    error (_("Packet too long for target."));

  bin2hex (bytes, *buffer, len);
  *buffer += 2 * len;
  *left -= 2 * len;

  /* NUL-terminate the buffer as a convenience, if there is
     room.  */
  if (*left)
    **buffer = '\0';
}

/* If *LEFT is large enough, convert VALUE to hex and add it to
   *BUFFER, update *BUFFER to point to the new end of the buffer, and
   decrease *LEFT.  Otherwise raise an error.  */

static void
remote_buffer_add_int (char **buffer, int *left, ULONGEST value)
{
  int len = hexnumlen (value);

  if (len > *left)
    error (_("Packet too long for target."));

  hexnumstr (*buffer, value);
  *buffer += len;
  *left -= len;

  /* NUL-terminate the buffer as a convenience, if there is
     room.  */
  if (*left)
    **buffer = '\0';
}

/* Parse an I/O result packet from BUFFER.  Set RETCODE to the return
   value, *REMOTE_ERRNO to the remote error number or zero if none
   was included, and *ATTACHMENT to point to the start of the annex
   if any.  The length of the packet isn't needed here; there may
   be NUL bytes in BUFFER, but they will be after *ATTACHMENT.

   Return 0 if the packet could be parsed, -1 if it could not.  If
   -1 is returned, the other variables may not be initialized.  */

static int
remote_hostio_parse_result (char *buffer, int *retcode,
			    int *remote_errno, char **attachment)
{
  char *p, *p2;

  *remote_errno = 0;
  *attachment = NULL;

  if (buffer[0] != 'F')
    return -1;

  errno = 0;
  *retcode = strtol (&buffer[1], &p, 16);
  if (errno != 0 || p == &buffer[1])
    return -1;

  /* Check for ",errno".  */
  if (*p == ',')
    {
      errno = 0;
      *remote_errno = strtol (p + 1, &p2, 16);
      if (errno != 0 || p + 1 == p2)
	return -1;
      p = p2;
    }

  /* Check for ";attachment".  If there is no attachment, the
     packet should end here.  */
  if (*p == ';')
    {
      *attachment = p + 1;
      return 0;
    }
  else if (*p == '\0')
    return 0;
  else
    return -1;
}

/* Send a prepared I/O packet to the target and read its response.
   The prepared packet is in the global RS->BUF before this function
   is called, and the answer is there when we return.

   COMMAND_BYTES is the length of the request to send, which may include
   binary data.  WHICH_PACKET is the packet configuration to check
   before attempting a packet.  If an error occurs, *REMOTE_ERRNO
   is set to the error number and -1 is returned.  Otherwise the value
   returned by the function is returned.

   ATTACHMENT and ATTACHMENT_LEN should be non-NULL if and only if an
   attachment is expected; an error will be reported if there's a
   mismatch.  If one is found, *ATTACHMENT will be set to point into
   the packet buffer and *ATTACHMENT_LEN will be set to the
   attachment's length.  */

static int
remote_hostio_send_command (int command_bytes, int which_packet,
			    int *remote_errno, char **attachment,
			    int *attachment_len)
{
  struct remote_state *rs = get_remote_state ();
  int ret, bytes_read;
  char *attachment_tmp;

  if (!rs->remote_desc
      || packet_support (which_packet) == PACKET_DISABLE)
    {
      *remote_errno = FILEIO_ENOSYS;
      return -1;
    }

  putpkt_binary (rs->buf, command_bytes);
  bytes_read = getpkt_sane (&rs->buf, &rs->buf_size, 0);

  /* If it timed out, something is wrong.  Don't try to parse the
     buffer.  */
  if (bytes_read < 0)
    {
      *remote_errno = FILEIO_EINVAL;
      return -1;
    }

  switch (packet_ok (rs->buf, &remote_protocol_packets[which_packet]))
    {
    case PACKET_ERROR:
      *remote_errno = FILEIO_EINVAL;
      return -1;
    case PACKET_UNKNOWN:
      *remote_errno = FILEIO_ENOSYS;
      return -1;
    case PACKET_OK:
      break;
    }

  if (remote_hostio_parse_result (rs->buf, &ret, remote_errno,
				  &attachment_tmp))
    {
      *remote_errno = FILEIO_EINVAL;
      return -1;
    }

  /* Make sure we saw an attachment if and only if we expected one.  */
  if ((attachment_tmp == NULL && attachment != NULL)
      || (attachment_tmp != NULL && attachment == NULL))
    {
      *remote_errno = FILEIO_EINVAL;
      return -1;
    }

  /* If an attachment was found, it must point into the packet buffer;
     work out how many bytes there were.  */
  if (attachment_tmp != NULL)
    {
      *attachment = attachment_tmp;
      *attachment_len = bytes_read - (*attachment - rs->buf);
    }

  return ret;
}

/* Invalidate the readahead cache.  */

static void
readahead_cache_invalidate (void)
{
  struct remote_state *rs = get_remote_state ();

  rs->readahead_cache.fd = -1;
}

/* Invalidate the readahead cache if it is holding data for FD.  */

static void
readahead_cache_invalidate_fd (int fd)
{
  struct remote_state *rs = get_remote_state ();

  if (rs->readahead_cache.fd == fd)
    rs->readahead_cache.fd = -1;
}

/* Set the filesystem remote_hostio functions that take FILENAME
   arguments will use.  Return 0 on success, or -1 if an error
   occurs (and set *REMOTE_ERRNO).  */

static int
remote_hostio_set_filesystem (struct inferior *inf, int *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  int required_pid = (inf == NULL || inf->fake_pid_p) ? 0 : inf->pid;
  char *p = rs->buf;
  int left = get_remote_packet_size () - 1;
  char arg[9];
  int ret;

  if (packet_support (PACKET_vFile_setfs) == PACKET_DISABLE)
    return 0;

  if (rs->fs_pid != -1 && required_pid == rs->fs_pid)
    return 0;

  remote_buffer_add_string (&p, &left, "vFile:setfs:");

  xsnprintf (arg, sizeof (arg), "%x", required_pid);
  remote_buffer_add_string (&p, &left, arg);

  ret = remote_hostio_send_command (p - rs->buf, PACKET_vFile_setfs,
				    remote_errno, NULL, NULL);

  if (packet_support (PACKET_vFile_setfs) == PACKET_DISABLE)
    return 0;

  if (ret == 0)
    rs->fs_pid = required_pid;

  return ret;
}

/* Implementation of to_fileio_open.  */

static int
remote_hostio_open (struct target_ops *self,
		    struct inferior *inf, const char *filename,
		    int flags, int mode, int warn_if_slow,
		    int *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf;
  int left = get_remote_packet_size () - 1;

  if (warn_if_slow)
    {
      static int warning_issued = 0;

      printf_unfiltered (_("Reading %s from remote target...\n"),
			 filename);

      if (!warning_issued)
	{
	  warning (_("File transfers from remote targets can be slow."
		     " Use \"set sysroot\" to access files locally"
		     " instead."));
	  warning_issued = 1;
	}
    }

  if (remote_hostio_set_filesystem (inf, remote_errno) != 0)
    return -1;

  remote_buffer_add_string (&p, &left, "vFile:open:");

  remote_buffer_add_bytes (&p, &left, (const gdb_byte *) filename,
			   strlen (filename));
  remote_buffer_add_string (&p, &left, ",");

  remote_buffer_add_int (&p, &left, flags);
  remote_buffer_add_string (&p, &left, ",");

  remote_buffer_add_int (&p, &left, mode);

  return remote_hostio_send_command (p - rs->buf, PACKET_vFile_open,
				     remote_errno, NULL, NULL);
}

/* Implementation of to_fileio_pwrite.  */

static int
remote_hostio_pwrite (struct target_ops *self,
		      int fd, const gdb_byte *write_buf, int len,
		      ULONGEST offset, int *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf;
  int left = get_remote_packet_size ();
  int out_len;

  readahead_cache_invalidate_fd (fd);

  remote_buffer_add_string (&p, &left, "vFile:pwrite:");

  remote_buffer_add_int (&p, &left, fd);
  remote_buffer_add_string (&p, &left, ",");

  remote_buffer_add_int (&p, &left, offset);
  remote_buffer_add_string (&p, &left, ",");

  p += remote_escape_output (write_buf, len, 1, (gdb_byte *) p, &out_len,
			     get_remote_packet_size () - (p - rs->buf));

  return remote_hostio_send_command (p - rs->buf, PACKET_vFile_pwrite,
				     remote_errno, NULL, NULL);
}

/* Helper for the implementation of to_fileio_pread.  Read the file
   from the remote side with vFile:pread.  */

static int
remote_hostio_pread_vFile (struct target_ops *self,
			   int fd, gdb_byte *read_buf, int len,
			   ULONGEST offset, int *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf;
  char *attachment;
  int left = get_remote_packet_size ();
  int ret, attachment_len;
  int read_len;

  remote_buffer_add_string (&p, &left, "vFile:pread:");

  remote_buffer_add_int (&p, &left, fd);
  remote_buffer_add_string (&p, &left, ",");

  remote_buffer_add_int (&p, &left, len);
  remote_buffer_add_string (&p, &left, ",");

  remote_buffer_add_int (&p, &left, offset);

  ret = remote_hostio_send_command (p - rs->buf, PACKET_vFile_pread,
				    remote_errno, &attachment,
				    &attachment_len);

  if (ret < 0)
    return ret;

  read_len = remote_unescape_input ((gdb_byte *) attachment, attachment_len,
				    read_buf, len);
  if (read_len != ret)
    error (_("Read returned %d, but %d bytes."), ret, (int) read_len);

  return ret;
}

/* Serve pread from the readahead cache.  Returns number of bytes
   read, or 0 if the request can't be served from the cache.  */

static int
remote_hostio_pread_from_cache (struct remote_state *rs,
				int fd, gdb_byte *read_buf, size_t len,
				ULONGEST offset)
{
  struct readahead_cache *cache = &rs->readahead_cache;

  if (cache->fd == fd
      && cache->offset <= offset
      && offset < cache->offset + cache->bufsize)
    {
      ULONGEST max = cache->offset + cache->bufsize;

      if (offset + len > max)
	len = max - offset;

      memcpy (read_buf, cache->buf + offset - cache->offset, len);
      return len;
    }

  return 0;
}

/* Implementation of to_fileio_pread.  */

static int
remote_hostio_pread (struct target_ops *self,
		     int fd, gdb_byte *read_buf, int len,
		     ULONGEST offset, int *remote_errno)
{
  int ret;
  struct remote_state *rs = get_remote_state ();
  struct readahead_cache *cache = &rs->readahead_cache;

  ret = remote_hostio_pread_from_cache (rs, fd, read_buf, len, offset);
  if (ret > 0)
    {
      cache->hit_count++;

      if (remote_debug)
	fprintf_unfiltered (gdb_stdlog, "readahead cache hit %s\n",
			    pulongest (cache->hit_count));
      return ret;
    }

  cache->miss_count++;
  if (remote_debug)
    fprintf_unfiltered (gdb_stdlog, "readahead cache miss %s\n",
			pulongest (cache->miss_count));

  cache->fd = fd;
  cache->offset = offset;
  cache->bufsize = get_remote_packet_size ();
  cache->buf = (gdb_byte *) xrealloc (cache->buf, cache->bufsize);

  ret = remote_hostio_pread_vFile (self, cache->fd, cache->buf, cache->bufsize,
				   cache->offset, remote_errno);
  if (ret <= 0)
    {
      readahead_cache_invalidate_fd (fd);
      return ret;
    }

  cache->bufsize = ret;
  return remote_hostio_pread_from_cache (rs, fd, read_buf, len, offset);
}

/* Implementation of to_fileio_close.  */

static int
remote_hostio_close (struct target_ops *self, int fd, int *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf;
  int left = get_remote_packet_size () - 1;

  readahead_cache_invalidate_fd (fd);

  remote_buffer_add_string (&p, &left, "vFile:close:");

  remote_buffer_add_int (&p, &left, fd);

  return remote_hostio_send_command (p - rs->buf, PACKET_vFile_close,
				     remote_errno, NULL, NULL);
}

/* Implementation of to_fileio_unlink.  */

static int
remote_hostio_unlink (struct target_ops *self,
		      struct inferior *inf, const char *filename,
		      int *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf;
  int left = get_remote_packet_size () - 1;

  if (remote_hostio_set_filesystem (inf, remote_errno) != 0)
    return -1;

  remote_buffer_add_string (&p, &left, "vFile:unlink:");

  remote_buffer_add_bytes (&p, &left, (const gdb_byte *) filename,
			   strlen (filename));

  return remote_hostio_send_command (p - rs->buf, PACKET_vFile_unlink,
				     remote_errno, NULL, NULL);
}

/* Implementation of to_fileio_readlink.  */

static char *
remote_hostio_readlink (struct target_ops *self,
			struct inferior *inf, const char *filename,
			int *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf;
  char *attachment;
  int left = get_remote_packet_size ();
  int len, attachment_len;
  int read_len;
  char *ret;

  if (remote_hostio_set_filesystem (inf, remote_errno) != 0)
    return NULL;

  remote_buffer_add_string (&p, &left, "vFile:readlink:");

  remote_buffer_add_bytes (&p, &left, (const gdb_byte *) filename,
			   strlen (filename));

  len = remote_hostio_send_command (p - rs->buf, PACKET_vFile_readlink,
				    remote_errno, &attachment,
				    &attachment_len);

  if (len < 0)
    return NULL;

  ret = (char *) xmalloc (len + 1);

  read_len = remote_unescape_input ((gdb_byte *) attachment, attachment_len,
				    (gdb_byte *) ret, len);
  if (read_len != len)
    error (_("Readlink returned %d, but %d bytes."), len, read_len);

  ret[len] = '\0';
  return ret;
}

/* Implementation of to_fileio_fstat.  */

static int
remote_hostio_fstat (struct target_ops *self,
		     int fd, struct stat *st,
		     int *remote_errno)
{
  struct remote_state *rs = get_remote_state ();
  char *p = rs->buf;
  int left = get_remote_packet_size ();
  int attachment_len, ret;
  char *attachment;
  struct fio_stat fst;
  int read_len;

  remote_buffer_add_string (&p, &left, "vFile:fstat:");

  remote_buffer_add_int (&p, &left, fd);

  ret = remote_hostio_send_command (p - rs->buf, PACKET_vFile_fstat,
				    remote_errno, &attachment,
				    &attachment_len);
  if (ret < 0)
    {
      if (*remote_errno != FILEIO_ENOSYS)
	return ret;

      /* Strictly we should return -1, ENOSYS here, but when
	 "set sysroot remote:" was implemented in August 2008
	 BFD's need for a stat function was sidestepped with
	 this hack.  This was not remedied until March 2015
	 so we retain the previous behavior to avoid breaking
	 compatibility.

	 Note that the memset is a March 2015 addition; older
	 GDBs set st_size *and nothing else* so the structure
	 would have garbage in all other fields.  This might
	 break something but retaining the previous behavior
	 here would be just too wrong.  */

      memset (st, 0, sizeof (struct stat));
      st->st_size = INT_MAX;
      return 0;
    }

  read_len = remote_unescape_input ((gdb_byte *) attachment, attachment_len,
				    (gdb_byte *) &fst, sizeof (fst));

  if (read_len != ret)
    error (_("vFile:fstat returned %d, but %d bytes."), ret, read_len);

  if (read_len != sizeof (fst))
    error (_("vFile:fstat returned %d bytes, but expecting %d."),
	   read_len, (int) sizeof (fst));

  remote_fileio_to_host_stat (&fst, st);

  return 0;
}

/* Implementation of to_filesystem_is_local.  */

static int
remote_filesystem_is_local (struct target_ops *self)
{
  /* Valgrind GDB presents itself as a remote target but works
     on the local filesystem: it does not implement remote get
     and users are not expected to set a sysroot.  To handle
     this case we treat the remote filesystem as local if the
     sysroot is exactly TARGET_SYSROOT_PREFIX and if the stub
     does not support vFile:open.  */
  if (strcmp (gdb_sysroot, TARGET_SYSROOT_PREFIX) == 0)
    {
      enum packet_support ps = packet_support (PACKET_vFile_open);

      if (ps == PACKET_SUPPORT_UNKNOWN)
	{
	  int fd, remote_errno;

	  /* Try opening a file to probe support.  The supplied
	     filename is irrelevant, we only care about whether
	     the stub recognizes the packet or not.  */
	  fd = remote_hostio_open (self, NULL, "just probing",
				   FILEIO_O_RDONLY, 0700, 0,
				   &remote_errno);

	  if (fd >= 0)
	    remote_hostio_close (self, fd, &remote_errno);

	  ps = packet_support (PACKET_vFile_open);
	}

      if (ps == PACKET_DISABLE)
	{
	  static int warning_issued = 0;

	  if (!warning_issued)
	    {
	      warning (_("remote target does not support file"
			 " transfer, attempting to access files"
			 " from local filesystem."));
	      warning_issued = 1;
	    }

	  return 1;
	}
    }

  return 0;
}

static int
remote_fileio_errno_to_host (int errnum)
{
  switch (errnum)
    {
      case FILEIO_EPERM:
        return EPERM;
      case FILEIO_ENOENT:
        return ENOENT;
      case FILEIO_EINTR:
        return EINTR;
      case FILEIO_EIO:
        return EIO;
      case FILEIO_EBADF:
        return EBADF;
      case FILEIO_EACCES:
        return EACCES;
      case FILEIO_EFAULT:
        return EFAULT;
      case FILEIO_EBUSY:
        return EBUSY;
      case FILEIO_EEXIST:
        return EEXIST;
      case FILEIO_ENODEV:
        return ENODEV;
      case FILEIO_ENOTDIR:
        return ENOTDIR;
      case FILEIO_EISDIR:
        return EISDIR;
      case FILEIO_EINVAL:
        return EINVAL;
      case FILEIO_ENFILE:
        return ENFILE;
      case FILEIO_EMFILE:
        return EMFILE;
      case FILEIO_EFBIG:
        return EFBIG;
      case FILEIO_ENOSPC:
        return ENOSPC;
      case FILEIO_ESPIPE:
        return ESPIPE;
      case FILEIO_EROFS:
        return EROFS;
      case FILEIO_ENOSYS:
        return ENOSYS;
      case FILEIO_ENAMETOOLONG:
        return ENAMETOOLONG;
    }
  return -1;
}

static char *
remote_hostio_error (int errnum)
{
  int host_error = remote_fileio_errno_to_host (errnum);

  if (host_error == -1)
    error (_("Unknown remote I/O error %d"), errnum);
  else
    error (_("Remote I/O error: %s"), safe_strerror (host_error));
}

static void
remote_hostio_close_cleanup (void *opaque)
{
  int fd = *(int *) opaque;
  int remote_errno;

  remote_hostio_close (find_target_at (process_stratum), fd, &remote_errno);
}

void
remote_file_put (const char *local_file, const char *remote_file, int from_tty)
{
  struct cleanup *back_to, *close_cleanup;
  int retcode, fd, remote_errno, bytes, io_size;
  FILE *file;
  gdb_byte *buffer;
  int bytes_in_buffer;
  int saw_eof;
  ULONGEST offset;
  struct remote_state *rs = get_remote_state ();

  if (!rs->remote_desc)
    error (_("command can only be used with remote target"));

  file = gdb_fopen_cloexec (local_file, "rb");
  if (file == NULL)
    perror_with_name (local_file);
  back_to = make_cleanup_fclose (file);

  fd = remote_hostio_open (find_target_at (process_stratum), NULL,
			   remote_file, (FILEIO_O_WRONLY | FILEIO_O_CREAT
					 | FILEIO_O_TRUNC),
			   0700, 0, &remote_errno);
  if (fd == -1)
    remote_hostio_error (remote_errno);

  /* Send up to this many bytes at once.  They won't all fit in the
     remote packet limit, so we'll transfer slightly fewer.  */
  io_size = get_remote_packet_size ();
  buffer = (gdb_byte *) xmalloc (io_size);
  make_cleanup (xfree, buffer);

  close_cleanup = make_cleanup (remote_hostio_close_cleanup, &fd);

  bytes_in_buffer = 0;
  saw_eof = 0;
  offset = 0;
  while (bytes_in_buffer || !saw_eof)
    {
      if (!saw_eof)
	{
	  bytes = fread (buffer + bytes_in_buffer, 1,
			 io_size - bytes_in_buffer,
			 file);
	  if (bytes == 0)
	    {
	      if (ferror (file))
		error (_("Error reading %s."), local_file);
	      else
		{
		  /* EOF.  Unless there is something still in the
		     buffer from the last iteration, we are done.  */
		  saw_eof = 1;
		  if (bytes_in_buffer == 0)
		    break;
		}
	    }
	}
      else
	bytes = 0;

      bytes += bytes_in_buffer;
      bytes_in_buffer = 0;

      retcode = remote_hostio_pwrite (find_target_at (process_stratum),
				      fd, buffer, bytes,
				      offset, &remote_errno);

      if (retcode < 0)
	remote_hostio_error (remote_errno);
      else if (retcode == 0)
	error (_("Remote write of %d bytes returned 0!"), bytes);
      else if (retcode < bytes)
	{
	  /* Short write.  Save the rest of the read data for the next
	     write.  */
	  bytes_in_buffer = bytes - retcode;
	  memmove (buffer, buffer + retcode, bytes_in_buffer);
	}

      offset += retcode;
    }

  discard_cleanups (close_cleanup);
  if (remote_hostio_close (find_target_at (process_stratum), fd, &remote_errno))
    remote_hostio_error (remote_errno);

  if (from_tty)
    printf_filtered (_("Successfully sent file \"%s\".\n"), local_file);
  do_cleanups (back_to);
}

void
remote_file_get (const char *remote_file, const char *local_file, int from_tty)
{
  struct cleanup *back_to, *close_cleanup;
  int fd, remote_errno, bytes, io_size;
  FILE *file;
  gdb_byte *buffer;
  ULONGEST offset;
  struct remote_state *rs = get_remote_state ();

  if (!rs->remote_desc)
    error (_("command can only be used with remote target"));

  fd = remote_hostio_open (find_target_at (process_stratum), NULL,
			   remote_file, FILEIO_O_RDONLY, 0, 0,
			   &remote_errno);
  if (fd == -1)
    remote_hostio_error (remote_errno);

  file = gdb_fopen_cloexec (local_file, "wb");
  if (file == NULL)
    perror_with_name (local_file);
  back_to = make_cleanup_fclose (file);

  /* Send up to this many bytes at once.  They won't all fit in the
     remote packet limit, so we'll transfer slightly fewer.  */
  io_size = get_remote_packet_size ();
  buffer = (gdb_byte *) xmalloc (io_size);
  make_cleanup (xfree, buffer);

  close_cleanup = make_cleanup (remote_hostio_close_cleanup, &fd);

  offset = 0;
  while (1)
    {
      bytes = remote_hostio_pread (find_target_at (process_stratum),
				   fd, buffer, io_size, offset, &remote_errno);
      if (bytes == 0)
	/* Success, but no bytes, means end-of-file.  */
	break;
      if (bytes == -1)
	remote_hostio_error (remote_errno);

      offset += bytes;

      bytes = fwrite (buffer, 1, bytes, file);
      if (bytes == 0)
	perror_with_name (local_file);
    }

  discard_cleanups (close_cleanup);
  if (remote_hostio_close (find_target_at (process_stratum), fd, &remote_errno))
    remote_hostio_error (remote_errno);

  if (from_tty)
    printf_filtered (_("Successfully fetched file \"%s\".\n"), remote_file);
  do_cleanups (back_to);
}

void
remote_file_delete (const char *remote_file, int from_tty)
{
  int retcode, remote_errno;
  struct remote_state *rs = get_remote_state ();

  if (!rs->remote_desc)
    error (_("command can only be used with remote target"));

  retcode = remote_hostio_unlink (find_target_at (process_stratum),
				  NULL, remote_file, &remote_errno);
  if (retcode == -1)
    remote_hostio_error (remote_errno);

  if (from_tty)
    printf_filtered (_("Successfully deleted file \"%s\".\n"), remote_file);
}

static void
remote_put_command (char *args, int from_tty)
{
  struct cleanup *back_to;
  char **argv;

  if (args == NULL)
    error_no_arg (_("file to put"));

  argv = gdb_buildargv (args);
  back_to = make_cleanup_freeargv (argv);
  if (argv[0] == NULL || argv[1] == NULL || argv[2] != NULL)
    error (_("Invalid parameters to remote put"));

  remote_file_put (argv[0], argv[1], from_tty);

  do_cleanups (back_to);
}

static void
remote_get_command (char *args, int from_tty)
{
  struct cleanup *back_to;
  char **argv;

  if (args == NULL)
    error_no_arg (_("file to get"));

  argv = gdb_buildargv (args);
  back_to = make_cleanup_freeargv (argv);
  if (argv[0] == NULL || argv[1] == NULL || argv[2] != NULL)
    error (_("Invalid parameters to remote get"));

  remote_file_get (argv[0], argv[1], from_tty);

  do_cleanups (back_to);
}

static void
remote_delete_command (char *args, int from_tty)
{
  struct cleanup *back_to;
  char **argv;

  if (args == NULL)
    error_no_arg (_("file to delete"));

  argv = gdb_buildargv (args);
  back_to = make_cleanup_freeargv (argv);
  if (argv[0] == NULL || argv[1] != NULL)
    error (_("Invalid parameters to remote delete"));

  remote_file_delete (argv[0], from_tty);

  do_cleanups (back_to);
}

static void
remote_command (char *args, int from_tty)
{
  help_list (remote_cmdlist, "remote ", all_commands, gdb_stdout);
}

static int
remote_can_execute_reverse (struct target_ops *self)
{
  if (packet_support (PACKET_bs) == PACKET_ENABLE
      || packet_support (PACKET_bc) == PACKET_ENABLE)
    return 1;
  else
    return 0;
}

static int
remote_supports_non_stop (struct target_ops *self)
{
  return 1;
}

static int
remote_supports_disable_randomization (struct target_ops *self)
{
  /* Only supported in extended mode.  */
  return 0;
}

static int
remote_supports_multi_process (struct target_ops *self)
{
  struct remote_state *rs = get_remote_state ();

  return remote_multi_process_p (rs);
}

static int
remote_supports_cond_tracepoints (void)
{
  return packet_support (PACKET_ConditionalTracepoints) == PACKET_ENABLE;
}

static int
remote_supports_cond_breakpoints (struct target_ops *self)
{
  return packet_support (PACKET_ConditionalBreakpoints) == PACKET_ENABLE;
}

static int
remote_supports_fast_tracepoints (void)
{
  return packet_support (PACKET_FastTracepoints) == PACKET_ENABLE;
}

static int
remote_supports_static_tracepoints (void)
{
  return packet_support (PACKET_StaticTracepoints) == PACKET_ENABLE;
}

static int
remote_supports_install_in_trace (void)
{
  return packet_support (PACKET_InstallInTrace) == PACKET_ENABLE;
}

static int
remote_supports_enable_disable_tracepoint (struct target_ops *self)
{
  return (packet_support (PACKET_EnableDisableTracepoints_feature)
	  == PACKET_ENABLE);
}

static int
remote_supports_string_tracing (struct target_ops *self)
{
  return packet_support (PACKET_tracenz_feature) == PACKET_ENABLE;
}

static int
remote_can_run_breakpoint_commands (struct target_ops *self)
{
  return packet_support (PACKET_BreakpointCommands) == PACKET_ENABLE;
}

static void
remote_trace_init (struct target_ops *self)
{
  putpkt ("QTinit");
  remote_get_noisy_reply (&target_buf, &target_buf_size);
  if (strcmp (target_buf, "OK") != 0)
    error (_("Target does not support this command."));
}

static void free_actions_list (char **actions_list);
static void free_actions_list_cleanup_wrapper (void *);
static void
free_actions_list_cleanup_wrapper (void *al)
{
  free_actions_list ((char **) al);
}

static void
free_actions_list (char **actions_list)
{
  int ndx;

  if (actions_list == 0)
    return;

  for (ndx = 0; actions_list[ndx]; ndx++)
    xfree (actions_list[ndx]);

  xfree (actions_list);
}

/* Recursive routine to walk through command list including loops, and
   download packets for each command.  */

static void
remote_download_command_source (int num, ULONGEST addr,
				struct command_line *cmds)
{
  struct remote_state *rs = get_remote_state ();
  struct command_line *cmd;

  for (cmd = cmds; cmd; cmd = cmd->next)
    {
      QUIT;	/* Allow user to bail out with ^C.  */
      strcpy (rs->buf, "QTDPsrc:");
      encode_source_string (num, addr, "cmd", cmd->line,
			    rs->buf + strlen (rs->buf),
			    rs->buf_size - strlen (rs->buf));
      putpkt (rs->buf);
      remote_get_noisy_reply (&target_buf, &target_buf_size);
      if (strcmp (target_buf, "OK"))
	warning (_("Target does not support source download."));

      if (cmd->control_type == while_control
	  || cmd->control_type == while_stepping_control)
	{
	  remote_download_command_source (num, addr, *cmd->body_list);

	  QUIT;	/* Allow user to bail out with ^C.  */
	  strcpy (rs->buf, "QTDPsrc:");
	  encode_source_string (num, addr, "cmd", "end",
				rs->buf + strlen (rs->buf),
				rs->buf_size - strlen (rs->buf));
	  putpkt (rs->buf);
	  remote_get_noisy_reply (&target_buf, &target_buf_size);
	  if (strcmp (target_buf, "OK"))
	    warning (_("Target does not support source download."));
	}
    }
}

static void
remote_download_tracepoint (struct target_ops *self, struct bp_location *loc)
{
#define BUF_SIZE 2048

  CORE_ADDR tpaddr;
  char addrbuf[40];
  char buf[BUF_SIZE];
  char **tdp_actions;
  char **stepping_actions;
  int ndx;
  struct cleanup *old_chain = NULL;
  struct agent_expr *aexpr;
  struct cleanup *aexpr_chain = NULL;
  char *pkt;
  struct breakpoint *b = loc->owner;
  struct tracepoint *t = (struct tracepoint *) b;

  encode_actions_rsp (loc, &tdp_actions, &stepping_actions);
  old_chain = make_cleanup (free_actions_list_cleanup_wrapper,
			    tdp_actions);
  (void) make_cleanup (free_actions_list_cleanup_wrapper,
		       stepping_actions);

  tpaddr = loc->address;
  sprintf_vma (addrbuf, tpaddr);
  xsnprintf (buf, BUF_SIZE, "QTDP:%x:%s:%c:%lx:%x", b->number,
	     addrbuf, /* address */
	     (b->enable_state == bp_enabled ? 'E' : 'D'),
	     t->step_count, t->pass_count);
  /* Fast tracepoints are mostly handled by the target, but we can
     tell the target how big of an instruction block should be moved
     around.  */
  if (b->type == bp_fast_tracepoint)
    {
      /* Only test for support at download time; we may not know
	 target capabilities at definition time.  */
      if (remote_supports_fast_tracepoints ())
	{
	  if (gdbarch_fast_tracepoint_valid_at (loc->gdbarch, tpaddr,
						NULL))
	    xsnprintf (buf + strlen (buf), BUF_SIZE - strlen (buf), ":F%x",
		       gdb_insn_length (loc->gdbarch, tpaddr));
	  else
	    /* If it passed validation at definition but fails now,
	       something is very wrong.  */
	    internal_error (__FILE__, __LINE__,
			    _("Fast tracepoint not "
			      "valid during download"));
	}
      else
	/* Fast tracepoints are functionally identical to regular
	   tracepoints, so don't take lack of support as a reason to
	   give up on the trace run.  */
	warning (_("Target does not support fast tracepoints, "
		   "downloading %d as regular tracepoint"), b->number);
    }
  else if (b->type == bp_static_tracepoint)
    {
      /* Only test for support at download time; we may not know
	 target capabilities at definition time.  */
      if (remote_supports_static_tracepoints ())
	{
	  struct static_tracepoint_marker marker;

	  if (target_static_tracepoint_marker_at (tpaddr, &marker))
	    strcat (buf, ":S");
	  else
	    error (_("Static tracepoint not valid during download"));
	}
      else
	/* Fast tracepoints are functionally identical to regular
	   tracepoints, so don't take lack of support as a reason
	   to give up on the trace run.  */
	error (_("Target does not support static tracepoints"));
    }
  /* If the tracepoint has a conditional, make it into an agent
     expression and append to the definition.  */
  if (loc->cond)
    {
      /* Only test support at download time, we may not know target
	 capabilities at definition time.  */
      if (remote_supports_cond_tracepoints ())
	{
	  aexpr = gen_eval_for_expr (tpaddr, loc->cond);
	  aexpr_chain = make_cleanup_free_agent_expr (aexpr);
	  xsnprintf (buf + strlen (buf), BUF_SIZE - strlen (buf), ":X%x,",
		     aexpr->len);
	  pkt = buf + strlen (buf);
	  for (ndx = 0; ndx < aexpr->len; ++ndx)
	    pkt = pack_hex_byte (pkt, aexpr->buf[ndx]);
	  *pkt = '\0';
	  do_cleanups (aexpr_chain);
	}
      else
	warning (_("Target does not support conditional tracepoints, "
		   "ignoring tp %d cond"), b->number);
    }

  if (b->commands || *default_collect)
    strcat (buf, "-");
  putpkt (buf);
  remote_get_noisy_reply (&target_buf, &target_buf_size);
  if (strcmp (target_buf, "OK"))
    error (_("Target does not support tracepoints."));

  /* do_single_steps (t); */
  if (tdp_actions)
    {
      for (ndx = 0; tdp_actions[ndx]; ndx++)
	{
	  QUIT;	/* Allow user to bail out with ^C.  */
	  xsnprintf (buf, BUF_SIZE, "QTDP:-%x:%s:%s%c",
		     b->number, addrbuf, /* address */
		     tdp_actions[ndx],
		     ((tdp_actions[ndx + 1] || stepping_actions)
		      ? '-' : 0));
	  putpkt (buf);
	  remote_get_noisy_reply (&target_buf,
				  &target_buf_size);
	  if (strcmp (target_buf, "OK"))
	    error (_("Error on target while setting tracepoints."));
	}
    }
  if (stepping_actions)
    {
      for (ndx = 0; stepping_actions[ndx]; ndx++)
	{
	  QUIT;	/* Allow user to bail out with ^C.  */
	  xsnprintf (buf, BUF_SIZE, "QTDP:-%x:%s:%s%s%s",
		     b->number, addrbuf, /* address */
		     ((ndx == 0) ? "S" : ""),
		     stepping_actions[ndx],
		     (stepping_actions[ndx + 1] ? "-" : ""));
	  putpkt (buf);
	  remote_get_noisy_reply (&target_buf,
				  &target_buf_size);
	  if (strcmp (target_buf, "OK"))
	    error (_("Error on target while setting tracepoints."));
	}
    }

  if (packet_support (PACKET_TracepointSource) == PACKET_ENABLE)
    {
      if (b->location != NULL)
	{
	  strcpy (buf, "QTDPsrc:");
	  encode_source_string (b->number, loc->address, "at",
				event_location_to_string (b->location),
				buf + strlen (buf), 2048 - strlen (buf));
	  putpkt (buf);
	  remote_get_noisy_reply (&target_buf, &target_buf_size);
	  if (strcmp (target_buf, "OK"))
	    warning (_("Target does not support source download."));
	}
      if (b->cond_string)
	{
	  strcpy (buf, "QTDPsrc:");
	  encode_source_string (b->number, loc->address,
				"cond", b->cond_string, buf + strlen (buf),
				2048 - strlen (buf));
	  putpkt (buf);
	  remote_get_noisy_reply (&target_buf, &target_buf_size);
	  if (strcmp (target_buf, "OK"))
	    warning (_("Target does not support source download."));
	}
      remote_download_command_source (b->number, loc->address,
				      breakpoint_commands (b));
    }

  do_cleanups (old_chain);
}

static int
remote_can_download_tracepoint (struct target_ops *self)
{
  struct remote_state *rs = get_remote_state ();
  struct trace_status *ts;
  int status;

  /* Don't try to install tracepoints until we've relocated our
     symbols, and fetched and merged the target's tracepoint list with
     ours.  */
  if (rs->starting_up)
    return 0;

  ts = current_trace_status ();
  status = remote_get_trace_status (self, ts);

  if (status == -1 || !ts->running_known || !ts->running)
    return 0;

  /* If we are in a tracing experiment, but remote stub doesn't support
     installing tracepoint in trace, we have to return.  */
  if (!remote_supports_install_in_trace ())
    return 0;

  return 1;
}


static void
remote_download_trace_state_variable (struct target_ops *self,
				      struct trace_state_variable *tsv)
{
  struct remote_state *rs = get_remote_state ();
  char *p;

  xsnprintf (rs->buf, get_remote_packet_size (), "QTDV:%x:%s:%x:",
	     tsv->number, phex ((ULONGEST) tsv->initial_value, 8),
	     tsv->builtin);
  p = rs->buf + strlen (rs->buf);
  if ((p - rs->buf) + strlen (tsv->name) * 2 >= get_remote_packet_size ())
    error (_("Trace state variable name too long for tsv definition packet"));
  p += 2 * bin2hex ((gdb_byte *) (tsv->name), p, strlen (tsv->name));
  *p++ = '\0';
  putpkt (rs->buf);
  remote_get_noisy_reply (&target_buf, &target_buf_size);
  if (*target_buf == '\0')
    error (_("Target does not support this command."));
  if (strcmp (target_buf, "OK") != 0)
    error (_("Error on target while downloading trace state variable."));
}

static void
remote_enable_tracepoint (struct target_ops *self,
			  struct bp_location *location)
{
  struct remote_state *rs = get_remote_state ();
  char addr_buf[40];

  sprintf_vma (addr_buf, location->address);
  xsnprintf (rs->buf, get_remote_packet_size (), "QTEnable:%x:%s",
	     location->owner->number, addr_buf);
  putpkt (rs->buf);
  remote_get_noisy_reply (&rs->buf, &rs->buf_size);
  if (*rs->buf == '\0')
    error (_("Target does not support enabling tracepoints while a trace run is ongoing."));
  if (strcmp (rs->buf, "OK") != 0)
    error (_("Error on target while enabling tracepoint."));
}

static void
remote_disable_tracepoint (struct target_ops *self,
			   struct bp_location *location)
{
  struct remote_state *rs = get_remote_state ();
  char addr_buf[40];

  sprintf_vma (addr_buf, location->address);
  xsnprintf (rs->buf, get_remote_packet_size (), "QTDisable:%x:%s",
	     location->owner->number, addr_buf);
  putpkt (rs->buf);
  remote_get_noisy_reply (&rs->buf, &rs->buf_size);
  if (*rs->buf == '\0')
    error (_("Target does not support disabling tracepoints while a trace run is ongoing."));
  if (strcmp (rs->buf, "OK") != 0)
    error (_("Error on target while disabling tracepoint."));
}

static void
remote_trace_set_readonly_regions (struct target_ops *self)
{
  asection *s;
  bfd *abfd = NULL;
  bfd_size_type size;
  bfd_vma vma;
  int anysecs = 0;
  int offset = 0;

  if (!exec_bfd)
    return;			/* No information to give.  */

  strcpy (target_buf, "QTro");
  offset = strlen (target_buf);
  for (s = exec_bfd->sections; s; s = s->next)
    {
      char tmp1[40], tmp2[40];
      int sec_length;

      if ((s->flags & SEC_LOAD) == 0 ||
      /*  (s->flags & SEC_CODE) == 0 || */
	  (s->flags & SEC_READONLY) == 0)
	continue;

      anysecs = 1;
      vma = bfd_get_section_vma (abfd, s);
      size = bfd_get_section_size (s);
      sprintf_vma (tmp1, vma);
      sprintf_vma (tmp2, vma + size);
      sec_length = 1 + strlen (tmp1) + 1 + strlen (tmp2);
      if (offset + sec_length + 1 > target_buf_size)
	{
	  if (packet_support (PACKET_qXfer_traceframe_info) != PACKET_ENABLE)
	    warning (_("\
Too many sections for read-only sections definition packet."));
	  break;
	}
      xsnprintf (target_buf + offset, target_buf_size - offset, ":%s,%s",
		 tmp1, tmp2);
      offset += sec_length;
    }
  if (anysecs)
    {
      putpkt (target_buf);
      getpkt (&target_buf, &target_buf_size, 0);
    }
}

static void
remote_trace_start (struct target_ops *self)
{
  putpkt ("QTStart");
  remote_get_noisy_reply (&target_buf, &target_buf_size);
  if (*target_buf == '\0')
    error (_("Target does not support this command."));
  if (strcmp (target_buf, "OK") != 0)
    error (_("Bogus reply from target: %s"), target_buf);
}

static int
remote_get_trace_status (struct target_ops *self, struct trace_status *ts)
{
  /* Initialize it just to avoid a GCC false warning.  */
  char *p = NULL;
  /* FIXME we need to get register block size some other way.  */
  extern int trace_regblock_size;
  enum packet_result result;

  if (packet_support (PACKET_qTStatus) == PACKET_DISABLE)
    return -1;

  trace_regblock_size = get_remote_arch_state ()->sizeof_g_packet;

  putpkt ("qTStatus");

  TRY
    {
      p = remote_get_noisy_reply (&target_buf, &target_buf_size);
    }
  CATCH (ex, RETURN_MASK_ERROR)
    {
      if (ex.error != TARGET_CLOSE_ERROR)
	{
	  exception_fprintf (gdb_stderr, ex, "qTStatus: ");
	  return -1;
	}
      throw_exception (ex);
    }
  END_CATCH

  result = packet_ok (p, &remote_protocol_packets[PACKET_qTStatus]);

  /* If the remote target doesn't do tracing, flag it.  */
  if (result == PACKET_UNKNOWN)
    return -1;

  /* We're working with a live target.  */
  ts->filename = NULL;

  if (*p++ != 'T')
    error (_("Bogus trace status reply from target: %s"), target_buf);

  /* Function 'parse_trace_status' sets default value of each field of
     'ts' at first, so we don't have to do it here.  */
  parse_trace_status (p, ts);

  return ts->running;
}

static void
remote_get_tracepoint_status (struct target_ops *self, struct breakpoint *bp,
			      struct uploaded_tp *utp)
{
  struct remote_state *rs = get_remote_state ();
  char *reply;
  struct bp_location *loc;
  struct tracepoint *tp = (struct tracepoint *) bp;
  size_t size = get_remote_packet_size ();

  if (tp)
    {
      tp->base.hit_count = 0;
      tp->traceframe_usage = 0;
      for (loc = tp->base.loc; loc; loc = loc->next)
	{
	  /* If the tracepoint was never downloaded, don't go asking for
	     any status.  */
	  if (tp->number_on_target == 0)
	    continue;
	  xsnprintf (rs->buf, size, "qTP:%x:%s", tp->number_on_target,
		     phex_nz (loc->address, 0));
	  putpkt (rs->buf);
	  reply = remote_get_noisy_reply (&target_buf, &target_buf_size);
	  if (reply && *reply)
	    {
	      if (*reply == 'V')
		parse_tracepoint_status (reply + 1, bp, utp);
	    }
	}
    }
  else if (utp)
    {
      utp->hit_count = 0;
      utp->traceframe_usage = 0;
      xsnprintf (rs->buf, size, "qTP:%x:%s", utp->number,
		 phex_nz (utp->addr, 0));
      putpkt (rs->buf);
      reply = remote_get_noisy_reply (&target_buf, &target_buf_size);
      if (reply && *reply)
	{
	  if (*reply == 'V')
	    parse_tracepoint_status (reply + 1, bp, utp);
	}
    }
}

static void
remote_trace_stop (struct target_ops *self)
{
  putpkt ("QTStop");
  remote_get_noisy_reply (&target_buf, &target_buf_size);
  if (*target_buf == '\0')
    error (_("Target does not support this command."));
  if (strcmp (target_buf, "OK") != 0)
    error (_("Bogus reply from target: %s"), target_buf);
}

static int
remote_trace_find (struct target_ops *self,
		   enum trace_find_type type, int num,
		   CORE_ADDR addr1, CORE_ADDR addr2,
		   int *tpp)
{
  struct remote_state *rs = get_remote_state ();
  char *endbuf = rs->buf + get_remote_packet_size ();
  char *p, *reply;
  int target_frameno = -1, target_tracept = -1;

  /* Lookups other than by absolute frame number depend on the current
     trace selected, so make sure it is correct on the remote end
     first.  */
  if (type != tfind_number)
    set_remote_traceframe ();

  p = rs->buf;
  strcpy (p, "QTFrame:");
  p = strchr (p, '\0');
  switch (type)
    {
    case tfind_number:
      xsnprintf (p, endbuf - p, "%x", num);
      break;
    case tfind_pc:
      xsnprintf (p, endbuf - p, "pc:%s", phex_nz (addr1, 0));
      break;
    case tfind_tp:
      xsnprintf (p, endbuf - p, "tdp:%x", num);
      break;
    case tfind_range:
      xsnprintf (p, endbuf - p, "range:%s:%s", phex_nz (addr1, 0),
		 phex_nz (addr2, 0));
      break;
    case tfind_outside:
      xsnprintf (p, endbuf - p, "outside:%s:%s", phex_nz (addr1, 0),
		 phex_nz (addr2, 0));
      break;
    default:
      error (_("Unknown trace find type %d"), type);
    }

  putpkt (rs->buf);
  reply = remote_get_noisy_reply (&(rs->buf), &rs->buf_size);
  if (*reply == '\0')
    error (_("Target does not support this command."));

  while (reply && *reply)
    switch (*reply)
      {
      case 'F':
	p = ++reply;
	target_frameno = (int) strtol (p, &reply, 16);
	if (reply == p)
	  error (_("Unable to parse trace frame number"));
	/* Don't update our remote traceframe number cache on failure
	   to select a remote traceframe.  */
	if (target_frameno == -1)
	  return -1;
	break;
      case 'T':
	p = ++reply;
	target_tracept = (int) strtol (p, &reply, 16);
	if (reply == p)
	  error (_("Unable to parse tracepoint number"));
	break;
      case 'O':		/* "OK"? */
	if (reply[1] == 'K' && reply[2] == '\0')
	  reply += 2;
	else
	  error (_("Bogus reply from target: %s"), reply);
	break;
      default:
	error (_("Bogus reply from target: %s"), reply);
      }
  if (tpp)
    *tpp = target_tracept;

  rs->remote_traceframe_number = target_frameno;
  return target_frameno;
}

static int
remote_get_trace_state_variable_value (struct target_ops *self,
				       int tsvnum, LONGEST *val)
{
  struct remote_state *rs = get_remote_state ();
  char *reply;
  ULONGEST uval;

  set_remote_traceframe ();

  xsnprintf (rs->buf, get_remote_packet_size (), "qTV:%x", tsvnum);
  putpkt (rs->buf);
  reply = remote_get_noisy_reply (&target_buf, &target_buf_size);
  if (reply && *reply)
    {
      if (*reply == 'V')
	{
	  unpack_varlen_hex (reply + 1, &uval);
	  *val = (LONGEST) uval;
	  return 1;
	}
    }
  return 0;
}

static int
remote_save_trace_data (struct target_ops *self, const char *filename)
{
  struct remote_state *rs = get_remote_state ();
  char *p, *reply;

  p = rs->buf;
  strcpy (p, "QTSave:");
  p += strlen (p);
  if ((p - rs->buf) + strlen (filename) * 2 >= get_remote_packet_size ())
    error (_("Remote file name too long for trace save packet"));
  p += 2 * bin2hex ((gdb_byte *) filename, p, strlen (filename));
  *p++ = '\0';
  putpkt (rs->buf);
  reply = remote_get_noisy_reply (&target_buf, &target_buf_size);
  if (*reply == '\0')
    error (_("Target does not support this command."));
  if (strcmp (reply, "OK") != 0)
    error (_("Bogus reply from target: %s"), reply);
  return 0;
}

/* This is basically a memory transfer, but needs to be its own packet
   because we don't know how the target actually organizes its trace
   memory, plus we want to be able to ask for as much as possible, but
   not be unhappy if we don't get as much as we ask for.  */

static LONGEST
remote_get_raw_trace_data (struct target_ops *self,
			   gdb_byte *buf, ULONGEST offset, LONGEST len)
{
  struct remote_state *rs = get_remote_state ();
  char *reply;
  char *p;
  int rslt;

  p = rs->buf;
  strcpy (p, "qTBuffer:");
  p += strlen (p);
  p += hexnumstr (p, offset);
  *p++ = ',';
  p += hexnumstr (p, len);
  *p++ = '\0';

  putpkt (rs->buf);
  reply = remote_get_noisy_reply (&target_buf, &target_buf_size);
  if (reply && *reply)
    {
      /* 'l' by itself means we're at the end of the buffer and
	 there is nothing more to get.  */
      if (*reply == 'l')
	return 0;

      /* Convert the reply into binary.  Limit the number of bytes to
	 convert according to our passed-in buffer size, rather than
	 what was returned in the packet; if the target is
	 unexpectedly generous and gives us a bigger reply than we
	 asked for, we don't want to crash.  */
      rslt = hex2bin (target_buf, buf, len);
      return rslt;
    }

  /* Something went wrong, flag as an error.  */
  return -1;
}

static void
remote_set_disconnected_tracing (struct target_ops *self, int val)
{
  struct remote_state *rs = get_remote_state ();

  if (packet_support (PACKET_DisconnectedTracing_feature) == PACKET_ENABLE)
    {
      char *reply;

      xsnprintf (rs->buf, get_remote_packet_size (), "QTDisconnected:%x", val);
      putpkt (rs->buf);
      reply = remote_get_noisy_reply (&target_buf, &target_buf_size);
      if (*reply == '\0')
	error (_("Target does not support this command."));
      if (strcmp (reply, "OK") != 0)
        error (_("Bogus reply from target: %s"), reply);
    }
  else if (val)
    warning (_("Target does not support disconnected tracing."));
}

static int
remote_core_of_thread (struct target_ops *ops, ptid_t ptid)
{
  struct thread_info *info = find_thread_ptid (ptid);

  if (info && info->priv)
    return info->priv->core;
  return -1;
}

static void
remote_set_circular_trace_buffer (struct target_ops *self, int val)
{
  struct remote_state *rs = get_remote_state ();
  char *reply;

  xsnprintf (rs->buf, get_remote_packet_size (), "QTBuffer:circular:%x", val);
  putpkt (rs->buf);
  reply = remote_get_noisy_reply (&target_buf, &target_buf_size);
  if (*reply == '\0')
    error (_("Target does not support this command."));
  if (strcmp (reply, "OK") != 0)
    error (_("Bogus reply from target: %s"), reply);
}

static struct traceframe_info *
remote_traceframe_info (struct target_ops *self)
{
  char *text;

  text = target_read_stralloc (&current_target,
			       TARGET_OBJECT_TRACEFRAME_INFO, NULL);
  if (text != NULL)
    {
      struct traceframe_info *info;
      struct cleanup *back_to = make_cleanup (xfree, text);

      info = parse_traceframe_info (text);
      do_cleanups (back_to);
      return info;
    }

  return NULL;
}

/* Handle the qTMinFTPILen packet.  Returns the minimum length of
   instruction on which a fast tracepoint may be placed.  Returns -1
   if the packet is not supported, and 0 if the minimum instruction
   length is unknown.  */

static int
remote_get_min_fast_tracepoint_insn_len (struct target_ops *self)
{
  struct remote_state *rs = get_remote_state ();
  char *reply;

  /* If we're not debugging a process yet, the IPA can't be
     loaded.  */
  if (!target_has_execution)
    return 0;

  /* Make sure the remote is pointing at the right process.  */
  set_general_process ();

  xsnprintf (rs->buf, get_remote_packet_size (), "qTMinFTPILen");
  putpkt (rs->buf);
  reply = remote_get_noisy_reply (&target_buf, &target_buf_size);
  if (*reply == '\0')
    return -1;
  else
    {
      ULONGEST min_insn_len;

      unpack_varlen_hex (reply, &min_insn_len);

      return (int) min_insn_len;
    }
}

static void
remote_set_trace_buffer_size (struct target_ops *self, LONGEST val)
{
  if (packet_support (PACKET_QTBuffer_size) != PACKET_DISABLE)
    {
      struct remote_state *rs = get_remote_state ();
      char *buf = rs->buf;
      char *endbuf = rs->buf + get_remote_packet_size ();
      enum packet_result result;

      gdb_assert (val >= 0 || val == -1);
      buf += xsnprintf (buf, endbuf - buf, "QTBuffer:size:");
      /* Send -1 as literal "-1" to avoid host size dependency.  */
      if (val < 0)
	{
	  *buf++ = '-';
          buf += hexnumstr (buf, (ULONGEST) -val);
	}
      else
	buf += hexnumstr (buf, (ULONGEST) val);

      putpkt (rs->buf);
      remote_get_noisy_reply (&rs->buf, &rs->buf_size);
      result = packet_ok (rs->buf,
		  &remote_protocol_packets[PACKET_QTBuffer_size]);

      if (result != PACKET_OK)
	warning (_("Bogus reply from target: %s"), rs->buf);
    }
}

static int
remote_set_trace_notes (struct target_ops *self,
			const char *user, const char *notes,
			const char *stop_notes)
{
  struct remote_state *rs = get_remote_state ();
  char *reply;
  char *buf = rs->buf;
  char *endbuf = rs->buf + get_remote_packet_size ();
  int nbytes;

  buf += xsnprintf (buf, endbuf - buf, "QTNotes:");
  if (user)
    {
      buf += xsnprintf (buf, endbuf - buf, "user:");
      nbytes = bin2hex ((gdb_byte *) user, buf, strlen (user));
      buf += 2 * nbytes;
      *buf++ = ';';
    }
  if (notes)
    {
      buf += xsnprintf (buf, endbuf - buf, "notes:");
      nbytes = bin2hex ((gdb_byte *) notes, buf, strlen (notes));
      buf += 2 * nbytes;
      *buf++ = ';';
    }
  if (stop_notes)
    {
      buf += xsnprintf (buf, endbuf - buf, "tstop:");
      nbytes = bin2hex ((gdb_byte *) stop_notes, buf, strlen (stop_notes));
      buf += 2 * nbytes;
      *buf++ = ';';
    }
  /* Ensure the buffer is terminated.  */
  *buf = '\0';

  putpkt (rs->buf);
  reply = remote_get_noisy_reply (&target_buf, &target_buf_size);
  if (*reply == '\0')
    return 0;

  if (strcmp (reply, "OK") != 0)
    error (_("Bogus reply from target: %s"), reply);

  return 1;
}

static int
remote_use_agent (struct target_ops *self, int use)
{
  if (packet_support (PACKET_QAgent) != PACKET_DISABLE)
    {
      struct remote_state *rs = get_remote_state ();

      /* If the stub supports QAgent.  */
      xsnprintf (rs->buf, get_remote_packet_size (), "QAgent:%d", use);
      putpkt (rs->buf);
      getpkt (&rs->buf, &rs->buf_size, 0);

      if (strcmp (rs->buf, "OK") == 0)
	{
	  use_agent = use;
	  return 1;
	}
    }

  return 0;
}

static int
remote_can_use_agent (struct target_ops *self)
{
  return (packet_support (PACKET_QAgent) != PACKET_DISABLE);
}

struct btrace_target_info
{
  /* The ptid of the traced thread.  */
  ptid_t ptid;

  /* The obtained branch trace configuration.  */
  struct btrace_config conf;
};

/* Reset our idea of our target's btrace configuration.  */

static void
remote_btrace_reset (void)
{
  struct remote_state *rs = get_remote_state ();

  memset (&rs->btrace_config, 0, sizeof (rs->btrace_config));
}

/* Check whether the target supports branch tracing.  */

static int
remote_supports_btrace (struct target_ops *self, enum btrace_format format)
{
  if (packet_support (PACKET_Qbtrace_off) != PACKET_ENABLE)
    return 0;
  if (packet_support (PACKET_qXfer_btrace) != PACKET_ENABLE)
    return 0;

  switch (format)
    {
      case BTRACE_FORMAT_NONE:
	return 0;

      case BTRACE_FORMAT_BTS:
	return (packet_support (PACKET_Qbtrace_bts) == PACKET_ENABLE);

      case BTRACE_FORMAT_PT:
	/* The trace is decoded on the host.  Even if our target supports it,
	   we still need to have libipt to decode the trace.  */
#if defined (HAVE_LIBIPT)
	return (packet_support (PACKET_Qbtrace_pt) == PACKET_ENABLE);
#else /* !defined (HAVE_LIBIPT)  */
	return 0;
#endif /* !defined (HAVE_LIBIPT)  */
    }

  internal_error (__FILE__, __LINE__, _("Unknown branch trace format"));
}

/* Synchronize the configuration with the target.  */

static void
btrace_sync_conf (const struct btrace_config *conf)
{
  struct packet_config *packet;
  struct remote_state *rs;
  char *buf, *pos, *endbuf;

  rs = get_remote_state ();
  buf = rs->buf;
  endbuf = buf + get_remote_packet_size ();

  packet = &remote_protocol_packets[PACKET_Qbtrace_conf_bts_size];
  if (packet_config_support (packet) == PACKET_ENABLE
      && conf->bts.size != rs->btrace_config.bts.size)
    {
      pos = buf;
      pos += xsnprintf (pos, endbuf - pos, "%s=0x%x", packet->name,
                        conf->bts.size);

      putpkt (buf);
      getpkt (&buf, &rs->buf_size, 0);

      if (packet_ok (buf, packet) == PACKET_ERROR)
	{
	  if (buf[0] == 'E' && buf[1] == '.')
	    error (_("Failed to configure the BTS buffer size: %s"), buf + 2);
	  else
	    error (_("Failed to configure the BTS buffer size."));
	}

      rs->btrace_config.bts.size = conf->bts.size;
    }

  packet = &remote_protocol_packets[PACKET_Qbtrace_conf_pt_size];
  if (packet_config_support (packet) == PACKET_ENABLE
      && conf->pt.size != rs->btrace_config.pt.size)
    {
      pos = buf;
      pos += xsnprintf (pos, endbuf - pos, "%s=0x%x", packet->name,
                        conf->pt.size);

      putpkt (buf);
      getpkt (&buf, &rs->buf_size, 0);

      if (packet_ok (buf, packet) == PACKET_ERROR)
	{
	  if (buf[0] == 'E' && buf[1] == '.')
	    error (_("Failed to configure the trace buffer size: %s"), buf + 2);
	  else
	    error (_("Failed to configure the trace buffer size."));
	}

      rs->btrace_config.pt.size = conf->pt.size;
    }
}

/* Read the current thread's btrace configuration from the target and
   store it into CONF.  */

static void
btrace_read_config (struct btrace_config *conf)
{
  char *xml;

  xml = target_read_stralloc (&current_target,
			      TARGET_OBJECT_BTRACE_CONF, "");
  if (xml != NULL)
    {
      struct cleanup *cleanup;

      cleanup = make_cleanup (xfree, xml);
      parse_xml_btrace_conf (conf, xml);
      do_cleanups (cleanup);
    }
}

/* Maybe reopen target btrace.  */

static void
remote_btrace_maybe_reopen (void)
{
  struct remote_state *rs = get_remote_state ();
  struct cleanup *cleanup;
  struct thread_info *tp;
  int btrace_target_pushed = 0;
  int warned = 0;

  cleanup = make_cleanup_restore_current_thread ();
  ALL_NON_EXITED_THREADS (tp)
    {
      set_general_thread (tp->ptid);

      memset (&rs->btrace_config, 0x00, sizeof (struct btrace_config));
      btrace_read_config (&rs->btrace_config);

      if (rs->btrace_config.format == BTRACE_FORMAT_NONE)
	continue;

#if !defined (HAVE_LIBIPT)
      if (rs->btrace_config.format == BTRACE_FORMAT_PT)
	{
	  if (!warned)
	    {
	      warned = 1;
	      warning (_("GDB does not support Intel Processor Trace. "
			 "\"record\" will not work in this session."));
	    }

	  continue;
	}
#endif /* !defined (HAVE_LIBIPT) */

      /* Push target, once, but before anything else happens.  This way our
	 changes to the threads will be cleaned up by unpushing the target
	 in case btrace_read_config () throws.  */
      if (!btrace_target_pushed)
	{
	  btrace_target_pushed = 1;
	  record_btrace_push_target ();
	  printf_filtered (_("Target is recording using %s.\n"),
			   btrace_format_string (rs->btrace_config.format));
	}

      tp->btrace.target = XCNEW (struct btrace_target_info);
      tp->btrace.target->ptid = tp->ptid;
      tp->btrace.target->conf = rs->btrace_config;
    }
  do_cleanups (cleanup);
}

/* Enable branch tracing.  */

static struct btrace_target_info *
remote_enable_btrace (struct target_ops *self, ptid_t ptid,
		      const struct btrace_config *conf)
{
  struct btrace_target_info *tinfo = NULL;
  struct packet_config *packet = NULL;
  struct remote_state *rs = get_remote_state ();
  char *buf = rs->buf;
  char *endbuf = rs->buf + get_remote_packet_size ();

  switch (conf->format)
    {
      case BTRACE_FORMAT_BTS:
	packet = &remote_protocol_packets[PACKET_Qbtrace_bts];
	break;

      case BTRACE_FORMAT_PT:
	packet = &remote_protocol_packets[PACKET_Qbtrace_pt];
	break;
    }

  if (packet == NULL || packet_config_support (packet) != PACKET_ENABLE)
    error (_("Target does not support branch tracing."));

  btrace_sync_conf (conf);

  set_general_thread (ptid);

  buf += xsnprintf (buf, endbuf - buf, "%s", packet->name);
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  if (packet_ok (rs->buf, packet) == PACKET_ERROR)
    {
      if (rs->buf[0] == 'E' && rs->buf[1] == '.')
	error (_("Could not enable branch tracing for %s: %s"),
	       target_pid_to_str (ptid), rs->buf + 2);
      else
	error (_("Could not enable branch tracing for %s."),
	       target_pid_to_str (ptid));
    }

  tinfo = XCNEW (struct btrace_target_info);
  tinfo->ptid = ptid;

  /* If we fail to read the configuration, we lose some information, but the
     tracing itself is not impacted.  */
  TRY
    {
      btrace_read_config (&tinfo->conf);
    }
  CATCH (err, RETURN_MASK_ERROR)
    {
      if (err.message != NULL)
	warning ("%s", err.message);
    }
  END_CATCH

  return tinfo;
}

/* Disable branch tracing.  */

static void
remote_disable_btrace (struct target_ops *self,
		       struct btrace_target_info *tinfo)
{
  struct packet_config *packet = &remote_protocol_packets[PACKET_Qbtrace_off];
  struct remote_state *rs = get_remote_state ();
  char *buf = rs->buf;
  char *endbuf = rs->buf + get_remote_packet_size ();

  if (packet_config_support (packet) != PACKET_ENABLE)
    error (_("Target does not support branch tracing."));

  set_general_thread (tinfo->ptid);

  buf += xsnprintf (buf, endbuf - buf, "%s", packet->name);
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  if (packet_ok (rs->buf, packet) == PACKET_ERROR)
    {
      if (rs->buf[0] == 'E' && rs->buf[1] == '.')
	error (_("Could not disable branch tracing for %s: %s"),
	       target_pid_to_str (tinfo->ptid), rs->buf + 2);
      else
	error (_("Could not disable branch tracing for %s."),
	       target_pid_to_str (tinfo->ptid));
    }

  xfree (tinfo);
}

/* Teardown branch tracing.  */

static void
remote_teardown_btrace (struct target_ops *self,
			struct btrace_target_info *tinfo)
{
  /* We must not talk to the target during teardown.  */
  xfree (tinfo);
}

/* Read the branch trace.  */

static enum btrace_error
remote_read_btrace (struct target_ops *self,
		    struct btrace_data *btrace,
		    struct btrace_target_info *tinfo,
		    enum btrace_read_type type)
{
  struct packet_config *packet = &remote_protocol_packets[PACKET_qXfer_btrace];
  struct cleanup *cleanup;
  const char *annex;
  char *xml;

  if (packet_config_support (packet) != PACKET_ENABLE)
    error (_("Target does not support branch tracing."));

#if !defined(HAVE_LIBEXPAT)
  error (_("Cannot process branch tracing result. XML parsing not supported."));
#endif

  switch (type)
    {
    case BTRACE_READ_ALL:
      annex = "all";
      break;
    case BTRACE_READ_NEW:
      annex = "new";
      break;
    case BTRACE_READ_DELTA:
      annex = "delta";
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("Bad branch tracing read type: %u."),
		      (unsigned int) type);
    }

  xml = target_read_stralloc (&current_target,
			      TARGET_OBJECT_BTRACE, annex);
  if (xml == NULL)
    return BTRACE_ERR_UNKNOWN;

  cleanup = make_cleanup (xfree, xml);
  parse_xml_btrace (btrace, xml);
  do_cleanups (cleanup);

  return BTRACE_ERR_NONE;
}

static const struct btrace_config *
remote_btrace_conf (struct target_ops *self,
		    const struct btrace_target_info *tinfo)
{
  return &tinfo->conf;
}

static int
remote_augmented_libraries_svr4_read (struct target_ops *self)
{
  return (packet_support (PACKET_augmented_libraries_svr4_read_feature)
	  == PACKET_ENABLE);
}

/* Implementation of to_load.  */

static void
remote_load (struct target_ops *self, const char *name, int from_tty)
{
  generic_load (name, from_tty);
}

/* Accepts an integer PID; returns a string representing a file that
   can be opened on the remote side to get the symbols for the child
   process.  Returns NULL if the operation is not supported.  */

static char *
remote_pid_to_exec_file (struct target_ops *self, int pid)
{
  static char *filename = NULL;
  struct inferior *inf;
  char *annex = NULL;

  if (packet_support (PACKET_qXfer_exec_file) != PACKET_ENABLE)
    return NULL;

  if (filename != NULL)
    xfree (filename);

  inf = find_inferior_pid (pid);
  if (inf == NULL)
    internal_error (__FILE__, __LINE__,
		    _("not currently attached to process %d"), pid);

  if (!inf->fake_pid_p)
    {
      const int annex_size = 9;

      annex = (char *) alloca (annex_size);
      xsnprintf (annex, annex_size, "%x", pid);
    }

  filename = target_read_stralloc (&current_target,
				   TARGET_OBJECT_EXEC_FILE, annex);

  return filename;
}

/* Implement the to_can_do_single_step target_ops method.  */

static int
remote_can_do_single_step (struct target_ops *ops)
{
  /* We can only tell whether target supports single step or not by
     supported s and S vCont actions if the stub supports vContSupported
     feature.  If the stub doesn't support vContSupported feature,
     we have conservatively to think target doesn't supports single
     step.  */
  if (packet_support (PACKET_vContSupported) == PACKET_ENABLE)
    {
      struct remote_state *rs = get_remote_state ();

      if (packet_support (PACKET_vCont) == PACKET_SUPPORT_UNKNOWN)
	remote_vcont_probe (rs);

      return rs->supports_vCont.s && rs->supports_vCont.S;
    }
  else
    return 0;
}

/* Implementation of the to_execution_direction method for the remote
   target.  */

static enum exec_direction_kind
remote_execution_direction (struct target_ops *self)
{
  struct remote_state *rs = get_remote_state ();

  return rs->last_resume_exec_dir;
}

static void
init_remote_ops (void)
{
  remote_ops.to_shortname = "remote";
  remote_ops.to_longname = "Remote serial target in gdb-specific protocol";
  remote_ops.to_doc =
    "Use a remote computer via a serial line, using a gdb-specific protocol.\n\
Specify the serial device it is connected to\n\
(e.g. /dev/ttyS0, /dev/ttya, COM1, etc.).";
  remote_ops.to_open = remote_open;
  remote_ops.to_close = remote_close;
  remote_ops.to_detach = remote_detach;
  remote_ops.to_disconnect = remote_disconnect;
  remote_ops.to_resume = remote_resume;
  remote_ops.to_wait = remote_wait;
  remote_ops.to_fetch_registers = remote_fetch_registers;
  remote_ops.to_store_registers = remote_store_registers;
  remote_ops.to_prepare_to_store = remote_prepare_to_store;
  remote_ops.to_files_info = remote_files_info;
  remote_ops.to_insert_breakpoint = remote_insert_breakpoint;
  remote_ops.to_remove_breakpoint = remote_remove_breakpoint;
  remote_ops.to_stopped_by_sw_breakpoint = remote_stopped_by_sw_breakpoint;
  remote_ops.to_supports_stopped_by_sw_breakpoint = remote_supports_stopped_by_sw_breakpoint;
  remote_ops.to_stopped_by_hw_breakpoint = remote_stopped_by_hw_breakpoint;
  remote_ops.to_supports_stopped_by_hw_breakpoint = remote_supports_stopped_by_hw_breakpoint;
  remote_ops.to_stopped_by_watchpoint = remote_stopped_by_watchpoint;
  remote_ops.to_stopped_data_address = remote_stopped_data_address;
  remote_ops.to_watchpoint_addr_within_range =
    remote_watchpoint_addr_within_range;
  remote_ops.to_can_use_hw_breakpoint = remote_check_watch_resources;
  remote_ops.to_insert_hw_breakpoint = remote_insert_hw_breakpoint;
  remote_ops.to_remove_hw_breakpoint = remote_remove_hw_breakpoint;
  remote_ops.to_region_ok_for_hw_watchpoint
     = remote_region_ok_for_hw_watchpoint;
  remote_ops.to_insert_watchpoint = remote_insert_watchpoint;
  remote_ops.to_remove_watchpoint = remote_remove_watchpoint;
  remote_ops.to_kill = remote_kill;
  remote_ops.to_load = remote_load;
  remote_ops.to_mourn_inferior = remote_mourn;
  remote_ops.to_pass_signals = remote_pass_signals;
  remote_ops.to_set_syscall_catchpoint = remote_set_syscall_catchpoint;
  remote_ops.to_program_signals = remote_program_signals;
  remote_ops.to_thread_alive = remote_thread_alive;
  remote_ops.to_thread_name = remote_thread_name;
  remote_ops.to_update_thread_list = remote_update_thread_list;
  remote_ops.to_pid_to_str = remote_pid_to_str;
  remote_ops.to_extra_thread_info = remote_threads_extra_info;
  remote_ops.to_get_ada_task_ptid = remote_get_ada_task_ptid;
  remote_ops.to_stop = remote_stop;
  remote_ops.to_interrupt = remote_interrupt;
  remote_ops.to_pass_ctrlc = remote_pass_ctrlc;
  remote_ops.to_xfer_partial = remote_xfer_partial;
  remote_ops.to_get_memory_xfer_limit = remote_get_memory_xfer_limit;
  remote_ops.to_rcmd = remote_rcmd;
  remote_ops.to_pid_to_exec_file = remote_pid_to_exec_file;
  remote_ops.to_log_command = serial_log_command;
  remote_ops.to_get_thread_local_address = remote_get_thread_local_address;
  remote_ops.to_stratum = process_stratum;
  remote_ops.to_has_all_memory = default_child_has_all_memory;
  remote_ops.to_has_memory = default_child_has_memory;
  remote_ops.to_has_stack = default_child_has_stack;
  remote_ops.to_has_registers = default_child_has_registers;
  remote_ops.to_has_execution = default_child_has_execution;
  remote_ops.to_has_thread_control = tc_schedlock;    /* can lock scheduler */
  remote_ops.to_can_execute_reverse = remote_can_execute_reverse;
  remote_ops.to_magic = OPS_MAGIC;
  remote_ops.to_memory_map = remote_memory_map;
  remote_ops.to_flash_erase = remote_flash_erase;
  remote_ops.to_flash_done = remote_flash_done;
  remote_ops.to_read_description = remote_read_description;
  remote_ops.to_search_memory = remote_search_memory;
  remote_ops.to_can_async_p = remote_can_async_p;
  remote_ops.to_is_async_p = remote_is_async_p;
  remote_ops.to_async = remote_async;
  remote_ops.to_thread_events = remote_thread_events;
  remote_ops.to_can_do_single_step = remote_can_do_single_step;
  remote_ops.to_terminal_inferior = remote_terminal_inferior;
  remote_ops.to_terminal_ours = remote_terminal_ours;
  remote_ops.to_supports_non_stop = remote_supports_non_stop;
  remote_ops.to_supports_multi_process = remote_supports_multi_process;
  remote_ops.to_supports_disable_randomization
    = remote_supports_disable_randomization;
  remote_ops.to_filesystem_is_local = remote_filesystem_is_local;
  remote_ops.to_fileio_open = remote_hostio_open;
  remote_ops.to_fileio_pwrite = remote_hostio_pwrite;
  remote_ops.to_fileio_pread = remote_hostio_pread;
  remote_ops.to_fileio_fstat = remote_hostio_fstat;
  remote_ops.to_fileio_close = remote_hostio_close;
  remote_ops.to_fileio_unlink = remote_hostio_unlink;
  remote_ops.to_fileio_readlink = remote_hostio_readlink;
  remote_ops.to_supports_enable_disable_tracepoint = remote_supports_enable_disable_tracepoint;
  remote_ops.to_supports_string_tracing = remote_supports_string_tracing;
  remote_ops.to_supports_evaluation_of_breakpoint_conditions = remote_supports_cond_breakpoints;
  remote_ops.to_can_run_breakpoint_commands = remote_can_run_breakpoint_commands;
  remote_ops.to_trace_init = remote_trace_init;
  remote_ops.to_download_tracepoint = remote_download_tracepoint;
  remote_ops.to_can_download_tracepoint = remote_can_download_tracepoint;
  remote_ops.to_download_trace_state_variable
    = remote_download_trace_state_variable;
  remote_ops.to_enable_tracepoint = remote_enable_tracepoint;
  remote_ops.to_disable_tracepoint = remote_disable_tracepoint;
  remote_ops.to_trace_set_readonly_regions = remote_trace_set_readonly_regions;
  remote_ops.to_trace_start = remote_trace_start;
  remote_ops.to_get_trace_status = remote_get_trace_status;
  remote_ops.to_get_tracepoint_status = remote_get_tracepoint_status;
  remote_ops.to_trace_stop = remote_trace_stop;
  remote_ops.to_trace_find = remote_trace_find;
  remote_ops.to_get_trace_state_variable_value
    = remote_get_trace_state_variable_value;
  remote_ops.to_save_trace_data = remote_save_trace_data;
  remote_ops.to_upload_tracepoints = remote_upload_tracepoints;
  remote_ops.to_upload_trace_state_variables
    = remote_upload_trace_state_variables;
  remote_ops.to_get_raw_trace_data = remote_get_raw_trace_data;
  remote_ops.to_get_min_fast_tracepoint_insn_len = remote_get_min_fast_tracepoint_insn_len;
  remote_ops.to_set_disconnected_tracing = remote_set_disconnected_tracing;
  remote_ops.to_set_circular_trace_buffer = remote_set_circular_trace_buffer;
  remote_ops.to_set_trace_buffer_size = remote_set_trace_buffer_size;
  remote_ops.to_set_trace_notes = remote_set_trace_notes;
  remote_ops.to_core_of_thread = remote_core_of_thread;
  remote_ops.to_verify_memory = remote_verify_memory;
  remote_ops.to_get_tib_address = remote_get_tib_address;
  remote_ops.to_set_permissions = remote_set_permissions;
  remote_ops.to_static_tracepoint_marker_at
    = remote_static_tracepoint_marker_at;
  remote_ops.to_static_tracepoint_markers_by_strid
    = remote_static_tracepoint_markers_by_strid;
  remote_ops.to_traceframe_info = remote_traceframe_info;
  remote_ops.to_use_agent = remote_use_agent;
  remote_ops.to_can_use_agent = remote_can_use_agent;
  remote_ops.to_supports_btrace = remote_supports_btrace;
  remote_ops.to_enable_btrace = remote_enable_btrace;
  remote_ops.to_disable_btrace = remote_disable_btrace;
  remote_ops.to_teardown_btrace = remote_teardown_btrace;
  remote_ops.to_read_btrace = remote_read_btrace;
  remote_ops.to_btrace_conf = remote_btrace_conf;
  remote_ops.to_augmented_libraries_svr4_read =
    remote_augmented_libraries_svr4_read;
  remote_ops.to_follow_fork = remote_follow_fork;
  remote_ops.to_follow_exec = remote_follow_exec;
  remote_ops.to_insert_fork_catchpoint = remote_insert_fork_catchpoint;
  remote_ops.to_remove_fork_catchpoint = remote_remove_fork_catchpoint;
  remote_ops.to_insert_vfork_catchpoint = remote_insert_vfork_catchpoint;
  remote_ops.to_remove_vfork_catchpoint = remote_remove_vfork_catchpoint;
  remote_ops.to_insert_exec_catchpoint = remote_insert_exec_catchpoint;
  remote_ops.to_remove_exec_catchpoint = remote_remove_exec_catchpoint;
  remote_ops.to_execution_direction = remote_execution_direction;
}

/* Set up the extended remote vector by making a copy of the standard
   remote vector and adding to it.  */

static void
init_extended_remote_ops (void)
{
  extended_remote_ops = remote_ops;

  extended_remote_ops.to_shortname = "extended-remote";
  extended_remote_ops.to_longname =
    "Extended remote serial target in gdb-specific protocol";
  extended_remote_ops.to_doc =
    "Use a remote computer via a serial line, using a gdb-specific protocol.\n\
Specify the serial device it is connected to (e.g. /dev/ttya).";
  extended_remote_ops.to_open = extended_remote_open;
  extended_remote_ops.to_create_inferior = extended_remote_create_inferior;
  extended_remote_ops.to_detach = extended_remote_detach;
  extended_remote_ops.to_attach = extended_remote_attach;
  extended_remote_ops.to_post_attach = extended_remote_post_attach;
  extended_remote_ops.to_supports_disable_randomization
    = extended_remote_supports_disable_randomization;
}

static int
remote_can_async_p (struct target_ops *ops)
{
  struct remote_state *rs = get_remote_state ();

  if (!target_async_permitted)
    /* We only enable async when the user specifically asks for it.  */
    return 0;

  /* We're async whenever the serial device is.  */
  return serial_can_async_p (rs->remote_desc);
}

static int
remote_is_async_p (struct target_ops *ops)
{
  struct remote_state *rs = get_remote_state ();

  if (!target_async_permitted)
    /* We only enable async when the user specifically asks for it.  */
    return 0;

  /* We're async whenever the serial device is.  */
  return serial_is_async_p (rs->remote_desc);
}

/* Pass the SERIAL event on and up to the client.  One day this code
   will be able to delay notifying the client of an event until the
   point where an entire packet has been received.  */

static serial_event_ftype remote_async_serial_handler;

static void
remote_async_serial_handler (struct serial *scb, void *context)
{
  /* Don't propogate error information up to the client.  Instead let
     the client find out about the error by querying the target.  */
  inferior_event_handler (INF_REG_EVENT, NULL);
}

static void
remote_async_inferior_event_handler (gdb_client_data data)
{
  inferior_event_handler (INF_REG_EVENT, NULL);
}

static void
remote_async (struct target_ops *ops, int enable)
{
  struct remote_state *rs = get_remote_state ();

  if (enable)
    {
      serial_async (rs->remote_desc, remote_async_serial_handler, rs);

      /* If there are pending events in the stop reply queue tell the
	 event loop to process them.  */
      if (!QUEUE_is_empty (stop_reply_p, stop_reply_queue))
	mark_async_event_handler (remote_async_inferior_event_token);
      /* For simplicity, below we clear the pending events token
	 without remembering whether it is marked, so here we always
	 mark it.  If there's actually no pending notification to
	 process, this ends up being a no-op (other than a spurious
	 event-loop wakeup).  */
      if (target_is_non_stop_p ())
	mark_async_event_handler (rs->notif_state->get_pending_events_token);
    }
  else
    {
      serial_async (rs->remote_desc, NULL, NULL);
      /* If the core is disabling async, it doesn't want to be
	 disturbed with target events.  Clear all async event sources
	 too.  */
      clear_async_event_handler (remote_async_inferior_event_token);
      if (target_is_non_stop_p ())
	clear_async_event_handler (rs->notif_state->get_pending_events_token);
    }
}

/* Implementation of the to_thread_events method.  */

static void
remote_thread_events (struct target_ops *ops, int enable)
{
  struct remote_state *rs = get_remote_state ();
  size_t size = get_remote_packet_size ();

  if (packet_support (PACKET_QThreadEvents) == PACKET_DISABLE)
    return;

  xsnprintf (rs->buf, size, "QThreadEvents:%x", enable ? 1 : 0);
  putpkt (rs->buf);
  getpkt (&rs->buf, &rs->buf_size, 0);

  switch (packet_ok (rs->buf,
		     &remote_protocol_packets[PACKET_QThreadEvents]))
    {
    case PACKET_OK:
      if (strcmp (rs->buf, "OK") != 0)
	error (_("Remote refused setting thread events: %s"), rs->buf);
      break;
    case PACKET_ERROR:
      warning (_("Remote failure reply: %s"), rs->buf);
      break;
    case PACKET_UNKNOWN:
      break;
    }
}

static void
set_remote_cmd (char *args, int from_tty)
{
  help_list (remote_set_cmdlist, "set remote ", all_commands, gdb_stdout);
}

static void
show_remote_cmd (char *args, int from_tty)
{
  /* We can't just use cmd_show_list here, because we want to skip
     the redundant "show remote Z-packet" and the legacy aliases.  */
  struct cleanup *showlist_chain;
  struct cmd_list_element *list = remote_show_cmdlist;
  struct ui_out *uiout = current_uiout;

  showlist_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "showlist");
  for (; list != NULL; list = list->next)
    if (strcmp (list->name, "Z-packet") == 0)
      continue;
    else if (list->type == not_set_cmd)
      /* Alias commands are exactly like the original, except they
	 don't have the normal type.  */
      continue;
    else
      {
	struct cleanup *option_chain
	  = make_cleanup_ui_out_tuple_begin_end (uiout, "option");

	ui_out_field_string (uiout, "name", list->name);
	ui_out_text (uiout, ":  ");
	if (list->type == show_cmd)
	  do_show_command ((char *) NULL, from_tty, list);
	else
	  cmd_func (list, NULL, from_tty);
	/* Close the tuple.  */
	do_cleanups (option_chain);
      }

  /* Close the tuple.  */
  do_cleanups (showlist_chain);
}


/* Function to be called whenever a new objfile (shlib) is detected.  */
static void
remote_new_objfile (struct objfile *objfile)
{
  struct remote_state *rs = get_remote_state ();

  if (rs->remote_desc != 0)		/* Have a remote connection.  */
    remote_check_symbols ();
}

/* Pull all the tracepoints defined on the target and create local
   data structures representing them.  We don't want to create real
   tracepoints yet, we don't want to mess up the user's existing
   collection.  */
  
static int
remote_upload_tracepoints (struct target_ops *self, struct uploaded_tp **utpp)
{
  struct remote_state *rs = get_remote_state ();
  char *p;

  /* Ask for a first packet of tracepoint definition.  */
  putpkt ("qTfP");
  getpkt (&rs->buf, &rs->buf_size, 0);
  p = rs->buf;
  while (*p && *p != 'l')
    {
      parse_tracepoint_definition (p, utpp);
      /* Ask for another packet of tracepoint definition.  */
      putpkt ("qTsP");
      getpkt (&rs->buf, &rs->buf_size, 0);
      p = rs->buf;
    }
  return 0;
}

static int
remote_upload_trace_state_variables (struct target_ops *self,
				     struct uploaded_tsv **utsvp)
{
  struct remote_state *rs = get_remote_state ();
  char *p;

  /* Ask for a first packet of variable definition.  */
  putpkt ("qTfV");
  getpkt (&rs->buf, &rs->buf_size, 0);
  p = rs->buf;
  while (*p && *p != 'l')
    {
      parse_tsv_definition (p, utsvp);
      /* Ask for another packet of variable definition.  */
      putpkt ("qTsV");
      getpkt (&rs->buf, &rs->buf_size, 0);
      p = rs->buf;
    }
  return 0;
}

/* The "set/show range-stepping" show hook.  */

static void
show_range_stepping (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c,
		     const char *value)
{
  fprintf_filtered (file,
		    _("Debugger's willingness to use range stepping "
		      "is %s.\n"), value);
}

/* The "set/show range-stepping" set hook.  */

static void
set_range_stepping (char *ignore_args, int from_tty,
		    struct cmd_list_element *c)
{
  struct remote_state *rs = get_remote_state ();

  /* Whene enabling, check whether range stepping is actually
     supported by the target, and warn if not.  */
  if (use_range_stepping)
    {
      if (rs->remote_desc != NULL)
	{
	  if (packet_support (PACKET_vCont) == PACKET_SUPPORT_UNKNOWN)
	    remote_vcont_probe (rs);

	  if (packet_support (PACKET_vCont) == PACKET_ENABLE
	      && rs->supports_vCont.r)
	    return;
	}

      warning (_("Range stepping is not supported by the current target"));
    }
}

void
_initialize_remote (void)
{
  struct cmd_list_element *cmd;
  const char *cmd_name;

  /* architecture specific data */
  remote_gdbarch_data_handle =
    gdbarch_data_register_post_init (init_remote_state);
  remote_g_packet_data_handle =
    gdbarch_data_register_pre_init (remote_g_packet_data_init);

  remote_pspace_data
    = register_program_space_data_with_cleanup (NULL,
						remote_pspace_data_cleanup);

  /* Initialize the per-target state.  At the moment there is only one
     of these, not one per target.  Only one target is active at a
     time.  */
  remote_state = new_remote_state ();

  init_remote_ops ();
  add_target (&remote_ops);

  init_extended_remote_ops ();
  add_target (&extended_remote_ops);

  /* Hook into new objfile notification.  */
  observer_attach_new_objfile (remote_new_objfile);
  /* We're no longer interested in notification events of an inferior
     when it exits.  */
  observer_attach_inferior_exit (discard_pending_stop_replies);

#if 0
  init_remote_threadtests ();
#endif

  stop_reply_queue = QUEUE_alloc (stop_reply_p, stop_reply_xfree);
  /* set/show remote ...  */

  add_prefix_cmd ("remote", class_maintenance, set_remote_cmd, _("\
Remote protocol specific variables\n\
Configure various remote-protocol specific variables such as\n\
the packets being used"),
		  &remote_set_cmdlist, "set remote ",
		  0 /* allow-unknown */, &setlist);
  add_prefix_cmd ("remote", class_maintenance, show_remote_cmd, _("\
Remote protocol specific variables\n\
Configure various remote-protocol specific variables such as\n\
the packets being used"),
		  &remote_show_cmdlist, "show remote ",
		  0 /* allow-unknown */, &showlist);

  add_cmd ("compare-sections", class_obscure, compare_sections_command, _("\
Compare section data on target to the exec file.\n\
Argument is a single section name (default: all loaded sections).\n\
To compare only read-only loaded sections, specify the -r option."),
	   &cmdlist);

  add_cmd ("packet", class_maintenance, packet_command, _("\
Send an arbitrary packet to a remote target.\n\
   maintenance packet TEXT\n\
If GDB is talking to an inferior via the GDB serial protocol, then\n\
this command sends the string TEXT to the inferior, and displays the\n\
response packet.  GDB supplies the initial `$' character, and the\n\
terminating `#' character and checksum."),
	   &maintenancelist);

  add_setshow_boolean_cmd ("remotebreak", no_class, &remote_break, _("\
Set whether to send break if interrupted."), _("\
Show whether to send break if interrupted."), _("\
If set, a break, instead of a cntrl-c, is sent to the remote target."),
			   set_remotebreak, show_remotebreak,
			   &setlist, &showlist);
  cmd_name = "remotebreak";
  cmd = lookup_cmd (&cmd_name, setlist, "", -1, 1);
  deprecate_cmd (cmd, "set remote interrupt-sequence");
  cmd_name = "remotebreak"; /* needed because lookup_cmd updates the pointer */
  cmd = lookup_cmd (&cmd_name, showlist, "", -1, 1);
  deprecate_cmd (cmd, "show remote interrupt-sequence");

  add_setshow_enum_cmd ("interrupt-sequence", class_support,
			interrupt_sequence_modes, &interrupt_sequence_mode,
			_("\
Set interrupt sequence to remote target."), _("\
Show interrupt sequence to remote target."), _("\
Valid value is \"Ctrl-C\", \"BREAK\" or \"BREAK-g\". The default is \"Ctrl-C\"."),
			NULL, show_interrupt_sequence,
			&remote_set_cmdlist,
			&remote_show_cmdlist);

  add_setshow_boolean_cmd ("interrupt-on-connect", class_support,
			   &interrupt_on_connect, _("\
Set whether interrupt-sequence is sent to remote target when gdb connects to."), _("		\
Show whether interrupt-sequence is sent to remote target when gdb connects to."), _("		\
If set, interrupt sequence is sent to remote target."),
			   NULL, NULL,
			   &remote_set_cmdlist, &remote_show_cmdlist);

  /* Install commands for configuring memory read/write packets.  */

  add_cmd ("remotewritesize", no_class, set_memory_write_packet_size, _("\
Set the maximum number of bytes per memory write packet (deprecated)."),
	   &setlist);
  add_cmd ("remotewritesize", no_class, show_memory_write_packet_size, _("\
Show the maximum number of bytes per memory write packet (deprecated)."),
	   &showlist);
  add_cmd ("memory-write-packet-size", no_class,
	   set_memory_write_packet_size, _("\
Set the maximum number of bytes per memory-write packet.\n\
Specify the number of bytes in a packet or 0 (zero) for the\n\
default packet size.  The actual limit is further reduced\n\
dependent on the target.  Specify ``fixed'' to disable the\n\
further restriction and ``limit'' to enable that restriction."),
	   &remote_set_cmdlist);
  add_cmd ("memory-read-packet-size", no_class,
	   set_memory_read_packet_size, _("\
Set the maximum number of bytes per memory-read packet.\n\
Specify the number of bytes in a packet or 0 (zero) for the\n\
default packet size.  The actual limit is further reduced\n\
dependent on the target.  Specify ``fixed'' to disable the\n\
further restriction and ``limit'' to enable that restriction."),
	   &remote_set_cmdlist);
  add_cmd ("memory-write-packet-size", no_class,
	   show_memory_write_packet_size,
	   _("Show the maximum number of bytes per memory-write packet."),
	   &remote_show_cmdlist);
  add_cmd ("memory-read-packet-size", no_class,
	   show_memory_read_packet_size,
	   _("Show the maximum number of bytes per memory-read packet."),
	   &remote_show_cmdlist);

  add_setshow_zinteger_cmd ("hardware-watchpoint-limit", no_class,
			    &remote_hw_watchpoint_limit, _("\
Set the maximum number of target hardware watchpoints."), _("\
Show the maximum number of target hardware watchpoints."), _("\
Specify a negative limit for unlimited."),
			    NULL, NULL, /* FIXME: i18n: The maximum
					   number of target hardware
					   watchpoints is %s.  */
			    &remote_set_cmdlist, &remote_show_cmdlist);
  add_setshow_zinteger_cmd ("hardware-watchpoint-length-limit", no_class,
			    &remote_hw_watchpoint_length_limit, _("\
Set the maximum length (in bytes) of a target hardware watchpoint."), _("\
Show the maximum length (in bytes) of a target hardware watchpoint."), _("\
Specify a negative limit for unlimited."),
			    NULL, NULL, /* FIXME: i18n: The maximum
                                           length (in bytes) of a target
                                           hardware watchpoint is %s.  */
			    &remote_set_cmdlist, &remote_show_cmdlist);
  add_setshow_zinteger_cmd ("hardware-breakpoint-limit", no_class,
			    &remote_hw_breakpoint_limit, _("\
Set the maximum number of target hardware breakpoints."), _("\
Show the maximum number of target hardware breakpoints."), _("\
Specify a negative limit for unlimited."),
			    NULL, NULL, /* FIXME: i18n: The maximum
					   number of target hardware
					   breakpoints is %s.  */
			    &remote_set_cmdlist, &remote_show_cmdlist);

  add_setshow_zuinteger_cmd ("remoteaddresssize", class_obscure,
			     &remote_address_size, _("\
Set the maximum size of the address (in bits) in a memory packet."), _("\
Show the maximum size of the address (in bits) in a memory packet."), NULL,
			     NULL,
			     NULL, /* FIXME: i18n: */
			     &setlist, &showlist);

  init_all_packet_configs ();

  add_packet_config_cmd (&remote_protocol_packets[PACKET_X],
			 "X", "binary-download", 1);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vCont],
			 "vCont", "verbose-resume", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_QPassSignals],
			 "QPassSignals", "pass-signals", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_QCatchSyscalls],
			 "QCatchSyscalls", "catch-syscalls", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_QProgramSignals],
			 "QProgramSignals", "program-signals", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qSymbol],
			 "qSymbol", "symbol-lookup", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_P],
			 "P", "set-register", 1);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_p],
			 "p", "fetch-register", 1);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_Z0],
			 "Z0", "software-breakpoint", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_Z1],
			 "Z1", "hardware-breakpoint", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_Z2],
			 "Z2", "write-watchpoint", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_Z3],
			 "Z3", "read-watchpoint", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_Z4],
			 "Z4", "access-watchpoint", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_auxv],
			 "qXfer:auxv:read", "read-aux-vector", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_exec_file],
			 "qXfer:exec-file:read", "pid-to-exec-file", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_features],
			 "qXfer:features:read", "target-features", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_libraries],
			 "qXfer:libraries:read", "library-info", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_libraries_svr4],
			 "qXfer:libraries-svr4:read", "library-info-svr4", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_memory_map],
			 "qXfer:memory-map:read", "memory-map", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_spu_read],
                         "qXfer:spu:read", "read-spu-object", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_spu_write],
                         "qXfer:spu:write", "write-spu-object", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_osdata],
                        "qXfer:osdata:read", "osdata", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_threads],
			 "qXfer:threads:read", "threads", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_siginfo_read],
                         "qXfer:siginfo:read", "read-siginfo-object", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_siginfo_write],
                         "qXfer:siginfo:write", "write-siginfo-object", 0);

  add_packet_config_cmd
    (&remote_protocol_packets[PACKET_qXfer_traceframe_info],
     "qXfer:traceframe-info:read", "traceframe-info", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_uib],
			 "qXfer:uib:read", "unwind-info-block", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qGetTLSAddr],
			 "qGetTLSAddr", "get-thread-local-storage-address",
			 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qGetTIBAddr],
			 "qGetTIBAddr", "get-thread-information-block-address",
			 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_bc],
			 "bc", "reverse-continue", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_bs],
			 "bs", "reverse-step", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qSupported],
			 "qSupported", "supported-packets", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qSearch_memory],
			 "qSearch:memory", "search-memory", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qTStatus],
			 "qTStatus", "trace-status", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vFile_setfs],
			 "vFile:setfs", "hostio-setfs", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vFile_open],
			 "vFile:open", "hostio-open", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vFile_pread],
			 "vFile:pread", "hostio-pread", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vFile_pwrite],
			 "vFile:pwrite", "hostio-pwrite", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vFile_close],
			 "vFile:close", "hostio-close", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vFile_unlink],
			 "vFile:unlink", "hostio-unlink", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vFile_readlink],
			 "vFile:readlink", "hostio-readlink", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vFile_fstat],
			 "vFile:fstat", "hostio-fstat", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vAttach],
			 "vAttach", "attach", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vRun],
			 "vRun", "run", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_QStartNoAckMode],
			 "QStartNoAckMode", "noack", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vKill],
			 "vKill", "kill", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qAttached],
			 "qAttached", "query-attached", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_ConditionalTracepoints],
			 "ConditionalTracepoints",
			 "conditional-tracepoints", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_ConditionalBreakpoints],
			 "ConditionalBreakpoints",
			 "conditional-breakpoints", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_BreakpointCommands],
			 "BreakpointCommands",
			 "breakpoint-commands", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_FastTracepoints],
			 "FastTracepoints", "fast-tracepoints", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_TracepointSource],
			 "TracepointSource", "TracepointSource", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_QAllow],
			 "QAllow", "allow", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_StaticTracepoints],
			 "StaticTracepoints", "static-tracepoints", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_InstallInTrace],
			 "InstallInTrace", "install-in-trace", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_statictrace_read],
                         "qXfer:statictrace:read", "read-sdata-object", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_fdpic],
			 "qXfer:fdpic:read", "read-fdpic-loadmap", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_QDisableRandomization],
			 "QDisableRandomization", "disable-randomization", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_QAgent],
			 "QAgent", "agent", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_QTBuffer_size],
			 "QTBuffer:size", "trace-buffer-size", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_Qbtrace_off],
       "Qbtrace:off", "disable-btrace", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_Qbtrace_bts],
       "Qbtrace:bts", "enable-btrace-bts", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_Qbtrace_pt],
       "Qbtrace:pt", "enable-btrace-pt", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_btrace],
       "qXfer:btrace", "read-btrace", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_qXfer_btrace_conf],
       "qXfer:btrace-conf", "read-btrace-conf", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_Qbtrace_conf_bts_size],
       "Qbtrace-conf:bts:size", "btrace-conf-bts-size", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_multiprocess_feature],
       "multiprocess-feature", "multiprocess-feature", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_swbreak_feature],
                         "swbreak-feature", "swbreak-feature", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_hwbreak_feature],
                         "hwbreak-feature", "hwbreak-feature", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_fork_event_feature],
			 "fork-event-feature", "fork-event-feature", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vfork_event_feature],
			 "vfork-event-feature", "vfork-event-feature", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_Qbtrace_conf_pt_size],
       "Qbtrace-conf:pt:size", "btrace-conf-pt-size", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vContSupported],
			 "vContSupported", "verbose-resume-supported", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_exec_event_feature],
			 "exec-event-feature", "exec-event-feature", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_vCtrlC],
			 "vCtrlC", "ctrl-c", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_QThreadEvents],
			 "QThreadEvents", "thread-events", 0);

  add_packet_config_cmd (&remote_protocol_packets[PACKET_no_resumed],
			 "N stop reply", "no-resumed-stop-reply", 0);

  /* Assert that we've registered "set remote foo-packet" commands
     for all packet configs.  */
  {
    int i;

    for (i = 0; i < PACKET_MAX; i++)
      {
	/* Ideally all configs would have a command associated.  Some
	   still don't though.  */
	int excepted;

	switch (i)
	  {
	  case PACKET_QNonStop:
	  case PACKET_EnableDisableTracepoints_feature:
	  case PACKET_tracenz_feature:
	  case PACKET_DisconnectedTracing_feature:
	  case PACKET_augmented_libraries_svr4_read_feature:
	  case PACKET_qCRC:
	    /* Additions to this list need to be well justified:
	       pre-existing packets are OK; new packets are not.  */
	    excepted = 1;
	    break;
	  default:
	    excepted = 0;
	    break;
	  }

	/* This catches both forgetting to add a config command, and
	   forgetting to remove a packet from the exception list.  */
	gdb_assert (excepted == (remote_protocol_packets[i].name == NULL));
      }
  }

  /* Keep the old ``set remote Z-packet ...'' working.  Each individual
     Z sub-packet has its own set and show commands, but users may
     have sets to this variable in their .gdbinit files (or in their
     documentation).  */
  add_setshow_auto_boolean_cmd ("Z-packet", class_obscure,
				&remote_Z_packet_detect, _("\
Set use of remote protocol `Z' packets"), _("\
Show use of remote protocol `Z' packets "), _("\
When set, GDB will attempt to use the remote breakpoint and watchpoint\n\
packets."),
				set_remote_protocol_Z_packet_cmd,
				show_remote_protocol_Z_packet_cmd,
				/* FIXME: i18n: Use of remote protocol
				   `Z' packets is %s.  */
				&remote_set_cmdlist, &remote_show_cmdlist);

  add_prefix_cmd ("remote", class_files, remote_command, _("\
Manipulate files on the remote system\n\
Transfer files to and from the remote target system."),
		  &remote_cmdlist, "remote ",
		  0 /* allow-unknown */, &cmdlist);

  add_cmd ("put", class_files, remote_put_command,
	   _("Copy a local file to the remote system."),
	   &remote_cmdlist);

  add_cmd ("get", class_files, remote_get_command,
	   _("Copy a remote file to the local system."),
	   &remote_cmdlist);

  add_cmd ("delete", class_files, remote_delete_command,
	   _("Delete a remote file."),
	   &remote_cmdlist);

  add_setshow_string_noescape_cmd ("exec-file", class_files,
				   &remote_exec_file_var, _("\
Set the remote pathname for \"run\""), _("\
Show the remote pathname for \"run\""), NULL,
				   set_remote_exec_file,
				   show_remote_exec_file,
				   &remote_set_cmdlist,
				   &remote_show_cmdlist);

  add_setshow_boolean_cmd ("range-stepping", class_run,
			   &use_range_stepping, _("\
Enable or disable range stepping."), _("\
Show whether target-assisted range stepping is enabled."), _("\
If on, and the target supports it, when stepping a source line, GDB\n\
tells the target to step the corresponding range of addresses itself instead\n\
of issuing multiple single-steps.  This speeds up source level\n\
stepping.  If off, GDB always issues single-steps, even if range\n\
stepping is supported by the target.  The default is on."),
			   set_range_stepping,
			   show_range_stepping,
			   &setlist,
			   &showlist);

  /* Eventually initialize fileio.  See fileio.c */
  initialize_remote_fileio (remote_set_cmdlist, remote_show_cmdlist);

  /* Take advantage of the fact that the TID field is not used, to tag
     special ptids with it set to != 0.  */
  magic_null_ptid = ptid_build (42000, -1, 1);
  not_sent_ptid = ptid_build (42000, -2, 1);
  any_thread_ptid = ptid_build (42000, 0, 1);

  target_buf_size = 2048;
  target_buf = (char *) xmalloc (target_buf_size);
}

