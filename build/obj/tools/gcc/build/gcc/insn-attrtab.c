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
bypass_p (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  enum attr_type cached_type ATTRIBUTE_UNUSED;
  enum attr_op_mem cached_op_mem ATTRIBUTE_UNUSED;
  enum attr_opx_type cached_opx_type ATTRIBUTE_UNUSED;
  enum attr_opy_type cached_opy_type ATTRIBUTE_UNUSED;

  switch (recog_memoized (insn))
    {
    case 294:  /* absdf2_cf */
    case 293:  /* abssf2_cf */
      extract_constrain_insn_cached (insn);
      if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (which_alternative == 0)) && ((
#line 244 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_op_mem (insn))) == (
(OP_MEM_00))))
        {
	  return 1;
        }
      else
        {
	  return 0;
        }

    case 150:  /* *addsi3_5200 */
      extract_constrain_insn_cached (insn);
      if ((((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((1ULL << which_alternative) & 0x1bULL))) && ((
#line 244 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_op_mem (insn))) == (
(OP_MEM_00)))) || ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (!((1ULL << which_alternative) & 0x3bULL))))
        {
	  return 1;
        }
      else
        {
	  return 0;
        }

    case 95:  /* *68k_extendhisi2 */
      extract_constrain_insn_cached (insn);
      if (((which_alternative == 1) && (((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4))))) && ((
#line 244 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_op_mem (insn))) == (
(OP_MEM_00))))
        {
	  return 1;
        }
      else
        {
	  return 0;
        }

    case 359:  /* *bclrmemqi_ext */
    case 358:  /* bclrmemqi */
    case 357:  /* *bclrdreg */
    case 356:  /* *bchgdreg */
    case 355:  /* *bsetdreg */
    case 354:  /* *bsetmemqi_ext */
    case 353:  /* bsetmemqi */
    case 317:  /* subreghi1ashrdi_const32 */
    case 298:  /* one_cmplsi2_5200 */
    case 275:  /* negsi2_5200 */
    case 274:  /* negsi2_internal */
    case 175:  /* subsi3 */
    case 173:  /* subdi_dishl32 */
    case 146:  /* adddi_dishl32 */
    case 97:  /* *cfv4_extendqisi2 */
    case 94:  /* *cfv4_extendhisi2 */
    case 88:  /* *zero_extendqisi2_cfv4 */
    case 85:  /* *zero_extendhisi2_cf */
    case 64:  /* *movstrictqi_cf */
      extract_constrain_insn_cached (insn);
      if ((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((
#line 244 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_op_mem (insn))) == (
(OP_MEM_00))))
        {
	  return 1;
        }
      else
        {
	  return 0;
        }

    case 49:  /* pushexthisi_const */
      extract_constrain_insn_cached (insn);
      if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && (((1ULL << which_alternative) & 0x3ULL))) && ((
#line 244 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_op_mem (insn))) == (
(OP_MEM_00))))
        {
	  return 1;
        }
      else
        {
	  return 0;
        }

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      /* FALLTHRU */
    case 50:  /* *movsi_const0_68000_10 */
    case 51:  /* *movsi_const0_68040_60 */
    case 52:  /* *movsi_const0 */
    case 55:  /* *movsi_cf */
    case 66:  /* movsf_cf_soft */
    case 318:  /* subregsi1ashrdi_const32 */
    case 329:  /* subreg1lshrdi_const32 */
      if (((((
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
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_MOVE_L)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_10))
        {
	  return 1;
        }
      else if (((((
#line 153 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_cpu)) == (
(CPU_CFV4)))) && ((cached_type = get_attr_type (insn)) == TYPE_MOVE_L)) && ((cached_op_mem = get_attr_op_mem (insn)) == OP_MEM_I0))
        {
	  return 1;
        }
      else
        {
	  return 0;
        }

    default:
      return 0;

    }
}

int
num_delay_slots (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  switch (recog_memoized (insn))
    {
    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      /* FALLTHRU */
    default:
      return 0;

    }
}

int
get_attr_enabled (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  switch (recog_memoized (insn))
    {
    case 92:  /* extendsidi2 */
      extract_constrain_insn_cached (insn);
      if ((
#line 267 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE)) && (which_alternative == 1))
        {
	  return 0;
        }
      else
        {
	  return 1;
        }

    case 148:  /* addsi_lshrsi_31 */
    case 27:  /* cbranchsi4_btst_reg_insn_1 */
    case 24:  /* cbranchsi4_btst_mem_insn */
      extract_constrain_insn_cached (insn);
      if ((
#line 267 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE)) && (which_alternative == 0))
        {
	  return 0;
        }
      else
        {
	  return 1;
        }

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      /* FALLTHRU */
      extract_constrain_insn_cached (insn);
      if ((
#line 267 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_COLDFIRE)) && (get_attr_ok_for_coldfire (insn) == OK_FOR_COLDFIRE_NO))
        {
	  return 0;
        }
      else
        {
	  return 1;
        }

    default:
      return 1;

    }
}

enum attr_flags_valid
get_attr_flags_valid (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  switch (recog_memoized (insn))
    {
    case 175:  /* subsi3 */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return FLAGS_VALID_NOOV;
        }
      else if (which_alternative == 1)
        {
	  return FLAGS_VALID_UNCHANGED;
        }
      else if (((1ULL << which_alternative) & 0xcULL))
        {
	  return FLAGS_VALID_NOOV;
        }
      else
        {
	  return FLAGS_VALID_UNCHANGED;
        }

    case 149:  /* *addsi3_internal */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return FLAGS_VALID_NOOV;
        }
      else if (((1ULL << which_alternative) & 0x6ULL))
        {
	  return FLAGS_VALID_UNCHANGED;
        }
      else if (which_alternative == 3)
        {
	  return FLAGS_VALID_NOOV;
        }
      else
        {
	  return FLAGS_VALID_UNCHANGED;
        }

    case 57:  /* *m68k.md:1084 */
    case 58:  /* *m68k.md:1094 */
      extract_insn_cached (insn);
      if (address_reg_operand (operands[0], VOIDmode))
        {
	  return FLAGS_VALID_UNCHANGED;
        }
      else
        {
	  return FLAGS_VALID_MOVE;
        }

    case 77:  /* truncsiqi2 */
    case 78:  /* trunchiqi2 */
    case 79:  /* truncsihi2 */
      extract_insn_cached (insn);
      if (register_operand (operands[0], VOIDmode))
        {
	  return FLAGS_VALID_NO;
        }
      else
        {
	  return FLAGS_VALID_YES;
        }

    case 152:  /* addhi3 */
      extract_insn_cached (insn);
      if (address_reg_operand (operands[0], VOIDmode))
        {
	  return FLAGS_VALID_UNCHANGED;
        }
      else
        {
	  return FLAGS_VALID_NOOV;
        }

    case 147:  /* adddi3 */
    case 174:  /* subdi3 */
      extract_insn_cached (insn);
      if (register_operand (operands[0], VOIDmode))
        {
	  return FLAGS_VALID_NOOV;
        }
      else
        {
	  return FLAGS_VALID_NO;
        }

    case 310:  /* ashlsi3 */
      extract_insn_cached (insn);
      if (const1_operand (operands[2], VOIDmode))
        {
	  return FLAGS_VALID_NOOV;
        }
      else
        {
	  return FLAGS_VALID_YES;
        }

    case 395:  /* blockage */
    case 396:  /* nop */
      return FLAGS_VALID_UNCHANGED;

    case 59:  /* *m68k.md:1110 */
    case 60:  /* *m68k.md:1117 */
    case 63:  /* *m68k.md:1150 */
    case 65:  /* *m68k.md:1209 */
    case 68:  /* *m68k.md:1355 */
    case 71:  /* *m68k.md:1475 */
      return FLAGS_VALID_MOVE;

    case 153:  /* *m68k.md:2628 */
    case 154:  /* *m68k.md:2677 */
    case 155:  /* addqi3 */
    case 177:  /* subhi3 */
    case 178:  /* *m68k.md:3003 */
    case 179:  /* subqi3 */
    case 180:  /* *m68k.md:3019 */
    case 274:  /* negsi2_internal */
    case 275:  /* negsi2_5200 */
    case 276:  /* neghi2 */
    case 277:  /* *m68k.md:4028 */
    case 278:  /* negqi2 */
    case 279:  /* *m68k.md:4042 */
    case 324:  /* ashrsi3 */
    case 325:  /* ashrhi3 */
    case 326:  /* *m68k.md:4914 */
    case 327:  /* ashrqi3 */
    case 328:  /* *m68k.md:4930 */
      return FLAGS_VALID_NOOV;

    case 6:  /* cbranchqi4_insn */
    case 7:  /* cbranchhi4_insn */
    case 8:  /* cbranchsi4_insn */
    case 9:  /* cbranchqi4_insn_rev */
    case 10:  /* cbranchhi4_insn_rev */
    case 11:  /* cbranchsi4_insn_rev */
    case 12:  /* cbranchqi4_insn_cf */
    case 13:  /* cbranchhi4_insn_cf */
    case 14:  /* cbranchsi4_insn_cf */
    case 15:  /* cbranchqi4_insn_cf_rev */
    case 16:  /* cbranchhi4_insn_cf_rev */
    case 17:  /* cbranchsi4_insn_cf_rev */
    case 18:  /* cstoreqi4_insn */
    case 19:  /* cstorehi4_insn */
    case 20:  /* cstoresi4_insn */
    case 21:  /* cstoreqi4_insn_cf */
    case 22:  /* cstorehi4_insn_cf */
    case 23:  /* cstoresi4_insn_cf */
    case 32:  /* cbranchsf4_insn_68881 */
    case 33:  /* cbranchdf4_insn_68881 */
    case 34:  /* cbranchxf4_insn_68881 */
    case 35:  /* cbranchsf4_insn_cf */
    case 36:  /* cbranchdf4_insn_cf */
    case 37:  /* cbranchxf4_insn_cf */
    case 38:  /* cbranchsf4_insn_rev_68881 */
    case 39:  /* cbranchdf4_insn_rev_68881 */
    case 40:  /* cbranchxf4_insn_rev_68881 */
    case 41:  /* cbranchsf4_insn_rev_cf */
    case 42:  /* cbranchdf4_insn_rev_cf */
    case 43:  /* cbranchxf4_insn_rev_cf */
    case 44:  /* cstoresf4_insn_68881 */
    case 45:  /* cstoredf4_insn_68881 */
    case 46:  /* cstorexf4_insn_68881 */
    case 47:  /* cstoresf4_insn_cf */
    case 48:  /* cstoredf4_insn_cf */
    case 53:  /* *movsi_m68k */
    case 54:  /* *movsi_m68k2 */
    case 61:  /* *m68k.md:1130 */
    case 62:  /* *m68k.md:1137 */
    case 245:  /* andsi3_internal */
    case 254:  /* iorsi3_internal */
    case 255:  /* iorsi3_5200 */
    case 264:  /* xorsi3_internal */
    case 265:  /* xorsi3_5200 */
      return FLAGS_VALID_SET;

    case 247:  /* andhi3 */
    case 248:  /* *m68k.md:3701 */
    case 249:  /* *m68k.md:3709 */
    case 250:  /* andqi3 */
    case 251:  /* *m68k.md:3725 */
    case 258:  /* *m68k.md:3807 */
    case 259:  /* iorqi3 */
    case 260:  /* *m68k.md:3823 */
    case 261:  /* *m68k.md:3831 */
    case 266:  /* xorhi3 */
    case 267:  /* *m68k.md:3913 */
    case 268:  /* *m68k.md:3921 */
    case 269:  /* xorqi3 */
    case 270:  /* *m68k.md:3937 */
    case 271:  /* *m68k.md:3945 */
    case 297:  /* one_cmplsi2_internal */
    case 299:  /* one_cmplhi2 */
    case 300:  /* *m68k.md:4386 */
    case 301:  /* one_cmplqi2 */
    case 302:  /* *m68k.md:4400 */
    case 311:  /* ashlhi3 */
    case 312:  /* *m68k.md:4668 */
    case 313:  /* ashlqi3 */
    case 314:  /* *m68k.md:4684 */
    case 337:  /* lshrsi3 */
    case 338:  /* lshrhi3 */
    case 339:  /* *m68k.md:5216 */
    case 340:  /* lshrqi3 */
    case 341:  /* *m68k.md:5232 */
    case 342:  /* rotlsi_16 */
    case 343:  /* rotlsi3 */
    case 344:  /* rotlhi3 */
    case 345:  /* *rotlhi3_lowpart */
    case 346:  /* rotlqi3 */
    case 347:  /* *rotlqi3_lowpart */
    case 348:  /* rotrsi3 */
    case 351:  /* rotrqi3 */
    case 352:  /* *m68k.md:5363 */
      return FLAGS_VALID_YES;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      /* FALLTHRU */
    default:
      return FLAGS_VALID_NO;

    }
}

enum attr_ok_for_coldfire
get_attr_ok_for_coldfire (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  switch (recog_memoized (insn))
    {
    case 92:  /* extendsidi2 */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return OK_FOR_COLDFIRE_YES;
        }
      else if (which_alternative == 1)
        {
	  return OK_FOR_COLDFIRE_NO;
        }
      else
        {
	  return OK_FOR_COLDFIRE_YES;
        }

    case 148:  /* addsi_lshrsi_31 */
    case 24:  /* cbranchsi4_btst_mem_insn */
    case 27:  /* cbranchsi4_btst_reg_insn_1 */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return OK_FOR_COLDFIRE_NO;
        }
      else
        {
	  return OK_FOR_COLDFIRE_YES;
        }

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      /* FALLTHRU */
    default:
      return OK_FOR_COLDFIRE_YES;

    }
}

enum attr_op_mem
get_attr_op_mem (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  switch (recog_memoized (insn))
    {
    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      /* FALLTHRU */
    default:
      extract_constrain_insn_cached (insn);
      return 
#line 244 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_op_mem (insn));

    }
}

enum attr_opx_access
get_attr_opx_access (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  enum attr_type cached_type ATTRIBUTE_UNUSED;

  switch (recog_memoized (insn))
    {
    case 150:  /* *addsi3_5200 */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 5)
        {
	  return OPX_ACCESS_NONE;
        }
      else if (!((1ULL << which_alternative) & 0x3bULL))
        {
	  return OPX_ACCESS_W;
        }
      else
        {
	  return OPX_ACCESS_RW;
        }

    case 55:  /* *movsi_cf */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 2)
        {
	  return OPX_ACCESS_NONE;
        }
      else
        {
	  return OPX_ACCESS_W;
        }

    case 294:  /* absdf2_cf */
    case 293:  /* abssf2_cf */
    case 95:  /* *68k_extendhisi2 */
    case 52:  /* *movsi_const0 */
      extract_constrain_insn_cached (insn);
      if (which_alternative != 0)
        {
	  return OPX_ACCESS_W;
        }
      else
        {
	  return OPX_ACCESS_RW;
        }

    case 50:  /* *movsi_const0_68000_10 */
      extract_constrain_insn_cached (insn);
      if (which_alternative != 1)
        {
	  return OPX_ACCESS_W;
        }
      else
        {
	  return OPX_ACCESS_RW;
        }

    case 1:  /* *movdf_internal */
      extract_constrain_insn_cached (insn);
      if (which_alternative != 0)
        {
	  return OPX_ACCESS_NONE;
        }
      else
        {
	  return OPX_ACCESS_W;
        }

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      /* FALLTHRU */
      extract_constrain_insn_cached (insn);
      if (((cached_type = get_attr_type (insn)) == TYPE_IB) || (cached_type == TYPE_IGNORE) || (cached_type == TYPE_NOP) || (cached_type == TYPE_RTS) || (cached_type == TYPE_TRAP) || (cached_type == TYPE_UNLK) || (cached_type == TYPE_UNKNOWN))
        {
	  return OPX_ACCESS_NONE;
        }
      else if ((cached_type == TYPE_BCC) || (cached_type == TYPE_BRA) || (cached_type == TYPE_BSR) || (cached_type == TYPE_BITR) || (cached_type == TYPE_CMP) || (cached_type == TYPE_CMP_L) || (cached_type == TYPE_FBCC) || (cached_type == TYPE_FCMP) || (cached_type == TYPE_FTST) || (cached_type == TYPE_JMP) || (cached_type == TYPE_JSR) || (cached_type == TYPE_TST) || (cached_type == TYPE_TST_L))
        {
	  return OPX_ACCESS_R;
        }
      else if ((cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_FNEG) || (cached_type == TYPE_FMOVE) || (cached_type == TYPE_LEA) || (cached_type == TYPE_MOV3Q_L) || (cached_type == TYPE_MOVE) || (cached_type == TYPE_MOVE_L) || (cached_type == TYPE_MOVEQ_L) || (cached_type == TYPE_MVSZ) || (cached_type == TYPE_PEA) || (cached_type == TYPE_SCC))
        {
	  return OPX_ACCESS_W;
        }
      else if ((cached_type == TYPE_ALU_L) || (cached_type == TYPE_ALUQ_L) || (cached_type == TYPE_ALUX_L) || (cached_type == TYPE_BITRW) || (cached_type == TYPE_DIV_W) || (cached_type == TYPE_DIV_L) || (cached_type == TYPE_EXT) || (cached_type == TYPE_FALU) || (cached_type == TYPE_FDIV) || (cached_type == TYPE_FMUL) || (cached_type == TYPE_FSQRT) || (cached_type == TYPE_LINK) || (cached_type == TYPE_MUL_W) || (cached_type == TYPE_MUL_L) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_SHIFT))
        {
	  return OPX_ACCESS_RW;
        }
      else
        {
	  return 
#line 232 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
((gcc_unreachable (), OPX_ACCESS_NONE));
        }

    case 402:  /* *link */
    case 359:  /* *bclrmemqi_ext */
    case 358:  /* bclrmemqi */
    case 357:  /* *bclrdreg */
    case 356:  /* *bchgdreg */
    case 355:  /* *bsetdreg */
    case 354:  /* *bsetmemqi_ext */
    case 353:  /* bsetmemqi */
    case 342:  /* rotlsi_16 */
    case 337:  /* lshrsi3 */
    case 324:  /* ashrsi3 */
    case 298:  /* one_cmplsi2_5200 */
    case 296:  /* *clzsi2_cf */
    case 289:  /* sqrtdf2_cf */
    case 288:  /* sqrtsf2_cf */
    case 287:  /* sqrtxf2_68881 */
    case 286:  /* sqrtdf2_68881 */
    case 285:  /* sqrtsf2_68881 */
    case 275:  /* negsi2_5200 */
    case 274:  /* negsi2_internal */
    case 240:  /* *m68k.md:3567 */
    case 238:  /* *m68k.md:3525 */
    case 237:  /* divdf3_cf */
    case 236:  /* divsf3_cf */
    case 223:  /* fmuldf3_cf */
    case 222:  /* fmulsf3_cf */
    case 218:  /* mulxf3_floatqi_68881 */
    case 217:  /* muldf3_floatqi_68881 */
    case 216:  /* mulsf3_floatqi_68881 */
    case 215:  /* mulxf3_floathi_68881 */
    case 214:  /* muldf3_floathi_68881 */
    case 213:  /* mulsf3_floathi_68881 */
    case 212:  /* mulxf3_floatsi_68881 */
    case 211:  /* muldf3_floatsi_68881 */
    case 210:  /* mulsf3_floatsi_68881 */
    case 201:  /* *mulhisisi3_z */
    case 200:  /* umulhisi3 */
    case 199:  /* *mulsi3_cf */
    case 198:  /* *mulsi3_68020 */
    case 197:  /* *mulhisisi3_s */
    case 196:  /* mulhisi3 */
    case 195:  /* mulhi3 */
    case 194:  /* subdf3_cf */
    case 193:  /* subsf3_cf */
    case 192:  /* subxf3_68881 */
    case 191:  /* subdf3_68881 */
    case 190:  /* subsf3_68881 */
    case 189:  /* subxf3_floatqi_68881 */
    case 188:  /* subdf3_floatqi_68881 */
    case 187:  /* subsf3_floatqi_68881 */
    case 186:  /* subxf3_floathi_68881 */
    case 185:  /* subdf3_floathi_68881 */
    case 184:  /* subsf3_floathi_68881 */
    case 183:  /* subxf3_floatsi_68881 */
    case 182:  /* subdf3_floatsi_68881 */
    case 181:  /* subsf3_floatsi_68881 */
    case 175:  /* subsi3 */
    case 173:  /* subdi_dishl32 */
    case 171:  /* adddf3_cf */
    case 170:  /* addsf3_cf */
    case 169:  /* addxf3_68881 */
    case 168:  /* adddf3_68881 */
    case 167:  /* addsf3_68881 */
    case 166:  /* addxf3_floatqi_68881 */
    case 165:  /* adddf3_floatqi_68881 */
    case 164:  /* addsf3_floatqi_68881 */
    case 163:  /* addxf3_floathi_68881 */
    case 162:  /* adddf3_floathi_68881 */
    case 161:  /* addsf3_floathi_68881 */
    case 160:  /* addxf3_floatsi_68881 */
    case 159:  /* adddf3_floatsi_68881 */
    case 158:  /* addsf3_floatsi_68881 */
    case 146:  /* adddi_dishl32 */
    case 126:  /* ftruncdf2_cf */
    case 125:  /* ftruncsf2_cf */
    case 124:  /* ftruncxf2_68881 */
    case 123:  /* ftruncdf2_68881 */
    case 122:  /* ftruncsf2_68881 */
    case 98:  /* *68k_extendqisi2 */
    case 96:  /* extendqihi2 */
      return OPX_ACCESS_RW;

    case 329:  /* subreg1lshrdi_const32 */
    case 318:  /* subregsi1ashrdi_const32 */
    case 317:  /* subreghi1ashrdi_const32 */
    case 141:  /* fixdfsi2_cf */
    case 140:  /* fixsfsi2_cf */
    case 139:  /* fixxfsi2_68881 */
    case 138:  /* fixdfsi2_68881 */
    case 137:  /* fixsfsi2_68881 */
    case 136:  /* fixdfhi2_cf */
    case 135:  /* fixsfhi2_cf */
    case 134:  /* fixxfhi2_68881 */
    case 133:  /* fixdfhi2_68881 */
    case 132:  /* fixsfhi2_68881 */
    case 131:  /* fixdfqi2_cf */
    case 130:  /* fixsfqi2_cf */
    case 129:  /* fixxfqi2_68881 */
    case 128:  /* fixdfqi2_68881 */
    case 127:  /* fixsfqi2_68881 */
    case 118:  /* floatqidf2_cf */
    case 117:  /* floatqisf2_cf */
    case 116:  /* floatqixf2_68881 */
    case 115:  /* floatqidf2_68881 */
    case 114:  /* floatqisf2_68881 */
    case 113:  /* floathidf2_cf */
    case 112:  /* floathisf2_cf */
    case 111:  /* floathixf2_68881 */
    case 110:  /* floathidf2_68881 */
    case 109:  /* floathisf2_68881 */
    case 108:  /* floatsidf2_cf */
    case 107:  /* floatsisf2_cf */
    case 106:  /* floatsixf2_68881 */
    case 105:  /* floatsidf2_68881 */
    case 104:  /* floatsisf2_68881 */
    case 103:  /* *truncdfsf2_68881 */
    case 102:  /* truncdfsf2_cf */
    case 97:  /* *cfv4_extendqisi2 */
    case 94:  /* *cfv4_extendhisi2 */
    case 88:  /* *zero_extendqisi2_cfv4 */
    case 85:  /* *zero_extendhisi2_cf */
    case 76:  /* pushasi */
    case 66:  /* movsf_cf_soft */
    case 64:  /* *movstrictqi_cf */
    case 51:  /* *movsi_const0_68040_60 */
    case 49:  /* pushexthisi_const */
      return OPX_ACCESS_W;

    case 405:  /* indirect_jump */
    case 394:  /* *symbolic_call_value_bsr */
    case 393:  /* *symbolic_call_value_jsr */
    case 392:  /* *non_symbolic_call_value */
    case 391:  /* *call */
    case 382:  /* *tablejump_internal */
    case 381:  /* jump */
      return OPX_ACCESS_R;

    default:
      return OPX_ACCESS_NONE;

    }
}

enum attr_opx_type
get_attr_opx_type (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  enum attr_type cached_type ATTRIBUTE_UNUSED;

  switch (recog_memoized (insn))
    {
    case 405:  /* indirect_jump */
    case 393:  /* *symbolic_call_value_jsr */
    case 392:  /* *non_symbolic_call_value */
    case 391:  /* *call */
    case 382:  /* *tablejump_internal */
      extract_constrain_insn_cached (insn);
      return 
#line 214 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opx_type (insn, 1));

    case 150:  /* *addsi3_5200 */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 5)
        {
	  return OPX_TYPE_NONE;
        }
      else
        {
	  return 
#line 215 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opx_type (insn, 0));
        }

    case 55:  /* *movsi_cf */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 2)
        {
	  return OPX_TYPE_NONE;
        }
      else if (which_alternative == 7)
        {
	  return OPX_TYPE_MEM1;
        }
      else
        {
	  return 
#line 215 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opx_type (insn, 0));
        }

    case 402:  /* *link */
    case 394:  /* *symbolic_call_value_bsr */
    case 381:  /* jump */
    case 359:  /* *bclrmemqi_ext */
    case 358:  /* bclrmemqi */
    case 357:  /* *bclrdreg */
    case 356:  /* *bchgdreg */
    case 355:  /* *bsetdreg */
    case 354:  /* *bsetmemqi_ext */
    case 353:  /* bsetmemqi */
    case 342:  /* rotlsi_16 */
    case 337:  /* lshrsi3 */
    case 329:  /* subreg1lshrdi_const32 */
    case 324:  /* ashrsi3 */
    case 318:  /* subregsi1ashrdi_const32 */
    case 317:  /* subreghi1ashrdi_const32 */
    case 298:  /* one_cmplsi2_5200 */
    case 296:  /* *clzsi2_cf */
    case 294:  /* absdf2_cf */
    case 293:  /* abssf2_cf */
    case 289:  /* sqrtdf2_cf */
    case 288:  /* sqrtsf2_cf */
    case 287:  /* sqrtxf2_68881 */
    case 286:  /* sqrtdf2_68881 */
    case 285:  /* sqrtsf2_68881 */
    case 275:  /* negsi2_5200 */
    case 274:  /* negsi2_internal */
    case 240:  /* *m68k.md:3567 */
    case 238:  /* *m68k.md:3525 */
    case 237:  /* divdf3_cf */
    case 236:  /* divsf3_cf */
    case 223:  /* fmuldf3_cf */
    case 222:  /* fmulsf3_cf */
    case 218:  /* mulxf3_floatqi_68881 */
    case 217:  /* muldf3_floatqi_68881 */
    case 216:  /* mulsf3_floatqi_68881 */
    case 215:  /* mulxf3_floathi_68881 */
    case 214:  /* muldf3_floathi_68881 */
    case 213:  /* mulsf3_floathi_68881 */
    case 212:  /* mulxf3_floatsi_68881 */
    case 211:  /* muldf3_floatsi_68881 */
    case 210:  /* mulsf3_floatsi_68881 */
    case 201:  /* *mulhisisi3_z */
    case 200:  /* umulhisi3 */
    case 199:  /* *mulsi3_cf */
    case 198:  /* *mulsi3_68020 */
    case 197:  /* *mulhisisi3_s */
    case 196:  /* mulhisi3 */
    case 195:  /* mulhi3 */
    case 194:  /* subdf3_cf */
    case 193:  /* subsf3_cf */
    case 192:  /* subxf3_68881 */
    case 191:  /* subdf3_68881 */
    case 190:  /* subsf3_68881 */
    case 189:  /* subxf3_floatqi_68881 */
    case 188:  /* subdf3_floatqi_68881 */
    case 187:  /* subsf3_floatqi_68881 */
    case 186:  /* subxf3_floathi_68881 */
    case 185:  /* subdf3_floathi_68881 */
    case 184:  /* subsf3_floathi_68881 */
    case 183:  /* subxf3_floatsi_68881 */
    case 182:  /* subdf3_floatsi_68881 */
    case 181:  /* subsf3_floatsi_68881 */
    case 175:  /* subsi3 */
    case 173:  /* subdi_dishl32 */
    case 171:  /* adddf3_cf */
    case 170:  /* addsf3_cf */
    case 169:  /* addxf3_68881 */
    case 168:  /* adddf3_68881 */
    case 167:  /* addsf3_68881 */
    case 166:  /* addxf3_floatqi_68881 */
    case 165:  /* adddf3_floatqi_68881 */
    case 164:  /* addsf3_floatqi_68881 */
    case 163:  /* addxf3_floathi_68881 */
    case 162:  /* adddf3_floathi_68881 */
    case 161:  /* addsf3_floathi_68881 */
    case 160:  /* addxf3_floatsi_68881 */
    case 159:  /* adddf3_floatsi_68881 */
    case 158:  /* addsf3_floatsi_68881 */
    case 146:  /* adddi_dishl32 */
    case 141:  /* fixdfsi2_cf */
    case 140:  /* fixsfsi2_cf */
    case 139:  /* fixxfsi2_68881 */
    case 138:  /* fixdfsi2_68881 */
    case 137:  /* fixsfsi2_68881 */
    case 136:  /* fixdfhi2_cf */
    case 135:  /* fixsfhi2_cf */
    case 134:  /* fixxfhi2_68881 */
    case 133:  /* fixdfhi2_68881 */
    case 132:  /* fixsfhi2_68881 */
    case 131:  /* fixdfqi2_cf */
    case 130:  /* fixsfqi2_cf */
    case 129:  /* fixxfqi2_68881 */
    case 128:  /* fixdfqi2_68881 */
    case 127:  /* fixsfqi2_68881 */
    case 126:  /* ftruncdf2_cf */
    case 125:  /* ftruncsf2_cf */
    case 124:  /* ftruncxf2_68881 */
    case 123:  /* ftruncdf2_68881 */
    case 122:  /* ftruncsf2_68881 */
    case 118:  /* floatqidf2_cf */
    case 117:  /* floatqisf2_cf */
    case 116:  /* floatqixf2_68881 */
    case 115:  /* floatqidf2_68881 */
    case 114:  /* floatqisf2_68881 */
    case 113:  /* floathidf2_cf */
    case 112:  /* floathisf2_cf */
    case 111:  /* floathixf2_68881 */
    case 110:  /* floathidf2_68881 */
    case 109:  /* floathisf2_68881 */
    case 108:  /* floatsidf2_cf */
    case 107:  /* floatsisf2_cf */
    case 106:  /* floatsixf2_68881 */
    case 105:  /* floatsidf2_68881 */
    case 104:  /* floatsisf2_68881 */
    case 103:  /* *truncdfsf2_68881 */
    case 102:  /* truncdfsf2_cf */
    case 98:  /* *68k_extendqisi2 */
    case 97:  /* *cfv4_extendqisi2 */
    case 96:  /* extendqihi2 */
    case 95:  /* *68k_extendhisi2 */
    case 94:  /* *cfv4_extendhisi2 */
    case 88:  /* *zero_extendqisi2_cfv4 */
    case 85:  /* *zero_extendhisi2_cf */
    case 66:  /* movsf_cf_soft */
    case 64:  /* *movstrictqi_cf */
    case 52:  /* *movsi_const0 */
    case 51:  /* *movsi_const0_68040_60 */
    case 50:  /* *movsi_const0_68000_10 */
      extract_constrain_insn_cached (insn);
      return 
#line 215 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opx_type (insn, 0));

    case 49:  /* pushexthisi_const */
      extract_constrain_insn_cached (insn);
      if (!((1ULL << which_alternative) & 0x3ULL))
        {
	  return OPX_TYPE_MEM1;
        }
      else
        {
	  return 
#line 215 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opx_type (insn, 0));
        }

    case 1:  /* *movdf_internal */
      extract_constrain_insn_cached (insn);
      if (which_alternative != 0)
        {
	  return OPX_TYPE_NONE;
        }
      else
        {
	  return 
#line 215 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opx_type (insn, 0));
        }

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      /* FALLTHRU */
      extract_constrain_insn_cached (insn);
      if (((cached_type = get_attr_type (insn)) == TYPE_IB) || (cached_type == TYPE_IGNORE) || (cached_type == TYPE_NOP) || (cached_type == TYPE_RTS) || (cached_type == TYPE_TRAP) || (cached_type == TYPE_UNLK) || (cached_type == TYPE_UNKNOWN))
        {
	  return OPX_TYPE_NONE;
        }
      else if (cached_type == TYPE_PEA)
        {
	  return OPX_TYPE_MEM1;
        }
      else if ((cached_type == TYPE_JMP) || (cached_type == TYPE_JSR))
        {
	  return 
#line 214 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opx_type (insn, 1));
        }
      else
        {
	  return 
#line 215 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opx_type (insn, 0));
        }

    case 76:  /* pushasi */
      return OPX_TYPE_MEM1;

    default:
      return OPX_TYPE_NONE;

    }
}

enum attr_opy_type
get_attr_opy_type (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  enum attr_type cached_type ATTRIBUTE_UNUSED;

  switch (recog_memoized (insn))
    {
    case 150:  /* *addsi3_5200 */
      extract_constrain_insn_cached (insn);
      if (((1ULL << which_alternative) & 0x3ULL))
        {
	  return 
#line 204 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 0));
        }
      else if (which_alternative == 2)
        {
	  return OPY_TYPE_MEM5;
        }
      else if (((1ULL << which_alternative) & 0x38ULL))
        {
	  if (which_alternative == 5)
	    {
	      return OPY_TYPE_NONE;
	    }
	  else
	    {
	      return 
#line 204 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 0));
	    }
        }
      else if (((1ULL << which_alternative) & 0xc0ULL))
        {
	  return OPY_TYPE_MEM6;
        }
      else
        {
	  return OPY_TYPE_MEM5;
        }

    case 95:  /* *68k_extendhisi2 */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return OPY_TYPE_NONE;
        }
      else
        {
	  return 
#line 204 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 0));
        }

    case 76:  /* pushasi */
      extract_constrain_insn_cached (insn);
      return 
#line 203 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 1));

    case 402:  /* *link */
    case 359:  /* *bclrmemqi_ext */
    case 358:  /* bclrmemqi */
    case 357:  /* *bclrdreg */
    case 356:  /* *bchgdreg */
    case 355:  /* *bsetdreg */
    case 354:  /* *bsetmemqi_ext */
    case 353:  /* bsetmemqi */
    case 342:  /* rotlsi_16 */
    case 337:  /* lshrsi3 */
    case 329:  /* subreg1lshrdi_const32 */
    case 324:  /* ashrsi3 */
    case 318:  /* subregsi1ashrdi_const32 */
    case 317:  /* subreghi1ashrdi_const32 */
    case 294:  /* absdf2_cf */
    case 293:  /* abssf2_cf */
    case 289:  /* sqrtdf2_cf */
    case 288:  /* sqrtsf2_cf */
    case 287:  /* sqrtxf2_68881 */
    case 286:  /* sqrtdf2_68881 */
    case 285:  /* sqrtsf2_68881 */
    case 240:  /* *m68k.md:3567 */
    case 238:  /* *m68k.md:3525 */
    case 237:  /* divdf3_cf */
    case 236:  /* divsf3_cf */
    case 223:  /* fmuldf3_cf */
    case 222:  /* fmulsf3_cf */
    case 218:  /* mulxf3_floatqi_68881 */
    case 217:  /* muldf3_floatqi_68881 */
    case 216:  /* mulsf3_floatqi_68881 */
    case 215:  /* mulxf3_floathi_68881 */
    case 214:  /* muldf3_floathi_68881 */
    case 213:  /* mulsf3_floathi_68881 */
    case 212:  /* mulxf3_floatsi_68881 */
    case 211:  /* muldf3_floatsi_68881 */
    case 210:  /* mulsf3_floatsi_68881 */
    case 201:  /* *mulhisisi3_z */
    case 200:  /* umulhisi3 */
    case 199:  /* *mulsi3_cf */
    case 198:  /* *mulsi3_68020 */
    case 197:  /* *mulhisisi3_s */
    case 196:  /* mulhisi3 */
    case 195:  /* mulhi3 */
    case 194:  /* subdf3_cf */
    case 193:  /* subsf3_cf */
    case 192:  /* subxf3_68881 */
    case 191:  /* subdf3_68881 */
    case 190:  /* subsf3_68881 */
    case 189:  /* subxf3_floatqi_68881 */
    case 188:  /* subdf3_floatqi_68881 */
    case 187:  /* subsf3_floatqi_68881 */
    case 186:  /* subxf3_floathi_68881 */
    case 185:  /* subdf3_floathi_68881 */
    case 184:  /* subsf3_floathi_68881 */
    case 183:  /* subxf3_floatsi_68881 */
    case 182:  /* subdf3_floatsi_68881 */
    case 181:  /* subsf3_floatsi_68881 */
    case 175:  /* subsi3 */
    case 173:  /* subdi_dishl32 */
    case 171:  /* adddf3_cf */
    case 170:  /* addsf3_cf */
    case 169:  /* addxf3_68881 */
    case 168:  /* adddf3_68881 */
    case 167:  /* addsf3_68881 */
    case 166:  /* addxf3_floatqi_68881 */
    case 165:  /* adddf3_floatqi_68881 */
    case 164:  /* addsf3_floatqi_68881 */
    case 163:  /* addxf3_floathi_68881 */
    case 162:  /* adddf3_floathi_68881 */
    case 161:  /* addsf3_floathi_68881 */
    case 160:  /* addxf3_floatsi_68881 */
    case 159:  /* adddf3_floatsi_68881 */
    case 158:  /* addsf3_floatsi_68881 */
    case 146:  /* adddi_dishl32 */
    case 141:  /* fixdfsi2_cf */
    case 140:  /* fixsfsi2_cf */
    case 139:  /* fixxfsi2_68881 */
    case 138:  /* fixdfsi2_68881 */
    case 137:  /* fixsfsi2_68881 */
    case 136:  /* fixdfhi2_cf */
    case 135:  /* fixsfhi2_cf */
    case 134:  /* fixxfhi2_68881 */
    case 133:  /* fixdfhi2_68881 */
    case 132:  /* fixsfhi2_68881 */
    case 131:  /* fixdfqi2_cf */
    case 130:  /* fixsfqi2_cf */
    case 129:  /* fixxfqi2_68881 */
    case 128:  /* fixdfqi2_68881 */
    case 127:  /* fixsfqi2_68881 */
    case 126:  /* ftruncdf2_cf */
    case 125:  /* ftruncsf2_cf */
    case 124:  /* ftruncxf2_68881 */
    case 123:  /* ftruncdf2_68881 */
    case 122:  /* ftruncsf2_68881 */
    case 118:  /* floatqidf2_cf */
    case 117:  /* floatqisf2_cf */
    case 116:  /* floatqixf2_68881 */
    case 115:  /* floatqidf2_68881 */
    case 114:  /* floatqisf2_68881 */
    case 113:  /* floathidf2_cf */
    case 112:  /* floathisf2_cf */
    case 111:  /* floathixf2_68881 */
    case 110:  /* floathidf2_68881 */
    case 109:  /* floathisf2_68881 */
    case 108:  /* floatsidf2_cf */
    case 107:  /* floatsisf2_cf */
    case 106:  /* floatsixf2_68881 */
    case 105:  /* floatsidf2_68881 */
    case 104:  /* floatsisf2_68881 */
    case 103:  /* *truncdfsf2_68881 */
    case 102:  /* truncdfsf2_cf */
    case 97:  /* *cfv4_extendqisi2 */
    case 94:  /* *cfv4_extendhisi2 */
    case 88:  /* *zero_extendqisi2_cfv4 */
    case 85:  /* *zero_extendhisi2_cf */
    case 66:  /* movsf_cf_soft */
      extract_constrain_insn_cached (insn);
      return 
#line 204 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 0));

    case 64:  /* *movstrictqi_cf */
      extract_constrain_insn_cached (insn);
      if (((1ULL << which_alternative) & 0x3ULL))
        {
	  return OPY_TYPE_NONE;
        }
      else
        {
	  return 
#line 204 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 0));
        }

    case 55:  /* *movsi_cf */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 2)
        {
	  return OPY_TYPE_NONE;
        }
      else if (((1ULL << which_alternative) & 0x180ULL))
        {
	  return 
#line 203 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 1));
        }
      else
        {
	  return 
#line 204 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 0));
        }

    case 51:  /* *movsi_const0_68040_60 */
      extract_constrain_insn_cached (insn);
      if (which_alternative != 0)
        {
	  return OPY_TYPE_NONE;
        }
      else
        {
	  return 
#line 203 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 1));
        }

    case 50:  /* *movsi_const0_68000_10 */
      extract_constrain_insn_cached (insn);
      if (!((1ULL << which_alternative) & 0x3ULL))
        {
	  return OPY_TYPE_NONE;
        }
      else
        {
	  return 
#line 204 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 0));
        }

    case 49:  /* pushexthisi_const */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return OPY_TYPE_NONE;
        }
      else if (!((1ULL << which_alternative) & 0x3ULL))
        {
	  return 
#line 203 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 1));
        }
      else
        {
	  return 
#line 204 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 0));
        }

    case 52:  /* *movsi_const0 */
    case 1:  /* *movdf_internal */
      extract_constrain_insn_cached (insn);
      if (which_alternative != 0)
        {
	  return OPY_TYPE_NONE;
        }
      else
        {
	  return 
#line 204 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 0));
        }

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      /* FALLTHRU */
      extract_constrain_insn_cached (insn);
      if (((cached_type = get_attr_type (insn)) == TYPE_EXT) || (cached_type == TYPE_FBCC) || (cached_type == TYPE_FTST) || (cached_type == TYPE_NEG_L) || (cached_type == TYPE_BCC) || (cached_type == TYPE_BRA) || (cached_type == TYPE_BSR) || (cached_type == TYPE_CLR) || (cached_type == TYPE_CLR_L) || (cached_type == TYPE_IB) || (cached_type == TYPE_IGNORE) || (cached_type == TYPE_JMP) || (cached_type == TYPE_JSR) || (cached_type == TYPE_NOP) || (cached_type == TYPE_RTS) || (cached_type == TYPE_SCC) || (cached_type == TYPE_TRAP) || (cached_type == TYPE_TST) || (cached_type == TYPE_TST_L) || (cached_type == TYPE_UNLK) || (cached_type == TYPE_UNKNOWN))
        {
	  return OPY_TYPE_NONE;
        }
      else if ((cached_type == TYPE_LEA) || (cached_type == TYPE_PEA))
        {
	  return 
#line 203 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 1));
        }
      else
        {
	  return 
#line 204 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_opy_type (insn, 0));
        }

    default:
      return OPY_TYPE_NONE;

    }
}

int
get_attr_opy (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  switch (recog_memoized (insn))
    {
    case 150:  /* *addsi3_5200 */
      extract_constrain_insn_cached (insn);
      if (((1ULL << which_alternative) & 0x3ULL))
        {
	  return 2;
        }
      else if (which_alternative == 2)
        {
	  return 1;
        }
      else if (((1ULL << which_alternative) & 0x18ULL))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 50:  /* *movsi_const0_68000_10 */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return 1;
        }
      else if (which_alternative == 1)
        {
	  return 0;
        }
      else
        {
	  return 1;
        }

    case 52:  /* *movsi_const0 */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return 0;
        }
      else
        {
	  return 1;
        }

    case 158:  /* addsf3_floatsi_68881 */
    case 159:  /* adddf3_floatsi_68881 */
    case 160:  /* addxf3_floatsi_68881 */
    case 161:  /* addsf3_floathi_68881 */
    case 162:  /* adddf3_floathi_68881 */
    case 163:  /* addxf3_floathi_68881 */
    case 164:  /* addsf3_floatqi_68881 */
    case 165:  /* adddf3_floatqi_68881 */
    case 166:  /* addxf3_floatqi_68881 */
    case 167:  /* addsf3_68881 */
    case 168:  /* adddf3_68881 */
    case 169:  /* addxf3_68881 */
    case 170:  /* addsf3_cf */
    case 171:  /* adddf3_cf */
    case 175:  /* subsi3 */
    case 181:  /* subsf3_floatsi_68881 */
    case 182:  /* subdf3_floatsi_68881 */
    case 183:  /* subxf3_floatsi_68881 */
    case 184:  /* subsf3_floathi_68881 */
    case 185:  /* subdf3_floathi_68881 */
    case 186:  /* subxf3_floathi_68881 */
    case 187:  /* subsf3_floatqi_68881 */
    case 188:  /* subdf3_floatqi_68881 */
    case 189:  /* subxf3_floatqi_68881 */
    case 190:  /* subsf3_68881 */
    case 191:  /* subdf3_68881 */
    case 192:  /* subxf3_68881 */
    case 193:  /* subsf3_cf */
    case 194:  /* subdf3_cf */
    case 195:  /* mulhi3 */
    case 196:  /* mulhisi3 */
    case 197:  /* *mulhisisi3_s */
    case 198:  /* *mulsi3_68020 */
    case 199:  /* *mulsi3_cf */
    case 200:  /* umulhisi3 */
    case 201:  /* *mulhisisi3_z */
    case 210:  /* mulsf3_floatsi_68881 */
    case 211:  /* muldf3_floatsi_68881 */
    case 212:  /* mulxf3_floatsi_68881 */
    case 213:  /* mulsf3_floathi_68881 */
    case 214:  /* muldf3_floathi_68881 */
    case 215:  /* mulxf3_floathi_68881 */
    case 216:  /* mulsf3_floatqi_68881 */
    case 217:  /* muldf3_floatqi_68881 */
    case 218:  /* mulxf3_floatqi_68881 */
    case 222:  /* fmulsf3_cf */
    case 223:  /* fmuldf3_cf */
    case 236:  /* divsf3_cf */
    case 237:  /* divdf3_cf */
    case 238:  /* *m68k.md:3525 */
    case 240:  /* *m68k.md:3567 */
    case 324:  /* ashrsi3 */
    case 337:  /* lshrsi3 */
      return 2;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      /* FALLTHRU */
    default:
      return 1;

    }
}

int
get_attr_opx (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  switch (recog_memoized (insn))
    {
    case 392:  /* *non_symbolic_call_value */
    case 393:  /* *symbolic_call_value_jsr */
    case 394:  /* *symbolic_call_value_bsr */
      return 1;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      /* FALLTHRU */
    default:
      return 0;

    }
}

enum attr_size
get_attr_size (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  switch (recog_memoized (insn))
    {
    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      /* FALLTHRU */
    default:
      extract_constrain_insn_cached (insn);
      return 
#line 248 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_sched_attr_size (insn));

    }
}

enum attr_type
get_attr_type (rtx_insn *insn ATTRIBUTE_UNUSED)
{
  switch (recog_memoized (insn))
    {
    case 175:  /* subsi3 */
      extract_constrain_insn_cached (insn);
      if (((1ULL << which_alternative) & 0x3ULL))
        {
	  return TYPE_ALUQ_L;
        }
      else
        {
	  return TYPE_ALU_L;
        }

    case 150:  /* *addsi3_5200 */
      extract_constrain_insn_cached (insn);
      if (((1ULL << which_alternative) & 0x3ULL))
        {
	  return TYPE_ALUQ_L;
        }
      else if (which_alternative == 2)
        {
	  return TYPE_LEA;
        }
      else if (((1ULL << which_alternative) & 0x18ULL))
        {
	  return TYPE_ALU_L;
        }
      else if (which_alternative == 5)
        {
	  return TYPE_UNKNOWN;
        }
      else
        {
	  return TYPE_LEA;
        }

    case 64:  /* *movstrictqi_cf */
      extract_constrain_insn_cached (insn);
      if (((1ULL << which_alternative) & 0x3ULL))
        {
	  return TYPE_CLR;
        }
      else
        {
	  return TYPE_MOVE;
        }

    case 55:  /* *movsi_cf */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return TYPE_MOV3Q_L;
        }
      else if (which_alternative == 1)
        {
	  return TYPE_MOVEQ_L;
        }
      else if (which_alternative == 2)
        {
	  return TYPE_UNKNOWN;
        }
      else if (((1ULL << which_alternative) & 0x18ULL))
        {
	  return TYPE_MVSZ;
        }
      else if (which_alternative == 5)
        {
	  return TYPE_MOVE_L;
        }
      else if (which_alternative == 6)
        {
	  return TYPE_MOVE;
        }
      else if (which_alternative == 7)
        {
	  return TYPE_PEA;
        }
      else if (which_alternative == 8)
        {
	  return TYPE_LEA;
        }
      else
        {
	  return TYPE_MOVE_L;
        }

    case 1:  /* *movdf_internal */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return TYPE_FMOVE;
        }
      else
        {
	  return TYPE_UNKNOWN;
        }

    case 49:  /* pushexthisi_const */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return TYPE_CLR_L;
        }
      else if (which_alternative == 1)
        {
	  return TYPE_MOV3Q_L;
        }
      else
        {
	  return TYPE_PEA;
        }

    case 50:  /* *movsi_const0_68000_10 */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return TYPE_MOVEQ_L;
        }
      else if (which_alternative == 1)
        {
	  return TYPE_ALU_L;
        }
      else
        {
	  return TYPE_CLR_L;
        }

    case 51:  /* *movsi_const0_68040_60 */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return TYPE_LEA;
        }
      else
        {
	  return TYPE_CLR_L;
        }

    case 52:  /* *movsi_const0 */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return TYPE_ALU_L;
        }
      else
        {
	  return TYPE_CLR_L;
        }

    case 95:  /* *68k_extendhisi2 */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return TYPE_EXT;
        }
      else
        {
	  return TYPE_MOVE;
        }

    case 293:  /* abssf2_cf */
    case 294:  /* absdf2_cf */
      extract_constrain_insn_cached (insn);
      if (which_alternative == 0)
        {
	  return TYPE_BITRW;
        }
      else
        {
	  return TYPE_FNEG;
        }

    case 403:  /* *unlink */
      return TYPE_UNLK;

    case 417:  /* trap */
      return TYPE_TRAP;

    case 324:  /* ashrsi3 */
    case 337:  /* lshrsi3 */
    case 342:  /* rotlsi_16 */
      return TYPE_SHIFT;

    case 397:  /* *return */
      return TYPE_RTS;

    case 76:  /* pushasi */
      return TYPE_PEA;

    case 396:  /* nop */
      return TYPE_NOP;

    case 274:  /* negsi2_internal */
    case 275:  /* negsi2_5200 */
    case 298:  /* one_cmplsi2_5200 */
      return TYPE_NEG_L;

    case 85:  /* *zero_extendhisi2_cf */
    case 88:  /* *zero_extendqisi2_cfv4 */
    case 94:  /* *cfv4_extendhisi2 */
    case 97:  /* *cfv4_extendqisi2 */
      return TYPE_MVSZ;

    case 198:  /* *mulsi3_68020 */
    case 199:  /* *mulsi3_cf */
      return TYPE_MUL_L;

    case 195:  /* mulhi3 */
    case 196:  /* mulhisi3 */
    case 197:  /* *mulhisisi3_s */
    case 200:  /* umulhisi3 */
    case 201:  /* *mulhisisi3_z */
      return TYPE_MUL_W;

    case 66:  /* movsf_cf_soft */
    case 318:  /* subregsi1ashrdi_const32 */
    case 329:  /* subreg1lshrdi_const32 */
      return TYPE_MOVE_L;

    case 317:  /* subreghi1ashrdi_const32 */
      return TYPE_MOVE;

    case 402:  /* *link */
      return TYPE_LINK;

    case 391:  /* *call */
    case 392:  /* *non_symbolic_call_value */
    case 393:  /* *symbolic_call_value_jsr */
      return TYPE_JSR;

    case 382:  /* *tablejump_internal */
    case 405:  /* indirect_jump */
      return TYPE_JMP;

    case 424:  /* stack_tie */
      return TYPE_IGNORE;

    case 425:  /* ib */
      return TYPE_IB;

    case 285:  /* sqrtsf2_68881 */
    case 286:  /* sqrtdf2_68881 */
    case 287:  /* sqrtxf2_68881 */
    case 288:  /* sqrtsf2_cf */
    case 289:  /* sqrtdf2_cf */
      return TYPE_FSQRT;

    case 210:  /* mulsf3_floatsi_68881 */
    case 211:  /* muldf3_floatsi_68881 */
    case 212:  /* mulxf3_floatsi_68881 */
    case 213:  /* mulsf3_floathi_68881 */
    case 214:  /* muldf3_floathi_68881 */
    case 215:  /* mulxf3_floathi_68881 */
    case 216:  /* mulsf3_floatqi_68881 */
    case 217:  /* muldf3_floatqi_68881 */
    case 218:  /* mulxf3_floatqi_68881 */
    case 222:  /* fmulsf3_cf */
    case 223:  /* fmuldf3_cf */
      return TYPE_FMUL;

    case 102:  /* truncdfsf2_cf */
    case 103:  /* *truncdfsf2_68881 */
    case 104:  /* floatsisf2_68881 */
    case 105:  /* floatsidf2_68881 */
    case 106:  /* floatsixf2_68881 */
    case 107:  /* floatsisf2_cf */
    case 108:  /* floatsidf2_cf */
    case 109:  /* floathisf2_68881 */
    case 110:  /* floathidf2_68881 */
    case 111:  /* floathixf2_68881 */
    case 112:  /* floathisf2_cf */
    case 113:  /* floathidf2_cf */
    case 114:  /* floatqisf2_68881 */
    case 115:  /* floatqidf2_68881 */
    case 116:  /* floatqixf2_68881 */
    case 117:  /* floatqisf2_cf */
    case 118:  /* floatqidf2_cf */
    case 127:  /* fixsfqi2_68881 */
    case 128:  /* fixdfqi2_68881 */
    case 129:  /* fixxfqi2_68881 */
    case 130:  /* fixsfqi2_cf */
    case 131:  /* fixdfqi2_cf */
    case 132:  /* fixsfhi2_68881 */
    case 133:  /* fixdfhi2_68881 */
    case 134:  /* fixxfhi2_68881 */
    case 135:  /* fixsfhi2_cf */
    case 136:  /* fixdfhi2_cf */
    case 137:  /* fixsfsi2_68881 */
    case 138:  /* fixdfsi2_68881 */
    case 139:  /* fixxfsi2_68881 */
    case 140:  /* fixsfsi2_cf */
    case 141:  /* fixdfsi2_cf */
      return TYPE_FMOVE;

    case 236:  /* divsf3_cf */
    case 237:  /* divdf3_cf */
      return TYPE_FDIV;

    case 122:  /* ftruncsf2_68881 */
    case 123:  /* ftruncdf2_68881 */
    case 124:  /* ftruncxf2_68881 */
    case 125:  /* ftruncsf2_cf */
    case 126:  /* ftruncdf2_cf */
    case 158:  /* addsf3_floatsi_68881 */
    case 159:  /* adddf3_floatsi_68881 */
    case 160:  /* addxf3_floatsi_68881 */
    case 161:  /* addsf3_floathi_68881 */
    case 162:  /* adddf3_floathi_68881 */
    case 163:  /* addxf3_floathi_68881 */
    case 164:  /* addsf3_floatqi_68881 */
    case 165:  /* adddf3_floatqi_68881 */
    case 166:  /* addxf3_floatqi_68881 */
    case 167:  /* addsf3_68881 */
    case 168:  /* adddf3_68881 */
    case 169:  /* addxf3_68881 */
    case 170:  /* addsf3_cf */
    case 171:  /* adddf3_cf */
    case 181:  /* subsf3_floatsi_68881 */
    case 182:  /* subdf3_floatsi_68881 */
    case 183:  /* subxf3_floatsi_68881 */
    case 184:  /* subsf3_floathi_68881 */
    case 185:  /* subdf3_floathi_68881 */
    case 186:  /* subxf3_floathi_68881 */
    case 187:  /* subsf3_floatqi_68881 */
    case 188:  /* subdf3_floatqi_68881 */
    case 189:  /* subxf3_floatqi_68881 */
    case 190:  /* subsf3_68881 */
    case 191:  /* subdf3_68881 */
    case 192:  /* subxf3_68881 */
    case 193:  /* subsf3_cf */
    case 194:  /* subdf3_cf */
      return TYPE_FALU;

    case 96:  /* extendqihi2 */
    case 98:  /* *68k_extendqisi2 */
    case 296:  /* *clzsi2_cf */
      return TYPE_EXT;

    case 238:  /* *m68k.md:3525 */
    case 240:  /* *m68k.md:3567 */
      return TYPE_DIV_L;

    case 394:  /* *symbolic_call_value_bsr */
      return TYPE_BSR;

    case 381:  /* jump */
      return TYPE_BRA;

    case 353:  /* bsetmemqi */
    case 354:  /* *bsetmemqi_ext */
    case 355:  /* *bsetdreg */
    case 356:  /* *bchgdreg */
    case 357:  /* *bclrdreg */
    case 358:  /* bclrmemqi */
    case 359:  /* *bclrmemqi_ext */
      return TYPE_BITRW;

    case 146:  /* adddi_dishl32 */
    case 173:  /* subdi_dishl32 */
      return TYPE_ALU_L;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      /* FALLTHRU */
    default:
      return TYPE_UNKNOWN;

    }
}

int
eligible_for_delay (rtx_insn *delay_insn ATTRIBUTE_UNUSED, int slot, 
		   rtx_insn *candidate_insn, int flags ATTRIBUTE_UNUSED)
{
  rtx_insn *insn ATTRIBUTE_UNUSED;

  if (num_delay_slots (delay_insn) == 0)
    return 0;
  gcc_assert (slot < 0);

  if (!INSN_P (candidate_insn))
    return 0;

  insn = candidate_insn;
  switch (slot)
    {
    default:
      gcc_unreachable ();
    }
}

int
eligible_for_annul_true (rtx_insn *delay_insn ATTRIBUTE_UNUSED,
    int slot ATTRIBUTE_UNUSED,
    rtx_insn *candidate_insn ATTRIBUTE_UNUSED,
    int flags ATTRIBUTE_UNUSED)
{
  return 0;
}

int
eligible_for_annul_false (rtx_insn *delay_insn ATTRIBUTE_UNUSED,
    int slot ATTRIBUTE_UNUSED,
    rtx_insn *candidate_insn ATTRIBUTE_UNUSED,
    int flags ATTRIBUTE_UNUSED)
{
  return 0;
}

int
const_num_delay_slots (rtx_insn *insn)
{
  switch (recog_memoized (insn))
    {
    default:
      return 1;
    }
}

EXPORTED_CONST int length_unit_log = 0;
