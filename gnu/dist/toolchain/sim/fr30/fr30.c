/* fr30 simulator support code
   Copyright (C) 1998, 1999 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions.

This file is part of the GNU simulators.

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

#define WANT_CPU
#define WANT_CPU_FR30BF

#include "sim-main.h"
#include "cgen-mem.h"
#include "cgen-ops.h"

/* Convert gdb dedicated register number to actual dr reg number.  */

static int
decode_gdb_dr_regnum (int gdb_regnum)
{
  switch (gdb_regnum)
    {
    case TBR_REGNUM : return H_DR_TBR;
    case RP_REGNUM : return H_DR_RP;
    case SSP_REGNUM : return H_DR_SSP;
    case USP_REGNUM : return H_DR_USP;
    case MDH_REGNUM : return H_DR_MDH;
    case MDL_REGNUM : return H_DR_MDL;
    }
  abort ();
}

/* The contents of BUF are in target byte order.  */

int
fr30bf_fetch_register (SIM_CPU *current_cpu, int rn, unsigned char *buf, int len)
{
  if (rn < 16)
    SETTWI (buf, fr30bf_h_gr_get (current_cpu, rn));
  else
    switch (rn)
      {
      case PC_REGNUM :
	SETTWI (buf, fr30bf_h_pc_get (current_cpu));
	break;
      case PS_REGNUM :
	SETTWI (buf, fr30bf_h_ps_get (current_cpu));
	break;
      case TBR_REGNUM :
      case RP_REGNUM :
      case SSP_REGNUM :
      case USP_REGNUM :
      case MDH_REGNUM :
      case MDL_REGNUM :
	SETTWI (buf, fr30bf_h_dr_get (current_cpu,
				      decode_gdb_dr_regnum (rn)));
	break;
      default :
	return 0;
      }

  return -1; /*FIXME*/
}

/* The contents of BUF are in target byte order.  */

int
fr30bf_store_register (SIM_CPU *current_cpu, int rn, unsigned char *buf, int len)
{
  if (rn < 16)
    fr30bf_h_gr_set (current_cpu, rn, GETTWI (buf));
  else
    switch (rn)
      {
      case PC_REGNUM :
	fr30bf_h_pc_set (current_cpu, GETTWI (buf));
	break;
      case PS_REGNUM :
	fr30bf_h_ps_set (current_cpu, GETTWI (buf));
	break;
      case TBR_REGNUM :
      case RP_REGNUM :
      case SSP_REGNUM :
      case USP_REGNUM :
      case MDH_REGNUM :
      case MDL_REGNUM :
	fr30bf_h_dr_set (current_cpu,
			 decode_gdb_dr_regnum (rn),
			 GETTWI (buf));
	break;
      default :
	return 0;
      }

  return -1; /*FIXME*/
}

/* Cover fns to access the ccr bits.  */

BI
fr30bf_h_sbit_get_handler (SIM_CPU *current_cpu)
{
  return CPU (h_sbit);
}

void
fr30bf_h_sbit_set_handler (SIM_CPU *current_cpu, BI newval)
{
  int old_sbit = CPU (h_sbit);
  int new_sbit = (newval != 0);

  CPU (h_sbit) = new_sbit;

  /* When switching stack modes, update the registers.  */
  if (old_sbit != new_sbit)
    {
      if (old_sbit)
	{
	  /* Switching user -> system.  */
	  CPU (h_dr[H_DR_USP]) = CPU (h_gr[H_GR_SP]);
	  CPU (h_gr[H_GR_SP]) = CPU (h_dr[H_DR_SSP]);
	}
      else
	{
	  /* Switching system -> user.  */
	  CPU (h_dr[H_DR_SSP]) = CPU (h_gr[H_GR_SP]);
	  CPU (h_gr[H_GR_SP]) = CPU (h_dr[H_DR_USP]);
	}
    }

  /* TODO: r15 interlock */
}

/* Cover fns to access the ccr bits.  */

UQI
fr30bf_h_ccr_get_handler (SIM_CPU *current_cpu)
{
  int ccr = (  (GET_H_CBIT () << 0)
	     | (GET_H_VBIT () << 1)
	     | (GET_H_ZBIT () << 2)
	     | (GET_H_NBIT () << 3)
	     | (GET_H_IBIT () << 4)
	     | (GET_H_SBIT () << 5));

  return ccr;
}

void
fr30bf_h_ccr_set_handler (SIM_CPU *current_cpu, UQI newval)
{
  int ccr = newval & 0x3f;

  SET_H_CBIT ((ccr & 1) != 0);
  SET_H_VBIT ((ccr & 2) != 0);
  SET_H_ZBIT ((ccr & 4) != 0);
  SET_H_NBIT ((ccr & 8) != 0);
  SET_H_IBIT ((ccr & 0x10) != 0);
  SET_H_SBIT ((ccr & 0x20) != 0);
}

/* Cover fns to access the scr bits.  */

UQI
fr30bf_h_scr_get_handler (SIM_CPU *current_cpu)
{
  int scr = (  (GET_H_TBIT () << 0)
	     | (GET_H_D0BIT () << 1)
	     | (GET_H_D1BIT () << 2));
  return scr;
}

void
fr30bf_h_scr_set_handler (SIM_CPU *current_cpu, UQI newval)
{
  int scr = newval & 7;

  SET_H_TBIT  ((scr & 1) != 0);
  SET_H_D0BIT ((scr & 2) != 0);
  SET_H_D1BIT ((scr & 4) != 0);
}

/* Cover fns to access the ilm bits.  */

UQI
fr30bf_h_ilm_get_handler (SIM_CPU *current_cpu)
{
  return CPU (h_ilm);
}

void
fr30bf_h_ilm_set_handler (SIM_CPU *current_cpu, UQI newval)
{
  int ilm = newval & 0x1f;
  int current_ilm = CPU (h_ilm);

  /* We can only set new ilm values < 16 if the current ilm is < 16.  Otherwise
     we add 16 to the value we are given.  */
  if (current_ilm >= 16 && ilm < 16)
    ilm += 16;

  CPU (h_ilm) = ilm;
}

/* Cover fns to access the ps register.  */

USI
fr30bf_h_ps_get_handler (SIM_CPU *current_cpu)
{
  int ccr = GET_H_CCR ();
  int scr = GET_H_SCR ();
  int ilm = GET_H_ILM ();

  return ccr | (scr << 8) | (ilm << 16);
}

void
fr30bf_h_ps_set_handler (SIM_CPU *current_cpu, USI newval)
{
  int ccr = newval & 0xff;
  int scr = (newval >> 8) & 7;
  int ilm = (newval >> 16) & 0x1f;

  SET_H_CCR (ccr);
  SET_H_SCR (scr);
  SET_H_ILM (ilm);
}

/* Cover fns to access the dedicated registers.  */

SI
fr30bf_h_dr_get_handler (SIM_CPU *current_cpu, UINT dr)
{
  switch (dr)
    {
    case H_DR_SSP :
      if (! GET_H_SBIT ())
	return GET_H_GR (H_GR_SP);
      else
	return CPU (h_dr[H_DR_SSP]);
    case H_DR_USP :
      if (GET_H_SBIT ())
	return GET_H_GR (H_GR_SP);
      else
	return CPU (h_dr[H_DR_USP]);
    case H_DR_TBR :
    case H_DR_RP :
    case H_DR_MDH :
    case H_DR_MDL :
      return CPU (h_dr[dr]);
    }
  return 0;
}

void
fr30bf_h_dr_set_handler (SIM_CPU *current_cpu, UINT dr, SI newval)
{
  switch (dr)
    {
    case H_DR_SSP :
      if (! GET_H_SBIT ())
	SET_H_GR (H_GR_SP, newval);
      else
	CPU (h_dr[H_DR_SSP]) = newval;
      break;
    case H_DR_USP :
      if (GET_H_SBIT ())
	SET_H_GR (H_GR_SP, newval);
      else
	CPU (h_dr[H_DR_USP]) = newval;
      break;
    case H_DR_TBR :
    case H_DR_RP :
    case H_DR_MDH :
    case H_DR_MDL :
      CPU (h_dr[dr]) = newval;
      break;
    }
}

#if WITH_PROFILE_MODEL_P

/* FIXME: Some of these should be inline or macros.  Later.  */

/* Initialize cycle counting for an insn.
   FIRST_P is non-zero if this is the first insn in a set of parallel
   insns.  */

void
fr30bf_model_insn_before (SIM_CPU *cpu, int first_p)
{
  MODEL_FR30_1_DATA *d = CPU_MODEL_DATA (cpu);
  d->load_regs_pending = 0;
}

/* Record the cycles computed for an insn.
   LAST_P is non-zero if this is the last insn in a set of parallel insns,
   and we update the total cycle count.
   CYCLES is the cycle count of the insn.  */

void
fr30bf_model_insn_after (SIM_CPU *cpu, int last_p, int cycles)
{
  PROFILE_DATA *p = CPU_PROFILE_DATA (cpu);
  MODEL_FR30_1_DATA *d = CPU_MODEL_DATA (cpu);

  PROFILE_MODEL_TOTAL_CYCLES (p) += cycles;
  PROFILE_MODEL_CUR_INSN_CYCLES (p) = cycles;
  d->load_regs = d->load_regs_pending;
}

static INLINE int
check_load_stall (SIM_CPU *cpu, int regno)
{
  const MODEL_FR30_1_DATA *d = CPU_MODEL_DATA (cpu);
  UINT load_regs = d->load_regs;

  if (regno != -1
      && (load_regs & (1 << regno)) != 0)
    {
      PROFILE_DATA *p = CPU_PROFILE_DATA (cpu);
      ++ PROFILE_MODEL_LOAD_STALL_CYCLES (p);
      if (TRACE_INSN_P (cpu))
	cgen_trace_printf (cpu, " ; Load stall.");
      return 1;
    }
  else
    return 0;
}

int
fr30bf_model_fr30_1_u_exec (SIM_CPU *cpu, const IDESC *idesc,
			    int unit_num, int referenced,
			    INT in_Ri, INT in_Rj, INT out_Ri)
{
  int cycles = idesc->timing->units[unit_num].done;
  cycles += check_load_stall (cpu, in_Ri);
  cycles += check_load_stall (cpu, in_Rj);
  return cycles;
}

int
fr30bf_model_fr30_1_u_cti (SIM_CPU *cpu, const IDESC *idesc,
			   int unit_num, int referenced,
			   INT in_Ri)
{
  PROFILE_DATA *p = CPU_PROFILE_DATA (cpu);
  /* (1 << 1): The pc is the 2nd element in inputs, outputs.
     ??? can be cleaned up */
  int taken_p = (referenced & (1 << 1)) != 0;
  int cycles = idesc->timing->units[unit_num].done;
  int delay_slot_p = CGEN_ATTR_VALUE (NULL, idesc->attrs, CGEN_INSN_DELAY_SLOT);

  cycles += check_load_stall (cpu, in_Ri);
  if (taken_p)
    {
      /* ??? Handling cti's without delay slots this way will run afoul of
	 accurate system simulation.  Later.  */
      if (! delay_slot_p)
	{
	  ++cycles;
	  ++PROFILE_MODEL_CTI_STALL_CYCLES (p);
	}
      ++PROFILE_MODEL_TAKEN_COUNT (p);
    }
  else
    ++PROFILE_MODEL_UNTAKEN_COUNT (p);

  return cycles;
}

int
fr30bf_model_fr30_1_u_load (SIM_CPU *cpu, const IDESC *idesc,
			    int unit_num, int referenced,
			    INT in_Rj, INT out_Ri)
{
  MODEL_FR30_1_DATA *d = CPU_MODEL_DATA (cpu);
  int cycles = idesc->timing->units[unit_num].done;
  d->load_regs_pending |= 1 << out_Ri;
  cycles += check_load_stall (cpu, in_Rj);
  return cycles;
}

int
fr30bf_model_fr30_1_u_store (SIM_CPU *cpu, const IDESC *idesc,
			     int unit_num, int referenced,
			     INT in_Ri, INT in_Rj)
{
  int cycles = idesc->timing->units[unit_num].done;
  cycles += check_load_stall (cpu, in_Ri);
  cycles += check_load_stall (cpu, in_Rj);
  return cycles;
}

int
fr30bf_model_fr30_1_u_ldm (SIM_CPU *cpu, const IDESC *idesc,
			   int unit_num, int referenced,
			   INT reglist)
{
  return idesc->timing->units[unit_num].done;
}

int
fr30bf_model_fr30_1_u_stm (SIM_CPU *cpu, const IDESC *idesc,
			   int unit_num, int referenced,
			   INT reglist)
{
  return idesc->timing->units[unit_num].done;
}

#endif /* WITH_PROFILE_MODEL_P */
