/* Intel 387 floating point stuff.
   Copyright 1988, 1989, 1991, 1992, 1993, 1994, 1998, 1999, 2000,
   2001, 2002 Free Software Foundation, Inc.

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
#include "frame.h"
#include "inferior.h"
#include "language.h"
#include "value.h"
#include "gdbcore.h"
#include "floatformat.h"
#include "regcache.h"
#include "gdb_assert.h"
#include "gdb_string.h"
#include "doublest.h"

#include "i386-tdep.h"

/* FIXME: Eliminate the next two functions when we have the time to
   change all the callers.  */

void i387_to_double (char *from, char *to);
void double_to_i387 (char *from, char *to);

void
i387_to_double (char *from, char *to)
{
  floatformat_to_double (&floatformat_i387_ext, from, (double *) to);
}

void
double_to_i387 (char *from, char *to)
{
  floatformat_from_double (&floatformat_i387_ext, (double *) from, to);
}


/* FIXME: The functions on this page are used by the old `info float'
   implementations that a few of the i386 targets provide.  These
   functions should be removed if all of these have been converted to
   use the generic implementation based on the new register file
   layout.  */

static void print_387_control_bits (unsigned int control);
static void print_387_status_bits (unsigned int status);

static void
print_387_control_bits (unsigned int control)
{
  switch ((control >> 8) & 3)
    {
    case 0:
      puts_unfiltered (" 24 bit; ");
      break;
    case 1:
      puts_unfiltered (" (bad); ");
      break;
    case 2:
      puts_unfiltered (" 53 bit; ");
      break;
    case 3:
      puts_unfiltered (" 64 bit; ");
      break;
    }
  switch ((control >> 10) & 3)
    {
    case 0:
      puts_unfiltered ("NEAR; ");
      break;
    case 1:
      puts_unfiltered ("DOWN; ");
      break;
    case 2:
      puts_unfiltered ("UP; ");
      break;
    case 3:
      puts_unfiltered ("CHOP; ");
      break;
    }
  if (control & 0x3f)
    {
      puts_unfiltered ("mask");
      if (control & 0x0001)
	puts_unfiltered (" INVAL");
      if (control & 0x0002)
	puts_unfiltered (" DENOR");
      if (control & 0x0004)
	puts_unfiltered (" DIVZ");
      if (control & 0x0008)
	puts_unfiltered (" OVERF");
      if (control & 0x0010)
	puts_unfiltered (" UNDER");
      if (control & 0x0020)
	puts_unfiltered (" LOS");
      puts_unfiltered (";");
    }

  if (control & 0xe080)
    warning ("\nreserved bits on: %s",
	     local_hex_string (control & 0xe080));
}

void
print_387_control_word (unsigned int control)
{
  printf_filtered ("control %s:", local_hex_string(control & 0xffff));
  print_387_control_bits (control);
  puts_unfiltered ("\n");
}

static void
print_387_status_bits (unsigned int status)
{
  printf_unfiltered (" flags %d%d%d%d; ",
		     (status & 0x4000) != 0,
		     (status & 0x0400) != 0,
		     (status & 0x0200) != 0,
		     (status & 0x0100) != 0);
  printf_unfiltered ("top %d; ", (status >> 11) & 7);
  if (status & 0xff) 
    {
      puts_unfiltered ("excep");
      if (status & 0x0001) puts_unfiltered (" INVAL");
      if (status & 0x0002) puts_unfiltered (" DENOR");
      if (status & 0x0004) puts_unfiltered (" DIVZ");
      if (status & 0x0008) puts_unfiltered (" OVERF");
      if (status & 0x0010) puts_unfiltered (" UNDER");
      if (status & 0x0020) puts_unfiltered (" LOS");
      if (status & 0x0040) puts_unfiltered (" STACK");
    }
}

void
print_387_status_word (unsigned int status)
{
  printf_filtered ("status %s:", local_hex_string (status & 0xffff));
  print_387_status_bits (status);
  puts_unfiltered ("\n");
}


/* Implement the `info float' layout based on the register definitions
   in `tm-i386.h'.  */

/* Print the floating point number specified by RAW.  */
static void
print_i387_value (char *raw, struct ui_file *file)
{
  DOUBLEST value;

  /* Using extract_typed_floating here might affect the representation
     of certain numbers such as NaNs, even if GDB is running natively.
     This is fine since our caller already detects such special
     numbers and we print the hexadecimal representation anyway.  */
  value = extract_typed_floating (raw, builtin_type_i387_ext);

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
print_i387_ext (unsigned char *raw, struct ui_file *file)
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
    print_i387_value (raw, file);
  else if (exponent == 0x0000)
    {
      /* Denormal or zero.  */
      print_i387_value (raw, file);
      
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

/* Print the status word STATUS.  */
static void
print_i387_status_word (unsigned int status, struct ui_file *file)
{
  fprintf_filtered (file, "Status Word:         %s",
		   local_hex_string_custom (status, "04"));
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

/* Print the control word CONTROL.  */
static void
print_i387_control_word (unsigned int control, struct ui_file *file)
{
  fprintf_filtered (file, "Control Word:        %s",
		   local_hex_string_custom (control, "04"));
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
  unsigned int fctrl;
  unsigned int fstat;
  unsigned int ftag;
  unsigned int fiseg;
  unsigned int fioff;
  unsigned int foseg;
  unsigned int fooff;
  unsigned int fop;
  int fpreg;
  int top;

  fctrl = read_register (FCTRL_REGNUM);
  fstat = read_register (FSTAT_REGNUM);
  ftag  = read_register (FTAG_REGNUM);
  fiseg = read_register (FCS_REGNUM);
  fioff = read_register (FCOFF_REGNUM);
  foseg = read_register (FDS_REGNUM);
  fooff = read_register (FDOFF_REGNUM);
  fop   = read_register (FOP_REGNUM);
  
  top = ((fstat >> 11) & 7);

  for (fpreg = 7; fpreg >= 0; fpreg--)
    {
      unsigned char raw[FPU_REG_RAW_SIZE];
      int tag = (ftag >> (fpreg * 2)) & 3;
      int i;

      fprintf_filtered (file, "%sR%d: ", fpreg == top ? "=>" : "  ", fpreg);

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

      read_register_gen ((fpreg + 8 - top) % 8 + FP0_REGNUM, raw);

      fputs_filtered ("0x", file);
      for (i = 9; i >= 0; i--)
	fprintf_filtered (file, "%02x", raw[i]);

      if (tag != 3)
	print_i387_ext (raw, file);

      fputs_filtered ("\n", file);
    }

  puts_filtered ("\n");

  print_i387_status_word (fstat, file);
  print_i387_control_word (fctrl, file);
  fprintf_filtered (file, "Tag Word:            %s\n",
		    local_hex_string_custom (ftag, "04"));
  fprintf_filtered (file, "Instruction Pointer: %s:",
		    local_hex_string_custom (fiseg, "02"));
  fprintf_filtered (file, "%s\n", local_hex_string_custom (fioff, "08"));
  fprintf_filtered (file, "Operand Pointer:     %s:",
		    local_hex_string_custom (foseg, "02"));
  fprintf_filtered (file, "%s\n", local_hex_string_custom (fooff, "08"));
  fprintf_filtered (file, "Opcode:              %s\n",
		    local_hex_string_custom (fop ? (fop | 0xd800) : 0, "04"));
}

/* FIXME: kettenis/2000-05-21: Right now more than a few i386 targets
   define their own routines to manage the floating-point registers in
   GDB's register array.  Most (if not all) of these targets use the
   format used by the "fsave" instruction in their communication with
   the OS.  They should all be converted to use the routines below.  */

/* At fsave_offset[REGNUM] you'll find the offset to the location in
   the data structure used by the "fsave" instruction where GDB
   register REGNUM is stored.  */

static int fsave_offset[] =
{
  28 + 0 * FPU_REG_RAW_SIZE,	/* FP0_REGNUM through ...  */
  28 + 1 * FPU_REG_RAW_SIZE,  
  28 + 2 * FPU_REG_RAW_SIZE,  
  28 + 3 * FPU_REG_RAW_SIZE,  
  28 + 4 * FPU_REG_RAW_SIZE,  
  28 + 5 * FPU_REG_RAW_SIZE,  
  28 + 6 * FPU_REG_RAW_SIZE,  
  28 + 7 * FPU_REG_RAW_SIZE,	/* ... FP7_REGNUM.  */
  0,				/* FCTRL_REGNUM (16 bits).  */
  4,				/* FSTAT_REGNUM (16 bits).  */
  8,				/* FTAG_REGNUM (16 bits).  */
  16,				/* FISEG_REGNUM (16 bits).  */
  12,				/* FIOFF_REGNUM.  */
  24,				/* FOSEG_REGNUM.  */
  20,				/* FOOFF_REGNUM.  */
  18				/* FOP_REGNUM (bottom 11 bits).  */
};

#define FSAVE_ADDR(fsave, regnum) (fsave + fsave_offset[regnum - FP0_REGNUM])


/* Fill register REGNUM in GDB's register array with the appropriate
   value from *FSAVE.  This function masks off any of the reserved
   bits in *FSAVE.  */

void
i387_supply_register (int regnum, char *fsave)
{
  /* Most of the FPU control registers occupy only 16 bits in
     the fsave area.  Give those a special treatment.  */
  if (regnum >= FPC_REGNUM
      && regnum != FIOFF_REGNUM && regnum != FOOFF_REGNUM)
    {
      unsigned char val[4];

      memcpy (val, FSAVE_ADDR (fsave, regnum), 2);
      val[2] = val[3] = 0;
      if (regnum == FOP_REGNUM)
	val[1] &= ((1 << 3) - 1);
      supply_register (regnum, val);
    }
  else
    supply_register (regnum, FSAVE_ADDR (fsave, regnum));
}

/* Fill GDB's register array with the floating-point register values
   in *FSAVE.  This function masks off any of the reserved
   bits in *FSAVE.  */

void
i387_supply_fsave (char *fsave)
{
  int i;

  for (i = FP0_REGNUM; i < XMM0_REGNUM; i++)
    i387_supply_register (i, fsave);
}

/* Fill register REGNUM (if it is a floating-point register) in *FSAVE
   with the value in GDB's register array.  If REGNUM is -1, do this
   for all registers.  This function doesn't touch any of the reserved
   bits in *FSAVE.  */

void
i387_fill_fsave (char *fsave, int regnum)
{
  int i;

  for (i = FP0_REGNUM; i < XMM0_REGNUM; i++)
    if (regnum == -1 || regnum == i)
      {
	/* Most of the FPU control registers occupy only 16 bits in
           the fsave area.  Give those a special treatment.  */
	if (i >= FPC_REGNUM
	    && i != FIOFF_REGNUM && i != FOOFF_REGNUM)
	  {
	    unsigned char buf[4];

	    regcache_collect (i, buf);

	    if (i == FOP_REGNUM)
	      {
		/* The opcode occupies only 11 bits.  Make sure we
                   don't touch the other bits.  */
		buf[1] &= ((1 << 3) - 1);
		buf[1] |= ((FSAVE_ADDR (fsave, i))[1] & ~((1 << 3) - 1));
	      }
	    memcpy (FSAVE_ADDR (fsave, i), buf, 2);
	  }
	else
	  regcache_collect (i, FSAVE_ADDR (fsave, i));
      }
}


/* At fxsave_offset[REGNUM] you'll find the offset to the location in
   the data structure used by the "fxsave" instruction where GDB
   register REGNUM is stored.  */

static int fxsave_offset[] =
{
  32,				/* FP0_REGNUM through ...  */
  48,
  64,
  80,
  96,
  112,
  128,
  144,				/* ... FP7_REGNUM (80 bits each).  */
  0,				/* FCTRL_REGNUM (16 bits).  */
  2,				/* FSTAT_REGNUM (16 bits).  */
  4,				/* FTAG_REGNUM (16 bits).  */
  12,				/* FISEG_REGNUM (16 bits).  */
  8,				/* FIOFF_REGNUM.  */
  20,				/* FOSEG_REGNUM (16 bits).  */
  16,				/* FOOFF_REGNUM.  */
  6,				/* FOP_REGNUM (bottom 11 bits).  */
  160,				/* XMM0_REGNUM through ...  */
  176,
  192,
  208,
  224,
  240,
  256,
  272,				/* ... XMM7_REGNUM (128 bits each).  */
  24,				/* MXCSR_REGNUM.  */
};

#define FXSAVE_ADDR(fxsave, regnum) \
  (fxsave + fxsave_offset[regnum - FP0_REGNUM])

static int i387_tag (unsigned char *raw);


/* Fill GDB's register array with the floating-point and SSE register
   values in *FXSAVE.  This function masks off any of the reserved
   bits in *FXSAVE.  */

void
i387_supply_fxsave (char *fxsave)
{
  int i, last_regnum = MXCSR_REGNUM;

  if (gdbarch_tdep (current_gdbarch)->num_xmm_regs == 0)
    last_regnum = FOP_REGNUM;

  for (i = FP0_REGNUM; i <= last_regnum; i++)
    {
      /* Most of the FPU control registers occupy only 16 bits in
	 the fxsave area.  Give those a special treatment.  */
      if (i >= FPC_REGNUM && i < XMM0_REGNUM
	  && i != FIOFF_REGNUM && i != FOOFF_REGNUM)
	{
	  unsigned char val[4];

	  memcpy (val, FXSAVE_ADDR (fxsave, i), 2);
	  val[2] = val[3] = 0;
	  if (i == FOP_REGNUM)
	    val[1] &= ((1 << 3) - 1);
	  else if (i== FTAG_REGNUM)
	    {
	      /* The fxsave area contains a simplified version of the
                 tag word.  We have to look at the actual 80-bit FP
                 data to recreate the traditional i387 tag word.  */

	      unsigned long ftag = 0;
	      int fpreg;
	      int top;

	      top = (((FXSAVE_ADDR (fxsave, FSTAT_REGNUM))[1] >> 3) & 0x7);

	      for (fpreg = 7; fpreg >= 0; fpreg--)
		{
		  int tag;

		  if (val[0] & (1 << fpreg))
		    {
		      int regnum = (fpreg + 8 - top) % 8 + FP0_REGNUM;
		      tag = i387_tag (FXSAVE_ADDR (fxsave, regnum));
		    }
		  else
		    tag = 3;		/* Empty */

		  ftag |= tag << (2 * fpreg);
		}
	      val[0] = ftag & 0xff;
	      val[1] = (ftag >> 8) & 0xff;
	    }
	  supply_register (i, val);
	}
      else
	supply_register (i, FXSAVE_ADDR (fxsave, i));
    }
}

/* Fill register REGNUM (if it is a floating-point or SSE register) in
   *FXSAVE with the value in GDB's register array.  If REGNUM is -1, do
   this for all registers.  This function doesn't touch any of the
   reserved bits in *FXSAVE.  */

void
i387_fill_fxsave (char *fxsave, int regnum)
{
  int i, last_regnum = MXCSR_REGNUM;

  if (gdbarch_tdep (current_gdbarch)->num_xmm_regs == 0)
    last_regnum = FOP_REGNUM;

  for (i = FP0_REGNUM; i <= last_regnum; i++)
    if (regnum == -1 || regnum == i)
      {
	/* Most of the FPU control registers occupy only 16 bits in
           the fxsave area.  Give those a special treatment.  */
	if (i >= FPC_REGNUM && i < XMM0_REGNUM
	    && i != FIOFF_REGNUM && i != FDOFF_REGNUM)
	  {
	    unsigned char buf[4];

	    regcache_collect (i, buf);

	    if (i == FOP_REGNUM)
	      {
		/* The opcode occupies only 11 bits.  Make sure we
                   don't touch the other bits.  */
		buf[1] &= ((1 << 3) - 1);
		buf[1] |= ((FXSAVE_ADDR (fxsave, i))[1] & ~((1 << 3) - 1));
	      }
	    else if (i == FTAG_REGNUM)
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
	    memcpy (FXSAVE_ADDR (fxsave, i), buf, 2);
	  }
	else
	  regcache_collect (i, FXSAVE_ADDR (fxsave, i));
      }
}

/* Recreate the FTW (tag word) valid bits from the 80-bit FP data in
   *RAW.  */

static int
i387_tag (unsigned char *raw)
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
