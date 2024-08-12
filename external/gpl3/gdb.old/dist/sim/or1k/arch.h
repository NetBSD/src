/* Simulator header for or1k.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996-2023 Free Software Foundation, Inc.

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
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.

*/

#ifndef OR1K_ARCH_H
#define OR1K_ARCH_H

#define TARGET_BIG_ENDIAN 1

#define WI  SI
#define UWI USI
#define AI  USI

#define IAI USI

/* Enum declaration for model types.  */
typedef enum model_type {
  MODEL_OR1200, MODEL_OR1200ND, MODEL_MAX
} MODEL_TYPE;

#define MAX_MODELS ((int) MODEL_MAX)

/* Enum declaration for unit types.  */
typedef enum unit_type {
  UNIT_NONE, UNIT_OR1200_U_EXEC, UNIT_OR1200ND_U_EXEC, UNIT_MAX
} UNIT_TYPE;

#define MAX_UNITS (1)

#endif /* OR1K_ARCH_H */
