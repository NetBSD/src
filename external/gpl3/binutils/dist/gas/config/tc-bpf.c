/* tc-bpf.c -- Assembler for the Linux eBPF.
   Copyright (C) 2019-2022 Free Software Foundation, Inc.
   Contributed by Oracle, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "as.h"
#include "subsegs.h"
#include "symcat.h"
#include "opcodes/bpf-desc.h"
#include "opcodes/bpf-opc.h"
#include "cgen.h"
#include "elf/common.h"
#include "elf/bpf.h"
#include "dwarf2dbg.h"

const char comment_chars[]        = ";";
const char line_comment_chars[] = "#";
const char line_separator_chars[] = "`";
const char EXP_CHARS[]            = "eE";
const char FLT_CHARS[]            = "fFdD";

/* Like s_lcomm_internal in gas/read.c but the alignment string
   is allowed to be optional.  */

static symbolS *
pe_lcomm_internal (int needs_align, symbolS *symbolP, addressT size)
{
  addressT align = 0;

  SKIP_WHITESPACE ();

  if (needs_align
      && *input_line_pointer == ',')
    {
      align = parse_align (needs_align - 1);

      if (align == (addressT) -1)
	return NULL;
    }
  else
    {
      if (size >= 8)
	align = 3;
      else if (size >= 4)
	align = 2;
      else if (size >= 2)
	align = 1;
      else
	align = 0;
    }

  bss_alloc (symbolP, size, align);
  return symbolP;
}

static void
pe_lcomm (int needs_align)
{
  s_comm_internal (needs_align * 2, pe_lcomm_internal);
}

/* The target specific pseudo-ops which we support.  */
const pseudo_typeS md_pseudo_table[] =
{
    { "half",      cons,              2 },
    { "word",      cons,              4 },
    { "dword",     cons,              8 },
    { "lcomm",	   pe_lcomm,	      1 },
    { NULL,        NULL,              0 }
};



/* ISA handling.  */
static CGEN_BITSET *bpf_isa;



/* Command-line options processing.  */

enum options
{
  OPTION_LITTLE_ENDIAN = OPTION_MD_BASE,
  OPTION_BIG_ENDIAN,
  OPTION_XBPF
};

struct option md_longopts[] =
{
  { "EL", no_argument, NULL, OPTION_LITTLE_ENDIAN },
  { "EB", no_argument, NULL, OPTION_BIG_ENDIAN },
  { "mxbpf", no_argument, NULL, OPTION_XBPF },
  { NULL,          no_argument, NULL, 0 },
};

size_t md_longopts_size = sizeof (md_longopts);

const char * md_shortopts = "";

extern int target_big_endian;

/* Whether target_big_endian has been set while parsing command-line
   arguments.  */
static int set_target_endian = 0;

static int target_xbpf = 0;

static int set_xbpf = 0;

int
md_parse_option (int c, const char * arg ATTRIBUTE_UNUSED)
{
  switch (c)
    {
    case OPTION_BIG_ENDIAN:
      set_target_endian = 1;
      target_big_endian = 1;
      break;
    case OPTION_LITTLE_ENDIAN:
      set_target_endian = 1;
      target_big_endian = 0;
      break;
    case OPTION_XBPF:
      set_xbpf = 1;
      target_xbpf = 1;
      break;
    default:
      return 0;
    }

  return 1;
}

void
md_show_usage (FILE * stream)
{
  fprintf (stream, _("\nBPF options:\n"));
  fprintf (stream, _("\
  --EL			generate code for a little endian machine\n\
  --EB			generate code for a big endian machine\n\
  -mxbpf                generate xBPF instructions\n"));
}


void
md_begin (void)
{
  /* Initialize the `cgen' interface.  */

  /* If not specified in the command line, use the host
     endianness.  */
  if (!set_target_endian)
    {
#ifdef WORDS_BIGENDIAN
      target_big_endian = 1;
#else
      target_big_endian = 0;
#endif
    }

  /* If not specified in the command line, use eBPF rather
     than xBPF.  */
  if (!set_xbpf)
      target_xbpf = 0;

  /* Set the ISA, which depends on the target endianness. */
  bpf_isa = cgen_bitset_create (ISA_MAX);
  if (target_big_endian)
    {
      if (target_xbpf)
	cgen_bitset_set (bpf_isa, ISA_XBPFBE);
      else
	cgen_bitset_set (bpf_isa, ISA_EBPFBE);
    }
  else
    {
      if (target_xbpf)
	cgen_bitset_set (bpf_isa, ISA_XBPFLE);
      else
	cgen_bitset_set (bpf_isa, ISA_EBPFLE);
    }

  /* Set the machine number and endian.  */
  gas_cgen_cpu_desc = bpf_cgen_cpu_open (CGEN_CPU_OPEN_ENDIAN,
                                         target_big_endian ?
                                         CGEN_ENDIAN_BIG : CGEN_ENDIAN_LITTLE,
                                         CGEN_CPU_OPEN_INSN_ENDIAN,
                                         CGEN_ENDIAN_LITTLE,
                                         CGEN_CPU_OPEN_ISAS,
                                         bpf_isa,
                                         CGEN_CPU_OPEN_END);
  bpf_cgen_init_asm (gas_cgen_cpu_desc);

  /* This is a callback from cgen to gas to parse operands.  */
  cgen_set_parse_operand_fn (gas_cgen_cpu_desc, gas_cgen_parse_operand);

  /* Set the machine type. */
  bfd_default_set_arch_mach (stdoutput, bfd_arch_bpf, bfd_mach_bpf);
}

valueT
md_section_align (segT segment, valueT size)
{
  int align = bfd_section_alignment (segment);

  return ((size + (1 << align) - 1) & -(1 << align));
}


/* Functions concerning relocs.  */

/* The location from which a PC relative jump should be calculated,
   given a PC relative reloc.  */

long
md_pcrel_from_section (fixS *fixP, segT sec)
{
  if (fixP->fx_addsy != (symbolS *) NULL
      && (! S_IS_DEFINED (fixP->fx_addsy)
          || (S_GET_SEGMENT (fixP->fx_addsy) != sec)
          || S_IS_EXTERNAL (fixP->fx_addsy)
          || S_IS_WEAK (fixP->fx_addsy)))
    {
        /* The symbol is undefined (or is defined but not in this section).
         Let the linker figure it out.  */
      return 0;
    }

  return fixP->fx_where + fixP->fx_frag->fr_address;
}

/* Write a value out to the object file, using the appropriate endianness.  */

void
md_number_to_chars (char * buf, valueT val, int n)
{
  if (target_big_endian)
    number_to_chars_bigendian (buf, val, n);
  else
    number_to_chars_littleendian (buf, val, n);
}

arelent *
tc_gen_reloc (asection *sec, fixS *fix)
{
  return gas_cgen_tc_gen_reloc (sec, fix);
}

/* Return the bfd reloc type for OPERAND of INSN at fixup FIXP.  This
   is called when the operand is an expression that couldn't be fully
   resolved.  Returns BFD_RELOC_NONE if no reloc type can be found.
   *FIXP may be modified if desired.  */

bfd_reloc_code_real_type
md_cgen_lookup_reloc (const CGEN_INSN *insn ATTRIBUTE_UNUSED,
		      const CGEN_OPERAND *operand,
		      fixS *fixP)
{
  switch (operand->type)
    {
    case BPF_OPERAND_OFFSET16:
      return BFD_RELOC_BPF_16;
    case BPF_OPERAND_IMM32:
      return BFD_RELOC_BPF_32;
    case BPF_OPERAND_IMM64:
      return BFD_RELOC_BPF_64;
    case BPF_OPERAND_DISP16:
      fixP->fx_pcrel = 1;
      return BFD_RELOC_BPF_DISP16;
    case BPF_OPERAND_DISP32:
      fixP->fx_pcrel = 1;
      return BFD_RELOC_BPF_DISP32;
    default:
      break;
    }
  return BFD_RELOC_NONE;
}

/* *FRAGP has been relaxed to its final size, and now needs to have
   the bytes inside it modified to conform to the new size.

   Called after relaxation is finished.
   fragP->fr_type == rs_machine_dependent.
   fragP->fr_subtype is the subtype of what the address relaxed to.  */

void
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED,
		 segT sec ATTRIBUTE_UNUSED,
		 fragS *fragP ATTRIBUTE_UNUSED)
{
  as_fatal (_("convert_frag called"));
}

int
md_estimate_size_before_relax (fragS *fragP ATTRIBUTE_UNUSED,
                               segT segment ATTRIBUTE_UNUSED)
{
  as_fatal (_("estimate_size_before_relax called"));
  return 0;
}


void
md_apply_fix (fixS *fixP, valueT *valP, segT seg)
{
  /* Some fixups for instructions require special attention.  This is
     handled in the code block below.  */
  if ((int) fixP->fx_r_type >= (int) BFD_RELOC_UNUSED)
    {
      int opindex = (int) fixP->fx_r_type - (int) BFD_RELOC_UNUSED;
      const CGEN_OPERAND *operand = cgen_operand_lookup_by_num (gas_cgen_cpu_desc,
                                                                opindex);
      char *where;

      switch (operand->type)
        {
        case BPF_OPERAND_DISP32:
          /* eBPF supports two kind of CALL instructions: the so
             called pseudo calls ("bpf to bpf") and external calls
             ("bpf to kernel").

             Both kind of calls use the same instruction (CALL).
             However, external calls are constructed by passing a
             constant argument to the instruction, whereas pseudo
             calls result from expressions involving symbols.  In
             practice, instructions requiring a fixup are interpreted
             as pseudo-calls.  If we are executing this code, this is
             a pseudo call.

             The kernel expects for pseudo-calls to be annotated by
             having BPF_PSEUDO_CALL in the SRC field of the
             instruction.  But beware the infamous nibble-swapping of
             eBPF and take endianness into account here.

             Note that the CALL instruction has only one operand, so
             this code is executed only once per instruction.  */
          where = fixP->fx_frag->fr_literal + fixP->fx_where + 1;
          where[0] = target_big_endian ? 0x01 : 0x10;
          /* Fallthrough.  */
        case BPF_OPERAND_DISP16:
          /* The PC-relative displacement fields in jump instructions
             shouldn't be in bytes.  Instead, they hold the number of
             64-bit words to the target, _minus one_.  */ 
          *valP = (((long) (*valP)) - 8) / 8;
          break;
        default:
          break;
        }
    }

  /* And now invoke CGEN's handler, which will eventually install
     *valP into the corresponding operand.  */
  gas_cgen_md_apply_fix (fixP, valP, seg);
}

void
md_assemble (char *str)
{
  const CGEN_INSN *insn;
  char *errmsg;
  CGEN_FIELDS fields;

#if CGEN_INT_INSN_P
  CGEN_INSN_INT buffer[CGEN_MAX_INSN_SIZE / sizeof (CGEN_INT_INSN_P)];
#else
  unsigned char buffer[CGEN_MAX_INSN_SIZE];
#endif

  gas_cgen_init_parse ();
  insn = bpf_cgen_assemble_insn (gas_cgen_cpu_desc, str, &fields,
                                  buffer, &errmsg);

  if (insn == NULL)
    {
      as_bad ("%s", errmsg);
      return;
    }

  gas_cgen_finish_insn (insn, buffer, CGEN_FIELDS_BITSIZE (&fields),
                        0, /* zero to ban relaxable insns.  */
                        NULL); /* NULL so results not returned here.  */
}

void
md_operand (expressionS *expressionP)
{
  gas_cgen_md_operand (expressionP);
}


symbolS *
md_undefined_symbol (char *name ATTRIBUTE_UNUSED)
{
  return NULL;
}


/* Turn a string in input_line_pointer into a floating point constant
   of type TYPE, and store the appropriate bytes in *LITP.  The number
   of LITTLENUMS emitted is stored in *SIZEP.  An error message is
   returned, or NULL on OK.  */

const char *
md_atof (int type, char *litP, int *sizeP)
{
  return ieee_md_atof (type, litP, sizeP, false);
}
