/* OBSOLETE /* Low level interface to ptrace, for GDB when running under m68k SVR2 Unix */
/* OBSOLETE    on Altos 3068.  Report bugs to Jyrki Kuoppala <jkp@cs.hut.fi> */
/* OBSOLETE    Copyright (C) 1989, 1991 Free Software Foundation, Inc. */
/* OBSOLETE  */
/* OBSOLETE This file is part of GDB. */
/* OBSOLETE  */
/* OBSOLETE This program is free software; you can redistribute it and/or modify */
/* OBSOLETE it under the terms of the GNU General Public License as published by */
/* OBSOLETE the Free Software Foundation; either version 2 of the License, or */
/* OBSOLETE (at your option) any later version. */
/* OBSOLETE  */
/* OBSOLETE This program is distributed in the hope that it will be useful, */
/* OBSOLETE but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* OBSOLETE MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* OBSOLETE GNU General Public License for more details. */
/* OBSOLETE  */
/* OBSOLETE You should have received a copy of the GNU General Public License */
/* OBSOLETE along with this program; if not, write to the Free Software */
/* OBSOLETE Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #include "defs.h" */
/* OBSOLETE #include "frame.h" */
/* OBSOLETE #include "inferior.h" */
/* OBSOLETE  */
/* OBSOLETE #ifdef USG */
/* OBSOLETE #include <sys/types.h> */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE #include <sys/param.h> */
/* OBSOLETE #include <sys/dir.h> */
/* OBSOLETE #include <signal.h> */
/* OBSOLETE #include <sys/ioctl.h> */
/* OBSOLETE #include <fcntl.h> */
/* OBSOLETE #ifdef USG */
/* OBSOLETE #include <sys/page.h> */
/* OBSOLETE #ifdef ALTOS */
/* OBSOLETE #include <sys/net.h> */
/* OBSOLETE #include <errno.h> */
/* OBSOLETE #endif */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE #include "gdbcore.h" */
/* OBSOLETE #include <sys/user.h>               /* After a.out.h  *x/ */
/* OBSOLETE #include <sys/file.h> */
/* OBSOLETE #include "gdb_stat.h" */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Work with core dump and executable files, for GDB.  */
/* OBSOLETE    This code would be in corefile.c if it weren't machine-dependent. *x/ */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE core_file_command (filename, from_tty) */
/* OBSOLETE      char *filename; */
/* OBSOLETE      int from_tty; */
/* OBSOLETE { */
/* OBSOLETE   int val; */
/* OBSOLETE  */
/* OBSOLETE   /* Discard all vestiges of any previous core file */
/* OBSOLETE      and mark data and stack spaces as empty.  *x/ */
/* OBSOLETE  */
/* OBSOLETE   if (corefile) */
/* OBSOLETE     free (corefile); */
/* OBSOLETE   corefile = 0; */
/* OBSOLETE  */
/* OBSOLETE   if (corechan >= 0) */
/* OBSOLETE     close (corechan); */
/* OBSOLETE   corechan = -1; */
/* OBSOLETE  */
/* OBSOLETE   data_start = 0; */
/* OBSOLETE   data_end = 0; */
/* OBSOLETE   stack_start = STACK_END_ADDR; */
/* OBSOLETE   stack_end = STACK_END_ADDR; */
/* OBSOLETE  */
/* OBSOLETE   /* Now, if a new core file was specified, open it and digest it.  *x/ */
/* OBSOLETE  */
/* OBSOLETE   if (filename) */
/* OBSOLETE     { */
/* OBSOLETE       filename = tilde_expand (filename); */
/* OBSOLETE       make_cleanup (free, filename); */
/* OBSOLETE        */
/* OBSOLETE       if (have_inferior_p ()) */
/* OBSOLETE     error ("To look at a core file, you must kill the program with \"kill\"."); */
/* OBSOLETE       corechan = open (filename, O_RDONLY, 0); */
/* OBSOLETE       if (corechan < 0) */
/* OBSOLETE     perror_with_name (filename); */
/* OBSOLETE       /* 4.2-style (and perhaps also sysV-style) core dump file.  *x/ */
/* OBSOLETE       { */
/* OBSOLETE     struct user u; */
/* OBSOLETE  */
/* OBSOLETE     unsigned int reg_offset; */
/* OBSOLETE  */
/* OBSOLETE     val = myread (corechan, &u, sizeof u); */
/* OBSOLETE     if (val < 0) */
/* OBSOLETE       perror_with_name ("Not a core file: reading upage"); */
/* OBSOLETE     if (val != sizeof u) */
/* OBSOLETE       error ("Not a core file: could only read %d bytes", val); */
/* OBSOLETE     data_start = exec_data_start; */
/* OBSOLETE  */
/* OBSOLETE #if !defined (NBPG) */
/* OBSOLETE #define NBPG NBPP */
/* OBSOLETE #endif */
/* OBSOLETE #if !defined (UPAGES) */
/* OBSOLETE #define UPAGES USIZE */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE     data_end = data_start + NBPG * u.u_dsize; */
/* OBSOLETE     stack_start = stack_end - NBPG * u.u_ssize; */
/* OBSOLETE     data_offset = NBPG * UPAGES + exec_data_start % NBPG /* Not sure about this //jkp *x/; */
/* OBSOLETE     stack_offset = NBPG * (UPAGES + u.u_dsize); */
/* OBSOLETE  */
/* OBSOLETE     /* Some machines put an absolute address in here and some put */
/* OBSOLETE        the offset in the upage of the regs.  *x/ */
/* OBSOLETE     reg_offset = (int) u.u_state; */
/* OBSOLETE     if (reg_offset > NBPG * UPAGES) */
/* OBSOLETE       reg_offset -= KERNEL_U_ADDR; */
/* OBSOLETE  */
/* OBSOLETE     memcpy (&core_aouthdr, &u.u_exdata, sizeof (AOUTHDR)); */
/* OBSOLETE     printf_unfiltered ("Core file is from \"%s\".\n", u.u_comm); */
/* OBSOLETE  */
/* OBSOLETE     /* I don't know where to find this info. */
/* OBSOLETE        So, for now, mark it as not available.  *x/ */
/* OBSOLETE     N_SET_MAGIC (core_aouthdr, 0); */
/* OBSOLETE  */
/* OBSOLETE     /* Read the register values out of the core file and store */
/* OBSOLETE        them where `read_register' will find them.  *x/ */
/* OBSOLETE  */
/* OBSOLETE     { */
/* OBSOLETE       register int regno; */
/* OBSOLETE  */
/* OBSOLETE       for (regno = 0; regno < NUM_REGS; regno++) */
/* OBSOLETE         { */
/* OBSOLETE           char buf[MAX_REGISTER_RAW_SIZE]; */
/* OBSOLETE  */
/* OBSOLETE           val = lseek (corechan, register_addr (regno, reg_offset), 0); */
/* OBSOLETE           if (val < 0 */
/* OBSOLETE               || (val = myread (corechan, buf, sizeof buf)) < 0) */
/* OBSOLETE             { */
/* OBSOLETE               char * buffer = (char *) alloca (strlen (REGISTER_NAME (regno)) */
/* OBSOLETE                                                + 30); */
/* OBSOLETE               strcpy (buffer, "Reading register "); */
/* OBSOLETE               strcat (buffer, REGISTER_NAME (regno)); */
/* OBSOLETE                                                 */
/* OBSOLETE               perror_with_name (buffer); */
/* OBSOLETE             } */
/* OBSOLETE  */
/* OBSOLETE           supply_register (regno, buf); */
/* OBSOLETE         } */
/* OBSOLETE     } */
/* OBSOLETE       } */
/* OBSOLETE       if (filename[0] == '/') */
/* OBSOLETE     corefile = savestring (filename, strlen (filename)); */
/* OBSOLETE       else */
/* OBSOLETE     { */
/* OBSOLETE       corefile = concat (current_directory, "/", filename, NULL); */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE       flush_cached_frames (); */
/* OBSOLETE       select_frame (get_current_frame (), 0); */
/* OBSOLETE       validate_files (); */
/* OBSOLETE     } */
/* OBSOLETE   else if (from_tty) */
/* OBSOLETE     printf_unfiltered ("No core file now.\n"); */
/* OBSOLETE } */
