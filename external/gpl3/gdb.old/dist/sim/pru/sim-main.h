/* Copyright 2016-2023 Free Software Foundation, Inc.
   Contributed by Dimitar Dimitrov <dimitar@dinux.eu>

   This file is part of the PRU simulator.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifndef PRU_SIM_MAIN
#define PRU_SIM_MAIN

#include <stdint.h>
#include <stddef.h>
#include "pru.h"
#include "sim-basics.h"

#include "sim-base.h"

/* The machine state.
   This state is maintained in host byte order.  The
   fetch/store register functions must translate between host
   byte order and the target processor byte order.
   Keeping this data in target byte order simplifies the register
   read/write functions.  Keeping this data in host order improves
   the performance of the simulator.  Simulation speed is deemed more
   important.  */

/* For clarity, please keep the same relative order in this enum as in the
   corresponding group of GP registers.

   In PRU ISA, Multiplier-Accumulator-Unit's registers are like "shadows" of
   the GP registers.  MAC registers are implicitly addressed when executing
   the XIN/XOUT instructions to access them.  Transfer to/from a MAC register
   can happen only from/to its corresponding GP peer register.  */

enum pru_macreg_id {
    /* MAC register	  CPU GP register     Description.  */
    PRU_MACREG_MODE,	  /* r25 */	      /* Mode (MUL/MAC).  */
    PRU_MACREG_PROD_L,	  /* r26 */	      /* Lower 32 bits of product.  */
    PRU_MACREG_PROD_H,	  /* r27 */	      /* Higher 32 bits of product.  */
    PRU_MACREG_OP_0,	  /* r28 */	      /* First operand.  */
    PRU_MACREG_OP_1,	  /* r29 */	      /* Second operand.  */
    PRU_MACREG_ACC_L,	  /* N/A */	      /* Accumulator (not exposed)  */
    PRU_MACREG_ACC_H,	  /* N/A */	      /* Higher 32 bits of MAC
						 accumulator.  */
    PRU_MAC_NREGS
};

struct pru_regset
{
  uint32_t	  regs[32];		/* Primary registers.  */
  uint16_t	  pc;			/* IMEM _word_ address.  */
  uint32_t	  pc_addr_space_marker; /* IMEM virtual linker offset.  This
					   is the artificial offset that
					   we invent in order to "separate"
					   the DMEM and IMEM memory spaces.  */
  unsigned int	  carry : 1;
  uint32_t	  ctable[32];		/* Constant offsets table for xBCO.  */
  uint32_t	  macregs[PRU_MAC_NREGS];
  uint32_t	  scratchpads[XFRID_MAX + 1][32];
  struct {
    uint16_t looptop;			/* LOOP top (PC of loop instr).  */
    uint16_t loopend;			/* LOOP end (PC of loop end label).  */
    int loop_in_progress;		/* Whether to check for PC==loopend.  */
    uint32_t loop_counter;		/* LOOP counter.  */
  } loop;
  int		  cycles;
  int		  insts;
};

struct _sim_cpu {
  struct pru_regset pru_cpu;
  sim_cpu_base base;
};

#endif /* PRU_SIM_MAIN */
