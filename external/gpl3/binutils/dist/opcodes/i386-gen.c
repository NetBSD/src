/* Copyright 2007, 2008  Free Software Foundation, Inc.

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include <stdio.h>
#include <errno.h>
#include "getopt.h"
#include "libiberty.h"
#include "safe-ctype.h"

#include "i386-opc.h"

#include <libintl.h>
#define _(String) gettext (String)

static const char *program_name = NULL;
static int debug = 0;

typedef struct initializer
{
  const char *name;
  const char *init;
} initializer;

static initializer cpu_flag_init [] =
{
  { "CPU_UNKNOWN_FLAGS",
    "unknown" },
  { "CPU_GENERIC32_FLAGS",
    "Cpu186|Cpu286|Cpu386" },
  { "CPU_GENERIC64_FLAGS", 
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|Cpu686|CpuP4|CpuMMX|CpuSSE|CpuSSE2|CpuLM" },
  { "CPU_NONE_FLAGS",
   "0" },
  { "CPU_I186_FLAGS",
    "Cpu186" },
  { "CPU_I286_FLAGS",
    "Cpu186|Cpu286" },
  { "CPU_I386_FLAGS",
    "Cpu186|Cpu286|Cpu386" },
  { "CPU_I486_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486" },
  { "CPU_I586_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586" },
  { "CPU_I686_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|Cpu686" },
  { "CPU_P2_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|Cpu686|CpuMMX" },
  { "CPU_P3_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|Cpu686|CpuMMX|CpuSSE" },
  { "CPU_P4_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|Cpu686|CpuP4|CpuMMX|CpuSSE|CpuSSE2" },
  { "CPU_NOCONA_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|Cpu686|CpuP4|CpuMMX|CpuSSE|CpuSSE2|CpuSSE3|CpuLM" },
  { "CPU_CORE_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|Cpu686|CpuP4|CpuMMX|CpuSSE|CpuSSE2|CpuSSE3" },
  { "CPU_CORE2_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|Cpu686|CpuP4|CpuMMX|CpuSSE|CpuSSE2|CpuSSE3|CpuSSSE3|CpuLM" },
  { "CPU_K6_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|CpuK6|CpuMMX" },
  { "CPU_K6_2_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|CpuK6|CpuMMX|Cpu3dnow" },
  { "CPU_ATHLON_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|Cpu686|CpuK6|CpuMMX|Cpu3dnow|Cpu3dnowA" },
  { "CPU_K8_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|Cpu686|CpuK6|CpuK8|CpuMMX|Cpu3dnow|Cpu3dnowA|CpuSSE|CpuSSE2|CpuLM" },
  { "CPU_AMDFAM10_FLAGS",
    "Cpu186|Cpu286|Cpu386|Cpu486|Cpu586|Cpu686|CpuK6|CpuK8|CpuMMX|Cpu3dnow|Cpu3dnowA|CpuSSE|CpuSSE2|CpuSSE3|CpuSSE4a|CpuABM|CpuLM" },
  { "CPU_MMX_FLAGS",
    "CpuMMX" },
  { "CPU_SSE_FLAGS",
    "CpuMMX|CpuSSE" },
  { "CPU_SSE2_FLAGS",
    "CpuMMX|CpuSSE|CpuSSE2" },
  { "CPU_SSE3_FLAGS",
    "CpuMMX|CpuSSE|CpuSSE2|CpuSSE3" },
  { "CPU_SSSE3_FLAGS",
    "CpuMMX|CpuSSE|CpuSSE2|CpuSSE3|CpuSSSE3" },
  { "CPU_SSE4_1_FLAGS",
    "CpuMMX|CpuSSE|CpuSSE2|CpuSSE3|CpuSSSE3|CpuSSE4_1" },
  { "CPU_SSE4_2_FLAGS",
    "CpuMMX|CpuSSE|CpuSSE2|CpuSSE3|CpuSSSE3|CpuSSE4_1|CpuSSE4_2" },
  { "CPU_VMX_FLAGS",
    "CpuVMX" },
  { "CPU_SMX_FLAGS",
    "CpuSMX" },
  { "CPU_XSAVE_FLAGS",
    "CpuXsave" },
  { "CPU_AES_FLAGS",
    "CpuMMX|CpuSSE|CpuSSE2|CpuSSE3|CpuSSSE3|CpuSSE4_1|CpuSSE4_2|CpuAES" },
  { "CPU_PCLMUL_FLAGS",
    "CpuMMX|CpuSSE|CpuSSE2|CpuSSE3|CpuSSSE3|CpuSSE4_1|CpuSSE4_2|CpuPCLMUL" },
  { "CPU_FMA_FLAGS",
    "CpuMMX|CpuSSE|CpuSSE2|CpuSSE3|CpuSSSE3|CpuSSE4_1|CpuSSE4_2|CpuAVX|CpuFMA" },
  { "CPU_MOVBE_FLAGS",
    "CpuMovbe" },
  { "CPU_EPT_FLAGS",
    "CpuEPT" },
  { "CPU_3DNOW_FLAGS",
    "CpuMMX|Cpu3dnow" },
  { "CPU_3DNOWA_FLAGS",
    "CpuMMX|Cpu3dnow|Cpu3dnowA" },
  { "CPU_PADLOCK_FLAGS",
    "CpuPadLock" },
  { "CPU_SVME_FLAGS",
    "CpuSVME" },
  { "CPU_SSE4A_FLAGS",
    "CpuMMX|CpuSSE|CpuSSE2|CpuSSE3|CpuSSE4a" },
  { "CPU_ABM_FLAGS",
    "CpuABM" },
  { "CPU_SSE5_FLAGS",
    "CpuMMX|CpuSSE|CpuSSE2|CpuSSE3|CpuSSE4a|CpuABM|CpuSSE5"},
  { "CPU_AVX_FLAGS",
    "CpuMMX|CpuSSE|CpuSSE2|CpuSSE3|CpuSSSE3|CpuSSE4_1|CpuSSE4_2|CpuAVX" },
};

static initializer operand_type_init [] =
{
  { "OPERAND_TYPE_NONE",
    "0" },
  { "OPERAND_TYPE_REG8",
    "Reg8" },
  { "OPERAND_TYPE_REG16",
    "Reg16" },
  { "OPERAND_TYPE_REG32",
    "Reg32" },
  { "OPERAND_TYPE_REG64",
    "Reg64" },
  { "OPERAND_TYPE_IMM1",
    "Imm1" },
  { "OPERAND_TYPE_IMM8",
    "Imm8" },
  { "OPERAND_TYPE_IMM8S",
    "Imm8S" },
  { "OPERAND_TYPE_IMM16",
    "Imm16" },
  { "OPERAND_TYPE_IMM32",
    "Imm32" },
  { "OPERAND_TYPE_IMM32S",
    "Imm32S" },
  { "OPERAND_TYPE_IMM64",
    "Imm64" },
  { "OPERAND_TYPE_BASEINDEX",
    "BaseIndex" },
  { "OPERAND_TYPE_DISP8",
    "Disp8" },
  { "OPERAND_TYPE_DISP16",
    "Disp16" },
  { "OPERAND_TYPE_DISP32",
    "Disp32" },
  { "OPERAND_TYPE_DISP32S",
    "Disp32S" },
  { "OPERAND_TYPE_DISP64",
    "Disp64" },
  { "OPERAND_TYPE_INOUTPORTREG",
    "InOutPortReg" },
  { "OPERAND_TYPE_SHIFTCOUNT",
    "ShiftCount" },
  { "OPERAND_TYPE_CONTROL",
    "Control" },
  { "OPERAND_TYPE_TEST",
    "Test" },
  { "OPERAND_TYPE_DEBUG",
    "FloatReg" },
  { "OPERAND_TYPE_FLOATREG",
    "FloatReg" },
  { "OPERAND_TYPE_FLOATACC",
    "FloatAcc" },
  { "OPERAND_TYPE_SREG2",
    "SReg2" },
  { "OPERAND_TYPE_SREG3",
    "SReg3" },
  { "OPERAND_TYPE_ACC",
    "Acc" },
  { "OPERAND_TYPE_JUMPABSOLUTE",
    "JumpAbsolute" },
  { "OPERAND_TYPE_REGMMX",
    "RegMMX" },
  { "OPERAND_TYPE_REGXMM",
    "RegXMM" },
  { "OPERAND_TYPE_REGYMM",
    "RegYMM" },
  { "OPERAND_TYPE_ESSEG",
    "EsSeg" },
  { "OPERAND_TYPE_ACC32",
    "Reg32|Acc|Dword" },
  { "OPERAND_TYPE_ACC64",
    "Reg64|Acc|Qword" },
  { "OPERAND_TYPE_INOUTPORTREG",
    "InOutPortReg" },
  { "OPERAND_TYPE_REG16_INOUTPORTREG",
    "Reg16|InOutPortReg" },
  { "OPERAND_TYPE_DISP16_32",
    "Disp16|Disp32" },
  { "OPERAND_TYPE_ANYDISP",
    "Disp8|Disp16|Disp32|Disp32S|Disp64" },
  { "OPERAND_TYPE_IMM16_32",
    "Imm16|Imm32" },
  { "OPERAND_TYPE_IMM16_32S",
    "Imm16|Imm32S" },
  { "OPERAND_TYPE_IMM16_32_32S",
    "Imm16|Imm32|Imm32S" },
  { "OPERAND_TYPE_IMM32_32S_DISP32",
    "Imm32|Imm32S|Disp32" },
  { "OPERAND_TYPE_IMM64_DISP64",
    "Imm64|Disp64" },
  { "OPERAND_TYPE_IMM32_32S_64_DISP32",
    "Imm32|Imm32S|Imm64|Disp32" },
  { "OPERAND_TYPE_IMM32_32S_64_DISP32_64",
    "Imm32|Imm32S|Imm64|Disp32|Disp64" },
  { "OPERAND_TYPE_VEX_IMM4",
    "VEX_Imm4" },
};

typedef struct bitfield
{
  int position;
  int value;
  const char *name;
} bitfield;

#define BITFIELD(n) { n, 0, #n }

static bitfield cpu_flags[] =
{
  BITFIELD (Cpu186),
  BITFIELD (Cpu286),
  BITFIELD (Cpu386),
  BITFIELD (Cpu486),
  BITFIELD (Cpu586),
  BITFIELD (Cpu686),
  BITFIELD (CpuP4),
  BITFIELD (CpuK6),
  BITFIELD (CpuK8),
  BITFIELD (CpuMMX),
  BITFIELD (CpuSSE),
  BITFIELD (CpuSSE2),
  BITFIELD (CpuSSE3),
  BITFIELD (CpuSSSE3),
  BITFIELD (CpuSSE4_1),
  BITFIELD (CpuSSE4_2),
  BITFIELD (CpuAVX),
  BITFIELD (CpuSSE4a),
  BITFIELD (CpuSSE5),
  BITFIELD (Cpu3dnow),
  BITFIELD (Cpu3dnowA),
  BITFIELD (CpuPadLock),
  BITFIELD (CpuSVME),
  BITFIELD (CpuVMX),
  BITFIELD (CpuSMX),
  BITFIELD (CpuABM),
  BITFIELD (CpuXsave),
  BITFIELD (CpuAES),
  BITFIELD (CpuPCLMUL),
  BITFIELD (CpuFMA),
  BITFIELD (CpuLM),
  BITFIELD (CpuMovbe),
  BITFIELD (CpuEPT),
  BITFIELD (Cpu64),
  BITFIELD (CpuNo64),
#ifdef CpuUnused
  BITFIELD (CpuUnused),
#endif
};

static bitfield opcode_modifiers[] =
{
  BITFIELD (D),
  BITFIELD (W),
  BITFIELD (Modrm),
  BITFIELD (ShortForm),
  BITFIELD (Jump),
  BITFIELD (JumpDword),
  BITFIELD (JumpByte),
  BITFIELD (JumpInterSegment),
  BITFIELD (FloatMF),
  BITFIELD (FloatR),
  BITFIELD (FloatD),
  BITFIELD (Size16),
  BITFIELD (Size32),
  BITFIELD (Size64),
  BITFIELD (IgnoreSize),
  BITFIELD (DefaultSize),
  BITFIELD (No_bSuf),
  BITFIELD (No_wSuf),
  BITFIELD (No_lSuf),
  BITFIELD (No_sSuf),
  BITFIELD (No_qSuf),
  BITFIELD (No_ldSuf),
  BITFIELD (FWait),
  BITFIELD (IsString),
  BITFIELD (RegKludge),
  BITFIELD (FirstXmm0),
  BITFIELD (Implicit1stXmm0),
  BITFIELD (ByteOkIntel),
  BITFIELD (ToDword),
  BITFIELD (ToQword),
  BITFIELD (AddrPrefixOp0),
  BITFIELD (IsPrefix),
  BITFIELD (ImmExt),
  BITFIELD (NoRex64),
  BITFIELD (Rex64),
  BITFIELD (Ugh),
  BITFIELD (Drex),
  BITFIELD (Drexv),
  BITFIELD (Drexc),
  BITFIELD (Vex),
  BITFIELD (Vex256),
  BITFIELD (VexNDD),
  BITFIELD (VexNDS),
  BITFIELD (VexW0),
  BITFIELD (VexW1),
  BITFIELD (Vex0F),
  BITFIELD (Vex0F38),
  BITFIELD (Vex0F3A),
  BITFIELD (Vex3Sources),
  BITFIELD (VexImmExt),
  BITFIELD (SSE2AVX),
  BITFIELD (NoAVX),
  BITFIELD (OldGcc),
  BITFIELD (ATTMnemonic),
  BITFIELD (ATTSyntax),
  BITFIELD (IntelSyntax),
};

static bitfield operand_types[] =
{
  BITFIELD (Reg8),
  BITFIELD (Reg16),
  BITFIELD (Reg32),
  BITFIELD (Reg64),
  BITFIELD (FloatReg),
  BITFIELD (RegMMX),
  BITFIELD (RegXMM),
  BITFIELD (RegYMM),
  BITFIELD (Imm8),
  BITFIELD (Imm8S),
  BITFIELD (Imm16),
  BITFIELD (Imm32),
  BITFIELD (Imm32S),
  BITFIELD (Imm64),
  BITFIELD (Imm1),
  BITFIELD (BaseIndex),
  BITFIELD (Disp8),
  BITFIELD (Disp16),
  BITFIELD (Disp32),
  BITFIELD (Disp32S),
  BITFIELD (Disp64),
  BITFIELD (InOutPortReg),
  BITFIELD (ShiftCount),
  BITFIELD (Control),
  BITFIELD (Debug),
  BITFIELD (Test),
  BITFIELD (SReg2),
  BITFIELD (SReg3),
  BITFIELD (Acc),
  BITFIELD (FloatAcc),
  BITFIELD (JumpAbsolute),
  BITFIELD (EsSeg),
  BITFIELD (RegMem),
  BITFIELD (Mem),
  BITFIELD (Byte),
  BITFIELD (Word),
  BITFIELD (Dword),
  BITFIELD (Fword),
  BITFIELD (Qword),
  BITFIELD (Tbyte),
  BITFIELD (Xmmword),
  BITFIELD (Ymmword),
  BITFIELD (Unspecified),
  BITFIELD (Anysize),
  BITFIELD (Vex_Imm4),
#ifdef OTUnused
  BITFIELD (OTUnused),
#endif
};

static int lineno;
static const char *filename;

static int
compare (const void *x, const void *y)
{
  const bitfield *xp = (const bitfield *) x;
  const bitfield *yp = (const bitfield *) y;
  return xp->position - yp->position;
}

static void
fail (const char *message, ...)
{
  va_list args;
  
  va_start (args, message);
  fprintf (stderr, _("%s: Error: "), program_name);
  vfprintf (stderr, message, args);
  va_end (args);
  xexit (1);
}

static void
process_copyright (FILE *fp)
{
  fprintf (fp, "/* This file is automatically generated by i386-gen.  Do not edit!  */\n\
/* Copyright 2007, 2008  Free Software Foundation, Inc.\n\
\n\
   This file is part of the GNU opcodes library.\n\
\n\
   This library is free software; you can redistribute it and/or modify\n\
   it under the terms of the GNU General Public License as published by\n\
   the Free Software Foundation; either version 3, or (at your option)\n\
   any later version.\n\
\n\
   It is distributed in the hope that it will be useful, but WITHOUT\n\
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY\n\
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public\n\
   License for more details.\n\
\n\
   You should have received a copy of the GNU General Public License\n\
   along with this program; if not, write to the Free Software\n\
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,\n\
   MA 02110-1301, USA.  */\n");
}

/* Remove leading white spaces.  */

static char *
remove_leading_whitespaces (char *str)
{
  while (ISSPACE (*str))
    str++;
  return str;
}

/* Remove trailing white spaces.  */

static void
remove_trailing_whitespaces (char *str)
{
  size_t last = strlen (str);

  if (last == 0)
    return;

  do
    {
      last--;
      if (ISSPACE (str [last]))
	str[last] = '\0';
      else
	break;
    }
  while (last != 0);
}

/* Find next field separated by SEP and terminate it. Return a
   pointer to the one after it.  */

static char *
next_field (char *str, char sep, char **next)
{
  char *p;

  p = remove_leading_whitespaces (str);
  for (str = p; *str != sep && *str != '\0'; str++);

  *str = '\0';
  remove_trailing_whitespaces (p);

  *next = str + 1; 

  return p;
}

static void
set_bitfield (const char *f, bitfield *array, unsigned int size)
{
  unsigned int i;

  if (strcmp (f, "CpuSledgehammer") == 0)
    f= "CpuK8";
  else if (strcmp (f, "Mmword") == 0)
    f= "Qword";
  else if (strcmp (f, "Oword") == 0)
    f= "Xmmword";

  for (i = 0; i < size; i++)
    if (strcasecmp (array[i].name, f) == 0)
      {
	array[i].value = 1;
	return;
      }

  fail (_("%s: %d: Unknown bitfield: %s\n"), filename, lineno, f);
}

static void
output_cpu_flags (FILE *table, bitfield *flags, unsigned int size,
		  int macro, const char *comma, const char *indent)
{
  unsigned int i;

  fprintf (table, "%s{ { ", indent);

  for (i = 0; i < size - 1; i++)
    {
      fprintf (table, "%d, ", flags[i].value);
      if (((i + 1) % 20) == 0)
	{
	  /* We need \\ for macro.  */
	  if (macro)
	    fprintf (table, " \\\n    %s", indent);
	  else
	    fprintf (table, "\n    %s", indent);
	}
    }

  fprintf (table, "%d } }%s\n", flags[i].value, comma);
}

static void
process_i386_cpu_flag (FILE *table, char *flag, int macro,
		       const char *comma, const char *indent)
{
  char *str, *next, *last;
  bitfield flags [ARRAY_SIZE (cpu_flags)];

  /* Copy the default cpu flags.  */
  memcpy (flags, cpu_flags, sizeof (cpu_flags));

  if (strcasecmp (flag, "unknown") == 0)
    {
      unsigned int i;

      /* We turn on everything except for cpu64 in case of
	 CPU_UNKNOWN_FLAGS. */
      for (i = 0; i < ARRAY_SIZE (flags); i++)
	if (flags[i].position != Cpu64)
	  flags[i].value = 1;
    }
  else if (strcmp (flag, "0"))
    {
      last = flag + strlen (flag);
      for (next = flag; next && next < last; )
	{
	  str = next_field (next, '|', &next);
	  if (str)
	    set_bitfield (str, flags, ARRAY_SIZE (flags));
	}
    }

  output_cpu_flags (table, flags, ARRAY_SIZE (flags), macro,
		    comma, indent);
}

static void
output_opcode_modifier (FILE *table, bitfield *modifier, unsigned int size)
{
  unsigned int i;

  fprintf (table, "    { ");

  for (i = 0; i < size - 1; i++)
    {
      fprintf (table, "%d, ", modifier[i].value);
      if (((i + 1) % 20) == 0)
	fprintf (table, "\n      ");
    }

  fprintf (table, "%d },\n", modifier[i].value);
}

static void
process_i386_opcode_modifier (FILE *table, char *mod)
{
  char *str, *next, *last;
  bitfield modifiers [ARRAY_SIZE (opcode_modifiers)];

  /* Copy the default opcode modifier.  */
  memcpy (modifiers, opcode_modifiers, sizeof (modifiers));

  if (strcmp (mod, "0"))
    {
      last = mod + strlen (mod);
      for (next = mod; next && next < last; )
	{
	  str = next_field (next, '|', &next);
	  if (str)
	    set_bitfield (str, modifiers, ARRAY_SIZE (modifiers));
	}
    }
  output_opcode_modifier (table, modifiers, ARRAY_SIZE (modifiers));
}

static void
output_operand_type (FILE *table, bitfield *types, unsigned int size,
		     int macro, const char *indent)
{
  unsigned int i;

  fprintf (table, "{ { ");

  for (i = 0; i < size - 1; i++)
    {
      fprintf (table, "%d, ", types[i].value);
      if (((i + 1) % 20) == 0)
	{
	  /* We need \\ for macro.  */
	  if (macro)
	    fprintf (table, "\\\n%s", indent);
	  else
	    fprintf (table, "\n%s", indent);
	}
    }

  fprintf (table, "%d } }", types[i].value);
}

static void
process_i386_operand_type (FILE *table, char *op, int macro,
			   const char *indent)
{
  char *str, *next, *last;
  bitfield types [ARRAY_SIZE (operand_types)];

  /* Copy the default operand type.  */
  memcpy (types, operand_types, sizeof (types));

  if (strcmp (op, "0"))
    {
      last = op + strlen (op);
      for (next = op; next && next < last; )
	{
	  str = next_field (next, '|', &next);
	  if (str)
	    set_bitfield (str, types, ARRAY_SIZE (types));
	}
    }
  output_operand_type (table, types, ARRAY_SIZE (types), macro,
		       indent);
}

static void
process_i386_opcodes (FILE *table)
{
  FILE *fp;
  char buf[2048];
  unsigned int i;
  char *str, *p, *last;
  char *name, *operands, *base_opcode, *extension_opcode;
  char *opcode_length;
  char *cpu_flags, *opcode_modifier, *operand_types [MAX_OPERANDS];

  filename = "i386-opc.tbl";
  fp = fopen (filename, "r");

  if (fp == NULL)
    fail (_("can't find i386-opc.tbl for reading, errno = %s\n"),
	  xstrerror (errno));

  fprintf (table, "\n/* i386 opcode table.  */\n\n");
  fprintf (table, "const template i386_optab[] =\n{\n");

  while (!feof (fp))
    {
      if (fgets (buf, sizeof (buf), fp) == NULL)
	break;

      lineno++;

      p = remove_leading_whitespaces (buf);

      /* Skip comments.  */
      str = strstr (p, "//");
      if (str != NULL)
	str[0] = '\0';

      /* Remove trailing white spaces.  */
      remove_trailing_whitespaces (p);

      switch (p[0])
	{
	case '#':
	  fprintf (table, "%s\n", p);
	case '\0':
	  continue;
	  break;
	default:
	  break;
	}

      last = p + strlen (p);

      /* Find name.  */
      name = next_field (p, ',', &str);

      if (str >= last)
	abort ();

      /* Find number of operands.  */
      operands = next_field (str, ',', &str);

      if (str >= last)
	abort ();

      /* Find base_opcode.  */
      base_opcode = next_field (str, ',', &str);

      if (str >= last)
	abort ();

      /* Find extension_opcode.  */
      extension_opcode = next_field (str, ',', &str);

      if (str >= last)
	abort ();

      /* Find opcode_length.  */
      opcode_length = next_field (str, ',', &str);

      if (str >= last)
	abort ();

      /* Find cpu_flags.  */
      cpu_flags = next_field (str, ',', &str);

      if (str >= last)
	abort ();

      /* Find opcode_modifier.  */
      opcode_modifier = next_field (str, ',', &str);

      if (str >= last)
	abort ();

      /* Remove the first {.  */
      str = remove_leading_whitespaces (str);
      if (*str != '{')
	abort ();
      str = remove_leading_whitespaces (str + 1);

      i = strlen (str);

      /* There are at least "X}".  */
      if (i < 2)
	abort ();

      /* Remove trailing white spaces and }. */
      do
	{
	  i--;
	  if (ISSPACE (str[i]) || str[i] == '}')
	    str[i] = '\0';
	  else
	    break;
	}
      while (i != 0);

      last = str + i;

      /* Find operand_types.  */
      for (i = 0; i < ARRAY_SIZE (operand_types); i++)
	{
	  if (str >= last)
	    {
	      operand_types [i] = NULL;
	      break;
	    }

	  operand_types [i] = next_field (str, ',', &str);
	  if (*operand_types[i] == '0')
	    {
	      if (i != 0)
		operand_types[i] = NULL;
	      break;
	    }
	}

      fprintf (table, "  { \"%s\", %s, %s, %s, %s,\n",
	       name, operands, base_opcode, extension_opcode,
	       opcode_length);

      process_i386_cpu_flag (table, cpu_flags, 0, ",", "    ");

      process_i386_opcode_modifier (table, opcode_modifier);

      fprintf (table, "    { ");

      for (i = 0; i < ARRAY_SIZE (operand_types); i++)
	{
	  if (operand_types[i] == NULL
	      || *operand_types[i] == '0')
	    {
	      if (i == 0)
		process_i386_operand_type (table, "0", 0, "\t  ");
	      break;
	    }

	  if (i != 0)
	    fprintf (table, ",\n      ");

	  process_i386_operand_type (table, operand_types[i], 0,
				     "\t  ");
	}
      fprintf (table, " } },\n");
    }

  fclose (fp);

  fprintf (table, "  { NULL, 0, 0, 0, 0,\n");

  process_i386_cpu_flag (table, "0", 0, ",", "    ");

  process_i386_opcode_modifier (table, "0");
 
  fprintf (table, "    { ");
  process_i386_operand_type (table, "0", 0, "\t  ");
  fprintf (table, " } }\n");

  fprintf (table, "};\n");
}

static void
process_i386_registers (FILE *table)
{
  FILE *fp;
  char buf[2048];
  char *str, *p, *last;
  char *reg_name, *reg_type, *reg_flags, *reg_num;
  char *dw2_32_num, *dw2_64_num;

  filename = "i386-reg.tbl";
  fp = fopen (filename, "r");
  if (fp == NULL)
    fail (_("can't find i386-reg.tbl for reading, errno = %s\n"),
	  xstrerror (errno));

  fprintf (table, "\n/* i386 register table.  */\n\n");
  fprintf (table, "const reg_entry i386_regtab[] =\n{\n");

  while (!feof (fp))
    {
      if (fgets (buf, sizeof (buf), fp) == NULL)
	break;

      lineno++;

      p = remove_leading_whitespaces (buf);

      /* Skip comments.  */
      str = strstr (p, "//");
      if (str != NULL)
	str[0] = '\0';

      /* Remove trailing white spaces.  */
      remove_trailing_whitespaces (p);

      switch (p[0])
	{
	case '#':
	  fprintf (table, "%s\n", p);
	case '\0':
	  continue;
	  break;
	default:
	  break;
	}

      last = p + strlen (p);

      /* Find reg_name.  */
      reg_name = next_field (p, ',', &str);

      if (str >= last)
	abort ();

      /* Find reg_type.  */
      reg_type = next_field (str, ',', &str);

      if (str >= last)
	abort ();

      /* Find reg_flags.  */
      reg_flags = next_field (str, ',', &str);

      if (str >= last)
	abort ();

      /* Find reg_num.  */
      reg_num = next_field (str, ',', &str);

      if (str >= last)
	abort ();

      fprintf (table, "  { \"%s\",\n    ", reg_name);

      process_i386_operand_type (table, reg_type, 0, "\t");

      /* Find 32-bit Dwarf2 register number.  */
      dw2_32_num = next_field (str, ',', &str);

      if (str >= last)
	abort ();

      /* Find 64-bit Dwarf2 register number.  */
      dw2_64_num = next_field (str, ',', &str);

      fprintf (table, ",\n    %s, %s, { %s, %s } },\n",
	       reg_flags, reg_num, dw2_32_num, dw2_64_num);
    }

  fclose (fp);

  fprintf (table, "};\n");

  fprintf (table, "\nconst unsigned int i386_regtab_size = ARRAY_SIZE (i386_regtab);\n");
}

static void
process_i386_initializers (void)
{
  unsigned int i;
  FILE *fp = fopen ("i386-init.h", "w");
  char *init;

  if (fp == NULL)
    fail (_("can't create i386-init.h, errno = %s\n"),
	  xstrerror (errno));

  process_copyright (fp);

  for (i = 0; i < ARRAY_SIZE (cpu_flag_init); i++)
    {
      fprintf (fp, "\n#define %s \\\n", cpu_flag_init[i].name);
      init = xstrdup (cpu_flag_init[i].init);
      process_i386_cpu_flag (fp, init, 1, "", "  ");
      free (init);
    }

  for (i = 0; i < ARRAY_SIZE (operand_type_init); i++)
    {
      fprintf (fp, "\n\n#define %s \\\n  ", operand_type_init[i].name);
      init = xstrdup (operand_type_init[i].init);
      process_i386_operand_type (fp, init, 1, "      ");
      free (init);
    }
  fprintf (fp, "\n");

  fclose (fp);
}

/* Program options.  */
#define OPTION_SRCDIR	200

struct option long_options[] = 
{
  {"srcdir",  required_argument, NULL, OPTION_SRCDIR},
  {"debug",   no_argument,       NULL, 'd'},
  {"version", no_argument,       NULL, 'V'},
  {"help",    no_argument,       NULL, 'h'},
  {0,         no_argument,       NULL, 0}
};

static void
print_version (void)
{
  printf ("%s: version 1.0\n", program_name);
  xexit (0);
}

static void
usage (FILE * stream, int status)
{
  fprintf (stream, "Usage: %s [-V | --version] [-d | --debug] [--srcdir=dirname] [--help]\n",
	   program_name);
  xexit (status);
}

int
main (int argc, char **argv)
{
  extern int chdir (char *);
  char *srcdir = NULL;
  int c;
  FILE *table;
  
  program_name = *argv;
  xmalloc_set_program_name (program_name);

  while ((c = getopt_long (argc, argv, "vVdh", long_options, 0)) != EOF)
    switch (c)
      {
      case OPTION_SRCDIR:
	srcdir = optarg;
	break;
      case 'V':
      case 'v':
	print_version ();
	break;
      case 'd':
	debug = 1;
	break;
      case 'h':
      case '?':
	usage (stderr, 0);
      default:
      case 0:
	break;
      }

  if (optind != argc)
    usage (stdout, 1);

  if (srcdir != NULL) 
    if (chdir (srcdir) != 0)
      fail (_("unable to change directory to \"%s\", errno = %s\n"),
	    srcdir, xstrerror (errno));

  /* Check the unused bitfield in i386_cpu_flags.  */
#ifndef CpuUnused
  c = CpuNumOfBits - CpuMax - 1;
  if (c)
    fail (_("%d unused bits in i386_cpu_flags.\n"), c);
#endif

  /* Check the unused bitfield in i386_operand_type.  */
#ifndef OTUnused
  c = OTNumOfBits - OTMax - 1;
  if (c)
    fail (_("%d unused bits in i386_operand_type.\n"), c);
#endif

  qsort (cpu_flags, ARRAY_SIZE (cpu_flags), sizeof (cpu_flags [0]),
	 compare);

  qsort (opcode_modifiers, ARRAY_SIZE (opcode_modifiers),
	 sizeof (opcode_modifiers [0]), compare);

  qsort (operand_types, ARRAY_SIZE (operand_types),
	 sizeof (operand_types [0]), compare);

  table = fopen ("i386-tbl.h", "w");
  if (table == NULL)
    fail (_("can't create i386-tbl.h, errno = %s\n"),
	  xstrerror (errno));

  process_copyright (table);

  process_i386_opcodes (table);
  process_i386_registers (table);
  process_i386_initializers ();

  fclose (table);

  exit (0);
}
