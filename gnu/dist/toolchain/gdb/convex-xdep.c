/* OBSOLETE /* Convex host-dependent code for GDB. */
/* OBSOLETE    Copyright 1990, 1991, 1992 Free Software Foundation, Inc. */
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
/* OBSOLETE #include "command.h" */
/* OBSOLETE #include "symtab.h" */
/* OBSOLETE #include "value.h" */
/* OBSOLETE #include "frame.h" */
/* OBSOLETE #include "inferior.h" */
/* OBSOLETE #include "gdb_wait.h" */
/* OBSOLETE  */
/* OBSOLETE #include <signal.h> */
/* OBSOLETE #include <fcntl.h> */
/* OBSOLETE #include "gdbcore.h" */
/* OBSOLETE  */
/* OBSOLETE #include <sys/param.h> */
/* OBSOLETE #include <sys/dir.h> */
/* OBSOLETE #include <sys/user.h> */
/* OBSOLETE #include <sys/ioctl.h> */
/* OBSOLETE #include <sys/pcntl.h> */
/* OBSOLETE #include <sys/thread.h> */
/* OBSOLETE #include <sys/proc.h> */
/* OBSOLETE #include <sys/file.h> */
/* OBSOLETE #include "gdb_stat.h" */
/* OBSOLETE #include <sys/mman.h> */
/* OBSOLETE  */
/* OBSOLETE #include <convex/vmparam.h> */
/* OBSOLETE #include <convex/filehdr.h> */
/* OBSOLETE #include <convex/opthdr.h> */
/* OBSOLETE #include <convex/scnhdr.h> */
/* OBSOLETE #include <convex/core.h> */
/* OBSOLETE  */
/* OBSOLETE /* Per-thread data, read from the inferior at each stop and written */
/* OBSOLETE    back at each resume.  *x/ */
/* OBSOLETE  */
/* OBSOLETE /* Number of active threads. */
/* OBSOLETE    Tables are valid for thread numbers less than this.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static int n_threads; */
/* OBSOLETE  */
/* OBSOLETE #define MAXTHREADS 8 */
/* OBSOLETE              */
/* OBSOLETE /* Thread state.  The remaining data is valid only if this is PI_TALIVE.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static int thread_state[MAXTHREADS]; */
/* OBSOLETE  */
/* OBSOLETE /* Stop pc, signal, signal subcode *x/ */
/* OBSOLETE  */
/* OBSOLETE static int thread_pc[MAXTHREADS]; */
/* OBSOLETE static int thread_signal[MAXTHREADS]; */
/* OBSOLETE static int thread_sigcode[MAXTHREADS];       */
/* OBSOLETE  */
/* OBSOLETE /* Thread registers. */
/* OBSOLETE    If thread is selected, the regs are in registers[] instead.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static char thread_regs[MAXTHREADS][REGISTER_BYTES]; */
/* OBSOLETE  */
/* OBSOLETE /* 1 if the top frame on the thread's stack was a context frame, */
/* OBSOLETE    meaning that the kernel is up to something and we should not */
/* OBSOLETE    touch the thread at all except to resume it.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static char thread_is_in_kernel[MAXTHREADS]; */
/* OBSOLETE  */
/* OBSOLETE /* The currently selected thread's number.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static int inferior_thread; */
/* OBSOLETE  */
/* OBSOLETE /* Inferior process's file handle and a process control block */
/* OBSOLETE    to feed args to ioctl with.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static int inferior_fd; */
/* OBSOLETE static struct pcntl ps; */
/* OBSOLETE  */
/* OBSOLETE /* SOFF file headers for exec or core file.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static FILEHDR filehdr; */
/* OBSOLETE static OPTHDR opthdr; */
/* OBSOLETE static SCNHDR scnhdr; */
/* OBSOLETE  */
/* OBSOLETE /* Address maps constructed from section headers of exec and core files. */
/* OBSOLETE    Defines process address -> file address translation.  *x/ */
/* OBSOLETE  */
/* OBSOLETE struct pmap  */
/* OBSOLETE { */
/* OBSOLETE     long mem_addr;          /* process start address *x/ */
/* OBSOLETE     long mem_end;           /* process end+1 address *x/ */
/* OBSOLETE     long file_addr;         /* file start address *x/ */
/* OBSOLETE     long thread;            /* -1 shared; 0,1,... thread-local *x/ */
/* OBSOLETE     long type;                      /* S_TEXT S_DATA S_BSS S_TBSS etc *x/ */
/* OBSOLETE     long which;                     /* used to sort map for info files *x/ */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE static int n_exec, n_core; */
/* OBSOLETE static struct pmap exec_map[100]; */
/* OBSOLETE static struct pmap core_map[100]; */
/* OBSOLETE  */
/* OBSOLETE /* Offsets in the core file of core_context and core_tcontext blocks.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static int context_offset; */
/* OBSOLETE static int tcontext_offset[MAXTHREADS]; */
/* OBSOLETE  */
/* OBSOLETE /* Core file control blocks.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static struct core_context_v70 c; */
/* OBSOLETE static struct core_tcontext_v70 tc; */
/* OBSOLETE static struct user u; */
/* OBSOLETE static thread_t th; */
/* OBSOLETE static proc_t pr; */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Vector and communication registers from core dump or from inferior. */
/* OBSOLETE    These are read on demand, ie, not normally valid.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static struct vecst vector_registers; */
/* OBSOLETE static struct creg_ctx comm_registers; */
/* OBSOLETE  */
/* OBSOLETE /* Flag, set on a vanilla CONT command and cleared when the inferior */
/* OBSOLETE    is continued.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static int all_continue; */
/* OBSOLETE  */
/* OBSOLETE /* Flag, set when the inferior is continued by a vanilla CONT command, */
/* OBSOLETE    cleared if it is continued for any other purpose.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static int thread_switch_ok; */
/* OBSOLETE  */
/* OBSOLETE /* Stack of signals recieved from threads but not yet delivered to gdb.  *x/ */
/* OBSOLETE  */
/* OBSOLETE struct threadpid  */
/* OBSOLETE { */
/* OBSOLETE     int pid; */
/* OBSOLETE     int thread; */
/* OBSOLETE     int signo; */
/* OBSOLETE     int subsig; */
/* OBSOLETE     int pc; */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE static struct threadpid signal_stack_bot[100]; */
/* OBSOLETE static struct threadpid *signal_stack = signal_stack_bot; */
/* OBSOLETE  */
/* OBSOLETE /* How to detect empty stack -- bottom frame is all zero.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define signal_stack_is_empty() (signal_stack->pid == 0) */
/* OBSOLETE  */
/* OBSOLETE /* Mode controlled by SET PIPE command, controls the psw SEQ bit */
/* OBSOLETE    which forces each instruction to complete before the next one starts.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static int sequential = 0; */
/* OBSOLETE  */
/* OBSOLETE /* Mode controlled by the SET PARALLEL command.  Values are: */
/* OBSOLETE    0  concurrency limit 1 thread, dynamic scheduling */
/* OBSOLETE    1  no concurrency limit, dynamic scheduling */
/* OBSOLETE    2  no concurrency limit, fixed scheduling  *x/ */
/* OBSOLETE  */
/* OBSOLETE static int parallel = 1; */
/* OBSOLETE  */
/* OBSOLETE /* Mode controlled by SET BASE command, output radix for unformatted */
/* OBSOLETE    integer typeout, as in argument lists, aggregates, and so on. */
/* OBSOLETE    Zero means guess whether it's an address (hex) or not (decimal).  *x/ */
/* OBSOLETE  */
/* OBSOLETE static int output_radix = 0; */
/* OBSOLETE  */
/* OBSOLETE /* Signal subcode at last thread stop.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static int stop_sigcode; */
/* OBSOLETE  */
/* OBSOLETE /* Hack, see wait() below.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static int exec_trap_timer; */
/* OBSOLETE  */
/* OBSOLETE #include "gdbcmd.h" */
/* OBSOLETE  */
/* OBSOLETE static struct type *vector_type (); */
/* OBSOLETE static long *read_vector_register (); */
/* OBSOLETE static long *read_vector_register_1 (); */
/* OBSOLETE static void write_vector_register (); */
/* OBSOLETE static ULONGEST read_comm_register (); */
/* OBSOLETE static void write_comm_register (); */
/* OBSOLETE static void convex_cont_command (); */
/* OBSOLETE static void thread_continue (); */
/* OBSOLETE static void select_thread (); */
/* OBSOLETE static void scan_stack (); */
/* OBSOLETE static void set_fixed_scheduling (); */
/* OBSOLETE static char *subsig_name (); */
/* OBSOLETE static void psw_info (); */
/* OBSOLETE static sig_noop (); */
/* OBSOLETE static ptr_cmp (); */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Execute ptrace.  Convex V7 replaced ptrace with pattach. */
/* OBSOLETE    Allow ptrace (0) as a no-op.  *x/ */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE call_ptrace (request, pid, procaddr, buf) */
/* OBSOLETE      int request, pid; */
/* OBSOLETE      PTRACE_ARG3_TYPE procaddr; */
/* OBSOLETE      int buf; */
/* OBSOLETE { */
/* OBSOLETE   if (request == 0) */
/* OBSOLETE     return; */
/* OBSOLETE   error ("no ptrace"); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Replacement for system execle routine. */
/* OBSOLETE    Convert it to an equivalent exect, which pattach insists on.  *x/ */
/* OBSOLETE  */
/* OBSOLETE execle (name, argv) */
/* OBSOLETE      char *name, *argv; */
/* OBSOLETE { */
/* OBSOLETE   char ***envp = (char ***) &argv; */
/* OBSOLETE   while (*envp++) ; */
/* OBSOLETE  */
/* OBSOLETE   signal (SIGTRAP, sig_noop); */
/* OBSOLETE   exect (name, &argv, *envp); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Stupid handler for stupid trace trap that otherwise causes */
/* OBSOLETE    startup to stupidly hang.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static sig_noop ()  */
/* OBSOLETE {} */
/* OBSOLETE  */
/* OBSOLETE /* Read registers from inferior into registers[] array. */
/* OBSOLETE    For convex, they are already there, read in when the inferior stops.  *x/ */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE fetch_inferior_registers (regno) */
/* OBSOLETE      int regno; */
/* OBSOLETE { */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Store our register values back into the inferior. */
/* OBSOLETE    For Convex, do this only once, right before resuming inferior.  *x/ */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE store_inferior_registers (regno) */
/* OBSOLETE      int regno; */
/* OBSOLETE { */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Copy LEN bytes from inferior's memory starting at MEMADDR */
/* OBSOLETE    to debugger memory starting at MYADDR.  */
/* OBSOLETE    On failure (cannot read from inferior, usually because address is out */
/* OBSOLETE    of bounds) returns the value of errno. *x/ */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE read_inferior_memory (memaddr, myaddr, len) */
/* OBSOLETE      CORE_ADDR memaddr; */
/* OBSOLETE      char *myaddr; */
/* OBSOLETE      int len; */
/* OBSOLETE { */
/* OBSOLETE   errno = 0; */
/* OBSOLETE   while (len > 0) */
/* OBSOLETE     { */
/* OBSOLETE       /* little-known undocumented max request size *x/ */
/* OBSOLETE       int i = (len < 12288) ? len : 12288; */
/* OBSOLETE  */
/* OBSOLETE       lseek (inferior_fd, memaddr, 0); */
/* OBSOLETE       read (inferior_fd, myaddr, i); */
/* OBSOLETE  */
/* OBSOLETE       memaddr += i; */
/* OBSOLETE       myaddr += i; */
/* OBSOLETE       len -= i; */
/* OBSOLETE     } */
/* OBSOLETE   if (errno)  */
/* OBSOLETE     memset (myaddr, '\0', len); */
/* OBSOLETE   return errno; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Copy LEN bytes of data from debugger memory at MYADDR */
/* OBSOLETE    to inferior's memory at MEMADDR. */
/* OBSOLETE    Returns errno on failure (cannot write the inferior) *x/ */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE write_inferior_memory (memaddr, myaddr, len) */
/* OBSOLETE      CORE_ADDR memaddr; */
/* OBSOLETE      char *myaddr; */
/* OBSOLETE      int len; */
/* OBSOLETE { */
/* OBSOLETE   errno = 0; */
/* OBSOLETE   lseek (inferior_fd, memaddr, 0); */
/* OBSOLETE   write (inferior_fd, myaddr, len); */
/* OBSOLETE   return errno; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Here from create_inferior when the inferior process has been created */
/* OBSOLETE    and started up.  We must do a pattach to grab it for debugging. */
/* OBSOLETE  */
/* OBSOLETE    Also, intercept the CONT command by altering its dispatch address.  *x/ */
/* OBSOLETE /* FIXME: This used to be called from a macro CREATE_INFERIOR_HOOK. */
/* OBSOLETE    But now init_trace_fun is in the same place.  So re-write this to */
/* OBSOLETE    use the init_trace_fun (making convex a debugging target).  *x/ */
/* OBSOLETE  */
/* OBSOLETE create_inferior_hook (pid) */
/* OBSOLETE     int pid; */
/* OBSOLETE { */
/* OBSOLETE   static char cont[] = "cont"; */
/* OBSOLETE   static char cont1[] = "c"; */
/* OBSOLETE   char *linep = cont; */
/* OBSOLETE   char *linep1 = cont1; */
/* OBSOLETE   char **line = &linep; */
/* OBSOLETE   char **line1 = &linep1; */
/* OBSOLETE   struct cmd_list_element *c; */
/* OBSOLETE  */
/* OBSOLETE   c = lookup_cmd (line, cmdlist, "", 0); */
/* OBSOLETE   c->function = convex_cont_command; */
/* OBSOLETE   c = lookup_cmd (line1, cmdlist, "", 0); */
/* OBSOLETE   c->function = convex_cont_command; */
/* OBSOLETE  */
/* OBSOLETE   inferior_fd = pattach (pid, O_EXCL); */
/* OBSOLETE   if (inferior_fd < 0) */
/* OBSOLETE     perror_with_name ("pattach"); */
/* OBSOLETE   inferior_thread = 0; */
/* OBSOLETE   set_fixed_scheduling (pid, parallel == 2); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Attach process PID for debugging.  *x/ */
/* OBSOLETE  */
/* OBSOLETE attach (pid) */
/* OBSOLETE     int pid; */
/* OBSOLETE { */
/* OBSOLETE   int fd = pattach (pid, O_EXCL); */
/* OBSOLETE   if (fd < 0) */
/* OBSOLETE     perror_with_name ("pattach"); */
/* OBSOLETE   attach_flag = 1; */
/* OBSOLETE   /* wait for strange kernel reverberations to go away *x/ */
/* OBSOLETE   sleep (1); */
/* OBSOLETE  */
/* OBSOLETE   setpgrp (pid, pid); */
/* OBSOLETE  */
/* OBSOLETE   inferior_fd = fd; */
/* OBSOLETE   inferior_thread = 0; */
/* OBSOLETE   return pid; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Stop debugging the process whose number is PID */
/* OBSOLETE    and continue it with signal number SIGNAL. */
/* OBSOLETE    SIGNAL = 0 means just continue it.  *x/ */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE detach (signal) */
/* OBSOLETE      int signal; */
/* OBSOLETE { */
/* OBSOLETE   signal_stack = signal_stack_bot; */
/* OBSOLETE   thread_continue (-1, 0, signal); */
/* OBSOLETE   ioctl (inferior_fd, PIXDETACH, &ps); */
/* OBSOLETE   close (inferior_fd); */
/* OBSOLETE   inferior_fd = 0; */
/* OBSOLETE   attach_flag = 0; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Kill off the inferior process.  *x/ */
/* OBSOLETE  */
/* OBSOLETE kill_inferior () */
/* OBSOLETE { */
/* OBSOLETE   if (inferior_pid == 0) */
/* OBSOLETE     return; */
/* OBSOLETE   ioctl (inferior_fd, PIXTERMINATE, 0); */
/* OBSOLETE   wait (0); */
/* OBSOLETE   target_mourn_inferior (); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Read vector register REG, and return a pointer to the value.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static long * */
/* OBSOLETE read_vector_register (reg) */
/* OBSOLETE     int reg; */
/* OBSOLETE { */
/* OBSOLETE   if (have_inferior_p ()) */
/* OBSOLETE     { */
/* OBSOLETE       errno = 0; */
/* OBSOLETE       ps.pi_buffer = (char *) &vector_registers; */
/* OBSOLETE       ps.pi_nbytes = sizeof vector_registers; */
/* OBSOLETE       ps.pi_offset = 0; */
/* OBSOLETE       ps.pi_thread = inferior_thread; */
/* OBSOLETE       ioctl (inferior_fd, PIXRDVREGS, &ps); */
/* OBSOLETE       if (errno) */
/* OBSOLETE     memset (&vector_registers, '\0', sizeof vector_registers); */
/* OBSOLETE     } */
/* OBSOLETE   else if (corechan >= 0) */
/* OBSOLETE     { */
/* OBSOLETE       lseek (corechan, tcontext_offset[inferior_thread], 0); */
/* OBSOLETE       if (myread (corechan, &tc, sizeof tc) < 0) */
/* OBSOLETE     perror_with_name (corefile); */
/* OBSOLETE       lseek (corechan, tc.core_thread_p, 0); */
/* OBSOLETE       if (myread (corechan, &th, sizeof th) < 0) */
/* OBSOLETE     perror_with_name (corefile); */
/* OBSOLETE       lseek (corechan, tc.core_vregs_p, 0); */
/* OBSOLETE       if (myread (corechan, &vector_registers, 16*128) < 0) */
/* OBSOLETE     perror_with_name (corefile); */
/* OBSOLETE       vector_registers.vm[0] = th.t_vect_ctx.vc_vm[0]; */
/* OBSOLETE       vector_registers.vm[1] = th.t_vect_ctx.vc_vm[1]; */
/* OBSOLETE       vector_registers.vls = th.t_vect_ctx.vc_vls; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   return read_vector_register_1 (reg); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Return a pointer to vector register REG, which must already have been */
/* OBSOLETE    fetched from the inferior or core file.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static long * */
/* OBSOLETE read_vector_register_1 (reg)  */
/* OBSOLETE     int reg; */
/* OBSOLETE { */
/* OBSOLETE   switch (reg) */
/* OBSOLETE     { */
/* OBSOLETE     case VM_REGNUM: */
/* OBSOLETE       return (long *) vector_registers.vm; */
/* OBSOLETE     case VS_REGNUM: */
/* OBSOLETE       return (long *) &vector_registers.vls; */
/* OBSOLETE     case VL_REGNUM: */
/* OBSOLETE       return 1 + (long *) &vector_registers.vls; */
/* OBSOLETE     default: */
/* OBSOLETE       return (long *) &vector_registers.vr[reg]; */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Write vector register REG, element ELEMENT, new value VAL. */
/* OBSOLETE    NB: must use read-modify-write on the entire vector state, */
/* OBSOLETE    since pattach does not do offsetted writes correctly.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE write_vector_register (reg, element, val) */
/* OBSOLETE     int reg, element; */
/* OBSOLETE     ULONGEST val; */
/* OBSOLETE { */
/* OBSOLETE   if (have_inferior_p ()) */
/* OBSOLETE     { */
/* OBSOLETE       errno = 0; */
/* OBSOLETE       ps.pi_thread = inferior_thread; */
/* OBSOLETE       ps.pi_offset = 0; */
/* OBSOLETE       ps.pi_buffer = (char *) &vector_registers; */
/* OBSOLETE       ps.pi_nbytes = sizeof vector_registers; */
/* OBSOLETE  */
/* OBSOLETE       ioctl (inferior_fd, PIXRDVREGS, &ps); */
/* OBSOLETE  */
/* OBSOLETE       switch (reg) */
/* OBSOLETE     { */
/* OBSOLETE     case VL_REGNUM: */
/* OBSOLETE       vector_registers.vls = */
/* OBSOLETE         (vector_registers.vls & 0xffffffff00000000LL) */
/* OBSOLETE           + (unsigned long) val; */
/* OBSOLETE       break; */
/* OBSOLETE  */
/* OBSOLETE     case VS_REGNUM: */
/* OBSOLETE       vector_registers.vls = */
/* OBSOLETE         (val << 32) + (unsigned long) vector_registers.vls; */
/* OBSOLETE       break; */
/* OBSOLETE          */
/* OBSOLETE     default: */
/* OBSOLETE       vector_registers.vr[reg].el[element] = val; */
/* OBSOLETE       break; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE       ioctl (inferior_fd, PIXWRVREGS, &ps); */
/* OBSOLETE  */
/* OBSOLETE       if (errno) */
/* OBSOLETE     perror_with_name ("writing vector register"); */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Return the contents of communication register NUM.  *x/  */
/* OBSOLETE  */
/* OBSOLETE static ULONGEST  */
/* OBSOLETE read_comm_register (num) */
/* OBSOLETE      int num; */
/* OBSOLETE { */
/* OBSOLETE   if (have_inferior_p ()) */
/* OBSOLETE     { */
/* OBSOLETE       ps.pi_buffer = (char *) &comm_registers; */
/* OBSOLETE       ps.pi_nbytes = sizeof comm_registers; */
/* OBSOLETE       ps.pi_offset = 0; */
/* OBSOLETE       ps.pi_thread = inferior_thread; */
/* OBSOLETE       ioctl (inferior_fd, PIXRDCREGS, &ps); */
/* OBSOLETE     } */
/* OBSOLETE   return comm_registers.crreg.r4[num]; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Store a new value VAL into communication register NUM.   */
/* OBSOLETE    NB: Must use read-modify-write on the whole comm register set */
/* OBSOLETE    since pattach does not do offsetted writes correctly.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE write_comm_register (num, val) */
/* OBSOLETE      int num; */
/* OBSOLETE      ULONGEST val; */
/* OBSOLETE { */
/* OBSOLETE   if (have_inferior_p ()) */
/* OBSOLETE     { */
/* OBSOLETE       ps.pi_buffer = (char *) &comm_registers; */
/* OBSOLETE       ps.pi_nbytes = sizeof comm_registers; */
/* OBSOLETE       ps.pi_offset = 0; */
/* OBSOLETE       ps.pi_thread = inferior_thread; */
/* OBSOLETE       ioctl (inferior_fd, PIXRDCREGS, &ps); */
/* OBSOLETE       comm_registers.crreg.r4[num] = val; */
/* OBSOLETE       ioctl (inferior_fd, PIXWRCREGS, &ps); */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Resume execution of the inferior process. */
/* OBSOLETE    If STEP is nonzero, single-step it. */
/* OBSOLETE    If SIGNAL is nonzero, give it that signal.  *x/ */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE resume (step, signal) */
/* OBSOLETE      int step; */
/* OBSOLETE      int signal; */
/* OBSOLETE { */
/* OBSOLETE   errno = 0; */
/* OBSOLETE   if (step || signal) */
/* OBSOLETE     thread_continue (inferior_thread, step, signal); */
/* OBSOLETE   else */
/* OBSOLETE     thread_continue (-1, 0, 0); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Maybe resume some threads. */
/* OBSOLETE    THREAD is which thread to resume, or -1 to resume them all. */
/* OBSOLETE    STEP and SIGNAL are as in resume. */
/* OBSOLETE  */
/* OBSOLETE    Global variable ALL_CONTINUE is set when we are here to do a */
/* OBSOLETE    `cont' command; otherwise we may be doing `finish' or a call or */
/* OBSOLETE    something else that will not tolerate an automatic thread switch. */
/* OBSOLETE  */
/* OBSOLETE    If there are stopped threads waiting to deliver signals, and */
/* OBSOLETE    ALL_CONTINUE, do not actually resume anything.  gdb will do a wait */
/* OBSOLETE    and see one of the stopped threads in the queue.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE thread_continue (thread, step, signal) */
/* OBSOLETE      int thread, step, signal; */
/* OBSOLETE { */
/* OBSOLETE   int n; */
/* OBSOLETE  */
/* OBSOLETE   /* If we are to continue all threads, but not for the CONTINUE command, */
/* OBSOLETE      pay no attention and continue only the selected thread.  *x/ */
/* OBSOLETE  */
/* OBSOLETE   if (thread < 0 && ! all_continue) */
/* OBSOLETE     thread = inferior_thread; */
/* OBSOLETE  */
/* OBSOLETE   /* If we are not stepping, we have now executed the continue part */
/* OBSOLETE      of a CONTINUE command.  *x/ */
/* OBSOLETE  */
/* OBSOLETE   if (! step) */
/* OBSOLETE     all_continue = 0; */
/* OBSOLETE  */
/* OBSOLETE   /* Allow wait() to switch threads if this is an all-out continue.  *x/ */
/* OBSOLETE  */
/* OBSOLETE   thread_switch_ok = thread < 0; */
/* OBSOLETE  */
/* OBSOLETE   /* If there are threads queued up, don't resume.  *x/ */
/* OBSOLETE  */
/* OBSOLETE   if (thread_switch_ok && ! signal_stack_is_empty ()) */
/* OBSOLETE     return; */
/* OBSOLETE  */
/* OBSOLETE   /* OK, do it.  *x/ */
/* OBSOLETE  */
/* OBSOLETE   for (n = 0; n < n_threads; n++) */
/* OBSOLETE     if (thread_state[n] == PI_TALIVE) */
/* OBSOLETE       { */
/* OBSOLETE     select_thread (n); */
/* OBSOLETE  */
/* OBSOLETE     if ((thread < 0 || n == thread) && ! thread_is_in_kernel[n]) */
/* OBSOLETE       { */
/* OBSOLETE         /* Blam the trace bits in the stack's saved psws to match  */
/* OBSOLETE            the desired step mode.  This is required so that */
/* OBSOLETE            single-stepping a return doesn't restore a psw with a */
/* OBSOLETE            clear trace bit and fly away, and conversely, */
/* OBSOLETE            proceeding through a return in a routine that was */
/* OBSOLETE            stepped into doesn't cause a phantom break by restoring */
/* OBSOLETE            a psw with the trace bit set. *x/ */
/* OBSOLETE         scan_stack (PSW_T_BIT, step); */
/* OBSOLETE         scan_stack (PSW_S_BIT, sequential); */
/* OBSOLETE       } */
/* OBSOLETE  */
/* OBSOLETE     ps.pi_buffer = registers; */
/* OBSOLETE     ps.pi_nbytes = REGISTER_BYTES; */
/* OBSOLETE     ps.pi_offset = 0; */
/* OBSOLETE     ps.pi_thread = n; */
/* OBSOLETE     if (! thread_is_in_kernel[n]) */
/* OBSOLETE       if (ioctl (inferior_fd, PIXWRREGS, &ps)) */
/* OBSOLETE         perror_with_name ("PIXWRREGS"); */
/* OBSOLETE  */
/* OBSOLETE     if (thread < 0 || n == thread) */
/* OBSOLETE       { */
/* OBSOLETE         ps.pi_pc = 1; */
/* OBSOLETE         ps.pi_signo = signal; */
/* OBSOLETE         if (ioctl (inferior_fd, step ? PIXSTEP : PIXCONTINUE, &ps) < 0) */
/* OBSOLETE           perror_with_name ("PIXCONTINUE"); */
/* OBSOLETE       } */
/* OBSOLETE       } */
/* OBSOLETE  */
/* OBSOLETE   if (ioctl (inferior_fd, PIXRUN, &ps) < 0) */
/* OBSOLETE     perror_with_name ("PIXRUN"); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Replacement for system wait routine.   */
/* OBSOLETE  */
/* OBSOLETE    The system wait returns with one or more threads stopped by */
/* OBSOLETE    signals.  Put stopped threads on a stack and return them one by */
/* OBSOLETE    one, so that it appears that wait returns one thread at a time. */
/* OBSOLETE  */
/* OBSOLETE    Global variable THREAD_SWITCH_OK is set when gdb can tolerate wait */
/* OBSOLETE    returning a new thread.  If it is false, then only one thread is */
/* OBSOLETE    running; we will do a real wait, the thread will do something, and */
/* OBSOLETE    we will return that.  *x/ */
/* OBSOLETE  */
/* OBSOLETE pid_t */
/* OBSOLETE wait (w) */
/* OBSOLETE     union wait *w; */
/* OBSOLETE { */
/* OBSOLETE   int pid; */
/* OBSOLETE  */
/* OBSOLETE   if (!w) */
/* OBSOLETE     return wait3 (0, 0, 0); */
/* OBSOLETE  */
/* OBSOLETE   /* Do a real wait if we were told to, or if there are no queued threads.  *x/ */
/* OBSOLETE  */
/* OBSOLETE   if (! thread_switch_ok || signal_stack_is_empty ()) */
/* OBSOLETE     { */
/* OBSOLETE       int thread; */
/* OBSOLETE  */
/* OBSOLETE       pid = wait3 (w, 0, 0); */
/* OBSOLETE  */
/* OBSOLETE       if (!WIFSTOPPED (*w) || pid != inferior_pid) */
/* OBSOLETE     return pid; */
/* OBSOLETE  */
/* OBSOLETE       /* The inferior has done something and stopped.  Read in all the */
/* OBSOLETE      threads' registers, and queue up any signals that happened.  *x/ */
/* OBSOLETE  */
/* OBSOLETE       if (ioctl (inferior_fd, PIXGETTHCOUNT, &ps) < 0) */
/* OBSOLETE     perror_with_name ("PIXGETTHCOUNT"); */
/* OBSOLETE        */
/* OBSOLETE       n_threads = ps.pi_othdcnt; */
/* OBSOLETE       for (thread = 0; thread < n_threads; thread++) */
/* OBSOLETE     { */
/* OBSOLETE       ps.pi_thread = thread; */
/* OBSOLETE       if (ioctl (inferior_fd, PIXGETSUBCODE, &ps) < 0) */
/* OBSOLETE         perror_with_name ("PIXGETSUBCODE"); */
/* OBSOLETE       thread_state[thread] = ps.pi_otstate; */
/* OBSOLETE  */
/* OBSOLETE       if (ps.pi_otstate == PI_TALIVE) */
/* OBSOLETE         { */
/* OBSOLETE           select_thread (thread); */
/* OBSOLETE           ps.pi_buffer = registers; */
/* OBSOLETE           ps.pi_nbytes = REGISTER_BYTES; */
/* OBSOLETE           ps.pi_offset = 0; */
/* OBSOLETE           ps.pi_thread = thread; */
/* OBSOLETE           if (ioctl (inferior_fd, PIXRDREGS, &ps) < 0) */
/* OBSOLETE             perror_with_name ("PIXRDREGS"); */
/* OBSOLETE  */
/* OBSOLETE           registers_fetched (); */
/* OBSOLETE  */
/* OBSOLETE           thread_pc[thread] = read_pc (); */
/* OBSOLETE           thread_signal[thread] = ps.pi_osigno; */
/* OBSOLETE           thread_sigcode[thread] = ps.pi_osigcode; */
/* OBSOLETE  */
/* OBSOLETE           /* If the thread's stack has a context frame */
/* OBSOLETE              on top, something fucked is going on.  I do not */
/* OBSOLETE              know what, but do I know this: the only thing you */
/* OBSOLETE              can do with such a thread is continue it.  *x/ */
/* OBSOLETE  */
/* OBSOLETE           thread_is_in_kernel[thread] =  */
/* OBSOLETE             ((read_register (PS_REGNUM) >> 25) & 3) == 0; */
/* OBSOLETE  */
/* OBSOLETE           /* Signals push an extended frame and then fault */
/* OBSOLETE              with a ridiculous pc.  Pop the frame.  *x/ */
/* OBSOLETE  */
/* OBSOLETE           if (thread_pc[thread] > STACK_END_ADDR) */
/* OBSOLETE             { */
/* OBSOLETE               POP_FRAME; */
/* OBSOLETE               if (is_break_pc (thread_pc[thread])) */
/* OBSOLETE                 thread_pc[thread] = read_pc () - 2; */
/* OBSOLETE               else */
/* OBSOLETE                 thread_pc[thread] = read_pc (); */
/* OBSOLETE               write_register (PC_REGNUM, thread_pc[thread]); */
/* OBSOLETE             } */
/* OBSOLETE            */
/* OBSOLETE           if (ps.pi_osigno || ps.pi_osigcode) */
/* OBSOLETE             { */
/* OBSOLETE               signal_stack++; */
/* OBSOLETE               signal_stack->pid = pid; */
/* OBSOLETE               signal_stack->thread = thread; */
/* OBSOLETE               signal_stack->signo = thread_signal[thread]; */
/* OBSOLETE               signal_stack->subsig = thread_sigcode[thread]; */
/* OBSOLETE               signal_stack->pc = thread_pc[thread]; */
/* OBSOLETE             } */
/* OBSOLETE  */
/* OBSOLETE           /* The following hackery is caused by a unix 7.1 feature: */
/* OBSOLETE              the inferior's fixed scheduling mode is cleared when */
/* OBSOLETE              it execs the shell (since the shell is not a parallel */
/* OBSOLETE              program).  So, note the 5.4 trap we get when */
/* OBSOLETE              the shell does its exec, then catch the 5.0 trap  */
/* OBSOLETE              that occurs when the debuggee starts, and set fixed */
/* OBSOLETE              scheduling mode properly.  *x/ */
/* OBSOLETE  */
/* OBSOLETE           if (ps.pi_osigno == 5 && ps.pi_osigcode == 4) */
/* OBSOLETE             exec_trap_timer = 1; */
/* OBSOLETE           else */
/* OBSOLETE             exec_trap_timer--; */
/* OBSOLETE            */
/* OBSOLETE           if (ps.pi_osigno == 5 && exec_trap_timer == 0) */
/* OBSOLETE             set_fixed_scheduling (pid, parallel == 2); */
/* OBSOLETE         } */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE       if (signal_stack_is_empty ()) */
/* OBSOLETE     error ("no active threads?!"); */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* Select the thread that stopped, and return *w saying why.  *x/ */
/* OBSOLETE  */
/* OBSOLETE   select_thread (signal_stack->thread); */
/* OBSOLETE  */
/* OBSOLETE  FIXME: need to convert from host sig. */
/* OBSOLETE   stop_signal = signal_stack->signo; */
/* OBSOLETE   stop_sigcode = signal_stack->subsig; */
/* OBSOLETE  */
/* OBSOLETE   WSETSTOP (*w, signal_stack->signo); */
/* OBSOLETE   w->w_thread = signal_stack->thread; */
/* OBSOLETE   return (signal_stack--)->pid; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Select thread THREAD -- its registers, stack, per-thread memory. */
/* OBSOLETE    This is the only routine that may assign to inferior_thread */
/* OBSOLETE    or thread_regs[].  *x/ */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE select_thread (thread) */
/* OBSOLETE      int thread; */
/* OBSOLETE { */
/* OBSOLETE   if (thread == inferior_thread) */
/* OBSOLETE     return; */
/* OBSOLETE  */
/* OBSOLETE   memcpy (thread_regs[inferior_thread], registers, REGISTER_BYTES); */
/* OBSOLETE   ps.pi_thread = inferior_thread = thread; */
/* OBSOLETE   if (have_inferior_p ()) */
/* OBSOLETE     ioctl (inferior_fd, PISETRWTID, &ps); */
/* OBSOLETE   memcpy (registers, thread_regs[thread], REGISTER_BYTES); */
/* OBSOLETE } */
/* OBSOLETE    */
/* OBSOLETE /* Routine to set or clear a psw bit in the psw and also all psws */
/* OBSOLETE    saved on the stack.  Quits when we get to a frame in which the */
/* OBSOLETE    saved psw is correct. *x/ */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE scan_stack (bit, val) */
/* OBSOLETE     long bit, val; */
/* OBSOLETE { */
/* OBSOLETE   long ps = read_register (PS_REGNUM); */
/* OBSOLETE   long fp; */
/* OBSOLETE   if (val ? !(ps & bit) : (ps & bit)) */
/* OBSOLETE     {     */
/* OBSOLETE       ps ^= bit; */
/* OBSOLETE       write_register (PS_REGNUM, ps); */
/* OBSOLETE  */
/* OBSOLETE       fp = read_register (FP_REGNUM); */
/* OBSOLETE       while (fp & 0x80000000) */
/* OBSOLETE     { */
/* OBSOLETE       ps = read_memory_integer (fp + 4, 4); */
/* OBSOLETE       if (val ? (ps & bit) : !(ps & bit)) */
/* OBSOLETE         break; */
/* OBSOLETE       ps ^= bit; */
/* OBSOLETE       write_memory (fp + 4, &ps, 4); */
/* OBSOLETE       fp = read_memory_integer (fp + 8, 4); */
/* OBSOLETE     } */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Set fixed scheduling (alliant mode) of process PID to ARG (0 or 1).  *x/ */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE set_fixed_scheduling (pid, arg) */
/* OBSOLETE       int arg; */
/* OBSOLETE { */
/* OBSOLETE   struct pattributes pattr; */
/* OBSOLETE   getpattr (pid, &pattr); */
/* OBSOLETE   pattr.pattr_pfixed = arg; */
/* OBSOLETE   setpattr (pid, &pattr); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE core_file_command (filename, from_tty) */
/* OBSOLETE      char *filename; */
/* OBSOLETE      int from_tty; */
/* OBSOLETE { */
/* OBSOLETE   int n; */
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
/* OBSOLETE   n_core = 0; */
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
/* OBSOLETE  */
/* OBSOLETE       if (myread (corechan, &filehdr, sizeof filehdr) < 0) */
/* OBSOLETE     perror_with_name (filename); */
/* OBSOLETE  */
/* OBSOLETE       if (!IS_CORE_SOFF_MAGIC (filehdr.h_magic)) */
/* OBSOLETE     error ("%s: not a core file.\n", filename); */
/* OBSOLETE  */
/* OBSOLETE       if (myread (corechan, &opthdr, filehdr.h_opthdr) < 0) */
/* OBSOLETE     perror_with_name (filename); */
/* OBSOLETE  */
/* OBSOLETE       /* Read through the section headers. */
/* OBSOLETE      For text, data, etc, record an entry in the core file map. */
/* OBSOLETE      For context and tcontext, record the file address of */
/* OBSOLETE      the context blocks.  *x/ */
/* OBSOLETE  */
/* OBSOLETE       lseek (corechan, (long) filehdr.h_scnptr, 0); */
/* OBSOLETE  */
/* OBSOLETE       n_threads = 0; */
/* OBSOLETE       for (n = 0; n < filehdr.h_nscns; n++) */
/* OBSOLETE     { */
/* OBSOLETE       if (myread (corechan, &scnhdr, sizeof scnhdr) < 0) */
/* OBSOLETE         perror_with_name (filename); */
/* OBSOLETE       if ((scnhdr.s_flags & S_TYPMASK) >= S_TEXT */
/* OBSOLETE           && (scnhdr.s_flags & S_TYPMASK) <= S_COMON) */
/* OBSOLETE         { */
/* OBSOLETE           core_map[n_core].mem_addr = scnhdr.s_vaddr; */
/* OBSOLETE           core_map[n_core].mem_end = scnhdr.s_vaddr + scnhdr.s_size; */
/* OBSOLETE           core_map[n_core].file_addr = scnhdr.s_scnptr; */
/* OBSOLETE           core_map[n_core].type = scnhdr.s_flags & S_TYPMASK; */
/* OBSOLETE           if (core_map[n_core].type != S_TBSS */
/* OBSOLETE               && core_map[n_core].type != S_TDATA */
/* OBSOLETE               && core_map[n_core].type != S_TTEXT) */
/* OBSOLETE             core_map[n_core].thread = -1; */
/* OBSOLETE           else if (n_core == 0 */
/* OBSOLETE                    || core_map[n_core-1].mem_addr != scnhdr.s_vaddr) */
/* OBSOLETE             core_map[n_core].thread = 0; */
/* OBSOLETE           else  */
/* OBSOLETE             core_map[n_core].thread = core_map[n_core-1].thread + 1; */
/* OBSOLETE           n_core++; */
/* OBSOLETE         } */
/* OBSOLETE       else if ((scnhdr.s_flags & S_TYPMASK) == S_CONTEXT) */
/* OBSOLETE         context_offset = scnhdr.s_scnptr; */
/* OBSOLETE       else if ((scnhdr.s_flags & S_TYPMASK) == S_TCONTEXT)  */
/* OBSOLETE         tcontext_offset[n_threads++] = scnhdr.s_scnptr; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE       /* Read the context block, struct user, struct proc, */
/* OBSOLETE      and the comm regs.  *x/ */
/* OBSOLETE  */
/* OBSOLETE       lseek (corechan, context_offset, 0); */
/* OBSOLETE       if (myread (corechan, &c, sizeof c) < 0) */
/* OBSOLETE     perror_with_name (filename); */
/* OBSOLETE       lseek (corechan, c.core_user_p, 0); */
/* OBSOLETE       if (myread (corechan, &u, sizeof u) < 0) */
/* OBSOLETE     perror_with_name (filename); */
/* OBSOLETE       lseek (corechan, c.core_proc_p, 0); */
/* OBSOLETE       if (myread (corechan, &pr, sizeof pr) < 0) */
/* OBSOLETE     perror_with_name (filename); */
/* OBSOLETE       comm_registers = pr.p_creg; */
/* OBSOLETE  */
/* OBSOLETE       /* Core file apparently is really there.  Make it really exist */
/* OBSOLETE      for xfer_core_file so we can do read_memory on it. *x/ */
/* OBSOLETE  */
/* OBSOLETE       if (filename[0] == '/') */
/* OBSOLETE     corefile = savestring (filename, strlen (filename)); */
/* OBSOLETE       else */
/* OBSOLETE     corefile = concat (current_directory, "/", filename, NULL); */
/* OBSOLETE  */
/* OBSOLETE       printf_filtered ("Program %s ", u.u_comm); */
/* OBSOLETE  */
/* OBSOLETE       /* Read the thread registers and fill in the thread_xxx[] data.  *x/ */
/* OBSOLETE  */
/* OBSOLETE       for (n = 0; n < n_threads; n++) */
/* OBSOLETE     { */
/* OBSOLETE       select_thread (n); */
/* OBSOLETE  */
/* OBSOLETE       lseek (corechan, tcontext_offset[n], 0); */
/* OBSOLETE       if (myread (corechan, &tc, sizeof tc) < 0) */
/* OBSOLETE         perror_with_name (corefile); */
/* OBSOLETE       lseek (corechan, tc.core_thread_p, 0); */
/* OBSOLETE       if (myread (corechan, &th, sizeof th) < 0) */
/* OBSOLETE         perror_with_name (corefile); */
/* OBSOLETE  */
/* OBSOLETE       lseek (corechan, tc.core_syscall_context_p, 0); */
/* OBSOLETE       if (myread (corechan, registers, REGISTER_BYTES) < 0) */
/* OBSOLETE         perror_with_name (corefile); */
/* OBSOLETE  */
/* OBSOLETE       thread_signal[n] = th.t_cursig; */
/* OBSOLETE       thread_sigcode[n] = th.t_code; */
/* OBSOLETE       thread_state[n] = th.t_state; */
/* OBSOLETE       thread_pc[n] = read_pc (); */
/* OBSOLETE  */
/* OBSOLETE       if (thread_pc[n] > STACK_END_ADDR) */
/* OBSOLETE         { */
/* OBSOLETE           POP_FRAME; */
/* OBSOLETE           if (is_break_pc (thread_pc[n])) */
/* OBSOLETE             thread_pc[n] = read_pc () - 2; */
/* OBSOLETE           else */
/* OBSOLETE             thread_pc[n] = read_pc (); */
/* OBSOLETE           write_register (PC_REGNUM, thread_pc[n]); */
/* OBSOLETE         } */
/* OBSOLETE  */
/* OBSOLETE       printf_filtered ("thread %d received signal %d, %s\n", */
/* OBSOLETE                        n, thread_signal[n], */
/* OBSOLETE                        safe_strsignal (thread_signal[n])); */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE       /* Select an interesting thread -- also-rans died with SIGKILL, */
/* OBSOLETE      so find one that didn't.  *x/ */
/* OBSOLETE  */
/* OBSOLETE       for (n = 0; n < n_threads; n++) */
/* OBSOLETE     if (thread_signal[n] != 0 && thread_signal[n] != SIGKILL) */
/* OBSOLETE       { */
/* OBSOLETE         select_thread (n); */
/* OBSOLETE         stop_signal = thread_signal[n]; */
/* OBSOLETE         stop_sigcode = thread_sigcode[n]; */
/* OBSOLETE         break; */
/* OBSOLETE       } */
/* OBSOLETE  */
/* OBSOLETE       core_aouthdr.a_magic = 0; */
/* OBSOLETE  */
/* OBSOLETE       flush_cached_frames (); */
/* OBSOLETE       select_frame (get_current_frame (), 0); */
/* OBSOLETE       validate_files (); */
/* OBSOLETE  */
/* OBSOLETE       print_stack_frame (selected_frame, selected_frame_level, -1); */
/* OBSOLETE     } */
/* OBSOLETE   else if (from_tty) */
/* OBSOLETE     printf_filtered ("No core file now.\n"); */
/* OBSOLETE } */
