/* Simulator for TI MSP430 and MSP430x processors.

   Copyright (C) 2012-2014 Free Software Foundation, Inc.
   Contributed by Red Hat, Inc.

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

#ifndef _MSP430_SIM_H_
#define _MSP430_SIM_H_

struct msp430_cpu_state
{
  int regs[16];
  int cio_breakpoint;
  int cio_buffer;
};

#endif
