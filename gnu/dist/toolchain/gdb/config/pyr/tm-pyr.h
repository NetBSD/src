/* OBSOLETE /* Definitions to make GDB run on a Pyramid under OSx 4.0 (4.2bsd). */
/* OBSOLETE    Copyright 1988, 1989, 1991, 1993 Free Software Foundation, Inc. */
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
/* OBSOLETE #define TARGET_BYTE_ORDER BIG_ENDIAN */
/* OBSOLETE  */
/* OBSOLETE /* Traditional Unix virtual address spaces have thre regions: text, */
/* OBSOLETE    data and stack.  The text, initialised data, and uninitialised data */
/* OBSOLETE    are represented in separate segments of the a.out file. */
/* OBSOLETE    When a process dumps core, the data and stack regions are written */
/* OBSOLETE    to a core file.  This gives a debugger enough information to */
/* OBSOLETE    reconstruct (and debug) the virtual address space at the time of */
/* OBSOLETE    the coredump. */
/* OBSOLETE    Pyramids have an distinct fourth region of the virtual address */
/* OBSOLETE    space, in which the contents of the windowed registers are stacked */
/* OBSOLETE    in fixed-size frames.  Pyramid refer to this region as the control */
/* OBSOLETE    stack.  Each call (or trap) automatically allocates a new register */
/* OBSOLETE    frame; each return deallocates the current frame and restores the */
/* OBSOLETE    windowed registers to their values before the call. */
/* OBSOLETE  */
/* OBSOLETE    When dumping core, the control stack is written to a core files as */
/* OBSOLETE    a third segment. The core-handling functions need to know to deal */
/* OBSOLETE    with it. *x/  */
/* OBSOLETE  */
/* OBSOLETE /* Tell corefile.c there is an extra segment.  *x/ */
/* OBSOLETE #define REG_STACK_SEGMENT */
/* OBSOLETE  */
/* OBSOLETE /* Floating point is IEEE compatible on most Pyramid hardware */
/* OBSOLETE    (Older processors do not have IEEE NaNs).  *x/ */
/* OBSOLETE #define IEEE_FLOAT */
/* OBSOLETE  */
/* OBSOLETE /* Offset from address of function to start of its code. */
/* OBSOLETE    Zero on most machines.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FUNCTION_START_OFFSET 0 */
/* OBSOLETE  */
/* OBSOLETE /* Advance PC across any function entry prologue instructions */
/* OBSOLETE    to reach some "real" code.  *x/ */
/* OBSOLETE  */
/* OBSOLETE /* FIXME -- do we want to skip insns to allocate the local frame? */
/* OBSOLETE    If so, what do they look like? */
/* OBSOLETE    This is becoming harder, since tege@sics.SE wants to change */
/* OBSOLETE    gcc to not output a prologue when no frame is needed.   *x/ */
/* OBSOLETE #define SKIP_PROLOGUE(pc)  (pc) */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Immediately after a function call, return the saved pc. */
/* OBSOLETE    Can't always go through the frames for this because on some machines */
/* OBSOLETE    the new frame is not set up until the new function executes */
/* OBSOLETE    some instructions.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define SAVED_PC_AFTER_CALL(frame) FRAME_SAVED_PC(frame) */
/* OBSOLETE  */
/* OBSOLETE /* Address of end of stack space.  *x/ */
/* OBSOLETE /* This seems to be right for the 90x comp.vuw.ac.nz. */
/* OBSOLETE    The correct value at any site may be a function of the configured */
/* OBSOLETE    maximum control stack depth.  If so, I don't know where the */
/* OBSOLETE    control-stack depth is configured, so I can't #include it here. *x/  */
/* OBSOLETE #define STACK_END_ADDR (0xc00cc000) */
/* OBSOLETE  */
/* OBSOLETE /* Register window stack (Control stack) stack definitions */
/* OBSOLETE     - Address of beginning of control stack. */
/* OBSOLETE     - size of control stack frame */
/* OBSOLETE    (Note that since crts0 is usually the first function called, */
/* OBSOLETE     main()'s control stack is one frame (0x80 bytes) beyond this value.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define CONTROL_STACK_ADDR (0xc00cd000) */
/* OBSOLETE  */
/* OBSOLETE /* Bytes in a register window -- 16 parameter regs, 16 local regs */
/* OBSOLETE    for each call, is 32 regs * 4 bytes *x/ */
/* OBSOLETE  */
/* OBSOLETE #define CONTROL_STACK_FRAME_SIZE (32*4) */
/* OBSOLETE  */
/* OBSOLETE /* FIXME.  On a pyr, Data Stack grows downward; control stack goes upwards.  */
/* OBSOLETE    Which direction should we use for INNER_THAN, PC_INNER_THAN ?? *x/ */
/* OBSOLETE  */
/* OBSOLETE #define INNER_THAN(lhs,rhs) ((lhs) < (rhs)) */
/* OBSOLETE  */
/* OBSOLETE /* Stack must be aligned on 32-bit boundaries when synthesizing */
/* OBSOLETE    function calls. *x/ */
/* OBSOLETE  */
/* OBSOLETE #define STACK_ALIGN(ADDR) (((ADDR) + 3) & -4) */
/* OBSOLETE  */
/* OBSOLETE /* Sequence of bytes for breakpoint instruction.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define BREAKPOINT {0xf0, 00, 00, 00} */
/* OBSOLETE  */
/* OBSOLETE /* Amount PC must be decremented by after a breakpoint. */
/* OBSOLETE    This is often the number of bytes in BREAKPOINT */
/* OBSOLETE    but not always.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define DECR_PC_AFTER_BREAK 0 */
/* OBSOLETE  */
/* OBSOLETE /* Say how long (ordinary) registers are.  This is a piece of bogosity */
/* OBSOLETE    used in push_word and a few other places; REGISTER_RAW_SIZE is the */
/* OBSOLETE    real way to know how big a register is.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_SIZE 4 */
/* OBSOLETE  */
/* OBSOLETE /* Number of machine registers *x/ */
/* OBSOLETE /* pyramids have 64, plus one for the PSW; plus perhaps one more for the */
/* OBSOLETE    kernel stack pointer (ksp) and control-stack pointer (CSP) *x/ */
/* OBSOLETE  */
/* OBSOLETE #define NUM_REGS 67 */
/* OBSOLETE  */
/* OBSOLETE /* Initializer for an array of names of registers. */
/* OBSOLETE    There should be NUM_REGS strings in this initializer.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_NAMES \ */
/* OBSOLETE {"gr0", "gr1", "gr2", "gr3", "gr4", "gr5", "gr6", "gr7", \ */
/* OBSOLETE  "gr8", "gr9", "gr10", "gr11", "logpsw", "cfp", "sp", "pc", \ */
/* OBSOLETE  "pr0", "pr1", "pr2", "pr3", "pr4", "pr5", "pr6", "pr7", \ */
/* OBSOLETE  "pr8", "pr9", "pr10", "pr11", "pr12", "pr13", "pr14", "pr15", \ */
/* OBSOLETE  "lr0", "lr1", "lr2", "lr3", "lr4", "lr5", "lr6", "lr7", \ */
/* OBSOLETE  "lr8", "lr9", "lr10", "lr11", "lr12", "lr13", "lr14", "lr15", \ */
/* OBSOLETE  "tr0", "tr1", "tr2", "tr3", "tr4", "tr5", "tr6", "tr7", \ */
/* OBSOLETE  "tr8", "tr9", "tr10", "tr11", "tr12", "tr13", "tr14", "tr15", \ */
/* OBSOLETE   "psw", "ksp", "csp"} */
/* OBSOLETE  */
/* OBSOLETE /* Register numbers of various important registers. */
/* OBSOLETE    Note that some of these values are "real" register numbers, */
/* OBSOLETE    and correspond to the general registers of the machine, */
/* OBSOLETE    and some are "phony" register numbers which are too large */
/* OBSOLETE    to be actual register numbers as far as the user is concerned */
/* OBSOLETE    but do serve to get the desired values when passed to read_register.  *x/ */
/* OBSOLETE  */
/* OBSOLETE /* pseudo-registers: *x/ */
/* OBSOLETE #define PS_REGNUM 64                /* Contains processor status *x/ */
/* OBSOLETE #define PSW_REGNUM 64               /* Contains current psw, whatever it is.*x/ */
/* OBSOLETE #define CSP_REGNUM 65               /* address of this control stack frame*x/ */
/* OBSOLETE #define KSP_REGNUM 66               /* Contains process's Kernel Stack Pointer *x/ */
/* OBSOLETE  */
/* OBSOLETE #define CFP_REGNUM 13               /* Current data-stack frame ptr *x/ */
/* OBSOLETE #define TR0_REGNUM 48               /* After function call, contains */
/* OBSOLETE                                function result *x/ */
/* OBSOLETE  */
/* OBSOLETE /* Registers interesting to the machine-independent part of gdb*x/ */
/* OBSOLETE  */
/* OBSOLETE #define FP_REGNUM CSP_REGNUM        /* Contains address of executing (control) */
/* OBSOLETE                                stack frame *x/ */
/* OBSOLETE #define SP_REGNUM 14                /* Contains address of top of stack -??*x/ */
/* OBSOLETE #define PC_REGNUM 15                /* Contains program counter *x/ */
/* OBSOLETE  */
/* OBSOLETE /* Define DO_REGISTERS_INFO() to do machine-specific formatting */
/* OBSOLETE    of register dumps. *x/ */
/* OBSOLETE  */
/* OBSOLETE #define DO_REGISTERS_INFO(_regnum, fp) pyr_do_registers_info(_regnum, fp) */
/* OBSOLETE  */
/* OBSOLETE /* need this so we can find the global registers: they never get saved. *x/ */
/* OBSOLETE extern unsigned int global_reg_offset; */
/* OBSOLETE extern unsigned int last_frame_offset; */
/* OBSOLETE  */
/* OBSOLETE /* Total amount of space needed to store our copies of the machine's */
/* OBSOLETE    register state, the array `registers'.  *x/ */
/* OBSOLETE #define REGISTER_BYTES (NUM_REGS*4) */
/* OBSOLETE  */
/* OBSOLETE /* the Pyramid has register windows.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define HAVE_REGISTER_WINDOWS */
/* OBSOLETE  */
/* OBSOLETE /* Is this register part of the register window system?  A yes answer */
/* OBSOLETE    implies that 1) The name of this register will not be the same in */
/* OBSOLETE    other frames, and 2) This register is automatically "saved" (out */
/* OBSOLETE    registers shifting into ins counts) upon subroutine calls and thus */
/* OBSOLETE    there is no need to search more than one stack frame for it. *x/ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_IN_WINDOW_P(regnum)        \ */
/* OBSOLETE   ((regnum) >= 16 && (regnum) < 64) */
/* OBSOLETE  */
/* OBSOLETE /* Index within `registers' of the first byte of the space for */
/* OBSOLETE    register N.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_BYTE(N) ((N) * 4) */
/* OBSOLETE  */
/* OBSOLETE /* Number of bytes of storage in the actual machine representation */
/* OBSOLETE    for register N.  On the Pyramid, all regs are 4 bytes.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_RAW_SIZE(N) 4 */
/* OBSOLETE  */
/* OBSOLETE /* Number of bytes of storage in the program's representation */
/* OBSOLETE    for register N.  On the Pyramid, all regs are 4 bytes.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_VIRTUAL_SIZE(N) 4 */
/* OBSOLETE  */
/* OBSOLETE /* Largest value REGISTER_RAW_SIZE can have.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define MAX_REGISTER_RAW_SIZE 4 */
/* OBSOLETE  */
/* OBSOLETE /* Largest value REGISTER_VIRTUAL_SIZE can have.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define MAX_REGISTER_VIRTUAL_SIZE 4 */
/* OBSOLETE  */
/* OBSOLETE /* Return the GDB type object for the "standard" data type */
/* OBSOLETE    of data in register N.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_VIRTUAL_TYPE(N) builtin_type_int */
/* OBSOLETE  */
/* OBSOLETE /* FIXME: It seems impossible for both EXTRACT_RETURN_VALUE and */
/* OBSOLETE    STORE_RETURN_VALUE to be correct. *x/ */
/* OBSOLETE  */
/* OBSOLETE /* Store the address of the place in which to copy the structure the */
/* OBSOLETE    subroutine will return.  This is called from call_function. *x/ */
/* OBSOLETE  */
/* OBSOLETE /****FIXME****x/ */
/* OBSOLETE #define STORE_STRUCT_RETURN(ADDR, SP) \ */
/* OBSOLETE   { write_register (TR0_REGNUM, (ADDR)); } */
/* OBSOLETE  */
/* OBSOLETE /* Extract from an array REGBUF containing the (raw) register state */
/* OBSOLETE    a function return value of type TYPE, and copy that, in virtual format, */
/* OBSOLETE    into VALBUF.  *x/ */
/* OBSOLETE  */
/* OBSOLETE /* Note that on a register-windowing machine (eg, Pyr, SPARC), this is */
/* OBSOLETE    where the value is found after the function call -- ie, it should */
/* OBSOLETE    correspond to GNU CC's FUNCTION_VALUE rather than FUNCTION_OUTGOING_VALUE.*x/ */
/* OBSOLETE  */
/* OBSOLETE #define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \ */
/* OBSOLETE   memcpy (VALBUF, ((int *)(REGBUF))+TR0_REGNUM, TYPE_LENGTH (TYPE)) */
/* OBSOLETE  */
/* OBSOLETE /* Write into appropriate registers a function return value */
/* OBSOLETE    of type TYPE, given in virtual format.  *x/ */
/* OBSOLETE /* on pyrs, values are returned in *x/ */
/* OBSOLETE  */
/* OBSOLETE #define STORE_RETURN_VALUE(TYPE,VALBUF) \ */
/* OBSOLETE   write_register_bytes (REGISTER_BYTE(TR0_REGNUM), VALBUF, TYPE_LENGTH (TYPE)) */
/* OBSOLETE  */
/* OBSOLETE /* Extract from an array REGBUF containing the (raw) register state */
/* OBSOLETE    the address in which a function should return its structure value, */
/* OBSOLETE    as a CORE_ADDR (or an expression that can be used as one).  *x/ */
/* OBSOLETE /* FIXME *x/ */
/* OBSOLETE #define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \ */
/* OBSOLETE   ( ((int *)(REGBUF)) [TR0_REGNUM]) */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Describe the pointer in each stack frame to the previous stack frame */
/* OBSOLETE    (its caller).  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define EXTRA_FRAME_INFO \ */
/* OBSOLETE     CORE_ADDR bottom;       \ */
/* OBSOLETE     CORE_ADDR frame_cfp;    \ */
/* OBSOLETE     CORE_ADDR frame_window_addr; */
/* OBSOLETE  */
/* OBSOLETE /* The bottom field is misnamed, since it might imply that memory from */
/* OBSOLETE    bottom to frame contains this frame.  That need not be true if */
/* OBSOLETE    stack frames are allocated in different segments (e.g. some on a */
/* OBSOLETE    stack, some on a heap in the data segment).  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define INIT_EXTRA_FRAME_INFO(fromleaf, fci)  \ */
/* OBSOLETE do {                                                                \ */
/* OBSOLETE   (fci)->frame_window_addr = (fci)->frame;                  \ */
/* OBSOLETE   (fci)->bottom =                                           \ */
/* OBSOLETE       ((fci)->next ?                                        \ */
/* OBSOLETE        ((fci)->frame == (fci)->next->frame ?                        \ */
/* OBSOLETE         (fci)->next->bottom : (fci)->next->frame) :         \ */
/* OBSOLETE        read_register (SP_REGNUM));                          \ */
/* OBSOLETE   (fci)->frame_cfp =                                                \ */
/* OBSOLETE       read_register (CFP_REGNUM);                           \ */
/* OBSOLETE   /***fprintf (stderr,                                              \ */
/* OBSOLETE        "[[creating new frame for %0x,pc=%0x,csp=%0x]]\n",   \ */
/* OBSOLETE        (fci)->frame, (fci)->pc,(fci)->frame_cfp);*x/                \ */
/* OBSOLETE } while (0); */
/* OBSOLETE  */
/* OBSOLETE /* FRAME_CHAIN takes a frame's nominal address */
/* OBSOLETE    and produces the frame's chain-pointer. *x/ */
/* OBSOLETE  */
/* OBSOLETE /* In the case of the pyr, the frame's nominal address is the address */
/* OBSOLETE    of parameter register 0.  The previous frame is found 32 words up.   *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_CHAIN(thisframe)      \ */
/* OBSOLETE   ( (thisframe) -> frame - CONTROL_STACK_FRAME_SIZE) */
/* OBSOLETE  */
/* OBSOLETE  /*((thisframe) >= CONTROL_STACK_ADDR))*x/ */
/* OBSOLETE  */
/* OBSOLETE /* Define other aspects of the stack frame.  *x/ */
/* OBSOLETE  */
/* OBSOLETE /* A macro that tells us whether the function invocation represented */
/* OBSOLETE    by FI does not have a frame on the stack associated with it.  If it */
/* OBSOLETE    does not, FRAMELESS is set to 1, else 0. */
/* OBSOLETE  */
/* OBSOLETE    I do not understand what this means on a Pyramid, where functions */
/* OBSOLETE    *always* have a control-stack frame, but may or may not have a */
/* OBSOLETE    frame on the data stack.  Since GBD uses the value of the */
/* OBSOLETE    control stack pointer as its "address" of a frame, FRAMELESS */
/* OBSOLETE    is always 1, so does not need to be defined.  *x/ */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Where is the PC for a specific frame *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_SAVED_PC(fi) \ */
/* OBSOLETE   ((CORE_ADDR) (read_memory_integer ( (fi) -> frame + 60, 4))) */
/* OBSOLETE  */
/* OBSOLETE /* There may be bugs in FRAME_ARGS_ADDRESS and FRAME_LOCALS_ADDRESS; */
/* OBSOLETE    or there may be bugs in accessing the registers that break */
/* OBSOLETE    their definitions. */
/* OBSOLETE    Having the macros expand into functions makes them easier to debug. */
/* OBSOLETE    When the bug is finally located, the inline macro defintions can */
/* OBSOLETE    be un-#if 0ed, and frame_args_addr and frame_locals_address can */
/* OBSOLETE    be deleted from pyr-dep.c *x/  */
/* OBSOLETE  */
/* OBSOLETE /* If the argument is on the stack, it will be here.  *x/ */
/* OBSOLETE #define FRAME_ARGS_ADDRESS(fi) \ */
/* OBSOLETE   frame_args_addr(fi) */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_LOCALS_ADDRESS(fi) \ */
/* OBSOLETE   frame_locals_address(fi) */
/* OBSOLETE  */
/* OBSOLETE /* The following definitions doesn't seem to work. */
/* OBSOLETE    I don't understand why. *x/ */
/* OBSOLETE #if 0 */
/* OBSOLETE #define FRAME_ARGS_ADDRESS(fi) \ */
/* OBSOLETE    /*(FRAME_FP(fi) + (13*4))*x/ (read_register (CFP_REGNUM)) */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_LOCALS_ADDRESS(fi) \ */
/* OBSOLETE   ((fi)->frame +(16*4)) */
/* OBSOLETE  */
/* OBSOLETE #endif /* 0 *x/ */
/* OBSOLETE  */
/* OBSOLETE /* Return number of args passed to a frame. */
/* OBSOLETE    Can return -1, meaning no way to tell.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_NUM_ARGS(fi)  (-1) */
/* OBSOLETE  */
/* OBSOLETE /* Return number of bytes at start of arglist that are not really args.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_ARGS_SKIP 0 */
/* OBSOLETE  */
/* OBSOLETE /* Put here the code to store, into a struct frame_saved_regs, */
/* OBSOLETE    the addresses of the saved registers of frame described by FRAME_INFO. */
/* OBSOLETE    This includes special registers such as pc and fp saved in special */
/* OBSOLETE    ways in the stack frame.  sp is even more special: */
/* OBSOLETE    the address we return for it IS the sp for the next frame. */
/* OBSOLETE  */
/* OBSOLETE    Note that on register window machines, we are currently making the */
/* OBSOLETE    assumption that window registers are being saved somewhere in the */
/* OBSOLETE    frame in which they are being used.  If they are stored in an */
/* OBSOLETE    inferior frame, find_saved_register will break. */
/* OBSOLETE  */
/* OBSOLETE    On pyrs, frames of window registers are stored contiguously on a */
/* OBSOLETE    separate stack.  All window registers are always stored. */
/* OBSOLETE    The pc and psw (gr15 and gr14)  are also always saved: the call */
/* OBSOLETE    insn saves them in pr15 and pr14 of the new frame (tr15,tr14 of the */
/* OBSOLETE    old frame).   */
/* OBSOLETE    The data-stack frame pointer (CFP) is only saved in functions which */
/* OBSOLETE    allocate a (data)stack frame (with "adsf").  We detect them by */
/* OBSOLETE    looking at the first insn of the procedure.  */
/* OBSOLETE  */
/* OBSOLETE    Other non-window registers (gr0-gr11) are never saved.  Pyramid's C */
/* OBSOLETE    compiler and gcc currently ignore them, so it's not an issue.   *x/  */
/* OBSOLETE  */
/* OBSOLETE #define FRAME_FIND_SAVED_REGS(fi_p, frame_saved_regs) \ */
/* OBSOLETE {  register int regnum;                                                     \ */
/* OBSOLETE   register CORE_ADDR pc;                                            \ */
/* OBSOLETE   register CORE_ADDR fn_start_pc;                                   \ */
/* OBSOLETE   register int first_insn;                                          \ */
/* OBSOLETE   register CORE_ADDR prev_cf_addr;                                  \ */
/* OBSOLETE   register int window_ptr;                                          \ */
/* OBSOLETE   if (!fi_p) fatal ("Bad frame info struct in FRAME_FIND_SAVED_REGS");      \ */
/* OBSOLETE   memset (&(frame_saved_regs), '\0', sizeof (frame_saved_regs));            \ */
/* OBSOLETE                                                                     \ */
/* OBSOLETE   window_ptr = prev_cf_addr = FRAME_FP(fi_p);                               \ */
/* OBSOLETE                                                                     \ */
/* OBSOLETE   for (regnum = 16 ; regnum < 64; regnum++,window_ptr+=4)           \ */
/* OBSOLETE   {                                                                 \ */
/* OBSOLETE     (frame_saved_regs).regs[regnum] = window_ptr;                   \ */
/* OBSOLETE   }                                                                 \ */
/* OBSOLETE                                                                     \ */
/* OBSOLETE   /* In each window, psw, and pc are "saved" in tr14,tr15. *x/              \ */
/* OBSOLETE   /*** psw is sometimes saved in gr12 (so sez <sys/pcb.h>) *x/              \ */
/* OBSOLETE   (frame_saved_regs).regs[PS_REGNUM] = FRAME_FP(fi_p) + (14*4);     \ */
/* OBSOLETE                                                                     \ */
/* OBSOLETE /*(frame_saved_regs).regs[PC_REGNUM] = (frame_saved_regs).regs[31];*x/      \ */
/* OBSOLETE   (frame_saved_regs).regs[PC_REGNUM] = FRAME_FP(fi_p) + ((15+32)*4);        \ */
/* OBSOLETE                                                                     \ */
/* OBSOLETE   /* Functions that allocate a frame save sp *where*? *x/           \ */
/* OBSOLETE /*first_insn = read_memory_integer (get_pc_function_start ((fi_p)->pc),4); *x/ \ */
/* OBSOLETE                                                                     \ */
/* OBSOLETE   fn_start_pc = (get_pc_function_start ((fi_p)->pc));                       \ */
/* OBSOLETE   first_insn = read_memory_integer(fn_start_pc, 4);                 \ */
/* OBSOLETE                                                                     \ */
/* OBSOLETE   if (0x08 == ((first_insn >> 20) &0x0ff)) {                                \ */
/* OBSOLETE     /* NB: because WINDOW_REGISTER_P(cfp) is false, a saved cfp             \ */
/* OBSOLETE        in this frame is only visible in this frame's callers.               \ */
/* OBSOLETE        That means the cfp we mark saved is my caller's cfp, ie pr13.        \ */
/* OBSOLETE        I don't understand why we don't have to do that for pc, too.  *x/    \ */
/* OBSOLETE                                                                     \ */
/* OBSOLETE     (frame_saved_regs).regs[CFP_REGNUM] = FRAME_FP(fi_p)+(13*4);    \ */
/* OBSOLETE                                                                     \ */
/* OBSOLETE     (frame_saved_regs).regs[SP_REGNUM] =                            \ */
/* OBSOLETE       read_memory_integer (FRAME_FP(fi_p)+((13+32)*4),4);           \ */
/* OBSOLETE   }                                                                 \ */
/* OBSOLETE                                                                     \ */
/* OBSOLETE /*                                                                  \ */
/* OBSOLETE  *(frame_saved_regs).regs[CFP_REGNUM] = (frame_saved_regs).regs[61];        \ */
/* OBSOLETE  * (frame_saved_regs).regs[SP_REGNUM] =                                     \ */
/* OBSOLETE  *    read_memory_integer (FRAME_FP(fi_p)+((13+32)*4),4);           \ */
/* OBSOLETE  *x/                                                                        \ */
/* OBSOLETE                                                                     \ */
/* OBSOLETE   (frame_saved_regs).regs[CSP_REGNUM] = prev_cf_addr;                       \ */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Things needed for making the inferior call functions.  *x/ */
/* OBSOLETE #if 0 */
/* OBSOLETE /* These are all lies.  These macro definitions are appropriate for a */
/* OBSOLETE     SPARC. On a pyramid, pushing a dummy frame will */
/* OBSOLETE    surely involve writing the control stack pointer, */
/* OBSOLETE    then saving the pc.  This requires a privileged instruction. */
/* OBSOLETE    Maybe one day Pyramid can be persuaded to add a syscall to do this. */
/* OBSOLETE    Until then, we are out of luck. *x/ */
/* OBSOLETE  */
/* OBSOLETE /* Push an empty stack frame, to record the current PC, etc.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define PUSH_DUMMY_FRAME \ */
/* OBSOLETE { register CORE_ADDR sp = read_register (SP_REGNUM);\ */
/* OBSOLETE   register int regnum;                                  \ */
/* OBSOLETE   sp = push_word (sp, 0); /* arglist *x/                \ */
/* OBSOLETE   for (regnum = 11; regnum >= 0; regnum--)      \ */
/* OBSOLETE     sp = push_word (sp, read_register (regnum));    \ */
/* OBSOLETE   sp = push_word (sp, read_register (PC_REGNUM));   \ */
/* OBSOLETE   sp = push_word (sp, read_register (FP_REGNUM));   \ */
/* OBSOLETE /*  sp = push_word (sp, read_register (AP_REGNUM));*x/   \ */
/* OBSOLETE   sp = push_word (sp, (read_register (PS_REGNUM) & 0xffef)   \ */
/* OBSOLETE                   + 0x2fff0000);                \ */
/* OBSOLETE   sp = push_word (sp, 0);                       \ */
/* OBSOLETE   write_register (SP_REGNUM, sp);               \ */
/* OBSOLETE   write_register (FP_REGNUM, sp);               \ */
/* OBSOLETE /*  write_register (AP_REGNUM, sp + 17 * sizeof (int));*x/ } */
/* OBSOLETE  */
/* OBSOLETE /* Discard from the stack the innermost frame, restoring all registers.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define POP_FRAME  \ */
/* OBSOLETE { register CORE_ADDR fp = read_register (FP_REGNUM);                 \ */
/* OBSOLETE   register int regnum;                                               \ */
/* OBSOLETE   register int regmask = read_memory_integer (fp + 4, 4);    \ */
/* OBSOLETE   write_register (PS_REGNUM,                                         \ */
/* OBSOLETE               (regmask & 0xffff)                             \ */
/* OBSOLETE               | (read_register (PS_REGNUM) & 0xffff0000));   \ */
/* OBSOLETE   write_register (PC_REGNUM, read_memory_integer (fp + 16, 4));  \ */
/* OBSOLETE   write_register (FP_REGNUM, read_memory_integer (fp + 12, 4));  \ */
/* OBSOLETE /*  write_register (AP_REGNUM, read_memory_integer (fp + 8, 4));*x/   \ */
/* OBSOLETE   fp += 16;                                                  \ */
/* OBSOLETE   for (regnum = 0; regnum < 12; regnum++)                    \ */
/* OBSOLETE     if (regmask & (0x10000 << regnum))                               \ */
/* OBSOLETE       write_register (regnum, read_memory_integer (fp += 4, 4)); \ */
/* OBSOLETE   fp = fp + 4 + ((regmask >> 30) & 3);                               \ */
/* OBSOLETE   if (regmask & 0x20000000)                                  \ */
/* OBSOLETE     { regnum = read_memory_integer (fp, 4);                  \ */
/* OBSOLETE       fp += (regnum + 1) * 4; }                                      \ */
/* OBSOLETE   write_register (SP_REGNUM, fp);                            \ */
/* OBSOLETE   set_current_frame (read_register (FP_REGNUM)); } */
/* OBSOLETE  */
/* OBSOLETE /* This sequence of words is the instructions */
/* OBSOLETE      calls #69, @#32323232 */
/* OBSOLETE      bpt */
/* OBSOLETE    Note this is 8 bytes.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define CALL_DUMMY {0x329f69fb, 0x03323232} */
/* OBSOLETE  */
/* OBSOLETE #define CALL_DUMMY_START_OFFSET 0  /* Start execution at beginning of dummy *x/ */
/* OBSOLETE  */
/* OBSOLETE /* Insert the specified number of args and function address */
/* OBSOLETE    into a call sequence of the above form stored at DUMMYNAME.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FIX_CALL_DUMMY(dummyname, pc, fun, nargs, args, type, gcc_p)   \ */
/* OBSOLETE { *((char *) dummyname + 1) = nargs;                \ */
/* OBSOLETE   *(int *)((char *) dummyname + 3) = fun; } */
/* OBSOLETE #endif /* 0 *x/ */
/* OBSOLETE  */
/* OBSOLETE #define POP_FRAME \ */
/* OBSOLETE   { error ("The return command is not supported on this machine."); } */
