/* Definitions for dealing with stack frames, for GDB, the GNU debugger.

   Copyright 1986, 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1996,
   1997, 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#if !defined (FRAME_H)
#define FRAME_H 1

/* Return the location (and possibly value) of REGNUM for the previous
   (older, up) frame.  All parameters except VALUEP can be assumed to
   be non NULL.  When VALUEP is NULL, just the location of the
   register should be returned.

   UNWIND_CACHE is provided as mechanism for implementing a per-frame
   local cache.  It's initial value being NULL.  Memory for that cache
   should be allocated using frame_obstack_alloc().

   Register window architectures (eg SPARC) should note that REGNUM
   identifies the register for the previous frame.  For instance, a
   request for the value of "o1" for the previous frame would be found
   in the register "i1" in this FRAME.  */

typedef void (frame_register_unwind_ftype) (struct frame_info *frame,
					    void **unwind_cache,
					    int regnum,
					    int *optimized,
					    enum lval_type *lvalp,
					    CORE_ADDR *addrp,
					    int *realnump,
					    void *valuep);

/* Describe the saved registers of a frame.  */

#if defined (EXTRA_FRAME_INFO) || defined (FRAME_FIND_SAVED_REGS)
/* XXXX - deprecated */
struct frame_saved_regs
  {
    /* For each register R (except the SP), regs[R] is the address at
       which it was saved on entry to the frame, or zero if it was not
       saved on entry to this frame.  This includes special registers
       such as pc and fp saved in special ways in the stack frame.

       regs[SP_REGNUM] is different.  It holds the actual SP, not the
       address at which it was saved.  */

    CORE_ADDR regs[NUM_REGS];
  };
#endif

/* We keep a cache of stack frames, each of which is a "struct
   frame_info".  The innermost one gets allocated (in
   wait_for_inferior) each time the inferior stops; current_frame
   points to it.  Additional frames get allocated (in
   get_prev_frame) as needed, and are chained through the next
   and prev fields.  Any time that the frame cache becomes invalid
   (most notably when we execute something, but also if we change how
   we interpret the frames (e.g. "set heuristic-fence-post" in
   mips-tdep.c, or anything which reads new symbols)), we should call
   reinit_frame_cache.  */

struct frame_info
  {
    /* Nominal address of the frame described.  See comments at FRAME_FP
       about what this means outside the *FRAME* macros; in the *FRAME*
       macros, it can mean whatever makes most sense for this machine.  */
    CORE_ADDR frame;

    /* Address at which execution is occurring in this frame.
       For the innermost frame, it's the current pc.
       For other frames, it is a pc saved in the next frame.  */
    CORE_ADDR pc;

    /* Level of this frame.  The inner-most (youngest) frame is at
       level 0.  As you move towards the outer-most (oldest) frame,
       the level increases.  This is a cached value.  It could just as
       easily be computed by counting back from the selected frame to
       the inner most frame.  */
    /* NOTE: cagney/2002-04-05: Perhaphs a level of ``-1'' should be
       reserved to indicate a bogus frame - one that has been created
       just to keep GDB happy (GDB always needs a frame).  For the
       moment leave this as speculation.  */
    int level;

    /* Nonzero if this is a frame associated with calling a signal handler.

       Set by machine-dependent code.  On some machines, if
       the machine-dependent code fails to check for this, the backtrace
       will look relatively normal.  For example, on the i386
       #3  0x158728 in sighold ()
       On other machines (e.g. rs6000), the machine-dependent code better
       set this to prevent us from trying to print it like a normal frame.  */
    int signal_handler_caller;

    /* For each register, address of where it was saved on entry to
       the frame, or zero if it was not saved on entry to this frame.
       This includes special registers such as pc and fp saved in
       special ways in the stack frame.  The SP_REGNUM is even more
       special, the address here is the sp for the previous frame, not
       the address where the sp was saved.  */
    /* Allocated by frame_saved_regs_zalloc () which is called /
       initialized by FRAME_INIT_SAVED_REGS(). */
    CORE_ADDR *saved_regs;	/*NUM_REGS + NUM_PSEUDO_REGS*/

#ifdef EXTRA_FRAME_INFO
    /* XXXX - deprecated */
    /* Anything extra for this structure that may have been defined
       in the machine dependent files. */
      EXTRA_FRAME_INFO
#endif

    /* Anything extra for this structure that may have been defined
       in the machine dependent files. */
    /* Allocated by frame_obstack_alloc () which is called /
       initialized by INIT_EXTRA_FRAME_INFO */
    struct frame_extra_info *extra_info;

    /* If dwarf2 unwind frame informations is used, this structure holds all
       related unwind data.  */
    struct context *context;

    /* See description above.  Return the register value for the
       previous frame.  */
    frame_register_unwind_ftype *register_unwind;
    void *register_unwind_cache;

    /* Pointers to the next (down, inner) and previous (up, outer)
       frame_info's in the frame cache.  */
    struct frame_info *next; /* down, inner */
    struct frame_info *prev; /* up, outer */
  };

/* Values for the source flag to be used in print_frame_info_base(). */
enum print_what
  { 
    /* Print only the source line, like in stepi. */
    SRC_LINE = -1, 
    /* Print only the location, i.e. level, address (sometimes)
       function, args, file, line, line num. */
    LOCATION,
    /* Print both of the above. */
    SRC_AND_LOC, 
    /* Print location only, but always include the address. */
    LOC_AND_ADDRESS 
  };

/* Allocate additional space for appendices to a struct frame_info.
   NOTE: Much of GDB's code works on the assumption that the allocated
   saved_regs[] array is the size specified below.  If you try to make
   that array smaller, GDB will happily walk off its end. */

#ifdef SIZEOF_FRAME_SAVED_REGS
#error "SIZEOF_FRAME_SAVED_REGS can not be re-defined"
#endif
#define SIZEOF_FRAME_SAVED_REGS \
        (sizeof (CORE_ADDR) * (NUM_REGS+NUM_PSEUDO_REGS))

extern void *frame_obstack_alloc (unsigned long size);
extern void frame_saved_regs_zalloc (struct frame_info *);

/* Return the frame address from FI.  Except in the machine-dependent
   *FRAME* macros, a frame address has no defined meaning other than
   as a magic cookie which identifies a frame over calls to the
   inferior.  The only known exception is inferior.h
   (PC_IN_CALL_DUMMY) [ON_STACK]; see comments there.  You cannot
   assume that a frame address contains enough information to
   reconstruct the frame; if you want more than just to identify the
   frame (e.g. be able to fetch variables relative to that frame),
   then save the whole struct frame_info (and the next struct
   frame_info, since the latter is used for fetching variables on some
   machines).  */

#define FRAME_FP(fi) ((fi)->frame)

/* Level of the frame: 0 for innermost, 1 for its caller, ...; or -1
   for an invalid frame.  */

extern int frame_relative_level (struct frame_info *fi);

/* Define a default FRAME_CHAIN_VALID, in the form that is suitable for most
   targets.  If FRAME_CHAIN_VALID returns zero it means that the given frame
   is the outermost one and has no caller.

   XXXX - both default and alternate frame_chain_valid functions are
   deprecated.  New code should use dummy frames and one of the
   generic functions. */

extern int file_frame_chain_valid (CORE_ADDR, struct frame_info *);
extern int func_frame_chain_valid (CORE_ADDR, struct frame_info *);
extern int nonnull_frame_chain_valid (CORE_ADDR, struct frame_info *);
extern int generic_file_frame_chain_valid (CORE_ADDR, struct frame_info *);
extern int generic_func_frame_chain_valid (CORE_ADDR, struct frame_info *);
extern void generic_save_dummy_frame_tos (CORE_ADDR sp);

/* The stack frame that the user has specified for commands to act on.
   Note that one cannot assume this is the address of valid data.  */

extern struct frame_info *selected_frame;

/* Level of the selected frame:
   0 for innermost, 1 for its caller, ...
   or -1 for frame specified by address with no defined level.  */

extern struct frame_info *create_new_frame (CORE_ADDR, CORE_ADDR);

extern void flush_cached_frames (void);

extern void reinit_frame_cache (void);


#ifdef FRAME_FIND_SAVED_REGS
/* XXX - deprecated */
#define FRAME_INIT_SAVED_REGS(FI) get_frame_saved_regs (FI, NULL)
extern void get_frame_saved_regs (struct frame_info *,
				  struct frame_saved_regs *);
#endif

extern void set_current_frame (struct frame_info *);

extern struct frame_info *get_prev_frame (struct frame_info *);

extern struct frame_info *get_current_frame (void);

extern struct frame_info *get_next_frame (struct frame_info *);

extern struct block *get_frame_block (struct frame_info *,
                                      CORE_ADDR *addr_in_block);

extern struct block *get_current_block (CORE_ADDR *addr_in_block);

extern struct block *get_selected_block (CORE_ADDR *addr_in_block);

extern struct symbol *get_frame_function (struct frame_info *);

extern CORE_ADDR get_frame_pc (struct frame_info *);

extern CORE_ADDR frame_address_in_block (struct frame_info *);

extern CORE_ADDR get_pc_function_start (CORE_ADDR);

extern struct block *block_for_pc (CORE_ADDR);

extern struct block *block_for_pc_sect (CORE_ADDR, asection *);

extern int frameless_look_for_prologue (struct frame_info *);

extern void print_frame_args (struct symbol *, struct frame_info *,
			      int, struct ui_file *);

extern struct frame_info *find_relative_frame (struct frame_info *, int *);

extern void show_and_print_stack_frame (struct frame_info *fi, int level,
					int source);

extern void print_stack_frame (struct frame_info *, int, int);

extern void print_only_stack_frame (struct frame_info *, int, int);

extern void show_stack_frame (struct frame_info *);

extern void select_frame (struct frame_info *);

/* Return an ID that can be used to re-find a frame.  */

struct frame_id
{
  /* The frame's address.  This should be constant through out the
     lifetime of a frame.  */
  CORE_ADDR base;
  /* The frame's current PC.  While this changes, the function that
     the PC falls into, does not.  */
  CORE_ADDR pc;
};

extern void get_frame_id (struct frame_info *fi, struct frame_id *id);

extern struct frame_info *frame_find_by_id (struct frame_id id);

extern void print_frame_info (struct frame_info *, int, int, int);

extern void show_frame_info (struct frame_info *, int, int, int);

extern CORE_ADDR find_saved_register (struct frame_info *, int);

extern struct frame_info *block_innermost_frame (struct block *);

extern struct frame_info *find_frame_addr_in_frame_chain (CORE_ADDR);

extern CORE_ADDR sigtramp_saved_pc (struct frame_info *);

extern CORE_ADDR generic_read_register_dummy (CORE_ADDR pc,
					      CORE_ADDR fp, int);
extern void generic_push_dummy_frame (void);
extern void generic_pop_current_frame (void (*)(struct frame_info *));
extern void generic_pop_dummy_frame (void);

extern int generic_pc_in_call_dummy (CORE_ADDR pc,
				     CORE_ADDR sp, CORE_ADDR fp);

/* NOTE: cagney/2002-06-26: Targets should no longer use this
   function.  Instead, the contents of a dummy frames registers can be
   obtained by applying: frame_register_unwind to the dummy frame; or
   get_saved_register to the next outer frame.  */

extern char *deprecated_generic_find_dummy_frame (CORE_ADDR pc, CORE_ADDR fp);

extern void generic_fix_call_dummy (char *dummy, CORE_ADDR pc, CORE_ADDR fun,
				    int nargs, struct value **args,
				    struct type *type, int gcc_p);

extern void generic_get_saved_register (char *, int *, CORE_ADDR *,
					struct frame_info *, int,
					enum lval_type *);

extern void generic_unwind_get_saved_register (char *raw_buffer,
					       int *optimized,
					       CORE_ADDR * addrp,
					       struct frame_info *frame,
					       int regnum,
					       enum lval_type *lval);

/* Unwind the stack frame so that the value of REGNUM, in the previous
   frame is returned.  If VALUEP is NULL, don't fetch/compute the
   value.  Instead just return the location of the value.  */

extern void frame_register_unwind (struct frame_info *frame, int regnum,
				   int *optimizedp, enum lval_type *lvalp,
				   CORE_ADDR *addrp, int *realnump,
				   void *valuep);

extern void generic_save_call_dummy_addr (CORE_ADDR lo, CORE_ADDR hi);

extern void get_saved_register (char *raw_buffer, int *optimized,
				CORE_ADDR * addrp,
				struct frame_info *frame,
				int regnum, enum lval_type *lval);

/* Return the register as found on the FRAME.  Return zero if the
   register could not be found.  */
extern int frame_register_read (struct frame_info *frame, int regnum,
				void *buf);

/* Map between a frame register number and its name.  A frame register
   space is a superset of the cooked register space --- it also
   includes builtin registers.  */

extern int frame_map_name_to_regnum (const char *name, int strlen);
extern const char *frame_map_regnum_to_name (int regnum);

#endif /* !defined (FRAME_H)  */
