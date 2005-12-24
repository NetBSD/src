/*	$NetBSD: gtreg.h,v 1.2 2005/12/24 20:07:03 perry Exp $	*/

#define GT_REGVAL(x)	*((volatile u_int32_t *) \
			    (MIPS_PHYS_TO_KSEG1(MALTA_CORECTRL_BASE + (x))))

/* CPU Configuration Register Map */
#define	GT_CPU_INT	0x000
#define	GT_MULTIGT	0x120

/* CPU Address Decode Register Map */

/* CPU Error Report Register Map */

/* CPU Sync Barrier Register Map */

/* SDRAM and Device Address Decode Register Map */

/* SDRAM Configuration Register Map */

/* SDRAM Parameters Register Map */

/* ECC Register Map */

/* Device Parameters Register Map */

/* DMA Record Register Map */

/* DMA Arbiter Register Map */

/* Timer/Counter Register Map */
//#define	GT_TC_0		0x850
//#define	GT_TC_1		0x854
//#define	GT_TC_2		0x858
//#define	GT_TC_3		0x85c
//#define	GT_TC_CONTROL	0x864

/* PCI Internal Register Map */
#define	GT_PCI0_CFG_ADDR	0xcf8
#define	GT_PCI0_CFG_DATA	0xcfc
#define	GT_PCI0_INTR_ACK	0xc34

/* Interrupts Register Map */
#define	GT_INTR_CAUSE	0xc18
#define	 GTIC_INTSUM	 0x00000001
#define	 GTIC_MEMOUT	 0x00000002
#define	 GTIC_DMAOUT	 0x00000004
#define	 GTIC_CPUOUT	 0x00000008
#define	 GTIC_DMA0COMP	 0x00000010
#define	 GTIC_DMA1COMP	 0x00000020
#define	 GTIC_DMA2COMP	 0x00000040
#define	 GTIC_DMA3COMP	 0x00000080
#define	 GTIC_T0EXP	 0x00000100
#define	 GTIC_T1EXP	 0x00000200
#define	 GTIC_T2EXP	 0x00000400
#define	 GTIC_T3EXP	 0x00000800
#define	 GTIC_MASRDERR0	 0x00001000
#define	 GTIC_SLVWRERR0	 0x00002000
#define	 GTIC_MASWRERR0	 0x00004000
#define	 GTIC_SLVRDERR0	 0x00008000
#define	 GTIC_ADDRERR0	 0x00010000
#define	 GTIC_MEMERR	 0x00020000
#define	 GTIC_MASABORT0	 0x00040000
#define	 GTIC_TARABORT0	 0x00080000
#define	 GTIC_RETRYCNT0	 0x00100000
#define	 GTIC_PMCINT_0	 0x00200000
#define	 GTIC_CPUINT	 0x0c300000
#define	 GTIC_PCINT	 0xc3000000
#define	 GTIC_CPUINTSUM	 0x40000000
#define	 GTIC_PCIINTSUM	 0x80000000

/* PCI Configuration Register Map */
//#define	GT_PCICONFIGBASE	0
//#define	GT_PCIDID		BONITO(GT_PCICONFIGBASE + 0x00)
//#define	GT_PCICMD		BONITO(GT_PCICONFIGBASE + 0x04)
//#define	GT_PCICLASS		BONITO(GT_PCICONFIGBASE + 0x08)
//#define	GT_PCILTIMER		BONITO(GT_PCICONFIGBASE + 0x0c)
//#define	GT_PCIBASE0		BONITO(GT_PCICONFIGBASE + 0x10)
//#define	GT_PCIBASE1		BONITO(GT_PCICONFIGBASE + 0x14)
//#define	GT_PCIBASE2		BONITO(GT_PCICONFIGBASE + 0x18)
//#define	GT_PCIEXPRBASE		BONITO(GT_PCICONFIGBASE + 0x30)
//#define	GT_PCIINT		BONITO(GT_PCICONFIGBASE + 0x3c)

/* PCI Configuration, Function 1, Register Map */

/* I2O Support Register Map */
