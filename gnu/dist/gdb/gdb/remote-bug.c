// OBSOLETE /* Remote debugging interface for Motorola's MVME187BUG monitor, an embedded
// OBSOLETE    monitor for the m88k.
// OBSOLETE 
// OBSOLETE    Copyright 1992, 1993, 1994, 1995, 1996, 1998, 1999, 2000, 2001,
// OBSOLETE    2002 Free Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE    Contributed by Cygnus Support.  Written by K. Richard Pixley.
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
// OBSOLETE #include "defs.h"
// OBSOLETE #include "inferior.h"
// OBSOLETE #include "gdb_string.h"
// OBSOLETE #include "regcache.h"
// OBSOLETE #include <ctype.h>
// OBSOLETE #include <fcntl.h>
// OBSOLETE #include <setjmp.h>
// OBSOLETE #include <errno.h>
// OBSOLETE 
// OBSOLETE #include "terminal.h"
// OBSOLETE #include "gdbcore.h"
// OBSOLETE #include "gdbcmd.h"
// OBSOLETE 
// OBSOLETE #include "serial.h"
// OBSOLETE #include "remote-utils.h"
// OBSOLETE 
// OBSOLETE /* External data declarations */
// OBSOLETE extern int stop_soon_quietly;	/* for wait_for_inferior */
// OBSOLETE 
// OBSOLETE /* Forward data declarations */
// OBSOLETE extern struct target_ops bug_ops;	/* Forward declaration */
// OBSOLETE 
// OBSOLETE /* Forward function declarations */
// OBSOLETE static int bug_clear_breakpoints (void);
// OBSOLETE 
// OBSOLETE static int bug_read_memory (CORE_ADDR memaddr,
// OBSOLETE 			    unsigned char *myaddr, int len);
// OBSOLETE 
// OBSOLETE static int bug_write_memory (CORE_ADDR memaddr,
// OBSOLETE 			     unsigned char *myaddr, int len);
// OBSOLETE 
// OBSOLETE /* This variable is somewhat arbitrary.  It's here so that it can be
// OBSOLETE    set from within a running gdb.  */
// OBSOLETE 
// OBSOLETE static int srec_max_retries = 3;
// OBSOLETE 
// OBSOLETE /* Each S-record download to the target consists of an S0 header
// OBSOLETE    record, some number of S3 data records, and one S7 termination
// OBSOLETE    record.  I call this download a "frame".  Srec_frame says how many
// OBSOLETE    bytes will be represented in each frame.  */
// OBSOLETE 
// OBSOLETE #define SREC_SIZE 160
// OBSOLETE static int srec_frame = SREC_SIZE;
// OBSOLETE 
// OBSOLETE /* This variable determines how many bytes will be represented in each
// OBSOLETE    S3 s-record.  */
// OBSOLETE 
// OBSOLETE static int srec_bytes = 40;
// OBSOLETE 
// OBSOLETE /* At one point it appeared to me as though the bug monitor could not
// OBSOLETE    really be expected to receive two sequential characters at 9600
// OBSOLETE    baud reliably.  Echo-pacing is an attempt to force data across the
// OBSOLETE    line even in this condition.  Specifically, in echo-pace mode, each
// OBSOLETE    character is sent one at a time and we look for the echo before
// OBSOLETE    sending the next.  This is excruciatingly slow.  */
// OBSOLETE 
// OBSOLETE static int srec_echo_pace = 0;
// OBSOLETE 
// OBSOLETE /* How long to wait after an srec for a possible error message.
// OBSOLETE    Similar to the above, I tried sleeping after sending each S3 record
// OBSOLETE    in hopes that I might actually see error messages from the bug
// OBSOLETE    monitor.  This might actually work if we were to use sleep
// OBSOLETE    intervals smaller than 1 second.  */
// OBSOLETE 
// OBSOLETE static int srec_sleep = 0;
// OBSOLETE 
// OBSOLETE /* Every srec_noise records, flub the checksum.  This is a debugging
// OBSOLETE    feature.  Set the variable to something other than 1 in order to
// OBSOLETE    inject *deliberate* checksum errors.  One might do this if one
// OBSOLETE    wanted to test error handling and recovery.  */
// OBSOLETE 
// OBSOLETE static int srec_noise = 0;
// OBSOLETE 
// OBSOLETE /* Called when SIGALRM signal sent due to alarm() timeout.  */
// OBSOLETE 
// OBSOLETE /* Number of SIGTRAPs we need to simulate.  That is, the next
// OBSOLETE    NEED_ARTIFICIAL_TRAP calls to bug_wait should just return
// OBSOLETE    SIGTRAP without actually waiting for anything.  */
// OBSOLETE 
// OBSOLETE static int need_artificial_trap = 0;
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE  * Download a file specified in 'args', to the bug.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE bug_load (char *args, int fromtty)
// OBSOLETE {
// OBSOLETE   bfd *abfd;
// OBSOLETE   asection *s;
// OBSOLETE   char buffer[1024];
// OBSOLETE 
// OBSOLETE   sr_check_open ();
// OBSOLETE 
// OBSOLETE   inferior_ptid = null_ptid;
// OBSOLETE   abfd = bfd_openr (args, 0);
// OBSOLETE   if (!abfd)
// OBSOLETE     {
// OBSOLETE       printf_filtered ("Unable to open file %s\n", args);
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (bfd_check_format (abfd, bfd_object) == 0)
// OBSOLETE     {
// OBSOLETE       printf_filtered ("File is not an object file\n");
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   s = abfd->sections;
// OBSOLETE   while (s != (asection *) NULL)
// OBSOLETE     {
// OBSOLETE       srec_frame = SREC_SIZE;
// OBSOLETE       if (s->flags & SEC_LOAD)
// OBSOLETE 	{
// OBSOLETE 	  int i;
// OBSOLETE 
// OBSOLETE 	  char *buffer = xmalloc (srec_frame);
// OBSOLETE 
// OBSOLETE 	  printf_filtered ("%s\t: 0x%4lx .. 0x%4lx  ", s->name, s->vma, s->vma + s->_raw_size);
// OBSOLETE 	  gdb_flush (gdb_stdout);
// OBSOLETE 	  for (i = 0; i < s->_raw_size; i += srec_frame)
// OBSOLETE 	    {
// OBSOLETE 	      if (srec_frame > s->_raw_size - i)
// OBSOLETE 		srec_frame = s->_raw_size - i;
// OBSOLETE 
// OBSOLETE 	      bfd_get_section_contents (abfd, s, buffer, i, srec_frame);
// OBSOLETE 	      bug_write_memory (s->vma + i, buffer, srec_frame);
// OBSOLETE 	      printf_filtered ("*");
// OBSOLETE 	      gdb_flush (gdb_stdout);
// OBSOLETE 	    }
// OBSOLETE 	  printf_filtered ("\n");
// OBSOLETE 	  xfree (buffer);
// OBSOLETE 	}
// OBSOLETE       s = s->next;
// OBSOLETE     }
// OBSOLETE   sprintf (buffer, "rs ip %lx", (unsigned long) abfd->start_address);
// OBSOLETE   sr_write_cr (buffer);
// OBSOLETE   gr_expect_prompt ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #if 0
// OBSOLETE static char *
// OBSOLETE get_word (char **p)
// OBSOLETE {
// OBSOLETE   char *s = *p;
// OBSOLETE   char *word;
// OBSOLETE   char *copy;
// OBSOLETE   size_t len;
// OBSOLETE 
// OBSOLETE   while (isspace (*s))
// OBSOLETE     s++;
// OBSOLETE 
// OBSOLETE   word = s;
// OBSOLETE 
// OBSOLETE   len = 0;
// OBSOLETE 
// OBSOLETE   while (*s && !isspace (*s))
// OBSOLETE     {
// OBSOLETE       s++;
// OBSOLETE       len++;
// OBSOLETE 
// OBSOLETE     }
// OBSOLETE   copy = xmalloc (len + 1);
// OBSOLETE   memcpy (copy, word, len);
// OBSOLETE   copy[len] = 0;
// OBSOLETE   *p = s;
// OBSOLETE   return copy;
// OBSOLETE }
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE static struct gr_settings bug_settings =
// OBSOLETE {
// OBSOLETE   "Bug>",			/* prompt */
// OBSOLETE   &bug_ops,			/* ops */
// OBSOLETE   bug_clear_breakpoints,	/* clear_all_breakpoints */
// OBSOLETE   gr_generic_checkin,		/* checkin */
// OBSOLETE };
// OBSOLETE 
// OBSOLETE static char *cpu_check_strings[] =
// OBSOLETE {
// OBSOLETE   "=",
// OBSOLETE   "Invalid Register",
// OBSOLETE };
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE bug_open (char *args, int from_tty)
// OBSOLETE {
// OBSOLETE   if (args == NULL)
// OBSOLETE     args = "";
// OBSOLETE 
// OBSOLETE   gr_open (args, from_tty, &bug_settings);
// OBSOLETE   /* decide *now* whether we are on an 88100 or an 88110 */
// OBSOLETE   sr_write_cr ("rs cr06");
// OBSOLETE   sr_expect ("rs cr06");
// OBSOLETE 
// OBSOLETE   switch (gr_multi_scan (cpu_check_strings, 0))
// OBSOLETE     {
// OBSOLETE     case 0:			/* this is an m88100 */
// OBSOLETE       target_is_m88110 = 0;
// OBSOLETE       break;
// OBSOLETE     case 1:			/* this is an m88110 */
// OBSOLETE       target_is_m88110 = 1;
// OBSOLETE       break;
// OBSOLETE     default:
// OBSOLETE       internal_error (__FILE__, __LINE__, "failed internal consistency check");
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Tell the remote machine to resume.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE bug_resume (ptid_t ptid, int step, enum target_signal sig)
// OBSOLETE {
// OBSOLETE   if (step)
// OBSOLETE     {
// OBSOLETE       sr_write_cr ("t");
// OBSOLETE 
// OBSOLETE       /* Force the next bug_wait to return a trap.  Not doing anything
// OBSOLETE          about I/O from the target means that the user has to type
// OBSOLETE          "continue" to see any.  FIXME, this should be fixed.  */
// OBSOLETE       need_artificial_trap = 1;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     sr_write_cr ("g");
// OBSOLETE 
// OBSOLETE   return;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Wait until the remote machine stops, then return,
// OBSOLETE    storing status in STATUS just as `wait' would.  */
// OBSOLETE 
// OBSOLETE static char *wait_strings[] =
// OBSOLETE {
// OBSOLETE   "At Breakpoint",
// OBSOLETE   "Exception: Data Access Fault (Local Bus Timeout)",
// OBSOLETE   "\r8??\?-Bug>",		/* The '\?' avoids creating a trigraph */
// OBSOLETE   "\r197-Bug>",
// OBSOLETE   NULL,
// OBSOLETE };
// OBSOLETE 
// OBSOLETE ptid_t
// OBSOLETE bug_wait (ptid_t ptid, struct target_waitstatus *status)
// OBSOLETE {
// OBSOLETE   int old_timeout = sr_get_timeout ();
// OBSOLETE   int old_immediate_quit = immediate_quit;
// OBSOLETE 
// OBSOLETE   status->kind = TARGET_WAITKIND_EXITED;
// OBSOLETE   status->value.integer = 0;
// OBSOLETE 
// OBSOLETE   /* read off leftovers from resume so that the rest can be passed
// OBSOLETE      back out as stdout.  */
// OBSOLETE   if (need_artificial_trap == 0)
// OBSOLETE     {
// OBSOLETE       sr_expect ("Effective address: ");
// OBSOLETE       (void) sr_get_hex_word ();
// OBSOLETE       sr_expect ("\r\n");
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   sr_set_timeout (-1);		/* Don't time out -- user program is running. */
// OBSOLETE   immediate_quit = 1;		/* Helps ability to QUIT */
// OBSOLETE 
// OBSOLETE   switch (gr_multi_scan (wait_strings, need_artificial_trap == 0))
// OBSOLETE     {
// OBSOLETE     case 0:			/* breakpoint case */
// OBSOLETE       status->kind = TARGET_WAITKIND_STOPPED;
// OBSOLETE       status->value.sig = TARGET_SIGNAL_TRAP;
// OBSOLETE       /* user output from the target can be discarded here. (?) */
// OBSOLETE       gr_expect_prompt ();
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case 1:			/* bus error */
// OBSOLETE       status->kind = TARGET_WAITKIND_STOPPED;
// OBSOLETE       status->value.sig = TARGET_SIGNAL_BUS;
// OBSOLETE       /* user output from the target can be discarded here. (?) */
// OBSOLETE       gr_expect_prompt ();
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case 2:			/* normal case */
// OBSOLETE     case 3:
// OBSOLETE       if (need_artificial_trap != 0)
// OBSOLETE 	{
// OBSOLETE 	  /* stepping */
// OBSOLETE 	  status->kind = TARGET_WAITKIND_STOPPED;
// OBSOLETE 	  status->value.sig = TARGET_SIGNAL_TRAP;
// OBSOLETE 	  need_artificial_trap--;
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  /* exit case */
// OBSOLETE 	  status->kind = TARGET_WAITKIND_EXITED;
// OBSOLETE 	  status->value.integer = 0;
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE     case -1:			/* trouble */
// OBSOLETE     default:
// OBSOLETE       fprintf_filtered (gdb_stderr,
// OBSOLETE 			"Trouble reading target during wait\n");
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   sr_set_timeout (old_timeout);
// OBSOLETE   immediate_quit = old_immediate_quit;
// OBSOLETE   return inferior_ptid;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Return the name of register number REGNO
// OBSOLETE    in the form input and output by bug.
// OBSOLETE 
// OBSOLETE    Returns a pointer to a static buffer containing the answer.  */
// OBSOLETE static char *
// OBSOLETE get_reg_name (int regno)
// OBSOLETE {
// OBSOLETE   static char *rn[] =
// OBSOLETE   {
// OBSOLETE     "r00", "r01", "r02", "r03", "r04", "r05", "r06", "r07",
// OBSOLETE     "r08", "r09", "r10", "r11", "r12", "r13", "r14", "r15",
// OBSOLETE     "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
// OBSOLETE     "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
// OBSOLETE 
// OBSOLETE   /* these get confusing because we omit a few and switch some ordering around. */
// OBSOLETE 
// OBSOLETE     "cr01",			/* 32 = psr */
// OBSOLETE     "fcr62",			/* 33 = fpsr */
// OBSOLETE     "fcr63",			/* 34 = fpcr */
// OBSOLETE     "ip",			/* this is something of a cheat. */
// OBSOLETE   /* 35 = sxip */
// OBSOLETE     "cr05",			/* 36 = snip */
// OBSOLETE     "cr06",			/* 37 = sfip */
// OBSOLETE 
// OBSOLETE     "x00", "x01", "x02", "x03", "x04", "x05", "x06", "x07",
// OBSOLETE     "x08", "x09", "x10", "x11", "x12", "x13", "x14", "x15",
// OBSOLETE     "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
// OBSOLETE     "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31",
// OBSOLETE   };
// OBSOLETE 
// OBSOLETE   return rn[regno];
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #if 0				/* not currently used */
// OBSOLETE /* Read from remote while the input matches STRING.  Return zero on
// OBSOLETE    success, -1 on failure.  */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE bug_scan (char *s)
// OBSOLETE {
// OBSOLETE   int c;
// OBSOLETE 
// OBSOLETE   while (*s)
// OBSOLETE     {
// OBSOLETE       c = sr_readchar ();
// OBSOLETE       if (c != *s++)
// OBSOLETE 	{
// OBSOLETE 	  fflush (stdout);
// OBSOLETE 	  printf ("\nNext character is '%c' - %d and s is \"%s\".\n", c, c, --s);
// OBSOLETE 	  return (-1);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return (0);
// OBSOLETE }
// OBSOLETE #endif /* never */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE bug_srec_write_cr (char *s)
// OBSOLETE {
// OBSOLETE   char *p = s;
// OBSOLETE 
// OBSOLETE   if (srec_echo_pace)
// OBSOLETE     for (p = s; *p; ++p)
// OBSOLETE       {
// OBSOLETE 	if (sr_get_debug () > 0)
// OBSOLETE 	  printf ("%c", *p);
// OBSOLETE 
// OBSOLETE 	do
// OBSOLETE 	  serial_write (sr_get_desc (), p, 1);
// OBSOLETE 	while (sr_pollchar () != *p);
// OBSOLETE       }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       sr_write_cr (s);
// OBSOLETE /*       return(bug_scan (s) || bug_scan ("\n")); */
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return (0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Store register REGNO, or all if REGNO == -1. */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE bug_fetch_register (int regno)
// OBSOLETE {
// OBSOLETE   sr_check_open ();
// OBSOLETE 
// OBSOLETE   if (regno == -1)
// OBSOLETE     {
// OBSOLETE       int i;
// OBSOLETE 
// OBSOLETE       for (i = 0; i < NUM_REGS; ++i)
// OBSOLETE 	bug_fetch_register (i);
// OBSOLETE     }
// OBSOLETE   else if (target_is_m88110 && regno == SFIP_REGNUM)
// OBSOLETE     {
// OBSOLETE       /* m88110 has no sfip. */
// OBSOLETE       long l = 0;
// OBSOLETE       supply_register (regno, (char *) &l);
// OBSOLETE     }
// OBSOLETE   else if (regno < XFP_REGNUM)
// OBSOLETE     {
// OBSOLETE       char buffer[MAX_REGISTER_RAW_SIZE];
// OBSOLETE 
// OBSOLETE       sr_write ("rs ", 3);
// OBSOLETE       sr_write_cr (get_reg_name (regno));
// OBSOLETE       sr_expect ("=");
// OBSOLETE       store_unsigned_integer (buffer, REGISTER_RAW_SIZE (regno),
// OBSOLETE 			      sr_get_hex_word ());
// OBSOLETE       gr_expect_prompt ();
// OBSOLETE       supply_register (regno, buffer);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       /* Float register so we need to parse a strange data format. */
// OBSOLETE       long p;
// OBSOLETE       unsigned char fpreg_buf[10];
// OBSOLETE 
// OBSOLETE       sr_write ("rs ", 3);
// OBSOLETE       sr_write (get_reg_name (regno), strlen (get_reg_name (regno)));
// OBSOLETE       sr_write_cr (";d");
// OBSOLETE       sr_expect ("rs");
// OBSOLETE       sr_expect (get_reg_name (regno));
// OBSOLETE       sr_expect (";d");
// OBSOLETE       sr_expect ("=");
// OBSOLETE 
// OBSOLETE       /* sign */
// OBSOLETE       p = sr_get_hex_digit (1);
// OBSOLETE       fpreg_buf[0] = p << 7;
// OBSOLETE 
// OBSOLETE       /* exponent */
// OBSOLETE       sr_expect ("_");
// OBSOLETE       p = sr_get_hex_digit (1);
// OBSOLETE       fpreg_buf[0] += (p << 4);
// OBSOLETE       fpreg_buf[0] += sr_get_hex_digit (1);
// OBSOLETE 
// OBSOLETE       fpreg_buf[1] = sr_get_hex_digit (1) << 4;
// OBSOLETE 
// OBSOLETE       /* fraction */
// OBSOLETE       sr_expect ("_");
// OBSOLETE       fpreg_buf[1] += sr_get_hex_digit (1);
// OBSOLETE 
// OBSOLETE       fpreg_buf[2] = (sr_get_hex_digit (1) << 4) + sr_get_hex_digit (1);
// OBSOLETE       fpreg_buf[3] = (sr_get_hex_digit (1) << 4) + sr_get_hex_digit (1);
// OBSOLETE       fpreg_buf[4] = (sr_get_hex_digit (1) << 4) + sr_get_hex_digit (1);
// OBSOLETE       fpreg_buf[5] = (sr_get_hex_digit (1) << 4) + sr_get_hex_digit (1);
// OBSOLETE       fpreg_buf[6] = (sr_get_hex_digit (1) << 4) + sr_get_hex_digit (1);
// OBSOLETE       fpreg_buf[7] = (sr_get_hex_digit (1) << 4) + sr_get_hex_digit (1);
// OBSOLETE       fpreg_buf[8] = 0;
// OBSOLETE       fpreg_buf[9] = 0;
// OBSOLETE 
// OBSOLETE       gr_expect_prompt ();
// OBSOLETE       supply_register (regno, fpreg_buf);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Store register REGNO, or all if REGNO == -1. */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE bug_store_register (int regno)
// OBSOLETE {
// OBSOLETE   char buffer[1024];
// OBSOLETE   sr_check_open ();
// OBSOLETE 
// OBSOLETE   if (regno == -1)
// OBSOLETE     {
// OBSOLETE       int i;
// OBSOLETE 
// OBSOLETE       for (i = 0; i < NUM_REGS; ++i)
// OBSOLETE 	bug_store_register (i);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       char *regname;
// OBSOLETE 
// OBSOLETE       regname = get_reg_name (regno);
// OBSOLETE 
// OBSOLETE       if (target_is_m88110 && regno == SFIP_REGNUM)
// OBSOLETE 	return;
// OBSOLETE       else if (regno < XFP_REGNUM)
// OBSOLETE 	sprintf (buffer, "rs %s %08lx",
// OBSOLETE 		 regname,
// OBSOLETE 		 (long) read_register (regno));
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  unsigned char *fpreg_buf =
// OBSOLETE 	  (unsigned char *) &registers[REGISTER_BYTE (regno)];
// OBSOLETE 
// OBSOLETE 	  sprintf (buffer, "rs %s %1x_%02x%1x_%1x%02x%02x%02x%02x%02x%02x;d",
// OBSOLETE 		   regname,
// OBSOLETE 	  /* sign */
// OBSOLETE 		   (fpreg_buf[0] >> 7) & 0xf,
// OBSOLETE 	  /* exponent */
// OBSOLETE 		   fpreg_buf[0] & 0x7f,
// OBSOLETE 		   (fpreg_buf[1] >> 8) & 0xf,
// OBSOLETE 	  /* fraction */
// OBSOLETE 		   fpreg_buf[1] & 0xf,
// OBSOLETE 		   fpreg_buf[2],
// OBSOLETE 		   fpreg_buf[3],
// OBSOLETE 		   fpreg_buf[4],
// OBSOLETE 		   fpreg_buf[5],
// OBSOLETE 		   fpreg_buf[6],
// OBSOLETE 		   fpreg_buf[7]);
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       sr_write_cr (buffer);
// OBSOLETE       gr_expect_prompt ();
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Transfer LEN bytes between GDB address MYADDR and target address
// OBSOLETE    MEMADDR.  If WRITE is non-zero, transfer them to the target,
// OBSOLETE    otherwise transfer them from the target.  TARGET is unused.
// OBSOLETE 
// OBSOLETE    Returns the number of bytes transferred. */
// OBSOLETE 
// OBSOLETE int
// OBSOLETE bug_xfer_memory (CORE_ADDR memaddr, char *myaddr, int len, int write,
// OBSOLETE 		 struct mem_attrib *attrib, struct target_ops *target)
// OBSOLETE {
// OBSOLETE   int res;
// OBSOLETE 
// OBSOLETE   if (len <= 0)
// OBSOLETE     return 0;
// OBSOLETE 
// OBSOLETE   if (write)
// OBSOLETE     res = bug_write_memory (memaddr, myaddr, len);
// OBSOLETE   else
// OBSOLETE     res = bug_read_memory (memaddr, myaddr, len);
// OBSOLETE 
// OBSOLETE   return res;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE start_load (void)
// OBSOLETE {
// OBSOLETE   char *command;
// OBSOLETE 
// OBSOLETE   command = (srec_echo_pace ? "lo 0 ;x" : "lo 0");
// OBSOLETE 
// OBSOLETE   sr_write_cr (command);
// OBSOLETE   sr_expect (command);
// OBSOLETE   sr_expect ("\r\n");
// OBSOLETE   bug_srec_write_cr ("S0030000FC");
// OBSOLETE   return;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* This is an extremely vulnerable and fragile function.  I've made
// OBSOLETE    considerable attempts to make this deterministic, but I've
// OBSOLETE    certainly forgotten something.  The trouble is that S-records are
// OBSOLETE    only a partial file format, not a protocol.  Worse, apparently the
// OBSOLETE    m88k bug monitor does not run in real time while receiving
// OBSOLETE    S-records.  Hence, we must pay excruciating attention to when and
// OBSOLETE    where error messages are returned, and what has actually been sent.
// OBSOLETE 
// OBSOLETE    Each call represents a chunk of memory to be sent to the target.
// OBSOLETE    We break that chunk into an S0 header record, some number of S3
// OBSOLETE    data records each containing srec_bytes, and an S7 termination
// OBSOLETE    record.  */
// OBSOLETE 
// OBSOLETE static char *srecord_strings[] =
// OBSOLETE {
// OBSOLETE   "S-RECORD",
// OBSOLETE   "-Bug>",
// OBSOLETE   NULL,
// OBSOLETE };
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE bug_write_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len)
// OBSOLETE {
// OBSOLETE   int done;
// OBSOLETE   int checksum;
// OBSOLETE   int x;
// OBSOLETE   int retries;
// OBSOLETE   char *buffer = alloca ((srec_bytes + 8) << 1);
// OBSOLETE 
// OBSOLETE   retries = 0;
// OBSOLETE 
// OBSOLETE   do
// OBSOLETE     {
// OBSOLETE       done = 0;
// OBSOLETE 
// OBSOLETE       if (retries > srec_max_retries)
// OBSOLETE 	return (-1);
// OBSOLETE 
// OBSOLETE       if (retries > 0)
// OBSOLETE 	{
// OBSOLETE 	  if (sr_get_debug () > 0)
// OBSOLETE 	    printf ("\n<retrying...>\n");
// OBSOLETE 
// OBSOLETE 	  /* This gr_expect_prompt call is extremely important.  Without
// OBSOLETE 	     it, we will tend to resend our packet so fast that it
// OBSOLETE 	     will arrive before the bug monitor is ready to receive
// OBSOLETE 	     it.  This would lead to a very ugly resend loop.  */
// OBSOLETE 
// OBSOLETE 	  gr_expect_prompt ();
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       start_load ();
// OBSOLETE 
// OBSOLETE       while (done < len)
// OBSOLETE 	{
// OBSOLETE 	  int thisgo;
// OBSOLETE 	  int idx;
// OBSOLETE 	  char *buf = buffer;
// OBSOLETE 	  CORE_ADDR address;
// OBSOLETE 
// OBSOLETE 	  checksum = 0;
// OBSOLETE 	  thisgo = len - done;
// OBSOLETE 	  if (thisgo > srec_bytes)
// OBSOLETE 	    thisgo = srec_bytes;
// OBSOLETE 
// OBSOLETE 	  address = memaddr + done;
// OBSOLETE 	  sprintf (buf, "S3%02X%08lX", thisgo + 4 + 1, (long) address);
// OBSOLETE 	  buf += 12;
// OBSOLETE 
// OBSOLETE 	  checksum += (thisgo + 4 + 1
// OBSOLETE 		       + (address & 0xff)
// OBSOLETE 		       + ((address >> 8) & 0xff)
// OBSOLETE 		       + ((address >> 16) & 0xff)
// OBSOLETE 		       + ((address >> 24) & 0xff));
// OBSOLETE 
// OBSOLETE 	  for (idx = 0; idx < thisgo; idx++)
// OBSOLETE 	    {
// OBSOLETE 	      sprintf (buf, "%02X", myaddr[idx + done]);
// OBSOLETE 	      checksum += myaddr[idx + done];
// OBSOLETE 	      buf += 2;
// OBSOLETE 	    }
// OBSOLETE 
// OBSOLETE 	  if (srec_noise > 0)
// OBSOLETE 	    {
// OBSOLETE 	      /* FIXME-NOW: insert a deliberate error every now and then.
// OBSOLETE 	         This is intended for testing/debugging the error handling
// OBSOLETE 	         stuff.  */
// OBSOLETE 	      static int counter = 0;
// OBSOLETE 	      if (++counter > srec_noise)
// OBSOLETE 		{
// OBSOLETE 		  counter = 0;
// OBSOLETE 		  ++checksum;
// OBSOLETE 		}
// OBSOLETE 	    }
// OBSOLETE 
// OBSOLETE 	  sprintf (buf, "%02X", ~checksum & 0xff);
// OBSOLETE 	  bug_srec_write_cr (buffer);
// OBSOLETE 
// OBSOLETE 	  if (srec_sleep != 0)
// OBSOLETE 	    sleep (srec_sleep);
// OBSOLETE 
// OBSOLETE 	  /* This pollchar is probably redundant to the gr_multi_scan
// OBSOLETE 	     below.  Trouble is, we can't be sure when or where an
// OBSOLETE 	     error message will appear.  Apparently, when running at
// OBSOLETE 	     full speed from a typical sun4, error messages tend to
// OBSOLETE 	     appear to arrive only *after* the s7 record.   */
// OBSOLETE 
// OBSOLETE 	  if ((x = sr_pollchar ()) != 0)
// OBSOLETE 	    {
// OBSOLETE 	      if (sr_get_debug () > 0)
// OBSOLETE 		printf ("\n<retrying...>\n");
// OBSOLETE 
// OBSOLETE 	      ++retries;
// OBSOLETE 
// OBSOLETE 	      /* flush any remaining input and verify that we are back
// OBSOLETE 	         at the prompt level. */
// OBSOLETE 	      gr_expect_prompt ();
// OBSOLETE 	      /* start all over again. */
// OBSOLETE 	      start_load ();
// OBSOLETE 	      done = 0;
// OBSOLETE 	      continue;
// OBSOLETE 	    }
// OBSOLETE 
// OBSOLETE 	  done += thisgo;
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       bug_srec_write_cr ("S7060000000000F9");
// OBSOLETE       ++retries;
// OBSOLETE 
// OBSOLETE       /* Having finished the load, we need to figure out whether we
// OBSOLETE          had any errors.  */
// OBSOLETE     }
// OBSOLETE   while (gr_multi_scan (srecord_strings, 0) == 0);;
// OBSOLETE 
// OBSOLETE   return (0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Copy LEN bytes of data from debugger memory at MYADDR
// OBSOLETE    to inferior's memory at MEMADDR.  Returns errno value.
// OBSOLETE    * sb/sh instructions don't work on unaligned addresses, when TU=1.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE /* Read LEN bytes from inferior memory at MEMADDR.  Put the result
// OBSOLETE    at debugger address MYADDR.  Returns errno value.  */
// OBSOLETE static int
// OBSOLETE bug_read_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len)
// OBSOLETE {
// OBSOLETE   char request[100];
// OBSOLETE   char *buffer;
// OBSOLETE   char *p;
// OBSOLETE   char type;
// OBSOLETE   char size;
// OBSOLETE   unsigned char c;
// OBSOLETE   unsigned int inaddr;
// OBSOLETE   unsigned int checksum;
// OBSOLETE 
// OBSOLETE   sprintf (request, "du 0 %lx:&%d", (long) memaddr, len);
// OBSOLETE   sr_write_cr (request);
// OBSOLETE 
// OBSOLETE   p = buffer = alloca (len);
// OBSOLETE 
// OBSOLETE   /* scan up through the header */
// OBSOLETE   sr_expect ("S0030000FC");
// OBSOLETE 
// OBSOLETE   while (p < buffer + len)
// OBSOLETE     {
// OBSOLETE       /* scan off any white space. */
// OBSOLETE       while (sr_readchar () != 'S');;
// OBSOLETE 
// OBSOLETE       /* what kind of s-rec? */
// OBSOLETE       type = sr_readchar ();
// OBSOLETE 
// OBSOLETE       /* scan record size */
// OBSOLETE       sr_get_hex_byte (&size);
// OBSOLETE       checksum = size;
// OBSOLETE       --size;
// OBSOLETE       inaddr = 0;
// OBSOLETE 
// OBSOLETE       switch (type)
// OBSOLETE 	{
// OBSOLETE 	case '7':
// OBSOLETE 	case '8':
// OBSOLETE 	case '9':
// OBSOLETE 	  goto done;
// OBSOLETE 
// OBSOLETE 	case '3':
// OBSOLETE 	  sr_get_hex_byte (&c);
// OBSOLETE 	  inaddr = (inaddr << 8) + c;
// OBSOLETE 	  checksum += c;
// OBSOLETE 	  --size;
// OBSOLETE 	  /* intentional fall through */
// OBSOLETE 	case '2':
// OBSOLETE 	  sr_get_hex_byte (&c);
// OBSOLETE 	  inaddr = (inaddr << 8) + c;
// OBSOLETE 	  checksum += c;
// OBSOLETE 	  --size;
// OBSOLETE 	  /* intentional fall through */
// OBSOLETE 	case '1':
// OBSOLETE 	  sr_get_hex_byte (&c);
// OBSOLETE 	  inaddr = (inaddr << 8) + c;
// OBSOLETE 	  checksum += c;
// OBSOLETE 	  --size;
// OBSOLETE 	  sr_get_hex_byte (&c);
// OBSOLETE 	  inaddr = (inaddr << 8) + c;
// OBSOLETE 	  checksum += c;
// OBSOLETE 	  --size;
// OBSOLETE 	  break;
// OBSOLETE 
// OBSOLETE 	default:
// OBSOLETE 	  /* bonk */
// OBSOLETE 	  error ("reading s-records.");
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       if (inaddr < memaddr
// OBSOLETE 	  || (memaddr + len) < (inaddr + size))
// OBSOLETE 	error ("srec out of memory range.");
// OBSOLETE 
// OBSOLETE       if (p != buffer + inaddr - memaddr)
// OBSOLETE 	error ("srec out of sequence.");
// OBSOLETE 
// OBSOLETE       for (; size; --size, ++p)
// OBSOLETE 	{
// OBSOLETE 	  sr_get_hex_byte (p);
// OBSOLETE 	  checksum += *p;
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       sr_get_hex_byte (&c);
// OBSOLETE       if (c != (~checksum & 0xff))
// OBSOLETE 	error ("bad s-rec checksum");
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE done:
// OBSOLETE   gr_expect_prompt ();
// OBSOLETE   if (p != buffer + len)
// OBSOLETE     return (1);
// OBSOLETE 
// OBSOLETE   memcpy (myaddr, buffer, len);
// OBSOLETE   return (0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #define MAX_BREAKS	16
// OBSOLETE static int num_brkpts = 0;
// OBSOLETE 
// OBSOLETE /* Insert a breakpoint at ADDR.  SAVE is normally the address of the
// OBSOLETE    pattern buffer where the instruction that the breakpoint overwrites
// OBSOLETE    is saved.  It is unused here since the bug is responsible for
// OBSOLETE    saving/restoring the original instruction. */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE bug_insert_breakpoint (CORE_ADDR addr, char *save)
// OBSOLETE {
// OBSOLETE   sr_check_open ();
// OBSOLETE 
// OBSOLETE   if (num_brkpts < MAX_BREAKS)
// OBSOLETE     {
// OBSOLETE       char buffer[100];
// OBSOLETE 
// OBSOLETE       num_brkpts++;
// OBSOLETE       sprintf (buffer, "br %lx", (long) addr);
// OBSOLETE       sr_write_cr (buffer);
// OBSOLETE       gr_expect_prompt ();
// OBSOLETE       return (0);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       fprintf_filtered (gdb_stderr,
// OBSOLETE 		      "Too many break points, break point not installed\n");
// OBSOLETE       return (1);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Remove a breakpoint at ADDR.  SAVE is normally the previously
// OBSOLETE    saved pattern, but is unused here since the bug is responsible
// OBSOLETE    for saving/restoring instructions. */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE bug_remove_breakpoint (CORE_ADDR addr, char *save)
// OBSOLETE {
// OBSOLETE   if (num_brkpts > 0)
// OBSOLETE     {
// OBSOLETE       char buffer[100];
// OBSOLETE 
// OBSOLETE       num_brkpts--;
// OBSOLETE       sprintf (buffer, "nobr %lx", (long) addr);
// OBSOLETE       sr_write_cr (buffer);
// OBSOLETE       gr_expect_prompt ();
// OBSOLETE 
// OBSOLETE     }
// OBSOLETE   return (0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Clear the bugs notion of what the break points are */
// OBSOLETE static int
// OBSOLETE bug_clear_breakpoints (void)
// OBSOLETE {
// OBSOLETE 
// OBSOLETE   if (sr_is_open ())
// OBSOLETE     {
// OBSOLETE       sr_write_cr ("nobr");
// OBSOLETE       sr_expect ("nobr");
// OBSOLETE       gr_expect_prompt ();
// OBSOLETE     }
// OBSOLETE   num_brkpts = 0;
// OBSOLETE   return (0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE struct target_ops bug_ops;
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE init_bug_ops (void)
// OBSOLETE {
// OBSOLETE   bug_ops.to_shortname = "bug";
// OBSOLETE   "Remote BUG monitor",
// OBSOLETE     bug_ops.to_longname = "Use the mvme187 board running the BUG monitor connected by a serial line.";
// OBSOLETE   bug_ops.to_doc = " ";
// OBSOLETE   bug_ops.to_open = bug_open;
// OBSOLETE   bug_ops.to_close = gr_close;
// OBSOLETE   bug_ops.to_attach = 0;
// OBSOLETE   bug_ops.to_post_attach = NULL;
// OBSOLETE   bug_ops.to_require_attach = NULL;
// OBSOLETE   bug_ops.to_detach = gr_detach;
// OBSOLETE   bug_ops.to_require_detach = NULL;
// OBSOLETE   bug_ops.to_resume = bug_resume;
// OBSOLETE   bug_ops.to_wait = bug_wait;
// OBSOLETE   bug_ops.to_post_wait = NULL;
// OBSOLETE   bug_ops.to_fetch_registers = bug_fetch_register;
// OBSOLETE   bug_ops.to_store_registers = bug_store_register;
// OBSOLETE   bug_ops.to_prepare_to_store = gr_prepare_to_store;
// OBSOLETE   bug_ops.to_xfer_memory = bug_xfer_memory;
// OBSOLETE   bug_ops.to_files_info = gr_files_info;
// OBSOLETE   bug_ops.to_insert_breakpoint = bug_insert_breakpoint;
// OBSOLETE   bug_ops.to_remove_breakpoint = bug_remove_breakpoint;
// OBSOLETE   bug_ops.to_terminal_init = 0;
// OBSOLETE   bug_ops.to_terminal_inferior = 0;
// OBSOLETE   bug_ops.to_terminal_ours_for_output = 0;
// OBSOLETE   bug_ops.to_terminal_ours = 0;
// OBSOLETE   bug_ops.to_terminal_info = 0;
// OBSOLETE   bug_ops.to_kill = gr_kill;
// OBSOLETE   bug_ops.to_load = bug_load;
// OBSOLETE   bug_ops.to_lookup_symbol = 0;
// OBSOLETE   bug_ops.to_create_inferior = gr_create_inferior;
// OBSOLETE   bug_ops.to_post_startup_inferior = NULL;
// OBSOLETE   bug_ops.to_acknowledge_created_inferior = NULL;
// OBSOLETE   bug_ops.to_clone_and_follow_inferior = NULL;
// OBSOLETE   bug_ops.to_post_follow_inferior_by_clone = NULL;
// OBSOLETE   bug_ops.to_insert_fork_catchpoint = NULL;
// OBSOLETE   bug_ops.to_remove_fork_catchpoint = NULL;
// OBSOLETE   bug_ops.to_insert_vfork_catchpoint = NULL;
// OBSOLETE   bug_ops.to_remove_vfork_catchpoint = NULL;
// OBSOLETE   bug_ops.to_has_forked = NULL;
// OBSOLETE   bug_ops.to_has_vforked = NULL;
// OBSOLETE   bug_ops.to_can_follow_vfork_prior_to_exec = NULL;
// OBSOLETE   bug_ops.to_post_follow_vfork = NULL;
// OBSOLETE   bug_ops.to_insert_exec_catchpoint = NULL;
// OBSOLETE   bug_ops.to_remove_exec_catchpoint = NULL;
// OBSOLETE   bug_ops.to_has_execd = NULL;
// OBSOLETE   bug_ops.to_reported_exec_events_per_exec_call = NULL;
// OBSOLETE   bug_ops.to_has_exited = NULL;
// OBSOLETE   bug_ops.to_mourn_inferior = gr_mourn;
// OBSOLETE   bug_ops.to_can_run = 0;
// OBSOLETE   bug_ops.to_notice_signals = 0;
// OBSOLETE   bug_ops.to_thread_alive = 0;
// OBSOLETE   bug_ops.to_stop = 0;
// OBSOLETE   bug_ops.to_pid_to_exec_file = NULL;
// OBSOLETE   bug_ops.to_stratum = process_stratum;
// OBSOLETE   bug_ops.DONT_USE = 0;
// OBSOLETE   bug_ops.to_has_all_memory = 1;
// OBSOLETE   bug_ops.to_has_memory = 1;
// OBSOLETE   bug_ops.to_has_stack = 1;
// OBSOLETE   bug_ops.to_has_registers = 0;
// OBSOLETE   bug_ops.to_has_execution = 0;
// OBSOLETE   bug_ops.to_sections = 0;
// OBSOLETE   bug_ops.to_sections_end = 0;
// OBSOLETE   bug_ops.to_magic = OPS_MAGIC;	/* Always the last thing */
// OBSOLETE }				/* init_bug_ops */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_remote_bug (void)
// OBSOLETE {
// OBSOLETE   init_bug_ops ();
// OBSOLETE   add_target (&bug_ops);
// OBSOLETE 
// OBSOLETE   add_show_from_set
// OBSOLETE     (add_set_cmd ("srec-bytes", class_support, var_uinteger,
// OBSOLETE 		  (char *) &srec_bytes,
// OBSOLETE 		  "\
// OBSOLETE Set the number of bytes represented in each S-record.\n\
// OBSOLETE This affects the communication protocol with the remote target.",
// OBSOLETE 		  &setlist),
// OBSOLETE      &showlist);
// OBSOLETE 
// OBSOLETE   add_show_from_set
// OBSOLETE     (add_set_cmd ("srec-max-retries", class_support, var_uinteger,
// OBSOLETE 		  (char *) &srec_max_retries,
// OBSOLETE 		  "\
// OBSOLETE Set the number of retries for shipping S-records.\n\
// OBSOLETE This affects the communication protocol with the remote target.",
// OBSOLETE 		  &setlist),
// OBSOLETE      &showlist);
// OBSOLETE 
// OBSOLETE #if 0
// OBSOLETE   /* This needs to set SREC_SIZE, not srec_frame which gets changed at the
// OBSOLETE      end of a download.  But do we need the option at all?  */
// OBSOLETE   add_show_from_set
// OBSOLETE     (add_set_cmd ("srec-frame", class_support, var_uinteger,
// OBSOLETE 		  (char *) &srec_frame,
// OBSOLETE 		  "\
// OBSOLETE Set the number of bytes in an S-record frame.\n\
// OBSOLETE This affects the communication protocol with the remote target.",
// OBSOLETE 		  &setlist),
// OBSOLETE      &showlist);
// OBSOLETE #endif /* 0 */
// OBSOLETE 
// OBSOLETE   add_show_from_set
// OBSOLETE     (add_set_cmd ("srec-noise", class_support, var_zinteger,
// OBSOLETE 		  (char *) &srec_noise,
// OBSOLETE 		  "\
// OBSOLETE Set number of S-record to send before deliberately flubbing a checksum.\n\
// OBSOLETE Zero means flub none at all.  This affects the communication protocol\n\
// OBSOLETE with the remote target.",
// OBSOLETE 		  &setlist),
// OBSOLETE      &showlist);
// OBSOLETE 
// OBSOLETE   add_show_from_set
// OBSOLETE     (add_set_cmd ("srec-sleep", class_support, var_zinteger,
// OBSOLETE 		  (char *) &srec_sleep,
// OBSOLETE 		  "\
// OBSOLETE Set number of seconds to sleep after an S-record for a possible error message to arrive.\n\
// OBSOLETE This affects the communication protocol with the remote target.",
// OBSOLETE 		  &setlist),
// OBSOLETE      &showlist);
// OBSOLETE 
// OBSOLETE   add_show_from_set
// OBSOLETE     (add_set_cmd ("srec-echo-pace", class_support, var_boolean,
// OBSOLETE 		  (char *) &srec_echo_pace,
// OBSOLETE 		  "\
// OBSOLETE Set echo-verification.\n\
// OBSOLETE When on, use verification by echo when downloading S-records.  This is\n\
// OBSOLETE much slower, but generally more reliable.",
// OBSOLETE 		  &setlist),
// OBSOLETE      &showlist);
// OBSOLETE }
