/* Generated automatically by the program `genemit'
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
#include "varasm.h"
#include "stor-layout.h"
#include "calls.h"
#include "memmodel.h"
#include "tm_p.h"
#include "flags.h"
#include "insn-config.h"
#include "expmed.h"
#include "dojump.h"
#include "explow.h"
#include "emit-rtl.h"
#include "stmt.h"
#include "expr.h"
#include "insn-codes.h"
#include "optabs.h"
#include "dfp.h"
#include "output.h"
#include "recog.h"
#include "df.h"
#include "resource.h"
#include "reload.h"
#include "diagnostic-core.h"
#include "regs.h"
#include "tm-constrs.h"
#include "ggc.h"
#include "target.h"

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:306 */
rtx
gen_pushdi (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	operand1);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:320 */
rtx
gen_beq0_di (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_EQ (VOIDmode,
	operand0,
	const0_rtx),
	gen_rtx_LABEL_REF (VOIDmode,
	operand1),
	pc_rtx)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:359 */
rtx
gen_bne0_di (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_NE (VOIDmode,
	operand0,
	const0_rtx),
	gen_rtx_LABEL_REF (VOIDmode,
	operand1),
	pc_rtx)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:392 */
rtx
gen_cbranchdi4_insn (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED,
	rtx operand4 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (3,
		gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand1), VOIDmode,
		operand2,
		operand3),
	gen_rtx_LABEL_REF (VOIDmode,
	operand4),
	pc_rtx)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (DImode)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:503 */
rtx
gen_cbranchqi4_insn (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:503 */
rtx
gen_cbranchhi4_insn (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:503 */
rtx
gen_cbranchsi4_insn (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:518 */
rtx
gen_cbranchqi4_insn_rev (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	pc_rtx,
	gen_rtx_LABEL_REF (VOIDmode,
	operand3)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:518 */
rtx
gen_cbranchhi4_insn_rev (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	pc_rtx,
	gen_rtx_LABEL_REF (VOIDmode,
	operand3)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:518 */
rtx
gen_cbranchsi4_insn_rev (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	pc_rtx,
	gen_rtx_LABEL_REF (VOIDmode,
	operand3)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:533 */
rtx
gen_cbranchqi4_insn_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:533 */
rtx
gen_cbranchhi4_insn_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:533 */
rtx
gen_cbranchsi4_insn_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:548 */
rtx
gen_cbranchqi4_insn_cf_rev (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	pc_rtx,
	gen_rtx_LABEL_REF (VOIDmode,
	operand3)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:548 */
rtx
gen_cbranchhi4_insn_cf_rev (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	pc_rtx,
	gen_rtx_LABEL_REF (VOIDmode,
	operand3)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:548 */
rtx
gen_cbranchsi4_insn_cf_rev (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	pc_rtx,
	gen_rtx_LABEL_REF (VOIDmode,
	operand3)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:563 */
rtx
gen_cstoreqi4_insn (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:563 */
rtx
gen_cstorehi4_insn (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:563 */
rtx
gen_cstoresi4_insn (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:576 */
rtx
gen_cstoreqi4_insn_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:576 */
rtx
gen_cstorehi4_insn_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:576 */
rtx
gen_cstoresi4_insn_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:593 */
rtx
gen_cbranchsi4_btst_mem_insn (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		gen_rtx_ZERO_EXTRACT (SImode,
	operand1,
	const1_rtx,
	gen_rtx_MINUS (SImode,
	const_int_rtx[MAX_SAVED_CONST_INT + (7)],
	operand2)),
		const0_rtx),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:611 */
rtx
gen_cbranchsi4_btst_reg_insn (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		gen_rtx_ZERO_EXTRACT (SImode,
	operand1,
	const1_rtx,
	gen_rtx_MINUS (SImode,
	const_int_rtx[MAX_SAVED_CONST_INT + (31)],
	operand2)),
		const0_rtx),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:630 */
rtx
gen_cbranchsi4_btst_mem_insn_1 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		gen_rtx_ZERO_EXTRACT (SImode,
	operand1,
	const1_rtx,
	operand2),
		const0_rtx),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:647 */
rtx
gen_cbranchsi4_btst_reg_insn_1 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		gen_rtx_ZERO_EXTRACT (SImode,
	operand1,
	const1_rtx,
	operand2),
		const0_rtx),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:681 */
rtx
gen_cbranch_bftstqi_insn (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED,
	rtx operand4 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		gen_rtx_ZERO_EXTRACT (SImode,
	operand1,
	operand2,
	operand3),
		const0_rtx),
	gen_rtx_LABEL_REF (VOIDmode,
	operand4),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:681 */
rtx
gen_cbranch_bftstsi_insn (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED,
	rtx operand4 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		gen_rtx_ZERO_EXTRACT (SImode,
	operand1,
	operand2,
	operand3),
		const0_rtx),
	gen_rtx_LABEL_REF (VOIDmode,
	operand4),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:700 */
rtx
gen_cstore_bftstqi_insn (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED,
	rtx operand4 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		gen_rtx_ZERO_EXTRACT (SImode,
	operand2,
	operand3,
	operand4),
		const0_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:700 */
rtx
gen_cstore_bftstsi_insn (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED,
	rtx operand4 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		gen_rtx_ZERO_EXTRACT (SImode,
	operand2,
	operand3,
	operand4),
		const0_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:738 */
rtx
gen_cbranchsf4_insn_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:738 */
rtx
gen_cbranchdf4_insn_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:738 */
rtx
gen_cbranchxf4_insn_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:756 */
rtx
gen_cbranchsf4_insn_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:756 */
rtx
gen_cbranchdf4_insn_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:756 */
rtx
gen_cbranchxf4_insn_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:774 */
rtx
gen_cbranchsf4_insn_rev_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	pc_rtx,
	gen_rtx_LABEL_REF (VOIDmode,
	operand3)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:774 */
rtx
gen_cbranchdf4_insn_rev_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	pc_rtx,
	gen_rtx_LABEL_REF (VOIDmode,
	operand3)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:774 */
rtx
gen_cbranchxf4_insn_rev_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	pc_rtx,
	gen_rtx_LABEL_REF (VOIDmode,
	operand3)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:792 */
rtx
gen_cbranchsf4_insn_rev_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	pc_rtx,
	gen_rtx_LABEL_REF (VOIDmode,
	operand3)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:792 */
rtx
gen_cbranchdf4_insn_rev_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	pc_rtx,
	gen_rtx_LABEL_REF (VOIDmode,
	operand3)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:792 */
rtx
gen_cbranchxf4_insn_rev_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	pc_rtx,
	gen_rtx_LABEL_REF (VOIDmode,
	operand3)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:810 */
rtx
gen_cstoresf4_insn_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:810 */
rtx
gen_cstoredf4_insn_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:810 */
rtx
gen_cstorexf4_insn_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:827 */
rtx
gen_cstoresf4_insn_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:827 */
rtx
gen_cstoredf4_insn_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:844 */
rtx
gen_pushexthisi_const (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	operand1);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1252 */
rtx
gen_movsf_cf_soft (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	operand1);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1261 */
rtx
gen_movsf_cf_hard (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	operand1);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1393 */
rtx
gen_movdf_cf_soft (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	operand1);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1405 */
rtx
gen_movdf_cf_hard (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	operand1);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1619 */
rtx
gen_pushasi (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	operand1);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1627 */
rtx
gen_truncsiqi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_TRUNCATE (QImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1645 */
rtx
gen_trunchiqi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_TRUNCATE (QImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1668 */
rtx
gen_truncsihi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_TRUNCATE (HImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1722 */
rtx
gen_zero_extendqidi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ZERO_EXTEND (DImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1737 */
rtx
gen_zero_extendhidi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ZERO_EXTEND (DImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1784 */
rtx
gen_zero_extendhisi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ZERO_EXTEND (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1809 */
rtx
gen_zero_extendqisi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ZERO_EXTEND (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1847 */
rtx
gen_extendqidi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SIGN_EXTEND (DImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1871 */
rtx
gen_extendhidi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SIGN_EXTEND (DImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1886 */
rtx
gen_extendsidi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_SIGN_EXTEND (DImode,
	operand1)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1921 */
rtx
gen_extendplussidi (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SIGN_EXTEND (DImode,
	gen_rtx_PLUS (SImode,
	operand1,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1972 */
rtx
gen_extendqihi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SIGN_EXTEND (HImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2034 */
rtx
gen_extendsfdf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT_EXTEND (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2073 */
rtx
gen_truncdfsf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT_TRUNCATE (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2105 */
rtx
gen_floatsisf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2105 */
rtx
gen_floatsidf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2105 */
rtx
gen_floatsixf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (XFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2112 */
rtx
gen_floatsisf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2112 */
rtx
gen_floatsidf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2126 */
rtx
gen_floathisf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2126 */
rtx
gen_floathidf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2126 */
rtx
gen_floathixf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (XFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2133 */
rtx
gen_floathisf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2133 */
rtx
gen_floathidf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2147 */
rtx
gen_floatqisf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2147 */
rtx
gen_floatqidf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2147 */
rtx
gen_floatqixf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (XFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2154 */
rtx
gen_floatqisf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2154 */
rtx
gen_floatqidf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2167 */
rtx
gen_fix_truncdfsi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (3,
		gen_rtx_SET (operand0,
	gen_rtx_FIX (SImode,
	gen_rtx_FIX (DFmode,
	operand1))),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2177 */
rtx
gen_fix_truncdfhi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (3,
		gen_rtx_SET (operand0,
	gen_rtx_FIX (HImode,
	gen_rtx_FIX (DFmode,
	operand1))),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2187 */
rtx
gen_fix_truncdfqi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (3,
		gen_rtx_SET (operand0,
	gen_rtx_FIX (QImode,
	gen_rtx_FIX (DFmode,
	operand1))),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2206 */
rtx
gen_ftruncsf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2206 */
rtx
gen_ftruncdf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2206 */
rtx
gen_ftruncxf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (XFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2217 */
rtx
gen_ftruncsf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2217 */
rtx
gen_ftruncdf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2236 */
rtx
gen_fixsfqi2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (QImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2236 */
rtx
gen_fixdfqi2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (QImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2236 */
rtx
gen_fixxfqi2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (QImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2243 */
rtx
gen_fixsfqi2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (QImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2243 */
rtx
gen_fixdfqi2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (QImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2256 */
rtx
gen_fixsfhi2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (HImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2256 */
rtx
gen_fixdfhi2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (HImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2256 */
rtx
gen_fixxfhi2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (HImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2263 */
rtx
gen_fixsfhi2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (HImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2263 */
rtx
gen_fixdfhi2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (HImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2276 */
rtx
gen_fixsfsi2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2276 */
rtx
gen_fixdfsi2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2276 */
rtx
gen_fixxfsi2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2283 */
rtx
gen_fixsfsi2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2283 */
rtx
gen_fixdfsi2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2293 */
rtx
gen_adddi_lshrdi_63 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_PLUS (DImode,
	gen_rtx_LSHIFTRT (DImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (63)]),
	copy_rtx (operand1))),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2326 */
rtx
gen_adddi_sexthishl32 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_PLUS (DImode,
	gen_rtx_ASHIFT (DImode,
	gen_rtx_SIGN_EXTEND (DImode,
	operand1),
	const_int_rtx[MAX_SAVED_CONST_INT + (32)]),
	operand2)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2367 */
rtx
gen_adddi_dishl32 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (DImode,
	gen_rtx_ASHIFT (DImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (32)]),
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2385 */
rtx
gen_adddi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_PLUS (DImode,
	operand1,
	operand2)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2466 */
rtx
gen_addsi_lshrsi_31 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (SImode,
	gen_rtx_LSHIFTRT (SImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (31)]),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2569 */
rtx
gen_addhi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (HImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2726 */
rtx
gen_addqi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (QImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2803 */
rtx
gen_addsf3_floatsi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (SFmode,
	gen_rtx_FLOAT (SFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2803 */
rtx
gen_adddf3_floatsi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (DFmode,
	gen_rtx_FLOAT (DFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2803 */
rtx
gen_addxf3_floatsi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (XFmode,
	gen_rtx_FLOAT (XFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2812 */
rtx
gen_addsf3_floathi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (SFmode,
	gen_rtx_FLOAT (SFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2812 */
rtx
gen_adddf3_floathi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (DFmode,
	gen_rtx_FLOAT (DFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2812 */
rtx
gen_addxf3_floathi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (XFmode,
	gen_rtx_FLOAT (XFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2821 */
rtx
gen_addsf3_floatqi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (SFmode,
	gen_rtx_FLOAT (SFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2821 */
rtx
gen_adddf3_floatqi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (DFmode,
	gen_rtx_FLOAT (DFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2821 */
rtx
gen_addxf3_floatqi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (XFmode,
	gen_rtx_FLOAT (XFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2830 */
rtx
gen_addsf3_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (SFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2830 */
rtx
gen_adddf3_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (DFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2830 */
rtx
gen_addxf3_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (XFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2843 */
rtx
gen_addsf3_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (SFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2843 */
rtx
gen_adddf3_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (DFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2858 */
rtx
gen_subdi_sexthishl32 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_MINUS (DImode,
	operand1,
	gen_rtx_ASHIFT (DImode,
	gen_rtx_SIGN_EXTEND (DImode,
	operand2),
	const_int_rtx[MAX_SAVED_CONST_INT + (32)]))),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2874 */
rtx
gen_subdi_dishl32 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (DImode,
	operand0,
	gen_rtx_ASHIFT (DImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (32)])));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2889 */
rtx
gen_subdi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_MINUS (DImode,
	operand1,
	operand2)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2972 */
rtx
gen_subsi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2995 */
rtx
gen_subhi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (HImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3011 */
rtx
gen_subqi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (QImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3034 */
rtx
gen_subsf3_floatsi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (SFmode,
	operand1,
	gen_rtx_FLOAT (SFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3034 */
rtx
gen_subdf3_floatsi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (DFmode,
	operand1,
	gen_rtx_FLOAT (DFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3034 */
rtx
gen_subxf3_floatsi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (XFmode,
	operand1,
	gen_rtx_FLOAT (XFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3043 */
rtx
gen_subsf3_floathi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (SFmode,
	operand1,
	gen_rtx_FLOAT (SFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3043 */
rtx
gen_subdf3_floathi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (DFmode,
	operand1,
	gen_rtx_FLOAT (DFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3043 */
rtx
gen_subxf3_floathi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (XFmode,
	operand1,
	gen_rtx_FLOAT (XFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3052 */
rtx
gen_subsf3_floatqi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (SFmode,
	operand1,
	gen_rtx_FLOAT (SFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3052 */
rtx
gen_subdf3_floatqi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (DFmode,
	operand1,
	gen_rtx_FLOAT (DFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3052 */
rtx
gen_subxf3_floatqi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (XFmode,
	operand1,
	gen_rtx_FLOAT (XFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3061 */
rtx
gen_subsf3_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (SFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3061 */
rtx
gen_subdf3_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (DFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3061 */
rtx
gen_subxf3_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (XFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3074 */
rtx
gen_subsf3_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (SFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3074 */
rtx
gen_subdf3_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (DFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3089 */
rtx
gen_mulhi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (HImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3100 */
rtx
gen_mulhisi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (SImode,
	gen_rtx_SIGN_EXTEND (SImode,
	operand1),
	gen_rtx_SIGN_EXTEND (SImode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3151 */
rtx
gen_umulhisi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (SImode,
	gen_rtx_ZERO_EXTEND (SImode,
	operand1),
	gen_rtx_ZERO_EXTEND (SImode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3288 */
rtx
gen_const_umulsi3_highpart (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_TRUNCATE (SImode,
	gen_rtx_LSHIFTRT (DImode,
	gen_rtx_MULT (DImode,
	gen_rtx_ZERO_EXTEND (DImode,
	operand2),
	operand3),
	const_int_rtx[MAX_SAVED_CONST_INT + (32)]))),
		gen_rtx_CLOBBER (VOIDmode,
	operand1)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3331 */
rtx
gen_const_smulsi3_highpart (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_TRUNCATE (SImode,
	gen_rtx_LSHIFTRT (DImode,
	gen_rtx_MULT (DImode,
	gen_rtx_SIGN_EXTEND (DImode,
	operand2),
	operand3),
	const_int_rtx[MAX_SAVED_CONST_INT + (32)]))),
		gen_rtx_CLOBBER (VOIDmode,
	operand1)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3349 */
rtx
gen_mulsf3_floatsi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (SFmode,
	gen_rtx_FLOAT (SFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3349 */
rtx
gen_muldf3_floatsi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (DFmode,
	gen_rtx_FLOAT (DFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3349 */
rtx
gen_mulxf3_floatsi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (XFmode,
	gen_rtx_FLOAT (XFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3362 */
rtx
gen_mulsf3_floathi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (SFmode,
	gen_rtx_FLOAT (SFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3362 */
rtx
gen_muldf3_floathi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (DFmode,
	gen_rtx_FLOAT (DFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3362 */
rtx
gen_mulxf3_floathi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (XFmode,
	gen_rtx_FLOAT (XFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3375 */
rtx
gen_mulsf3_floatqi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (SFmode,
	gen_rtx_FLOAT (SFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3375 */
rtx
gen_muldf3_floatqi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (DFmode,
	gen_rtx_FLOAT (DFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3375 */
rtx
gen_mulxf3_floatqi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (XFmode,
	gen_rtx_FLOAT (XFmode,
	operand2),
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3388 */
rtx
gen_muldf_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (DFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3406 */
rtx
gen_mulsf_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (SFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3421 */
rtx
gen_mulxf3_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (XFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3430 */
rtx
gen_fmulsf3_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (SFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3430 */
rtx
gen_fmuldf3_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (DFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3452 */
rtx
gen_divsf3_floatsi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (SFmode,
	operand1,
	gen_rtx_FLOAT (SFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3452 */
rtx
gen_divdf3_floatsi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (DFmode,
	operand1,
	gen_rtx_FLOAT (DFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3452 */
rtx
gen_divxf3_floatsi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (XFmode,
	operand1,
	gen_rtx_FLOAT (XFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3463 */
rtx
gen_divsf3_floathi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (SFmode,
	operand1,
	gen_rtx_FLOAT (SFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3463 */
rtx
gen_divdf3_floathi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (DFmode,
	operand1,
	gen_rtx_FLOAT (DFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3463 */
rtx
gen_divxf3_floathi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (XFmode,
	operand1,
	gen_rtx_FLOAT (XFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3474 */
rtx
gen_divsf3_floatqi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (SFmode,
	operand1,
	gen_rtx_FLOAT (SFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3474 */
rtx
gen_divdf3_floatqi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (DFmode,
	operand1,
	gen_rtx_FLOAT (DFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3474 */
rtx
gen_divxf3_floatqi_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (XFmode,
	operand1,
	gen_rtx_FLOAT (XFmode,
	operand2)));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3485 */
rtx
gen_divsf3_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (SFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3485 */
rtx
gen_divdf3_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (DFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3485 */
rtx
gen_divxf3_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (XFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3500 */
rtx
gen_divsf3_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (SFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3500 */
rtx
gen_divdf3_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (DFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3599 */
rtx
gen_divmodhi4 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_DIV (HImode,
	operand1,
	operand2)),
		gen_rtx_SET (operand3,
	gen_rtx_MOD (HImode,
	copy_rtx (operand1),
	copy_rtx (operand2)))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3617 */
rtx
gen_udivmodhi4 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_UDIV (HImode,
	operand1,
	operand2)),
		gen_rtx_SET (operand3,
	gen_rtx_UMOD (HImode,
	copy_rtx (operand1),
	copy_rtx (operand2)))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3665 */
rtx
gen_andsi3_internal (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_AND (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3675 */
rtx
gen_andsi3_5200 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_AND (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3693 */
rtx
gen_andhi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_AND (HImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3717 */
rtx
gen_andqi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_AND (QImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3742 */
rtx
gen_iordi_zext (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_IOR (DImode,
	gen_rtx_ZERO_EXTEND (DImode,
	operand1),
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3773 */
rtx
gen_iorsi3_internal (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_IOR (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3783 */
rtx
gen_iorsi3_5200 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_IOR (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3793 */
rtx
gen_iorhi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_IOR (HImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3815 */
rtx
gen_iorqi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_IOR (QImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3842 */
rtx
gen_iorsi_zexthi_ashl16 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_IOR (SImode,
	gen_rtx_ZERO_EXTEND (SImode,
	operand1),
	gen_rtx_ASHIFT (SImode,
	operand2,
	const_int_rtx[MAX_SAVED_CONST_INT + (16)])));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3857 */
rtx
gen_iorsi_zext (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_IOR (SImode,
	gen_rtx_ZERO_EXTEND (SImode,
	operand1),
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3884 */
rtx
gen_xorsi3_internal (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_XOR (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3895 */
rtx
gen_xorsi3_5200 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_XOR (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3905 */
rtx
gen_xorhi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_XOR (HImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3929 */
rtx
gen_xorqi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_XOR (QImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3967 */
rtx
gen_negdi2_internal (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NEG (DImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3984 */
rtx
gen_negdi2_5200 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NEG (DImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4005 */
rtx
gen_negsi2_internal (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NEG (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4013 */
rtx
gen_negsi2_5200 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NEG (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4021 */
rtx
gen_neghi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NEG (HImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4035 */
rtx
gen_negqi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NEG (QImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4142 */
rtx
gen_negsf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NEG (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4142 */
rtx
gen_negdf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NEG (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4142 */
rtx
gen_negxf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NEG (XFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4157 */
rtx
gen_negsf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NEG (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4157 */
rtx
gen_negdf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NEG (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4180 */
rtx
gen_sqrtsf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SQRT (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4180 */
rtx
gen_sqrtdf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SQRT (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4180 */
rtx
gen_sqrtxf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SQRT (XFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4191 */
rtx
gen_sqrtsf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SQRT (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4191 */
rtx
gen_sqrtdf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SQRT (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4295 */
rtx
gen_abssf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ABS (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4295 */
rtx
gen_absdf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ABS (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4295 */
rtx
gen_absxf2_68881 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ABS (XFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4310 */
rtx
gen_abssf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ABS (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4310 */
rtx
gen_absdf2_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ABS (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4365 */
rtx
gen_one_cmplsi2_internal (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NOT (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4372 */
rtx
gen_one_cmplsi2_5200 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NOT (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4379 */
rtx
gen_one_cmplhi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NOT (HImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4393 */
rtx
gen_one_cmplqi2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_NOT (QImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4409 */
rtx
gen_ashldi_extsi (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ASHIFT (DImode,
	gen_rtx_fmt_e (GET_CODE (operand2), DImode,
		operand1),
	const_int_rtx[MAX_SAVED_CONST_INT + (32)]));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4422 */
rtx
gen_ashldi_sexthi (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_ASHIFT (DImode,
	gen_rtx_SIGN_EXTEND (DImode,
	operand1),
	const_int_rtx[MAX_SAVED_CONST_INT + (32)])),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4621 */
rtx
gen_ashlsi_16 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ASHIFT (SImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (16)]));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4633 */
rtx
gen_ashlsi_17_24 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ASHIFT (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4645 */
rtx
gen_ashlsi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ASHIFT (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4660 */
rtx
gen_ashlhi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ASHIFT (HImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4676 */
rtx
gen_ashlqi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ASHIFT (QImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4694 */
rtx
gen_ashrsi_16 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ASHIFTRT (SImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (16)]));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4715 */
rtx
gen_subreghi1ashrdi_const32 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SUBREG (HImode,
	gen_rtx_ASHIFTRT (DImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (32)]),
	6));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4727 */
rtx
gen_subregsi1ashrdi_const32 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SUBREG (SImode,
	gen_rtx_ASHIFTRT (DImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (32)]),
	4));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4843 */
rtx
gen_ashrdi_const (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ASHIFTRT (DImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4887 */
rtx
gen_ashrsi_31 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ASHIFTRT (SImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (31)]));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4896 */
rtx
gen_ashrsi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ASHIFTRT (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4906 */
rtx
gen_ashrhi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ASHIFTRT (HImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4922 */
rtx
gen_ashrqi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ASHIFTRT (QImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4969 */
rtx
gen_subreg1lshrdi_const32 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SUBREG (SImode,
	gen_rtx_LSHIFTRT (DImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (32)]),
	4));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5163 */
rtx
gen_lshrsi_31 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_LSHIFTRT (SImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (31)]));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5174 */
rtx
gen_lshrsi_16 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_LSHIFTRT (SImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (16)]));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5185 */
rtx
gen_lshrsi_17_24 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_LSHIFTRT (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5198 */
rtx
gen_lshrsi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_LSHIFTRT (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5208 */
rtx
gen_lshrhi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_LSHIFTRT (HImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5224 */
rtx
gen_lshrqi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_LSHIFTRT (QImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5242 */
rtx
gen_rotlsi_16 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ROTATE (SImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (16)]));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5251 */
rtx
gen_rotlsi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ROTATE (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5269 */
rtx
gen_rotlhi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ROTATE (HImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5301 */
rtx
gen_rotlqi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ROTATE (QImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5333 */
rtx
gen_rotrsi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ROTATERT (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5341 */
rtx
gen_rotrhi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ROTATERT (HImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5348 */
rtx
gen_rotrhi_lowpart (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	operand0),
	gen_rtx_ROTATERT (HImode,
	operand0,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5355 */
rtx
gen_rotrqi3 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ROTATERT (QImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5388 */
rtx
gen_bsetmemqi (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_IOR (QImode,
	gen_rtx_SUBREG (QImode,
	gen_rtx_ASHIFT (SImode,
	const1_rtx,
	operand1),
	3),
	operand0));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5439 */
rtx
gen_bclrmemqi (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (gen_rtx_ZERO_EXTRACT (SImode,
	operand0,
	const1_rtx,
	gen_rtx_MINUS (SImode,
	const_int_rtx[MAX_SAVED_CONST_INT + (7)],
	operand1)),
	const0_rtx);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5750 */
rtx
gen_scc0_di (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), VOIDmode,
		operand2,
		const0_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5759 */
rtx
gen_scc0_di_5200 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), VOIDmode,
		operand2,
		const0_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5768 */
rtx
gen_scc_di (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), VOIDmode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5778 */
rtx
gen_scc_di_5200 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), VOIDmode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5789 */
rtx
gen_jump (rtx operand0 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_LABEL_REF (VOIDmode,
	operand0));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6089 */
rtx
gen_blockage (void)
{
  return gen_rtx_UNSPEC_VOLATILE (VOIDmode,
	gen_rtvec (1,
		const0_rtx),
	0);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6095 */
rtx
gen_nop (void)
{
  return const0_rtx;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6249 */
rtx
gen_load_got (rtx operand0 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_UNSPEC (SImode,
	gen_rtvec (1,
		const0_rtx),
	3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6276 */
rtx
gen_indirect_jump (rtx operand0 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (pc_rtx,
	operand0);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6520 */
rtx
gen_extendsfxf2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT_EXTEND (XFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6548 */
rtx
gen_extenddfxf2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT_EXTEND (XFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6580 */
rtx
gen_truncxfdf2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT_TRUNCATE (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6595 */
rtx
gen_truncxfsf2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT_TRUNCATE (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6602 */
rtx
gen_sinsf2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_UNSPEC (SFmode,
	gen_rtvec (1,
		operand1),
	1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6602 */
rtx
gen_sindf2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_UNSPEC (DFmode,
	gen_rtvec (1,
		operand1),
	1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6602 */
rtx
gen_sinxf2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_UNSPEC (XFmode,
	gen_rtvec (1,
		operand1),
	1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6614 */
rtx
gen_cossf2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_UNSPEC (SFmode,
	gen_rtvec (1,
		operand1),
	2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6614 */
rtx
gen_cosdf2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_UNSPEC (DFmode,
	gen_rtvec (1,
		operand1),
	2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6614 */
rtx
gen_cosxf2 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (operand0,
	gen_rtx_UNSPEC (XFmode,
	gen_rtvec (1,
		operand1),
	2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6627 */
rtx
gen_trap (void)
{
  return gen_rtx_TRAP_IF (VOIDmode,
	constm1_rtx,
	const_int_rtx[MAX_SAVED_CONST_INT + (7)]);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6637 */
rtx
gen_ctrapqi4 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_TRAP_IF (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	operand3);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6637 */
rtx
gen_ctraphi4 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_TRAP_IF (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	operand3);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6637 */
rtx
gen_ctrapsi4 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_TRAP_IF (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	operand3);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6662 */
rtx
gen_ctrapqi4_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_TRAP_IF (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	operand3);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6662 */
rtx
gen_ctraphi4_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_TRAP_IF (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	operand3);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6662 */
rtx
gen_ctrapsi4_cf (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED)
{
  return gen_rtx_TRAP_IF (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	operand3);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6689 */
rtx
gen_stack_tie (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_SET (gen_rtx_MEM (BLKmode,
	gen_rtx_SCRATCH (VOIDmode)),
	gen_rtx_UNSPEC (BLKmode,
	gen_rtvec (2,
		operand0,
		operand1),
	5));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6701 */
rtx
gen_ib (void)
{
  return gen_rtx_UNSPEC (VOIDmode,
	gen_rtvec (1,
		const0_rtx),
	4);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:39 */
rtx
gen_atomic_compare_and_swapqi_1 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED,
	rtx operand4 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (3,
		gen_rtx_SET (operand1,
	gen_rtx_UNSPEC_VOLATILE (QImode,
	gen_rtvec (3,
		operand2,
		operand3,
		operand4),
	1)),
		gen_rtx_SET (copy_rtx (operand2),
	gen_rtx_UNSPEC_VOLATILE (QImode,
	gen_rtvec (3,
		copy_rtx (operand2),
		copy_rtx (operand3),
		copy_rtx (operand4)),
	2)),
		gen_rtx_SET (operand0,
	gen_rtx_UNSPEC_VOLATILE (QImode,
	gen_rtvec (3,
		copy_rtx (operand2),
		copy_rtx (operand3),
		copy_rtx (operand4)),
	2))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:39 */
rtx
gen_atomic_compare_and_swaphi_1 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED,
	rtx operand4 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (3,
		gen_rtx_SET (operand1,
	gen_rtx_UNSPEC_VOLATILE (HImode,
	gen_rtvec (3,
		operand2,
		operand3,
		operand4),
	1)),
		gen_rtx_SET (copy_rtx (operand2),
	gen_rtx_UNSPEC_VOLATILE (HImode,
	gen_rtvec (3,
		copy_rtx (operand2),
		copy_rtx (operand3),
		copy_rtx (operand4)),
	2)),
		gen_rtx_SET (operand0,
	gen_rtx_UNSPEC_VOLATILE (QImode,
	gen_rtvec (3,
		copy_rtx (operand2),
		copy_rtx (operand3),
		copy_rtx (operand4)),
	2))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:39 */
rtx
gen_atomic_compare_and_swapsi_1 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED,
	rtx operand2 ATTRIBUTE_UNUSED,
	rtx operand3 ATTRIBUTE_UNUSED,
	rtx operand4 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (3,
		gen_rtx_SET (operand1,
	gen_rtx_UNSPEC_VOLATILE (SImode,
	gen_rtvec (3,
		operand2,
		operand3,
		operand4),
	1)),
		gen_rtx_SET (copy_rtx (operand2),
	gen_rtx_UNSPEC_VOLATILE (SImode,
	gen_rtvec (3,
		copy_rtx (operand2),
		copy_rtx (operand3),
		copy_rtx (operand4)),
	2)),
		gen_rtx_SET (operand0,
	gen_rtx_UNSPEC_VOLATILE (QImode,
	gen_rtvec (3,
		copy_rtx (operand2),
		copy_rtx (operand3),
		copy_rtx (operand4)),
	2))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:72 */
rtx
gen_atomic_test_and_set_1 (rtx operand0 ATTRIBUTE_UNUSED,
	rtx operand1 ATTRIBUTE_UNUSED)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_UNSPEC_VOLATILE (QImode,
	gen_rtvec (1,
		operand1),
	3)),
		gen_rtx_SET (copy_rtx (operand1),
	gen_rtx_UNSPEC_VOLATILE (QImode,
	gen_rtvec (1,
		copy_rtx (operand1)),
	4))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:291 */
extern rtx_insn *gen_split_1 (rtx_insn *, rtx *);
rtx_insn *
gen_split_1 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands ATTRIBUTE_UNUSED)
{
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_1 (m68k.md:291)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 300 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  m68k_emit_move_double (operands);
  DONE;
}
#undef DONE
#undef FAIL
  emit_insn (const0_rtx);
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:306 */
extern rtx_insn *gen_split_2 (rtx_insn *, rtx *);
rtx_insn *
gen_split_2 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands ATTRIBUTE_UNUSED)
{
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_2 (m68k.md:306)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 313 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  m68k_emit_move_double (operands);
  DONE;
}
#undef DONE
#undef FAIL
  emit_insn (const0_rtx);
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:409 */
rtx
gen_cbranchdi4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[6];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
    operands[3] = operand3;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 420 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[0]);
  if ((code == GE || code == LT) && operands[2] == const0_rtx)
    {
      rtx xop1 = operand_subword_force (operands[1], 0, DImode);
      rtx xop2 = operand_subword_force (operands[2], 0, DImode);
      /* gen_cbranchsi4 won't use anything from operands[0] other than the
	 code.  */
      emit_jump_insn (gen_cbranchsi4 (operands[0], xop1, xop2, operands[3]));
      DONE;
    }
  if (code == EQ && operands[2] == const0_rtx)
    {
      emit_jump_insn (gen_beq0_di (operands[1], operands[3]));
      DONE;
    }
  if (code == NE && operands[2] == const0_rtx)
    {
      emit_jump_insn (gen_bne0_di (operands[1], operands[3]));
      DONE;
    }
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
    operand3 = operands[3];
    (void) operand3;
  }
  emit_jump_insn (gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (3,
		gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (DImode)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode)))));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:443 */
rtx
gen_cstoredi4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[4];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
    operands[3] = operand3;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 449 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx_code code = GET_CODE (operands[1]);
  if ((code == GE || code == LT) && operands[3] == const0_rtx)
    {
      rtx xop2 = operand_subword_force (operands[2], 0, DImode);
      rtx xop3 = operand_subword_force (operands[3], 0, DImode);
      /* gen_cstoresi4 won't use anything from operands[1] other than the
	 code.  */
      emit_jump_insn (gen_cstoresi4 (operands[0], operands[1], xop2, xop3));
      DONE;
    }
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
    operand3 = operands[3];
    (void) operand3;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:464 */
rtx
gen_cbranchqi4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:464 */
rtx
gen_cbranchhi4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:464 */
rtx
gen_cbranchsi4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:474 */
rtx
gen_cstoreqi4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:474 */
rtx
gen_cstorehi4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:474 */
rtx
gen_cstoresi4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  return gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:717 */
rtx
gen_cbranchsf4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:717 */
rtx
gen_cbranchdf4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:717 */
rtx
gen_cbranchxf4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  return gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand0), VOIDmode,
		operand1,
		operand2),
	gen_rtx_LABEL_REF (VOIDmode,
	operand3),
	pc_rtx));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:729 */
rtx
gen_cstoresf4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[4];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
    operands[3] = operand3;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 735 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
if (TARGET_COLDFIRE && operands[2] != const0_rtx)
     FAIL;
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
    operand3 = operands[3];
    (void) operand3;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:729 */
rtx
gen_cstoredf4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[4];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
    operands[3] = operand3;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 735 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
if (TARGET_COLDFIRE && operands[2] != const0_rtx)
     FAIL;
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
    operand3 = operands[3];
    (void) operand3;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:729 */
rtx
gen_cstorexf4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[4];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
    operands[3] = operand3;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 735 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
if (TARGET_COLDFIRE && operands[2] != const0_rtx)
     FAIL;
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
    operand3 = operands[3];
    (void) operand3;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand1), QImode,
		operand2,
		operand3)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:914 */
rtx
gen_movsi (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 918 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx tmp, base, offset;

  /* Recognize the case where operand[1] is a reference to thread-local
     data and load its address to a register.  */
  if (!TARGET_PCREL && m68k_tls_reference_p (operands[1], false))
    {
      rtx tmp = operands[1];
      rtx addend = NULL;

      if (GET_CODE (tmp) == CONST && GET_CODE (XEXP (tmp, 0)) == PLUS)
        {
          addend = XEXP (XEXP (tmp, 0), 1);
          tmp = XEXP (XEXP (tmp, 0), 0);
        }

      gcc_assert (GET_CODE (tmp) == SYMBOL_REF);
      gcc_assert (SYMBOL_REF_TLS_MODEL (tmp) != 0);

      tmp = m68k_legitimize_tls_address (tmp);

      if (addend)
        {
	  if (!REG_P (tmp))
	    {
	      rtx reg;

	      reg = gen_reg_rtx (Pmode);
	      emit_move_insn (reg, tmp);
	      tmp = reg;
	    }

          tmp = gen_rtx_PLUS (SImode, tmp, addend);
	}

      operands[1] = tmp;
    }
  else if (flag_pic && !TARGET_PCREL && symbolic_operand (operands[1], SImode))
    {
      /* The source is an address which requires PIC relocation.
         Call legitimize_pic_address with the source, mode, and a relocation
         register (a new pseudo, or the final destination if reload_in_progress
         is set).   Then fall through normally */
      rtx temp = reload_in_progress ? operands[0] : gen_reg_rtx (Pmode);
      operands[1] = legitimize_pic_address (operands[1], SImode, temp);
    }
  else if (flag_pic && TARGET_PCREL && ! reload_in_progress)
    {
      /* Don't allow writes to memory except via a register;
	 the m68k doesn't consider PC-relative addresses to be writable.  */
      if (symbolic_operand (operands[0], SImode))
	operands[0] = force_reg (SImode, XEXP (operands[0], 0));
      else if (GET_CODE (operands[0]) == MEM
	       && symbolic_operand (XEXP (operands[0], 0), SImode))
	operands[0] = gen_rtx_MEM (SImode,
			       force_reg (SImode, XEXP (operands[0], 0)));
    }
  if (M68K_OFFSETS_MUST_BE_WITHIN_SECTIONS_P)
    {
      split_const (operands[1], &base, &offset);
      if (GET_CODE (base) == SYMBOL_REF
	  && !offset_within_block_p (base, INTVAL (offset)))
	{
	  tmp = !can_create_pseudo_p () ? operands[0] : gen_reg_rtx (SImode);
	  emit_move_insn (tmp, base);
	  emit_insn (gen_addsi3 (operands[0], tmp, offset));
	  DONE;
	}
    }
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1078 */
rtx
gen_movhi (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	operand1);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1104 */
rtx
gen_movstricthi (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	operand0),
	operand1);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1124 */
rtx
gen_movqi (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	operand1);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1144 */
rtx
gen_movstrictqi (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	operand0),
	operand1);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1168 */
rtx
gen_pushqi1 (rtx operand0)
{
  rtx_insn *_val = 0;
  start_sequence ();
  emit_insn (gen_rtx_SET (gen_rtx_REG (SImode,
	15),
	gen_rtx_PLUS (SImode,
	gen_rtx_REG (SImode,
	15),
	const_int_rtx[MAX_SAVED_CONST_INT + (-2)])));
  emit_insn (gen_rtx_SET (gen_rtx_MEM (QImode,
	gen_rtx_PLUS (SImode,
	gen_rtx_REG (SImode,
	15),
	const1_rtx)),
	operand0));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1175 */
rtx
gen_reload_insf (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[3];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1180 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (emit_move_sequence (operands, SFmode, operands[2]))
    DONE;

  /* We don't want the clobber emitted, so handle this ourselves. */
  emit_insn (gen_rtx_SET (operands[0], operands[1]));
  DONE;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
  }
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  emit_insn (gen_rtx_CLOBBER (VOIDmode,
	operand2));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1189 */
rtx
gen_reload_outsf (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[3];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1194 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (emit_move_sequence (operands, SFmode, operands[2]))
    DONE;

  /* We don't want the clobber emitted, so handle this ourselves. */
  emit_insn (gen_rtx_SET (operands[0], operands[1]));
  DONE;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
  }
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  emit_insn (gen_rtx_CLOBBER (VOIDmode,
	operand2));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1203 */
rtx
gen_movsf (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	operand1);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1317 */
rtx
gen_reload_indf (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[3];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1322 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (emit_move_sequence (operands, DFmode, operands[2]))
    DONE;

  /* We don't want the clobber emitted, so handle this ourselves. */
  emit_insn (gen_rtx_SET (operands[0], operands[1]));
  DONE;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
  }
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  emit_insn (gen_rtx_CLOBBER (VOIDmode,
	operand2));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1331 */
rtx
gen_reload_outdf (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[3];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1336 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (emit_move_sequence (operands, DFmode, operands[2]))
    DONE;

  /* We don't want the clobber emitted, so handle this ourselves. */
  emit_insn (gen_rtx_SET (operands[0], operands[1]));
  DONE;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
  }
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  emit_insn (gen_rtx_CLOBBER (VOIDmode,
	operand2));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1345 */
rtx
gen_movdf (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1349 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (TARGET_COLDFIRE_FPU)
    if (emit_move_sequence (operands, DFmode, 0))
      DONE;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1393 */
extern rtx_insn *gen_split_3 (rtx_insn *, rtx *);
rtx_insn *
gen_split_3 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands ATTRIBUTE_UNUSED)
{
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_3 (m68k.md:1393)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1400 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  m68k_emit_move_double (operands);
  DONE;
}
#undef DONE
#undef FAIL
  emit_insn (const0_rtx);
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1449 */
rtx
gen_movxf (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1453 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  /* We can't rewrite operands during reload.  */
  if (! reload_in_progress)
    {
      if (CONSTANT_P (operands[1]))
	{
	  operands[1] = force_const_mem (XFmode, operands[1]);
	  if (! memory_address_p (XFmode, XEXP (operands[1], 0)))
	    operands[1] = adjust_address (operands[1], XFmode, 0);
	}
      if (flag_pic && TARGET_PCREL)
	{
	  /* Don't allow writes to memory except via a register; the
	     m68k doesn't consider PC-relative addresses to be writable.  */
	  if (GET_CODE (operands[0]) == MEM
	      && symbolic_operand (XEXP (operands[0], 0), SImode))
	    operands[0] = gen_rtx_MEM (XFmode,
				   force_reg (SImode, XEXP (operands[0], 0)));
	}
    }
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1560 */
rtx
gen_movdi (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	operand1);
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1689 */
extern rtx_insn *gen_split_4 (rtx_insn *, rtx *);
rtx_insn *
gen_split_4 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_4 (m68k.md:1689)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1701 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = adjust_address (operands[0], GET_MODE (operands[1]), 0);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	const0_rtx));
  emit_insn (gen_rtx_SET (copy_rtx (operand0),
	operand1));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1705 */
extern rtx_insn *gen_split_5 (rtx_insn *, rtx *);
rtx_insn *
gen_split_5 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_5 (m68k.md:1705)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1718 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = adjust_address (operands[0], GET_MODE (operands[1]), 0);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  emit_insn (gen_rtx_SET (copy_rtx (operand0),
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1722 */
extern rtx_insn *gen_split_6 (rtx_insn *, rtx *);
rtx_insn *
gen_split_6 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_6 (m68k.md:1722)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1732 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_lowpart (SImode, operands[0]);
  operands[3] = gen_highpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  emit_insn (gen_rtx_SET (operand2,
	gen_rtx_ZERO_EXTEND (SImode,
	operand1)));
  emit_insn (gen_rtx_SET (operand3,
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1737 */
extern rtx_insn *gen_split_7 (rtx_insn *, rtx *);
rtx_insn *
gen_split_7 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_7 (m68k.md:1737)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1747 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_lowpart (SImode, operands[0]);
  operands[3] = gen_highpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  emit_insn (gen_rtx_SET (operand2,
	gen_rtx_ZERO_EXTEND (SImode,
	operand1)));
  emit_insn (gen_rtx_SET (operand3,
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1752 */
rtx
gen_zero_extendsidi2 (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1756 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (GET_CODE (operands[0]) == MEM
      && GET_CODE (operands[1]) == MEM)
    operands[1] = force_reg (SImode, operands[1]);
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_ZERO_EXTEND (DImode,
	operand1)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1762 */
extern rtx_insn *gen_split_8 (rtx_insn *, rtx *);
rtx_insn *
gen_split_8 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_8 (m68k.md:1762)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1772 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_lowpart (SImode, operands[0]);
  operands[3] = gen_highpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  emit_insn (gen_rtx_SET (operand2,
	operand1));
  emit_insn (gen_rtx_SET (operand3,
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1790 */
rtx
gen_zero_extendqihi2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ZERO_EXTEND (HImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1817 */
extern rtx_insn *gen_split_9 (rtx_insn *, rtx *);
rtx_insn *
gen_split_9 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_9 (m68k.md:1817)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1827 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_lowpart (GET_MODE (operands[1]), operands[0]);
  operands[3] = GEN_INT (GET_MODE_MASK (GET_MODE (operands[1])));
  operands[4] = gen_rtx_AND (GET_MODE (operands[0]), operands[0], operands[3]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  operand4 = operands[4];
  (void) operand4;
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	operand2),
	operand1));
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_fmt_ee (GET_CODE (operand4), GET_MODE (operand4),
		copy_rtx (operand0),
		operand3)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1833 */
extern rtx_insn *gen_split_10 (rtx_insn *, rtx *);
rtx_insn *
gen_split_10 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_10 (m68k.md:1833)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 1841 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_lowpart (GET_MODE (operands[1]), operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  emit_insn (gen_rtx_SET (operand0,
	const0_rtx));
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	operand2),
	operand1));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1947 */
rtx
gen_extendhisi2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SIGN_EXTEND (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:1979 */
rtx
gen_extendqisi2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SIGN_EXTEND (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2001 */
rtx
gen_extendsfdf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT_EXTEND (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2054 */
rtx
gen_truncdfsf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT_TRUNCATE (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2099 */
rtx
gen_floatsisf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2099 */
rtx
gen_floatsidf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2099 */
rtx
gen_floatsixf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (XFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2120 */
rtx
gen_floathisf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2120 */
rtx
gen_floathidf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2120 */
rtx
gen_floathixf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (XFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2141 */
rtx
gen_floatqisf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2141 */
rtx
gen_floatqidf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2141 */
rtx
gen_floatqixf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FLOAT (XFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2200 */
rtx
gen_ftruncsf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2200 */
rtx
gen_ftruncdf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2200 */
rtx
gen_ftruncxf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (XFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2230 */
rtx
gen_fixsfqi2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (QImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2230 */
rtx
gen_fixdfqi2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (QImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2230 */
rtx
gen_fixxfqi2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (QImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2250 */
rtx
gen_fixsfhi2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (HImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2250 */
rtx
gen_fixdfhi2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (HImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2250 */
rtx
gen_fixxfhi2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (HImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2270 */
rtx
gen_fixsfsi2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2270 */
rtx
gen_fixdfsi2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2270 */
rtx
gen_fixxfsi2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_FIX (SImode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2491 */
rtx
gen_addsi3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2512 */
extern rtx_insn *gen_split_11 (rtx_insn *, rtx *);
rtx_insn *
gen_split_11 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_11 (m68k.md:2512)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 2556 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"

#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  emit_insn (gen_rtx_SET (operand0,
	operand2));
  emit_insn (gen_rtx_SET (copy_rtx (operand0),
	gen_rtx_PLUS (SImode,
	copy_rtx (operand0),
	operand1)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2796 */
rtx
gen_addsf3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (SFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2796 */
rtx
gen_adddf3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (DFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:2796 */
rtx
gen_addxf3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_PLUS (XFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3027 */
rtx
gen_subsf3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (SFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3027 */
rtx
gen_subdf3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (DFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3027 */
rtx
gen_subxf3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MINUS (XFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3125 */
rtx
gen_mulsi3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3179 */
rtx
gen_umulsidi3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (gen_rtx_SUBREG (SImode,
	operand0,
	4),
	gen_rtx_MULT (SImode,
	operand1,
	operand2)),
		gen_rtx_SET (gen_rtx_SUBREG (SImode,
	operand0,
	0),
	gen_rtx_TRUNCATE (SImode,
	gen_rtx_LSHIFTRT (DImode,
	gen_rtx_MULT (DImode,
	gen_rtx_ZERO_EXTEND (DImode,
	operand1),
	gen_rtx_ZERO_EXTEND (DImode,
	operand2)),
	const_int_rtx[MAX_SAVED_CONST_INT + (32)])))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3218 */
rtx
gen_mulsidi3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (gen_rtx_SUBREG (SImode,
	operand0,
	4),
	gen_rtx_MULT (SImode,
	operand1,
	operand2)),
		gen_rtx_SET (gen_rtx_SUBREG (SImode,
	operand0,
	0),
	gen_rtx_TRUNCATE (SImode,
	gen_rtx_LSHIFTRT (DImode,
	gen_rtx_MULT (DImode,
	gen_rtx_SIGN_EXTEND (DImode,
	operand1),
	gen_rtx_SIGN_EXTEND (DImode,
	operand2)),
	const_int_rtx[MAX_SAVED_CONST_INT + (32)])))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3252 */
rtx
gen_umulsi3_highpart (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  rtx operand3;
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[4];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 3262 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[3] = gen_reg_rtx (SImode);

  if (GET_CODE (operands[2]) == CONST_INT)
    {
      operands[2] = immed_double_const (INTVAL (operands[2]) & 0xffffffff,
					0, DImode);

      /* We have to adjust the operand order for the matching constraints.  */
      emit_insn (gen_const_umulsi3_highpart (operands[0], operands[3],
					     operands[1], operands[2]));
      DONE;
    }
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
    operand3 = operands[3];
    (void) operand3;
  }
  emit (gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_TRUNCATE (SImode,
	gen_rtx_LSHIFTRT (DImode,
	gen_rtx_MULT (DImode,
	gen_rtx_ZERO_EXTEND (DImode,
	operand1),
	gen_rtx_ZERO_EXTEND (DImode,
	operand2)),
	const_int_rtx[MAX_SAVED_CONST_INT + (32)]))),
		gen_rtx_CLOBBER (VOIDmode,
	operand3))), false);
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3299 */
rtx
gen_smulsi3_highpart (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  rtx operand3;
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[4];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 3309 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[3] = gen_reg_rtx (SImode);
  if (GET_CODE (operands[2]) == CONST_INT)
    {
      /* We have to adjust the operand order for the matching constraints.  */
      emit_insn (gen_const_smulsi3_highpart (operands[0], operands[3],
					     operands[1], operands[2]));
      DONE;
    }
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
    operand3 = operands[3];
    (void) operand3;
  }
  emit (gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_TRUNCATE (SImode,
	gen_rtx_LSHIFTRT (DImode,
	gen_rtx_MULT (DImode,
	gen_rtx_SIGN_EXTEND (DImode,
	operand1),
	gen_rtx_SIGN_EXTEND (DImode,
	operand2)),
	const_int_rtx[MAX_SAVED_CONST_INT + (32)]))),
		gen_rtx_CLOBBER (VOIDmode,
	operand3))), false);
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3342 */
rtx
gen_mulsf3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (SFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3342 */
rtx
gen_muldf3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (DFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3342 */
rtx
gen_mulxf3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_MULT (XFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3445 */
rtx
gen_divsf3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (SFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3445 */
rtx
gen_divdf3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (DFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3445 */
rtx
gen_divxf3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_DIV (XFmode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3515 */
rtx
gen_divmodsi4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_DIV (SImode,
	operand1,
	operand2)),
		gen_rtx_SET (operand3,
	gen_rtx_MOD (SImode,
	operand1,
	operand2))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3557 */
rtx
gen_udivmodsi4 (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	gen_rtx_UDIV (SImode,
	operand1,
	operand2)),
		gen_rtx_SET (operand3,
	gen_rtx_UMOD (SImode,
	operand1,
	operand2))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3648 */
rtx
gen_andsi3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_AND (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3766 */
rtx
gen_iorsi3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_IOR (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3877 */
rtx
gen_xorsi3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  return gen_rtx_SET (operand0,
	gen_rtx_XOR (SImode,
	operand1,
	operand2));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3955 */
rtx
gen_negdi2 (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 3959 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (TARGET_COLDFIRE)
    emit_insn (gen_negdi2_5200 (operands[0], operands[1]));
  else
    emit_insn (gen_negdi2_internal (operands[0], operands[1]));
  DONE;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_NEG (DImode,
	operand1)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:3993 */
rtx
gen_negsi2 (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 3997 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (TARGET_COLDFIRE)
    emit_insn (gen_negsi2_5200 (operands[0], operands[1]));
  else
    emit_insn (gen_negsi2_internal (operands[0], operands[1]));
  DONE;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_NEG (SImode,
	operand1)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4051 */
rtx
gen_negsf2 (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4055 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (!TARGET_HARD_FLOAT)
    {
      rtx result;
      rtx target;

      target = operand_subword_force (operands[0], 0, SFmode);
      result = expand_binop (SImode, xor_optab,
			     operand_subword_force (operands[1], 0, SFmode),
			     GEN_INT (-2147483647 - 1), target, 0, OPTAB_WIDEN);
      gcc_assert (result);

      if (result != target)
	emit_move_insn (result, target);

      /* Make a place for REG_EQUAL.  */
      emit_move_insn (operands[0], operands[0]);
      DONE;
    }
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_NEG (SFmode,
	operand1)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4076 */
rtx
gen_negdf2 (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4080 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (!TARGET_HARD_FLOAT)
    {
      rtx result;
      rtx target;
      rtx insns;

      start_sequence ();
      target = operand_subword (operands[0], 0, 1, DFmode);
      result = expand_binop (SImode, xor_optab,
			     operand_subword_force (operands[1], 0, DFmode),
			     GEN_INT (-2147483647 - 1), target, 0, OPTAB_WIDEN);
      gcc_assert (result);

      if (result != target)
	emit_move_insn (result, target);

      emit_move_insn (operand_subword (operands[0], 1, 1, DFmode),
		      operand_subword_force (operands[1], 1, DFmode));

      insns = get_insns ();
      end_sequence ();

      emit_insn (insns);
      DONE;
    }
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_NEG (DFmode,
	operand1)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4108 */
rtx
gen_negxf2 (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4112 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (!TARGET_68881)
    {
      rtx result;
      rtx target;
      rtx insns;

      start_sequence ();
      target = operand_subword (operands[0], 0, 1, XFmode);
      result = expand_binop (SImode, xor_optab,
			     operand_subword_force (operands[1], 0, XFmode),
			     GEN_INT (-2147483647 - 1), target, 0, OPTAB_WIDEN);
      gcc_assert (result);

      if (result != target)
	emit_move_insn (result, target);

      emit_move_insn (operand_subword (operands[0], 1, 1, XFmode),
		      operand_subword_force (operands[1], 1, XFmode));
      emit_move_insn (operand_subword (operands[0], 2, 1, XFmode),
		      operand_subword_force (operands[1], 2, XFmode));

      insns = get_insns ();
      end_sequence ();

      emit_insn (insns);
      DONE;
    }
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_NEG (XFmode,
	operand1)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4174 */
rtx
gen_sqrtsf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SQRT (SFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4174 */
rtx
gen_sqrtdf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SQRT (DFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4174 */
rtx
gen_sqrtxf2 (rtx operand0,
	rtx operand1)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SQRT (XFmode,
	operand1));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4204 */
rtx
gen_abssf2 (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4208 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (!TARGET_HARD_FLOAT)
    {
      rtx result;
      rtx target;

      target = operand_subword_force (operands[0], 0, SFmode);
      result = expand_binop (SImode, and_optab,
			     operand_subword_force (operands[1], 0, SFmode),
			     GEN_INT (0x7fffffff), target, 0, OPTAB_WIDEN);
      gcc_assert (result);

      if (result != target)
	emit_move_insn (result, target);

      /* Make a place for REG_EQUAL.  */
      emit_move_insn (operands[0], operands[0]);
      DONE;
    }
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_ABS (SFmode,
	operand1)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4229 */
rtx
gen_absdf2 (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4233 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (!TARGET_HARD_FLOAT)
    {
      rtx result;
      rtx target;
      rtx insns;

      start_sequence ();
      target = operand_subword (operands[0], 0, 1, DFmode);
      result = expand_binop (SImode, and_optab,
			     operand_subword_force (operands[1], 0, DFmode),
			     GEN_INT (0x7fffffff), target, 0, OPTAB_WIDEN);
      gcc_assert (result);

      if (result != target)
	emit_move_insn (result, target);

      emit_move_insn (operand_subword (operands[0], 1, 1, DFmode),
		      operand_subword_force (operands[1], 1, DFmode));

      insns = get_insns ();
      end_sequence ();

      emit_insn (insns);
      DONE;
    }
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_ABS (DFmode,
	operand1)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4261 */
rtx
gen_absxf2 (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4265 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (!TARGET_68881)
    {
      rtx result;
      rtx target;
      rtx insns;

      start_sequence ();
      target = operand_subword (operands[0], 0, 1, XFmode);
      result = expand_binop (SImode, and_optab,
			     operand_subword_force (operands[1], 0, XFmode),
			     GEN_INT (0x7fffffff), target, 0, OPTAB_WIDEN);
      gcc_assert (result);

      if (result != target)
	emit_move_insn (result, target);

      emit_move_insn (operand_subword (operands[0], 1, 1, XFmode),
		      operand_subword_force (operands[1], 1, XFmode));
      emit_move_insn (operand_subword (operands[0], 2, 1, XFmode),
		      operand_subword_force (operands[1], 2, XFmode));

      insns = get_insns ();
      end_sequence ();

      emit_insn (insns);
      DONE;
    }
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_ABS (XFmode,
	operand1)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4328 */
rtx
gen_clzsi2 (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL _Pragma ("GCC error \"clzsi2 cannot FAIL\"") (void)0
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4332 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (ISA_HAS_FF1)
    operands[1] = force_reg (SImode, operands[1]);
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_CLZ (SImode,
	operand1)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4353 */
rtx
gen_one_cmplsi2 (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4357 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  if (TARGET_COLDFIRE)
    emit_insn (gen_one_cmplsi2_5200 (operands[0], operands[1]));
  else
    emit_insn (gen_one_cmplsi2_internal (operands[0], operands[1]));
  DONE;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_NOT (SImode,
	operand1)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4409 */
extern rtx_insn *gen_split_12 (rtx_insn *, rtx *);
rtx_insn *
gen_split_12 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_12 (m68k.md:4409)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4420 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
split_di(operands, 1, operands + 2, operands + 3);
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  emit_insn (gen_rtx_SET (operand3,
	operand1));
  emit_insn (gen_rtx_SET (operand2,
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4454 */
extern rtx_insn *gen_split_13 (rtx_insn *, rtx *);
rtx_insn *
gen_split_13 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_13 (m68k.md:4454)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4463 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"

#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_ASHIFT (DImode,
	operand1,
	const1_rtx)));
  emit_insn (gen_rtx_SET (copy_rtx (operand0),
	gen_rtx_ASHIFT (DImode,
	copy_rtx (operand0),
	const1_rtx)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4465 */
extern rtx_insn *gen_split_14 (rtx_insn *, rtx *);
rtx_insn *
gen_split_14 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_14 (m68k.md:4465)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4474 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"

#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_ASHIFT (DImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (2)])));
  emit_insn (gen_rtx_SET (copy_rtx (operand0),
	gen_rtx_ASHIFT (DImode,
	copy_rtx (operand0),
	const1_rtx)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4476 */
extern rtx_insn *gen_split_15 (rtx_insn *, rtx *);
rtx_insn *
gen_split_15 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_15 (m68k.md:4476)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4489 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_highpart (SImode, operands[0]);
  operands[3] = gen_lowpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  emit_insn (gen_rtx_SET (operand2,
	gen_rtx_ROTATE (SImode,
	copy_rtx (operand2),
	const_int_rtx[MAX_SAVED_CONST_INT + (8)])));
  emit_insn (gen_rtx_SET (operand3,
	gen_rtx_ROTATE (SImode,
	copy_rtx (operand3),
	const_int_rtx[MAX_SAVED_CONST_INT + (8)])));
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	gen_rtx_SUBREG (QImode,
	operand0,
	3)),
	gen_rtx_SUBREG (QImode,
	copy_rtx (operand0),
	7)));
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	gen_rtx_SUBREG (QImode,
	copy_rtx (operand0),
	7)),
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4494 */
extern rtx_insn *gen_split_16 (rtx_insn *, rtx *);
rtx_insn *
gen_split_16 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_16 (m68k.md:4494)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4507 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_highpart (SImode, operands[0]);
  operands[3] = gen_lowpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  emit_insn (gen_rtx_SET (operand2,
	gen_rtx_ROTATE (SImode,
	copy_rtx (operand2),
	const_int_rtx[MAX_SAVED_CONST_INT + (16)])));
  emit_insn (gen_rtx_SET (operand3,
	gen_rtx_ROTATE (SImode,
	copy_rtx (operand3),
	const_int_rtx[MAX_SAVED_CONST_INT + (16)])));
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	gen_rtx_SUBREG (HImode,
	operand0,
	2)),
	gen_rtx_SUBREG (HImode,
	copy_rtx (operand0),
	6)));
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	gen_rtx_SUBREG (HImode,
	copy_rtx (operand0),
	6)),
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4512 */
extern rtx_insn *gen_split_17 (rtx_insn *, rtx *);
rtx_insn *
gen_split_17 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_17 (m68k.md:4512)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4519 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = adjust_address(operands[0], SImode, 0);
  operands[1] = gen_lowpart(SImode, operands[1]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	const0_rtx));
  emit_insn (gen_rtx_SET (copy_rtx (operand0),
	operand1));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4524 */
extern rtx_insn *gen_split_18 (rtx_insn *, rtx *);
rtx_insn *
gen_split_18 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_18 (m68k.md:4524)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4531 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = adjust_address(operands[0], SImode, 0);
  operands[1] = gen_lowpart(SImode, operands[1]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  emit_insn (gen_rtx_SET (copy_rtx (operand0),
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4536 */
extern rtx_insn *gen_split_19 (rtx_insn *, rtx *);
rtx_insn *
gen_split_19 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_19 (m68k.md:4536)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4545 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
split_di(operands, 2, operands + 2, operands + 4);
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  operand4 = operands[4];
  (void) operand4;
  emit_insn (gen_rtx_SET (operand4,
	operand3));
  emit_insn (gen_rtx_SET (operand2,
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4547 */
extern rtx_insn *gen_split_20 (rtx_insn *, rtx *);
rtx_insn *
gen_split_20 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_20 (m68k.md:4547)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4556 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = GEN_INT (INTVAL (operands[2]) - 32);
  operands[3] = gen_highpart (SImode, operands[0]);
  operands[4] = gen_lowpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  operand4 = operands[4];
  (void) operand4;
  emit_insn (gen_rtx_SET (operand4,
	gen_rtx_ASHIFT (SImode,
	copy_rtx (operand4),
	operand2)));
  emit_insn (gen_rtx_SET (operand3,
	copy_rtx (operand4)));
  emit_insn (gen_rtx_SET (copy_rtx (operand4),
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4562 */
extern rtx_insn *gen_split_21 (rtx_insn *, rtx *);
rtx_insn *
gen_split_21 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_21 (m68k.md:4562)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4573 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_highpart (SImode, operands[0]);
  operands[3] = gen_lowpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  emit_insn (gen_rtx_SET (operand2,
	operand3));
  emit_insn (gen_rtx_SET (copy_rtx (operand2),
	gen_rtx_ROTATE (SImode,
	copy_rtx (operand2),
	const_int_rtx[MAX_SAVED_CONST_INT + (16)])));
  emit_insn (gen_rtx_SET (copy_rtx (operand3),
	const0_rtx));
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	gen_rtx_SUBREG (HImode,
	operand0,
	2)),
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4578 */
extern rtx_insn *gen_split_22 (rtx_insn *, rtx *);
rtx_insn *
gen_split_22 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_22 (m68k.md:4578)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4588 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = GEN_INT (INTVAL (operands[2]) - 32);
  operands[3] = gen_highpart (SImode, operands[0]);
  operands[4] = gen_lowpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  operand4 = operands[4];
  (void) operand4;
  emit_insn (gen_rtx_SET (operand3,
	operand2));
  emit_insn (gen_rtx_SET (operand4,
	gen_rtx_ASHIFT (SImode,
	copy_rtx (operand4),
	copy_rtx (operand3))));
  emit_insn (gen_rtx_SET (copy_rtx (operand3),
	copy_rtx (operand4)));
  emit_insn (gen_rtx_SET (copy_rtx (operand4),
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4604 */
rtx
gen_ashldi3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[3];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4609 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  /* ???  This is a named pattern like this is not allowed to FAIL based
     on its operands.  */
  if (GET_CODE (operands[2]) != CONST_INT
      || ((INTVAL (operands[2]) < 1 || INTVAL (operands[2]) > 3)
	  && INTVAL (operands[2]) != 8 && INTVAL (operands[2]) != 16
	  && (INTVAL (operands[2]) < 32 || INTVAL (operands[2]) > 63)))
    FAIL;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_ASHIFT (DImode,
	operand1,
	operand2)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4747 */
extern rtx_insn *gen_split_23 (rtx_insn *, rtx *);
rtx_insn *
gen_split_23 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_23 (m68k.md:4747)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4756 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"

#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_ASHIFTRT (DImode,
	operand1,
	const1_rtx)));
  emit_insn (gen_rtx_SET (copy_rtx (operand0),
	gen_rtx_ASHIFTRT (DImode,
	copy_rtx (operand0),
	const1_rtx)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4758 */
extern rtx_insn *gen_split_24 (rtx_insn *, rtx *);
rtx_insn *
gen_split_24 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_24 (m68k.md:4758)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4767 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"

#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_ASHIFTRT (DImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (2)])));
  emit_insn (gen_rtx_SET (copy_rtx (operand0),
	gen_rtx_ASHIFTRT (DImode,
	copy_rtx (operand0),
	const1_rtx)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4769 */
extern rtx_insn *gen_split_25 (rtx_insn *, rtx *);
rtx_insn *
gen_split_25 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_25 (m68k.md:4769)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4780 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_highpart (SImode, operands[0]);
  operands[3] = gen_lowpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	gen_rtx_SUBREG (QImode,
	operand0,
	7)),
	gen_rtx_SUBREG (QImode,
	copy_rtx (operand0),
	3)));
  emit_insn (gen_rtx_SET (operand2,
	gen_rtx_ASHIFTRT (SImode,
	copy_rtx (operand2),
	const_int_rtx[MAX_SAVED_CONST_INT + (8)])));
  emit_insn (gen_rtx_SET (operand3,
	gen_rtx_ROTATERT (SImode,
	copy_rtx (operand3),
	const_int_rtx[MAX_SAVED_CONST_INT + (8)])));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4785 */
extern rtx_insn *gen_split_26 (rtx_insn *, rtx *);
rtx_insn *
gen_split_26 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_26 (m68k.md:4785)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4798 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_highpart (SImode, operands[0]);
  operands[3] = gen_lowpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	gen_rtx_SUBREG (HImode,
	operand0,
	6)),
	gen_rtx_SUBREG (HImode,
	copy_rtx (operand0),
	2)));
  emit_insn (gen_rtx_SET (operand2,
	gen_rtx_ROTATE (SImode,
	copy_rtx (operand2),
	const_int_rtx[MAX_SAVED_CONST_INT + (16)])));
  emit_insn (gen_rtx_SET (operand3,
	gen_rtx_ROTATE (SImode,
	copy_rtx (operand3),
	const_int_rtx[MAX_SAVED_CONST_INT + (16)])));
  emit_insn (gen_rtx_SET (copy_rtx (operand2),
	gen_rtx_SIGN_EXTEND (SImode,
	gen_rtx_SUBREG (HImode,
	copy_rtx (operand2),
	2))));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4831 */
extern rtx_insn *gen_split_27 (rtx_insn *, rtx *);
rtx_insn *
gen_split_27 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_27 (m68k.md:4831)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4840 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
split_di(operands, 1, operands + 2, operands + 3);
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  emit_insn (gen_rtx_SET (operand3,
	gen_rtx_ASHIFTRT (SImode,
	copy_rtx (operand3),
	const_int_rtx[MAX_SAVED_CONST_INT + (31)])));
  emit_insn (gen_rtx_SET (operand2,
	copy_rtx (operand3)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4870 */
rtx
gen_ashrdi3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[3];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4875 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  /* ???  This is a named pattern like this is not allowed to FAIL based
     on its operands.  */
  if (GET_CODE (operands[2]) != CONST_INT
      || ((INTVAL (operands[2]) < 1 || INTVAL (operands[2]) > 3)
	  && INTVAL (operands[2]) != 8 && INTVAL (operands[2]) != 16
	  && (INTVAL (operands[2]) < 31 || INTVAL (operands[2]) > 63)))
    FAIL;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_ASHIFTRT (DImode,
	operand1,
	operand2)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4986 */
extern rtx_insn *gen_split_28 (rtx_insn *, rtx *);
rtx_insn *
gen_split_28 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_28 (m68k.md:4986)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 4995 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"

#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_LSHIFTRT (DImode,
	operand1,
	const1_rtx)));
  emit_insn (gen_rtx_SET (copy_rtx (operand0),
	gen_rtx_LSHIFTRT (DImode,
	copy_rtx (operand0),
	const1_rtx)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:4997 */
extern rtx_insn *gen_split_29 (rtx_insn *, rtx *);
rtx_insn *
gen_split_29 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_29 (m68k.md:4997)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5006 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"

#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_LSHIFTRT (DImode,
	operand1,
	const_int_rtx[MAX_SAVED_CONST_INT + (2)])));
  emit_insn (gen_rtx_SET (copy_rtx (operand0),
	gen_rtx_LSHIFTRT (DImode,
	copy_rtx (operand0),
	const1_rtx)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5008 */
extern rtx_insn *gen_split_30 (rtx_insn *, rtx *);
rtx_insn *
gen_split_30 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_30 (m68k.md:5008)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5019 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_highpart (SImode, operands[0]);
  operands[3] = gen_lowpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	gen_rtx_SUBREG (QImode,
	operand0,
	7)),
	gen_rtx_SUBREG (QImode,
	copy_rtx (operand0),
	3)));
  emit_insn (gen_rtx_SET (operand2,
	gen_rtx_LSHIFTRT (SImode,
	copy_rtx (operand2),
	const_int_rtx[MAX_SAVED_CONST_INT + (8)])));
  emit_insn (gen_rtx_SET (operand3,
	gen_rtx_ROTATERT (SImode,
	copy_rtx (operand3),
	const_int_rtx[MAX_SAVED_CONST_INT + (8)])));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5024 */
extern rtx_insn *gen_split_31 (rtx_insn *, rtx *);
rtx_insn *
gen_split_31 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_31 (m68k.md:5024)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5037 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_highpart (SImode, operands[0]);
  operands[3] = gen_lowpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	gen_rtx_SUBREG (HImode,
	operand0,
	6)),
	gen_rtx_SUBREG (HImode,
	copy_rtx (operand0),
	2)));
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	gen_rtx_SUBREG (HImode,
	copy_rtx (operand0),
	2)),
	const0_rtx));
  emit_insn (gen_rtx_SET (operand3,
	gen_rtx_ROTATE (SImode,
	copy_rtx (operand3),
	const_int_rtx[MAX_SAVED_CONST_INT + (16)])));
  emit_insn (gen_rtx_SET (operand2,
	gen_rtx_ROTATE (SImode,
	copy_rtx (operand2),
	const_int_rtx[MAX_SAVED_CONST_INT + (16)])));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5042 */
extern rtx_insn *gen_split_32 (rtx_insn *, rtx *);
rtx_insn *
gen_split_32 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_32 (m68k.md:5042)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5049 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = adjust_address(operands[0], SImode, 0);
  operands[1] = gen_highpart(SImode, operands[1]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  emit_insn (gen_rtx_SET (copy_rtx (operand0),
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5054 */
extern rtx_insn *gen_split_33 (rtx_insn *, rtx *);
rtx_insn *
gen_split_33 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_33 (m68k.md:5054)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5061 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = adjust_address(operands[0], SImode, 0);
  operands[1] = gen_highpart(SImode, operands[1]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	const0_rtx));
  emit_insn (gen_rtx_SET (copy_rtx (operand0),
	operand1));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5066 */
extern rtx_insn *gen_split_34 (rtx_insn *, rtx *);
rtx_insn *
gen_split_34 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx operand5;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_34 (m68k.md:5066)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5073 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
split_di(operands, 2, operands + 2, operands + 4);
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  operand4 = operands[4];
  (void) operand4;
  operand5 = operands[5];
  (void) operand5;
  emit_insn (gen_rtx_SET (operand2,
	operand5));
  emit_insn (gen_rtx_SET (operand4,
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5082 */
extern rtx_insn *gen_split_35 (rtx_insn *, rtx *);
rtx_insn *
gen_split_35 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_35 (m68k.md:5082)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5091 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = GEN_INT (INTVAL (operands[2]) - 32);
  operands[3] = gen_highpart (SImode, operands[0]);
  operands[4] = gen_lowpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  operand4 = operands[4];
  (void) operand4;
  emit_insn (gen_rtx_SET (operand3,
	gen_rtx_LSHIFTRT (SImode,
	copy_rtx (operand3),
	operand2)));
  emit_insn (gen_rtx_SET (operand4,
	copy_rtx (operand3)));
  emit_insn (gen_rtx_SET (copy_rtx (operand3),
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5097 */
extern rtx_insn *gen_split_36 (rtx_insn *, rtx *);
rtx_insn *
gen_split_36 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_36 (m68k.md:5097)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5108 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_highpart (SImode, operands[0]);
  operands[3] = gen_lowpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  emit_insn (gen_rtx_SET (operand3,
	operand2));
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	gen_rtx_SUBREG (HImode,
	operand0,
	6)),
	const0_rtx));
  emit_insn (gen_rtx_SET (copy_rtx (operand2),
	const0_rtx));
  emit_insn (gen_rtx_SET (copy_rtx (operand3),
	gen_rtx_ROTATE (SImode,
	copy_rtx (operand3),
	const_int_rtx[MAX_SAVED_CONST_INT + (16)])));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5113 */
extern rtx_insn *gen_split_37 (rtx_insn *, rtx *);
rtx_insn *
gen_split_37 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_split_37 (m68k.md:5113)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5123 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = GEN_INT (INTVAL (operands[2]) - 32);
  operands[3] = gen_highpart (SImode, operands[0]);
  operands[4] = gen_lowpart (SImode, operands[0]);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  operand4 = operands[4];
  (void) operand4;
  emit_insn (gen_rtx_SET (operand4,
	operand2));
  emit_insn (gen_rtx_SET (operand3,
	gen_rtx_LSHIFTRT (SImode,
	copy_rtx (operand3),
	copy_rtx (operand4))));
  emit_insn (gen_rtx_SET (copy_rtx (operand4),
	copy_rtx (operand3)));
  emit_insn (gen_rtx_SET (copy_rtx (operand3),
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5146 */
rtx
gen_lshrdi3 (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[3];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5151 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  /* ???  This is a named pattern like this is not allowed to FAIL based
     on its operands.  */
  if (GET_CODE (operands[2]) != CONST_INT
      || ((INTVAL (operands[2]) < 1 || INTVAL (operands[2]) > 3)
	  && INTVAL (operands[2]) != 8 && INTVAL (operands[2]) != 16
	  && (INTVAL (operands[2]) < 32 || INTVAL (operands[2]) > 63)))
    FAIL;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_LSHIFTRT (DImode,
	operand1,
	operand2)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5371 */
rtx
gen_bswapsi2 (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5375 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx x = operands[0];
  emit_move_insn (x, operands[1]);
  emit_insn (gen_rotrhi_lowpart (gen_lowpart (HImode, x), GEN_INT (8)));
  emit_insn (gen_rotlsi3 (x, x, GEN_INT (16)));
  emit_insn (gen_rotrhi_lowpart (gen_lowpart (HImode, x), GEN_INT (8)));
  DONE;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_BSWAP (SImode,
	operand1)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5595 */
rtx
gen_extv (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  return gen_rtx_SET (operand0,
	gen_rtx_SIGN_EXTRACT (SImode,
	operand1,
	operand2,
	operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5611 */
rtx
gen_extzv (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  return gen_rtx_SET (operand0,
	gen_rtx_ZERO_EXTRACT (SImode,
	operand1,
	operand2,
	operand3));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5663 */
rtx
gen_insv (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[4];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
    operands[3] = operand3;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5669 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"

{
  /* Special case initializing a field to all ones. */
  if (GET_CODE (operands[3]) == CONST_INT)
    {
      if (exact_log2 (INTVAL (operands[3]) + 1) != INTVAL (operands[1]))
	operands[3] = force_reg (SImode, operands[3]);
      else
	operands[3] = constm1_rtx;

    }
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
    operand3 = operands[3];
    (void) operand3;
  }
  emit_insn (gen_rtx_SET (gen_rtx_ZERO_EXTRACT (SImode,
	operand0,
	operand1,
	operand2),
	operand3));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5796 */
rtx
gen_tablejump (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5800 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
#if CASE_VECTOR_PC_RELATIVE
    operands[0] = gen_rtx_PLUS (SImode, pc_rtx,
				TARGET_LONG_JUMP_TABLE_OFFSETS
				? operands[0]
				: gen_rtx_SIGN_EXTEND (SImode, operands[0]));
#endif
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_jump_insn (gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (pc_rtx,
	operand0),
		gen_rtx_USE (VOIDmode,
	gen_rtx_LABEL_REF (VOIDmode,
	operand1)))));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5919 */
rtx
gen_decrement_and_branch_until_zero (rtx operand0,
	rtx operand1)
{
  return gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_GE (VOIDmode,
	gen_rtx_PLUS (SImode,
	operand0,
	constm1_rtx),
	const0_rtx),
	gen_rtx_LABEL_REF (VOIDmode,
	operand1),
	pc_rtx)),
		gen_rtx_SET (operand0,
	gen_rtx_PLUS (SImode,
	operand0,
	constm1_rtx))));
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5953 */
rtx
gen_sibcall (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5957 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = m68k_legitimize_sibcall_address (operands[0]);
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_call_insn (gen_rtx_CALL (VOIDmode,
	operand0,
	operand1));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5969 */
rtx
gen_sibcall_value (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[3];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5974 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[1] = m68k_legitimize_sibcall_address (operands[1]);
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
  }
  emit_call_insn (gen_rtx_SET (operand0,
	gen_rtx_CALL (VOIDmode,
	operand1,
	operand2)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:5989 */
rtx
gen_call (rtx operand0,
	rtx operand1)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 5994 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = m68k_legitimize_call_address (operands[0]);
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit_call_insn (gen_rtx_CALL (VOIDmode,
	operand0,
	operand1));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6010 */
rtx
gen_call_value (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[3];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6016 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[1] = m68k_legitimize_call_address (operands[1]);
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
  }
  emit_call_insn (gen_rtx_SET (operand0,
	gen_rtx_CALL (VOIDmode,
	operand1,
	operand2)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6060 */
rtx
gen_untyped_call (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[3];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6066 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  int i;

  emit_call_insn (gen_call (operands[0], const0_rtx));

  for (i = 0; i < XVECLEN (operands[2], 0); i++)
    {
      rtx set = XVECEXP (operands[2], 0, i);
      emit_move_insn (SET_DEST (set), SET_SRC (set));
    }

  /* The optimizer does not know that the call sets the function value
     registers we stored in the result block.  We avoid problems by
     claiming that all hard registers are used and clobbered at this
     point.  */
  emit_insn (gen_blockage ());

  DONE;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
  }
  emit_call_insn (gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (3,
		gen_rtx_CALL (VOIDmode,
	operand0,
	const0_rtx),
		operand1,
		operand2)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6102 */
rtx
gen_prologue (void)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6105 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  m68k_expand_prologue ();
  DONE;
}
#undef DONE
#undef FAIL
  }
  emit_insn (const0_rtx);
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6110 */
rtx
gen_epilogue (void)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6113 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  m68k_expand_epilogue (false);
  DONE;
}
#undef DONE
#undef FAIL
  }
  emit_jump_insn (ret_rtx);
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6118 */
rtx
gen_sibcall_epilogue (void)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6121 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  m68k_expand_epilogue (true);
  DONE;
}
#undef DONE
#undef FAIL
  }
  emit_jump_insn (ret_rtx);
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6127 */
rtx
gen_return (void)
{
  return ret_rtx;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6192 */
rtx
gen_link (rtx operand0,
	rtx operand1)
{
  rtx operand2;
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[3];
    operands[0] = operand0;
    operands[1] = operand1;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6202 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[2] = gen_frame_mem (SImode,
			       plus_constant (Pmode, stack_pointer_rtx, -4));
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
  }
  emit (gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (3,
		gen_rtx_SET (operand0,
	gen_rtx_PLUS (SImode,
	gen_rtx_REG (SImode,
	15),
	const_int_rtx[MAX_SAVED_CONST_INT + (-4)])),
		gen_rtx_SET (operand2,
	copy_rtx (operand0)),
		gen_rtx_SET (gen_rtx_REG (SImode,
	15),
	gen_rtx_PLUS (SImode,
	gen_rtx_REG (SImode,
	15),
	operand1)))), false);
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6227 */
rtx
gen_unlink (rtx operand0)
{
  rtx operand1;
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[2];
    operands[0] = operand0;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6235 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[1] = gen_frame_mem (SImode, copy_rtx (operands[0]));
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
  }
  emit (gen_rtx_PARALLEL (VOIDmode,
	gen_rtvec (2,
		gen_rtx_SET (operand0,
	operand1),
		gen_rtx_SET (gen_rtx_REG (SImode,
	15),
	gen_rtx_PLUS (SImode,
	copy_rtx (operand0),
	const_int_rtx[MAX_SAVED_CONST_INT + (4)])))), false);
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6295 */
extern rtx_insn *gen_peephole2_1 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_1 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_1 (m68k.md:6295)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6303 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
split_di(operands + 1, 1, operands + 1, operands + 2);
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  emit_insn (gen_rtx_SET (gen_rtx_MEM (SImode,
	gen_rtx_REG (SImode,
	15)),
	operand1));
  emit_insn (gen_rtx_SET (gen_rtx_MEM (SImode,
	gen_rtx_PRE_DEC (SImode,
	gen_rtx_REG (SImode,
	15))),
	operand2));
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_MEM (DFmode,
	gen_rtx_POST_INC (SImode,
	gen_rtx_REG (SImode,
	15)))));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6309 */
extern rtx_insn *gen_peephole2_2 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_2 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_2 (m68k.md:6309)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6315 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
operands[0] = replace_equiv_address (operands[0], stack_pointer_rtx);
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6317 */
extern rtx_insn *gen_peephole2_3 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_3 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_3 (m68k.md:6317)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6326 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = GEN_INT (INTVAL (operands[0]) - 4);
  operands[1] = replace_equiv_address (operands[1], stack_pointer_rtx);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  emit_insn (gen_rtx_SET (gen_rtx_REG (SImode,
	15),
	gen_rtx_PLUS (SImode,
	gen_rtx_REG (SImode,
	15),
	operand0)));
  emit_insn (gen_rtx_SET (operand1,
	operand2));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6336 */
extern rtx_insn *gen_peephole2_4 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_4 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_4 (m68k.md:6336)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6342 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
operands[0] = replace_equiv_address (operands[0], stack_pointer_rtx);
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6345 */
extern rtx_insn *gen_peephole2_5 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_5 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx_insn *_val = NULL;
  HARD_REG_SET _regs_allocated;
  CLEAR_HARD_REG_SET (_regs_allocated);
  if ((operands[2] = peep2_find_free_register (0, 0, "d", SImode, &_regs_allocated)) == NULL_RTX)
    return NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_5 (m68k.md:6345)\n");
  start_sequence ();
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  emit_insn (gen_rtx_SET (operand2,
	operand1));
  emit_insn (gen_rtx_SET (operand0,
	copy_rtx (operand2)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6357 */
extern rtx_insn *gen_peephole2_6 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_6 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx_insn *_val = NULL;
  HARD_REG_SET _regs_allocated;
  CLEAR_HARD_REG_SET (_regs_allocated);
  if ((operands[2] = peep2_find_free_register (0, 0, "d", SImode, &_regs_allocated)) == NULL_RTX)
    return NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_6 (m68k.md:6357)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6368 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
operands[0] = replace_equiv_address (operands[0], stack_pointer_rtx);
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  emit_insn (gen_rtx_SET (gen_rtx_REG (SImode,
	15),
	gen_rtx_PLUS (SImode,
	gen_rtx_REG (SImode,
	15),
	const_int_rtx[MAX_SAVED_CONST_INT + (8)])));
  emit_insn (gen_rtx_SET (operand2,
	operand1));
  emit_insn (gen_rtx_SET (operand0,
	copy_rtx (operand2)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6371 */
extern rtx_insn *gen_peephole2_7 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_7 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_7 (m68k.md:6371)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6383 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = GEN_INT (INTVAL (operands[0]) - 4);
  operands[1] = replace_equiv_address (operands[1], stack_pointer_rtx);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  emit_insn (gen_rtx_SET (gen_rtx_REG (SImode,
	15),
	gen_rtx_PLUS (SImode,
	gen_rtx_REG (SImode,
	15),
	operand0)));
  emit_insn (gen_rtx_SET (operand1,
	operand2));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6391 */
extern rtx_insn *gen_peephole2_8 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_8 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_8 (m68k.md:6391)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6401 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  rtx addr = gen_rtx_PRE_DEC (Pmode, stack_pointer_rtx);
  operands[0] = adjust_automodify_address (operands[0], SImode, addr, -3);
  operands[1] = simplify_gen_subreg (SImode, operands[1], QImode, 0);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6407 */
extern rtx_insn *gen_peephole2_9 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_9 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_9 (m68k.md:6407)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6413 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = adjust_automodify_address (operands[0], SImode,
					   XEXP (operands[0], 0), -3);
  operands[1] = simplify_gen_subreg (SImode, operands[1], QImode, 0);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6419 */
extern rtx_insn *gen_peephole2_10 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_10 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_10 (m68k.md:6419)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6425 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{
  operands[0] = adjust_automodify_address (operands[0], SImode,
					   XEXP (operands[0], 0), -2);
  operands[1] = simplify_gen_subreg (SImode, operands[1], HImode, 0);
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  emit_insn (gen_rtx_SET (operand0,
	operand1));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6433 */
extern rtx_insn *gen_peephole2_11 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_11 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_11 (m68k.md:6433)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6441 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"

#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	operand1),
	operand2));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6443 */
extern rtx_insn *gen_peephole2_12 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_12 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_12 (m68k.md:6443)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6451 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"

#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  emit_insn (gen_rtx_SET (gen_rtx_STRICT_LOW_PART (VOIDmode,
	operand1),
	operand2));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:21 */
rtx
gen_atomic_compare_and_swapqi (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3,
	rtx operand4,
	rtx operand5,
	rtx operand6,
	rtx operand7)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[8];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
    operands[3] = operand3;
    operands[4] = operand4;
    operands[5] = operand5;
    operands[6] = operand6;
    operands[7] = operand7;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 31 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
{
  emit_insn (gen_atomic_compare_and_swapqi_1
	     (operands[0], operands[1], operands[2],
	      operands[3], operands[4]));
  emit_insn (gen_negqi2 (operands[0], operands[0]));
  DONE;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
    operand3 = operands[3];
    (void) operand3;
    operand4 = operands[4];
    (void) operand4;
    operand5 = operands[5];
    (void) operand5;
    operand6 = operands[6];
    (void) operand6;
    operand7 = operands[7];
    (void) operand7;
  }
  emit (operand0, true);
  emit (operand1, true);
  emit (operand2, true);
  emit (operand3, true);
  emit (operand4, true);
  emit (operand5, true);
  emit (operand6, true);
  emit (operand7, false);
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:21 */
rtx
gen_atomic_compare_and_swaphi (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3,
	rtx operand4,
	rtx operand5,
	rtx operand6,
	rtx operand7)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[8];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
    operands[3] = operand3;
    operands[4] = operand4;
    operands[5] = operand5;
    operands[6] = operand6;
    operands[7] = operand7;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 31 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
{
  emit_insn (gen_atomic_compare_and_swaphi_1
	     (operands[0], operands[1], operands[2],
	      operands[3], operands[4]));
  emit_insn (gen_negqi2 (operands[0], operands[0]));
  DONE;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
    operand3 = operands[3];
    (void) operand3;
    operand4 = operands[4];
    (void) operand4;
    operand5 = operands[5];
    (void) operand5;
    operand6 = operands[6];
    (void) operand6;
    operand7 = operands[7];
    (void) operand7;
  }
  emit (operand0, true);
  emit (operand1, true);
  emit (operand2, true);
  emit (operand3, true);
  emit (operand4, true);
  emit (operand5, true);
  emit (operand6, true);
  emit (operand7, false);
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:21 */
rtx
gen_atomic_compare_and_swapsi (rtx operand0,
	rtx operand1,
	rtx operand2,
	rtx operand3,
	rtx operand4,
	rtx operand5,
	rtx operand6,
	rtx operand7)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[8];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
    operands[3] = operand3;
    operands[4] = operand4;
    operands[5] = operand5;
    operands[6] = operand6;
    operands[7] = operand7;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 31 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
{
  emit_insn (gen_atomic_compare_and_swapsi_1
	     (operands[0], operands[1], operands[2],
	      operands[3], operands[4]));
  emit_insn (gen_negqi2 (operands[0], operands[0]));
  DONE;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
    operand3 = operands[3];
    (void) operand3;
    operand4 = operands[4];
    (void) operand4;
    operand5 = operands[5];
    (void) operand5;
    operand6 = operands[6];
    (void) operand6;
    operand7 = operands[7];
    (void) operand7;
  }
  emit (operand0, true);
  emit (operand1, true);
  emit (operand2, true);
  emit (operand3, true);
  emit (operand4, true);
  emit (operand5, true);
  emit (operand6, true);
  emit (operand7, false);
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md:58 */
rtx
gen_atomic_test_and_set (rtx operand0,
	rtx operand1,
	rtx operand2)
{
  rtx_insn *_val = 0;
  start_sequence ();
  {
    rtx operands[3];
    operands[0] = operand0;
    operands[1] = operand1;
    operands[2] = operand2;
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 63 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
{
  rtx t = gen_reg_rtx (QImode);
  emit_insn (gen_atomic_test_and_set_1 (t, operands[1]));
  t = expand_simple_unop (QImode, NEG, t, operands[0], 0);
  if (t != operands[0])
    emit_move_insn (operands[0], t);
  DONE;
}
#undef DONE
#undef FAIL
    operand0 = operands[0];
    (void) operand0;
    operand1 = operands[1];
    (void) operand1;
    operand2 = operands[2];
    (void) operand2;
  }
  emit (operand0, true);
  emit (operand1, true);
  emit (operand2, false);
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6722 */
extern rtx_insn *gen_peephole2_13 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_13 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx operand5;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_13 (m68k.md:6722)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6741 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
operands[5] = (operands[0] == operands[3]) ? operands[4] : operands[3];
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  operand4 = operands[4];
  (void) operand4;
  operand5 = operands[5];
  (void) operand5;
  emit_insn (gen_rtx_SET (operand5,
	gen_rtx_PLUS (SImode,
	copy_rtx (operand5),
	gen_rtx_MEM (SImode,
	gen_rtx_PLUS (SImode,
	operand1,
	operand2)))));
  emit_insn (gen_rtx_SET (gen_rtx_MEM (QImode,
	copy_rtx (operand5)),
	const0_rtx));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6763 */
extern rtx_insn *gen_peephole2_14 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_14 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx operand5;
  rtx operand6;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_14 (m68k.md:6763)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6779 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
operands[6] = GEN_INT (-INTVAL (operands[1]));
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  operand4 = operands[4];
  (void) operand4;
  operand5 = operands[5];
  (void) operand5;
  operand6 = operands[6];
  (void) operand6;
  emit_insn (gen_rtx_SET (operand2,
	gen_rtx_PLUS (SImode,
	copy_rtx (operand2),
	operand6)));
  emit_jump_insn (gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand5), GET_MODE (operand5),
		copy_rtx (operand2),
		const0_rtx),
	operand3,
	operand4)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6781 */
extern rtx_insn *gen_peephole2_15 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_15 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx operand5;
  rtx operand6;
  rtx operand7;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_15 (m68k.md:6781)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6798 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"

{
  operands[6] = GEN_INT (exact_log2 (INTVAL (operands[1]) + 1));
  operands[7] = operands[2];
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  operand4 = operands[4];
  (void) operand4;
  operand5 = operands[5];
  (void) operand5;
  operand6 = operands[6];
  (void) operand6;
  operand7 = operands[7];
  (void) operand7;
  emit_insn (gen_rtx_SET (operand7,
	gen_rtx_LSHIFTRT (SImode,
	copy_rtx (operand7),
	operand6)));
  emit_jump_insn (gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_NE (VOIDmode,
	copy_rtx (operand7),
	const0_rtx),
	operand4,
	operand5)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6804 */
extern rtx_insn *gen_peephole2_16 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_16 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_16 (m68k.md:6804)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6817 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{ operands[4] = GEN_INT (exact_log2 (INTVAL (operands[1]) + 1)); }
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  operand4 = operands[4];
  (void) operand4;
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_LSHIFTRT (SImode,
	copy_rtx (operand0),
	operand4)));
  emit_jump_insn (gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_NE (VOIDmode,
	copy_rtx (operand0),
	const0_rtx),
	operand2,
	operand3)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6819 */
extern rtx_insn *gen_peephole2_17 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_17 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx operand5;
  rtx operand6;
  rtx operand7;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_17 (m68k.md:6819)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6836 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"

{
  operands[6] = GEN_INT (exact_log2 (INTVAL (operands[1]) + 1));
  operands[7] = operands[2];
}
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  operand4 = operands[4];
  (void) operand4;
  operand5 = operands[5];
  (void) operand5;
  operand6 = operands[6];
  (void) operand6;
  operand7 = operands[7];
  (void) operand7;
  emit_insn (gen_rtx_SET (operand7,
	gen_rtx_LSHIFTRT (SImode,
	copy_rtx (operand7),
	operand6)));
  emit_jump_insn (gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_EQ (VOIDmode,
	copy_rtx (operand7),
	const0_rtx),
	operand4,
	operand5)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6841 */
extern rtx_insn *gen_peephole2_18 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_18 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_18 (m68k.md:6841)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6854 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
{ operands[4] = GEN_INT (exact_log2 (INTVAL (operands[1]) + 1)); }
#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  operand4 = operands[4];
  (void) operand4;
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_LSHIFTRT (SImode,
	copy_rtx (operand0),
	operand4)));
  emit_jump_insn (gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_EQ (VOIDmode,
	copy_rtx (operand0),
	const0_rtx),
	operand2,
	operand3)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}

/* /Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md:6860 */
extern rtx_insn *gen_peephole2_19 (rtx_insn *, rtx *);
rtx_insn *
gen_peephole2_19 (rtx_insn *curr_insn ATTRIBUTE_UNUSED, rtx *operands)
{
  rtx operand0;
  rtx operand1;
  rtx operand2;
  rtx operand3;
  rtx_insn *_val = NULL;
  if (dump_file)
    fprintf (dump_file, "Splitting with gen_peephole2_19 (m68k.md:6860)\n");
  start_sequence ();
#define FAIL return (end_sequence (), _val)
#define DONE return (_val = get_insns (), end_sequence (), _val)
#line 6873 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"

#undef DONE
#undef FAIL
  operand0 = operands[0];
  (void) operand0;
  operand1 = operands[1];
  (void) operand1;
  operand2 = operands[2];
  (void) operand2;
  operand3 = operands[3];
  (void) operand3;
  emit_insn (gen_rtx_SET (operand0,
	gen_rtx_ROTATE (SImode,
	copy_rtx (operand0),
	const_int_rtx[MAX_SAVED_CONST_INT + (16)])));
  emit_jump_insn (gen_rtx_SET (pc_rtx,
	gen_rtx_IF_THEN_ELSE (VOIDmode,
	gen_rtx_fmt_ee (GET_CODE (operand1), GET_MODE (operand1),
		gen_rtx_SUBREG (HImode,
	copy_rtx (operand0),
	2),
		const0_rtx),
	operand2,
	operand3)));
  _val = get_insns ();
  end_sequence ();
  return _val;
}



void
add_clobbers (rtx pattern ATTRIBUTE_UNUSED, int insn_code_number)
{
  switch (insn_code_number)
    {
    case 121:
    case 120:
    case 119:
      XVECEXP (pattern, 0, 1) = gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode));
      XVECEXP (pattern, 0, 2) = gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode));
      break;

    case 5:
      XVECEXP (pattern, 0, 1) = gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (DImode));
      XVECEXP (pattern, 0, 2) = gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode));
      break;

    case 321:
    case 304:
    case 174:
    case 172:
    case 147:
    case 143:
    case 142:
    case 92:
    case 4:
    case 3:
      XVECEXP (pattern, 0, 1) = gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (SImode));
      break;

    default:
      gcc_unreachable ();
    }
}


int
added_clobbers_hard_reg_p (int insn_code_number)
{
  switch (insn_code_number)
    {
    case 121:
    case 120:
    case 119:
    case 5:
    case 321:
    case 304:
    case 174:
    case 172:
    case 147:
    case 143:
    case 142:
    case 92:
    case 4:
    case 3:
      return 0;

    default:
      gcc_unreachable ();
    }
}
