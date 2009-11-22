/*	$NetBSD: gemini_reg.h,v 1.9 2009/11/22 19:09:15 mbalmer Exp $	*/

#ifndef _ARM_GEMINI_REG_H_
#define _ARM_GEMINI_REG_H_

/*
 * Register definitions for Gemini SOC
 */

#include "opt_gemini.h"
#include <machine/endian.h>
#include <sys/cdefs.h>

#if defined(SL3516)
/*
 * Gemini SL3516 memory map
 */
#define GEMINI_SRAM_BASE	0x00000000	/* Internal SRAM  */
						/* NOTE: use the SHADOW to avoid conflict w/ DRAM */
#define GEMINI_SRAM_SIZE	0x10000000 	/* 128 MB */
#define GEMINI_DRAM_BASE	0x00000000 	/* DRAM (via DDR Control Module) */
						/* NOTE: this is a shadow of 0x10000000 */
#define GEMINI_DRAM_SIZE	0x20000000	/* 512 MB */
						/* NOTE: size of addr space, not necessarily populated */
#define GEMINI_FLASH_BASE	0x30000000 	/* DRAM (via DDR Control Module) */
#define GEMINI_FLASH_SIZE	0x10000000	/* 128 MB */

/*
 * Gemini SL3516 device map
 */
#define GEMINI_GLOBAL_BASE	0x40000000 	/* Global registers */
#define GEMINI_WATCHDOG_BASE	0x41000000 	/* Watch dog timer module */
#define GEMINI_UART_BASE	0x42000000 	/* UART control module */
#define GEMINI_UART_SIZE	0x20 
#define GEMINI_TIMER_BASE	0x43000000 	/* Timer module */
#define GEMINI_LCD_BASE		0x44000000 	/* LCD Interface module */
#define GEMINI_RTC_BASE		0x45000000 	/* Real Time Clock module */
#define GEMINI_SATA_BASE	0x46000000 	/* Serial ATA module */
#define GEMINI_LPCHC_BASE	0x47000000 	/* LPC Hosr Controller module */
#define GEMINI_LPCIO_BASE	0x47800000 	/* LPC Peripherals IO space */
#define GEMINI_IC0_BASE		0x48000000 	/* Interrupt Control module #0 */
#define GEMINI_IC1_BASE		0x49000000 	/* Interrupt Control module #1 */
#define GEMINI_SSPC_BASE	0x4a000000 	/* Synchronous Serial Port Control module */
#define GEMINI_PWRC_BASE	0x4b000000 	/* Power Control module */
#define GEMINI_CIR_BASE		0x4c000000 	/* CIR Control module */
#define GEMINI_GPIO0_BASE	0x4d000000 	/* GPIO module #0 */
#define GEMINI_GPIO1_BASE	0x4e000000 	/* GPIO module #1 */
#define GEMINI_GPIO2_BASE	0x4f000000 	/* GPIO module #2 */
#define GEMINI_PCICFG_BASE	0x50000000 	/* PCI IO, configuration and control space */
#define GEMINI_PCIIO_BASE	0x50001000 	/* PCI IO     space      */
#define GEMINI_PCIIO_SIZE	0x0007f000 	/* PCI IO     space size */
#define GEMINI_PCIMEM_BASE	0x58000000 	/* PCI Memory space      */
#define GEMINI_PCIMEM_SIZE	0x08000000 	/* PCI Memory space size */
#define GEMINI_GMAC_BASE	0x60000000 	/* NetEngine & GMAC Configuration registers */
#define GEMINI_GMAC_SIZE	0x00010000 	/* NetEngine & GMAC Configuration size */
#define GEMINI_SDC_BASE		0x62000000 	/* Security DMA and Configure registers */
#define GEMINI_MIDE_BASE	0x63000000 	/* Multi IDE registers */
#define GEMINI_RXDC_BASE	0x64000000 	/* RAID XOR DMA Configuration registers */
#define GEMINI_FLASHC_BASE	0x65000000 	/* Flash Controller registers */
#define GEMINI_DRAMC_BASE	0x66000000 	/* DRAM (DDR/SDR) Controller registers */
#define GEMINI_GDMA_BASE	0x67000000 	/* General DMA registers */
#define GEMINI_USB0_BASE	0x68000000 	/* USB #0 registers */
#define GEMINI_USB1_BASE	0x69000000 	/* USB #1 registers */
#define GEMINI_TVE_BASE		0x6a000000 	/* TVE registers */
#define GEMINI_SRAM_SHADOW_BASE	0x70000000 	/* Shadow of internal SRAM */

/*
 * Gemini SL3516 Global register offsets and bits
 */
#define GEMINI_GLOBAL_WORD_ID	0x0		/* Global Word ID */			/* ro */
#define  GLOBAL_ID_CHIP_ID	__BITS(31,8)
#define  GLOBAL_ID_CHIP_REV	__BITS(7,0)
#define GEMINI_GLOBAL_RESET_CTL	0xc		/* Global Soft Reset Control */		/* rw */
#define GLOBAL_RESET_GLOBAL	__BIT(31)	/* Global Soft Reset */
#define GLOBAL_RESET_CPU1	__BIT(30)	/* CPU#1 reset hold */
#define GLOBAL_RESET_GMAC1	__BIT(6)	/* GMAC1 reset hold */
#define GLOBAL_RESET_GMAC0	__BIT(5)	/* CGMAC reset hold */
#define GEMINI_GLOBAL_MISC_CTL	0x30		/* Miscellaneous Control */		/* rw */
#define GEMINI_GLOBAL_CPU0	0x38		/* CPU #0 Status and Control */		/* rw */
#define  GLOBAL_CPU0_IPICPU1	__BIT(31)	/* IPI to CPU#1 */
#define GEMINI_GLOBAL_CPU1	0x3c		/* CPU #1 Status and Control */		/* rw */
#define  GLOBAL_CPU1_IPICPU0	__BIT(31)	/* IPI to CPU#0 */

/*
 * Gemini SL3516 Watchdog device register offsets and bits
 */
#define GEMINI_WDT_WDCOUNTER	0x0		/* Watchdog Timer Counter */		/* ro */
#define GEMINI_WDT_WDLOAD	0x4		/* Watchdog Timer Load */		/* rw */
#define  WDT_WDLOAD_DFLT	0x3EF1480	/* default Load reg val */
#define GEMINI_WDT_WDRESTART	0x8		/* Watchdog Timer Restart */		/* wo */
#define  WDT_WDRESTART_Resv	__BITS(31,16)
#define  WDT_WDRESTART_RST	__BITS(15,0)
#define   WDT_WDRESTART_MAGIC	0x5ab9
#define GEMINI_WDT_WDCR		0xc		/* Watchdog Timer Control */		/* rw */
#define  WDT_WDCR_Resv		__BITS(31,5)
#define  WDT_WDCR_CLKSRC	__BIT(4)	/* Timer Clock Source: 5 MHz clock */
#define   WDCR_CLKSRC_PCLK	(0 << 4)	/* Timer Clock Source: PCLK (APB CLK) */
#define   WDCR_CLKSRC_5MHZ	(1 << 4)	/* Timer Clock Source: 5 MHz clock */
#define  WDT_WDCR_EXTSIG_ENB	__BIT(3)	/* Timer External Signal Enable */
#define  WDT_WDCR_INTR_ENB	__BIT(2)	/* Timer System Interrupt Enable */
#define  WDT_WDCR_RESET_ENB	__BIT(1)	/* Timer System Reset Enable */
#define  WDT_WDCR_ENB		__BIT(0)	/* Timer Enable */
#define GEMINI_WDT_WDSTATUS	0x10		/* Watchdog Timer Status */		/* ro */
#define  WDT_WDSTATUS_Resv	__BITS(31,1)
#define  WDT_WDSTATUS_ZERO	__BIT(0)	/* non-zero if timer counted down to zero! */
#define GEMINI_WDT_WDCLEAR	0x14		/* Watchdog Timer Clear */		/* wo */
#define  WDT_WDCLEAR_Resv	__BITS(31,1)
#define  WDT_WDCLEAR_CLEAR	__BIT(0)	/* write this bit to clear Status */
#define GEMINI_WDT_WDINTERLEN	0x18		/* Watchdog Timer Interrupt Length */	/* rw */
						/*  duration of signal assertion,  */
						/*  in units of clock cycles       */
#define  WDT_WDINTERLEN_DFLT	0xff		/*  default is 256 cycles          */


/*
 * Gemini SL3516 Timer device register offsets and bits
 *
 * have 3 timers, here indexed 1<=(n)<=3 as in the doc
 * each has 4 sequential 32bit rw regs
 */
#define GEMINI_NTIMERS			3
#define GEMINI_TIMERn_REG(n, o)		((((n) - 1) * 0x10) + (o))
#define GEMINI_TIMERn_COUNTER(n)	GEMINI_TIMERn_REG((n), 0x0)			/* rw */
#define GEMINI_TIMERn_LOAD(n)		GEMINI_TIMERn_REG((n), 0x4)			/* rw */
#define GEMINI_TIMERn_MATCH1(n)		GEMINI_TIMERn_REG((n), 0x08)			/* rw */
#define GEMINI_TIMERn_MATCH2(n) 	GEMINI_TIMERn_REG((n), 0x0C)			/* rw */
#define GEMINI_TIMER_TMCR		0x30						/* rw */
#define  TIMER_TMCR_Resv		__BITS(31,12)
#define  TIMER_TMCR_TMnUPDOWN(n)	__BIT(9 + (n) - 1)
#define  TIMER_TMCR_TMnOFENABLE(n)	__BIT((((n) - 1) * 3) + 2)
#define  TIMER_TMCR_TMnCLOCK(n)		__BIT((((n) - 1) * 3) + 1)
#define  TIMER_TMCR_TMnENABLE(n)	__BIT((((n) - 1) * 3) + 0)
#define  GEMINI_TIMER_TMnCR_MASK(n) 		\
		( TIMER_TMCR_TMnUPDOWN(n)	\
		| TIMER_TMCR_TMnOFENABLE(n)	\
		| TIMER_TMCR_TMnCLOCK(n)	\
		| TIMER_TMCR_TMnENABLE(n) )
#define GEMINI_TIMER_INTRSTATE		0x34						/* rw */
#define  TIMER_INTRSTATE_Resv		__BITS(31,9)
#define  TIMER_INTRSTATE_TMnOVFLOW(n)	__BIT((((n) -1) * 3) + 2)
#define  TIMER_INTRSTATE_TMnMATCH2(n)	__BIT((((n) -1) * 3) + 1)
#define  TIMER_INTRSTATE_TMnMATCH1(n)	__BIT((((n) -1) * 3) + 0)
#define GEMINI_TIMER_INTRMASK		0x38						/* rw */
#define  TIMER_INTRMASK_Resv		__BITS(31,9)
#define  TIMER_INTRMASK_TMnOVFLOW(n)	__BIT((((n) -1) * 3) + 2)
#define  TIMER_INTRMASK_TMnMATCH2(n)	__BIT((((n) -1) * 3) + 1)
#define  TIMER_INTRMASK_TMnMATCH1(n)	__BIT((((n) -1) * 3) + 0)
#define GEMINI_TIMERn_INTRMASK(n) \
		( TIMER_INTRMASK_TMnOVFLOW(n)	\
		| TIMER_INTRMASK_TMnMATCH2(n)	\
		| TIMER_INTRMASK_TMnMATCH1(n) )

/*
 * Gemini SL3516 Interrupt Controller device register offsets and bits
 */
#define GEMINI_ICU_IRQ_SOURCE		0x0						/* ro */
#define GEMINI_ICU_IRQ_ENABLE		0x4						/* rw */
#define GEMINI_ICU_IRQ_CLEAR		0x8						/* wo */
#define GEMINI_ICU_IRQ_TRIGMODE		0xc						/* rw */
#define  ICU_IRQ_TRIGMODE_EDGE		1		/* edge triggered */
#define  ICU_IRQ_TRIGMODE_LEVEL		0		/* level triggered */
#define GEMINI_ICU_IRQ_TRIGLEVEL	0x10						/* rw */
#define  ICU_IRQ_TRIGLEVEL_LO		1		/* active low or falling edge */
#define  ICU_IRQ_TRIGLEVEL_HI		0		/* active high or rising edge */
#define GEMINI_ICU_IRQ_STATUS		0x14						/* ro */

#define GEMINI_ICU_FIQ_SOURCE		0x20						/* ro */
#define GEMINI_ICU_FIQ_ENABLE		0x24						/* rw */
#define GEMINI_ICU_FIQ_CLEAR		0x28						/* wo */
#define GEMINI_ICU_FIQ_TRIGMODE		0x2c						/* rw */
#define GEMINI_ICU_FIQ_TRIGLEVEL	0x30						/* rw */
#define GEMINI_ICU_FIQ_STATUS		0x34						/* ro */

#define GEMINI_ICU_REVISION		0x50						/* ro */
#define GEMINI_ICU_INPUT_NUM		0x54						/* ro */
#define  ICU_INPUT_NUM_RESV		__BITS(31,16)
#define  ICU_INPUT_NUM_IRQ		__BITS(15,8)
#define  ICU_INPUT_NUM_FIQ		__BITS(7,0)
#define GEMINI_ICU_IRQ_DEBOUNCE		0x58						/* ro */
#define GEMINI_ICU_FIQ_DEBOUNCE		0x5c						/* ro */


/*
 * Gemini LPC controller register offsets and bits
 */
#define GEMINI_LPCHC_ID			0x00						/* ro */
# define LPCHC_ID_DEVICE		__BITS(31,8)	/* Device ID */
# define LPCHC_ID_REV			__BITS(7,0)	/* Revision */
# define _LPCHC_ID_DEVICE(r)		((typeof(r))(((r) & LPCHC_ID_DEVICE) >> 8))
# define _LPCHC_ID_REV(r)		((typeof(r))(((r) & LPCHC_ID_REV) >> 0))
#define GEMINI_LPCHC_CSR		0x04						/* rw */
# define LPCHC_CSR_RESa			__BITS(31,24)
# define LPCHC_CSR_STO			__BIT(23)	/* Sync Time Out */
# define LPCHC_CSR_SERR			__BIT(22)	/* Sync Error */
# define LPCHC_CSR_RESb			__BITS(21,8)
# define LPCHC_CSR_STOE			__BIT(7)	/* Sync Time Out Enable */
# define LPCHC_CSR_SERRE		__BIT(6)	/* Sync Error Enable */
# define LPCHC_CSR_RESc			__BITS(5,1)
# define LPCHC_CSR_BEN			__BIT(0)	/* Bridge Enable */
#define GEMINI_LPCHC_IRQCTL		0x08						/* rw */
# define LPCHC_IRQCTL_RESV		__BITS(31,8)
# define LPCHC_IRQCTL_SIRQEN		__BIT(7)	/* Serial IRQ Enable */
# define LPCHC_IRQCTL_SIRQMS		__BIT(6)	/* Serial IRQ Mode Select */
# define LPCHC_IRQCTL_SIRQFN		__BITS(5,2)	/* Serial IRQ Frame Number */
# define LPCHC_IRQCTL_SIRQFW		__BITS(1,0)	/* Serial IRQ Frame Width */
#  define IRQCTL_SIRQFW_4		0		
#  define IRQCTL_SIRQFW_6		1		
#  define IRQCTL_SIRQFW_8		2		
#  define IRQCTL_SIRQFW_RESV		3		
#define GEMINI_LPCHC_SERIRQSTS		0x0c						/* rwc */
# define LPCHC_SERIRQSTS_RESV		__BITS(31,17)
#define GEMINI_LPCHC_SERIRQTYP		0x10						/* rw */
# define LPCHC_SERIRQTYP_RESV		__BITS(31,17)
#  define SERIRQTYP_EDGE		1
#  define SERIRQTYP_LEVEL		0
#define GEMINI_LPCHC_SERIRQPOLARITY	0x14						/* rw */
# define LPCHC_SERIRQPOLARITY_RESV	__BITS(31,17)
#  define SERIRQPOLARITY_HI		1
#  define SERIRQPOLARITY_LO		0
#define GEMINI_LPCHC_SIZE		(GEMINI_LPCHC_SERIRQPOLARITY + 4)
#define GEMINI_LPCHC_NSERIRQ		17

/*
 * Gemini GPIO controller register offsets and bits
 */
#define GEMINI_GPIO_DATAOUT		0x00		/* Data Out */			/* rw */
#define GEMINI_GPIO_DATAIN		0x04		/* Data Out */			/* ro */
#define GEMINI_GPIO_PINDIR		0x08		/* Pin Direction */		/* rw */
#define  GPIO_PINDIR_INPUT		0
#define  GPIO_PINDIR_OUTPUT		1
#define GEMINI_GPIO_PINBYPASS		0x0c		/* Pin Bypass */		/* rw */
#define GEMINI_GPIO_DATASET		0x10		/* Data Set */			/* wo */
#define GEMINI_GPIO_DATACLR		0x14		/* Data Clear */		/* wo */
#define GEMINI_GPIO_PULLENB		0x18		/* Pullup Enable */		/* rw */
#define GEMINI_GPIO_PULLTYPE		0x1c		/* Pullup Type */		/* rw */
#define  GPIO_PULLTYPE_LOW		0
#define  GPIO_PULLTYPE_HIGH		1
#define GEMINI_GPIO_INTRENB		0x20		/* Interrupt Enable */		/* rw */
#define GEMINI_GPIO_INTRRAWSTATE	0x24		/* Interrupt Raw State */	/* ro */
#define GEMINI_GPIO_INTRMSKSTATE	0x28		/* Interrupt Masked State */	/* ro */
#define GEMINI_GPIO_INTRMASK		0x2c		/* Interrupt Mask */		/* rw */
#define GEMINI_GPIO_INTRCLR		0x30		/* Interrupt Clear */		/* wo */
#define GEMINI_GPIO_INTRTRIG		0x34		/* Interrupt Trigger Method */	/* rw */
#define  GPIO_INTRTRIG_EDGE		0
#define  GPIO_INTRTRIG_LEVEL		1
#define GEMINI_GPIO_INTREDGEBOTH	0x38		/* Both edges trigger Intr. */	/* rw */
#define GEMINI_GPIO_INTRDIR		0x3c		/* edge/level direction */	/* rw */
#define  GPIO_INTRDIR_EDGE_RISING	0
#define  GPIO_INTRDIR_EDGE_FALLING	1
#define  GPIO_INTRDIR_LEVEL_HIGH	0
#define  GPIO_INTRDIR_LEVEL_LOW		1
#define GEMINI_GPIO_BOUNCEENB		0x40		/* Bounce Enable */		/* rw */
#define GEMINI_GPIO_BOUNCESCALE		0x44		/* Bounce Pre-Scale */		/* rw */
#define  GPIO_BOUNCESCALE_RESV		__BITS(31,16)
#define  GPIO_BOUNCESCALE_VAL		__BITS(15,0)	/* NOTE:
							 * if bounce is enabled, and bounce pre-scale == 0
							 * then the pin will not detect any interrupt 
							 */
#define GEMINI_GPIO_SIZE		(GEMINI_GPIO_BOUNCESCALE + 4)

/*
 * Gemini PCI controller register offsets and bits
 */
#define GEMINI_PCI_IOSIZE		0x00		/* I/O Space Size */		/* rw */
#define GEMINI_PCI_PROT			0x04		/* AHB Protection */		/* rw */
#define GEMINI_PCI_PCICTRL		0x08		/* PCI Control Signal */	/* rw */
#define GEMINI_PCI_ERREN		0x0c		/* Soft Reset Counter and
							 * Response Error Enable */	/* rw */
#define GEMINI_PCI_SOFTRST		0x10		/* Soft Reset */
#define GEMINI_PCI_CFG_CMD		0x28		/* PCI Configuration Command */	/* rw */
#define  PCI_CFG_CMD_ENB		__BIT(31)	/*  Enable */
#define  PCI_CFG_CMD_RESa		__BITS(30,24)
#define  PCI_CFG_CMD_BUSNO		__BITS(23,16)	/*  Bus      Number */
#define  PCI_CFG_CMD_BUSn(n)		(((n) << 16) & PCI_CFG_CMD_BUSNO)
#define  PCI_CFG_CMD_DEVNO		__BITS(15,11)	/*  Device   Number */
#define  PCI_CFG_CMD_DEVn(n)		(((n) << 11) & PCI_CFG_CMD_DEVNO)
#define  PCI_CFG_CMD_FUNCNO		__BITS(10,8)	/*  Function Number */
#define  PCI_CFG_CMD_FUNCn(n)		(((n) << 8) & PCI_CFG_CMD_FUNCNO)
#define  PCI_CFG_CMD_REGNO		__BITS(7,2)	/*  Register Number */
#define  PCI_CFG_CMD_REGn(n)		(((n) << 0) & PCI_CFG_CMD_REGNO)
#define  PCI_CFG_CMD_RESb		__BITS(1,0)
#define  PCI_CFG_CMD_RESV	\
		(PCI_CFG_CMD_RESa | PCI_CFG_CMD_RESb)
#define GEMINI_PCI_CFG_DATA		0x2c		/* PCI Configuration Data */	/* rw */

/*
 * Gemini machine dependent PCI config registers
 */
#define	GEMINI_PCI_CFG_REG_PMR1		0x40		/* Power Management 1 */	/* rw */
#define	GEMINI_PCI_CFG_REG_PMR2		0x44		/* Power Management 2 */	/* rw */
#define	GEMINI_PCI_CFG_REG_CTL1		0x48		/* Control 1 */			/* rw */
#define	GEMINI_PCI_CFG_REG_CTL2		0x4c		/* Control 2 */			/* rw */
#define	 PCI_CFG_REG_CTL2_INTSTS	__BITS(31,28)
#define	  CFG_REG_CTL2_INTSTS_INTD	__BIT(28 + 3)
#define	  CFG_REG_CTL2_INTSTS_INTC	__BIT(28 + 2)
#define	  CFG_REG_CTL2_INTSTS_INTB	__BIT(28 + 1)
#define	  CFG_REG_CTL2_INTSTS_INTA	__BIT(28 + 0)
#define	 PCI_CFG_REG_CTL2_INTMASK	__BITS(27,16)
#define	  CFG_REG_CTL2_INTMASK_CMDERR	__BIT(16 + 11)
#define	  CFG_REG_CTL2_INTMASK_PARERR	__BIT(16 + 10)
#define	  CFG_REG_CTL2_INTMASK_INTD	__BIT(16 + 9)
#define	  CFG_REG_CTL2_INTMASK_INTC	__BIT(16 + 8)
#define	  CFG_REG_CTL2_INTMASK_INTB	__BIT(16 + 7)
#define	  CFG_REG_CTL2_INTMASK_INTA	__BIT(16 + 6)
#define	  CFG_REG_CTL2_INTMASK_INT_ABCD	__BITS(16+9,16+6)
#define	  CFG_REG_CTL2_INTMASK_MABRT_RX	__BIT(16 + 5)
#define	  CFG_REG_CTL2_INTMASK_TABRT_RX	__BIT(16 + 4)
#define	  CFG_REG_CTL2_INTMASK_TABRT_TX	__BIT(16 + 3)
#define	  CFG_REG_CTL2_INTMASK_RETRY4	__BIT(16 + 2)
#define	  CFG_REG_CTL2_INTMASK_SERR_RX	__BIT(16 + 1)
#define	  CFG_REG_CTL2_INTMASK_PERR_RX	__BIT(16 + 0)
#define	 PCI_CFG_REG_CTL2_RESa		__BIT(15)
#define	 PCI_CFG_REG_CTL2_MSTPRI	__BITS(14,8)
#define	  CFG_REG_CTL2_MSTPRI_REQn(n)	__BIT(8 + (n))
#define	 PCI_CFG_REG_CTL2_RESb		__BITS(7,4)
#define	 PCI_CFG_REG_CTL2_TRDYW		__BITS(3,0)
#define	 PCI_CFG_REG_CTL2_RESV	\
		(PCI_CFG_REG_CTL2_RESa | PCI_CFG_REG_CTL2_RESb)
#define	GEMINI_PCI_CFG_REG_MEM1		0x50		/* Memory 1 Base */		/* rw */
#define	GEMINI_PCI_CFG_REG_MEM2		0x54		/* Memory 2 Base */		/* rw */
#define	GEMINI_PCI_CFG_REG_MEM3		0x58		/* Memory 3 Base */		/* rw */
#define	 PCI_CFG_REG_MEM_BASE_MASK	__BITS(31,20)
#define	 PCI_CFG_REG_MEM_BASE(base)	((base) & PCI_CFG_REG_MEM_BASE_MASK)
#define	 PCI_CFG_REG_MEM_SIZE_MASK	__BITS(19,16)
#define	 PCI_CFG_REG_MEM_SIZE_1MB	(0x0 << 16)
#define	 PCI_CFG_REG_MEM_SIZE_2MB	(0x1 << 16)
#define	 PCI_CFG_REG_MEM_SIZE_4MB	(0x2 << 16)
#define	 PCI_CFG_REG_MEM_SIZE_8MB	(0x3 << 16)
#define	 PCI_CFG_REG_MEM_SIZE_16MB	(0x4 << 16)
#define	 PCI_CFG_REG_MEM_SIZE_32MB	(0x5 << 16)
#define	 PCI_CFG_REG_MEM_SIZE_64MB	(0x6 << 16)
#define	 PCI_CFG_REG_MEM_SIZE_128MB	(0x7 << 16)
#define	 PCI_CFG_REG_MEM_SIZE_256MB	(0x8 << 16)
#define	 PCI_CFG_REG_MEM_SIZE_512MB	(0x9 << 16)
#define	 PCI_CFG_REG_MEM_SIZE_1GB	(0xa << 16)
#define	 PCI_CFG_REG_MEM_SIZE_2GB	(0xb << 16)
#define	 PCI_CFG_REG_MEM_RESV		__BITS(19,16)

#ifndef _LOCORE
static inline unsigned int
gemini_pci_cfg_reg_mem_size(size_t sz)
{
	switch (sz) {
	case (1 << 20):
		return PCI_CFG_REG_MEM_SIZE_1MB;
	case (2 << 20):
		return PCI_CFG_REG_MEM_SIZE_2MB;
	case (4 << 20):
		return PCI_CFG_REG_MEM_SIZE_4MB;
	case (8 << 20):
		return PCI_CFG_REG_MEM_SIZE_8MB;
	case (16 << 20):
		return PCI_CFG_REG_MEM_SIZE_16MB;
	case (32 << 20):
		return PCI_CFG_REG_MEM_SIZE_32MB;
	case (64 << 20):
		return PCI_CFG_REG_MEM_SIZE_64MB;
	case (128 << 20):
		return PCI_CFG_REG_MEM_SIZE_128MB;
	case (256 << 20):
		return PCI_CFG_REG_MEM_SIZE_256MB;
	case (512 << 20):
		return PCI_CFG_REG_MEM_SIZE_512MB;
	case (1024 << 20):
		return PCI_CFG_REG_MEM_SIZE_1GB;
	case (2048 << 20):
		return PCI_CFG_REG_MEM_SIZE_2GB;
	default:
		panic("gemini_pci_cfg_reg_mem_size: bad size %#lx\n", sz);
	}
	/* NOTREACHED */
}
#endif	/* _LOCORE */

/*
 * Gemini SL3516 IDE device register offsets, &etc.
 */
#define GEMINI_MIDE_NCHAN		2
#define GEMINI_MIDE_OFFSET(chan)	((chan == 0) ? 0x0 : 0x400000)
#define GEMINI_MIDE_BASEn(chan)		(GEMINI_MIDE_BASE + GEMINI_MIDE_OFFSET(chan))
#define GEMINI_MIDE_CMDBLK		0x20
#define GEMINI_MIDE_CTLBLK		0x36
#define GEMINI_MIDE_SIZE		0x40


/*
 * Gemini DRAM Controller register offsets, &etc.
 */
#define GEMINI_DRAMC_RMCR		0x40		/* CPU Remap Control */				/* rw */
#define  DRAMC_RMCR_RESa		__BITS(31,29)
#define  DRAMC_RMCR_RMBAR		__BITS(28,20)	/* Remap Base Address */
#define  DRAMC_RMCR_RMBAR_SHFT		20
#define  DRAMC_RMCR_RESb		__BITS(19,9)
#define  DRAMC_RMCR_RMSZR		__BITS(8,0)	/* Remap Size Address */
#define  DRAMC_RMCR_RMSZR_SHFT		0

#else
# error unknown gemini cpu type
#endif

#endif	/* _ARM_GEMINI_REG_H_ */
