/* trace.h --- interface to tracing output for the MSP430 simulator.

   Copyright (C) 2005-2014 Free Software Foundation, Inc.
   Contributed by Red Hat, Inc.

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

#ifndef SIM_MSP430_TRACE_H_
#define SIM_MSP430_TRACE_H_

extern void  msp430_trace_init (bfd *);
extern void  msp430_trace_one (int pc);
extern int   msp430_get_current_source_location (const char **, const char **, unsigned int *);

#endif
