// OBSOLETE /* Remote debugging interface for boot monitors, for GDB.
// OBSOLETE 
// OBSOLETE    Copyright 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1998, 1999,
// OBSOLETE    2000, 2001, 2002 Free Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE    This file is part of GDB.
// OBSOLETE 
// OBSOLETE    This program is free software; you can redistribute it and/or modify
// OBSOLETE    it under the terms of the GNU General Public License as published by
// OBSOLETE    the Free Software Foundation; either version 2 of the License, or
// OBSOLETE    (at your option) any later version.
// OBSOLETE 
// OBSOLETE    This program is distributed in the hope that it will be useful,
// OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE    GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE    You should have received a copy of the GNU General Public License
// OBSOLETE    along with this program; if not, write to the Free Software
// OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330,
// OBSOLETE    Boston, MA 02111-1307, USA.  */
// OBSOLETE 
// OBSOLETE /* This file was derived from remote-eb.c, which did a similar job, but for
// OBSOLETE    an AMD-29K running EBMON.  That file was in turn derived from remote.c
// OBSOLETE    as mentioned in the following comment (left in for comic relief):
// OBSOLETE 
// OBSOLETE    "This is like remote.c but is for a different situation--
// OBSOLETE    having a PC running os9000 hook up with a unix machine with
// OBSOLETE    a serial line, and running ctty com2 on the PC. os9000 has a debug
// OBSOLETE    monitor called ROMBUG running.  Not to mention that the PC
// OBSOLETE    has PC/NFS, so it can access the same executables that gdb can,
// OBSOLETE    over the net in real time."
// OBSOLETE 
// OBSOLETE    In reality, this module talks to a debug monitor called 'ROMBUG', which
// OBSOLETE    We communicate with ROMBUG via a direct serial line, the network version
// OBSOLETE    of ROMBUG is not available yet.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE /* FIXME This file needs to be rewritten if it's to work again, either
// OBSOLETE    to self-contained or to use the new monitor interface.  */
// OBSOLETE 
// OBSOLETE #include "defs.h"
// OBSOLETE #include "gdbcore.h"
// OBSOLETE #include "target.h"
// OBSOLETE #include "gdb_string.h"
// OBSOLETE #include <sys/types.h>
// OBSOLETE #include "command.h"
// OBSOLETE #include "serial.h"
// OBSOLETE #include "monitor.h"
// OBSOLETE #include "remote-utils.h"
// OBSOLETE #include "symtab.h"
// OBSOLETE #include "symfile.h"
// OBSOLETE #include "objfiles.h"
// OBSOLETE #include "gdb-stabs.h"
// OBSOLETE #include "regcache.h"
// OBSOLETE 
// OBSOLETE struct cmd_list_element *showlist;
// OBSOLETE extern struct target_ops rombug_ops;	/* Forward declaration */
// OBSOLETE extern struct monitor_ops rombug_cmds;	/* Forward declaration */
// OBSOLETE extern struct cmd_list_element *setlist;
// OBSOLETE extern struct cmd_list_element *unsetlist;
// OBSOLETE extern int attach_flag;
// OBSOLETE 
// OBSOLETE static void rombug_close ();
// OBSOLETE static void rombug_fetch_register ();
// OBSOLETE static void rombug_fetch_registers ();
// OBSOLETE static void rombug_store_register ();
// OBSOLETE #if 0
// OBSOLETE static int sr_get_debug ();	/* flag set by "set remotedebug" */
// OBSOLETE #endif
// OBSOLETE static int hashmark;		/* flag set by "set hash" */
// OBSOLETE static int rombug_is_open = 0;
// OBSOLETE 
// OBSOLETE /* FIXME: Replace with sr_get_debug ().  */
// OBSOLETE #define LOG_FILE "monitor.log"
// OBSOLETE FILE *log_file;
// OBSOLETE static int monitor_log = 0;
// OBSOLETE static int tty_xon = 0;
// OBSOLETE static int tty_xoff = 0;
// OBSOLETE 
// OBSOLETE static int timeout = 10;
// OBSOLETE static int is_trace_mode = 0;
// OBSOLETE /* Descriptor for I/O to remote machine.  Initialize it to NULL */
// OBSOLETE static struct serial *monitor_desc = NULL;
// OBSOLETE 
// OBSOLETE static CORE_ADDR bufaddr = 0;
// OBSOLETE static int buflen = 0;
// OBSOLETE static char readbuf[16];
// OBSOLETE 
// OBSOLETE /* Send data to monitor.  Works just like printf. */
// OBSOLETE static void
// OBSOLETE printf_monitor (char *pattern,...)
// OBSOLETE {
// OBSOLETE   va_list args;
// OBSOLETE   char buf[200];
// OBSOLETE   int i;
// OBSOLETE 
// OBSOLETE   va_start (args, pattern);
// OBSOLETE 
// OBSOLETE   vsprintf (buf, pattern, args);
// OBSOLETE   va_end (args);
// OBSOLETE 
// OBSOLETE   if (serial_write (monitor_desc, buf, strlen (buf)))
// OBSOLETE     fprintf_unfiltered (gdb_stderr, "serial_write failed: %s\n",
// OBSOLETE 			safe_strerror (errno));
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Read a character from the remote system, doing all the fancy timeout stuff */
// OBSOLETE static int
// OBSOLETE readchar (int timeout)
// OBSOLETE {
// OBSOLETE   int c;
// OBSOLETE 
// OBSOLETE   c = serial_readchar (monitor_desc, timeout);
// OBSOLETE 
// OBSOLETE   if (sr_get_debug ())
// OBSOLETE     putchar (c & 0x7f);
// OBSOLETE 
// OBSOLETE   if (monitor_log && isascii (c))
// OBSOLETE     putc (c & 0x7f, log_file);
// OBSOLETE 
// OBSOLETE   if (c >= 0)
// OBSOLETE     return c & 0x7f;
// OBSOLETE 
// OBSOLETE   if (c == SERIAL_TIMEOUT)
// OBSOLETE     {
// OBSOLETE       if (timeout == 0)
// OBSOLETE 	return c;		/* Polls shouldn't generate timeout errors */
// OBSOLETE 
// OBSOLETE       error ("Timeout reading from remote system.");
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   perror_with_name ("remote-monitor");
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Scan input from the remote system, until STRING is found.  If DISCARD is
// OBSOLETE    non-zero, then discard non-matching input, else print it out.
// OBSOLETE    Let the user break out immediately.  */
// OBSOLETE static void
// OBSOLETE expect (char *string, int discard)
// OBSOLETE {
// OBSOLETE   char *p = string;
// OBSOLETE   int c;
// OBSOLETE 
// OBSOLETE   if (sr_get_debug ())
// OBSOLETE     printf ("Expecting \"%s\"\n", string);
// OBSOLETE 
// OBSOLETE   immediate_quit++;
// OBSOLETE   while (1)
// OBSOLETE     {
// OBSOLETE       c = readchar (timeout);
// OBSOLETE       if (!isascii (c))
// OBSOLETE 	continue;
// OBSOLETE       if (c == *p++)
// OBSOLETE 	{
// OBSOLETE 	  if (*p == '\0')
// OBSOLETE 	    {
// OBSOLETE 	      immediate_quit--;
// OBSOLETE 	      if (sr_get_debug ())
// OBSOLETE 		printf ("\nMatched\n");
// OBSOLETE 	      return;
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  if (!discard)
// OBSOLETE 	    {
// OBSOLETE 	      fwrite (string, 1, (p - 1) - string, stdout);
// OBSOLETE 	      putchar ((char) c);
// OBSOLETE 	      fflush (stdout);
// OBSOLETE 	    }
// OBSOLETE 	  p = string;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Keep discarding input until we see the ROMBUG prompt.
// OBSOLETE 
// OBSOLETE    The convention for dealing with the prompt is that you
// OBSOLETE    o give your command
// OBSOLETE    o *then* wait for the prompt.
// OBSOLETE 
// OBSOLETE    Thus the last thing that a procedure does with the serial line
// OBSOLETE    will be an expect_prompt().  Exception:  rombug_resume does not
// OBSOLETE    wait for the prompt, because the terminal is being handed over
// OBSOLETE    to the inferior.  However, the next thing which happens after that
// OBSOLETE    is a rombug_wait which does wait for the prompt.
// OBSOLETE    Note that this includes abnormal exit, e.g. error().  This is
// OBSOLETE    necessary to prevent getting into states from which we can't
// OBSOLETE    recover.  */
// OBSOLETE static void
// OBSOLETE expect_prompt (int discard)
// OBSOLETE {
// OBSOLETE   if (monitor_log)
// OBSOLETE     /* This is a convenient place to do this.  The idea is to do it often
// OBSOLETE        enough that we never lose much data if we terminate abnormally.  */
// OBSOLETE     fflush (log_file);
// OBSOLETE 
// OBSOLETE   if (is_trace_mode)
// OBSOLETE     {
// OBSOLETE       expect ("trace", discard);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       expect (PROMPT, discard);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get a hex digit from the remote system & return its value.
// OBSOLETE    If ignore_space is nonzero, ignore spaces (not newline, tab, etc).  */
// OBSOLETE static int
// OBSOLETE get_hex_digit (int ignore_space)
// OBSOLETE {
// OBSOLETE   int ch;
// OBSOLETE   while (1)
// OBSOLETE     {
// OBSOLETE       ch = readchar (timeout);
// OBSOLETE       if (ch >= '0' && ch <= '9')
// OBSOLETE 	return ch - '0';
// OBSOLETE       else if (ch >= 'A' && ch <= 'F')
// OBSOLETE 	return ch - 'A' + 10;
// OBSOLETE       else if (ch >= 'a' && ch <= 'f')
// OBSOLETE 	return ch - 'a' + 10;
// OBSOLETE       else if (ch == ' ' && ignore_space)
// OBSOLETE 	;
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  expect_prompt (1);
// OBSOLETE 	  error ("Invalid hex digit from remote system.");
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get a byte from monitor and put it in *BYT.  Accept any number
// OBSOLETE    leading spaces.  */
// OBSOLETE static void
// OBSOLETE get_hex_byte (char *byt)
// OBSOLETE {
// OBSOLETE   int val;
// OBSOLETE 
// OBSOLETE   val = get_hex_digit (1) << 4;
// OBSOLETE   val |= get_hex_digit (0);
// OBSOLETE   *byt = val;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get N 32-bit words from remote, each preceded by a space,
// OBSOLETE    and put them in registers starting at REGNO.  */
// OBSOLETE static void
// OBSOLETE get_hex_regs (int n, int regno)
// OBSOLETE {
// OBSOLETE   long val;
// OBSOLETE   int i;
// OBSOLETE   unsigned char b;
// OBSOLETE 
// OBSOLETE   for (i = 0; i < n; i++)
// OBSOLETE     {
// OBSOLETE       int j;
// OBSOLETE 
// OBSOLETE       val = 0;
// OBSOLETE       for (j = 0; j < 4; j++)
// OBSOLETE 	{
// OBSOLETE 	  get_hex_byte (&b);
// OBSOLETE 	  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
// OBSOLETE 	    val = (val << 8) + b;
// OBSOLETE 	  else
// OBSOLETE 	    val = val + (b << (j * 8));
// OBSOLETE 	}
// OBSOLETE       supply_register (regno++, (char *) &val);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* This is called not only when we first attach, but also when the
// OBSOLETE    user types "run" after having attached.  */
// OBSOLETE static void
// OBSOLETE rombug_create_inferior (char *execfile, char *args, char **env)
// OBSOLETE {
// OBSOLETE   int entry_pt;
// OBSOLETE 
// OBSOLETE   if (args && *args)
// OBSOLETE     error ("Can't pass arguments to remote ROMBUG process");
// OBSOLETE 
// OBSOLETE   if (execfile == 0 || exec_bfd == 0)
// OBSOLETE     error ("No executable file specified");
// OBSOLETE 
// OBSOLETE   entry_pt = (int) bfd_get_start_address (exec_bfd);
// OBSOLETE 
// OBSOLETE   if (monitor_log)
// OBSOLETE     fputs ("\nIn Create_inferior()", log_file);
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* The "process" (board) is already stopped awaiting our commands, and
// OBSOLETE    the program is already downloaded.  We just set its PC and go.  */
// OBSOLETE 
// OBSOLETE   init_wait_for_inferior ();
// OBSOLETE   proceed ((CORE_ADDR) entry_pt, TARGET_SIGNAL_DEFAULT, 0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Open a connection to a remote debugger.
// OBSOLETE    NAME is the filename used for communication.  */
// OBSOLETE 
// OBSOLETE static char dev_name[100];
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE rombug_open (char *args, int from_tty)
// OBSOLETE {
// OBSOLETE   if (args == NULL)
// OBSOLETE     error ("Use `target RomBug DEVICE-NAME' to use a serial port, or \n\
// OBSOLETE `target RomBug HOST-NAME:PORT-NUMBER' to use a network connection.");
// OBSOLETE 
// OBSOLETE   target_preopen (from_tty);
// OBSOLETE 
// OBSOLETE   if (rombug_is_open)
// OBSOLETE     unpush_target (&rombug_ops);
// OBSOLETE 
// OBSOLETE   strcpy (dev_name, args);
// OBSOLETE   monitor_desc = serial_open (dev_name);
// OBSOLETE   if (monitor_desc == NULL)
// OBSOLETE     perror_with_name (dev_name);
// OBSOLETE 
// OBSOLETE   /* if baud rate is set by 'set remotebaud' */
// OBSOLETE   if (serial_setbaudrate (monitor_desc, sr_get_baud_rate ()))
// OBSOLETE     {
// OBSOLETE       serial_close (monitor_desc);
// OBSOLETE       perror_with_name ("RomBug");
// OBSOLETE     }
// OBSOLETE   serial_raw (monitor_desc);
// OBSOLETE   if (tty_xon || tty_xoff)
// OBSOLETE     {
// OBSOLETE       struct hardware_ttystate
// OBSOLETE 	{
// OBSOLETE 	  struct termios t;
// OBSOLETE 	}
// OBSOLETE        *tty_s;
// OBSOLETE 
// OBSOLETE       tty_s = (struct hardware_ttystate *) serial_get_tty_state (monitor_desc);
// OBSOLETE       if (tty_xon)
// OBSOLETE 	tty_s->t.c_iflag |= IXON;
// OBSOLETE       if (tty_xoff)
// OBSOLETE 	tty_s->t.c_iflag |= IXOFF;
// OBSOLETE       serial_set_tty_state (monitor_desc, (serial_ttystate) tty_s);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   rombug_is_open = 1;
// OBSOLETE 
// OBSOLETE   log_file = fopen (LOG_FILE, "w");
// OBSOLETE   if (log_file == NULL)
// OBSOLETE     perror_with_name (LOG_FILE);
// OBSOLETE 
// OBSOLETE   push_monitor (&rombug_cmds);
// OBSOLETE   printf_monitor ("\r");	/* CR wakes up monitor */
// OBSOLETE   expect_prompt (1);
// OBSOLETE   push_target (&rombug_ops);
// OBSOLETE   attach_flag = 1;
// OBSOLETE 
// OBSOLETE   if (from_tty)
// OBSOLETE     printf ("Remote %s connected to %s\n", target_shortname,
// OBSOLETE 	    dev_name);
// OBSOLETE 
// OBSOLETE   rombug_fetch_registers ();
// OBSOLETE 
// OBSOLETE   printf_monitor ("ov e \r");
// OBSOLETE   expect_prompt (1);
// OBSOLETE   bufaddr = 0;
// OBSOLETE   buflen = 0;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE  * Close out all files and local state before this target loses control.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE rombug_close (int quitting)
// OBSOLETE {
// OBSOLETE   if (rombug_is_open)
// OBSOLETE     {
// OBSOLETE       serial_close (monitor_desc);
// OBSOLETE       monitor_desc = NULL;
// OBSOLETE       rombug_is_open = 0;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (log_file)
// OBSOLETE     {
// OBSOLETE       if (ferror (log_file))
// OBSOLETE 	fprintf_unfiltered (gdb_stderr, "Error writing log file.\n");
// OBSOLETE       if (fclose (log_file) != 0)
// OBSOLETE 	fprintf_unfiltered (gdb_stderr, "Error closing log file.\n");
// OBSOLETE       log_file = 0;
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE int
// OBSOLETE rombug_link (char *mod_name, CORE_ADDR *text_reloc)
// OBSOLETE {
// OBSOLETE   int i, j;
// OBSOLETE   unsigned long val;
// OBSOLETE   unsigned char b;
// OBSOLETE 
// OBSOLETE   printf_monitor ("l %s \r", mod_name);
// OBSOLETE   expect_prompt (1);
// OBSOLETE   printf_monitor (".r \r");
// OBSOLETE   expect (REG_DELIM, 1);
// OBSOLETE   for (i = 0; i <= 7; i++)
// OBSOLETE     {
// OBSOLETE       val = 0;
// OBSOLETE       for (j = 0; j < 4; j++)
// OBSOLETE 	{
// OBSOLETE 	  get_hex_byte (&b);
// OBSOLETE 	  val = (val << 8) + b;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   expect_prompt (1);
// OBSOLETE   *text_reloc = val;
// OBSOLETE   return 1;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Terminate the open connection to the remote debugger.
// OBSOLETE    Use this when you want to detach and do something else
// OBSOLETE    with your gdb.  */
// OBSOLETE static void
// OBSOLETE rombug_detach (int from_tty)
// OBSOLETE {
// OBSOLETE   if (attach_flag)
// OBSOLETE     {
// OBSOLETE       printf_monitor (GO_CMD);
// OBSOLETE       attach_flag = 0;
// OBSOLETE     }
// OBSOLETE   pop_target ();		/* calls rombug_close to do the real work */
// OBSOLETE   if (from_tty)
// OBSOLETE     printf ("Ending remote %s debugging\n", target_shortname);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE  * Tell the remote machine to resume.
// OBSOLETE  */
// OBSOLETE static void
// OBSOLETE rombug_resume (ptid_t ptid, int step, enum target_signal sig)
// OBSOLETE {
// OBSOLETE   if (monitor_log)
// OBSOLETE     fprintf (log_file, "\nIn Resume (step=%d, sig=%d)\n", step, sig);
// OBSOLETE 
// OBSOLETE   if (step)
// OBSOLETE     {
// OBSOLETE       is_trace_mode = 1;
// OBSOLETE       printf_monitor (STEP_CMD);
// OBSOLETE       /* wait for the echo.  **
// OBSOLETE          expect (STEP_CMD, 1);
// OBSOLETE        */
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       printf_monitor (GO_CMD);
// OBSOLETE       /* swallow the echo.  **
// OBSOLETE          expect (GO_CMD, 1);
// OBSOLETE        */
// OBSOLETE     }
// OBSOLETE   bufaddr = 0;
// OBSOLETE   buflen = 0;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE  * Wait until the remote machine stops, then return,
// OBSOLETE  * storing status in status just as `wait' would.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE static ptid *
// OBSOLETE rombug_wait (ptid_t ptid, struct target_waitstatus *status)
// OBSOLETE {
// OBSOLETE   int old_timeout = timeout;
// OBSOLETE   struct section_offsets *offs;
// OBSOLETE   CORE_ADDR addr, pc;
// OBSOLETE   struct obj_section *obj_sec;
// OBSOLETE 
// OBSOLETE   if (monitor_log)
// OBSOLETE     fputs ("\nIn wait ()", log_file);
// OBSOLETE 
// OBSOLETE   status->kind = TARGET_WAITKIND_EXITED;
// OBSOLETE   status->value.integer = 0;
// OBSOLETE 
// OBSOLETE   timeout = -1;			/* Don't time out -- user program is running. */
// OBSOLETE   expect ("eax:", 0);		/* output any message before register display */
// OBSOLETE   expect_prompt (1);		/* Wait for prompt, outputting extraneous text */
// OBSOLETE 
// OBSOLETE   status->kind = TARGET_WAITKIND_STOPPED;
// OBSOLETE   status->value.sig = TARGET_SIGNAL_TRAP;
// OBSOLETE   timeout = old_timeout;
// OBSOLETE   rombug_fetch_registers ();
// OBSOLETE   bufaddr = 0;
// OBSOLETE   buflen = 0;
// OBSOLETE   pc = read_register (PC_REGNUM);
// OBSOLETE   addr = read_register (DATABASE_REG);
// OBSOLETE   obj_sec = find_pc_section (pc);
// OBSOLETE   if (obj_sec != NULL)
// OBSOLETE     {
// OBSOLETE       if (obj_sec->objfile != symfile_objfile)
// OBSOLETE 	new_symfile_objfile (obj_sec->objfile, 1, 0);
// OBSOLETE       offs = (struct section_offsets *) alloca (SIZEOF_SECTION_OFFSETS);
// OBSOLETE       memcpy (offs, symfile_objfile->section_offsets, SIZEOF_SECTION_OFFSETS);
// OBSOLETE       offs->offsets[SECT_OFF_DATA (symfile_objfile)]  = addr;
// OBSOLETE       offs->offsets[SECT_OFF_BSS (symfile_objfile)]  = addr;
// OBSOLETE 
// OBSOLETE       objfile_relocate (symfile_objfile, offs);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return inferior_ptid;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Return the name of register number regno in the form input and output by
// OBSOLETE    monitor.  Currently, register_names just happens to contain exactly what
// OBSOLETE    monitor wants.  Lets take advantage of that just as long as possible! */
// OBSOLETE 
// OBSOLETE static char *
// OBSOLETE get_reg_name (int regno)
// OBSOLETE {
// OBSOLETE   static char buf[50];
// OBSOLETE   char *p;
// OBSOLETE   char *b;
// OBSOLETE 
// OBSOLETE   b = buf;
// OBSOLETE 
// OBSOLETE   if (regno < 0)
// OBSOLETE     return ("");
// OBSOLETE /*
// OBSOLETE    for (p = REGISTER_NAME (regno); *p; p++)
// OBSOLETE    *b++ = toupper(*p);
// OBSOLETE    *b = '\000';
// OBSOLETE  */
// OBSOLETE   p = (char *) REGISTER_NAME (regno);
// OBSOLETE   return p;
// OBSOLETE /*
// OBSOLETE    return buf;
// OBSOLETE  */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* read the remote registers into the block regs.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE rombug_fetch_registers (void)
// OBSOLETE {
// OBSOLETE   int regno, j, i;
// OBSOLETE   long val;
// OBSOLETE   unsigned char b;
// OBSOLETE 
// OBSOLETE   printf_monitor (GET_REG);
// OBSOLETE   expect ("eax:", 1);
// OBSOLETE   expect ("\n", 1);
// OBSOLETE   get_hex_regs (1, 0);
// OBSOLETE   get_hex_regs (1, 3);
// OBSOLETE   get_hex_regs (1, 1);
// OBSOLETE   get_hex_regs (1, 2);
// OBSOLETE   get_hex_regs (1, 6);
// OBSOLETE   get_hex_regs (1, 7);
// OBSOLETE   get_hex_regs (1, 5);
// OBSOLETE   get_hex_regs (1, 4);
// OBSOLETE   for (regno = 8; regno <= 15; regno++)
// OBSOLETE     {
// OBSOLETE       expect (REG_DELIM, 1);
// OBSOLETE       if (regno >= 8 && regno <= 13)
// OBSOLETE 	{
// OBSOLETE 	  val = 0;
// OBSOLETE 	  for (j = 0; j < 2; j++)
// OBSOLETE 	    {
// OBSOLETE 	      get_hex_byte (&b);
// OBSOLETE 	      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
// OBSOLETE 		val = (val << 8) + b;
// OBSOLETE 	      else
// OBSOLETE 		val = val + (b << (j * 8));
// OBSOLETE 	    }
// OBSOLETE 
// OBSOLETE 	  if (regno == 8)
// OBSOLETE 	    i = 10;
// OBSOLETE 	  if (regno >= 9 && regno <= 12)
// OBSOLETE 	    i = regno + 3;
// OBSOLETE 	  if (regno == 13)
// OBSOLETE 	    i = 11;
// OBSOLETE 	  supply_register (i, (char *) &val);
// OBSOLETE 	}
// OBSOLETE       else if (regno == 14)
// OBSOLETE 	{
// OBSOLETE 	  get_hex_regs (1, PC_REGNUM);
// OBSOLETE 	}
// OBSOLETE       else if (regno == 15)
// OBSOLETE 	{
// OBSOLETE 	  get_hex_regs (1, 9);
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  val = 0;
// OBSOLETE 	  supply_register (regno, (char *) &val);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   is_trace_mode = 0;
// OBSOLETE   expect_prompt (1);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Fetch register REGNO, or all registers if REGNO is -1.
// OBSOLETE    Returns errno value.  */
// OBSOLETE static void
// OBSOLETE rombug_fetch_register (int regno)
// OBSOLETE {
// OBSOLETE   int val, j;
// OBSOLETE   unsigned char b;
// OBSOLETE 
// OBSOLETE   if (monitor_log)
// OBSOLETE     {
// OBSOLETE       fprintf (log_file, "\nIn Fetch Register (reg=%s)\n", get_reg_name (regno));
// OBSOLETE       fflush (log_file);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (regno < 0)
// OBSOLETE     {
// OBSOLETE       rombug_fetch_registers ();
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       char *name = get_reg_name (regno);
// OBSOLETE       printf_monitor (GET_REG);
// OBSOLETE       if (regno >= 10 && regno <= 15)
// OBSOLETE 	{
// OBSOLETE 	  expect ("\n", 1);
// OBSOLETE 	  expect ("\n", 1);
// OBSOLETE 	  expect (name, 1);
// OBSOLETE 	  expect (REG_DELIM, 1);
// OBSOLETE 	  val = 0;
// OBSOLETE 	  for (j = 0; j < 2; j++)
// OBSOLETE 	    {
// OBSOLETE 	      get_hex_byte (&b);
// OBSOLETE 	      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
// OBSOLETE 		val = (val << 8) + b;
// OBSOLETE 	      else
// OBSOLETE 		val = val + (b << (j * 8));
// OBSOLETE 	    }
// OBSOLETE 	  supply_register (regno, (char *) &val);
// OBSOLETE 	}
// OBSOLETE       else if (regno == 8 || regno == 9)
// OBSOLETE 	{
// OBSOLETE 	  expect ("\n", 1);
// OBSOLETE 	  expect ("\n", 1);
// OBSOLETE 	  expect ("\n", 1);
// OBSOLETE 	  expect (name, 1);
// OBSOLETE 	  expect (REG_DELIM, 1);
// OBSOLETE 	  get_hex_regs (1, regno);
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  expect (name, 1);
// OBSOLETE 	  expect (REG_DELIM, 1);
// OBSOLETE 	  expect ("\n", 1);
// OBSOLETE 	  get_hex_regs (1, 0);
// OBSOLETE 	  get_hex_regs (1, 3);
// OBSOLETE 	  get_hex_regs (1, 1);
// OBSOLETE 	  get_hex_regs (1, 2);
// OBSOLETE 	  get_hex_regs (1, 6);
// OBSOLETE 	  get_hex_regs (1, 7);
// OBSOLETE 	  get_hex_regs (1, 5);
// OBSOLETE 	  get_hex_regs (1, 4);
// OBSOLETE 	}
// OBSOLETE       expect_prompt (1);
// OBSOLETE     }
// OBSOLETE   return;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Store the remote registers from the contents of the block REGS.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE rombug_store_registers (void)
// OBSOLETE {
// OBSOLETE   int regno;
// OBSOLETE 
// OBSOLETE   for (regno = 0; regno <= PC_REGNUM; regno++)
// OBSOLETE     rombug_store_register (regno);
// OBSOLETE 
// OBSOLETE   registers_changed ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Store register REGNO, or all if REGNO == 0.
// OBSOLETE    return errno value.  */
// OBSOLETE static void
// OBSOLETE rombug_store_register (int regno)
// OBSOLETE {
// OBSOLETE   char *name;
// OBSOLETE 
// OBSOLETE   if (monitor_log)
// OBSOLETE     fprintf (log_file, "\nIn Store_register (regno=%d)\n", regno);
// OBSOLETE 
// OBSOLETE   if (regno == -1)
// OBSOLETE     rombug_store_registers ();
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       if (sr_get_debug ())
// OBSOLETE 	printf ("Setting register %s to 0x%x\n", get_reg_name (regno), read_register (regno));
// OBSOLETE 
// OBSOLETE       name = get_reg_name (regno);
// OBSOLETE       if (name == 0)
// OBSOLETE 	return;
// OBSOLETE       printf_monitor (SET_REG, name, read_register (regno));
// OBSOLETE 
// OBSOLETE       is_trace_mode = 0;
// OBSOLETE       expect_prompt (1);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get ready to modify the registers array.  On machines which store
// OBSOLETE    individual registers, this doesn't need to do anything.  On machines
// OBSOLETE    which store all the registers in one fell swoop, this makes sure
// OBSOLETE    that registers contains all the registers from the program being
// OBSOLETE    debugged.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE rombug_prepare_to_store (void)
// OBSOLETE {
// OBSOLETE   /* Do nothing, since we can store individual regs */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE rombug_files_info (void)
// OBSOLETE {
// OBSOLETE   printf ("\tAttached to %s at %d baud.\n",
// OBSOLETE 	  dev_name, sr_get_baud_rate ());
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Copy LEN bytes of data from debugger memory at MYADDR
// OBSOLETE    to inferior's memory at MEMADDR.  Returns length moved.  */
// OBSOLETE static int
// OBSOLETE rombug_write_inferior_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE   char buf[10];
// OBSOLETE 
// OBSOLETE   if (monitor_log)
// OBSOLETE     fprintf (log_file, "\nIn Write_inferior_memory (memaddr=%x, len=%d)\n", memaddr, len);
// OBSOLETE 
// OBSOLETE   printf_monitor (MEM_SET_CMD, memaddr);
// OBSOLETE   for (i = 0; i < len; i++)
// OBSOLETE     {
// OBSOLETE       expect (CMD_DELIM, 1);
// OBSOLETE       printf_monitor ("%x \r", myaddr[i]);
// OBSOLETE       if (sr_get_debug ())
// OBSOLETE 	printf ("\nSet 0x%x to 0x%x\n", memaddr + i, myaddr[i]);
// OBSOLETE     }
// OBSOLETE   expect (CMD_DELIM, 1);
// OBSOLETE   if (CMD_END)
// OBSOLETE     printf_monitor (CMD_END);
// OBSOLETE   is_trace_mode = 0;
// OBSOLETE   expect_prompt (1);
// OBSOLETE 
// OBSOLETE   bufaddr = 0;
// OBSOLETE   buflen = 0;
// OBSOLETE   return len;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Read LEN bytes from inferior memory at MEMADDR.  Put the result
// OBSOLETE    at debugger address MYADDR.  Returns length moved.  */
// OBSOLETE static int
// OBSOLETE rombug_read_inferior_memory (CORE_ADDR memaddr, char *myaddr, int len)
// OBSOLETE {
// OBSOLETE   int i, j;
// OBSOLETE 
// OBSOLETE   /* Number of bytes read so far.  */
// OBSOLETE   int count;
// OBSOLETE 
// OBSOLETE   /* Starting address of this pass.  */
// OBSOLETE   unsigned long startaddr;
// OBSOLETE 
// OBSOLETE   /* Number of bytes to read in this pass.  */
// OBSOLETE   int len_this_pass;
// OBSOLETE 
// OBSOLETE   if (monitor_log)
// OBSOLETE     fprintf (log_file, "\nIn Read_inferior_memory (memaddr=%x, len=%d)\n", memaddr, len);
// OBSOLETE 
// OBSOLETE   /* Note that this code works correctly if startaddr is just less
// OBSOLETE      than UINT_MAX (well, really CORE_ADDR_MAX if there was such a
// OBSOLETE      thing).  That is, something like
// OBSOLETE      rombug_read_bytes (CORE_ADDR_MAX - 4, foo, 4)
// OBSOLETE      works--it never adds len To memaddr and gets 0.  */
// OBSOLETE   /* However, something like
// OBSOLETE      rombug_read_bytes (CORE_ADDR_MAX - 3, foo, 4)
// OBSOLETE      doesn't need to work.  Detect it and give up if there's an attempt
// OBSOLETE      to do that.  */
// OBSOLETE   if (((memaddr - 1) + len) < memaddr)
// OBSOLETE     {
// OBSOLETE       errno = EIO;
// OBSOLETE       return 0;
// OBSOLETE     }
// OBSOLETE   if (bufaddr <= memaddr && (memaddr + len) <= (bufaddr + buflen))
// OBSOLETE     {
// OBSOLETE       memcpy (myaddr, &readbuf[memaddr - bufaddr], len);
// OBSOLETE       return len;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   startaddr = memaddr;
// OBSOLETE   count = 0;
// OBSOLETE   while (count < len)
// OBSOLETE     {
// OBSOLETE       len_this_pass = 16;
// OBSOLETE       if ((startaddr % 16) != 0)
// OBSOLETE 	len_this_pass -= startaddr % 16;
// OBSOLETE       if (len_this_pass > (len - count))
// OBSOLETE 	len_this_pass = (len - count);
// OBSOLETE       if (sr_get_debug ())
// OBSOLETE 	printf ("\nDisplay %d bytes at %x\n", len_this_pass, startaddr);
// OBSOLETE 
// OBSOLETE       printf_monitor (MEM_DIS_CMD, startaddr, 8);
// OBSOLETE       expect ("- ", 1);
// OBSOLETE       for (i = 0; i < 16; i++)
// OBSOLETE 	{
// OBSOLETE 	  get_hex_byte (&readbuf[i]);
// OBSOLETE 	}
// OBSOLETE       bufaddr = startaddr;
// OBSOLETE       buflen = 16;
// OBSOLETE       memcpy (&myaddr[count], readbuf, len_this_pass);
// OBSOLETE       count += len_this_pass;
// OBSOLETE       startaddr += len_this_pass;
// OBSOLETE       expect (CMD_DELIM, 1);
// OBSOLETE     }
// OBSOLETE   if (CMD_END)
// OBSOLETE     printf_monitor (CMD_END);
// OBSOLETE   is_trace_mode = 0;
// OBSOLETE   expect_prompt (1);
// OBSOLETE 
// OBSOLETE   return len;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Transfer LEN bytes between GDB address MYADDR and target address
// OBSOLETE    MEMADDR.  If WRITE is non-zero, transfer them to the target,
// OBSOLETE    otherwise transfer them from the target.  TARGET is unused.
// OBSOLETE 
// OBSOLETE    Returns the number of bytes transferred. */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE rombug_xfer_inferior_memory (CORE_ADDR memaddr, char *myaddr, int len,
// OBSOLETE 			     int write, struct mem_attrib *attrib,
// OBSOLETE 			     struct target_ops *target)
// OBSOLETE {
// OBSOLETE   if (write)
// OBSOLETE     return rombug_write_inferior_memory (memaddr, myaddr, len);
// OBSOLETE   else
// OBSOLETE     return rombug_read_inferior_memory (memaddr, myaddr, len);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE rombug_kill (char *args, int from_tty)
// OBSOLETE {
// OBSOLETE   return;			/* ignore attempts to kill target system */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Clean up when a program exits.
// OBSOLETE    The program actually lives on in the remote processor's RAM, and may be
// OBSOLETE    run again without a download.  Don't leave it full of breakpoint
// OBSOLETE    instructions.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE rombug_mourn_inferior (void)
// OBSOLETE {
// OBSOLETE   remove_breakpoints ();
// OBSOLETE   generic_mourn_inferior ();	/* Do all the proper things now */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #define MAX_MONITOR_BREAKPOINTS 16
// OBSOLETE 
// OBSOLETE static CORE_ADDR breakaddr[MAX_MONITOR_BREAKPOINTS] =
// OBSOLETE {0};
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE rombug_insert_breakpoint (CORE_ADDR addr, char *shadow)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE   CORE_ADDR bp_addr = addr;
// OBSOLETE   int bp_size = 0;
// OBSOLETE 
// OBSOLETE   if (monitor_log)
// OBSOLETE     fprintf (log_file, "\nIn Insert_breakpoint (addr=%x)\n", addr);
// OBSOLETE   BREAKPOINT_FROM_PC (&bp_addr, &bp_size);
// OBSOLETE 
// OBSOLETE   for (i = 0; i <= MAX_MONITOR_BREAKPOINTS; i++)
// OBSOLETE     if (breakaddr[i] == 0)
// OBSOLETE       {
// OBSOLETE 	breakaddr[i] = addr;
// OBSOLETE 	if (sr_get_debug ())
// OBSOLETE 	  printf ("Breakpoint at %x\n", addr);
// OBSOLETE 	rombug_read_inferior_memory (bp_addr, shadow, bp_size);
// OBSOLETE 	printf_monitor (SET_BREAK_CMD, addr);
// OBSOLETE 	is_trace_mode = 0;
// OBSOLETE 	expect_prompt (1);
// OBSOLETE 	return 0;
// OBSOLETE       }
// OBSOLETE 
// OBSOLETE   fprintf_unfiltered (gdb_stderr, "Too many breakpoints (> 16) for monitor\n");
// OBSOLETE   return 1;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE  * _remove_breakpoint -- Tell the monitor to remove a breakpoint
// OBSOLETE  */
// OBSOLETE static int
// OBSOLETE rombug_remove_breakpoint (CORE_ADDR addr, char *shadow)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE 
// OBSOLETE   if (monitor_log)
// OBSOLETE     fprintf (log_file, "\nIn Remove_breakpoint (addr=%x)\n", addr);
// OBSOLETE 
// OBSOLETE   for (i = 0; i < MAX_MONITOR_BREAKPOINTS; i++)
// OBSOLETE     if (breakaddr[i] == addr)
// OBSOLETE       {
// OBSOLETE 	breakaddr[i] = 0;
// OBSOLETE 	printf_monitor (CLR_BREAK_CMD, addr);
// OBSOLETE 	is_trace_mode = 0;
// OBSOLETE 	expect_prompt (1);
// OBSOLETE 	return 0;
// OBSOLETE       }
// OBSOLETE 
// OBSOLETE   fprintf_unfiltered (gdb_stderr,
// OBSOLETE 		      "Can't find breakpoint associated with 0x%x\n", addr);
// OBSOLETE   return 1;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Load a file. This is usually an srecord, which is ascii. No 
// OBSOLETE    protocol, just sent line by line. */
// OBSOLETE 
// OBSOLETE #define DOWNLOAD_LINE_SIZE 100
// OBSOLETE static void
// OBSOLETE rombug_load (char *arg)
// OBSOLETE {
// OBSOLETE /* this part comment out for os9* */
// OBSOLETE #if 0
// OBSOLETE   FILE *download;
// OBSOLETE   char buf[DOWNLOAD_LINE_SIZE];
// OBSOLETE   int i, bytes_read;
// OBSOLETE 
// OBSOLETE   if (sr_get_debug ())
// OBSOLETE     printf ("Loading %s to monitor\n", arg);
// OBSOLETE 
// OBSOLETE   download = fopen (arg, "r");
// OBSOLETE   if (download == NULL)
// OBSOLETE     {
// OBSOLETE       error (sprintf (buf, "%s Does not exist", arg));
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   printf_monitor (LOAD_CMD);
// OBSOLETE /*  expect ("Waiting for S-records from host... ", 1); */
// OBSOLETE 
// OBSOLETE   while (!feof (download))
// OBSOLETE     {
// OBSOLETE       bytes_read = fread (buf, sizeof (char), DOWNLOAD_LINE_SIZE, download);
// OBSOLETE       if (hashmark)
// OBSOLETE 	{
// OBSOLETE 	  putchar ('.');
// OBSOLETE 	  fflush (stdout);
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       if (serial_write (monitor_desc, buf, bytes_read))
// OBSOLETE 	{
// OBSOLETE 	  fprintf_unfiltered (gdb_stderr,
// OBSOLETE 			      "serial_write failed: (while downloading) %s\n",
// OBSOLETE 			      safe_strerror (errno));
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE       i = 0;
// OBSOLETE       while (i++ <= 200000)
// OBSOLETE 	{
// OBSOLETE 	};			/* Ugly HACK, probably needs flow control */
// OBSOLETE       if (bytes_read < DOWNLOAD_LINE_SIZE)
// OBSOLETE 	{
// OBSOLETE 	  if (!feof (download))
// OBSOLETE 	    error ("Only read %d bytes\n", bytes_read);
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (hashmark)
// OBSOLETE     {
// OBSOLETE       putchar ('\n');
// OBSOLETE     }
// OBSOLETE   if (!feof (download))
// OBSOLETE     error ("Never got EOF while downloading");
// OBSOLETE   fclose (download);
// OBSOLETE #endif /* 0 */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Put a command string, in args, out to MONITOR.  
// OBSOLETE    Output from MONITOR is placed on the users terminal until the prompt 
// OBSOLETE    is seen. */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE rombug_command (char *args, int fromtty)
// OBSOLETE {
// OBSOLETE   if (monitor_desc == NULL)
// OBSOLETE     error ("monitor target not open.");
// OBSOLETE 
// OBSOLETE   if (monitor_log)
// OBSOLETE     fprintf (log_file, "\nIn command (args=%s)\n", args);
// OBSOLETE 
// OBSOLETE   if (!args)
// OBSOLETE     error ("Missing command.");
// OBSOLETE 
// OBSOLETE   printf_monitor ("%s\r", args);
// OBSOLETE   expect_prompt (0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #if 0
// OBSOLETE /* Connect the user directly to MONITOR.  This command acts just like the
// OBSOLETE    'cu' or 'tip' command.  Use <CR>~. or <CR>~^D to break out.  */
// OBSOLETE 
// OBSOLETE static struct ttystate ttystate;
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE cleanup_tty (void)
// OBSOLETE {
// OBSOLETE   printf ("\r\n[Exiting connect mode]\r\n");
// OBSOLETE   /*serial_restore(0, &ttystate); */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE connect_command (char *args, int fromtty)
// OBSOLETE {
// OBSOLETE   fd_set readfds;
// OBSOLETE   int numfds;
// OBSOLETE   int c;
// OBSOLETE   char cur_esc = 0;
// OBSOLETE 
// OBSOLETE   dont_repeat ();
// OBSOLETE 
// OBSOLETE   if (monitor_desc == NULL)
// OBSOLETE     error ("monitor target not open.");
// OBSOLETE 
// OBSOLETE   if (args)
// OBSOLETE     fprintf ("This command takes no args.  They have been ignored.\n");
// OBSOLETE 
// OBSOLETE   printf ("[Entering connect mode.  Use ~. or ~^D to escape]\n");
// OBSOLETE 
// OBSOLETE   serial_raw (0, &ttystate);
// OBSOLETE 
// OBSOLETE   make_cleanup (cleanup_tty, 0);
// OBSOLETE 
// OBSOLETE   FD_ZERO (&readfds);
// OBSOLETE 
// OBSOLETE   while (1)
// OBSOLETE     {
// OBSOLETE       do
// OBSOLETE 	{
// OBSOLETE 	  FD_SET (0, &readfds);
// OBSOLETE 	  FD_SET (deprecated_serial_fd (monitor_desc), &readfds);
// OBSOLETE 	  numfds = select (sizeof (readfds) * 8, &readfds, 0, 0, 0);
// OBSOLETE 	}
// OBSOLETE       while (numfds == 0);
// OBSOLETE 
// OBSOLETE       if (numfds < 0)
// OBSOLETE 	perror_with_name ("select");
// OBSOLETE 
// OBSOLETE       if (FD_ISSET (0, &readfds))
// OBSOLETE 	{			/* tty input, send to monitor */
// OBSOLETE 	  c = getchar ();
// OBSOLETE 	  if (c < 0)
// OBSOLETE 	    perror_with_name ("connect");
// OBSOLETE 
// OBSOLETE 	  printf_monitor ("%c", c);
// OBSOLETE 	  switch (cur_esc)
// OBSOLETE 	    {
// OBSOLETE 	    case 0:
// OBSOLETE 	      if (c == '\r')
// OBSOLETE 		cur_esc = c;
// OBSOLETE 	      break;
// OBSOLETE 	    case '\r':
// OBSOLETE 	      if (c == '~')
// OBSOLETE 		cur_esc = c;
// OBSOLETE 	      else
// OBSOLETE 		cur_esc = 0;
// OBSOLETE 	      break;
// OBSOLETE 	    case '~':
// OBSOLETE 	      if (c == '.' || c == '\004')
// OBSOLETE 		return;
// OBSOLETE 	      else
// OBSOLETE 		cur_esc = 0;
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       if (FD_ISSET (deprecated_serial_fd (monitor_desc), &readfds))
// OBSOLETE 	{
// OBSOLETE 	  while (1)
// OBSOLETE 	    {
// OBSOLETE 	      c = readchar (0);
// OBSOLETE 	      if (c < 0)
// OBSOLETE 		break;
// OBSOLETE 	      putchar (c);
// OBSOLETE 	    }
// OBSOLETE 	  fflush (stdout);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE  * Define the monitor command strings. Since these are passed directly
// OBSOLETE  * through to a printf style function, we need can include formatting
// OBSOLETE  * strings. We also need a CR or LF on the end.
// OBSOLETE  */
// OBSOLETE #warning FIXME: monitor interface pattern strings, stale struct decl
// OBSOLETE struct monitor_ops rombug_cmds =
// OBSOLETE {
// OBSOLETE   "g \r",			/* execute or usually GO command */
// OBSOLETE   "g \r",			/* continue command */
// OBSOLETE   "t \r",			/* single step */
// OBSOLETE   "b %x\r",			/* set a breakpoint */
// OBSOLETE   "k %x\r",			/* clear a breakpoint */
// OBSOLETE   "c %x\r",			/* set memory to a value */
// OBSOLETE   "d %x %d\r",			/* display memory */
// OBSOLETE   "$%08X",			/* prompt memory commands use */
// OBSOLETE   ".%s %x\r",			/* set a register */
// OBSOLETE   ":",				/* delimiter between registers */
// OBSOLETE   ". \r",			/* read a register */
// OBSOLETE   "mf \r",			/* download command */
// OBSOLETE   "RomBug: ",			/* monitor command prompt */
// OBSOLETE   ": ",				/* end-of-command delimitor */
// OBSOLETE   ".\r"				/* optional command terminator */
// OBSOLETE };
// OBSOLETE 
// OBSOLETE struct target_ops rombug_ops;
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE init_rombug_ops (void)
// OBSOLETE {
// OBSOLETE   rombug_ops.to_shortname = "rombug";
// OBSOLETE   rombug_ops.to_longname = "Microware's ROMBUG debug monitor";
// OBSOLETE   rombug_ops.to_doc = "Use a remote computer running the ROMBUG debug monitor.\n\
// OBSOLETE Specify the serial device it is connected to (e.g. /dev/ttya).",
// OBSOLETE     rombug_ops.to_open = rombug_open;
// OBSOLETE   rombug_ops.to_close = rombug_close;
// OBSOLETE   rombug_ops.to_attach = 0;
// OBSOLETE   rombug_ops.to_post_attach = NULL;
// OBSOLETE   rombug_ops.to_require_attach = NULL;
// OBSOLETE   rombug_ops.to_detach = rombug_detach;
// OBSOLETE   rombug_ops.to_require_detach = NULL;
// OBSOLETE   rombug_ops.to_resume = rombug_resume;
// OBSOLETE   rombug_ops.to_wait = rombug_wait;
// OBSOLETE   rombug_ops.to_post_wait = NULL;
// OBSOLETE   rombug_ops.to_fetch_registers = rombug_fetch_register;
// OBSOLETE   rombug_ops.to_store_registers = rombug_store_register;
// OBSOLETE   rombug_ops.to_prepare_to_store = rombug_prepare_to_store;
// OBSOLETE   rombug_ops.to_xfer_memory = rombug_xfer_inferior_memory;
// OBSOLETE   rombug_ops.to_files_info = rombug_files_info;
// OBSOLETE   rombug_ops.to_insert_breakpoint = rombug_insert_breakpoint;
// OBSOLETE   rombug_ops.to_remove_breakpoint = rombug_remove_breakpoint;	/* Breakpoints */
// OBSOLETE   rombug_ops.to_terminal_init = 0;
// OBSOLETE   rombug_ops.to_terminal_inferior = 0;
// OBSOLETE   rombug_ops.to_terminal_ours_for_output = 0;
// OBSOLETE   rombug_ops.to_terminal_ours = 0;
// OBSOLETE   rombug_ops.to_terminal_info = 0;	/* Terminal handling */
// OBSOLETE   rombug_ops.to_kill = rombug_kill;
// OBSOLETE   rombug_ops.to_load = rombug_load;	/* load */
// OBSOLETE   rombug_ops.to_lookup_symbol = rombug_link;	/* lookup_symbol */
// OBSOLETE   rombug_ops.to_create_inferior = rombug_create_inferior;
// OBSOLETE   rombug_ops.to_post_startup_inferior = NULL;
// OBSOLETE   rombug_ops.to_acknowledge_created_inferior = NULL;
// OBSOLETE   rombug_ops.to_clone_and_follow_inferior = NULL;
// OBSOLETE   rombug_ops.to_post_follow_inferior_by_clone = NULL;
// OBSOLETE   rombug_ops.to_insert_fork_catchpoint = NULL;
// OBSOLETE   rombug_ops.to_remove_fork_catchpoint = NULL;
// OBSOLETE   rombug_ops.to_insert_vfork_catchpoint = NULL;
// OBSOLETE   rombug_ops.to_remove_vfork_catchpoint = NULL;
// OBSOLETE   rombug_ops.to_has_forked = NULL;
// OBSOLETE   rombug_ops.to_has_vforked = NULL;
// OBSOLETE   rombug_ops.to_can_follow_vfork_prior_to_exec = NULL;
// OBSOLETE   rombug_ops.to_post_follow_vfork = NULL;
// OBSOLETE   rombug_ops.to_insert_exec_catchpoint = NULL;
// OBSOLETE   rombug_ops.to_remove_exec_catchpoint = NULL;
// OBSOLETE   rombug_ops.to_has_execd = NULL;
// OBSOLETE   rombug_ops.to_reported_exec_events_per_exec_call = NULL;
// OBSOLETE   rombug_ops.to_has_exited = NULL;
// OBSOLETE   rombug_ops.to_mourn_inferior = rombug_mourn_inferior;
// OBSOLETE   rombug_ops.to_can_run = 0;	/* can_run */
// OBSOLETE   rombug_ops.to_notice_signals = 0;	/* notice_signals */
// OBSOLETE   rombug_ops.to_thread_alive = 0;
// OBSOLETE   rombug_ops.to_stop = 0;	/* to_stop */
// OBSOLETE   rombug_ops.to_pid_to_exec_file = NULL;
// OBSOLETE   rombug_ops.to_stratum = process_stratum;
// OBSOLETE   rombug_ops.DONT_USE = 0;	/* next */
// OBSOLETE   rombug_ops.to_has_all_memory = 1;
// OBSOLETE   rombug_ops.to_has_memory = 1;
// OBSOLETE   rombug_ops.to_has_stack = 1;
// OBSOLETE   rombug_ops.to_has_registers = 1;
// OBSOLETE   rombug_ops.to_has_execution = 1;	/* has execution */
// OBSOLETE   rombug_ops.to_sections = 0;
// OBSOLETE   rombug_ops.to_sections_end = 0;	/* Section pointers */
// OBSOLETE   rombug_ops.to_magic = OPS_MAGIC;	/* Always the last thing */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_remote_os9k (void)
// OBSOLETE {
// OBSOLETE   init_rombug_ops ();
// OBSOLETE   add_target (&rombug_ops);
// OBSOLETE 
// OBSOLETE   add_show_from_set (
// OBSOLETE 	     add_set_cmd ("hash", no_class, var_boolean, (char *) &hashmark,
// OBSOLETE 			  "Set display of activity while downloading a file.\nWhen enabled, a period \'.\' is displayed.",
// OBSOLETE 			  &setlist),
// OBSOLETE 		      &showlist);
// OBSOLETE 
// OBSOLETE   add_show_from_set (
// OBSOLETE 		      add_set_cmd ("timeout", no_class, var_zinteger,
// OBSOLETE 				   (char *) &timeout,
// OBSOLETE 		       "Set timeout in seconds for remote MIPS serial I/O.",
// OBSOLETE 				   &setlist),
// OBSOLETE 		      &showlist);
// OBSOLETE 
// OBSOLETE   add_show_from_set (
// OBSOLETE 		      add_set_cmd ("remotelog", no_class, var_zinteger,
// OBSOLETE 				   (char *) &monitor_log,
// OBSOLETE 			      "Set monitor activity log on(=1) or off(=0).",
// OBSOLETE 				   &setlist),
// OBSOLETE 		      &showlist);
// OBSOLETE 
// OBSOLETE   add_show_from_set (
// OBSOLETE 		      add_set_cmd ("remotexon", no_class, var_zinteger,
// OBSOLETE 				   (char *) &tty_xon,
// OBSOLETE 				   "Set remote tty line XON control",
// OBSOLETE 				   &setlist),
// OBSOLETE 		      &showlist);
// OBSOLETE 
// OBSOLETE   add_show_from_set (
// OBSOLETE 		      add_set_cmd ("remotexoff", no_class, var_zinteger,
// OBSOLETE 				   (char *) &tty_xoff,
// OBSOLETE 				   "Set remote tty line XOFF control",
// OBSOLETE 				   &setlist),
// OBSOLETE 		      &showlist);
// OBSOLETE 
// OBSOLETE   add_com ("rombug <command>", class_obscure, rombug_command,
// OBSOLETE 	   "Send a command to the debug monitor.");
// OBSOLETE #if 0
// OBSOLETE   add_com ("connect", class_obscure, connect_command,
// OBSOLETE 	   "Connect the terminal directly up to a serial based command monitor.\nUse <CR>~. or <CR>~^D to break out.");
// OBSOLETE #endif
// OBSOLETE }
