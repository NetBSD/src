/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
/*#include <sys/seg.h>*/
#include <sys/acct.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/syslog.h>
#include <sys/user.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#define PRINTF(x)

int fancy (unsigned short buf[])
{
  int is, iis, wl, bdsize;
  int advance;

  if ((buf[2] & 0x0100) == 0) {  /* Brief format extention word */
    advance = 2;
  } else {  /* Full format extention word(s) */
    advance = 2;
    is = (buf[2] >> 6) & 0x1;
    iis = buf[2] & 0x7;
    switch (iis & 0x3) { /* Advance for outer displacement */
      case 0: advance += 0; break;
      case 1: advance += 0; break;
      case 2: advance += 2; break;
      case 3: advance += 4; break;
    }
    bdsize = (buf[2] >> 4) & 0x3;
    switch (bdsize) { /* Advance for base displacement */
      case 0: advance += 0; break;
      case 1: advance += 0; break;
      case 2: advance += 2; break;
      case 3: advance += 4; break;
    }
  }

  return advance;
}

int opsize (int rm, int mode, int reg, int src, unsigned short buf[])
{
  int len;

  if (rm == 0) {
    /* Register-to-register */
    return 0;
  }

  len = 0;
  switch (src) {
    case 0: len = 4; break;  /* Int */
    case 1: len = 4; break;  /* Single */
    case 2: len = 10; break; /* Extended */
    case 3: len = 12; break; /* Packed BCD */
    case 4: len = 2; break;  /* Word */
    case 5: len = 8; break;  /* Double */
    case 6: len = 2; break;  /* Byte */
    case 7: len = 0; break;  /* Unknown */
  }

  switch (mode) {
    case 0: return 0;
    case 1: return 0;
    case 2: return 0;
    case 3: return 0;
    case 4: return 0;
    case 5: return 2;  /* 16-bit indirect */
    case 6:  /* 1, 2, 3, 4, or 5 */
      return fancy (buf);
    case 7:  /* 1, 2, 3, 4, or 5 */
      switch (reg) {
        case 0: return 2;  /* Word address */
        case 1: return 4;  /* Long address */
        case 2: return 2;  /* 16-bit PC offset */
        case 3: return fancy (buf); /* Fancy shmancy addressing */
        case 4: return len; /* Immediate */
      }
      printf ("Invalid register value: %d\n", reg);
      return 0;
  }

  printf ("Invalid mode: %d\n", mode);
  return 0;
}

FPUemul(frame)
	struct frame frame;
{
  unsigned short buf[16];
  int advance, mode, reg, source, dest, rm, coid, type, inst, id, misc;

  advance = 0;

  PRINTF ( ("FPUemul() - "));

  copyin ((char *)frame.f_pc, buf, sizeof (buf));

  PRINTF ( ("Word 0: %x, ", (int)buf[0]));
  PRINTF ( ("Word 1: %x, ", (int)buf[1]));

  /* Check co-processor ID.  FPU is 1.  (MMU is 0.) */
  id = (int)((buf[0] >> 9) & 0x7);
  if (id != 1) {
    printf ("Unsupported co-processor (%d).\n", id);
#ifdef DDB
    Debugger();
#endif
    return;
  }

  type = (buf[0] >> 6) & 0x7;
  mode = (buf[0] >> 3) & 0x7;
  reg = buf[0] & 0x7;
  source = (buf[1] >> 10) & 0x7;
  dest = (buf[1] >> 7) & 0x7;
  rm = (buf[1] >> 14) & 0x1;
  misc = (buf[1] >> 13) & 0x1;
  inst = buf[1] & 0x7F;

  switch (type) {
    case 0:
      if ((buf[1] & 0x8000) == 0) {
        if (mode == 0 && reg == 0 && source == 7 && rm == 1 && misc == 0) {
          PRINTF (("FMOVECR (constant %d)\n", inst));
          advance = 4;
        } else if (rm == 1 && misc == 1) {
          PRINTF (("FMOVE from FPn\n"));
          advance = 4 + opsize (rm, mode, reg, source, buf);
        } else {
          /* Most instructions fall in this category... */
          switch (inst) {
            case 0: PRINTF ( ("FMOVE to FPn\n")); break;
            case 1: PRINTF ( ("FINT\n")); break;
            case 2: PRINTF ( ("FSINH\n")); break;
            case 3: PRINTF ( ("FINTRZ\n")); break;
            case 4: PRINTF ( ("FSQRT\n")); break;
            case 6: PRINTF ( ("FLOGNP1\n")); break;
            case 8: PRINTF ( ("FETOXM1\n")); break;
            case 9: PRINTF ( ("FTANH\n")); break;
            case 10: PRINTF ( ("FATAN\n")); break;
            case 12: PRINTF ( ("FASIN\n")); break;
            case 13: PRINTF ( ("FATANH\n")); break;
            case 14: PRINTF ( ("FSIN\n")); break;
            case 15: PRINTF ( ("FTAN\n")); break;
            case 16: PRINTF ( ("FETOX\n")); break;
            case 17: PRINTF ( ("FTWOTOX\n")); break;
            case 18: PRINTF ( ("FTENTOX\n")); break;
            case 20: PRINTF ( ("FLOGN\n")); break;
            case 21: PRINTF ( ("FLOG10\n")); break;
            case 22: PRINTF ( ("FLOG2\n")); break;
            case 24: PRINTF ( ("FABS\n")); break;
            case 25: PRINTF ( ("FCOSH\n")); break;
            case 26: PRINTF ( ("FNEG\n")); break;
            case 28: PRINTF ( ("FACOS\n")); break;
            case 29: PRINTF ( ("FCOS\n")); break;
            case 30: PRINTF ( ("FGETEXP\n")); break;
            case 31: PRINTF ( ("FGETMAN\n")); break;
            case 32: PRINTF ( ("FDIV\n")); break;
            case 33: PRINTF ( ("FMOD\n")); break;
            case 34: PRINTF ( ("FADD\n")); break;
            case 35: PRINTF ( ("FMUL\n")); break;
            case 36: PRINTF ( ("FSGLDIV\n")); break;
            case 37: PRINTF ( ("FREM\n")); break;
            case 38: PRINTF ( ("FSCALE\n")); break;
            case 39: PRINTF ( ("FSGLMUL\n")); break;
            case 40: PRINTF ( ("FSUB\n")); break;
            case 48:
            case 49:
            case 50:
            case 51:
            case 52:
            case 53:
            case 54:
            case 55: PRINTF ( ("FSINCOS\n")); break;
            case 56: PRINTF ( ("FCMP\n")); break;
            case 58: PRINTF ( ("FTST\n")); break;
            default: PRINTF ( ("Unknown instruction %d.\n", inst)); break;
          }
          advance = 4 + opsize (rm, mode, reg, source, buf);
        }
      } else {
        if (rm) {
          PRINTF ( ("FMOVE[M] FPn\n"));
          advance = 4 + opsize (rm, mode, reg, source, buf);
        } else {
          PRINTF ( ("FMOVE[M] FPcm\n"));
          advance = 4 + opsize (rm, mode, reg, source, buf);
        }
      }
      break;
    case 1:
      if (mode == 1) {
        PRINTF ( ("FDBcc\n"));
        advance = 6;
      } else {
        if (mode == 7 && reg > 1) {
          PRINTF ( ("FTRAPcc\n"));
          switch (reg) {
            case 2: advance = 6; break;
            case 3: advance = 8; break;
            case 4: advance = 4; break;
            default: printf ("Unknown reg: %d\n", reg); break;
          }
        } else {
          PRINTF ( ("FScc\n"));
          advance = 4 + opsize (rm, mode, reg, source, buf);
        }
      }
      break;
    case 2:
    case 3:
      if (mode == 0 && reg == 0 && source == 0 && dest == 0) {
        PRINTF ( ("FNOP\n"));
        advance = 4;
      } else {
        PRINTF ( ("FBcc\n"));
        if (type == 2) {
          advance = 4;
        } else {
          advance = 6;
        }
      }
      break;
    case 4:
      PRINTF ( ("FSAVE\n"));
      advance = 2 + opsize (rm, mode, reg, source, buf);
      break;
    case 5:
      PRINTF ( ("FRESTORE\n"));
      advance = 2 + opsize (rm, mode, reg, source, buf);
      break;
    case 6:
    case 7:
      printf ("Unknown type: %d\n", type);
      advance = 2;
      break;
  }

  PRINTF ( ("Exiting FPUemul() (%d)\n", advance));
  frame.f_pc += advance;
}
