/*	$NetBSD: if_cs_isa.c,v 1.3 1998/06/20 20:38:33 mark Exp $	*/

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
**  FACILITY
**
**     Device Driver for the Crystal CS8900 ISA Ethernet Controller.
**
**  ABSTRACT
**
**     This module provides standard ethernet access for INET protocols
**     only. 
**
**  AUTHORS
**
**     Peter Dettori     SEA - Software Engineering.
**
**  CREATION DATE:
**
**     13-Feb-1997.
**
**  MODIFICATION HISTORY (Digital):
**
**     Revision 1.27  1998/01/20  17:59:40  cgd
**     update for moved headers
**
**     Revision 1.26  1998/01/12  19:29:36  cgd
**     use arm32/isa versions of isadma code.
**
**     Revision 1.25  1997/12/12  01:35:27  cgd
**     convert to use new arp code (from Brini)
**
**     Revision 1.24  1997/12/10  22:31:56  cgd
**     trim some fat (get rid of ability to explicitly supply enet addr, since
**     it was never used and added a bunch of code which really doesn't belong in
**     an enet driver), and clean up slightly.
**
**     Revision 1.23  1997/10/06  16:42:12  cgd
**     copyright notices
**
**     Revision 1.22  1997/06/20  19:38:01  chaiken
**     fixes some smartcard problems
**
**     Revision 1.21  1997/06/10 02:56:20  grohn
**     Added call to ledNetActive
**
**     Revision 1.20  1997/06/05 00:47:06  dettori
**     Changed csProcessRxDMA to reset and re-initialise the
**     ethernet chip when DMA gets out of sync, or mbufs
**     can't be allocated.
**
**     Revision 1.19  1997/06/03 03:09:58  dettori
**     Turn off txInProgress flag when a transmit underrun
**     occurs.
**
**     Revision 1.18  1997/06/02 00:04:35  dettori
**     redefined the transmit table to get around the nfs_timer bug while we are
**     looking into it further.
**
**     Also changed interrupts from EDGE to LEVEL.
**
**     Revision 1.17  1997/05/27 23:31:01  dettori
**     Pulled out changes to DMAMODE defines.
**
**     Revision 1.16  1997/05/23 04:25:16  cgd
**     reformat log so it fits in 80cols
**
**     Revision 1.15  1997/05/23  04:22:18  cgd
**     remove the existing copyright notice (which Peter Dettori indicated
**     was incorrect, copied from an existing NetBSD file only so that the
**     file would have a copyright notice on it, and which he'd intended to
**     replace).  Replace it with a Digital copyright notice, cloned from
**     ess.c.  It's not really correct either (it indicates that the source
**     is Digital confidential!), but is better than nothing and more
**     correct than what was there before.
**
**     Revision 1.14  1997/05/23  04:12:50  cgd
**     use an adaptive transmit start algorithm: start by telling the chip
**     to start transmitting after 381 bytes have been fed to it.  if that
**     gets transmit underruns, ramp down to 1021 bytes then "whole
**     packet."  If successful at a given level for a while, try the next
**     more agressive level.  This code doesn't ever try to start
**     transmitting after 5 bytes have been sent to the NIC, because
**     that underruns rather regularly.  The back-off and ramp-up mechanism
**     could probably be tuned a little bit, but this works well enough to
**     support > 1MB/s transmit rates on a clear ethernet (which is about
**     20-25% better than the driver had previously been getting).
**
**     Revision 1.13  1997/05/22  21:06:54  cgd
**     redo csCopyTxFrame() from scratch.  It had a fatal flaw: it was blindly
**     casting from u_int8_t * to u_int16_t * without worrying about alignment
**     issues.  This would cause bogus data to be spit out for mbufs with
**     misaligned data.  For instance, it caused the following bits to appear
**     on the wire:
**     	... etBND 1S2C .SHA(K) R ...
**     	    11112222333344445555
**     which should have appeared as:
**     	... NetBSD 1.2C (SHARK) ...
**     	    11112222333344445555
**     Note the apparent 'rotate' of the bytes in the word, which was due to
**     incorrect unaligned accesses.  This data corruption was the cause of
**     incoming telnet/rlogin hangs.
**
**     Revision 1.12  1997/05/22  01:55:32  cgd
**     reformat log so it fits in 80cols
**
**     Revision 1.11  1997/05/22  01:50:27  cgd
**     * enable input packet address checking in the BPF+IFF_PROMISCUOUS case,
**       so packets aimed at other hosts don't get sent to ether_input().
**     * Add a static const char *rcsid initialized with an RCS Id tag, so that
**       you can easily tell (`strings`) what version of the driver is in your
**       kernel binary.
**     * get rid of ether_cmp().  It was inconsistently used, not necessarily
**       safe, and not really a performance win anyway.  (It was only used when
**       setting up the multicast logical address filter, which is an
**       infrequent event.  It could have been used in the IFF_PROMISCUOUS
**       address check above, but the benefit of it vs. bcmp or memcmp would be
**       inconsequential, there.)  Use bcmp() instead.  Eventually, this should
**       use memcmp(), so that the compiler can optimize it into inline code.
**     * restructure csStartOuput to avoid the following bugs in the case where
**       txWait was being set:
**         * it would accidentally drop the outgoing packet if told to wait
**           but the outgoing packet queue was empty.
**         * it would bpf_mtap() the outgoing packet multiple times (once for
**           each time it was told to wait), and would also recalculate
**           the length of the outgoing packet each time it was told to
**           wait.
**       While there, rename txWait to txLoop, since with the new structure of
**       the code, the latter name makes more sense.
**
**     Revision 1.10  1997/05/19  02:03:20  cgd
**     Set RX_CTL in csSetLadrFilt(), rather than csInitChip().  csInitChip()
**     is the only caller of csSetLadrFilt(), and always calls it, so this
**     ends up being logically the same.  In csSetLadrFilt(), if IFF_PROMISC
**     is set, enable promiscuous mode (and set IFF_ALLMULTI), otherwise behave
**     as before.
**
**     Revision 1.9  1997/05/19  01:45:37  cgd
**     create a new function, csEtherInput(), which does received-packet
**     BPF and ether_input processing.  This code used to be in three places,
**     and centralizing it will make adding IFF_PROMISC support much easier.
**     Also, in csCopyTxFrame(), put it some (currently disabled) code to
**     do copies with bus_space_write_region_2().  It's more correct, and
**     potentially more efficient.  That function needs to be gutted (to
**     deal properly with alignment issues, which it currently does wrong),
**     however, and the change doesn't gain much, so there's no point in
**     enabling it now.
**
**     Revision 1.8  1997/05/19  01:17:10  cgd
**     fix a comment re: the setting of the TxConfig register.  Clean up
**     interface counter maintenance (make it use standard idiom).
**
**--
*/


/*
 * INCLUDE DEFINITIONS
 *
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#include <net/if.h>
#include <net/if_ether.h>
#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <vm/vm.h>

#include <machine/kerndebug.h>
#include <machine/cpu.h>
#include <machine/intr.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <arm32/isa/isadmavar.h>

#include "bpfilter.h"
#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include "if_csvar.h"
#ifdef SHARK
#include <arm32/shark/sequoia.h>
#endif

#ifdef OFW
#include <dev/ofw/openfirm.h>
#endif

/*
 * MACRO DEFINITIONS
 *
 */
#define CS_OUTPUT_LOOP_MAX 100 /* max times round notorious tx loop */
#define DMA_BUFFER_SIZE 16384  /* 16K or 64K */
#define DMA_STATUS_BITS 0x0007 /* bit masks for checking DMA status */
#define DMA_STATUS_OK 0x0004 
#define CS_MEM_SIZE 4096 /* 4096 bytes of on chip memory */
#define ETHER_MTU 1518 /* ETHERMTU is defiend in if_ether.h as 1500
			* ie. without the header.
			*/

/* 
 * the kerndebug macros reserve use of the 
 * first and last byte, so we can only use the 
 * middle two bytes for flags.
 */

#define CSPROBE_DBG_INFO    0x00000100
#define CSATTACH_DBG_INFO   0x00000200
#define CSRESET_DBG_INFO    0x00000400
#define CSINIT_DBG_INFO     0x00000800
#define CSIOCTL_DBG_INFO    0x00001000
#define CSSTARTTX_DBG_INFO  0x00002000
#define CSTXEVENT_DBG_INFO  0x00004000
#define CSRXNORM_DBG_INFO   0x00008000
#define CSRXEARLY_DBG_INFO  0x00010000
#define CSRXDMA_DBG_INFO    0x00020000
#define CSMCAST_DBG_INFO    0x00040000
#define CSBUFF_DBG_INFO     0x00080000
#define CSUTIL_DBG_INFO     0x00800000

/* 
 * FUNCTION PROTOTYPES
 */
#ifdef  __BROKEN_INDIRECT_CONFIG
int csProbe __P((struct device *, void *, void *));
#else
int csProbe __P((struct device *, struct cfdata *, void *));
#endif
void csAttach __P((struct device *, struct device *, void *));
ushort csReadPacketPage __P(( struct cs_softc *sc, ushort offset ));
void csWritePacketPage __P(( struct cs_softc *sc, ushort offset, ushort value ));
int csGetUnspecifiedParms __P(( struct cs_softc *sc ));
int csValidateParms __P(( struct cs_softc *sc ));
int csGetEthernetAddr __P(( struct cs_softc *sc ));
int csReadEEPROM __P(( struct cs_softc *sc, ushort offset, ushort *pValue ));
int csResetChip __P(( struct cs_softc *sc ));
void csShow __P(( struct cs_softc *sc, int Zap ));
int csInit __P(( struct cs_softc *sc ));
void csReset __P(( void * ));
int csIoctl __P(( struct ifnet *, ulong com, caddr_t pData ));
void csInitChip __P(( struct cs_softc *sc ));
int csIntr __P(( void *arg ));
void csBufferEvent __P(( struct cs_softc *sc, ushort BuffEvent ));
void csTransmitEvent __P(( struct cs_softc *sc, ushort txEvent ));
void csReceiveEvent __P(( struct cs_softc *sc, ushort rxEvent ));
void csEtherInput __P(( struct cs_softc *sc, struct mbuf *m ));
void csProcessReceive __P(( struct cs_softc *sc ));
void csProcessRxEarly __P(( struct cs_softc *sc ));
void csProcessRxDMA(struct cs_softc *sc);
void csStartOutput __P((struct ifnet *ifp  ));
void csCopyTxFrame __P(( struct cs_softc *sc, struct mbuf *pMbufChain ));
void csSetLadrFilt( struct cs_softc *sc, struct ethercom *ec );
ushort csHashIndex(char *addr);
void csCounterEvent( struct cs_softc *sc, ushort cntEvent );

/* 
 * GLOBAL DECLARATIONS
 */

/*
 * Xmit-early table.
 *
 * To get better performance, we tell the chip to start packet
 * transmission before the whole packet is copied to the chip.
 * However, this can fail under load.  When it fails, we back off
 * to a safer setting for a little while.
 *
 * txcmd is the value of txcmd used to indicate when to start transmission.
 * better is the next 'better' state in the table.
 * better_count is the number of output packets before transition to the
 *   better state.
 * worse is the next 'worse' state in the table.
 *
 * Transition to the next worse state happens automatically when a
 * transmittion underrun occurs.
 */
struct cs_xmit_early {
	u_int16_t	txcmd;
	int		better;
	int		better_count;
	int		worse;
} cs_xmit_early_table[3] = {
	{	TX_CMD_START_381,	0, INT_MAX,	1,	},
	{	TX_CMD_START_1021,	0, 50000,	2,	},
	{	TX_CMD_START_ALL,	1, 5000,	2,	},
};

/*
 * this is static config information, the configuration file
 * expects this struct to exist and contain the probe and attach
 * functions, and softc size for allocation.
 */
struct cfattach cs_ca = 
{
    sizeof(struct cs_softc), csProbe, csAttach
};

extern struct cfdriver cs_cd;

int csdebug  = 0x00000000; /* debug status, used with kerndebug macros */
#ifndef SHARK
/* SHARKS don't have the kernel in the lower megs of physical
 * and hence this won't work. I don't know any other machines that 
 * this won't work for (I didn't look either) so I ifdef'd it like this.
 */
char cs_dma_buffer[DMA_BUFFER_SIZE] __attribute__ ((aligned (4096)));
#endif


#ifdef  __BROKEN_INDIRECT_CONFIG
/*-------------------------------------------------------------------------*/
/*
**  Function         :  csProbe.
**
**  Description      :  This function will be called at system startup
**                      if the kernel is configured for 
**                      cs8900 ethernet devices. The probe will 
**                      check if a cs8900 ethernet device is plugged
**                      into the isa bus.
**
**  Parameters       :  struct device *parent unused parent device
**                      void   *match is a cs_softc structure
**                      void   *aux is a pointer to isa_attach_args
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  1 if the device is found 0 if not.
**
**  Side Effects     :  None.
*/
int csProbe(struct device *parent, void *match, void *aux)
#else
int csProbe(struct device *parent, struct cfdata *cf, void *aux)
#endif
{
#ifdef  __BROKEN_INDIRECT_CONFIG
    struct cs_softc *sc = match;
#else
    struct cs_softc junksc;
    struct cs_softc *sc = &junksc;
#endif
    struct isa_attach_args *ia = aux;  /* this holds info statically 
                                        * configured in the config file 
                                        */
    ushort isaId;

    sc->sc_iobase = ia->ia_iobase;  /* store the IO base address for later use */
    sc->sc_drq = ia->ia_drq;     /* store the drq line */
    sc->inMemoryMode = FALSE;    /* must work in IO mode initially */

    /* do bus handle stuff 
    */
    sc->sc_iot = ia->ia_iot;
    sc->sc_memt = ia->ia_memt;
    
    /* map the bus space here and put the arguments in the softc
     * so that the csRead/WritePacketPage can operate.
     */
    if ( bus_space_map(sc->sc_iot, ia->ia_iobase, CS8900_IOSIZE, 0, 
                       &sc->sc_ioh) != 0 )
    {
        return 0;    /* bus map didn't work, not much we can do */
    }

    /* Verify that we can read from the chip */
    isaId =  csReadPacketPage( sc, PKTPG_EISA_NUM );

    /* Verify that the chip is a Crystal Semiconductor chip */
    if ( isaId != EISA_NUM_CRYSTAL )
    {
        /* unmap the bus space and clear the softc 
         * structure.
         */
        bus_space_unmap(sc->sc_iot, sc->sc_ioh, CS8900_IOSIZE);
        sc->sc_iot = 0;
        sc->sc_ioh = 0;
        printf("\n%s - Chip is not a Crystal Semiconductor chip\n",
               (sc->sc_dev).dv_xname);
        return 0;
    }

    /* Verify that the chip is a CS8900 */
    if ( (csReadPacketPage( sc, PKTPG_PRODUCT_ID ) & PROD_ID_MASK ) 
        != PROD_ID_CS8900 )
    {
        /* unmap the bus space and clear the softc 
         * structure.
         */
        bus_space_unmap(sc->sc_iot, sc->sc_ioh, CS8900_IOSIZE);
        sc->sc_iot = 0;
        sc->sc_ioh = 0;
        printf("\n%s - Chip is not a CS8900\n", (sc->sc_dev).dv_xname);
        return 0;
    }

#ifdef  __BROKEN_INDIRECT_CONFIG
    /* because of the  BROKEN_INDIRECT...
     * macro must unmap the bus space here.
     */
    bus_space_unmap(sc->sc_iot, sc->sc_ioh, CS8900_IOSIZE);
#endif

    ia->ia_iosize = CS8900_IOSIZE; /* this is not passed to us for some reason,
                                    * so we just hardcode it here 
                                    */
    return 1;        /* chip has been located can now attach it */
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csAttach.
**
**  Description      :  This function will be called at system startup
**                      time if the kernel is configured for 
**                      cs8900 ethernet devices and one has been found
**                      on the isa bus by the probe. The attach will
**                      will register the device with the system structures
**                      so that upper layer protocols can access the network.
**
**  Parameters       :  struct device *parent unused parent device
**                      struct device *self is actually a softc struct
**                      void   *aux is a pointer to isa_attach_args
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  None.
**
**  Side Effects     :  The device is registered as an ether device on
**                      an isa bus.
*/
void csAttach(struct device *parent, struct device *self, void *aux)
{
    struct cs_softc *sc = (struct cs_softc *)self;
    struct isa_attach_args *ia = aux;
    struct ifnet *ifp = &sc->sc_ethercom.ec_if;

#ifndef __BROKEN_INDIRECT_CONFIG
    sc->sc_iobase = ia->ia_iobase;
    sc->sc_drq = ia->ia_drq;
    sc->inMemoryMode = FALSE;
    sc->sc_iot = ia->ia_iot;
    sc->sc_memt = ia->ia_memt;
    if (bus_space_map(sc->sc_iot, ia->ia_iobase,
                      ia->ia_iosize, 0, &sc->sc_ioh) != 0)
    {
        panic("csAttach: bus space map failed");
    }
#endif

    printf(": %s Ethernet\n", sc->sc_dev.dv_xname);

    /* the first thing to do is check that the mbuf cluster size is 
     * greater than the MTU for an ethernet frame. The code depends 
     * on this and to port this to a OS where this was not the case
     * would not be straightforward.
     */
    if (MCLBYTES < ETHER_MTU)
    {
	panic("%s : Attach failed -> machines mbuf cluster size is insufficient\n",
	      sc->sc_dev.dv_xname);
    }
    
    /* save relevant data passed in through the isa_attach_args structure */
    sc->sc_int = ia->ia_irq;

    /* this driver only supports mapped memory and DMA modes of 
     * operation. So if a memory base address is not given we can't 
     * go on.
     */
    if (ia->ia_maddr != MADDRUNK)
    {
        sc->pPacketPagePhys = ia->ia_maddr;
    }
    else
    {
        panic("%s : Attach failed -> configuration info incomplete, no maddr\n",
	      sc->sc_dev.dv_xname);
    }

    /* Start out in IO mode */
    sc->inMemoryMode = FALSE;

    /* Start out not transmitting */
    sc->txInProgress = FALSE;

    /* Set up early transmit threshhold */
    sc->sc_xe_ent = 0;
    sc->sc_xe_togo = cs_xmit_early_table[sc->sc_xe_ent].better_count;

    /* Initialize ifnet structure. */
    bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
    ifp->if_softc = sc;
    ifp->if_start = csStartOutput;
    ifp->if_ioctl = csIoctl;
    ifp->if_watchdog = NULL;    /* no watchdog at this stage */
    ifp->if_flags = IFF_SIMPLEX | IFF_NOTRAILERS | 
                    IFF_BROADCAST | IFF_MULTICAST;  

#ifdef OFW
    /* Dig MAC address out of the firmware. */
    if (OF_getprop((int)ia->ia_aux, "mac-address", sc->sc_enaddr,
        sizeof(sc->sc_enaddr)) < 0) {
	int ofnet_handle;
	ofnet_handle = OF_open("/isa/ethernet");
	if (!ofnet_handle) 
		panic("OF_open: Unable to open /isa/ethernet");
	if (OF_getprop((int)ia->ia_aux, "mac-address",
	    sc->sc_enaddr, 
	    sizeof(sc->sc_enaddr)) < 0)
		panic("csAttach: no mac-address");
	/* closing the device causes the kernel to panic ... */ 
#if 0
	OF_close(ofnet_handle);
#endif
	}

    sc->mediaType = MEDIA_10BASET;
#else
    /* Get parameters, which were not specified, from the EEPROM */
    if ( csGetUnspecifiedParms(sc) == CS_ERROR )
    {
        printf("%s : Couldn't get the unspecified parameters in the attach\n",\
               sc->sc_dev.dv_xname);
        return ;
    }

    /* Verify that parameters are valid */
    if ( csValidateParms(sc) == CS_ERROR )
    {
        printf("%s : Couldn't get the validate parameters in the attach\n",\
               sc->sc_dev.dv_xname );
        return ;
    }

    /* Get and store the Ethernet address */
    if ( csGetEthernetAddr(sc) == CS_ERROR )
    {
        printf("%s : couldn't get the ethernet address in the attach\n",\
               sc->sc_dev.dv_xname );
        return ;
    }
#endif

    printf("%s : address %s\n", sc->sc_dev.dv_xname,
           ether_sprintf(sc->sc_enaddr));
    
    /*
     * First figure out which memory mode to use, by:
     *      1. if we have a valid drq then use DMA only mode
     *         for receives. Chip only supports 5,6 & 7
     *      2. else we have been given a memory address for IO
     *         mapping then use it.
     */

    if ( (sc->sc_drq >= 5) && (sc->sc_drq <= 7) )
    {
	/* The sharks can't use the cs_dma_buffer because
	 * kernel is not placed in the lower megs of physical 
	 * memory. Instead they call a function which returns
	 * an offset into the isaphysmem, which is a block 
	 * of memory allocated at boot-time for use as 
	 * DMA bounce buffers. The SHARKS allocate another 
	 * 64Kbytes for a possible large buffer and allow
	 * access to it through isa_dmabuffer_get() defined
	 * in isadmavar.h
	 */
#ifdef SHARK
	/* If we get the buffer we never relinquish it,
	 * primarily because I expect the ethernet 
	 * would be required always.
	 */
	sc->dmaBase = (char *) isa_dmabuffer_get();
#else
	sc->dmaBase = cs_dma_buffer; 
#endif

	/* if we couldn't get a dma buffer then we can't 
	 * go on with DMA.
	 */
	if ( sc->dmaBase != NULL )
	{
	    KERN_DEBUG(csdebug, CSATTACH_DBG_INFO, 
		       ("csAttach : alloced memory address = 0x%x\n"\
			,(int) sc->dmaBase));

	    sc->dmaMemSize = CS8900_DMA_BUFFER_SIZE;
	    
	    /* now set the config flag and let the csResetChip and csInitChip
	     * routines do the rest.
	     */
	    KERN_DEBUG(csdebug, CSATTACH_DBG_INFO,
		       ("csAttach : setting DMA mode\n"));
	    
	    sc->configFlags |= CFGFLG_DMA_MODE;
	}
	/* now go on and try mapped memory, or programmed IO. */
    } /* if (drq) */

    /* the default EEPROM configuration is to use programmed IO. If 
     * we want to use mapped memory we must manually set the 
     * chips registers.
     * 
     * check here to see if we are configured to use mapped memory.
     */
    if ( sc->pPacketPagePhys != MADDRUNK )
    { 
        /* check that the configured IO memory size is 4K
         * otherwise hardcode the driver to use programmed IO.
         */
        if ( ia->ia_msize != CS_MEM_SIZE )
        {
            panic(": %s IO mem size is not CS_MEM_SIZE invalid configuration\n"
		   , (sc->sc_dev).dv_xname);
        } 
        else
        {
	    /* map the memory addresses */
	    if (bus_space_map(sc->sc_memt, sc->pPacketPagePhys,
			      CS_MEM_SIZE, 0, &sc->sc_memh) != 0)
	    {
		panic("%s : csAttach : bus space map failed\n", 
		       (sc->sc_dev).dv_xname);
	    }
	    else
	    {
		/* I think we always want to use the external latch logic */
		sc->configFlags = sc->configFlags | CFGFLG_MEM_MODE | 
		    CFGFLG_USE_SA | CFGFLG_IOCHRDY;
	    }
        }
    }

    /* Attach the interface. */
    if_attach(ifp);
    /* add to generic ethernet code */
    ether_ifattach(ifp, sc->sc_enaddr);

    /* attach the Berkely Packet Filter interface */
#if NBPFILTER > 0
    bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

    sc->sc_sh = shutdownhook_establish(csReset, sc);
    if (sc->sc_sh == NULL)
    {
        panic("csAttach: can't establish shutdownhook");
    }
    /* Setup the interrupt handler */
    sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_LEVEL,
                                   IPL_NET, csIntr, sc);

    /* Reset the chip */
    if ( csResetChip(sc) == CS_ERROR )
    {
        panic("\n%s - Can not reset the ethernet chip\n", 
	      (sc->sc_dev).dv_xname );
    }

    return ;
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csReadPacketPage
**
**  Description      :  This routine reads a word from the PacketPage at 
**                      the specified offset.
**                        
**                      This routine should be maually in-lined as required 
**                      for fast loops reading from the chip. ie. the
**                      loop should be inside the if (memory-mode) statement.
**
**  Parameters       :  struct cs_softc *sc
**                      ushort offset is the offset in the PacketPage to
**                      read 
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  The 16 bit PacketPage register.
**            
**  Side Effects     :  None.
**
*/
ushort csReadPacketPage( struct cs_softc *sc, ushort offset )
{
    if ( sc->inMemoryMode )
    {
        return bus_space_read_2( sc->sc_memt, sc->sc_memh, offset );
    }
    else  /* In IO mode */
    {
        bus_space_write_2( sc->sc_iot, sc->sc_ioh, PORT_PKTPG_PTR, offset );
        return bus_space_read_2( sc->sc_iot, sc->sc_ioh, PORT_PKTPG_DATA );
    }
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csWritePacketPage
**
**  Description      :  This routine writes a value to the PacketPage at 
**                      the specified offset.
**
**                      This routine should be maually in-lined as required 
**                      for fast loops writing to the chip. 
**
**  Parameters       :  struct cs_softc *sc
**                      offset,    PacketPage register offset
**                      value, value to write to the register
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  void.
**
**  Side Effects     :  None.
**
*/
void csWritePacketPage( struct cs_softc *sc, ushort offset, ushort value )
{
    if ( sc->inMemoryMode )
    {
        bus_space_write_2( sc->sc_memt, sc->sc_memh, offset, value );
    }
    else  /* In IO mode */
    {
        bus_space_write_2( sc->sc_iot, sc->sc_ioh, PORT_PKTPG_PTR, offset );
        bus_space_write_2( sc->sc_iot, sc->sc_ioh, PORT_PKTPG_DATA, value );
    }
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :    csGetUnspecifiedParms
**
**  Description      :    This routine gets parameters, that were not 
**                        specifed to csAttach(), from the EEPROM and puts 
**                        them in the cs_softc structure.  If all the
**                        parameters were specified in csAttach(), then this
**                        routine does not attempt to read the EEPROM.
**
**  Parameters       :    struct cs_softc *sc
**
**  Implicit Inputs  :    None.
**
**  Implicit Outputs :    None.
**
**  Return Value     :    CS_ERROR if it can't get the info from the EEPROM,
**                        else CS_OK.
**
**  Side Effects     :    None.
**
*/
int csGetUnspecifiedParms( struct cs_softc *sc )
{
    ushort selfStatus;
    ushort isaConfig;
    ushort memBase;
    ushort adapterConfig;
    ushort xmitCtl;

    /* If all of these parameters were specified */
    if ( sc->configFlags != 0 && sc->pPacketPagePhys != MADDRUNK 
        && sc->sc_int != 0  && sc->mediaType != 0 )
    {
        KERN_DEBUG(csdebug, CSUTIL_DBG_INFO, 
                   ("csGetUnspecifiedParms : have got everything already\n"));
        return CS_OK;  /* Don't need to get anything from the EEPROM */
    }

    /* Verify that the EEPROM is present and OK */
    selfStatus = csReadPacketPage( sc, PKTPG_SELF_ST );
    if ( !((selfStatus & SELF_ST_EEP_PRES) && (selfStatus & SELF_ST_EEP_OK)) )
    {
        printf("\n%s - EEPROM is missing or bad\n", (sc->sc_dev).dv_xname);
        return CS_ERROR;
    }

    /* Get ISA configuration from the EEPROM */
    if ( csReadEEPROM(sc, EEPROM_ISA_CFG, &isaConfig) == CS_ERROR )
        return CS_ERROR;

    /* Get adapter configuration from the EEPROM */
    if ( csReadEEPROM(sc, EEPROM_ADPTR_CFG, &adapterConfig) == CS_ERROR )
        return CS_ERROR;

    /* Get transmission control from the EEPROM */
    if ( csReadEEPROM(sc, EEPROM_XMIT_CTL, &xmitCtl) == CS_ERROR )
        return CS_ERROR;
    
    /* If the configuration flags were not specified */
    if ( sc->configFlags == 0 )
    {
        /* Copy the memory mode flag */
        if ( isaConfig & ISA_CFG_MEM_MODE )
            sc->configFlags |= CFGFLG_MEM_MODE;

        /* Copy the USE_SA flag */
        if ( isaConfig & ISA_CFG_USE_SA )
            sc->configFlags |= CFGFLG_USE_SA;

        /* Copy the IO Channel Ready flag */
        if ( isaConfig & ISA_CFG_IOCHRDY )
            sc->configFlags |= CFGFLG_IOCHRDY;

        /* Copy the DC/DC Polarity flag */
        if ( adapterConfig & ADPTR_CFG_DCDC_POL )
            sc->configFlags |= CFGFLG_DCDC_POL;

        /* Copy the Full Duplex flag */
        if ( xmitCtl & XMIT_CTL_FDX )
            sc->configFlags |= CFGFLG_FDX;
    }

    /* If the PacketPage pointer was not specified */
    if ( sc->pPacketPagePhys == MADDRUNK )
    {
        /* If memory mode is enabled */
        if ( sc->configFlags & CFGFLG_MEM_MODE )
        {
            /* Get the memory base address from EEPROM */
            if ( csReadEEPROM(sc,EEPROM_MEM_BASE,&memBase) == CS_ERROR )
                return CS_ERROR;

            memBase &= MEM_BASE_MASK;  /* Clear unused bits */
            
            /* Setup the PacketPage pointer */
            sc->pPacketPagePhys = ( ( (ulong) memBase) << 8 );
        }
    }

    /* If the interrupt level was not specified */
    if ( sc->sc_int == 0 )
    {
        /* Get the interrupt level from the ISA config */
        sc->sc_int = isaConfig & ISA_CFG_IRQ_MASK;
        if ( sc->sc_int == 3 )
            sc->sc_int = 5;
        else
            sc->sc_int += 10;
    }

    /* If the media type was not specified */
    if ( sc->mediaType == 0 )
    {
        switch( adapterConfig & ADPTR_CFG_MEDIA )
        {
            case ADPTR_CFG_AUI:
               sc->mediaType = MEDIA_AUI;
            break;
            case ADPTR_CFG_10BASE2:
               sc->mediaType = MEDIA_10BASE2;
            break;
            case ADPTR_CFG_10BASET:
            default:
               sc->mediaType = MEDIA_10BASET;
            break;
       }
    }

    return CS_OK;
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csValidateParms
**
**  Description      :  This routine verifies that the memory address, 
**                      interrupt level and media type are valid.  If any 
**                      of these parameters are invalid, then an error
**                      message is printed and CS_ERROR is returned.
**
**  Parameters       :  struct cs_softc *sc 
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  returns CS_ERROR if any of the parameters are invalid
**
**  Side Effects     :  None.
**
*/

int csValidateParms( struct cs_softc *sc )
{
    int memAddr; /* this is an int so that we can 
		  * do integer manipulation, not
		  * allowed on pointers.
		  */

    memAddr = sc->pPacketPagePhys;

    if ( (memAddr & 0x000FFF) != 0 )
    {
        printf("\n%s - Memory address (0x%X) must start on a 4K boundary\n", 
               (sc->sc_dev).dv_xname, memAddr );
        return CS_ERROR;
    }

    /* check if beyond 16M ISA space */
    if ( memAddr > 0xFFF000 )
    {
        printf("\n%s - Memory address (0x%X) is too large\n", 
               (sc->sc_dev).dv_xname, memAddr );
        return CS_ERROR;
    }

    if ( !(sc->sc_int==5 || sc->sc_int==10 || sc->sc_int==11 ||
           sc->sc_int==12) )
    {
        printf("\n%s - Interrupt level (%d) is invalid\n",
               (sc->sc_dev).dv_xname, sc->sc_int );
        return CS_ERROR;
    }

    if ( !(sc->mediaType==MEDIA_AUI || sc->mediaType==MEDIA_10BASE2 ||
           sc->mediaType==MEDIA_10BASET) )
    {
        printf("\n%s - Media type (%d) is invalid\n", 
               (sc->sc_dev).dv_xname, sc->mediaType );
        return CS_ERROR;
    }

    return CS_OK;
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csGetEthernetAddr
**
**  Description      :  Read the Ethernet address from the EEPROM 
**                      and save it in the arpcom structure.
**
**  Parameters       :  struct cs_softc *sc
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  returns CS_ERROR if EEPROM can't be read.
**
**  Side Effects     :  None.
**
*/
int csGetEthernetAddr( struct cs_softc *sc)
{
    ushort selfStatus;
    pia pIA;

    /* Setup pointer of where to store the Ethernet address */
    pIA = (pia) sc->sc_enaddr;  

    /* Verify that the EEPROM is present and OK */
    selfStatus = csReadPacketPage( sc, PKTPG_SELF_ST );
    if ( !((selfStatus & SELF_ST_EEP_PRES) && (selfStatus & SELF_ST_EEP_OK)))
    {
	printf("\n%s - EEPROM is missing or bad\n", (sc->sc_dev).dv_xname);
	return CS_ERROR;
    }

    /* Get Ethernet address from the EEPROM */
    /* XXX this will likely lose on a big-endian machine. -- cgd */
    if ( csReadEEPROM(sc,EEPROM_IND_ADDR_H,&pIA->word[0]) == CS_ERROR )
	return CS_ERROR;
    if ( csReadEEPROM(sc,EEPROM_IND_ADDR_M,&pIA->word[1]) == CS_ERROR )
	return CS_ERROR;
    if ( csReadEEPROM(sc,EEPROM_IND_ADDR_L,&pIA->word[2]) == CS_ERROR )
	return CS_ERROR;

    return CS_OK;
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csResetChip
**
**  Description      :  This routine resets the CS8900 chip. It
**                      will wait until the chip is ready for use again.
**
**  Parameters       :  struct cs_softc sc
**
**  Implicit Inputs  :  None. 
**
**  Implicit Outputs :  None.
**
**  Return Value     :  Returns error if can't read from the chip.
**
**  Side Effects     :  Memory mode will always be programmed IO after
**                        a reset.
**
*/
int csResetChip( struct cs_softc *sc )
{
    int intState;
    int x;

    /* Disable interrupts at the CPU so reset command is atomic */
    intState = splimp();   

    /* We are now resetting the chip 
     *
     * A spurious interrupt is generated by the 
     * chip when it is reset. This variable 
     * informs the interrupt handler to ignore 
     * this interrupt. 
     */
    sc->resetting = TRUE;

    /* Issue a reset command to the chip */
    csWritePacketPage( sc, PKTPG_SELF_CTL, SELF_CTL_RESET );

    /* Re-enable interrupts at the CPU */
    splx( intState );

    /* The chip is always in IO mode after a reset */
    sc->inMemoryMode = FALSE;

    /* If transmission was in progress, it is not now */
    sc->txInProgress = FALSE;

    /* 
     * there was a delay(125); here, but it seems uneccesary
     * 125 usec is 1/8000 of a second, not 1/8 of a second.
     * the data sheet advises 1/10 of a second here, but the
     * SI_BUSY and INIT_DONE loops below should be sufficient.
     */

    /* Transition SBHE to switch chip from 8-bit to 16-bit */
    bus_space_read_1( sc->sc_iot, sc->sc_iobase, PORT_PKTPG_PTR );
    bus_space_read_1( sc->sc_iot, sc->sc_iobase, PORT_PKTPG_PTR + 1 );
    bus_space_read_1( sc->sc_iot, sc->sc_iobase, PORT_PKTPG_PTR   );
    bus_space_read_1( sc->sc_iot, sc->sc_iobase, PORT_PKTPG_PTR + 1 );

    /* Wait until the EEPROM is not busy */
    for ( x = 0; x < MAXLOOP; x++ )
    {
        if ( !(csReadPacketPage(sc, PKTPG_SELF_ST ) & SELF_ST_SI_BUSY) )
            break;
    }
    
    if ( x == MAXLOOP ) 
        return CS_ERROR;

    /* Wait until initialization is done */
    for ( x = 0; x < MAXLOOP; x++ )
    {
        if ( csReadPacketPage(sc, PKTPG_SELF_ST ) & SELF_ST_INIT_DONE )
            break;
    }
    
    if ( x == MAXLOOP ) 
        return CS_ERROR;

    /* Reset is no longer in progress */
    sc->resetting = FALSE;

    return CS_OK;
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :    csReadEEPROM
**
**  Description      :    This routine reads a word from the EEPROM at the 
**                        specified offset.
**
**  Parameters       :    struct cs_softc *sc, 
**                        ushort offset, into the EEPROM
**                        ushort *pValue, EEPROM data is placed here
**
**  Implicit Inputs  :    None.
**
**  Implicit Outputs :    None.
**
**  Return Value     :    CS_OK, else CS_ERROR if can't read from the EEPROM.
**
**  Side Effects     :    None.
**
*/
int csReadEEPROM( struct cs_softc *sc, ushort offset, ushort *pValue )
{
    int x;

    /* Ensure that the EEPROM is not busy */
    for ( x=0; x < MAXLOOP; x++ )
    {
        if ( !(csReadPacketPage(sc, PKTPG_SELF_ST) & SELF_ST_SI_BUSY) )
            break;
    }
    
    if ( x == MAXLOOP )
    {
        printf("\n%s - Can not read from EEPROM\n", (sc->sc_dev).dv_xname);
        return CS_ERROR;
    }

    /* Issue the command to read the offset within the EEPROM */
    csWritePacketPage( sc, PKTPG_EEPROM_CMD, offset | EEPROM_CMD_READ );

    /* Wait until the command is completed */
    for ( x=0; x < MAXLOOP; x++ )
    {
        if ( !(csReadPacketPage(sc, PKTPG_SELF_ST) & SELF_ST_SI_BUSY) )
            break;
    }
    
    if ( x == MAXLOOP )
    {
        printf("\n%s - Can not read from EEPROM\n", (sc->sc_dev).dv_xname);
        return CS_ERROR;
    }

    /* Get the EEPROM data from the EEPROM Data register */
    *pValue = csReadPacketPage( sc, PKTPG_EEPROM_DATA );

    return CS_OK;
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csInitChip
**
**  Description      :  This routine uses the instance global variables 
**                      in the cs_softc structure to initialize the CS8900 
**                      chip. 
**
**  Parameters       :  struct cs_softc *sc
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  None.
**            
**  Side Effects     :  None.
**
*/
void csInitChip( struct cs_softc *sc )
{
    ushort busCtl;
    ushort selfCtl;
    pia    pIA;
    ushort isaId;

    /* Disable reception and transmission of frames */
    csWritePacketPage( sc, PKTPG_LINE_CTL,
                      csReadPacketPage(sc,PKTPG_LINE_CTL) & 
                      ~LINE_CTL_RX_ON & ~LINE_CTL_TX_ON );

    /* Disable interrupt at the chip */
    csWritePacketPage( sc, PKTPG_BUS_CTL,
                      csReadPacketPage(sc,PKTPG_BUS_CTL) & ~BUS_CTL_INT_ENBL );

    /* If IOCHRDY is enabled then clear the bit in the busCtl register */
    busCtl = csReadPacketPage( sc, PKTPG_BUS_CTL );
    if ( sc->configFlags & CFGFLG_IOCHRDY )
    {   
        csWritePacketPage( sc, PKTPG_BUS_CTL, busCtl & ~BUS_CTL_IOCHRDY );
    }
    else
    {   
        csWritePacketPage( sc, PKTPG_BUS_CTL, busCtl | BUS_CTL_IOCHRDY );
    }
    
    /* Set the Line Control register to match the media type */
    if ( sc->mediaType == MEDIA_10BASET )
    {
        csWritePacketPage( sc, PKTPG_LINE_CTL, LINE_CTL_10BASET );
    }
    else
    {   
        csWritePacketPage( sc, PKTPG_LINE_CTL, LINE_CTL_AUI_ONLY );
    }
    
    /* Set the BSTATUS/HC1 pin to be used as HC1 */
    /* HC1 is used to enable the DC/DC converter */
    selfCtl = SELF_CTL_HC1E;

    /* If the media type is 10Base2 */
    if ( sc->mediaType == MEDIA_10BASE2 )
    {
        /* Enable the DC/DC converter */
        /* If the DC/DC converter has a low enable */
        if ( (sc->configFlags & CFGFLG_DCDC_POL) == 0 )
            /* Set the HCB1 bit, which causes the HC1 pin to go low */
            selfCtl |= SELF_CTL_HCB1;
    }
    else  /* Media type is 10BaseT or AUI */
    {
        /* Disable the DC/DC converter */
        /* If the DC/DC converter has a high enable */
        if ( (sc->configFlags & CFGFLG_DCDC_POL) != 0 )
        {
            /* Set the HCB1 bit, which causes the HC1 pin to go low */
            selfCtl |= SELF_CTL_HCB1;
        }
    }
    csWritePacketPage( sc, PKTPG_SELF_CTL, selfCtl );

    /* If media type is 10BaseT */
    if ( sc->mediaType == MEDIA_10BASET )
    {
        /* If full duplex mode then set the FDX bit in TestCtl register */
        if ( sc->configFlags & CFGFLG_FDX )
        {
            KERN_DEBUG(csdebug, CSINIT_DBG_INFO, 
                       ("csInitChip : in full duplex mode\n"));
            csWritePacketPage( sc, PKTPG_TEST_CTL, TEST_CTL_FDX );
        }
    }

    /* RX_CTL set in csSetLadrFilt(), below */

    /* enable all transmission interrupts */
    csWritePacketPage( sc, PKTPG_TX_CFG, TX_CFG_ALL_IE );

    /* Accept all receive interrupts */
    csWritePacketPage( sc, PKTPG_RX_CFG, RX_CFG_ALL_IE );

    /* 
     * Configure Operational Modes
     * 
     * I have turned off the BUF_CFG_RX_MISS_IE, to speed things up, this is 
     * a better way to do it because the card has a counter which can be 
     * read to update the RX_MISS counter. This saves many interupts.
     * 
     * I have turned on the tx and rx overflow interupts to counter using 
     * the receive miss interrupt. This is a better estimate of errors 
     * and requires lower system overhead.
     */
    csWritePacketPage( sc, PKTPG_BUF_CFG, BUF_CFG_TX_UNDR_IE | 
		      BUF_CFG_RX_DMA_IE );

    if (sc->configFlags & CFGFLG_DMA_MODE)
    {
        /* 
         * First we program the DMA controller 
         * and ensure the memory buffer is valid.
         * If it isn't then we just go on without DMA
         */
        isa_dmastart(DMAMODE_READ | DMAMODE_LOOP, sc->dmaBase, sc->dmaMemSize, 
                     sc->sc_drq );      
            
        sc->dma_offset = sc->dmaBase;
        /* interrupt when a DMA'd frame is received */
        csWritePacketPage( sc, PKTPG_RX_CFG, RX_CFG_ALL_IE 
                          | RX_CFG_RX_DMA_ONLY );
            
        /* set the DMA burst bit so we don't tie up the bus
         * for too long.
         */
        if ( sc->dmaMemSize == 16384 )
        {
            KERN_DEBUG(csdebug, CSINIT_DBG_INFO,
                       ("csInitChip : using 16K buffer\n"));
            csWritePacketPage( sc, PKTPG_BUS_CTL,
                              ( (csReadPacketPage(sc, PKTPG_BUS_CTL) \
			       & ~BUS_CTL_DMA_SIZE) | BUS_CTL_DMA_BURST));
        }
        else /* use 64K */
        {
            csWritePacketPage( sc, PKTPG_BUS_CTL,
                              csReadPacketPage(sc, PKTPG_BUS_CTL) \
                               | BUS_CTL_DMA_SIZE | BUS_CTL_DMA_BURST);
        }
        
	csWritePacketPage( sc, PKTPG_DMA_CHANNEL, sc->sc_drq - 5);
        
    }

    /* If memory mode is enabled */
    if ( sc->configFlags & CFGFLG_MEM_MODE )
    {
        KERN_DEBUG(csdebug , CSINIT_DBG_INFO,
                   ("csInitChip : Using Memory Mode\n"));
        
        /* If external logic is present for address decoding */
        if ( csReadPacketPage(sc,PKTPG_SELF_ST) & SELF_ST_EL_PRES )
        {
            /* Program the external logic to decode address bits SA20-SA23 */
            csWritePacketPage( sc, PKTPG_EEPROM_CMD,
                              ( (sc->pPacketPagePhys & 0xffffff) >> 20) |
                              EEPROM_CMD_ELSEL );
        }

        /* Setup chip for memory mode */
        /* Write the packet page base physical address to the memory
         * base register.
         */
        csWritePacketPage( sc, PKTPG_MEM_BASE, 
                          sc->pPacketPagePhys & 0xFFFF );
        csWritePacketPage( sc, PKTPG_MEM_BASE + 2, 
                          sc->pPacketPagePhys >> 16 );
        busCtl = BUS_CTL_MEM_MODE;

        /* tell the chip to read the addresses off the SA pins */
        if ( sc->configFlags & CFGFLG_USE_SA )
        {
            busCtl |= BUS_CTL_USE_SA;
        }
        csWritePacketPage( sc, PKTPG_BUS_CTL, 
			  csReadPacketPage(sc, PKTPG_BUS_CTL) |  busCtl );

        /* We are in memory mode now! */
        sc->inMemoryMode = TRUE;

        /* wait here (10ms) for the chip to swap over. 
         * this is the maximum time that this could take.
         */
        delay(10000);
        
        /* Verify that we can read from the chip */
        isaId =  csReadPacketPage( sc, PKTPG_EISA_NUM );

        /* As a last minute sanity check before actually using mapped memory
         * we verify that we can read the isa number from the chip
         * in memory mode.
         */
        if ( isaId != EISA_NUM_CRYSTAL )
        {
            panic("\n%s - Mapped memory mode failed.\n",
		  (sc->sc_dev).dv_xname);
            /* nope didn't work abandon the idea and use pIO */
            sc->inMemoryMode = FALSE; 
        }
        else
        {
            /* we are in memory mode so if we aren't using DMA, then 
             * program the chip to interrupt early.
             */
            if ( (sc->configFlags & CFGFLG_DMA_MODE) == 0 )
            {
                KERN_DEBUG(csdebug, CSINIT_DBG_INFO,
                       ("csInitChip : about to turn on early interrupts\n"));
                csWritePacketPage(sc, PKTPG_BUF_CFG, BUF_CFG_RX_DEST_IE |
                                  BUF_CFG_RX_MISS_OVER_IE |
                                  BUF_CFG_TX_COL_OVER_IE );
            }
        }
        
    } /* if configFlags=memory mapped */

    /* Put Ethernet address into the Individual Address register */
    pIA = (pia)sc->sc_enaddr;
    csWritePacketPage( sc, PKTPG_IND_ADDR,   pIA->word[0] );
    csWritePacketPage( sc, PKTPG_IND_ADDR+2, pIA->word[1] );
    csWritePacketPage( sc, PKTPG_IND_ADDR+4, pIA->word[2] );
    
    /* Set the interrupt level in the chip */
    if ( sc->sc_int == 5 )
    {
        csWritePacketPage( sc, PKTPG_INT_NUM, 3 );
    }
    else
    {
        csWritePacketPage( sc, PKTPG_INT_NUM, (sc->sc_int)-10 );
    }
    
    /* write the multicast mask to the address filter register */
    csSetLadrFilt(sc, &sc->sc_ethercom);

    /* Enable reception and transmission of frames */
    csWritePacketPage( sc, PKTPG_LINE_CTL,
                      csReadPacketPage(sc,PKTPG_LINE_CTL) | 
                      LINE_CTL_RX_ON | LINE_CTL_TX_ON );
    
    /* Enable interrupt at the chip */
    csWritePacketPage( sc, PKTPG_BUS_CTL,
                      csReadPacketPage(sc,PKTPG_BUS_CTL) | BUS_CTL_INT_ENBL );

}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csInit
**
**  Description      :  This routine is called 
**                      to initialize this network interface driver.  This 
**                      routine may be called several times for each
**                      operating system reboot to dynamically bring the 
**                      network interface driver to an up and running state.  
**                      This routine is called by the csIoctl() routine.
**
**                      This routine resets and then initializes the 
**                      CS8900 chip.
**            
**  Parameters       :  struct cs_softc *sc
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  CS_ERROR if can't read from the chip.
**            
**  Side Effects     :  None.
**
*/
int csInit( struct cs_softc *sc )
{
    int intState;
    int error = CS_OK;

    intState = splimp();

    /* Mark the interface as down */
    sc->sc_ethercom.ec_if.if_flags &= ~(IFF_UP | IFF_RUNNING);

    /* Enable debugging */
#ifdef CS_DEBUG
    sc->sc_ethercom.ec_if.if_flags |= IFF_DEBUG; 
#endif

    /* Reset the chip */
    if ( (error = csResetChip(sc)) == CS_OK )
    {
	/* Initialize the chip */
	csInitChip( sc );

	/* Mark the interface as up and running */
	sc->sc_ethercom.ec_if.if_flags |= (IFF_UP | IFF_RUNNING);
	sc->sc_ethercom.ec_if.if_flags &= ~IFF_OACTIVE;
	sc->sc_ethercom.ec_if.if_timer = 0;
    }
    else
    {
	printf("csInit : couldn't reset the chip!\n");
    }
     
    splx(intState);
    return error;
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csSetLadrFilt
**
**  Description      :  Reads the multicast addresses from the sc_enaddr
**                      structure and programs the cs8900's logical 
**                      address filter to accept the frames.
**            
**                      This function should only be called when the chip is 
**                      being reset, it will overwrite whatever is in the
**                      logical address filter register.
** 
**  Parameters       :  struct ethercom
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  
**            
**  Side Effects     :  The logical address filter register on the 
**                      cs8900 is reprogrammed, any addresses which 
**                      were set will be cleared unless they are still
**                      in the multicast list.
**
*/
void csSetLadrFilt( struct cs_softc *sc, struct ethercom *ec )
{
    struct ifnet *ifp = &(ec->ec_if);
    struct ether_multi *enm;
    struct ether_multistep step;
    ushort af[4];
    ushort port, mask, index;

    /*
     * Set up multicast address filter by passing all multicast addresses
     * through a crc generator, and then using the high order 6 bits as an
     * index into the 64 bit logical address filter.  The high order bit
     * selects the word, while the rest of the bits select the bit within
     * the word.
     *
     */
    if (ifp->if_flags & IFF_PROMISC)
    {
	/* accept all valid frames. */
	 csWritePacketPage( sc, PKTPG_RX_CTL,
			   RX_CTL_PROMISC_A | RX_CTL_RX_OK_A |
			   RX_CTL_IND_A | RX_CTL_BCAST_A | RX_CTL_MCAST_A );
	ifp->if_flags |= IFF_ALLMULTI;
        return;
    }

    /* accept frames if a. crc valid, b. individual address match
     * c. broadcast address,and d. multicast addresses matched in the 
     * hash filter
     */
    csWritePacketPage( sc, PKTPG_RX_CTL,
                      RX_CTL_RX_OK_A | RX_CTL_IND_A | RX_CTL_BCAST_A |
                      RX_CTL_MCAST_A );


    /* start off with all multicast flag clear, set it if we need to later,
     * otherwise we will leave it.
     */
    ifp->if_flags &= ~IFF_ALLMULTI;

    af[0] = af[1] = af[2] = af[3] = 0x0000;
    ETHER_FIRST_MULTI(step, ec, enm);
    /* 
     * Loop through all the multicast addresses unless we get a range 
     * of addresses, in which case we will just accept all packets. 
     * Justification for this is given in the next comment.
     */
    while ( (enm != NULL) && ( (ifp->if_flags | IFF_ALLMULTI) == 0) ) 
    {
        if (bcmp(enm->enm_addrlo, enm->enm_addrhi, sizeof enm->enm_addrlo))
        {
            /*
             * We must listen to a range of multicast addresses.
             * For now, just accept all multicasts, rather than
             * trying to set only those filter bits needed to match
             * the range.  (At this time, the only use of address
             * ranges is for IP multicast routing, for which the
             * range is big enough to require all bits set.)
             */
            ifp->if_flags |= IFF_ALLMULTI;
            af[0] = af[1] = af[2] = af[3] = 0xffff;
        }
        else
        {
            /*
             * we have got an individual address so just set that 
             * bit.
             */
            index = csHashIndex(enm->enm_addrlo);

            /* Set the bit the Logical address filter. */
            port = (ushort) ( 2 * ((index / 16)) );
            mask = (ushort) ( 1 << (15 - (index % 16)) );
            af[port] |= mask;
            
            ETHER_NEXT_MULTI(step, enm);
        }
    }
    /* now program the chip with the addresses */
    csWritePacketPage(sc, PKTPG_LOG_ADDR + 2, af[0]);
    csWritePacketPage(sc, PKTPG_LOG_ADDR + 2, af[1]);
    csWritePacketPage(sc, PKTPG_LOG_ADDR + 2, af[2]);
    csWritePacketPage(sc, PKTPG_LOG_ADDR + 2, af[3]);
    return;
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csHashIndex.
**
**  Description      :  This function calculates Hash index for address.
**                      This code is chip specific, I pulled it from the 
**                      SCO Unix driver I think. I could not find this 
**                      algorithm anywhere in the manuals.
** 
**  Parameters       :  char *addr        Pointer to address.
**
**  Implicit Inputs  :  None.
**  
**  Implicit Outputs :  None.
**
**  Return Value     :  The corresponding bit number in the logical 
**                      address filter corresponding to addr. 
**
**  Side Effects     :  None.
*/
ushort csHashIndex(char *addr)
{
    uint POLY = 0x04c11db6;
    uint crc_value   = 0xffffffff;
    ushort hash_code  = 0;
    int   i;
    uint current_bit;
    char current_byte = *addr;
    uint cur_crc_high;

    for ( i = 0; i < 6; i++)
    {
        current_byte = *addr;
        addr++;

        for(current_bit = 8; current_bit; current_bit--)
        {
            cur_crc_high = crc_value >> 31;
            crc_value <<= 1;
            if(cur_crc_high ^ (current_byte & 0x01))
            {
                crc_value ^= POLY;
                crc_value |= 0x00000001;
            }
            current_byte >>= 1;
        }
    }

    /*
    **  The hash code is the 6 least significant bits of the CRC
    **  in the reverse order: CRC[0] = hash[5],CRC[1] = hash[4],etc.
    */
    for(i=0; i < 6; i++) 
    {
        hash_code = (ushort)( (hash_code << 1) | 
                             (ushort)((crc_value >> i) & 0x00000001));
    }
 
    return hash_code;
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csReset
**
**  Description      :  This routine is a major entry point and is called 
**                      by the protocol stack to shut down this network 
**                      interface driver.  This routine may be called 
**                      several times for each operating system reboot 
**                      to dynamically bring the network interface driver 
**                      to a non-running state.
**
**                      This routine resets the CS8900 chip.
**
**  Parameters       :  struct cs_softc *sc
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  void
**
**  Side Effects     :  None.
**
*/
void csReset( void * arg)
{
    struct cs_softc *sc = arg;
    /* Mark the interface as down */
    sc->sc_ethercom.ec_if.if_flags &= ~IFF_RUNNING;

    /* Reset the chip */
    csResetChip( sc );
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csIoctl
**
**  Description      :  This routine is a major entry point and is called 
**                      by the protocol stack to modify characteristics 
**                      of this network interface driver.  There are many
**                      network interface ioctl commands, this 
**                      driver only supports:
**                            Set Interface Address
**                            Set Interface Flags.
**                            Add/Delete Multicast addresses
**
**  Parameters       :  struct ifnet
**                      command
**                      pData
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  0, if successful, else a standard error code
**                      is returned. 
**
**  Side Effects     :  None.
**
*/
int csIoctl( struct ifnet *pIf, ulong com, caddr_t pData )
{
    int state;
    int result;
    struct cs_softc *sc = pIf->if_softc;
    struct ifaddr *ifa = (struct ifaddr *) pData;
    struct ifreq *ifr = (struct ifreq *)pData;

    state = splimp();

    result = 0;  /* only set if something goes wrong */

    switch( com )
    {
        case SIOCSIFADDR:  
	   /* only support INET at this stage */
           if ( ifa->ifa_addr->sa_family == AF_INET ) 
           {
               csInit(sc);
               /* Set interface address (IP address) */
               arp_ifinit(&sc->sc_ethercom.ec_if, ifa);
           } 
	   else
	   {
	       result  = EPERM;
	   }
	   break;
        case SIOCSIFFLAGS:  
           /* Set interface flags */
           KERN_DEBUG(csdebug, CSIOCTL_DBG_INFO,
                      ("csIoctl: Set interface flags\n"));
           if ( pIf->if_flags & IFF_UP )
           {
               /* Mark the interface as up and running */
               sc->sc_ethercom.ec_if.if_flags |= (IFF_UP | IFF_RUNNING);

               /* Initialize the interface */
               csInit( pIf->if_softc );
           }
           else  /* The interface is down */
           {
               /* Mark the interface as down */
               sc->sc_ethercom.ec_if.if_flags &= ~(IFF_UP | IFF_RUNNING);

               /* Reset the chip */
               csResetChip( sc );
           }
	   break;
        case SIOCADDMULTI:
           result = ether_addmulti(ifr, &sc->sc_ethercom); 
    
           if (result == ENETRESET) 
           {
               /*
                * Multicast list has changed; set the hardware filter
                * accordingly.
                */
               csInit(sc);
               result = 0;
           }
	   break;
        case SIOCDELMULTI:
           result = ether_delmulti(ifr, &sc->sc_ethercom);

           if (result == ENETRESET) 
           {
               /*
                * Multicast list has changed; set the hardware filter
                * accordingly.
                */
               csInit(sc);
               result = 0;
           }
	   break;
        default:
           result = EINVAL;  /* Invalid argument */
	   break;
    }

    splx( state );

    return result;
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csIntr
**
**  Description      :  This routine is the interrupt service routine.  
**                      This routine is called by assembly language wrapper 
**                      code whenever the CS8900 chip generates an
**                      interrupt.  The wrapper code issues an EOI command 
**                      to the 8259 PIC.
**
**                      This routine processes the events on the Interrupt 
**                      Status Queue.  The events are read one at a time 
**                      from the ISQ and the appropriate event handlers are
**                      called.  The ISQ is read until it is empty.  If 
**                      the chip's interrupt request line is active, then 
**                      reading a zero from the ISQ will deactivate the
**                      interrupt request line.
**            
**  Parameters       :  arg is a pointer to the softc struct
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  0 if the interupt was not handled (chaining)
**                      1 if the interupt was handled.
**                      -1 is also valid, but we don't return it, 
**                      because I don't know what it means.
**            
**  Side Effects     :  None.
**
*/
int csIntr( void *arg )
{
    struct cs_softc *sc = arg;
    ushort Event;

    /* Ignore any interrupts that happen while the chip is being reset */
    if ( sc->resetting ) 
    {
        printf("%s : interupt handler: already resetting\n", 
               (sc->sc_dev).dv_xname);
        /* this was probably still caused by the chip but don't worry */
        return 1;
    }

    /* Read an event from the Interrupt Status Queue */
    Event = bus_space_read_2( sc->sc_memt, sc->sc_memh, PKTPG_ISQ );

    /* Process all the events in the Interrupt Status Queue */
    while ( Event != 0 )
    {
        /* Dispatch to an event handler based on the register number */
        switch ( Event & REG_NUM_MASK )
        {
            case REG_NUM_RX_EVENT:
               csReceiveEvent( sc, Event );
               break;
            case REG_NUM_TX_EVENT:
               csTransmitEvent(sc, Event);
               break;
            case REG_NUM_BUF_EVENT:
               csBufferEvent( sc, Event );
	       break;
            case REG_NUM_TX_COL:
            case REG_NUM_RX_MISS:
               csCounterEvent( sc, Event );
	       break;
            default:
               printf("%s : Unknown interrupt event 0x%x\n", 
                      (sc->sc_dev).dv_xname, Event );
	       break;
        }

        /* Read another event from the Interrupt Status Queue */
	Event = bus_space_read_2( sc->sc_memt, sc->sc_memh, PKTPG_ISQ );
    }
    /* have handled the interupt */
    return 1;
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csCounterEvent
**
**  Description      :  Handles the transmit collision and receive miss
**                      counter overflow events. These event occur
**                      when the counters reach 0x200. This should
**                      leave ample time to read the counter before 
**                      overflow of the 11 bit counter.
**
**  Parameters       :  struct cs_softc *sc
**                      cntEvent : the interrupt status queue register.
**
**  Implicit Inputs  :  none.
**
**  Implicit Outputs :  none.
**
**  Return Value     :  void.
**
**  Side Effects     :  the ifnet's counters have been updated.
**
*/
void csCounterEvent( struct cs_softc *sc, ushort cntEvent )
{
    struct ifnet *pIf;
    ushort errorCount;
    
    pIf = &sc->sc_ethercom.ec_if;

    switch ( cntEvent & REG_NUM_MASK ) 
    {
        case REG_NUM_TX_COL :
            /* the count should be read before
             * an overflow occurs.
             */
            errorCount = csReadPacketPage(sc, PKTPG_TX_COL);
            /* the tramsit event routine always 
             * checks the number of collisions for any packet
             * so we don't increment any counters here, as they
             * should already have been considered.
             */
        break;
        case REG_NUM_RX_MISS :
            /* the count should be read before
             * an overflow occurs.
             */
            errorCount = csReadPacketPage(sc, PKTPG_RX_MISS);
            /* Increment the input error count,
             * the first 6bits are the register id.
             */
            pIf->if_ierrors += ( (errorCount & 0xffC0) >> 6 );
        break;
        default :
            /* do nothing */
            break;
    }
    return ;
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csBufferEvent
**
**  Description      :  The routine is called whenever an event occurs 
**                      regarding the transmit and receive buffers within 
**                      the CS8900 chip.  This should be called when 
**                      early detection of received packets is enabled.
**
**                      Software generated interrupts are checked but 
**                      should never be generated.
**         
**  Parameters       :  struct cs_softc *sc
**                      bufEvent
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  void.
**         
**  Side Effects     :  None.
**
*/
void csBufferEvent( struct cs_softc *sc, ushort bufEvent )
{
    struct ifnet *pIf;

    pIf = &sc->sc_ethercom.ec_if;

    KERN_DEBUG(csdebug, CSBUFF_DBG_INFO,
               ("csBufferEvent : event = 0x%x\n",bufEvent));
    
    /* multiple events can be in the buffer event register at 
     * one time so a standard switch statement will not suffice, here
     * every event must be checked.
     */

    /* if 128 bits have been rxed by the time we get here, the
     * dest event will be cleared and 128 event will be set.
     */
    if ( (bufEvent & (BUF_EVENT_RX_DEST | BUF_EVENT_RX_128)) != 0 )
    {
        csProcessRxEarly(sc);
    }

    if ( bufEvent & BUF_EVENT_RX_DMA )
    {
        /* process the receive data */
        csProcessRxDMA(sc);
    }
  
    if (bufEvent & BUF_EVENT_TX_UNDR) {
#if 0 /* this is an expected condition (occasionally) */
        printf("%s: transmit underrun (%d -> %d)\n", sc->sc_dev.dv_xname,
	  sc->sc_xe_ent, cs_xmit_early_table[sc->sc_xe_ent].worse);
#endif
	sc->sc_xe_ent = cs_xmit_early_table[sc->sc_xe_ent].worse;
	sc->sc_xe_togo = cs_xmit_early_table[sc->sc_xe_ent].better_count;
	/* had an underrun, transmit is finished */
	sc->txInProgress = FALSE;
    }

    if ( bufEvent & BUF_EVENT_SW_INT )
    {
        printf( "%s : Software initiated interrupt\n", 
               (sc->sc_dev).dv_xname );
    }

}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csTransmitEvent
**
**  Description      :  This routine is called whenever the transmission 
**                      of a packet has completed successfully or 
**                      unsuccessfully.  If the transmission was not 
**                      successful, then the output error count is 
**                      incremented.  If collisions occured while
**                      sending the packet, then the number of collisions 
**                      is added to the collision counter.  If there are 
**                      more packets on the transmit queue, then the next
**                      packet is started by calling 
**                      csStartOutput().
**         
**  Parameters       :  struct cs_softc *sc
**                      txEvent
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  void.
**         
**  Side Effects     :  None.
**
*/
void csTransmitEvent( struct cs_softc *sc, ushort txEvent )
{
    struct ifnet *pIf = &sc->sc_ethercom.ec_if;

    KERN_DEBUG(csdebug, CSTXEVENT_DBG_INFO,
               ("csTransmitEvent : event = 0x%x\n", txEvent));

    /* If there were any errors transmitting this frame */
    if ( txEvent & (TX_EVENT_LOSS_CRS | TX_EVENT_SQE_ERR | TX_EVENT_OUT_WIN |
                    TX_EVENT_JABBER | TX_EVENT_16_COLL) )
    {
        /* Increment the output error count */
        pIf->if_oerrors++;

        /* If debugging is enabled then log error messages */
        if ( pIf->if_flags & IFF_DEBUG )
        {
            if ( txEvent & TX_EVENT_LOSS_CRS )
	    {
                printf( "%s : Loss of carrier\n", (sc->sc_dev).dv_xname );
	    }
            if ( txEvent & TX_EVENT_SQE_ERR )
	    {
                printf( "%s : SQE error\n", (sc->sc_dev).dv_xname );
	    }
            if ( txEvent & TX_EVENT_OUT_WIN )
	    {
                printf( "%s : Out-of-window collision\n", 
                       (sc->sc_dev).dv_xname );
	    }
            if ( txEvent & TX_EVENT_JABBER )
	    {
                printf("%s : Jabber\n", (sc->sc_dev).dv_xname );
	    }
            if ( txEvent & TX_EVENT_16_COLL )
	    {
                printf( "%s : 16 collisions\n", (sc->sc_dev).dv_xname );
	    }
        }
    }
    else
    {
        ledNetActive();
    }

    /* Add the number of collisions for this frame */
    if ( txEvent & TX_EVENT_16_COLL )
    {
        pIf->if_collisions += 16;
    }
    else
    {
        pIf->if_collisions += ((txEvent & TX_EVENT_COLL_MASK) >> 11);
    }

    pIf->if_opackets++;
    /* Transmission is no longer in progress */
    sc->txInProgress = FALSE;

    /* If there is more to transmit */
    if ( pIf->if_snd.ifq_head != NULL )
    {
        /* Start the next transmission */
        csStartOutput(pIf);
    }
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csReceiveEvent
**
**  Description      :  This routine is called whenever a packet is 
**                      received at the chip.  If the packet is received 
**                      with errors, then the input error count is 
**                      incremented. If the packet is received OK, then 
**                      the data is copied to an internal receive
**                      buffer and the csProcessReceive() routine is 
**                      called to process the received packet.
**     
**  Parameters       :  struct cs_softc *sc
**                      rxEvent
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  void.
**     
**  Side Effects     :  None.
**
*/
void csReceiveEvent( struct cs_softc *sc, ushort rxEvent )
{
    struct ifnet *pIf = &sc->sc_ethercom.ec_if;

    KERN_DEBUG(csdebug, CSRXNORM_DBG_INFO, 
               ("csProcessReceive : packet received event = 0x%x\n",rxEvent));

    /* If the frame was not received OK */
    if ( !(rxEvent & RX_EVENT_RX_OK) )
    {
        /* Increment the input error count */
        pIf->if_ierrors++;
        
        /* If debugging is enabled then log error messages */
        if ( pIf->if_flags & IFF_DEBUG )
        {
            /* If an error bit is set */
            if ( rxEvent != REG_NUM_RX_EVENT )
            {
                if ( rxEvent & RX_EVENT_RUNT )
		{
                    printf( "%s : Runt\n", (sc->sc_dev).dv_xname );
		}
                if ( rxEvent & RX_EVENT_X_DATA )
		{
                    printf( "%s : Extra data\n", (sc->sc_dev).dv_xname );
		}
                if ( rxEvent & RX_EVENT_CRC_ERR )
                {
                    if ( rxEvent & RX_EVENT_DRIBBLE )
		    {
                        printf( "%s : Alignment error\n", 
                               (sc->sc_dev).dv_xname );
		    }
                    else
		    {
                        printf( "%s : CRC Error\n", (sc->sc_dev).dv_xname );
		    }
                }
                else 
		{
                    if ( rxEvent & RX_EVENT_DRIBBLE )
		    {
                        printf( "%s : Dribble bits\n", (sc->sc_dev).dv_xname );
		    }
		}

                /* Must read the length of all received frames */
                csReadPacketPage( sc, PKTPG_RX_LENGTH );
                
                /* Skip the received frame */
                csWritePacketPage( sc, PKTPG_RX_CFG,
                                  csReadPacketPage(sc,PKTPG_RX_CFG) | 
                                  RX_CFG_SKIP );
            }
            else 
	    {
                printf( "%s : Implied skip\n", (sc->sc_dev).dv_xname );
	    }
        }
    }
    else
    {
        /* process the received frame and pass it up to the upper layers. */
        csProcessReceive(sc);
    }
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csEtherInput
**
**  Description      :  This routine hands a received packet to the higher-
**                      level networking code, taking care to pass it
**                      to BPF as appropriate and to discard it when
**                      necessary.
**
**  Parameters       :  struct cs_softc *sc	network interface softc
**                      struct mbuf *m		incoming packet
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  void.
**         
**  Side Effects     :  Mbuf chain passed to higher-level network code,
**                      or deallocated if it shouldn't be passed up.
**
*/
void
csEtherInput(sc, m)
	struct cs_softc *sc;
	struct mbuf *m;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_header *eh;

	ifp->if_ipackets++;

	/* the first thing in the mbuf is the ethernet header.
	 * we need to pass this header to the upper layers as a structure,
	 * so cast the start of the mbuf, and adjust the mbuf to point to 
	 * the end of the ethernet header, ie the ethernet packet data.
	 */
	eh = mtod(m, struct ether_header *);

#if NBPFILTER > 0
	/*
	 * Check if there's a BPF listener on this interface.
	 * If so, hand off the raw packet to BPF.
	 * 
	 */
	if (ifp->if_bpf) {
		bpf_mtap(ifp->if_bpf, m);

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no BPF listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 */
		if ((ifp->if_flags & IFF_PROMISC) &&
		    (eh->ether_dhost[0] & 1) == 0 && /* !mcast and !bcast */
		    bcmp(eh->ether_dhost, sc->sc_enaddr,
			    sizeof(eh->ether_dhost)) != 0) {
			m_freem(m);
			return;
		}
	}
#endif
    
	/* Pass the packet up, with the ether header sort-of removed. 
	 * ie. adjust the data pointer to point to the data.
	 */
	m_adj(m, sizeof(struct ether_header));    
	ether_input(ifp, eh, m);
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csProcessReceive
**
**  Description      :  This routine processes a received packet. The 
**                      received packet is read from the chip directly
**                      into a clustered mbuf. These being 1 page 
**                      on i386 netBSD, and thus guarenteed to fit 
**                      an ethernet frame.
**
**  Parameters       :  struct cs_softc *sc
**                      pRxBuf
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  void.
**         
**  Side Effects     :  None.
**
*/
void csProcessReceive( struct cs_softc *sc )
{
    struct ifnet *pIf;
    struct mbuf *m;
    int totlen;
    int len;
    volatile ushort *pBuff, *pBuffLimit;
    int pad;
    unsigned int frameOffset;

    ledNetActive();

    pIf = &sc->sc_ethercom.ec_if;

    /* Initialize the frame offset*/
    frameOffset = PKTPG_RX_LENGTH;

    /* Get the length of the received frame */
    totlen = bus_space_read_2( sc->sc_memt, sc->sc_memh, 
			      frameOffset );
    frameOffset += 2;
    KERN_DEBUG(csdebug, CSRXNORM_DBG_INFO, 
	       ("csProcessReceive : length = 0x%x\n",totlen));

    MGETHDR(m, M_DONTWAIT, MT_DATA);
    if (m == 0)
    {
        printf("%s : csProcessReceive : Couldn't allocate mbuf\n", 
               (sc->sc_dev).dv_xname);
	pIf->if_ierrors++;
        /* couldn't allocate an mbuf so things are not good, 
         * may as well drop the packet I think.
         *
         * have already read the length so we should be 
         * right to skip the packet.
         */
        csWritePacketPage( sc, PKTPG_RX_CFG,
                          csReadPacketPage(sc,PKTPG_RX_CFG) | RX_CFG_SKIP );
        return ;
    }

    m->m_pkthdr.rcvif = pIf;
    m->m_pkthdr.len = totlen;

    /* save processing by always using a mbuf cluster, 
     * guarenteed to fit packet, on i386 netBSD anyway.
     */
    MCLGET(m, M_DONTWAIT);
    if (m->m_flags & M_EXT)
    {
        len = MCLBYTES;
    }
    else
    {
        /* couldn't allocate an mbuf cluster */
        printf("%s : csProcessReceive : couldn't allocate a cluster\n",
	       (sc->sc_dev).dv_xname);
        m_freem(m);
        /* skip the received frame */
        csWritePacketPage( sc, PKTPG_RX_CFG,
                          csReadPacketPage(sc,PKTPG_RX_CFG) | RX_CFG_SKIP );

        return ;
    }

    /* align ip header on word boundary for ipintr */
    pad = ALIGN(sizeof(struct ether_header)) 
        - sizeof(struct ether_header);
    m->m_data += pad;
    
    m->m_len = len = min(totlen, len);
    pBuff = mtod(m, ushort *);
    pBuffLimit = pBuff + (len + 1 )/2; /* don't want to go over */
    /* now read the data from the chip */
    while ( pBuff <  pBuffLimit )
    {
	*pBuff++ =  bus_space_read_2( sc->sc_memt, sc->sc_memh, 
				     frameOffset );
	frameOffset += 2;
    }

    csEtherInput(sc, m);
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csProcessRxDMA
**
**  Description      :  This routine is called by the ISR to accept a
**                      frame from the adapter. (Called on a Rx ISQ
**                      event). It copies frames which have been
**                      DMA transfered to memory.
**
**  Parameters       :  softc
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  None.
**
**  Side Effects     :
*/
void csProcessRxDMA(struct cs_softc *sc)
{
    struct ifnet *pIf;
    ushort num_dma_frames;
    ushort pkt_length;
    ushort status;
    uint to_copy;
    char *dma_mem_ptr;
    struct mbuf *m;
    u_char *pBuff;
    int pad;

    /* initialise the pointers */
    pIf = &sc->sc_ethercom.ec_if;

    /* Read the number of frames DMAed. */
    num_dma_frames = csReadPacketPage( sc, PKTPG_DMA_FRAME_COUNT);
    num_dma_frames &= (ushort) (0x0fff);

    /* Loop till number of DMA frames ready to read is zero. 
     * After reading the frame out of memory we must check 
     * if any have been received while we were processing
     */
    while ( num_dma_frames != 0 )
    {
        dma_mem_ptr =  sc->dma_offset;
        
        /* process all of the dma frames in memory
         *
         * This loop relies on the dma_mem_ptr variable being set
         * to the next frames start address.
         */
        for(; num_dma_frames > 0; num_dma_frames--)
        {
	    
            /* Get the length and status of the packet. 
	     * Only the status is guarenteed to be at 
	     * dma_mem_ptr, ie need to check for wraparound 
	     * before reading the length
	     */
            status = *((unsigned short *)dma_mem_ptr)++;
	    if (dma_mem_ptr > (sc->dmaBase + sc->dmaMemSize))
	    {
		dma_mem_ptr = sc->dmaBase;
	    }
            pkt_length = *((unsigned short *)dma_mem_ptr)++;

            /* Do some sanity checks on the length and status. */
            if( (pkt_length > ETHER_MTU) || ((status & DMA_STATUS_BITS) != 
					DMA_STATUS_OK) )
            {
                /* the SCO driver kills the adapter in this situation */
                /* should increment the error count
                 * and reset the dma operation.
                 */
                printf("%s : csProcessRxDMA : DMA buffer out of sync about to reset\n",
		       (sc->sc_dev).dv_xname);
                KERN_DEBUG(csdebug, CSRXDMA_DBG_INFO,
                           ("csProcessRxDMA : error length = %d, \
                            status = 0x%x\n", pkt_length, status));
                KERN_DEBUG(csdebug, CSRXDMA_DBG_INFO,
                           ("csProcessRxDMA : the offset is 0x%x\n", \
                            (int) sc->dma_offset));
                pIf->if_ierrors++;
                /* skip the rest of the DMA buffer */
		isa_dmadone(DMAMODE_READ | DMAMODE_LOOP, sc->dmaBase, sc->dmaMemSize,
			    sc->sc_drq);
		/* now reset the chip and reinitialise */
		csInit(sc);
                return ;        
            }

            /* Check the status of the received packet. */
            if ( status & RX_EVENT_RX_OK )
            {
                /* get a new mbuf */
                MGETHDR(m, M_DONTWAIT, MT_DATA);
                if (m == 0)
                {
                    printf("%s : csProcessRxDMA : Couldn't allocate mbuf\n",\
                           (sc->sc_dev).dv_xname);
                    pIf->if_ierrors++;
                    /* couldn't allocate an mbuf so things are not good, 
                     * may as well drop all the packets I think.
                     */
                    csReadPacketPage( sc, PKTPG_DMA_FRAME_COUNT );
                    /* now reset DMA operation */
		    isa_dmadone(DMAMODE_READ | DMAMODE_LOOP, sc->dmaBase, 
				sc->dmaMemSize, sc->sc_drq);
		    /* now reset the chip and reinitialise */
		    csInit(sc);
                    return ;
                }
		
		
                /* save processing by always using a mbuf cluster,
                 * guarenteed to fit packet
                 */
                MCLGET(m, M_DONTWAIT);
                if ( (m->m_flags & M_EXT) == 0)
                {
                    /* couldn't allocate an mbuf cluster */
                    printf("%s : csProcessRxDMA : couldn't allocate \
                            a cluster\n", (sc->sc_dev).dv_xname);
                    m_freem(m);
                    /* skip the frame */
                    csReadPacketPage( sc, PKTPG_DMA_FRAME_COUNT );
		    isa_dmadone(DMAMODE_READ | DMAMODE_LOOP, sc->dmaBase, 
				sc->dmaMemSize, sc->sc_drq);
		    /* now reset the chip and reinitialise */
		    csInit(sc);
                    return ;
                }
                m->m_pkthdr.rcvif = pIf;
                m->m_pkthdr.len = pkt_length;
                m->m_len = pkt_length;

                /* align ip header on word boundary for ipintr */
                pad = ALIGN(sizeof(struct ether_header)) 
                    - sizeof(struct ether_header);
                m->m_data += pad;               

                /* set up the buffer pointer to point 
                 * to the data area 
                 */
                pBuff = mtod(m, char *);

                /*
		 * Read the frame into free_pktbuf 
		 * The buffer is circular buffer, either
		 * 16K or 64K in length.
		 *
		 * need to check where the end of the buffer 
		 * is and go back to the start.
		 */
                if ( (dma_mem_ptr + pkt_length )
                    < sc->dmaBase + sc->dmaMemSize)
                {
                    /* No wrap around. Copy the frame header */
                    bcopy(dma_mem_ptr, pBuff, pkt_length);
                    dma_mem_ptr += pkt_length; 
                }
                else
                {
                    KERN_DEBUG(csdebug, CSRXDMA_DBG_INFO,
                               ("csProcessRxDMA : wraparound\n"));
                    to_copy = (uint) ( (sc->dmaBase + sc->dmaMemSize)
				      - dma_mem_ptr);

                    /* Copy the first half of the frame. */
                    bcopy(dma_mem_ptr, pBuff, to_copy);
                    pBuff += to_copy;
                    
                    /*
                     * Rest of the frame is to be read from the 
                     * first byte of the DMA memory.
                     */
                    /* Get the number of bytes leftout in the frame. */
                    to_copy = pkt_length - to_copy;
                
                    dma_mem_ptr = sc->dmaBase;
                    
                    /* Copy rest of the frame. */
                    bcopy(dma_mem_ptr, pBuff, to_copy);
                    dma_mem_ptr += to_copy;
                }

                KERN_DEBUG(csdebug, CSRXDMA_DBG_INFO,
                           ("csProcessRxDMA : about to pass packet up protocol stack\n"));
                KERN_DEBUG(csdebug, CSRXDMA_DBG_INFO,
                           ("csProcessRxDMA : start addr = 0x%x\n",
                            (unsigned int) mtod(m, char *)) );
                csEtherInput(sc, m);
            } /* (status & RX_OK) */
            else
            {
                /* the frame was not received OK */
                /* Increment the input error count */
                pIf->if_ierrors++;

                /* If debugging is enabled then log error messages */
		if ( pIf->if_flags & IFF_DEBUG )
		{
		    /* If an error bit is set */
		    if ( status != REG_NUM_RX_EVENT )
		    {
			if ( status & RX_EVENT_RUNT )
			{
			    printf( "%s : Runt\n", (sc->sc_dev).dv_xname );
			}
			if ( status & RX_EVENT_X_DATA )
			{
			    printf( "%s : Extra data\n", 
				   (sc->sc_dev).dv_xname );
			}
			if ( status & RX_EVENT_CRC_ERR )
			{
			    if ( status & RX_EVENT_DRIBBLE )
			    {
				printf( "%s : Alignment error\n", \
				       (sc->sc_dev).dv_xname);
			    }
			    else
			    {
				printf("%s : CRC Error\n", \
				       (sc->sc_dev).dv_xname );
			    }
			}
			else
			{
			    if ( status & RX_EVENT_DRIBBLE )
			    {
				printf("%s : Dribble bits\n", \
				       (sc->sc_dev).dv_xname );
			    }
			}
		    }
		}
            }
            /* now update the current frame pointer.
             * the dma_mem_ptr should point to the next packet to 
             * be received, without the alignment considerations.
	     * 
	     * The cs8900 pads all frames to start at the next 32bit
	     * aligned addres. hence we need to pad our offset pointer.
             */
            dma_mem_ptr += 3;
            dma_mem_ptr = (char *) ((long) dma_mem_ptr & 0xfffffffc);
            if ( dma_mem_ptr < (sc->dmaBase + sc->dmaMemSize) )
            {
                sc->dma_offset = dma_mem_ptr;
            }
            else
            {
                dma_mem_ptr = sc->dma_offset = sc->dmaBase;
            }
        } /* for all frames */
        /* Read the number of frames DMAed again. */
        num_dma_frames = csReadPacketPage( sc, PKTPG_DMA_FRAME_COUNT );
        num_dma_frames &= (ushort) (0x0fff);
    } /* while there are frames left */
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csProcessRxEarly
**
**  Description      :  This routine is called after a buffer event
**                      RxDest or Rx128 has occurred. The frame is read 
**                      from the chip as it arrives. The packet is then 
**                      skipped, so that the next packet can be read.
**
**                      For a more detailed description of this operation
**                      refer to section "5.2.9 Receive Frame Byte 
**                      Counter" in the chip spec.
**
**                      This code presently supports both mapped memory 
**                      mode and programmed I/O mode. Programmed I/O is
**                      not as fast as mapped memory and the programmed
**                      I/O code slows the mapped memory implementation
**                      down. To improve performance remove the programmed 
**                      I/O support.
**
**  Parameters       :  struct cs_softc *sc
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  void.
**
**  Side Effects     :  The frame is read from the chip and passed to the
**                        upper layer protocols if it is ok.
**
*/
void csProcessRxEarly( struct cs_softc *sc )
{
    struct ifnet *pIf;
    struct mbuf *m;
    ushort frameCount, oldFrameCount;
    ushort rxEvent;
    ushort *pBuff;
    int pad;    
    unsigned int frameOffset;
   

    pIf = &sc->sc_ethercom.ec_if;

    /* Initialize the frame offset */
    frameOffset = PKTPG_RX_FRAME;
    frameCount = 0;

    MGETHDR(m, M_DONTWAIT, MT_DATA);
    if (m == 0)
    {
        printf("%s : csProcessRxEarly : Couldn't allocate mbuf\n", 
               (sc->sc_dev).dv_xname);
        pIf->if_ierrors++;
        /*  couldn't allocate an mbuf so things are not good, may as well drop
         *  the packet I think.
         *
         * have already read the length so we should be
         * right to skip the packet.
         */
        csWritePacketPage( sc, PKTPG_RX_CFG,
                          csReadPacketPage(sc,PKTPG_RX_CFG) | RX_CFG_SKIP );
        return ;
    }
    
    m->m_pkthdr.rcvif = pIf;
    /* save processing by always using a mbuf cluster,
     * guarenteed to fit packet
     */
    MCLGET(m, M_DONTWAIT);
    if ( (m->m_flags & M_EXT) == 0)
    {
        /* couldn't allocate an mbuf cluster */
        printf("%s : csProcessRxEarly : couldn't allocate a cluster\n", 
               (sc->sc_dev).dv_xname);
        m_freem(m);
        /* skip the frame */
        csWritePacketPage( sc, PKTPG_RX_CFG,
                          csReadPacketPage(sc,PKTPG_RX_CFG) | RX_CFG_SKIP );
        return ;
    }

    /* align ip header on word boundary for ipintr */
    pad = ALIGN(sizeof(struct ether_header)) 
        - sizeof(struct ether_header);
    m->m_data += pad;           
    
    /* set up the buffer pointer to point to the data area */
    pBuff = mtod(m, ushort *);
    
    /* now read the frame byte counter until we have
     * finished reading the frame
     */
    oldFrameCount = 0;
    frameCount = bus_space_read_2( sc->sc_memt, sc->sc_memh, 
				  PKTPG_FRAME_BYTE_COUNT );
    while ( (frameCount != 0) && ( frameCount < MCLBYTES ) )
    {
	for (; oldFrameCount < frameCount; oldFrameCount += 2)
	{
	    *pBuff++ = bus_space_read_2( sc->sc_memt, sc->sc_memh, 
					frameOffset );

	    frameOffset += 2;
	}
	/* read the new count from the chip */
	frameCount = bus_space_read_2( sc->sc_memt, sc->sc_memh, 
				      PKTPG_FRAME_BYTE_COUNT );
    }

    /* update the mbuf counts */
    m->m_len = oldFrameCount;
    m->m_pkthdr.len = oldFrameCount;
    
    /* now check the Rx Event register */
    rxEvent = csReadPacketPage(sc,PKTPG_RX_EVENT);
    
    if ( (rxEvent & RX_EVENT_RX_OK ) != 0 )
    {
        
        /* do an implied skip, it seems to be more reliable 
         * than a forced skip.
         */
	rxEvent = bus_space_read_2( sc->sc_memt, sc->sc_memh, 
				   PKTPG_RX_STATUS );
	rxEvent = bus_space_read_2( sc->sc_memt, sc->sc_memh, 
				   PKTPG_RX_LENGTH );

        /* now read the RX_EVENT register to
         * perform an implied skip.
         */
        rxEvent = csReadPacketPage(sc,PKTPG_RX_EVENT);

	csEtherInput(sc, m);
    }
    else
    {
        m_freem(m);
        pIf->if_ierrors++;
    }
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csStartOutput
**
**  Description      :  This routine is called to start the transmission 
**                      of the packet at the head of the transmit queue.  
**                      This routine is called by the ether_output() 
**                      routine when a new packet is added to the transmit 
**                      queue and this routine is called by 
**                      csTransmitEvent() when a previous packet 
**                      transmission is completed.
**
**                      This should always be called at splimp.
** 
**  Parameters       :  pIf : pointer to devices ifnet
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  void.
** 
**  Side Effects     :  None.
**
*/
void csStartOutput( struct ifnet *pIf )
{
    struct cs_softc *sc;
    struct mbuf *pMbuf;
    struct mbuf *pMbufChain;
    struct ifqueue *pTxQueue;
    ushort BusStatus;
    ushort Length;
    int txLoop = 0;
    int dropout = 0;

    sc = pIf->if_softc;
    pTxQueue = &sc->sc_ethercom.ec_if.if_snd;

    /* check that the interface is up and running */
    if ((pIf->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
    {
        return ;
    }

    /* Don't interrupt a transmission in progress */
    if ( sc->txInProgress ) 
    {
	return;
    }
    
    /* this loop will only run through once if transmission is successful */
    /* While there are packets to transmit and a transmit is not in progress */
    while ( (pTxQueue->ifq_head != NULL) && !(sc->txInProgress) 
	   && !(dropout) )
    {
	IF_DEQUEUE( pTxQueue, pMbufChain );
        
#if NBPFILTER > 0
        /*
         * If BPF is listening on this interface, let it see the packet
         * before we commit it to the wire.
         */
        if (pIf->if_bpf)
            bpf_mtap(pIf->if_bpf, pMbufChain);
#endif
        
        /* Find the total length of the data to transmit */
        Length = 0;
        for ( pMbuf=pMbufChain; pMbuf!=NULL; pMbuf=pMbuf->m_next )
            Length += pMbuf->m_len;

        do {
            /* Request that the transmit be started after all data has
             * been copied 
             *
             * In IO mode must write to the IO port not the packet page
             * address 
             *
             * If this is changed to start transmission after a small
             * amount of data has been copied you tend to get packet
             * missed errors i think because the ISA bus is too slow. Or
             * possibly the copy routine is not streamlined enough.
             */
            csWritePacketPage( sc, PKTPG_TX_CMD,
              cs_xmit_early_table[sc->sc_xe_ent].txcmd );
            csWritePacketPage( sc, PKTPG_TX_LENGTH, Length );

	    /* Adjust early-transmit machinery.
	     */
	    if (--sc->sc_xe_togo == 0) {
		sc->sc_xe_ent = cs_xmit_early_table[sc->sc_xe_ent].better;
		sc->sc_xe_togo =
		    cs_xmit_early_table[sc->sc_xe_ent].better_count;
	    }

            /* Read the BusStatus register which indicates success of
             * the request 
             */
            BusStatus = csReadPacketPage( sc, PKTPG_BUS_ST );

            /* If there was an error in the transmit bid 
             * free the mbuf and go on. This is presuming that 
             * mbuf is corrupt.
             */
            if ( BusStatus & BUS_ST_TX_BID_ERR )
            {
                printf( "%s : Transmit bid error (too big)", 
                    (sc->sc_dev).dv_xname );
                /* Discard the bad mbuf chain */
                m_freem( pMbufChain );
                sc->sc_ethercom.ec_if.if_oerrors++;
                /* Loop up to transmit the next chain */
                txLoop = 0;
            }
            else
            {
                if ( BusStatus & BUS_ST_RDY4TXNOW )
                {
                    KERN_DEBUG(csdebug, CSSTARTTX_DBG_INFO,
                        ("csStartOutput : about to send frame\n"));

                    /* The chip is ready for transmission now */
                    /* Copy the frame to the chip to start
                     * transmission
                     */
                    csCopyTxFrame( sc, pMbufChain );
                    /* Free the mbuf chain */
                    m_freem( pMbufChain );
                    /* Transmission is now in progress */
                    sc->txInProgress = TRUE;
                    txLoop = 0;
                }
                else 
                {
                    /* if we get here we want to try again with
                     * the same mbuf, until the chip lets us
                     * transmit.
                     */
                    txLoop++;
		    if ( txLoop > CS_OUTPUT_LOOP_MAX )
		    {
			/* Free the mbuf chain */
			m_freem( pMbufChain );
			/* Transmission is not in progress */
			sc->txInProgress = FALSE;
			/* Increment the output error count */
			pIf->if_oerrors++;
			/* exit the routine and drop the packet. */
			txLoop = 0;
			dropout = 1;
		    }
                    KERN_DEBUG(csdebug, CSSTARTTX_DBG_INFO,
                        ( "%s : Not ready for transmission now", 
                        (sc->sc_dev).dv_xname ));
                }
            }
        } while (txLoop);
    }
}


/*-------------------------------------------------------------------------*/
/*
**  Function         :  csCopyTxFrame
**
**  Description      :  This routine copies the packet from a chain of 
**                      mbufs to the chip.  When all the data has been 
**                      copied, then the chip automatically begins 
**                      transmitting the data.
**
**                      The reason why this "simple" copy routine is so 
**                      long and complicated is because all reads and writes 
**                      to the chip must be done as 16-bit words. If an mbuf 
**                      has an odd number of bytes, then the last byte must 
**                      be saved and combined with the first byte of the 
**                      next mbuf.
**
**  Parameters       :  struct cs_softc *sc : device softc structure
**                      pMbufChain : mbuf to transmit.
**
**  Implicit Inputs  :  None.
**
**  Implicit Outputs :  None.
**
**  Return Value     :  void.
**
**  Side Effects     :  None.
**
*/
void csCopyTxFrame( struct cs_softc *sc, struct mbuf *m0 )
{
	bus_space_tag_t memt = sc->sc_memt;
	bus_space_handle_t memh = sc->sc_memh;
	struct mbuf *m;
	int len, leftover, frameoff;
	u_int16_t dbuf;
	u_int8_t *p;
#ifdef DIAGNOSTIC
	u_int8_t *lim;
#endif

	/* Initialize frame pointer and data port address */
	frameoff = PKTPG_TX_FRAME;

	/* start out with no leftover data */
	leftover = 0;
	dbuf = 0;
    
	/* Process the chain of mbufs */
	for (m = m0; m != NULL; m = m->m_next) {

		/*
		 * Process all of the data in a single mbuf.
		 */
		p = mtod(m, u_int8_t *);
		len = m->m_len;
#ifdef DIAGNOSTIC
		lim = p + len;
#endif

		while (len > 0) {
			if (leftover) {
				/*
				 * Data left over (from mbuf or realignment).
				 * Buffer the next byte, and write it and
				 * the leftover data out.
				 */
				dbuf |= *p++ << 8;
				len--;

				bus_space_write_2(memt, memh, frameoff,
				    dbuf);
				frameoff += 2;

				leftover = 0;
			} else if ((long)p & 1) {
				/*
				 * Misaligned data.  Buffer the next byte.
				 */
				dbuf = *p++;
				len--;

				leftover = 1;
			} else {
				/*
				 * Aligned data.  This is the case we like.
				 *
				 * Write-region out as much as we can, then
				 * buffer the remaining byte (if any).
				 */
				leftover = len & 1;
				len &= ~1;

				bus_space_write_region_2(memt, memh, frameoff,
				    (u_int16_t *)p, len >> 1);
				p += len;
				frameoff += len;

				if (leftover)
					dbuf = *p++;
				len = 0;
			}
		}
		if (len < 0)
			panic("csCopyTxFrame: negative len");
#ifdef DIAGNOSTIC
		if (p != lim)
			panic("csCopyTxFrame: p != lim");
#endif
	}
	if (leftover)
		bus_space_write_2(memt, memh, frameoff, dbuf);
}
