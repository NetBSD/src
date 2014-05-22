/* Functions for manipulating expressions designed to be executed on the agent
   Copyright (C) 1998-2013 Free Software Foundation, Inc.

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

/* Despite what the above comment says about this file being part of
   GDB, we would like to keep these functions free of GDB
   dependencies, since we want to be able to use them in contexts
   outside of GDB (test suites, the stub, etc.)  */

#include "defs.h"
#include "ax.h"

#include "value.h"
#include "gdb_string.h"

#include "user-regs.h"

static void grow_expr (struct agent_expr *x, int n);

static void append_const (struct agent_expr *x, LONGEST val, int n);

static LONGEST read_const (struct agent_expr *x, int o, int n);

static void generic_ext (struct agent_expr *x, enum agent_op op, int n);

/* Functions for building expressions.  */

/* Allocate a new, empty agent expression.  */
struct agent_expr *
new_agent_expr (struct gdbarch *gdbarch, CORE_ADDR scope)
{
  struct agent_expr *x = xmalloc (sizeof (*x));

  x->len = 0;
  x->size = 1;			/* Change this to a larger value once
				   reallocation code is tested.  */
  x->buf = xmalloc (x->size);

  x->gdbarch = gdbarch;
  x->scope = scope;

  /* Bit vector for registers used.  */
  x->reg_mask_len = 1;
  x->reg_mask = xmalloc (x->reg_mask_len * sizeof (x->reg_mask[0]));
  memset (x->reg_mask, 0, x->reg_mask_len * sizeof (x->reg_mask[0]));

  return x;
}

/* Free a agent expression.  */
void
free_agent_expr (struct agent_expr *x)
{
  xfree (x->buf);
  xfree (x->reg_mask);
  xfree (x);
}

static void
do_free_agent_expr_cleanup (void *x)
{
  free_agent_expr (x);
}

struct cleanup *
make_cleanup_free_agent_expr (struct agent_expr *x)
{
  return make_cleanup (do_free_agent_expr_cleanup, x);
}


/* Make sure that X has room for at least N more bytes.  This doesn't
   affect the length, just the allocated size.  */
static void
grow_expr (struct agent_expr *x, int n)
{
  if (x->len + n > x->size)
    {
      x->size *= 2;
      if (x->size < x->len + n)
	x->size = x->len + n + 10;
      x->buf = xrealloc (x->buf, x->size);
    }
}


/* Append the low N bytes of VAL as an N-byte integer to the
   expression X, in big-endian order.  */
static void
append_const (struct agent_expr *x, LONGEST val, int n)
{
  int i;

  grow_expr (x, n);
  for (i = n - 1; i >= 0; i--)
    {
      x->buf[x->len + i] = val & 0xff;
      val >>= 8;
    }
  x->len += n;
}


/* Extract an N-byte big-endian unsigned integer from expression X at
   offset O.  */
static LONGEST
read_const (struct agent_expr *x, int o, int n)
{
  int i;
  LONGEST accum = 0;

  /* Make sure we're not reading off the end of the expression.  */
  if (o + n > x->len)
    error (_("GDB bug: ax-general.c (read_const): incomplete constant"));

  for (i = 0; i < n; i++)
    accum = (accum << 8) | x->buf[o + i];

  return accum;
}


/* Append a simple operator OP to EXPR.  */
void
ax_simple (struct agent_expr *x, enum agent_op op)
{
  grow_expr (x, 1);
  x->buf[x->len++] = op;
}

/* Append a pick operator to EXPR.  DEPTH is the stack item to pick,
   with 0 being top of stack.  */

void
ax_pick (struct agent_expr *x, int depth)
{
  if (depth < 0 || depth > 255)
    error (_("GDB bug: ax-general.c (ax_pick): stack depth out of range"));
  ax_simple (x, aop_pick);
  append_const (x, 1, depth);
}


/* Append a sign-extension or zero-extension instruction to EXPR, to
   extend an N-bit value.  */
static void
generic_ext (struct agent_expr *x, enum agent_op op, int n)
{
  /* N must fit in a byte.  */
  if (n < 0 || n > 255)
    error (_("GDB bug: ax-general.c (generic_ext): bit count out of range"));
  /* That had better be enough range.  */
  if (sizeof (LONGEST) * 8 > 255)
    error (_("GDB bug: ax-general.c (generic_ext): "
	     "opcode has inadequate range"));

  grow_expr (x, 2);
  x->buf[x->len++] = op;
  x->buf[x->len++] = n;
}


/* Append a sign-extension instruction to EXPR, to extend an N-bit value.  */
void
ax_ext (struct agent_expr *x, int n)
{
  generic_ext (x, aop_ext, n);
}


/* Append a zero-extension instruction to EXPR, to extend an N-bit value.  */
void
ax_zero_ext (struct agent_expr *x, int n)
{
  generic_ext (x, aop_zero_ext, n);
}


/* Append a trace_quick instruction to EXPR, to record N bytes.  */
void
ax_trace_quick (struct agent_expr *x, int n)
{
  /* N must fit in a byte.  */
  if (n < 0 || n > 255)
    error (_("GDB bug: ax-general.c (ax_trace_quick): "
	     "size out of range for trace_quick"));

  grow_expr (x, 2);
  x->buf[x->len++] = aop_trace_quick;
  x->buf[x->len++] = n;
}


/* Append a goto op to EXPR.  OP is the actual op (must be aop_goto or
   aop_if_goto).  We assume we don't know the target offset yet,
   because it's probably a forward branch, so we leave space in EXPR
   for the target, and return the offset in EXPR of that space, so we
   can backpatch it once we do know the target offset.  Use ax_label
   to do the backpatching.  */
int
ax_goto (struct agent_expr *x, enum agent_op op)
{
  grow_expr (x, 3);
  x->buf[x->len + 0] = op;
  x->buf[x->len + 1] = 0xff;
  x->buf[x->len + 2] = 0xff;
  x->len += 3;
  return x->len - 2;
}

/* Suppose a given call to ax_goto returns some value PATCH.  When you
   know the offset TARGET that goto should jump to, call
   ax_label (EXPR, PATCH, TARGET)
   to patch TARGET into the ax_goto instruction.  */
void
ax_label (struct agent_expr *x, int patch, int target)
{
  /* Make sure the value is in range.  Don't accept 0xffff as an
     offset; that's our magic sentinel value for unpatched branches.  */
  if (target < 0 || target >= 0xffff)
    error (_("GDB bug: ax-general.c (ax_label): label target out of range"));

  x->buf[patch] = (target >> 8) & 0xff;
  x->buf[patch + 1] = target & 0xff;
}


/* Assemble code to push a constant on the stack.  */
void
ax_const_l (struct agent_expr *x, LONGEST l)
{
  static enum agent_op ops[]
  =
  {aop_const8, aop_const16, aop_const32, aop_const64};
  int size;
  int op;

  /* How big is the number?  'op' keeps track of which opcode to use.
     Notice that we don't really care whether the original number was
     signed or unsigned; we always reproduce the value exactly, and
     use the shortest representation.  */
  for (op = 0, size = 8; size < 64; size *= 2, op++)
    {
      LONGEST lim = ((LONGEST) 1) << (size - 1);

      if (-lim <= l && l <= lim - 1)
        break;
    }

  /* Emit the right opcode...  */
  ax_simple (x, ops[op]);

  /* Emit the low SIZE bytes as an unsigned number.  We know that
     sign-extending this will yield l.  */
  append_const (x, l, size / 8);

  /* Now, if it was negative, and not full-sized, sign-extend it.  */
  if (l < 0 && size < 64)
    ax_ext (x, size);
}


void
ax_const_d (struct agent_expr *x, LONGEST d)
{
  /* FIXME: floating-point support not present yet.  */
  error (_("GDB bug: ax-general.c (ax_const_d): "
	   "floating point not supported yet"));
}


/* Assemble code to push the value of register number REG on the
   stack.  */
void
ax_reg (struct agent_expr *x, int reg)
{
  if (reg >= gdbarch_num_regs (x->gdbarch))
    {
      /* This is a pseudo-register.  */
      if (!gdbarch_ax_pseudo_register_push_stack_p (x->gdbarch))
	error (_("'%s' is a pseudo-register; "
		 "GDB cannot yet trace its contents."),
	       user_reg_map_regnum_to_name (x->gdbarch, reg));
      if (gdbarch_ax_pseudo_register_push_stack (x->gdbarch, x, reg))
	error (_("Trace '%s' failed."),
	       user_reg_map_regnum_to_name (x->gdbarch, reg));
    }
  else
    {
      /* Make sure the register number is in range.  */
      if (reg < 0 || reg > 0xffff)
        error (_("GDB bug: ax-general.c (ax_reg): "
		 "register number out of range"));
      grow_expr (x, 3);
      x->buf[x->len] = aop_reg;
      x->buf[x->len + 1] = (reg >> 8) & 0xff;
      x->buf[x->len + 2] = (reg) & 0xff;
      x->len += 3;
    }
}

/* Assemble code to operate on a trace state variable.  */

void
ax_tsv (struct agent_expr *x, enum agent_op op, int num)
{
  /* Make sure the tsv number is in range.  */
  if (num < 0 || num > 0xffff)
    internal_error (__FILE__, __LINE__, 
		    _("ax-general.c (ax_tsv): variable "
		      "number is %d, out of range"), num);

  grow_expr (x, 3);
  x->buf[x->len] = op;
  x->buf[x->len + 1] = (num >> 8) & 0xff;
  x->buf[x->len + 2] = (num) & 0xff;
  x->len += 3;
}

/* Append a string to the expression.  Note that the string is going
   into the bytecodes directly, not on the stack.  As a precaution,
   include both length as prefix, and terminate with a NUL.  (The NUL
   is counted in the length.)  */

void
ax_string (struct agent_expr *x, const char *str, int slen)
{
  int i;

  /* Make sure the string length is reasonable.  */
  if (slen < 0 || slen > 0xffff)
    internal_error (__FILE__, __LINE__, 
		    _("ax-general.c (ax_string): string "
		      "length is %d, out of allowed range"), slen);

  grow_expr (x, 2 + slen + 1);
  x->buf[x->len++] = ((slen + 1) >> 8) & 0xff;
  x->buf[x->len++] = (slen + 1) & 0xff;
  for (i = 0; i < slen; ++i)
    x->buf[x->len++] = str[i];
  x->buf[x->len++] = '\0';
}



/* Functions for disassembling agent expressions, and otherwise
   debugging the expression compiler.  */

struct aop_map aop_map[] =
{
  {0, 0, 0, 0, 0}
#define DEFOP(NAME, SIZE, DATA_SIZE, CONSUMED, PRODUCED, VALUE) \
  , { # NAME, SIZE, DATA_SIZE, CONSUMED, PRODUCED }
#include "ax.def"
#undef DEFOP
};


/* Disassemble the expression EXPR, writing to F.  */
void
ax_print (struct ui_file *f, struct agent_expr *x)
{
  int i;

  fprintf_filtered (f, _("Scope: %s\n"), paddress (x->gdbarch, x->scope));
  fprintf_filtered (f, _("Reg mask:"));
  for (i = 0; i < x->reg_mask_len; ++i)
    fprintf_filtered (f, _(" %02x"), x->reg_mask[i]);
  fprintf_filtered (f, _("\n"));

  /* Check the size of the name array against the number of entries in
     the enum, to catch additions that people didn't sync.  */
  if ((sizeof (aop_map) / sizeof (aop_map[0]))
      != aop_last)
    error (_("GDB bug: ax-general.c (ax_print): opcode map out of sync"));

  for (i = 0; i < x->len;)
    {
      enum agent_op op = x->buf[i];

      if (op >= (sizeof (aop_map) / sizeof (aop_map[0]))
	  || !aop_map[op].name)
	{
	  fprintf_filtered (f, _("%3d  <bad opcode %02x>\n"), i, op);
	  i++;
	  continue;
	}
      if (i + 1 + aop_map[op].op_size > x->len)
	{
	  fprintf_filtered (f, _("%3d  <incomplete opcode %s>\n"),
			    i, aop_map[op].name);
	  break;
	}

      fprintf_filtered (f, "%3d  %s", i, aop_map[op].name);
      if (aop_map[op].op_size > 0)
	{
	  fputs_filtered (" ", f);

	  print_longest (f, 'd', 0,
			 read_const (x, i + 1, aop_map[op].op_size));
	}
      /* Handle the complicated printf arguments specially.  */
      else if (op == aop_printf)
	{
	  int slen, nargs;

	  i++;
	  nargs = x->buf[i++];
	  slen = x->buf[i++];
	  slen = slen * 256 + x->buf[i++];
	  fprintf_filtered (f, _(" \"%s\", %d args"),
			    &(x->buf[i]), nargs);
	  i += slen - 1;
	}
      fprintf_filtered (f, "\n");
      i += 1 + aop_map[op].op_size;
    }
}

/* Add register REG to the register mask for expression AX.  */
void
ax_reg_mask (struct agent_expr *ax, int reg)
{
  if (reg >= gdbarch_num_regs (ax->gdbarch))
    {
      /* This is a pseudo-register.  */
      if (!gdbarch_ax_pseudo_register_collect_p (ax->gdbarch))
	error (_("'%s' is a pseudo-register; "
		 "GDB cannot yet trace its contents."),
	       user_reg_map_regnum_to_name (ax->gdbarch, reg));
      if (gdbarch_ax_pseudo_register_collect (ax->gdbarch, ax, reg))
	error (_("Trace '%s' failed."),
	       user_reg_map_regnum_to_name (ax->gdbarch, reg));
    }
  else
    {
      int byte = reg / 8;

      /* Grow the bit mask if necessary.  */
      if (byte >= ax->reg_mask_len)
        {
          /* It's not appropriate to double here.  This isn't a
	     string buffer.  */
          int new_len = byte + 1;
          unsigned char *new_reg_mask = xrealloc (ax->reg_mask,
					          new_len
					          * sizeof (ax->reg_mask[0]));
          memset (new_reg_mask + ax->reg_mask_len, 0,
	          (new_len - ax->reg_mask_len) * sizeof (ax->reg_mask[0]));
          ax->reg_mask_len = new_len;
          ax->reg_mask = new_reg_mask;
        }

      ax->reg_mask[byte] |= 1 << (reg % 8);
    }
}

/* Given an agent expression AX, fill in requirements and other descriptive
   bits.  */
void
ax_reqs (struct agent_expr *ax)
{
  int i;
  int height;

  /* Jump target table.  targets[i] is non-zero iff we have found a
     jump to offset i.  */
  char *targets = (char *) alloca (ax->len * sizeof (targets[0]));

  /* Instruction boundary table.  boundary[i] is non-zero iff our scan
     has reached an instruction starting at offset i.  */
  char *boundary = (char *) alloca (ax->len * sizeof (boundary[0]));

  /* Stack height record.  If either targets[i] or boundary[i] is
     non-zero, heights[i] is the height the stack should have before
     executing the bytecode at that point.  */
  int *heights = (int *) alloca (ax->len * sizeof (heights[0]));

  /* Pointer to a description of the present op.  */
  struct aop_map *op;

  memset (targets, 0, ax->len * sizeof (targets[0]));
  memset (boundary, 0, ax->len * sizeof (boundary[0]));

  ax->max_height = ax->min_height = height = 0;
  ax->flaw = agent_flaw_none;
  ax->max_data_size = 0;

  for (i = 0; i < ax->len; i += 1 + op->op_size)
    {
      if (ax->buf[i] > (sizeof (aop_map) / sizeof (aop_map[0])))
	{
	  ax->flaw = agent_flaw_bad_instruction;
	  return;
	}

      op = &aop_map[ax->buf[i]];

      if (!op->name)
	{
	  ax->flaw = agent_flaw_bad_instruction;
	  return;
	}

      if (i + 1 + op->op_size > ax->len)
	{
	  ax->flaw = agent_flaw_incomplete_instruction;
	  return;
	}

      /* If this instruction is a forward jump target, does the
         current stack height match the stack height at the jump
         source?  */
      if (targets[i] && (heights[i] != height))
	{
	  ax->flaw = agent_flaw_height_mismatch;
	  return;
	}

      boundary[i] = 1;
      heights[i] = height;

      height -= op->consumed;
      if (height < ax->min_height)
	ax->min_height = height;
      height += op->produced;
      if (height > ax->max_height)
	ax->max_height = height;

      if (op->data_size > ax->max_data_size)
	ax->max_data_size = op->data_size;

      /* For jump instructions, check that the target is a valid
         offset.  If it is, record the fact that that location is a
         jump target, and record the height we expect there.  */
      if (aop_goto == op - aop_map
	  || aop_if_goto == op - aop_map)
	{
	  int target = read_const (ax, i + 1, 2);
	  if (target < 0 || target >= ax->len)
	    {
	      ax->flaw = agent_flaw_bad_jump;
	      return;
	    }

	  /* Do we have any information about what the stack height
             should be at the target?  */
	  if (targets[target] || boundary[target])
	    {
	      if (heights[target] != height)
		{
		  ax->flaw = agent_flaw_height_mismatch;
		  return;
		}
	    }

          /* Record the target, along with the stack height we expect.  */
          targets[target] = 1;
          heights[target] = height;
	}

      /* For unconditional jumps with a successor, check that the
         successor is a target, and pick up its stack height.  */
      if (aop_goto == op - aop_map
	  && i + 3 < ax->len)
	{
	  if (!targets[i + 3])
	    {
	      ax->flaw = agent_flaw_hole;
	      return;
	    }

	  height = heights[i + 3];
	}

      /* For reg instructions, record the register in the bit mask.  */
      if (aop_reg == op - aop_map)
	{
	  int reg = read_const (ax, i + 1, 2);

	  ax_reg_mask (ax, reg);
	}
    }

  /* Check that all the targets are on boundaries.  */
  for (i = 0; i < ax->len; i++)
    if (targets[i] && !boundary[i])
      {
	ax->flaw = agent_flaw_bad_jump;
	return;
      }

  ax->final_height = height;
}
