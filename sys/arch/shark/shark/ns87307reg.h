/*	$NetBSD: ns87307reg.h,v 1.1.14.2 2002/06/23 17:41:31 jdolecek Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
**++
** 
**  FACILITY:
**
**     ns87307.h
**     
**  ABSTRACT:
**
**     
**
**  AUTHORS:
**
**    Patrick Crilly Digital Equipment Corporation.
**
**  CREATION DATE:
**
**    25-February-1997
**
**--
*/

#ifndef _NS87307REG_H
#define _NS87307REG_H

/*
** Define TRUE/FALSE if not already defined.  It
** annoys me that C doesn't do this in a standard 
** header.
*/
#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

/*
** Macro for debugging
*/
#define NSIODEBUG
#ifdef  NSIODEBUG
#define nsioDebug(x) printf x
#else
#define nsioDebug(x)                  
#endif

/*
** Base address of chip.  Used to configure console
** devices.
*/
#ifndef CONNSIOADDR
#define CONNSIOADDR 0x15c
#endif

/*
** Base Register Offsets
*/
#define NSIO_OFFSET_INDEX       0x00  /* Index register           */
#define NSIO_OFFSET_DATA        0x01  /* Data register            */

/*
** Number of io ports
*/
#define NSIO_NPORTS               2  /* Number of io ports        */

/*
** Card Configuration Registers
*/

#define NSIO_CFG_LOGDEV         0x07   /* Select logical device        */
#define NSIO_CFG_SID            0x20   /* Chip SID register            */
 
/*
** Logical Device Configuration Registers
*/

#define NSIO_CFG_ACTIVATE        0x30  /* activate register            */
#define NSIO_CFG_IORNGCHK        0x31  /* I/O range check register     */
#define NSIO_CFG_IOBASEH         0x60  /* IO port base bits 15-8       */
#define NSIO_CFG_IOBASEL         0x61  /* IO port base bits 7-0        */
#define NSIO_CFG_IRQ             0x70  /* Interrupt Request Level      */
#define NSIO_CFG_IRQTYPE         0x71  /* Interrupt Request Type       */
#define NSIO_CFG_DMA1            0x74  /* DMA channel for DMA 0        */
#define NSIO_CFG_DMA2            0x75  /* DMA channel for DMA 1        */
#define NSIO_CFG_REG0            0xF0  /* First configuration register */

/*
** KBC Configuration Registers
*/
#define NSIO_KBC_DATAH           0x60  /* kbc data address bits 15-8   */
#define NSIO_KBC_DATAL           0x61  /* kbc data address bits  7-0   */
#define NSIO_KBC_CMDH            0x62  /* kbc cmd  address bits 15-8   */
#define NSIO_KBC_CMDL            0x63  /* kbc cmd  address bits  7-0   */
#define NSIO_KBC_CFG             0xF0  /* kbc Super I/O cfg register   */

/*
** Chip SID defines
*/
#define NSIO_CHIP_ID_MASK        0xFC  /* Mask to obtain chip id       */
#define NSIO_CHIP_ID             0xC0  /* ns87307 chip id              */

/*
** Interrupt Type Masks
*/
#define NSIO_IRQ_LEVEL           0x01	 /* Trigger, level = 1 edge = 0 */
#define NSIO_IRQ_HIGH            0x02	 /* Int level, high = 1 low = 0 */
#define NSIO_IRQ_LOW             0x00    

/*
** IO Range check bit masks
*/
#define NSIO_RNGCHK_ENABLE       0x02   /* Enable IO range checking     */
#define NSIO_RNGCHK_USE55        0x01   /* Set to read 0x55             */

/*
** Logical Devices
*/
#define NSIO_DEV_KBC                 0	 /* keyboard controller         */
#define NSIO_DEV_MOUSE               1	 /* mouse controller            */
#define NSIO_DEV_RTC                 2	 /* real-time clock             */
#define NSIO_DEV_FDC                 3	 /* floppy disk controller      */
#define NSIO_DEV_LPT                 4	 /* parallel port               */
#define NSIO_DEV_USI                 5	 /* USI = UART + inafred        */
#define NSIO_DEV_UART                6	 /* UART                        */
#define NSIO_DEV_GPIO                7	 /* gerneral purpose I/O        */
#define NSIO_DEV_PWR                 8	 /* power management            */



/*
** PARREL PORT CONFIGURATION
*/

#define NSIO_LPT_TRISTATE_DISABLE    (0  << 0)    /* tri-state when inactive */
#define NSIO_LPT_TRISTATE_ENABLE     (1  << 0)    /* tri-state when inactive */

#define NSIO_LPT_CLOCK_DISABLE      (0  << 1)    /* report ecp mode */
#define NSIO_LPT_CLOCK_ENABLE       (1  (( 1)    /* ecp/epp timeout function when active */

                                    /* bit 2 reserved */

#define NSIO_LPT_REPORT_ECP         (0  << 3)    /* report ecp mode */
#define NSIO_LPT_REPORT_SPP         (1  << 3)    /* report spp mode */

#define NSIO_LPT_REG403_DISABLE     (0  << 4)    /* dont respond to reg 403 */
#define NSIO_LPT_REG403_ENABLE      (1  << 4)    /* respond to reg 403 */

#define NSIO_LPT_SPP_NORMAL         (0x0<< 5)
#define NSIO_LPT_SPP_EXTENDED       (0x1<< 5)
#define NSIO_LPT_EPP_V_1_7          (0x2<< 5)
#define NSIO_LPT_EPP_V_1_9          (0x3<< 5)
#define NSIO_LPT_ECP_NO_EPP         (0x4<< 5)
                          /* bit7-5 0x5 reserved */
                          /* bit7-5 0x6 reserved */
#define NSIO_LPT_ECP_AND_EPP4       (0x7<< 5)

/*
** As there are two devices which can be used for serial
** communication, set up defines so it's easy to configure 
** the logical devices used for serial communication. 
*/
#define NSIO_DEV_COM0           NSIO_DEV_UART
#define NSIO_DEV_COM1           NSIO_DEV_USI

/*---------------------------------------------------------------------------*/
/*	       Macros used to configure logical devices                      */
/*---------------------------------------------------------------------------*/

/* 
** NSIO_READ_REG
**
** Read a configuration register.  Use the currently
** selected logical device. 
**
** sc       pointer to nsio devices softc 
** reg      index of register to read
** value    value read from register
*/
#define NSIO_READ_REG( iot, ioh, reg, value ) \
{ \
    bus_space_write_1(iot, ioh, NSIO_OFFSET_INDEX, reg ); \
    value = bus_space_read_1( iot, ioh, NSIO_OFFSET_DATA ); \
}

/* 
** NSIO_WRITE_REG
**
** Write to a configuration register.  Use the currently
** selected logical device. 
**
** sc       pointer to nsio devices softc 
** reg      index of register to read
** value    value read from register
*/
#define NSIO_WRITE_REG( iot, ioh, reg, value ) \
{ \
    bus_space_write_1(iot, ioh, NSIO_OFFSET_INDEX, reg ); \
    bus_space_write_1( iot, ioh, NSIO_OFFSET_DATA, value ); \
}

/*
** NSIO_SELECT_DEV
** 
** Select logDev as the current the logical device
**
** sc       pointer to nsio devices softc 
** reg      index of register to read
** logDev   logical device to select
*/
#define NSIO_SELECT_DEV( iot, ioh, logDev ) \
    NSIO_WRITE_REG( iot, ioh, NSIO_CFG_LOGDEV, logDev )

/* 
** NSIO_CONFIG_IRQ
**
** Set the irq number and triggering for the currently
** selected logical device.  If irqNum is unknown 
** the number won't be set and the device will be left
** with it's default value. 
**
** sc        pointer to nsio devices softc 
** irqNum    irq number to set
** irqType   trigger flags e.g. edge/level, high/low
*/
#define NSIO_CONFIG_IRQ( iot, ioh, irqNum, irqType ) \
{ \
   if ( irqNum != ISACF_IRQ_DEFAULT ) \
   { \
	NSIO_WRITE_REG( iot, ioh, NSIO_CFG_IRQ, irqNum ); \
   } \
   NSIO_WRITE_REG( iot, ioh, NSIO_CFG_IRQTYPE, irqType ); \
}	  

/*
** NSIO_CONFIG_KBCCMD
** 
** Set the io base for the currently selected logical device. 
**
** sc         pointer to nsio devices softc 
** ioBase     address to set io base to
*/
#define NSIO_CONFIG_IOBASE( iot, ioh, ioBase ) \
{ \
   NSIO_WRITE_REG( iot, ioh, NSIO_CFG_IOBASEH, \
		  (unsigned char)(((ioBase) >> 8) & 0xff) ); \
   NSIO_WRITE_REG( iot, ioh, NSIO_CFG_IOBASEL, \
		  (unsigned char)(((ioBase) & 0xff ) ); \
}

/*
** NSIO_CONFIG_KBCDATA
** 
** Set the data base for the keyboard.  The keyboard 
** controller must be the currently selected logical device. 
**
** sc         pointer to nsio devices softc 
** dataBase   address to set data base to
*/

#define NSIO_CONFIG_KBCDATA( iot, ioh, dataBase ) \
{ \
   NSIO_WRITE_REG( iot, ioh, NSIO_KBC_DATAH, \
		  (unsigned char)(((dataBase) >> 8) & 0xff) ); \
   NSIO_WRITE_REG( iot, ioh, NSIO_KBC_DATAL, \
		  (unsigned char)((dataBase) & 0xff ) ); \
}

/*
** NSIO_CONFIG_KBCCMD
** 
** Set the command base for the keyboard.  The keyboard 
** controller must be the currently selected logical device. 
**
** sc         pointer to nsio devices softc 
** cmdBase    address to set command base to
*/

#define NSIO_CONFIG_KBCCMD( iot, ioh, cmdBase ) \
{ \
   NSIO_WRITE_REG( iot, ioh, NSIO_KBC_CMDH, \
		  (unsigned char)(((cmdBase) >> 8) & 0xff) ); \
   NSIO_WRITE_REG( iot, ioh, NSIO_KBC_CMDL, \
		  (unsigned char)((cmdBase) & 0xff ) ); \
}

/* 
** NSIO_ACTIVATE_DEV
**
** Activate the currently selected logical device. 
**
** sc    pointer to nsio devices softc 
*/

#define NSIO_ACTIVATE_DEV( iot, ioh ) \
{ \
   NSIO_WRITE_REG( iot, ioh, NSIO_CFG_ACTIVATE, (0x01) ); \
}

/* 
** NSIO_DEACTIVATE_DEV
**
** Deactivate the currently selected logical device. 
**
** sc    pointer to nsio devices softc 
*/

#define NSIO_DEACTIVATE_DEV( iot, ioh ) \
{ \
   NSIO_WRITE_REG( iot, ioh, NSIO_CFG_ACTIVATE, (0x00) ); \
}

                                                                        
/* 
** NSIO_CONFIG_DEBUG
**
** Print out configuration information for the device
**
** sc    pointer to nsio devices softc
*/
#ifdef DDB
#define NSIO_CONFIG_DEBUG( iot, ioh ) \
{ \
    /* nsioConfigPrint( iot, ioh ); */ \
} 
#else
#define NSIO_CONFIG_DEBUG( iot, ioh )
#endif

/* 
** NSIO_CONFIG_KBCDEBUG
**
** Print out configuration information for the keyboard device
**
** sc    pointer to nsio devices softc
*/
#ifdef DDB
#define NSIO_CONFIG_KBCDEBUG( iot, ioh ) \
{ \
    u_char cmdL; \
    u_char cmdH; \
    /* nsioConfigPrint( iot, ioh ); */ \
    NSIO_READ_REG( iot, ioh, NSIO_KBC_CMDH, cmdH ); \
    NSIO_READ_REG( iot, ioh, NSIO_KBC_CMDH, cmdL ); \
    printf("kbc command: %x\n", (((u_short)(cmdH)) << 8)| cmdL ); \
} 
#else
#define NSIO_CONFIG_KBCDEBUG( iot, ioh )
#endif

/* Functions to help configure the ns87307 logical devices.
*/
int  i87307KbdConfig       __P((bus_space_tag_t  iot,
				u_int            comBase, 
				u_int            irqNum ));
int  i87307MouseConfig     __P((bus_space_tag_t iot,
				u_int           irqNum ));


void nsioConfigPrint(bus_space_tag_t nsioIot, bus_space_handle_t  nsioIoh );                        

#endif /* _NS87307REG_H */
