// OBSOLETE /* Memory-access and commands for remote NINDY process, for GDB.
// OBSOLETE 
// OBSOLETE    Copyright 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1998, 1999,
// OBSOLETE    2000, 2001, 2002 Free Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE    Contributed by Intel Corporation.  Modified from remote.c by Chris Benenati.
// OBSOLETE 
// OBSOLETE    GDB is distributed in the hope that it will be useful, but WITHOUT ANY
// OBSOLETE    WARRANTY.  No author or distributor accepts responsibility to anyone
// OBSOLETE    for the consequences of using it or for whether it serves any
// OBSOLETE    particular purpose or works at all, unless he says so in writing.
// OBSOLETE    Refer to the GDB General Public License for full details.
// OBSOLETE 
// OBSOLETE    Everyone is granted permission to copy, modify and redistribute GDB,
// OBSOLETE    but only under the conditions described in the GDB General Public
// OBSOLETE    License.  A copy of this license is supposed to have been given to you
// OBSOLETE    along with GDB so you can know your rights and responsibilities.  It
// OBSOLETE    should be in a file named COPYING.  Among other things, the copyright
// OBSOLETE    notice and this notice must be preserved on all copies.
// OBSOLETE 
// OBSOLETE    In other words, go ahead and share GDB, but don't try to stop
// OBSOLETE    anyone else from sharing it farther.  Help stamp out software hoarding!  */
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE    Except for the data cache routines, this file bears little resemblence
// OBSOLETE    to remote.c.  A new (although similar) protocol has been specified, and
// OBSOLETE    portions of the code are entirely dependent on having an i80960 with a
// OBSOLETE    NINDY ROM monitor at the other end of the line.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE /*****************************************************************************
// OBSOLETE  *
// OBSOLETE  * REMOTE COMMUNICATION PROTOCOL BETWEEN GDB960 AND THE NINDY ROM MONITOR.
// OBSOLETE  *
// OBSOLETE  *
// OBSOLETE  * MODES OF OPERATION
// OBSOLETE  * ----- -- ---------
// OBSOLETE  *	
// OBSOLETE  * As far as NINDY is concerned, GDB is always in one of two modes: command
// OBSOLETE  * mode or passthrough mode.
// OBSOLETE  *
// OBSOLETE  * In command mode (the default) pre-defined packets containing requests
// OBSOLETE  * are sent by GDB to NINDY.  NINDY never talks except in reponse to a request.
// OBSOLETE  *
// OBSOLETE  * Once the the user program is started, GDB enters passthrough mode, to give
// OBSOLETE  * the user program access to the terminal.  GDB remains in this mode until
// OBSOLETE  * NINDY indicates that the program has stopped.
// OBSOLETE  *
// OBSOLETE  *
// OBSOLETE  * PASSTHROUGH MODE
// OBSOLETE  * ----------- ----
// OBSOLETE  *
// OBSOLETE  * GDB writes all input received from the keyboard directly to NINDY, and writes
// OBSOLETE  * all characters received from NINDY directly to the monitor.
// OBSOLETE  *
// OBSOLETE  * Keyboard input is neither buffered nor echoed to the monitor.
// OBSOLETE  *
// OBSOLETE  * GDB remains in passthrough mode until NINDY sends a single ^P character,
// OBSOLETE  * to indicate that the user process has stopped.
// OBSOLETE  *
// OBSOLETE  * Note:
// OBSOLETE  *	GDB assumes NINDY performs a 'flushreg' when the user program stops.
// OBSOLETE  *
// OBSOLETE  *
// OBSOLETE  * COMMAND MODE
// OBSOLETE  * ------- ----
// OBSOLETE  *
// OBSOLETE  * All info (except for message ack and nak) is transferred between gdb
// OBSOLETE  * and the remote processor in messages of the following format:
// OBSOLETE  *
// OBSOLETE  *		<info>#<checksum>
// OBSOLETE  *
// OBSOLETE  * where 
// OBSOLETE  *	#	is a literal character
// OBSOLETE  *
// OBSOLETE  *	<info>	ASCII information;  all numeric information is in the
// OBSOLETE  *		form of hex digits ('0'-'9' and lowercase 'a'-'f').
// OBSOLETE  *
// OBSOLETE  *	<checksum>
// OBSOLETE  *		is a pair of ASCII hex digits representing an 8-bit
// OBSOLETE  *		checksum formed by adding together each of the
// OBSOLETE  *		characters in <info>.
// OBSOLETE  *
// OBSOLETE  * The receiver of a message always sends a single character to the sender
// OBSOLETE  * to indicate that the checksum was good ('+') or bad ('-');  the sender
// OBSOLETE  * re-transmits the entire message over until a '+' is received.
// OBSOLETE  *
// OBSOLETE  * In response to a command NINDY always sends back either data or
// OBSOLETE  * a result code of the form "Xnn", where "nn" are hex digits and "X00"
// OBSOLETE  * means no errors.  (Exceptions: the "s" and "c" commands don't respond.)
// OBSOLETE  *
// OBSOLETE  * SEE THE HEADER OF THE FILE "gdb.c" IN THE NINDY MONITOR SOURCE CODE FOR A
// OBSOLETE  * FULL DESCRIPTION OF LEGAL COMMANDS.
// OBSOLETE  *
// OBSOLETE  * SEE THE FILE "stop.h" IN THE NINDY MONITOR SOURCE CODE FOR A LIST
// OBSOLETE  * OF STOP CODES.
// OBSOLETE  *
// OBSOLETE  ***************************************************************************/
// OBSOLETE 
// OBSOLETE #include "defs.h"
// OBSOLETE #include <signal.h>
// OBSOLETE #include <sys/types.h>
// OBSOLETE #include <setjmp.h>
// OBSOLETE 
// OBSOLETE #include "frame.h"
// OBSOLETE #include "inferior.h"
// OBSOLETE #include "bfd.h"
// OBSOLETE #include "symfile.h"
// OBSOLETE #include "target.h"
// OBSOLETE #include "gdbcore.h"
// OBSOLETE #include "command.h"
// OBSOLETE #include "floatformat.h"
// OBSOLETE #include "regcache.h"
// OBSOLETE 
// OBSOLETE #include <sys/file.h>
// OBSOLETE #include <ctype.h>
// OBSOLETE #include "serial.h"
// OBSOLETE #include "nindy-share/env.h"
// OBSOLETE #include "nindy-share/stop.h"
// OBSOLETE #include "remote-utils.h"
// OBSOLETE 
// OBSOLETE extern int unlink ();
// OBSOLETE extern char *getenv ();
// OBSOLETE extern char *mktemp ();
// OBSOLETE 
// OBSOLETE extern void generic_mourn_inferior ();
// OBSOLETE 
// OBSOLETE extern struct target_ops nindy_ops;
// OBSOLETE extern FILE *instream;
// OBSOLETE 
// OBSOLETE extern char ninStopWhy ();
// OBSOLETE extern int ninMemGet ();
// OBSOLETE extern int ninMemPut ();
// OBSOLETE 
// OBSOLETE int nindy_initial_brk;		/* nonzero if want to send an initial BREAK to nindy */
// OBSOLETE int nindy_old_protocol;		/* nonzero if want to use old protocol */
// OBSOLETE char *nindy_ttyname;		/* name of tty to talk to nindy on, or null */
// OBSOLETE 
// OBSOLETE #define DLE	'\020'		/* Character NINDY sends to indicate user program has
// OBSOLETE 				   * halted.  */
// OBSOLETE #define TRUE	1
// OBSOLETE #define FALSE	0
// OBSOLETE 
// OBSOLETE /* From nindy-share/nindy.c.  */
// OBSOLETE extern struct serial *nindy_serial;
// OBSOLETE 
// OBSOLETE static int have_regs = 0;	/* 1 iff regs read since i960 last halted */
// OBSOLETE static int regs_changed = 0;	/* 1 iff regs were modified since last read */
// OBSOLETE 
// OBSOLETE extern char *exists ();
// OBSOLETE 
// OBSOLETE static void nindy_fetch_registers (int);
// OBSOLETE 
// OBSOLETE static void nindy_store_registers (int);
// OBSOLETE 
// OBSOLETE static char *savename;
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE nindy_close (int quitting)
// OBSOLETE {
// OBSOLETE   if (nindy_serial != NULL)
// OBSOLETE     serial_close (nindy_serial);
// OBSOLETE   nindy_serial = NULL;
// OBSOLETE 
// OBSOLETE   if (savename)
// OBSOLETE     xfree (savename);
// OBSOLETE   savename = 0;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Open a connection to a remote debugger.   
// OBSOLETE    FIXME, there should be "set" commands for the options that are
// OBSOLETE    now specified with gdb command-line options (old_protocol,
// OBSOLETE    and initial_brk).  */
// OBSOLETE void
// OBSOLETE nindy_open (char *name,		/* "/dev/ttyXX", "ttyXX", or "XX": tty to be opened */
// OBSOLETE 	    int from_tty)
// OBSOLETE {
// OBSOLETE   char baudrate[1024];
// OBSOLETE 
// OBSOLETE   if (!name)
// OBSOLETE     error_no_arg ("serial port device name");
// OBSOLETE 
// OBSOLETE   target_preopen (from_tty);
// OBSOLETE 
// OBSOLETE   nindy_close (0);
// OBSOLETE 
// OBSOLETE   have_regs = regs_changed = 0;
// OBSOLETE 
// OBSOLETE   /* Allow user to interrupt the following -- we could hang if there's
// OBSOLETE      no NINDY at the other end of the remote tty.  */
// OBSOLETE   immediate_quit++;
// OBSOLETE   /* If baud_rate is -1, then ninConnect will not recognize the baud rate
// OBSOLETE      and will deal with the situation in a (more or less) reasonable
// OBSOLETE      fashion.  */
// OBSOLETE   sprintf (baudrate, "%d", baud_rate);
// OBSOLETE   ninConnect (name, baudrate,
// OBSOLETE 	      nindy_initial_brk, !from_tty, nindy_old_protocol);
// OBSOLETE   immediate_quit--;
// OBSOLETE 
// OBSOLETE   if (nindy_serial == NULL)
// OBSOLETE     {
// OBSOLETE       perror_with_name (name);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   savename = savestring (name, strlen (name));
// OBSOLETE   push_target (&nindy_ops);
// OBSOLETE 
// OBSOLETE   target_fetch_registers (-1);
// OBSOLETE 
// OBSOLETE   init_thread_list ();
// OBSOLETE   init_wait_for_inferior ();
// OBSOLETE   clear_proceed_status ();
// OBSOLETE   normal_stop ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* User-initiated quit of nindy operations.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE nindy_detach (char *name, int from_tty)
// OBSOLETE {
// OBSOLETE   if (name)
// OBSOLETE     error ("Too many arguments");
// OBSOLETE   pop_target ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE nindy_files_info (void)
// OBSOLETE {
// OBSOLETE   /* FIXME: this lies about the baud rate if we autobauded.  */
// OBSOLETE   printf_unfiltered ("\tAttached to %s at %d bits per second%s%s.\n", savename,
// OBSOLETE 		     baud_rate,
// OBSOLETE 		     nindy_old_protocol ? " in old protocol" : "",
// OBSOLETE 		     nindy_initial_brk ? " with initial break" : "");
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Return the number of characters in the buffer BUF before
// OBSOLETE    the first DLE character.  N is maximum number of characters to
// OBSOLETE    consider.  */
// OBSOLETE 
// OBSOLETE static
// OBSOLETE int
// OBSOLETE non_dle (char *buf, int n)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE 
// OBSOLETE   for (i = 0; i < n; i++)
// OBSOLETE     {
// OBSOLETE       if (buf[i] == DLE)
// OBSOLETE 	{
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   return i;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Tell the remote machine to resume.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE nindy_resume (ptid_t ptid, int step, enum target_signal siggnal)
// OBSOLETE {
// OBSOLETE   if (siggnal != TARGET_SIGNAL_0 && siggnal != stop_signal)
// OBSOLETE     warning ("Can't send signals to remote NINDY targets.");
// OBSOLETE 
// OBSOLETE   if (regs_changed)
// OBSOLETE     {
// OBSOLETE       nindy_store_registers (-1);
// OBSOLETE       regs_changed = 0;
// OBSOLETE     }
// OBSOLETE   have_regs = 0;
// OBSOLETE   ninGo (step);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* FIXME, we can probably use the normal terminal_inferior stuff here.
// OBSOLETE    We have to do terminal_inferior and then set up the passthrough
// OBSOLETE    settings initially.  Thereafter, terminal_ours and terminal_inferior
// OBSOLETE    will automatically swap the settings around for us.  */
// OBSOLETE 
// OBSOLETE struct clean_up_tty_args
// OBSOLETE {
// OBSOLETE   serial_ttystate state;
// OBSOLETE   struct serial *serial;
// OBSOLETE };
// OBSOLETE static struct clean_up_tty_args tty_args;
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE clean_up_tty (PTR ptrarg)
// OBSOLETE {
// OBSOLETE   struct clean_up_tty_args *args = (struct clean_up_tty_args *) ptrarg;
// OBSOLETE   serial_set_tty_state (args->serial, args->state);
// OBSOLETE   xfree (args->state);
// OBSOLETE   warning ("\n\nYou may need to reset the 80960 and/or reload your program.\n");
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Recover from ^Z or ^C while remote process is running */
// OBSOLETE static void (*old_ctrlc) ();
// OBSOLETE #ifdef SIGTSTP
// OBSOLETE static void (*old_ctrlz) ();
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE clean_up_int (void)
// OBSOLETE {
// OBSOLETE   serial_set_tty_state (tty_args.serial, tty_args.state);
// OBSOLETE   xfree (tty_args.state);
// OBSOLETE 
// OBSOLETE   signal (SIGINT, old_ctrlc);
// OBSOLETE #ifdef SIGTSTP
// OBSOLETE   signal (SIGTSTP, old_ctrlz);
// OBSOLETE #endif
// OBSOLETE   error ("\n\nYou may need to reset the 80960 and/or reload your program.\n");
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Wait until the remote machine stops. While waiting, operate in passthrough
// OBSOLETE  * mode; i.e., pass everything NINDY sends to gdb_stdout, and everything from
// OBSOLETE  * stdin to NINDY.
// OBSOLETE  *
// OBSOLETE  * Return to caller, storing status in 'status' just as `wait' would.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE static ptid_t
// OBSOLETE nindy_wait (ptid_t ptid, struct target_waitstatus *status)
// OBSOLETE {
// OBSOLETE   fd_set fds;
// OBSOLETE   int c;
// OBSOLETE   char buf[2];
// OBSOLETE   int i, n;
// OBSOLETE   unsigned char stop_exit;
// OBSOLETE   unsigned char stop_code;
// OBSOLETE   struct cleanup *old_cleanups;
// OBSOLETE   long ip_value, fp_value, sp_value;	/* Reg values from stop */
// OBSOLETE 
// OBSOLETE   status->kind = TARGET_WAITKIND_EXITED;
// OBSOLETE   status->value.integer = 0;
// OBSOLETE 
// OBSOLETE   /* OPERATE IN PASSTHROUGH MODE UNTIL NINDY SENDS A DLE CHARACTER */
// OBSOLETE 
// OBSOLETE   /* Save current tty attributes, and restore them when done.  */
// OBSOLETE   tty_args.serial = serial_fdopen (0);
// OBSOLETE   tty_args.state = serial_get_tty_state (tty_args.serial);
// OBSOLETE   old_ctrlc = signal (SIGINT, clean_up_int);
// OBSOLETE #ifdef SIGTSTP
// OBSOLETE   old_ctrlz = signal (SIGTSTP, clean_up_int);
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE   old_cleanups = make_cleanup (clean_up_tty, &tty_args);
// OBSOLETE 
// OBSOLETE   /* Pass input from keyboard to NINDY as it arrives.  NINDY will interpret
// OBSOLETE      <CR> and perform echo.  */
// OBSOLETE   /* This used to set CBREAK and clear ECHO and CRMOD.  I hope this is close
// OBSOLETE      enough.  */
// OBSOLETE   serial_raw (tty_args.serial);
// OBSOLETE 
// OBSOLETE   while (1)
// OBSOLETE     {
// OBSOLETE       /* Input on remote */
// OBSOLETE       c = serial_readchar (nindy_serial, -1);
// OBSOLETE       if (c == SERIAL_ERROR)
// OBSOLETE 	{
// OBSOLETE 	  error ("Cannot read from serial line");
// OBSOLETE 	}
// OBSOLETE       else if (c == 0x1b)	/* ESC */
// OBSOLETE 	{
// OBSOLETE 	  c = serial_readchar (nindy_serial, -1);
// OBSOLETE 	  c &= ~0x40;
// OBSOLETE 	}
// OBSOLETE       else if (c != 0x10)	/* DLE */
// OBSOLETE 	/* Write out any characters preceding DLE */
// OBSOLETE 	{
// OBSOLETE 	  buf[0] = (char) c;
// OBSOLETE 	  write (1, buf, 1);
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  stop_exit = ninStopWhy (&stop_code,
// OBSOLETE 				  &ip_value, &fp_value, &sp_value);
// OBSOLETE 	  if (!stop_exit && (stop_code == STOP_SRQ))
// OBSOLETE 	    {
// OBSOLETE 	      immediate_quit++;
// OBSOLETE 	      ninSrq ();
// OBSOLETE 	      immediate_quit--;
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    {
// OBSOLETE 	      /* Get out of loop */
// OBSOLETE 	      supply_register (IP_REGNUM,
// OBSOLETE 			       (char *) &ip_value);
// OBSOLETE 	      supply_register (FP_REGNUM,
// OBSOLETE 			       (char *) &fp_value);
// OBSOLETE 	      supply_register (SP_REGNUM,
// OBSOLETE 			       (char *) &sp_value);
// OBSOLETE 	      break;
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   serial_set_tty_state (tty_args.serial, tty_args.state);
// OBSOLETE   xfree (tty_args.state);
// OBSOLETE   discard_cleanups (old_cleanups);
// OBSOLETE 
// OBSOLETE   if (stop_exit)
// OBSOLETE     {
// OBSOLETE       status->kind = TARGET_WAITKIND_EXITED;
// OBSOLETE       status->value.integer = stop_code;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       /* nindy has some special stop code need to be handled */
// OBSOLETE       if (stop_code == STOP_GDB_BPT)
// OBSOLETE 	stop_code = TRACE_STEP;
// OBSOLETE       status->kind = TARGET_WAITKIND_STOPPED;
// OBSOLETE       status->value.sig = i960_fault_to_signal (stop_code);
// OBSOLETE     }
// OBSOLETE   return inferior_ptid;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Read the remote registers into the block REGS.  */
// OBSOLETE 
// OBSOLETE /* This is the block that ninRegsGet and ninRegsPut handles.  */
// OBSOLETE struct nindy_regs
// OBSOLETE {
// OBSOLETE   char local_regs[16 * 4];
// OBSOLETE   char global_regs[16 * 4];
// OBSOLETE   char pcw_acw[2 * 4];
// OBSOLETE   char ip[4];
// OBSOLETE   char tcw[4];
// OBSOLETE   char fp_as_double[4 * 8];
// OBSOLETE };
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE nindy_fetch_registers (int regno)
// OBSOLETE {
// OBSOLETE   struct nindy_regs nindy_regs;
// OBSOLETE   int regnum;
// OBSOLETE 
// OBSOLETE   immediate_quit++;
// OBSOLETE   ninRegsGet ((char *) &nindy_regs);
// OBSOLETE   immediate_quit--;
// OBSOLETE 
// OBSOLETE   memcpy (&registers[REGISTER_BYTE (R0_REGNUM)], nindy_regs.local_regs, 16 * 4);
// OBSOLETE   memcpy (&registers[REGISTER_BYTE (G0_REGNUM)], nindy_regs.global_regs, 16 * 4);
// OBSOLETE   memcpy (&registers[REGISTER_BYTE (PCW_REGNUM)], nindy_regs.pcw_acw, 2 * 4);
// OBSOLETE   memcpy (&registers[REGISTER_BYTE (IP_REGNUM)], nindy_regs.ip, 1 * 4);
// OBSOLETE   memcpy (&registers[REGISTER_BYTE (TCW_REGNUM)], nindy_regs.tcw, 1 * 4);
// OBSOLETE   memcpy (&registers[REGISTER_BYTE (FP0_REGNUM)], nindy_regs.fp_as_double, 4 * 8);
// OBSOLETE 
// OBSOLETE   registers_fetched ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE nindy_prepare_to_store (void)
// OBSOLETE {
// OBSOLETE   /* Fetch all regs if they aren't already here.  */
// OBSOLETE   read_register_bytes (0, NULL, REGISTER_BYTES);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE nindy_store_registers (int regno)
// OBSOLETE {
// OBSOLETE   struct nindy_regs nindy_regs;
// OBSOLETE   int regnum;
// OBSOLETE 
// OBSOLETE   memcpy (nindy_regs.local_regs, &registers[REGISTER_BYTE (R0_REGNUM)], 16 * 4);
// OBSOLETE   memcpy (nindy_regs.global_regs, &registers[REGISTER_BYTE (G0_REGNUM)], 16 * 4);
// OBSOLETE   memcpy (nindy_regs.pcw_acw, &registers[REGISTER_BYTE (PCW_REGNUM)], 2 * 4);
// OBSOLETE   memcpy (nindy_regs.ip, &registers[REGISTER_BYTE (IP_REGNUM)], 1 * 4);
// OBSOLETE   memcpy (nindy_regs.tcw, &registers[REGISTER_BYTE (TCW_REGNUM)], 1 * 4);
// OBSOLETE   memcpy (nindy_regs.fp_as_double, &registers[REGISTER_BYTE (FP0_REGNUM)], 8 * 4);
// OBSOLETE 
// OBSOLETE   immediate_quit++;
// OBSOLETE   ninRegsPut ((char *) &nindy_regs);
// OBSOLETE   immediate_quit--;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Copy LEN bytes to or from inferior's memory starting at MEMADDR
// OBSOLETE    to debugger memory starting at MYADDR.   Copy to inferior if
// OBSOLETE    SHOULD_WRITE is nonzero.  Returns the length copied.  TARGET is
// OBSOLETE    unused.  */
// OBSOLETE 
// OBSOLETE int
// OBSOLETE nindy_xfer_inferior_memory (CORE_ADDR memaddr, char *myaddr, int len,
// OBSOLETE 			    int should_write, struct mem_attrib *attrib,
// OBSOLETE 			    struct target_ops *target)
// OBSOLETE {
// OBSOLETE   int res;
// OBSOLETE 
// OBSOLETE   if (len <= 0)
// OBSOLETE     return 0;
// OBSOLETE 
// OBSOLETE   if (should_write)
// OBSOLETE     res = ninMemPut (memaddr, myaddr, len);
// OBSOLETE   else
// OBSOLETE     res = ninMemGet (memaddr, myaddr, len);
// OBSOLETE 
// OBSOLETE   return res;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE nindy_create_inferior (char *execfile, char *args, char **env)
// OBSOLETE {
// OBSOLETE   int entry_pt;
// OBSOLETE   int pid;
// OBSOLETE 
// OBSOLETE   if (args && *args)
// OBSOLETE     error ("Can't pass arguments to remote NINDY process");
// OBSOLETE 
// OBSOLETE   if (execfile == 0 || exec_bfd == 0)
// OBSOLETE     error ("No executable file specified");
// OBSOLETE 
// OBSOLETE   entry_pt = (int) bfd_get_start_address (exec_bfd);
// OBSOLETE 
// OBSOLETE   pid = 42;
// OBSOLETE 
// OBSOLETE   /* The "process" (board) is already stopped awaiting our commands, and
// OBSOLETE      the program is already downloaded.  We just set its PC and go.  */
// OBSOLETE 
// OBSOLETE   inferior_ptid = pid_to_ptid (pid);	/* Needed for wait_for_inferior below */
// OBSOLETE 
// OBSOLETE   clear_proceed_status ();
// OBSOLETE 
// OBSOLETE   /* Tell wait_for_inferior that we've started a new process.  */
// OBSOLETE   init_wait_for_inferior ();
// OBSOLETE 
// OBSOLETE   /* Set up the "saved terminal modes" of the inferior
// OBSOLETE      based on what modes we are starting it with.  */
// OBSOLETE   target_terminal_init ();
// OBSOLETE 
// OBSOLETE   /* Install inferior's terminal modes.  */
// OBSOLETE   target_terminal_inferior ();
// OBSOLETE 
// OBSOLETE   /* insert_step_breakpoint ();  FIXME, do we need this?  */
// OBSOLETE   /* Let 'er rip... */
// OBSOLETE   proceed ((CORE_ADDR) entry_pt, TARGET_SIGNAL_DEFAULT, 0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE reset_command (char *args, int from_tty)
// OBSOLETE {
// OBSOLETE   if (nindy_serial == NULL)
// OBSOLETE     {
// OBSOLETE       error ("No target system to reset -- use 'target nindy' command.");
// OBSOLETE     }
// OBSOLETE   if (query ("Really reset the target system?", 0, 0))
// OBSOLETE     {
// OBSOLETE       serial_send_break (nindy_serial);
// OBSOLETE       tty_flush (nindy_serial);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE nindy_kill (char *args, int from_tty)
// OBSOLETE {
// OBSOLETE   return;			/* Ignore attempts to kill target system */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Clean up when a program exits.
// OBSOLETE 
// OBSOLETE    The program actually lives on in the remote processor's RAM, and may be
// OBSOLETE    run again without a download.  Don't leave it full of breakpoint
// OBSOLETE    instructions.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE nindy_mourn_inferior (void)
// OBSOLETE {
// OBSOLETE   remove_breakpoints ();
// OBSOLETE   unpush_target (&nindy_ops);
// OBSOLETE   generic_mourn_inferior ();	/* Do all the proper things now */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Pass the args the way catch_errors wants them.  */
// OBSOLETE static int
// OBSOLETE nindy_open_stub (char *arg)
// OBSOLETE {
// OBSOLETE   nindy_open (arg, 1);
// OBSOLETE   return 1;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE nindy_load (char *filename, int from_tty)
// OBSOLETE {
// OBSOLETE   asection *s;
// OBSOLETE   /* Can't do unix style forking on a VMS system, so we'll use bfd to do
// OBSOLETE      all the work for us
// OBSOLETE    */
// OBSOLETE 
// OBSOLETE   bfd *file = bfd_openr (filename, 0);
// OBSOLETE   if (!file)
// OBSOLETE     {
// OBSOLETE       perror_with_name (filename);
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (!bfd_check_format (file, bfd_object))
// OBSOLETE     {
// OBSOLETE       error ("can't prove it's an object file\n");
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   for (s = file->sections; s; s = s->next)
// OBSOLETE     {
// OBSOLETE       if (s->flags & SEC_LOAD)
// OBSOLETE 	{
// OBSOLETE 	  char *buffer = xmalloc (s->_raw_size);
// OBSOLETE 	  bfd_get_section_contents (file, s, buffer, 0, s->_raw_size);
// OBSOLETE 	  printf ("Loading section %s, size %x vma %x\n",
// OBSOLETE 		  s->name,
// OBSOLETE 		  s->_raw_size,
// OBSOLETE 		  s->vma);
// OBSOLETE 	  ninMemPut (s->vma, buffer, s->_raw_size);
// OBSOLETE 	  xfree (buffer);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   bfd_close (file);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE load_stub (char *arg)
// OBSOLETE {
// OBSOLETE   target_load (arg, 1);
// OBSOLETE   return 1;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* This routine is run as a hook, just before the main command loop is
// OBSOLETE    entered.  If gdb is configured for the i960, but has not had its
// OBSOLETE    nindy target specified yet, this will loop prompting the user to do so.
// OBSOLETE 
// OBSOLETE    Unlike the loop provided by Intel, we actually let the user get out
// OBSOLETE    of this with a RETURN.  This is useful when e.g. simply examining
// OBSOLETE    an i960 object file on the host system.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE nindy_before_main_loop (void)
// OBSOLETE {
// OBSOLETE   char ttyname[100];
// OBSOLETE   char *p, *p2;
// OBSOLETE 
// OBSOLETE   while (target_stack->target_ops != &nindy_ops)	/* What is this crap??? */
// OBSOLETE     {				/* remote tty not specified yet */
// OBSOLETE       if (instream == stdin)
// OBSOLETE 	{
// OBSOLETE 	  printf_unfiltered ("\nAttach /dev/ttyNN -- specify NN, or \"quit\" to quit:  ");
// OBSOLETE 	  gdb_flush (gdb_stdout);
// OBSOLETE 	}
// OBSOLETE       fgets (ttyname, sizeof (ttyname) - 1, stdin);
// OBSOLETE 
// OBSOLETE       /* Strip leading and trailing whitespace */
// OBSOLETE       for (p = ttyname; isspace (*p); p++)
// OBSOLETE 	{
// OBSOLETE 	  ;
// OBSOLETE 	}
// OBSOLETE       if (*p == '\0')
// OBSOLETE 	{
// OBSOLETE 	  return;		/* User just hit spaces or return, wants out */
// OBSOLETE 	}
// OBSOLETE       for (p2 = p; !isspace (*p2) && (*p2 != '\0'); p2++)
// OBSOLETE 	{
// OBSOLETE 	  ;
// OBSOLETE 	}
// OBSOLETE       *p2 = '\0';
// OBSOLETE       if (STREQ ("quit", p))
// OBSOLETE 	{
// OBSOLETE 	  exit (1);
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       if (catch_errors (nindy_open_stub, p, "", RETURN_MASK_ALL))
// OBSOLETE 	{
// OBSOLETE 	  /* Now that we have a tty open for talking to the remote machine,
// OBSOLETE 	     download the executable file if one was specified.  */
// OBSOLETE 	  if (exec_bfd)
// OBSOLETE 	    {
// OBSOLETE 	      catch_errors (load_stub, bfd_get_filename (exec_bfd), "",
// OBSOLETE 			    RETURN_MASK_ALL);
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Define the target subroutine names */
// OBSOLETE 
// OBSOLETE struct target_ops nindy_ops;
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE init_nindy_ops (void)
// OBSOLETE {
// OBSOLETE   nindy_ops.to_shortname = "nindy";
// OBSOLETE   "Remote serial target in i960 NINDY-specific protocol",
// OBSOLETE     nindy_ops.to_longname = "Use a remote i960 system running NINDY connected by a serial line.\n\
// OBSOLETE Specify the name of the device the serial line is connected to.\n\
// OBSOLETE The speed (baud rate), whether to use the old NINDY protocol,\n\
// OBSOLETE and whether to send a break on startup, are controlled by options\n\
// OBSOLETE specified when you started GDB.";
// OBSOLETE   nindy_ops.to_doc = "";
// OBSOLETE   nindy_ops.to_open = nindy_open;
// OBSOLETE   nindy_ops.to_close = nindy_close;
// OBSOLETE   nindy_ops.to_attach = 0;
// OBSOLETE   nindy_ops.to_post_attach = NULL;
// OBSOLETE   nindy_ops.to_require_attach = NULL;
// OBSOLETE   nindy_ops.to_detach = nindy_detach;
// OBSOLETE   nindy_ops.to_require_detach = NULL;
// OBSOLETE   nindy_ops.to_resume = nindy_resume;
// OBSOLETE   nindy_ops.to_wait = nindy_wait;
// OBSOLETE   nindy_ops.to_post_wait = NULL;
// OBSOLETE   nindy_ops.to_fetch_registers = nindy_fetch_registers;
// OBSOLETE   nindy_ops.to_store_registers = nindy_store_registers;
// OBSOLETE   nindy_ops.to_prepare_to_store = nindy_prepare_to_store;
// OBSOLETE   nindy_ops.to_xfer_memory = nindy_xfer_inferior_memory;
// OBSOLETE   nindy_ops.to_files_info = nindy_files_info;
// OBSOLETE   nindy_ops.to_insert_breakpoint = memory_insert_breakpoint;
// OBSOLETE   nindy_ops.to_remove_breakpoint = memory_remove_breakpoint;
// OBSOLETE   nindy_ops.to_terminal_init = 0;
// OBSOLETE   nindy_ops.to_terminal_inferior = 0;
// OBSOLETE   nindy_ops.to_terminal_ours_for_output = 0;
// OBSOLETE   nindy_ops.to_terminal_ours = 0;
// OBSOLETE   nindy_ops.to_terminal_info = 0;	/* Terminal crud */
// OBSOLETE   nindy_ops.to_kill = nindy_kill;
// OBSOLETE   nindy_ops.to_load = nindy_load;
// OBSOLETE   nindy_ops.to_lookup_symbol = 0;	/* lookup_symbol */
// OBSOLETE   nindy_ops.to_create_inferior = nindy_create_inferior;
// OBSOLETE   nindy_ops.to_post_startup_inferior = NULL;
// OBSOLETE   nindy_ops.to_acknowledge_created_inferior = NULL;
// OBSOLETE   nindy_ops.to_clone_and_follow_inferior = NULL;
// OBSOLETE   nindy_ops.to_post_follow_inferior_by_clone = NULL;
// OBSOLETE   nindy_ops.to_insert_fork_catchpoint = NULL;
// OBSOLETE   nindy_ops.to_remove_fork_catchpoint = NULL;
// OBSOLETE   nindy_ops.to_insert_vfork_catchpoint = NULL;
// OBSOLETE   nindy_ops.to_remove_vfork_catchpoint = NULL;
// OBSOLETE   nindy_ops.to_has_forked = NULL;
// OBSOLETE   nindy_ops.to_has_vforked = NULL;
// OBSOLETE   nindy_ops.to_can_follow_vfork_prior_to_exec = NULL;
// OBSOLETE   nindy_ops.to_post_follow_vfork = NULL;
// OBSOLETE   nindy_ops.to_insert_exec_catchpoint = NULL;
// OBSOLETE   nindy_ops.to_remove_exec_catchpoint = NULL;
// OBSOLETE   nindy_ops.to_has_execd = NULL;
// OBSOLETE   nindy_ops.to_reported_exec_events_per_exec_call = NULL;
// OBSOLETE   nindy_ops.to_has_exited = NULL;
// OBSOLETE   nindy_ops.to_mourn_inferior = nindy_mourn_inferior;
// OBSOLETE   nindy_ops.to_can_run = 0;	/* can_run */
// OBSOLETE   nindy_ops.to_notice_signals = 0;	/* notice_signals */
// OBSOLETE   nindy_ops.to_thread_alive = 0;	/* to_thread_alive */
// OBSOLETE   nindy_ops.to_stop = 0;	/* to_stop */
// OBSOLETE   nindy_ops.to_pid_to_exec_file = NULL;
// OBSOLETE   nindy_ops.to_stratum = process_stratum;
// OBSOLETE   nindy_ops.DONT_USE = 0;	/* next */
// OBSOLETE   nindy_ops.to_has_all_memory = 1;
// OBSOLETE   nindy_ops.to_has_memory = 1;
// OBSOLETE   nindy_ops.to_has_stack = 1;
// OBSOLETE   nindy_ops.to_has_registers = 1;
// OBSOLETE   nindy_ops.to_has_execution = 1;	/* all mem, mem, stack, regs, exec */
// OBSOLETE   nindy_ops.to_sections = 0;
// OBSOLETE   nindy_ops.to_sections_end = 0;	/* Section pointers */
// OBSOLETE   nindy_ops.to_magic = OPS_MAGIC;	/* Always the last thing */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_nindy (void)
// OBSOLETE {
// OBSOLETE   init_nindy_ops ();
// OBSOLETE   add_target (&nindy_ops);
// OBSOLETE   add_com ("reset", class_obscure, reset_command,
// OBSOLETE 	   "Send a 'break' to the remote target system.\n\
// OBSOLETE Only useful if the target has been equipped with a circuit\n\
// OBSOLETE to perform a hard reset when a break is detected.");
// OBSOLETE }
