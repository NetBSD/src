/* OBSOLETE /* Low level Pyramid interface to ptrace, for GDB when running under Unix. */
/* OBSOLETE    Copyright (C) 1988, 1989, 1991 Free Software Foundation, Inc. */
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
/* OBSOLETE #include <sys/param.h> */
/* OBSOLETE #include <sys/dir.h> */
/* OBSOLETE #include <signal.h> */
/* OBSOLETE #include <sys/ioctl.h> */
/* OBSOLETE /* #include <fcntl.h>  Can we live without this?  *x/ */
/* OBSOLETE  */
/* OBSOLETE #include "gdbcore.h" */
/* OBSOLETE #include <sys/user.h>               /* After a.out.h  *x/ */
/* OBSOLETE #include <sys/file.h> */
/* OBSOLETE #include "gdb_stat.h" */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE fetch_inferior_registers (regno) */
/* OBSOLETE      int regno; */
/* OBSOLETE { */
/* OBSOLETE   register int datum; */
/* OBSOLETE   register unsigned int regaddr; */
/* OBSOLETE   int reg_buf[NUM_REGS+1]; */
/* OBSOLETE   struct user u; */
/* OBSOLETE   register int skipped_frames = 0; */
/* OBSOLETE  */
/* OBSOLETE   registers_fetched (); */
/* OBSOLETE    */
/* OBSOLETE   for (regno = 0; regno < 64; regno++) { */
/* OBSOLETE     reg_buf[regno] = ptrace (3, inferior_pid, (PTRACE_ARG3_TYPE) regno, 0); */
/* OBSOLETE      */
/* OBSOLETE #if defined(PYRAMID_CONTROL_FRAME_DEBUGGING) */
/* OBSOLETE     printf_unfiltered ("Fetching register %s, got %0x\n", */
/* OBSOLETE         REGISTER_NAME (regno), */
/* OBSOLETE         reg_buf[regno]); */
/* OBSOLETE #endif /* PYRAMID_CONTROL_FRAME_DEBUGGING *x/ */
/* OBSOLETE      */
/* OBSOLETE     if (reg_buf[regno] == -1 && errno == EIO) { */
/* OBSOLETE       printf_unfiltered("fetch_interior_registers: fetching register %s\n", */
/* OBSOLETE          REGISTER_NAME (regno)); */
/* OBSOLETE       errno = 0; */
/* OBSOLETE     } */
/* OBSOLETE     supply_register (regno, reg_buf+regno); */
/* OBSOLETE   } */
/* OBSOLETE   /* that leaves regs 64, 65, and 66 *x/ */
/* OBSOLETE   datum = ptrace (3, inferior_pid, */
/* OBSOLETE               (PTRACE_ARG3_TYPE) (((char *)&u.u_pcb.pcb_csp) - */
/* OBSOLETE               ((char *)&u)), 0); */
/* OBSOLETE    */
/* OBSOLETE    */
/* OBSOLETE    */
/* OBSOLETE   /* FIXME: Find the Current Frame Pointer (CFP). CFP is a global */
/* OBSOLETE      register (ie, NOT windowed), that gets saved in a frame iff */
/* OBSOLETE      the code for that frame has a prologue (ie, "adsf N").  If */
/* OBSOLETE      there is a prologue, the adsf insn saves the old cfp in */
/* OBSOLETE      pr13, cfp is set to sp, and N bytes of locals are allocated */
/* OBSOLETE      (sp is decremented by n). */
/* OBSOLETE      This makes finding CFP hard. I guess the right way to do it */
/* OBSOLETE      is:  */
/* OBSOLETE      - If this is the innermost frame, believe ptrace() or */
/* OBSOLETE      the core area. */
/* OBSOLETE      - Otherwise: */
/* OBSOLETE      Find the first insn of the current frame. */
/* OBSOLETE      - find the saved pc; */
/* OBSOLETE      - find the call insn that saved it; */
/* OBSOLETE      - figure out where the call is to; */
/* OBSOLETE      - if the first insn is an adsf, we got a frame */
/* OBSOLETE      pointer. *x/ */
/* OBSOLETE    */
/* OBSOLETE    */
/* OBSOLETE   /* Normal processors have separate stack pointers for user and */
/* OBSOLETE      kernel mode. Getting the last user mode frame on such */
/* OBSOLETE      machines is easy: the kernel context of the ptrace()'d */
/* OBSOLETE      process is on the kernel stack, and the USP points to what */
/* OBSOLETE      we want. But Pyramids only have a single cfp for both user and */
/* OBSOLETE      kernel mode.  And processes being ptrace()'d have some */
/* OBSOLETE      kernel-context control frames on their stack. */
/* OBSOLETE      To avoid tracing back into the kernel context of an inferior, */
/* OBSOLETE      we skip 0 or more contiguous control frames where the pc is */
/* OBSOLETE      in the kernel. *x/  */
/* OBSOLETE    */
/* OBSOLETE   while (1) { */
/* OBSOLETE     register int inferior_saved_pc; */
/* OBSOLETE     inferior_saved_pc = ptrace (1, inferior_pid, */
/* OBSOLETE                             (PTRACE_ARG3_TYPE) (datum+((32+15)*4)), 0); */
/* OBSOLETE     if (inferior_saved_pc > 0) break; */
/* OBSOLETE #if defined(PYRAMID_CONTROL_FRAME_DEBUGGING) */
/* OBSOLETE     printf_unfiltered("skipping kernel frame %08x, pc=%08x\n", datum, */
/* OBSOLETE        inferior_saved_pc); */
/* OBSOLETE #endif /* PYRAMID_CONTROL_FRAME_DEBUGGING *x/ */
/* OBSOLETE     skipped_frames++; */
/* OBSOLETE     datum -= CONTROL_STACK_FRAME_SIZE; */
/* OBSOLETE   } */
/* OBSOLETE    */
/* OBSOLETE   reg_buf[CSP_REGNUM] = datum; */
/* OBSOLETE   supply_register(CSP_REGNUM, reg_buf+CSP_REGNUM); */
/* OBSOLETE #ifdef  PYRAMID_CONTROL_FRAME_DEBUGGING */
/* OBSOLETE   if (skipped_frames) { */
/* OBSOLETE     fprintf_unfiltered (gdb_stderr, */
/* OBSOLETE          "skipped %d frames from %x to %x; cfp was %x, now %x\n", */
/* OBSOLETE          skipped_frames, reg_buf[CSP_REGNUM]); */
/* OBSOLETE   } */
/* OBSOLETE #endif /* PYRAMID_CONTROL_FRAME_DEBUGGING *x/ */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Store our register values back into the inferior. */
/* OBSOLETE    If REGNO is -1, do this for all registers. */
/* OBSOLETE    Otherwise, REGNO specifies which register (so we can save time).  *x/ */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE store_inferior_registers (regno) */
/* OBSOLETE      int regno; */
/* OBSOLETE { */
/* OBSOLETE   register unsigned int regaddr; */
/* OBSOLETE   char buf[80]; */
/* OBSOLETE  */
/* OBSOLETE   if (regno >= 0) */
/* OBSOLETE     { */
/* OBSOLETE       if ((0 <= regno) && (regno < 64)) { */
/* OBSOLETE     /*regaddr = register_addr (regno, offset);*x/ */
/* OBSOLETE     regaddr = regno; */
/* OBSOLETE     errno = 0; */
/* OBSOLETE     ptrace (6, inferior_pid, (PTRACE_ARG3_TYPE) regaddr, */
/* OBSOLETE             read_register (regno)); */
/* OBSOLETE     if (errno != 0) */
/* OBSOLETE       { */
/* OBSOLETE         sprintf (buf, "writing register number %d", regno); */
/* OBSOLETE         perror_with_name (buf); */
/* OBSOLETE       } */
/* OBSOLETE       } */
/* OBSOLETE     } */
/* OBSOLETE   else */
/* OBSOLETE     { */
/* OBSOLETE       for (regno = 0; regno < NUM_REGS; regno++) */
/* OBSOLETE     { */
/* OBSOLETE       /*regaddr = register_addr (regno, offset);*x/ */
/* OBSOLETE       regaddr = regno; */
/* OBSOLETE       errno = 0; */
/* OBSOLETE       ptrace (6, inferior_pid, (PTRACE_ARG3_TYPE) regaddr, */
/* OBSOLETE               read_register (regno)); */
/* OBSOLETE       if (errno != 0) */
/* OBSOLETE         { */
/* OBSOLETE           sprintf (buf, "writing all regs, number %d", regno); */
/* OBSOLETE           perror_with_name (buf); */
/* OBSOLETE         } */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /*** Extensions to  core and dump files, for GDB. *x/ */
/* OBSOLETE  */
/* OBSOLETE extern unsigned int last_frame_offset; */
/* OBSOLETE  */
/* OBSOLETE #ifdef PYRAMID_CORE */
/* OBSOLETE  */
/* OBSOLETE /* Can't make definitions here static, since corefile.c needs them */
/* OBSOLETE    to do bounds checking on the core-file areas. O well. *x/ */
/* OBSOLETE  */
/* OBSOLETE /* have two stacks: one for data, one for register windows. *x/ */
/* OBSOLETE extern CORE_ADDR reg_stack_start; */
/* OBSOLETE extern CORE_ADDR reg_stack_end; */
/* OBSOLETE  */
/* OBSOLETE /* need this so we can find the global registers: they never get saved. *x/ */
/* OBSOLETE CORE_ADDR global_reg_offset; */
/* OBSOLETE static CORE_ADDR last_frame_address; */
/* OBSOLETE CORE_ADDR last_frame_offset; */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Address in core file of start of register window stack area. */
/* OBSOLETE    Don't know if is this any of meaningful, useful or necessary.   *x/ */
/* OBSOLETE extern int reg_stack_offset; */
/* OBSOLETE  */
/* OBSOLETE #endif /* PYRAMID_CORE *x/   */
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
/* OBSOLETE #ifdef PYRAMID_CORE */
/* OBSOLETE   reg_stack_start = CONTROL_STACK_ADDR; */
/* OBSOLETE   reg_stack_end = CONTROL_STACK_ADDR;       /* this isn't strictly true...*x/ */
/* OBSOLETE #endif /* PYRAMID_CORE *x/ */
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
/* OBSOLETE     data_end = data_start + NBPG * u.u_dsize; */
/* OBSOLETE     data_offset = NBPG * UPAGES; */
/* OBSOLETE     stack_offset = NBPG * (UPAGES + u.u_dsize); */
/* OBSOLETE  */
/* OBSOLETE     /* find registers in core file *x/ */
/* OBSOLETE #ifdef PYRAMID_PTRACE */
/* OBSOLETE     stack_start = stack_end - NBPG * u.u_ussize; */
/* OBSOLETE     reg_stack_offset = stack_offset + (NBPG *u.u_ussize); */
/* OBSOLETE     reg_stack_end = reg_stack_start + NBPG * u.u_cssize; */
/* OBSOLETE  */
/* OBSOLETE     last_frame_address = ((int) u.u_pcb.pcb_csp); */
/* OBSOLETE     last_frame_offset = reg_stack_offset + last_frame_address */
/* OBSOLETE             - CONTROL_STACK_ADDR ; */
/* OBSOLETE     global_reg_offset = (char *)&u - (char *)&u.u_pcb.pcb_gr0 ; */
/* OBSOLETE  */
/* OBSOLETE     /* skip any control-stack frames that were executed in the */
/* OBSOLETE        kernel. *x/ */
/* OBSOLETE  */
/* OBSOLETE     while (1) { */
/* OBSOLETE         char buf[4]; */
/* OBSOLETE         val = lseek (corechan, last_frame_offset+(47*4), 0); */
/* OBSOLETE         if (val < 0) */
/* OBSOLETE                 perror_with_name (filename); */
/* OBSOLETE         val = myread (corechan, buf, sizeof buf); */
/* OBSOLETE         if (val < 0) */
/* OBSOLETE                 perror_with_name (filename); */
/* OBSOLETE  */
/* OBSOLETE         if (*(int *)buf >= 0) */
/* OBSOLETE                 break; */
/* OBSOLETE         printf_unfiltered ("skipping frame %s\n", local_hex_string (last_frame_address)); */
/* OBSOLETE         last_frame_offset -= CONTROL_STACK_FRAME_SIZE; */
/* OBSOLETE         last_frame_address -= CONTROL_STACK_FRAME_SIZE; */
/* OBSOLETE     } */
/* OBSOLETE     reg_offset = last_frame_offset; */
/* OBSOLETE  */
/* OBSOLETE #if 1 || defined(PYRAMID_CONTROL_FRAME_DEBUGGING) */
/* OBSOLETE     printf_unfiltered ("Control stack pointer = %s\n", */
/* OBSOLETE             local_hex_string (u.u_pcb.pcb_csp)); */
/* OBSOLETE     printf_unfiltered ("offset to control stack %d outermost frame %d (%s)\n", */
/* OBSOLETE           reg_stack_offset, reg_offset, local_hex_string (last_frame_address)); */
/* OBSOLETE #endif /* PYRAMID_CONTROL_FRAME_DEBUGGING *x/ */
/* OBSOLETE  */
/* OBSOLETE #else /* not PYRAMID_CORE *x/ */
/* OBSOLETE     stack_start = stack_end - NBPG * u.u_ssize; */
/* OBSOLETE         reg_offset = (int) u.u_ar0 - KERNEL_U_ADDR; */
/* OBSOLETE #endif /* not PYRAMID_CORE *x/ */
/* OBSOLETE  */
/* OBSOLETE #ifdef __not_on_pyr_yet */
/* OBSOLETE     /* Some machines put an absolute address in here and some put */
/* OBSOLETE        the offset in the upage of the regs.  *x/ */
/* OBSOLETE     reg_offset = (int) u.u_ar0; */
/* OBSOLETE     if (reg_offset > NBPG * UPAGES) */
/* OBSOLETE       reg_offset -= KERNEL_U_ADDR; */
/* OBSOLETE #endif */
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
/* OBSOLETE       for (regno = 0; regno < 64; regno++) */
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
/* OBSOLETE           if (val < 0) */
/* OBSOLETE             perror_with_name (filename); */
/* OBSOLETE #ifdef PYRAMID_CONTROL_FRAME_DEBUGGING */
/* OBSOLETE       printf_unfiltered ("[reg %s(%d), offset in file %s=0x%0x, addr =0x%0x, =%0x]\n", */
/* OBSOLETE           REGISTER_NAME (regno), regno, filename, */
/* OBSOLETE           register_addr(regno, reg_offset), */
/* OBSOLETE           regno * 4 + last_frame_address, */
/* OBSOLETE           *((int *)buf)); */
/* OBSOLETE #endif /* PYRAMID_CONTROL_FRAME_DEBUGGING *x/ */
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
/* OBSOLETE #if 1 || defined(PYRAMID_CONTROL_FRAME_DEBUGGING) */
/* OBSOLETE       printf_unfiltered ("Providing CSP (%s) as nominal address of current frame.\n", */
/* OBSOLETE           local_hex_string(last_frame_address)); */
/* OBSOLETE #endif PYRAMID_CONTROL_FRAME_DEBUGGING */
/* OBSOLETE       /* FIXME: Which of the following is correct? *x/ */
/* OBSOLETE #if 0 */
/* OBSOLETE       set_current_frame ( create_new_frame (read_register (FP_REGNUM), */
/* OBSOLETE                                         read_pc ())); */
/* OBSOLETE #else */
/* OBSOLETE       set_current_frame ( create_new_frame (last_frame_address, */
/* OBSOLETE                                         read_pc ())); */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE       select_frame (get_current_frame (), 0); */
/* OBSOLETE       validate_files (); */
/* OBSOLETE     } */
/* OBSOLETE   else if (from_tty) */
/* OBSOLETE     printf_unfiltered ("No core file now.\n"); */
/* OBSOLETE } */
