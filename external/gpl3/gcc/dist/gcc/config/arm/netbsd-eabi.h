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

#undef MULTILIB_DEFAULTS
#define MULTILIB_DEFAULTS { "mabi=aapcs-linux" }

#undef MUST_USE_SJLJ_EXCEPTIONS
#define MUST_USE_SJLJ_EXCEPTIONS (!TARGET_AAPCS_BASED)

#undef ARM_EABI_UNWIND_TABLES
#define ARM_EABI_UNWIND_TABLES \
  ((!USING_SJLJ_EXCEPTIONS && flag_exceptions) || flag_unwind_tables)

#define TARGET_LINKER_EABI_SUFFIX \
    (TARGET_DEFAULT_FLOAT_ABI == ARM_FLOAT_ABI_SOFT \
     ? "%{!mabi=apcs-gnu:%{!mabi=atpcs:%{mfloat-abi=hard:_eabihf;:_eabi}}}" \
     : "%{!mabi=apcs-gnu:%{!mabi=atpcs:%{mfloat-abi=soft:_eabi;:_eabihf}}}")
#define TARGET_LINKER_BIG_EMULATION "armelfb_nbsd%(linker_eabi_suffix)"
#define TARGET_LINKER_LITTLE_EMULATION "armelf_nbsd%(linker_eabi_suffix)"

/* TARGET_BIG_ENDIAN_DEFAULT is set in
   config.gcc for big endian configurations.  */
#undef  TARGET_LINKER_EMULATION
#if TARGET_BIG_ENDIAN_DEFAULT
#define TARGET_LINKER_EMULATION TARGET_LINKER_BIG_EMULATION
#undef BE8_LINK_SPEC
#define BE8_LINK_SPEC " %{!mlittle-endian:%{march=armv7-a|mcpu=cortex-a5|mcpu=cortex-a8|mcpu=cortex-a9:%{!r:--be8}}}" 
#else
#define TARGET_LINKER_EMULATION TARGET_LINKER_LITTLE_EMULATION
#endif

#undef MULTILIB_DEFAULTS

#undef ARM_DEFAULT_ABI
#define ARM_DEFAULT_ABI ARM_ABI_AAPCS_LINUX

#undef TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS()		\
  do						\
    {						\
      if (TARGET_AAPCS_BASED)			\
	TARGET_BPABI_CPP_BUILTINS();		\
      NETBSD_OS_CPP_BUILTINS_ELF();		\
      if (ARM_EABI_UNWIND_TABLES)		\
	builtin_define ("__UNWIND_TABLES__");	\
    }						\
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
  "-matpcs %{mabi=apcs-gnu|mabi=atpcs:-meabi=gnu} %{fpic|fpie:-k} %{fPIC|fPIE:-k}"

/* Default to full VFP if -mhard-float is specified.  */
#undef SUBTARGET_ASM_FLOAT_SPEC
#define SUBTARGET_ASM_FLOAT_SPEC	\
  "%{mhard-float:%{!mfpu=*:-mfpu=vfp}}   \
   %{mfloat-abi=hard:%{!mfpu=*:-mfpu=vfp}}"

#undef SUBTARGET_EXTRA_SPECS
#define SUBTARGET_EXTRA_SPECS				\
  { "subtarget_extra_asm_spec",	SUBTARGET_EXTRA_ASM_SPEC }, \
  { "subtarget_asm_float_spec", SUBTARGET_ASM_FLOAT_SPEC }, \
  { "netbsd_link_spec",		NETBSD_LINK_SPEC_ELF },	\
  { "linker_eabi_suffix",	TARGET_LINKER_EABI_SUFFIX }, \
  { "linker_emulation",		TARGET_LINKER_EMULATION }, \
  { "linker_big_emulation",	TARGET_LINKER_BIG_EMULATION }, \
  { "linker_little_emulation",	TARGET_LINKER_LITTLE_EMULATION }, \
  { "be8_link_spec",		BE8_LINK_SPEC }, \
  { "target_fix_v4bx_spec",	TARGET_FIX_V4BX_SPEC }, \
  { "netbsd_entry_point",	NETBSD_ENTRY_POINT },

#define NETBSD_ENTRY_POINT "__start"

#undef LINK_SPEC
#define LINK_SPEC \
  "-X %{mbig-endian:-EB -m %(linker_big_emulation)} \
   %{mlittle-endian:-EL -m %(linker_liitle_emulation)} \
   %{!mbig-endian:%{!mlittle-endian:-m %(linker_emulation)}} \
   %(be8_link_spec) %(target_fix_v4bx_spec) \
   %(netbsd_link_spec)"
