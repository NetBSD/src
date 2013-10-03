/* PPC GNU/Linux native support.

   Copyright (C) 1988-2013 Free Software Foundation, Inc.

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
#include "gdb_string.h"
#include "observer.h"
#include "frame.h"
#include "inferior.h"
#include "gdbthread.h"
#include "gdbcore.h"
#include "regcache.h"
#include "gdb_assert.h"
#include "target.h"
#include "linux-nat.h"

#include <stdint.h>
#include <sys/types.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include "gdb_wait.h"
#include <fcntl.h>
#include <sys/procfs.h>
#include <sys/ptrace.h>

/* Prototypes for supply_gregset etc.  */
#include "gregset.h"
#include "ppc-tdep.h"
#include "ppc-linux-tdep.h"

/* Required when using the AUXV.  */
#include "elf/common.h"
#include "auxv.h"

/* This sometimes isn't defined.  */
#ifndef PT_ORIG_R3
#define PT_ORIG_R3 34
#endif
#ifndef PT_TRAP
#define PT_TRAP 40
#endif

/* The PPC_FEATURE_* defines should be provided by <asm/cputable.h>.
   If they aren't, we can provide them ourselves (their values are fixed
   because they are part of the kernel ABI).  They are used in the AT_HWCAP
   entry of the AUXV.  */
#ifndef PPC_FEATURE_CELL
#define PPC_FEATURE_CELL 0x00010000
#endif
#ifndef PPC_FEATURE_BOOKE
#define PPC_FEATURE_BOOKE 0x00008000
#endif
#ifndef PPC_FEATURE_HAS_DFP
#define PPC_FEATURE_HAS_DFP	0x00000400  /* Decimal Floating Point.  */
#endif

/* Glibc's headers don't define PTRACE_GETVRREGS so we cannot use a
   configure time check.  Some older glibc's (for instance 2.2.1)
   don't have a specific powerpc version of ptrace.h, and fall back on
   a generic one.  In such cases, sys/ptrace.h defines
   PTRACE_GETFPXREGS and PTRACE_SETFPXREGS to the same numbers that
   ppc kernel's asm/ptrace.h defines PTRACE_GETVRREGS and
   PTRACE_SETVRREGS to be.  This also makes a configury check pretty
   much useless.  */

/* These definitions should really come from the glibc header files,
   but Glibc doesn't know about the vrregs yet.  */
#ifndef PTRACE_GETVRREGS
#define PTRACE_GETVRREGS 18
#define PTRACE_SETVRREGS 19
#endif

/* PTRACE requests for POWER7 VSX registers.  */
#ifndef PTRACE_GETVSXREGS
#define PTRACE_GETVSXREGS 27
#define PTRACE_SETVSXREGS 28
#endif

/* Similarly for the ptrace requests for getting / setting the SPE
   registers (ev0 -- ev31, acc, and spefscr).  See the description of
   gdb_evrregset_t for details.  */
#ifndef PTRACE_GETEVRREGS
#define PTRACE_GETEVRREGS 20
#define PTRACE_SETEVRREGS 21
#endif

/* Similarly for the hardware watchpoint support.  These requests are used
   when the BookE kernel interface is not available.  */
#ifndef PTRACE_GET_DEBUGREG
#define PTRACE_GET_DEBUGREG    25
#endif
#ifndef PTRACE_SET_DEBUGREG
#define PTRACE_SET_DEBUGREG    26
#endif
#ifndef PTRACE_GETSIGINFO
#define PTRACE_GETSIGINFO    0x4202
#endif

/* These requests are used when the BookE kernel interface is available.
   It exposes the additional debug features of BookE processors, such as
   ranged breakpoints and watchpoints and hardware-accelerated condition
   evaluation.  */
#ifndef PPC_PTRACE_GETHWDBGINFO

/* Not having PPC_PTRACE_GETHWDBGINFO defined means that the new BookE
   interface is not present in ptrace.h, so we'll have to pretty much include
   it all here so that the code at least compiles on older systems.  */
#define PPC_PTRACE_GETHWDBGINFO 0x89
#define PPC_PTRACE_SETHWDEBUG   0x88
#define PPC_PTRACE_DELHWDEBUG   0x87

struct ppc_debug_info
{
        uint32_t version;               /* Only version 1 exists to date.  */
        uint32_t num_instruction_bps;
        uint32_t num_data_bps;
        uint32_t num_condition_regs;
        uint32_t data_bp_alignment;
        uint32_t sizeof_condition;      /* size of the DVC register.  */
        uint64_t features;
};

/* Features will have bits indicating whether there is support for:  */
#define PPC_DEBUG_FEATURE_INSN_BP_RANGE         0x1
#define PPC_DEBUG_FEATURE_INSN_BP_MASK          0x2
#define PPC_DEBUG_FEATURE_DATA_BP_RANGE         0x4
#define PPC_DEBUG_FEATURE_DATA_BP_MASK          0x8

struct ppc_hw_breakpoint
{
        uint32_t version;               /* currently, version must be 1 */
        uint32_t trigger_type;          /* only some combinations allowed */
        uint32_t addr_mode;             /* address match mode */
        uint32_t condition_mode;        /* break/watchpoint condition flags */
        uint64_t addr;                  /* break/watchpoint address */
        uint64_t addr2;                 /* range end or mask */
        uint64_t condition_value;       /* contents of the DVC register */
};

/* Trigger type.  */
#define PPC_BREAKPOINT_TRIGGER_EXECUTE  0x1
#define PPC_BREAKPOINT_TRIGGER_READ     0x2
#define PPC_BREAKPOINT_TRIGGER_WRITE    0x4
#define PPC_BREAKPOINT_TRIGGER_RW       0x6

/* Address mode.  */
#define PPC_BREAKPOINT_MODE_EXACT               0x0
#define PPC_BREAKPOINT_MODE_RANGE_INCLUSIVE     0x1
#define PPC_BREAKPOINT_MODE_RANGE_EXCLUSIVE     0x2
#define PPC_BREAKPOINT_MODE_MASK                0x3

/* Condition mode.  */
#define PPC_BREAKPOINT_CONDITION_NONE   0x0
#define PPC_BREAKPOINT_CONDITION_AND    0x1
#define PPC_BREAKPOINT_CONDITION_EXACT  0x1
#define PPC_BREAKPOINT_CONDITION_OR     0x2
#define PPC_BREAKPOINT_CONDITION_AND_OR 0x3
#define PPC_BREAKPOINT_CONDITION_BE_ALL 0x00ff0000
#define PPC_BREAKPOINT_CONDITION_BE_SHIFT       16
#define PPC_BREAKPOINT_CONDITION_BE(n)  \
        (1<<((n)+PPC_BREAKPOINT_CONDITION_BE_SHIFT))
#endif /* PPC_PTRACE_GETHWDBGINFO */



/* Similarly for the general-purpose (gp0 -- gp31)
   and floating-point registers (fp0 -- fp31).  */
#ifndef PTRACE_GETREGS
#define PTRACE_GETREGS 12
#endif
#ifndef PTRACE_SETREGS
#define PTRACE_SETREGS 13
#endif
#ifndef PTRACE_GETFPREGS
#define PTRACE_GETFPREGS 14
#endif
#ifndef PTRACE_SETFPREGS
#define PTRACE_SETFPREGS 15
#endif

/* This oddity is because the Linux kernel defines elf_vrregset_t as
   an array of 33 16 bytes long elements.  I.e. it leaves out vrsave.
   However the PTRACE_GETVRREGS and PTRACE_SETVRREGS requests return
   the vrsave as an extra 4 bytes at the end.  I opted for creating a
   flat array of chars, so that it is easier to manipulate for gdb.

   There are 32 vector registers 16 bytes longs, plus a VSCR register
   which is only 4 bytes long, but is fetched as a 16 bytes
   quantity.  Up to here we have the elf_vrregset_t structure.
   Appended to this there is space for the VRSAVE register: 4 bytes.
   Even though this vrsave register is not included in the regset
   typedef, it is handled by the ptrace requests.

   Note that GNU/Linux doesn't support little endian PPC hardware,
   therefore the offset at which the real value of the VSCR register
   is located will be always 12 bytes.

   The layout is like this (where x is the actual value of the vscr reg): */

/* *INDENT-OFF* */
/*
   |.|.|.|.|.....|.|.|.|.||.|.|.|x||.|
   <------->     <-------><-------><->
     VR0           VR31     VSCR    VRSAVE
*/
/* *INDENT-ON* */

#define SIZEOF_VRREGS 33*16+4

typedef char gdb_vrregset_t[SIZEOF_VRREGS];

/* This is the layout of the POWER7 VSX registers and the way they overlap
   with the existing FPR and VMX registers.

                    VSR doubleword 0               VSR doubleword 1
           ----------------------------------------------------------------
   VSR[0]  |             FPR[0]            |                              |
           ----------------------------------------------------------------
   VSR[1]  |             FPR[1]            |                              |
           ----------------------------------------------------------------
           |              ...              |                              |
           |              ...              |                              |
           ----------------------------------------------------------------
   VSR[30] |             FPR[30]           |                              |
           ----------------------------------------------------------------
   VSR[31] |             FPR[31]           |                              |
           ----------------------------------------------------------------
   VSR[32] |                             VR[0]                            |
           ----------------------------------------------------------------
   VSR[33] |                             VR[1]                            |
           ----------------------------------------------------------------
           |                              ...                             |
           |                              ...                             |
           ----------------------------------------------------------------
   VSR[62] |                             VR[30]                           |
           ----------------------------------------------------------------
   VSR[63] |                             VR[31]                           |
          ----------------------------------------------------------------

   VSX has 64 128bit registers.  The first 32 registers overlap with
   the FP registers (doubleword 0) and hence extend them with additional
   64 bits (doubleword 1).  The other 32 regs overlap with the VMX
   registers.  */
#define SIZEOF_VSXREGS 32*8

typedef char gdb_vsxregset_t[SIZEOF_VSXREGS];

/* On PPC processors that support the Signal Processing Extension
   (SPE) APU, the general-purpose registers are 64 bits long.
   However, the ordinary Linux kernel PTRACE_PEEKUSER / PTRACE_POKEUSER
   ptrace calls only access the lower half of each register, to allow
   them to behave the same way they do on non-SPE systems.  There's a
   separate pair of calls, PTRACE_GETEVRREGS / PTRACE_SETEVRREGS, that
   read and write the top halves of all the general-purpose registers
   at once, along with some SPE-specific registers.

   GDB itself continues to claim the general-purpose registers are 32
   bits long.  It has unnamed raw registers that hold the upper halves
   of the gprs, and the full 64-bit SIMD views of the registers,
   'ev0' -- 'ev31', are pseudo-registers that splice the top and
   bottom halves together.

   This is the structure filled in by PTRACE_GETEVRREGS and written to
   the inferior's registers by PTRACE_SETEVRREGS.  */
struct gdb_evrregset_t
{
  unsigned long evr[32];
  unsigned long long acc;
  unsigned long spefscr;
};

/* Non-zero if our kernel may support the PTRACE_GETVSXREGS and
   PTRACE_SETVSXREGS requests, for reading and writing the VSX
   POWER7 registers 0 through 31.  Zero if we've tried one of them and
   gotten an error.  Note that VSX registers 32 through 63 overlap
   with VR registers 0 through 31.  */
int have_ptrace_getsetvsxregs = 1;

/* Non-zero if our kernel may support the PTRACE_GETVRREGS and
   PTRACE_SETVRREGS requests, for reading and writing the Altivec
   registers.  Zero if we've tried one of them and gotten an
   error.  */
int have_ptrace_getvrregs = 1;

/* Non-zero if our kernel may support the PTRACE_GETEVRREGS and
   PTRACE_SETEVRREGS requests, for reading and writing the SPE
   registers.  Zero if we've tried one of them and gotten an
   error.  */
int have_ptrace_getsetevrregs = 1;

/* Non-zero if our kernel may support the PTRACE_GETREGS and
   PTRACE_SETREGS requests, for reading and writing the
   general-purpose registers.  Zero if we've tried one of
   them and gotten an error.  */
int have_ptrace_getsetregs = 1;

/* Non-zero if our kernel may support the PTRACE_GETFPREGS and
   PTRACE_SETFPREGS requests, for reading and writing the
   floating-pointers registers.  Zero if we've tried one of
   them and gotten an error.  */
int have_ptrace_getsetfpregs = 1;

/* *INDENT-OFF* */
/* registers layout, as presented by the ptrace interface:
PT_R0, PT_R1, PT_R2, PT_R3, PT_R4, PT_R5, PT_R6, PT_R7,
PT_R8, PT_R9, PT_R10, PT_R11, PT_R12, PT_R13, PT_R14, PT_R15,
PT_R16, PT_R17, PT_R18, PT_R19, PT_R20, PT_R21, PT_R22, PT_R23,
PT_R24, PT_R25, PT_R26, PT_R27, PT_R28, PT_R29, PT_R30, PT_R31,
PT_FPR0, PT_FPR0 + 2, PT_FPR0 + 4, PT_FPR0 + 6,
PT_FPR0 + 8, PT_FPR0 + 10, PT_FPR0 + 12, PT_FPR0 + 14,
PT_FPR0 + 16, PT_FPR0 + 18, PT_FPR0 + 20, PT_FPR0 + 22,
PT_FPR0 + 24, PT_FPR0 + 26, PT_FPR0 + 28, PT_FPR0 + 30,
PT_FPR0 + 32, PT_FPR0 + 34, PT_FPR0 + 36, PT_FPR0 + 38,
PT_FPR0 + 40, PT_FPR0 + 42, PT_FPR0 + 44, PT_FPR0 + 46,
PT_FPR0 + 48, PT_FPR0 + 50, PT_FPR0 + 52, PT_FPR0 + 54,
PT_FPR0 + 56, PT_FPR0 + 58, PT_FPR0 + 60, PT_FPR0 + 62,
PT_NIP, PT_MSR, PT_CCR, PT_LNK, PT_CTR, PT_XER, PT_MQ */
/* *INDENT_ON * */

static int
ppc_register_u_addr (struct gdbarch *gdbarch, int regno)
{
  int u_addr = -1;
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  /* NOTE: cagney/2003-11-25: This is the word size used by the ptrace
     interface, and not the wordsize of the program's ABI.  */
  int wordsize = sizeof (long);

  /* General purpose registers occupy 1 slot each in the buffer.  */
  if (regno >= tdep->ppc_gp0_regnum 
      && regno < tdep->ppc_gp0_regnum + ppc_num_gprs)
    u_addr = ((regno - tdep->ppc_gp0_regnum + PT_R0) * wordsize);

  /* Floating point regs: eight bytes each in both 32- and 64-bit
     ptrace interfaces.  Thus, two slots each in 32-bit interface, one
     slot each in 64-bit interface.  */
  if (tdep->ppc_fp0_regnum >= 0
      && regno >= tdep->ppc_fp0_regnum
      && regno < tdep->ppc_fp0_regnum + ppc_num_fprs)
    u_addr = (PT_FPR0 * wordsize) + ((regno - tdep->ppc_fp0_regnum) * 8);

  /* UISA special purpose registers: 1 slot each.  */
  if (regno == gdbarch_pc_regnum (gdbarch))
    u_addr = PT_NIP * wordsize;
  if (regno == tdep->ppc_lr_regnum)
    u_addr = PT_LNK * wordsize;
  if (regno == tdep->ppc_cr_regnum)
    u_addr = PT_CCR * wordsize;
  if (regno == tdep->ppc_xer_regnum)
    u_addr = PT_XER * wordsize;
  if (regno == tdep->ppc_ctr_regnum)
    u_addr = PT_CTR * wordsize;
#ifdef PT_MQ
  if (regno == tdep->ppc_mq_regnum)
    u_addr = PT_MQ * wordsize;
#endif
  if (regno == tdep->ppc_ps_regnum)
    u_addr = PT_MSR * wordsize;
  if (regno == PPC_ORIG_R3_REGNUM)
    u_addr = PT_ORIG_R3 * wordsize;
  if (regno == PPC_TRAP_REGNUM)
    u_addr = PT_TRAP * wordsize;
  if (tdep->ppc_fpscr_regnum >= 0
      && regno == tdep->ppc_fpscr_regnum)
    {
      /* NOTE: cagney/2005-02-08: On some 64-bit GNU/Linux systems the
	 kernel headers incorrectly contained the 32-bit definition of
	 PT_FPSCR.  For the 32-bit definition, floating-point
	 registers occupy two 32-bit "slots", and the FPSCR lives in
	 the second half of such a slot-pair (hence +1).  For 64-bit,
	 the FPSCR instead occupies the full 64-bit 2-word-slot and
	 hence no adjustment is necessary.  Hack around this.  */
      if (wordsize == 8 && PT_FPSCR == (48 + 32 + 1))
	u_addr = (48 + 32) * wordsize;
      /* If the FPSCR is 64-bit wide, we need to fetch the whole 64-bit
	 slot and not just its second word.  The PT_FPSCR supplied when
	 GDB is compiled as a 32-bit app doesn't reflect this.  */
      else if (wordsize == 4 && register_size (gdbarch, regno) == 8
	       && PT_FPSCR == (48 + 2*32 + 1))
	u_addr = (48 + 2*32) * wordsize;
      else
	u_addr = PT_FPSCR * wordsize;
    }
  return u_addr;
}

/* The Linux kernel ptrace interface for POWER7 VSX registers uses the
   registers set mechanism, as opposed to the interface for all the
   other registers, that stores/fetches each register individually.  */
static void
fetch_vsx_register (struct regcache *regcache, int tid, int regno)
{
  int ret;
  gdb_vsxregset_t regs;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int vsxregsize = register_size (gdbarch, tdep->ppc_vsr0_upper_regnum);

  ret = ptrace (PTRACE_GETVSXREGS, tid, 0, &regs);
  if (ret < 0)
    {
      if (errno == EIO)
	{
	  have_ptrace_getsetvsxregs = 0;
	  return;
	}
      perror_with_name (_("Unable to fetch VSX register"));
    }

  regcache_raw_supply (regcache, regno,
		       regs + (regno - tdep->ppc_vsr0_upper_regnum)
		       * vsxregsize);
}

/* The Linux kernel ptrace interface for AltiVec registers uses the
   registers set mechanism, as opposed to the interface for all the
   other registers, that stores/fetches each register individually.  */
static void
fetch_altivec_register (struct regcache *regcache, int tid, int regno)
{
  int ret;
  int offset = 0;
  gdb_vrregset_t regs;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int vrregsize = register_size (gdbarch, tdep->ppc_vr0_regnum);

  ret = ptrace (PTRACE_GETVRREGS, tid, 0, &regs);
  if (ret < 0)
    {
      if (errno == EIO)
        {
          have_ptrace_getvrregs = 0;
          return;
        }
      perror_with_name (_("Unable to fetch AltiVec register"));
    }
 
  /* VSCR is fetched as a 16 bytes quantity, but it is really 4 bytes
     long on the hardware.  We deal only with the lower 4 bytes of the
     vector.  VRSAVE is at the end of the array in a 4 bytes slot, so
     there is no need to define an offset for it.  */
  if (regno == (tdep->ppc_vrsave_regnum - 1))
    offset = vrregsize - register_size (gdbarch, tdep->ppc_vrsave_regnum);
  
  regcache_raw_supply (regcache, regno,
		       regs + (regno
			       - tdep->ppc_vr0_regnum) * vrregsize + offset);
}

/* Fetch the top 32 bits of TID's general-purpose registers and the
   SPE-specific registers, and place the results in EVRREGSET.  If we
   don't support PTRACE_GETEVRREGS, then just fill EVRREGSET with
   zeros.

   All the logic to deal with whether or not the PTRACE_GETEVRREGS and
   PTRACE_SETEVRREGS requests are supported is isolated here, and in
   set_spe_registers.  */
static void
get_spe_registers (int tid, struct gdb_evrregset_t *evrregset)
{
  if (have_ptrace_getsetevrregs)
    {
      if (ptrace (PTRACE_GETEVRREGS, tid, 0, evrregset) >= 0)
        return;
      else
        {
          /* EIO means that the PTRACE_GETEVRREGS request isn't supported;
             we just return zeros.  */
          if (errno == EIO)
            have_ptrace_getsetevrregs = 0;
          else
            /* Anything else needs to be reported.  */
            perror_with_name (_("Unable to fetch SPE registers"));
        }
    }

  memset (evrregset, 0, sizeof (*evrregset));
}

/* Supply values from TID for SPE-specific raw registers: the upper
   halves of the GPRs, the accumulator, and the spefscr.  REGNO must
   be the number of an upper half register, acc, spefscr, or -1 to
   supply the values of all registers.  */
static void
fetch_spe_register (struct regcache *regcache, int tid, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  struct gdb_evrregset_t evrregs;

  gdb_assert (sizeof (evrregs.evr[0])
              == register_size (gdbarch, tdep->ppc_ev0_upper_regnum));
  gdb_assert (sizeof (evrregs.acc)
              == register_size (gdbarch, tdep->ppc_acc_regnum));
  gdb_assert (sizeof (evrregs.spefscr)
              == register_size (gdbarch, tdep->ppc_spefscr_regnum));

  get_spe_registers (tid, &evrregs);

  if (regno == -1)
    {
      int i;

      for (i = 0; i < ppc_num_gprs; i++)
        regcache_raw_supply (regcache, tdep->ppc_ev0_upper_regnum + i,
                             &evrregs.evr[i]);
    }
  else if (tdep->ppc_ev0_upper_regnum <= regno
           && regno < tdep->ppc_ev0_upper_regnum + ppc_num_gprs)
    regcache_raw_supply (regcache, regno,
                         &evrregs.evr[regno - tdep->ppc_ev0_upper_regnum]);

  if (regno == -1
      || regno == tdep->ppc_acc_regnum)
    regcache_raw_supply (regcache, tdep->ppc_acc_regnum, &evrregs.acc);

  if (regno == -1
      || regno == tdep->ppc_spefscr_regnum)
    regcache_raw_supply (regcache, tdep->ppc_spefscr_regnum,
                         &evrregs.spefscr);
}

static void
fetch_register (struct regcache *regcache, int tid, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  /* This isn't really an address.  But ptrace thinks of it as one.  */
  CORE_ADDR regaddr = ppc_register_u_addr (gdbarch, regno);
  int bytes_transferred;
  unsigned int offset;         /* Offset of registers within the u area.  */
  gdb_byte buf[MAX_REGISTER_SIZE];

  if (altivec_register_p (gdbarch, regno))
    {
      /* If this is the first time through, or if it is not the first
         time through, and we have comfirmed that there is kernel
         support for such a ptrace request, then go and fetch the
         register.  */
      if (have_ptrace_getvrregs)
       {
         fetch_altivec_register (regcache, tid, regno);
         return;
       }
     /* If we have discovered that there is no ptrace support for
        AltiVec registers, fall through and return zeroes, because
        regaddr will be -1 in this case.  */
    }
  if (vsx_register_p (gdbarch, regno))
    {
      if (have_ptrace_getsetvsxregs)
	{
	  fetch_vsx_register (regcache, tid, regno);
	  return;
	}
    }
  else if (spe_register_p (gdbarch, regno))
    {
      fetch_spe_register (regcache, tid, regno);
      return;
    }

  if (regaddr == -1)
    {
      memset (buf, '\0', register_size (gdbarch, regno));   /* Supply zeroes */
      regcache_raw_supply (regcache, regno, buf);
      return;
    }

  /* Read the raw register using sizeof(long) sized chunks.  On a
     32-bit platform, 64-bit floating-point registers will require two
     transfers.  */
  for (bytes_transferred = 0;
       bytes_transferred < register_size (gdbarch, regno);
       bytes_transferred += sizeof (long))
    {
      long l;

      errno = 0;
      l = ptrace (PTRACE_PEEKUSER, tid, (PTRACE_TYPE_ARG3) regaddr, 0);
      regaddr += sizeof (long);
      if (errno != 0)
	{
          char message[128];
	  xsnprintf (message, sizeof (message), "reading register %s (#%d)",
		     gdbarch_register_name (gdbarch, regno), regno);
	  perror_with_name (message);
	}
      memcpy (&buf[bytes_transferred], &l, sizeof (l));
    }

  /* Now supply the register.  Keep in mind that the regcache's idea
     of the register's size may not be a multiple of sizeof
     (long).  */
  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_LITTLE)
    {
      /* Little-endian values are always found at the left end of the
         bytes transferred.  */
      regcache_raw_supply (regcache, regno, buf);
    }
  else if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
    {
      /* Big-endian values are found at the right end of the bytes
         transferred.  */
      size_t padding = (bytes_transferred - register_size (gdbarch, regno));
      regcache_raw_supply (regcache, regno, buf + padding);
    }
  else 
    internal_error (__FILE__, __LINE__,
                    _("fetch_register: unexpected byte order: %d"),
                    gdbarch_byte_order (gdbarch));
}

static void
supply_vsxregset (struct regcache *regcache, gdb_vsxregset_t *vsxregsetp)
{
  int i;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int vsxregsize = register_size (gdbarch, tdep->ppc_vsr0_upper_regnum);

  for (i = 0; i < ppc_num_vshrs; i++)
    {
	regcache_raw_supply (regcache, tdep->ppc_vsr0_upper_regnum + i,
			     *vsxregsetp + i * vsxregsize);
    }
}

static void
supply_vrregset (struct regcache *regcache, gdb_vrregset_t *vrregsetp)
{
  int i;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int num_of_vrregs = tdep->ppc_vrsave_regnum - tdep->ppc_vr0_regnum + 1;
  int vrregsize = register_size (gdbarch, tdep->ppc_vr0_regnum);
  int offset = vrregsize - register_size (gdbarch, tdep->ppc_vrsave_regnum);

  for (i = 0; i < num_of_vrregs; i++)
    {
      /* The last 2 registers of this set are only 32 bit long, not
         128.  However an offset is necessary only for VSCR because it
         occupies a whole vector, while VRSAVE occupies a full 4 bytes
         slot.  */
      if (i == (num_of_vrregs - 2))
        regcache_raw_supply (regcache, tdep->ppc_vr0_regnum + i,
			     *vrregsetp + i * vrregsize + offset);
      else
        regcache_raw_supply (regcache, tdep->ppc_vr0_regnum + i,
			     *vrregsetp + i * vrregsize);
    }
}

static void
fetch_vsx_registers (struct regcache *regcache, int tid)
{
  int ret;
  gdb_vsxregset_t regs;

  ret = ptrace (PTRACE_GETVSXREGS, tid, 0, &regs);
  if (ret < 0)
    {
      if (errno == EIO)
	{
	  have_ptrace_getsetvsxregs = 0;
	  return;
	}
      perror_with_name (_("Unable to fetch VSX registers"));
    }
  supply_vsxregset (regcache, &regs);
}

static void
fetch_altivec_registers (struct regcache *regcache, int tid)
{
  int ret;
  gdb_vrregset_t regs;
  
  ret = ptrace (PTRACE_GETVRREGS, tid, 0, &regs);
  if (ret < 0)
    {
      if (errno == EIO)
	{
          have_ptrace_getvrregs = 0;
	  return;
	}
      perror_with_name (_("Unable to fetch AltiVec registers"));
    }
  supply_vrregset (regcache, &regs);
}

/* This function actually issues the request to ptrace, telling
   it to get all general-purpose registers and put them into the
   specified regset.
   
   If the ptrace request does not exist, this function returns 0
   and properly sets the have_ptrace_* flag.  If the request fails,
   this function calls perror_with_name.  Otherwise, if the request
   succeeds, then the regcache gets filled and 1 is returned.  */
static int
fetch_all_gp_regs (struct regcache *regcache, int tid)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  gdb_gregset_t gregset;

  if (ptrace (PTRACE_GETREGS, tid, 0, (void *) &gregset) < 0)
    {
      if (errno == EIO)
        {
          have_ptrace_getsetregs = 0;
          return 0;
        }
      perror_with_name (_("Couldn't get general-purpose registers."));
    }

  supply_gregset (regcache, (const gdb_gregset_t *) &gregset);

  return 1;
}

/* This is a wrapper for the fetch_all_gp_regs function.  It is
   responsible for verifying if this target has the ptrace request
   that can be used to fetch all general-purpose registers at one
   shot.  If it doesn't, then we should fetch them using the
   old-fashioned way, which is to iterate over the registers and
   request them one by one.  */
static void
fetch_gp_regs (struct regcache *regcache, int tid)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int i;

  if (have_ptrace_getsetregs)
    if (fetch_all_gp_regs (regcache, tid))
      return;

  /* If we've hit this point, it doesn't really matter which
     architecture we are using.  We just need to read the
     registers in the "old-fashioned way".  */
  for (i = 0; i < ppc_num_gprs; i++)
    fetch_register (regcache, tid, tdep->ppc_gp0_regnum + i);
}

/* This function actually issues the request to ptrace, telling
   it to get all floating-point registers and put them into the
   specified regset.
   
   If the ptrace request does not exist, this function returns 0
   and properly sets the have_ptrace_* flag.  If the request fails,
   this function calls perror_with_name.  Otherwise, if the request
   succeeds, then the regcache gets filled and 1 is returned.  */
static int
fetch_all_fp_regs (struct regcache *regcache, int tid)
{
  gdb_fpregset_t fpregs;

  if (ptrace (PTRACE_GETFPREGS, tid, 0, (void *) &fpregs) < 0)
    {
      if (errno == EIO)
        {
          have_ptrace_getsetfpregs = 0;
          return 0;
        }
      perror_with_name (_("Couldn't get floating-point registers."));
    }

  supply_fpregset (regcache, (const gdb_fpregset_t *) &fpregs);

  return 1;
}

/* This is a wrapper for the fetch_all_fp_regs function.  It is
   responsible for verifying if this target has the ptrace request
   that can be used to fetch all floating-point registers at one
   shot.  If it doesn't, then we should fetch them using the
   old-fashioned way, which is to iterate over the registers and
   request them one by one.  */
static void
fetch_fp_regs (struct regcache *regcache, int tid)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int i;

  if (have_ptrace_getsetfpregs)
    if (fetch_all_fp_regs (regcache, tid))
      return;
 
  /* If we've hit this point, it doesn't really matter which
     architecture we are using.  We just need to read the
     registers in the "old-fashioned way".  */
  for (i = 0; i < ppc_num_fprs; i++)
    fetch_register (regcache, tid, tdep->ppc_fp0_regnum + i);
}

static void 
fetch_ppc_registers (struct regcache *regcache, int tid)
{
  int i;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  fetch_gp_regs (regcache, tid);
  if (tdep->ppc_fp0_regnum >= 0)
    fetch_fp_regs (regcache, tid);
  fetch_register (regcache, tid, gdbarch_pc_regnum (gdbarch));
  if (tdep->ppc_ps_regnum != -1)
    fetch_register (regcache, tid, tdep->ppc_ps_regnum);
  if (tdep->ppc_cr_regnum != -1)
    fetch_register (regcache, tid, tdep->ppc_cr_regnum);
  if (tdep->ppc_lr_regnum != -1)
    fetch_register (regcache, tid, tdep->ppc_lr_regnum);
  if (tdep->ppc_ctr_regnum != -1)
    fetch_register (regcache, tid, tdep->ppc_ctr_regnum);
  if (tdep->ppc_xer_regnum != -1)
    fetch_register (regcache, tid, tdep->ppc_xer_regnum);
  if (tdep->ppc_mq_regnum != -1)
    fetch_register (regcache, tid, tdep->ppc_mq_regnum);
  if (ppc_linux_trap_reg_p (gdbarch))
    {
      fetch_register (regcache, tid, PPC_ORIG_R3_REGNUM);
      fetch_register (regcache, tid, PPC_TRAP_REGNUM);
    }
  if (tdep->ppc_fpscr_regnum != -1)
    fetch_register (regcache, tid, tdep->ppc_fpscr_regnum);
  if (have_ptrace_getvrregs)
    if (tdep->ppc_vr0_regnum != -1 && tdep->ppc_vrsave_regnum != -1)
      fetch_altivec_registers (regcache, tid);
  if (have_ptrace_getsetvsxregs)
    if (tdep->ppc_vsr0_upper_regnum != -1)
      fetch_vsx_registers (regcache, tid);
  if (tdep->ppc_ev0_upper_regnum >= 0)
    fetch_spe_register (regcache, tid, -1);
}

/* Fetch registers from the child process.  Fetch all registers if
   regno == -1, otherwise fetch all general registers or all floating
   point registers depending upon the value of regno.  */
static void
ppc_linux_fetch_inferior_registers (struct target_ops *ops,
				    struct regcache *regcache, int regno)
{
  /* Overload thread id onto process id.  */
  int tid = TIDGET (inferior_ptid);

  /* No thread id, just use process id.  */
  if (tid == 0)
    tid = PIDGET (inferior_ptid);

  if (regno == -1)
    fetch_ppc_registers (regcache, tid);
  else 
    fetch_register (regcache, tid, regno);
}

/* Store one VSX register.  */
static void
store_vsx_register (const struct regcache *regcache, int tid, int regno)
{
  int ret;
  gdb_vsxregset_t regs;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int vsxregsize = register_size (gdbarch, tdep->ppc_vsr0_upper_regnum);

  ret = ptrace (PTRACE_GETVSXREGS, tid, 0, &regs);
  if (ret < 0)
    {
      if (errno == EIO)
	{
	  have_ptrace_getsetvsxregs = 0;
	  return;
	}
      perror_with_name (_("Unable to fetch VSX register"));
    }

  regcache_raw_collect (regcache, regno, regs +
			(regno - tdep->ppc_vsr0_upper_regnum) * vsxregsize);

  ret = ptrace (PTRACE_SETVSXREGS, tid, 0, &regs);
  if (ret < 0)
    perror_with_name (_("Unable to store VSX register"));
}

/* Store one register.  */
static void
store_altivec_register (const struct regcache *regcache, int tid, int regno)
{
  int ret;
  int offset = 0;
  gdb_vrregset_t regs;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int vrregsize = register_size (gdbarch, tdep->ppc_vr0_regnum);

  ret = ptrace (PTRACE_GETVRREGS, tid, 0, &regs);
  if (ret < 0)
    {
      if (errno == EIO)
        {
          have_ptrace_getvrregs = 0;
          return;
        }
      perror_with_name (_("Unable to fetch AltiVec register"));
    }

  /* VSCR is fetched as a 16 bytes quantity, but it is really 4 bytes
     long on the hardware.  */
  if (regno == (tdep->ppc_vrsave_regnum - 1))
    offset = vrregsize - register_size (gdbarch, tdep->ppc_vrsave_regnum);

  regcache_raw_collect (regcache, regno,
			regs + (regno
				- tdep->ppc_vr0_regnum) * vrregsize + offset);

  ret = ptrace (PTRACE_SETVRREGS, tid, 0, &regs);
  if (ret < 0)
    perror_with_name (_("Unable to store AltiVec register"));
}

/* Assuming TID referrs to an SPE process, set the top halves of TID's
   general-purpose registers and its SPE-specific registers to the
   values in EVRREGSET.  If we don't support PTRACE_SETEVRREGS, do
   nothing.

   All the logic to deal with whether or not the PTRACE_GETEVRREGS and
   PTRACE_SETEVRREGS requests are supported is isolated here, and in
   get_spe_registers.  */
static void
set_spe_registers (int tid, struct gdb_evrregset_t *evrregset)
{
  if (have_ptrace_getsetevrregs)
    {
      if (ptrace (PTRACE_SETEVRREGS, tid, 0, evrregset) >= 0)
        return;
      else
        {
          /* EIO means that the PTRACE_SETEVRREGS request isn't
             supported; we fail silently, and don't try the call
             again.  */
          if (errno == EIO)
            have_ptrace_getsetevrregs = 0;
          else
            /* Anything else needs to be reported.  */
            perror_with_name (_("Unable to set SPE registers"));
        }
    }
}

/* Write GDB's value for the SPE-specific raw register REGNO to TID.
   If REGNO is -1, write the values of all the SPE-specific
   registers.  */
static void
store_spe_register (const struct regcache *regcache, int tid, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  struct gdb_evrregset_t evrregs;

  gdb_assert (sizeof (evrregs.evr[0])
              == register_size (gdbarch, tdep->ppc_ev0_upper_regnum));
  gdb_assert (sizeof (evrregs.acc)
              == register_size (gdbarch, tdep->ppc_acc_regnum));
  gdb_assert (sizeof (evrregs.spefscr)
              == register_size (gdbarch, tdep->ppc_spefscr_regnum));

  if (regno == -1)
    /* Since we're going to write out every register, the code below
       should store to every field of evrregs; if that doesn't happen,
       make it obvious by initializing it with suspicious values.  */
    memset (&evrregs, 42, sizeof (evrregs));
  else
    /* We can only read and write the entire EVR register set at a
       time, so to write just a single register, we do a
       read-modify-write maneuver.  */
    get_spe_registers (tid, &evrregs);

  if (regno == -1)
    {
      int i;

      for (i = 0; i < ppc_num_gprs; i++)
        regcache_raw_collect (regcache,
                              tdep->ppc_ev0_upper_regnum + i,
                              &evrregs.evr[i]);
    }
  else if (tdep->ppc_ev0_upper_regnum <= regno
           && regno < tdep->ppc_ev0_upper_regnum + ppc_num_gprs)
    regcache_raw_collect (regcache, regno,
                          &evrregs.evr[regno - tdep->ppc_ev0_upper_regnum]);

  if (regno == -1
      || regno == tdep->ppc_acc_regnum)
    regcache_raw_collect (regcache,
                          tdep->ppc_acc_regnum,
                          &evrregs.acc);

  if (regno == -1
      || regno == tdep->ppc_spefscr_regnum)
    regcache_raw_collect (regcache,
                          tdep->ppc_spefscr_regnum,
                          &evrregs.spefscr);

  /* Write back the modified register set.  */
  set_spe_registers (tid, &evrregs);
}

static void
store_register (const struct regcache *regcache, int tid, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  /* This isn't really an address.  But ptrace thinks of it as one.  */
  CORE_ADDR regaddr = ppc_register_u_addr (gdbarch, regno);
  int i;
  size_t bytes_to_transfer;
  gdb_byte buf[MAX_REGISTER_SIZE];

  if (altivec_register_p (gdbarch, regno))
    {
      store_altivec_register (regcache, tid, regno);
      return;
    }
  if (vsx_register_p (gdbarch, regno))
    {
      store_vsx_register (regcache, tid, regno);
      return;
    }
  else if (spe_register_p (gdbarch, regno))
    {
      store_spe_register (regcache, tid, regno);
      return;
    }

  if (regaddr == -1)
    return;

  /* First collect the register.  Keep in mind that the regcache's
     idea of the register's size may not be a multiple of sizeof
     (long).  */
  memset (buf, 0, sizeof buf);
  bytes_to_transfer = align_up (register_size (gdbarch, regno), sizeof (long));
  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_LITTLE)
    {
      /* Little-endian values always sit at the left end of the buffer.  */
      regcache_raw_collect (regcache, regno, buf);
    }
  else if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
    {
      /* Big-endian values sit at the right end of the buffer.  */
      size_t padding = (bytes_to_transfer - register_size (gdbarch, regno));
      regcache_raw_collect (regcache, regno, buf + padding);
    }

  for (i = 0; i < bytes_to_transfer; i += sizeof (long))
    {
      long l;

      memcpy (&l, &buf[i], sizeof (l));
      errno = 0;
      ptrace (PTRACE_POKEUSER, tid, (PTRACE_TYPE_ARG3) regaddr, l);
      regaddr += sizeof (long);

      if (errno == EIO 
          && (regno == tdep->ppc_fpscr_regnum
	      || regno == PPC_ORIG_R3_REGNUM
	      || regno == PPC_TRAP_REGNUM))
	{
	  /* Some older kernel versions don't allow fpscr, orig_r3
	     or trap to be written.  */
	  continue;
	}

      if (errno != 0)
	{
          char message[128];
	  xsnprintf (message, sizeof (message), "writing register %s (#%d)",
		     gdbarch_register_name (gdbarch, regno), regno);
	  perror_with_name (message);
	}
    }
}

static void
fill_vsxregset (const struct regcache *regcache, gdb_vsxregset_t *vsxregsetp)
{
  int i;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int vsxregsize = register_size (gdbarch, tdep->ppc_vsr0_upper_regnum);

  for (i = 0; i < ppc_num_vshrs; i++)
    regcache_raw_collect (regcache, tdep->ppc_vsr0_upper_regnum + i,
			  *vsxregsetp + i * vsxregsize);
}

static void
fill_vrregset (const struct regcache *regcache, gdb_vrregset_t *vrregsetp)
{
  int i;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int num_of_vrregs = tdep->ppc_vrsave_regnum - tdep->ppc_vr0_regnum + 1;
  int vrregsize = register_size (gdbarch, tdep->ppc_vr0_regnum);
  int offset = vrregsize - register_size (gdbarch, tdep->ppc_vrsave_regnum);

  for (i = 0; i < num_of_vrregs; i++)
    {
      /* The last 2 registers of this set are only 32 bit long, not
         128, but only VSCR is fetched as a 16 bytes quantity.  */
      if (i == (num_of_vrregs - 2))
        regcache_raw_collect (regcache, tdep->ppc_vr0_regnum + i,
			      *vrregsetp + i * vrregsize + offset);
      else
        regcache_raw_collect (regcache, tdep->ppc_vr0_regnum + i,
			      *vrregsetp + i * vrregsize);
    }
}

static void
store_vsx_registers (const struct regcache *regcache, int tid)
{
  int ret;
  gdb_vsxregset_t regs;

  ret = ptrace (PTRACE_GETVSXREGS, tid, 0, &regs);
  if (ret < 0)
    {
      if (errno == EIO)
	{
	  have_ptrace_getsetvsxregs = 0;
	  return;
	}
      perror_with_name (_("Couldn't get VSX registers"));
    }

  fill_vsxregset (regcache, &regs);

  if (ptrace (PTRACE_SETVSXREGS, tid, 0, &regs) < 0)
    perror_with_name (_("Couldn't write VSX registers"));
}

static void
store_altivec_registers (const struct regcache *regcache, int tid)
{
  int ret;
  gdb_vrregset_t regs;

  ret = ptrace (PTRACE_GETVRREGS, tid, 0, &regs);
  if (ret < 0)
    {
      if (errno == EIO)
        {
          have_ptrace_getvrregs = 0;
          return;
        }
      perror_with_name (_("Couldn't get AltiVec registers"));
    }

  fill_vrregset (regcache, &regs);
  
  if (ptrace (PTRACE_SETVRREGS, tid, 0, &regs) < 0)
    perror_with_name (_("Couldn't write AltiVec registers"));
}

/* This function actually issues the request to ptrace, telling
   it to store all general-purpose registers present in the specified
   regset.
   
   If the ptrace request does not exist, this function returns 0
   and properly sets the have_ptrace_* flag.  If the request fails,
   this function calls perror_with_name.  Otherwise, if the request
   succeeds, then the regcache is stored and 1 is returned.  */
static int
store_all_gp_regs (const struct regcache *regcache, int tid, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  gdb_gregset_t gregset;

  if (ptrace (PTRACE_GETREGS, tid, 0, (void *) &gregset) < 0)
    {
      if (errno == EIO)
        {
          have_ptrace_getsetregs = 0;
          return 0;
        }
      perror_with_name (_("Couldn't get general-purpose registers."));
    }

  fill_gregset (regcache, &gregset, regno);

  if (ptrace (PTRACE_SETREGS, tid, 0, (void *) &gregset) < 0)
    {
      if (errno == EIO)
        {
          have_ptrace_getsetregs = 0;
          return 0;
        }
      perror_with_name (_("Couldn't set general-purpose registers."));
    }

  return 1;
}

/* This is a wrapper for the store_all_gp_regs function.  It is
   responsible for verifying if this target has the ptrace request
   that can be used to store all general-purpose registers at one
   shot.  If it doesn't, then we should store them using the
   old-fashioned way, which is to iterate over the registers and
   store them one by one.  */
static void
store_gp_regs (const struct regcache *regcache, int tid, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int i;

  if (have_ptrace_getsetregs)
    if (store_all_gp_regs (regcache, tid, regno))
      return;

  /* If we hit this point, it doesn't really matter which
     architecture we are using.  We just need to store the
     registers in the "old-fashioned way".  */
  for (i = 0; i < ppc_num_gprs; i++)
    store_register (regcache, tid, tdep->ppc_gp0_regnum + i);
}

/* This function actually issues the request to ptrace, telling
   it to store all floating-point registers present in the specified
   regset.
   
   If the ptrace request does not exist, this function returns 0
   and properly sets the have_ptrace_* flag.  If the request fails,
   this function calls perror_with_name.  Otherwise, if the request
   succeeds, then the regcache is stored and 1 is returned.  */
static int
store_all_fp_regs (const struct regcache *regcache, int tid, int regno)
{
  gdb_fpregset_t fpregs;

  if (ptrace (PTRACE_GETFPREGS, tid, 0, (void *) &fpregs) < 0)
    {
      if (errno == EIO)
        {
          have_ptrace_getsetfpregs = 0;
          return 0;
        }
      perror_with_name (_("Couldn't get floating-point registers."));
    }

  fill_fpregset (regcache, &fpregs, regno);

  if (ptrace (PTRACE_SETFPREGS, tid, 0, (void *) &fpregs) < 0)
    {
      if (errno == EIO)
        {
          have_ptrace_getsetfpregs = 0;
          return 0;
        }
      perror_with_name (_("Couldn't set floating-point registers."));
    }

  return 1;
}

/* This is a wrapper for the store_all_fp_regs function.  It is
   responsible for verifying if this target has the ptrace request
   that can be used to store all floating-point registers at one
   shot.  If it doesn't, then we should store them using the
   old-fashioned way, which is to iterate over the registers and
   store them one by one.  */
static void
store_fp_regs (const struct regcache *regcache, int tid, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int i;

  if (have_ptrace_getsetfpregs)
    if (store_all_fp_regs (regcache, tid, regno))
      return;

  /* If we hit this point, it doesn't really matter which
     architecture we are using.  We just need to store the
     registers in the "old-fashioned way".  */
  for (i = 0; i < ppc_num_fprs; i++)
    store_register (regcache, tid, tdep->ppc_fp0_regnum + i);
}

static void
store_ppc_registers (const struct regcache *regcache, int tid)
{
  int i;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
 
  store_gp_regs (regcache, tid, -1);
  if (tdep->ppc_fp0_regnum >= 0)
    store_fp_regs (regcache, tid, -1);
  store_register (regcache, tid, gdbarch_pc_regnum (gdbarch));
  if (tdep->ppc_ps_regnum != -1)
    store_register (regcache, tid, tdep->ppc_ps_regnum);
  if (tdep->ppc_cr_regnum != -1)
    store_register (regcache, tid, tdep->ppc_cr_regnum);
  if (tdep->ppc_lr_regnum != -1)
    store_register (regcache, tid, tdep->ppc_lr_regnum);
  if (tdep->ppc_ctr_regnum != -1)
    store_register (regcache, tid, tdep->ppc_ctr_regnum);
  if (tdep->ppc_xer_regnum != -1)
    store_register (regcache, tid, tdep->ppc_xer_regnum);
  if (tdep->ppc_mq_regnum != -1)
    store_register (regcache, tid, tdep->ppc_mq_regnum);
  if (tdep->ppc_fpscr_regnum != -1)
    store_register (regcache, tid, tdep->ppc_fpscr_regnum);
  if (ppc_linux_trap_reg_p (gdbarch))
    {
      store_register (regcache, tid, PPC_ORIG_R3_REGNUM);
      store_register (regcache, tid, PPC_TRAP_REGNUM);
    }
  if (have_ptrace_getvrregs)
    if (tdep->ppc_vr0_regnum != -1 && tdep->ppc_vrsave_regnum != -1)
      store_altivec_registers (regcache, tid);
  if (have_ptrace_getsetvsxregs)
    if (tdep->ppc_vsr0_upper_regnum != -1)
      store_vsx_registers (regcache, tid);
  if (tdep->ppc_ev0_upper_regnum >= 0)
    store_spe_register (regcache, tid, -1);
}

/* Fetch the AT_HWCAP entry from the aux vector.  */
static unsigned long
ppc_linux_get_hwcap (void)
{
  CORE_ADDR field;

  if (target_auxv_search (&current_target, AT_HWCAP, &field))
    return (unsigned long) field;

  return 0;
}

/* The cached DABR value, to install in new threads.
   This variable is used when we are dealing with non-BookE
   processors.  */
static long saved_dabr_value;

/* Global structure that will store information about the available
   features on this BookE processor.  */
static struct ppc_debug_info booke_debug_info;

/* Global variable that holds the maximum number of slots that the
   kernel will use.  This is only used when the processor is BookE.  */
static size_t max_slots_number = 0;

struct hw_break_tuple
{
  long slot;
  struct ppc_hw_breakpoint *hw_break;
};

/* This is an internal VEC created to store information about *points inserted
   for each thread.  This is used for BookE processors.  */
typedef struct thread_points
  {
    /* The TID to which this *point relates.  */
    int tid;
    /* Information about the *point, such as its address, type, etc.

       Each element inside this vector corresponds to a hardware
       breakpoint or watchpoint in the thread represented by TID.  The maximum
       size of these vector is MAX_SLOTS_NUMBER.  If the hw_break element of
       the tuple is NULL, then the position in the vector is free.  */
    struct hw_break_tuple *hw_breaks;
  } *thread_points_p;
DEF_VEC_P (thread_points_p);

VEC(thread_points_p) *ppc_threads = NULL;

/* The version of the kernel interface that we will use if the processor is
   BookE.  */
#define PPC_DEBUG_CURRENT_VERSION 1

/* Returns non-zero if we support the ptrace interface which enables
   booke debugging resources.  */
static int
have_ptrace_booke_interface (void)
{
  static int have_ptrace_booke_interface = -1;

  if (have_ptrace_booke_interface == -1)
    {
      int tid;

      tid = TIDGET (inferior_ptid);
      if (tid == 0)
	tid = PIDGET (inferior_ptid);

      /* Check for kernel support for BOOKE debug registers.  */
      if (ptrace (PPC_PTRACE_GETHWDBGINFO, tid, 0, &booke_debug_info) >= 0)
	{
	  /* Check whether ptrace BOOKE interface is functional and
	     provides any supported feature.  */
	  if (booke_debug_info.features != 0)
	    {
	      have_ptrace_booke_interface = 1;
	      max_slots_number = booke_debug_info.num_instruction_bps
	        + booke_debug_info.num_data_bps
	        + booke_debug_info.num_condition_regs;
	      return have_ptrace_booke_interface;
	    }
	}
      /* Old school interface and no BOOKE debug registers support.  */
      have_ptrace_booke_interface = 0;
      memset (&booke_debug_info, 0, sizeof (struct ppc_debug_info));
    }

  return have_ptrace_booke_interface;
}

static int
ppc_linux_can_use_hw_breakpoint (int type, int cnt, int ot)
{
  int total_hw_wp, total_hw_bp;

  if (have_ptrace_booke_interface ())
    {
      /* For PPC BookE processors, the number of available hardware
         watchpoints and breakpoints is stored at the booke_debug_info
	 struct.  */
      total_hw_bp = booke_debug_info.num_instruction_bps;
      total_hw_wp = booke_debug_info.num_data_bps;
    }
  else
    {
      /* For PPC server processors, we accept 1 hardware watchpoint and 0
	 hardware breakpoints.  */
      total_hw_bp = 0;
      total_hw_wp = 1;
    }

  if (type == bp_hardware_watchpoint || type == bp_read_watchpoint
      || type == bp_access_watchpoint || type == bp_watchpoint)
    {
      if (cnt + ot > total_hw_wp)
	return -1;
    }
  else if (type == bp_hardware_breakpoint)
    {
      if (cnt > total_hw_bp)
	return -1;
    }

  if (!have_ptrace_booke_interface ())
    {
      int tid;
      ptid_t ptid = inferior_ptid;

      /* We need to know whether ptrace supports PTRACE_SET_DEBUGREG
	 and whether the target has DABR.  If either answer is no, the
	 ptrace call will return -1.  Fail in that case.  */
      tid = TIDGET (ptid);
      if (tid == 0)
	tid = PIDGET (ptid);

      if (ptrace (PTRACE_SET_DEBUGREG, tid, 0, 0) == -1)
	return 0;
    }

  return 1;
}

static int
ppc_linux_region_ok_for_hw_watchpoint (CORE_ADDR addr, int len)
{
  /* Handle sub-8-byte quantities.  */
  if (len <= 0)
    return 0;

  /* The new BookE ptrace interface tells if there are alignment restrictions
     for watchpoints in the processors.  In that case, we use that information
     to determine the hardcoded watchable region for watchpoints.  */
  if (have_ptrace_booke_interface ())
    {
      /* DAC-based processors (i.e., embedded processors), like the PowerPC 440
	 have ranged watchpoints and can watch any access within an arbitrary
	 memory region.  This is useful to watch arrays and structs, for
	 instance.  It takes two hardware watchpoints though.  */
      if (len > 1
	  && booke_debug_info.features & PPC_DEBUG_FEATURE_DATA_BP_RANGE)
	return 2;
      else if (booke_debug_info.data_bp_alignment
	       && (addr + len > (addr & ~(booke_debug_info.data_bp_alignment - 1))
		   + booke_debug_info.data_bp_alignment))
	return 0;
    }
  /* addr+len must fall in the 8 byte watchable region for DABR-based
     processors (i.e., server processors).  Without the new BookE ptrace
     interface, DAC-based processors (i.e., embedded processors) will use
     addresses aligned to 4-bytes due to the way the read/write flags are
     passed in the old ptrace interface.  */
  else if (((ppc_linux_get_hwcap () & PPC_FEATURE_BOOKE)
	   && (addr + len) > (addr & ~3) + 4)
	   || (addr + len) > (addr & ~7) + 8)
    return 0;

  return 1;
}

/* This function compares two ppc_hw_breakpoint structs field-by-field.  */
static int
booke_cmp_hw_point (struct ppc_hw_breakpoint *a, struct ppc_hw_breakpoint *b)
{
  return (a->trigger_type == b->trigger_type
	  && a->addr_mode == b->addr_mode
	  && a->condition_mode == b->condition_mode
	  && a->addr == b->addr
	  && a->addr2 == b->addr2
	  && a->condition_value == b->condition_value);
}

/* This function can be used to retrieve a thread_points by the TID of the
   related process/thread.  If nothing has been found, and ALLOC_NEW is 0,
   it returns NULL.  If ALLOC_NEW is non-zero, a new thread_points for the
   provided TID will be created and returned.  */
static struct thread_points *
booke_find_thread_points_by_tid (int tid, int alloc_new)
{
  int i;
  struct thread_points *t;

  for (i = 0; VEC_iterate (thread_points_p, ppc_threads, i, t); i++)
    if (t->tid == tid)
      return t;

  t = NULL;

  /* Do we need to allocate a new point_item
     if the wanted one does not exist?  */
  if (alloc_new)
    {
      t = xmalloc (sizeof (struct thread_points));
      t->hw_breaks
	= xzalloc (max_slots_number * sizeof (struct hw_break_tuple));
      t->tid = tid;
      VEC_safe_push (thread_points_p, ppc_threads, t);
    }

  return t;
}

/* This function is a generic wrapper that is responsible for inserting a
   *point (i.e., calling `ptrace' in order to issue the request to the
   kernel) and registering it internally in GDB.  */
static void
booke_insert_point (struct ppc_hw_breakpoint *b, int tid)
{
  int i;
  long slot;
  struct ppc_hw_breakpoint *p = xmalloc (sizeof (struct ppc_hw_breakpoint));
  struct hw_break_tuple *hw_breaks;
  struct cleanup *c = make_cleanup (xfree, p);
  struct thread_points *t;
  struct hw_break_tuple *tuple;

  memcpy (p, b, sizeof (struct ppc_hw_breakpoint));

  errno = 0;
  slot = ptrace (PPC_PTRACE_SETHWDEBUG, tid, 0, p);
  if (slot < 0)
    perror_with_name (_("Unexpected error setting breakpoint or watchpoint"));

  /* Everything went fine, so we have to register this *point.  */
  t = booke_find_thread_points_by_tid (tid, 1);
  gdb_assert (t != NULL);
  hw_breaks = t->hw_breaks;

  /* Find a free element in the hw_breaks vector.  */
  for (i = 0; i < max_slots_number; i++)
    if (hw_breaks[i].hw_break == NULL)
      {
	hw_breaks[i].slot = slot;
	hw_breaks[i].hw_break = p;
	break;
      }

  gdb_assert (i != max_slots_number);

  discard_cleanups (c);
}

/* This function is a generic wrapper that is responsible for removing a
   *point (i.e., calling `ptrace' in order to issue the request to the
   kernel), and unregistering it internally at GDB.  */
static void
booke_remove_point (struct ppc_hw_breakpoint *b, int tid)
{
  int i;
  struct hw_break_tuple *hw_breaks;
  struct thread_points *t;

  t = booke_find_thread_points_by_tid (tid, 0);
  gdb_assert (t != NULL);
  hw_breaks = t->hw_breaks;

  for (i = 0; i < max_slots_number; i++)
    if (hw_breaks[i].hw_break && booke_cmp_hw_point (hw_breaks[i].hw_break, b))
      break;

  gdb_assert (i != max_slots_number);

  /* We have to ignore ENOENT errors because the kernel implements hardware
     breakpoints/watchpoints as "one-shot", that is, they are automatically
     deleted when hit.  */
  errno = 0;
  if (ptrace (PPC_PTRACE_DELHWDEBUG, tid, 0, hw_breaks[i].slot) < 0)
    if (errno != ENOENT)
      perror_with_name (_("Unexpected error deleting "
			  "breakpoint or watchpoint"));

  xfree (hw_breaks[i].hw_break);
  hw_breaks[i].hw_break = NULL;
}

/* Return the number of registers needed for a ranged breakpoint.  */

static int
ppc_linux_ranged_break_num_registers (struct target_ops *target)
{
  return ((have_ptrace_booke_interface ()
	   && booke_debug_info.features & PPC_DEBUG_FEATURE_INSN_BP_RANGE)?
	  2 : -1);
}

/* Insert the hardware breakpoint described by BP_TGT.  Returns 0 for
   success, 1 if hardware breakpoints are not supported or -1 for failure.  */

static int
ppc_linux_insert_hw_breakpoint (struct gdbarch *gdbarch,
				  struct bp_target_info *bp_tgt)
{
  struct lwp_info *lp;
  struct ppc_hw_breakpoint p;

  if (!have_ptrace_booke_interface ())
    return -1;

  p.version = PPC_DEBUG_CURRENT_VERSION;
  p.trigger_type = PPC_BREAKPOINT_TRIGGER_EXECUTE;
  p.condition_mode = PPC_BREAKPOINT_CONDITION_NONE;
  p.addr = (uint64_t) bp_tgt->placed_address;
  p.condition_value = 0;

  if (bp_tgt->length)
    {
      p.addr_mode = PPC_BREAKPOINT_MODE_RANGE_INCLUSIVE;

      /* The breakpoint will trigger if the address of the instruction is
	 within the defined range, as follows: p.addr <= address < p.addr2.  */
      p.addr2 = (uint64_t) bp_tgt->placed_address + bp_tgt->length;
    }
  else
    {
      p.addr_mode = PPC_BREAKPOINT_MODE_EXACT;
      p.addr2 = 0;
    }

  ALL_LWPS (lp)
    booke_insert_point (&p, TIDGET (lp->ptid));

  return 0;
}

static int
ppc_linux_remove_hw_breakpoint (struct gdbarch *gdbarch,
				  struct bp_target_info *bp_tgt)
{
  struct lwp_info *lp;
  struct ppc_hw_breakpoint p;

  if (!have_ptrace_booke_interface ())
    return -1;

  p.version = PPC_DEBUG_CURRENT_VERSION;
  p.trigger_type = PPC_BREAKPOINT_TRIGGER_EXECUTE;
  p.condition_mode = PPC_BREAKPOINT_CONDITION_NONE;
  p.addr = (uint64_t) bp_tgt->placed_address;
  p.condition_value = 0;

  if (bp_tgt->length)
    {
      p.addr_mode = PPC_BREAKPOINT_MODE_RANGE_INCLUSIVE;

      /* The breakpoint will trigger if the address of the instruction is within
	 the defined range, as follows: p.addr <= address < p.addr2.  */
      p.addr2 = (uint64_t) bp_tgt->placed_address + bp_tgt->length;
    }
  else
    {
      p.addr_mode = PPC_BREAKPOINT_MODE_EXACT;
      p.addr2 = 0;
    }

  ALL_LWPS (lp)
    booke_remove_point (&p, TIDGET (lp->ptid));

  return 0;
}

static int
get_trigger_type (int rw)
{
  int t;

  if (rw == hw_read)
    t = PPC_BREAKPOINT_TRIGGER_READ;
  else if (rw == hw_write)
    t = PPC_BREAKPOINT_TRIGGER_WRITE;
  else
    t = PPC_BREAKPOINT_TRIGGER_READ | PPC_BREAKPOINT_TRIGGER_WRITE;

  return t;
}

/* Insert a new masked watchpoint at ADDR using the mask MASK.
   RW may be hw_read for a read watchpoint, hw_write for a write watchpoint
   or hw_access for an access watchpoint.  Returns 0 on success and throws
   an error on failure.  */

static int
ppc_linux_insert_mask_watchpoint (struct target_ops *ops, CORE_ADDR addr,
				  CORE_ADDR mask, int rw)
{
  struct lwp_info *lp;
  struct ppc_hw_breakpoint p;

  gdb_assert (have_ptrace_booke_interface ());

  p.version = PPC_DEBUG_CURRENT_VERSION;
  p.trigger_type = get_trigger_type (rw);
  p.addr_mode = PPC_BREAKPOINT_MODE_MASK;
  p.condition_mode = PPC_BREAKPOINT_CONDITION_NONE;
  p.addr = addr;
  p.addr2 = mask;
  p.condition_value = 0;

  ALL_LWPS (lp)
    booke_insert_point (&p, TIDGET (lp->ptid));

  return 0;
}

/* Remove a masked watchpoint at ADDR with the mask MASK.
   RW may be hw_read for a read watchpoint, hw_write for a write watchpoint
   or hw_access for an access watchpoint.  Returns 0 on success and throws
   an error on failure.  */

static int
ppc_linux_remove_mask_watchpoint (struct target_ops *ops, CORE_ADDR addr,
				  CORE_ADDR mask, int rw)
{
  struct lwp_info *lp;
  struct ppc_hw_breakpoint p;

  gdb_assert (have_ptrace_booke_interface ());

  p.version = PPC_DEBUG_CURRENT_VERSION;
  p.trigger_type = get_trigger_type (rw);
  p.addr_mode = PPC_BREAKPOINT_MODE_MASK;
  p.condition_mode = PPC_BREAKPOINT_CONDITION_NONE;
  p.addr = addr;
  p.addr2 = mask;
  p.condition_value = 0;

  ALL_LWPS (lp)
    booke_remove_point (&p, TIDGET (lp->ptid));

  return 0;
}

/* Check whether we have at least one free DVC register.  */
static int
can_use_watchpoint_cond_accel (void)
{
  struct thread_points *p;
  int tid = TIDGET (inferior_ptid);
  int cnt = booke_debug_info.num_condition_regs, i;
  CORE_ADDR tmp_value;

  if (!have_ptrace_booke_interface () || cnt == 0)
    return 0;

  p = booke_find_thread_points_by_tid (tid, 0);

  if (p)
    {
      for (i = 0; i < max_slots_number; i++)
	if (p->hw_breaks[i].hw_break != NULL
	    && (p->hw_breaks[i].hw_break->condition_mode
		!= PPC_BREAKPOINT_CONDITION_NONE))
	  cnt--;

      /* There are no available slots now.  */
      if (cnt <= 0)
	return 0;
    }

  return 1;
}

/* Calculate the enable bits and the contents of the Data Value Compare
   debug register present in BookE processors.

   ADDR is the address to be watched, LEN is the length of watched data
   and DATA_VALUE is the value which will trigger the watchpoint.
   On exit, CONDITION_MODE will hold the enable bits for the DVC, and
   CONDITION_VALUE will hold the value which should be put in the
   DVC register.  */
static void
calculate_dvc (CORE_ADDR addr, int len, CORE_ADDR data_value,
	       uint32_t *condition_mode, uint64_t *condition_value)
{
  int i, num_byte_enable, align_offset, num_bytes_off_dvc,
      rightmost_enabled_byte;
  CORE_ADDR addr_end_data, addr_end_dvc;

  /* The DVC register compares bytes within fixed-length windows which
     are word-aligned, with length equal to that of the DVC register.
     We need to calculate where our watch region is relative to that
     window and enable comparison of the bytes which fall within it.  */

  align_offset = addr % booke_debug_info.sizeof_condition;
  addr_end_data = addr + len;
  addr_end_dvc = (addr - align_offset
		  + booke_debug_info.sizeof_condition);
  num_bytes_off_dvc = (addr_end_data > addr_end_dvc)?
			 addr_end_data - addr_end_dvc : 0;
  num_byte_enable = len - num_bytes_off_dvc;
  /* Here, bytes are numbered from right to left.  */
  rightmost_enabled_byte = (addr_end_data < addr_end_dvc)?
			      addr_end_dvc - addr_end_data : 0;

  *condition_mode = PPC_BREAKPOINT_CONDITION_AND;
  for (i = 0; i < num_byte_enable; i++)
    *condition_mode
      |= PPC_BREAKPOINT_CONDITION_BE (i + rightmost_enabled_byte);

  /* Now we need to match the position within the DVC of the comparison
     value with where the watch region is relative to the window
     (i.e., the ALIGN_OFFSET).  */

  *condition_value = ((uint64_t) data_value >> num_bytes_off_dvc * 8
		      << rightmost_enabled_byte * 8);
}

/* Return the number of memory locations that need to be accessed to
   evaluate the expression which generated the given value chain.
   Returns -1 if there's any register access involved, or if there are
   other kinds of values which are not acceptable in a condition
   expression (e.g., lval_computed or lval_internalvar).  */
static int
num_memory_accesses (struct value *v)
{
  int found_memory_cnt = 0;
  struct value *head = v;

  /* The idea here is that evaluating an expression generates a series
     of values, one holding the value of every subexpression.  (The
     expression a*b+c has five subexpressions: a, b, a*b, c, and
     a*b+c.)  GDB's values hold almost enough information to establish
     the criteria given above --- they identify memory lvalues,
     register lvalues, computed values, etcetera.  So we can evaluate
     the expression, and then scan the chain of values that leaves
     behind to determine the memory locations involved in the evaluation
     of an expression.

     However, I don't think that the values returned by inferior
     function calls are special in any way.  So this function may not
     notice that an expression contains an inferior function call.
     FIXME.  */

  for (; v; v = value_next (v))
    {
      /* Constants and values from the history are fine.  */
      if (VALUE_LVAL (v) == not_lval || deprecated_value_modifiable (v) == 0)
	continue;
      else if (VALUE_LVAL (v) == lval_memory)
	{
	  /* A lazy memory lvalue is one that GDB never needed to fetch;
	     we either just used its address (e.g., `a' in `a.b') or
	     we never needed it at all (e.g., `a' in `a,b').  */
	  if (!value_lazy (v))
	    found_memory_cnt++;
	}
      /* Other kinds of values are not fine.  */
      else
	return -1;
    }

  return found_memory_cnt;
}

/* Verifies whether the expression COND can be implemented using the
   DVC (Data Value Compare) register in BookE processors.  The expression
   must test the watch value for equality with a constant expression.
   If the function returns 1, DATA_VALUE will contain the constant against
   which the watch value should be compared and LEN will contain the size
   of the constant.  */
static int
check_condition (CORE_ADDR watch_addr, struct expression *cond,
		 CORE_ADDR *data_value, int *len)
{
  int pc = 1, num_accesses_left, num_accesses_right;
  struct value *left_val, *right_val, *left_chain, *right_chain;

  if (cond->elts[0].opcode != BINOP_EQUAL)
    return 0;

  fetch_subexp_value (cond, &pc, &left_val, NULL, &left_chain);
  num_accesses_left = num_memory_accesses (left_chain);

  if (left_val == NULL || num_accesses_left < 0)
    {
      free_value_chain (left_chain);

      return 0;
    }

  fetch_subexp_value (cond, &pc, &right_val, NULL, &right_chain);
  num_accesses_right = num_memory_accesses (right_chain);

  if (right_val == NULL || num_accesses_right < 0)
    {
      free_value_chain (left_chain);
      free_value_chain (right_chain);

      return 0;
    }

  if (num_accesses_left == 1 && num_accesses_right == 0
      && VALUE_LVAL (left_val) == lval_memory
      && value_address (left_val) == watch_addr)
    {
      *data_value = value_as_long (right_val);

      /* DATA_VALUE is the constant in RIGHT_VAL, but actually has
	 the same type as the memory region referenced by LEFT_VAL.  */
      *len = TYPE_LENGTH (check_typedef (value_type (left_val)));
    }
  else if (num_accesses_left == 0 && num_accesses_right == 1
	   && VALUE_LVAL (right_val) == lval_memory
	   && value_address (right_val) == watch_addr)
    {
      *data_value = value_as_long (left_val);

      /* DATA_VALUE is the constant in LEFT_VAL, but actually has
	 the same type as the memory region referenced by RIGHT_VAL.  */
      *len = TYPE_LENGTH (check_typedef (value_type (right_val)));
    }
  else
    {
      free_value_chain (left_chain);
      free_value_chain (right_chain);

      return 0;
    }

  free_value_chain (left_chain);
  free_value_chain (right_chain);

  return 1;
}

/* Return non-zero if the target is capable of using hardware to evaluate
   the condition expression, thus only triggering the watchpoint when it is
   true.  */
static int
ppc_linux_can_accel_watchpoint_condition (CORE_ADDR addr, int len, int rw,
					  struct expression *cond)
{
  CORE_ADDR data_value;

  return (have_ptrace_booke_interface ()
	  && booke_debug_info.num_condition_regs > 0
	  && check_condition (addr, cond, &data_value, &len));
}

/* Set up P with the parameters necessary to request a watchpoint covering
   LEN bytes starting at ADDR and if possible with condition expression COND
   evaluated by hardware.  INSERT tells if we are creating a request for
   inserting or removing the watchpoint.  */

static void
create_watchpoint_request (struct ppc_hw_breakpoint *p, CORE_ADDR addr,
			   int len, int rw, struct expression *cond,
			   int insert)
{
  if (len == 1
      || !(booke_debug_info.features & PPC_DEBUG_FEATURE_DATA_BP_RANGE))
    {
      int use_condition;
      CORE_ADDR data_value;

      use_condition = (insert? can_use_watchpoint_cond_accel ()
			: booke_debug_info.num_condition_regs > 0);
      if (cond && use_condition && check_condition (addr, cond,
						    &data_value, &len))
	calculate_dvc (addr, len, data_value, &p->condition_mode,
		       &p->condition_value);
      else
	{
	  p->condition_mode = PPC_BREAKPOINT_CONDITION_NONE;
	  p->condition_value = 0;
	}

      p->addr_mode = PPC_BREAKPOINT_MODE_EXACT;
      p->addr2 = 0;
    }
  else
    {
      p->addr_mode = PPC_BREAKPOINT_MODE_RANGE_INCLUSIVE;
      p->condition_mode = PPC_BREAKPOINT_CONDITION_NONE;
      p->condition_value = 0;

      /* The watchpoint will trigger if the address of the memory access is
	 within the defined range, as follows: p->addr <= address < p->addr2.

	 Note that the above sentence just documents how ptrace interprets
	 its arguments; the watchpoint is set to watch the range defined by
	 the user _inclusively_, as specified by the user interface.  */
      p->addr2 = (uint64_t) addr + len;
    }

  p->version = PPC_DEBUG_CURRENT_VERSION;
  p->trigger_type = get_trigger_type (rw);
  p->addr = (uint64_t) addr;
}

static int
ppc_linux_insert_watchpoint (CORE_ADDR addr, int len, int rw,
			     struct expression *cond)
{
  struct lwp_info *lp;
  int ret = -1;

  if (have_ptrace_booke_interface ())
    {
      struct ppc_hw_breakpoint p;

      create_watchpoint_request (&p, addr, len, rw, cond, 1);

      ALL_LWPS (lp)
	booke_insert_point (&p, TIDGET (lp->ptid));

      ret = 0;
    }
  else
    {
      long dabr_value;
      long read_mode, write_mode;

      if (ppc_linux_get_hwcap () & PPC_FEATURE_BOOKE)
	{
	  /* PowerPC 440 requires only the read/write flags to be passed
	     to the kernel.  */
	  read_mode = 1;
	  write_mode = 2;
	}
      else
	{
	  /* PowerPC 970 and other DABR-based processors are required to pass
	     the Breakpoint Translation bit together with the flags.  */
	  read_mode = 5;
	  write_mode = 6;
	}

      dabr_value = addr & ~(read_mode | write_mode);
      switch (rw)
	{
	  case hw_read:
	    /* Set read and translate bits.  */
	    dabr_value |= read_mode;
	    break;
	  case hw_write:
	    /* Set write and translate bits.  */
	    dabr_value |= write_mode;
	    break;
	  case hw_access:
	    /* Set read, write and translate bits.  */
	    dabr_value |= read_mode | write_mode;
	    break;
	}

      saved_dabr_value = dabr_value;

      ALL_LWPS (lp)
	if (ptrace (PTRACE_SET_DEBUGREG, TIDGET (lp->ptid), 0,
		    saved_dabr_value) < 0)
	  return -1;

      ret = 0;
    }

  return ret;
}

static int
ppc_linux_remove_watchpoint (CORE_ADDR addr, int len, int rw,
			     struct expression *cond)
{
  struct lwp_info *lp;
  int ret = -1;

  if (have_ptrace_booke_interface ())
    {
      struct ppc_hw_breakpoint p;

      create_watchpoint_request (&p, addr, len, rw, cond, 0);

      ALL_LWPS (lp)
	booke_remove_point (&p, TIDGET (lp->ptid));

      ret = 0;
    }
  else
    {
      saved_dabr_value = 0;
      ALL_LWPS (lp)
	if (ptrace (PTRACE_SET_DEBUGREG, TIDGET (lp->ptid), 0,
		    saved_dabr_value) < 0)
	  return -1;

      ret = 0;
    }

  return ret;
}

static void
ppc_linux_new_thread (struct lwp_info *lp)
{
  int tid = TIDGET (lp->ptid);

  if (have_ptrace_booke_interface ())
    {
      int i;
      struct thread_points *p;
      struct hw_break_tuple *hw_breaks;

      if (VEC_empty (thread_points_p, ppc_threads))
	return;

      /* Get a list of breakpoints from any thread.  */
      p = VEC_last (thread_points_p, ppc_threads);
      hw_breaks = p->hw_breaks;

      /* Copy that thread's breakpoints and watchpoints to the new thread.  */
      for (i = 0; i < max_slots_number; i++)
	if (hw_breaks[i].hw_break)
	  booke_insert_point (hw_breaks[i].hw_break, tid);
    }
  else
    ptrace (PTRACE_SET_DEBUGREG, tid, 0, saved_dabr_value);
}

static void
ppc_linux_thread_exit (struct thread_info *tp, int silent)
{
  int i;
  int tid = TIDGET (tp->ptid);
  struct hw_break_tuple *hw_breaks;
  struct thread_points *t = NULL, *p;

  if (!have_ptrace_booke_interface ())
    return;

  for (i = 0; VEC_iterate (thread_points_p, ppc_threads, i, p); i++)
    if (p->tid == tid)
      {
	t = p;
	break;
      }

  if (t == NULL)
    return;

  VEC_unordered_remove (thread_points_p, ppc_threads, i);

  hw_breaks = t->hw_breaks;

  for (i = 0; i < max_slots_number; i++)
    if (hw_breaks[i].hw_break)
      xfree (hw_breaks[i].hw_break);

  xfree (t->hw_breaks);
  xfree (t);
}

static int
ppc_linux_stopped_data_address (struct target_ops *target, CORE_ADDR *addr_p)
{
  siginfo_t siginfo;

  if (!linux_nat_get_siginfo (inferior_ptid, &siginfo))
    return 0;

  if (siginfo.si_signo != SIGTRAP
      || (siginfo.si_code & 0xffff) != 0x0004 /* TRAP_HWBKPT */)
    return 0;

  if (have_ptrace_booke_interface ())
    {
      int i;
      struct thread_points *t;
      struct hw_break_tuple *hw_breaks;
      /* The index (or slot) of the *point is passed in the si_errno field.  */
      int slot = siginfo.si_errno;

      t = booke_find_thread_points_by_tid (TIDGET (inferior_ptid), 0);

      /* Find out if this *point is a hardware breakpoint.
	 If so, we should return 0.  */
      if (t)
	{
	  hw_breaks = t->hw_breaks;
	  for (i = 0; i < max_slots_number; i++)
	   if (hw_breaks[i].hw_break && hw_breaks[i].slot == slot
	       && hw_breaks[i].hw_break->trigger_type
		    == PPC_BREAKPOINT_TRIGGER_EXECUTE)
	     return 0;
	}
    }

  *addr_p = (CORE_ADDR) (uintptr_t) siginfo.si_addr;
  return 1;
}

static int
ppc_linux_stopped_by_watchpoint (void)
{
  CORE_ADDR addr;
  return ppc_linux_stopped_data_address (&current_target, &addr);
}

static int
ppc_linux_watchpoint_addr_within_range (struct target_ops *target,
					CORE_ADDR addr,
					CORE_ADDR start, int length)
{
  int mask;

  if (have_ptrace_booke_interface ()
      && ppc_linux_get_hwcap () & PPC_FEATURE_BOOKE)
    return start <= addr && start + length >= addr;
  else if (ppc_linux_get_hwcap () & PPC_FEATURE_BOOKE)
    mask = 3;
  else
    mask = 7;

  addr &= ~mask;

  /* Check whether [start, start+length-1] intersects [addr, addr+mask].  */
  return start <= addr + mask && start + length - 1 >= addr;
}

/* Return the number of registers needed for a masked hardware watchpoint.  */

static int
ppc_linux_masked_watch_num_registers (struct target_ops *target,
				      CORE_ADDR addr, CORE_ADDR mask)
{
  if (!have_ptrace_booke_interface ()
	   || (booke_debug_info.features & PPC_DEBUG_FEATURE_DATA_BP_MASK) == 0)
    return -1;
  else if ((mask & 0xC0000000) != 0xC0000000)
    {
      warning (_("The given mask covers kernel address space "
		 "and cannot be used.\n"));

      return -2;
    }
  else
    return 2;
}

static void
ppc_linux_store_inferior_registers (struct target_ops *ops,
				    struct regcache *regcache, int regno)
{
  /* Overload thread id onto process id.  */
  int tid = TIDGET (inferior_ptid);

  /* No thread id, just use process id.  */
  if (tid == 0)
    tid = PIDGET (inferior_ptid);

  if (regno >= 0)
    store_register (regcache, tid, regno);
  else
    store_ppc_registers (regcache, tid);
}

/* Functions for transferring registers between a gregset_t or fpregset_t
   (see sys/ucontext.h) and gdb's regcache.  The word size is that used
   by the ptrace interface, not the current program's ABI.  Eg. if a
   powerpc64-linux gdb is being used to debug a powerpc32-linux app, we
   read or write 64-bit gregsets.  This is to suit the host libthread_db.  */

void
supply_gregset (struct regcache *regcache, const gdb_gregset_t *gregsetp)
{
  const struct regset *regset = ppc_linux_gregset (sizeof (long));

  ppc_supply_gregset (regset, regcache, -1, gregsetp, sizeof (*gregsetp));
}

void
fill_gregset (const struct regcache *regcache,
	      gdb_gregset_t *gregsetp, int regno)
{
  const struct regset *regset = ppc_linux_gregset (sizeof (long));

  if (regno == -1)
    memset (gregsetp, 0, sizeof (*gregsetp));
  ppc_collect_gregset (regset, regcache, regno, gregsetp, sizeof (*gregsetp));
}

void
supply_fpregset (struct regcache *regcache, const gdb_fpregset_t * fpregsetp)
{
  const struct regset *regset = ppc_linux_fpregset ();

  ppc_supply_fpregset (regset, regcache, -1,
		       fpregsetp, sizeof (*fpregsetp));
}

void
fill_fpregset (const struct regcache *regcache,
	       gdb_fpregset_t *fpregsetp, int regno)
{
  const struct regset *regset = ppc_linux_fpregset ();

  ppc_collect_fpregset (regset, regcache, regno,
			fpregsetp, sizeof (*fpregsetp));
}

static int
ppc_linux_target_wordsize (void)
{
  int wordsize = 4;

  /* Check for 64-bit inferior process.  This is the case when the host is
     64-bit, and in addition the top bit of the MSR register is set.  */
#ifdef __powerpc64__
  long msr;

  int tid = TIDGET (inferior_ptid);
  if (tid == 0)
    tid = PIDGET (inferior_ptid);

  errno = 0;
  msr = (long) ptrace (PTRACE_PEEKUSER, tid, PT_MSR * 8, 0);
  if (errno == 0 && msr < 0)
    wordsize = 8;
#endif

  return wordsize;
}

static int
ppc_linux_auxv_parse (struct target_ops *ops, gdb_byte **readptr,
                      gdb_byte *endptr, CORE_ADDR *typep, CORE_ADDR *valp)
{
  int sizeof_auxv_field = ppc_linux_target_wordsize ();
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  gdb_byte *ptr = *readptr;

  if (endptr == ptr)
    return 0;

  if (endptr - ptr < sizeof_auxv_field * 2)
    return -1;

  *typep = extract_unsigned_integer (ptr, sizeof_auxv_field, byte_order);
  ptr += sizeof_auxv_field;
  *valp = extract_unsigned_integer (ptr, sizeof_auxv_field, byte_order);
  ptr += sizeof_auxv_field;

  *readptr = ptr;
  return 1;
}

static const struct target_desc *
ppc_linux_read_description (struct target_ops *ops)
{
  int altivec = 0;
  int vsx = 0;
  int isa205 = 0;
  int cell = 0;

  int tid = TIDGET (inferior_ptid);
  if (tid == 0)
    tid = PIDGET (inferior_ptid);

  if (have_ptrace_getsetevrregs)
    {
      struct gdb_evrregset_t evrregset;

      if (ptrace (PTRACE_GETEVRREGS, tid, 0, &evrregset) >= 0)
        return tdesc_powerpc_e500l;

      /* EIO means that the PTRACE_GETEVRREGS request isn't supported.
	 Anything else needs to be reported.  */
      else if (errno != EIO)
	perror_with_name (_("Unable to fetch SPE registers"));
    }

  if (have_ptrace_getsetvsxregs)
    {
      gdb_vsxregset_t vsxregset;

      if (ptrace (PTRACE_GETVSXREGS, tid, 0, &vsxregset) >= 0)
	vsx = 1;

      /* EIO means that the PTRACE_GETVSXREGS request isn't supported.
	 Anything else needs to be reported.  */
      else if (errno != EIO)
	perror_with_name (_("Unable to fetch VSX registers"));
    }

  if (have_ptrace_getvrregs)
    {
      gdb_vrregset_t vrregset;

      if (ptrace (PTRACE_GETVRREGS, tid, 0, &vrregset) >= 0)
        altivec = 1;

      /* EIO means that the PTRACE_GETVRREGS request isn't supported.
	 Anything else needs to be reported.  */
      else if (errno != EIO)
	perror_with_name (_("Unable to fetch AltiVec registers"));
    }

  /* Power ISA 2.05 (implemented by Power 6 and newer processors) increases
     the FPSCR from 32 bits to 64 bits.  Even though Power 7 supports this
     ISA version, it doesn't have PPC_FEATURE_ARCH_2_05 set, only
     PPC_FEATURE_ARCH_2_06.  Since for now the only bits used in the higher
     half of the register are for Decimal Floating Point, we check if that
     feature is available to decide the size of the FPSCR.  */
  if (ppc_linux_get_hwcap () & PPC_FEATURE_HAS_DFP)
    isa205 = 1;

  if (ppc_linux_get_hwcap () & PPC_FEATURE_CELL)
    cell = 1;

  if (ppc_linux_target_wordsize () == 8)
    {
      if (cell)
	return tdesc_powerpc_cell64l;
      else if (vsx)
	return isa205? tdesc_powerpc_isa205_vsx64l : tdesc_powerpc_vsx64l;
      else if (altivec)
	return isa205
	  ? tdesc_powerpc_isa205_altivec64l : tdesc_powerpc_altivec64l;

      return isa205? tdesc_powerpc_isa205_64l : tdesc_powerpc_64l;
    }

  if (cell)
    return tdesc_powerpc_cell32l;
  else if (vsx)
    return isa205? tdesc_powerpc_isa205_vsx32l : tdesc_powerpc_vsx32l;
  else if (altivec)
    return isa205? tdesc_powerpc_isa205_altivec32l : tdesc_powerpc_altivec32l;

  return isa205? tdesc_powerpc_isa205_32l : tdesc_powerpc_32l;
}

void _initialize_ppc_linux_nat (void);

void
_initialize_ppc_linux_nat (void)
{
  struct target_ops *t;

  /* Fill in the generic GNU/Linux methods.  */
  t = linux_target ();

  /* Add our register access methods.  */
  t->to_fetch_registers = ppc_linux_fetch_inferior_registers;
  t->to_store_registers = ppc_linux_store_inferior_registers;

  /* Add our breakpoint/watchpoint methods.  */
  t->to_can_use_hw_breakpoint = ppc_linux_can_use_hw_breakpoint;
  t->to_insert_hw_breakpoint = ppc_linux_insert_hw_breakpoint;
  t->to_remove_hw_breakpoint = ppc_linux_remove_hw_breakpoint;
  t->to_region_ok_for_hw_watchpoint = ppc_linux_region_ok_for_hw_watchpoint;
  t->to_insert_watchpoint = ppc_linux_insert_watchpoint;
  t->to_remove_watchpoint = ppc_linux_remove_watchpoint;
  t->to_insert_mask_watchpoint = ppc_linux_insert_mask_watchpoint;
  t->to_remove_mask_watchpoint = ppc_linux_remove_mask_watchpoint;
  t->to_stopped_by_watchpoint = ppc_linux_stopped_by_watchpoint;
  t->to_stopped_data_address = ppc_linux_stopped_data_address;
  t->to_watchpoint_addr_within_range = ppc_linux_watchpoint_addr_within_range;
  t->to_can_accel_watchpoint_condition
    = ppc_linux_can_accel_watchpoint_condition;
  t->to_masked_watch_num_registers = ppc_linux_masked_watch_num_registers;
  t->to_ranged_break_num_registers = ppc_linux_ranged_break_num_registers;

  t->to_read_description = ppc_linux_read_description;
  t->to_auxv_parse = ppc_linux_auxv_parse;

  observer_attach_thread_exit (ppc_linux_thread_exit);

  /* Register the target.  */
  linux_nat_add_target (t);
  linux_nat_set_new_thread (t, ppc_linux_new_thread);
}
