/*	$NetBSD: asm.h,v 1.1.2.2 2002/02/28 04:10:56 nathanw Exp $	*/

#include <powerpc/asm.h>

#define	HID0_NOOPTI	(1 << 0)	/* No-op D-cache touch instrutions */
#define	HID0_BTCD	(1 << 1)
#define	HID0_BHTE	(1 << 2)
#define	HID0_FBIOB	(1 << 4)	/* Force branch indirect on bus */
#define	HID0_SIED	(1 << 7)
#define	HID0_DCFI	(1 << 10)	/* D-cache flash invalidate */
#define	HID0_ICFI	(1 << 11)	/* I-cache flash invalidate */
#define	HID0_DLOCK	(1 << 12)	/* D-cache lock */
#define	HID0_ILOCK	(1 << 13)	/* I-cache lock */
#define	HID0_DCE	(1 << 14)	/* D-cache enable */
#define	HID0_ICE	(1 << 15)	/* I-cache enable */
#define	HID0_NHR	(1 << 16)
#define	HID0_RISEG	(1 << 19)
#define	HID0_DPM	(1 << 20)	/* Dynamic power management enable */
#define	HID0_SLEEP	(1 << 21)	/* Sleep mode enable */
#define	HID0_NAP	(1 << 22)	/* Nap mode enable */
#define	HID0_DOZE	(1 << 23)	/* Doze mode enable */
#define	HID0_PAR	(1 << 24)	/* Disable precharge of #ARTRY */
#define	HID0_ECLK	(1 << 25)	/* CLK_OUT output enable */
#define	HID0_EICE	(1 << 26)	/* Enable ICE pipeline tracking */
#define	HID0_BCLK	(1 << 27)	/* CLK_OUT output enable */
#define	HID0_EBD	(1 << 28)	/* Enable 60x-bus data parity checks */
#define	HID0_EBA	(1 << 29)	/* Enable 60x-bus addr parity checks */
#define	HID0_EMCP	(1 << 31)	/* Enable Machine Checks */

#define	LDCONST(r,v)	lis r,v@ha ; addi r,r,v@l
#define	LDVAR(r,a)	lis r,a@ha ; lwz r,a@l(r)
