/* tc-z80.c -- Assemble code for the Zilog Z80, Z180, EZ80 and ASCII R800
   Copyright (C) 2005-2020 Free Software Foundation, Inc.
   Contributed by Arnold Metselaar <arnold_m@operamail.com>

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
#include "safe-ctype.h"
#include "subsegs.h"
#include "elf/z80.h"
#include "dwarf2dbg.h"

/* Exported constants.  */
const char comment_chars[] = ";\0";
const char line_comment_chars[] = "#;\0";
const char line_separator_chars[] = "\0";
const char EXP_CHARS[] = "eE\0";
const char FLT_CHARS[] = "RrDdFfSsHh\0";

/* For machine specific options.  */
const char * md_shortopts = ""; /* None yet.  */

enum options
{
  OPTION_MACH_Z80 = OPTION_MD_BASE,
  OPTION_MACH_R800,
  OPTION_MACH_Z180,
  OPTION_MACH_EZ80_Z80,
  OPTION_MACH_EZ80_ADL,
  OPTION_MACH_GBZ80,
  OPTION_MACH_INST,
  OPTION_MACH_NO_INST,
  OPTION_MACH_IUD,
  OPTION_MACH_WUD,
  OPTION_MACH_FUD,
  OPTION_MACH_IUP,
  OPTION_MACH_WUP,
  OPTION_MACH_FUP,
  OPTION_FP_SINGLE_FORMAT,
  OPTION_FP_DOUBLE_FORMAT,
  OPTION_COMPAT_LL_PREFIX,
  OPTION_COMPAT_COLONLESS,
  OPTION_COMPAT_SDCC
};

#define INS_Z80      (1 << 0)
#define INS_R800     (1 << 1)
#define INS_GBZ80    (1 << 2)
#define INS_Z180     (1 << 3)
#define INS_EZ80     (1 << 4)
#define INS_MARCH_MASK 0xffff

#define INS_IDX_HALF (1 << 16)
#define INS_IN_F_C   (1 << 17)
#define INS_OUT_C_0  (1 << 18)
#define INS_SLI      (1 << 19)
#define INS_ROT_II_LD (1 << 20)  /* instructions like SLA (ii+d),r; which is: LD r,(ii+d); SLA r; LD (ii+d),r */
#define INS_TUNE_MASK 0xffff0000

#define INS_NOT_GBZ80 (INS_Z80 | INS_Z180 | INS_R800 | INS_EZ80)

#define INS_ALL 0
#define INS_UNDOC (INS_IDX_HALF | INS_IN_F_C)
#define INS_UNPORT (INS_OUT_C_0 | INS_SLI | INS_ROT_II_LD)

struct option md_longopts[] =
{
  { "z80",       no_argument, NULL, OPTION_MACH_Z80},
  { "r800",      no_argument, NULL, OPTION_MACH_R800},
  { "z180",      no_argument, NULL, OPTION_MACH_Z180},
  { "ez80",      no_argument, NULL, OPTION_MACH_EZ80_Z80},
  { "ez80-adl",  no_argument, NULL, OPTION_MACH_EZ80_ADL},
  { "fp-s",      required_argument, NULL, OPTION_FP_SINGLE_FORMAT},
  { "fp-d",      required_argument, NULL, OPTION_FP_DOUBLE_FORMAT},
  { "strict",    no_argument, NULL, OPTION_MACH_FUD},
  { "full",      no_argument, NULL, OPTION_MACH_IUP},
  { "with-inst", required_argument, NULL, OPTION_MACH_INST},
  { "Wnins",     required_argument, NULL, OPTION_MACH_INST},
  { "without-inst", required_argument, NULL, OPTION_MACH_NO_INST},
  { "local-prefix", required_argument, NULL, OPTION_COMPAT_LL_PREFIX},
  { "colonless", no_argument, NULL, OPTION_COMPAT_COLONLESS},
  { "sdcc",      no_argument, NULL, OPTION_COMPAT_SDCC},
  { "Fins",      required_argument, NULL, OPTION_MACH_NO_INST},
  { "ignore-undocumented-instructions", no_argument, NULL, OPTION_MACH_IUD },
  { "Wnud",  no_argument, NULL, OPTION_MACH_IUD },
  { "warn-undocumented-instructions",  no_argument, NULL, OPTION_MACH_WUD },
  { "Wud",  no_argument, NULL, OPTION_MACH_WUD },
  { "forbid-undocumented-instructions", no_argument, NULL, OPTION_MACH_FUD },
  { "Fud",  no_argument, NULL, OPTION_MACH_FUD },
  { "ignore-unportable-instructions", no_argument, NULL, OPTION_MACH_IUP },
  { "Wnup",  no_argument, NULL, OPTION_MACH_IUP },
  { "warn-unportable-instructions",  no_argument, NULL, OPTION_MACH_WUP },
  { "Wup",  no_argument, NULL, OPTION_MACH_WUP },
  { "forbid-unportable-instructions", no_argument, NULL, OPTION_MACH_FUP },
  { "Fup",  no_argument, NULL, OPTION_MACH_FUP },

  { NULL, no_argument, NULL, 0 }
} ;

size_t md_longopts_size = sizeof (md_longopts);

extern int coff_flags;
/* Instruction classes that silently assembled.  */
static int ins_ok = INS_Z80 | INS_UNDOC;
/* Instruction classes that generate errors.  */
static int ins_err = ~(INS_Z80 | INS_UNDOC);
/* eZ80 CPU mode (ADL or Z80) */
static int cpu_mode = 0; /* 0 - Z80, 1 - ADL */
/* accept SDCC specific instruction encoding */
static int sdcc_compat = 0;
/* accept colonless labels */
static int colonless_labels = 0;
/* local label prefix (NULL - default) */
static const char *local_label_prefix = NULL;
/* floating point support */
typedef const char *(*str_to_float_t)(char *litP, int *sizeP);
static str_to_float_t str_to_float;
static str_to_float_t str_to_double;

/* mode of current instruction */
#define INST_MODE_S 0      /* short data mode */
#define INST_MODE_IS 0     /* short instruction mode */
#define INST_MODE_L 2      /* long data mode */
#define INST_MODE_IL 1     /* long instruction mode */
#define INST_MODE_FORCED 4 /* CPU mode changed by instruction suffix*/
static char inst_mode;

static int
setup_instruction (const char *inst, int *add, int *sub)
{
  int n;
  if (!strcmp (inst, "idx-reg-halves"))
    n = INS_IDX_HALF;
  else if (!strcmp (inst, "sli"))
    n = INS_SLI;
  else if (!strcmp (inst, "op-ii-ld"))
    n = INS_ROT_II_LD;
  else if (!strcmp (inst, "in-f-c"))
    n = INS_IN_F_C;
  else if (!strcmp (inst, "out-c-0"))
    n = INS_OUT_C_0;
  else
    return 0;
  *add |= n;
  *sub &= ~n;
  return 1;
}

static const char *
str_to_zeda32 (char *litP, int *sizeP);
static const char *
str_to_float48 (char *litP, int *sizeP);
static const char *
str_to_ieee754_h (char *litP, int *sizeP);
static const char *
str_to_ieee754_s (char *litP, int *sizeP);
static const char *
str_to_ieee754_d (char *litP, int *sizeP);

static str_to_float_t
get_str_to_float (const char *arg)
{
  if (strcasecmp (arg, "zeda32") == 0)
    return str_to_zeda32;

  if (strcasecmp (arg, "math48") == 0)
    return str_to_float48;

  if (strcasecmp (arg, "half") != 0)
    return str_to_ieee754_h;

  if (strcasecmp (arg, "single") != 0)
    return str_to_ieee754_s;

  if (strcasecmp (arg, "double") != 0)
    return str_to_ieee754_d;

  if (strcasecmp (arg, "ieee754") == 0)
    as_fatal (_("invalid floating point numbers type `%s'"), arg);
  return NULL;
}

static int
setup_instruction_list (const char *list, int *add, int *sub)
{
  char buf[16];
  const char *b;
  const char *e;
  int sz;
  int res = 0;
  for (b = list; *b != '\0';)
    {
      e = strchr (b, ',');
      if (e == NULL)
        sz = strlen (b);
      else
        sz = e - b;
      if (sz == 0 || sz >= (int)sizeof (buf))
        {
          as_bad (_("invalid INST in command line: %s"), b);
          return 0;
        }
      memcpy (buf, b, sz);
      buf[sz] = '\0';
      if (setup_instruction (buf, add, sub))
        res++;
      else
        {
          as_bad (_("invalid INST in command line: %s"), buf);
          return 0;
        }
      b = &b[sz];
      if (*b == ',')
        ++b;
    }
  return res;
}

int
md_parse_option (int c, const char* arg)
{
  switch (c)
    {
    default:
      return 0;
    case OPTION_MACH_Z80:
      ins_ok = (ins_ok & INS_TUNE_MASK) | INS_Z80;
      ins_err = (ins_err & INS_MARCH_MASK) | (~INS_Z80 & INS_MARCH_MASK);
      break;
    case OPTION_MACH_R800:
      ins_ok = INS_R800 | INS_IDX_HALF;
      ins_err = INS_UNPORT;
      break;
    case OPTION_MACH_Z180:
      ins_ok = INS_Z180;
      ins_err = INS_UNDOC | INS_UNPORT;
      break;
    case OPTION_MACH_EZ80_Z80:
      ins_ok = INS_EZ80;
      ins_err = (INS_UNDOC | INS_UNPORT) & ~INS_IDX_HALF;
      cpu_mode = 0;
      break;
    case OPTION_MACH_EZ80_ADL:
      ins_ok = INS_EZ80;
      ins_err = (INS_UNDOC | INS_UNPORT) & ~INS_IDX_HALF;
      cpu_mode = 1;
      break;
    case OPTION_MACH_GBZ80:
      ins_ok = INS_GBZ80;
      ins_err = INS_UNDOC | INS_UNPORT;
      break;
    case OPTION_FP_SINGLE_FORMAT:
      str_to_float = get_str_to_float (arg);
      break;
    case OPTION_FP_DOUBLE_FORMAT:
      str_to_double = get_str_to_float (arg);
      break;
    case OPTION_MACH_INST:
      if ((ins_ok & INS_GBZ80) == 0)
        return setup_instruction_list (arg, & ins_ok, & ins_err);
      break;
    case OPTION_MACH_NO_INST:
      if ((ins_ok & INS_GBZ80) == 0)
        return setup_instruction_list (arg, & ins_err, & ins_ok);
      break;
    case OPTION_MACH_WUD:
    case OPTION_MACH_IUD:
      if ((ins_ok & INS_GBZ80) == 0)
        {
          ins_ok |= INS_UNDOC;
          ins_err &= ~INS_UNDOC;
        }
      break;
    case OPTION_MACH_WUP:
    case OPTION_MACH_IUP:
      if ((ins_ok & INS_GBZ80) == 0)
        {
          ins_ok |= INS_UNDOC | INS_UNPORT;
          ins_err &= ~(INS_UNDOC | INS_UNPORT);
        }
      break;
    case OPTION_MACH_FUD:
      if ((ins_ok & (INS_R800 | INS_GBZ80)) == 0)
	{
	  ins_ok &= (INS_UNDOC | INS_UNPORT);
	  ins_err |= INS_UNDOC | INS_UNPORT;
	}
      break;
    case OPTION_MACH_FUP:
      ins_ok &= ~INS_UNPORT;
      ins_err |= INS_UNPORT;
      break;
    case OPTION_COMPAT_LL_PREFIX:
      local_label_prefix = (arg && *arg) ? arg : NULL;
      break;
    case OPTION_COMPAT_SDCC:
      sdcc_compat = 1;
      local_label_prefix = "_";
      break;
    case OPTION_COMPAT_COLONLESS:
      colonless_labels = 1;
      break;
    }

  return 1;
}

void
md_show_usage (FILE * f)
{
  fprintf (f, "\n\
CPU model options:\n\
  -z80\t\t\t  assemble for Z80\n\
  -r800\t\t\t  assemble for R800\n\
  -z180\t\t\t  assemble for Z180\n\
  -ez80\t\t\t  assemble for eZ80 in Z80 mode by default\n\
  -ez80-adl\t\t  assemble for eZ80 in ADL mode by default\n\
\n\
Compatibility options:\n\
  -local-prefix=TEXT\t  treat labels prefixed by TEXT as local\n\
  -colonless\t\t  permit colonless labels\n\
  -sdcc\t\t\t  accept SDCC specific instruction syntax\n\
  -fp-s=FORMAT\t\t  set single precission FP numbers format\n\
  -fp-d=FORMAT\t\t  set double precission FP numbers format\n\
Where FORMAT one of:\n\
  ieee754\t\t  IEEE754 compatible\n\
  half\t\t\t  IEEE754 half precision (16 bit)\n\
  single\t\t  IEEE754 single precision (32 bit)\n\
  double\t\t  IEEE754 double precision (64 bit)\n\
  zeda32\t\t  Zeda z80float library 32 bit format\n\
  math48\t\t  48 bit format from Math48 library\n\
\n\
Support for known undocumented instructions:\n\
  -strict\t\t  assemble only documented instructions\n\
  -full\t\t\t  assemble all undocumented instructions\n\
  -with-inst=INST[,...]\n\
  -Wnins INST[,...]\t  assemble specified instruction(s)\n\
  -without-inst=INST[,...]\n\
  -Fins INST[,...]\t  do not assemble specified instruction(s)\n\
Where INST is one of:\n\
  idx-reg-halves\t  instructions with halves of index registers\n\
  sli\t\t\t  instruction SLI/SLL\n\
  op-ii-ld\t\t  instructions like SLA (II+dd),R (opcodes DD/FD CB dd xx)\n\
  in-f-c\t\t  instruction IN F,(C)\n\
  out-c-0\t\t  instruction OUT (C),0\n\
\n\
Obsolete options:\n\
  -ignore-undocumented-instructions\n\
  -Wnud\t\t\t  silently assemble undocumented Z80-instructions that work on R800\n\
  -ignore-unportable-instructions\n\
  -Wnup\t\t\t  silently assemble all undocumented Z80-instructions\n\
  -warn-undocumented-instructions\n\
  -Wud\t\t\t  issue warnings for undocumented Z80-instructions that work on R800\n\
  -warn-unportable-instructions\n\
  -Wup\t\t\t  issue warnings for other undocumented Z80-instructions\n\
  -forbid-undocumented-instructions\n\
  -Fud\t\t\t  treat all undocumented Z80-instructions as errors\n\
  -forbid-unportable-instructions\n\
  -Fup\t\t\t  treat undocumented Z80-instructions that do not work on R800 as errors\n\
\n\
Default: -z80 -ignore-undocumented-instructions -warn-unportable-instructions.\n");
}

static symbolS * zero;

struct reg_entry
{
  const char* name;
  int number;
};
#define R_STACKABLE (0x80)
#define R_ARITH     (0x40)
#define R_IX        (0x20)
#define R_IY        (0x10)
#define R_INDEX     (R_IX | R_IY)

#define REG_A (7)
#define REG_B (0)
#define REG_C (1)
#define REG_D (2)
#define REG_E (3)
#define REG_H (4)
#define REG_L (5)
#define REG_F (6 | 8)
#define REG_I (9)
#define REG_R (10)
#define REG_MB (11)

#define REG_AF (3 | R_STACKABLE)
#define REG_BC (0 | R_STACKABLE | R_ARITH)
#define REG_DE (1 | R_STACKABLE | R_ARITH)
#define REG_HL (2 | R_STACKABLE | R_ARITH)
#define REG_IX (REG_HL | R_IX)
#define REG_IY (REG_HL | R_IY)
#define REG_SP (3 | R_ARITH)

static const struct reg_entry regtable[] =
{
  {"a",  REG_A },
  {"af", REG_AF },
  {"b",  REG_B },
  {"bc", REG_BC },
  {"c",  REG_C },
  {"d",  REG_D },
  {"de", REG_DE },
  {"e",  REG_E },
  {"f",  REG_F },
  {"h",  REG_H },
  {"hl", REG_HL },
  {"i",  REG_I },
  {"ix", REG_IX },
  {"ixh",REG_H | R_IX },
  {"ixl",REG_L | R_IX },
  {"iy", REG_IY },
  {"iyh",REG_H | R_IY },
  {"iyl",REG_L | R_IY },
  {"l",  REG_L },
  {"mb", REG_MB },
  {"r",  REG_R },
  {"sp", REG_SP },
} ;

#define BUFLEN 8 /* Large enough for any keyword.  */

void
md_begin (void)
{
  expressionS nul, reg;
  char * p;
  unsigned int i, j, k;
  char buf[BUFLEN];

  if (ins_ok & INS_EZ80)   /* if select EZ80 cpu then */
    listing_lhs_width = 6; /* use 6 bytes per line in the listing */

  reg.X_op = O_register;
  reg.X_md = 0;
  reg.X_add_symbol = reg.X_op_symbol = 0;
  for ( i = 0 ; i < ARRAY_SIZE ( regtable ) ; ++i )
    {
      reg.X_add_number = regtable[i].number;
      k = strlen ( regtable[i].name );
      buf[k] = 0;
      if ( k+1 < BUFLEN )
        {
          for ( j = ( 1<<k ) ; j ; --j )
            {
              for ( k = 0 ; regtable[i].name[k] ; ++k )
                {
                  buf[k] = ( j & ( 1<<k ) ) ? TOUPPER (regtable[i].name[k]) : regtable[i].name[k];
                }
              symbolS * psym = symbol_find_or_make (buf);
	      S_SET_SEGMENT (psym, reg_section);
	      symbol_set_value_expression (psym, &reg);
            }
        }
    }
  p = input_line_pointer;
  input_line_pointer = (char *) "0";
  nul.X_md=0;
  expression (& nul);
  input_line_pointer = p;
  zero = make_expr_symbol (& nul);
  /* We do not use relaxation (yet).  */
  linkrelax = 0;
}

void
z80_md_end (void)
{
  int mach_type;

  switch (ins_ok & INS_MARCH_MASK)
    {
    case INS_Z80:
      if (ins_ok & INS_UNPORT)
        mach_type = bfd_mach_z80full;
      else if (ins_ok & INS_UNDOC)
        mach_type = bfd_mach_z80;
      else
        mach_type = bfd_mach_z80strict;
      break;
    case INS_R800:
      mach_type = bfd_mach_r800;
      break;
    case INS_Z180:
      mach_type = bfd_mach_z180;
      break;
    case INS_GBZ80:
      mach_type = bfd_mach_gbz80;
      break;
    case INS_EZ80:
      mach_type = cpu_mode ? bfd_mach_ez80_adl : bfd_mach_ez80_z80;
      break;
    default:
      mach_type = 0;
    }

  bfd_set_arch_mach (stdoutput, TARGET_ARCH, mach_type);
}

#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
void
z80_elf_final_processing (void)
{
  unsigned elf_flags;
  switch (ins_ok & INS_MARCH_MASK)
    {
    case INS_Z80:
      elf_flags = EF_Z80_MACH_Z80;
      break;
    case INS_R800:
      elf_flags = EF_Z80_MACH_R800;
      break;
    case INS_Z180:
      elf_flags = EF_Z80_MACH_Z180;
      break;
    case INS_GBZ80:
      elf_flags = EF_Z80_MACH_GBZ80;
      break;
    case INS_EZ80:
      elf_flags = cpu_mode ? EF_Z80_MACH_EZ80_ADL : EF_Z80_MACH_EZ80_Z80;
      break;
    default:
      elf_flags = 0;
    }

  elf_elfheader (stdoutput)->e_flags = elf_flags;
}
#endif

static const char *
skip_space (const char *s)
{
  while (*s == ' ' || *s == '\t')
    ++s;
  return s;
}

/* A non-zero return-value causes a continue in the
   function read_a_source_file () in ../read.c.  */
int
z80_start_line_hook (void)
{
  char *p, quote;
  char buf[4];

  /* Convert one character constants.  */
  for (p = input_line_pointer; *p && *p != '\n'; ++p)
    {
      switch (*p)
	{
	case '\'':
	  if (p[1] != 0 && p[1] != '\'' && p[2] == '\'')
	    {
	      snprintf (buf, 4, "%3d", (unsigned char)p[1]);
	      *p++ = buf[0];
	      *p++ = buf[1];
	      *p++ = buf[2];
	      break;
	    }
	  /* Fall through.  */
	case '"':
	  for (quote = *p++; quote != *p && '\n' != *p; ++p)
	    /* No escapes.  */ ;
	  if (quote != *p)
	    {
	      as_bad (_("-- unterminated string"));
	      ignore_rest_of_line ();
	      return 1;
	    }
	  break;
	case '#':
	  if (sdcc_compat)
	    *p = (*skip_space (p + 1) == '(') ? '+' : ' ';
	  break;
	}
    }
  /* Check for <label>[:] [.](EQU|DEFL) <value>.  */
  if (is_name_beginner (*input_line_pointer))
    {
      char *name;
      char c, *rest, *line_start;
      int len;

      line_start = input_line_pointer;
      if (ignore_input ())
	return 0;

      c = get_symbol_name (&name);
      rest = input_line_pointer + 1;

      if (ISSPACE (c) && colonless_labels)
        {
          if (c == '\n')
            {
              bump_line_counters ();
              LISTING_NEWLINE ();
            }
          c = ':';
        }
      if (c == ':' && sdcc_compat && rest[-2] != '$')
        dollar_label_clear ();
      if (*rest == ':')
        {
          /* remove second colon if SDCC compatibility enabled */
          if (sdcc_compat)
            *rest = ' ';
          ++rest;
        }
      rest = (char*)skip_space (rest);
      if (*rest == '.')
	++rest;
      if (strncasecmp (rest, "EQU", 3) == 0)
	len = 3;
      else if (strncasecmp (rest, "DEFL", 4) == 0)
	len = 4;
      else
	len = 0;
      if (len && (!ISALPHA (rest[len])))
	{
	  /* Handle assignment here.  */
	  if (line_start[-1] == '\n')
	    {
	      bump_line_counters ();
	      LISTING_NEWLINE ();
	    }
	  input_line_pointer = rest + len - 1;
	  /* Allow redefining with "DEFL" (len == 4), but not with "EQU".  */
	  equals (name, len == 4);
	  return 1;
	}
      else
	{
	  /* Restore line and pointer.  */
	  (void) restore_line_pointer (c);
	  input_line_pointer = line_start;
	}
    }
  return 0;
}

symbolS *
md_undefined_symbol (char *name ATTRIBUTE_UNUSED)
{
  return NULL;
}

const char *
md_atof (int type, char *litP, int *sizeP)
{
  switch (type)
    {
    case 'f':
    case 'F':
    case 's':
    case 'S':
      if (str_to_float)
	return str_to_float (litP, sizeP);
      break;
    case 'd':
    case 'D':
    case 'r':
    case 'R':
      if (str_to_double)
	return str_to_double (litP, sizeP);
      break;
    }
  return ieee_md_atof (type, litP, sizeP, FALSE);
}

valueT
md_section_align (segT seg ATTRIBUTE_UNUSED, valueT size)
{
  return size;
}

long
md_pcrel_from (fixS * fixp)
{
  return fixp->fx_where + fixp->fx_frag->fr_address;
}

typedef const char * (asfunc)(char, char, const char*);

typedef struct _table_t
{
  const char* name;
  unsigned char prefix;
  unsigned char opcode;
  asfunc * fp;
  unsigned inss; /*0 - all CPU types or list of supported INS_* */
} table_t;

/* Compares the key for structs that start with a char * to the key.  */
static int
key_cmp (const void * a, const void * b)
{
  const char *str_a, *str_b;

  str_a = *((const char**)a);
  str_b = *((const char**)b);
  return strcmp (str_a, str_b);
}

char buf[BUFLEN];
const char *key = buf;

/* Prevent an error on a line from also generating
   a "junk at end of line" error message.  */
static char err_flag;

static void
error (const char * message)
{
  if (err_flag)
    return;

  as_bad ("%s", message);
  err_flag = 1;
}

static void
ill_op (void)
{
  error (_("illegal operand"));
}

static void
wrong_mach (int ins_type)
{
  if (ins_type & ins_err)
    ill_op ();
  else
    as_warn (_("undocumented instruction"));
}

static void
check_mach (int ins_type)
{
  if ((ins_type & ins_ok) == 0)
    wrong_mach (ins_type);
}

/* Check whether an expression is indirect.  */
static int
is_indir (const char *s)
{
  char quote;
  const char *p;
  int indir, depth;

  /* Indirection is indicated with parentheses.  */
  indir = (*s == '(');

  for (p = s, depth = 0; *p && *p != ','; ++p)
    {
      switch (*p)
	{
	case '"':
	case '\'':
	  for (quote = *p++; quote != *p && *p != '\n'; ++p)
	    if (*p == '\\' && p[1])
	      ++p;
	  break;
	case '(':
	  ++ depth;
	  break;
	case ')':
	  -- depth;
	  if (depth == 0)
	    {
	      p = skip_space (p + 1);
	      if (*p && *p != ',')
		indir = 0;
	      --p;
	    }
	  if (depth < 0)
	    error (_("mismatched parentheses"));
	  break;
	}
    }

  if (depth != 0)
    error (_("mismatched parentheses"));

  return indir;
}

/* Check whether a symbol involves a register.  */
static int
contains_register (symbolS *sym)
{
  if (sym)
    {
      expressionS * ex = symbol_get_value_expression(sym);

      return (O_register == ex->X_op)
	|| (ex->X_add_symbol && contains_register(ex->X_add_symbol))
	|| (ex->X_op_symbol && contains_register(ex->X_op_symbol));
    }

  return 0;
}

/* Parse general expression, not looking for indexed addressing.  */
static const char *
parse_exp_not_indexed (const char *s, expressionS *op)
{
  const char *p;
  int indir;
  int make_shift = -1;

  p = skip_space (s);
  if (sdcc_compat && (*p == '<' || *p == '>'))
    {
      switch (*p)
	{
	case '<': /* LSB request */
	  make_shift = 0;
	  break;
	case '>': /* MSB request */
	  make_shift = cpu_mode ? 16 : 8;
	  break;
	}
      s = ++p;
      p = skip_space (p);
    }

  op->X_md = indir = is_indir (p);
  input_line_pointer = (char*) s ;
  expression (op);
  switch (op->X_op)
    {
    case O_absent:
      error (_("missing operand"));
      break;
    case O_illegal:
      error (_("bad expression syntax"));
      break;
    default:
      break;
    }

  if (make_shift >= 0)
    {
      /* replace [op] by [op >> shift] */
      expressionS data;
      op->X_add_symbol = make_expr_symbol (op);
      op->X_add_number = 0;
      op->X_op = O_right_shift;
      memset (&data, 0, sizeof (data));
      data.X_op = O_constant;
      data.X_add_number = make_shift;
      op->X_op_symbol = make_expr_symbol (&data);
    }
  return input_line_pointer;
}

static int
unify_indexed (expressionS *op)
{
  if (O_register != symbol_get_value_expression (op->X_add_symbol)->X_op)
    return 0;

  int rnum = symbol_get_value_expression (op->X_add_symbol)->X_add_number;
  if ( ((REG_IX != rnum) && (REG_IY != rnum)) || contains_register (op->X_op_symbol))
    {
      ill_op ();
      return 0;
    }

  /* Convert subtraction to addition of negative value.  */
  if (O_subtract == op->X_op)
    {
      expressionS minus;
      minus.X_op = O_uminus;
      minus.X_add_number = 0;
      minus.X_add_symbol = op->X_op_symbol;
      minus.X_op_symbol = 0;
      op->X_op_symbol = make_expr_symbol (&minus);
      op->X_op = O_add;
    }

  /* Clear X_add_number of the expression.  */
  if (op->X_add_number != 0)
    {
      expressionS add;
      memset (&add, 0, sizeof (add));
      add.X_op = O_symbol;
      add.X_add_number = op->X_add_number;
      add.X_add_symbol = op->X_op_symbol;
      add.X_op_symbol = 0;
      op->X_add_symbol = make_expr_symbol (&add);
    }
  else
    op->X_add_symbol = op->X_op_symbol;

  op->X_add_number = rnum;
  op->X_op_symbol = 0;
  return 1;
}

/* Parse expression, change operator to O_md1 for indexed addressing.  */
static const char *
parse_exp (const char *s, expressionS *op)
{
  const char* res = parse_exp_not_indexed (s, op);
  switch (op->X_op)
    {
    case O_add:
    case O_subtract:
      if (unify_indexed (op) && op->X_md)
        op->X_op = O_md1;
      break;
    case O_register:
      if (op->X_md && ((REG_IX == op->X_add_number) || (REG_IY == op->X_add_number)))
        {
	  op->X_add_symbol = zero;
	  op->X_op = O_md1;
	}
	break;
    case O_constant:
      /* parse SDCC syntax where index register offset placed before parentheses */
      if (sdcc_compat && is_indir (res))
        {
          expressionS off;
          off = *op;
          res = parse_exp (res, op);
          if (op->X_op != O_md1 || op->X_add_symbol != zero)
            ill_op ();
          else
              op->X_add_symbol = make_expr_symbol (&off);
        }
      break;
    default:
      break;
    }
  return res;
}

/* Condition codes, including some synonyms provided by HiTech zas.  */
static const struct reg_entry cc_tab[] =
{
  { "age", 6 << 3 },
  { "alt", 7 << 3 },
  { "c",   3 << 3 },
  { "di",  4 << 3 },
  { "ei",  5 << 3 },
  { "lge", 2 << 3 },
  { "llt", 3 << 3 },
  { "m",   7 << 3 },
  { "nc",  2 << 3 },
  { "nz",  0 << 3 },
  { "p",   6 << 3 },
  { "pe",  5 << 3 },
  { "po",  4 << 3 },
  { "z",   1 << 3 },
} ;

/* Parse condition code.  */
static const char *
parse_cc (const char *s, char * op)
{
  const char *p;
  int i;
  struct reg_entry * cc_p;

  for (i = 0; i < BUFLEN; ++i)
    {
      if (!ISALPHA (s[i])) /* Condition codes consist of letters only.  */
	break;
      buf[i] = TOLOWER (s[i]);
    }

  if ((i < BUFLEN)
      && ((s[i] == 0) || (s[i] == ',')))
    {
      buf[i] = 0;
      cc_p = bsearch (&key, cc_tab, ARRAY_SIZE (cc_tab),
		      sizeof (cc_tab[0]), key_cmp);
    }
  else
    cc_p = NULL;

  if (cc_p)
    {
      *op = cc_p->number;
      p = s + i;
    }
  else
    p = NULL;

  return p;
}

static const char *
emit_insn (char prefix, char opcode, const char * args)
{
  char *p;

  if (prefix)
    {
      p = frag_more (2);
      *p++ = prefix;
    }
  else
    p = frag_more (1);
  *p = opcode;
  return args;
}

void z80_cons_fix_new (fragS *frag_p, int offset, int nbytes, expressionS *exp)
{
  bfd_reloc_code_real_type r[4] =
    {
      BFD_RELOC_8,
      BFD_RELOC_16,
      BFD_RELOC_24,
      BFD_RELOC_32
    };

  if (nbytes < 1 || nbytes > 4)
    {
      as_bad (_("unsupported BFD relocation size %u"), nbytes);
    }
  else
    {
      fix_new_exp (frag_p, offset, nbytes, exp, 0, r[nbytes-1]);
    }
}

static void
emit_data_val (expressionS * val, int size)
{
  char *p;
  bfd_reloc_code_real_type r_type;

  p = frag_more (size);
  if (val->X_op == O_constant)
    {
      int i;
      for (i = 0; i < size; ++i)
	p[i] = (char)(val->X_add_number >> (i*8));
      return;
    }

  switch (size)
    {
    case 1: r_type = BFD_RELOC_8; break;
    case 2: r_type = BFD_RELOC_16; break;
    case 3: r_type = BFD_RELOC_24; break;
    case 4: r_type = BFD_RELOC_32; break;
    case 8: r_type = BFD_RELOC_64; break;
    default:
      as_fatal (_("invalid data size %d"), size);
    }

  if (   (val->X_op == O_register)
      || (val->X_op == O_md1)
      || contains_register (val->X_add_symbol)
      || contains_register (val->X_op_symbol))
    ill_op ();

  if (size <= 2 && val->X_op_symbol)
    {
      bfd_boolean simplify = TRUE;
      int shift = symbol_get_value_expression (val->X_op_symbol)->X_add_number;
      if (val->X_op == O_bit_and && shift == (1 << (size*8))-1)
	shift = 0;
      else if (val->X_op != O_right_shift)
	shift = -1;

      if (size == 1)
	{
	  switch (shift)
	    {
	    case 0: r_type = BFD_RELOC_Z80_BYTE0; break;
	    case 8: r_type = BFD_RELOC_Z80_BYTE1; break;
	    case 16: r_type = BFD_RELOC_Z80_BYTE2; break;
	    case 24: r_type = BFD_RELOC_Z80_BYTE3; break;
	    default: simplify = FALSE;
	    }
	}
      else /* if (size == 2) */
	{
	  switch (shift)
	    {
	    case 0: r_type = BFD_RELOC_Z80_WORD0; break;
	    case 16: r_type = BFD_RELOC_Z80_WORD1; break;
	    default: simplify = FALSE;
	    }
	}

      if (simplify)
	{
	  val->X_op = O_symbol;
	  val->X_op_symbol = NULL;
	  val->X_add_number = 0;
	}
    }

  fix_new_exp (frag_now, p - frag_now->fr_literal, size, val, FALSE, r_type);
}

static void
emit_byte (expressionS * val, bfd_reloc_code_real_type r_type)
{
  char *p;
  int lo, hi;

  if (r_type == BFD_RELOC_8)
    {
      emit_data_val (val, 1);
      return;
    }
  p = frag_more (1);
  *p = val->X_add_number;
  if ( contains_register (val->X_add_symbol) || contains_register (val->X_op_symbol) )
    {
      ill_op ();
    }
  else if ((r_type == BFD_RELOC_8_PCREL) && (val->X_op == O_constant))
    {
      as_bad (_("cannot make a relative jump to an absolute location"));
    }
  else if (val->X_op == O_constant)
    {
      lo = -128;
      hi = (BFD_RELOC_8 == r_type) ? 255 : 127;

      if ((val->X_add_number < lo) || (val->X_add_number > hi))
	{
	  if (r_type == BFD_RELOC_Z80_DISP8)
	    as_bad (_("offset too large"));
	  else
	    as_warn (_("overflow"));
	}
    }
  else
    {
      /* For symbols only, constants are stored at begin of function */
      fix_new_exp (frag_now, p - frag_now->fr_literal, 1, val,
		   (r_type == BFD_RELOC_8_PCREL) ? TRUE : FALSE, r_type);
    }
}

static void
emit_word (expressionS * val)
{
  emit_data_val (val, (inst_mode & INST_MODE_IL) ? 3 : 2);
}

static void
emit_mx (char prefix, char opcode, int shift, expressionS * arg)
     /* The operand m may be r, (hl), (ix+d), (iy+d),
	if 0 == prefix m may also be ixl, ixh, iyl, iyh.  */
{
  char *q;
  int rnum;

  rnum = arg->X_add_number;
  switch (arg->X_op)
    {
    case O_register:
      if (arg->X_md)
	{
	  if (rnum != REG_HL)
	    {
	      ill_op ();
	      break;
	    }
	  else
	    rnum = 6;
	}
      else
	{
	  if ((prefix == 0) && (rnum & R_INDEX))
	    {
	      prefix = (rnum & R_IX) ? 0xDD : 0xFD;
              if (!(ins_ok & INS_EZ80))
                check_mach (INS_IDX_HALF);
	      rnum &= ~R_INDEX;
	    }
	  if (rnum > 7)
	    {
	      ill_op ();
	      break;
	    }
	}
      q = frag_more (prefix ? 2 : 1);
      if (prefix)
	* q ++ = prefix;
      * q ++ = opcode + (rnum << shift);
      break;
    case O_md1:
      if (ins_ok & INS_GBZ80)
        {
          ill_op ();
          break;
        }
      q = frag_more (2);
      *q++ = (rnum & R_IX) ? 0xDD : 0xFD;
      *q = (prefix) ? prefix : (opcode + (6 << shift));
      {
	expressionS offset = *arg;
	offset.X_op = O_symbol;
	offset.X_add_number = 0;
	emit_byte (&offset, BFD_RELOC_Z80_DISP8);
      }
      if (prefix)
	{
	  q = frag_more (1);
	  *q = opcode+(6<<shift);
	}
      break;
    default:
      abort ();
    }
}

/* The operand m may be r, (hl), (ix+d), (iy+d),
   if 0 = prefix m may also be ixl, ixh, iyl, iyh.  */
static const char *
emit_m (char prefix, char opcode, const char *args)
{
  expressionS arg_m;
  const char *p;

  p = parse_exp (args, &arg_m);
  switch (arg_m.X_op)
    {
    case O_md1:
    case O_register:
      emit_mx (prefix, opcode, 0, &arg_m);
      break;
    default:
      ill_op ();
    }
  return p;
}

/* The operand m may be as above or one of the undocumented
   combinations (ix+d),r and (iy+d),r (if unportable instructions
   are allowed).  */

static const char *
emit_mr (char prefix, char opcode, const char *args)
{
  expressionS arg_m, arg_r;
  const char *p;

  p = parse_exp (args, & arg_m);

  switch (arg_m.X_op)
    {
    case O_md1:
      if (*p == ',')
	{
	  p = parse_exp (p + 1, & arg_r);

	  if ((arg_r.X_md == 0)
	      && (arg_r.X_op == O_register)
	      && (arg_r.X_add_number < 8))
	    opcode += arg_r.X_add_number - 6; /* Emit_mx () will add 6.  */
	  else
	    {
	      ill_op ();
	      break;
	    }
	  check_mach (INS_ROT_II_LD);
	}
      /* Fall through.  */
    case O_register:
      emit_mx (prefix, opcode, 0, & arg_m);
      break;
    default:
      ill_op ();
    }
  return p;
}

static void
emit_sx (char prefix, char opcode, expressionS * arg_p)
{
  char *q;

  switch (arg_p->X_op)
    {
    case O_register:
    case O_md1:
      emit_mx (prefix, opcode, 0, arg_p);
      break;
    default:
      if (arg_p->X_md)
	ill_op ();
      else
	{
	  q = frag_more (prefix ? 2 : 1);
	  if (prefix)
	    *q++ = prefix;
	  *q = opcode ^ 0x46;
	  emit_byte (arg_p, BFD_RELOC_8);
	}
    }
}

/* The operand s may be r, (hl), (ix+d), (iy+d), n.  */
static const char *
emit_s (char prefix, char opcode, const char *args)
{
  expressionS arg_s;
  const char *p;

  p = parse_exp (args, & arg_s);
  if (*p == ',' && arg_s.X_md == 0 && arg_s.X_op == O_register && arg_s.X_add_number == REG_A)
    { /* possible instruction in generic format op A,x */
      if (!(ins_ok & INS_EZ80) && !sdcc_compat)
        ill_op ();
      ++p;
      p = parse_exp (p, & arg_s);
    }
  emit_sx (prefix, opcode, & arg_s);
  return p;
}

static const char *
emit_call (char prefix ATTRIBUTE_UNUSED, char opcode, const char * args)
{
  expressionS addr;
  const char *p;  char *q;

  p = parse_exp_not_indexed (args, &addr);
  if (addr.X_md)
    ill_op ();
  else
    {
      q = frag_more (1);
      *q = opcode;
      emit_word (& addr);
    }
  return p;
}

/* Operand may be rr, r, (hl), (ix+d), (iy+d).  */
static const char *
emit_incdec (char prefix, char opcode, const char * args)
{
  expressionS operand;
  int rnum;
  const char *p;  char *q;

  p = parse_exp (args, &operand);
  rnum = operand.X_add_number;
  if ((! operand.X_md)
      && (operand.X_op == O_register)
      && (R_ARITH&rnum))
    {
      q = frag_more ((rnum & R_INDEX) ? 2 : 1);
      if (rnum & R_INDEX)
	*q++ = (rnum & R_IX) ? 0xDD : 0xFD;
      *q = prefix + ((rnum & 3) << 4);
    }
  else
    {
      if ((operand.X_op == O_md1) || (operand.X_op == O_register))
	emit_mx (0, opcode, 3, & operand);
      else
	ill_op ();
    }
  return p;
}

static const char *
emit_jr (char prefix ATTRIBUTE_UNUSED, char opcode, const char * args)
{
  expressionS addr;
  const char *p;
  char *q;

  p = parse_exp_not_indexed (args, &addr);
  if (addr.X_md)
    ill_op ();
  else
    {
      q = frag_more (1);
      *q = opcode;
      addr.X_add_number--; /* pcrel computes after offset code */
      emit_byte (&addr, BFD_RELOC_8_PCREL);
    }
  return p;
}

static const char *
emit_jp (char prefix, char opcode, const char * args)
{
  expressionS addr;
  const char *p;
  char *q;
  int rnum;

  p = parse_exp_not_indexed (args, & addr);
  if (addr.X_md)
    {
      rnum = addr.X_add_number;
      if ((O_register == addr.X_op) && (REG_HL == (rnum & ~R_INDEX)))
	{
	  q = frag_more ((rnum & R_INDEX) ? 2 : 1);
	  if (rnum & R_INDEX)
	    *q++ = (rnum & R_IX) ? 0xDD : 0xFD;
	  *q = prefix;
	}
      else
	ill_op ();
    }
  else
    {
      q = frag_more (1);
      *q = opcode;
      emit_word (& addr);
    }
  return p;
}

static const char *
emit_im (char prefix, char opcode, const char * args)
{
  expressionS mode;
  const char *p;
  char *q;

  p = parse_exp (args, & mode);
  if (mode.X_md || (mode.X_op != O_constant))
    ill_op ();
  else
    switch (mode.X_add_number)
      {
      case 1:
      case 2:
	++mode.X_add_number;
	/* Fall through.  */
      case 0:
	q = frag_more (2);
	*q++ = prefix;
	*q = opcode + 8*mode.X_add_number;
	break;
      default:
	ill_op ();
      }
  return p;
}

static const char *
emit_pop (char prefix ATTRIBUTE_UNUSED, char opcode, const char * args)
{
  expressionS regp;
  const char *p;
  char *q;

  p = parse_exp (args, & regp);
  if ((!regp.X_md)
      && (regp.X_op == O_register)
      && (regp.X_add_number & R_STACKABLE))
    {
      int rnum;

      rnum = regp.X_add_number;
      if (rnum&R_INDEX)
	{
	  q = frag_more (2);
	  *q++ = (rnum&R_IX)?0xDD:0xFD;
	}
      else
	q = frag_more (1);
      *q = opcode + ((rnum & 3) << 4);
    }
  else
    ill_op ();

  return p;
}

static const char *
emit_retcc (char prefix ATTRIBUTE_UNUSED, char opcode, const char * args)
{
  char cc, *q;
  const char *p;

  p = parse_cc (args, &cc);
  q = frag_more (1);
  if (p)
    *q = opcode + cc;
  else
    *q = prefix;
  return p ? p : args;
}

static const char *
emit_adc (char prefix, char opcode, const char * args)
{
  expressionS term;
  int rnum;
  const char *p;
  char *q;

  p = parse_exp (args, &term);
  if (*p++ != ',')
    {
      error (_("bad instruction syntax"));
      return p;
    }

  if ((term.X_md) || (term.X_op != O_register))
    ill_op ();
  else
    switch (term.X_add_number)
      {
      case REG_A:
	p = emit_s (0, prefix, p);
	break;
      case REG_HL:
	p = parse_exp (p, &term);
	if ((!term.X_md) && (term.X_op == O_register))
	  {
	    rnum = term.X_add_number;
	    if (R_ARITH == (rnum & (R_ARITH | R_INDEX)))
	      {
		q = frag_more (2);
		*q++ = 0xED;
		*q = opcode + ((rnum & 3) << 4);
		break;
	      }
	  }
	/* Fall through.  */
      default:
	ill_op ();
      }
  return p;
}

static const char *
emit_add (char prefix, char opcode, const char * args)
{
  expressionS term;
  int lhs, rhs;
  const char *p;
  char *q;

  p = parse_exp (args, &term);
  if (*p++ != ',')
    {
      error (_("bad instruction syntax"));
      return p;
    }

  if ((term.X_md) || (term.X_op != O_register))
    ill_op ();
  else
    switch (term.X_add_number & ~R_INDEX)
      {
      case REG_A:
	p = emit_s (0, prefix, p);
	break;
      case REG_HL:
	lhs = term.X_add_number;
	p = parse_exp (p, &term);
	if ((!term.X_md) && (term.X_op == O_register))
	  {
	    rhs = term.X_add_number;
	    if ((rhs & R_ARITH)
		&& ((rhs == lhs) || ((rhs & ~R_INDEX) != REG_HL)))
	      {
		q = frag_more ((lhs & R_INDEX) ? 2 : 1);
		if (lhs & R_INDEX)
		  *q++ = (lhs & R_IX) ? 0xDD : 0xFD;
		*q = opcode + ((rhs & 3) << 4);
		break;
	      }
	  }
	/* Fall through.  */
      default:
	ill_op ();
      }
  return p;
}

static const char *
emit_bit (char prefix, char opcode, const char * args)
{
  expressionS b;
  int bn;
  const char *p;

  p = parse_exp (args, &b);
  if (*p++ != ',')
    error (_("bad instruction syntax"));

  bn = b.X_add_number;
  if ((!b.X_md)
      && (b.X_op == O_constant)
      && (0 <= bn)
      && (bn < 8))
    {
      if (opcode == 0x40)
	/* Bit : no optional third operand.  */
	p = emit_m (prefix, opcode + (bn << 3), p);
      else
	/* Set, res : resulting byte can be copied to register.  */
        p = emit_mr (prefix, opcode + (bn << 3), p);
    }
  else
    ill_op ();
  return p;
}

static const char *
emit_jpcc (char prefix, char opcode, const char * args)
{
  char cc;
  const char *p;

  p = parse_cc (args, & cc);
  if (p && *p++ == ',')
    p = emit_call (0, opcode + cc, p);
  else
    p = (prefix == (char)0xC3)
      ? emit_jp (0xE9, prefix, args)
      : emit_call (0, prefix, args);
  return p;
}

static const char *
emit_jrcc (char prefix, char opcode, const char * args)
{
  char cc;
  const char *p;

  p = parse_cc (args, &cc);
  if (p && *p++ == ',')
    {
      if (cc > (3 << 3))
	error (_("condition code invalid for jr"));
      else
	p = emit_jr (0, opcode + cc, p);
    }
  else
    p = emit_jr (0, prefix, args);

  return p;
}

static const char *
emit_ex (char prefix_in ATTRIBUTE_UNUSED,
	 char opcode_in ATTRIBUTE_UNUSED, const char * args)
{
  expressionS op;
  const char * p;
  char prefix, opcode;

  p = parse_exp_not_indexed (args, &op);
  p = skip_space (p);
  if (*p++ != ',')
    {
      error (_("bad instruction syntax"));
      return p;
    }

  prefix = opcode = 0;
  if (op.X_op == O_register)
    switch (op.X_add_number | (op.X_md ? 0x8000 : 0))
      {
      case REG_AF:
	if (TOLOWER (*p++) == 'a' && TOLOWER (*p++) == 'f')
	  {
	    /* The scrubber changes '\'' to '`' in this context.  */
	    if (*p == '`')
	      ++p;
	    opcode = 0x08;
	  }
	break;
      case REG_DE:
	if (TOLOWER (*p++) == 'h' && TOLOWER (*p++) == 'l')
	  opcode = 0xEB;
	break;
      case REG_SP|0x8000:
	p = parse_exp (p, & op);
	if (op.X_op == O_register
	    && op.X_md == 0
	    && (op.X_add_number & ~R_INDEX) == REG_HL)
	  {
	    opcode = 0xE3;
	    if (R_INDEX & op.X_add_number)
	      prefix = (R_IX & op.X_add_number) ? 0xDD : 0xFD;
	  }
	break;
      }
  if (opcode)
    emit_insn (prefix, opcode, p);
  else
    ill_op ();

  return p;
}

static const char *
emit_in (char prefix ATTRIBUTE_UNUSED, char opcode ATTRIBUTE_UNUSED,
	const char * args)
{
  expressionS reg, port;
  const char *p;
  char *q;

  p = parse_exp (args, &reg);
  if (*p++ != ',')
    {
      error (_("bad instruction syntax"));
      return p;
    }

  p = parse_exp (p, &port);
  if (reg.X_md == 0
      && reg.X_op == O_register
      && (reg.X_add_number <= 7 || reg.X_add_number == REG_F)
      && (port.X_md))
    {
      if (port.X_op != O_md1 && port.X_op != O_register)
	{
	  if (REG_A == reg.X_add_number)
	    {
	      q = frag_more (1);
	      *q = 0xDB;
	      emit_byte (&port, BFD_RELOC_8);
	    }
	  else
	    ill_op ();
	}
      else
	{
          if (port.X_add_number == REG_C || port.X_add_number == REG_BC)
	    {
              if (port.X_add_number == REG_BC && !(ins_ok & INS_EZ80))
                ill_op ();
              else if (reg.X_add_number == REG_F && !(ins_ok & INS_R800))
                check_mach (INS_IN_F_C);
          q = frag_more (2);
          *q++ = 0xED;
          *q = 0x40|((reg.X_add_number&7)<<3);
	    }
	  else
	    ill_op ();
	}
    }
  else
    ill_op ();
  return p;
}

static const char *
emit_in0 (char prefix ATTRIBUTE_UNUSED, char opcode ATTRIBUTE_UNUSED,
        const char * args)
{
  expressionS reg, port;
  const char *p;
  char *q;

  p = parse_exp (args, &reg);
  if (*p++ != ',')
    {
      error (_("bad instruction syntax"));
      return p;
    }

  p = parse_exp (p, &port);
  if (reg.X_md == 0
      && reg.X_op == O_register
      && reg.X_add_number <= 7
      && port.X_md
      && port.X_op != O_md1
      && port.X_op != O_register)
    {
      q = frag_more (2);
      *q++ = 0xED;
      *q = 0x00|(reg.X_add_number << 3);
      emit_byte (&port, BFD_RELOC_8);
    }
  else
    ill_op ();
  return p;
}

static const char *
emit_out (char prefix ATTRIBUTE_UNUSED, char opcode ATTRIBUTE_UNUSED,
	 const char * args)
{
  expressionS reg, port;
  const char *p;
  char *q;

  p = parse_exp (args, & port);
  if (*p++ != ',')
    {
      error (_("bad instruction syntax"));
      return p;
    }
  p = parse_exp (p, &reg);
  if (!port.X_md)
    { ill_op (); return p; }
  /* Allow "out (c), 0" as unportable instruction.  */
  if (reg.X_op == O_constant && reg.X_add_number == 0)
    {
      check_mach (INS_OUT_C_0);
      reg.X_op = O_register;
      reg.X_add_number = 6;
    }
  if (reg.X_md
      || reg.X_op != O_register
      || reg.X_add_number > 7)
    ill_op ();
  else
    if (port.X_op != O_register && port.X_op != O_md1)
      {
	if (REG_A == reg.X_add_number)
	  {
	    q = frag_more (1);
	    *q = 0xD3;
	    emit_byte (&port, BFD_RELOC_8);
	  }
	else
	  ill_op ();
      }
    else
      {
        if (REG_C == port.X_add_number || port.X_add_number == REG_BC)
	  {
            if (port.X_add_number == REG_BC && !(ins_ok & INS_EZ80))
              ill_op ();
	    q = frag_more (2);
	    *q++ = 0xED;
	    *q = 0x41 | (reg.X_add_number << 3);
	  }
	else
	  ill_op ();
      }
  return p;
}

static const char *
emit_out0 (char prefix ATTRIBUTE_UNUSED, char opcode ATTRIBUTE_UNUSED,
         const char * args)
{
  expressionS reg, port;
  const char *p;
  char *q;

  p = parse_exp (args, & port);
  if (*p++ != ',')
    {
      error (_("bad instruction syntax"));
      return p;
    }
  p = parse_exp (p, &reg);
  if (port.X_md != 0
      && port.X_op != O_register
      && port.X_op != O_md1
      && reg.X_md == 0
      && reg.X_op == O_register
      && reg.X_add_number <= 7)
    {
      q = frag_more (2);
      *q++ = 0xED;
      *q = 0x01 | (reg.X_add_number << 3);
      emit_byte (&port, BFD_RELOC_8);
    }
  else
    ill_op ();
  return p;
}

static const char *
emit_rst (char prefix ATTRIBUTE_UNUSED, char opcode, const char * args)
{
  expressionS addr;
  const char *p;
  char *q;

  p = parse_exp_not_indexed (args, &addr);
  if (addr.X_op != O_constant)
    {
      error ("rst needs constant address");
      return p;
    }

  if (addr.X_add_number & ~(7 << 3))
    ill_op ();
  else
    {
      q = frag_more (1);
      *q = opcode + (addr.X_add_number & (7 << 3));
    }
  return p;
}

/* For 8-bit indirect load to memory instructions like: LD (HL),n or LD (ii+d),n.  */
static void
emit_ld_m_n (expressionS *dst, expressionS *src)
{
  char *q;
  char prefix;
  expressionS dst_offset;

  switch (dst->X_add_number)
    {
    case REG_HL: prefix = 0x00; break;
    case REG_IX: prefix = 0xDD; break;
    case REG_IY: prefix = 0xFD; break;
    default:
      ill_op ();
      return;
    }

  q = frag_more (prefix ? 2 : 1);
  if (prefix)
    *q++ = prefix;
  *q = 0x36;
  if (prefix)
    {
      dst_offset = *dst;
      dst_offset.X_op = O_symbol;
      dst_offset.X_add_number = 0;
      emit_byte (& dst_offset, BFD_RELOC_Z80_DISP8);
    }
  emit_byte (src, BFD_RELOC_8);
}

/* For 8-bit load register to memory instructions: LD (<expression>),r.  */
static void
emit_ld_m_r (expressionS *dst, expressionS *src)
{
  char *q;
  char prefix = 0;
  expressionS dst_offset;

  switch (dst->X_op)
    {
    case O_md1:
      prefix = (dst->X_add_number == REG_IX) ? 0xDD : 0xFD;
      /* Fall through.  */
    case O_register:
      switch (dst->X_add_number)
        {
        case REG_BC: /* LD (BC),A */
        case REG_DE: /* LD (DE),A */
          if (src->X_add_number == REG_A)
            {
              q = frag_more (1);
              *q = 0x02 | ((dst->X_add_number & 3) << 4);
              return;
            }
          break;
        case REG_IX:
        case REG_IY:
        case REG_HL: /* LD (HL),r or LD (ii+d),r */
          if (src->X_add_number <= 7)
            {
              q = frag_more (prefix ? 2 : 1);
              if (prefix)
                *q++ = prefix;
              *q = 0x70 | src->X_add_number;
              if (prefix)
                {
                  dst_offset = *dst;
                  dst_offset.X_op = O_symbol;
                  dst_offset.X_add_number = 0;
                  emit_byte (& dst_offset, BFD_RELOC_Z80_DISP8);
                }
              return;
            }
          break;
        default:;
        }
        break;
    default: /* LD (nn),A */
      if (src->X_add_number == REG_A)
        {
          q = frag_more (1);
          *q = 0x32;
          emit_word (dst);
          return;
        }
      break;
    }
    ill_op ();
}

/* For 16-bit load register to memory instructions: LD (<expression>),rr.  */
static void
emit_ld_m_rr (expressionS *dst, expressionS *src)
{
  char *q;
  int prefix = 0;
  int opcode = 0;
  expressionS dst_offset;

  switch (dst->X_op)
    {
    case O_md1:      /* eZ80 instructions LD (ii+d),rr */
    case O_register: /* eZ80 instructions LD (HL),rr */
      if (!(ins_ok & INS_EZ80)) /* 16-bit indirect load group is supported by eZ80 only */
          ill_op ();
      switch (dst->X_add_number)
        {
        case REG_IX: prefix = 0xDD; break;
        case REG_IY: prefix = 0xFD; break;
        case REG_HL: prefix = 0xED; break;
        default:
          ill_op ();
        }
      switch (src->X_add_number)
        {
        case REG_BC: opcode = 0x0F; break;
        case REG_DE: opcode = 0x1F; break;
        case REG_HL: opcode = 0x2F; break;
	case REG_IX: opcode = (prefix != 0xFD) ? 0x3F : 0x3E; break;
	case REG_IY: opcode = (prefix != 0xFD) ? 0x3E : 0x3F; break;
        default:
          ill_op ();
        }
        q = frag_more (prefix ? 2 : 1);
        *q++ = prefix;
        *q = opcode;
	if (prefix == 0xFD || prefix == 0xDD)
          {
            dst_offset = *dst;
            dst_offset.X_op = O_symbol;
            dst_offset.X_add_number = 0;
            emit_byte (& dst_offset, BFD_RELOC_Z80_DISP8);
          }
        break;
    default: /* LD (nn),rr */
      if (ins_ok & INS_GBZ80)
        {
          /* GBZ80 supports only LD (nn),SP */
          if (src->X_add_number == REG_SP)
            {
              prefix = 0x00;
              opcode = 0x08;
            }
          else
            ill_op ();
        }
      else
        {
          switch (src->X_add_number)
            {
            case REG_BC: prefix = 0xED; opcode = 0x43; break;
            case REG_DE: prefix = 0xED; opcode = 0x53; break;
            case REG_HL: prefix = 0x00; opcode = 0x22; break;
            case REG_IX: prefix = 0xDD; opcode = 0x22; break;
            case REG_IY: prefix = 0xFD; opcode = 0x22; break;
            case REG_SP: prefix = 0xED; opcode = 0x73; break;
            default:
              ill_op ();
            }
        }
      q = frag_more (prefix ? 2 : 1);
      if (prefix)
        *q++ = prefix;
      *q = opcode;
      emit_word (dst);
    }
}

static void
emit_ld_r_m (expressionS *dst, expressionS *src)
{ /* for 8-bit memory load to register: LD r,(xxx) */
  char *q;
  char prefix = 0;
  char opcode = 0;
  expressionS src_offset;

  if (dst->X_add_number == REG_A && src->X_op == O_register)
    { /* LD A,(BC) or LD A,(DE) */
      switch (src->X_add_number)
        {
        case REG_BC: opcode = 0x0A; break;
        case REG_DE: opcode = 0x1A; break;
        default: break;
        }
      if (opcode != 0)
        {
          q = frag_more (1);
          *q = opcode;
          return;
        }
    }

  switch (src->X_op)
    {
    case O_md1:
    case O_register:
      if (dst->X_add_number > 7)
        ill_op ();
      opcode = 0x46; /* LD B,(HL) */
      switch (src->X_add_number)
        {
        case REG_HL: prefix = 0x00; break;
        case REG_IX: prefix = 0xDD; break;
        case REG_IY: prefix = 0xFD; break;
        default:
          ill_op ();
        }
      q = frag_more (prefix ? 2 : 1);
      if (prefix)
        *q++ = prefix;
      *q = opcode | ((dst->X_add_number & 7) << 3);
      if (prefix)
        {
          src_offset = *src;
          src_offset.X_op = O_symbol;
          src_offset.X_add_number = 0;
          emit_byte (& src_offset, BFD_RELOC_Z80_DISP8);
        }
      break;
    default: /* LD A,(nn) */
      if (dst->X_add_number == REG_A)
        {
          q = frag_more (1);
          *q = 0x3A;
          emit_word (src);
        }
    }
}

static void
emit_ld_r_n (expressionS *dst, expressionS *src)
{ /* for 8-bit immediate value load to register: LD r,n */
  char *q;
  char prefix = 0;

  switch (dst->X_add_number)
    {
    case REG_H|R_IX:
    case REG_L|R_IX:
      prefix = 0xDD;
      break;
    case REG_H|R_IY:
    case REG_L|R_IY:
      prefix = 0xFD;
      break;
    case REG_A:
    case REG_B:
    case REG_C:
    case REG_D:
    case REG_E:
    case REG_H:
    case REG_L:
      break;
    default:
      ill_op ();
//      return;
    }

  q = frag_more (prefix ? 2 : 1);
  if (prefix)
    {
      if (ins_ok & INS_GBZ80)
        ill_op ();
      else if (!(ins_ok & INS_EZ80))
        check_mach (INS_IDX_HALF);
      *q++ = prefix;
    }
  *q = 0x06 | ((dst->X_add_number & 7) << 3);
  emit_byte (src, BFD_RELOC_8);
}

static void
emit_ld_r_r (expressionS *dst, expressionS *src)
{ /* mostly 8-bit load register from register instructions: LD r,r */
  /* there are some exceptions: LD SP,HL/IX/IY; LD I,HL and LD HL,I */
  char *q;
  int prefix = 0;
  int opcode = 0;
  int ii_halves = 0;

  switch (dst->X_add_number)
    {
    case REG_SP:
      switch (src->X_add_number)
        {
        case REG_HL: prefix = 0x00; break;
        case REG_IX: prefix = 0xDD; break;
        case REG_IY: prefix = 0xFD; break;
        default:
          ill_op ();
        }
      if (ins_ok & INS_GBZ80)
        ill_op ();
      opcode = 0xF9;
      break;
    case REG_HL:
      if (!(ins_ok & INS_EZ80))
        ill_op ();
      if (src->X_add_number != REG_I)
        ill_op ();
      if (cpu_mode < 1)
        error (_("ADL mode instruction"));
      /* LD HL,I */
      prefix = 0xED;
      opcode = 0xD7;
      break;
    case REG_I:
      if (src->X_add_number == REG_HL)
        {
          if (!(ins_ok & INS_EZ80))
            ill_op ();
          if (cpu_mode < 1)
            error (_("ADL mode instruction"));
          prefix = 0xED;
          opcode = 0xC7;
        }
      else if (src->X_add_number == REG_A)
        {
          prefix = 0xED;
          opcode = 0x47;
        }
      else
        ill_op ();
      break;
    case REG_MB:
      if (!(ins_ok & INS_EZ80) || (src->X_add_number != REG_A))
        ill_op ();
      if (cpu_mode < 1)
        error (_("ADL mode instruction"));
      prefix = 0xED;
      opcode = 0x6D;
      break;
    case REG_R:
      if (src->X_add_number == REG_A) /* LD R,A */
        {
          prefix = 0xED;
          opcode = 0x4F;
        }
      else
        ill_op ();
      break;
    case REG_A:
      if (src->X_add_number == REG_I) /* LD A,I */
        {
          prefix = 0xED;
          opcode = 0x57;
          break;
        }
      else if (src->X_add_number == REG_R) /* LD A,R */
        {
          prefix = 0xED;
          opcode = 0x5F;
          break;
        }
      else if (src->X_add_number == REG_MB) /* LD A,MB */
        {
          if (!(ins_ok & INS_EZ80))
            ill_op ();
          else
            {
              if (cpu_mode < 1)
                error (_("ADL mode instruction"));
              prefix = 0xED;
              opcode = 0x6E;
            }
          break;
        }
      /* Fall through. */
    case REG_B:
    case REG_C:
    case REG_D:
    case REG_E:
    case REG_H:
    case REG_L:
      prefix = 0x00;
      break;
    case REG_H|R_IX:
    case REG_L|R_IX:
      prefix = 0xDD;
      ii_halves = 1;
      break;
    case REG_H|R_IY:
    case REG_L|R_IY:
      prefix = 0xFD;
      ii_halves = 1;
      break;
    default:
      ill_op ();
    }

  if (opcode == 0)
    {
      switch (src->X_add_number)
        {
          case REG_A:
          case REG_B:
          case REG_C:
          case REG_D:
          case REG_E:
            break;
          case REG_H:
          case REG_L:
            if (prefix != 0)
              ill_op (); /* LD iiH/L,H/L are not permitted */
            break;
          case REG_H|R_IX:
          case REG_L|R_IX:
	    if (prefix == 0xFD || dst->X_add_number == REG_H || dst->X_add_number == REG_L)
              ill_op (); /* LD IYL,IXL and LD H,IXH are not permitted */
            prefix = 0xDD;
            ii_halves = 1;
            break;
          case REG_H|R_IY:
          case REG_L|R_IY:
	    if (prefix == 0xDD || dst->X_add_number == REG_H || dst->X_add_number == REG_L)
              ill_op (); /* LD IXH,IYH and LD L,IYL are not permitted */
            prefix = 0xFD;
            ii_halves = 1;
            break;
          default:
            ill_op ();
        }
      opcode = 0x40 + ((dst->X_add_number & 7) << 3) + (src->X_add_number & 7);
    }
  if ((ins_ok & INS_GBZ80) && prefix != 0)
    ill_op ();
  if (ii_halves && !(ins_ok & INS_EZ80))
    check_mach (INS_IDX_HALF);
  if (prefix == 0 && (ins_ok & INS_EZ80))
    {
      switch (opcode)
        {
        case 0x40: /* SIS prefix, in Z80 it is LD B,B */
        case 0x49: /* LIS prefix, in Z80 it is LD C,C */
        case 0x52: /* SIL prefix, in Z80 it is LD D,D */
        case 0x5B: /* LIL prefix, in Z80 it is LD E,E */
          as_warn (_("unsupported instruction, assembled as NOP"));
          opcode = 0x00;
          break;
        default:;
        }
    }
  q = frag_more (prefix ? 2 : 1);
  if (prefix)
    *q++ = prefix;
  *q = opcode;
}

static void
emit_ld_rr_m (expressionS *dst, expressionS *src)
{ /* for 16-bit indirect load from memory to register: LD rr,(xxx) */
  char *q;
  int prefix = 0;
  int opcode = 0;
  expressionS src_offset;

  /* GBZ80 has no support for 16-bit load from memory instructions */
  if (ins_ok & INS_GBZ80)
    ill_op ();

  prefix = 0xED;
  switch (src->X_op)
    {
    case O_md1: /* LD rr,(ii+d) */
      prefix = (src->X_add_number == REG_IX) ? 0xDD : 0xFD;
      /* Fall through.  */
    case O_register: /* LD rr,(HL) */
      /* currently only EZ80 has support for 16bit indirect memory load instructions */
      if (!(ins_ok & INS_EZ80))
        ill_op ();
      switch (dst->X_add_number)
        {
        case REG_BC: opcode = 0x07; break;
        case REG_DE: opcode = 0x17; break;
        case REG_HL: opcode = 0x27; break;
	case REG_IX: opcode = (!prefix || prefix == 0xDD) ? 0x37 : 0x31; break;
	case REG_IY: opcode = prefix ? ((prefix == 0xDD) ? 0x31 : 0x37) : 0x36; break;
        default:
          ill_op ();
        }
      q = frag_more (2);
      *q++ = prefix;
      *q = opcode;
      if (prefix != 0xED)
        {
          src_offset = *src;
          src_offset.X_op = O_symbol;
          src_offset.X_add_number = 0;
          emit_byte (& src_offset, BFD_RELOC_Z80_DISP8);
        }
      break;
    default: /* LD rr,(nn) */
      switch (dst->X_add_number)
        {
        case REG_BC: prefix = 0xED; opcode = 0x4B; break;
        case REG_DE: prefix = 0xED; opcode = 0x5B; break;
        case REG_HL: prefix = 0x00; opcode = 0x2A; break;
        case REG_SP: prefix = 0xED; opcode = 0x7B; break;
        case REG_IX: prefix = 0xDD; opcode = 0x2A; break;
        case REG_IY: prefix = 0xFD; opcode = 0x2A; break;
        default:
          ill_op ();
        }
      q = frag_more (prefix ? 2 : 1);
      if (prefix)
        *q++ = prefix;
      *q = opcode;
      emit_word (src);
    }
    return;
}

static void
emit_ld_rr_nn (expressionS *dst, expressionS *src)
{ /* mostly load imediate value to multibyte register instructions: LD rr,nn */
  char *q;
  int prefix = 0x00;
  int opcode = 0x21; /* LD HL,nn */
  switch (dst->X_add_number)
    {
    case REG_IX:
      prefix = 0xDD;
      break;
    case REG_IY:
      prefix = 0xFD;
      break;
    case REG_HL:
      break;
    case REG_BC:
    case REG_DE:
    case REG_SP:
      opcode = 0x01 + ((dst->X_add_number & 3) << 4);
      break;
    default:
      ill_op ();
      return;
    }
  if (prefix && (ins_ok & INS_GBZ80))
    ill_op ();
  q = frag_more (prefix ? 2 : 1);
  if (prefix)
    *q++ = prefix;
  *q = opcode;
  emit_word (src);
}

static const char *
emit_ld (char prefix_in ATTRIBUTE_UNUSED, char opcode_in ATTRIBUTE_UNUSED,
	const char * args)
{
  expressionS dst, src;
  const char *p;

  p = parse_exp (args, & dst);
  if (*p++ != ',')
    error (_("bad instruction syntax"));
  p = parse_exp (p, & src);

  if (dst.X_md)
    {
      if (src.X_op == O_register)
        {
          if (src.X_add_number <= 7)
            emit_ld_m_r (& dst, & src); /* LD (xxx),r */
          else
            emit_ld_m_rr (& dst, & src); /* LD (xxx),rr */
        }
      else
        emit_ld_m_n (& dst, & src); /* LD (hl),n or LD (ix/y+r),n */
    }
  else if (dst.X_op == O_register)
    {
      if (src.X_md)
        {
          if (dst.X_add_number <= 7)
            emit_ld_r_m (& dst, & src);
          else
            emit_ld_rr_m (& dst, & src);
        }
      else if (src.X_op == O_register)
        emit_ld_r_r (& dst, & src);
      else if ((dst.X_add_number & ~R_INDEX) <= 7)
        emit_ld_r_n (& dst, & src);
      else
        emit_ld_rr_nn (& dst, & src);
    }
  else
    ill_op ();

  return p;
}

static const char *
emit_lddldi (char prefix, char opcode, const char * args)
{
  expressionS dst, src;
  const char *p;
  char *q;

  if (!(ins_ok & INS_GBZ80))
    return emit_insn (prefix, opcode, args);

  p = parse_exp (args, & dst);
  if (*p++ != ',')
    error (_("bad instruction syntax"));
  p = parse_exp (args, & src);

  if (dst.X_op != O_register || src.X_op != O_register)
    ill_op ();

  /* convert opcode 0xA0 . 0x22, 0xA8 . 0x32 */
  opcode = (opcode & 0x08) * 2 + 0x22;

  if (dst.X_md != 0
      && dst.X_add_number == REG_HL
      && src.X_md == 0
      && src.X_add_number == REG_A)
    opcode |= 0x00; /* LDx (HL),A */
  else if (dst.X_md == 0
      && dst.X_add_number == REG_A
      && src.X_md != 0
      && src.X_add_number == REG_HL)
    opcode |= 0x08; /* LDx A,(HL) */
  else
    ill_op ();

  q = frag_more (1);
  *q = opcode;
  return p;
}

static const char *
emit_ldh (char prefix ATTRIBUTE_UNUSED, char opcode ATTRIBUTE_UNUSED,
        const char * args)
{
  expressionS dst, src;
  const char *p;
  char *q;

  p = parse_exp (args, & dst);
  if (*p++ != ',')
    {
      error (_("bad instruction syntax"));
      return p;
    }

  p = parse_exp (p, & src);
  if (dst.X_md == 0
      && dst.X_op == O_register
      && dst.X_add_number == REG_A
      && src.X_md != 0
      && src.X_op != O_md1
      && src.X_op != O_register)
    {
      q = frag_more (1);
      *q = 0xF0;
      emit_byte (& src, BFD_RELOC_8);
    }
  else if (dst.X_md != 0
      && dst.X_op != O_md1
      && src.X_md == 0
      && src.X_op == O_register
      && src.X_add_number == REG_A)
    {
      if (dst.X_op == O_register)
        {
          if (dst.X_add_number == REG_C)
            {
              q = frag_more (1);
              *q = 0xE2;
            }
          else
            ill_op ();
        }
      else
        {
          q = frag_more (1);
          *q = 0xE0;
          emit_byte (& dst, BFD_RELOC_8);
        }
    }
  else
    ill_op ();

  return p;
}

static const char *
parse_lea_pea_args (const char * args, expressionS *op)
{
  const char *p;
  p = parse_exp (args, op);
  if (sdcc_compat && *p == ',' && op->X_op == O_register)
    {
      expressionS off;
      p = parse_exp (p + 1, &off);
      op->X_op = O_add;
      op->X_add_symbol = make_expr_symbol (&off);
    }
  return p;
}

static const char *
emit_lea (char prefix, char opcode, const char * args)
{
  expressionS dst, src;
  const char *p;
  char *q;
  int rnum;

  p = parse_exp (args, & dst);
  if (dst.X_md != 0 || dst.X_op != O_register)
    ill_op ();

  rnum = dst.X_add_number;
  switch (rnum)
    {
    case REG_BC:
    case REG_DE:
    case REG_HL:
      opcode = 0x02 | ((rnum & 0x03) << 4);
      break;
    case REG_IX:
      opcode = 0x32; /* lea ix,ix+d has opcode 0x32; lea ix,iy+d has opcode 0x54 */
      break;
    case REG_IY:
      opcode = 0x33; /* lea iy,iy+d has opcode 0x33; lea iy,ix+d has opcode 0x55 */
      break;
    default:
      ill_op ();
    }

  if (*p++ != ',')
    error (_("bad instruction syntax"));

  p = parse_lea_pea_args (p, & src);
  if (src.X_md != 0 || src.X_op != O_add /*&& src.X_op != O_register*/)
    ill_op ();

  rnum = src.X_add_number;
  switch (src.X_op)
    {
    case O_add:
      break;
    case O_register: /* permit instructions like LEA rr,IX without displacement specified */
      src.X_add_symbol = zero;
      break;
    default:
      ill_op ();
    }

  switch (rnum)
    {
    case REG_IX:
      opcode = (opcode == (char)0x33) ? 0x55 : (opcode|0x00);
      break;
    case REG_IY:
      opcode = (opcode == (char)0x32) ? 0x54 : (opcode|0x01);
    }

  q = frag_more (2);
  *q++ = prefix;
  *q = opcode;

  src.X_op = O_symbol;
  src.X_add_number = 0;
  emit_byte (& src, BFD_RELOC_Z80_DISP8);

  return p;
}

static const char *
emit_mlt (char prefix, char opcode, const char * args)
{
  expressionS arg;
  const char *p;
  char *q;

  p = parse_exp (args, & arg);
  if (arg.X_md != 0 || arg.X_op != O_register || !(arg.X_add_number & R_ARITH))
    ill_op ();

  q = frag_more (2);
  *q++ = prefix;
  *q = opcode | ((arg.X_add_number & 3) << 4);

  return p;
}

static const char *
emit_pea (char prefix, char opcode, const char * args)
{
  expressionS arg;
  const char *p;
  char *q;

  p = parse_lea_pea_args (args, & arg);
  if (arg.X_md != 0
      || (/*arg.X_op != O_register &&*/ arg.X_op != O_add)
      || !(arg.X_add_number & R_INDEX))
    ill_op ();
  /* PEA ii without displacement is mostly typo,
     because there is PUSH instruction which is shorter and faster */
  /*if (arg.X_op == O_register)
    as_warn (_("PEA is used without displacement, use PUSH instead"));*/

  q = frag_more (2);
  *q++ = prefix;
  *q = opcode + (arg.X_add_number == REG_IY ? 1 : 0);

  arg.X_op = O_symbol;
  arg.X_add_number = 0;
  emit_byte (& arg, BFD_RELOC_Z80_DISP8);

  return p;
}

static const char *
emit_reti (char prefix, char opcode, const char * args)
{
  if (ins_ok & INS_GBZ80)
    return emit_insn (0x00, 0xD9, args);

  return emit_insn (prefix, opcode, args);
}

static const char *
emit_tst (char prefix, char opcode, const char *args)
{
  expressionS arg_s;
  const char *p;
  char *q;
  int rnum;

  p = parse_exp (args, & arg_s);
  if (*p == ',' && arg_s.X_md == 0 && arg_s.X_op == O_register && arg_s.X_add_number == REG_A)
    {
      if (!(ins_ok & INS_EZ80))
        ill_op ();
      ++p;
      p = parse_exp (p, & arg_s);
    }

  rnum = arg_s.X_add_number;
  switch (arg_s.X_op)
    {
    case O_md1:
      ill_op ();
      break;
    case O_register:
      rnum = arg_s.X_add_number;
      if (arg_s.X_md != 0)
        {
          if (rnum != REG_HL)
            ill_op ();
          else
            rnum = 6;
        }
      q = frag_more (2);
      *q++ = prefix;
      *q = opcode | (rnum << 3);
      break;
    default:
      if (arg_s.X_md)
        ill_op ();
      q = frag_more (2);
      *q++ = prefix;
      *q = opcode | 0x60;
      emit_byte (& arg_s, BFD_RELOC_8);
    }
  return p;
}

static const char *
emit_tstio (char prefix, char opcode, const char *args)
{
  expressionS arg;
  const char *p;
  char *q;

  p = parse_exp (args, & arg);
  if (arg.X_md || arg.X_op == O_register || arg.X_op == O_md1)
    ill_op ();

  q = frag_more (2);
  *q++ = prefix;
  *q = opcode;
  emit_byte (& arg, BFD_RELOC_8);

  return p;
}

static void
emit_data (int size ATTRIBUTE_UNUSED)
{
  const char *p, *q;
  char *u, quote;
  int cnt;
  expressionS exp;

  if (is_it_end_of_statement ())
    {
      demand_empty_rest_of_line ();
      return;
    }
  p = skip_space (input_line_pointer);

  do
    {
      if (*p == '\"' || *p == '\'')
	{
	    for (quote = *p, q = ++p, cnt = 0; *p && quote != *p; ++p, ++cnt)
	      ;
	    u = frag_more (cnt);
	    memcpy (u, q, cnt);
	    if (!*p)
	      as_warn (_("unterminated string"));
	    else
	      p = skip_space (p+1);
	}
      else
	{
	  p = parse_exp (p, &exp);
	  if (exp.X_op == O_md1 || exp.X_op == O_register)
	    {
	      ill_op ();
	      break;
	    }
	  if (exp.X_md)
	    as_warn (_("parentheses ignored"));
	  emit_byte (&exp, BFD_RELOC_8);
	  p = skip_space (p);
	}
    }
  while (*p++ == ',') ;
  input_line_pointer = (char *)(p-1);
}

static void
z80_cons (int size)
{
  const char *p;
  expressionS exp;

  if (is_it_end_of_statement ())
    {
      demand_empty_rest_of_line ();
      return;
    }
  p = skip_space (input_line_pointer);

  do
    {
      p = parse_exp (p, &exp);
      if (exp.X_op == O_md1 || exp.X_op == O_register)
	{
	  ill_op ();
	  break;
	}
      if (exp.X_md)
	as_warn (_("parentheses ignored"));
      emit_data_val (&exp, size);
      p = skip_space (p);
  } while (*p++ == ',') ;
  input_line_pointer = (char *)(p-1);
}

/* next functions were commented out because it is difficult to mix
   both ADL and Z80 mode instructions within one COFF file:
   objdump cannot recognize point of mode switching.
*/
static void
set_cpu_mode (int mode)
{
  if (ins_ok & INS_EZ80)
    cpu_mode = mode;
  else
    error (_("CPU mode is unsupported by target"));
}

static void
assume (int arg ATTRIBUTE_UNUSED)
{
  char *name;
  char c;
  int n;

  input_line_pointer = (char*)skip_space (input_line_pointer);
  c = get_symbol_name (& name);
  if (strncasecmp (name, "ADL", 4) != 0)
    {
      ill_op ();
      return;
    }

  restore_line_pointer (c);
  input_line_pointer = (char*)skip_space (input_line_pointer);
  if (*input_line_pointer++ != '=')
    {
      error (_("assignment expected"));
      return;
    }
  input_line_pointer = (char*)skip_space (input_line_pointer);
  n = get_single_number ();

  set_cpu_mode (n);
}

static const char *
emit_mulub (char prefix ATTRIBUTE_UNUSED, char opcode, const char * args)
{
  const char *p;

  p = skip_space (args);
  if (TOLOWER (*p++) != 'a' || *p++ != ',')
    ill_op ();
  else
    {
      char *q, reg;

      reg = TOLOWER (*p++);
      switch (reg)
	{
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	  check_mach (INS_R800);
	  if (!*skip_space (p))
	    {
	      q = frag_more (2);
	      *q++ = prefix;
	      *q = opcode + ((reg - 'b') << 3);
	      break;
	    }
	  /* Fall through.  */
	default:
	  ill_op ();
	}
    }
  return p;
}

static const char *
emit_muluw (char prefix ATTRIBUTE_UNUSED, char opcode, const char * args)
{
  const char *p;

  p = skip_space (args);
  if (TOLOWER (*p++) != 'h' || TOLOWER (*p++) != 'l' || *p++ != ',')
    ill_op ();
  else
    {
      expressionS reg;
      char *q;

      p = parse_exp (p, & reg);

      if ((!reg.X_md) && reg.X_op == O_register)
	switch (reg.X_add_number)
	  {
	  case REG_BC:
	  case REG_SP:
	    check_mach (INS_R800);
	    q = frag_more (2);
	    *q++ = prefix;
	    *q = opcode + ((reg.X_add_number & 3) << 4);
	    break;
	  default:
	    ill_op ();
	  }
    }
  return p;
}

static int
assemble_suffix (const char **suffix)
{
  static
  const char sf[8][4] = 
    {
      "il",
      "is",
      "l",
      "lil",
      "lis",
      "s",
      "sil",
      "sis"
    };
  const char *p;
  const char (*t)[4];
  char sbuf[4];
  int i;

  p = *suffix;
  if (*p++ != '.')
    return 0;

  for (i = 0; (i < 3) && (ISALPHA (*p)); i++)
    sbuf[i] = TOLOWER (*p++);
  if (*p && !ISSPACE (*p))
    return 0;
  *suffix = p;
  sbuf[i] = 0;

  t = bsearch (sbuf, sf, ARRAY_SIZE (sf), sizeof (sf[0]), (int(*)(const void*, const void*)) strcmp);
  if (t == NULL)
    return 0;
  i = t - sf;
  switch (i)
    {
      case 0: /* IL */
        i = cpu_mode ? 0x5B : 0x52;
        break;
      case 1: /* IS */
        i = cpu_mode ? 0x49 : 0x40;
        break;
      case 2: /* L */
        i = cpu_mode ? 0x5B : 0x49;
        break;
      case 3: /* LIL */
        i = 0x5B;
        break;
      case 4: /* LIS */
        i = 0x49;
        break;
      case 5: /* S */
        i = cpu_mode ? 0x52 : 0x40;
        break;
      case 6: /* SIL */
        i = 0x52;
        break;
      case 7: /* SIS */
        i = 0x40;
        break;
    }
  *frag_more (1) = (char)i;
  switch (i)
    {
    case 0x40: inst_mode = INST_MODE_FORCED | INST_MODE_S | INST_MODE_IS; break;
    case 0x49: inst_mode = INST_MODE_FORCED | INST_MODE_L | INST_MODE_IS; break;
    case 0x52: inst_mode = INST_MODE_FORCED | INST_MODE_S | INST_MODE_IL; break;
    case 0x5B: inst_mode = INST_MODE_FORCED | INST_MODE_L | INST_MODE_IL; break;
    }
  return 1;
}

static void
psect (int arg)
{
#if defined(OBJ_ELF)
  return obj_elf_section (arg);
#elif defined(OBJ_COFF)
  return obj_coff_section (arg);
#else
#error Unknown object format
#endif
}

static void
set_inss (int inss)
{
  int old_ins;

  if (!sdcc_compat)
    as_fatal (_("Invalid directive"));

  old_ins = ins_ok;
  ins_ok &= INS_MARCH_MASK;
  ins_ok |= inss;
  if (old_ins != ins_ok)
    cpu_mode = 0;
}

static void
ignore (int arg ATTRIBUTE_UNUSED)
{
  ignore_rest_of_line ();
}

static void
area (int arg)
{
  char *p;
  if (!sdcc_compat)
    as_fatal (_("Invalid directive"));
  for (p = input_line_pointer; *p && *p != '(' && *p != '\n'; p++)
    ;
  if (*p == '(')
    {
      *p = '\n';
      psect (arg);
      *p++ = '(';
      ignore_rest_of_line ();
    }
  else
    psect (arg);
}

/* Port specific pseudo ops.  */
const pseudo_typeS md_pseudo_table[] =
{
  { ".area", area, 0},
  { ".assume", assume, 0},
  { ".ez80", set_inss, INS_EZ80},
  { ".gbz80", set_inss, INS_GBZ80},
  { ".module", ignore, 0},
  { ".optsdcc", ignore, 0},
  { ".r800", set_inss, INS_R800},
  { ".set", s_set, 0},
  { ".z180", set_inss, INS_Z180},
  { ".z80", set_inss, INS_Z80},
  { "db" , emit_data, 1},
  { "d24", z80_cons, 3},
  { "d32", z80_cons, 4},
  { "def24", z80_cons, 3},
  { "def32", z80_cons, 4},
  { "defb", emit_data, 1},
  { "defm", emit_data, 1},
  { "defs", s_space, 1}, /* Synonym for ds on some assemblers.  */
  { "defw", z80_cons, 2},
  { "ds",   s_space, 1}, /* Fill with bytes rather than words.  */
  { "dw", z80_cons, 2},
  { "psect", psect, 0}, /* TODO: Translate attributes.  */
  { "set", 0, 0}, 		/* Real instruction on z80.  */
  { NULL, 0, 0 }
} ;

static table_t instab[] =
{
  { "adc",  0x88, 0x4A, emit_adc,  INS_ALL },
  { "add",  0x80, 0x09, emit_add,  INS_ALL },
  { "and",  0x00, 0xA0, emit_s,    INS_ALL },
  { "bit",  0xCB, 0x40, emit_bit,  INS_ALL },
  { "call", 0xCD, 0xC4, emit_jpcc, INS_ALL },
  { "ccf",  0x00, 0x3F, emit_insn, INS_ALL },
  { "cp",   0x00, 0xB8, emit_s,    INS_ALL },
  { "cpd",  0xED, 0xA9, emit_insn, INS_NOT_GBZ80 },
  { "cpdr", 0xED, 0xB9, emit_insn, INS_NOT_GBZ80 },
  { "cpi",  0xED, 0xA1, emit_insn, INS_NOT_GBZ80 },
  { "cpir", 0xED, 0xB1, emit_insn, INS_NOT_GBZ80 },
  { "cpl",  0x00, 0x2F, emit_insn, INS_ALL },
  { "daa",  0x00, 0x27, emit_insn, INS_ALL },
  { "dec",  0x0B, 0x05, emit_incdec,INS_ALL },
  { "di",   0x00, 0xF3, emit_insn, INS_ALL },
  { "djnz", 0x00, 0x10, emit_jr,   INS_NOT_GBZ80 },
  { "ei",   0x00, 0xFB, emit_insn, INS_ALL },
  { "ex",   0x00, 0x00, emit_ex,   INS_NOT_GBZ80 },
  { "exx",  0x00, 0xD9, emit_insn, INS_NOT_GBZ80 },
  { "halt", 0x00, 0x76, emit_insn, INS_ALL },
  { "im",   0xED, 0x46, emit_im,   INS_NOT_GBZ80 },
  { "in",   0x00, 0x00, emit_in,   INS_NOT_GBZ80 },
  { "in0",  0xED, 0x00, emit_in0,  INS_Z180|INS_EZ80 },
  { "inc",  0x03, 0x04, emit_incdec,INS_ALL },
  { "ind",  0xED, 0xAA, emit_insn, INS_NOT_GBZ80 },
  { "ind2", 0xED, 0x8C, emit_insn, INS_EZ80 },
  { "ind2r",0xED, 0x9C, emit_insn, INS_EZ80 },
  { "indm", 0xED, 0x8A, emit_insn, INS_EZ80 },
  { "indmr",0xED, 0x9A, emit_insn, INS_EZ80 },
  { "indr", 0xED, 0xBA, emit_insn, INS_NOT_GBZ80 },
  { "indrx",0xED, 0xCA, emit_insn, INS_EZ80 },
  { "ini",  0xED, 0xA2, emit_insn, INS_NOT_GBZ80 },
  { "ini2", 0xED, 0x84, emit_insn, INS_EZ80 },
  { "ini2r",0xED, 0x94, emit_insn, INS_EZ80 },
  { "inim", 0xED, 0x82, emit_insn, INS_EZ80 },
  { "inimr",0xED, 0x92, emit_insn, INS_EZ80 },
  { "inir", 0xED, 0xB2, emit_insn, INS_NOT_GBZ80 },
  { "inirx",0xED, 0xC2, emit_insn, INS_EZ80 },
  { "jp",   0xC3, 0xC2, emit_jpcc, INS_ALL },
  { "jr",   0x18, 0x20, emit_jrcc, INS_ALL },
  { "ld",   0x00, 0x00, emit_ld,   INS_ALL },
  { "ldd",  0xED, 0xA8, emit_lddldi,INS_ALL }, /* GBZ80 has special meaning */
  { "lddr", 0xED, 0xB8, emit_insn, INS_NOT_GBZ80 },
  { "ldh",  0xE0, 0x00, emit_ldh,  INS_GBZ80 },
  { "ldhl", 0xE0, 0x00, emit_ldh,  INS_GBZ80 },
  { "ldi",  0xED, 0xA0, emit_lddldi,INS_ALL }, /* GBZ80 has special meaning */
  { "ldir", 0xED, 0xB0, emit_insn, INS_NOT_GBZ80 },
  { "lea",  0xED, 0x02, emit_lea,  INS_EZ80 },
  { "mlt",  0xED, 0x4C, emit_mlt,  INS_Z180|INS_EZ80 },
  { "mulub",0xED, 0xC5, emit_mulub,INS_R800 },
  { "muluw",0xED, 0xC3, emit_muluw,INS_R800 },
  { "neg",  0xed, 0x44, emit_insn, INS_NOT_GBZ80 },
  { "nop",  0x00, 0x00, emit_insn, INS_ALL },
  { "or",   0x00, 0xB0, emit_s,    INS_ALL },
  { "otd2r",0xED, 0xBC, emit_insn, INS_EZ80 },
  { "otdm", 0xED, 0x8B, emit_insn, INS_Z180|INS_EZ80 },
  { "otdmr",0xED, 0x9B, emit_insn, INS_Z180|INS_EZ80 },
  { "otdr", 0xED, 0xBB, emit_insn, INS_NOT_GBZ80 },
  { "otdrx",0xED, 0xCB, emit_insn, INS_EZ80 },
  { "oti2r",0xED, 0xB4, emit_insn, INS_EZ80 },
  { "otim", 0xED, 0x83, emit_insn, INS_Z180|INS_EZ80 },
  { "otimr",0xED, 0x93, emit_insn, INS_Z180|INS_EZ80 },
  { "otir", 0xED, 0xB3, emit_insn, INS_NOT_GBZ80 },
  { "otirx",0xED, 0xC3, emit_insn, INS_EZ80 },
  { "out",  0x00, 0x00, emit_out,  INS_NOT_GBZ80 },
  { "out0", 0xED, 0x01, emit_out0, INS_Z180|INS_EZ80 },
  { "outd", 0xED, 0xAB, emit_insn, INS_NOT_GBZ80 },
  { "outd2",0xED, 0xAC, emit_insn, INS_EZ80 },
  { "outi", 0xED, 0xA3, emit_insn, INS_NOT_GBZ80 },
  { "outi2",0xED, 0xA4, emit_insn, INS_EZ80 },
  { "pea",  0xED, 0x65, emit_pea,  INS_EZ80 },
  { "pop",  0x00, 0xC1, emit_pop,  INS_ALL },
  { "push", 0x00, 0xC5, emit_pop,  INS_ALL },
  { "res",  0xCB, 0x80, emit_bit,  INS_ALL },
  { "ret",  0xC9, 0xC0, emit_retcc,INS_ALL },
  { "reti", 0xED, 0x4D, emit_reti, INS_ALL }, /*GBZ80 has its own opcode for it*/
  { "retn", 0xED, 0x45, emit_insn, INS_NOT_GBZ80 },
  { "rl",   0xCB, 0x10, emit_mr,   INS_ALL },
  { "rla",  0x00, 0x17, emit_insn, INS_ALL },
  { "rlc",  0xCB, 0x00, emit_mr,   INS_ALL },
  { "rlca", 0x00, 0x07, emit_insn, INS_ALL },
  { "rld",  0xED, 0x6F, emit_insn, INS_NOT_GBZ80 },
  { "rr",   0xCB, 0x18, emit_mr,   INS_ALL },
  { "rra",  0x00, 0x1F, emit_insn, INS_ALL },
  { "rrc",  0xCB, 0x08, emit_mr,   INS_ALL },
  { "rrca", 0x00, 0x0F, emit_insn, INS_ALL },
  { "rrd",  0xED, 0x67, emit_insn, INS_NOT_GBZ80 },
  { "rsmix",0xED, 0x7E, emit_insn, INS_EZ80 },
  { "rst",  0x00, 0xC7, emit_rst,  INS_ALL },
  { "sbc",  0x98, 0x42, emit_adc,  INS_ALL },
  { "scf",  0x00, 0x37, emit_insn, INS_ALL },
  { "set",  0xCB, 0xC0, emit_bit,  INS_ALL },
  { "sla",  0xCB, 0x20, emit_mr,   INS_ALL },
  { "sli",  0xCB, 0x30, emit_mr,   INS_SLI },
  { "sll",  0xCB, 0x30, emit_mr,   INS_SLI },
  { "slp",  0xED, 0x76, emit_insn, INS_Z180|INS_EZ80 },
  { "sra",  0xCB, 0x28, emit_mr,   INS_ALL },
  { "srl",  0xCB, 0x38, emit_mr,   INS_ALL },
  { "stmix",0xED, 0x7D, emit_insn, INS_EZ80 },
  { "stop", 0x00, 0x10, emit_insn, INS_GBZ80 },
  { "sub",  0x00, 0x90, emit_s,    INS_ALL },
  { "swap", 0xCB, 0x30, emit_mr,   INS_GBZ80 },
  { "tst",  0xED, 0x04, emit_tst,  INS_Z180|INS_EZ80 },
  { "tstio",0xED, 0x74, emit_tstio,INS_Z180|INS_EZ80 },
  { "xor",  0x00, 0xA8, emit_s,    INS_ALL },
} ;

void
md_assemble (char *str)
{
  const char *p;
  char * old_ptr;
  int i;
  table_t *insp;

  err_flag = 0;
  inst_mode = cpu_mode ? (INST_MODE_L | INST_MODE_IL) : (INST_MODE_S | INST_MODE_IS);
  old_ptr = input_line_pointer;
  p = skip_space (str);
  for (i = 0; (i < BUFLEN) && (ISALPHA (*p) || ISDIGIT (*p));)
    buf[i++] = TOLOWER (*p++);

  if (i == BUFLEN)
    {
      buf[BUFLEN-3] = buf[BUFLEN-2] = '.'; /* Mark opcode as abbreviated.  */
      buf[BUFLEN-1] = 0;
      as_bad (_("Unknown instruction '%s'"), buf);
    }
  else
    {
      dwarf2_emit_insn (0);
      if ((*p) && (!ISSPACE (*p)))
        {
          if (*p != '.' || !(ins_ok & INS_EZ80) || !assemble_suffix (&p))
            {
              as_bad (_("syntax error"));
              goto end;
            }
        }
      buf[i] = 0;
      p = skip_space (p);
      key = buf;

      insp = bsearch (&key, instab, ARRAY_SIZE (instab),
		    sizeof (instab[0]), key_cmp);
      if (!insp || (insp->inss && !(insp->inss & ins_ok)))
        {
          as_bad (_("Unknown instruction '%s'"), buf);
          *frag_more (1) = 0;
        }
      else
	{
	  p = insp->fp (insp->prefix, insp->opcode, p);
	  p = skip_space (p);
	if ((!err_flag) && *p)
	  as_bad (_("junk at end of line, first unrecognized character is `%c'"),
		  *p);
	}
    }
end:
  input_line_pointer = old_ptr;
}

void
md_apply_fix (fixS * fixP, valueT* valP, segT seg ATTRIBUTE_UNUSED)
{
  long val = * (long *) valP;
  char *p_lit = fixP->fx_where + fixP->fx_frag->fr_literal;

  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_8_PCREL:
      if (fixP->fx_addsy)
        {
          fixP->fx_no_overflow = 1;
          fixP->fx_done = 0;
        }
      else
        {
	  fixP->fx_no_overflow = (-128 <= val && val < 128);
	  if (!fixP->fx_no_overflow)
            as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("relative jump out of range"));
	  *p_lit++ = val;
          fixP->fx_done = 1;
        }
      break;

    case BFD_RELOC_Z80_DISP8:
      if (fixP->fx_addsy)
        {
          fixP->fx_no_overflow = 1;
          fixP->fx_done = 0;
        }
      else
        {
	  fixP->fx_no_overflow = (-128 <= val && val < 128);
	  if (!fixP->fx_no_overflow)
            as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("index offset out of range"));
	  *p_lit++ = val;
          fixP->fx_done = 1;
        }
      break;

    case BFD_RELOC_Z80_BYTE0:
      *p_lit++ = val;
      fixP->fx_no_overflow = 1;
      if (fixP->fx_addsy == NULL)
        fixP->fx_done = 1;
      break;

    case BFD_RELOC_Z80_BYTE1:
      *p_lit++ = (val >> 8);
      fixP->fx_no_overflow = 1;
      if (fixP->fx_addsy == NULL)
        fixP->fx_done = 1;
      break;

    case BFD_RELOC_Z80_BYTE2:
      *p_lit++ = (val >> 16);
      fixP->fx_no_overflow = 1;
      if (fixP->fx_addsy == NULL)
        fixP->fx_done = 1;
      break;

    case BFD_RELOC_Z80_BYTE3:
      *p_lit++ = (val >> 24);
      fixP->fx_no_overflow = 1;
      if (fixP->fx_addsy == NULL)
        fixP->fx_done = 1;
      break;

    case BFD_RELOC_8:
      if (val > 255 || val < -128)
	as_warn_where (fixP->fx_file, fixP->fx_line, _("overflow"));
      *p_lit++ = val;
      fixP->fx_no_overflow = 1;
      if (fixP->fx_addsy == NULL)
	fixP->fx_done = 1;
      break;

    case BFD_RELOC_Z80_WORD1:
      *p_lit++ = (val >> 16);
      *p_lit++ = (val >> 24);
      fixP->fx_no_overflow = 1;
      if (fixP->fx_addsy == NULL)
        fixP->fx_done = 1;
      break;

    case BFD_RELOC_Z80_WORD0:
    case BFD_RELOC_16:
      *p_lit++ = val;
      *p_lit++ = (val >> 8);
      fixP->fx_no_overflow = 1;
      if (fixP->fx_addsy == NULL)
	fixP->fx_done = 1;
      break;

    case BFD_RELOC_24: /* Def24 may produce this.  */
      *p_lit++ = val;
      *p_lit++ = (val >> 8);
      *p_lit++ = (val >> 16);
      fixP->fx_no_overflow = 1;
      if (fixP->fx_addsy == NULL)
	fixP->fx_done = 1;
      break;

    case BFD_RELOC_32: /* Def32 and .long may produce this.  */
      *p_lit++ = val;
      *p_lit++ = (val >> 8);
      *p_lit++ = (val >> 16);
      *p_lit++ = (val >> 24);
      if (fixP->fx_addsy == NULL)
	fixP->fx_done = 1;
      break;

    default:
      printf (_("md_apply_fix: unknown r_type 0x%x\n"), fixP->fx_r_type);
      abort ();
    }
}

/* GAS will call this to generate a reloc.  GAS will pass the
   resulting reloc to `bfd_install_relocation'.  This currently works
   poorly, as `bfd_install_relocation' often does the wrong thing, and
   instances of `tc_gen_reloc' have been written to work around the
   problems, which in turns makes it difficult to fix
   `bfd_install_relocation'.  */

/* If while processing a fixup, a reloc really
   needs to be created then it is done here.  */

arelent *
tc_gen_reloc (asection *seg ATTRIBUTE_UNUSED , fixS *fixp)
{
  arelent *reloc;

  if (! bfd_reloc_type_lookup (stdoutput, fixp->fx_r_type))
    {
      as_bad_where (fixp->fx_file, fixp->fx_line,
		    _("reloc %d not supported by object file format"),
		    (int) fixp->fx_r_type);
      return NULL;
    }

  reloc               = XNEW (arelent);
  reloc->sym_ptr_ptr  = XNEW (asymbol *);
  *reloc->sym_ptr_ptr = symbol_get_bfdsym (fixp->fx_addsy);
  reloc->address      = fixp->fx_frag->fr_address + fixp->fx_where;
  reloc->howto        = bfd_reloc_type_lookup (stdoutput, fixp->fx_r_type);
  reloc->addend       = fixp->fx_offset;

  return reloc;
}

int
z80_tc_label_is_local (const char *name)
{
  const char *n;
  const char *p;
  if (local_label_prefix == NULL)
    return 0;
  for (p = local_label_prefix, n = name; *p && *n && *n == *p; p++, n++)
    ;
  return *p == '\0';
}

/* Parse floating point number from string and compute mantissa and
   exponent. Mantissa is normalized.
*/
#define EXP_MIN -0x10000
#define EXP_MAX 0x10000
static int
str_to_broken_float (bfd_boolean *signP, bfd_uint64_t *mantissaP, int *expP)
{
  char *p;
  bfd_boolean sign;
  bfd_uint64_t mantissa = 0;
  int exponent = 0;
  int i;

  p = (char*)skip_space (input_line_pointer);
  sign = (*p == '-');
  *signP = sign;
  if (sign || *p == '+')
    ++p;
  if (strncasecmp (p, "NaN", 3) == 0)
    {
      *mantissaP = 0;
      *expP = 0;
      input_line_pointer = p + 3;
      return 1;
    }
  if (strncasecmp (p, "inf", 3) == 0)
    {
      *mantissaP = 1ull << 63;
      *expP = EXP_MAX;
      input_line_pointer = p + 3;
      return 1;
    }
  for (; ISDIGIT (*p); ++p)
    {
      if (mantissa >> 60)
	{
	  if (*p >= '5')
	    mantissa++;
	  break;
	}
      mantissa = mantissa * 10 + (*p - '0');
    }
  /* skip non-significant digits */
  for (; ISDIGIT (*p); ++p)
    exponent++;

  if (*p == '.')
    {
      p++;
      if (!exponent) /* If no precission overflow.  */
	{
	  for (; ISDIGIT (*p); ++p, --exponent)
	    {
	      if (mantissa >> 60)
		{
		  if (*p >= '5')
		    mantissa++;
		  break;
		}
	      mantissa = mantissa * 10 + (*p - '0');
	    }
	}
      for (; ISDIGIT (*p); ++p)
	;
    }
  if (*p == 'e' || *p == 'E')
    {
      int es;
      int t = 0;
      ++p;
      es = (*p == '-');
      if (es || *p == '+')
        p++;
      for (; ISDIGIT (*p); ++p)
	{
	  if (t < 100)
	    t = t * 10 + (*p - '0');
	}
      exponent += (es) ? -t : t;
    }
  if (ISALNUM (*p) || *p == '.')
    return 0;
  input_line_pointer = p;
  if (mantissa == 0)
    {
      *mantissaP = 1ull << 63;
      *expP = EXP_MIN;
      return 1; /* result is 0 */
    }
  /* normalization */
  for (; mantissa <= ~0ull/10; --exponent)
    mantissa *= 10;
  /* Now we have sign, mantissa, and signed decimal exponent
     need to recompute to binary exponent.  */
  for (i = 64; exponent > 0; --exponent)
    {
      /* be sure that no integer overflow */
      while (mantissa > ~0ull/10)
	{
	  mantissa >>= 1;
	  i += 1;
	}
	mantissa *= 10;
    }
  for (; exponent < 0; ++exponent)
    {
      while (!(mantissa >> 63))
	{
	  mantissa <<= 1;
	  i -= 1;
	}
	mantissa /= 10;
    }
  /* normalization */
  for (; !(mantissa >> 63); --i)
    mantissa <<= 1;
  *mantissaP = mantissa;
  *expP = i;
  return 1;
}

static const char *
str_to_zeda32(char *litP, int *sizeP)
{
  bfd_uint64_t mantissa;
  bfd_boolean sign;
  int exponent;
  unsigned i;

  *sizeP = 4;
  if (!str_to_broken_float (&sign, &mantissa, &exponent))
    return _("invalid syntax");
  /* I do not know why decrement is needed */
  --exponent;
  /* shift by 39 bits right keeping 25 bit mantissa for rounding */
  mantissa >>= 39;
  /* do rounding */
  ++mantissa;
  /* make 24 bit mantissa */
  mantissa >>= 1;
  /* check for overflow */
  if (mantissa >> 24)
    {
      mantissa >>= 1;
      ++exponent;
    }
  /* check for 0 */
  if (exponent < -127)
    {
      exponent = -128;
      mantissa = 0;
    }
  else if (exponent > 127)
    {
      exponent = -128;
      mantissa = sign ? 0xc00000 : 0x400000;
    }
  else if (mantissa == 0)
    {
      exponent = -128;
      mantissa = 0x200000;
    }
  else if (!sign)
    mantissa &= (1ull << 23) - 1;
  for (i = 0; i < 24; i += 8)
    *litP++ = (char)(mantissa >> i);
  *litP = (char)(0x80 + exponent);
  return NULL;
}

/*
  Math48 by Anders Hejlsberg support.
  Mantissa is 39 bits wide, exponent 8 bit wide.
  Format is:
  bit 47: sign
  bit 46-8: normalized mantissa (bits 38-0, bit39 assumed to be 1)
  bit 7-0: exponent+128 (0 - value is null)
  MIN: 2.938735877e-39
  MAX: 1.701411835e+38
*/
static const char *
str_to_float48(char *litP, int *sizeP)
{
  bfd_uint64_t mantissa;
  bfd_boolean sign;
  int exponent;
  unsigned i;

  *sizeP = 6;
  if (!str_to_broken_float (&sign, &mantissa, &exponent))
    return _("invalid syntax");
  /* shift by 23 bits right keeping 41 bit mantissa for rounding */
  mantissa >>= 23;
  /* do rounding */
  ++mantissa;
  /* make 40 bit mantissa */
  mantissa >>= 1;
  /* check for overflow */
  if (mantissa >> 40)
    {
      mantissa >>= 1;
      ++exponent;
    }
  if (exponent < -127)
    {
      memset (litP, 0, 6);
      return NULL;
    }
  if (exponent > 127)
    return _("overflow");
  if (!sign)
    mantissa &= (1ull << 39) - 1;
  *litP++ = (char)(0x80 + exponent);
  for (i = 0; i < 40; i += 8)
    *litP++ = (char)(mantissa >> i);
  return NULL;
}

static const char *
str_to_ieee754_h(char *litP, int *sizeP)
{
  return ieee_md_atof ('h', litP, sizeP, FALSE);
}

static const char *
str_to_ieee754_s(char *litP, int *sizeP)
{
  return ieee_md_atof ('s', litP, sizeP, FALSE);
}

static const char *
str_to_ieee754_d(char *litP, int *sizeP)
{
  return ieee_md_atof ('d', litP, sizeP, FALSE);
}
