/* TI C6X assembler.
   Copyright 2010
   Free Software Foundation, Inc.
   Contributed by Joseph Myers <joseph@codesourcery.com>
   		  Bernd Schmidt  <bernds@codesourcery.com>

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
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "as.h"
#include "dwarf2dbg.h"
#include "safe-ctype.h"
#include "subsegs.h"
#include "opcode/tic6x.h"
#include "elf/tic6x.h"
#include "elf32-tic6x.h"

/* Truncate and sign-extend at 32 bits, so that building on a 64-bit
   host gives identical results to a 32-bit host.  */
#define TRUNC(X)	((valueT) (X) & 0xffffffffU)
#define SEXT(X)		((TRUNC (X) ^ 0x80000000U) - 0x80000000U)

const char comment_chars[] = ";";
const char line_comment_chars[] = "#*;";
const char line_separator_chars[] = "@";

const char EXP_CHARS[] = "eE";
const char FLT_CHARS[] = "dDfF";

const char *md_shortopts = "";

enum
  {
    OPTION_MARCH = OPTION_MD_BASE,
    OPTION_MATOMIC,
    OPTION_MNO_ATOMIC,
    OPTION_MBIG_ENDIAN,
    OPTION_MLITTLE_ENDIAN,
    OPTION_MDSBT,
    OPTION_MNO_DSBT,
    OPTION_MPID,
    OPTION_MPIC,
    OPTION_MNO_PIC,
    OPTION_MGENERATE_REL
  };

struct option md_longopts[] =
  {
    { "march", required_argument, NULL, OPTION_MARCH },
    { "matomic", no_argument, NULL, OPTION_MATOMIC },
    { "mno-atomic", no_argument, NULL, OPTION_MNO_ATOMIC },
    { "mbig-endian", no_argument, NULL, OPTION_MBIG_ENDIAN },
    { "mlittle-endian", no_argument, NULL, OPTION_MLITTLE_ENDIAN },
    { "mdsbt", no_argument, NULL, OPTION_MDSBT },
    { "mno-dsbt", no_argument, NULL, OPTION_MNO_DSBT },
    { "mpid", required_argument, NULL, OPTION_MPID },
    { "mpic", no_argument, NULL, OPTION_MPIC },
    { "mno-pic", no_argument, NULL, OPTION_MNO_PIC },
    { "mgenerate-rel", no_argument, NULL, OPTION_MGENERATE_REL },
    { NULL, no_argument, NULL, 0 }
  };
size_t md_longopts_size = sizeof (md_longopts);

/* Whether to enable atomic instructions.  1 to enable them, 0 to
   disable, -1 to default from architecture.  */
static int tic6x_atomic = -1;

/* The instructions enabled based only on the selected architecture
   (all instructions, if no architecture specified).  Atomic
   instructions may be enabled or disabled separately.  */
static unsigned short tic6x_arch_enable = (TIC6X_INSN_C62X
					   | TIC6X_INSN_C64X
					   | TIC6X_INSN_C64XP
					   | TIC6X_INSN_C67X
					   | TIC6X_INSN_C67XP
					   | TIC6X_INSN_C674X
					   | TIC6X_INSN_ATOMIC);

/* The instructions enabled based on the current set of features
   (architecture, as modified by other options).  */
static unsigned short tic6x_features;

/* The architecture attribute value, or C6XABI_Tag_ISA_none if
   not yet set.  */
static int tic6x_arch_attribute = C6XABI_Tag_ISA_none;

/* Whether any instructions at all have been seen.  Once any
   instructions have been seen, architecture attributes merge into the
   previous attribute value rather than replacing it.  */
static bfd_boolean tic6x_seen_insns = FALSE;

/* The number of registers in each register file supported by the
   current architecture.  */
static unsigned int tic6x_num_registers;

/* Whether predication on A0 is possible.  */
static bfd_boolean tic6x_predicate_a0;

/* Whether execute packets can cross fetch packet boundaries.  */
static bfd_boolean tic6x_can_cross_fp_boundary;

/* Whether there are constraints on simultaneous reads and writes of
   40-bit data.  */
static bfd_boolean tic6x_long_data_constraints;

/* Whether compact instructions are available.  */
static bfd_boolean tic6x_compact_insns;

/* Whether to generate RELA relocations.  */
static bfd_boolean tic6x_generate_rela = TRUE;

/* Whether the code uses DSBT addressing.  */
static bfd_boolean tic6x_dsbt;

/* Types of position-independent data (attribute values for
   Tag_ABI_PID).  */
typedef enum
  {
    tic6x_pid_no = 0,
    tic6x_pid_near = 1,
    tic6x_pid_far = 2
  } tic6x_pid_type;

/* The type of data addressing used in this code.  */
static tic6x_pid_type tic6x_pid;

/* Whether the code uses position-independent code.  */
static bfd_boolean tic6x_pic;

/* Table of supported architecture variants.  */
typedef struct
{
  const char *arch;
  int attr;
  unsigned short features;
} tic6x_arch_table;
static const tic6x_arch_table tic6x_arches[] =
  {
    { "c62x", C6XABI_Tag_ISA_C62X, TIC6X_INSN_C62X },
    { "c64x", C6XABI_Tag_ISA_C64X, TIC6X_INSN_C62X | TIC6X_INSN_C64X },
    { "c64x+", C6XABI_Tag_ISA_C64XP, (TIC6X_INSN_C62X
				      | TIC6X_INSN_C64X
				      | TIC6X_INSN_C64XP) },
    { "c67x", C6XABI_Tag_ISA_C67X, TIC6X_INSN_C62X | TIC6X_INSN_C67X },
    { "c67x+", C6XABI_Tag_ISA_C67XP, (TIC6X_INSN_C62X
				      | TIC6X_INSN_C67X
				      | TIC6X_INSN_C67XP) },
    { "c674x", C6XABI_Tag_ISA_C674X, (TIC6X_INSN_C62X
				      | TIC6X_INSN_C64X
				      | TIC6X_INSN_C64XP
				      | TIC6X_INSN_C67X
				      | TIC6X_INSN_C67XP
				      | TIC6X_INSN_C674X) }
  };

/* Update the selected architecture based on ARCH, giving an error if
   ARCH is an invalid value.  Does not call tic6x_update_features; the
   caller must do that if necessary.  */

static void
tic6x_use_arch (const char *arch)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (tic6x_arches); i++)
    if (strcmp (arch, tic6x_arches[i].arch) == 0)
      {
	tic6x_arch_enable = tic6x_arches[i].features;
	if (tic6x_seen_insns)
	  tic6x_arch_attribute
	    = elf32_tic6x_merge_arch_attributes (tic6x_arch_attribute,
						 tic6x_arches[i].attr);
	else
	  tic6x_arch_attribute = tic6x_arches[i].attr;
	return;
      }

  as_bad (_("unknown architecture '%s'"), arch);
}

/* Table of supported -mpid arguments.  */
typedef struct
{
  const char *arg;
  tic6x_pid_type attr;
} tic6x_pid_type_table;
static const tic6x_pid_type_table tic6x_pid_types[] =
  {
    { "no", tic6x_pid_no },
    { "near", tic6x_pid_near },
    { "far", tic6x_pid_far }
  };

/* Handle -mpid=ARG.  */

static void
tic6x_use_pid (const char *arg)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (tic6x_pid_types); i++)
    if (strcmp (arg, tic6x_pid_types[i].arg) == 0)
      {
	tic6x_pid = tic6x_pid_types[i].attr;
	return;
      }

  as_bad (_("unknown -mpid= argument '%s'"), arg);
}

/* Parse a target-specific option.  */

int
md_parse_option (int c, char *arg)
{
  switch (c)
    {
    case OPTION_MARCH:
      tic6x_use_arch (arg);
      break;

    case OPTION_MATOMIC:
      tic6x_atomic = 1;
      break;

    case OPTION_MNO_ATOMIC:
      tic6x_atomic = 0;
      break;

    case OPTION_MBIG_ENDIAN:
      target_big_endian = 1;
      break;

    case OPTION_MLITTLE_ENDIAN:
      target_big_endian = 0;
      break;

    case OPTION_MDSBT:
      tic6x_dsbt = 1;
      break;

    case OPTION_MNO_DSBT:
      tic6x_dsbt = 0;
      break;

    case OPTION_MPID:
      tic6x_use_pid (arg);
      break;

    case OPTION_MPIC:
      tic6x_pic = 1;
      break;

    case OPTION_MNO_PIC:
      tic6x_pic = 0;
      break;

    case OPTION_MGENERATE_REL:
      tic6x_generate_rela = FALSE;
      break;

    default:
      return 0;
    }
  return 1;
}

void
md_show_usage (FILE *stream ATTRIBUTE_UNUSED)
{
  unsigned int i;

  fputc ('\n', stream);
  fprintf (stream, _("TMS320C6000 options:\n"));
  fprintf (stream, _("  -march=ARCH             enable instructions from architecture ARCH\n"));
  fprintf (stream, _("  -matomic                enable atomic operation instructions\n"));
  fprintf (stream, _("  -mno-atomic             disable atomic operation instructions\n"));
  fprintf (stream, _("  -mbig-endian            generate big-endian code\n"));
  fprintf (stream, _("  -mlittle-endian         generate little-endian code\n"));
  fprintf (stream, _("  -mdsbt                  code uses DSBT addressing\n"));
  fprintf (stream, _("  -mno-dsbt               code does not use DSBT addressing\n"));
  fprintf (stream, _("  -mpid=no                code uses position-dependent data addressing\n"));
  fprintf (stream, _("  -mpid=near              code uses position-independent data addressing,\n"
		     "                            GOT accesses use near DP addressing\n"));
  fprintf (stream, _("  -mpid=far               code uses position-independent data addressing,\n"
		     "                            GOT accesses use far DP addressing\n"));
  fprintf (stream, _("  -mpic                   code addressing is position-independent\n"));
  fprintf (stream, _("  -mno-pic                code addressing is position-dependent\n"));
  /* -mgenerate-rel is only for testsuite use and is deliberately
      undocumented.  */

  fputc ('\n', stream);
  fprintf (stream, _("Supported ARCH values are:"));
  for (i = 0; i < ARRAY_SIZE (tic6x_arches); i++)
    fprintf (stream, " %s", tic6x_arches[i].arch);
  fputc ('\n', stream);
}

/* Update enabled features based on the current architecture and
   related settings.  */
static void
tic6x_update_features (void)
{
  switch (tic6x_atomic)
    {
    case -1:
      tic6x_features = tic6x_arch_enable;
      break;

    case 0:
      tic6x_features = tic6x_arch_enable & ~TIC6X_INSN_ATOMIC;
      break;

    case 1:
      tic6x_features = tic6x_arch_enable | TIC6X_INSN_ATOMIC;
      break;

    default:
      abort ();
    }

  tic6x_num_registers
    = (tic6x_arch_enable & (TIC6X_INSN_C64X | TIC6X_INSN_C67XP)) ? 32 : 16;

  tic6x_predicate_a0 = (tic6x_arch_enable & TIC6X_INSN_C64X) ? TRUE : FALSE;

  tic6x_can_cross_fp_boundary
    = (tic6x_arch_enable
       & (TIC6X_INSN_C64X | TIC6X_INSN_C67XP)) ? TRUE : FALSE;

  tic6x_long_data_constraints
    = (tic6x_arch_enable & TIC6X_INSN_C64X) ? FALSE : TRUE;

  tic6x_compact_insns = (tic6x_arch_enable & TIC6X_INSN_C64XP) ? TRUE : FALSE;
}

/* Do configuration after all options have been parsed.  */

void
tic6x_after_parse_args (void)
{
  tic6x_update_features ();
}

/* Parse a .arch directive.  */

static void
s_tic6x_arch (int ignored ATTRIBUTE_UNUSED)
{
  char c;
  char *arch;

  arch = input_line_pointer;
  while (*input_line_pointer && !ISSPACE (*input_line_pointer))
    input_line_pointer++;
  c = *input_line_pointer;
  *input_line_pointer = 0;

  tic6x_use_arch (arch);
  tic6x_update_features ();
  *input_line_pointer = c;
  demand_empty_rest_of_line ();
}

/* Parse a .atomic directive.  */

static void
s_tic6x_atomic (int ignored ATTRIBUTE_UNUSED)
{
  tic6x_atomic = 1;
  tic6x_update_features ();
  demand_empty_rest_of_line ();
}

/* Parse a .noatomic directive.  */

static void
s_tic6x_noatomic (int ignored ATTRIBUTE_UNUSED)
{
  tic6x_atomic = 0;
  tic6x_update_features ();
  demand_empty_rest_of_line ();
}

/* Parse a .nocmp directive.  */

static void
s_tic6x_nocmp (int ignored ATTRIBUTE_UNUSED)
{
  seg_info (now_seg)->tc_segment_info_data.nocmp = TRUE;
  demand_empty_rest_of_line ();
}

/* Track for each attribute whether it has been set explicitly (and so
   should not have a default value set by the assembler).  */
static bfd_boolean tic6x_attributes_set_explicitly[NUM_KNOWN_OBJ_ATTRIBUTES];

/* Parse a .c6xabi_attribute directive.  */

static void
s_tic6x_c6xabi_attribute (int ignored ATTRIBUTE_UNUSED)
{
  int tag = s_vendor_attribute (OBJ_ATTR_PROC);

  if (tag < NUM_KNOWN_OBJ_ATTRIBUTES)
    tic6x_attributes_set_explicitly[tag] = TRUE;
}

typedef struct
{
  const char *name;
  int tag;
} tic6x_attribute_table;

static const tic6x_attribute_table tic6x_attributes[] =
  {
#define TAG(tag, value) { #tag, tag },
#include "elf/tic6x-attrs.h"
#undef TAG
  };

/* Convert an attribute name to a number.  */

int
tic6x_convert_symbolic_attribute (const char *name)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (tic6x_attributes); i++)
    if (strcmp (name, tic6x_attributes[i].name) == 0)
      return tic6x_attributes[i].tag;

  return -1;
}

const pseudo_typeS md_pseudo_table[] =
  {
    { "arch", s_tic6x_arch, 0 },
    { "atomic", s_tic6x_atomic, 0 },
    { "c6xabi_attribute", s_tic6x_c6xabi_attribute, 0 },
    { "noatomic", s_tic6x_noatomic, 0 },
    { "nocmp", s_tic6x_nocmp, 0 },
    { "word", cons, 4 },
    { 0, 0, 0 }
  };

/* Hash table of opcodes.  For each opcode name, this stores a pointer
   to a tic6x_opcode_list listing (in an arbitrary order) all opcode
   table entries with that name.  */
static struct hash_control *opcode_hash;

/* Initialize the assembler (called once at assembler startup).  */

void
md_begin (void)
{
  tic6x_opcode_id id;

  bfd_set_arch_mach (stdoutput, TARGET_ARCH, 0);

  /* Insert opcodes into the hash table.  */
  opcode_hash = hash_new ();
  for (id = 0; id < tic6x_opcode_max; id++)
    {
      const char *errmsg;
      tic6x_opcode_list *opc = xmalloc (sizeof (tic6x_opcode_list));

      opc->id = id;
      opc->next = hash_find (opcode_hash, tic6x_opcode_table[id].name);
      if ((errmsg = hash_jam (opcode_hash, tic6x_opcode_table[id].name, opc))
	  != NULL)
	as_fatal ("%s", _(errmsg));
    }
}

/* Whether the current line being parsed had the "||" parallel bars.  */
static bfd_boolean tic6x_line_parallel;

/* Whether the current line being parsed started "||^" to indicate an
   SPMASKed parallel instruction.  */
static bfd_boolean tic6x_line_spmask;

/* If the current line being parsed had an instruction predicate, the
   creg value for that predicate (which must be nonzero); otherwise
   0.  */
static unsigned int tic6x_line_creg;

/* If the current line being parsed had an instruction predicate, the
   z value for that predicate; otherwise 0.  */
static unsigned int tic6x_line_z;

/* Return 1 (updating input_line_pointer as appropriate) if the line
   starting with C (immediately before input_line_pointer) starts with
   pre-opcode text appropriate for this target, 0 otherwise.  */

int
tic6x_unrecognized_line (int c)
{
  char *p, *endp;
  unsigned int z;
  bfd_boolean areg;
  bfd_boolean bad_predicate;

  switch (c)
    {
    case '|':
      if (input_line_pointer[0] == '|')
	{
	  if (input_line_pointer[1] == '^')
	    {
	      tic6x_line_spmask = TRUE;
	      input_line_pointer += 2;
	    }
	  else
	    input_line_pointer += 1;
	  if (tic6x_line_parallel)
	    as_bad (_("multiple '||' on same line"));
	  tic6x_line_parallel = TRUE;
	  if (tic6x_line_creg)
	    as_bad (_("'||' after predicate"));
	  return 1;
	}
      return 0;

    case '[':
      /* If it doesn't look like a predicate at all, just return 0.
	 If it looks like one but not a valid one, give a better
	 error.  */
      p = input_line_pointer;
      while (*p != ']' && !is_end_of_line[(unsigned char) *p])
	p++;
      if (*p != ']')
	return 0;
      endp = p + 1;
      p = input_line_pointer;
      z = 0;
      bad_predicate = FALSE;
      if (*p == '!')
	{
	  z = 1;
	  p++;
	}
      if (*p == 'A' || *p == 'a')
	areg = TRUE;
      else if (*p == 'B' || *p == 'b')
	areg = FALSE;
      else
	{
	  areg = TRUE; /* Avoid uninitialized warning.  */
	  bad_predicate = TRUE;
	}
      if (!bad_predicate)
	{
	  p++;
	  if (*p != '0' && *p != '1' && *p != '2')
	    bad_predicate = TRUE;
	  else if (p[1] != ']')
	    bad_predicate = TRUE;
	  else
	    input_line_pointer = p + 2;
	}

      if (tic6x_line_creg)
	as_bad (_("multiple predicates on same line"));

      if (bad_predicate)
	{
	  char ctmp = *endp;
	  *endp = 0;
	  as_bad (_("bad predicate '%s'"), input_line_pointer - 1);
	  *endp = ctmp;
	  input_line_pointer = endp;
	  return 1;
	}

      switch (*p)
	{
	case '0':
	  tic6x_line_creg = (areg ? 6 : 1);
	  if (areg && !tic6x_predicate_a0)
	    as_bad (_("predication on A0 not supported on this architecture"));
	  break;

	case '1':
	  tic6x_line_creg = (areg ? 4 : 2);
	  break;

	case '2':
	  tic6x_line_creg = (areg ? 5 : 3);
	  break;

	default:
	  abort ();
	}

      tic6x_line_z = z;
      return 1;

    default:
      return 0;
    }
}

/* Do any target-specific handling of a label required.  */

void
tic6x_frob_label (symbolS *sym)
{
  segment_info_type *si;
  tic6x_label_list *list;

  if (tic6x_line_parallel)
    {
      as_bad (_("label after '||'"));
      tic6x_line_parallel = FALSE;
      tic6x_line_spmask = FALSE;
    }
  if (tic6x_line_creg)
    {
      as_bad (_("label after predicate"));
      tic6x_line_creg = 0;
      tic6x_line_z = 0;
    }

  si = seg_info (now_seg);
  list = si->tc_segment_info_data.label_list;
  si->tc_segment_info_data.label_list = xmalloc (sizeof (tic6x_label_list));
  si->tc_segment_info_data.label_list->next = list;
  si->tc_segment_info_data.label_list->label = sym;

  /* Defining tc_frob_label overrides the ELF definition of
     obj_frob_label, so we need to apply its effects here.  */
  dwarf2_emit_label (sym);
}

/* At end-of-line, give errors for start-of-line decorations that
   needed an instruction but were not followed by one.  */

static void
tic6x_end_of_line (void)
{
  if (tic6x_line_parallel)
    {
      as_bad (_("'||' not followed by instruction"));
      tic6x_line_parallel = FALSE;
      tic6x_line_spmask = FALSE;
    }
  if (tic6x_line_creg)
    {
      as_bad (_("predicate not followed by instruction"));
      tic6x_line_creg = 0;
      tic6x_line_z = 0;
    }
}

/* Do any target-specific handling of the start of a logical line.  */

void
tic6x_start_line_hook (void)
{
  tic6x_end_of_line ();
}

/* Do target-specific handling immediately after an input file from
   the command line, and any other inputs it includes, have been
   read.  */

void
tic6x_cleanup (void)
{
  tic6x_end_of_line ();
}

/* Do target-specific initialization after arguments have been
   processed and the output file created.  */

void
tic6x_init_after_args (void)
{
  elf32_tic6x_set_use_rela_p (stdoutput, tic6x_generate_rela);
}

/* Free LIST of labels (possibly NULL).  */

static void
tic6x_free_label_list (tic6x_label_list *list)
{
  while (list)
    {
      tic6x_label_list *old = list;

      list = list->next;
      free (old);
    }
}

/* Handle a data alignment of N bytes.  */

void
tic6x_cons_align (int n ATTRIBUTE_UNUSED)
{
  segment_info_type *seginfo = seg_info (now_seg);

  /* Data means there is no current execute packet, and that any label
     applies to that data rather than a subsequent instruction.  */
  tic6x_free_label_list (seginfo->tc_segment_info_data.label_list);
  seginfo->tc_segment_info_data.label_list = NULL;
  seginfo->tc_segment_info_data.execute_packet_frag = NULL;
  seginfo->tc_segment_info_data.last_insn_lsb = NULL;
  seginfo->tc_segment_info_data.spmask_addr = NULL;
  seginfo->tc_segment_info_data.func_units_used = 0;
}

/* Handle an alignment directive.  Return TRUE if the
   machine-independent frag generation should be skipped.  */

bfd_boolean
tic6x_do_align (int n, char *fill, int len ATTRIBUTE_UNUSED, int max)
{
  /* Given code alignments of 4, 8, 16 or 32 bytes, we try to handle
     them in the md_end pass by inserting NOPs in parallel with
     previous instructions.  We only do this in sections containing
     nothing but instructions.  Code alignments of 1 or 2 bytes have
     no effect in such sections (but we record them with
     machine-dependent frags anyway so they can be skipped or
     converted to machine-independent), while those of more than 64
     bytes cannot reliably be handled in this way.  */
  if (n > 0
      && max >= 0
      && max < (1 << n)
      && !need_pass_2
      && fill == NULL
      && subseg_text_p (now_seg))
    {
      fragS *align_frag;
      char *p;

      if (n > 5)
	return FALSE;

      /* Machine-independent code would generate a frag here, but we
	 wish to handle it in a machine-dependent way.  */
      if (frag_now_fix () != 0)
	{
	  if (frag_now->fr_type != rs_machine_dependent)
	    frag_wane (frag_now);

	  frag_new (0);
	}
      frag_grow (32);
      align_frag = frag_now;
      p = frag_var (rs_machine_dependent, 32, 32, max, NULL, n, NULL);
      /* This must be the same as the frag to which a pointer was just
	 saved.  */
      if (p != align_frag->fr_literal)
	abort ();
      align_frag->tc_frag_data.is_insns = FALSE;
      return TRUE;
    }
  else
    return FALSE;
}

/* Types of operand for parsing purposes.  These are used as bit-masks
   to tell tic6x_parse_operand what forms of operand are
   permitted.  */
#define TIC6X_OP_EXP		0x0001u
#define TIC6X_OP_REG		0x0002u
#define TIC6X_OP_REGPAIR	0x0004u
#define TIC6X_OP_IRP		0x0008u
#define TIC6X_OP_NRP		0x0010u
/* With TIC6X_OP_MEM_NOUNREG, the contents of a () offset are always
   interpreted as an expression, which may be a symbol with the same
   name as a register that ends up being implicitly DP-relative.  With
   TIC6X_OP_MEM_UNREG, the contents of a () offset are interpreted as
   a register if they match one, and failing that as an expression,
   which must be constant.  */
#define TIC6X_OP_MEM_NOUNREG	0x0020u
#define TIC6X_OP_MEM_UNREG	0x0040u
#define TIC6X_OP_CTRL		0x0080u
#define TIC6X_OP_FUNC_UNIT	0x0100u

/* A register or register pair read by the assembler.  */
typedef struct
{
  /* The side the register is on (1 or 2).  */
  unsigned int side;
  /* The register number (0 to 31).  */
  unsigned int num;
} tic6x_register;

/* Types of modification of a base address.  */
typedef enum
  {
    tic6x_mem_mod_none,
    tic6x_mem_mod_plus,
    tic6x_mem_mod_minus,
    tic6x_mem_mod_preinc,
    tic6x_mem_mod_predec,
    tic6x_mem_mod_postinc,
    tic6x_mem_mod_postdec
  } tic6x_mem_mod;

/* Scaled [] or unscaled () nature of an offset.  */
typedef enum
  {
    tic6x_offset_none,
    tic6x_offset_scaled,
    tic6x_offset_unscaled
  } tic6x_mem_scaling;

/* A memory operand read by the assembler.  */
typedef struct
{
  /* The base register.  */
  tic6x_register base_reg;
  /* How the base register is modified.  */
  tic6x_mem_mod mod;
  /* Whether there is an offset (required with plain "+" and "-"), and
     whether it is scaled or unscaled if so.  */
  tic6x_mem_scaling scaled;
  /* Whether the offset is a register (TRUE) or an expression
     (FALSE).  */
  bfd_boolean offset_is_reg;
  /* The offset.  */
  union
  {
    expressionS exp;
    tic6x_register reg;
  } offset;
} tic6x_mem_ref;

/* A functional unit in SPMASK operands read by the assembler.  */
typedef struct
{
  /* The basic unit.  */
  tic6x_func_unit_base base;
  /* The side (1 or 2).  */
  unsigned int side;
} tic6x_func_unit_operand;

/* An operand read by the assembler.  */
typedef struct
{
  /* The syntactic form of the operand, as one of the bit-masks
     above.  */
  unsigned int form;
  /* The operand value.  */
  union
  {
    /* An expression: TIC6X_OP_EXP.  */
    expressionS exp;
    /* A register: TIC6X_OP_REG, TIC6X_OP_REGPAIR.  */
    tic6x_register reg;
    /* A memory reference: TIC6X_OP_MEM_NOUNREG,
       TIC6X_OP_MEM_UNREG.  */
    tic6x_mem_ref mem;
    /* A control register: TIC6X_OP_CTRL.  */
    tic6x_ctrl_id ctrl;
    /* A functional unit: TIC6X_OP_FUNC_UNIT.  */
    tic6x_func_unit_operand func_unit;
  } value;
} tic6x_operand;

#define skip_whitespace(str)  do { if (*(str) == ' ') ++(str); } while (0)

/* Parse a register operand, or part of an operand, starting at *P.
   If syntactically OK (including that the number is in the range 0 to
   31, but not necessarily in range for this architecture), return
   TRUE, putting the register side and number in *REG and update *P to
   point immediately after the register number; otherwise return FALSE
   without changing *P (but possibly changing *REG).  Do not print any
   diagnostics.  */

static bfd_boolean
tic6x_parse_register (char **p, tic6x_register *reg)
{
  char *r = *p;

  switch (*r)
    {
    case 'a':
    case 'A':
      reg->side = 1;
      break;

    case 'b':
    case 'B':
      reg->side = 2;
      break;

    default:
      return FALSE;
    }
  r++;

  if (*r >= '0' && *r <= '9')
    {
      reg->num = *r - '0';
      r++;
    }
  else
    return FALSE;

  if (reg->num > 0 && *r >= '0' && *r <= '9')
    {
      reg->num = reg->num * 10 + (*r - '0');
      r++;
    }

  if (*r >= '0' && *r <= '9')
    return FALSE;

  if (reg->num >= 32)
    return FALSE;
  *p = r;
  return TRUE;
}

/* Parse the initial two characters of a functional unit name starting
   at *P.  If OK, set *BASE and *SIDE and return TRUE; otherwise,
   return FALSE.  */

static bfd_boolean
tic6x_parse_func_unit_base (char *p, tic6x_func_unit_base *base,
			    unsigned int *side)
{
  bfd_boolean good_func_unit = TRUE;
  tic6x_func_unit_base maybe_base = tic6x_func_unit_nfu;
  unsigned int maybe_side = 0;

  switch (p[0])
    {
    case 'd':
    case 'D':
      maybe_base = tic6x_func_unit_d;
      break;

    case 'l':
    case 'L':
      maybe_base = tic6x_func_unit_l;
      break;

    case 'm':
    case 'M':
      maybe_base = tic6x_func_unit_m;
      break;

    case 's':
    case 'S':
      maybe_base = tic6x_func_unit_s;
      break;

    default:
      good_func_unit = FALSE;
      break;
    }

  if (good_func_unit)
    switch (p[1])
      {
      case '1':
	maybe_side = 1;
	break;

      case '2':
	maybe_side = 2;
	break;

      default:
	good_func_unit = FALSE;
	break;
      }

  if (good_func_unit)
    {
      *base = maybe_base;
      *side = maybe_side;
    }

  return good_func_unit;
}

/* Parse an operand starting at *P.  If the operand parses OK, return
   TRUE and store the value in *OP; otherwise return FALSE (possibly
   changing *OP).  In any case, update *P to point to the following
   comma or end of line.  The possible operand forms are given by
   OP_FORMS.  For diagnostics, this is operand OPNO of an opcode
   starting at STR, length OPC_LEN.  */

static bfd_boolean
tic6x_parse_operand (char **p, tic6x_operand *op, unsigned int op_forms,
		     char *str, int opc_len, unsigned int opno)
{
  bfd_boolean operand_parsed = FALSE;
  char *q = *p;

  if ((op_forms & (TIC6X_OP_MEM_NOUNREG | TIC6X_OP_MEM_UNREG))
      == (TIC6X_OP_MEM_NOUNREG | TIC6X_OP_MEM_UNREG))
    abort ();

  /* Check for functional unit names for SPMASK and SPMASKR.  */
  if (!operand_parsed && (op_forms & TIC6X_OP_FUNC_UNIT))
    {
      tic6x_func_unit_base base = tic6x_func_unit_nfu;
      unsigned int side = 0;

      if (tic6x_parse_func_unit_base (q, &base, &side))
	{
	  char *rq = q + 2;

	  skip_whitespace (rq);
	  if (is_end_of_line[(unsigned char) *rq] || *rq == ',')
	    {
	      op->form = TIC6X_OP_FUNC_UNIT;
	      op->value.func_unit.base = base;
	      op->value.func_unit.side = side;
	      operand_parsed = TRUE;
	      q = rq;
	    }
	}
    }

  /* Check for literal "irp".  */
  if (!operand_parsed && (op_forms & TIC6X_OP_IRP))
    {
      if ((q[0] == 'i' || q[0] == 'I')
	  && (q[1] == 'r' || q[1] == 'R')
	  && (q[2] == 'p' || q[2] == 'P'))
	{
	  char *rq = q + 3;

	  skip_whitespace (rq);
	  if (is_end_of_line[(unsigned char) *rq] || *rq == ',')
	    {
	      op->form = TIC6X_OP_IRP;
	      operand_parsed = TRUE;
	      q = rq;
	    }
	}
    }

  /* Check for literal "nrp".  */
  if (!operand_parsed && (op_forms & TIC6X_OP_NRP))
    {
      if ((q[0] == 'n' || q[0] == 'N')
	  && (q[1] == 'r' || q[1] == 'R')
	  && (q[2] == 'p' || q[2] == 'P'))
	{
	  char *rq = q + 3;

	  skip_whitespace (rq);
	  if (is_end_of_line[(unsigned char) *rq] || *rq == ',')
	    {
	      op->form = TIC6X_OP_NRP;
	      operand_parsed = TRUE;
	      q = rq;
	    }
	}
    }

  /* Check for control register names.  */
  if (!operand_parsed && (op_forms & TIC6X_OP_CTRL))
    {
      tic6x_ctrl_id crid;

      for (crid = 0; crid < tic6x_ctrl_max; crid++)
	{
	  size_t len = strlen (tic6x_ctrl_table[crid].name);

	  if (strncasecmp (tic6x_ctrl_table[crid].name, q, len) == 0)
	    {
	      char *rq = q + len;

	      skip_whitespace (rq);
	      if (is_end_of_line[(unsigned char) *rq] || *rq == ',')
		{
		  op->form = TIC6X_OP_CTRL;
		  op->value.ctrl = crid;
		  operand_parsed = TRUE;
		  q = rq;
		  if (!(tic6x_ctrl_table[crid].isa_variants & tic6x_features))
		    as_bad (_("control register '%s' not supported "
			      "on this architecture"),
			    tic6x_ctrl_table[crid].name);
		}
	    }
	}
    }

  /* See if this looks like a memory reference.  */
  if (!operand_parsed
      && (op_forms & (TIC6X_OP_MEM_NOUNREG | TIC6X_OP_MEM_UNREG)))
    {
      bfd_boolean mem_ok = TRUE;
      char *mq = q;
      tic6x_mem_mod mem_mod = tic6x_mem_mod_none;
      tic6x_register base_reg;
      bfd_boolean require_offset, permit_offset;
      tic6x_mem_scaling scaled;
      bfd_boolean offset_is_reg;
      expressionS offset_exp;
      tic6x_register offset_reg;

      if (*mq == '*')
	mq++;
      else
	mem_ok = FALSE;

      if (mem_ok)
	{
	  skip_whitespace (mq);
	  switch (*mq)
	    {
	    case '+':
	      if (mq[1] == '+')
		{
		  mem_mod = tic6x_mem_mod_preinc;
		  mq += 2;
		}
	      else
		{
		  mem_mod = tic6x_mem_mod_plus;
		  mq++;
		}
	      break;

	    case '-':
	      if (mq[1] == '-')
		{
		  mem_mod = tic6x_mem_mod_predec;
		  mq += 2;
		}
	      else
		{
		  mem_mod = tic6x_mem_mod_minus;
		  mq++;
		}
	      break;

	    default:
	      break;
	    }
	}

      if (mem_ok)
	{
	  skip_whitespace (mq);
	  mem_ok = tic6x_parse_register (&mq, &base_reg);
	}

      if (mem_ok && mem_mod == tic6x_mem_mod_none)
	{
	  skip_whitespace (mq);
	  if (mq[0] == '+' && mq[1] == '+')
	    {
	      mem_mod = tic6x_mem_mod_postinc;
	      mq += 2;
	    }
	  else if (mq[0] == '-' && mq[1] == '-')
	    {
	      mem_mod = tic6x_mem_mod_postdec;
	      mq += 2;
	    }
	}

      if (mem_mod == tic6x_mem_mod_none)
	permit_offset = FALSE;
      else
	permit_offset = TRUE;
      if (mem_mod == tic6x_mem_mod_plus || mem_mod == tic6x_mem_mod_minus)
	require_offset = TRUE;
      else
	require_offset = FALSE;
      scaled = tic6x_offset_none;
      offset_is_reg = FALSE;

      if (mem_ok && permit_offset)
	{
	  char endc = 0;

	  skip_whitespace (mq);
	  switch (*mq)
	    {
	    case '[':
	      scaled = tic6x_offset_scaled;
	      mq++;
	      endc = ']';
	      break;

	    case '(':
	      scaled = tic6x_offset_unscaled;
	      mq++;
	      endc = ')';
	      break;

	    default:
	      break;
	    }
	  if (scaled != tic6x_offset_none)
	    {
	      skip_whitespace (mq);
	      if (scaled == tic6x_offset_scaled
		  || (op_forms & TIC6X_OP_MEM_UNREG))
		{
		  bfd_boolean reg_ok;
		  char *rq = mq;

		  reg_ok = tic6x_parse_register (&rq, &offset_reg);
		  if (reg_ok)
		    {
		      skip_whitespace (rq);
		      if (*rq == endc)
			{
			  mq = rq;
			  offset_is_reg = TRUE;
			}
		    }
		}
	      if (!offset_is_reg)
		{
		  char *save_input_line_pointer;

		  save_input_line_pointer = input_line_pointer;
		  input_line_pointer = mq;
		  expression (&offset_exp);
		  mq = input_line_pointer;
		  input_line_pointer = save_input_line_pointer;
		}
	      skip_whitespace (mq);
	      if (*mq == endc)
		mq++;
	      else
		mem_ok = FALSE;
	    }
	}

      if (mem_ok && require_offset && scaled == tic6x_offset_none)
	mem_ok = FALSE;

      if (mem_ok)
	{
	  skip_whitespace (mq);
	  if (!is_end_of_line[(unsigned char) *mq] && *mq != ',')
	    mem_ok = FALSE;
	}

      if (mem_ok)
	{
	  op->form = op_forms & (TIC6X_OP_MEM_NOUNREG | TIC6X_OP_MEM_UNREG);
	  op->value.mem.base_reg = base_reg;
	  op->value.mem.mod = mem_mod;
	  op->value.mem.scaled = scaled;
	  op->value.mem.offset_is_reg = offset_is_reg;
	  if (offset_is_reg)
	    op->value.mem.offset.reg = offset_reg;
	  else
	    op->value.mem.offset.exp = offset_exp;
	  operand_parsed = TRUE;
	  q = mq;
	  if (base_reg.num >= tic6x_num_registers)
	    as_bad (_("register number %u not supported on this architecture"),
		    base_reg.num);
	  if (offset_is_reg && offset_reg.num >= tic6x_num_registers)
	    as_bad (_("register number %u not supported on this architecture"),
		    offset_reg.num);
	}
    }

  /* See if this looks like a register or register pair.  */
  if (!operand_parsed && (op_forms & (TIC6X_OP_REG | TIC6X_OP_REGPAIR)))
    {
      tic6x_register first_reg, second_reg;
      bfd_boolean reg_ok;
      char *rq = q;

      reg_ok = tic6x_parse_register (&rq, &first_reg);

      if (reg_ok)
	{
	  if (*rq == ':' && (op_forms & TIC6X_OP_REGPAIR))
	    {
	      rq++;
	      reg_ok = tic6x_parse_register (&rq, &second_reg);
	      if (reg_ok)
		{
		  skip_whitespace (rq);
		  if (is_end_of_line[(unsigned char) *rq] || *rq == ',')
		    {
		      if ((second_reg.num & 1)
			  || (first_reg.num != second_reg.num + 1)
			  || (first_reg.side != second_reg.side))
			as_bad (_("register pair for operand %u of '%.*s'"
				  " not a valid even/odd pair"), opno,
				opc_len, str);
		      op->form = TIC6X_OP_REGPAIR;
		      op->value.reg = second_reg;
		      operand_parsed = TRUE;
		      q = rq;
		    }
		}
	    }
	  else if (op_forms & TIC6X_OP_REG)
	    {
	      skip_whitespace (rq);
	      if (is_end_of_line[(unsigned char) *rq] || *rq == ',')
		{
		  op->form = TIC6X_OP_REG;
		  op->value.reg = first_reg;
		  operand_parsed = TRUE;
		  q = rq;
		}
	    }
	}
      if (operand_parsed)
	{
	  if (first_reg.num >= tic6x_num_registers)
	    as_bad (_("register number %u not supported on this architecture"),
		    first_reg.num);
	  if (op->form == TIC6X_OP_REGPAIR
	      && second_reg.num >= tic6x_num_registers)
	    as_bad (_("register number %u not supported on this architecture"),
		    second_reg.num);
	}
    }

  /* Otherwise, parse it as an expression.  */
  if (!operand_parsed && (op_forms & TIC6X_OP_EXP))
    {
      char *save_input_line_pointer;

      save_input_line_pointer = input_line_pointer;
      input_line_pointer = q;
      op->form = TIC6X_OP_EXP;
      expression (&op->value.exp);
      q = input_line_pointer;
      input_line_pointer = save_input_line_pointer;
      operand_parsed = TRUE;
    }

  if (operand_parsed)
    {
      /* Now the operand has been parsed, there must be nothing more
	 before the comma or end of line.  */
      skip_whitespace (q);
      if (!is_end_of_line[(unsigned char) *q] && *q != ',')
	{
	  operand_parsed = FALSE;
	  as_bad (_("junk after operand %u of '%.*s'"), opno,
		  opc_len, str);
	  while (!is_end_of_line[(unsigned char) *q] && *q != ',')
	    q++;
	}
    }
  else
    {
      /* This could not be parsed as any acceptable form of
	 operand.  */
      switch (op_forms)
	{
	case TIC6X_OP_REG | TIC6X_OP_REGPAIR:
	  as_bad (_("bad register or register pair for operand %u of '%.*s'"),
		  opno, opc_len, str);
	  break;

	case TIC6X_OP_REG | TIC6X_OP_CTRL:
	case TIC6X_OP_REG:
	  as_bad (_("bad register for operand %u of '%.*s'"),
		  opno, opc_len, str);
	  break;

	case TIC6X_OP_REGPAIR:
	  as_bad (_("bad register pair for operand %u of '%.*s'"),
		  opno, opc_len, str);
	  break;

	case TIC6X_OP_FUNC_UNIT:
	  as_bad (_("bad functional unit for operand %u of '%.*s'"),
		  opno, opc_len, str);
	  break;

	default:
	  as_bad (_("bad operand %u of '%.*s'"),
		  opno, opc_len, str);
	  break;

	}
      while (!is_end_of_line[(unsigned char) *q] && *q != ',')
	q++;
    }
  *p = q;
  return operand_parsed;
}

/* Table of assembler operators and associated O_* values.  */
typedef struct
{
  const char *name;
  operatorT op;
} tic6x_operator_table;
static const tic6x_operator_table tic6x_operators[] = {
#define O_dsbt_index O_md1
  { "dsbt_index", O_dsbt_index },
#define O_got O_md2
  { "got", O_got },
#define O_dpr_got O_md3
  { "dpr_got", O_dpr_got },
#define O_dpr_byte O_md4
  { "dpr_byte", O_dpr_byte },
#define O_dpr_hword O_md5
  { "dpr_hword", O_dpr_hword },
#define O_dpr_word O_md6
  { "dpr_word", O_dpr_word },
};

/* Parse a name in some machine-specific way.  Used on C6X to handle
   assembler operators.  */

int
tic6x_parse_name (const char *name, expressionS *exprP,
		  enum expr_mode mode ATTRIBUTE_UNUSED, char *nextchar)
{
  char *p = input_line_pointer;
  char c, *name_start, *name_end;
  const char *inner_name;
  unsigned int i;
  operatorT op = O_illegal;
  symbolS *sym;

  if (*name != '$')
    return 0;

  for (i = 0; i < ARRAY_SIZE (tic6x_operators); i++)
    if (strcasecmp (name + 1, tic6x_operators[i].name) == 0)
      {
	op = tic6x_operators[i].op;
	break;
      }

  if (op == O_illegal)
    return 0;

  *input_line_pointer = *nextchar;
  skip_whitespace (p);

  if (*p != '(')
    {
      *input_line_pointer = 0;
      return 0;
    }
  p++;
  skip_whitespace (p);

  if (!is_name_beginner (*p))
    {
      *input_line_pointer = 0;
      return 0;
    }

  name_start = p;
  p++;
  while (is_part_of_name (*p))
    p++;
  name_end = p;
  skip_whitespace (p);

  if (*p != ')')
    {
      *input_line_pointer = 0;
      return 0;
    }

  input_line_pointer = p + 1;
  *nextchar = *input_line_pointer;
  *input_line_pointer = 0;

  c = *name_end;
  *name_end = 0;
  inner_name = name_start;
  if (op == O_dsbt_index && strcmp (inner_name, "__c6xabi_DSBT_BASE") != 0)
    {
      as_bad (_("$DSBT_INDEX must be used with __c6xabi_DSBT_BASE"));
      inner_name = "__c6xabi_DSBT_BASE";
    }
  sym = symbol_find_or_make (inner_name);
  *name_end = c;

  exprP->X_op = op;
  exprP->X_add_symbol = sym;
  exprP->X_add_number = 0;
  exprP->X_op_symbol = NULL;
  exprP->X_md = 0;

  return 1;
}

/* Create a fixup for an expression.  Same arguments as fix_new_exp,
   plus FIX_ADDA which is TRUE for ADDA instructions (to indicate that
   fixes resolving to constants should have those constants implicitly
   shifted) and FALSE otherwise, but look for C6X-specific expression
   types and adjust the relocations or give errors accordingly.  */

static void
tic6x_fix_new_exp (fragS *frag, int where, int size, expressionS *exp,
		   int pcrel, bfd_reloc_code_real_type r_type,
		   bfd_boolean fix_adda)
{
  bfd_reloc_code_real_type new_reloc = BFD_RELOC_UNUSED;
  fixS *fix;

  switch (exp->X_op)
    {
    case O_dsbt_index:
      switch (r_type)
	{
	case BFD_RELOC_C6000_SBR_U15_W:
	  new_reloc = BFD_RELOC_C6000_DSBT_INDEX;
	  break;

	default:
	  as_bad (_("$DSBT_INDEX not supported in this context"));
	  return;
	}
      break;

    case O_got:
      switch (r_type)
	{
	case BFD_RELOC_C6000_SBR_U15_W:
	  new_reloc = BFD_RELOC_C6000_SBR_GOT_U15_W;
	  break;

	default:
	  as_bad (_("$GOT not supported in this context"));
	  return;
	}
      break;

    case O_dpr_got:
      switch (r_type)
	{
	case BFD_RELOC_C6000_ABS_L16:
	  new_reloc = BFD_RELOC_C6000_SBR_GOT_L16_W;
	  break;

	case BFD_RELOC_C6000_ABS_H16:
	  new_reloc = BFD_RELOC_C6000_SBR_GOT_H16_W;
	  break;

	default:
	  as_bad (_("$DPR_GOT not supported in this context"));
	  return;
	}
      break;

    case O_dpr_byte:
      switch (r_type)
	{
	case BFD_RELOC_C6000_ABS_S16:
	  new_reloc = BFD_RELOC_C6000_SBR_S16;
	  break;

	case BFD_RELOC_C6000_ABS_L16:
	  new_reloc = BFD_RELOC_C6000_SBR_L16_B;
	  break;

	case BFD_RELOC_C6000_ABS_H16:
	  new_reloc = BFD_RELOC_C6000_SBR_H16_B;
	  break;

	default:
	  as_bad (_("$DPR_BYTE not supported in this context"));
	  return;
	}
      break;

    case O_dpr_hword:
      switch (r_type)
	{
	case BFD_RELOC_C6000_ABS_L16:
	  new_reloc = BFD_RELOC_C6000_SBR_L16_H;
	  break;

	case BFD_RELOC_C6000_ABS_H16:
	  new_reloc = BFD_RELOC_C6000_SBR_H16_H;
	  break;

	default:
	  as_bad (_("$DPR_HWORD not supported in this context"));
	  return;
	}
      break;

    case O_dpr_word:
      switch (r_type)
	{
	case BFD_RELOC_C6000_ABS_L16:
	  new_reloc = BFD_RELOC_C6000_SBR_L16_W;
	  break;

	case BFD_RELOC_C6000_ABS_H16:
	  new_reloc = BFD_RELOC_C6000_SBR_H16_W;
	  break;

	default:
	  as_bad (_("$DPR_WORD not supported in this context"));
	  return;
	}
      break;

    case O_symbol:
      break;

    default:
      if (pcrel)
	{
	  as_bad (_("invalid PC-relative operand"));
	  return;
	}
      break;
    }

  if (new_reloc == BFD_RELOC_UNUSED)
    fix = fix_new_exp (frag, where, size, exp, pcrel, r_type);
  else
    fix = fix_new (frag, where, size, exp->X_add_symbol, exp->X_add_number,
		   pcrel, new_reloc);
  fix->tc_fix_data.fix_adda = fix_adda;
}

/* Generate a fix for a constant (.word etc.).  Needed to ensure these
   go through the error checking in tic6x_fix_new_exp.  */

void
tic6x_cons_fix_new (fragS *frag, int where, int size, expressionS *exp)
{
  bfd_reloc_code_real_type r_type;

  switch (size)
    {
    case 1:
      r_type = BFD_RELOC_8;
      break;

    case 2:
      r_type = BFD_RELOC_16;
      break;

    case 4:
      r_type = BFD_RELOC_32;
      break;

    default:
      as_bad (_("no %d-byte relocations available"), size);
      return;
    }

  tic6x_fix_new_exp (frag, where, size, exp, 0, r_type, FALSE);
}

/* Initialize target-specific fix data.  */

void
tic6x_init_fix_data (fixS *fixP)
{
  fixP->tc_fix_data.fix_adda = FALSE;
}

/* Return true if the fix can be handled by GAS, false if it must
   be passed through to the linker.  */

bfd_boolean
tic6x_fix_adjustable (fixS *fixP)
{
  switch (fixP->fx_r_type)
    {
      /* Adjust_reloc_syms doesn't know about the GOT.  */
    case BFD_RELOC_C6000_SBR_GOT_U15_W:
    case BFD_RELOC_C6000_SBR_GOT_H16_W:
    case BFD_RELOC_C6000_SBR_GOT_L16_W:
      return 0;

    default:
      return 1;
    }
}

/* Given the fine-grained form of an operand, return the coarse
   (bit-mask) form.  */

static unsigned int
tic6x_coarse_operand_form (tic6x_operand_form form)
{
  switch (form)
    {
    case tic6x_operand_asm_const:
    case tic6x_operand_link_const:
      return TIC6X_OP_EXP;

    case tic6x_operand_reg:
    case tic6x_operand_xreg:
    case tic6x_operand_dreg:
    case tic6x_operand_areg:
    case tic6x_operand_retreg:
      return TIC6X_OP_REG;

    case tic6x_operand_regpair:
    case tic6x_operand_xregpair:
    case tic6x_operand_dregpair:
      return TIC6X_OP_REGPAIR;

    case tic6x_operand_irp:
      return TIC6X_OP_IRP;

    case tic6x_operand_nrp:
      return TIC6X_OP_NRP;

    case tic6x_operand_ctrl:
      return TIC6X_OP_CTRL;

    case tic6x_operand_mem_short:
    case tic6x_operand_mem_long:
    case tic6x_operand_mem_deref:
      return TIC6X_OP_MEM_NOUNREG;

    case tic6x_operand_mem_ndw:
      return TIC6X_OP_MEM_UNREG;

    case tic6x_operand_func_unit:
      return TIC6X_OP_FUNC_UNIT;

    default:
      abort ();
    }
}

/* How an operand may match or not match a desired form.  If different
   instruction alternatives fail in different ways, the first failure
   in this list determines the diagnostic.  */
typedef enum
  {
    /* Matches.  */
    tic6x_match_matches,
    /* Bad coarse form.  */
    tic6x_match_coarse,
    /* Not constant.  */
    tic6x_match_non_const,
    /* Register on wrong side.  */
    tic6x_match_wrong_side,
    /* Not a valid address register.  */
    tic6x_match_bad_address,
    /* Not a valid return address register.  */
    tic6x_match_bad_return,
    /* Control register not readable.  */
    tic6x_match_ctrl_write_only,
    /* Control register not writable.  */
    tic6x_match_ctrl_read_only,
    /* Not a valid memory reference for this instruction.  */
    tic6x_match_bad_mem
  } tic6x_operand_match;

/* Return whether an operand matches the given fine-grained form and
   read/write usage, and, if it does not match, how it fails to match.
   The main functional unit side is SIDE; the cross-path side is CROSS
   (the same as SIDE if a cross path not used); the data side is
   DATA_SIDE.  */
static tic6x_operand_match
tic6x_operand_matches_form (const tic6x_operand *op, tic6x_operand_form form,
			    tic6x_rw rw, unsigned int side, unsigned int cross,
			    unsigned int data_side)
{
  unsigned int coarse = tic6x_coarse_operand_form (form);

  if (coarse != op->form)
    return tic6x_match_coarse;

  switch (form)
    {
    case tic6x_operand_asm_const:
      if (op->value.exp.X_op == O_constant)
	return tic6x_match_matches;
      else
	return tic6x_match_non_const;

    case tic6x_operand_link_const:
    case tic6x_operand_irp:
    case tic6x_operand_nrp:
    case tic6x_operand_func_unit:
      /* All expressions are link-time constants, although there may
	 not be relocations to express them in the output file.  "irp"
	 and "nrp" are unique operand values.  All parsed functional
	 unit names are valid.  */
      return tic6x_match_matches;

    case tic6x_operand_reg:
    case tic6x_operand_regpair:
      if (op->value.reg.side == side)
	return tic6x_match_matches;
      else
	return tic6x_match_wrong_side;

    case tic6x_operand_xreg:
    case tic6x_operand_xregpair:
      if (op->value.reg.side == cross)
	return tic6x_match_matches;
      else
	return tic6x_match_wrong_side;

    case tic6x_operand_dreg:
    case tic6x_operand_dregpair:
      if (op->value.reg.side == data_side)
	return tic6x_match_matches;
      else
	return tic6x_match_wrong_side;

    case tic6x_operand_areg:
      if (op->value.reg.side != cross)
	return tic6x_match_wrong_side;
      else if (op->value.reg.side == 2
	       && (op->value.reg.num == 14 || op->value.reg.num == 15))
	return tic6x_match_matches;
      else
	return tic6x_match_bad_address;

    case tic6x_operand_retreg:
      if (op->value.reg.side != side)
	return tic6x_match_wrong_side;
      else if (op->value.reg.num != 3)
	return tic6x_match_bad_return;
      else
	return tic6x_match_matches;

    case tic6x_operand_ctrl:
      switch (rw)
	{
	case tic6x_rw_read:
	  if (tic6x_ctrl_table[op->value.ctrl].rw == tic6x_rw_read
	      || tic6x_ctrl_table[op->value.ctrl].rw == tic6x_rw_read_write)
	    return tic6x_match_matches;
	  else
	    return tic6x_match_ctrl_write_only;

	case tic6x_rw_write:
	  if (tic6x_ctrl_table[op->value.ctrl].rw == tic6x_rw_write
	      || tic6x_ctrl_table[op->value.ctrl].rw == tic6x_rw_read_write)
	    return tic6x_match_matches;
	  else
	    return tic6x_match_ctrl_read_only;

	default:
	  abort ();
	}

    case tic6x_operand_mem_deref:
      if (op->value.mem.mod != tic6x_mem_mod_none)
	return tic6x_match_bad_mem;
      else if (op->value.mem.scaled != tic6x_offset_none)
	abort ();
      else if (op->value.mem.base_reg.side != side)
	return tic6x_match_bad_mem;
      else
	return tic6x_match_matches;

    case tic6x_operand_mem_short:
    case tic6x_operand_mem_ndw:
      if (op->value.mem.base_reg.side != side)
	return tic6x_match_bad_mem;
      if (op->value.mem.mod == tic6x_mem_mod_none)
	{
	  if (op->value.mem.scaled != tic6x_offset_none)
	    abort ();
	  return tic6x_match_matches;
	}
      if (op->value.mem.scaled == tic6x_offset_none)
	{
	  if (op->value.mem.mod == tic6x_mem_mod_plus
	      || op->value.mem.mod == tic6x_mem_mod_minus)
	    abort ();
	  return tic6x_match_matches;
	}
      if (op->value.mem.offset_is_reg)
	{
	  if (op->value.mem.scaled == tic6x_offset_unscaled
	      && form != tic6x_operand_mem_ndw)
	    abort ();
	  if (op->value.mem.offset.reg.side == side)
	    return tic6x_match_matches;
	  else
	    return tic6x_match_bad_mem;
	}
      else
	{
	  if (op->value.mem.offset.exp.X_op == O_constant)
	    return tic6x_match_matches;
	  else
	    return tic6x_match_bad_mem;
	}

    case tic6x_operand_mem_long:
      if (op->value.mem.base_reg.side == 2
	  && (op->value.mem.base_reg.num == 14
	      || op->value.mem.base_reg.num == 15))
	{
	  switch (op->value.mem.mod)
	    {
	    case tic6x_mem_mod_none:
	      if (op->value.mem.scaled != tic6x_offset_none)
		abort ();
	      return tic6x_match_matches;

	    case tic6x_mem_mod_plus:
	      if (op->value.mem.scaled == tic6x_offset_none)
		abort ();
	      if (op->value.mem.offset_is_reg)
		return tic6x_match_bad_mem;
	      else if (op->value.mem.scaled == tic6x_offset_scaled
		       && op->value.mem.offset.exp.X_op != O_constant)
		return tic6x_match_bad_mem;
	      else
		return tic6x_match_matches;

	    case tic6x_mem_mod_minus:
	    case tic6x_mem_mod_preinc:
	    case tic6x_mem_mod_predec:
	    case tic6x_mem_mod_postinc:
	    case tic6x_mem_mod_postdec:
	      return tic6x_match_bad_mem;

	    default:
	      abort ();
	    }

	}
      else
	return tic6x_match_bad_mem;

    default:
      abort ();
    }
}

/* Return the number of bits shift used with DP-relative coding method
   CODING.  */

static unsigned int
tic6x_dpr_shift (tic6x_coding_method coding)
{
  switch (coding)
    {
    case tic6x_coding_ulcst_dpr_byte:
      return 0;

    case tic6x_coding_ulcst_dpr_half:
      return 1;

    case tic6x_coding_ulcst_dpr_word:
      return 2;

    default:
      abort ();
    }
}

/* Return the relocation used with DP-relative coding method
   CODING.  */

static bfd_reloc_code_real_type
tic6x_dpr_reloc (tic6x_coding_method coding)
{
  switch (coding)
    {
    case tic6x_coding_ulcst_dpr_byte:
      return BFD_RELOC_C6000_SBR_U15_B;

    case tic6x_coding_ulcst_dpr_half:
      return BFD_RELOC_C6000_SBR_U15_H;

    case tic6x_coding_ulcst_dpr_word:
      return BFD_RELOC_C6000_SBR_U15_W;

    default:
      abort ();
    }
}

/* Given a memory reference *MEM_REF as originally parsed, fill in
   defaults for missing offsets.  */

static void
tic6x_default_mem_ref (tic6x_mem_ref *mem_ref)
{
  switch (mem_ref->mod)
    {
    case tic6x_mem_mod_none:
      if (mem_ref->scaled != tic6x_offset_none)
	abort ();
      mem_ref->mod = tic6x_mem_mod_plus;
      mem_ref->scaled = tic6x_offset_unscaled;
      mem_ref->offset_is_reg = FALSE;
      memset (&mem_ref->offset.exp, 0, sizeof mem_ref->offset.exp);
      mem_ref->offset.exp.X_op = O_constant;
      mem_ref->offset.exp.X_add_number = 0;
      mem_ref->offset.exp.X_unsigned = 0;
      break;

    case tic6x_mem_mod_plus:
    case tic6x_mem_mod_minus:
      if (mem_ref->scaled == tic6x_offset_none)
	abort ();
      break;

    case tic6x_mem_mod_preinc:
    case tic6x_mem_mod_predec:
    case tic6x_mem_mod_postinc:
    case tic6x_mem_mod_postdec:
      if (mem_ref->scaled != tic6x_offset_none)
	break;
      mem_ref->scaled = tic6x_offset_scaled;
      mem_ref->offset_is_reg = FALSE;
      memset (&mem_ref->offset.exp, 0, sizeof mem_ref->offset.exp);
      mem_ref->offset.exp.X_op = O_constant;
      mem_ref->offset.exp.X_add_number = 1;
      mem_ref->offset.exp.X_unsigned = 0;
      break;

    default:
      abort ();
    }
}

/* Return the encoding in the 8-bit field of an SPMASK or SPMASKR
   instruction of the specified UNIT, side SIDE.  */

static unsigned int
tic6x_encode_spmask (tic6x_func_unit_base unit, unsigned int side)
{
  switch (unit)
    {
    case tic6x_func_unit_l:
      return 1 << (side - 1);

    case tic6x_func_unit_s:
      return 1 << (side + 1);

    case tic6x_func_unit_d:
      return 1 << (side + 3);

    case tic6x_func_unit_m:
      return 1 << (side + 5);

    default:
      abort ();
    }
}

/* Try to encode the instruction with opcode number ID and operands
   OPERANDS (number NUM_OPERANDS), creg value THIS_LINE_CREG and z
   value THIS_LINE_Z; FUNC_UNIT_SIDE, FUNC_UNIT_CROSS and
   FUNC_UNIT_DATA_SIDE describe the functional unit specification;
   SPLOOP_II is the ii value from the previous SPLOOP-family
   instruction, or 0 if not in such a loop; the only possible problems
   are operands being out of range (they already match the
   fine-grained form), and inappropriate predication.  If this
   succeeds, return the encoding and set *OK to TRUE; otherwise return
   0 and set *OK to FALSE.  If a fix is needed, set *FIX_NEEDED to
   true and fill in *FIX_EXP, *FIX_PCREL, *FX_R_TYPE and *FIX_ADDA.
   Print error messages for failure if PRINT_ERRORS is TRUE; the
   opcode starts at STR and has length OPC_LEN.  */

static unsigned int
tic6x_try_encode (tic6x_opcode_id id, tic6x_operand *operands,
		  unsigned int num_operands, unsigned int this_line_creg,
		  unsigned int this_line_z, unsigned int func_unit_side,
		  unsigned int func_unit_cross,
		  unsigned int func_unit_data_side, int sploop_ii,
		  expressionS **fix_exp, int *fix_pcrel,
		  bfd_reloc_code_real_type *fx_r_type, bfd_boolean *fix_adda,
		  bfd_boolean *fix_needed, bfd_boolean *ok,
		  bfd_boolean print_errors, char *str, int opc_len)
{
  const tic6x_opcode *opct;
  const tic6x_insn_format *fmt;
  unsigned int opcode_value;
  unsigned int fld;

  opct = &tic6x_opcode_table[id];
  fmt = &tic6x_insn_format_table[opct->format];
  opcode_value = fmt->cst_bits;

  for (fld = 0; fld < opct->num_fixed_fields; fld++)
    {
      if (opct->fixed_fields[fld].min_val == opct->fixed_fields[fld].max_val)
	{
	  const tic6x_insn_field *fldd;
	  fldd = tic6x_field_from_fmt (fmt, opct->fixed_fields[fld].field_id);
	  if (fldd == NULL)
	    abort ();
	  opcode_value |= opct->fixed_fields[fld].min_val << fldd->low_pos;
	}
    }

  for (fld = 0; fld < opct->num_variable_fields; fld++)
    {
      const tic6x_insn_field *fldd;
      unsigned int value;
      unsigned int opno;
      unsigned int ffld;
      offsetT sign_value;
      unsigned int bits;
      unsigned int fcyc_bits;
      expressionS *expp;
      expressionS ucexp;
      tic6x_mem_ref mem;

      fldd = tic6x_field_from_fmt (fmt, opct->variable_fields[fld].field_id);
      if (fldd == NULL)
	abort ();
      opno = opct->variable_fields[fld].operand_num;
      switch (opct->variable_fields[fld].coding_method)
	{
	case tic6x_coding_ucst:
	  if (operands[opno].form != TIC6X_OP_EXP)
	    abort ();
	  if (operands[opno].value.exp.X_op != O_constant)
	    abort ();
	  ucexp = operands[opno].value.exp;
	unsigned_constant:
	  if (ucexp.X_add_number < 0
	      || ucexp.X_add_number >= (1 << fldd->width))
	    {
	      if (print_errors)
		as_bad (_("operand %u of '%.*s' out of range"), opno + 1,
			opc_len, str);
	      *ok = FALSE;
	      return 0;
	    }
	  value = ucexp.X_add_number;
	  break;

	case tic6x_coding_scst:
	  if (operands[opno].form != TIC6X_OP_EXP)
	    abort ();
	  if (operands[opno].value.exp.X_op != O_constant)
	    {
	      value = 0;
	      /* Opcode table should not permit non-constants without
		 a known relocation for them.  */
	      if (fldd->low_pos != 7 || fldd->width != 16)
		abort ();
	      *fix_needed = TRUE;
	      *fix_exp = &operands[opno].value.exp;
	      *fix_pcrel = 0;
	      *fx_r_type = BFD_RELOC_C6000_ABS_S16;
	      *fix_adda = FALSE;
	      break;
	    }
	  sign_value = SEXT (operands[opno].value.exp.X_add_number);
	signed_constant:
	  if (sign_value < -(1 << (fldd->width - 1))
	      || (sign_value >= (1 << (fldd->width - 1))))
	    {
	      if (print_errors)
		as_bad (_("operand %u of '%.*s' out of range"), opno + 1,
			opc_len, str);
	      *ok = FALSE;
	      return 0;
	    }
	  value = sign_value + (1 << (fldd->width - 1));
	  value ^= (1 << (fldd->width - 1));
	  break;

	case tic6x_coding_ucst_minus_one:
	  if (operands[opno].form != TIC6X_OP_EXP)
	    abort ();
	  if (operands[opno].value.exp.X_op != O_constant)
	    abort ();
	  if (operands[opno].value.exp.X_add_number <= 0
	      || operands[opno].value.exp.X_add_number > (1 << fldd->width))
	    {
	      if (print_errors)
		as_bad (_("operand %u of '%.*s' out of range"), opno + 1,
			opc_len, str);
	      *ok = FALSE;
	      return 0;
	    }
	  value = operands[opno].value.exp.X_add_number - 1;
	  break;

	case tic6x_coding_scst_negate:
	  if (operands[opno].form != TIC6X_OP_EXP)
	    abort ();
	  if (operands[opno].value.exp.X_op != O_constant)
	    abort ();
	  sign_value = SEXT (-operands[opno].value.exp.X_add_number);
	  goto signed_constant;

	case tic6x_coding_ulcst_dpr_byte:
	case tic6x_coding_ulcst_dpr_half:
	case tic6x_coding_ulcst_dpr_word:
	  bits = tic6x_dpr_shift (opct->variable_fields[fld].coding_method);
	  switch (operands[opno].form)
	    {
	    case TIC6X_OP_EXP:
	      if (operands[opno].value.exp.X_op == O_constant)
		{
		  ucexp = operands[opno].value.exp;
		  goto unsigned_constant;
		}
	      expp = &operands[opno].value.exp;
	      break;

	    case TIC6X_OP_MEM_NOUNREG:
	      mem = operands[opno].value.mem;
	      tic6x_default_mem_ref (&mem);
	      if (mem.offset_is_reg)
		abort ();
	      if (mem.offset.exp.X_op == O_constant)
		{
		  ucexp = mem.offset.exp;
		  if (mem.scaled == tic6x_offset_unscaled)
		    {
		      if (ucexp.X_add_number & ((1 << bits) - 1))
			{
			  if (print_errors)
			    as_bad (_("offset in operand %u of '%.*s' not "
				      "divisible by %u"), opno + 1, opc_len,
				    str, 1u << bits);
			  *ok = FALSE;
			  return 0;
			}
		      ucexp.X_add_number >>= bits;
		    }
		  goto unsigned_constant;
		}
	      if (mem.scaled != tic6x_offset_unscaled)
		abort ();
	      if (operands[opno].value.mem.mod == tic6x_mem_mod_none
		  || operands[opno].value.mem.scaled != tic6x_offset_unscaled
		  || operands[opno].value.mem.offset_is_reg)
		abort ();
	      expp = &operands[opno].value.mem.offset.exp;
	      break;

	    default:
	      abort ();
	    }
	  value = 0;
	  /* Opcode table should not use this encoding without a known
	     relocation.  */
	  if (fldd->low_pos != 8 || fldd->width != 15)
	    abort ();
	  /* We do not check for offset divisibility here; such a
	     check is not needed at this point to encode the value,
	     and if there is eventually a problem it will be detected
	     either in md_apply_fix or at link time.  */
	  *fix_needed = TRUE;
	  *fix_exp = expp;
	  *fix_pcrel = 0;
	  *fx_r_type
	    = tic6x_dpr_reloc (opct->variable_fields[fld].coding_method);
	  if (operands[opno].form == TIC6X_OP_EXP)
	    *fix_adda = TRUE;
	  else
	    *fix_adda = FALSE;
	  break;

	case tic6x_coding_lcst_low16:
	  if (operands[opno].form != TIC6X_OP_EXP)
	    abort ();
	  if (operands[opno].value.exp.X_op == O_constant)
	    value = operands[opno].value.exp.X_add_number & 0xffff;
	  else
	    {
	      value = 0;
	      /* Opcode table should not use this encoding without a
		 known relocation.  */
	      if (fldd->low_pos != 7 || fldd->width != 16)
		abort ();
	      *fix_needed = TRUE;
	      *fix_exp = &operands[opno].value.exp;
	      *fix_pcrel = 0;
	      *fx_r_type = BFD_RELOC_C6000_ABS_L16;
	      *fix_adda = FALSE;
	    }
	  break;

	case tic6x_coding_lcst_high16:
	  if (operands[opno].form != TIC6X_OP_EXP)
	    abort ();
	  if (operands[opno].value.exp.X_op == O_constant)
	    value = (operands[opno].value.exp.X_add_number >> 16) & 0xffff;
	  else
	    {
	      value = 0;
	      /* Opcode table should not use this encoding without a
		 known relocation.  */
	      if (fldd->low_pos != 7 || fldd->width != 16)
		abort ();
	      *fix_needed = TRUE;
	      *fix_exp = &operands[opno].value.exp;
	      *fix_pcrel = 0;
	      *fx_r_type = BFD_RELOC_C6000_ABS_H16;
	      *fix_adda = FALSE;
	    }
	  break;

	case tic6x_coding_pcrel:
	case tic6x_coding_pcrel_half:
	  if (operands[opno].form != TIC6X_OP_EXP)
	    abort ();
	  value = 0;
	  *fix_needed = TRUE;
	  *fix_exp = &operands[opno].value.exp;
	  *fix_pcrel = 1;
	  if (fldd->low_pos == 7 && fldd->width == 21)
	    *fx_r_type = BFD_RELOC_C6000_PCR_S21;
	  else if (fldd->low_pos == 16 && fldd->width == 12)
	    *fx_r_type = BFD_RELOC_C6000_PCR_S12;
	  else if (fldd->low_pos == 13 && fldd->width == 10)
	    *fx_r_type = BFD_RELOC_C6000_PCR_S10;
	  else if (fldd->low_pos == 16 && fldd->width == 7)
	    *fx_r_type = BFD_RELOC_C6000_PCR_S7;
	  else
	    /* Opcode table should not use this encoding without a
	       known relocation.  */
	    abort ();
	  *fix_adda = FALSE;
	  break;

	case tic6x_coding_reg:
	  switch (operands[opno].form)
	    {
	    case TIC6X_OP_REG:
	    case TIC6X_OP_REGPAIR:
	      value = operands[opno].value.reg.num;
	      break;

	    case TIC6X_OP_MEM_NOUNREG:
	    case TIC6X_OP_MEM_UNREG:
	      value = operands[opno].value.mem.base_reg.num;
	      break;

	    default:
	      abort ();
	    }
	  break;

	case tic6x_coding_areg:
	  switch (operands[opno].form)
	    {
	    case TIC6X_OP_REG:
	      value = (operands[opno].value.reg.num == 15 ? 1 : 0);
	      break;

	    case TIC6X_OP_MEM_NOUNREG:
	      value = (operands[opno].value.mem.base_reg.num == 15 ? 1 : 0);
	      break;

	    default:
	      abort ();
	    }
	  break;

	case tic6x_coding_crlo:
	  if (operands[opno].form != TIC6X_OP_CTRL)
	    abort ();
	  value = tic6x_ctrl_table[operands[opno].value.ctrl].crlo;
	  break;

	case tic6x_coding_crhi:
	  if (operands[opno].form != TIC6X_OP_CTRL)
	    abort ();
	  value = 0;
	  break;

	case tic6x_coding_reg_shift:
	  if (operands[opno].form != TIC6X_OP_REGPAIR)
	    abort ();
	  value = operands[opno].value.reg.num >> 1;
	  break;

	case tic6x_coding_mem_offset:
	  if (operands[opno].form != TIC6X_OP_MEM_NOUNREG)
	    abort ();
	  mem = operands[opno].value.mem;
	  tic6x_default_mem_ref (&mem);
	  if (mem.offset_is_reg)
	    {
	      if (mem.scaled != tic6x_offset_scaled)
		abort ();
	      value = mem.offset.reg.num;
	    }
	  else
	    {
	      int scale;

	      if (mem.offset.exp.X_op != O_constant)
		abort ();
	      switch (mem.scaled)
		{
		case tic6x_offset_scaled:
		  scale = 1;
		  break;

		case tic6x_offset_unscaled:
		  scale = opct->operand_info[opno].size;
		  if (scale != 1 && scale != 2 && scale != 4 && scale != 8)
		    abort ();
		  break;

		default:
		  abort ();
		}
	      if (mem.offset.exp.X_add_number < 0
		  || mem.offset.exp.X_add_number >= (1 << fldd->width) * scale)
		{
		  if (print_errors)
		    as_bad (_("offset in operand %u of '%.*s' out of range"),
			    opno + 1, opc_len, str);
		  *ok = FALSE;
		  return 0;
		}
	      if (mem.offset.exp.X_add_number % scale)
		{
		  if (print_errors)
		    as_bad (_("offset in operand %u of '%.*s' not "
			      "divisible by %u"),
			    opno + 1, opc_len, str, scale);
		  *ok = FALSE;
		  return 0;
		}
	      value = mem.offset.exp.X_add_number / scale;
	    }
	  break;

	case tic6x_coding_mem_offset_noscale:
	  if (operands[opno].form != TIC6X_OP_MEM_UNREG)
	    abort ();
	  mem = operands[opno].value.mem;
	  tic6x_default_mem_ref (&mem);
	  if (mem.offset_is_reg)
	    value = mem.offset.reg.num;
	  else
	    {
	      if (mem.offset.exp.X_op != O_constant)
		abort ();
	      if (mem.offset.exp.X_add_number < 0
		  || mem.offset.exp.X_add_number >= (1 << fldd->width))
		{
		  if (print_errors)
		    as_bad (_("offset in operand %u of '%.*s' out of range"),
			    opno + 1, opc_len, str);
		  *ok = FALSE;
		  return 0;
		}
	      value = mem.offset.exp.X_add_number;
	    }
	  break;

	case tic6x_coding_mem_mode:
	  if (operands[opno].form != TIC6X_OP_MEM_NOUNREG
	      && operands[opno].form != TIC6X_OP_MEM_UNREG)
	    abort ();
	  mem = operands[opno].value.mem;
	  tic6x_default_mem_ref (&mem);
	  switch (mem.mod)
	    {
	    case tic6x_mem_mod_plus:
	      value = 1;
	      break;

	    case tic6x_mem_mod_minus:
	      value = 0;
	      break;

	    case tic6x_mem_mod_preinc:
	      value = 9;
	      break;

	    case tic6x_mem_mod_predec:
	      value = 8;
	      break;

	    case tic6x_mem_mod_postinc:
	      value = 11;
	      break;

	    case tic6x_mem_mod_postdec:
	      value = 10;
	      break;

	    default:
	      abort ();
	    }
	  value += (mem.offset_is_reg ? 4 : 0);
	  break;

	case tic6x_coding_scaled:
	  if (operands[opno].form != TIC6X_OP_MEM_UNREG)
	    abort ();
	  mem = operands[opno].value.mem;
	  tic6x_default_mem_ref (&mem);
	  switch (mem.scaled)
	    {
	    case tic6x_offset_unscaled:
	      value = 0;
	      break;

	    case tic6x_offset_scaled:
	      value = 1;
	      break;

	    default:
	      abort ();
	    }
	  break;

	case tic6x_coding_spmask:
	  /* The position of such a field is hardcoded in the handling
	     of "||^".  */
	  if (fldd->low_pos != 18)
	    abort ();
	  value = 0;
	  for (opno = 0; opno < num_operands; opno++)
	    {
	      unsigned int v;

	      v = tic6x_encode_spmask (operands[opno].value.func_unit.base,
				       operands[opno].value.func_unit.side);
	      if (value & v)
		{
		  if (print_errors)
		    as_bad (_("functional unit already masked for operand "
			      "%u of '%.*s'"), opno + 1, opc_len, str);
		  *ok = FALSE;
		  return 0;
		}
	      value |= v;
	    }
	  break;

	case tic6x_coding_reg_unused:
	  /* This is a placeholder; correct handling goes along with
	     resource constraint checks.  */
	  value = 0;
	  break;

	case tic6x_coding_fstg:
	case tic6x_coding_fcyc:
	  if (operands[opno].form != TIC6X_OP_EXP)
	    abort ();
	  if (operands[opno].value.exp.X_op != O_constant)
	    abort ();
	  if (!sploop_ii)
	    {
	      if (print_errors)
		as_bad (_("'%.*s' instruction not in a software "
			  "pipelined loop"),
			opc_len, str);
	      *ok = FALSE;
	      return 0;
	    }

	  if (sploop_ii <= 1)
	    fcyc_bits = 0;
	  else if (sploop_ii <= 2)
	    fcyc_bits = 1;
	  else if (sploop_ii <= 4)
	    fcyc_bits = 2;
	  else if (sploop_ii <= 8)
	    fcyc_bits = 3;
	  else if (sploop_ii <= 14)
	    fcyc_bits = 4;
	  else
	    abort ();
	  if (fcyc_bits > fldd->width)
	    abort ();

	  if (opct->variable_fields[fld].coding_method == tic6x_coding_fstg)
	    {
	      int i, t;
	      if (operands[opno].value.exp.X_add_number < 0
		  || (operands[opno].value.exp.X_add_number
		      >= (1 << (fldd->width - fcyc_bits))))
		{
		  if (print_errors)
		    as_bad (_("operand %u of '%.*s' out of range"), opno + 1,
			    opc_len, str);
		  *ok = FALSE;
		  return 0;
		}
	      value = operands[opno].value.exp.X_add_number;
	      for (t = 0, i = fcyc_bits; i < fldd->width; i++)
		{
		  t = (t << 1) | (value & 1);
		  value >>= 1;
		}
	      value = t << fcyc_bits;
	    }
	  else
	    {
	      if (operands[opno].value.exp.X_add_number < 0
		  || (operands[opno].value.exp.X_add_number >= sploop_ii))
		{
		  if (print_errors)
		    as_bad (_("operand %u of '%.*s' out of range"), opno + 1,
			    opc_len, str);
		  *ok = FALSE;
		  return 0;
		}
	      value = operands[opno].value.exp.X_add_number;
	    }
	  break;

	case tic6x_coding_fu:
	  value = func_unit_side == 2 ? 1 : 0;
	  break;

	case tic6x_coding_data_fu:
	  value = func_unit_data_side == 2 ? 1 : 0;
	  break;

	case tic6x_coding_xpath:
	  value = func_unit_cross;
	  break;

	default:
	  abort ();
	}

      for (ffld = 0; ffld < opct->num_fixed_fields; ffld++)
	if ((opct->fixed_fields[ffld].field_id
	     == opct->variable_fields[fld].field_id)
	    && (value < opct->fixed_fields[ffld].min_val
		|| value > opct->fixed_fields[ffld].max_val))
	  {
	    if (print_errors)
	      as_bad (_("operand %u of '%.*s' out of range"), opno + 1,
		      opc_len, str);
	    *ok = FALSE;
	    return 0;
	  }

      opcode_value |= value << fldd->low_pos;
    }

  if (this_line_creg)
    {
      const tic6x_insn_field *creg;
      const tic6x_insn_field *z;

      creg = tic6x_field_from_fmt (fmt, tic6x_field_creg);
      if (creg == NULL)
	{
	  if (print_errors)
	    as_bad (_("instruction '%.*s' cannot be predicated"),
		    opc_len, str);
	  *ok = FALSE;
	  return 0;
	}
      z = tic6x_field_from_fmt (fmt, tic6x_field_z);
      /* If there is a creg field, there must be a z field; otherwise
	 there is an error in the format table.  */
      if (z == NULL)
	abort ();

      opcode_value |= this_line_creg << creg->low_pos;
      opcode_value |= this_line_z << z->low_pos;
    }

  *ok = TRUE;
  return opcode_value;
}

/* Convert the target integer stored in N bytes in BUF to a host
   integer, returning that value.  */

static valueT
md_chars_to_number (char *buf, int n)
{
  valueT result = 0;
  unsigned char *p = (unsigned char *) buf;

  if (target_big_endian)
    {
      while (n--)
	{
	  result <<= 8;
	  result |= (*p++ & 0xff);
	}
    }
  else
    {
      while (n--)
	{
	  result <<= 8;
	  result |= (p[n] & 0xff);
	}
    }

  return result;
}

/* Assemble the instruction starting at STR (an opcode, with the
   opcode name all-lowercase).  */

void
md_assemble (char *str)
{
  char *p;
  int opc_len;
  bfd_boolean this_line_parallel;
  bfd_boolean this_line_spmask;
  unsigned int this_line_creg;
  unsigned int this_line_z;
  tic6x_label_list *this_insn_label_list;
  segment_info_type *seginfo;
  tic6x_opcode_list *opc_list, *opc;
  tic6x_func_unit_base func_unit_base = tic6x_func_unit_nfu;
  unsigned int func_unit_side = 0;
  unsigned int func_unit_cross = 0;
  unsigned int cross_side = 0;
  unsigned int func_unit_data_side = 0;
  unsigned int max_matching_opcodes, num_matching_opcodes;
  tic6x_opcode_id *opcm = NULL;
  unsigned int opc_rank[TIC6X_NUM_PREFER];
  const tic6x_opcode *opct = NULL;
  int min_rank, try_rank, max_rank;
  bfd_boolean num_operands_permitted[TIC6X_MAX_SOURCE_OPERANDS + 1]
    = { FALSE };
  unsigned int operand_forms[TIC6X_MAX_SOURCE_OPERANDS] = { 0 };
  tic6x_operand operands[TIC6X_MAX_SOURCE_OPERANDS];
  unsigned int max_num_operands;
  unsigned int num_operands_read;
  bfd_boolean ok_this_arch, ok_this_fu, ok_this_arch_fu;
  bfd_boolean bad_operands = FALSE;
  unsigned int opcode_value;
  bfd_boolean encoded_ok;
  bfd_boolean fix_needed = FALSE;
  expressionS *fix_exp = NULL;
  int fix_pcrel = 0;
  bfd_reloc_code_real_type fx_r_type = BFD_RELOC_UNUSED;
  bfd_boolean fix_adda = FALSE;
  fragS *insn_frag;
  char *output;

  p = str;
  while (*p && !is_end_of_line[(unsigned char) *p] && *p != ' ')
    p++;

  /* This function should only have been called when there is actually
     an instruction to assemble.  */
  if (p == str)
    abort ();

  /* Now an instruction has been seen, architecture attributes from
     .arch directives merge with rather than overriding the previous
     value.  */
  tic6x_seen_insns = TRUE;
  /* If no .arch directives or -march options have been seen, we are
     assessing instruction validity based on the C674X default, so set
     the attribute accordingly.  */
  if (tic6x_arch_attribute == C6XABI_Tag_ISA_none)
    tic6x_arch_attribute = C6XABI_Tag_ISA_C674X;

  /* Reset global settings for parallel bars and predicates now to
     avoid extra errors if there are problems with this opcode.  */
  this_line_parallel = tic6x_line_parallel;
  this_line_spmask = tic6x_line_spmask;
  this_line_creg = tic6x_line_creg;
  this_line_z = tic6x_line_z;
  tic6x_line_parallel = FALSE;
  tic6x_line_spmask = FALSE;
  tic6x_line_creg = 0;
  tic6x_line_z = 0;
  seginfo = seg_info (now_seg);
  this_insn_label_list = seginfo->tc_segment_info_data.label_list;
  seginfo->tc_segment_info_data.label_list = NULL;

  opc_list = hash_find_n (opcode_hash, str, p - str);
  if (opc_list == NULL)
    {
      char c = *p;
      *p = 0;
      as_bad (_("unknown opcode '%s'"), str);
      *p = c;
      return;
    }

  opc_len = p - str;
  skip_whitespace (p);

  /* See if there is something that looks like a functional unit
     specifier.  */
  if (*p == '.')
    {
      bfd_boolean good_func_unit;
      tic6x_func_unit_base maybe_base = tic6x_func_unit_nfu;
      unsigned int maybe_side = 0;
      unsigned int maybe_cross = 0;
      unsigned int maybe_data_side = 0;

      good_func_unit = tic6x_parse_func_unit_base (p + 1, &maybe_base,
						   &maybe_side);

      if (good_func_unit)
	{
	  if (p[3] == ' ' || is_end_of_line[(unsigned char) p[3]])
	    p += 3;
	  else if ((p[3] == 'x' || p[3] == 'X')
		   && (p[4] == ' ' || is_end_of_line[(unsigned char) p[4]]))
	    {
	      maybe_cross = 1;
	      p += 4;
	    }
	  else if (maybe_base == tic6x_func_unit_d
		   && (p[3] == 't' || p[3] == 'T')
		   && (p[4] == '1' || p[4] == '2')
		   && (p[5] == ' ' || is_end_of_line[(unsigned char) p[5]]))
	    {
	      maybe_data_side = p[4] - '0';
	      p += 5;
	    }
	  else
	    good_func_unit = FALSE;
	}

      if (good_func_unit)
	{
	  func_unit_base = maybe_base;
	  func_unit_side = maybe_side;
	  func_unit_cross = maybe_cross;
	  cross_side = (func_unit_cross ? 3 - func_unit_side : func_unit_side);
	  func_unit_data_side = maybe_data_side;
	}

      skip_whitespace (p);
    }

  /* Determine which entries in the opcode table match, and the
     associated permitted forms of operands.  */
  max_matching_opcodes = 0;
  for (opc = opc_list; opc; opc = opc->next)
    max_matching_opcodes++;
  num_matching_opcodes = 0;
  opcm = xmalloc (max_matching_opcodes * sizeof (*opcm));
  max_num_operands = 0;
  ok_this_arch = FALSE;
  ok_this_fu = FALSE;
  ok_this_arch_fu = FALSE;
  for (opc = opc_list; opc; opc = opc->next)
    {
      unsigned int num_operands;
      unsigned int i;
      bfd_boolean this_opc_arch_ok = TRUE;
      bfd_boolean this_opc_fu_ok = TRUE;

      if (tic6x_insn_format_table[tic6x_opcode_table[opc->id].format].num_bits
	  != 32)
	continue;
      if (!(tic6x_opcode_table[opc->id].isa_variants & tic6x_features))
	this_opc_arch_ok = FALSE;
      if (tic6x_opcode_table[opc->id].func_unit != func_unit_base)
	this_opc_fu_ok = FALSE;
      if (func_unit_side == 1
	  && (tic6x_opcode_table[opc->id].flags & TIC6X_FLAG_SIDE_B_ONLY))
	this_opc_fu_ok = FALSE;
      if (func_unit_cross
	  && (tic6x_opcode_table[opc->id].flags & TIC6X_FLAG_NO_CROSS))
	this_opc_fu_ok = FALSE;
      if (!func_unit_data_side
	  && (tic6x_opcode_table[opc->id].flags
	      & (TIC6X_FLAG_LOAD | TIC6X_FLAG_STORE)))
	this_opc_fu_ok = FALSE;
      if (func_unit_data_side
	  && !(tic6x_opcode_table[opc->id].flags
	       & (TIC6X_FLAG_LOAD | TIC6X_FLAG_STORE)))
	this_opc_fu_ok = FALSE;
      if (func_unit_data_side == 1
	  && (tic6x_opcode_table[opc->id].flags & TIC6X_FLAG_SIDE_T2_ONLY))
	this_opc_fu_ok = FALSE;
      if (this_opc_arch_ok)
	ok_this_arch = TRUE;
      if (this_opc_fu_ok)
	ok_this_fu = TRUE;
      if (!this_opc_arch_ok || !this_opc_fu_ok)
	continue;
      ok_this_arch_fu = TRUE;
      opcm[num_matching_opcodes] = opc->id;
      num_matching_opcodes++;
      num_operands = tic6x_opcode_table[opc->id].num_operands;

      if (tic6x_opcode_table[opc->id].flags & TIC6X_FLAG_SPMASK)
	{
	  if (num_operands != 1
	      || (tic6x_opcode_table[opc->id].operand_info[0].form
		  != tic6x_operand_func_unit))
	    abort ();
	  num_operands = 8;
	  for (i = 0; i < num_operands; i++)
	    {
	      operand_forms[i]
		|= tic6x_coarse_operand_form (tic6x_operand_func_unit);
	      num_operands_permitted[i] = TRUE;
	    }
	}
      else
	{
	  for (i = 0; i < num_operands; i++)
	    {
	      tic6x_operand_form f
		= tic6x_opcode_table[opc->id].operand_info[i].form;

	      operand_forms[i] |= tic6x_coarse_operand_form (f);
	    }
	}
      num_operands_permitted[num_operands] = TRUE;
      if (num_operands > max_num_operands)
	max_num_operands = num_operands;
    }

  if (!ok_this_arch)
    {
      as_bad (_("'%.*s' instruction not supported on this architecture"),
	      opc_len, str);
      free (opcm);
      return;
    }

  if (!ok_this_fu)
    {
      as_bad (_("'%.*s' instruction not supported on this functional unit"),
	      opc_len, str);
      free (opcm);
      return;
    }

  if (!ok_this_arch_fu)
    {
      as_bad (_("'%.*s' instruction not supported on this functional unit"
		" for this architecture"),
	      opc_len, str);
      free (opcm);
      return;
    }

  /* If there were no instructions matching the above availability
     checks, we should now have given an error and returned.  */
  if (num_matching_opcodes == 0)
    abort ();

  num_operands_read = 0;
  while (TRUE)
    {
      skip_whitespace (p);
      if (is_end_of_line[(unsigned char) *p])
	{
	  if (num_operands_read > 0)
	    {
	      as_bad (_("missing operand after comma"));
	      bad_operands = TRUE;
	    }
	  break;
	}

      if (max_num_operands == 0)
	{
	  as_bad (_("too many operands to '%.*s'"), opc_len, str);
	  bad_operands = TRUE;
	  break;
	}

      if (!tic6x_parse_operand (&p, &operands[num_operands_read],
				operand_forms[num_operands_read], str, opc_len,
				num_operands_read + 1))
	bad_operands = TRUE;
      num_operands_read++;

      if (is_end_of_line[(unsigned char) *p])
	break;
      else if (*p == ',')
	{
	  p++;
	  if (num_operands_read == max_num_operands)
	    {
	      as_bad (_("too many operands to '%.*s'"), opc_len, str);
	      bad_operands = TRUE;
	      break;
	    }
	  continue;
	}
      else
	/* Operand parsing should consume whole operands.  */
	abort ();
    }

  if (!bad_operands && !num_operands_permitted[num_operands_read])
    {
      as_bad (_("bad number of operands to '%.*s'"), opc_len, str);
      bad_operands = TRUE;
    }

  if (!bad_operands)
    {
      /* Each operand is of the right syntactic form for some opcode
	 choice, and the number of operands is valid.  Check that each
	 operand is OK in detail for some opcode choice with the right
	 number of operands.  */
      unsigned int i;

      for (i = 0; i < num_operands_read; i++)
	{
	  bfd_boolean coarse_ok = FALSE;
	  bfd_boolean fine_ok = FALSE;
	  tic6x_operand_match fine_failure = tic6x_match_matches;
	  unsigned int j;

	  for (j = 0; j < num_matching_opcodes; j++)
	    {
	      tic6x_operand_form f;
	      tic6x_rw rw;
	      unsigned int cf;
	      tic6x_operand_match this_fine_failure;

	      if (tic6x_opcode_table[opcm[j]].flags & TIC6X_FLAG_SPMASK)
		{
		  f = tic6x_operand_func_unit;
		  rw = tic6x_rw_none;
		}
	      else
		{
		  if (tic6x_opcode_table[opcm[j]].num_operands
		      != num_operands_read)
		    continue;

		  f = tic6x_opcode_table[opcm[j]].operand_info[i].form;
		  rw = tic6x_opcode_table[opcm[j]].operand_info[i].rw;
		}
	      cf = tic6x_coarse_operand_form (f);

	      if (operands[i].form != cf)
		continue;

	      coarse_ok = TRUE;
	      this_fine_failure
		= tic6x_operand_matches_form (&operands[i], f, rw,
					      func_unit_side,
					      cross_side,
					      func_unit_data_side);
	      if (this_fine_failure == tic6x_match_matches)
		{
		  fine_ok = TRUE;
		  break;
		}
	      if (fine_failure == tic6x_match_matches
		  || fine_failure > this_fine_failure)
		fine_failure = this_fine_failure;
	    }

	  /* No instructions should have operand syntactic forms only
	     acceptable with certain numbers of operands, so no
	     diagnostic for this case.  */
	  if (!coarse_ok)
	    abort ();

	  if (!fine_ok)
	    {
	      switch (fine_failure)
		{
		case tic6x_match_non_const:
		  as_bad (_("operand %u of '%.*s' not constant"),
			  i + 1, opc_len, str);
		  break;

		case tic6x_match_wrong_side:
		  as_bad (_("operand %u of '%.*s' on wrong side"),
			  i + 1, opc_len, str);
		  break;

		case tic6x_match_bad_return:
		  as_bad (_("operand %u of '%.*s' not a valid return "
			    "address register"),
			  i + 1, opc_len, str);
		  break;

		case tic6x_match_ctrl_write_only:
		  as_bad (_("operand %u of '%.*s' is write-only"),
			  i + 1, opc_len, str);
		  break;

		case tic6x_match_ctrl_read_only:
		  as_bad (_("operand %u of '%.*s' is read-only"),
			  i + 1, opc_len, str);
		  break;

		case tic6x_match_bad_mem:
		  as_bad (_("operand %u of '%.*s' not a valid memory "
			    "reference"),
			  i + 1, opc_len, str);
		  break;

		case tic6x_match_bad_address:
		  as_bad (_("operand %u of '%.*s' not a valid base "
			    "address register"),
			  i + 1, opc_len, str);
		  break;

		default:
		  abort ();
		}
	      bad_operands = TRUE;
	      break;
	    }
	}
    }

  if (!bad_operands)
    {
      /* Each operand is OK for some opcode choice, and the number of
	 operands is valid.  Check whether there is an opcode choice
	 for which all operands are simultaneously valid.  */
      unsigned int i;
      bfd_boolean found_match = FALSE;

      for (i = 0; i < TIC6X_NUM_PREFER; i++)
	opc_rank[i] = (unsigned int) -1;

      min_rank = TIC6X_NUM_PREFER - 1;
      max_rank = 0;

      for (i = 0; i < num_matching_opcodes; i++)
	{
	  unsigned int j;
	  bfd_boolean this_matches = TRUE;

	  if (!(tic6x_opcode_table[opcm[i]].flags & TIC6X_FLAG_SPMASK)
	      && tic6x_opcode_table[opcm[i]].num_operands != num_operands_read)
	    continue;

	  for (j = 0; j < num_operands_read; j++)
	    {
	      tic6x_operand_form f;
	      tic6x_rw rw;

	      if (tic6x_opcode_table[opcm[i]].flags & TIC6X_FLAG_SPMASK)
		{
		  f = tic6x_operand_func_unit;
		  rw = tic6x_rw_none;
		}
	      else
		{
		  f = tic6x_opcode_table[opcm[i]].operand_info[j].form;
		  rw = tic6x_opcode_table[opcm[i]].operand_info[j].rw;
		}
	      if (tic6x_operand_matches_form (&operands[j], f, rw,
					      func_unit_side,
					      cross_side,
					      func_unit_data_side)
		  != tic6x_match_matches)
		{
		  this_matches = FALSE;
		  break;
		}
	    }

	  if (this_matches)
	    {
	      int rank = TIC6X_PREFER_VAL (tic6x_opcode_table[opcm[i]].flags);

	      if (rank < min_rank)
		min_rank = rank;
	      if (rank > max_rank)
		max_rank = rank;

	      if (opc_rank[rank] == (unsigned int) -1)
		opc_rank[rank] = i;
	      else
		/* The opcode table should provide a total ordering
		   for all cases where multiple matches may get
		   here.  */
		abort ();

	      found_match = TRUE;
	    }
	}

      if (!found_match)
	{
	  as_bad (_("bad operand combination for '%.*s'"), opc_len, str);
	  bad_operands = TRUE;
	}
    }

  if (bad_operands)
    {
      free (opcm);
      return;
    }

  opcode_value = 0;
  encoded_ok = FALSE;
  for (try_rank = max_rank; try_rank >= min_rank; try_rank--)
    {
      fix_needed = FALSE;

      if (opc_rank[try_rank] == (unsigned int) -1)
	continue;

      opcode_value = tic6x_try_encode (opcm[opc_rank[try_rank]], operands,
				       num_operands_read, this_line_creg,
				       this_line_z, func_unit_side,
				       func_unit_cross, func_unit_data_side,
				       seginfo->tc_segment_info_data.sploop_ii,
				       &fix_exp, &fix_pcrel, &fx_r_type,
				       &fix_adda, &fix_needed, &encoded_ok,
				       (try_rank == min_rank ? TRUE : FALSE),
				       str, opc_len);
      if (encoded_ok)
	{
	  opct = &tic6x_opcode_table[opcm[opc_rank[try_rank]]];
	  break;
	}
    }

  free (opcm);

  if (!encoded_ok)
    return;

  if (this_line_parallel)
    {
      insn_frag = seginfo->tc_segment_info_data.execute_packet_frag;
      if (insn_frag == NULL)
	{
	  as_bad (_("parallel instruction not following another instruction"));
	  return;
	}

      if (insn_frag->fr_fix >= 32)
	{
	  as_bad (_("too many instructions in execute packet"));
	  return;
	}

      if (this_insn_label_list != NULL)
	as_bad (_("label not at start of execute packet"));

      if (opct->flags & TIC6X_FLAG_FIRST)
	as_bad (_("'%.*s' instruction not at start of execute packet"),
		opc_len, str);

      *seginfo->tc_segment_info_data.last_insn_lsb |= 0x1;
      output = insn_frag->fr_literal + insn_frag->fr_fix;
    }
  else
    {
      tic6x_label_list *l;

      seginfo->tc_segment_info_data.spmask_addr = NULL;
      seginfo->tc_segment_info_data.func_units_used = 0;

      /* Start a new frag for this execute packet.  */
      if (frag_now_fix () != 0)
	{
	  if (frag_now->fr_type != rs_machine_dependent)
	    frag_wane (frag_now);

	  frag_new (0);
	}
      frag_grow (32);
      insn_frag = seginfo->tc_segment_info_data.execute_packet_frag = frag_now;
      for (l = this_insn_label_list; l; l = l->next)
	{
	  symbol_set_frag (l->label, frag_now);
	  S_SET_VALUE (l->label, 0);
	  S_SET_SEGMENT (l->label, now_seg);
	}
      tic6x_free_label_list (this_insn_label_list);
      dwarf2_emit_insn (0);
      output = frag_var (rs_machine_dependent, 32, 32, 0, NULL, 0, NULL);
      /* This must be the same as the frag to which a pointer was just
	 saved.  */
      if (output != insn_frag->fr_literal)
	abort ();
      insn_frag->tc_frag_data.is_insns = TRUE;
      insn_frag->tc_frag_data.can_cross_fp_boundary
	= tic6x_can_cross_fp_boundary;
    }

  if (func_unit_base != tic6x_func_unit_nfu)
    {
      unsigned int func_unit_enc;

      func_unit_enc = tic6x_encode_spmask (func_unit_base, func_unit_side);

      if (seginfo->tc_segment_info_data.func_units_used & func_unit_enc)
	as_bad (_("functional unit already used in this execute packet"));

      seginfo->tc_segment_info_data.func_units_used |= func_unit_enc;
    }

  if (opct->flags & TIC6X_FLAG_SPLOOP)
    {
      if (seginfo->tc_segment_info_data.sploop_ii)
	as_bad (_("nested software pipelined loop"));
      if (num_operands_read != 1
	  || operands[0].form != TIC6X_OP_EXP
	  || operands[0].value.exp.X_op != O_constant)
	abort ();
      seginfo->tc_segment_info_data.sploop_ii
	= operands[0].value.exp.X_add_number;
    }
  else if (opct->flags & TIC6X_FLAG_SPKERNEL)
    {
      if (!seginfo->tc_segment_info_data.sploop_ii)
	as_bad (_("'%.*s' instruction not in a software pipelined loop"),
		opc_len, str);
      seginfo->tc_segment_info_data.sploop_ii = 0;
    }

  if (this_line_spmask)
    {
      if (seginfo->tc_segment_info_data.spmask_addr == NULL)
	as_bad (_("'||^' without previous SPMASK"));
      else if (func_unit_base == tic6x_func_unit_nfu)
	as_bad (_("cannot mask instruction using no functional unit"));
      else
	{
	  unsigned int spmask_opcode;
	  unsigned int mask_bit;

	  spmask_opcode
	    = md_chars_to_number (seginfo->tc_segment_info_data.spmask_addr,
				  4);
	  mask_bit = tic6x_encode_spmask (func_unit_base, func_unit_side);
	  mask_bit <<= 18;
	  if (spmask_opcode & mask_bit)
	    as_bad (_("functional unit already masked"));
	  spmask_opcode |= mask_bit;
	  md_number_to_chars (seginfo->tc_segment_info_data.spmask_addr,
			      spmask_opcode, 4);
	}
    }

  record_alignment (now_seg, 5);
  md_number_to_chars (output, opcode_value, 4);
  if (fix_needed)
    tic6x_fix_new_exp (insn_frag, output - insn_frag->fr_literal, 4, fix_exp,
		       fix_pcrel, fx_r_type, fix_adda);
  insn_frag->fr_fix += 4;
  insn_frag->fr_var -= 4;
  seginfo->tc_segment_info_data.last_insn_lsb
    = (target_big_endian ? output + 3 : output);
  if (opct->flags & TIC6X_FLAG_SPMASK)
    seginfo->tc_segment_info_data.spmask_addr = output;
}

/* Modify NEWVAL (32-bit) by inserting VALUE, shifted right by SHIFT
   and the least significant BITS bits taken, at position POS.  */
#define MODIFY_VALUE(NEWVAL, VALUE, SHIFT, POS, BITS)			\
  do {									\
    (NEWVAL) &= 0xffffffffU & ~(((1U << (BITS)) - 1) << (POS));		\
    (NEWVAL) |= (((VALUE) >> (SHIFT)) & ((1U << (BITS)) - 1)) << (POS);	\
  } while (0)

/* Apply a fixup to the object file.  */

void
md_apply_fix (fixS *fixP, valueT *valP, segT seg ATTRIBUTE_UNUSED)
{
  offsetT value = *valP;
  char *buf = fixP->fx_where + fixP->fx_frag->fr_literal;

  value = SEXT (value);
  *valP = value;

  fixP->fx_offset = SEXT (fixP->fx_offset);

  if (fixP->fx_addsy == NULL && fixP->fx_pcrel == 0)
    fixP->fx_done = 1;

  /* We do our own overflow checks.  */
  fixP->fx_no_overflow = 1;

  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_NONE:
      /* Force output to the object file.  */
      fixP->fx_done = 0;
      break;

    case BFD_RELOC_32:
      if (fixP->fx_done || !seg->use_rela_p)
	md_number_to_chars (buf, value, 4);
      break;

    case BFD_RELOC_16:
      if (fixP->fx_done || !seg->use_rela_p)
	{
	  if (value < -0x8000 || value > 0xffff)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("value too large for 2-byte field"));
	  md_number_to_chars (buf, value, 2);
	}
      break;

    case BFD_RELOC_8:
      if (fixP->fx_done || !seg->use_rela_p)
	{
	  if (value < -0x80 || value > 0xff)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("value too large for 1-byte field"));
	  md_number_to_chars (buf, value, 1);
	}
      break;

    case BFD_RELOC_C6000_ABS_S16:
    case BFD_RELOC_C6000_ABS_L16:
    case BFD_RELOC_C6000_SBR_S16:
    case BFD_RELOC_C6000_SBR_L16_B:
    case BFD_RELOC_C6000_SBR_L16_H:
    case BFD_RELOC_C6000_SBR_L16_W:
    case BFD_RELOC_C6000_SBR_GOT_L16_W:
      if (fixP->fx_done || !seg->use_rela_p)
	{
	  offsetT newval = md_chars_to_number (buf, 4);
	  int shift;

	  switch (fixP->fx_r_type)
	    {
	    case BFD_RELOC_C6000_SBR_L16_H:
	      shift = 1;
	      break;

	    case BFD_RELOC_C6000_SBR_L16_W:
	    case BFD_RELOC_C6000_SBR_GOT_L16_W:
	      shift = 2;
	      break;

	    default:
	      shift = 0;
	      break;
	    }

	  MODIFY_VALUE (newval, value, shift, 7, 16);
	  if ((value < -0x8000 || value > 0x7fff)
	      && (fixP->fx_r_type == BFD_RELOC_C6000_ABS_S16
		  || fixP->fx_r_type == BFD_RELOC_C6000_SBR_S16))
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("immediate offset out of range"));

	  md_number_to_chars (buf, newval, 4);
	}
      if (fixP->fx_done
	  && fixP->fx_r_type != BFD_RELOC_C6000_ABS_S16
	  && fixP->fx_r_type != BFD_RELOC_C6000_ABS_L16)
	abort ();
      break;

    case BFD_RELOC_C6000_ABS_H16:
    case BFD_RELOC_C6000_SBR_H16_B:
    case BFD_RELOC_C6000_SBR_H16_H:
    case BFD_RELOC_C6000_SBR_H16_W:
    case BFD_RELOC_C6000_SBR_GOT_H16_W:
      if (fixP->fx_done || !seg->use_rela_p)
	{
	  offsetT newval = md_chars_to_number (buf, 4);
	  int shift;

	  switch (fixP->fx_r_type)
	    {
	    case BFD_RELOC_C6000_SBR_H16_H:
	      shift = 17;
	      break;

	    case BFD_RELOC_C6000_SBR_H16_W:
	    case BFD_RELOC_C6000_SBR_GOT_H16_W:
	      shift = 18;
	      break;

	    default:
	      shift = 16;
	      break;
	    }

	  MODIFY_VALUE (newval, value, shift, 7, 16);

	  md_number_to_chars (buf, newval, 4);
	}
      if (fixP->fx_done && fixP->fx_r_type != BFD_RELOC_C6000_ABS_H16)
	abort ();
      break;

    case BFD_RELOC_C6000_SBR_U15_B:
      if (fixP->fx_done || !seg->use_rela_p)
	{
	  offsetT newval = md_chars_to_number (buf, 4);

	  MODIFY_VALUE (newval, value, 0, 8, 15);
	  if (value < 0 || value > 0x7fff)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("immediate offset out of range"));

	  md_number_to_chars (buf, newval, 4);
	}
      break;

    case BFD_RELOC_C6000_SBR_U15_H:
      if (fixP->fx_done || !seg->use_rela_p)
	{
	  offsetT newval = md_chars_to_number (buf, 4);

	  /* Constant ADDA operands, processed as constant when the
	     instruction is parsed, are encoded as-is rather than
	     shifted.  If the operand of an ADDA instruction is now
	     constant (for example, the difference between two labels
	     found after the instruction), ensure it is encoded the
	     same way it would have been if the constant value had
	     been known when the instruction was parsed.  */
	  if (fixP->tc_fix_data.fix_adda && fixP->fx_done)
	    value <<= 1;

	  MODIFY_VALUE (newval, value, 1, 8, 15);
	  if (value & 1)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("immediate offset not 2-byte-aligned"));
	  if (value < 0 || value > 0xfffe)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("immediate offset out of range"));

	  md_number_to_chars (buf, newval, 4);
	}
      break;

    case BFD_RELOC_C6000_SBR_U15_W:
    case BFD_RELOC_C6000_SBR_GOT_U15_W:
      if (fixP->fx_done || !seg->use_rela_p)
	{
	  offsetT newval = md_chars_to_number (buf, 4);

	  /* Constant ADDA operands, processed as constant when the
	     instruction is parsed, are encoded as-is rather than
	     shifted.  If the operand of an ADDA instruction is now
	     constant (for example, the difference between two labels
	     found after the instruction), ensure it is encoded the
	     same way it would have been if the constant value had
	     been known when the instruction was parsed.  */
	  if (fixP->tc_fix_data.fix_adda && fixP->fx_done)
	    value <<= 2;

	  MODIFY_VALUE (newval, value, 2, 8, 15);
	  if (value & 3)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("immediate offset not 4-byte-aligned"));
	  if (value < 0 || value > 0x1fffc)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("immediate offset out of range"));

	  md_number_to_chars (buf, newval, 4);
	}
      if (fixP->fx_done && fixP->fx_r_type != BFD_RELOC_C6000_SBR_U15_W)
	abort ();
      break;

    case BFD_RELOC_C6000_DSBT_INDEX:
      if (value != 0)
	as_bad_where (fixP->fx_file, fixP->fx_line,
		      _("addend used with $DSBT_INDEX"));
      if (fixP->fx_done)
	abort ();
      break;

    case BFD_RELOC_C6000_PCR_S21:
      if (fixP->fx_done || !seg->use_rela_p)
	{
	  offsetT newval = md_chars_to_number (buf, 4);

	  MODIFY_VALUE (newval, value, 2, 7, 21);

	  if (value & 3)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("PC-relative offset not 4-byte-aligned"));
	  if (value < -0x400000 || value > 0x3ffffc)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("PC-relative offset out of range"));

	  md_number_to_chars (buf, newval, 4);
	}
      break;

    case BFD_RELOC_C6000_PCR_S12:
      if (fixP->fx_done || !seg->use_rela_p)
	{
	  offsetT newval = md_chars_to_number (buf, 4);

	  MODIFY_VALUE (newval, value, 2, 16, 12);

	  if (value & 3)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("PC-relative offset not 4-byte-aligned"));
	  if (value < -0x2000 || value > 0x1ffc)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("PC-relative offset out of range"));

	  md_number_to_chars (buf, newval, 4);
	}
      break;

    case BFD_RELOC_C6000_PCR_S10:
      if (fixP->fx_done || !seg->use_rela_p)
	{
	  offsetT newval = md_chars_to_number (buf, 4);

	  MODIFY_VALUE (newval, value, 2, 13, 10);

	  if (value & 3)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("PC-relative offset not 4-byte-aligned"));
	  if (value < -0x800 || value > 0x7fc)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("PC-relative offset out of range"));

	  md_number_to_chars (buf, newval, 4);
	}
      break;

    case BFD_RELOC_C6000_PCR_S7:
      if (fixP->fx_done || !seg->use_rela_p)
	{
	  offsetT newval = md_chars_to_number (buf, 4);

	  MODIFY_VALUE (newval, value, 2, 16, 7);

	  if (value & 3)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("PC-relative offset not 4-byte-aligned"));
	  if (value < -0x100 || value > 0xfc)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("PC-relative offset out of range"));

	  md_number_to_chars (buf, newval, 4);
	}
      break;

    default:
      abort ();
    }
}

/* Convert a floating-point number to target (IEEE) format.  */

char *
md_atof (int type, char *litP, int *sizeP)
{
  return ieee_md_atof (type, litP, sizeP, target_big_endian);
}

/* Adjust the frags in SECTION (see tic6x_end).  */

static void
tic6x_adjust_section (bfd *abfd ATTRIBUTE_UNUSED, segT section,
		      void *dummy ATTRIBUTE_UNUSED)
{
  segment_info_type *info;
  frchainS *frchp;
  fragS *fragp;
  bfd_boolean have_code = FALSE;
  bfd_boolean have_non_code = FALSE;

  info = seg_info (section);
  if (info == NULL)
    return;

  for (frchp = info->frchainP; frchp; frchp = frchp->frch_next)
    for (fragp = frchp->frch_root; fragp; fragp = fragp->fr_next)
      switch (fragp->fr_type)
	{
	case rs_machine_dependent:
	  if (fragp->tc_frag_data.is_insns)
	    have_code = TRUE;
	  break;

	case rs_dummy:
	case rs_fill:
	  if (fragp->fr_fix > 0)
	    have_non_code = TRUE;
	  break;

	default:
	  have_non_code = TRUE;
	  break;
	}

  /* Process alignment requirements in a code-only section.  */
  if (have_code && !have_non_code)
    {
      /* If we need to insert an odd number of instructions to meet an
	 alignment requirement, there must have been an odd number of
	 instructions since the last 8-byte-aligned execute packet
	 boundary.  So there must have been an execute packet with an
	 odd number (and so a number fewer than 8) of instructions
	 into which we can insert a NOP without breaking any previous
	 alignments.

	 If then we need to insert a number 2 mod 4 of instructions,
	 the number of instructions since the last 16-byte-aligned
	 execute packet boundary must be 2 mod 4.  So between that
	 boundary and the following 8-byte-aligned boundary there must
	 either be at least one execute packet with 2-mod-4
	 instructions, or at least two with an odd number of
	 instructions; again, greedily inserting NOPs as soon as
	 possible suffices to meet the alignment requirement.

	 If then we need to insert 4 instructions, we look between the
	 last 32-byte-aligned boundary and the following
	 16-byte-aligned boundary.  The sizes of the execute packets
	 in this range total 4 instructions mod 8, so again there is
	 room for greedy insertion of NOPs to meet the alignment
	 requirement, and before any intermediate point with 8-byte
	 (2-instruction) alignment requirement the sizes of execute
	 packets (and so the room for NOPs) will total 2 instructions
	 mod 4 so greedy insertion will not break such alignments.

	 So we can always meet these alignment requirements by
	 inserting NOPs in parallel with existing execute packets, and
	 by induction the approach described above inserts the minimum
	 number of such NOPs.  */

      /* The number of NOPs we are currently looking to insert, if we
	 have gone back to insert NOPs.  */
      unsigned int want_insert = 0;

      /* Out of that number, the number inserted so far in the current
	 stage of the above algorithm.  */
      unsigned int want_insert_done_so_far = 0;

      /* The position mod 32 at the start of the current frag.  */
      unsigned int pos = 0;

      /* The locations in the frag chain of the most recent frags at
	 the start of which there is the given alignment.  */
      frchainS *frchp_last32, *frchp_last16, *frchp_last8;
      fragS *fragp_last32, *fragp_last16, *fragp_last8;
      unsigned int pos_last32, pos_last16, pos_last8;

      frchp_last32 = frchp_last16 = frchp_last8 = info->frchainP;
      fragp_last32 = fragp_last16 = fragp_last8 = info->frchainP->frch_root;
      pos_last32 = pos_last16 = pos_last8 = 0;

      for (frchp = info->frchainP; frchp; frchp = frchp->frch_next)
	for (fragp = frchp->frch_root; fragp; fragp = fragp->fr_next)
	look_at_frag:
	  {
	    bfd_boolean go_back = FALSE;
	    frchainS *frchp_next;
	    fragS *fragp_next;

	    if (fragp->fr_type != rs_machine_dependent)
	      continue;

	    if (fragp->tc_frag_data.is_insns
		&& pos + fragp->fr_fix > 32
		&& !fragp->tc_frag_data.can_cross_fp_boundary)
	      {
		/* As described above, we should always have met an
		   alignment requirement by the time we come back to
		   it.  */
		if (want_insert)
		  abort ();

		if (pos & 3)
		  abort ();
		want_insert = (32 - pos) >> 2;
		if (want_insert > 7)
		  abort ();
		want_insert_done_so_far = 0;
		go_back = TRUE;
	      }

	    if (!fragp->tc_frag_data.is_insns)
	      {
		unsigned int would_insert_bytes;

		if (!(pos & ((1 << fragp->fr_offset) - 1)))
		  /* This alignment requirement is already met.  */
		  continue;

		/* As described above, we should always have met an
		   alignment requirement by the time we come back to
		   it.  */
		if (want_insert)
		  abort ();

		/* We may not be able to meet this requirement within
		   the given number of characters.  */
		would_insert_bytes
		  = ((1 << fragp->fr_offset)
		     - (pos & ((1 << fragp->fr_offset) - 1)));

		if (fragp->fr_subtype != 0
		    && would_insert_bytes > fragp->fr_subtype)
		  continue;

		/* An unmet alignment must be 8, 16 or 32 bytes;
		   smaller ones must always be met within code-only
		   sections and larger ones cause the section not to
		   be code-only.  */
		if (fragp->fr_offset != 3
		    && fragp->fr_offset != 4
		    && fragp->fr_offset != 5)
		  abort ();

		if (would_insert_bytes & 3)
		  abort ();
		want_insert = would_insert_bytes >> 2;
		if (want_insert > 7)
		  abort ();
		want_insert_done_so_far = 0;
		go_back = TRUE;
	      }
	    else if (want_insert && !go_back)
	      {
		unsigned int num_insns = fragp->fr_fix >> 2;
		unsigned int max_poss_nops = 8 - num_insns;

		if (max_poss_nops)
		  {
		    unsigned int cur_want_nops, max_want_nops, do_nops, i;

		    if (want_insert & 1)
		      cur_want_nops = 1;
		    else if (want_insert & 2)
		      cur_want_nops = 2;
		    else if (want_insert & 4)
		      cur_want_nops = 4;
		    else
		      abort ();

		    max_want_nops = cur_want_nops - want_insert_done_so_far;

		    do_nops = (max_poss_nops < max_want_nops
			       ? max_poss_nops
			       : max_want_nops);
		    for (i = 0; i < do_nops; i++)
		      {
			md_number_to_chars (fragp->fr_literal + fragp->fr_fix,
					    0, 4);
			if (target_big_endian)
			  fragp->fr_literal[fragp->fr_fix - 1] |= 0x1;
			else
			  fragp->fr_literal[fragp->fr_fix - 4] |= 0x1;
			fragp->fr_fix += 4;
			fragp->fr_var -= 4;
		      }
		    want_insert_done_so_far += do_nops;
		    if (want_insert_done_so_far == cur_want_nops)
		      {
			want_insert -= want_insert_done_so_far;
			want_insert_done_so_far = 0;
			if (want_insert)
			  go_back = TRUE;
		      }
		  }
	      }
	    if (go_back)
	      {
		if (want_insert & 1)
		  {
		    frchp = frchp_last8;
		    fragp = fragp_last8;
		    pos = pos_last8;
		  }
		else if (want_insert & 2)
		  {
		    frchp = frchp_last8 = frchp_last16;
		    fragp = fragp_last8 = fragp_last16;
		    pos = pos_last8 = pos_last16;
		  }
		else if (want_insert & 4)
		  {
		    frchp = frchp_last8 = frchp_last16 = frchp_last32;
		    fragp = fragp_last8 = fragp_last16 = fragp_last32;
		    pos = pos_last8 = pos_last16 = pos_last32;
		  }
		else
		  abort ();

		goto look_at_frag;
	      }

	    /* Update current position for moving past a code
	       frag.  */
	    pos += fragp->fr_fix;
	    pos &= 31;
	    frchp_next = frchp;
	    fragp_next = fragp->fr_next;
	    if (fragp_next == NULL)
	      {
		frchp_next = frchp->frch_next;
		if (frchp_next != NULL)
		  fragp_next = frchp_next->frch_root;
	      }
	    if (!(pos & 7))
	      {
		frchp_last8 = frchp_next;
		fragp_last8 = fragp_next;
		pos_last8 = pos;
	      }
	    if (!(pos & 15))
	      {
		frchp_last16 = frchp_next;
		fragp_last16 = fragp_next;
		pos_last16 = pos;
	      }
	    if (!(pos & 31))
	      {
		frchp_last32 = frchp_next;
		fragp_last32 = fragp_next;
		pos_last32 = pos;
	      }
	  }
    }

  /* Now convert the machine-dependent frags to machine-independent
     ones.  */
  for (frchp = info->frchainP; frchp; frchp = frchp->frch_next)
    for (fragp = frchp->frch_root; fragp; fragp = fragp->fr_next)
      {
	if (fragp->fr_type == rs_machine_dependent)
	  {
	    if (fragp->tc_frag_data.is_insns)
	      frag_wane (fragp);
	    else
	      {
		fragp->fr_type = rs_align_code;
		fragp->fr_var = 1;
		*fragp->fr_literal = 0;
	      }
	  }
      }
}

/* Initialize the machine-dependent parts of a frag.  */

void
tic6x_frag_init (fragS *fragp)
{
  fragp->tc_frag_data.is_insns = FALSE;
  fragp->tc_frag_data.can_cross_fp_boundary = FALSE;
}

/* Set an attribute if it has not already been set by the user.  */

static void
tic6x_set_attribute_int (int tag, int value)
{
  if (tag < 1
      || tag >= NUM_KNOWN_OBJ_ATTRIBUTES)
    abort ();
  if (!tic6x_attributes_set_explicitly[tag])
    bfd_elf_add_proc_attr_int (stdoutput, tag, value);
}

/* Set object attributes deduced from the input file and command line
   rather than given explicitly.  */
static void
tic6x_set_attributes (void)
{
  if (tic6x_arch_attribute == C6XABI_Tag_ISA_none)
    tic6x_arch_attribute = C6XABI_Tag_ISA_C674X;

  tic6x_set_attribute_int (Tag_ISA, tic6x_arch_attribute);
  tic6x_set_attribute_int (Tag_ABI_DSBT, tic6x_dsbt);
  tic6x_set_attribute_int (Tag_ABI_PID, tic6x_pid);
  tic6x_set_attribute_int (Tag_ABI_PIC, tic6x_pic);
}

/* Do machine-dependent manipulations of the frag chains after all
   input has been read and before the machine-independent sizing and
   relaxing.  */

void
tic6x_end (void)
{
  /* Set object attributes at this point if not explicitly set.  */
  tic6x_set_attributes ();

  /* Meeting alignment requirements may require inserting NOPs in
     parallel in execute packets earlier in the segment.  Future
     16-bit instruction generation involves whole-segment optimization
     to determine the best choice and ordering of 32-bit or 16-bit
     instructions.  This doesn't fit will in the general relaxation
     framework, so handle alignment and 16-bit instruction generation
     here.  */
  bfd_map_over_sections (stdoutput, tic6x_adjust_section, NULL);
}

/* No machine-dependent frags at this stage; all converted in
   tic6x_end.  */

void
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED, segT asec ATTRIBUTE_UNUSED,
		 fragS *fragp ATTRIBUTE_UNUSED)
{
  abort ();
}

/* No machine-dependent frags at this stage; all converted in
   tic6x_end.  */

int
md_estimate_size_before_relax (fragS *fragp ATTRIBUTE_UNUSED,
			       segT seg ATTRIBUTE_UNUSED)
{
  abort ();
}

/* Put a number into target byte order.  */

void
md_number_to_chars (char *buf, valueT val, int n)
{
  if (target_big_endian)
    number_to_chars_bigendian (buf, val, n);
  else
    number_to_chars_littleendian (buf, val, n);
}

/* Machine-dependent operand parsing not currently needed.  */

void
md_operand (expressionS *op ATTRIBUTE_UNUSED)
{
}

/* PC-relative operands are relative to the start of the fetch
   packet.  */

long
tic6x_pcrel_from_section (fixS *fixp, segT sec)
{
  if (fixp->fx_addsy != NULL
      && (!S_IS_DEFINED (fixp->fx_addsy)
	  || S_GET_SEGMENT (fixp->fx_addsy) != sec))
    return 0;
  return (fixp->fx_where + fixp->fx_frag->fr_address) & ~(long) 0x1f;
}

/* Round up a section size to the appropriate boundary.  */

valueT
md_section_align (segT segment ATTRIBUTE_UNUSED,
		  valueT size)
{
  /* Round up section sizes to ensure that text sections consist of
     whole fetch packets.  */
  int align = bfd_get_section_alignment (stdoutput, segment);
  return ((size + (1 << align) - 1) & ((valueT) -1 << align));
}

/* No special undefined symbol handling needed for now.  */

symbolS *
md_undefined_symbol (char *name ATTRIBUTE_UNUSED)
{
  return NULL;
}

/* Translate internal representation of relocation info to BFD target
   format.  */

arelent *
tc_gen_reloc (asection *section ATTRIBUTE_UNUSED, fixS *fixp)
{
  arelent *reloc;
  bfd_reloc_code_real_type r_type;

  reloc = xmalloc (sizeof (arelent));
  reloc->sym_ptr_ptr = xmalloc (sizeof (asymbol *));
  *reloc->sym_ptr_ptr = symbol_get_bfdsym (fixp->fx_addsy);
  reloc->address = fixp->fx_frag->fr_address + fixp->fx_where;
  reloc->addend = (tic6x_generate_rela ? fixp->fx_offset : 0);
  r_type = fixp->fx_r_type;
  reloc->howto = bfd_reloc_type_lookup (stdoutput, r_type);

  if (reloc->howto == NULL)
    {
      as_bad_where (fixp->fx_file, fixp->fx_line,
		    _("Cannot represent relocation type %s"),
		    bfd_get_reloc_code_name (r_type));
      return NULL;
    }

  /* Correct for adjustments bfd_install_relocation will make.  */
  if (reloc->howto->pcrel_offset && reloc->howto->partial_inplace)
    reloc->addend += reloc->address;

  return reloc;
}
