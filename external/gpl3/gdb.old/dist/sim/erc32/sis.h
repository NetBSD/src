/* Copyright (C) 1995-2016 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "ansidecl.h"
#include "gdb/callback.h"
#include "gdb/remote-sim.h"
#include <sim-config.h>
#include <stdint.h>

#if HOST_BYTE_ORDER == BIG_ENDIAN
#define HOST_BIG_ENDIAN
#define EBT 0
#else
#define HOST_LITTLE_ENDIAN
#define EBT 3
#endif

#define I_ACC_EXC 1

/* Maximum events in event queue */
#define EVENT_MAX	256

/* Maximum # of floating point queue */
#define FPUQN	1

/* Maximum # of breakpoints */
#define BPT_MAX	256

struct histype {
    unsigned        addr;
    unsigned        time;
};

/* type definitions */

typedef short int int16;	/* 16-bit signed int */
typedef unsigned short int uint16;	/* 16-bit unsigned int */
typedef int     int32;		/* 32-bit signed int */
typedef unsigned int uint32;	/* 32-bit unsigned int */
typedef float   float32;	/* 32-bit float */
typedef double  float64;	/* 64-bit float */

typedef uint64_t uint64; /* 64-bit unsigned int */
typedef int64_t int64;	   /* 64-bit signed int */

struct pstate {

    float64         fd[16];	/* FPU registers */
#ifdef HOST_LITTLE_ENDIAN
    float32         fs[32];
    float32        *fdp;
#else
    float32        *fs;
#endif
    int32          *fsi;
    uint32          fsr;
    int32           fpstate;
    uint32          fpq[FPUQN * 2];
    uint32          fpqn;
    uint32          ftime;
    uint32          flrd;
    uint32          frd;
    uint32          frs1;
    uint32          frs2;
    uint32          fpu_pres;	/* FPU present (0 = No, 1 = Yes) */

    uint32          psr;	/* IU registers */
    uint32          tbr;
    uint32          wim;
    uint32          g[8];
    uint32          r[128];
    uint32          y;
    uint32          asr17;      /* Single vector trapping */
    uint32          pc, npc;


    uint32          trap;	/* Current trap type */
    uint32          annul;	/* Instruction annul */
    uint32          data;	/* Loaded data	     */
    uint32          inst;	/* Current instruction */
    uint32          asi;	/* Current ASI */
    uint32          err_mode;	/* IU error mode */
    uint32          breakpoint;
    uint32          bptnum;
    uint32          bphit;
    uint32          bpts[BPT_MAX];	/* Breakpoints */

    uint32          ltime;	/* Load interlock time */
    uint32          hold;	/* IU hold cycles in current inst */
    uint32          fhold;	/* FPU hold cycles in current inst */
    uint32          icnt;	/* Instruction cycles in curr inst */

    uint32          histlen;	/* Trace history management */
    uint32          histind;
    struct histype *histbuf;
    float32         freq;	/* Simulated processor frequency */


    double          tottime;
    uint64          ninst;
    uint64          fholdt;
    uint64          holdt;
    uint64          icntt;
    uint64          finst;
    uint64          simstart;
    double          starttime;
    uint64          tlimit;	/* Simulation time limit */
    uint64          pwdtime;	/* Cycles in power-down mode */
    uint64          nstore;	/* Number of load instructions */
    uint64          nload;	/* Number of store instructions */
    uint64          nannul;	/* Number of annuled instructions */
    uint64          nbranch;	/* Number of branch instructions */
    uint32          ildreg;	/* Destination of last load instruction */
    uint64          ildtime;	/* Last time point for load dependency */

    int             rett_err;	/* IU in jmpl/restore error state (Rev.0) */
    int             jmpltime;
};

struct evcell {
    void            (*cfunc) ();
    int32           arg;
    uint64          time;
    struct evcell  *nxt;
};

struct estate {
    struct evcell   eq;
    struct evcell  *freeq;
    uint64          simtime;
};

struct irqcell {
    void            (*callback) ();
    int32           arg;
};


#define OK 0
#define TIME_OUT 1
#define BPT_HIT 2
#define ERROR 3
#define CTRL_C 4

/* Prototypes  */

/* erc32.c */
extern void	init_sim (void);
extern void	reset (void);
extern void	error_mode (uint32 pc);
extern void	sim_halt (void);
extern void	exit_sim (void);
extern void	init_stdio (void);
extern void	restore_stdio (void);
extern int	memory_iread (uint32 addr, uint32 *data, int32 *ws);
extern int	memory_read (int32 asi, uint32 addr, uint32 *data,
			     int32 sz, int32 *ws);
extern int	memory_write (int32 asi, uint32 addr, uint32 *data,
			      int32 sz, int32 *ws);
extern int	sis_memory_write (uint32 addr,
				  const unsigned char *data, uint32 length);
extern int	sis_memory_read (uint32 addr, char *data,
				 uint32 length);

/* func.c */
extern struct pstate  sregs;
extern void	set_regi (struct pstate *sregs, int32 reg,
			  uint32 rval);
extern void	get_regi (struct pstate *sregs, int32 reg, char *buf);
extern int	exec_cmd (struct pstate *sregs, const char *cmd);
extern void	reset_stat (struct pstate  *sregs);
extern void	show_stat (struct pstate  *sregs);
extern void	init_bpt (struct pstate  *sregs);
extern void	init_signals (void);

struct disassemble_info;
extern void	dis_mem (uint32 addr, uint32 len,
			 struct disassemble_info *info);
extern void	event (void (*cfunc) (), int32 arg, uint64 delta);
extern void	set_int (int32 level, void (*callback) (), int32 arg);
extern void	advance_time (struct pstate  *sregs);
extern uint32	now (void);
extern int	wait_for_irq (void);
extern int	check_bpt (struct pstate *sregs);
extern void	reset_all (void);
extern void	sys_reset (void);
extern void	sys_halt (void);
extern int	bfd_load (const char *fname);
extern double	get_time (void);

/* exec.c */
extern int	dispatch_instruction (struct pstate *sregs);
extern int	execute_trap (struct pstate *sregs);
extern int	check_interrupts (struct pstate  *sregs);
extern void	init_regs (struct pstate *sregs);

/* interf.c */
extern int	run_sim (struct pstate *sregs,
			 uint64 icount, int dis);

/* float.c */
extern int	get_accex (void);
extern void	clear_accex (void);
extern void	set_fsr (uint32 fsr);

/* help.c */
extern void	usage (void);
extern void	gen_help (void);
