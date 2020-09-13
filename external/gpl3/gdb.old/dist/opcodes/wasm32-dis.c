/* Opcode printing code for the WebAssembly target
   Copyright (C) 2017-2019 Free Software Foundation, Inc.

   This file is part of libopcodes.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "disassemble.h"
#include "opintl.h"
#include "safe-ctype.h"
#include "floatformat.h"
#include "libiberty.h"
#include "elf-bfd.h"
#include "elf/internal.h"
#include "elf/wasm32.h"
#include "bfd_stdint.h"

/* Type names for blocks and signatures.  */
#define BLOCK_TYPE_NONE              0x40
#define BLOCK_TYPE_I32               0x7f
#define BLOCK_TYPE_I64               0x7e
#define BLOCK_TYPE_F32               0x7d
#define BLOCK_TYPE_F64               0x7c

enum wasm_class
{
  wasm_typed,
  wasm_special,
  wasm_break,
  wasm_break_if,
  wasm_break_table,
  wasm_return,
  wasm_call,
  wasm_call_import,
  wasm_call_indirect,
  wasm_get_local,
  wasm_set_local,
  wasm_tee_local,
  wasm_drop,
  wasm_constant_i32,
  wasm_constant_i64,
  wasm_constant_f32,
  wasm_constant_f64,
  wasm_unary,
  wasm_binary,
  wasm_conv,
  wasm_load,
  wasm_store,
  wasm_select,
  wasm_relational,
  wasm_eqz,
  wasm_current_memory,
  wasm_grow_memory,
  wasm_signature
};

struct wasm32_private_data
{
  bfd_boolean print_registers;
  bfd_boolean print_well_known_globals;

  /* Limit valid symbols to those with a given prefix.  */
  const char *section_prefix;
};

typedef struct
{
  const char *name;
  const char *description;
} wasm32_options_t;

static const wasm32_options_t options[] =
{
  { "registers", N_("Disassemble \"register\" names") },
  { "globals",   N_("Name well-known globals") },
};

#define WASM_OPCODE(opcode, name, intype, outtype, clas, signedness)     \
  { name, wasm_ ## clas, opcode },

struct wasm32_opcode_s
{
  const char *name;
  enum wasm_class clas;
  unsigned char opcode;
} wasm32_opcodes[] =
{
#include "opcode/wasm.h"
  { NULL, 0, 0 }
};

/* Parse the disassembler options in OPTS and initialize INFO.  */

static void
parse_wasm32_disassembler_options (struct disassemble_info *info,
                                   const char *opts)
{
  struct wasm32_private_data *private = info->private_data;

  while (opts != NULL)
    {
      if (CONST_STRNEQ (opts, "registers"))
        private->print_registers = TRUE;
      else if (CONST_STRNEQ (opts, "globals"))
        private->print_well_known_globals = TRUE;

      opts = strchr (opts, ',');
      if (opts)
        opts++;
    }
}

/* Check whether SYM is valid.  Special-case absolute symbols, which
   are unhelpful to print, and arguments to a "call" insn, which we
   want to be in a section matching a given prefix.  */

static bfd_boolean
wasm32_symbol_is_valid (asymbol *sym,
                        struct disassemble_info *info)
{
  struct wasm32_private_data *private_data = info->private_data;

  if (sym == NULL)
    return FALSE;

  if (strcmp(sym->section->name, "*ABS*") == 0)
    return FALSE;

  if (private_data && private_data->section_prefix != NULL
      && strncmp (sym->section->name, private_data->section_prefix,
                  strlen (private_data->section_prefix)))
    return FALSE;

  return TRUE;
}

/* Initialize the disassembler structures for INFO.  */

void
disassemble_init_wasm32 (struct disassemble_info *info)
{
  if (info->private_data == NULL)
    {
      static struct wasm32_private_data private;

      private.print_registers = FALSE;
      private.print_well_known_globals = FALSE;
      private.section_prefix = NULL;

      info->private_data = &private;
    }

  if (info->disassembler_options)
    {
      parse_wasm32_disassembler_options (info, info->disassembler_options);

      info->disassembler_options = NULL;
    }

  info->symbol_is_valid = wasm32_symbol_is_valid;
}

/* Read an LEB128-encoded integer from INFO at address PC, reading one
   byte at a time.  Set ERROR_RETURN if no complete integer could be
   read, LENGTH_RETURN to the number oof bytes read (including bytes
   in incomplete numbers).  SIGN means interpret the number as
   SLEB128.  Unfortunately, this is a duplicate of wasm-module.c's
   wasm_read_leb128 ().  */

static uint64_t
wasm_read_leb128 (bfd_vma                   pc,
                  struct disassemble_info * info,
                  bfd_boolean *             error_return,
                  unsigned int *            length_return,
                  bfd_boolean               sign)
{
  uint64_t result = 0;
  unsigned int num_read = 0;
  unsigned int shift = 0;
  unsigned char byte = 0;
  bfd_boolean success = FALSE;

  while (info->read_memory_func (pc + num_read, &byte, 1, info) == 0)
    {
      num_read++;

      result |= ((bfd_vma) (byte & 0x7f)) << shift;

      shift += 7;
      if ((byte & 0x80) == 0)
        {
          success = TRUE;
          break;
        }
    }

  if (length_return != NULL)
    *length_return = num_read;
  if (error_return != NULL)
    *error_return = ! success;

  if (sign && (shift < 8 * sizeof (result)) && (byte & 0x40))
    result |= -((uint64_t) 1 << shift);

  return result;
}

/* Read a 32-bit IEEE float from PC using INFO, convert it to a host
   double, and store it at VALUE.  */

static int
read_f32 (double *value, bfd_vma pc, struct disassemble_info *info)
{
  bfd_byte buf[4];

  if (info->read_memory_func (pc, buf, sizeof (buf), info))
    return -1;

  floatformat_to_double (&floatformat_ieee_single_little, buf,
                         value);

  return sizeof (buf);
}

/* Read a 64-bit IEEE float from PC using INFO, convert it to a host
   double, and store it at VALUE.  */

static int
read_f64 (double *value, bfd_vma pc, struct disassemble_info *info)
{
  bfd_byte buf[8];

  if (info->read_memory_func (pc, buf, sizeof (buf), info))
    return -1;

  floatformat_to_double (&floatformat_ieee_double_little, buf,
                         value);

  return sizeof (buf);
}

/* Main disassembly routine.  Disassemble insn at PC using INFO.  */

int
print_insn_wasm32 (bfd_vma pc, struct disassemble_info *info)
{
  unsigned char opcode;
  struct wasm32_opcode_s *op;
  bfd_byte buffer[16];
  void *stream = info->stream;
  fprintf_ftype prin = info->fprintf_func;
  struct wasm32_private_data *private_data = info->private_data;
  long long constant = 0;
  double fconstant = 0.0;
  long flags = 0;
  long offset = 0;
  long depth = 0;
  long function_index = 0;
  long target_count = 0;
  long block_type = 0;
  int len = 1;
  int ret = 0;
  unsigned int bytes_read = 0;
  int i;
  const char *locals[] =
    {
      "$dpc", "$sp1", "$r0", "$r1", "$rpc", "$pc0",
      "$rp", "$fp", "$sp",
      "$r2", "$r3", "$r4", "$r5", "$r6", "$r7",
      "$i0", "$i1", "$i2", "$i3", "$i4", "$i5", "$i6", "$i7",
      "$f0", "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7",
    };
  int nlocals = ARRAY_SIZE (locals);
  const char *globals[] =
    {
      "$got", "$plt", "$gpo"
    };
  int nglobals = ARRAY_SIZE (globals);
  bfd_boolean error = FALSE;

  if (info->read_memory_func (pc, buffer, 1, info))
    return -1;

  opcode = buffer[0];

  for (op = wasm32_opcodes; op->name; op++)
    if (op->opcode == opcode)
      break;

  if (!op->name)
    {
      prin (stream, "\t.byte 0x%02x\n", buffer[0]);
      return 1;
    }
  else
    {
      len = 1;

      prin (stream, "\t");
      prin (stream, "%s", op->name);

      if (op->clas == wasm_typed)
        {
          block_type = wasm_read_leb128
            (pc + len, info, &error, &bytes_read, FALSE);
          if (error)
            return -1;
          len += bytes_read;
          switch (block_type)
            {
            case BLOCK_TYPE_NONE:
              prin (stream, "[]");
              break;
            case BLOCK_TYPE_I32:
              prin (stream, "[i]");
              break;
            case BLOCK_TYPE_I64:
              prin (stream, "[l]");
              break;
            case BLOCK_TYPE_F32:
              prin (stream, "[f]");
              break;
            case BLOCK_TYPE_F64:
              prin (stream, "[d]");
              break;
            }
        }

      switch (op->clas)
        {
        case wasm_special:
        case wasm_eqz:
        case wasm_binary:
        case wasm_unary:
        case wasm_conv:
        case wasm_relational:
        case wasm_drop:
        case wasm_signature:
        case wasm_call_import:
        case wasm_typed:
        case wasm_select:
          break;

        case wasm_break_table:
          target_count = wasm_read_leb128
            (pc + len, info, &error, &bytes_read, FALSE);
          if (error)
            return -1;
          len += bytes_read;
          prin (stream, " %ld", target_count);
          for (i = 0; i < target_count + 1; i++)
            {
              long target = 0;
              target = wasm_read_leb128
                (pc + len, info, &error, &bytes_read, FALSE);
              if (error)
                return -1;
              len += bytes_read;
              prin (stream, " %ld", target);
            }
          break;

        case wasm_break:
        case wasm_break_if:
          depth = wasm_read_leb128
            (pc + len, info, &error, &bytes_read, FALSE);
          if (error)
            return -1;
          len += bytes_read;
          prin (stream, " %ld", depth);
          break;

        case wasm_return:
          break;

        case wasm_constant_i32:
        case wasm_constant_i64:
          constant = wasm_read_leb128
            (pc + len, info, &error, &bytes_read, TRUE);
          if (error)
            return -1;
          len += bytes_read;
          prin (stream, " %lld", constant);
          break;

        case wasm_constant_f32:
          /* This appears to be the best we can do, even though we're
             using host doubles for WebAssembly floats.  */
          ret = read_f32 (&fconstant, pc + len, info);
          if (ret < 0)
            return -1;
          len += ret;
	  prin (stream, " %.9g", fconstant);
          break;

        case wasm_constant_f64:
          ret = read_f64 (&fconstant, pc + len, info);
          if (ret < 0)
            return -1;
          len += ret;
	  prin (stream, " %.17g", fconstant);
          break;

        case wasm_call:
          function_index = wasm_read_leb128
            (pc + len, info, &error, &bytes_read, FALSE);
          if (error)
            return -1;
          len += bytes_read;
          prin (stream, " ");
          private_data->section_prefix = ".space.function_index";
          (*info->print_address_func) ((bfd_vma) function_index, info);
          private_data->section_prefix = NULL;
          break;

        case wasm_call_indirect:
          constant = wasm_read_leb128
            (pc + len, info, &error, &bytes_read, FALSE);
          if (error)
            return -1;
          len += bytes_read;
          prin (stream, " %lld", constant);
          constant = wasm_read_leb128
            (pc + len, info, &error, &bytes_read, FALSE);
          if (error)
            return -1;
          len += bytes_read;
          prin (stream, " %lld", constant);
          break;

        case wasm_get_local:
        case wasm_set_local:
        case wasm_tee_local:
          constant = wasm_read_leb128
            (pc + len, info, &error, &bytes_read, FALSE);
          if (error)
            return -1;
          len += bytes_read;
          prin (stream, " %lld", constant);
          if (strcmp (op->name + 4, "local") == 0)
            {
              if (private_data->print_registers
                  && constant >= 0 && constant < nlocals)
                prin (stream, " <%s>", locals[constant]);
            }
          else
            {
              if (private_data->print_well_known_globals
                  && constant >= 0 && constant < nglobals)
                prin (stream, " <%s>", globals[constant]);
            }
          break;

        case wasm_grow_memory:
        case wasm_current_memory:
          constant = wasm_read_leb128
            (pc + len, info, &error, &bytes_read, FALSE);
          if (error)
            return -1;
          len += bytes_read;
          prin (stream, " %lld", constant);
          break;

        case wasm_load:
        case wasm_store:
          flags = wasm_read_leb128
            (pc + len, info, &error, &bytes_read, FALSE);
          if (error)
            return -1;
          len += bytes_read;
          offset = wasm_read_leb128
            (pc + len, info, &error, &bytes_read, FALSE);
          if (error)
            return -1;
          len += bytes_read;
          prin (stream, " a=%ld %ld", flags, offset);
        }
    }
  return len;
}

/* Print valid disassembler options to STREAM.  */

void
print_wasm32_disassembler_options (FILE *stream)
{
  unsigned int i, max_len = 0;

  fprintf (stream, _("\
The following WebAssembly-specific disassembler options are supported for use\n\
with the -M switch:\n"));

  for (i = 0; i < ARRAY_SIZE (options); i++)
    {
      unsigned int len = strlen (options[i].name);

      if (max_len < len)
	max_len = len;
    }

  for (i = 0, max_len++; i < ARRAY_SIZE (options); i++)
    fprintf (stream, "  %s%*c %s\n",
	     options[i].name,
	     (int)(max_len - strlen (options[i].name)), ' ',
	     _(options[i].description));
}
