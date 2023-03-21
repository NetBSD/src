/* Copyright 2016-2020 Free Software Foundation, Inc.
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

#ifndef PRU_H
#define PRU_H

#include "config.h"
#include "opcode/pru.h"

/* Needed for handling the dual PRU address space.  */
#define IMEM_ADDR_MASK	((1u << 23) - 1)

#define IMEM_ADDR_DEFAULT 0x20000000

/* Define memory sizes to allocate for simulated target.  Sizes are
   artificially large to accommodate execution of compiler test suite.
   Please synchronize with the linker script for prusim target.  */
#define DMEM_DEFAULT_SIZE (64 * 1024 * 1024)

/* 16-bit word addressable space.  */
#define IMEM_DEFAULT_SIZE (64 * 4 * 1024)

/* For AM335x SoCs.  */
#define XFRID_SCRATCH_BANK_0	  10
#define XFRID_SCRATCH_BANK_1	  11
#define XFRID_SCRATCH_BANK_2	  12
#define XFRID_SCRATCH_BANK_PEER	  14
#define XFRID_MAX		  255

#define CPU     (cpu->pru_cpu)

#define PC		(CPU.pc)
#define PC_byteaddr	((PC << 2) | PC_ADDR_SPACE_MARKER)

/* Various opcode fields.  */
#define RS1 extract_regval (CPU.regs[GET_INSN_FIELD (RS1, inst)], \
			    GET_INSN_FIELD (RS1SEL, inst))
#define RS2 extract_regval (CPU.regs[GET_INSN_FIELD (RS2, inst)], \
			    GET_INSN_FIELD (RS2SEL, inst))

#define RS2_w0 extract_regval (CPU.regs[GET_INSN_FIELD (RS2, inst)], \
			       RSEL_15_0)

#define XBBO_BASEREG (CPU.regs[GET_INSN_FIELD (RS1, inst)])

#define RDSEL GET_INSN_FIELD (RDSEL, inst)
#define RD_WIDTH regsel_width (RDSEL)
#define RD_REGN GET_INSN_FIELD (RD, inst)
#define IO GET_INSN_FIELD (IO, inst)
#define IMM8 GET_INSN_FIELD (IMM8, inst)
#define IMM16 GET_INSN_FIELD (IMM16, inst)
#define WAKEONSTATUS GET_INSN_FIELD (WAKEONSTATUS, inst)
#define CB GET_INSN_FIELD (CB, inst)
#define RDB GET_INSN_FIELD (RDB, inst)
#define XFR_WBA GET_INSN_FIELD (XFR_WBA, inst)
#define LOOP_JMPOFFS GET_INSN_FIELD (LOOP_JMPOFFS, inst)
#define BROFF ((uint32_t) GET_BROFF_SIGNED (inst))

#define _BURSTLEN_CALCULATE(BITFIELD)					    \
  ((BITFIELD) >= LSSBBO_BYTECOUNT_R0_BITS7_0 ?				    \
  (CPU.regs[0] >> ((BITFIELD) - LSSBBO_BYTECOUNT_R0_BITS7_0) * 8) & 0xff    \
  : (BITFIELD) + 1)

#define BURSTLEN _BURSTLEN_CALCULATE (GET_BURSTLEN (inst))
#define XFR_LENGTH _BURSTLEN_CALCULATE (GET_INSN_FIELD (XFR_LENGTH, inst))

#define DO_XIN(wba,regn,rdb,l)	  \
  pru_sim_xin (sd, cpu, (wba), (regn), (rdb), (l))
#define DO_XOUT(wba,regn,rdb,l)	  \
  pru_sim_xout (sd, cpu, (wba), (regn), (rdb), (l))
#define DO_XCHG(wba,regn,rdb,l)	  \
  pru_sim_xchg (sd, cpu, (wba), (regn), (rdb), (l))

#define RAISE_SIGILL(sd)  sim_engine_halt ((sd), NULL, NULL, PC_byteaddr, \
					   sim_stopped, SIM_SIGILL)
#define RAISE_SIGINT(sd)  sim_engine_halt ((sd), NULL, NULL, PC_byteaddr, \
					   sim_stopped, SIM_SIGINT)

#define MAC_R25_MAC_MODE_MASK	  (1u << 0)
#define MAC_R25_ACC_CARRY_MASK	  (1u << 1)

#define CARRY	CPU.carry
#define CTABLE	CPU.ctable

#define PC_ADDR_SPACE_MARKER	CPU.pc_addr_space_marker

#define LOOPTOP		  CPU.loop.looptop
#define LOOPEND		  CPU.loop.loopend
#define LOOP_IN_PROGRESS  CPU.loop.loop_in_progress
#define LOOPCNT		  CPU.loop.loop_counter

/* 32 GP registers plus PC.  */
#define NUM_REGS	33

#endif /* PRU_H */
