/*	$NetBSD: fpureg.h,v 1.1.2.2 2001/01/05 17:34:02 bouyer Exp $	*/

/*
 * ARM FPU definitions
 */

/* System ID byte */
#define FPSR_SYSID_MASK		0xff000000
#define FPSR_SYSID_FPPC		0x80000000
#define FPSR_SYSID_FPA		0x81000000

/* Trap enable bits */
#define FPSR_TE_IVO		0x00010000 /* InValid Operation */
#define FPSR_TE_DVZ		0x00020000 /* DiVision by Zero */
#define FPSR_TE_OFL		0x00040000 /* OverFLow */
#define FPSR_TE_UFL		0x00080000 /* UnderFLow */
#define FPSR_TE_INX		0x00100000 /* INeXact */

/* System control byte (FPA only) */
#define FPSR_CTL_ND		0x00000100 /* No Denormalised numbers */
#define FPSR_CTL_NE		0x00000200 /* NaN Exception */
#define FPSR_CTL_SO		0x00000400 /* Synchronous Operation */
#define FPSR_CTL_EP		0x00000800 /* Expanded Packed decimal */
#define FPSR_CTL_AC		0x00001000 /* Alternative C flag */

/* Cumulative exception bits */
#define FPSR_EX_IVO		0x00000001 /* InValid Operation */
#define FPSR_EX_DVZ		0x00000002 /* DiVision by Zero */
#define FPSR_EX_OFL		0x00000004 /* OverFLow */
#define FPSR_EX_UFL		0x00000008 /* UnderFLow */
#define FPSR_EX_INX		0x00000010 /* INeXact */

/*
 * FPPC FPCR
 */
#define FPPC_FPCR_DA	0x00000001 /* Disable */
#define FPPC_FPCR_EX	0x00000002 /* FP exception occurred */
#define FPPC_FPCR_AS	0x00000004 /* Last exception was async */
#define FPPC_FPCR_SBM	0x00000010 /* Use supervisor bank 'm' */
#define FPPC_FPCR_SBN	0x00000020 /* Use supervisor bank 'n' */
#define FPPC_FPCR_SBD	0x00000040 /* Use supervisor bank 'd' */
#define FPPC_FPCR_PR	0x00000080 /* Last RMF gave partial remainder */

/*
 * FPA FPCR
 * This is provisional, from the RISC OS 3 PRM
 */
#define FPA_FPCR_S2	0x0000000f /* AU source register 2 */
#define FPA_FPCR_OP	0x00f08010 /* AU operation code */
#define FPA_FPCR_RM	0x00000060 /* AU rounding mode */
#define FPA_FPCR_PR	0x00080080 /* AU precision */
#define FPA_FPCR_EN	0x00000100 /* Enable FPA */
#define FPA_FPCR_RE	0x00000200 /* Rounding exception */
#define FPA_FPCR_AB	0x00000400 /* Asynchronous bounce */
#define FPA_FPCR_SB	0x00000800 /* Synchronous bounce */
#define FPA_FPCR_DS	0x00007000 /* AU destination register */
#define FPA_FPCR_S1	0x00070000 /* AU source register 1 */
#define FPA_FPCR_EO	0x04000000 /* Exponent overflow */
#define FPA_FPCR_MO	0x08000000 /* Mantissa overflow */
#define FPA_FPCR_IE	0x10000000 /* Inexact bit */
#define FPA_FPCR_RU	0x80000000 /* Rounded up bit */
