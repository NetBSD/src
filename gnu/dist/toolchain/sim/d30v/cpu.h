/* Mitsubishi Electric Corp. D30V Simulator.
   Copyright (C) 1997, Free Software Foundation, Inc.
   Contributed by Cygnus Support.

This file is part of GDB, the GNU debugger.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


#ifndef _CPU_H_
#define _CPU_H_

enum {
  NR_GENERAL_PURPOSE_REGISTERS = 64,
  NR_CONTROL_REGISTERS = 64,
  NR_ACCUMULATORS = 2,
  STACK_POINTER_GPR = 63,
  NR_STACK_POINTERS = 2,
};

enum {
  processor_status_word_cr = 0,
  backup_processor_status_word_cr = 1,
  program_counter_cr = 2,
  backup_program_counter_cr = 3,
  debug_backup_processor_status_word_cr = 4,
  debug_backup_program_counter_cr = 5,
  reserved_6_cr = 6,
  repeat_count_cr = 7,
  repeat_start_address_cr = 8,
  repeat_end_address_cr = 9,
  modulo_start_address_cr = 10,
  modulo_end_address_cr = 11,
  instruction_break_address_cr = 14,
  eit_vector_base_cr = 15,
};


enum {
  PSW_SM = 0,
  PSW_EA = 2,
  PSW_DB = 3,
  PSW_DS = 4,
  PSW_IE = 5,
  PSW_RP = 6,
  PSW_MD = 7,
  PSW_F0 = 17,
  PSW_F1 = 19,
  PSW_F2 = 21,
  PSW_F3 = 23,
  PSW_S = 25,
  PSW_V = 27,
  PSW_VA = 29,
  PSW_C = 31,
};

/* aliases for PSW flag numbers (F0..F7) */
enum
{
  PSW_S_FLAG = 4,
};

typedef struct _registers {
  unsigned32 general_purpose[NR_GENERAL_PURPOSE_REGISTERS];
  /* keep track of the stack pointer */
  unsigned32 sp[NR_STACK_POINTERS]; /* swap with SP */
  unsigned32 current_sp;
  unsigned32 control[NR_CONTROL_REGISTERS];
  unsigned64 accumulator[NR_ACCUMULATORS];
} registers;

typedef enum _cpu_units {
  memory_unit,
  integer_unit,
  any_unit,
} cpu_units;

/* In order to support parallel instructions, which one instruction can be
   writing to a register that is used as input to another, queue up the
   writes to the end of the instruction boundaries.  */

#define MAX_WRITE32	16
#define MAX_WRITE64	2

struct _write32 {
  int num;				/* # of 32-bit writes queued up */
  unsigned32 value[MAX_WRITE32];	/* value to write */
  unsigned32 mask[MAX_WRITE32];		/* mask to use */
  unsigned32 *ptr[MAX_WRITE32];		/* address to write to */
};

struct _write64 {
  int num;				/* # of 64-bit writes queued up */
  unsigned64 value[MAX_WRITE64];	/* value to write */
  unsigned64 *ptr[MAX_WRITE64];		/* address to write to */
};

struct _sim_cpu {
  cpu_units unit;
  registers regs;
  sim_cpu_base base;
  int trace_call_p;			/* Whether to do call tracing.  */
  int trace_trap_p;			/* If unknown traps dump out the regs */
  int trace_action;			/* trace bits at end of instructions */
  int left_kills_right_p;               /* left insn kills insn in right slot of -> */
  int mvtsys_left_p;			/* left insn was mvtsys */
  int did_trap;				/* we did a trap & need to finish it */
  struct _write32 write32;		/* queued up 32-bit writes */
  struct _write64 write64;		/* queued up 64-bit writes */
};

#define PC	(STATE_CPU (sd, 0)->regs.control[program_counter_cr])
#define PSW 	(STATE_CPU (sd, 0)->regs.control[processor_status_word_cr])
#define PSWL    (*AL2_4(&PSW))
#define PSWH    (*AH2_4(&PSW))
#define DPSW 	(STATE_CPU (sd, 0)->regs.control[debug_backup_processor_status_word_cr])
#define DPC 	(STATE_CPU (sd, 0)->regs.control[debug_backup_program_counter_cr])
#define bPC 	(STATE_CPU (sd, 0)->regs.control[backup_program_counter_cr])
#define bPSW 	(STATE_CPU (sd, 0)->regs.control[backup_processor_status_word_cr])
#define RPT_C 	(STATE_CPU (sd, 0)->regs.control[repeat_count_cr])
#define RPT_S 	(STATE_CPU (sd, 0)->regs.control[repeat_start_address_cr])
#define RPT_E 	(STATE_CPU (sd, 0)->regs.control[repeat_end_address_cr])
#define MOD_S 	(STATE_CPU (sd, 0)->regs.control[modulo_start_address_cr])
#define MOD_E 	(STATE_CPU (sd, 0)->regs.control[modulo_end_address_cr])
#define IBA 	(STATE_CPU (sd, 0)->regs.control[instruction_break_address_cr])
#define EIT_VB	(STATE_CPU (sd, 0)->regs.control[eit_vector_base_cr])
#define GPR	(STATE_CPU (sd, 0)->regs.general_purpose)
#define GPR_SET(N,VAL) (GPR[(N)] = (VAL))
#define ACC	(STATE_CPU (sd, 0)->regs.accumulator)
#define CREG	(STATE_CPU (sd, 0)->regs.control)
#define SP      (GPR[STACK_POINTER_GPR])
#define TRACE_CALL_P (STATE_CPU (sd, 0)->trace_call_p)
#define TRACE_TRAP_P (STATE_CPU (sd, 0)->trace_trap_p)
#define TRACE_ACTION (STATE_CPU (sd, 0)->trace_action)
#define     TRACE_ACTION_CALL	0x00000001	/* call occurred */
#define     TRACE_ACTION_RETURN	0x00000002	/* return occurred */

#define WRITE32 (STATE_CPU (sd, 0)->write32)
#define WRITE32_NUM	 (WRITE32.num)
#define WRITE32_PTR(N)	 (WRITE32.ptr[N])
#define WRITE32_MASK(N)	 (WRITE32.mask[N])
#define WRITE32_VALUE(N) (WRITE32.value[N])
#define WRITE32_QUEUE(PTR, VALUE) WRITE32_QUEUE_MASK (PTR, VALUE, 0xffffffff)

#define WRITE32_QUEUE_MASK(PTR, VALUE, MASK)				\
do {									\
  int _num = WRITE32_NUM;						\
  if (_num >= MAX_WRITE32)						\
    sim_engine_abort (sd, STATE_CPU (sd, 0), cia,			\
		      "Too many queued 32-bit writes");			\
  WRITE32_PTR(_num) = PTR;						\
  WRITE32_VALUE(_num) = VALUE;						\
  WRITE32_MASK(_num) = MASK;						\
  WRITE32_NUM = _num+1;							\
} while (0)

#define DID_TRAP	(STATE_CPU (sd, 0)->did_trap)

#define WRITE64 (STATE_CPU (sd, 0)->write64)
#define WRITE64_NUM	 (WRITE64.num)
#define WRITE64_PTR(N)	 (WRITE64.ptr[N])
#define WRITE64_VALUE(N) (WRITE64.value[N])
#define WRITE64_QUEUE(PTR, VALUE)					\
do {									\
  int _num = WRITE64_NUM;						\
  if (_num >= MAX_WRITE64)						\
    sim_engine_abort (sd, STATE_CPU (sd, 0), cia,			\
		      "Too many queued 64-bit writes");			\
  WRITE64_PTR(_num) = PTR;						\
  WRITE64_VALUE(_num) = VALUE;						\
  WRITE64_NUM = _num+1;							\
} while (0)

#define DPSW_VALID	0xbf005555
#define PSW_VALID	0xb7005555
#define EIT_VALID	0xfffff000	/* From page 7-4 of D30V/MPEG arch. manual  */
#define EIT_VB_DEFAULT	0xfffff000	/* Value of the EIT_VB register after reset */

/* Verify that the instruction is in the correct slot */

#define IS_WRONG_SLOT is_wrong_slot(sd, cia, MY_INDEX)
extern int is_wrong_slot
(SIM_DESC sd,
 address_word cia,
 itable_index index);

#define IS_CONDITION_OK is_condition_ok(sd, cia, CCC)
extern int is_condition_ok
(SIM_DESC sd,
 address_word cia,
 int cond);

#define SIM_HAVE_BREAKPOINTS	/* Turn on internal breakpoint module */

/* Internal breakpoint instruction is syscall 5 */
#define SIM_BREAKPOINT {0x0e, 0x00, 0x00, 0x05}
#define SIM_BREAKPOINT_SIZE (4)

/* Call occurred */
extern void call_occurred
(SIM_DESC sd,
 sim_cpu *cpu,
 address_word cia,
 address_word nia);

/* Return occurred */
extern void return_occurred
(SIM_DESC sd,
 sim_cpu *cpu,
 address_word cia,
 address_word nia);

/* Whether to do call tracing.  */
extern int d30v_call_trace_p;

/* Read/write functions for system call interface.  */
extern int d30v_read_mem
(host_callback *cb,
 struct cb_syscall *sc,
 unsigned long taddr,
 char *buf,
 int bytes);

extern int d30v_write_mem
(host_callback *cb,
 struct cb_syscall *sc,
 unsigned long taddr,
 const char *buf,
 int bytes);

/* Process all of the queued up writes in order now */
void unqueue_writes
(SIM_DESC sd,
 sim_cpu *cpu,
 address_word cia);

#endif /* _CPU_H_ */
