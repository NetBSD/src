/* i387-specific utility functions, for the remote server for GDB.
   Copyright (C) 2000-2014 Free Software Foundation, Inc.

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

#include "server.h"
#include "i387-fp.h"
#include "i386-xstate.h"

static const int num_mpx_bnd_registers = 4;
static const int num_mpx_cfg_registers = 2;

/* Note: These functions preserve the reserved bits in control registers.
   However, gdbserver promptly throws away that information.  */

/* These structs should have the proper sizes and alignment on both
   i386 and x86-64 machines.  */

struct i387_fsave {
  /* All these are only sixteen bits, plus padding, except for fop (which
     is only eleven bits), and fooff / fioff (which are 32 bits each).  */
  unsigned short fctrl;
  unsigned short pad1;
  unsigned short fstat;
  unsigned short pad2;
  unsigned short ftag;
  unsigned short pad3;
  unsigned int fioff;
  unsigned short fiseg;
  unsigned short fop;
  unsigned int fooff;
  unsigned short foseg;
  unsigned short pad4;

  /* Space for eight 80-bit FP values.  */
  unsigned char st_space[80];
};

struct i387_fxsave {
  /* All these are only sixteen bits, plus padding, except for fop (which
     is only eleven bits), and fooff / fioff (which are 32 bits each).  */
  unsigned short fctrl;
  unsigned short fstat;
  unsigned short ftag;
  unsigned short fop;
  unsigned int fioff;
  unsigned short fiseg;
  unsigned short pad1;
  unsigned int fooff;
  unsigned short foseg;
  unsigned short pad12;

  unsigned int mxcsr;
  unsigned int pad3;

  /* Space for eight 80-bit FP values in 128-bit spaces.  */
  unsigned char st_space[128];

  /* Space for eight 128-bit XMM values, or 16 on x86-64.  */
  unsigned char xmm_space[256];
};

struct i387_xsave {
  /* All these are only sixteen bits, plus padding, except for fop (which
     is only eleven bits), and fooff / fioff (which are 32 bits each).  */
  unsigned short fctrl;
  unsigned short fstat;
  unsigned short ftag;
  unsigned short fop;
  unsigned int fioff;
  unsigned short fiseg;
  unsigned short pad1;
  unsigned int fooff;
  unsigned short foseg;
  unsigned short pad12;

  unsigned int mxcsr;
  unsigned int mxcsr_mask;

  /* Space for eight 80-bit FP values in 128-bit spaces.  */
  unsigned char st_space[128];

  /* Space for eight 128-bit XMM values, or 16 on x86-64.  */
  unsigned char xmm_space[256];

  unsigned char reserved1[48];

  /* The extended control register 0 (the XFEATURE_ENABLED_MASK
     register).  */
  unsigned long long xcr0;

  unsigned char reserved2[40];

  /* The XSTATE_BV bit vector.  */
  unsigned long long xstate_bv;

  unsigned char reserved3[56];

  /* Space for eight upper 128-bit YMM values, or 16 on x86-64.  */
  unsigned char ymmh_space[256];

  unsigned char reserved4[128];

  /* Space for 4 bound registers values of 128 bits.  */
  unsigned char mpx_bnd_space[64];

  /* Space for 2 MPX configuration registers of 64 bits
     plus reserved space.  */
  unsigned char mpx_cfg_space[16];
};

void
i387_cache_to_fsave (struct regcache *regcache, void *buf)
{
  struct i387_fsave *fp = (struct i387_fsave *) buf;
  int i;
  int st0_regnum = find_regno (regcache->tdesc, "st0");
  unsigned long val, val2;

  for (i = 0; i < 8; i++)
    collect_register (regcache, i + st0_regnum,
		      ((char *) &fp->st_space[0]) + i * 10);

  collect_register_by_name (regcache, "fioff", &fp->fioff);
  collect_register_by_name (regcache, "fooff", &fp->fooff);
  
  /* This one's 11 bits... */
  collect_register_by_name (regcache, "fop", &val2);
  fp->fop = (val2 & 0x7FF) | (fp->fop & 0xF800);

  /* Some registers are 16-bit.  */
  collect_register_by_name (regcache, "fctrl", &val);
  fp->fctrl = val;

  collect_register_by_name (regcache, "fstat", &val);
  val &= 0xFFFF;
  fp->fstat = val;

  collect_register_by_name (regcache, "ftag", &val);
  val &= 0xFFFF;
  fp->ftag = val;

  collect_register_by_name (regcache, "fiseg", &val);
  val &= 0xFFFF;
  fp->fiseg = val;

  collect_register_by_name (regcache, "foseg", &val);
  val &= 0xFFFF;
  fp->foseg = val;
}

void
i387_fsave_to_cache (struct regcache *regcache, const void *buf)
{
  struct i387_fsave *fp = (struct i387_fsave *) buf;
  int i;
  int st0_regnum = find_regno (regcache->tdesc, "st0");
  unsigned long val;

  for (i = 0; i < 8; i++)
    supply_register (regcache, i + st0_regnum,
		     ((char *) &fp->st_space[0]) + i * 10);

  supply_register_by_name (regcache, "fioff", &fp->fioff);
  supply_register_by_name (regcache, "fooff", &fp->fooff);

  /* Some registers are 16-bit.  */
  val = fp->fctrl & 0xFFFF;
  supply_register_by_name (regcache, "fctrl", &val);

  val = fp->fstat & 0xFFFF;
  supply_register_by_name (regcache, "fstat", &val);

  val = fp->ftag & 0xFFFF;
  supply_register_by_name (regcache, "ftag", &val);

  val = fp->fiseg & 0xFFFF;
  supply_register_by_name (regcache, "fiseg", &val);

  val = fp->foseg & 0xFFFF;
  supply_register_by_name (regcache, "foseg", &val);

  /* fop has only 11 valid bits.  */
  val = (fp->fop) & 0x7FF;
  supply_register_by_name (regcache, "fop", &val);
}

void
i387_cache_to_fxsave (struct regcache *regcache, void *buf)
{
  struct i387_fxsave *fp = (struct i387_fxsave *) buf;
  int i;
  int st0_regnum = find_regno (regcache->tdesc, "st0");
  int xmm0_regnum = find_regno (regcache->tdesc, "xmm0");
  unsigned long val, val2;
  /* Amd64 has 16 xmm regs; I386 has 8 xmm regs.  */
  int num_xmm_registers = register_size (regcache->tdesc, 0) == 8 ? 16 : 8;

  for (i = 0; i < 8; i++)
    collect_register (regcache, i + st0_regnum,
		      ((char *) &fp->st_space[0]) + i * 16);
  for (i = 0; i < num_xmm_registers; i++)
    collect_register (regcache, i + xmm0_regnum,
		      ((char *) &fp->xmm_space[0]) + i * 16);

  collect_register_by_name (regcache, "fioff", &fp->fioff);
  collect_register_by_name (regcache, "fooff", &fp->fooff);
  collect_register_by_name (regcache, "mxcsr", &fp->mxcsr);

  /* This one's 11 bits... */
  collect_register_by_name (regcache, "fop", &val2);
  fp->fop = (val2 & 0x7FF) | (fp->fop & 0xF800);

  /* Some registers are 16-bit.  */
  collect_register_by_name (regcache, "fctrl", &val);
  fp->fctrl = val;

  collect_register_by_name (regcache, "fstat", &val);
  fp->fstat = val;

  /* Convert to the simplifed tag form stored in fxsave data.  */
  collect_register_by_name (regcache, "ftag", &val);
  val &= 0xFFFF;
  val2 = 0;
  for (i = 7; i >= 0; i--)
    {
      int tag = (val >> (i * 2)) & 3;

      if (tag != 3)
	val2 |= (1 << i);
    }
  fp->ftag = val2;

  collect_register_by_name (regcache, "fiseg", &val);
  fp->fiseg = val;

  collect_register_by_name (regcache, "foseg", &val);
  fp->foseg = val;
}

void
i387_cache_to_xsave (struct regcache *regcache, void *buf)
{
  struct i387_xsave *fp = (struct i387_xsave *) buf;
  int i;
  unsigned long val, val2;
  unsigned int clear_bv;
  unsigned long long xstate_bv = 0;
  char raw[16];
  char *p;
  /* Amd64 has 16 xmm regs; I386 has 8 xmm regs.  */
  int num_xmm_registers = register_size (regcache->tdesc, 0) == 8 ? 16 : 8;

  /* The supported bits in `xstat_bv' are 1 byte.  Clear part in
     vector registers if its bit in xstat_bv is zero.  */
  clear_bv = (~fp->xstate_bv) & x86_xcr0;

  /* Clear part in x87 and vector registers if its bit in xstat_bv is
     zero.  */
  if (clear_bv)
    {
      if ((clear_bv & I386_XSTATE_X87))
	for (i = 0; i < 8; i++)
	  memset (((char *) &fp->st_space[0]) + i * 16, 0, 10);

      if ((clear_bv & I386_XSTATE_SSE))
	for (i = 0; i < num_xmm_registers; i++) 
	  memset (((char *) &fp->xmm_space[0]) + i * 16, 0, 16);

      if ((clear_bv & I386_XSTATE_AVX))
	for (i = 0; i < num_xmm_registers; i++) 
	  memset (((char *) &fp->ymmh_space[0]) + i * 16, 0, 16);

      if ((clear_bv & I386_XSTATE_BNDREGS))
	for (i = 0; i < num_mpx_bnd_registers; i++)
	  memset (((char *) &fp->mpx_bnd_space[0]) + i * 16, 0, 16);

      if ((clear_bv & I386_XSTATE_BNDCFG))
	for (i = 0; i < num_mpx_cfg_registers; i++)
	  memset (((char *) &fp->mpx_cfg_space[0]) + i * 8, 0, 8);
    }

  /* Check if any x87 registers are changed.  */
  if ((x86_xcr0 & I386_XSTATE_X87))
    {
      int st0_regnum = find_regno (regcache->tdesc, "st0");

      for (i = 0; i < 8; i++)
	{
	  collect_register (regcache, i + st0_regnum, raw);
	  p = ((char *) &fp->st_space[0]) + i * 16;
	  if (memcmp (raw, p, 10))
	    {
	      xstate_bv |= I386_XSTATE_X87;
	      memcpy (p, raw, 10);
	    }
	}
    }

  /* Check if any SSE registers are changed.  */
  if ((x86_xcr0 & I386_XSTATE_SSE))
    {
      int xmm0_regnum = find_regno (regcache->tdesc, "xmm0");

      for (i = 0; i < num_xmm_registers; i++) 
	{
	  collect_register (regcache, i + xmm0_regnum, raw);
	  p = ((char *) &fp->xmm_space[0]) + i * 16;
	  if (memcmp (raw, p, 16))
	    {
	      xstate_bv |= I386_XSTATE_SSE;
	      memcpy (p, raw, 16);
	    }
	}
    }

  /* Check if any AVX registers are changed.  */
  if ((x86_xcr0 & I386_XSTATE_AVX))
    {
      int ymm0h_regnum = find_regno (regcache->tdesc, "ymm0h");

      for (i = 0; i < num_xmm_registers; i++) 
	{
	  collect_register (regcache, i + ymm0h_regnum, raw);
	  p = ((char *) &fp->ymmh_space[0]) + i * 16;
	  if (memcmp (raw, p, 16))
	    {
	      xstate_bv |= I386_XSTATE_AVX;
	      memcpy (p, raw, 16);
	    }
	}
    }

  /* Check if any bound register has changed.  */
  if ((x86_xcr0 & I386_XSTATE_BNDREGS))
    {
     int bnd0r_regnum = find_regno (regcache->tdesc, "bnd0raw");

      for (i = 0; i < num_mpx_bnd_registers; i++)
	{
	  collect_register (regcache, i + bnd0r_regnum, raw);
	  p = ((char *) &fp->mpx_bnd_space[0]) + i * 16;
	  if (memcmp (raw, p, 16))
	    {
	      xstate_bv |= I386_XSTATE_BNDREGS;
	      memcpy (p, raw, 16);
	    }
	}
    }

  /* Check if any status register has changed.  */
  if ((x86_xcr0 & I386_XSTATE_BNDCFG))
    {
      int bndcfg_regnum = find_regno (regcache->tdesc, "bndcfgu");

      for (i = 0; i < num_mpx_cfg_registers; i++)
	{
	  collect_register (regcache, i + bndcfg_regnum, raw);
	  p = ((char *) &fp->mpx_cfg_space[0]) + i * 8;
	  if (memcmp (raw, p, 8))
	    {
	      xstate_bv |= I386_XSTATE_BNDCFG;
	      memcpy (p, raw, 8);
	    }
	}
    }

  /* Update the corresponding bits in xstate_bv if any SSE/AVX
     registers are changed.  */
  fp->xstate_bv |= xstate_bv;

  collect_register_by_name (regcache, "fioff", &fp->fioff);
  collect_register_by_name (regcache, "fooff", &fp->fooff);
  collect_register_by_name (regcache, "mxcsr", &fp->mxcsr);

  /* This one's 11 bits... */
  collect_register_by_name (regcache, "fop", &val2);
  fp->fop = (val2 & 0x7FF) | (fp->fop & 0xF800);

  /* Some registers are 16-bit.  */
  collect_register_by_name (regcache, "fctrl", &val);
  fp->fctrl = val;

  collect_register_by_name (regcache, "fstat", &val);
  fp->fstat = val;

  /* Convert to the simplifed tag form stored in fxsave data.  */
  collect_register_by_name (regcache, "ftag", &val);
  val &= 0xFFFF;
  val2 = 0;
  for (i = 7; i >= 0; i--)
    {
      int tag = (val >> (i * 2)) & 3;

      if (tag != 3)
	val2 |= (1 << i);
    }
  fp->ftag = val2;

  collect_register_by_name (regcache, "fiseg", &val);
  fp->fiseg = val;

  collect_register_by_name (regcache, "foseg", &val);
  fp->foseg = val;
}

static int
i387_ftag (struct i387_fxsave *fp, int regno)
{
  unsigned char *raw = &fp->st_space[regno * 16];
  unsigned int exponent;
  unsigned long fraction[2];
  int integer;

  integer = raw[7] & 0x80;
  exponent = (((raw[9] & 0x7f) << 8) | raw[8]);
  fraction[0] = ((raw[3] << 24) | (raw[2] << 16) | (raw[1] << 8) | raw[0]);
  fraction[1] = (((raw[7] & 0x7f) << 24) | (raw[6] << 16)
		 | (raw[5] << 8) | raw[4]);

  if (exponent == 0x7fff)
    {
      /* Special.  */
      return (2);
    }
  else if (exponent == 0x0000)
    {
      if (fraction[0] == 0x0000 && fraction[1] == 0x0000 && !integer)
	{
	  /* Zero.  */
	  return (1);
	}
      else
	{
	  /* Special.  */
	  return (2);
	}
    }
  else
    {
      if (integer)
	{
	  /* Valid.  */
	  return (0);
	}
      else
	{
	  /* Special.  */
	  return (2);
	}
    }
}

void
i387_fxsave_to_cache (struct regcache *regcache, const void *buf)
{
  struct i387_fxsave *fp = (struct i387_fxsave *) buf;
  int i, top;
  int st0_regnum = find_regno (regcache->tdesc, "st0");
  int xmm0_regnum = find_regno (regcache->tdesc, "xmm0");
  unsigned long val;
  /* Amd64 has 16 xmm regs; I386 has 8 xmm regs.  */
  int num_xmm_registers = register_size (regcache->tdesc, 0) == 8 ? 16 : 8;

  for (i = 0; i < 8; i++)
    supply_register (regcache, i + st0_regnum,
		     ((char *) &fp->st_space[0]) + i * 16);
  for (i = 0; i < num_xmm_registers; i++)
    supply_register (regcache, i + xmm0_regnum,
		     ((char *) &fp->xmm_space[0]) + i * 16);

  supply_register_by_name (regcache, "fioff", &fp->fioff);
  supply_register_by_name (regcache, "fooff", &fp->fooff);
  supply_register_by_name (regcache, "mxcsr", &fp->mxcsr);

  /* Some registers are 16-bit.  */
  val = fp->fctrl & 0xFFFF;
  supply_register_by_name (regcache, "fctrl", &val);

  val = fp->fstat & 0xFFFF;
  supply_register_by_name (regcache, "fstat", &val);

  /* Generate the form of ftag data that GDB expects.  */
  top = (fp->fstat >> 11) & 0x7;
  val = 0;
  for (i = 7; i >= 0; i--)
    {
      int tag;
      if (fp->ftag & (1 << i))
	tag = i387_ftag (fp, (i + 8 - top) % 8);
      else
	tag = 3;
      val |= tag << (2 * i);
    }
  supply_register_by_name (regcache, "ftag", &val);

  val = fp->fiseg & 0xFFFF;
  supply_register_by_name (regcache, "fiseg", &val);

  val = fp->foseg & 0xFFFF;
  supply_register_by_name (regcache, "foseg", &val);

  val = (fp->fop) & 0x7FF;
  supply_register_by_name (regcache, "fop", &val);
}

void
i387_xsave_to_cache (struct regcache *regcache, const void *buf)
{
  struct i387_xsave *fp = (struct i387_xsave *) buf;
  struct i387_fxsave *fxp = (struct i387_fxsave *) buf;
  int i, top;
  unsigned long val;
  unsigned int clear_bv;
  gdb_byte *p;
  /* Amd64 has 16 xmm regs; I386 has 8 xmm regs.  */
  int num_xmm_registers = register_size (regcache->tdesc, 0) == 8 ? 16 : 8;

  /* The supported bits in `xstat_bv' are 1 byte.  Clear part in
     vector registers if its bit in xstat_bv is zero.  */
  clear_bv = (~fp->xstate_bv) & x86_xcr0;

  /* Check if any x87 registers are changed.  */
  if ((x86_xcr0 & I386_XSTATE_X87) != 0)
    {
      int st0_regnum = find_regno (regcache->tdesc, "st0");

      if ((clear_bv & I386_XSTATE_X87) != 0)
	{
	  for (i = 0; i < 8; i++)
	    supply_register_zeroed (regcache, i + st0_regnum);
	}
      else
	{
	  p = (gdb_byte *) &fp->st_space[0];
	  for (i = 0; i < 8; i++)
	    supply_register (regcache, i + st0_regnum, p + i * 16);
	}
    }

  if ((x86_xcr0 & I386_XSTATE_SSE) != 0)
    {
      int xmm0_regnum = find_regno (regcache->tdesc, "xmm0");

      if ((clear_bv & I386_XSTATE_SSE))
	{
	  for (i = 0; i < num_xmm_registers; i++)
	    supply_register_zeroed (regcache, i + xmm0_regnum);
	}
      else
	{
	  p = (gdb_byte *) &fp->xmm_space[0];
	  for (i = 0; i < num_xmm_registers; i++)
	    supply_register (regcache, i + xmm0_regnum, p + i * 16);
	}
    }

  if ((x86_xcr0 & I386_XSTATE_AVX) != 0)
    {
      int ymm0h_regnum = find_regno (regcache->tdesc, "ymm0h");

      if ((clear_bv & I386_XSTATE_AVX) != 0)
	{
	  for (i = 0; i < num_xmm_registers; i++)
	    supply_register_zeroed (regcache, i + ymm0h_regnum);
	}
      else
	{
	  p = (gdb_byte *) &fp->ymmh_space[0];
	  for (i = 0; i < num_xmm_registers; i++)
	    supply_register (regcache, i + ymm0h_regnum, p + i * 16);
	}
    }

  if ((x86_xcr0 & I386_XSTATE_BNDREGS))
    {
      int bnd0r_regnum = find_regno (regcache->tdesc, "bnd0raw");


      if ((clear_bv & I386_XSTATE_BNDREGS) != 0)
	{
	  for (i = 0; i < num_mpx_bnd_registers; i++)
	    supply_register_zeroed (regcache, i + bnd0r_regnum);
	}
      else
	{
	  p = (gdb_byte *) &fp->mpx_bnd_space[0];
	  for (i = 0; i < num_mpx_bnd_registers; i++)
	    supply_register (regcache, i + bnd0r_regnum, p + i * 16);
	}

    }

  if ((x86_xcr0 & I386_XSTATE_BNDCFG))
    {
      int bndcfg_regnum = find_regno (regcache->tdesc, "bndcfgu");

      if ((clear_bv & I386_XSTATE_BNDCFG) != 0)
	{
	  for (i = 0; i < num_mpx_cfg_registers; i++)
	    supply_register_zeroed (regcache, i + bndcfg_regnum);
	}
      else
	{
	  p = (gdb_byte *) &fp->mpx_cfg_space[0];
	  for (i = 0; i < num_mpx_cfg_registers; i++)
	    supply_register (regcache, i + bndcfg_regnum, p + i * 8);
	}
    }

  supply_register_by_name (regcache, "fioff", &fp->fioff);
  supply_register_by_name (regcache, "fooff", &fp->fooff);
  supply_register_by_name (regcache, "mxcsr", &fp->mxcsr);

  /* Some registers are 16-bit.  */
  val = fp->fctrl & 0xFFFF;
  supply_register_by_name (regcache, "fctrl", &val);

  val = fp->fstat & 0xFFFF;
  supply_register_by_name (regcache, "fstat", &val);

  /* Generate the form of ftag data that GDB expects.  */
  top = (fp->fstat >> 11) & 0x7;
  val = 0;
  for (i = 7; i >= 0; i--)
    {
      int tag;
      if (fp->ftag & (1 << i))
	tag = i387_ftag (fxp, (i + 8 - top) % 8);
      else
	tag = 3;
      val |= tag << (2 * i);
    }
  supply_register_by_name (regcache, "ftag", &val);

  val = fp->fiseg & 0xFFFF;
  supply_register_by_name (regcache, "fiseg", &val);

  val = fp->foseg & 0xFFFF;
  supply_register_by_name (regcache, "foseg", &val);

  val = (fp->fop) & 0x7FF;
  supply_register_by_name (regcache, "fop", &val);
}

/* Default to SSE.  */
unsigned long long x86_xcr0 = I386_XSTATE_SSE_MASK;
