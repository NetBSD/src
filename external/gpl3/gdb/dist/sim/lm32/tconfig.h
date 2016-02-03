/* Lattice Mico32 simulator configuration.
   Contributed by Jon Beniston <jon@beniston.com>

   This file is part of GDB.

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

#ifndef LM32_TCONFIG_H
#define LM32_TCONFIG_H

/* See sim-hload.c.  We properly handle LMA.  */
#define SIM_HANDLES_LMA 1

#define WITH_SCACHE_PBB 1

#endif /* LM32_TCONFIG_H */
