/* Simulator header for fr30.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.

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
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifndef FR30_ARCH_H
#define FR30_ARCH_H

#define TARGET_BIG_ENDIAN 1

/* Enum declaration for model types.  */
typedef enum model_type {
  MODEL_FR30_1, MODEL_MAX
} MODEL_TYPE;

#define MAX_MODELS ((int) MODEL_MAX)

/* Enum declaration for unit types.  */
typedef enum unit_type {
  UNIT_NONE, UNIT_FR30_1_U_STM, UNIT_FR30_1_U_LDM, UNIT_FR30_1_U_STORE
 , UNIT_FR30_1_U_LOAD, UNIT_FR30_1_U_CTI, UNIT_FR30_1_U_EXEC, UNIT_MAX
} UNIT_TYPE;

#define MAX_UNITS (3)

#endif /* FR30_ARCH_H */
