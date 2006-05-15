/* collection of junk waiting time to sort out
   Copyright (C) 1998, 1999 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions.

This file is part of the GNU Simulators.

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

#ifndef IQ2000_SIM_H
#define IQ2000_SIM_H

#define GETTWI GETTSI
#define SETTWI SETTSI


/* Hardware/device support.
/* sim_core_attach device argument.  */
extern device iq2000_devices;

/* FIXME: Temporary, until device support ready.  */
struct _device { int foo; };

#endif /* IQ2000_SIM_H */
