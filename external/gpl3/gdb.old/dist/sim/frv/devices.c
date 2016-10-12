/* frv device support
   Copyright (C) 1998-2015 Free Software Foundation, Inc.
   Contributed by Red Hat.

This file is part of the GNU simulators.

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

/* ??? All of this is just to get something going.  wip!  */

#include "sim-main.h"

device frv_devices;

int
device_io_read_buffer (device *me, void *source, int space,
		       address_word addr, unsigned nr_bytes,
		       SIM_DESC sd, SIM_CPU *cpu, sim_cia cia)
{
  if (STATE_ENVIRONMENT (sd) != OPERATING_ENVIRONMENT)
    return nr_bytes;

  return nr_bytes;
}

int
device_io_write_buffer (device *me, const void *source, int space,
			address_word addr, unsigned nr_bytes,
                        SIM_DESC sd, SIM_CPU *cpu, sim_cia cia)
{

#if WITH_SCACHE
  if (addr == MCCR_ADDR)
    {
      if ((*(const char *) source & MCCR_CP) != 0)
	scache_flush (sd);
      return nr_bytes;
    }
#endif

  if (STATE_ENVIRONMENT (sd) != OPERATING_ENVIRONMENT)
    return nr_bytes;

  return nr_bytes;
}

void device_error (device *me, const char *message, ...) {}
