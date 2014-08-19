/* Target-dependent code for GNU/Linux AArch64.

   Copyright (C) 2009-2014 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

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

#include "defs.h"

#include "gdbarch.h"
#include "glibc-tdep.h"
#include "linux-tdep.h"
#include "aarch64-tdep.h"
#include "aarch64-linux-tdep.h"
#include "osabi.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "tramp-frame.h"
#include "trad-frame.h"

#include "inferior.h"
#include "regcache.h"
#include "regset.h"

#include "cli/cli-utils.h"
#include "stap-probe.h"
#include "parser-defs.h"
#include "user-regs.h"
#include <ctype.h>

/* The general-purpose regset consists of 31 X registers, plus SP, PC,
   and PSTATE registers, as defined in the AArch64 port of the Linux
   kernel.  */
#define AARCH64_LINUX_SIZEOF_GREGSET  (34 * X_REGISTER_SIZE)

/* The fp regset consists of 32 V registers, plus FPCR and FPSR which
   are 4 bytes wide each, and the whole structure is padded to 128 bit
   alignment.  */
#define AARCH64_LINUX_SIZEOF_FPREGSET (33 * V_REGISTER_SIZE)

/* Signal frame handling.

      +----------+  ^
      | saved lr |  |
   +->| saved fp |--+
   |  |          |
   |  |          |
   |  +----------+
   |  | saved lr |
   +--| saved fp |
   ^  |          |
   |  |          |
   |  +----------+
   ^  |          |
   |  | signal   |
   |  |          |
   |  | saved lr |-->interrupted_function_pc
   +--| saved fp |
   |  +----------+
   |  | saved lr |--> default_restorer (movz x8, NR_sys_rt_sigreturn; svc 0)
   +--| saved fp |<- FP
      |          |
      |          |<- SP
      +----------+

  On signal delivery, the kernel will create a signal handler stack
  frame and setup the return address in LR to point at restorer stub.
  The signal stack frame is defined by:

  struct rt_sigframe
  {
    siginfo_t info;
    struct ucontext uc;
  };

  typedef struct
  {
    ...                                    128 bytes
  } siginfo_t;

  The ucontext has the following form:
  struct ucontext
  {
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    sigset_t uc_sigmask;
    struct sigcontext uc_mcontext;
  };

  typedef struct sigaltstack
  {
    void *ss_sp;
    int ss_flags;
    size_t ss_size;
  } stack_t;

  struct sigcontext
  {
    unsigned long fault_address;
    unsigned long regs[31];
    unsigned long sp;		/ * 31 * /
    unsigned long pc;		/ * 32 * /
    unsigned long pstate;	/ * 33 * /
    __u8 __reserved[4096]
  };

  The restorer stub will always have the form:

  d28015a8        movz    x8, #0xad
  d4000001        svc     #0x0

  We detect signal frames by snooping the return code for the restorer
  instruction sequence.

  The handler then needs to recover the saved register set from
  ucontext.uc_mcontext.  */

/* These magic numbers need to reflect the layout of the kernel
   defined struct rt_sigframe and ucontext.  */
#define AARCH64_SIGCONTEXT_REG_SIZE             8
#define AARCH64_RT_SIGFRAME_UCONTEXT_OFFSET     128
#define AARCH64_UCONTEXT_SIGCONTEXT_OFFSET      176
#define AARCH64_SIGCONTEXT_XO_OFFSET            8

/* Implement the "init" method of struct tramp_frame.  */

static void
aarch64_linux_sigframe_init (const struct tramp_frame *self,
			     struct frame_info *this_frame,
			     struct trad_frame_cache *this_cache,
			     CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, AARCH64_SP_REGNUM);
  CORE_ADDR fp = get_frame_register_unsigned (this_frame, AARCH64_FP_REGNUM);
  CORE_ADDR sigcontext_addr =
    sp
    + AARCH64_RT_SIGFRAME_UCONTEXT_OFFSET
    + AARCH64_UCONTEXT_SIGCONTEXT_OFFSET;
  int i;

  for (i = 0; i < 31; i++)
    {
      trad_frame_set_reg_addr (this_cache,
			       AARCH64_X0_REGNUM + i,
			       sigcontext_addr + AARCH64_SIGCONTEXT_XO_OFFSET
			       + i * AARCH64_SIGCONTEXT_REG_SIZE);
    }

  trad_frame_set_reg_addr (this_cache, AARCH64_FP_REGNUM, fp);
  trad_frame_set_reg_addr (this_cache, AARCH64_LR_REGNUM, fp + 8);
  trad_frame_set_reg_addr (this_cache, AARCH64_PC_REGNUM, fp + 8);

  trad_frame_set_id (this_cache, frame_id_build (fp, func));
}

static const struct tramp_frame aarch64_linux_rt_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    /* movz x8, 0x8b (S=1,o=10,h=0,i=0x8b,r=8)
       Soo1 0010 1hhi iiii iiii iiii iiir rrrr  */
    {0xd2801168, -1},

    /* svc  0x0      (o=0, l=1)
       1101 0100 oooi iiii iiii iiii iii0 00ll  */
    {0xd4000001, -1},
    {TRAMP_SENTINEL_INSN, -1}
  },
  aarch64_linux_sigframe_init
};

/* Fill GDB's register array with the general-purpose register values
   in the buffer pointed by GREGS_BUF.  */

void
aarch64_linux_supply_gregset (struct regcache *regcache,
			      const gdb_byte *gregs_buf)
{
  int regno;

  for (regno = AARCH64_X0_REGNUM; regno <= AARCH64_CPSR_REGNUM; regno++)
    regcache_raw_supply (regcache, regno,
			 gregs_buf + X_REGISTER_SIZE
			 * (regno - AARCH64_X0_REGNUM));
}

/* The "supply_regset" function for the general-purpose register set.  */

static void
supply_gregset_from_core (const struct regset *regset,
			  struct regcache *regcache,
			  int regnum, const void *regbuf, size_t len)
{
  aarch64_linux_supply_gregset (regcache, (const gdb_byte *) regbuf);
}

/* Fill GDB's register array with the floating-point register values
   in the buffer pointed by FPREGS_BUF.  */

void
aarch64_linux_supply_fpregset (struct regcache *regcache,
			       const gdb_byte *fpregs_buf)
{
  int regno;

  for (regno = AARCH64_V0_REGNUM; regno <= AARCH64_V31_REGNUM; regno++)
    regcache_raw_supply (regcache, regno,
			 fpregs_buf + V_REGISTER_SIZE
			 * (regno - AARCH64_V0_REGNUM));

  regcache_raw_supply (regcache, AARCH64_FPSR_REGNUM,
		       fpregs_buf + V_REGISTER_SIZE * 32);
  regcache_raw_supply (regcache, AARCH64_FPCR_REGNUM,
		       fpregs_buf + V_REGISTER_SIZE * 32 + 4);
}

/* The "supply_regset" function for the floating-point register set.  */

static void
supply_fpregset_from_core (const struct regset *regset,
			   struct regcache *regcache,
			   int regnum, const void *regbuf, size_t len)
{
  aarch64_linux_supply_fpregset (regcache, (const gdb_byte *) regbuf);
}

/* Implement the "regset_from_core_section" gdbarch method.  */

static const struct regset *
aarch64_linux_regset_from_core_section (struct gdbarch *gdbarch,
					const char *sect_name,
					size_t sect_size)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (strcmp (sect_name, ".reg") == 0
      && sect_size == AARCH64_LINUX_SIZEOF_GREGSET)
    {
      if (tdep->gregset == NULL)
	tdep->gregset = regset_alloc (gdbarch, supply_gregset_from_core,
				      NULL);
      return tdep->gregset;
    }

  if (strcmp (sect_name, ".reg2") == 0
      && sect_size == AARCH64_LINUX_SIZEOF_FPREGSET)
    {
      if (tdep->fpregset == NULL)
	tdep->fpregset = regset_alloc (gdbarch, supply_fpregset_from_core,
				       NULL);
      return tdep->fpregset;
    }
  return NULL;
}

/* Implementation of `gdbarch_stap_is_single_operand', as defined in
   gdbarch.h.  */

static int
aarch64_stap_is_single_operand (struct gdbarch *gdbarch, const char *s)
{
  return (*s == '#' || isdigit (*s) /* Literal number.  */
	  || *s == '[' /* Register indirection.  */
	  || isalpha (*s)); /* Register value.  */
}

/* This routine is used to parse a special token in AArch64's assembly.

   The special tokens parsed by it are:

      - Register displacement (e.g, [fp, #-8])

   It returns one if the special token has been parsed successfully,
   or zero if the current token is not considered special.  */

static int
aarch64_stap_parse_special_token (struct gdbarch *gdbarch,
				  struct stap_parse_info *p)
{
  if (*p->arg == '[')
    {
      /* Temporary holder for lookahead.  */
      const char *tmp = p->arg;
      char *endp;
      /* Used to save the register name.  */
      const char *start;
      char *regname;
      int len;
      int got_minus = 0;
      long displacement;
      struct stoken str;

      ++tmp;
      start = tmp;

      /* Register name.  */
      while (isalnum (*tmp))
	++tmp;

      if (*tmp != ',')
	return 0;

      len = tmp - start;
      regname = alloca (len + 2);

      strncpy (regname, start, len);
      regname[len] = '\0';

      if (user_reg_map_name_to_regnum (gdbarch, regname, len) == -1)
	error (_("Invalid register name `%s' on expression `%s'."),
	       regname, p->saved_arg);

      ++tmp;
      tmp = skip_spaces_const (tmp);
      /* Now we expect a number.  It can begin with '#' or simply
	 a digit.  */
      if (*tmp == '#')
	++tmp;

      if (*tmp == '-')
	{
	  ++tmp;
	  got_minus = 1;
	}
      else if (*tmp == '+')
	++tmp;

      if (!isdigit (*tmp))
	return 0;

      displacement = strtol (tmp, &endp, 10);
      tmp = endp;

      /* Skipping last `]'.  */
      if (*tmp++ != ']')
	return 0;

      /* The displacement.  */
      write_exp_elt_opcode (OP_LONG);
      write_exp_elt_type (builtin_type (gdbarch)->builtin_long);
      write_exp_elt_longcst (displacement);
      write_exp_elt_opcode (OP_LONG);
      if (got_minus)
	write_exp_elt_opcode (UNOP_NEG);

      /* The register name.  */
      write_exp_elt_opcode (OP_REGISTER);
      str.ptr = regname;
      str.length = len;
      write_exp_string (str);
      write_exp_elt_opcode (OP_REGISTER);

      write_exp_elt_opcode (BINOP_ADD);

      /* Casting to the expected type.  */
      write_exp_elt_opcode (UNOP_CAST);
      write_exp_elt_type (lookup_pointer_type (p->arg_type));
      write_exp_elt_opcode (UNOP_CAST);

      write_exp_elt_opcode (UNOP_IND);

      p->arg = tmp;
    }
  else
    return 0;

  return 1;
}

static void
aarch64_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  static const char *const stap_integer_prefixes[] = { "#", "", NULL };
  static const char *const stap_register_prefixes[] = { "", NULL };
  static const char *const stap_register_indirection_prefixes[] = { "[",
								    NULL };
  static const char *const stap_register_indirection_suffixes[] = { "]",
								    NULL };
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  tdep->lowest_pc = 0x8000;

  linux_init_abi (info, gdbarch);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 svr4_lp64_fetch_link_map_offsets);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
                                             svr4_fetch_objfile_link_map);

  /* Shared library handling.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  set_gdbarch_get_siginfo_type (gdbarch, linux_get_siginfo_type);
  tramp_frame_prepend_unwinder (gdbarch, &aarch64_linux_rt_sigframe);

  /* Enable longjmp.  */
  tdep->jb_pc = 11;

  set_gdbarch_regset_from_core_section (gdbarch,
					aarch64_linux_regset_from_core_section);

  /* SystemTap related.  */
  set_gdbarch_stap_integer_prefixes (gdbarch, stap_integer_prefixes);
  set_gdbarch_stap_register_prefixes (gdbarch, stap_register_prefixes);
  set_gdbarch_stap_register_indirection_prefixes (gdbarch,
					    stap_register_indirection_prefixes);
  set_gdbarch_stap_register_indirection_suffixes (gdbarch,
					    stap_register_indirection_suffixes);
  set_gdbarch_stap_is_single_operand (gdbarch, aarch64_stap_is_single_operand);
  set_gdbarch_stap_parse_special_token (gdbarch,
					aarch64_stap_parse_special_token);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_aarch64_linux_tdep;

void
_initialize_aarch64_linux_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_aarch64, 0, GDB_OSABI_LINUX,
			  aarch64_linux_init_abi);
}
