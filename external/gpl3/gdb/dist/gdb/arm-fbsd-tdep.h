/* FreeBSD/arm target support, prototypes.

   Copyright (C) 2017-2020 Free Software Foundation, Inc.

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

#ifndef ARM_FBSD_TDEP_H
#define ARM_FBSD_TDEP_H

#include "regset.h"

/* The general-purpose regset consists of 13 R registers, plus SP, LR,
   PC, and CPSR registers.  */
#define ARM_FBSD_SIZEOF_GREGSET  (17 * 4)

/* The VFP regset consists of 32 D registers plus FPSCR, and the whole
   structure is padded to 64-bit alignment.  */
#define	ARM_FBSD_SIZEOF_VFPREGSET	(33 * 8)

extern const struct regset arm_fbsd_gregset;
extern const struct regset arm_fbsd_vfpregset;

/* Flags passed in AT_HWCAP. */
#define	HWCAP_VFP		0x00000040
#define	HWCAP_NEON		0x00001000
#define	HWCAP_VFPv3		0x00002000
#define	HWCAP_VFPD32		0x00080000

extern const struct target_desc *
arm_fbsd_read_description_auxv (struct target_ops *target);

#endif /* ARM_FBSD_TDEP_H */
