/* Simulator header for iq2000.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright 1996-2015 Free Software Foundation, Inc.

This file is part of the GNU simulators.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, see <http://www.gnu.org/licenses/>.

*/

#ifndef IQ2000_ARCH_H
#define IQ2000_ARCH_H

#define TARGET_BIG_ENDIAN 1

/* Enum declaration for model types.  */
typedef enum model_type {
  MODEL_IQ2000, MODEL_MAX
} MODEL_TYPE;

#define MAX_MODELS ((int) MODEL_MAX)

/* Enum declaration for unit types.  */
typedef enum unit_type {
  UNIT_NONE, UNIT_IQ2000_U_EXEC, UNIT_MAX
} UNIT_TYPE;

#define MAX_UNITS (1)

#endif /* IQ2000_ARCH_H */
