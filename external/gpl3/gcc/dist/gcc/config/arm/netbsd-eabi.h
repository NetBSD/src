/* Definitions of target machine for GNU compiler, NetBSD/arm ELF version.
   Copyright (C) 2002, 2003, 2004, 2005, 2007 Free Software Foundation, Inc.
   Contributed by Wasabi Systems, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

/* Run-time Target Specification.  */
#undef TARGET_VERSION
#define TARGET_VERSION fputs (" (NetBSD/earm ELF)", stderr);

/* This defaults us to little-endian.  */
#ifndef TARGET_ENDIAN_DEFAULT
#define TARGET_ENDIAN_DEFAULT 0
#endif

/* TARGET_BIG_ENDIAN_DEFAULT is set in
   config.gcc for big endian configurations.  */
#undef  TARGET_LINKER_EMULATION
#if TARGET_ENDIAN_DEFAULT == MASK_BIG
#define TARGET_LINKER_EMULATION "-m armelfb_nbsd_eabi"
#else
#define TARGET_LINKER_EMULATION "-m armelf_nbsd_eabi"
#endif

#undef MULTILIB_DEFAULTS

#undef ARM_DEFAULT_ABI
#define ARM_DEFAULT_ABI ARM_ABI_AAPCS_LINUX

#undef TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS()	\
  do					\
    {					\
      if (TARGET_AAPCS_BASED)		\
	TARGET_BPABI_CPP_BUILTINS();	\
      NETBSD_OS_CPP_BUILTINS_ELF();	\
    }					\
  while (0)

#undef SUBTARGET_CPP_SPEC
#define SUBTARGET_CPP_SPEC NETBSD_CPP_SPEC

/*
 * Override AAPCS types to remain compatible the existing NetBSD types.
 */
#undef WCHAR_TYPE
#define WCHAR_TYPE "int"

#undef SIZE_TYPE
#define SIZE_TYPE "long unsigned int"
 
#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "long int"

#undef SUBTARGET_EXTRA_ASM_SPEC
#define SUBTARGET_EXTRA_ASM_SPEC	\
  "-matpcs %{fpic|fpie:-k} %{fPIC|fPIE:-k}"

/* Default to full VFP if -mhard-float is specified.  */
#undef SUBTARGET_ASM_FLOAT_SPEC
#define SUBTARGET_ASM_FLOAT_SPEC	\
  "%{mhard-float:{!mfpu=*:-mfpu=vfp}}   \
   %{mfloat-abi=hard:{!mfpu=*:-mfpu=vfp}}"

#undef SUBTARGET_EXTRA_SPECS
#define SUBTARGET_EXTRA_SPECS				\
  { "subtarget_extra_asm_spec",	SUBTARGET_EXTRA_ASM_SPEC }, \
  { "subtarget_asm_float_spec", SUBTARGET_ASM_FLOAT_SPEC }, \
  { "netbsd_link_spec",		NETBSD_LINK_SPEC_ELF },	\
  { "be8_link_spec",		BE8_LINK_SPEC }, \
  { "target_fix_v4bx_spec",	TARGET_FIX_V4BX_SPEC }, \
  { "netbsd_entry_point",	NETBSD_ENTRY_POINT },

#define NETBSD_ENTRY_POINT "__start"

#undef LINK_SPEC
#define LINK_SPEC \
  "-X %{mbig-endian:-EB -m armelfb_nbsd_eabi} \
   %{mlittle-endian:-EL -m armelf_nbsd_eabi} \
   %(be8_link_spec) %(target_fix_v4bx_spec) \
   %(netbsd_link_spec)"
