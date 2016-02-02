/* Blackfin Event Vector Table (EVT) model.

   Copyright (C) 2010-2015 Free Software Foundation, Inc.
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

#ifndef DV_BFIN_EVT_H
#define DV_BFIN_EVT_H

extern void cec_set_evt (SIM_CPU *, int ivg, bu32 handler_addr);
extern bu32 cec_get_evt (SIM_CPU *, int ivg);
extern bu32 cec_get_reset_evt (SIM_CPU *);

#endif
