/* LoongArch opcode support.
   Copyright (C) 2021-2022 Free Software Foundation, Inc.
   Contributed by Loongson Ltd.

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
   along with this program; see the file COPYING3.  If not,
   see <http://www.gnu.org/licenses/>.  */

#include "sysdep.h"
#include "disassemble.h"
#include "opintl.h"
#include "opcode/loongarch.h"
#include "libiberty.h"
#include <stdlib.h>

static const struct loongarch_opcode *
get_loongarch_opcode_by_binfmt (insn_t insn)
{
  const struct loongarch_opcode *it;
  struct loongarch_ase *ase;
  size_t i;
  for (ase = loongarch_ASEs; ase->enabled; ase++)
    {
      if (!*ase->enabled || (ase->include && !*ase->include)
	  || (ase->exclude && *ase->exclude))
	continue;

      if (!ase->opc_htab_inited)
	{
	  for (it = ase->opcodes; it->mask; it++)
	    if (!ase->opc_htab[LARCH_INSN_OPC (it->match)]
		&& it->macro == NULL)
	      ase->opc_htab[LARCH_INSN_OPC (it->match)] = it;
	  for (i = 0; i < 16; i++)
	    if (!ase->opc_htab[i])
	      ase->opc_htab[i] = it;
	  ase->opc_htab_inited = 1;
	}

      it = ase->opc_htab[LARCH_INSN_OPC (insn)];
      for (; it->name; it++)
	if ((insn & it->mask) == it->match && it->mask
	    && !(it->include && !*it->include)
	    && !(it->exclude && *it->exclude))
	  return it;
    }
  return NULL;
}

static const char *const *loongarch_r_disname = NULL;
static const char *const *loongarch_f_disname = NULL;
static const char *const *loongarch_c_disname = NULL;
static const char *const *loongarch_cr_disname = NULL;
static const char *const *loongarch_v_disname = NULL;
static const char *const *loongarch_x_disname = NULL;

static void
set_default_loongarch_dis_options (void)
{
  LARCH_opts.ase_ilp32 = 1;
  LARCH_opts.ase_lp64 = 1;
  LARCH_opts.ase_sf = 1;
  LARCH_opts.ase_df = 1;
  LARCH_opts.ase_lsx = 1;
  LARCH_opts.ase_lasx = 1;

  loongarch_r_disname = loongarch_r_lp64_name;
  loongarch_f_disname = loongarch_f_lp64_name;
  loongarch_c_disname = loongarch_c_normal_name;
  loongarch_cr_disname = loongarch_cr_normal_name;
  loongarch_v_disname = loongarch_v_normal_name;
  loongarch_x_disname = loongarch_x_normal_name;
}

static int
parse_loongarch_dis_option (const char *option)
{
  if (strcmp (option, "numeric") == 0)
    {
      loongarch_r_disname = loongarch_r_normal_name;
      loongarch_f_disname = loongarch_f_normal_name;
    }
  return -1;
}

static int
parse_loongarch_dis_options (const char *opts_in)
{
  set_default_loongarch_dis_options ();

  if (opts_in == NULL)
    return 0;

  char *opts, *opt, *opt_end;
  opts = xmalloc (strlen (opts_in) + 1);
  strcpy (opts, opts_in);

  for (opt = opt_end = opts; opt_end != NULL; opt = opt_end + 1)
    {
      if ((opt_end = strchr (opt, ',')) != NULL)
	*opt_end = 0;
      if (parse_loongarch_dis_option (opt) != 0)
	return -1;
    }
  free (opts);
  return 0;
}

static int32_t
dis_one_arg (char esc1, char esc2, const char *bit_field,
	     const char *arg ATTRIBUTE_UNUSED, void *context)
{
  static int need_comma = 0;
  struct disassemble_info *info = context;
  insn_t insn = *(insn_t *) info->private_data;
  int32_t imm, u_imm;

  if (esc1)
    {
      if (need_comma)
	info->fprintf_func (info->stream, ", ");
      need_comma = 1;
      imm = loongarch_decode_imm (bit_field, insn, 1);
      u_imm = loongarch_decode_imm (bit_field, insn, 0);
    }

  switch (esc1)
    {
    case 'r':
      info->fprintf_func (info->stream, "%s", loongarch_r_disname[u_imm]);
      break;
    case 'f':
      info->fprintf_func (info->stream, "%s", loongarch_f_disname[u_imm]);
      break;
    case 'c':
      switch (esc2)
	{
	case 'r':
	  info->fprintf_func (info->stream, "%s", loongarch_cr_disname[u_imm]);
	  break;
	default:
	  info->fprintf_func (info->stream, "%s", loongarch_c_disname[u_imm]);
	}
      break;
    case 'v':
      info->fprintf_func (info->stream, "%s", loongarch_v_disname[u_imm]);
      break;
    case 'x':
      info->fprintf_func (info->stream, "%s", loongarch_x_disname[u_imm]);
      break;
    case 'u':
      info->fprintf_func (info->stream, "0x%x", u_imm);
      break;
    case 's':
      if (imm == 0)
	info->fprintf_func (info->stream, "%d", imm);
      else
	info->fprintf_func (info->stream, "%d(0x%x)", imm, u_imm);
      switch (esc2)
	{
	case 'b':
	  info->insn_type = dis_branch;
	  info->target += imm;
	}
      break;
    case '\0':
      need_comma = 0;
    }
  return 0;
}

static void
disassemble_one (insn_t insn, struct disassemble_info *info)
{
  const struct loongarch_opcode *opc = get_loongarch_opcode_by_binfmt (insn);

#ifdef LOONGARCH_DEBUG
  char have_space[32] = { 0 };
  insn_t t;
  int i;
  const char *t_f = opc ? opc->format : NULL;
  if (t_f)
    while (*t_f)
      {
	while (('a' <= t_f[0] && t_f[0] <= 'z')
	       || ('A' <= t_f[0] && t_f[0] <= 'Z')
	       || t_f[0] == ',')
	  t_f++;
	while (1)
	  {
	    i = strtol (t_f, &t_f, 10);
	    have_space[i] = 1;
	    t_f++; /* ':' */
	    i += strtol (t_f, &t_f, 10);
	    have_space[i] = 1;
	    if (t_f[0] == '|')
	      t_f++;
	    else
	      break;
	  }
	if (t_f[0] == '<')
	  t_f += 2; /* '<' '<' */
	strtol (t_f, &t_f, 10);
      }

  have_space[28] = 1;
  have_space[0] = 0;
  t = ~((insn_t) -1 >> 1);
  for (i = 31; 0 <= i; i--)
    {
      if (t & insn)
	info->fprintf_func (info->stream, "1");
      else
	info->fprintf_func (info->stream, "0");
      if (have_space[i])
	info->fprintf_func (info->stream, " ");
      t = t >> 1;
    }
  info->fprintf_func (info->stream, "\t");
#endif

  if (!opc)
    {
      info->insn_type = dis_noninsn;
      info->fprintf_func (info->stream, "0x%08x", insn);
      return;
    }

  info->insn_type = dis_nonbranch;
  info->fprintf_func (info->stream, "%-12s", opc->name);

  {
    char *fake_args = xmalloc (strlen (opc->format) + 1);
    const char *fake_arg_strs[MAX_ARG_NUM_PLUS_2];
    strcpy (fake_args, opc->format);
    if (0 < loongarch_split_args_by_comma (fake_args, fake_arg_strs))
      info->fprintf_func (info->stream, "\t");
    info->private_data = &insn;
    loongarch_foreach_args (opc->format, fake_arg_strs, dis_one_arg, info);
    free (fake_args);
  }

  if (info->insn_type == dis_branch || info->insn_type == dis_condbranch
      /* Someother if we have extra info to print.  */)
    info->fprintf_func (info->stream, "\t#");

  if (info->insn_type == dis_branch || info->insn_type == dis_condbranch)
    {
      info->fprintf_func (info->stream, " ");
      info->print_address_func (info->target, info);
    }
}

int
print_insn_loongarch (bfd_vma memaddr, struct disassemble_info *info)
{
  insn_t insn;
  int status;

  static int not_init_yet = 1;
  if (not_init_yet)
    {
      parse_loongarch_dis_options (info->disassembler_options);
      not_init_yet = 0;
    }

  info->bytes_per_chunk = 4;
  info->bytes_per_line = 4;
  info->display_endian = BFD_ENDIAN_LITTLE;
  info->insn_info_valid = 1;
  info->target = memaddr;

  if ((status = info->read_memory_func (memaddr, (bfd_byte *) &insn,
					sizeof (insn), info)) != 0)
    {
      info->memory_error_func (status, memaddr, info);
      return -1; /* loongarch_insn_length (0); */
    }

  disassemble_one (insn, info);

  return loongarch_insn_length (insn);
}

void
print_loongarch_disassembler_options (FILE *stream)
{
  fprintf (stream, _("\n\
The following LoongArch disassembler options are supported for use\n\
with the -M switch (multiple options should be separated by commas):\n"));

  fprintf (stream, _("\n\
    numeric       Print numeric register names, rather than ABI names.\n"));
  fprintf (stream, _("\n"));
}

int
loongarch_parse_dis_options (const char *opts_in)
{
  return parse_loongarch_dis_options (opts_in);
}

static void
my_print_address_func (bfd_vma addr, struct disassemble_info *dinfo)
{
  dinfo->fprintf_func (dinfo->stream, "0x%llx", (long long) addr);
}

void
loongarch_disassemble_one (int64_t pc, insn_t insn,
			   int (*fprintf_func) (void *stream,
						const char *format, ...),
			   void *stream)
{
  static struct disassemble_info my_disinfo =
  {
    .print_address_func = my_print_address_func,
  };
  static int not_init_yet = 1;
  if (not_init_yet)
    {
      loongarch_parse_dis_options (NULL);
      not_init_yet = 0;
    }

  my_disinfo.fprintf_func = fprintf_func;
  my_disinfo.stream = stream;
  my_disinfo.target = pc;
  disassemble_one (insn, &my_disinfo);
}
