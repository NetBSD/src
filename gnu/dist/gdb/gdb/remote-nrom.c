// OBSOLETE /* Remote debugging with the XLNT Designs, Inc (XDI) NetROM.
// OBSOLETE    Copyright 1990, 1991, 1992, 1995, 1998, 1999, 2000
// OBSOLETE    Free Software Foundation, Inc.
// OBSOLETE    Contributed by:
// OBSOLETE    Roger Moyers 
// OBSOLETE    XLNT Designs, Inc.
// OBSOLETE    15050 Avenue of Science, Suite 106
// OBSOLETE    San Diego, CA  92128
// OBSOLETE    (619)487-9320
// OBSOLETE    roger@xlnt.com
// OBSOLETE    Adapted from work done at Cygnus Support in remote-nindy.c,
// OBSOLETE    later merged in by Stan Shebs at Cygnus.
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
// OBSOLETE #include "gdbcmd.h"
// OBSOLETE #include "serial.h"
// OBSOLETE #include "target.h"
// OBSOLETE 
// OBSOLETE /* Default ports used to talk with the NetROM.  */
// OBSOLETE 
// OBSOLETE #define DEFAULT_NETROM_LOAD_PORT    1236
// OBSOLETE #define DEFAULT_NETROM_CONTROL_PORT 1237
// OBSOLETE 
// OBSOLETE static void nrom_close (int quitting);
// OBSOLETE 
// OBSOLETE /* New commands.  */
// OBSOLETE 
// OBSOLETE static void nrom_passthru (char *, int);
// OBSOLETE 
// OBSOLETE /* We talk to the NetROM over these sockets.  */
// OBSOLETE 
// OBSOLETE static struct serial *load_desc = NULL;
// OBSOLETE static struct serial *ctrl_desc = NULL;
// OBSOLETE 
// OBSOLETE static int load_port = DEFAULT_NETROM_LOAD_PORT;
// OBSOLETE static int control_port = DEFAULT_NETROM_CONTROL_PORT;
// OBSOLETE 
// OBSOLETE static char nrom_hostname[100];
// OBSOLETE 
// OBSOLETE /* Forward data declaration. */
// OBSOLETE 
// OBSOLETE extern struct target_ops nrom_ops;
// OBSOLETE 
// OBSOLETE /* Scan input from the remote system, until STRING is found.  Print chars that
// OBSOLETE    don't match.  */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE expect (char *string)
// OBSOLETE {
// OBSOLETE   char *p = string;
// OBSOLETE   int c;
// OBSOLETE 
// OBSOLETE   immediate_quit++;
// OBSOLETE 
// OBSOLETE   while (1)
// OBSOLETE     {
// OBSOLETE       c = serial_readchar (ctrl_desc, 5);
// OBSOLETE 
// OBSOLETE       if (c == *p++)
// OBSOLETE 	{
// OBSOLETE 	  if (*p == '\0')
// OBSOLETE 	    {
// OBSOLETE 	      immediate_quit--;
// OBSOLETE 	      return 0;
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  fputc_unfiltered (c, gdb_stdout);
// OBSOLETE 	  p = string;
// OBSOLETE 	  if (c == *p)
// OBSOLETE 	    p++;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE nrom_kill (void)
// OBSOLETE {
// OBSOLETE   nrom_close (0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static struct serial *
// OBSOLETE open_socket (char *name, int port)
// OBSOLETE {
// OBSOLETE   char sockname[100];
// OBSOLETE   struct serial *desc;
// OBSOLETE 
// OBSOLETE   sprintf (sockname, "%s:%d", name, port);
// OBSOLETE   desc = serial_open (sockname);
// OBSOLETE   if (!desc)
// OBSOLETE     perror_with_name (sockname);
// OBSOLETE 
// OBSOLETE   return desc;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE load_cleanup (void)
// OBSOLETE {
// OBSOLETE   serial_close (load_desc);
// OBSOLETE   load_desc = NULL;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Download a file specified in ARGS to the netROM.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE nrom_load (char *args, int fromtty)
// OBSOLETE {
// OBSOLETE   int fd, rd_amt, fsize;
// OBSOLETE   bfd *pbfd;
// OBSOLETE   asection *section;
// OBSOLETE   char *downloadstring = "download 0\n";
// OBSOLETE   struct cleanup *old_chain;
// OBSOLETE 
// OBSOLETE   /* Tell the netrom to get ready to download. */
// OBSOLETE   if (serial_write (ctrl_desc, downloadstring, strlen (downloadstring)))
// OBSOLETE     error ("nrom_load: control_send() of `%s' failed", downloadstring);
// OBSOLETE 
// OBSOLETE   expect ("Waiting for a connection...\n");
// OBSOLETE 
// OBSOLETE   load_desc = open_socket (nrom_hostname, load_port);
// OBSOLETE 
// OBSOLETE   old_chain = make_cleanup (load_cleanup, 0);
// OBSOLETE 
// OBSOLETE   pbfd = bfd_openr (args, 0);
// OBSOLETE 
// OBSOLETE   if (pbfd)
// OBSOLETE     {
// OBSOLETE       make_cleanup (bfd_close, pbfd);
// OBSOLETE 
// OBSOLETE       if (!bfd_check_format (pbfd, bfd_object))
// OBSOLETE 	error ("\"%s\": not in executable format: %s",
// OBSOLETE 	       args, bfd_errmsg (bfd_get_error ()));
// OBSOLETE 
// OBSOLETE       for (section = pbfd->sections; section; section = section->next)
// OBSOLETE 	{
// OBSOLETE 	  if (bfd_get_section_flags (pbfd, section) & SEC_ALLOC)
// OBSOLETE 	    {
// OBSOLETE 	      bfd_vma section_address;
// OBSOLETE 	      unsigned long section_size;
// OBSOLETE 	      const char *section_name;
// OBSOLETE 
// OBSOLETE 	      section_name = bfd_get_section_name (pbfd, section);
// OBSOLETE 	      section_address = bfd_get_section_vma (pbfd, section);
// OBSOLETE 	      section_size = bfd_section_size (pbfd, section);
// OBSOLETE 
// OBSOLETE 	      if (bfd_get_section_flags (pbfd, section) & SEC_LOAD)
// OBSOLETE 		{
// OBSOLETE 		  file_ptr fptr;
// OBSOLETE 
// OBSOLETE 		  printf_filtered ("[Loading section %s at %x (%d bytes)]\n",
// OBSOLETE 				   section_name, section_address,
// OBSOLETE 				   section_size);
// OBSOLETE 
// OBSOLETE 		  fptr = 0;
// OBSOLETE 
// OBSOLETE 		  while (section_size > 0)
// OBSOLETE 		    {
// OBSOLETE 		      char buffer[1024];
// OBSOLETE 		      int count;
// OBSOLETE 
// OBSOLETE 		      count = min (section_size, 1024);
// OBSOLETE 
// OBSOLETE 		      bfd_get_section_contents (pbfd, section, buffer, fptr,
// OBSOLETE 						count);
// OBSOLETE 
// OBSOLETE 		      serial_write (load_desc, buffer, count);
// OBSOLETE 		      section_address += count;
// OBSOLETE 		      fptr += count;
// OBSOLETE 		      section_size -= count;
// OBSOLETE 		    }
// OBSOLETE 		}
// OBSOLETE 	      else
// OBSOLETE 		/* BSS and such */
// OBSOLETE 		{
// OBSOLETE 		  printf_filtered ("[section %s: not loading]\n",
// OBSOLETE 				   section_name);
// OBSOLETE 		}
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     error ("\"%s\": Could not open", args);
// OBSOLETE 
// OBSOLETE   do_cleanups (old_chain);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Open a connection to the remote NetROM devices.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE nrom_open (char *name, int from_tty)
// OBSOLETE {
// OBSOLETE   int errn;
// OBSOLETE 
// OBSOLETE   if (!name || strchr (name, '/') || strchr (name, ':'))
// OBSOLETE     error (
// OBSOLETE 	    "To open a NetROM connection, you must specify the hostname\n\
// OBSOLETE or IP address of the NetROM device you wish to use.");
// OBSOLETE 
// OBSOLETE   strcpy (nrom_hostname, name);
// OBSOLETE 
// OBSOLETE   target_preopen (from_tty);
// OBSOLETE 
// OBSOLETE   unpush_target (&nrom_ops);
// OBSOLETE 
// OBSOLETE   ctrl_desc = open_socket (nrom_hostname, control_port);
// OBSOLETE 
// OBSOLETE   push_target (&nrom_ops);
// OBSOLETE 
// OBSOLETE   if (from_tty)
// OBSOLETE     printf_filtered ("Connected to NetROM device \"%s\"\n", nrom_hostname);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Close out all files and local state before this target loses control. */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE nrom_close (int quitting)
// OBSOLETE {
// OBSOLETE   if (load_desc)
// OBSOLETE     serial_close (load_desc);
// OBSOLETE   if (ctrl_desc)
// OBSOLETE     serial_close (ctrl_desc);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Pass arguments directly to the NetROM. */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE nrom_passthru (char *args, int fromtty)
// OBSOLETE {
// OBSOLETE   char buf[1024];
// OBSOLETE 
// OBSOLETE   sprintf (buf, "%s\n", args);
// OBSOLETE   if (serial_write (ctrl_desc, buf, strlen (buf)))
// OBSOLETE     error ("nrom_reset: control_send() of `%s'failed", args);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE nrom_mourn (void)
// OBSOLETE {
// OBSOLETE   unpush_target (&nrom_ops);
// OBSOLETE   generic_mourn_inferior ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Define the target vector. */
// OBSOLETE 
// OBSOLETE struct target_ops nrom_ops;
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE init_nrom_ops (void)
// OBSOLETE {
// OBSOLETE   nrom_ops.to_shortname = "nrom";
// OBSOLETE   nrom_ops.to_longname = "Remote XDI `NetROM' target";
// OBSOLETE   nrom_ops.to_doc = "Remote debug using a NetROM over Ethernet";
// OBSOLETE   nrom_ops.to_open = nrom_open;
// OBSOLETE   nrom_ops.to_close = nrom_close;
// OBSOLETE   nrom_ops.to_attach = NULL;
// OBSOLETE   nrom_ops.to_post_attach = NULL;
// OBSOLETE   nrom_ops.to_require_attach = NULL;
// OBSOLETE   nrom_ops.to_detach = NULL;
// OBSOLETE   nrom_ops.to_require_detach = NULL;
// OBSOLETE   nrom_ops.to_resume = NULL;
// OBSOLETE   nrom_ops.to_wait = NULL;
// OBSOLETE   nrom_ops.to_post_wait = NULL;
// OBSOLETE   nrom_ops.to_fetch_registers = NULL;
// OBSOLETE   nrom_ops.to_store_registers = NULL;
// OBSOLETE   nrom_ops.to_prepare_to_store = NULL;
// OBSOLETE   nrom_ops.to_xfer_memory = NULL;
// OBSOLETE   nrom_ops.to_files_info = NULL;
// OBSOLETE   nrom_ops.to_insert_breakpoint = NULL;
// OBSOLETE   nrom_ops.to_remove_breakpoint = NULL;
// OBSOLETE   nrom_ops.to_terminal_init = NULL;
// OBSOLETE   nrom_ops.to_terminal_inferior = NULL;
// OBSOLETE   nrom_ops.to_terminal_ours_for_output = NULL;
// OBSOLETE   nrom_ops.to_terminal_ours = NULL;
// OBSOLETE   nrom_ops.to_terminal_info = NULL;
// OBSOLETE   nrom_ops.to_kill = nrom_kill;
// OBSOLETE   nrom_ops.to_load = nrom_load;
// OBSOLETE   nrom_ops.to_lookup_symbol = NULL;
// OBSOLETE   nrom_ops.to_create_inferior = NULL;
// OBSOLETE   nrom_ops.to_post_startup_inferior = NULL;
// OBSOLETE   nrom_ops.to_acknowledge_created_inferior = NULL;
// OBSOLETE   nrom_ops.to_clone_and_follow_inferior = NULL;
// OBSOLETE   nrom_ops.to_post_follow_inferior_by_clone = NULL;
// OBSOLETE   nrom_ops.to_insert_fork_catchpoint = NULL;
// OBSOLETE   nrom_ops.to_remove_fork_catchpoint = NULL;
// OBSOLETE   nrom_ops.to_insert_vfork_catchpoint = NULL;
// OBSOLETE   nrom_ops.to_remove_vfork_catchpoint = NULL;
// OBSOLETE   nrom_ops.to_has_forked = NULL;
// OBSOLETE   nrom_ops.to_has_vforked = NULL;
// OBSOLETE   nrom_ops.to_can_follow_vfork_prior_to_exec = NULL;
// OBSOLETE   nrom_ops.to_post_follow_vfork = NULL;
// OBSOLETE   nrom_ops.to_insert_exec_catchpoint = NULL;
// OBSOLETE   nrom_ops.to_remove_exec_catchpoint = NULL;
// OBSOLETE   nrom_ops.to_has_execd = NULL;
// OBSOLETE   nrom_ops.to_reported_exec_events_per_exec_call = NULL;
// OBSOLETE   nrom_ops.to_has_exited = NULL;
// OBSOLETE   nrom_ops.to_mourn_inferior = nrom_mourn;
// OBSOLETE   nrom_ops.to_can_run = NULL;
// OBSOLETE   nrom_ops.to_notice_signals = 0;
// OBSOLETE   nrom_ops.to_thread_alive = 0;
// OBSOLETE   nrom_ops.to_stop = 0;
// OBSOLETE   nrom_ops.to_pid_to_exec_file = NULL;
// OBSOLETE   nrom_ops.to_stratum = download_stratum;
// OBSOLETE   nrom_ops.DONT_USE = NULL;
// OBSOLETE   nrom_ops.to_has_all_memory = 1;
// OBSOLETE   nrom_ops.to_has_memory = 1;
// OBSOLETE   nrom_ops.to_has_stack = 1;
// OBSOLETE   nrom_ops.to_has_registers = 1;
// OBSOLETE   nrom_ops.to_has_execution = 0;
// OBSOLETE   nrom_ops.to_sections = NULL;
// OBSOLETE   nrom_ops.to_sections_end = NULL;
// OBSOLETE   nrom_ops.to_magic = OPS_MAGIC;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_remote_nrom (void)
// OBSOLETE {
// OBSOLETE   init_nrom_ops ();
// OBSOLETE   add_target (&nrom_ops);
// OBSOLETE 
// OBSOLETE   add_show_from_set (
// OBSOLETE   add_set_cmd ("nrom_load_port", no_class, var_zinteger, (char *) &load_port,
// OBSOLETE 	       "Set the port to use for NetROM downloads\n", &setlist),
// OBSOLETE 		      &showlist);
// OBSOLETE 
// OBSOLETE   add_show_from_set (
// OBSOLETE 		      add_set_cmd ("nrom_control_port", no_class, var_zinteger, (char *) &control_port,
// OBSOLETE 	    "Set the port to use for NetROM debugger services\n", &setlist),
// OBSOLETE 		      &showlist);
// OBSOLETE 
// OBSOLETE   add_cmd ("nrom", no_class, nrom_passthru,
// OBSOLETE 	   "Pass arguments as command to NetROM",
// OBSOLETE 	   &cmdlist);
// OBSOLETE }
