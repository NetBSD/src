/*	$NetBSD: walnut.h,v 1.1 2001/06/13 06:01:59 simonb Exp $	*/

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

/*-----------------------------------------------------------------------------+
| 405GP PVR.
+-----------------------------------------------------------------------------*/
#define PVR_405GP      		0x40110000 
#define PVR_405GP_PASS1 	0x40110000	/* RevA */ 
#define PVR_405GP_PASS2 	0x40110040	/* RevB */ 
#define PVR_405GP_PASS2_1 	0x40110082	/* RevC */ 
#define PVR_405GP_PASS3 	0x401100C4	/* RevD */ 

/*-----------------------------------------------------------------------------+
| 405GP UART Base Address defines.                        
+-----------------------------------------------------------------------------*/
#define UART0_BASE	0xEF600300
#define UART1_BASE	0xEF600400

/*-----------------------------------------------------------------------------+
| 405GP IIC Base Address define.                        
+-----------------------------------------------------------------------------*/
#define IIC0_BASE 	0xEF600500

/*----------------------------------------------------------------------------+
| 405GP PCI core memory map defines.
+----------------------------------------------------------------------------*/
#define MIN_PCI_MEMADDR_NOPREFETCH	0x80000000
#define MIN_PCI_MEMADDR_PREFETCH	0xC0000000
#define MIN_PCI_MEMADDR_VGA    		0x00000000
#define MIN_PLB_PCI_IOADDR  0xE8000000  /* PLB side of PCI I/O address space */
#define MIN_PCI_PCI_IOADDR  0x00000000  /* PCI side of PCI I/O address space */
#define MAX_PCI_DEVICES     5

/*----------------------------------------------------------------------------+
| Defines for the 405GP PCI Config address and data registers followed by
| defines for the standard PCI device configuration header.
+----------------------------------------------------------------------------*/
#define PCI0_CFGADDR       0xEEC00000
#define PCI0_CFGDATA       0xEEC00004

#define PCI0_VENDID    0x00
#define PCI0_DEVID     0x02
#define PCI0_CMD       0x04
  #define SE	   0x0100
  #define ME       0x0004
  #define MA       0x0002
  #define IOA      0x0001
#define PCI0_STATUS    0x06
  #define C66      0x0020
#define PCI0_REVID     0x08
#define PCI0_CLS       0x09
  #define PCI0_INTCLS  0x09
  #define PCI0_SUBCLS  0x0A
  #define PCI0_BASECLS 0x0B
#define PCI0_CACHELS   0x0C
#define PCI0_LATTIM    0x0D
#define PCI0_HDTYPE    0x0E
#define PCI0_BIST      0x0F
#define PCI0_BAR0      0x10	
#define PCI0_BAR1      0x14   /* PCI name */ 
#define PCI0_PTM1BAR   0x14   /* 405GP name */
#define PCI0_BAR2      0x18   /* PCI name */
#define PCI0_PTM2BAR   0x18   /* 405GP name */
#define PCI0_BAR3      0x1C	
#define PCI0_BAR4      0x20	
#define PCI0_BAR5      0x24	
#define PCI0_SBSYSVID  0x2C
#define PCI0_SBSYSID   0x2E
#define PCI0_CAP       0x34
#define PCI0_INTLN     0x3C
#define PCI0_INTPN     0x3D
#define PCI0_MINGNT    0x3E
#define PCI0_MAXLTNCY  0x3F

#define PCI0_ICS       0x44        /* 405GP specific parameters */
#define PCI0_ERREN     0x48    
#define PCI0_ERRSTS    0x49    
#define PCI0_BRDGOPT1  0x4A    
#define PCI0_PLBBESR0  0x4C    
#define PCI0_PLBBESR1  0x50    
#define PCI0_PLBBEAR   0x54    
#define PCI0_CAPID     0x58    
#define PCI0_NEXTIPTR  0x59    
#define PCI0_PMC       0x5A    
#define PCI0_PMCSR     0x5C    
#define PCI0_PMCSRBSE  0x5E    
#define PCI0_BRDGOPT2  0x60    
#define PCI0_PMSCRR    0x64    

/*----------------------------------------------------------------------------+
| Defines for 405GP PCI Master local configuration regs.
+----------------------------------------------------------------------------*/
#define PCIL0_PMM0LA          0xEF400000
#define PCIL0_PMM0MA          0xEF400004
#define PCIL0_PMM0PCILA       0xEF400008
#define PCIL0_PMM0PCIHA       0xEF40000C
#define PCIL0_PMM1LA          0xEF400010
#define PCIL0_PMM1MA          0xEF400014
#define PCIL0_PMM1PCILA       0xEF400018
#define PCIL0_PMM1PCIHA       0xEF40001C
#define PCIL0_PMM2LA          0xEF400020
#define PCIL0_PMM2MA          0xEF400024
#define PCIL0_PMM2PCILA       0xEF400028
#define PCIL0_PMM2PCIHA       0xEF40002C

/*----------------------------------------------------------------------------+
| Defines for 405GP PCI Target local configuration regs.
+----------------------------------------------------------------------------*/
#define PCIL0_PTM1MS          0xEF400030
#define PCIL0_PTM1LA          0xEF400034
#define PCIL0_PTM2MS          0xEF400038
#define PCIL0_PTM2LA          0xEF40003C

/*-----------------------------------------------------------------------------+
|       PCI-PCI bridge header
+-----------------------------------------------------------------------------*/
#define PCIPCI_PRIMARYBUS       0x18
#define PCIPCI_SECONDARYBUS     0x19
#define PCIPCI_SUBORDINATEBUS   0x1A
#define PCIPCI_SECONDARYLATENCY 0x1B
#define PCIPCI_IOBASE           0x1C
#define PCIPCI_IOLIMIT          0x1D
#define PCIPCI_SECONDARYSTATUS  0x1E
#define PCIPCI_MEMBASE          0x20
#define PCIPCI_MEMLIMIT         0x22
#define PCIPCI_PREFETCHMEMBASE  0x24
#define PCIPCI_PREFETCHMEMLIMIT 0x26
#define PCIPCI_IOBASEUPPER16    0x30
#define PCIPCI_IOLIMITUPPER16   0x32

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
#define NVRAM_BASE      0xF0000000
#define RTC_CONTROL     0xF0001FF8
#define RTC_SECONDS     0xF0001FF9
#define RTC_MINUTES     0xF0001FFA
#define RTC_HOURS       0xF0001FFB
#define RTC_DAY         0xF0001FFC
#define RTC_DATE        0xF0001FFD
#define RTC_MONTH       0xF0001FFE
#define RTC_YEAR        0xF0001FFF

/*-----------------------------------------------------------------------------+
| Defines for the Keyboard/Mouse controller.
+-----------------------------------------------------------------------------*/
#define KEY_MOUSE_BASE  0xF0100000
#define KEY_MOUSE_DATA  0xF0100000
#define KEY_MOUSE_CMD   0xF0100001  /* write only */
#define KEY_MOUSE_STAT  0xF0100001  /* read only */

/*-----------------------------------------------------------------------------+
| Defines for FPGA regs.
+-----------------------------------------------------------------------------*/
#define FPGA_INT_STATUS 0xF0300000    /* Int status - read only 	*/
#define FPGA_INT_ENABLE 0xF0300001    /* Int enable 			*/
#define FPGA_INT_POL    0xF0300002    /* Int polarity 			*/
#define FPGA_INT_TRIG   0xF0300003    /* Int type 			*/
#define FPGA_BRDC       0xF0300004    /* Board controls 		*/
#define FPGA_BRDS1      0xF0300005    /* Board status - read only 	*/
#define FPGA_BRDS2      0xF0300006    /* Board status - read only       */
#define SW_CLK_SRC1     0x40          /* if async pci, ext or int clk   */
#define FSEL_A          0x03          /* use for mask                   */
#define FSEL_INVALID1   0x00          /* invalid FSEL_A0, FSEL_A1 settings*/
#define FSEL_INVALID2   0x02          /* invalid FSEL_A0, FSEL_A1 settings*/
#define FSEL_66         0x01          /* select 66Mhz async int PCI	*/
#define FSEL_33         0x03          /* select 33Mhz async int PCI     */   
#define FPGA_SPARE1     0xF030000E    /* Spare inputs - read only       */
#define FPGA_SPARE2     0xF030000F    /* Spare outputs                  */

/*-----------------------------------------------------------------------------+
| Misc defines
+-----------------------------------------------------------------------------*/
#define STACK_SIZE 	0x2000          /* ROM Monitor stack size */
#define ONE_BILLION	1000000000
#define ONE_MILLION	1000000
#define HWADDRLEN       6
#define MAXDEVICES      6
#define ENTRY_PT_MIN    0x25000         /* min entry point for an application */
					/* loaded by the ROM Monitor */
#define MAXPACKET       1518
#define ENET_MINPACKET  64
#define SRAM_START_ADDR 0xFFF00000      /* SRAM starting addr */
#define SRAM_SIZE 	0x80000         /* SRAM size - 512K */
#define VPD_ADDR        0xFFFFFE00      /* Vital Product Data area in ROM */
#define TEXTADDR        0xFFFE0000      /* start of ROM Monitor text in ROM */
#define DATAADDR        0x00006000      /* start of ROM Monitor data in RAM */
#define NO_BAUD  	0
#define S1_BAUD  	1
#define I_CACHEABLE_REGIONS       0x80000001  /* DRAM, ROM, and SRAM regions */
#define D_CACHEABLE_REGIONS       0x80000000  /* DRAM region only */
#define NVRVFY1 	0x4f532d4f    /* used to determine if state data in */
#define NVRVFY2 	0x50454e00    /* NVRAM initialized (ascii for OS-OPEN)*/
#define RFI             0x4c000064    /* RFI instruction */
#define PCIDEVID_405GP 	0x0
#define PCI_SLOT0_DEVID 0x4           /* Note: on Walnut slot 0 is furthest */ 
#define PCI_SLOT1_DEVID 0x3           /* slot away from the processor card */
#define PCI_SLOT2_DEVID 0x2
#define PCI_SLOT3_DEVID 0x1
#define VCO_MIN		400           /* VCO min frequency in MHz */
#define VCO_MAX		800           /* VCO max frequency in MHz */
#define asyncClockRate  11059200      /* ext serial clock for UARTs */

/*-----------------------------------------------------------------------------+
| Define readable memory spaces when in ROM Monitor debug mode.
| No need to specify DRAM region; handled in check_address() in dbgreg.c. 
+-----------------------------------------------------------------------------*/
#define MEM0_START	 0x0		/* Reserved for DRAM */
#define MEM0_END         0x0
#define MEM1_START       0x80000000     /* PCI Memory and IO */
#define MEM1_END         0xE800FFFF
#define MEM2_START       0xE8800000     /* PCI IO */
#define MEM2_END         0xEBFFFFFF
#define MEM3_START       0xF0000000	/* Walnut board NVRAM/FPGA space */
#define MEM3_END         0xF030000F
#define MEM4_START       0xFFF00000	/* Walnut board ROM/SRAM space */
#define MEM4_END         0xFFFFFFFF
#define MEM5_START       0x0     	/* Unused */
#define MEM5_END         0x0       
#define MEM6_START       0x0            /* Unused */
#define MEM6_END         0x0       
#define MEM7_START       0x0            /* Unused */
#define MEM7_END         0x0       

/*-----------------------------------------------------------------------------+
| Define MMIO memory spaces when in ROM Monitor debug mode.
+-----------------------------------------------------------------------------*/
#define MIO0_START       0xEEC00000     /* PCI Config ADDr and Data Regs */
#define MIO0_END         0xEEC00007
#define MIO0_WIDTH       4		/* register width for region in bytes*/
#define MIO1_START       0xEF400000     /* PCI Local Config Regs */
#define MIO1_END         0xEF40003F
#define MIO1_WIDTH       4         
#define MIO2_START       0xEF600300     /* UART0 Regs */
#define MIO2_END         0xEF600307
#define MIO2_WIDTH       1         
#define MIO3_START       0xEF600400     /* UART1 Regs */
#define MIO3_END         0xEF600407
#define MIO3_WIDTH       1         
#define MIO4_START       0xEF600500     /* I2C Regs */
#define MIO4_END         0xEF600513
#define MIO4_WIDTH       1         
#define MIO5_START       0xEF600700     /* GPIO Regs */
#define MIO5_END         0xEF60077F
#define MIO5_WIDTH       4         
#define MIO6_START       0xEF600800     /* EMAC Regs */
#define MIO6_END         0xEF600867
#define MIO6_WIDTH       4         
#define MIO7_START       0x0            /* unused   */
#define MIO7_END         0x0       
#define MIO7_WIDTH       0         

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
