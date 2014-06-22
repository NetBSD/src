/* Blackfin device support.

   Copyright (C) 2010-2014 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

   This file is part of simulators.

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

#include "sim-main.h"
#include "sim-hw.h"
#include "hw-device.h"
#include "devices.h"
#include "dv-bfin_cec.h"
#include "dv-bfin_mmu.h"

static void
bfin_mmr_invalid (struct hw *me, SIM_CPU *cpu, address_word addr,
		  unsigned nr_bytes, bool write)
{
  if (!cpu)
    cpu = hw_system_cpu (me);

  /* Only throw a fit if the cpu is doing the access.  DMA/GDB simply
     go unnoticed.  Not exactly hardware behavior, but close enough.  */
  if (!cpu)
    {
      sim_io_eprintf (hw_system (me), "%s: invalid MMR access @ %#x\n",
		      hw_path (me), addr);
      return;
    }

  HW_TRACE ((me, "invalid MMR %s to 0x%08lx length %u",
	     write ? "write" : "read", (unsigned long) addr, nr_bytes));

  /* XXX: is this what hardware does ?  */
  if (addr >= BFIN_CORE_MMR_BASE)
    /* XXX: This should be setting up CPLB fault addrs ?  */
    mmu_process_fault (cpu, addr, write, false, false, true);
  else
    /* XXX: Newer parts set up an interrupt from EBIU and program
            EBIU_ERRADDR with the address.  */
    cec_hwerr (cpu, HWERR_SYSTEM_MMR);
}

void
dv_bfin_mmr_invalid (struct hw *me, address_word addr, unsigned nr_bytes,
		     bool write)
{
  bfin_mmr_invalid (me, NULL, addr, nr_bytes, write);
}

void
dv_bfin_mmr_require (struct hw *me, address_word addr, unsigned nr_bytes,
		     unsigned size, bool write)
{
  if (nr_bytes != size)
    dv_bfin_mmr_invalid (me, addr, nr_bytes, write);
}

static bool
bfin_mmr_check (struct hw *me, SIM_CPU *cpu, address_word addr,
		unsigned nr_bytes, bool write)
{
  if (addr >= BFIN_CORE_MMR_BASE)
    {
      /* All Core MMRs are aligned 32bits.  */
      if ((addr & 3) == 0 && nr_bytes == 4)
	return true;
    }
  else if (addr >= BFIN_SYSTEM_MMR_BASE)
    {
      /* All System MMRs are 32bit aligned, but can be 16bits or 32bits.  */
      if ((addr & 0x3) == 0 && (nr_bytes == 2 || nr_bytes == 4))
	return true;
    }
  else
    return true;

  /* Still here ?  Must be crap.  */
  bfin_mmr_invalid (me, cpu, addr, nr_bytes, write);

  return false;
}

bool
dv_bfin_mmr_check (struct hw *me, address_word addr, unsigned nr_bytes,
		   bool write)
{
  return bfin_mmr_check (me, NULL, addr, nr_bytes, write);
}

int
device_io_read_buffer (device *me, void *source, int space,
		       address_word addr, unsigned nr_bytes,
		       SIM_DESC sd, SIM_CPU *cpu, sim_cia cia)
{
  struct hw *dv_me = (struct hw *) me;

  if (STATE_ENVIRONMENT (sd) != OPERATING_ENVIRONMENT)
    return nr_bytes;

  if (bfin_mmr_check (dv_me, cpu, addr, nr_bytes, false))
    if (cpu)
      {
	sim_cpu_hw_io_read_buffer (cpu, cia, dv_me, source, space,
				   addr, nr_bytes);
	return nr_bytes;
      }
    else
      return sim_hw_io_read_buffer (sd, dv_me, source, space, addr, nr_bytes);
  else
    return 0;
}

int
device_io_write_buffer (device *me, const void *source, int space,
			address_word addr, unsigned nr_bytes,
                        SIM_DESC sd, SIM_CPU *cpu, sim_cia cia)
{
  struct hw *dv_me = (struct hw *) me;

  if (STATE_ENVIRONMENT (sd) != OPERATING_ENVIRONMENT)
    return nr_bytes;

  if (bfin_mmr_check (dv_me, cpu, addr, nr_bytes, true))
    if (cpu)
      {
	sim_cpu_hw_io_write_buffer (cpu, cia, dv_me, source, space,
				    addr, nr_bytes);
	return nr_bytes;
      }
    else
      return sim_hw_io_write_buffer (sd, dv_me, source, space, addr, nr_bytes);
  else
    return 0;
}

void device_error (device *me, const char *message, ...)
{
  /* Don't bother doing anything here -- any place in common code that
     calls device_error() follows it with sim_hw_abort().  Since the
     device isn't bound to the system yet, we can't call any common
     hardware error funcs on it or we'll hit a NULL pointer.  */
}

unsigned int dv_get_bus_num (struct hw *me)
{
  const hw_unit *unit = hw_unit_address (me);
  return unit->cells[unit->nr_cells - 1];
}
