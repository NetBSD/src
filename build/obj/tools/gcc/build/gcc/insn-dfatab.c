/* Generated automatically by the program `genattrtab'
   from the machine description file `md'.  */

#define IN_TARGET_CODE 1
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "predict.h"
#include "tree.h"
#include "rtl.h"
#include "alias.h"
#include "options.h"
#include "varasm.h"
#include "stor-layout.h"
#include "calls.h"
#include "insn-attr.h"
#include "memmodel.h"
#include "tm_p.h"
#include "insn-config.h"
#include "recog.h"
#include "regs.h"
#include "real.h"
#include "output.h"
#include "toplev.h"
#include "flags.h"
#include "emit-rtl.h"

#define operands recog_data.operand

int
internal_dfa_insn_code (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  enum attr_type cached_type ATTRIBUTE_UNUSED;
  enum attr_op_mem cached_op_mem ATTRIBUTE_UNUSED;
  enum attr_size cached_size ATTRIBUTE_UNUSED;
  enum attr_opx_type cached_opx_type ATTRIBUTE_UNUSED;
  enum attr_opy_type cached_opy_type ATTRIBUTE_UNUSED;

  switch (recog_memoized (insn))
    {
    case 425:  /* ib */
      if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))))
        {
	  return 0;
        }
      else
        {
	  return 239 /* 0xef */;
        }

    case 424:  /* stack_tie */
      return 237 /* 0xed */;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      /* FALLTHRU */
    default:
      if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && ((cached_type = get_attr_type (insn)) == TYPE_IB))
        {
	  return 0;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 1;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 2;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 4;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 5;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 6;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 7;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 8;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 9;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 10 /* 0xa */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 11 /* 0xb */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 12 /* 0xc */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 13 /* 0xd */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 14 /* 0xe */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 15 /* 0xf */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 16 /* 0x10 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 17 /* 0x11 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 18 /* 0x12 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 19 /* 0x13 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 20 /* 0x14 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 21 /* 0x15 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 22 /* 0x16 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 23 /* 0x17 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 24 /* 0x18 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 25 /* 0x19 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 26 /* 0x1a */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 27 /* 0x1b */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 28 /* 0x1c */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 29 /* 0x1d */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 30 /* 0x1e */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 31 /* 0x1f */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 32 /* 0x20 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 33 /* 0x21 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 34 /* 0x22 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 35 /* 0x23 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 36 /* 0x24 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 37 /* 0x25 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 38 /* 0x26 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 39 /* 0x27 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_0I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 40 /* 0x28 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_0I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 41 /* 0x29 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_0I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 42 /* 0x2a */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_0I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 43 /* 0x2b */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 44 /* 0x2c */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 45 /* 0x2d */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 46 /* 0x2e */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 47 /* 0x2f */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 48 /* 0x30 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 49 /* 0x31 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 50 /* 0x32 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 51 /* 0x33 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 52 /* 0x34 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 53 /* 0x35 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 54 /* 0x36 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 55 /* 0x37 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 56 /* 0x38 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_size = get_attr_size (insn)) == SIZE_2)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11))
        {
	  return 57 /* 0x39 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 58 /* 0x3a */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 59 /* 0x3b */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_size = get_attr_size (insn)) == SIZE_2)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11))
        {
	  return 60 /* 0x3c */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 61 /* 0x3d */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 62 /* 0x3e */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 63 /* 0x3f */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 64 /* 0x40 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 65 /* 0x41 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 66 /* 0x42 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 67 /* 0x43 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 68 /* 0x44 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 69 /* 0x45 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 70 /* 0x46 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 71 /* 0x47 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 72 /* 0x48 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 73 /* 0x49 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 74 /* 0x4a */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 75 /* 0x4b */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 76 /* 0x4c */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 77 /* 0x4d */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 78 /* 0x4e */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 79 /* 0x4f */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 80 /* 0x50 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 81 /* 0x51 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 82 /* 0x52 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 83 /* 0x53 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 84 /* 0x54 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 85 /* 0x55 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && ((cached_type = get_attr_type (insn)) == TYPE_LEA)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 86 /* 0x56 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && ((cached_type = get_attr_type (insn)) == TYPE_LEA)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 87 /* 0x57 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && ((cached_type = get_attr_type (insn)) == TYPE_LEA)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 88 /* 0x58 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && ((cached_type = get_attr_type (insn)) == TYPE_LEA)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 89 /* 0x59 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && ((cached_type = get_attr_type (insn)) == TYPE_LEA)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 90 /* 0x5a */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 91 /* 0x5b */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 92 /* 0x5c */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 93 /* 0x5d */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 94 /* 0x5e */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 95 /* 0x5f */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 96 /* 0x60 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 97 /* 0x61 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 98 /* 0x62 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 99 /* 0x63 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 100 /* 0x64 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 101 /* 0x65 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 102 /* 0x66 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 103 /* 0x67 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 104 /* 0x68 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 105 /* 0x69 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 106 /* 0x6a */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 107 /* 0x6b */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 108 /* 0x6c */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 109 /* 0x6d */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 110 /* 0x6e */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 111 /* 0x6f */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 112 /* 0x70 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 113 /* 0x71 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 114 /* 0x72 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 115 /* 0x73 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 116 /* 0x74 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 117 /* 0x75 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 118 /* 0x76 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 119 /* 0x77 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 120 /* 0x78 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 121 /* 0x79 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 122 /* 0x7a */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 123 /* 0x7b */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 124 /* 0x7c */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 125 /* 0x7d */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 126 /* 0x7e */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 127 /* 0x7f */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 128 /* 0x80 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 129 /* 0x81 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 130 /* 0x82 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 131 /* 0x83 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 132 /* 0x84 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 133 /* 0x85 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 134 /* 0x86 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 135 /* 0x87 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 136 /* 0x88 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 137 /* 0x89 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 138 /* 0x8a */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 139 /* 0x8b */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 140 /* 0x8c */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 141 /* 0x8d */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 142 /* 0x8e */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 143 /* 0x8f */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 144 /* 0x90 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && (((cached_type = get_attr_type (insn)) == TYPE_MUL_L) || (cached_type == TYPE_MUL_W))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 145 /* 0x91 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && (((cached_type = get_attr_type (insn)) == TYPE_MUL_L) || (cached_type == TYPE_MUL_W))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 146 /* 0x92 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && (((cached_type = get_attr_type (insn)) == TYPE_MUL_L) || (cached_type == TYPE_MUL_W))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 147 /* 0x93 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 148 /* 0x94 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 149 /* 0x95 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 150 /* 0x96 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 151 /* 0x97 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 152 /* 0x98 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 153 /* 0x99 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 154 /* 0x9a */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 155 /* 0x9b */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 156 /* 0x9c */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 157 /* 0x9d */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 158 /* 0x9e */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 159 /* 0x9f */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 160 /* 0xa0 */;
        }
      else if ((((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 161 /* 0xa1 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 162 /* 0xa2 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 163 /* 0xa3 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_RTS))
        {
	  return 164 /* 0xa4 */;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_RTS))
        {
	  return 165 /* 0xa5 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_BSR) || (cached_type == TYPE_JSR))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 166 /* 0xa6 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_BSR) || (cached_type == TYPE_JSR))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 167 /* 0xa7 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_BSR) || (cached_type == TYPE_JSR))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 168 /* 0xa8 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_BSR) || (cached_type == TYPE_JSR))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 169 /* 0xa9 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_BSR) || (cached_type == TYPE_JSR))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 170 /* 0xaa */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_BSR) || (cached_type == TYPE_JSR))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 171 /* 0xab */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_BCC)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 172 /* 0xac */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_BCC)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 173 /* 0xad */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_BCC)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 174 /* 0xae */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_BCC)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 175 /* 0xaf */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_BCC)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 176 /* 0xb0 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_BCC)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 177 /* 0xb1 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_BRA)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 178 /* 0xb2 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_BRA)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 179 /* 0xb3 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_BRA)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 180 /* 0xb4 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_BRA)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 181 /* 0xb5 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_BRA)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 182 /* 0xb6 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_BRA)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 183 /* 0xb7 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_JMP)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 184 /* 0xb8 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_JMP)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 185 /* 0xb9 */;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_JMP)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 186 /* 0xba */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_JMP)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 187 /* 0xbb */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_JMP)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 188 /* 0xbc */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_JMP)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 189 /* 0xbd */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_UNLK))
        {
	  return 190 /* 0xbe */;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_UNLK))
        {
	  return 191 /* 0xbf */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3))))) && (((cached_type = get_attr_type (insn)) == TYPE_FALU) || (cached_type == TYPE_FBCC) || (cached_type == TYPE_FCMP) || (cached_type == TYPE_FDIV) || (cached_type == TYPE_FMOVE) || (cached_type == TYPE_FMUL) || (cached_type == TYPE_FNEG) || (cached_type == TYPE_FSQRT) || (cached_type == TYPE_FTST) || (cached_type == TYPE_DIV_W) || (cached_type == TYPE_DIV_L) || (cached_type == TYPE_LINK) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NOP) || (cached_type == TYPE_TRAP) || (cached_type == TYPE_UNKNOWN)))
        {
	  return 192 /* 0xc0 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_NEG_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00))
        {
	  return 193 /* 0xc1 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_CLR_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) && ((cached_opx_type = get_attr_opx_type (insn)) == OPX_TYPE_RN) && (((cached_opy_type = get_attr_opy_type (insn)) == OPY_TYPE_NONE) || (cached_opy_type == OPY_TYPE_IMM_Q) || (cached_opy_type == OPY_TYPE_IMM_W) || (cached_opy_type == OPY_TYPE_IMM_L)))
        {
	  return 194 /* 0xc2 */;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_LEA))
        {
	  return 195 /* 0xc3 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_CLR_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) && ((! ((cached_opx_type = get_attr_opx_type (insn)) == OPX_TYPE_RN)) || (! (((cached_opy_type = get_attr_opy_type (insn)) == OPY_TYPE_NONE) || (cached_opy_type == OPY_TYPE_IMM_Q) || (cached_opy_type == OPY_TYPE_IMM_W) || (cached_opy_type == OPY_TYPE_IMM_L)))))
        {
	  return 196 /* 0xc4 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_MOVEQ_L)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00))
        {
	  return 197 /* 0xc5 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CMP) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_SCC) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00))
        {
	  return 198 /* 0xc6 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00))
        {
	  return 199 /* 0xc7 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_TST) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10))
        {
	  return 200 /* 0xc8 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_MOVE_L)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10))
        {
	  return 201 /* 0xc9 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_TST) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0))
        {
	  return 202 /* 0xca */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_MOVE_L)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0))
        {
	  return 203 /* 0xcb */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01))
        {
	  return 204 /* 0xcc */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_0I))
        {
	  return 205 /* 0xcd */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11))
        {
	  return 206 /* 0xce */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1))
        {
	  return 207 /* 0xcf */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I))
        {
	  return 208 /* 0xd0 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11) || (cached_op_mem == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I) || (cached_op_mem == OP_MEM_10)))
        {
	  return 209 /* 0xd1 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1) || (cached_op_mem == OP_MEM_1I)))
        {
	  return 210 /* 0xd2 */;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_LINK))
        {
	  return 211 /* 0xd3 */;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_UNLK))
        {
	  return 212 /* 0xd4 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_DIV_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I)))
        {
	  return 213 /* 0xd5 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_DIV_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I)))
        {
	  return 214 /* 0xd6 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_DIV_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1)))
        {
	  return 215 /* 0xd7 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_DIV_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I)))
        {
	  return 216 /* 0xd8 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_DIV_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1)))
        {
	  return 217 /* 0xd9 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_MUL_W) || (cached_type == TYPE_MUL_L))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I)))
        {
	  return 218 /* 0xda */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_MUL_W) || (cached_type == TYPE_MUL_L))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I)))
        {
	  return 219 /* 0xdb */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_MUL_W) || (cached_type == TYPE_MUL_L))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1)))
        {
	  return 220 /* 0xdc */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_FALU) || (cached_type == TYPE_FCMP) || (cached_type == TYPE_FMUL))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I)))
        {
	  return 221 /* 0xdd */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_FALU) || (cached_type == TYPE_FCMP) || (cached_type == TYPE_FMUL))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I) || (cached_op_mem == OP_MEM_I1)))
        {
	  return 222 /* 0xde */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_FMOVE) || (cached_type == TYPE_FNEG) || (cached_type == TYPE_FTST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00))
        {
	  return 223 /* 0xdf */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_FMOVE) || (cached_type == TYPE_FNEG) || (cached_type == TYPE_FTST))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I) || (cached_op_mem == OP_MEM_I1)))
        {
	  return 224 /* 0xe0 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_FMOVE) || (cached_type == TYPE_FNEG) || (cached_type == TYPE_FTST))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01) || (cached_op_mem == OP_MEM_0I)))
        {
	  return 225 /* 0xe1 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_FDIV)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I)))
        {
	  return 226 /* 0xe2 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_FDIV)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I) || (cached_op_mem == OP_MEM_I1)))
        {
	  return 227 /* 0xe3 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_FSQRT)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I)))
        {
	  return 228 /* 0xe4 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_FSQRT)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I) || (cached_op_mem == OP_MEM_I1)))
        {
	  return 229 /* 0xe5 */;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_BCC))
        {
	  return 230 /* 0xe6 */;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_FBCC))
        {
	  return 231 /* 0xe7 */;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_BRA) || (cached_type == TYPE_BSR)))
        {
	  return 232 /* 0xe8 */;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_JMP) || (cached_type == TYPE_JSR)))
        {
	  return 233 /* 0xe9 */;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_RTS))
        {
	  return 234 /* 0xea */;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_NOP))
        {
	  return 235 /* 0xeb */;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_TRAP) || (cached_type == TYPE_UNKNOWN)))
        {
	  return 236 /* 0xec */;
        }
      else if ((cached_type = get_attr_type (insn)) == TYPE_IGNORE)
        {
	  return 237 /* 0xed */;
        }
      else
        {
	  return 239 /* 0xef */;
        }

    }
}

