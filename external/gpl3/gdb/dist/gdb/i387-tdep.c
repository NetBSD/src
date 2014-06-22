/* Intel 387 floating point stuff.

   Copyright (C) 1988-2014 Free Software Foundation, Inc.

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
#include "doublest.h"
#include "floatformat.h"
#include "frame.h"
#include "gdbcore.h"
#include "inferior.h"
#include "language.h"
#include "regcache.h"
#include "value.h"

#include "gdb_assert.h"
#include <string.h>

#include "i386-tdep.h"
#include "i387-tdep.h"
#include "i386-xstate.h"

/* Print the floating point number specified by RAW.  */

static void
print_i387_value (struct gdbarch *gdbarch,
		  const gdb_byte *raw, struct ui_file *file)
{
  DOUBLEST value;

  /* Using extract_typed_floating here might affect the representation
     of certain numbers such as NaNs, even if GDB is running natively.
     This is fine since our caller already detects such special
     numbers and we print the hexadecimal representation anyway.  */
  value = extract_typed_floating (raw, i387_ext_type (gdbarch));

  /* We try to print 19 digits.  The last digit may or may not contain
     garbage, but we'd better print one too many.  We need enough room
     to print the value, 1 position for the sign, 1 for the decimal
     point, 19 for the digits and 6 for the exponent adds up to 27.  */
#ifdef PRINTF_HAS_LONG_DOUBLE
  fprintf_filtered (file, " %-+27.19Lg", (long double) value);
#else
  fprintf_filtered (file, " %-+27.19g", (double) value);
#endif
}

/* Print the classification for the register contents RAW.  */

static void
print_i387_ext (struct gdbarch *gdbarch,
		const gdb_byte *raw, struct ui_file *file)
{
  int sign;
  int integer;
  unsigned int exponent;
  unsigned long fraction[2];

  sign = raw[9] & 0x80;
  integer = raw[7] & 0x80;
  exponent = (((raw[9] & 0x7f) << 8) | raw[8]);
  fraction[0] = ((raw[3] << 24) | (raw[2] << 16) | (raw[1] << 8) | raw[0]);
  fraction[1] = (((raw[7] & 0x7f) << 24) | (raw[6] << 16)
		 | (raw[5] << 8) | raw[4]);

  if (exponent == 0x7fff && integer)
    {
      if (fraction[0] == 0x00000000 && fraction[1] == 0x00000000)
	/* Infinity.  */
	fprintf_filtered (file, " %cInf", (sign ? '-' : '+'));
      else if (sign && fraction[0] == 0x00000000 && fraction[1] == 0x40000000)
	/* Real Indefinite (QNaN).  */
	fputs_unfiltered (" Real Indefinite (QNaN)", file);
      else if (fraction[1] & 0x40000000)
	/* QNaN.  */
	fputs_filtered (" QNaN", file);
      else
	/* SNaN.  */
	fputs_filtered (" SNaN", file);
    }
  else if (exponent < 0x7fff && exponent > 0x0000 && integer)
    /* Normal.  */
    print_i387_value (gdbarch, raw, file);
  else if (exponent == 0x0000)
    {
      /* Denormal or zero.  */
      print_i387_value (gdbarch, raw, file);
      
      if (integer)
	/* Pseudo-denormal.  */
	fputs_filtered (" Pseudo-denormal", file);
      else if (fraction[0] || fraction[1])
	/* Denormal.  */
	fputs_filtered (" Denormal", file);
    }
  else
    /* Unsupported.  */
    fputs_filtered (" Unsupported", file);
}

/* Print the status word STATUS.  If STATUS_P is false, then STATUS
   was unavailable.  */

static void
print_i387_status_word (int status_p,
			unsigned int status, struct ui_file *file)
{
  fprintf_filtered (file, "Status Word:         ");
  if (!status_p)
    {
      fprintf_filtered (file, "%s\n", _("<unavailable>"));
      return;
    }

  fprintf_filtered (file, "%s", hex_string_custom (status, 4));
  fputs_filtered ("  ", file);
  fprintf_filtered (file, " %s", (status & 0x0001) ? "IE" : "  ");
  fprintf_filtered (file, " %s", (status & 0x0002) ? "DE" : "  ");
  fprintf_filtered (file, " %s", (status & 0x0004) ? "ZE" : "  ");
  fprintf_filtered (file, " %s", (status & 0x0008) ? "OE" : "  ");
  fprintf_filtered (file, " %s", (status & 0x0010) ? "UE" : "  ");
  fprintf_filtered (file, " %s", (status & 0x0020) ? "PE" : "  ");
  fputs_filtered ("  ", file);
  fprintf_filtered (file, " %s", (status & 0x0080) ? "ES" : "  ");
  fputs_filtered ("  ", file);
  fprintf_filtered (file, " %s", (status & 0x0040) ? "SF" : "  ");
  fputs_filtered ("  ", file);
  fprintf_filtered (file, " %s", (status & 0x0100) ? "C0" : "  ");
  fprintf_filtered (file, " %s", (status & 0x0200) ? "C1" : "  ");
  fprintf_filtered (file, " %s", (status & 0x0400) ? "C2" : "  ");
  fprintf_filtered (file, " %s", (status & 0x4000) ? "C3" : "  ");

  fputs_filtered ("\n", file);

  fprintf_filtered (file,
		    "                       TOP: %d\n", ((status >> 11) & 7));
}

/* Print the control word CONTROL.  If CONTROL_P is false, then
   CONTROL was unavailable.  */

static void
print_i387_control_word (int control_p,
			 unsigned int control, struct ui_file *file)
{
  fprintf_filtered (file, "Control Word:        ");
  if (!control_p)
    {
      fprintf_filtered (file, "%s\n", _("<unavailable>"));
      return;
    }

  fprintf_filtered (file, "%s", hex_string_custom (control, 4));
  fputs_filtered ("  ", file);
  fprintf_filtered (file, " %s", (control & 0x0001) ? "IM" : "  ");
  fprintf_filtered (file, " %s", (control & 0x0002) ? "DM" : "  ");
  fprintf_filtered (file, " %s", (control & 0x0004) ? "ZM" : "  ");
  fprintf_filtered (file, " %s", (control & 0x0008) ? "OM" : "  ");
  fprintf_filtered (file, " %s", (control & 0x0010) ? "UM" : "  ");
  fprintf_filtered (file, " %s", (control & 0x0020) ? "PM" : "  ");

  fputs_filtered ("\n", file);

  fputs_filtered ("                       PC: ", file);
  switch ((control >> 8) & 3)
    {
    case 0:
      fputs_filtered ("Single Precision (24-bits)\n", file);
      break;
    case 1:
      fputs_filtered ("Reserved\n", file);
      break;
    case 2:
      fputs_filtered ("Double Precision (53-bits)\n", file);
      break;
    case 3:
      fputs_filtered ("Extended Precision (64-bits)\n", file);
      break;
    }
      
  fputs_filtered ("                       RC: ", file);
  switch ((control >> 10) & 3)
    {
    case 0:
      fputs_filtered ("Round to nearest\n", file);
      break;
    case 1:
      fputs_filtered ("Round down\n", file);
      break;
    case 2:
      fputs_filtered ("Round up\n", file);
      break;
    case 3:
      fputs_filtered ("Round toward zero\n", file);
      break;
    }
}

/* Print out the i387 floating point state.  Note that we ignore FRAME
   in the code below.  That's OK since floating-point registers are
   never saved on the stack.  */

void
i387_print_float_info (struct gdbarch *gdbarch, struct ui_file *file,
		       struct frame_info *frame, const char *args)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (get_frame_arch (frame));
  ULONGEST fctrl;
  int fctrl_p;
  ULONGEST fstat;
  int fstat_p;
  ULONGEST ftag;
  int ftag_p;
  ULONGEST fiseg;
  int fiseg_p;
  ULONGEST fioff;
  int fioff_p;
  ULONGEST foseg;
  int foseg_p;
  ULONGEST fooff;
  int fooff_p;
  ULONGEST fop;
  int fop_p;
  int fpreg;
  int top;

  gdb_assert (gdbarch == get_frame_arch (frame));

  fctrl_p = read_frame_register_unsigned (frame,
					  I387_FCTRL_REGNUM (tdep), &fctrl);
  fstat_p = read_frame_register_unsigned (frame,
					  I387_FSTAT_REGNUM (tdep), &fstat);
  ftag_p = read_frame_register_unsigned (frame,
					 I387_FTAG_REGNUM (tdep), &ftag);
  fiseg_p = read_frame_register_unsigned (frame,
					  I387_FISEG_REGNUM (tdep), &fiseg);
  fioff_p = read_frame_register_unsigned (frame,
					  I387_FIOFF_REGNUM (tdep), &fioff);
  foseg_p = read_frame_register_unsigned (frame,
					  I387_FOSEG_REGNUM (tdep), &foseg);
  fooff_p = read_frame_register_unsigned (frame,
					  I387_FOOFF_REGNUM (tdep), &fooff);
  fop_p = read_frame_register_unsigned (frame,
					I387_FOP_REGNUM (tdep), &fop);

  if (fstat_p)
    {
      top = ((fstat >> 11) & 7);

      for (fpreg = 7; fpreg >= 0; fpreg--)
	{
	  struct value *regval;
	  int regnum;
	  int i;
	  int tag = -1;

	  fprintf_filtered (file, "%sR%d: ", fpreg == top ? "=>" : "  ", fpreg);

	  if (ftag_p)
	    {
	      tag = (ftag >> (fpreg * 2)) & 3;

	      switch (tag)
		{
		case 0:
		  fputs_filtered ("Valid   ", file);
		  break;
		case 1:
		  fputs_filtered ("Zero    ", file);
		  break;
		case 2:
		  fputs_filtered ("Special ", file);
		  break;
		case 3:
		  fputs_filtered ("Empty   ", file);
		  break;
		}
	    }
	  else
	    fputs_filtered ("Unknown ", file);

	  regnum = (fpreg + 8 - top) % 8 + I387_ST0_REGNUM (tdep);
	  regval = get_frame_register_value (frame, regnum);

	  if (value_entirely_available (regval))
	    {
	      const gdb_byte *raw = value_contents (regval);

	      fputs_filtered ("0x", file);
	      for (i = 9; i >= 0; i--)
		fprintf_filtered (file, "%02x", raw[i]);

	      if (tag != -1 && tag != 3)
		print_i387_ext (gdbarch, raw, file);
	    }
	  else
	    fprintf_filtered (file, "%s", _("<unavailable>"));

	  fputs_filtered ("\n", file);
	}
    }

  fputs_filtered ("\n", file);
  print_i387_status_word (fstat_p, fstat, file);
  print_i387_control_word (fctrl_p, fctrl, file);
  fprintf_filtered (file, "Tag Word:            %s\n",
		    ftag_p ? hex_string_custom (ftag, 4) : _("<unavailable>"));
  fprintf_filtered (file, "Instruction Pointer: %s:",
		    fiseg_p ? hex_string_custom (fiseg, 2) : _("<unavailable>"));
  fprintf_filtered (file, "%s\n",
		    fioff_p ? hex_string_custom (fioff, 8) : _("<unavailable>"));
  fprintf_filtered (file, "Operand Pointer:     %s:",
		    foseg_p ? hex_string_custom (foseg, 2) : _("<unavailable>"));
  fprintf_filtered (file, "%s\n",
		    fooff_p ? hex_string_custom (fooff, 8) : _("<unavailable>"));
  fprintf_filtered (file, "Opcode:              %s\n",
		    fop_p
		    ? (hex_string_custom (fop ? (fop | 0xd800) : 0, 4))
		    : _("<unavailable>"));
}


/* Return nonzero if a value of type TYPE stored in register REGNUM
   needs any special handling.  */

int
i387_convert_register_p (struct gdbarch *gdbarch, int regnum,
			 struct type *type)
{
  if (i386_fp_regnum_p (gdbarch, regnum))
    {
      /* Floating point registers must be converted unless we are
	 accessing them in their hardware type.  */
      if (type == i387_ext_type (gdbarch))
	return 0;
      else
	return 1;
    }

  return 0;
}

/* Read a value of type TYPE from register REGNUM in frame FRAME, and
   return its contents in TO.  */

int
i387_register_to_value (struct frame_info *frame, int regnum,
			struct type *type, gdb_byte *to,
			int *optimizedp, int *unavailablep)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  gdb_byte from[I386_MAX_REGISTER_SIZE];

  gdb_assert (i386_fp_regnum_p (gdbarch, regnum));

  /* We only support floating-point values.  */
  if (TYPE_CODE (type) != TYPE_CODE_FLT)
    {
      warning (_("Cannot convert floating-point register value "
	       "to non-floating-point type."));
      *optimizedp = *unavailablep = 0;
      return 0;
    }

  /* Convert to TYPE.  */
  if (!get_frame_register_bytes (frame, regnum, 0, TYPE_LENGTH (type),
				 from, optimizedp, unavailablep))
    return 0;

  convert_typed_floating (from, i387_ext_type (gdbarch), to, type);
  *optimizedp = *unavailablep = 0;
  return 1;
}

/* Write the contents FROM of a value of type TYPE into register
   REGNUM in frame FRAME.  */

void
i387_value_to_register (struct frame_info *frame, int regnum,
			struct type *type, const gdb_byte *from)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  gdb_byte to[I386_MAX_REGISTER_SIZE];

  gdb_assert (i386_fp_regnum_p (gdbarch, regnum));

  /* We only support floating-point values.  */
  if (TYPE_CODE (type) != TYPE_CODE_FLT)
    {
      warning (_("Cannot convert non-floating-point type "
	       "to floating-point register value."));
      return;
    }

  /* Convert from TYPE.  */
  convert_typed_floating (from, type, to, i387_ext_type (gdbarch));
  put_frame_register (frame, regnum, to);
}


/* Handle FSAVE and FXSAVE formats.  */

/* At fsave_offset[REGNUM] you'll find the offset to the location in
   the data structure used by the "fsave" instruction where GDB
   register REGNUM is stored.  */

static int fsave_offset[] =
{
  28 + 0 * 10,			/* %st(0) ...  */
  28 + 1 * 10,
  28 + 2 * 10,
  28 + 3 * 10,
  28 + 4 * 10,
  28 + 5 * 10,
  28 + 6 * 10,
  28 + 7 * 10,			/* ... %st(7).  */
  0,				/* `fctrl' (16 bits).  */
  4,				/* `fstat' (16 bits).  */
  8,				/* `ftag' (16 bits).  */
  16,				/* `fiseg' (16 bits).  */
  12,				/* `fioff'.  */
  24,				/* `foseg' (16 bits).  */
  20,				/* `fooff'.  */
  18				/* `fop' (bottom 11 bits).  */
};

#define FSAVE_ADDR(tdep, fsave, regnum) \
  (fsave + fsave_offset[regnum - I387_ST0_REGNUM (tdep)])


/* Fill register REGNUM in REGCACHE with the appropriate value from
   *FSAVE.  This function masks off any of the reserved bits in
   *FSAVE.  */

void
i387_supply_fsave (struct regcache *regcache, int regnum, const void *fsave)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  const gdb_byte *regs = fsave;
  int i;

  gdb_assert (tdep->st0_regnum >= I386_ST0_REGNUM);

  for (i = I387_ST0_REGNUM (tdep); i < I387_XMM0_REGNUM (tdep); i++)
    if (regnum == -1 || regnum == i)
      {
	if (fsave == NULL)
	  {
	    regcache_raw_supply (regcache, i, NULL);
	    continue;
	  }

	/* Most of the FPU control registers occupy only 16 bits in the
	   fsave area.  Give those a special treatment.  */
	if (i >= I387_FCTRL_REGNUM (tdep)
	    && i != I387_FIOFF_REGNUM (tdep) && i != I387_FOOFF_REGNUM (tdep))
	  {
	    gdb_byte val[4];

	    memcpy (val, FSAVE_ADDR (tdep, regs, i), 2);
	    val[2] = val[3] = 0;
	    if (i == I387_FOP_REGNUM (tdep))
	      val[1] &= ((1 << 3) - 1);
	    regcache_raw_supply (regcache, i, val);
	  }
	else
	  regcache_raw_supply (regcache, i, FSAVE_ADDR (tdep, regs, i));
      }

  /* Provide dummy values for the SSE registers.  */
  for (i = I387_XMM0_REGNUM (tdep); i < I387_MXCSR_REGNUM (tdep); i++)
    if (regnum == -1 || regnum == i)
      regcache_raw_supply (regcache, i, NULL);
  if (regnum == -1 || regnum == I387_MXCSR_REGNUM (tdep))
    {
      gdb_byte buf[4];

      store_unsigned_integer (buf, 4, byte_order, 0x1f80);
      regcache_raw_supply (regcache, I387_MXCSR_REGNUM (tdep), buf);
    }
}

/* Fill register REGNUM (if it is a floating-point register) in *FSAVE
   with the value from REGCACHE.  If REGNUM is -1, do this for all
   registers.  This function doesn't touch any of the reserved bits in
   *FSAVE.  */

void
i387_collect_fsave (const struct regcache *regcache, int regnum, void *fsave)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (get_regcache_arch (regcache));
  gdb_byte *regs = fsave;
  int i;

  gdb_assert (tdep->st0_regnum >= I386_ST0_REGNUM);

  for (i = I387_ST0_REGNUM (tdep); i < I387_XMM0_REGNUM (tdep); i++)
    if (regnum == -1 || regnum == i)
      {
	/* Most of the FPU control registers occupy only 16 bits in
           the fsave area.  Give those a special treatment.  */
	if (i >= I387_FCTRL_REGNUM (tdep)
	    && i != I387_FIOFF_REGNUM (tdep) && i != I387_FOOFF_REGNUM (tdep))
	  {
	    gdb_byte buf[4];

	    regcache_raw_collect (regcache, i, buf);

	    if (i == I387_FOP_REGNUM (tdep))
	      {
		/* The opcode occupies only 11 bits.  Make sure we
                   don't touch the other bits.  */
		buf[1] &= ((1 << 3) - 1);
		buf[1] |= ((FSAVE_ADDR (tdep, regs, i))[1] & ~((1 << 3) - 1));
	      }
	    memcpy (FSAVE_ADDR (tdep, regs, i), buf, 2);
	  }
	else
	  regcache_raw_collect (regcache, i, FSAVE_ADDR (tdep, regs, i));
      }
}


/* At fxsave_offset[REGNUM] you'll find the offset to the location in
   the data structure used by the "fxsave" instruction where GDB
   register REGNUM is stored.  */

static int fxsave_offset[] =
{
  32,				/* %st(0) through ...  */
  48,
  64,
  80,
  96,
  112,
  128,
  144,				/* ... %st(7) (80 bits each).  */
  0,				/* `fctrl' (16 bits).  */
  2,				/* `fstat' (16 bits).  */
  4,				/* `ftag' (16 bits).  */
  12,				/* `fiseg' (16 bits).  */
  8,				/* `fioff'.  */
  20,				/* `foseg' (16 bits).  */
  16,				/* `fooff'.  */
  6,				/* `fop' (bottom 11 bits).  */
  160 + 0 * 16,			/* %xmm0 through ...  */
  160 + 1 * 16,
  160 + 2 * 16,
  160 + 3 * 16,
  160 + 4 * 16,
  160 + 5 * 16,
  160 + 6 * 16,
  160 + 7 * 16,
  160 + 8 * 16,
  160 + 9 * 16,
  160 + 10 * 16,
  160 + 11 * 16,
  160 + 12 * 16,
  160 + 13 * 16,
  160 + 14 * 16,
  160 + 15 * 16,		/* ... %xmm15 (128 bits each).  */
};

#define FXSAVE_ADDR(tdep, fxsave, regnum) \
  (fxsave + fxsave_offset[regnum - I387_ST0_REGNUM (tdep)])

/* We made an unfortunate choice in putting %mxcsr after the SSE
   registers %xmm0-%xmm7 instead of before, since it makes supporting
   the registers %xmm8-%xmm15 on AMD64 a bit involved.  Therefore we
   don't include the offset for %mxcsr here above.  */

#define FXSAVE_MXCSR_ADDR(fxsave) (fxsave + 24)

static int i387_tag (const gdb_byte *raw);


/* Fill register REGNUM in REGCACHE with the appropriate
   floating-point or SSE register value from *FXSAVE.  This function
   masks off any of the reserved bits in *FXSAVE.  */

void
i387_supply_fxsave (struct regcache *regcache, int regnum, const void *fxsave)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (get_regcache_arch (regcache));
  const gdb_byte *regs = fxsave;
  int i;

  gdb_assert (tdep->st0_regnum >= I386_ST0_REGNUM);
  gdb_assert (tdep->num_xmm_regs > 0);

  for (i = I387_ST0_REGNUM (tdep); i < I387_MXCSR_REGNUM (tdep); i++)
    if (regnum == -1 || regnum == i)
      {
	if (regs == NULL)
	  {
	    regcache_raw_supply (regcache, i, NULL);
	    continue;
	  }

	/* Most of the FPU control registers occupy only 16 bits in
	   the fxsave area.  Give those a special treatment.  */
	if (i >= I387_FCTRL_REGNUM (tdep) && i < I387_XMM0_REGNUM (tdep)
	    && i != I387_FIOFF_REGNUM (tdep) && i != I387_FOOFF_REGNUM (tdep))
	  {
	    gdb_byte val[4];

	    memcpy (val, FXSAVE_ADDR (tdep, regs, i), 2);
	    val[2] = val[3] = 0;
	    if (i == I387_FOP_REGNUM (tdep))
	      val[1] &= ((1 << 3) - 1);
	    else if (i== I387_FTAG_REGNUM (tdep))
	      {
		/* The fxsave area contains a simplified version of
		   the tag word.  We have to look at the actual 80-bit
		   FP data to recreate the traditional i387 tag word.  */

		unsigned long ftag = 0;
		int fpreg;
		int top;

		top = ((FXSAVE_ADDR (tdep, regs,
				     I387_FSTAT_REGNUM (tdep)))[1] >> 3);
		top &= 0x7;

		for (fpreg = 7; fpreg >= 0; fpreg--)
		  {
		    int tag;

		    if (val[0] & (1 << fpreg))
		      {
			int thisreg = (fpreg + 8 - top) % 8 
			               + I387_ST0_REGNUM (tdep);
			tag = i387_tag (FXSAVE_ADDR (tdep, regs, thisreg));
		      }
		    else
		      tag = 3;		/* Empty */

		    ftag |= tag << (2 * fpreg);
		  }
		val[0] = ftag & 0xff;
		val[1] = (ftag >> 8) & 0xff;
	      }
	    regcache_raw_supply (regcache, i, val);
	  }
	else
	  regcache_raw_supply (regcache, i, FXSAVE_ADDR (tdep, regs, i));
      }

  if (regnum == I387_MXCSR_REGNUM (tdep) || regnum == -1)
    {
      if (regs == NULL)
	regcache_raw_supply (regcache, I387_MXCSR_REGNUM (tdep), NULL);
      else
	regcache_raw_supply (regcache, I387_MXCSR_REGNUM (tdep),
			     FXSAVE_MXCSR_ADDR (regs));
    }
}

/* Fill register REGNUM (if it is a floating-point or SSE register) in
   *FXSAVE with the value from REGCACHE.  If REGNUM is -1, do this for
   all registers.  This function doesn't touch any of the reserved
   bits in *FXSAVE.  */

void
i387_collect_fxsave (const struct regcache *regcache, int regnum, void *fxsave)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (get_regcache_arch (regcache));
  gdb_byte *regs = fxsave;
  int i;

  gdb_assert (tdep->st0_regnum >= I386_ST0_REGNUM);
  gdb_assert (tdep->num_xmm_regs > 0);

  for (i = I387_ST0_REGNUM (tdep); i < I387_MXCSR_REGNUM (tdep); i++)
    if (regnum == -1 || regnum == i)
      {
	/* Most of the FPU control registers occupy only 16 bits in
           the fxsave area.  Give those a special treatment.  */
	if (i >= I387_FCTRL_REGNUM (tdep) && i < I387_XMM0_REGNUM (tdep)
	    && i != I387_FIOFF_REGNUM (tdep) && i != I387_FOOFF_REGNUM (tdep))
	  {
	    gdb_byte buf[4];

	    regcache_raw_collect (regcache, i, buf);

	    if (i == I387_FOP_REGNUM (tdep))
	      {
		/* The opcode occupies only 11 bits.  Make sure we
                   don't touch the other bits.  */
		buf[1] &= ((1 << 3) - 1);
		buf[1] |= ((FXSAVE_ADDR (tdep, regs, i))[1] & ~((1 << 3) - 1));
	      }
	    else if (i == I387_FTAG_REGNUM (tdep))
	      {
		/* Converting back is much easier.  */

		unsigned short ftag;
		int fpreg;

		ftag = (buf[1] << 8) | buf[0];
		buf[0] = 0;
		buf[1] = 0;

		for (fpreg = 7; fpreg >= 0; fpreg--)
		  {
		    int tag = (ftag >> (fpreg * 2)) & 3;

		    if (tag != 3)
		      buf[0] |= (1 << fpreg);
		  }
	      }
	    memcpy (FXSAVE_ADDR (tdep, regs, i), buf, 2);
	  }
	else
	  regcache_raw_collect (regcache, i, FXSAVE_ADDR (tdep, regs, i));
      }

  if (regnum == I387_MXCSR_REGNUM (tdep) || regnum == -1)
    regcache_raw_collect (regcache, I387_MXCSR_REGNUM (tdep),
			  FXSAVE_MXCSR_ADDR (regs));
}

/* `xstate_bv' is at byte offset 512.  */
#define XSAVE_XSTATE_BV_ADDR(xsave) (xsave + 512)

/* At xsave_avxh_offset[REGNUM] you'll find the offset to the location in
   the upper 128bit of AVX register data structure used by the "xsave"
   instruction where GDB register REGNUM is stored.  */

static int xsave_avxh_offset[] =
{
  576 + 0 * 16,		/* Upper 128bit of %ymm0 through ...  */
  576 + 1 * 16,
  576 + 2 * 16,
  576 + 3 * 16,
  576 + 4 * 16,
  576 + 5 * 16,
  576 + 6 * 16,
  576 + 7 * 16,
  576 + 8 * 16,
  576 + 9 * 16,
  576 + 10 * 16,
  576 + 11 * 16,
  576 + 12 * 16,
  576 + 13 * 16,
  576 + 14 * 16,
  576 + 15 * 16		/* Upper 128bit of ... %ymm15 (128 bits each).  */
};

static int xsave_mpx_offset[] =
{
  960 + 0 * 16,			/* bnd0r...bnd3r registers.  */
  960 + 1 * 16,
  960 + 2 * 16,
  960 + 3 * 16,
  1024 + 0 * 8,			/* bndcfg ... bndstatus.  */
  1024 + 1 * 8,
};

#define XSAVE_AVXH_ADDR(tdep, xsave, regnum) \
  (xsave + xsave_avxh_offset[regnum - I387_YMM0H_REGNUM (tdep)])

#define XSAVE_MPX_ADDR(tdep, xsave, regnum) \
  (xsave + xsave_mpx_offset[regnum - I387_BND0R_REGNUM (tdep)])

/* Similar to i387_supply_fxsave, but use XSAVE extended state.  */

void
i387_supply_xsave (struct regcache *regcache, int regnum,
		   const void *xsave)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (get_regcache_arch (regcache));
  const gdb_byte *regs = xsave;
  int i;
  unsigned int clear_bv;
  static const gdb_byte zero[MAX_REGISTER_SIZE] = { 0 };
  enum
    {
      none = 0x0,
      x87 = 0x1,
      sse = 0x2,
      avxh = 0x4,
      mpx  = 0x8,
      all = x87 | sse | avxh | mpx
    } regclass;

  gdb_assert (regs != NULL);
  gdb_assert (tdep->st0_regnum >= I386_ST0_REGNUM);
  gdb_assert (tdep->num_xmm_regs > 0);

  if (regnum == -1)
    regclass = all;
  else if (regnum >= I387_YMM0H_REGNUM (tdep)
	   && regnum < I387_YMMENDH_REGNUM (tdep))
    regclass = avxh;
  else if (regnum >= I387_BND0R_REGNUM (tdep)
	   && regnum < I387_MPXEND_REGNUM (tdep))
    regclass = mpx;
  else if (regnum >= I387_XMM0_REGNUM(tdep)
	   && regnum < I387_MXCSR_REGNUM (tdep))
    regclass = sse;
  else if (regnum >= I387_ST0_REGNUM (tdep)
	   && regnum < I387_FCTRL_REGNUM (tdep))
    regclass = x87;
  else
    regclass = none;

  if (regclass != none)
    {
      /* Get `xstat_bv'.  */
      const gdb_byte *xstate_bv_p = XSAVE_XSTATE_BV_ADDR (regs);

      /* The supported bits in `xstat_bv' are 1 byte.  Clear part in
	 vector registers if its bit in xstat_bv is zero.  */
      clear_bv = (~(*xstate_bv_p)) & tdep->xcr0;
    }
  else
    clear_bv = I386_XSTATE_ALL_MASK;

  /* With the delayed xsave mechanism, in between the program
     starting, and the program accessing the vector registers for the
     first time, the register's values are invalid.  The kernel
     initializes register states to zero when they are set the first
     time in a program.  This means that from the user-space programs'
     perspective, it's the same as if the registers have always been
     zero from the start of the program.  Therefore, the debugger
     should provide the same illusion to the user.  */

  switch (regclass)
    {
    case none:
      break;

    case avxh:
      if ((clear_bv & I386_XSTATE_AVX))
	regcache_raw_supply (regcache, regnum, zero);
      else
	regcache_raw_supply (regcache, regnum,
			     XSAVE_AVXH_ADDR (tdep, regs, regnum));
      return;

    case mpx:
      if ((clear_bv & I386_XSTATE_BNDREGS))
	regcache_raw_supply (regcache, regnum, zero);
      else
	regcache_raw_supply (regcache, regnum,
			     XSAVE_MPX_ADDR (tdep, regs, regnum));
      return;

    case sse:
      if ((clear_bv & I386_XSTATE_SSE))
	regcache_raw_supply (regcache, regnum, zero);
      else
	regcache_raw_supply (regcache, regnum,
			     FXSAVE_ADDR (tdep, regs, regnum));
      return;

    case x87:
      if ((clear_bv & I386_XSTATE_X87))
	regcache_raw_supply (regcache, regnum, zero);
      else
	regcache_raw_supply (regcache, regnum,
			     FXSAVE_ADDR (tdep, regs, regnum));
      return;

    case all:
      /* Handle the upper YMM registers.  */
      if ((tdep->xcr0 & I386_XSTATE_AVX))
	{
	  if ((clear_bv & I386_XSTATE_AVX))
	    {
	      for (i = I387_YMM0H_REGNUM (tdep);
		   i < I387_YMMENDH_REGNUM (tdep);
		   i++)
		regcache_raw_supply (regcache, i, zero);
	    }
	  else
	    {
	      for (i = I387_YMM0H_REGNUM (tdep);
		   i < I387_YMMENDH_REGNUM (tdep);
		   i++)
		regcache_raw_supply (regcache, i,
				     XSAVE_AVXH_ADDR (tdep, regs, i));
	    }
	}

      /* Handle the MPX registers.  */
      if ((tdep->xcr0 & I386_XSTATE_BNDREGS))
	{
	  if (clear_bv & I386_XSTATE_BNDREGS)
	    {
	      for (i = I387_BND0R_REGNUM (tdep);
		   i < I387_BNDCFGU_REGNUM (tdep); i++)
		regcache_raw_supply (regcache, i, zero);
	    }
	  else
	    {
	      for (i = I387_BND0R_REGNUM (tdep);
		   i < I387_BNDCFGU_REGNUM (tdep); i++)
		regcache_raw_supply (regcache, i,
				     XSAVE_MPX_ADDR (tdep, regs, i));
	    }
	}

      /* Handle the MPX registers.  */
      if ((tdep->xcr0 & I386_XSTATE_BNDCFG))
	{
	  if (clear_bv & I386_XSTATE_BNDCFG)
	    {
	      for (i = I387_BNDCFGU_REGNUM (tdep);
		   i < I387_MPXEND_REGNUM (tdep); i++)
		regcache_raw_supply (regcache, i, zero);
	    }
	  else
	    {
	      for (i = I387_BNDCFGU_REGNUM (tdep);
		   i < I387_MPXEND_REGNUM (tdep); i++)
		regcache_raw_supply (regcache, i,
				     XSAVE_MPX_ADDR (tdep, regs, i));
	    }
	}

      /* Handle the XMM registers.  */
      if ((tdep->xcr0 & I386_XSTATE_SSE))
	{
	  if ((clear_bv & I386_XSTATE_SSE))
	    {
	      for (i = I387_XMM0_REGNUM (tdep);
		   i < I387_MXCSR_REGNUM (tdep);
		   i++)
		regcache_raw_supply (regcache, i, zero);
	    }
	  else
	    {
	      for (i = I387_XMM0_REGNUM (tdep);
		   i < I387_MXCSR_REGNUM (tdep); i++)
		regcache_raw_supply (regcache, i,
				     FXSAVE_ADDR (tdep, regs, i));
	    }
	}

      /* Handle the x87 registers.  */
      if ((tdep->xcr0 & I386_XSTATE_X87))
	{
	  if ((clear_bv & I386_XSTATE_X87))
	    {
	      for (i = I387_ST0_REGNUM (tdep);
		   i < I387_FCTRL_REGNUM (tdep);
		   i++)
		regcache_raw_supply (regcache, i, zero);
	    }
	  else
	    {
	      for (i = I387_ST0_REGNUM (tdep);
		   i < I387_FCTRL_REGNUM (tdep);
		   i++)
		regcache_raw_supply (regcache, i, FXSAVE_ADDR (tdep, regs, i));
	    }
	}
      break;
    }

  /* Only handle x87 control registers.  */
  for (i = I387_FCTRL_REGNUM (tdep); i < I387_XMM0_REGNUM (tdep); i++)
    if (regnum == -1 || regnum == i)
      {
	/* Most of the FPU control registers occupy only 16 bits in
	   the xsave extended state.  Give those a special treatment.  */
	if (i != I387_FIOFF_REGNUM (tdep)
	    && i != I387_FOOFF_REGNUM (tdep))
	  {
	    gdb_byte val[4];

	    memcpy (val, FXSAVE_ADDR (tdep, regs, i), 2);
	    val[2] = val[3] = 0;
	    if (i == I387_FOP_REGNUM (tdep))
	      val[1] &= ((1 << 3) - 1);
	    else if (i== I387_FTAG_REGNUM (tdep))
	      {
		/* The fxsave area contains a simplified version of
		   the tag word.  We have to look at the actual 80-bit
		   FP data to recreate the traditional i387 tag word.  */

		unsigned long ftag = 0;
		int fpreg;
		int top;

		top = ((FXSAVE_ADDR (tdep, regs,
				     I387_FSTAT_REGNUM (tdep)))[1] >> 3);
		top &= 0x7;

		for (fpreg = 7; fpreg >= 0; fpreg--)
		  {
		    int tag;

		    if (val[0] & (1 << fpreg))
		      {
			int thisreg = (fpreg + 8 - top) % 8 
				       + I387_ST0_REGNUM (tdep);
			tag = i387_tag (FXSAVE_ADDR (tdep, regs, thisreg));
		      }
		    else
		      tag = 3;		/* Empty */

		    ftag |= tag << (2 * fpreg);
		  }
		val[0] = ftag & 0xff;
		val[1] = (ftag >> 8) & 0xff;
	      }
	    regcache_raw_supply (regcache, i, val);
	  }
	else 
	  regcache_raw_supply (regcache, i, FXSAVE_ADDR (tdep, regs, i));
      }

  if (regnum == I387_MXCSR_REGNUM (tdep) || regnum == -1)
    regcache_raw_supply (regcache, I387_MXCSR_REGNUM (tdep),
			 FXSAVE_MXCSR_ADDR (regs));
}

/* Similar to i387_collect_fxsave, but use XSAVE extended state.  */

void
i387_collect_xsave (const struct regcache *regcache, int regnum,
		    void *xsave, int gcore)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (get_regcache_arch (regcache));
  gdb_byte *regs = xsave;
  int i;
  enum
    {
      none = 0x0,
      check = 0x1,
      x87 = 0x2 | check,
      sse = 0x4 | check,
      avxh = 0x8 | check,
      mpx  = 0x10 | check,
      all = x87 | sse | avxh | mpx
    } regclass;

  gdb_assert (tdep->st0_regnum >= I386_ST0_REGNUM);
  gdb_assert (tdep->num_xmm_regs > 0);

  if (regnum == -1)
    regclass = all;
  else if (regnum >= I387_YMM0H_REGNUM (tdep)
	   && regnum < I387_YMMENDH_REGNUM (tdep))
    regclass = avxh;
  else if (regnum >= I387_BND0R_REGNUM (tdep)
	   && regnum < I387_MPXEND_REGNUM (tdep))
    regclass = mpx;
  else if (regnum >= I387_XMM0_REGNUM (tdep)
	   && regnum < I387_MXCSR_REGNUM (tdep))
    regclass = sse;
  else if (regnum >= I387_ST0_REGNUM (tdep)
	   && regnum < I387_FCTRL_REGNUM (tdep))
    regclass = x87;
  else
    regclass = none;

  if (gcore)
    {
      /* Clear XSAVE extended state.  */
      memset (regs, 0, I386_XSTATE_SIZE (tdep->xcr0));

      /* Update XCR0 and `xstate_bv' with XCR0 for gcore.  */
      if (tdep->xsave_xcr0_offset != -1)
	memcpy (regs + tdep->xsave_xcr0_offset, &tdep->xcr0, 8);
      memcpy (XSAVE_XSTATE_BV_ADDR (regs), &tdep->xcr0, 8);
    }

  if ((regclass & check))
    {
      gdb_byte raw[I386_MAX_REGISTER_SIZE];
      gdb_byte *xstate_bv_p = XSAVE_XSTATE_BV_ADDR (regs);
      unsigned int xstate_bv = 0;
      /* The supported bits in `xstat_bv' are 1 byte.  */
      unsigned int clear_bv = (~(*xstate_bv_p)) & tdep->xcr0;
      gdb_byte *p;

      /* Clear register set if its bit in xstat_bv is zero.  */
      if (clear_bv)
	{
	  if ((clear_bv & I386_XSTATE_BNDREGS))
	    for (i = I387_BND0R_REGNUM (tdep);
		 i < I387_BNDCFGU_REGNUM (tdep); i++)
	      memset (XSAVE_MPX_ADDR (tdep, regs, i), 0, 16);

	  if ((clear_bv & I386_XSTATE_BNDCFG))
	    for (i = I387_BNDCFGU_REGNUM (tdep);
		 i < I387_MPXEND_REGNUM (tdep); i++)
	      memset (XSAVE_MPX_ADDR (tdep, regs, i), 0, 8);

	  if ((clear_bv & I386_XSTATE_AVX))
	    for (i = I387_YMM0H_REGNUM (tdep);
		 i < I387_YMMENDH_REGNUM (tdep); i++)
	      memset (XSAVE_AVXH_ADDR (tdep, regs, i), 0, 16);

	  if ((clear_bv & I386_XSTATE_SSE))
	    for (i = I387_XMM0_REGNUM (tdep);
		 i < I387_MXCSR_REGNUM (tdep); i++)
	      memset (FXSAVE_ADDR (tdep, regs, i), 0, 16);

	  if ((clear_bv & I386_XSTATE_X87))
	    for (i = I387_ST0_REGNUM (tdep);
		 i < I387_FCTRL_REGNUM (tdep); i++)
	      memset (FXSAVE_ADDR (tdep, regs, i), 0, 10);
	}

      if (regclass == all)
	{
	  /* Check if any upper YMM registers are changed.  */
	  if ((tdep->xcr0 & I386_XSTATE_AVX))
	    for (i = I387_YMM0H_REGNUM (tdep);
		 i < I387_YMMENDH_REGNUM (tdep); i++)
	      {
		regcache_raw_collect (regcache, i, raw);
		p = XSAVE_AVXH_ADDR (tdep, regs, i);
		if (memcmp (raw, p, 16))
		  {
		    xstate_bv |= I386_XSTATE_AVX;
		    memcpy (p, raw, 16);
		  }
	      }
	  /* Check if any upper MPX registers are changed.  */
	  if ((tdep->xcr0 & I386_XSTATE_BNDREGS))
	    for (i = I387_BND0R_REGNUM (tdep);
		 i < I387_BNDCFGU_REGNUM (tdep); i++)
	      {
		regcache_raw_collect (regcache, i, raw);
		p = XSAVE_MPX_ADDR (tdep, regs, i);
		if (memcmp (raw, p, 16))
		  {
		    xstate_bv |= I386_XSTATE_BNDREGS;
		    memcpy (p, raw, 16);
		  }
	      }

	  /* Check if any upper MPX registers are changed.  */
	  if ((tdep->xcr0 & I386_XSTATE_BNDCFG))
	    for (i = I387_BNDCFGU_REGNUM (tdep);
		 i < I387_MPXEND_REGNUM (tdep); i++)
	      {
		regcache_raw_collect (regcache, i, raw);
		p = XSAVE_MPX_ADDR (tdep, regs, i);
		if (memcmp (raw, p, 8))
		  {
		    xstate_bv |= I386_XSTATE_BNDCFG;
		    memcpy (p, raw, 8);
		  }
	      }

	  /* Check if any SSE registers are changed.  */
	  if ((tdep->xcr0 & I386_XSTATE_SSE))
	    for (i = I387_XMM0_REGNUM (tdep);
		 i < I387_MXCSR_REGNUM (tdep); i++)
	      {
		regcache_raw_collect (regcache, i, raw);
		p = FXSAVE_ADDR (tdep, regs, i);
		if (memcmp (raw, p, 16))
		  {
		    xstate_bv |= I386_XSTATE_SSE;
		    memcpy (p, raw, 16);
		  }
	      }

	  /* Check if any X87 registers are changed.  */
	  if ((tdep->xcr0 & I386_XSTATE_X87))
	    for (i = I387_ST0_REGNUM (tdep);
		 i < I387_FCTRL_REGNUM (tdep); i++)
	      {
		regcache_raw_collect (regcache, i, raw);
		p = FXSAVE_ADDR (tdep, regs, i);
		if (memcmp (raw, p, 10))
		  {
		    xstate_bv |= I386_XSTATE_X87;
		    memcpy (p, raw, 10);
		  }
	      }
	}
      else
	{
	  /* Check if REGNUM is changed.  */
	  regcache_raw_collect (regcache, regnum, raw);

	  switch (regclass)
	    {
	    default:
	      internal_error (__FILE__, __LINE__,
			      _("invalid i387 regclass"));

	    case avxh:
	      /* This is an upper YMM register.  */
	      p = XSAVE_AVXH_ADDR (tdep, regs, regnum);
	      if (memcmp (raw, p, 16))
		{
		  xstate_bv |= I386_XSTATE_AVX;
		  memcpy (p, raw, 16);
		}
	      break;

	    case mpx:
	      if (regnum < I387_BNDCFGU_REGNUM (tdep))
		{
		  regcache_raw_collect (regcache, regnum, raw);
		  p = XSAVE_MPX_ADDR (tdep, regs, regnum);
		  if (memcmp (raw, p, 16))
		    {
		      xstate_bv |= I386_XSTATE_BNDREGS;
		      memcpy (p, raw, 16);
		    }
		}
	      else
		{
		  p = XSAVE_MPX_ADDR (tdep, regs, regnum);
		  xstate_bv |= I386_XSTATE_BNDCFG;
		  memcpy (p, raw, 8);
		}
	      break;

	    case sse:
	      /* This is an SSE register.  */
	      p = FXSAVE_ADDR (tdep, regs, regnum);
	      if (memcmp (raw, p, 16))
		{
		  xstate_bv |= I386_XSTATE_SSE;
		  memcpy (p, raw, 16);
		}
	      break;

	    case x87:
	      /* This is an x87 register.  */
	      p = FXSAVE_ADDR (tdep, regs, regnum);
	      if (memcmp (raw, p, 10))
		{
		  xstate_bv |= I386_XSTATE_X87;
		  memcpy (p, raw, 10);
		}
	      break;
	    }
	}

      /* Update the corresponding bits in `xstate_bv' if any SSE/AVX
	 registers are changed.  */
      if (xstate_bv)
	{
	  /* The supported bits in `xstat_bv' are 1 byte.  */
	  *xstate_bv_p |= (gdb_byte) xstate_bv;

	  switch (regclass)
	    {
	    default:
	      internal_error (__FILE__, __LINE__,
			      _("invalid i387 regclass"));

	    case all:
	      break;

	    case x87:
	    case sse:
	    case avxh:
	    case mpx:
	      /* Register REGNUM has been updated.  Return.  */
	      return;
	    }
	}
      else
	{
	  /* Return if REGNUM isn't changed.  */
	  if (regclass != all)
	    return;
	}
    }

  /* Only handle x87 control registers.  */
  for (i = I387_FCTRL_REGNUM (tdep); i < I387_XMM0_REGNUM (tdep); i++)
    if (regnum == -1 || regnum == i)
      {
	/* Most of the FPU control registers occupy only 16 bits in
	   the xsave extended state.  Give those a special treatment.  */
	if (i != I387_FIOFF_REGNUM (tdep)
	    && i != I387_FOOFF_REGNUM (tdep))
	  {
	    gdb_byte buf[4];

	    regcache_raw_collect (regcache, i, buf);

	    if (i == I387_FOP_REGNUM (tdep))
	      {
		/* The opcode occupies only 11 bits.  Make sure we
		   don't touch the other bits.  */
		buf[1] &= ((1 << 3) - 1);
		buf[1] |= ((FXSAVE_ADDR (tdep, regs, i))[1] & ~((1 << 3) - 1));
	      }
	    else if (i == I387_FTAG_REGNUM (tdep))
	      {
		/* Converting back is much easier.  */

		unsigned short ftag;
		int fpreg;

		ftag = (buf[1] << 8) | buf[0];
		buf[0] = 0;
		buf[1] = 0;

		for (fpreg = 7; fpreg >= 0; fpreg--)
		  {
		    int tag = (ftag >> (fpreg * 2)) & 3;

		    if (tag != 3)
		      buf[0] |= (1 << fpreg);
		  }
	      }
	    memcpy (FXSAVE_ADDR (tdep, regs, i), buf, 2);
	  }
	else
	  regcache_raw_collect (regcache, i, FXSAVE_ADDR (tdep, regs, i));
      }

  if (regnum == I387_MXCSR_REGNUM (tdep) || regnum == -1)
    regcache_raw_collect (regcache, I387_MXCSR_REGNUM (tdep),
			  FXSAVE_MXCSR_ADDR (regs));
}

/* Recreate the FTW (tag word) valid bits from the 80-bit FP data in
   *RAW.  */

static int
i387_tag (const gdb_byte *raw)
{
  int integer;
  unsigned int exponent;
  unsigned long fraction[2];

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

/* Prepare the FPU stack in REGCACHE for a function return.  */

void
i387_return_value (struct gdbarch *gdbarch, struct regcache *regcache)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  ULONGEST fstat;

  /* Set the top of the floating-point register stack to 7.  The
     actual value doesn't really matter, but 7 is what a normal
     function return would end up with if the program started out with
     a freshly initialized FPU.  */
  regcache_raw_read_unsigned (regcache, I387_FSTAT_REGNUM (tdep), &fstat);
  fstat |= (7 << 11);
  regcache_raw_write_unsigned (regcache, I387_FSTAT_REGNUM (tdep), fstat);

  /* Mark %st(1) through %st(7) as empty.  Since we set the top of the
     floating-point register stack to 7, the appropriate value for the
     tag word is 0x3fff.  */
  regcache_raw_write_unsigned (regcache, I387_FTAG_REGNUM (tdep), 0x3fff);

}
