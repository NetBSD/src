/* Native-dependent code for NetBSD/i386, for GDB.
   Copyright 1988, 1989, 1991, 1992, 1994, 1996, 2000
   Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include "inferior.h"
#include "gdbcore.h"

#ifdef PT_GETXMMREGS
static int have_ptrace_xmmregs = -1;
#endif

#define RF(dst, src) \
	supply_register((dst), (char *) &(src))

#define RS(src, dst) \
	memcpy(&dst, &registers[REGISTER_BYTE(src)], sizeof(dst))
     
struct env387
  {
    unsigned short control;
    unsigned short r0;
    unsigned short status;
    unsigned short r1;
    unsigned short tag;  
    unsigned short r2;
    unsigned long eip;
    unsigned short code_seg;
    unsigned short opcode;
    unsigned long operand; 
    unsigned short operand_seg;
    unsigned short r3;
    unsigned char regs[8][10];
  };

struct xmmctx
  {
    unsigned short control;
    unsigned short status;
    unsigned char r0;
    unsigned char tag;        /* abridged */
    unsigned short opcode;
    unsigned long eip;
    unsigned short code_seg;
    unsigned short r1;
    unsigned long operand;
    unsigned short operand_seg;
    unsigned short r2;
    unsigned long mxcsr;
    unsigned long r3;
    struct
      {
        unsigned char fp_bytes[10];
        unsigned char fp_rsvd[6];
      } fpregs[8];
    struct
      {
        unsigned char sse_bytes[16];
      } sseregs[8];
    unsigned char r4[16 * 14];
  };

static void
supply_regs (regs)
     struct reg *regs;
{

  RF ( 0, regs->r_eax);
  RF ( 1, regs->r_ecx);
  RF ( 2, regs->r_edx);
  RF ( 3, regs->r_ebx);
  RF ( 4, regs->r_esp);
  RF ( 5, regs->r_ebp);
  RF ( 6, regs->r_esi);
  RF ( 7, regs->r_edi);
  RF ( 8, regs->r_eip);
  RF ( 9, regs->r_eflags);
  RF (10, regs->r_cs);
  RF (11, regs->r_ss);
  RF (12, regs->r_ds);
  RF (13, regs->r_es);
  RF (14, regs->r_fs);
  RF (15, regs->r_gs);
}

static void
supply_387regs (s87)
     struct env387 *s87;
{
  int i;

  for (i = 0; i < 8; i++)
    {
      RF (FP0_REGNUM + i, s87->regs[i]);
    }

  RF (FCTRL_REGNUM,   s87->control);
  RF (FSTAT_REGNUM,   s87->status);
  RF (FTAG_REGNUM,    s87->tag);
  RF (FCS_REGNUM,     s87->code_seg);
  RF (FCOFF_REGNUM,   s87->eip);
  RF (FDS_REGNUM,     s87->operand_seg);
  RF (FDOFF_REGNUM,   s87->operand);
  RF (FOP_REGNUM,     s87->opcode);
}

#ifdef PT_GETXMMREGS
static void
supply_xmmregs (sxmm)
     struct xmmctx *sxmm;
{
  static unsigned char empty_significand[8] = { 0 };
  unsigned long reg;
  unsigned short exponent;
  int i;

  for (i = 0; i < 8; i++)
    {
      RF (FP0_REGNUM + i,  sxmm->fpregs[i].fp_bytes);
      RF (XMM0_REGNUM + i, sxmm->sseregs[i].sse_bytes);
    }

  /* Note: unlike the 387-format, the XMM-format does not have
     16-bits of padding after the 16-bit registers.  This means
     that we need to pull these registers into a suitably-padded
     storage space before letting supply_register() have its
     way with it.  */

  reg = sxmm->control;
  RF (FCTRL_REGNUM,   reg);

  reg = sxmm->status;
  RF (FSTAT_REGNUM,   reg);

  reg = sxmm->code_seg;
  RF (FCS_REGNUM,     reg);

  RF (FCOFF_REGNUM,   sxmm->eip);

  reg = sxmm->operand_seg;
  RF (FDS_REGNUM,     reg);

  RF (FDOFF_REGNUM,   sxmm->operand);

  RF (FOP_REGNUM, sxmm->opcode);

  /* The kernel has provided us the "tag" info in XMM format, but
     GDB expects it in i387 format; convert it.  */
  for (reg = 0, i = 0; i < 8; i++)
    {
      if (sxmm->tag & (1U << i))
        {
          exponent = sxmm->fpregs[i].fp_bytes[8] |
                     (sxmm->fpregs[i].fp_bytes[9] << 8);
          switch (exponent & 0x7fff)
            {
              case 0x7fff:
                reg |= (2U << (i * 2));
                break;

              case 0x0000:
                if (memcmp(empty_significand, sxmm->fpregs[i].fp_bytes,
                           sizeof (empty_significand)) == 0)
                  {
                    reg |= (1U << (i * 2));
                  }
                else
                  {
                    reg |= (2U << (i * 2));
                  }
                break;
              default:
                if ((sxmm->fpregs[i].fp_bytes[7] & 0x80) == 0)
                  {
                    reg |= (2U << (i * 2));
                  }
                /* else reg |= (0U << (i * 2)); */
                break;
            }
        }
      else
        {
          reg |= (3U << (i * 2));
        }
    }
  RF (FTAG_REGNUM,    reg);

  RF (MXCSR_REGNUM,   sxmm->mxcsr);
}
#endif

void
fetch_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct env387 inferior_fpregisters;
#ifdef PT_GETXMMREGS
  struct xmmctx inferior_xmmregisters;
#endif

  ptrace (PT_GETREGS, inferior_pid,
          (PTRACE_ARG3_TYPE) &inferior_registers, 0);

#ifdef PT_GETXMMREGS
  if (have_ptrace_xmmregs != 0 &&
      ptrace(PT_GETXMMREGS, inferior_pid,
             (PTRACE_ARG3_TYPE) &inferior_xmmregisters, 0) == 0)
    {
      have_ptrace_xmmregs = 1;
    }
  else
    {
      ptrace (PT_GETFPREGS, inferior_pid,
              (PTRACE_ARG3_TYPE) &inferior_fpregisters, 0);
      have_ptrace_xmmregs = 0;
    }
#else
    ptrace (PT_GETFPREGS, inferior_pid,
            (PTRACE_ARG3_TYPE) &inferior_fpregisters, 0);
#endif

  supply_regs (&inferior_registers);

#ifdef PT_GETXMMREGS
  if (have_ptrace_xmmregs)
    {
      supply_xmmregs (&inferior_xmmregisters);
    }
  else
    {
      supply_387regs (&inferior_fpregisters);
    }
#else
  supply_387regs (&inferior_fpregisters);
#endif
}

void
store_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct env387 inferior_fpregisters;
#ifdef PT_GETXMMREGS
  struct xmmctx inferior_xmmregisters;
#endif
  int i;

  RS ( 0, inferior_registers.r_eax);
  RS ( 1, inferior_registers.r_ecx);
  RS ( 2, inferior_registers.r_edx);
  RS ( 3, inferior_registers.r_ebx);
  RS ( 4, inferior_registers.r_esp);
  RS ( 5, inferior_registers.r_ebp);
  RS ( 6, inferior_registers.r_esi);
  RS ( 7, inferior_registers.r_edi);
  RS ( 8, inferior_registers.r_eip);
  RS ( 9, inferior_registers.r_eflags);
  RS (10, inferior_registers.r_cs);
  RS (11, inferior_registers.r_ss);
  RS (12, inferior_registers.r_ds);
  RS (13, inferior_registers.r_es);
  RS (14, inferior_registers.r_fs);
  RS (15, inferior_registers.r_gs);

#ifdef PT_GETXMMREGS
  if (have_ptrace_xmmregs != 0)
    {
      unsigned short tag;

      for (i = 0; i < 8; i++)
        {
          RS (FP0_REGNUM + i,  inferior_xmmregisters.fpregs[i].fp_bytes);
          RS (XMM0_REGNUM + i, inferior_xmmregisters.sseregs[i].sse_bytes);
        }

      /* Note: even though there's no padding after the 16-bit
	 registers, because we copy only as much as the destination
	 will hold *and* we're little-endian, this all works out
	 fine.  */

      RS (FCTRL_REGNUM,   inferior_xmmregisters.control);
      RS (FSTAT_REGNUM,   inferior_xmmregisters.status);
      RS (FCS_REGNUM,     inferior_xmmregisters.code_seg);
      RS (FCOFF_REGNUM,   inferior_xmmregisters.eip);
      RS (FDS_REGNUM,     inferior_xmmregisters.operand_seg);
      RS (FDOFF_REGNUM,   inferior_xmmregisters.operand);

      /* GDB has provided as the "tag" info in i387 format, but the
         kernel expects it to be in XMM format; convert it.  */
      RS (FTAG_REGNUM,    tag);
      for (inferior_xmmregisters.tag = 0, i = 0; i < 8; i++)
        {
          if (((tag >> (i * 2)) & 3) != 3)
            {
              inferior_xmmregisters.tag |= (1U << i);
            }
        }

       RS (MXCSR_REGNUM,   inferior_xmmregisters.mxcsr);
    }
  else
    {
#endif
      for (i = 0; i < 8; i++)
        {
          RF (FP0_REGNUM + i, inferior_fpregisters.regs[i]);
        }

      RS (FCTRL_REGNUM,   inferior_fpregisters.control);
      RS (FSTAT_REGNUM,   inferior_fpregisters.status);
      RS (FTAG_REGNUM,    inferior_fpregisters.tag);
      RS (FCS_REGNUM,     inferior_fpregisters.code_seg);
      RS (FCOFF_REGNUM,   inferior_fpregisters.eip);
      RS (FDS_REGNUM,     inferior_fpregisters.operand_seg);
      RS (FDOFF_REGNUM,   inferior_fpregisters.operand);
      RS (FOP_REGNUM,     inferior_fpregisters.opcode);
#ifdef PT_GETXMMREGS
    }
#endif

  ptrace (PT_SETREGS, inferior_pid,
          (PTRACE_ARG3_TYPE) &inferior_registers, 0);
#ifdef PT_GETXMMREGS
  if (have_ptrace_xmmregs != 0)
    ptrace (PT_SETXMMREGS, inferior_pid,
            (PTRACE_ARG3_TYPE) &inferior_xmmregisters, 0);
  else
#endif
    ptrace (PT_SETFPREGS, inferior_pid,
            (PTRACE_ARG3_TYPE) &inferior_fpregisters, 0);
}

struct md_core
{
  struct reg intreg;
  struct env387 freg;
};

static void
fetch_core_registers (core_reg_sect, core_reg_size, which, ignore)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     CORE_ADDR ignore;
{
  struct md_core *core_reg = (struct md_core *) core_reg_sect;

  /* integer registers */
  supply_regs (&core_reg->intreg);

  /* floating point registers */
  supply_387regs (&core_reg->freg);
}

static void
fetch_elfcore_registers (core_reg_sect, core_reg_size, which, ignore)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     CORE_ADDR ignore;
{
  struct reg intreg;
  struct env387 freg;
  struct xmmctx xmm;

  switch (which)
    {
    case 0:  /* Integer registers */
      if (core_reg_size != sizeof (intreg))
        warning ("Wrong size register set in core file.");
      else
        {
          memcpy (&intreg, core_reg_sect, sizeof (intreg));
          supply_regs (&intreg);
        }
      break;

    case 2:  /* Floating point registers */
      if (core_reg_size != sizeof (freg))
        warning ("Wrong size FP register set in core file.");
      else
        {
          memcpy (&freg, core_reg_sect, sizeof (freg));
          supply_387regs (&freg);
        }
      break;

    case 3:  /* "Extended" floating point registers.  This is gdb-speak
                for SSE/SSE2.  */
      if (core_reg_size != sizeof (xmm))
        warning ("Wrong size XMM register set in core file.");
      else
        {
          memcpy (&xmm, core_reg_sect, sizeof (xmm));
          supply_xmmregs (&xmm);
        }
      break;

    default:
      /* Don't know what kind of register request this is; just ignore it.  */
      break;
    }
}

#ifdef FETCH_KCORE_REGISTERS
/*
 * Get registers from a kernel crash dump or live kernel.
 * Called by kcore-nbsd.c:get_kcore_registers().
 */
void
fetch_kcore_registers (pcb)
     struct pcb *pcb;
{
  int i, regno, regs[4];

  /*
   * get the register values out of the sys pcb and
   * store them where `read_register' will find them.
   */
  if (target_read_memory(pcb->pcb_tss.tss_esp+4,
                         (char *)regs, sizeof(regs)))
    error("Cannot read ebx, esi, and edi.");
  for (i = 0, regno = 0; regno < 3; regno++)
    supply_register(regno, (char *)&i);
  supply_register(3, (char *)&regs[2]);
  supply_register(4, (char *)&pcb->pcb_tss.tss_esp);
  supply_register(5, (char *)&pcb->pcb_tss.tss_ebp);
  supply_register(6, (char *)&regs[1]);
  supply_register(7, (char *)&regs[0]);
  supply_register(8, (char *)&regs[3]);
  for (i = 0, regno = 9; regno < 10; regno++)
    supply_register(regno, (char *)&i);
#if 0
  i = 0x08;
  supply_register(10, (char *)&i);
  i = 0x10;
  supply_register(11, (char *)&i);
#endif
}
#endif /* FETCH_KCORE_REGISTERS */

/* Register that we are able to handle i386nbsd core file formats.
   FIXME: is this really bfd_target_unknown_flavour? */

static struct core_fns i386nbsd_core_fns =
{
  bfd_target_unknown_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL					/* next */
};

static struct core_fns i386nbsd_elfcore_fns =
{
  bfd_target_elf_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_elfcore_registers,		/* core_read_registers */
  NULL					/* next */
};

void
_initialize_i386nbsd_nat ()
{
  add_core_fns (&i386nbsd_core_fns);
  add_core_fns (&i386nbsd_elfcore_fns);
}
