/*	$NetBSD: if_cs_isa.c,v 1.7 1998/07/15 00:01:17 thorpej Exp $	*/

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

#include "opt_inet.h"

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

#include <machine/cpu.h>
#include <machine/intr.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <arch/arm32/isa/if_csvar.h>

#include "bpfilter.h"
#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#ifdef SHARK
#include <arm32/shark/sequoia.h>
#endif

#ifdef OFW
#include <dev/ofw/openfirm.h>
#endif

/*
 * MACRO DEFINITIONS
 */
#define CS_OUTPUT_LOOP_MAX 100	/* max times round notorious tx loop */
#define DMA_BUFFER_SIZE 16384	/* 16K or 64K */
#define DMA_STATUS_BITS 0x0007	/* bit masks for checking DMA status */
#define DMA_STATUS_OK 0x0004
#define CS_MEM_SIZE 4096	/* 4096 bytes of on chip memory */
#define ETHER_MTU 1518		/* ETHERMTU is defiend in if_ether.h as 1500
				 * ie. without the header. */

/*
 * Macros for reading/writing the packet page register.
 */
#define	CS_READ_PACKET_PAGE_IO(iot, ioh, offset)			\
	(bus_space_write_2((iot), (ioh), PORT_PKTPG_PTR, (offset)),	\
	 bus_space_read_2((iot), (ioh), PORT_PKTPG_DATA))

#define	CS_READ_PACKET_PAGE_MEM(memt, memh, offset)			\
	bus_space_read_2((memt), (memh), (offset))

#define	CS_READ_PACKET_PAGE(sc, offset)					\
	((sc)->inMemoryMode ? CS_READ_PACKET_PAGE_MEM((sc)->sc_memt,	\
	                      (sc)->sc_memh, (offset)) :		\
	 CS_READ_PACKET_PAGE_IO((sc)->sc_iot, (sc)->sc_ioh, (offset)))

#define	CS_WRITE_PACKET_PAGE_IO(iot, ioh, offset, val)			\
do {									\
	bus_space_write_2((iot), (ioh), PORT_PKTPG_PTR, (offset));	\
	bus_space_write_2((iot), (ioh), PORT_PKTPG_DATA, (val));	\
} while (0)

#define	CS_WRITE_PACKET_PAGE_MEM(memt, memh, offset, val)		\
	bus_space_write_2((memt), (memh), (offset), (val))

#define	CS_WRITE_PACKET_PAGE(sc, offset, val)				\
do {									\
	if ((sc)->inMemoryMode)						\
		CS_WRITE_PACKET_PAGE_MEM((sc)->sc_memt, (sc)->sc_memh,	\
		    (offset), (val));					\
	else								\
		CS_WRITE_PACKET_PAGE_IO((sc)->sc_iot, (sc)->sc_ioh,	\
		    (offset), (val));					\
} while (0)

/*
 * the kerndebug macros reserve use of the first and last byte, so we can
 * only use the middle two bytes for flags.
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
int	csProbe __P((struct device *, struct cfdata *, void *));
void	csAttach __P((struct device *, struct device *, void *));
int	csGetUnspecifiedParms __P((struct cs_softc *sc));
int	csValidateParms __P((struct cs_softc *sc));
int	csGetEthernetAddr __P((struct cs_softc *sc));
int	csReadEEPROM __P((struct cs_softc *sc, u_int16_t offset,
	    u_int16_t *pValue));
int	csResetChip __P((struct cs_softc *sc));
void	csShow __P((struct cs_softc *sc, int Zap));
int	csInit __P((struct cs_softc *sc));
void	csReset __P((void *));
int	csIoctl __P((struct ifnet *, u_long cmd, caddr_t data));
void	csInitChip __P((struct cs_softc *sc));
int	csIntr __P((void *arg));
void	csBufferEvent __P((struct cs_softc *sc, u_int16_t BuffEvent));
void	csTransmitEvent __P((struct cs_softc *sc, u_int16_t txEvent));
void	csReceiveEvent __P((struct cs_softc *sc, u_int16_t rxEvent));
void	csEtherInput __P((struct cs_softc *sc, struct mbuf *m));
void	csProcessReceive __P((struct cs_softc *sc));
void	csProcessRxEarly __P((struct cs_softc *sc));
void	csProcessRxDMA __P((struct cs_softc *sc));
void	csStartOutput __P((struct ifnet *ifp));
void	csCopyTxFrame __P((struct cs_softc *sc, struct mbuf *pMbufChain));
void	csSetLadrFilt __P((struct cs_softc *sc, struct ethercom *ec));
u_int16_t csHashIndex __P((char *addr));
void	csCounterEvent __P((struct cs_softc * sc, u_int16_t cntEvent));

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
	u_int16_t       txcmd;
	int             better;
	int             better_count;
	int             worse;
} cs_xmit_early_table[3] = {
	{ TX_CMD_START_381,	0,	INT_MAX,	1, },
	{ TX_CMD_START_1021,	0,	50000,		2, },
	{ TX_CMD_START_ALL,	1,	5000,		2, },
};

struct cfattach cs_ca = {
	sizeof(struct cs_softc), csProbe, csAttach
};

extern struct cfdriver cs_cd;

int csdebug = 0x00000000;	/* debug status, used with kerndebug
				 * macros */

int 
csProbe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_tag_t memt = ia->ia_memt;
	bus_space_handle_t ioh, memh;
	int rv = 0, have_io = 0, have_mem = 0;

	/*
	 * Disallow wildcarded I/O base.
	 */
	if (ia->ia_iobase == ISACF_PORT_DEFAULT)
		return (0);

	/*
	 * Map the I/O and mem (if specified) space.
	 */
	if (bus_space_map(ia->ia_iot, ia->ia_iobase, CS8900_IOSIZE, 0, &ioh))
		goto out;
	have_io = 1;

	if (ia->ia_maddr != ISACF_IOMEM_DEFAULT &&
	    bus_space_map(ia->ia_memt, ia->ia_maddr, CS_MEM_SIZE, 0, &memh))
		goto out;
	have_mem = 1;

	/* Verify that it's a Crystal product. */
	if (CS_READ_PACKET_PAGE_IO(iot, ioh, PKTPG_EISA_NUM) !=
	    EISA_NUM_CRYSTAL)
		goto out;

	/*
	 * Verify that it's a supported chip.
	 */
	switch (CS_READ_PACKET_PAGE_IO(iot, ioh, PKTPG_PRODUCT_ID) &
	    PROD_ID_MASK) {
	case PROD_ID_CS8900:
#ifdef notyet			/* XXX why not? */
	case PROD_ID_CS8920:
	case PROD_ID_CS892X:
#endif
		rv = 1;
	}

 out:
	if (have_io)
		bus_space_unmap(iot, ioh, CS8900_IOSIZE);
	if (have_mem)
		bus_space_unmap(memt, memh, CS_MEM_SIZE);

	if (rv) {
		ia->ia_iosize = CS8900_IOSIZE;
		ia->ia_msize = CS_MEM_SIZE;
	}
	return (rv);
}

void 
csAttach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cs_softc *sc = (struct cs_softc *) self;
	struct isa_attach_args *ia = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	const char *chipname;
	u_int16_t reg;

	sc->sc_ic = ia->ia_ic;
	sc->sc_iot = ia->ia_iot;
	sc->sc_memt = ia->ia_memt;

	sc->sc_iobase = ia->ia_iobase;
	sc->sc_drq = ia->ia_drq;
	sc->sc_int = ia->ia_irq;

	/*
	 * Map the device.
	 */
	if (bus_space_map(sc->sc_iot, ia->ia_iobase, ia->ia_iosize,
	    0, &sc->sc_ioh)) {
		printf("\n%s: unable to map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}

	reg = CS_READ_PACKET_PAGE_IO(sc->sc_iot, sc->sc_ioh, PKTPG_PRODUCT_ID);

	switch (reg & PROD_ID_MASK) {
	case PROD_ID_CS8900:
		chipname = "CS8900";
		break;
	case PROD_ID_CS8920:
		chipname = "CS8920";
		break;
	case PROD_ID_CS892X:
		chipname = "CS892x";
		break;
	default:
		panic("csAttach: impossible");
	}

	printf(": %s, rev. %d\n", chipname, (reg & PROD_REV_MASK) >> 8);

	/*
	 * XXX We only support the memory-mapped mode of operation right
	 * XXX now.  (??? --thorpej)
	 */
	if (ia->ia_maddr == ISACF_IOMEM_DEFAULT ||
	    ia->ia_msize != CS_MEM_SIZE) {
		printf("%s: only memory-mapped mode is supported\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	if (bus_space_map(sc->sc_memt, ia->ia_maddr, ia->ia_msize,
	    0, &sc->sc_memh)) {
		printf("%s: unable to map memory space\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * The default EEPROM configuration is to use programmed IO. If we
	 * want to use mapped memory we must manually set the chips
	 * registers.
	 *
	 * I think we always want to use the external latch logic.
	 */
	sc->pPacketPagePhys = ia->ia_maddr;
	sc->configFlags |= CFGFLG_MEM_MODE | CFGFLG_USE_SA | CFGFLG_IOCHRDY;

	/*
	 * the first thing to do is check that the mbuf cluster size is
	 * greater than the MTU for an ethernet frame. The code depends on
	 * this and to port this to a OS where this was not the case would
	 * not be straightforward.
	 */
	if (MCLBYTES < ETHER_MTU) {
		printf("%s: MCLBYTES too small for Ethernet frame\n",
		    sc->sc_dev.dv_xname);
		return;
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
	ifp->if_watchdog = NULL;	/* no watchdog at this stage */
	ifp->if_flags = IFF_SIMPLEX | IFF_NOTRAILERS |
	    IFF_BROADCAST | IFF_MULTICAST;

#ifdef OFW
	/* Dig MAC address out of the firmware. */
	if (OF_getprop((int) ia->ia_aux, "mac-address", sc->sc_enaddr,
	    sizeof(sc->sc_enaddr)) < 0) {
		printf("%s: unable to get Ethernet address from OpenFirmware\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->mediaType = MEDIA_10BASET;	/* XXX get from OpenFirmware */
#else
	/* Get parameters, which were not specified, from the EEPROM */
	if (csGetUnspecifiedParms(sc) == CS_ERROR) {
		printf("%s: unable to get settings from EEPROM\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* Verify that parameters are valid */
	if (csValidateParms(sc) == CS_ERROR) {
		printf("%s: invalid EEPROM settings\n", sc->sc_dev.dv_xname);
		return;
	}

	/* Get and store the Ethernet address */
	if (csGetEthernetAddr(sc) == CS_ERROR) {
		printf("%s: unable to read Ethernet address\n",
		    sc->sc_dev.dv_xname);
		return;
	}
#endif /* OFW */

	/* XXX Print default media. */
	printf("%s: address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(sc->sc_enaddr));

	if (sc->sc_drq == ISACF_DRQ_DEFAULT)
		printf("%s: DMA channel unspecified, not using DMA\n",
		    sc->sc_dev.dv_xname);
	else if (sc->sc_drq < 5 || sc->sc_drq > 7)
		printf("%s: invalid DMA channel, not using DMA\n",
		    sc->sc_dev.dv_xname);
	else {
		bus_addr_t dma_addr;

		if (isa_dmamap_create(sc->sc_ic, sc->sc_drq,
		    CS8900_DMA_BUFFER_SIZE, BUS_DMA_NOWAIT) != 0) {
			printf("%s: unable to create ISA DMA map\n",
			    sc->sc_dev.dv_xname);
			goto after_dma_block;
		}
		if (isa_dmamem_alloc(sc->sc_ic, sc->sc_drq,
		    CS8900_DMA_BUFFER_SIZE, &dma_addr, BUS_DMA_NOWAIT) != 0) {
			printf("%s: unable to allocate DMA buffer\n",
			    sc->sc_dev.dv_xname);
			goto after_dma_block;
		}
		if (isa_dmamem_map(sc->sc_ic, sc->sc_drq, dma_addr,
		    CS8900_DMA_BUFFER_SIZE, &sc->dmaBase,
		       BUS_DMA_NOWAIT | BUS_DMA_COHERENT /* XXX */ ) != 0) {
			printf("%s: unable to map DMA buffer\n",
			    sc->sc_dev.dv_xname);
			isa_dmamem_free(sc->sc_ic, sc->sc_drq, dma_addr,
			    CS8900_DMA_BUFFER_SIZE);
			goto after_dma_block;
		}

		sc->dmaMemSize = CS8900_DMA_BUFFER_SIZE;
		sc->configFlags |= CFGFLG_DMA_MODE;
	}
after_dma_block:

	sc->sc_sh = shutdownhook_establish(csReset, sc);
	if (sc->sc_sh == NULL) {
		printf("%s: unable to establish shutdownhook\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* XXX get IST from front-end. */
	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_LEVEL,
	    IPL_NET, csIntr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);

#if NBPFILTER > 0
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	/* Reset the chip */
	if (csResetChip(sc) == CS_ERROR)
		printf("%s: reset failed\n", sc->sc_dev.dv_xname);
}

int 
csGetUnspecifiedParms(sc)
	struct cs_softc *sc;
{
	u_int16_t selfStatus;
	u_int16_t isaConfig;
	u_int16_t memBase;
	u_int16_t adapterConfig;
	u_int16_t xmitCtl;

	/* If all of these parameters were specified */
	if (sc->configFlags != 0 && sc->pPacketPagePhys != MADDRUNK
	    && sc->sc_int != 0 && sc->mediaType != 0) {
		return CS_OK;
	}

	/* Verify that the EEPROM is present and OK */
	selfStatus = CS_READ_PACKET_PAGE(sc, PKTPG_SELF_ST);
	if (!((selfStatus & SELF_ST_EEP_PRES) &&
	    (selfStatus & SELF_ST_EEP_OK))) {
		printf("%s: EEPROM is missing or bad\n", sc->sc_dev.dv_xname);
		return CS_ERROR;
	}

	/* Get ISA configuration from the EEPROM */
	if (csReadEEPROM(sc, EEPROM_ISA_CFG, &isaConfig) == CS_ERROR)
		return CS_ERROR;

	/* Get adapter configuration from the EEPROM */
	if (csReadEEPROM(sc, EEPROM_ADPTR_CFG, &adapterConfig) == CS_ERROR)
		return CS_ERROR;

	/* Get transmission control from the EEPROM */
	if (csReadEEPROM(sc, EEPROM_XMIT_CTL, &xmitCtl) == CS_ERROR)
		return CS_ERROR;

	/* If the configuration flags were not specified */
	if (sc->configFlags == 0) {
		/* Copy the memory mode flag */
		if (isaConfig & ISA_CFG_MEM_MODE)
			sc->configFlags |= CFGFLG_MEM_MODE;

		/* Copy the USE_SA flag */
		if (isaConfig & ISA_CFG_USE_SA)
			sc->configFlags |= CFGFLG_USE_SA;

		/* Copy the IO Channel Ready flag */
		if (isaConfig & ISA_CFG_IOCHRDY)
			sc->configFlags |= CFGFLG_IOCHRDY;

		/* Copy the DC/DC Polarity flag */
		if (adapterConfig & ADPTR_CFG_DCDC_POL)
			sc->configFlags |= CFGFLG_DCDC_POL;

		/* Copy the Full Duplex flag */
		if (xmitCtl & XMIT_CTL_FDX)
			sc->configFlags |= CFGFLG_FDX;
	}

	/* If the PacketPage pointer was not specified */
	if (sc->pPacketPagePhys == MADDRUNK) {
		/* If memory mode is enabled */
		if (sc->configFlags & CFGFLG_MEM_MODE) {
			/* Get the memory base address from EEPROM */
			if (csReadEEPROM(sc, EEPROM_MEM_BASE,
			    &memBase) == CS_ERROR)
				return CS_ERROR;

			memBase &= MEM_BASE_MASK;	/* Clear unused bits */

			/* Setup the PacketPage pointer */
			sc->pPacketPagePhys = (((u_long) memBase) << 8);
		}
	}

	/* If the interrupt level was not specified */
	if (sc->sc_int == 0) {
		/* Get the interrupt level from the ISA config */
		sc->sc_int = isaConfig & ISA_CFG_IRQ_MASK;
		if (sc->sc_int == 3)
			sc->sc_int = 5;
		else
			sc->sc_int += 10;
	}

	/* If the media type was not specified */
	if (sc->mediaType == 0) {
		switch (adapterConfig & ADPTR_CFG_MEDIA) {
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

int 
csValidateParms(sc)
	struct cs_softc *sc;
{
	int memAddr;

	memAddr = sc->pPacketPagePhys;

	if ((memAddr & 0x000FFF) != 0) {
		printf("%s: memory address not on 4k boundary\n",
		    sc->sc_dev.dv_xname);
		return CS_ERROR;
	}

	if (!(sc->sc_int == 5 || sc->sc_int == 10 || sc->sc_int == 11 ||
	      sc->sc_int == 12)) {
		printf("%s: invalid IRQ\n", sc->sc_dev.dv_xname);
		return CS_ERROR;
	}

	if (!(sc->mediaType == MEDIA_AUI || sc->mediaType == MEDIA_10BASE2 ||
	      sc->mediaType == MEDIA_10BASET)) {
		printf("%s: invalud media type\n", sc->sc_dev.dv_xname);
		return CS_ERROR;
	}

	return CS_OK;
}

int 
csGetEthernetAddr(sc)
	struct cs_softc *sc;
{
	u_int16_t selfStatus;
	pia pIA;

	/* Setup pointer of where to store the Ethernet address */
	pIA = (pia) sc->sc_enaddr;

	/* Verify that the EEPROM is present and OK */
	selfStatus = CS_READ_PACKET_PAGE(sc, PKTPG_SELF_ST);
	if (!((selfStatus & SELF_ST_EEP_PRES) &&
	    (selfStatus & SELF_ST_EEP_OK))) {
		printf("%s: EEPROM is missing or bad\n", sc->sc_dev.dv_xname);
		return CS_ERROR;
	}

	/* Get Ethernet address from the EEPROM */
	/* XXX this will likely lose on a big-endian machine. -- cgd */
	if (csReadEEPROM(sc, EEPROM_IND_ADDR_H, &pIA->word[0]) == CS_ERROR)
		return CS_ERROR;
	if (csReadEEPROM(sc, EEPROM_IND_ADDR_M, &pIA->word[1]) == CS_ERROR)
		return CS_ERROR;
	if (csReadEEPROM(sc, EEPROM_IND_ADDR_L, &pIA->word[2]) == CS_ERROR)
		return CS_ERROR;

	return CS_OK;
}

int 
csResetChip(sc)
	struct cs_softc *sc;
{
	int intState;
	int x;

	/* Disable interrupts at the CPU so reset command is atomic */
	intState = splimp();

	/*
	 * We are now resetting the chip
	 * 
	 * A spurious interrupt is generated by the chip when it is reset. This
	 * variable informs the interrupt handler to ignore this interrupt.
	 */
	sc->resetting = TRUE;

	/* Issue a reset command to the chip */
	CS_WRITE_PACKET_PAGE(sc, PKTPG_SELF_CTL, SELF_CTL_RESET);

	/* Re-enable interrupts at the CPU */
	splx(intState);

	/* The chip is always in IO mode after a reset */
	sc->inMemoryMode = FALSE;

	/* If transmission was in progress, it is not now */
	sc->txInProgress = FALSE;

	/*
	 * there was a delay(125); here, but it seems uneccesary 125 usec is
	 * 1/8000 of a second, not 1/8 of a second. the data sheet advises
	 * 1/10 of a second here, but the SI_BUSY and INIT_DONE loops below
	 * should be sufficient.
	 */

	/* Transition SBHE to switch chip from 8-bit to 16-bit */
	bus_space_read_1(sc->sc_iot, sc->sc_iobase, PORT_PKTPG_PTR);
	bus_space_read_1(sc->sc_iot, sc->sc_iobase, PORT_PKTPG_PTR + 1);
	bus_space_read_1(sc->sc_iot, sc->sc_iobase, PORT_PKTPG_PTR);
	bus_space_read_1(sc->sc_iot, sc->sc_iobase, PORT_PKTPG_PTR + 1);

	/* Wait until the EEPROM is not busy */
	for (x = 0; x < MAXLOOP; x++) {
		if (!(CS_READ_PACKET_PAGE(sc, PKTPG_SELF_ST) & SELF_ST_SI_BUSY))
			break;
	}

	if (x == MAXLOOP)
		return CS_ERROR;

	/* Wait until initialization is done */
	for (x = 0; x < MAXLOOP; x++) {
		if (CS_READ_PACKET_PAGE(sc, PKTPG_SELF_ST) & SELF_ST_INIT_DONE)
			break;
	}

	if (x == MAXLOOP)
		return CS_ERROR;

	/* Reset is no longer in progress */
	sc->resetting = FALSE;

	return CS_OK;
}

int 
csReadEEPROM(sc, offset, pValue)
	struct cs_softc *sc;
	u_int16_t offset, *pValue;
{
	int x;

	/* Ensure that the EEPROM is not busy */
	for (x = 0; x < MAXLOOP; x++) {
		if (!(CS_READ_PACKET_PAGE(sc, PKTPG_SELF_ST) & SELF_ST_SI_BUSY))
			break;
	}

	if (x == MAXLOOP) {
		printf("%s: unable to read from EEPROM\n", sc->sc_dev.dv_xname);
		return CS_ERROR;
	}

	/* Issue the command to read the offset within the EEPROM */
	CS_WRITE_PACKET_PAGE(sc, PKTPG_EEPROM_CMD, offset | EEPROM_CMD_READ);

	/* Wait until the command is completed */
	for (x = 0; x < MAXLOOP; x++) {
		if (!(CS_READ_PACKET_PAGE(sc, PKTPG_SELF_ST) & SELF_ST_SI_BUSY))
			break;
	}

	if (x == MAXLOOP) {
		printf("%s: unable to read from EEPROM\n", sc->sc_dev.dv_xname);
		return CS_ERROR;
	}

	/* Get the EEPROM data from the EEPROM Data register */
	*pValue = CS_READ_PACKET_PAGE(sc, PKTPG_EEPROM_DATA);

	return CS_OK;
}

void 
csInitChip(sc)
	struct cs_softc *sc;
{
	u_int16_t busCtl;
	u_int16_t selfCtl;
	pia pIA;
	u_int16_t isaId;

	/* Disable reception and transmission of frames */
	CS_WRITE_PACKET_PAGE(sc, PKTPG_LINE_CTL,
	    CS_READ_PACKET_PAGE(sc, PKTPG_LINE_CTL) &
	    ~LINE_CTL_RX_ON & ~LINE_CTL_TX_ON);

	/* Disable interrupt at the chip */
	CS_WRITE_PACKET_PAGE(sc, PKTPG_BUS_CTL,
	    CS_READ_PACKET_PAGE(sc, PKTPG_BUS_CTL) & ~BUS_CTL_INT_ENBL);

	/* If IOCHRDY is enabled then clear the bit in the busCtl register */
	busCtl = CS_READ_PACKET_PAGE(sc, PKTPG_BUS_CTL);
	if (sc->configFlags & CFGFLG_IOCHRDY) {
		CS_WRITE_PACKET_PAGE(sc, PKTPG_BUS_CTL,
		    busCtl & ~BUS_CTL_IOCHRDY);
	} else {
		CS_WRITE_PACKET_PAGE(sc, PKTPG_BUS_CTL,
		    busCtl | BUS_CTL_IOCHRDY);
	}

	/* Set the Line Control register to match the media type */
	if (sc->mediaType == MEDIA_10BASET) {
		CS_WRITE_PACKET_PAGE(sc, PKTPG_LINE_CTL, LINE_CTL_10BASET);
	} else {
		CS_WRITE_PACKET_PAGE(sc, PKTPG_LINE_CTL, LINE_CTL_AUI_ONLY);
	}

	/*
	 * Set the BSTATUS/HC1 pin to be used as HC1.  HC1 is used to
	 * enable the DC/DC converter
	 */
	selfCtl = SELF_CTL_HC1E;

	/* If the media type is 10Base2 */
	if (sc->mediaType == MEDIA_10BASE2) {
		/*
		 * Enable the DC/DC converter if it has a low enable.
		 */
		if ((sc->configFlags & CFGFLG_DCDC_POL) == 0)
			/*
			 * Set the HCB1 bit, which causes the HC1 pin to go
			 * low.
			 */
			selfCtl |= SELF_CTL_HCB1;
	} else { /* Media type is 10BaseT or AUI */
		/*
		 * Disable the DC/DC converter if it has a high enable.
		 */
		if ((sc->configFlags & CFGFLG_DCDC_POL) != 0) {
			/*
			 * Set the HCB1 bit, which causes the HC1 pin to go
			 * low.
			 */
			selfCtl |= SELF_CTL_HCB1;
		}
	}
	CS_WRITE_PACKET_PAGE(sc, PKTPG_SELF_CTL, selfCtl);

	/* If media type is 10BaseT */
	if (sc->mediaType == MEDIA_10BASET) {
		/*
		 * If full duplex mode then set the FDX bit in TestCtl
		 * register
		 */
		if (sc->configFlags & CFGFLG_FDX) {
			CS_WRITE_PACKET_PAGE(sc, PKTPG_TEST_CTL, TEST_CTL_FDX);
		}
	}

	/* RX_CTL set in csSetLadrFilt(), below */

	/* enable all transmission interrupts */
	CS_WRITE_PACKET_PAGE(sc, PKTPG_TX_CFG, TX_CFG_ALL_IE);

	/* Accept all receive interrupts */
	CS_WRITE_PACKET_PAGE(sc, PKTPG_RX_CFG, RX_CFG_ALL_IE);

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
	CS_WRITE_PACKET_PAGE(sc, PKTPG_BUF_CFG, BUF_CFG_TX_UNDR_IE |
			  BUF_CFG_RX_DMA_IE);

	if (sc->configFlags & CFGFLG_DMA_MODE) {
		/*
		 * First we program the DMA controller and ensure the memory
		 * buffer is valid. If it isn't then we just go on without
		 * DMA.
		 */
		if (isa_dmastart(sc->sc_ic, sc->sc_drq, sc->dmaBase,
		    sc->dmaMemSize, NULL, DMAMODE_READ | DMAMODE_LOOPDEMAND,
		    BUS_DMA_NOWAIT)) {
			/* XXX XXX XXX */
			panic("%s: unable to start DMA\n", sc->sc_dev.dv_xname);
		}
		sc->dma_offset = sc->dmaBase;

		/* interrupt when a DMA'd frame is received */
		CS_WRITE_PACKET_PAGE(sc, PKTPG_RX_CFG,
		    RX_CFG_ALL_IE | RX_CFG_RX_DMA_ONLY);

		/*
		 * set the DMA burst bit so we don't tie up the bus for too
		 * long.
		 */
		if (sc->dmaMemSize == 16384) {
			CS_WRITE_PACKET_PAGE(sc, PKTPG_BUS_CTL,
			    ((CS_READ_PACKET_PAGE(sc, PKTPG_BUS_CTL) &
			     ~BUS_CTL_DMA_SIZE) | BUS_CTL_DMA_BURST));
		} else { /* use 64K */
			CS_WRITE_PACKET_PAGE(sc, PKTPG_BUS_CTL,
			    CS_READ_PACKET_PAGE(sc, PKTPG_BUS_CTL) |
			     BUS_CTL_DMA_SIZE | BUS_CTL_DMA_BURST);
		}

		CS_WRITE_PACKET_PAGE(sc, PKTPG_DMA_CHANNEL, sc->sc_drq - 5);
	}

	/* If memory mode is enabled */
	if (sc->configFlags & CFGFLG_MEM_MODE) {
		/* If external logic is present for address decoding */
		if (CS_READ_PACKET_PAGE(sc, PKTPG_SELF_ST) & SELF_ST_EL_PRES) {
			/*
			 * Program the external logic to decode address bits
			 * SA20-SA23
			 */
			CS_WRITE_PACKET_PAGE(sc, PKTPG_EEPROM_CMD,
			    ((sc->pPacketPagePhys & 0xffffff) >> 20) |
			    EEPROM_CMD_ELSEL);
		}

		/*
		 * Write the packet page base physical address to the memory
		 * base register.
		 */
		CS_WRITE_PACKET_PAGE(sc, PKTPG_MEM_BASE,
		    sc->pPacketPagePhys & 0xFFFF);
		CS_WRITE_PACKET_PAGE(sc, PKTPG_MEM_BASE + 2,
		    sc->pPacketPagePhys >> 16);
		busCtl = BUS_CTL_MEM_MODE;

		/* tell the chip to read the addresses off the SA pins */
		if (sc->configFlags & CFGFLG_USE_SA) {
			busCtl |= BUS_CTL_USE_SA;
		}
		CS_WRITE_PACKET_PAGE(sc, PKTPG_BUS_CTL,
		    CS_READ_PACKET_PAGE(sc, PKTPG_BUS_CTL) | busCtl);

		/* We are in memory mode now! */
		sc->inMemoryMode = TRUE;

		/*
		 * wait here (10ms) for the chip to swap over. this is the
		 * maximum time that this could take.
		 */
		delay(10000);

		/* Verify that we can read from the chip */
		isaId = CS_READ_PACKET_PAGE(sc, PKTPG_EISA_NUM);

		/*
		 * As a last minute sanity check before actually using mapped
		 * memory we verify that we can read the isa number from the
		 * chip in memory mode.
		 */
		if (isaId != EISA_NUM_CRYSTAL) {
			printf("%s: failed to enable memory mode\n",
			    sc->sc_dev.dv_xname);
			sc->inMemoryMode = FALSE;
		} else {
			/*
			 * we are in memory mode so if we aren't using DMA,
			 * then program the chip to interrupt early.
			 */
			if ((sc->configFlags & CFGFLG_DMA_MODE) == 0) {
				CS_WRITE_PACKET_PAGE(sc, PKTPG_BUF_CFG,
				    BUF_CFG_RX_DEST_IE |
				    BUF_CFG_RX_MISS_OVER_IE |
				    BUF_CFG_TX_COL_OVER_IE);
			}
		}

	}

	/* Put Ethernet address into the Individual Address register */
	pIA = (pia) sc->sc_enaddr;
	CS_WRITE_PACKET_PAGE(sc, PKTPG_IND_ADDR, pIA->word[0]);
	CS_WRITE_PACKET_PAGE(sc, PKTPG_IND_ADDR + 2, pIA->word[1]);
	CS_WRITE_PACKET_PAGE(sc, PKTPG_IND_ADDR + 4, pIA->word[2]);

	/* Set the interrupt level in the chip */
	if (sc->sc_int == 5) {
		CS_WRITE_PACKET_PAGE(sc, PKTPG_INT_NUM, 3);
	} else {
		CS_WRITE_PACKET_PAGE(sc, PKTPG_INT_NUM, (sc->sc_int) - 10);
	}

	/* write the multicast mask to the address filter register */
	csSetLadrFilt(sc, &sc->sc_ethercom);

	/* Enable reception and transmission of frames */
	CS_WRITE_PACKET_PAGE(sc, PKTPG_LINE_CTL,
	    CS_READ_PACKET_PAGE(sc, PKTPG_LINE_CTL) |
	    LINE_CTL_RX_ON | LINE_CTL_TX_ON);

	/* Enable interrupt at the chip */
	CS_WRITE_PACKET_PAGE(sc, PKTPG_BUS_CTL,
	    CS_READ_PACKET_PAGE(sc, PKTPG_BUS_CTL) | BUS_CTL_INT_ENBL);
}

int 
csInit(sc)
	struct cs_softc *sc;
{
	int intState;
	int error = CS_OK;

	intState = splimp();

	/* Mark the interface as down */
	sc->sc_ethercom.ec_if.if_flags &= ~(IFF_UP | IFF_RUNNING);

#ifdef CS_DEBUG
	/* Enable debugging */
	sc->sc_ethercom.ec_if.if_flags |= IFF_DEBUG;
#endif

	/* Reset the chip */
	if ((error = csResetChip(sc)) == CS_OK) {
		/* Initialize the chip */
		csInitChip(sc);

		/* Mark the interface as up and running */
		sc->sc_ethercom.ec_if.if_flags |= (IFF_UP | IFF_RUNNING);
		sc->sc_ethercom.ec_if.if_flags &= ~IFF_OACTIVE;
		sc->sc_ethercom.ec_if.if_timer = 0;
	} else {
		printf("%s: unable to reset chip\n", sc->sc_dev.dv_xname);
	}

	splx(intState);
	return error;
}

void 
csSetLadrFilt(sc, ec)
	struct cs_softc *sc;
	struct ethercom *ec;
{
	struct ifnet *ifp = &ec->ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	u_int16_t af[4];
	u_int16_t port, mask, index;

	/*
         * Set up multicast address filter by passing all multicast addresses
         * through a crc generator, and then using the high order 6 bits as an
         * index into the 64 bit logical address filter.  The high order bit
         * selects the word, while the rest of the bits select the bit within
         * the word.
         */
	if (ifp->if_flags & IFF_PROMISC) {
		/* accept all valid frames. */
		CS_WRITE_PACKET_PAGE(sc, PKTPG_RX_CTL,
		    RX_CTL_PROMISC_A | RX_CTL_RX_OK_A |
		    RX_CTL_IND_A | RX_CTL_BCAST_A | RX_CTL_MCAST_A);
		ifp->if_flags |= IFF_ALLMULTI;
		return;
	}

	/*
	 * accept frames if a. crc valid, b. individual address match c.
	 * broadcast address,and d. multicast addresses matched in the hash
	 * filter
	 */
	CS_WRITE_PACKET_PAGE(sc, PKTPG_RX_CTL,
	    RX_CTL_RX_OK_A | RX_CTL_IND_A | RX_CTL_BCAST_A | RX_CTL_MCAST_A);


	/*
	 * start off with all multicast flag clear, set it if we need to
	 * later, otherwise we will leave it.
	 */
	ifp->if_flags &= ~IFF_ALLMULTI;

	af[0] = af[1] = af[2] = af[3] = 0x0000;
	ETHER_FIRST_MULTI(step, ec, enm);
	/*
	 * Loop through all the multicast addresses unless we get a range of
	 * addresses, in which case we will just accept all packets.
	 * Justification for this is given in the next comment.
	 */
	while ((enm != NULL) && ((ifp->if_flags | IFF_ALLMULTI) == 0)) {
		if (bcmp(enm->enm_addrlo, enm->enm_addrhi,
		    sizeof enm->enm_addrlo)) {
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
		} else {
			/*
	                 * we have got an individual address so just set that
	                 * bit.
	                 */
			index = csHashIndex(enm->enm_addrlo);

			/* Set the bit the Logical address filter. */
			port = (u_int16_t) (2 * ((index / 16)));
			mask = (u_int16_t) (1 << (15 - (index % 16)));
			af[port] |= mask;

			ETHER_NEXT_MULTI(step, enm);
		}
	}

	/* now program the chip with the addresses */
	CS_WRITE_PACKET_PAGE(sc, PKTPG_LOG_ADDR + 2, af[0]);
	CS_WRITE_PACKET_PAGE(sc, PKTPG_LOG_ADDR + 2, af[1]);
	CS_WRITE_PACKET_PAGE(sc, PKTPG_LOG_ADDR + 2, af[2]);
	CS_WRITE_PACKET_PAGE(sc, PKTPG_LOG_ADDR + 2, af[3]);
	return;
}

u_int16_t
csHashIndex(addr)
	char *addr;
{
	u_int POLY = 0x04c11db6;
	u_int crc_value = 0xffffffff;
	u_int16_t hash_code = 0;
	int i;
	u_int current_bit;
	char current_byte = *addr;
	u_int cur_crc_high;

	for (i = 0; i < 6; i++) {
		current_byte = *addr;
		addr++;

		for (current_bit = 8; current_bit; current_bit--) {
			cur_crc_high = crc_value >> 31;
			crc_value <<= 1;
			if (cur_crc_high ^ (current_byte & 0x01)) {
				crc_value ^= POLY;
				crc_value |= 0x00000001;
			}
			current_byte >>= 1;
		}
	}

	/*
         * The hash code is the 6 least significant bits of the CRC
         * in the reverse order: CRC[0] = hash[5],CRC[1] = hash[4],etc.
         */
	for (i = 0; i < 6; i++) {
		hash_code = (u_int16_t) ((hash_code << 1) |
		    (u_int16_t) ((crc_value >> i) & 0x00000001));
	}

	return hash_code;
}

void 
csReset(arg)
	void *arg;
{
	struct cs_softc *sc = arg;

	/* Mark the interface as down */
	sc->sc_ethercom.ec_if.if_flags &= ~IFF_RUNNING;

	/* Reset the chip */
	csResetChip(sc);
}

int 
csIoctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct cs_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *) data;
	struct ifreq *ifr = (struct ifreq *) data;
	int state;
	int result;

	state = splimp();

	result = 0;		/* only set if something goes wrong */

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			csInit(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif
		default:
			csInit(sc);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running,
			 * then stop it.
			 */
			csResetChip(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
			   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped,
			 * start it.
			 */
			csInit(sc);
		} else {
			/*
			 * Reset the interface to pick up any changes in
			 * any other flags that affect hardware registers.
			 */
			csInit(sc);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		result = (cmd == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_ethercom) :
		    ether_delmulti(ifr, &sc->sc_ethercom);

		if (result == ENETRESET) {
			/*
	                 * Multicast list has changed; set the hardware filter
	                 * accordingly.
	                 */
			csInit(sc);
			result = 0;
		}
		break;

	default:
		result = EINVAL;
		break;
	}

	splx(state);

	return result;
}

int 
csIntr(arg)
	void *arg;
{
	struct cs_softc *sc = arg;
	u_int16_t Event;

	/* Ignore any interrupts that happen while the chip is being reset */
	if (sc->resetting) {
		printf("%s: csIntr: reset in progress\n",
		    sc->sc_dev.dv_xname);
		return 1;
	}

	/* Read an event from the Interrupt Status Queue */
	Event = bus_space_read_2(sc->sc_memt, sc->sc_memh, PKTPG_ISQ);

	if ((Event & REG_NUM_MASK) == 0)
		return 0;	/* not ours */

	/* Process all the events in the Interrupt Status Queue */
	while (Event != 0) {
		/* Dispatch to an event handler based on the register number */
		switch (Event & REG_NUM_MASK) {
		case REG_NUM_RX_EVENT:
			csReceiveEvent(sc, Event);
			break;
		case REG_NUM_TX_EVENT:
			csTransmitEvent(sc, Event);
			break;
		case REG_NUM_BUF_EVENT:
			csBufferEvent(sc, Event);
			break;
		case REG_NUM_TX_COL:
		case REG_NUM_RX_MISS:
			csCounterEvent(sc, Event);
			break;
		default:
			printf("%s: unknown interrupt event 0x%x\n",
			    sc->sc_dev.dv_xname, Event);
			break;
		}

		/* Read another event from the Interrupt Status Queue */
		Event = bus_space_read_2(sc->sc_memt, sc->sc_memh, PKTPG_ISQ);
	}

	/* have handled the interupt */
	return 1;
}

void 
csCounterEvent(sc, cntEvent)
	struct cs_softc *sc;
	u_int16_t cntEvent;
{
	struct ifnet *pIf;
	u_int16_t errorCount;

	pIf = &sc->sc_ethercom.ec_if;

	switch (cntEvent & REG_NUM_MASK) {
	case REG_NUM_TX_COL:
		/*
		 * the count should be read before an overflow occurs.
		 */
		errorCount = CS_READ_PACKET_PAGE(sc, PKTPG_TX_COL);
		/*
		 * the tramsit event routine always checks the number of
		 * collisions for any packet so we don't increment any
		 * counters here, as they should already have been
		 * considered.
		 */
		break;
	case REG_NUM_RX_MISS:
		/*
		 * the count should be read before an overflow occurs.
		 */
		errorCount = CS_READ_PACKET_PAGE(sc, PKTPG_RX_MISS);
		/*
		 * Increment the input error count, the first 6bits are the
		 * register id.
		 */
		pIf->if_ierrors += ((errorCount & 0xffC0) >> 6);
		break;
	default:
		/* do nothing */
		break;
	}
}

void 
csBufferEvent(sc, bufEvent)
	struct cs_softc *sc;
	u_int16_t bufEvent;
{
	struct ifnet *pIf;

	pIf = &sc->sc_ethercom.ec_if;

	/*
	 * multiple events can be in the buffer event register at one time so
	 * a standard switch statement will not suffice, here every event
	 * must be checked.
	 */

	/*
	 * if 128 bits have been rxed by the time we get here, the dest event
	 * will be cleared and 128 event will be set.
	 */
	if ((bufEvent & (BUF_EVENT_RX_DEST | BUF_EVENT_RX_128)) != 0) {
		csProcessRxEarly(sc);
	}

	if (bufEvent & BUF_EVENT_RX_DMA) {
		/* process the receive data */
		csProcessRxDMA(sc);
	}

	if (bufEvent & BUF_EVENT_TX_UNDR) {
#if 0
		/*
		 * This can happen occasionally, and it's not worth worrying
		 * about.
		 */
		printf("%s: transmit underrun (%d -> %d)\n",
		    sc->sc_dev.dv_xname, sc->sc_xe_ent,
		    cs_xmit_early_table[sc->sc_xe_ent].worse);
#endif
		sc->sc_xe_ent = cs_xmit_early_table[sc->sc_xe_ent].worse;
		sc->sc_xe_togo =
		    cs_xmit_early_table[sc->sc_xe_ent].better_count;

		/* had an underrun, transmit is finished */
		sc->txInProgress = FALSE;
	}

	if (bufEvent & BUF_EVENT_SW_INT) {
		printf("%s: software initiated interrupt\n",
		    sc->sc_dev.dv_xname);
	}
}

void 
csTransmitEvent(sc, txEvent)
	struct cs_softc *sc;
	u_int16_t txEvent;
{
	struct ifnet *pIf = &sc->sc_ethercom.ec_if;

	/* If there were any errors transmitting this frame */
	if (txEvent & (TX_EVENT_LOSS_CRS | TX_EVENT_SQE_ERR | TX_EVENT_OUT_WIN |
		       TX_EVENT_JABBER | TX_EVENT_16_COLL)) {
		/* Increment the output error count */
		pIf->if_oerrors++;

		/* If debugging is enabled then log error messages */
		if (pIf->if_flags & IFF_DEBUG) {
			if (txEvent & TX_EVENT_LOSS_CRS) {
				printf("%s: lost carrier\n",
				    sc->sc_dev.dv_xname);
			}
			if (txEvent & TX_EVENT_SQE_ERR) {
				printf("%s: SQE error\n",
				    sc->sc_dev.dv_xname);
			}
			if (txEvent & TX_EVENT_OUT_WIN) {
				printf("%s: out-of-window collision\n",
				    sc->sc_dev.dv_xname);
			}
			if (txEvent & TX_EVENT_JABBER) {
				printf("%s: jabber\n", sc->sc_dev.dv_xname);
			}
			if (txEvent & TX_EVENT_16_COLL) {
				printf("%s: 16 collisions\n",
				    sc->sc_dev.dv_xname);
			}
		}
	}
#ifdef SHARK
	else {
		ledNetActive();
	}
#endif

	/* Add the number of collisions for this frame */
	if (txEvent & TX_EVENT_16_COLL) {
		pIf->if_collisions += 16;
	} else {
		pIf->if_collisions += ((txEvent & TX_EVENT_COLL_MASK) >> 11);
	}

	pIf->if_opackets++;

	/* Transmission is no longer in progress */
	sc->txInProgress = FALSE;

	/* If there is more to transmit */
	if (pIf->if_snd.ifq_head != NULL) {
		/* Start the next transmission */
		csStartOutput(pIf);
	}
}

void 
csReceiveEvent(sc, rxEvent)
	struct cs_softc *sc;
	u_int16_t rxEvent;
{
	struct ifnet *pIf = &sc->sc_ethercom.ec_if;

	/* If the frame was not received OK */
	if (!(rxEvent & RX_EVENT_RX_OK)) {
		/* Increment the input error count */
		pIf->if_ierrors++;

		/* If debugging is enabled then log error messages */
		if (pIf->if_flags & IFF_DEBUG) {
			/* If an error bit is set */
			if (rxEvent != REG_NUM_RX_EVENT) {
				if (rxEvent & RX_EVENT_RUNT) {
					printf("%s: runt\n",
					    sc->sc_dev.dv_xname);
				}
				if (rxEvent & RX_EVENT_X_DATA) {
					printf("%s: extra data\n",
					    sc->sc_dev.dv_xname);
				}
				if (rxEvent & RX_EVENT_CRC_ERR) {
					if (rxEvent & RX_EVENT_DRIBBLE) {
						printf("%s: alignment error\n",
						    sc->sc_dev.dv_xname);
					} else {
						printf("%s: CRC error\n",
						    sc->sc_dev.dv_xname);
					}
				} else {
					if (rxEvent & RX_EVENT_DRIBBLE) {
						printf("%s: dribble bits\n",
						    sc->sc_dev.dv_xname);
					}
				}

				/*
				 * Must read the length of all received
				 * frames
				 */
				CS_READ_PACKET_PAGE(sc, PKTPG_RX_LENGTH);

				/* Skip the received frame */
				CS_WRITE_PACKET_PAGE(sc, PKTPG_RX_CFG,
					CS_READ_PACKET_PAGE(sc, PKTPG_RX_CFG) |
						  RX_CFG_SKIP);
			} else {
				printf("%s: implied skip\n",
				    sc->sc_dev.dv_xname);
			}
		}
	} else {
		/*
		 * process the received frame and pass it up to the upper
		 * layers.
		 */
		csProcessReceive(sc);
	}
}

void
csEtherInput(sc, m)
	struct cs_softc *sc;
	struct mbuf *m;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_header *eh;

	ifp->if_ipackets++;

	/*
	 * the first thing in the mbuf is the ethernet header. we need to
	 * pass this header to the upper layers as a structure, so cast the
	 * start of the mbuf, and adjust the mbuf to point to the end of the
	 * ethernet header, ie the ethernet packet data.
	 */
	eh = mtod(m, struct ether_header *);

#if NBPFILTER > 0
	/*
	 * Check if there's a BPF listener on this interface.
	 * If so, hand off the raw packet to BPF.
	 */
	if (ifp->if_bpf) {
		bpf_mtap(ifp->if_bpf, m);

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no BPF listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 */
		if ((ifp->if_flags & IFF_PROMISC) &&
		    (eh->ether_dhost[0] & 1) == 0 &&	/* !mcast and !bcast */
		    bcmp(eh->ether_dhost, sc->sc_enaddr,
			 sizeof(eh->ether_dhost)) != 0) {
			m_freem(m);
			return;
		}
	}
#endif

	/*
	 * Pass the packet up, with the ether header sort-of removed. ie.
	 * adjust the data pointer to point to the data.
	 */
	m_adj(m, sizeof(struct ether_header));
	ether_input(ifp, eh, m);
}

void 
csProcessReceive(sc)
	struct cs_softc *sc;
{
	struct ifnet *pIf;
	struct mbuf *m;
	int totlen;
	int len;
	volatile u_int16_t *pBuff, *pBuffLimit;
	int pad;
	unsigned int frameOffset;

#ifdef SHARK
	ledNetActive();
#endif

	pIf = &sc->sc_ethercom.ec_if;

	/* Initialize the frame offset */
	frameOffset = PKTPG_RX_LENGTH;

	/* Get the length of the received frame */
	totlen = bus_space_read_2(sc->sc_memt, sc->sc_memh, frameOffset);
	frameOffset += 2;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0) {
		printf("%s: csProcessReceive: unable to allocate mbuf\n",
		    sc->sc_dev.dv_xname);
		pIf->if_ierrors++;
		/*
		 * couldn't allocate an mbuf so things are not good, may as
		 * well drop the packet I think.
		 * 
		 * have already read the length so we should be right to skip
		 * the packet.
		 */
		CS_WRITE_PACKET_PAGE(sc, PKTPG_RX_CFG,
		    CS_READ_PACKET_PAGE(sc, PKTPG_RX_CFG) | RX_CFG_SKIP);
		return;
	}
	m->m_pkthdr.rcvif = pIf;
	m->m_pkthdr.len = totlen;

	/*
	 * save processing by always using a mbuf cluster, guarenteed to fit
	 * packet, on i386 NetBSD anyway.
	 */
	MCLGET(m, M_DONTWAIT);
	if (m->m_flags & M_EXT) {
		len = MCLBYTES;
	} else {
		/* couldn't allocate an mbuf cluster */
		printf("%s: csProcessReceive: unable to allocate a cluster\n",
		    sc->sc_dev.dv_xname);
		m_freem(m);

		/* skip the received frame */
		CS_WRITE_PACKET_PAGE(sc, PKTPG_RX_CFG,
		    CS_READ_PACKET_PAGE(sc, PKTPG_RX_CFG) | RX_CFG_SKIP);
		return;
	}

	/* align ip header on word boundary for ipintr */
	pad = ALIGN(sizeof(struct ether_header)) - sizeof(struct ether_header);
	m->m_data += pad;

	m->m_len = len = min(totlen, len);
	pBuff = mtod(m, u_int16_t *);
	pBuffLimit = pBuff + (len + 1) / 2;	/* don't want to go over */

	/* now read the data from the chip */
	while (pBuff < pBuffLimit) {
		*pBuff++ = bus_space_read_2(sc->sc_memt, sc->sc_memh,
		    frameOffset);
		frameOffset += 2;
	}

	csEtherInput(sc, m);
}

void 
csProcessRxDMA(sc)
	struct cs_softc *sc;
{
	struct ifnet *pIf;
	u_int16_t num_dma_frames;
	u_int16_t pkt_length;
	u_int16_t status;
	u_int to_copy;
	char *dma_mem_ptr;
	struct mbuf *m;
	u_char *pBuff;
	int pad;

	/* initialise the pointers */
	pIf = &sc->sc_ethercom.ec_if;

	/* Read the number of frames DMAed. */
	num_dma_frames = CS_READ_PACKET_PAGE(sc, PKTPG_DMA_FRAME_COUNT);
	num_dma_frames &= (u_int16_t) (0x0fff);

	/*
	 * Loop till number of DMA frames ready to read is zero. After
	 * reading the frame out of memory we must check if any have been
	 * received while we were processing
	 */
	while (num_dma_frames != 0) {
		dma_mem_ptr = sc->dma_offset;

		/*
		 * process all of the dma frames in memory
		 * 
		 * This loop relies on the dma_mem_ptr variable being set to the
		 * next frames start address.
		 */
		for (; num_dma_frames > 0; num_dma_frames--) {

			/*
			 * Get the length and status of the packet. Only the
			 * status is guarenteed to be at dma_mem_ptr, ie need
			 * to check for wraparound before reading the length
			 */
			status = *((unsigned short *) dma_mem_ptr)++;
			if (dma_mem_ptr > (sc->dmaBase + sc->dmaMemSize)) {
				dma_mem_ptr = sc->dmaBase;
			}
			pkt_length = *((unsigned short *) dma_mem_ptr)++;

			/* Do some sanity checks on the length and status. */
			if ((pkt_length > ETHER_MTU) ||
			    ((status & DMA_STATUS_BITS) != DMA_STATUS_OK)) {
				/*
				 * the SCO driver kills the adapter in this
				 * situation
				 */
				/*
				 * should increment the error count and reset
				 * the dma operation.
				 */
				printf("%s: csProcessRxDMA: DMA buffer out of sync about to reset\n",
				    sc->sc_dev.dv_xname);
				pIf->if_ierrors++;

				/* skip the rest of the DMA buffer */
				isa_dmaabort(sc->sc_ic, sc->sc_drq);

				/* now reset the chip and reinitialise */
				csInit(sc);
				return;
			}
			/* Check the status of the received packet. */
			if (status & RX_EVENT_RX_OK) {
				/* get a new mbuf */
				MGETHDR(m, M_DONTWAIT, MT_DATA);
				if (m == 0) {
					printf("%s: csProcessRxDMA: unable to allocate mbuf\n",
					    sc->sc_dev.dv_xname);
					pIf->if_ierrors++;
					/*
					 * couldn't allocate an mbuf so
					 * things are not good, may as well
					 * drop all the packets I think.
					 */
					CS_READ_PACKET_PAGE(sc,
					    PKTPG_DMA_FRAME_COUNT);

					/* now reset DMA operation */
					isa_dmaabort(sc->sc_ic, sc->sc_drq);

					/*
					 * now reset the chip and
					 * reinitialise
					 */
					csInit(sc);
					return;
				}
				/*
				 * save processing by always using a mbuf
				 * cluster, guarenteed to fit packet
				 */
				MCLGET(m, M_DONTWAIT);
				if ((m->m_flags & M_EXT) == 0) {
					/* couldn't allocate an mbuf cluster */
					printf("%s: csProcessRxDMA: unable to allocate a cluster\n",
					    sc->sc_dev.dv_xname);
					m_freem(m);

					/* skip the frame */
					CS_READ_PACKET_PAGE(sc, PKTPG_DMA_FRAME_COUNT);
					isa_dmaabort(sc->sc_ic, sc->sc_drq);

					/*
					 * now reset the chip and
					 * reinitialise
					 */
					csInit(sc);
					return;
				}
				m->m_pkthdr.rcvif = pIf;
				m->m_pkthdr.len = pkt_length;
				m->m_len = pkt_length;

				/*
				 * align ip header on word boundary for
				 * ipintr
				 */
				pad = ALIGN(sizeof(struct ether_header)) -
				    sizeof(struct ether_header);
				m->m_data += pad;

				/*
				 * set up the buffer pointer to point to the
				 * data area
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
				if ((dma_mem_ptr + pkt_length) <
				    (sc->dmaBase + sc->dmaMemSize)) {
					/*
					 * No wrap around. Copy the frame
					 * header
					 */
					bcopy(dma_mem_ptr, pBuff, pkt_length);
					dma_mem_ptr += pkt_length;
				} else {
					to_copy = (u_int)
					    ((sc->dmaBase + sc->dmaMemSize) -
					    dma_mem_ptr);

					/* Copy the first half of the frame. */
					bcopy(dma_mem_ptr, pBuff, to_copy);
					pBuff += to_copy;

					/*
		                         * Rest of the frame is to be read
		                         * from the first byte of the DMA
		                         * memory.
		                         */
					/*
					 * Get the number of bytes leftout in
					 * the frame.
					 */
					to_copy = pkt_length - to_copy;

					dma_mem_ptr = sc->dmaBase;

					/* Copy rest of the frame. */
					bcopy(dma_mem_ptr, pBuff, to_copy);
					dma_mem_ptr += to_copy;
				}

				csEtherInput(sc, m);
			}
			/* (status & RX_OK) */ 
			else {
				/* the frame was not received OK */
				/* Increment the input error count */
				pIf->if_ierrors++;

				/*
				 * If debugging is enabled then log error
				 * messages
				 */
				if (pIf->if_flags & IFF_DEBUG) {
					/* If an error bit is set */
					if (status != REG_NUM_RX_EVENT) {
						if (status & RX_EVENT_RUNT) {
							printf("%s: runt\n",
							  sc->sc_dev.dv_xname);
						}
						if (status & RX_EVENT_X_DATA) {
							printf("%s: extra data\n",
							  sc->sc_dev.dv_xname);
						}
						if (status & RX_EVENT_CRC_ERR) {
							if (status & RX_EVENT_DRIBBLE) {
								printf("%s: alignment error\n",
								  sc->sc_dev.dv_xname);
							} else {
								printf("%s: CRC error\n",
								  sc->sc_dev.dv_xname);
							}
						} else {
							if (status & RX_EVENT_DRIBBLE) {
								printf("%s: dribble bits\n",
								  sc->sc_dev.dv_xname);
							}
						}
					}
				}
			}
			/*
			 * now update the current frame pointer. the
			 * dma_mem_ptr should point to the next packet to be
			 * received, without the alignment considerations.
			 * 
			 * The cs8900 pads all frames to start at the next 32bit
			 * aligned addres. hence we need to pad our offset
			 * pointer.
			 */
			dma_mem_ptr += 3;
			dma_mem_ptr = (char *)
			    ((long) dma_mem_ptr & 0xfffffffc);
			if (dma_mem_ptr < (sc->dmaBase + sc->dmaMemSize)) {
				sc->dma_offset = dma_mem_ptr;
			} else {
				dma_mem_ptr = sc->dma_offset = sc->dmaBase;
			}
		} /* for all frames */
		/* Read the number of frames DMAed again. */
		num_dma_frames = CS_READ_PACKET_PAGE(sc, PKTPG_DMA_FRAME_COUNT);
		num_dma_frames &= (u_int16_t) (0x0fff);
	} /* while there are frames left */
}

void 
csProcessRxEarly(sc)
	struct cs_softc *sc;
{
	struct ifnet *pIf;
	struct mbuf *m;
	u_int16_t frameCount, oldFrameCount;
	u_int16_t rxEvent;
	u_int16_t *pBuff;
	int pad;
	unsigned int frameOffset;


	pIf = &sc->sc_ethercom.ec_if;

	/* Initialize the frame offset */
	frameOffset = PKTPG_RX_FRAME;
	frameCount = 0;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0) {
		printf("%s: csProcessRxEarly: unable to allocate mbuf\n",
		    sc->sc_dev.dv_xname);
		pIf->if_ierrors++;
		/*
		 * couldn't allocate an mbuf so things are not good, may as
		 * well drop the packet I think.
		 * 
		 * have already read the length so we should be right to skip
		 * the packet.
		 */
		CS_WRITE_PACKET_PAGE(sc, PKTPG_RX_CFG,
		    CS_READ_PACKET_PAGE(sc, PKTPG_RX_CFG) | RX_CFG_SKIP);
		return;
	}
	m->m_pkthdr.rcvif = pIf;
	/*
	 * save processing by always using a mbuf cluster, guarenteed to fit
	 * packet
	 */
	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		/* couldn't allocate an mbuf cluster */
		printf("%s: csProcessRxEarly: unable to allocate a cluster\n",
		    sc->sc_dev.dv_xname);
		m_freem(m);
		/* skip the frame */
		CS_WRITE_PACKET_PAGE(sc, PKTPG_RX_CFG,
		    CS_READ_PACKET_PAGE(sc, PKTPG_RX_CFG) | RX_CFG_SKIP);
		return;
	}

	/* align ip header on word boundary for ipintr */
	pad = ALIGN(sizeof(struct ether_header)) - sizeof(struct ether_header);
	m->m_data += pad;

	/* set up the buffer pointer to point to the data area */
	pBuff = mtod(m, u_int16_t *);

	/*
	 * now read the frame byte counter until we have finished reading the
	 * frame
	 */
	oldFrameCount = 0;
	frameCount = bus_space_read_2(sc->sc_memt, sc->sc_memh,
	    PKTPG_FRAME_BYTE_COUNT);
	while ((frameCount != 0) && (frameCount < MCLBYTES)) {
		for (; oldFrameCount < frameCount; oldFrameCount += 2) {
			*pBuff++ = bus_space_read_2(sc->sc_memt, sc->sc_memh,
			    frameOffset);
			frameOffset += 2;
		}

		/* read the new count from the chip */
		frameCount = bus_space_read_2(sc->sc_memt, sc->sc_memh,
		    PKTPG_FRAME_BYTE_COUNT);
	}

	/* update the mbuf counts */
	m->m_len = oldFrameCount;
	m->m_pkthdr.len = oldFrameCount;

	/* now check the Rx Event register */
	rxEvent = CS_READ_PACKET_PAGE(sc, PKTPG_RX_EVENT);

	if ((rxEvent & RX_EVENT_RX_OK) != 0) {
		/*
		 * do an implied skip, it seems to be more reliable than a
		 * forced skip.
		 */
		rxEvent = bus_space_read_2(sc->sc_memt, sc->sc_memh,
		    PKTPG_RX_STATUS);
		rxEvent = bus_space_read_2(sc->sc_memt, sc->sc_memh,
		    PKTPG_RX_LENGTH);

		/*
		 * now read the RX_EVENT register to perform an implied skip.
		 */
		rxEvent = CS_READ_PACKET_PAGE(sc, PKTPG_RX_EVENT);

		csEtherInput(sc, m);
	} else {
		m_freem(m);
		pIf->if_ierrors++;
	}
}

void 
csStartOutput(pIf)
	struct ifnet *pIf;
{
	struct cs_softc *sc;
	struct mbuf *pMbuf;
	struct mbuf *pMbufChain;
	struct ifqueue *pTxQueue;
	u_int16_t BusStatus;
	u_int16_t Length;
	int txLoop = 0;
	int dropout = 0;

	sc = pIf->if_softc;
	pTxQueue = &sc->sc_ethercom.ec_if.if_snd;

	/* check that the interface is up and running */
	if ((pIf->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING) {
		return;
	}

	/* Don't interrupt a transmission in progress */
	if (sc->txInProgress) {
		return;
	}

	/* this loop will only run through once if transmission is successful */
	/*
	 * While there are packets to transmit and a transmit is not in
	 * progress
	 */
	while ((pTxQueue->ifq_head != NULL) && !(sc->txInProgress) &&
	    !(dropout)) {
		IF_DEQUEUE(pTxQueue, pMbufChain);

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
		for (pMbuf = pMbufChain; pMbuf != NULL; pMbuf = pMbuf->m_next)
			Length += pMbuf->m_len;

		do {
			/*
			 * Request that the transmit be started after all
			 * data has been copied
			 * 
			 * In IO mode must write to the IO port not the packet
			 * page address
			 * 
			 * If this is changed to start transmission after a
			 * small amount of data has been copied you tend to
			 * get packet missed errors i think because the ISA
			 * bus is too slow. Or possibly the copy routine is
			 * not streamlined enough.
			 */
			CS_WRITE_PACKET_PAGE(sc, PKTPG_TX_CMD,
			    cs_xmit_early_table[sc->sc_xe_ent].txcmd);
			CS_WRITE_PACKET_PAGE(sc, PKTPG_TX_LENGTH, Length);

			/*
			 * Adjust early-transmit machinery.
			 */
			if (--sc->sc_xe_togo == 0) {
				sc->sc_xe_ent =
				    cs_xmit_early_table[sc->sc_xe_ent].better;
				sc->sc_xe_togo =
			    cs_xmit_early_table[sc->sc_xe_ent].better_count;
			}
			/*
			 * Read the BusStatus register which indicates
			 * success of the request
			 */
			BusStatus = CS_READ_PACKET_PAGE(sc, PKTPG_BUS_ST);

			/*
			 * If there was an error in the transmit bid free the
			 * mbuf and go on. This is presuming that mbuf is
			 * corrupt.
			 */
			if (BusStatus & BUS_ST_TX_BID_ERR) {
				printf("%s: transmit bid error (too big)",
				    sc->sc_dev.dv_xname);

				/* Discard the bad mbuf chain */
				m_freem(pMbufChain);
				sc->sc_ethercom.ec_if.if_oerrors++;

				/* Loop up to transmit the next chain */
				txLoop = 0;
			} else {
				if (BusStatus & BUS_ST_RDY4TXNOW) {
					/*
					 * The chip is ready for transmission
					 * now
					 */
					/*
					 * Copy the frame to the chip to
					 * start transmission
					 */
					csCopyTxFrame(sc, pMbufChain);

					/* Free the mbuf chain */
					m_freem(pMbufChain);

					/* Transmission is now in progress */
					sc->txInProgress = TRUE;
					txLoop = 0;
				} else {
					/*
					 * if we get here we want to try
					 * again with the same mbuf, until
					 * the chip lets us transmit.
					 */
					txLoop++;
					if (txLoop > CS_OUTPUT_LOOP_MAX) {
						/* Free the mbuf chain */
						m_freem(pMbufChain);
						/*
						 * Transmission is not in
						 * progress
						 */
						sc->txInProgress = FALSE;
						/*
						 * Increment the output error
						 * count
						 */
						pIf->if_oerrors++;
						/*
						 * exit the routine and drop
						 * the packet.
						 */
						txLoop = 0;
						dropout = 1;
					}
				}
			}
		} while (txLoop);
	}
}

void 
csCopyTxFrame(sc, m0)
	struct cs_softc *sc;
	struct mbuf *m0;
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
				bus_space_write_2(memt, memh, frameoff, dbuf);
				frameoff += 2;
				leftover = 0;
			} else if ((long) p & 1) {
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
				    (u_int16_t *) p, len >> 1);
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
