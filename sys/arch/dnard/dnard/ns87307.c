/*	$NetBSD: ns87307.c,v 1.1 2001/05/09 15:58:07 matt Exp $	*/

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
**    ns87307 SuperIO chip configuration functions.
**
**  ABSTRACT:
**
**    This file contains routines to configure the National Semiconductor
**    PC87307VUL SuperIO chip.
**
**  AUTHORS:
**
**    John Court, Digital Equipment Corporation.
**
**  CREATION DATE:
**
**    16/4/1997
**
**--
*/

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/isa/isavar.h>
#include <dnard/dnard/ns87307reg.h>









/*
**++
**  FUNCTION NAME:
**
**      i87307KbdConfig
**
**  FUNCTIONAL DESCRIPTION:
**
**     This function configures the Keyboard controller logical
**     device on the ns87307 SuperIO hardware.  It sets up the addresses 
**     of the data and command ports and configures the irq number and 
**     triggering for the device.  It then activates the device.     
**
**  FORMAL PARAMETERS:
**
**       iot
**       kbdBase   
**       irqNum    
**
**  IMPLICIT INPUTS:
**
**      None.
**
**  IMPLICIT OUTPUTS:
**
**      None.
**
**  function value or completion codes
**
**      TRUE    configuration of the kdb device was successful.
**      FALSE   configuration of the kdb device failed.
**
**  SIDE EFFECTS:
**
**      None.
**--
*/
int
i87307KbdConfig(bus_space_tag_t iot,
		u_int           kbdBase, 
		u_int           irqNum )
{
    u_int                configured = FALSE;
    bus_space_handle_t   ioh;	     


    if (!(bus_space_map( iot, CONNSIOADDR, NSIO_NPORTS, 0 , &ioh )))
    {
	NSIO_SELECT_DEV( iot, ioh,  NSIO_DEV_KBC );
	NSIO_DEACTIVATE_DEV( iot, ioh );
	NSIO_CONFIG_IRQ( iot, ioh, irqNum, NSIO_IRQ_LEVEL | NSIO_IRQ_HIGH );
	NSIO_CONFIG_KBCDATA( iot, ioh, kbdBase );
	NSIO_CONFIG_KBCCMD(iot, ioh, kbdBase+4);
	NSIO_WRITE_REG( iot, ioh, NSIO_KBC_CFG, 0x80 );
	NSIO_ACTIVATE_DEV( iot, ioh ); 
	NSIO_CONFIG_KBCDEBUG( iot, ioh );

	/* unmap the space so can probe later
	*/
	bus_space_unmap( iot, ioh, NSIO_NPORTS );

	configured = TRUE;
    }

    return (configured);
} /* End i87307KbdConfig() */



/*
**++
**  FUNCTION NAME:
**
**      i87307MouseConfig
**
**  FUNCTIONAL DESCRIPTION:
**
**     This function configures the Mouse logical device on the ns87307
**     SuperIO chip. It sets up only the interrupt parameters since the 
**     mouse shares the device ports with the Keyboard. It also activates
**     the device. 
**
**  FORMAL PARAMETERS:
**
**       iot
**       irqNum    
**
**  IMPLICIT INPUTS:
**
**      None.
**
**  IMPLICIT OUTPUTS:
**
**      None.
**
**  function value or completion codes
**
**      TRUE    configuration of the kdb device was successful.
**      FALSE   configuration of the kdb device failed.
**
**  SIDE EFFECTS:
**
**      None.
**--
*/
int
i87307MouseConfig(bus_space_tag_t iot,
		  u_int           irqNum )
{
    u_int                configured;
    bus_space_handle_t   ioh;	     

    configured = FALSE; /* be a pessimist */

    if (!(bus_space_map( iot, CONNSIOADDR, NSIO_NPORTS, 0 , &ioh )))
    {
	/* Now do the mouse logical device IRQ settup.
	*/
	NSIO_SELECT_DEV( iot, ioh, NSIO_DEV_MOUSE );
	NSIO_DEACTIVATE_DEV( iot, ioh );   
	NSIO_CONFIG_IRQ( iot, ioh, irqNum, NSIO_IRQ_LEVEL | NSIO_IRQ_HIGH);
	NSIO_ACTIVATE_DEV( iot, ioh );
	NSIO_CONFIG_DEBUG( iot, ioh );
	/* unmap the space so can probe later 
	*/
	bus_space_unmap( iot, ioh, NSIO_NPORTS );

	configured = TRUE;
    }


    return (configured);
} /* End i87307MouseConfig() */



/*
**++
**  FUNCTION NAME:
**
**      i87307PrinterConfig
**
**  FUNCTIONAL DESCRIPTION:
**
**     This function configures the Parrel logical device on the ns87307
**     SuperIO chip.
**     the device. 
**
**  FORMAL PARAMETERS:
**
**       iot
**       irqNum    
**
**  IMPLICIT INPUTS:
**
**      None.
**
**  IMPLICIT OUTPUTS:
**
**      None.
**
**  function value or completion codes
**
**      TRUE    configuration of the kdb device was successful.
**      FALSE   configuration of the kdb device failed.
**
**  SIDE EFFECTS:
**
**      None.
**--
*/
int
i87307PrinterConfig(bus_space_tag_t iot,
		  u_int           irqNum )
{
    u_int                configured = FALSE;
    bus_space_handle_t   ioh;	     

    u_char value;
 
    if (!(bus_space_map( iot, CONNSIOADDR, NSIO_NPORTS, 0 , &ioh )))
    {
        /* select the printer */
        NSIO_SELECT_DEV( iot, ioh,  NSIO_DEV_LPT );
    	NSIO_DEACTIVATE_DEV( iot, ioh );
        
        value = NSIO_LPT_TRISTATE_DISABLE | 
                NSIO_LPT_CLOCK_DISABLE    |
                NSIO_LPT_REPORT_SPP       |
                NSIO_LPT_REG403_DISABLE   |
                NSIO_LPT_SPP_EXTENDED;
    	NSIO_WRITE_REG( iot, ioh, NSIO_CFG_REG0, value);
    	
        /* set the type of interupt */
        value = NSIO_IRQ_HIGH | 
                NSIO_IRQ_LEVEL;
        NSIO_CONFIG_IRQ( iot, ioh, irqNum,value);
    
    	/* activate the device */
    	NSIO_ACTIVATE_DEV( iot, ioh ); 
        
        
        /* unmap the space so can probe later */
    	bus_space_unmap( iot, ioh, NSIO_NPORTS );
    
    	configured = TRUE;
    }

    
    return (configured);
} /* End i87307PrinterConfig() */



/*
**++
**  FUNCTION NAME:
**
**      nsioConfigPrint
**
**  FUNCTIONAL DESCRIPTION:
**
**     This function prints out the irq, iobase etc, for the 
**     currently selected logical device.  It is intended to
**     be used for debugging only.     
**
**  FORMAL PARAMETERS:
**
**       sc     pointer to the nsio's softc structure
**
**  IMPLICIT INPUTS:
**
**      None.
**
**  IMPLICIT OUTPUTS:
**
**      None.
**
**  function value or completion codes
**
**      None
**
**  SIDE EFFECTS:
**
**      None.
**
**--
*/     
//#ifdef DDB
void nsioConfigPrint(bus_space_tag_t nsioIot, 
                     bus_space_handle_t  nsioIoh )                        
{                                                                       
    u_char     dev;                                                     
    u_char     activate;                                                
    u_char     iorange;                                                 
    u_char     iobaseh;                                                 
    u_char     iobasel;                                                 
    u_char     irq;                                                     
    u_char     irqType;                                                 
    u_char     dma1;                                                    
    u_char     dma2;                                                    
    u_char     reg0;                                                    
                                                                        
    NSIO_READ_REG( nsioIot, nsioIoh, NSIO_CFG_LOGDEV, dev );            
    NSIO_READ_REG( nsioIot, nsioIoh, NSIO_CFG_ACTIVATE, activate );     
    NSIO_READ_REG( nsioIot, nsioIoh, NSIO_CFG_IORNGCHK, iorange );      
    NSIO_READ_REG( nsioIot, nsioIoh, NSIO_CFG_IOBASEH, iobaseh );       
    NSIO_READ_REG( nsioIot, nsioIoh, NSIO_CFG_IOBASEL, iobasel );       
    NSIO_READ_REG( nsioIot, nsioIoh, NSIO_CFG_IRQ, irq );               
    NSIO_READ_REG( nsioIot, nsioIoh, NSIO_CFG_IRQTYPE, irqType );       
    NSIO_READ_REG( nsioIot, nsioIoh, NSIO_CFG_DMA1, dma1 );             
    NSIO_READ_REG( nsioIot, nsioIoh, NSIO_CFG_DMA2, dma2 );             
    NSIO_READ_REG( nsioIot, nsioIoh, NSIO_CFG_REG0, reg0 );             
                                                                        
    printf("nsio config for logical device %d\n", dev );                
    printf("activate:   %x\n",activate);                                
    printf("iorange:    %x\n",iorange);                                 
    printf("iobase:     %x\n",(iobaseh << 8) | iobasel);                
    printf("irq:        %x\n",irq);                                           
    printf("irqtype:    %x\n",irqType);                                 
    printf("dma1:       %x\n",dma1);                                    
    printf("dma2:       %x\n",dma2) ;                                   
    printf("reg0:       %x\n",reg0);                                    
}  
//#endif
