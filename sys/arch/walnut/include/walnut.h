/*	$NetBSD: walnut.h,v 1.2 2001/06/24 01:13:11 simonb Exp $	*/

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

#if 0
/*-----------------------------------------------------------------------------+
| Defines for FPGA regs.
+-----------------------------------------------------------------------------*/
#define	FPGA_BASE	0xf0300000
#define FPGA_INT_STATUS 0x00    /* Int status - read only 	*/
#define FPGA_INT_ENABLE 0x01    /* Int enable 			*/
#define FPGA_INT_POL    0x02    /* Int polarity 		*/
#define FPGA_INT_TRIG   0x03    /* Int type 			*/
#define FPGA_BRDC       0x04    /* Board controls 		*/
#define FPGA_BRDS1      0x05    /* Board status - read only 	*/
#define FPGA_BRDS2      0x06    /* Board status - read only       */
#define  SW_CLK_SRC1     0x40      /* if async pci, ext or int clk   */
#define  FSEL_A          0x03      /* use for mask                   */
#define  FSEL_INVALID1   0x00      /* invalid FSEL_A0, FSEL_A1 settings*/
#define  FSEL_INVALID2   0x02      /* invalid FSEL_A0, FSEL_A1 settings*/
#define  FSEL_66         0x01      /* select 66Mhz async int PCI	*/
#define  FSEL_33         0x03      /* select 33Mhz async int PCI     */   
#define FPGA_SPARE1     0x0e    /* Spare inputs - read only       */
#define FPGA_SPARE2     0x0f    /* Spare outputs                  */
#endif

#ifndef _LOCORE
/*
 * Board configuration structure from the OpenBIOS.
 */
struct board_cfg_data {
	unsigned char	usr_config_ver[4];
	unsigned char	rom_sw_ver[30];
	unsigned int	mem_size;
	unsigned char	mac_address_local[6];
	unsigned char	mac_address_pci[6];
	unsigned int	processor_speed;
	unsigned int	plb_speed;
	unsigned int	pci_speed;
} board_data;

extern struct board_cfg_data board_data;
#endif /* _LOCORE */
#endif /* _WALNUT_H_ */
