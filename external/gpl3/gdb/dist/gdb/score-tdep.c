/* Target-dependent code for the S+core architecture, for GDB,
   the GNU Debugger.

   Copyright (C) 2006-2013 Free Software Foundation, Inc.

   Contributed by Qinwei (qinwei@sunnorth.com.cn)
   Contributed by Ching-Peng Lin (cplin@sunplus.com)

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
#include "gdb_assert.h"
#include "inferior.h"
#include "symtab.h"
#include "objfiles.h"
#include "gdbcore.h"
#include "target.h"
#include "arch-utils.h"
#include "regcache.h"
#include "regset.h"
#include "dis-asm.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "trad-frame.h"
#include "dwarf2-frame.h"
#include "score-tdep.h"

#define G_FLD(_i,_ms,_ls) \
    ((unsigned)((_i) << (31 - (_ms))) >> (31 - (_ms) + (_ls)))

typedef struct{
  unsigned long long v;
  unsigned long long raw;
  unsigned int len;
}inst_t;

struct score_frame_cache
{
  CORE_ADDR base;
  CORE_ADDR fp;
  struct trad_frame_saved_reg *saved_regs;
};

static int target_mach = bfd_mach_score7;

static struct type *
score_register_type (struct gdbarch *gdbarch, int regnum)
{
  gdb_assert (regnum >= 0 
              && regnum < ((target_mach == bfd_mach_score7)
			   ? SCORE7_NUM_REGS : SCORE3_NUM_REGS));
  return builtin_type (gdbarch)->builtin_uint32;
}

static CORE_ADDR
score_unwind_sp (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  return frame_unwind_register_unsigned (next_frame, SCORE_SP_REGNUM);
}

static CORE_ADDR
score_unwind_pc (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  return frame_unwind_register_unsigned (next_frame, SCORE_PC_REGNUM);
}

static const char *
score7_register_name (struct gdbarch *gdbarch, int regnum)
{
  const char *score_register_names[] = {
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",

    "PSR",     "COND",  "ECR",     "EXCPVEC", "CCR",
    "EPC",     "EMA",   "TLBLOCK", "TLBPT",   "PEADDR",
    "TLBRPT",  "PEVN",  "PECTX",   "LIMPFN",  "LDMPFN", 
    "PREV",    "DREG",  "PC",      "DSAVE",   "COUNTER",
    "LDCR",    "STCR",  "CEH",     "CEL",
  };

  gdb_assert (regnum >= 0 && regnum < SCORE7_NUM_REGS);
  return score_register_names[regnum];
}

static const char *
score3_register_name (struct gdbarch *gdbarch, int regnum)
{
  const char *score_register_names[] = {
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",

    "PSR",      "COND",   "ECR",   "EXCPVEC",  "CCR",
    "EPC",      "EMA",    "PREV",  "DREG",     "DSAVE",
    "COUNTER",  "LDCR",   "STCR",  "CEH",      "CEL",
    "",         "",       "PC",
  };

  gdb_assert (regnum >= 0 && regnum < SCORE3_NUM_REGS);
  return score_register_names[regnum];
}

#if WITH_SIM
static int
score_register_sim_regno (struct gdbarch *gdbarch, int regnum)
{
  gdb_assert (regnum >= 0 
              && regnum < ((target_mach == bfd_mach_score7)
			   ? SCORE7_NUM_REGS : SCORE3_NUM_REGS));
  return regnum;
}
#endif

static int
score_print_insn (bfd_vma memaddr, struct disassemble_info *info)
{
  if (info->endian == BFD_ENDIAN_BIG)
    return print_insn_big_score (memaddr, info);
  else
    return print_insn_little_score (memaddr, info);
}

static inst_t *
score7_fetch_inst (struct gdbarch *gdbarch, CORE_ADDR addr, gdb_byte *memblock)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  static inst_t inst = { 0, 0, 0 };
  gdb_byte buf[SCORE_INSTLEN] = { 0 };
  int big;
  int ret;

  if (target_has_execution && memblock != NULL)
    {
      /* Fetch instruction from local MEMBLOCK.  */
      memcpy (buf, memblock, SCORE_INSTLEN);
    }
  else
    {
      /* Fetch instruction from target.  */
      ret = target_read_memory (addr & ~0x3, buf, SCORE_INSTLEN);
      if (ret)
        {
          error (_("Error: target_read_memory in file:%s, line:%d!"),
                  __FILE__, __LINE__);
          return 0;
        }
    }

  inst.raw = extract_unsigned_integer (buf, SCORE_INSTLEN, byte_order);
  inst.len = (inst.raw & 0x80008000) ? 4 : 2;
  inst.v = ((inst.raw >> 16 & 0x7FFF) << 15) | (inst.raw & 0x7FFF); 
  big = (byte_order == BFD_ENDIAN_BIG);
  if (inst.len == 2)
    {
      if (big ^ ((addr & 0x2) == 2))
        inst.v = G_FLD (inst.v, 29, 15);
      else
        inst.v = G_FLD (inst.v, 14, 0);
    }
  return &inst;
}

static inst_t *
score3_adjust_pc_and_fetch_inst (CORE_ADDR *pcptr, int *lenptr,
				 enum bfd_endian byte_order)
{
  static inst_t inst = { 0, 0, 0 };

  struct breakplace
  {
    int break_offset;
    int inst_len;
  };
  /*     raw        table 1 (column 2, 3, 4)
    *  0  1  0  *   # 2
    *  0  1  1  0   # 3
    0  1  1  0  *   # 6
                    table 2 (column 1, 2, 3)
    *  0  0  *  *   # 0, 4
    0  1  0  *  *   # 2
    1  1  0  *  *   # 6
   */

  static const struct breakplace bk_table[16] =
    {
      /* table 1 */
      {0, 0},
      {0, 0},
      {0, 4},
      {0, 6},
      {0, 0},
      {0, 0},
      {-2, 6},
      {0, 0},
      /* table 2 */
      {0, 2},
      {0, 0},
      {-2, 4},
      {0, 0},
      {0, 2},
      {0, 0},
      {-4, 6},
      {0, 0}
    };

#define EXTRACT_LEN 2
  CORE_ADDR adjust_pc = *pcptr & ~0x1;
  gdb_byte buf[5][EXTRACT_LEN] =
    {
      {'\0', '\0'},
      {'\0', '\0'},
      {'\0', '\0'},
      {'\0', '\0'},
      {'\0', '\0'}
    };
  int ret;
  unsigned int raw;
  unsigned int cbits = 0;
  int bk_index;
  int i, count;

  inst.v = 0;
  inst.raw = 0;
  inst.len = 0;

  adjust_pc -= 4;
  for (i = 0; i < 5; i++)
    {
      ret = target_read_memory (adjust_pc + 2 * i, buf[i], EXTRACT_LEN);
      if (ret != 0)
        {
          buf[i][0] = '\0';
          buf[i][1] = '\0';
	  if (i == 2)
            error (_("Error: target_read_memory in file:%s, line:%d!"),
		   __FILE__, __LINE__);
        }

      raw = extract_unsigned_integer (buf[i], EXTRACT_LEN, byte_order);
      cbits = (cbits << 1) | (raw >> 15); 
    }
  adjust_pc += 4;

  if (cbits & 0x4)
    {
      /* table 1 */
      cbits = (cbits >> 1) & 0x7;
      bk_index = cbits;
    }
  else
    {
      /* table 2 */
      cbits = (cbits >> 2) & 0x7;
      bk_index = cbits + 8; 
    }

  gdb_assert (!((bk_table[bk_index].break_offset == 0)
		&& (bk_table[bk_index].inst_len == 0)));

  inst.len = bk_table[bk_index].inst_len;

  i = (bk_table[bk_index].break_offset + 4) / 2;
  count = inst.len / 2;
  for (; count > 0; i++, count--)
    {
      inst.raw = (inst.raw << 16)
	         | extract_unsigned_integer (buf[i], EXTRACT_LEN, byte_order);
    }

  switch (inst.len)
    {
    case 2:
      inst.v = inst.raw & 0x7FFF;
      break;
    case 4:
      inst.v = ((inst.raw >> 16 & 0x7FFF) << 15) | (inst.raw & 0x7FFF);
      break;
    case 6:
      inst.v = ((inst.raw >> 32 & 0x7FFF) << 30)
	       | ((inst.raw >> 16 & 0x7FFF) << 15) | (inst.raw & 0x7FFF);
      break;
    }

  if (pcptr)
    *pcptr = adjust_pc + bk_table[bk_index].break_offset;
  if (lenptr)
    *lenptr = bk_table[bk_index].inst_len;

#undef EXTRACT_LEN

  return &inst;
}

static const gdb_byte *
score7_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr,
			   int *lenptr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[SCORE_INSTLEN] = { 0 };
  int ret;
  unsigned int raw;

  if ((ret = target_read_memory (*pcptr & ~0x3, buf, SCORE_INSTLEN)) != 0)
    {
      error (_("Error: target_read_memory in file:%s, line:%d!"),
             __FILE__, __LINE__);
    }
  raw = extract_unsigned_integer (buf, SCORE_INSTLEN, byte_order);

  if (byte_order == BFD_ENDIAN_BIG)
    {
      if (!(raw & 0x80008000))
        {
          /* 16bits instruction.  */
          static gdb_byte big_breakpoint16[] = { 0x60, 0x02 };
          *pcptr &= ~0x1;
          *lenptr = sizeof (big_breakpoint16);
          return big_breakpoint16;
        }
      else
        {
          /* 32bits instruction.  */
          static gdb_byte big_breakpoint32[] = { 0x80, 0x00, 0x80, 0x06 };
          *pcptr &= ~0x3;
          *lenptr = sizeof (big_breakpoint32);
          return big_breakpoint32;
        }
    }
  else
    {
      if (!(raw & 0x80008000))
        {
          /* 16bits instruction.  */
          static gdb_byte little_breakpoint16[] = { 0x02, 0x60 };
          *pcptr &= ~0x1;
          *lenptr = sizeof (little_breakpoint16);
          return little_breakpoint16;
        }
      else
        {
          /* 32bits instruction.  */
          static gdb_byte little_breakpoint32[] = { 0x06, 0x80, 0x00, 0x80 };
          *pcptr &= ~0x3;
          *lenptr = sizeof (little_breakpoint32);
          return little_breakpoint32;
        }
    }
}

static const gdb_byte *
score3_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr,
			   int *lenptr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR adjust_pc = *pcptr; 
  int len;
  static gdb_byte score_break_insns[6][6] = {
    /* The following three instructions are big endian.  */
    { 0x00, 0x20 },
    { 0x80, 0x00, 0x00, 0x06 },
    { 0x80, 0x00, 0x80, 0x00, 0x00, 0x00 },
    /* The following three instructions are little endian.  */
    { 0x20, 0x00 },
    { 0x00, 0x80, 0x06, 0x00 },
    { 0x00, 0x80, 0x00, 0x80, 0x00, 0x00 }};

  gdb_byte *p = NULL;
  int index = 0;

  score3_adjust_pc_and_fetch_inst (&adjust_pc, &len, byte_order);

  index = ((byte_order == BFD_ENDIAN_BIG) ? 0 : 3) + (len / 2 - 1);
  p = score_break_insns[index];

  *pcptr = adjust_pc;
  *lenptr = len;

  return p;
}

static CORE_ADDR
score_adjust_breakpoint_address (struct gdbarch *gdbarch, CORE_ADDR bpaddr)
{
  CORE_ADDR adjust_pc = bpaddr; 

  if (target_mach == bfd_mach_score3)
    score3_adjust_pc_and_fetch_inst (&adjust_pc, NULL,
		    		     gdbarch_byte_order (gdbarch));
  else
    adjust_pc = align_down (adjust_pc, 2);
  
  return adjust_pc;
}

static CORE_ADDR
score_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return align_down (addr, 16);
}

static void
score_xfer_register (struct regcache *regcache, int regnum, int length,
                     enum bfd_endian endian, gdb_byte *readbuf,
                     const gdb_byte *writebuf, int buf_offset)
{
  int reg_offset = 0;
  gdb_assert (regnum >= 0 
              && regnum < ((target_mach == bfd_mach_score7)
			   ? SCORE7_NUM_REGS : SCORE3_NUM_REGS));

  switch (endian)
    {
    case BFD_ENDIAN_BIG:
      reg_offset = SCORE_REGSIZE - length;
      break;
    case BFD_ENDIAN_LITTLE:
      reg_offset = 0;
      break;
    case BFD_ENDIAN_UNKNOWN:
      reg_offset = 0;
      break;
    default:
      error (_("Error: score_xfer_register in file:%s, line:%d!"),
             __FILE__, __LINE__);
    }

  if (readbuf != NULL)
    regcache_cooked_read_part (regcache, regnum, reg_offset, length,
                               readbuf + buf_offset);
  if (writebuf != NULL)
    regcache_cooked_write_part (regcache, regnum, reg_offset, length,
                                writebuf + buf_offset);
}

static enum return_value_convention
score_return_value (struct gdbarch *gdbarch, struct value *function,
                    struct type *type, struct regcache *regcache,
                    gdb_byte * readbuf, const gdb_byte * writebuf)
{
  if (TYPE_CODE (type) == TYPE_CODE_STRUCT
      || TYPE_CODE (type) == TYPE_CODE_UNION
      || TYPE_CODE (type) == TYPE_CODE_ARRAY)
    return RETURN_VALUE_STRUCT_CONVENTION;
  else
    {
      int offset;
      int regnum;
      for (offset = 0, regnum = SCORE_A0_REGNUM;
           offset < TYPE_LENGTH (type);
           offset += SCORE_REGSIZE, regnum++)
        {
          int xfer = SCORE_REGSIZE;

          if (offset + xfer > TYPE_LENGTH (type))
            xfer = TYPE_LENGTH (type) - offset;
          score_xfer_register (regcache, regnum, xfer,
			       gdbarch_byte_order(gdbarch),
                               readbuf, writebuf, offset);
        }
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

static struct frame_id
score_dummy_id (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  return frame_id_build (get_frame_register_unsigned (this_frame,
						      SCORE_SP_REGNUM),
			 get_frame_pc (this_frame));
}

static int
score_type_needs_double_align (struct type *type)
{
  enum type_code typecode = TYPE_CODE (type);

  if ((typecode == TYPE_CODE_INT && TYPE_LENGTH (type) == 8)
      || (typecode == TYPE_CODE_FLT && TYPE_LENGTH (type) == 8))
    return 1;
  else if (typecode == TYPE_CODE_STRUCT || typecode == TYPE_CODE_UNION)
    {
      int i, n;

      n = TYPE_NFIELDS (type);
      for (i = 0; i < n; i++)
        if (score_type_needs_double_align (TYPE_FIELD_TYPE (type, i)))
          return 1;
      return 0;
    }
  return 0;
}

static CORE_ADDR
score_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
                       struct regcache *regcache, CORE_ADDR bp_addr,
                       int nargs, struct value **args, CORE_ADDR sp,
                       int struct_return, CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int argnum;
  int argreg;
  int arglen = 0;
  CORE_ADDR stack_offset = 0;
  CORE_ADDR addr = 0;

  /* Step 1, Save RA.  */
  regcache_cooked_write_unsigned (regcache, SCORE_RA_REGNUM, bp_addr);

  /* Step 2, Make space on the stack for the args.  */
  struct_addr = align_down (struct_addr, 16);
  sp = align_down (sp, 16);
  for (argnum = 0; argnum < nargs; argnum++)
    arglen += align_up (TYPE_LENGTH (value_type (args[argnum])),
                        SCORE_REGSIZE);
  sp -= align_up (arglen, 16);

  argreg = SCORE_BEGIN_ARG_REGNUM;

  /* Step 3, Check if struct return then save the struct address to
     r4 and increase the stack_offset by 4.  */
  if (struct_return)
    {
      regcache_cooked_write_unsigned (regcache, argreg++, struct_addr);
      stack_offset += SCORE_REGSIZE;
    }

  /* Step 4, Load arguments:
     If arg length is too long (> 4 bytes), then split the arg and
     save every parts.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (value_type (arg));
      enum type_code typecode = TYPE_CODE (arg_type);
      const gdb_byte *val = value_contents (arg);
      int downward_offset = 0;
      int odd_sized_struct_p;
      int arg_last_part_p = 0;

      arglen = TYPE_LENGTH (arg_type);
      odd_sized_struct_p = (arglen > SCORE_REGSIZE
                            && arglen % SCORE_REGSIZE != 0);

      /* If a arg should be aligned to 8 bytes (long long or double),
         the value should be put to even register numbers.  */
      if (score_type_needs_double_align (arg_type))
        {
          if (argreg & 1)
            argreg++;
        }

      /* If sizeof a block < SCORE_REGSIZE, then Score GCC will chose
         the default "downward"/"upward" method:

         Example:

         struct struc
         {
           char a; char b; char c;
         } s = {'a', 'b', 'c'};

         Big endian:    s = {X, 'a', 'b', 'c'}
         Little endian: s = {'a', 'b', 'c', X}

         Where X is a hole.  */

      if (gdbarch_byte_order(gdbarch) == BFD_ENDIAN_BIG
          && (typecode == TYPE_CODE_STRUCT
              || typecode == TYPE_CODE_UNION)
          && argreg > SCORE_LAST_ARG_REGNUM
          && arglen < SCORE_REGSIZE)
        downward_offset += (SCORE_REGSIZE - arglen);

      while (arglen > 0)
        {
          int partial_len = arglen < SCORE_REGSIZE ? arglen : SCORE_REGSIZE;
          ULONGEST regval = extract_unsigned_integer (val, partial_len,
			  			      byte_order);

          /* The last part of a arg should shift left when
             gdbarch_byte_order is BFD_ENDIAN_BIG.  */
          if (byte_order == BFD_ENDIAN_BIG
              && arg_last_part_p == 1
              && (typecode == TYPE_CODE_STRUCT
                  || typecode == TYPE_CODE_UNION))
            regval <<= ((SCORE_REGSIZE - partial_len) * TARGET_CHAR_BIT);

          /* Always increase the stack_offset and save args to stack.  */
          addr = sp + stack_offset + downward_offset;
          write_memory (addr, val, partial_len);

          if (argreg <= SCORE_LAST_ARG_REGNUM)
            {
              regcache_cooked_write_unsigned (regcache, argreg++, regval);
              if (arglen > SCORE_REGSIZE && arglen < SCORE_REGSIZE * 2)
                arg_last_part_p = 1;
            }

          val += partial_len;
          arglen -= partial_len;
          stack_offset += align_up (partial_len, SCORE_REGSIZE);
        }
    }

  /* Step 5, Save SP.  */
  regcache_cooked_write_unsigned (regcache, SCORE_SP_REGNUM, sp);

  return sp;
}

static CORE_ADDR
score7_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR cpc = pc;
  int iscan = 32, stack_sub = 0;
  while (iscan-- > 0)
    {
      inst_t *inst = score7_fetch_inst (gdbarch, cpc, NULL);
      if (!inst)
        break;
      if ((inst->len == 4) && !stack_sub
          && (G_FLD (inst->v, 29, 25) == 0x1
              && G_FLD (inst->v, 24, 20) == 0x0))
        {
          /* addi r0, offset */
          stack_sub = cpc + SCORE_INSTLEN;
          pc = cpc + SCORE_INSTLEN;
        }
      else if ((inst->len == 4)
               && (G_FLD (inst->v, 29, 25) == 0x0)
               && (G_FLD (inst->v, 24, 20) == 0x2)
               && (G_FLD (inst->v, 19, 15) == 0x0)
               && (G_FLD (inst->v, 14, 10) == 0xF)
               && (G_FLD (inst->v, 9, 0) == 0x56))
        {
          /* mv r2, r0  */
          pc = cpc + SCORE_INSTLEN;
          break;
        }
      else if ((inst->len == 2)
               && (G_FLD (inst->v, 14, 12) == 0x0)
               && (G_FLD (inst->v, 11, 8) == 0x2)
               && (G_FLD (inst->v, 7, 4) == 0x0)
               && (G_FLD (inst->v, 3, 0) == 0x3))
        {
          /* mv! r2, r0 */
          pc = cpc + SCORE16_INSTLEN;
          break;
        }
      else if ((inst->len == 2)
               && ((G_FLD (inst->v, 14, 12) == 3)    /* j15 form */
                   || (G_FLD (inst->v, 14, 12) == 4) /* b15 form */
                   || (G_FLD (inst->v, 14, 12) == 0x0
                       && G_FLD (inst->v, 3, 0) == 0x4))) /* br! */
        break;
      else if ((inst->len == 4)
               && ((G_FLD (inst->v, 29, 25) == 2)    /* j32 form */
                   || (G_FLD (inst->v, 29, 25) == 4) /* b32 form */
                   || (G_FLD (inst->v, 29, 25) == 0x0
                       && G_FLD (inst->v, 6, 1) == 0x4)))  /* br */
        break;

      cpc += (inst->len == 2) ? SCORE16_INSTLEN : SCORE_INSTLEN;
    }
  return pc;
}

static CORE_ADDR
score3_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR cpc = pc;
  int iscan = 32, stack_sub = 0;
  while (iscan-- > 0)
    {
      inst_t *inst
	= score3_adjust_pc_and_fetch_inst (&cpc, NULL,
					   gdbarch_byte_order (gdbarch));

      if (!inst)
        break;
      if (inst->len == 4 && !stack_sub
          && (G_FLD (inst->v, 29, 25) == 0x1)
          && (G_FLD (inst->v, 19, 17) == 0x0)
	  && (G_FLD (inst->v, 24, 20) == 0x0))
        {
          /* addi r0, offset */
          stack_sub = cpc + inst->len;
          pc = cpc + inst->len;
        }
      else if (inst->len == 4
               && (G_FLD (inst->v, 29, 25) == 0x0)
	       && (G_FLD (inst->v, 24, 20) == 0x2)
	       && (G_FLD (inst->v, 19, 15) == 0x0)
	       && (G_FLD (inst->v, 14, 10) == 0xF)
	       && (G_FLD (inst->v, 9, 0) == 0x56))
        {
          /* mv r2, r0  */
          pc = cpc + inst->len;
          break;
        }
      else if ((inst->len == 2)
               && (G_FLD (inst->v, 14, 10) == 0x10)
               && (G_FLD (inst->v, 9, 5) == 0x2)
	       && (G_FLD (inst->v, 4, 0) == 0x0))
        {
          /* mv! r2, r0 */
          pc = cpc + inst->len;
          break;
        }
      else if (inst->len == 2
               && ((G_FLD (inst->v, 14, 12) == 3) /* b15 form */
                   || (G_FLD (inst->v, 14, 12) == 0x0
                       && G_FLD (inst->v, 11, 5) == 0x4))) /* br! */
        break;
      else if (inst->len == 4
               && ((G_FLD (inst->v, 29, 25) == 2)    /* j32 form */
                   || (G_FLD (inst->v, 29, 25) == 4))) /* b32 form */
        break;

      cpc += inst->len;
    }
  return pc;
}

static int
score7_in_function_epilogue_p (struct gdbarch *gdbarch, CORE_ADDR cur_pc)
{
  inst_t *inst = score7_fetch_inst (gdbarch, cur_pc, NULL);

  if (inst->v == 0x23)
    return 1;   /* mv! r0, r2 */
  else if (G_FLD (inst->v, 14, 12) == 0x2
           && G_FLD (inst->v, 3, 0) == 0xa)
    return 1;   /* pop! */
  else if (G_FLD (inst->v, 14, 12) == 0x0
           && G_FLD (inst->v, 7, 0) == 0x34)
    return 1;   /* br! r3 */
  else if (G_FLD (inst->v, 29, 15) == 0x2
           && G_FLD (inst->v, 6, 1) == 0x2b)
    return 1;   /* mv r0, r2 */
  else if (G_FLD (inst->v, 29, 25) == 0x0
           && G_FLD (inst->v, 6, 1) == 0x4
           && G_FLD (inst->v, 19, 15) == 0x3)
    return 1;   /* br r3 */
  else
    return 0;
}

static int
score3_in_function_epilogue_p (struct gdbarch *gdbarch, CORE_ADDR cur_pc)
{
  CORE_ADDR pc = cur_pc;
  inst_t *inst
    = score3_adjust_pc_and_fetch_inst (&pc, NULL,
				       gdbarch_byte_order (gdbarch));

  if (inst->len == 2
      && (G_FLD (inst->v, 14, 10) == 0x10)
      && (G_FLD (inst->v, 9, 5) == 0x0)
      && (G_FLD (inst->v, 4, 0) == 0x2))
    return 1;   /* mv! r0, r2 */
  else if (inst->len == 4
           && (G_FLD (inst->v, 29, 25) == 0x0)
           && (G_FLD (inst->v, 24, 20) == 0x2)
           && (G_FLD (inst->v, 19, 15) == 0x0)
	   && (G_FLD (inst->v, 14, 10) == 0xF)
	   && (G_FLD (inst->v, 9, 0) == 0x56))
    return 1;   /* mv r0, r2 */
  else if (inst->len == 2
           && (G_FLD (inst->v, 14, 12) == 0x0)
           && (G_FLD (inst->v, 11, 5) == 0x2))
    return 1;   /* pop! */
  else if (inst->len == 2
           && (G_FLD (inst->v, 14, 12) == 0x0)
           && (G_FLD (inst->v, 11, 7) == 0x0)
           && (G_FLD (inst->v, 6, 5) == 0x2))
    return 1;   /* rpop! */
  else if (inst->len == 2
           && (G_FLD (inst->v, 14, 12) == 0x0)
           && (G_FLD (inst->v, 11, 5) == 0x4)
           && (G_FLD (inst->v, 4, 0) == 0x3))
    return 1;   /* br! r3 */
  else if (inst->len == 4
           && (G_FLD (inst->v, 29, 25) == 0x0)
           && (G_FLD (inst->v, 24, 20) == 0x0)
           && (G_FLD (inst->v, 19, 15) == 0x3)
           && (G_FLD (inst->v, 14, 10) == 0xF)
           && (G_FLD (inst->v, 9, 0) == 0x8))
    return 1;   /* br r3 */
  else
    return 0;
}

static gdb_byte *
score7_malloc_and_get_memblock (CORE_ADDR addr, CORE_ADDR size)
{
  int ret;
  gdb_byte *memblock = NULL;

  if (size < 0)
    {
      error (_("Error: malloc size < 0 in file:%s, line:%d!"),
             __FILE__, __LINE__);
      return NULL;
    }
  else if (size == 0)
    return NULL;

  memblock = xmalloc (size);
  memset (memblock, 0, size);
  ret = target_read_memory (addr & ~0x3, memblock, size);
  if (ret)
    {
      error (_("Error: target_read_memory in file:%s, line:%d!"),
             __FILE__, __LINE__);
      return NULL;
    }
  return memblock;
}

static void
score7_free_memblock (char *memblock)
{
  xfree (memblock);
}

static void
score7_adjust_memblock_ptr (gdb_byte **memblock, CORE_ADDR prev_pc,
                           CORE_ADDR cur_pc)
{
  if (prev_pc == -1)
    {
      /* First time call this function, do nothing.  */
    }
  else if (cur_pc - prev_pc == 2 && (cur_pc & 0x3) == 0)
    {
      /* First 16-bit instruction, then 32-bit instruction.  */
      *memblock += SCORE_INSTLEN;
    }
  else if (cur_pc - prev_pc == 4)
    {
      /* Is 32-bit instruction, increase MEMBLOCK by 4.  */
      *memblock += SCORE_INSTLEN;
    }
}

static void
score7_analyze_prologue (CORE_ADDR startaddr, CORE_ADDR pc,
                        struct frame_info *this_frame,
                        struct score_frame_cache *this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR sp;
  CORE_ADDR fp;
  CORE_ADDR cur_pc = startaddr;

  int sp_offset = 0;
  int ra_offset = 0;
  int fp_offset = 0;
  int ra_offset_p = 0;
  int fp_offset_p = 0;
  int inst_len = 0;

  gdb_byte *memblock = NULL;
  gdb_byte *memblock_ptr = NULL;
  CORE_ADDR prev_pc = -1;

  /* Allocate MEMBLOCK if PC - STARTADDR > 0.  */
  memblock_ptr = memblock =
    score7_malloc_and_get_memblock (startaddr, pc - startaddr);

  sp = get_frame_register_unsigned (this_frame, SCORE_SP_REGNUM);
  fp = get_frame_register_unsigned (this_frame, SCORE_FP_REGNUM);

  for (; cur_pc < pc; prev_pc = cur_pc, cur_pc += inst_len)
    {
      inst_t *inst = NULL;
      if (memblock != NULL)
        {
          /* Reading memory block from target succefully and got all
             the instructions(from STARTADDR to PC) needed.  */
          score7_adjust_memblock_ptr (&memblock, prev_pc, cur_pc);
          inst = score7_fetch_inst (gdbarch, cur_pc, memblock);
        }
      else
        {
          /* Otherwise, we fetch 4 bytes from target, and GDB also
             work correctly.  */
          inst = score7_fetch_inst (gdbarch, cur_pc, NULL);
        }

      /* FIXME: make a full-power prologue analyzer.  */
      if (inst->len == 2)
        {
          inst_len = SCORE16_INSTLEN;

          if (G_FLD (inst->v, 14, 12) == 0x2
              && G_FLD (inst->v, 3, 0) == 0xe)
            {
              /* push! */
              sp_offset += 4;

              if (G_FLD (inst->v, 11, 7) == 0x6
                  && ra_offset_p == 0)
                {
                  /* push! r3, [r0] */
                  ra_offset = sp_offset;
                  ra_offset_p = 1;
                }
              else if (G_FLD (inst->v, 11, 7) == 0x4
                       && fp_offset_p == 0)
                {
                  /* push! r2, [r0] */
                  fp_offset = sp_offset;
                  fp_offset_p = 1;
                }
            }
          else if (G_FLD (inst->v, 14, 12) == 0x2
                   && G_FLD (inst->v, 3, 0) == 0xa)
            {
              /* pop! */
              sp_offset -= 4;
            }
          else if (G_FLD (inst->v, 14, 7) == 0xc1
                   && G_FLD (inst->v, 2, 0) == 0x0)
            {
              /* subei! r0, n */
              sp_offset += (int) pow (2, G_FLD (inst->v, 6, 3));
            }
          else if (G_FLD (inst->v, 14, 7) == 0xc0
                   && G_FLD (inst->v, 2, 0) == 0x0)
            {
              /* addei! r0, n */
              sp_offset -= (int) pow (2, G_FLD (inst->v, 6, 3));
            }
        }
      else
        {
          inst_len = SCORE_INSTLEN;

          if (G_FLD(inst->v, 29, 25) == 0x3
              && G_FLD(inst->v, 2, 0) == 0x4
              && G_FLD(inst->v, 19, 15) == 0)
            {
                /* sw rD, [r0, offset]+ */
                sp_offset += SCORE_INSTLEN;

                if (G_FLD(inst->v, 24, 20) == 0x3)
                  {
                      /* rD = r3 */
                      if (ra_offset_p == 0)
                        {
                            ra_offset = sp_offset;
                            ra_offset_p = 1;
                        }
                  }
                else if (G_FLD(inst->v, 24, 20) == 0x2)
                  {
                      /* rD = r2 */
                      if (fp_offset_p == 0)
                        {
                            fp_offset = sp_offset;
                            fp_offset_p = 1;
                        }
                  }
            }
          else if (G_FLD(inst->v, 29, 25) == 0x14
                   && G_FLD(inst->v, 19,15) == 0)
            {
                /* sw rD, [r0, offset] */
                if (G_FLD(inst->v, 24, 20) == 0x3)
                  {
                      /* rD = r3 */
                      ra_offset = sp_offset - G_FLD(inst->v, 14, 0);
                      ra_offset_p = 1;
                  }
                else if (G_FLD(inst->v, 24, 20) == 0x2)
                  {
                      /* rD = r2 */
                      fp_offset = sp_offset - G_FLD(inst->v, 14, 0);
                      fp_offset_p = 1;
                  }
            }
          else if (G_FLD (inst->v, 29, 15) == 0x1c60
                   && G_FLD (inst->v, 2, 0) == 0x0)
            {
              /* lw r3, [r0]+, 4 */
              sp_offset -= SCORE_INSTLEN;
              ra_offset_p = 1;
            }
          else if (G_FLD (inst->v, 29, 15) == 0x1c40
                   && G_FLD (inst->v, 2, 0) == 0x0)
            {
              /* lw r2, [r0]+, 4 */
              sp_offset -= SCORE_INSTLEN;
              fp_offset_p = 1;
            }

          else if (G_FLD (inst->v, 29, 17) == 0x100
                   && G_FLD (inst->v, 0, 0) == 0x0)
            {
              /* addi r0, -offset */
              sp_offset += 65536 - G_FLD (inst->v, 16, 1);
            }
          else if (G_FLD (inst->v, 29, 17) == 0x110
                   && G_FLD (inst->v, 0, 0) == 0x0)
            {
              /* addi r2, offset */
              if (pc - cur_pc > 4)
                {
                  unsigned int save_v = inst->v;
                  inst_t *inst2 =
                    score7_fetch_inst (gdbarch, cur_pc + SCORE_INSTLEN, NULL);
                  if (inst2->v == 0x23)
                    {
                      /* mv! r0, r2 */
                      sp_offset -= G_FLD (save_v, 16, 1);
                    }
                }
            }
        }
    }

  /* Save RA.  */
  if (ra_offset_p == 1)
    {
      if (this_cache->saved_regs[SCORE_PC_REGNUM].addr == -1)
        this_cache->saved_regs[SCORE_PC_REGNUM].addr =
          sp + sp_offset - ra_offset;
    }
  else
    {
      this_cache->saved_regs[SCORE_PC_REGNUM] =
        this_cache->saved_regs[SCORE_RA_REGNUM];
    }

  /* Save FP.  */
  if (fp_offset_p == 1)
    {
      if (this_cache->saved_regs[SCORE_FP_REGNUM].addr == -1)
        this_cache->saved_regs[SCORE_FP_REGNUM].addr =
          sp + sp_offset - fp_offset;
    }

  /* Save SP and FP.  */
  this_cache->base = sp + sp_offset;
  this_cache->fp = fp;

  /* Don't forget to free MEMBLOCK if we allocated it.  */
  if (memblock_ptr != NULL)
    score7_free_memblock (memblock_ptr);
}

static void
score3_analyze_prologue (CORE_ADDR startaddr, CORE_ADDR pc,
                        struct frame_info *this_frame,
                        struct score_frame_cache *this_cache)
{
  CORE_ADDR sp;
  CORE_ADDR fp;
  CORE_ADDR cur_pc = startaddr;
  enum bfd_endian byte_order
    = gdbarch_byte_order (get_frame_arch (this_frame));

  int sp_offset = 0;
  int ra_offset = 0;
  int fp_offset = 0;
  int ra_offset_p = 0;
  int fp_offset_p = 0;
  int inst_len = 0;

  CORE_ADDR prev_pc = -1;

  sp = get_frame_register_unsigned (this_frame, SCORE_SP_REGNUM);
  fp = get_frame_register_unsigned (this_frame, SCORE_FP_REGNUM);

  for (; cur_pc < pc; prev_pc = cur_pc, cur_pc += inst_len)
    {
      inst_t *inst = NULL;

      inst = score3_adjust_pc_and_fetch_inst (&cur_pc, &inst_len, byte_order);

      /* FIXME: make a full-power prologue analyzer.  */
      if (inst->len == 2)
        {
          if (G_FLD (inst->v, 14, 12) == 0x0
              && G_FLD (inst->v, 11, 7) == 0x0
              && G_FLD (inst->v, 6, 5) == 0x3)
            {
              /* push! */
              sp_offset += 4;

              if (G_FLD (inst->v, 4, 0) == 0x3
                  && ra_offset_p == 0)
                {
                  /* push! r3, [r0] */
                  ra_offset = sp_offset;
                  ra_offset_p = 1;
                }
              else if (G_FLD (inst->v, 4, 0) == 0x2
                       && fp_offset_p == 0)
                {
                  /* push! r2, [r0] */
                  fp_offset = sp_offset;
                  fp_offset_p = 1;
                }
            }
          else if (G_FLD (inst->v, 14, 12) == 0x6
                   && G_FLD (inst->v, 11, 10) == 0x3)
            {
              /* rpush! */
              int start_r = G_FLD (inst->v, 9, 5);
              int cnt = G_FLD (inst->v, 4, 0);
     
              if ((ra_offset_p == 0)
		  && (start_r <= SCORE_RA_REGNUM)
                  && (SCORE_RA_REGNUM < start_r + cnt))
                {
                  /* rpush! contains r3 */
                  ra_offset_p = 1;
                  ra_offset = sp_offset + 4 * (SCORE_RA_REGNUM - start_r) + 4;
                }

              if ((fp_offset_p == 0)
		  && (start_r <= SCORE_FP_REGNUM)
                  && (SCORE_FP_REGNUM < start_r + cnt))
                {
                  /* rpush! contains r2 */
                  fp_offset_p = 1;
                  fp_offset = sp_offset + 4 * (SCORE_FP_REGNUM - start_r) + 4;
                }

              sp_offset += 4 * cnt;
            }
          else if (G_FLD (inst->v, 14, 12) == 0x0
                   && G_FLD (inst->v, 11, 7) == 0x0
                   && G_FLD (inst->v, 6, 5) == 0x2)
            {
              /* pop! */
              sp_offset -= 4;
            }
          else if (G_FLD (inst->v, 14, 12) == 0x6
                   && G_FLD (inst->v, 11, 10) == 0x2)
            {
              /* rpop! */
              sp_offset -= 4 * G_FLD (inst->v, 4, 0);
            }
          else if (G_FLD (inst->v, 14, 12) == 0x5
                   && G_FLD (inst->v, 11, 10) == 0x3
                   && G_FLD (inst->v, 9, 6) == 0x0)
            {
              /* addi! r0, -offset */
              int imm = G_FLD (inst->v, 5, 0);
	      if (imm >> 5)
		imm = -(0x3F - imm + 1);
	      sp_offset -= imm;
            }
          else if (G_FLD (inst->v, 14, 12) == 0x5
                   && G_FLD (inst->v, 11, 10) == 0x3
                   && G_FLD (inst->v, 9, 6) == 0x2)
            {
              /* addi! r2, offset */
              if (pc - cur_pc >= 2)
                {
		  unsigned int save_v = inst->v;
		  inst_t *inst2;
		  
		  cur_pc += inst->len;
		  inst2 = score3_adjust_pc_and_fetch_inst (&cur_pc, NULL,
							   byte_order);

                  if (inst2->len == 2
                      && G_FLD (inst2->v, 14, 10) == 0x10
                      && G_FLD (inst2->v, 9, 5) == 0x0
                      && G_FLD (inst2->v, 4, 0) == 0x2)
                    {
                      /* mv! r0, r2 */
                      int imm = G_FLD (inst->v, 5, 0);
	              if (imm >> 5)
		        imm = -(0x3F - imm + 1);
                      sp_offset -= imm;
                    }
                }
	    }
        }
      else if (inst->len == 4)
        {
          if (G_FLD (inst->v, 29, 25) == 0x3
              && G_FLD (inst->v, 2, 0) == 0x4
              && G_FLD (inst->v, 24, 20) == 0x3
              && G_FLD (inst->v, 19, 15) == 0x0)
            {
              /* sw r3, [r0, offset]+ */
              sp_offset += inst->len;
              if (ra_offset_p == 0)
                {
                  ra_offset = sp_offset;
                  ra_offset_p = 1;
                }
            }
          else if (G_FLD (inst->v, 29, 25) == 0x3
                   && G_FLD (inst->v, 2, 0) == 0x4
                   && G_FLD (inst->v, 24, 20) == 0x2
                   && G_FLD (inst->v, 19, 15) == 0x0)
            {
              /* sw r2, [r0, offset]+ */
              sp_offset += inst->len;
              if (fp_offset_p == 0)
                {
                  fp_offset = sp_offset;
                  fp_offset_p = 1;
                }
            }
          else if (G_FLD (inst->v, 29, 25) == 0x7
                   && G_FLD (inst->v, 2, 0) == 0x0
                   && G_FLD (inst->v, 24, 20) == 0x3
                   && G_FLD (inst->v, 19, 15) == 0x0)
            {
              /* lw r3, [r0]+, 4 */
              sp_offset -= inst->len;
              ra_offset_p = 1;
            }
          else if (G_FLD (inst->v, 29, 25) == 0x7
                   && G_FLD (inst->v, 2, 0) == 0x0
                   && G_FLD (inst->v, 24, 20) == 0x2
                   && G_FLD (inst->v, 19, 15) == 0x0)
            {
              /* lw r2, [r0]+, 4 */
              sp_offset -= inst->len;
              fp_offset_p = 1;
            }
          else if (G_FLD (inst->v, 29, 25) == 0x1
                   && G_FLD (inst->v, 19, 17) == 0x0
                   && G_FLD (inst->v, 24, 20) == 0x0
                   && G_FLD (inst->v, 0, 0) == 0x0)
            {
              /* addi r0, -offset */
              int imm = G_FLD (inst->v, 16, 1);
	      if (imm >> 15)
		imm = -(0xFFFF - imm + 1);
              sp_offset -= imm;
            }
          else if (G_FLD (inst->v, 29, 25) == 0x1
                   && G_FLD (inst->v, 19, 17) == 0x0
                   && G_FLD (inst->v, 24, 20) == 0x2
                   && G_FLD (inst->v, 0, 0) == 0x0)
            {
              /* addi r2, offset */
              if (pc - cur_pc >= 2)
                {
		  unsigned int save_v = inst->v;
		  inst_t *inst2;
		  
		  cur_pc += inst->len;
		  inst2 = score3_adjust_pc_and_fetch_inst (&cur_pc, NULL,
							   byte_order);

                  if (inst2->len == 2
                      && G_FLD (inst2->v, 14, 10) == 0x10
                      && G_FLD (inst2->v, 9, 5) == 0x0
                      && G_FLD (inst2->v, 4, 0) == 0x2)
                    {
                      /* mv! r0, r2 */
                      int imm = G_FLD (inst->v, 16, 1);
	              if (imm >> 15)
		        imm = -(0xFFFF - imm + 1);
                      sp_offset -= imm;
                    }
                }
            }
        }
    }

  /* Save RA.  */
  if (ra_offset_p == 1)
    {
      if (this_cache->saved_regs[SCORE_PC_REGNUM].addr == -1)
        this_cache->saved_regs[SCORE_PC_REGNUM].addr =
          sp + sp_offset - ra_offset;
    }
  else
    {
      this_cache->saved_regs[SCORE_PC_REGNUM] =
        this_cache->saved_regs[SCORE_RA_REGNUM];
    }

  /* Save FP.  */
  if (fp_offset_p == 1)
    {
      if (this_cache->saved_regs[SCORE_FP_REGNUM].addr == -1)
        this_cache->saved_regs[SCORE_FP_REGNUM].addr =
          sp + sp_offset - fp_offset;
    }

  /* Save SP and FP.  */
  this_cache->base = sp + sp_offset;
  this_cache->fp = fp;
}

static struct score_frame_cache *
score_make_prologue_cache (struct frame_info *this_frame, void **this_cache)
{
  struct score_frame_cache *cache;

  if ((*this_cache) != NULL)
    return (*this_cache);

  cache = FRAME_OBSTACK_ZALLOC (struct score_frame_cache);
  (*this_cache) = cache;
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* Analyze the prologue.  */
  {
    const CORE_ADDR pc = get_frame_pc (this_frame);
    CORE_ADDR start_addr;

    find_pc_partial_function (pc, NULL, &start_addr, NULL);
    if (start_addr == 0)
      return cache;

    if (target_mach == bfd_mach_score3)
      score3_analyze_prologue (start_addr, pc, this_frame, *this_cache);
    else
      score7_analyze_prologue (start_addr, pc, this_frame, *this_cache);
  }

  /* Save SP.  */
  trad_frame_set_value (cache->saved_regs, SCORE_SP_REGNUM, cache->base);

  return (*this_cache);
}

static void
score_prologue_this_id (struct frame_info *this_frame, void **this_cache,
                        struct frame_id *this_id)
{
  struct score_frame_cache *info = score_make_prologue_cache (this_frame,
                                                              this_cache);
  (*this_id) = frame_id_build (info->base, get_frame_func (this_frame));
}

static struct value *
score_prologue_prev_register (struct frame_info *this_frame,
                              void **this_cache, int regnum)
{
  struct score_frame_cache *info = score_make_prologue_cache (this_frame,
                                                              this_cache);
  return trad_frame_get_prev_register (this_frame, info->saved_regs, regnum);
}

static const struct frame_unwind score_prologue_unwind =
{
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  score_prologue_this_id,
  score_prologue_prev_register,
  NULL,
  default_frame_sniffer,
  NULL
};

static CORE_ADDR
score_prologue_frame_base_address (struct frame_info *this_frame,
                                   void **this_cache)
{
  struct score_frame_cache *info =
    score_make_prologue_cache (this_frame, this_cache);
  return info->fp;
}

static const struct frame_base score_prologue_frame_base =
{
  &score_prologue_unwind,
  score_prologue_frame_base_address,
  score_prologue_frame_base_address,
  score_prologue_frame_base_address,
};

static const struct frame_base *
score_prologue_frame_base_sniffer (struct frame_info *this_frame)
{
  return &score_prologue_frame_base;
}

/* Core file support (dirty hack)
  
   The core file MUST be generated by GNU/Linux on S+core.  */

static void
score7_linux_supply_gregset(const struct regset *regset,
                struct regcache *regcache,
                int regnum, const void *gregs_buf, size_t len)
{
  int regno;
  elf_gregset_t *gregs;

  gdb_assert (regset != NULL);
  gdb_assert ((regcache != NULL) && (gregs_buf != NULL));

  gregs = (elf_gregset_t *) gregs_buf;

  for (regno = 0; regno < 32; regno++)
    if (regnum == -1 || regnum == regno)
      regcache_raw_supply (regcache, regno, gregs->regs + regno);

  {
    struct sreg {
      int regnum;
      void *buf;
    } sregs [] = {
      { 55, &(gregs->cel) },  /* CEL */
      { 54, &(gregs->ceh) },  /* CEH */
      { 53, &(gregs->sr0) },  /* sr0, i.e. cnt or COUNTER */
      { 52, &(gregs->sr1) },  /* sr1, i.e. lcr or LDCR */
      { 51, &(gregs->sr1) },  /* sr2, i.e. scr or STCR */

      /* Exception occured at this address, exactly the PC we want */
      { 49, &(gregs->cp0_epc) }, /* PC */

      { 38, &(gregs->cp0_ema) }, /* EMA */
      { 37, &(gregs->cp0_epc) }, /* EPC */
      { 34, &(gregs->cp0_ecr) }, /* ECR */
      { 33, &(gregs->cp0_condition) }, /* COND */
      { 32, &(gregs->cp0_psr) }, /* PSR */
    };

    for (regno = 0; regno < sizeof(sregs)/sizeof(sregs[0]); regno++)
      if (regnum == -1 || regnum == sregs[regno].regnum)
	regcache_raw_supply (regcache,
			     sregs[regno].regnum, sregs[regno].buf);
  }
}

/* Return the appropriate register set from the core section identified
   by SECT_NAME and SECT_SIZE.  */

static const struct regset *
score7_linux_regset_from_core_section(struct gdbarch *gdbarch,
                    const char *sect_name, size_t sect_size)
{
  struct gdbarch_tdep *tdep;

  gdb_assert (gdbarch != NULL);
  gdb_assert (sect_name != NULL);

  tdep = gdbarch_tdep (gdbarch);

  if (strcmp(sect_name, ".reg") == 0 && sect_size == sizeof(elf_gregset_t))
    {
      if (tdep->gregset == NULL)
	tdep->gregset = regset_alloc (gdbarch,
				      score7_linux_supply_gregset, NULL);
      return tdep->gregset;
    }

  return NULL;
}

static struct gdbarch *
score_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;
  struct gdbarch_tdep *tdep;
  target_mach = info.bfd_arch_info->mach;

  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    {
      return (arches->gdbarch);
    }
  tdep = xcalloc(1, sizeof(struct gdbarch_tdep));
  gdbarch = gdbarch_alloc (&info, tdep);

  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 64);
#if WITH_SIM
  set_gdbarch_register_sim_regno (gdbarch, score_register_sim_regno);
#endif
  set_gdbarch_pc_regnum (gdbarch, SCORE_PC_REGNUM);
  set_gdbarch_sp_regnum (gdbarch, SCORE_SP_REGNUM);
  set_gdbarch_adjust_breakpoint_address (gdbarch,
					 score_adjust_breakpoint_address);
  set_gdbarch_register_type (gdbarch, score_register_type);
  set_gdbarch_frame_align (gdbarch, score_frame_align);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_unwind_sp (gdbarch, score_unwind_sp);
  set_gdbarch_unwind_pc (gdbarch, score_unwind_pc);
  set_gdbarch_print_insn (gdbarch, score_print_insn);

  switch (target_mach)
    {
    case bfd_mach_score7:
      set_gdbarch_breakpoint_from_pc (gdbarch, score7_breakpoint_from_pc);
      set_gdbarch_skip_prologue (gdbarch, score7_skip_prologue);
      set_gdbarch_in_function_epilogue_p (gdbarch,
					  score7_in_function_epilogue_p);
      set_gdbarch_register_name (gdbarch, score7_register_name);
      set_gdbarch_num_regs (gdbarch, SCORE7_NUM_REGS);
      /* Core file support.  */
      set_gdbarch_regset_from_core_section (gdbarch,
					    score7_linux_regset_from_core_section);
      break;

    case bfd_mach_score3:
      set_gdbarch_breakpoint_from_pc (gdbarch, score3_breakpoint_from_pc);
      set_gdbarch_skip_prologue (gdbarch, score3_skip_prologue);
      set_gdbarch_in_function_epilogue_p (gdbarch,
					  score3_in_function_epilogue_p);
      set_gdbarch_register_name (gdbarch, score3_register_name);
      set_gdbarch_num_regs (gdbarch, SCORE3_NUM_REGS);
      break;
    }

  /* Watchpoint hooks.  */
  set_gdbarch_have_nonsteppable_watchpoint (gdbarch, 1);

  /* Dummy frame hooks.  */
  set_gdbarch_return_value (gdbarch, score_return_value);
  set_gdbarch_call_dummy_location (gdbarch, AT_ENTRY_POINT);
  set_gdbarch_dummy_id (gdbarch, score_dummy_id);
  set_gdbarch_push_dummy_call (gdbarch, score_push_dummy_call);

  /* Normal frame hooks.  */
  dwarf2_append_unwinders (gdbarch);
  frame_base_append_sniffer (gdbarch, dwarf2_frame_base_sniffer);
  frame_unwind_append_unwinder (gdbarch, &score_prologue_unwind);
  frame_base_append_sniffer (gdbarch, score_prologue_frame_base_sniffer);

  return gdbarch;
}

extern initialize_file_ftype _initialize_score_tdep;

void
_initialize_score_tdep (void)
{
  gdbarch_register (bfd_arch_score, score_gdbarch_init, NULL);
}
