/*
 *	IOCS call macros for X680x0
 */

#ifndef X68k_IOCSCALL_H
#define X68k_IOCSCALL_H

#ifdef __NeXT__
# define IMM	\#
#else
# define IMM	#
#endif

#define IOCS(n)	\
	moveq	IMM n,d0;\
	trap	IMM 15

#define __B_KEYINP	0x00
#define __B_SFTSNS	0x02
#define __TPALET2	0x14
#define __B_PUTC	0x20
#define __B_PRINT	0x21
#define __B_COLOR	0x22
#define __B_LOCATE	0x23
#define __B_CLR_ST	0x2A
#define __B_READ	0x46
#define __BOOTINF	0xFFFFFF8E
#define __JISSFT	0xFFFFFFA1
#define __SYS_STAT	0xFFFFFFAC	/* only for X68030 or Xellent */
#define __SCSIDRV	0xFFFFFFF5
#define __IPLERR	0xFFFFFFFE

#define SCSIIOCS(s)	\
	moveq	IMM s,d1;\
	IOCS(__SCSIDRV)

#define __S_READ	0x21

#endif /*X68k_IOCSCALL_H*/
