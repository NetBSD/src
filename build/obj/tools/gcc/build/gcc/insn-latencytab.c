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
insn_default_latency (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  enum attr_type cached_type ATTRIBUTE_UNUSED;
  enum attr_op_mem cached_op_mem ATTRIBUTE_UNUSED;
  enum attr_size cached_size ATTRIBUTE_UNUSED;
  enum attr_opx_type cached_opx_type ATTRIBUTE_UNUSED;
  enum attr_opy_type cached_opy_type ATTRIBUTE_UNUSED;

  switch (recog_memoized (insn))
    {
    case 425:  /* ib */
    case 424:  /* stack_tie */
      return 0;

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
(CPU_CFV3))))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 4;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 4;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 4;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 4;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 4;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 4;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 4;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 5;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 5;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 4;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 4;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 1;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 1;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 1;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_0I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_0I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_0I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_0I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_size = get_attr_size (insn)) == SIZE_2)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_size = get_attr_size (insn)) == SIZE_2)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) && (((cached_type = get_attr_type (insn)) == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SCC) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
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
(CPU_CFV3))))) && ((cached_type = get_attr_type (insn)) == TYPE_LEA)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
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
(CPU_CFV3))))) && ((cached_type = get_attr_type (insn)) == TYPE_LEA)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
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
(CPU_CFV3))))) && ((cached_type = get_attr_type (insn)) == TYPE_LEA)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
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
(CPU_CFV3))))) && ((cached_type = get_attr_type (insn)) == TYPE_LEA)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
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
(CPU_CFV3))))) && ((cached_type = get_attr_type (insn)) == TYPE_LEA)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 1;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 1;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 1;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
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
	  return 18 /* 0x12 */;
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
	  return 18 /* 0x12 */;
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
	  return 18 /* 0x12 */;
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
	  return 9;
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
	  return 9;
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
	  return 9;
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
	  return 20 /* 0x14 */;
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
	  return 20 /* 0x14 */;
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
	  return 20 /* 0x14 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 21 /* 0x15 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 21 /* 0x15 */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 21 /* 0x15 */;
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
	  return 11 /* 0xb */;
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
	  return 11 /* 0xb */;
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
	  return 11 /* 0xb */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 12 /* 0xc */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 12 /* 0xc */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 12 /* 0xc */;
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
	  return 12 /* 0xc */;
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
	  return 12 /* 0xc */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 13 /* 0xd */;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_NO))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 13 /* 0xd */;
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
	  return 5;
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
	  return 5;
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
	  return 5;
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
	  return 3;
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
	  return 3;
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
	  return 3;
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
	  return 7;
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
	  return 7;
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
	  return 7;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 8;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 8;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 8;
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
	  return 5;
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
	  return 5;
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
	  return 5;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 6;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 6;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 6;
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
	  return 6;
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
	  return 6;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 7;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_MAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 7;
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
	  return 4;
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
	  return 4;
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
	  return 4;
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
	  return 6;
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
	  return 6;
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
	  return 6;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 7;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 7;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 7;
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
	  return 6;
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
	  return 6;
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
	  return 6;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 7;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 7;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 7;
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
	  return 7;
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
	  return 7;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && (((cached_size = get_attr_size (insn)) == SIZE_1) || (cached_size == SIZE_2)))
        {
	  return 8;
        }
      else if (((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((
#line 157 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_mac)) == (
(MAC_CF_EMAC))))) && ((cached_type = get_attr_type (insn)) == TYPE_MUL_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 8;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_RTS))
        {
	  return 5;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_RTS))
        {
	  return 8;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_BSR) || (cached_type == TYPE_JSR))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_BSR) || (cached_type == TYPE_JSR))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && (((cached_type = get_attr_type (insn)) == TYPE_BSR) || (cached_type == TYPE_JSR))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 3;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_BSR) || (cached_type == TYPE_JSR))) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_BSR) || (cached_type == TYPE_JSR))) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && (((cached_type = get_attr_type (insn)) == TYPE_BSR) || (cached_type == TYPE_JSR))) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_BCC)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_BCC)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_BCC)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_BCC)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_BCC)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_BCC)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_BRA)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_BRA)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 2;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_BRA)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 2;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_BRA)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_BRA)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_BRA)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 1;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_JMP)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_JMP)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 3;
        }
      else if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_JMP)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 3;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_JMP)) && ((cached_size = get_attr_size (insn)) == SIZE_1))
        {
	  return 5;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_JMP)) && ((cached_size = get_attr_size (insn)) == SIZE_2))
        {
	  return 5;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_JMP)) && ((cached_size = get_attr_size (insn)) == SIZE_3))
        {
	  return 5;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV1)))) || (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV2))))) && ((cached_type = get_attr_type (insn)) == TYPE_UNLK))
        {
	  return 2;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV3)))) && ((cached_type = get_attr_type (insn)) == TYPE_UNLK))
        {
	  return 3;
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
	  return 3;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_NEG_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_CLR_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) && ((cached_opx_type = get_attr_opx_type (insn)) == OPX_TYPE_RN) && (((cached_opy_type = get_attr_opy_type (insn)) == OPY_TYPE_NONE) || (cached_opy_type == OPY_TYPE_IMM_Q) || (cached_opy_type == OPY_TYPE_IMM_W) || (cached_opy_type == OPY_TYPE_IMM_L)))
        {
	  return 1;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_LEA))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_MOVE_L) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_CLR_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) && ((! ((cached_opx_type = get_attr_opx_type (insn)) == OPX_TYPE_RN)) || (! (((cached_opy_type = get_attr_opy_type (insn)) == OPY_TYPE_NONE) || (cached_opy_type == OPY_TYPE_IMM_Q) || (cached_opy_type == OPY_TYPE_IMM_W) || (cached_opy_type == OPY_TYPE_IMM_L)))))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_MOVEQ_L)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CMP) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_SCC) || (cached_type == TYPE_TST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00))
        {
	  return 4;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALUX_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00))
        {
	  return 4;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_TST) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10))
        {
	  return 4;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_MOVE_L)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10))
        {
	  return 3;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT) || (cached_type == TYPE_TST) || (cached_type == TYPE_TST_L))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0))
        {
	  return 5;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_MOVE_L)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0))
        {
	  return 4;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_0I))
        {
	  return 2;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1))
        {
	  return 2;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITR) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_1I))
        {
	  return 2;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_11) || (cached_op_mem == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I) || (cached_op_mem == OP_MEM_10)))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_PEA)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I1) || (cached_op_mem == OP_MEM_1I)))
        {
	  return 1;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_LINK))
        {
	  return 2;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_UNLK))
        {
	  return 2;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_DIV_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I)))
        {
	  return 20 /* 0x14 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_DIV_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I)))
        {
	  return 20 /* 0x14 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_DIV_W)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1)))
        {
	  return 21 /* 0x15 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_DIV_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I)))
        {
	  return 35 /* 0x23 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_DIV_L)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1)))
        {
	  return 35 /* 0x23 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_MUL_W) || (cached_type == TYPE_MUL_L))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I)))
        {
	  return 7;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_MUL_W) || (cached_type == TYPE_MUL_L))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I)))
        {
	  return 7;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_MUL_W) || (cached_type == TYPE_MUL_L))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0) || (cached_op_mem == OP_MEM_I1)))
        {
	  return 8;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_FALU) || (cached_type == TYPE_FCMP) || (cached_type == TYPE_FMUL))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I)))
        {
	  return 7;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_FALU) || (cached_type == TYPE_FCMP) || (cached_type == TYPE_FMUL))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I) || (cached_op_mem == OP_MEM_I1)))
        {
	  return 7;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_FMOVE) || (cached_type == TYPE_FNEG) || (cached_type == TYPE_FTST))) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00))
        {
	  return 4;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_FMOVE) || (cached_type == TYPE_FNEG) || (cached_type == TYPE_FTST))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I) || (cached_op_mem == OP_MEM_I1)))
        {
	  return 4;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_FMOVE) || (cached_type == TYPE_FNEG) || (cached_type == TYPE_FTST))) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_01) || (cached_op_mem == OP_MEM_0I)))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_FDIV)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I)))
        {
	  return 23 /* 0x17 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_FDIV)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I) || (cached_op_mem == OP_MEM_I1)))
        {
	  return 23 /* 0x17 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_FSQRT)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_00) || (cached_op_mem == OP_MEM_01) || (cached_op_mem == OP_MEM_0I)))
        {
	  return 56 /* 0x38 */;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_FSQRT)) && (((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10) || (cached_op_mem == OP_MEM_I0) || (cached_op_mem == OP_MEM_11) || (cached_op_mem == OP_MEM_1I) || (cached_op_mem == OP_MEM_I1)))
        {
	  return 56 /* 0x38 */;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_BCC))
        {
	  return 0;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_FBCC))
        {
	  return 2;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_BRA) || (cached_type == TYPE_BSR)))
        {
	  return 1;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_JMP) || (cached_type == TYPE_JSR)))
        {
	  return 5;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_RTS))
        {
	  return 2;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_NOP))
        {
	  return 1;
        }
      else if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((cached_type = get_attr_type (insn)) == TYPE_TRAP) || (cached_type == TYPE_UNKNOWN)))
        {
	  return 10 /* 0xa */;
        }
      else if ((cached_type = get_attr_type (insn)) == TYPE_IGNORE)
        {
	  return 0;
        }
      else
        {
	  return 0;
        }

    }
}

