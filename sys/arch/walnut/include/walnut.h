/*	$NetBSD: walnut.h,v 1.3.2.2 2002/04/01 07:43:37 nathanw Exp $	*/

/* include/eval.h, openbios_walnut, walnut_bios 8/10/00 14:35:05 */
/*-----------------------------------------------------------------------------+
|
|       This source code has been made available to you by IBM on an AS-IS
|       basis.  Anyone receiving this source is licensed under IBM
|       copyrights to use it in any way he or she deems fit, including
|       copying it, modifying it, compiling it, and redistributing it either
|       with or without modifications.  No license under IBM patents or
|       patent applications is to be implied by the copyright license.
|
|       Any user of this software should understand that IBM cannot provide
|       technical support for this software and will not be responsible for
|       any consequences resulting from the use of this software.
|
|       Any person who transfers this source code or any derivative work
|       must include the IBM copyright notice, this paragraph, and the
|       preceding two paragraphs in the transferred software.
|
|       COPYRIGHT   I B M   CORPORATION 1995
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------+
|
|  File Name:   eval.h
|
|  Function:    Openbios board specific defines. Should contain no 
|		prototypes since this file gets included in assembly files.
|
|  Author:      James Burke 
|
|  Change Activity-
|
|  Date        Description of Change                                       BY
|  ---------   ---------------------                                       ---
|  11-May-99   Created for Walnut                                          JWB
|  01-Jul-99   Made ROM/SRAM non-cacheable in D_CACHEABLE_REGIONS          JWB
|  08-Aug-00   Added memory regions and MMIO regions for ROM Monitor debug JWB
|  10-Aug-00   Modified PCI memory regions                                 JWB
|
+-----------------------------------------------------------------------------*/

#ifndef _WALNUT_H_
#define _WALNUT_H_

/*----------------------------------------------------------------------------+
| 405GP PCI core memory map defines.
+----------------------------------------------------------------------------*/
#define MIN_PCI_MEMADDR_NOPREFETCH	0x80000000
#define MIN_PCI_MEMADDR_PREFETCH	0xc0000000
#define MIN_PCI_MEMADDR_VGA    		0x00000000
#define MIN_PLB_PCI_IOADDR  0xe8000000  /* PLB side of PCI I/O address space */
#define MIN_PCI_PCI_IOADDR  0x00000000  /* PCI side of PCI I/O address space */
#define MAX_PCI_DEVICES     5

#define	SRAM_START_ADDR	0xfff00000	/* SRAM starting addr */
#define	SRAM_SIZE	0x80000		/* SRAM size - 512K */

/*----------------------------------------------------------------------------+
| Universal Interrupt Controller (UIC) events for the Walnut board.
+----------------------------------------------------------------------------*/
/* Walnut board external IRQs */
#define EXT_IRQ_FPGA      	UIC_E0IS   	/* IRQ 25 */ 
#define EXT_IRQ_SMI      	UIC_E1IS        /* IRQ 26 */
#define EXT_IRQ_UNUSED 		UIC_E2IS    	/* IRQ 27 */ 
#define EXT_IRQ_PCI_SLOT3 	UIC_E3IS        /* IRQ 28 */
#define EXT_IRQ_PCI_SLOT2 	UIC_E4IS        /* IRQ 29 */
#define EXT_IRQ_PCI_SLOT1 	UIC_E5IS        /* IRQ 30 */
#define EXT_IRQ_PCI_SLOT0 	UIC_E6IS        /* IRQ 31 */

#define EXT_IRQ_CASCADE  	EXT_IRQ_FPGA 
#define EXT_IRQ_EXPANSION	EXT_IRQ_FPGA 
#define EXT_IRQ_IR       	EXT_IRQ_FPGA
#define EXT_IRQ_KEYBOARD	EXT_IRQ_FPGA
#define EXT_IRQ_MOUSE  		EXT_IRQ_FPGA

/*-----------------------------------------------------------------------------+
| Defines for the RTC/NVRAM.
+-----------------------------------------------------------------------------*/
#define NVRAM_BASE      0xf0000000
#if 0
#define RTC_CONTROL     0x1ff8
#define RTC_SECONDS     0x1ff9
#define RTC_MINUTES     0x1ffa
#define RTC_HOURS       0x1ffb
#define RTC_DAY         0x1ffc
#define RTC_DATE        0x1ffd
#define RTC_MONTH       0x1ffe
#define RTC_YEAR        0x1fff
#endif

/*-----------------------------------------------------------------------------+
| Defines for the Keyboard/Mouse controller.
+-----------------------------------------------------------------------------*/
#define KEY_MOUSE_BASE  0xf0100000
#define KEY_MOUSE_DATA  0x0
#define KEY_MOUSE_CMD   0x1  /* write only */
#define KEY_MOUSE_STAT  0x1  /* read only */

/*-----------------------------------------------------------------------------+
| Defines for FPGA regs.
+-----------------------------------------------------------------------------*/
#define	FPGA_BASE	0xf0300000
#define FPGA_INT_STATUS 0x00    /* Int status - read only 	*/
#define	 FPGA_SW_SMI	  0x10     /* SW_SMI_N present */
#define	 FPGA_EXT_IRQ	  0x08     /* EXT_IRQ present */
#define	 FPGA_IRQ_IRDA	  0x04     /* IRQ_IRDA present */
#define	 FPGA_IRQ_KYBD	  0x02     /* IRQ_KYBD present */
#define	 FPGA_IRQ_MOUSE	  0x01     /* IRQ_MOUSE present */
#define FPGA_INT_ENABLE 0x01    /* Int enable 			*/
      /* FPGA_SW_SMI  */	   /* enable SW_SMI_N */
      /* FPGA_EXT_IRQ  */	   /* enable FPGA_EXT_IRQ */
      /* FPGA_IRQ_IRDA  */	   /* enable FPGA_IRQ_IRDA */
      /* FPGA_IRQ_KYBD  */	   /* enable FPGA_IRQ_KYBD */
      /* FPGA_IRQ_MOUSE  */	   /* enable FPGA_IRQ_MOUSE */
#define FPGA_INT_POL    0x02    /* Int polarity 		*/
      /* FPGA_SW_SMI  */	   /* SW_SMI_N active high/rising */
      /* FPGA_EXT_IRQ  */	   /* FPGA_EXT_IRQ active high/rising */
      /* FPGA_IRQ_IRDA  */	   /* FPGA_IRQ_IRDA active high/rising */
      /* FPGA_IRQ_KYBD  */	   /* FPGA_IRQ_KYBD active high/rising */
      /* FPGA_IRQ_MOUSE  */	   /* FPGA_IRQ_MOUSE active high/rising */
#define FPGA_INT_TRIG   0x03    /* Int type 			*/
      /* FPGA_SW_SMI  */	   /* SW_SMI_N level */
      /* FPGA_EXT_IRQ  */	   /* FPGA_EXT_IRQ level */
      /* FPGA_IRQ_IRDA  */	   /* FPGA_IRQ_IRDA level */
      /* FPGA_IRQ_KYBD  */	   /* FPGA_IRQ_KYBD level */
      /* FPGA_IRQ_MOUSE  */	   /* FPGA_IRQ_MOUSE level */
#define FPGA_BRDC       0x04    /* Board controls 		*/
#define	 FPGA_BRDC_INT	 0x80	   /* IRQ_MOUSE is separate */
#define	 FPGA_BRDC_TC3	 0x10	   /* DMA_EOT/TC3 is set to TC */
#define	 FPGA_BRDC_TC2	 0x08	   /* DMA_EOT/TC2 is set to TC */
#define	 FPGA_BRDC_DIS_EI 0x04	   /* disable expansion interface */
#define	 FPGA_BRDC_EN_INV 0x02	   /* enable invalid address checking */
#define	 FPGA_BRDC_UART_CR 0x01	   /* UART1 is set to CTS/RTS */
#define FPGA_BRDS1      0x05    /* Board status - read only 	*/
#define	 FPGA_BRDS1_CLK  0x04	   /* 405 SDRAM CLK disabled, MPC972 used */
#define	 FPGA_BRDS1_FLASH_EN 0x02  /* On board FLASH disabled */
#define	 FPGA_BRDS1_FLASH_SEL 0x01 /* FLASH at low addr */
#define FPGA_BRDS2      0x06    /* Board status - read only */
#define  SW_CLK_SRC1     0x40      /* if async pci, ext or int clk */
#define	 SW_SEL1	 0x20	   /* use test clock for master clock */
#define	 SW_SEL0	 0x10	   /* use 405GP arbiter */
#define  FSEL_B          0x0c      /* use for mask */
#define  FSEL_SDRAM100   0x01      /* select 100Mhz SDRAM */
#define  FSEL_SDRAM66    0x03      /* select 66Mhz SDRAM */   
#define  FSEL_A          0x03      /* use for mask */
#define  FSEL_PCI_66     0x01      /* select 66Mhz async int PCI */
#define  FSEL_PCI_33     0x03      /* select 33Mhz async int PCI */   
#define FPGA_SPARE1     0x0e    /* Spare inputs - read only */
#define FPGA_SPARE2     0x0f    /* Spare outputs */
#define	FPGA_SIZE	FPGA_SPARE2

#endif /* _WALNUT_H_ */
