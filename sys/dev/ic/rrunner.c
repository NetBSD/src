/*	$NetBSD: rrunner.c,v 1.5 1998/07/05 06:49:12 jonathan Exp $	*/

/*
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code contributed to The NetBSD Foundation by Kevin M. Lahey
 * of the Numerical Aerospace Simulation Facility, NASA Ames Research 
 * Center.
 *
 * Partially based on a HIPPI driver written by Essential Communications 
 * Corporation.  Thanks to Jason Thorpe, Matt Jacob, and Fred Templin
 * for invaluable advice and encouragement!
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_inet.h"
#include "opt_ns.h"

#include "assym.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>

#include <net/if_hippi.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/rrunnerreg.h>
#include <dev/ic/rrunnervar.h>

/*
#define ESH_PRINTF
*/

/* Autoconfig defintion of driver back-end */
extern struct cfdriver esh_cd;

struct esh_softc *esh_softc_debug;  /* for gdb */
u_int32_t write_len;
u_int32_t max_write_len;

void eshinit __P((struct esh_softc *));
int  eshioctl __P((struct ifnet *, u_long, caddr_t));
void eshread __P((struct esh_softc *, u_int16_t, int));
void eshreset __P((struct esh_softc *));
void eshstart __P((struct ifnet *));
static int eshstatus __P((struct esh_softc *));
void eshstop __P((struct esh_softc *));
static void eshshutdown __P((void *));
void eshwatchdog __P((struct ifnet *));

static void esh_send __P((struct esh_softc *));
static void esh_send2 __P((struct esh_softc *));
static void esh_send_cmd __P((struct esh_softc *, 
			      u_int8_t, u_int8_t, u_int8_t));
static void esh_write_addr __P((bus_space_tag_t, bus_space_handle_t,
				bus_addr_t, bus_addr_t));
static void esh_init_snap_ring __P((struct esh_softc *));
static void esh_fill_snap_ring __P((struct esh_softc *));
static void esh_reset_runcode __P((struct esh_softc *));
static void eshstart_cleanup __P((struct esh_softc *, u_int16_t, int));
static u_int32_t esh_read_eeprom __P((struct esh_softc *, u_int32_t));
static int esh_write_eeprom __P((struct esh_softc *, 
				 u_int32_t, u_int32_t));
static void esh_print_ring __P((struct esh_softc *, char *, 
				struct esh_ring_ctl *, int));
static void esh_dma_sync __P((struct esh_softc *, void *,
			      int, int, int, int, int, int));

#ifdef ESH_PRINTF
static int esh_check __P((struct esh_softc *));
#endif


/*
 * Back-end attach and configure.  Allocate DMA space and initialize
 * all structures.
 */

void
eshconfig(sc)
	struct esh_softc *sc;
{
	struct ifnet *ifp = &sc->sc_if;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int32_t misc_host_ctl;
	u_int32_t misc_local_ctl;
	u_int32_t header_format;
	u_int32_t ula_tmp;
	bus_size_t size;
	int rseg;
	int error;
	int i;

	sc->sc_flags = 0;

	/* 
	 * Allocate and divvy up some host side memory that can hold
	 * data structures that will be DMA'ed over to the NIC
	 */

	sc->sc_dma_size = sizeof(struct rr_gen_info) + 
		sizeof(struct rr_ring_ctl) * RR_ULP_COUNT +
		max(sizeof(struct rr_descr) * RR_SEND_RING_SIZE,
		    sizeof(struct rr2_descr) * RR2_SEND_RING_SIZE) + 
		max(sizeof(struct rr_descr) * RR_SNAP_RECV_RING_SIZE,
		    sizeof(struct rr2_descr) * RR2_SNAP_RECV_RING_SIZE) +
		sizeof(struct rr_event) * RR_EVENT_RING_SIZE;
	
	/* Configure the interface */

	error = bus_dmamem_alloc(sc->sc_dmat, sc->sc_dma_size, 
				 0, RR_DMA_BOUNDRY, &sc->sc_dmaseg, 1, 
				 &rseg, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s:  couldn't allocate space for host-side"
		       "data structures\n", sc->sc_dev.dv_xname);
		return;
	}

	if (rseg > 1) {
		printf("%s:  contiguous memory not available\n",
		       sc->sc_dev.dv_xname);
		goto bad_dmamem_map;
	}	

	error = bus_dmamem_map(sc->sc_dmat, &sc->sc_dmaseg, rseg, 
			       sc->sc_dma_size, &sc->sc_dma_addr,
			       BUS_DMA_NOWAIT | BUS_DMA_COHERENT);
	if (error) {
		printf("%s:  couldn't map memory for host-side structures\n",
		       sc->sc_dev.dv_xname);
		goto bad_dmamem_map;
	}
    
	if (bus_dmamap_create(sc->sc_dmat, sc->sc_dma_size, 
			      1, sc->sc_dma_size, RR_DMA_BOUNDRY, 
			      BUS_DMA_ALLOCNOW | BUS_DMA_NOWAIT, 
			      &sc->sc_dma)) {
		printf("%s: couldn't create DMA map\n", sc->sc_dev.dv_xname);
		goto bad_dmamap_create;
	}

    
	if (bus_dmamap_load(sc->sc_dmat, sc->sc_dma, sc->sc_dma_addr, 
			    sc->sc_dma_size, NULL, BUS_DMA_NOWAIT)) {
		printf("%s: couldn't load DMA map\n", sc->sc_dev.dv_xname);
		goto bad_dmamap_load;
	}
    
	bzero(sc->sc_dma_addr, sc->sc_dma_size);

	sc->sc_gen_info_dma = sc->sc_dma->dm_segs->ds_addr;
	sc->sc_gen_info = (struct rr_gen_info *) sc->sc_dma_addr;
	size = sizeof(struct rr_gen_info);

	sc->sc_recv_ring_table_dma = sc->sc_dma->dm_segs->ds_addr + size;
	sc->sc_recv_ring_table = 
		(struct rr_ring_ctl *) (sc->sc_dma_addr + size);
	size += sizeof(struct rr_ring_ctl) * RR_ULP_COUNT;

	sc->sc_send_ring_dma = sc->sc_dma->dm_segs->ds_addr + size;
	sc->sc_send_ring = (struct rr_descr *) (sc->sc_dma_addr + size);
	sc->sc2_send_ring = (struct rr2_descr *) (sc->sc_dma_addr + size);
	size += max(sizeof(struct rr_descr) * RR_SEND_RING_SIZE,
		    sizeof(struct rr2_descr) * RR2_SEND_RING_SIZE);

	sc->sc_snap_recv_ring_dma = sc->sc_dma->dm_segs->ds_addr + size;
	sc->sc_snap_recv_ring = (struct rr_descr *) (sc->sc_dma_addr + size);
	sc->sc2_snap_recv_ring = (struct rr2_descr *) (sc->sc_dma_addr + size);
	size += max(sizeof(struct rr_descr) * RR_SNAP_RECV_RING_SIZE,
		    sizeof(struct rr2_descr) * RR2_SNAP_RECV_RING_SIZE);

	sc->sc_event_ring_dma = sc->sc_dma->dm_segs->ds_addr + size;
	sc->sc_event_ring = (struct rr_event *) (sc->sc_dma_addr + size);
	size += sizeof(struct rr_event) * RR_EVENT_RING_SIZE;

#ifdef DIAGNOSTIC
	if (size > sc->sc_dmaseg.ds_len) {
		printf("%s:  bogus size calculation\n", sc->sc_dev.dv_xname);
		goto bad_other;
	}
#endif

	/* 
	 * Allocate DMA maps for transfers.  We do this here and now so we
	 * won't have to wait for them in the middle of sending something.
	 * 
	 * XXX:  Note that we only allocate space for one segment at a time.
	 * With IP, we will be mapping mbufs (bus_dmamap_load_mbuf was
	 * not finished at the time of this writing), so we'll only need
	 * room for a cluster at a time.  We'll need to change this later,
	 * but allocating 256 or more segments for each dmamap seemed
	 * a little over the top!
	 */

	for (i = 0; i < max(RR_SEND_RING_SIZE, RR2_SEND_RING_SIZE); i++)
		if (bus_dmamap_create(sc->sc_dmat, RR_DMA_MAX, 1,
				      RR_DMA_MAX, RR_DMA_BOUNDRY, 
				      BUS_DMA_ALLOCNOW | BUS_DMA_NOWAIT, 
				      &sc->sc_send.ec_dma[i])) { 
			printf("%s:  failed bus_dmamap_create\n", 
			       sc->sc_dev.dv_xname);
			for (i--; i >= 0; i--)
				bus_dmamap_destroy(sc->sc_dmat, 
						   sc->sc_send.ec_dma[i]);
			goto bad_other;
		}
	sc->sc_send.ec_descr = sc->sc_send_ring;
	sc->sc_send.ec2_descr = sc->sc2_send_ring;
    
	for (i = 0; i < RR_MAX_SNAP_RECV_RING_SIZE; i++)
		if (bus_dmamap_create(sc->sc_dmat, RR_DMA_MAX, 1, RR_DMA_MAX, 
				      RR_DMA_BOUNDRY, 
				      BUS_DMA_ALLOCNOW | BUS_DMA_NOWAIT, 
				      &sc->sc_snap_recv.ec_dma[i])) {
			printf("%s:  failed bus_dmamap_create\n", 
			       sc->sc_dev.dv_xname);
			for (i--; i >= 0; i--)
				bus_dmamap_destroy(sc->sc_dmat, 
						   sc->sc_snap_recv.ec_dma[i]);
			goto bad_ring_dmamap_create;
		}

	/*
	 * If this is a coldboot, the NIC RunCode should be operational.
	 * If it is a warmboot, it may or may not be operational.
	 * Just to be sure, we'll stop the RunCode and reset everything.
	 */

	/* Halt the processor (preserve NO_SWAP, if set) */

	misc_host_ctl = bus_space_read_4(iot, ioh, RR_MISC_HOST_CTL);
	bus_space_write_4(iot, ioh, RR_MISC_HOST_CTL, 
			  (misc_host_ctl & RR_MH_NO_SWAP) | RR_MH_HALT_PROC);

	/* Make the EEPROM readable */

	misc_local_ctl = bus_space_read_4(iot, ioh, RR_MISC_LOCAL_CTL);
	bus_space_write_4(iot, ioh, RR_MISC_LOCAL_CTL,
	    misc_local_ctl & ~(RR_LC_FAST_PROM | RR_LC_ADD_SRAM | 
			       RR_LC_PARITY_ON));

	/* Extract interesting information from the EEPROM: */

	header_format = esh_read_eeprom(sc, RR_EE_HEADER_FORMAT);
	if (header_format != RR_EE_HEADER_FORMAT_MAGIC) {
		printf("%s:  bogus EEPROM header format value %x\n",
		       sc->sc_dev.dv_xname, header_format);
		goto bad_other;
	}

	/* 
	 * As it is now, the runcode version in the EEPROM doesn't
	 * reflect the actual runcode version number.  That is only
	 * available once the runcode starts up.  We should probably
	 * change the firmware update code to modify this value,
	 * but Essential itself doesn't do it right now.
	 */

	sc->sc_sram_size = 4 * esh_read_eeprom(sc, RR_EE_SRAM_SIZE);
	sc->sc_runcode_start = esh_read_eeprom(sc, RR_EE_RUNCODE_START);
	sc->sc_runcode_version = esh_read_eeprom(sc, RR_EE_RUNCODE_VERSION);

	sc->sc_pci_latency = esh_read_eeprom(sc, RR_EE_PCI_LATENCY);
	sc->sc_pci_lat_gnt = esh_read_eeprom(sc, RR_EE_PCI_LAT_GNT);

	/* General tuning values */

	sc->sc_tune.rt_mode_and_status = 
		esh_read_eeprom(sc, RR_EE_MODE_AND_STATUS);
	sc->sc_tune.rt_conn_retry_count = 
		esh_read_eeprom(sc, RR_EE_CONN_RETRY_COUNT);
	sc->sc_tune.rt_conn_retry_timer = 
		esh_read_eeprom(sc, RR_EE_CONN_RETRY_TIMER);
	sc->sc_tune.rt_conn_timeout = 
		esh_read_eeprom(sc, RR_EE_CONN_TIMEOUT);
	sc->sc_tune.rt_interrupt_timer = 
		esh_read_eeprom(sc, RR_EE_INTERRUPT_TIMER);
	sc->sc_tune.rt_tx_timeout = 
		esh_read_eeprom(sc, RR_EE_TX_TIMEOUT);
	sc->sc_tune.rt_rx_timeout = 
		esh_read_eeprom(sc, RR_EE_RX_TIMEOUT);
	sc->sc_tune.rt_stats_timer = 
		esh_read_eeprom(sc, RR_EE_STATS_TIMER);
	sc->sc_tune.rt_stats_timer = ESH_STATS_TIMER_DEFAULT;

	/* DMA tuning values */

	sc->sc_tune.rt_pci_state = 
		esh_read_eeprom(sc, RR_EE_PCI_STATE);
	sc->sc_tune.rt_dma_write_state = 
		esh_read_eeprom(sc, RR_EE_DMA_WRITE_STATE);
	sc->sc_tune.rt_dma_read_state = 
		esh_read_eeprom(sc, RR_EE_DMA_READ_STATE);
	sc->sc_tune.rt_driver_param = 
		esh_read_eeprom(sc, RR_EE_DRIVER_PARAM);

	/* 
	 * Snag the ULA.  The first two bytes are reserved.
	 * We don't really use it immediately, but it would be good to
	 * have for building IPv6 addresses, etc.
	 */

	ula_tmp = esh_read_eeprom(sc, RR_EE_ULA_HI);
	sc->sc_ula[0] = (ula_tmp >> 8) & 0xff;
	sc->sc_ula[1] = ula_tmp & 0xff;

	ula_tmp = esh_read_eeprom(sc, RR_EE_ULA_LO);
	sc->sc_ula[2] = (ula_tmp >> 24) & 0xff;
	sc->sc_ula[3] = (ula_tmp >> 16) & 0xff;
	sc->sc_ula[4] = (ula_tmp >> 8) & 0xff;
	sc->sc_ula[5] = ula_tmp & 0xff;

	/* Reset EEPROM readability */

	bus_space_write_4(iot, ioh, RR_MISC_LOCAL_CTL, misc_local_ctl);

	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = eshstart;
	ifp->if_ioctl = eshioctl;
	ifp->if_watchdog = eshwatchdog;
	ifp->if_flags = IFF_SIMPLEX | IFF_NOTRAILERS | IFF_NOARP;

	if_attach(ifp);
	hippi_ifattach(ifp, sc->sc_ula);

	/* Some more configuration? */

	sc->sc_misaligned_bufs = sc->sc_bad_lens = 0;

#if NBPFILTER > 0
	bpfattach(&sc->sc_if.if_bpf, ifp, DLT_HIPPI,
		  sizeof(struct hippi_header));
#endif
	return;

bad_ring_dmamap_create:
	for (i = 0; i < RR_MAX_SEND_RING_SIZE; i++)
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_send.ec_dma[i]);
bad_other:
	bus_dmamap_unload(sc->sc_dmat, sc->sc_dma);
bad_dmamap_load:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_dma);
bad_dmamap_create:
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_dma_addr, sc->sc_dma_size);
bad_dmamem_map:
	bus_dmamem_free(sc->sc_dmat, &sc->sc_dmaseg, rseg);
	return;
}


/*
 * Bring device up.
 *
 * Assume that the on-board processor has already been stopped, 
 * the rings have been cleared of valid buffers, and everything 
 * is pretty much as it was when the system started.
 *
 * Stop the processor (just for good measure), clear the SRAM, 
 * reload the boot code, and start it all up again, with the PC 
 * pointing at the boot code.  Once the boot code has had a chance 
 * to come up, adjust all of the appropriate parameters, and send 
 * the 'start firmware' command.
 *
 * The NIC won't actually be up until it gets an interrupt with an
 * event indicating things are up.
 */

void
eshinit(sc)
	register struct esh_softc *sc;
{
	register struct ifnet *ifp = &sc->sc_if;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct rr_ring_ctl *ring;
	u_int32_t misc_host_ctl;
	u_int32_t misc_local_ctl;
	u_int32_t value;
	u_int32_t mode;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma, 0, sc->sc_dma_size,
			BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	/* Halt the processor (preserve NO_SWAP, if set) */

	misc_host_ctl = bus_space_read_4(iot, ioh, RR_MISC_HOST_CTL);
	bus_space_write_4(iot, ioh, RR_MISC_HOST_CTL, 
			  (misc_host_ctl & RR_MH_NO_SWAP) 
			  | RR_MH_HALT_PROC | RR_MH_CLEAR_INT);

	/* Make the EEPROM readable */

	misc_local_ctl = bus_space_read_4(iot, ioh, RR_MISC_LOCAL_CTL);
	bus_space_write_4(iot, ioh, RR_MISC_LOCAL_CTL,
			  misc_local_ctl & ~(RR_LC_FAST_PROM | 
					     RR_LC_ADD_SRAM | 
					     RR_LC_PARITY_ON));

	/* Reset DMA */

	bus_space_write_4(iot, ioh, RR_RX_STATE, RR_RS_RESET);
	bus_space_write_4(iot, ioh, RR_TX_STATE, 0);
	bus_space_write_4(iot, ioh, RR_DMA_READ_STATE, RR_DR_RESET);
	bus_space_write_4(iot, ioh, RR_DMA_WRITE_STATE, RR_DW_RESET);
	bus_space_write_4(iot, ioh, RR_PCI_STATE, 0);
	bus_space_write_4(iot, ioh, RR_TIMER, 0);
	bus_space_write_4(iot, ioh, RR_TIMER_REF, 0);

	/*
	 * Reset the assist register that the documentation suggests
	 * resetting.  Too bad that the docs don't mention anything
	 * else about the register!
	 */

	bus_space_write_4(iot, ioh, 0x15C, 1);

	/* Clear BIST, set the PC to the start of the code and let 'er rip */

	value = bus_space_read_4(iot, ioh, RR_PCI_BIST);
	bus_space_write_4(iot, ioh, RR_PCI_BIST, (value & ~0xff) | 8);

	sc->sc_bist_write(sc, 0);
	esh_reset_runcode(sc);

	bus_space_write_4(iot, ioh, RR_PROC_PC, sc->sc_runcode_start);
	bus_space_write_4(iot, ioh, RR_PROC_BREAKPT, 0x00000001);

	misc_host_ctl &= ~RR_MH_HALT_PROC;
	bus_space_write_4(iot, ioh, RR_MISC_HOST_CTL, misc_host_ctl);

	delay(1000);  /* Need 500 us, but we'll give it more */

	value = sc->sc_bist_read(sc);

#ifdef ESH_PRINTF
	printf("%s:  BIST is %x\n", sc->sc_dev.dv_xname, value);
	eshstatus(sc);
#endif

	/* RunCode is up.  Initialize NIC */

	esh_write_addr(iot, ioh, RR_GEN_INFO_PTR, sc->sc_gen_info_dma);
	esh_write_addr(iot, ioh, RR_RECV_RING_PTR, sc->sc_recv_ring_table_dma);

	sc->sc_event_consumer = 0;
	bus_space_write_4(iot, ioh, RR_EVENT_CONSUMER, sc->sc_event_consumer);
	sc->sc_event_producer = bus_space_read_4(iot, ioh, RR_EVENT_PRODUCER);
	sc->sc_cmd_producer = RR_INIT_CMD;
	sc->sc_cmd_consumer = 0;

	mode = bus_space_read_4(iot, ioh, RR_MODE_AND_STATUS);
	mode |= (RR_MS_WARNINGS | 
		 RR_MS_ERR_TERM |
		 RR_MS_NO_RESTART |
		 RR_MS_SWAP_DATA);
	bus_space_write_4(iot, ioh, RR_MODE_AND_STATUS, mode);

	/* Set tuning parameters */

	bus_space_write_4(iot, ioh, RR_CONN_RETRY_COUNT, 
			  sc->sc_tune.rt_conn_retry_count);
	bus_space_write_4(iot, ioh, RR_CONN_RETRY_TIMER, 
			  sc->sc_tune.rt_conn_retry_timer);
	bus_space_write_4(iot, ioh, RR_CONN_TIMEOUT, 
			  sc->sc_tune.rt_conn_timeout);
	bus_space_write_4(iot, ioh, RR_INTERRUPT_TIMER, 
			  sc->sc_tune.rt_interrupt_timer);
	bus_space_write_4(iot, ioh, RR_TX_TIMEOUT, 
			  sc->sc_tune.rt_tx_timeout);
	bus_space_write_4(iot, ioh, RR_RX_TIMEOUT, 
			  sc->sc_tune.rt_rx_timeout);
	bus_space_write_4(iot, ioh, RR_STATS_TIMER, 
			  sc->sc_tune.rt_stats_timer);
	bus_space_write_4(iot, ioh, RR_PCI_STATE, 
			  sc->sc_tune.rt_pci_state);
	bus_space_write_4(iot, ioh, RR_DMA_WRITE_STATE, 
			  sc->sc_tune.rt_dma_write_state);
	bus_space_write_4(iot, ioh, RR_DMA_READ_STATE, 
			  sc->sc_tune.rt_dma_read_state);

	sc->sc_runcode_version = 
		bus_space_read_4(iot, ioh, RR_RUNCODE_VERSION);
	sc->sc_version = sc->sc_runcode_version >> 16;
	if (sc->sc_version != 1 && sc->sc_version != 2) {
		printf("%s:  bad version number %d in runcode\n",
		       sc->sc_dev.dv_xname, sc->sc_version);
		return;
	}

	if (sc->sc_version == 1) {
		sc->sc_options = 0;
	} else {
		value = bus_space_read_4(iot, ioh, RR_ULA);
		sc->sc_options = value >> 16;
	}

	printf("%s: startup runcode version %d.%d.%d, options %x\n",
	       sc->sc_dev.dv_xname,
	       sc->sc_version, 
	       (sc->sc_runcode_version >> 8) & 0xff,
	       sc->sc_runcode_version & 0xff, 
	       sc->sc_options);

	/* Initialize the general ring information */

	bzero(sc->sc_recv_ring_table, 
	      sizeof(struct rr_ring_ctl) * RR_ULP_COUNT);

	ring = &sc->sc_gen_info->ri_event_ring_ctl;
	ring->rr_ring_addr = sc->sc_event_ring_dma;
	ring->rr_entry_size = sizeof(struct rr_event);
	ring->rr_free_bufs = RR_EVENT_RING_SIZE / 4;
	ring->rr_entries = RR_EVENT_RING_SIZE;
	ring->rr_prod_index = 0;

	ring = &sc->sc_gen_info->ri_cmd_ring_ctl;
	ring->rr_free_bufs = 8;
	ring->rr_entry_size = sizeof(union rr_cmd);
	ring->rr_prod_index = RR_INIT_CMD;

	ring = &sc->sc_gen_info->ri_send_ring_ctl;
	ring->rr_ring_addr = sc->sc_send_ring_dma;
	if (sc->sc_version == 1) {
		ring->rr_free_bufs = RR_RR_DONT_COMPLAIN;
	} else {
		ring->rr_free_bufs = 0;
	}

	if (sc->sc_options & RR_OP_LONG_TX) {
		ring->rr_entries = RR2_SEND_RING_SIZE;
		ring->rr_entry_size = sizeof(struct rr2_descr);
	} else {
		ring->rr_entries = RR_SEND_RING_SIZE;
		ring->rr_entry_size = sizeof(struct rr_descr);
	}

	ring->rr_prod_index = sc->sc_send.ec_producer = 
		sc->sc_send.ec_consumer = 0;

	sc->sc_snap_recv.ec_descr = sc->sc_snap_recv_ring;
	sc->sc_snap_recv.ec2_descr = sc->sc2_snap_recv_ring;
	sc->sc_snap_recv.ec_consumer = sc->sc_snap_recv.ec_producer = 0;

	/* Initialize the ring of descriptors as necessary */

	if (sc->sc_version == 2 && (sc->sc_options & RR_OP_LONG_TX)) {
		int i;
		for (i = 0; i < RR2_SEND_RING_SIZE; i++)
			sc->sc_snap_recv.ec2_descr[i].rd_dma_state = RR_DM_TX;
	}

#ifdef ESH_PRINTF
	eshstatus(sc);
#endif
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma, 0, sc->sc_dma_size,
			BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	esh_send_cmd(sc, RR_CC_START_RUNCODE, 0, 0);

#ifdef ESH_PRINTF
	eshstatus(sc);
#endif

	/* Set up the watchdog to make sure something happens! */

	sc->sc_watchdog = 0;
	ifp->if_timer = 5;

	/* 
	 * Can't actually turn on interface until we see some events,
	 * so set initialized flag, but don't start sending.
	 */

	sc->sc_flags = ESH_FL_INITIALIZED;
}


/*
 * Handle interrupts.  We only get interrupts to indicate that there 
 * are events to process, so this is all just event handling code.
 */

int
eshintr(arg)
	void *arg;
{
	register struct esh_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct ifnet *ifp = &sc->sc_if;
	u_int32_t rc_offsets;
	u_int32_t misc_host_ctl;
	int rc_send_consumer = 0;	/* shut up compiler */
	int rc_snap_ring_consumer = 0;	/* ditto */
	int start_consumer;
	int ret = 0;

	/* Check to see if this is our interrupt. */

	misc_host_ctl = bus_space_read_4(iot, ioh, RR_MISC_HOST_CTL);
	if ((misc_host_ctl & RR_MH_INTERRUPT) == 0)
		return 0;

	rc_offsets = bus_space_read_4(iot, ioh, RR_EVENT_PRODUCER);
	sc->sc_event_producer = rc_offsets & 0xff;
	if (sc->sc_version == 2) {
		rc_send_consumer = (rc_offsets >> 8) & 0xff;
		rc_snap_ring_consumer = (rc_offsets >> 16) & 0xff;
	}
	start_consumer = sc->sc_event_consumer;

	/* Take care of synchronizing DMA with entries we read... */

	esh_dma_sync(sc, sc->sc_event_ring, 
		     start_consumer, sc->sc_event_producer, 
		     RR_EVENT_RING_SIZE, sizeof(struct rr_event), 0,
		     BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	while (sc->sc_event_consumer != sc->sc_event_producer) {
		struct rr_event *event = 
			&sc->sc_event_ring[sc->sc_event_consumer];

#ifdef ESH_PRINTF
		printf("%s:  event code %x, index %x\n", sc->sc_dev.dv_xname,
		       event->re_code, event->re_index);
#endif
		ret = 1;   /* some action was taken by card */

		switch(event->re_code) {
		case RR_EC_RUNCODE_UP:
			printf("%s:  firmware up\n", sc->sc_dev.dv_xname);
			sc->sc_flags |= ESH_FL_RUNCODE_UP;
			esh_send_cmd(sc, RR_CC_WATCHDOG, 0, 0);
			esh_send_cmd(sc, RR_CC_UPDATE_STATS, 0, 0);
			esh_init_snap_ring(sc);
			break;

		case RR_EC_WATCHDOG:
			/* 
			 * Record the watchdog event.  
			 * This is checked by eshwatchdog 
			 */

			sc->sc_watchdog = 1;
			break;

		case RR_EC_SET_CMD_CONSUMER:
			sc->sc_cmd_consumer = event->re_index;
			break;

		case RR_EC_LINK_ON:
			printf("%s:  link up\n", sc->sc_dev.dv_xname);
			sc->sc_flags |= ESH_FL_LINK_UP;

			esh_send_cmd(sc, RR_CC_WATCHDOG, 0, 0);
			esh_send_cmd(sc, RR_CC_UPDATE_STATS, 0, 0);
			if ((sc->sc_flags) & ESH_FL_RRING_UP) {
				/* Interface is now `running', with no output active. */
				ifp->if_flags |= IFF_RUNNING;
				ifp->if_flags &= ~IFF_OACTIVE;

				/* Attempt to start output, if any. */
				eshstart(ifp);
			}
			break;

		case RR_EC_LINK_OFF:
			sc->sc_flags &= ~ESH_FL_LINK_UP;
			break;

			/* 
			 * These are all unexpected.  We need to handle all 
			 * of them, though. 
			 */

#define CALLOUT(a) case a: \
	printf("Event " #a " received -- ring %x index %x timestamp %x\n", \
	       event->re_ring, event->re_index, event->re_timestamp); \
	break;

		case RR_EC_INVALID_CMD:
		case RR_EC_INTERNAL_ERROR:
		case RR2_EC_INTERNAL_ERROR:
		case RR_EC_BAD_SEND_RING:
		case RR_EC_BAD_SEND_BUF:
		case RR_EC_BAD_SEND_DESC:
		case RR_EC_RING_ENABLE_ERR:
		case RR_EC_RING_DISABLED:
		case RR_EC_RECV_RING_FLUSH:
		case RR_EC_RECV_ERROR_INFO:
		case RR_EC_BAD_RECV_BUF:
		case RR_EC_BAD_RECV_DESC:
		case RR_EC_BAD_RECV_RING:
		case RR_EC_UNIMPLEMENTED:
			printf("%s:  unexpected event %x;"
			       "shutting down interface\n",
			       sc->sc_dev.dv_xname, event->re_code);
			ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
			eshstatus(sc);
			break;

			CALLOUT(RR_EC_OUT_OF_BUF);
			CALLOUT(RR_EC_NO_RING_FOR_ULP);
			CALLOUT(RR_EC_REJECTING);  /* dropping packets */
#undef CALLOUT

			/* Send events */

		case RR_EC_PACKET_SENT:   	/* not used in firmware 2.x */
			ifp->if_opackets++;
			/* FALLTHROUGH */

		case RR_EC_SET_SND_CONSUMER:
			assert(sc->sc_version == 1);
			/* FALLTHROUGH */

		case RR_EC_SEND_RING_LOW:
			eshstart_cleanup(sc, event->re_index, 0); 
			break;


		case RR_EC_CONN_REJECT:
		case RR_EC_CAMPON_TIMEOUT:
		case RR_EC_CONN_TIMEOUT:
		case RR_EC_DISCONN_ERR:
		case RR_EC_INTERNAL_PARITY:
		case RR_EC_TX_IDLE:
		case RR_EC_SEND_LINK_OFF:
			eshstart_cleanup(sc, event->re_index, event->re_code); 
			break;

			/* Receive events */

		case RR_EC_RING_ENABLED:
			sc->sc_flags |= ESH_FL_RRING_UP;
			assert(event->re_ring == HIPPI_ULP_802);

			/* XXX: need to pass in rc consumer: */
			esh_fill_snap_ring(sc);

			if (sc->sc_flags & ESH_FL_LINK_UP) {
				/* 
				 * Interface is now `running', with no 
				 * output active. 
				 */
				ifp->if_flags |= IFF_RUNNING;
				ifp->if_flags &= ~IFF_OACTIVE;

				/* Attempt to start output, if any. */
				eshstart(ifp);
			}
			break;

		case RR_EC_RECV_RING_LOW:
		case RR_EC_RECV_RING_OUT:
		case RR_EC_SET_RECV_CONSUMER:
		case RR_EC_PACKET_RECVED:
			eshread(sc, event->re_index, 0);
			break;

		case RR_EC_PACKET_DISCARDED:
		case RR_EC_PARITY_ERR:
		case RR_EC_LLRC_ERR:
		case RR_EC_PKT_LENGTH_ERR:
		case RR_EC_IP_HDR_CKSUM_ERR:
		case RR_EC_DATA_CKSUM_ERR:
		case RR_EC_SHORT_BURST_ERR:
		case RR_EC_RECV_LINK_OFF:
		case RR_EC_FLAG_SYNC_ERR:
		case RR_EC_FRAME_ERR:
		case RR_EC_RECV_IDLE:
		case RR_EC_STATE_TRANS_ERR:
		case RR_EC_NO_READY_PULSE:
			eshread(sc, event->re_index, event->re_code);
			break;

		/*
		 * Statistics events can be ignored for now.  They might become
		 * necessary if we have to deliver stats on demand, rather than
		 * just returning the statistics block of memory.
		 */

		case RR_EC_STATS_UPDATE:
		case RR_EC_STATS_RETRIEVED:
		case RR_EC_TRACE:
			break;

		default:
			printf("%s:  Bogus event code %x, "
			       "ring %x, index %x, timestamp %x\n",
			       sc->sc_dev.dv_xname, event->re_code, 
			       event->re_ring, event->re_index, 
			       event->re_timestamp);
			break;
		}

		sc->sc_event_consumer = NEXT_EVENT(sc->sc_event_consumer);
	}

	/* Do the receive and send ring processing for version 2 RunCode */

	if (sc->sc_version == 2) {
		if (sc->sc_send.ec_consumer != rc_send_consumer) {
			eshstart_cleanup(sc, rc_send_consumer, 0);
			ret = 1;
		}
		if (sc->sc_snap_recv.ec_consumer != rc_snap_ring_consumer) {
			eshread(sc, rc_snap_ring_consumer, 0); 
			ret = 1;
		}
		rc_offsets = (sc->sc_snap_recv.ec_consumer << 16) | 
			(sc->sc_send.ec_consumer << 8) | sc->sc_event_consumer;
	} else {
		rc_offsets = sc->sc_event_consumer;
	}

	esh_dma_sync(sc, sc->sc_event_ring, 
		     start_consumer, sc->sc_event_producer, 
		     RR_EVENT_RING_SIZE, sizeof(struct rr_event), 0,
		     BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	/* Clear interrupt */

	bus_space_write_4(iot, ioh, RR_EVENT_CONSUMER, rc_offsets);

	return (ret);
}


/*
 * Start output on the interface.  Always called as splnet().
 * Check to see if there are any mbufs that didn't get sent the
 * last time this was called.  If there are none, get more mbufs
 * and send 'em.
 *
 * For now, we only send one packet at a time.
 */

void
eshstart(ifp)
	struct ifnet *ifp;
{
	register struct esh_softc *sc = ifp->if_softc;
	struct mbuf *m, *m0;
	u_int32_t m_write_len;
	struct esh_ring_ctl *send = &sc->sc_send;

	/* Don't transmit if interface is busy or not running */
	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

#ifdef ESH_PRINTF
	if (esh_check(sc))
		return;
#endif

	/* If we have sent the current packet, get another */
	if (!(m = send->ec_cur_mbuf)) {
		struct mbuf *n, *n0;

		IF_DEQUEUE(&ifp->if_snd, m);
		if (m == 0)		/* not really needed */
			return;

#if NBPFILTER > 0
		if (ifp->if_bpf) {
			/*
			 * On output, the raw packet has a eight-byte CCI 
			 * field prepended.  On input, there is no such field.
			 * The bpf expects the packet to look the same in both
			 * places, so we temporarily lop off the prepended CCI
			 * field here, then replace it.  Ugh.
			 *
			 * XXX:  Need to use standard mbuf manipulation 
			 *       functions, first mbuf may be less than 
			 *       8 bytes long.
			 */
 
			m->m_len -= 8;
			m->m_data += 8;
			m->m_pkthdr.len -= 8; 
			bpf_mtap(ifp->if_bpf, m);
			m->m_len += 8;
			m->m_data -= 8;
			m->m_pkthdr.len += 8;
		}
#endif

		/* 
		 * Version 1 over the firmware sent an event each
		 * time it sent out a packet.  Later versions do not
		 * (which results in a considerable speedup), so we
		 * have to keep track here.
		 */

		if (sc->sc_version != 1)
			sc->sc_if.if_opackets++;
		m_write_len = write_len = m->m_pkthdr.len;

		if (write_len > max_write_len)
			max_write_len = write_len;

		/*
		 * XXX:  Ouch:  The NIC can only send word-aligned
		 *       buffers, and only the last buffer in the packet can
		 *       have a length that is not a multiple of four!
		 *
		 * Here we traverse the packet, pick out the bogus mbufs,
		 * and fix 'em if possible.  The fix is amazingly expensive,
		 * so we sure hope that this is a rare occurance (it seems to 
		 * be).  Note that we can really play pretty rough with this 
		 * stuff, since the send routine doesn't need mbuf packet 
		 * headers or any of that silly stuff...
		 */

		for (n0 = n = m; n; n = n->m_next) {
			while (n && n->m_len == 0) {
				MFREE(n, m0);
				if (n == m)
					n = n0 = m = m0;
				else
					n = n0->m_next = m0;
			}
			if (n == NULL)
				break;

			if (mtod(n, long) & 3 || (n->m_next && n->m_len & 3)) {
				/* Gotta clean it up */
				struct mbuf *o;
				u_int32_t len;

				sc->sc_misaligned_bufs++;

#ifdef ESH_PRINTF
				assert(n != NULL);
				printf("eshstart:  adjusting misaligned "
				       "mbuf %p, len %x\n",
				       mtod(n, void *), n->m_len);
#endif

				MGETHDR(o, M_DONTWAIT, MT_DATA);
				if (!o) 
					goto bogosity;

				MCLGET(o, M_DONTWAIT);
				if (!(o->m_flags & M_EXT)) {
					MFREE(o, m0);
					goto bogosity;
				}

				/* 
				 * XXX: Copy as much as we can into the 
				 *      cluster.  For now we can't have more 
				 *      than a cluster in there.  May change.
				 *      I'd prefer not to get this 
				 *      down-n-dirty, but we have to be able 
				 *      to do this kind of funky copy.
				 */

				len = min(MCLBYTES, m_write_len);
#ifdef ESH_PRINTF
				printf("eshstart:  using len of %x,"
				       "o of %p\n", 
				       len, mtod(o, void *));
				assert(n->m_len <= len);
				assert(len <= MCLBYTES);
#endif
				m_copydata(n, 0, len, mtod(o, void *));
				m_adj(n, len);
				o->m_len = len;
				o->m_next = n;

				if (n == m)
					m = o;
				else
					n0->m_next = o;
				n = o;
			}
			n0 = n;
			m_write_len -= n->m_len;
		}
#ifdef ESH_PRINTF
		printf("eshstart:  m_write_len  %d, m %p\n", m_write_len, m);
#endif
		send->ec_cur_mbuf = send->ec_cur_pkt = m;
	}

	if (sc->sc_options & RR_OP_LONG_TX)
		esh_send2(sc);
	else
		esh_send(sc);
	return;

bogosity:
	printf("%s:  eshstart:  unable to allocate cluster for "
	       "mbuf %p, len %x\n",
	       sc->sc_dev.dv_xname, mtod(m, void *), m->m_len);
	m_freem(m);
	send->ec_cur_mbuf = m = NULL;
	return;
}


/* 
 * Yeah, this is a little ugly.  For some unknown reason, I've decided
 * to try to make this driver work for HIPPI, and possibly Giga-E,
 * or whatever else is annoying enough to use long descriptors.
 * Just trying to switch on all the instances of ec_descr drove
 * me nuts, so I broke out the function into esh_send and esh_send2.  
 * I'm still not convinced I shouldn't yank out all of the
 * long descriptor stuff!
 */

static void
esh_send(sc)
	struct esh_softc *sc;
{
	struct esh_ring_ctl *send = &sc->sc_send;
	struct mbuf *m = send->ec_cur_mbuf;
	int start_producer = send->ec_producer;
 
#ifdef ESH_PRINTF
	printf("esh_send:  producer %x  consumer %x  m %p\n",
	       send->ec_producer, send->ec_consumer, m);
#endif

	esh_dma_sync(sc, send->ec_descr,
		     send->ec_producer, send->ec_consumer,
		     RR_SEND_RING_SIZE, sizeof(struct rr_descr), 1,
		     BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	while (NEXT_SEND(send->ec_producer) != send->ec_consumer && m) {
		int offset = send->ec_producer;
		int error;

		send->ec_m[offset] = m;
		error = bus_dmamap_load(sc->sc_dmat, send->ec_dma[offset],
					mtod(m, void *), m->m_len, 
					NULL, BUS_DMA_NOWAIT);
		if (error)
			panic("%s:  eshstart:  bus_dmamap_load failed "
			      "descr %x err %d\n", 
			      sc->sc_dev.dv_xname, offset, error);

		/* 
		 * In this implementation, we should only see one segment 
		 * per DMA.
		 */

		assert(send->ec_dma[offset]->dm_nsegs == 1);

		send->ec_descr[offset].rd_buffer_addr = 
			send->ec_dma[offset]->dm_segs->ds_addr;
		send->ec_descr[offset].rd_length =
			send->ec_dma[offset]->dm_segs->ds_len;
		send->ec_descr[offset].rd_control = 0;

		if (m == send->ec_cur_pkt) {
			/* Start of the mbuf... */
			send->ec_descr[offset].rd_control |= 
				RR_CT_PACKET_START;
		}

		if (!m->m_next) {
			send->ec_descr[offset].rd_control |= RR_CT_PACKET_END;
		}

#ifdef ESH_PRINTF
		printf("esh_send:  offset %x  buf addr %x  len %d  "
		       "control %x  m->m_len %d\n",
		       offset, 
		       send->ec_descr[offset].rd_buffer_addr,
		       send->ec_descr[offset].rd_length,
		       send->ec_descr[offset].rd_control, m->m_len);
#endif

		/* 
		 * When we have bus_dmamap_load_mbuf, this can be done once at
		 * the end of the load.
		 */

		bus_dmamap_sync(sc->sc_dmat, send->ec_dma[offset], 0, m->m_len,
				BUS_DMASYNC_PREWRITE);
		m = m->m_next;
		send->ec_producer = NEXT_SEND(send->ec_producer);
	}

	esh_dma_sync(sc, send->ec_descr,
		     start_producer, send->ec_consumer,
		     RR_SEND_RING_SIZE, sizeof(struct rr_descr), 1, 
		     BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

#ifdef ESH_PRINTF
	if (m) printf("eshstart:  couldn't fit packet in send ring!\n");
#endif

	send->ec_cur_mbuf = m;
	if (sc->sc_version == 1) {
		esh_send_cmd(sc, RR_CC_SET_SEND_PRODUCER, 
			     0, send->ec_producer);
	} else {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, 
				  RR_SEND_PRODUCER, send->ec_producer); 
	}
	return;
}


static void
esh_send2(sc)
	struct esh_softc *sc;
{
	struct esh_ring_ctl *send = &sc->sc_send;
	struct mbuf *m = send->ec_cur_mbuf;
	int start_producer = send->ec_producer;

	esh_dma_sync(sc, send->ec2_descr,
		     start_producer, send->ec_consumer,
		     RR2_SEND_RING_SIZE, sizeof(struct rr2_descr), 1, 
		     BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

 	while (NEXT_SEND2(send->ec_producer) != send->ec_consumer && m) {
		int offset = send->ec_producer;
		int error;

		send->ec_m[offset] = m;
		error = bus_dmamap_load(sc->sc_dmat, send->ec_dma[offset],
					mtod(m, void *), m->m_len, 
					NULL, BUS_DMA_NOWAIT);
		if (error)
			panic("%s:  eshstart:  bus_dmamap_load failed "
			      "descr %x err %d\n", 
			      sc->sc_dev.dv_xname, offset, error);

		/* 
		 * In this implementation, we should only see one segment 
		 * per DMA 
		 */

		assert(send->ec_dma[offset]->dm_nsegs == 1);

		/* Load into the descriptors */

		send->ec2_descr[offset].rd_buffer_addr = 
			send->ec_dma[offset]->dm_segs->ds_addr;
		send->ec2_descr[offset].rd_length =
			send->ec_dma[offset]->dm_segs->ds_len;
		send->ec2_descr[offset].rd_control = 0;
		if (m == send->ec_cur_pkt) {
			/* Start of the mbuf... */
			send->ec2_descr[offset].rd_control |= 
				RR2_CT_PACKET_START;
		}

		if (!m->m_next) {
			send->ec2_descr[offset].rd_control |= 
				RR2_CT_PACKET_END;
		}

		/* 
		 * When we have bus_dmamap_load_mbuf, this can be done once at
		 * the end of the load.
		 */

		bus_dmamap_sync(sc->sc_dmat, send->ec_dma[offset], 0, m->m_len,
				BUS_DMASYNC_PREWRITE);
		m = m->m_next;
		send->ec_producer = NEXT_SEND2(send->ec_producer);
	}

	esh_dma_sync(sc, send->ec2_descr,
		     start_producer, send->ec_consumer,
		     RR2_SEND_RING_SIZE, sizeof(struct rr2_descr), 1, 
		     BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

#ifdef ESH_PRINTF
	if (m) printf("eshstart:  couldn't fit packet in send ring!\n");
#endif

	send->ec_cur_mbuf = m;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, 
			  RR_SEND_PRODUCER, send->ec_producer); 
	return;
}

/*
 * Cleanup for the send routine.  When the NIC sends us an event to 
 * let us know that it has consumed our buffers, we need to free the
 * buffers and increment our count.
 */

static void
eshstart_cleanup(sc, consumer, error)
	register struct esh_softc *sc;
	u_int16_t consumer;
	int error;
{
	struct esh_ring_ctl *send = &sc->sc_send;
	struct mbuf *m0;  /* garbage for MFREE */
	int start_consumer = send->ec_consumer;
	int offset;

#ifdef ESH_PRINTF
	printf("eshstart_cleanup:  consumer %x, send->consumer %x\n",
	       consumer, send->ec_consumer);	
#endif

	if (sc->sc_options & RR_OP_LONG_TX)
		esh_dma_sync(sc, send->ec2_descr,
			     send->ec_consumer, consumer,
			     RR2_SEND_RING_SIZE, sizeof(struct rr2_descr), 0, 
			     BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	else
		esh_dma_sync(sc, send->ec_descr,
			     send->ec_consumer, consumer,
			     RR_SEND_RING_SIZE, sizeof(struct rr_descr), 0, 
			     BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	while (send->ec_consumer != consumer) {
		offset = send->ec_consumer;
		assert(send->ec_dma[offset]->dm_nsegs);
		assert(send->ec_m[offset]);

		bus_dmamap_sync(sc->sc_dmat, send->ec_dma[offset], 
				0, send->ec_m[offset]->m_len, 
				BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, send->ec_dma[offset]);
		MFREE(send->ec_m[offset], m0);
		send->ec_m[offset] = NULL;
		send->ec_dma[offset]->dm_nsegs = 0;
		if (sc->sc_options & RR_OP_LONG_TX) {
			send->ec2_descr[offset].rd_length = 0;
			send->ec_consumer = NEXT_SEND2(send->ec_consumer);
		} else {
			send->ec_descr[offset].rd_length = 0;
			send->ec_consumer = NEXT_SEND(send->ec_consumer);
		}
	}

	if (sc->sc_options & RR_OP_LONG_TX)
		esh_dma_sync(sc, send->ec2_descr,
			     start_consumer, consumer,
			     RR2_SEND_RING_SIZE, sizeof(struct rr2_descr), 0, 
			     BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	else
		esh_dma_sync(sc, send->ec_descr,
			     start_consumer, consumer,
			     RR_SEND_RING_SIZE, sizeof(struct rr_descr), 0, 
			     BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	eshstart(&sc->sc_if);
}


/*
 * Read in the current valid entries from the ring and forward
 * them to the upper layer protocols.  It is possible that we
 * haven't received the whole packet yet, in which case we just
 * add each of the buffers into the packet until we have the whole
 * thing.
 */

void
eshread(sc, consumer, error)
	register struct esh_softc *sc;
	u_int16_t consumer;
	int error;
{
	struct ifnet *ifp = &sc->sc_if;
	struct esh_ring_ctl *recv = &sc->sc_snap_recv;
	struct hippi_header *hh;
	int start_consumer = recv->ec_consumer;
	u_int16_t control;

	if (error)
		recv->ec_error = error;

	if (sc->sc_options & RR_OP_LONG_TX)
		esh_dma_sync(sc, recv->ec2_descr,
			     start_consumer, consumer,
			     RR2_SNAP_RECV_RING_SIZE, 
			     sizeof(struct rr2_descr), 0, 
			     BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	else
		esh_dma_sync(sc, recv->ec_descr,
			     start_consumer, consumer,
			     RR_SNAP_RECV_RING_SIZE, 
			     sizeof(struct rr_descr), 0, 
			     BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	while (recv->ec_consumer != consumer) {
		u_int16_t offset = recv->ec_consumer;
		struct mbuf *m;

		m = recv->ec_m[offset];
		if (sc->sc_options & RR_OP_LONG_RX) {
			m->m_len = recv->ec2_descr[offset].rd_length;
			control = recv->ec2_descr[offset].rd_control;
		} else {
			m->m_len = recv->ec_descr[offset].rd_length;
			control = recv->ec_descr[offset].rd_control;
		}
		bus_dmamap_sync(sc->sc_dmat, recv->ec_dma[offset], 0, m->m_len,
				BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, recv->ec_dma[offset]);

#ifdef ESH_PRINTF
		printf("eshread: offset %x addr %p len %x flags %x\n",
		       offset, mtod(m, void *), m->m_len, control);
#endif
		if (control & RR_CT_PACKET_START || !recv->ec_cur_mbuf) {
			if (recv->ec_cur_pkt) {
				m_freem(recv->ec_cur_pkt);
				recv->ec_cur_pkt = NULL;
				printf("%s:  possible skipped packet!\n", 
				       sc->sc_dev.dv_xname);
			}
			recv->ec_cur_pkt = recv->ec_cur_mbuf = m;
			/* allocated buffers all have pkthdrs... */
			m->m_pkthdr.rcvif = ifp;
			m->m_pkthdr.len = m->m_len;
		} else {
			if (!recv->ec_cur_pkt)
				panic("eshread:  no cur_pkt");
		
			recv->ec_cur_mbuf->m_next = m;
			recv->ec_cur_mbuf = m;
			recv->ec_cur_pkt->m_pkthdr.len += m->m_len;
		}

		recv->ec_m[offset] = NULL;
		if (sc->sc_options & RR_OP_LONG_TX) {
			recv->ec2_descr[offset].rd_length = 0;
			recv->ec2_descr[offset].rd_buffer_addr = 0;
		} else {
			recv->ec_descr[offset].rd_length = 0;
			recv->ec_descr[offset].rd_buffer_addr = 0;
		}

		/* Note that we can START and END on the same buffer */

		if (control & RR_CT_PACKET_END) { /* XXX: RR2_ matches */
			m = recv->ec_cur_pkt;
			if (!error && !recv->ec_error) {
				/* We have a complete packet, send it up 
				 * the stack... 
				 */
				ifp->if_ipackets++;

#if NBPFILTER > 0
				/*
				 * Check if there's a BPF listener on this 
				 * interface.  If so, hand off the raw packet
				 * to BPF.
				 */
				if (ifp->if_bpf) {
					/*
					 * Incoming packets start with the FP
					 * data, so no alignment problems 
					 * here...
					 */
					bpf_mtap(ifp->if_bpf, m);
				}
#endif
				m = m_pullup(m, sizeof(struct hippi_header *));
				hh = mtod(m, struct hippi_header *);
				m_adj(m, sizeof(struct hippi_header));
				hippi_input(ifp, hh, m);
			} else {
				ifp->if_ierrors++;
				recv->ec_error = 0;
				m_freem(m);
			}
			recv->ec_cur_pkt = recv->ec_cur_mbuf = NULL;
		}

		if (sc->sc_options & RR_OP_LONG_RX) {
			recv->ec2_descr[offset].rd_control = 0;
			recv->ec_consumer = NEXT_RECV2(recv->ec_consumer);
		} else {
			recv->ec_descr[offset].rd_control = 0;
			recv->ec_consumer = NEXT_RECV(recv->ec_consumer);
		}
	}

	if (sc->sc_options & RR_OP_LONG_TX)
		esh_dma_sync(sc, recv->ec2_descr,
			     start_consumer, consumer,
			     RR2_SNAP_RECV_RING_SIZE, 
			     sizeof(struct rr2_descr), 0, 
			     BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	else
		esh_dma_sync(sc, recv->ec_descr,
			     start_consumer, consumer,
			     RR_SNAP_RECV_RING_SIZE, 
			     sizeof(struct rr_descr), 0, 
			     BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	esh_fill_snap_ring(sc);
}


/*
 * Add the SNAP (IEEE 802) receive ring to the NIC.  It is possible
 * that we are doing this after resetting the card, in which case
 * the structures have already been filled in and we may need to
 * resume sending data.
 */

static void
esh_init_snap_ring(sc)
	struct esh_softc *sc;
{
	struct rr_ring_ctl *ring = sc->sc_recv_ring_table + HIPPI_ULP_802;
	int i;


	if (ring->rr_entry_size == 0) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma,
				(caddr_t) ring - (caddr_t) sc->sc_dma_addr,
				sizeof(ring),
				BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		ring->rr_ring_addr = sc->sc_snap_recv_ring_dma;
		ring->rr_free_bufs = RR_SNAP_RECV_RING_SIZE / 4;
		if (sc->sc_options & RR_OP_LONG_RX) {
			ring->rr_entries = RR2_SNAP_RECV_RING_SIZE;
			ring->rr_entry_size = sizeof(struct rr2_descr);
		} else {
			ring->rr_entries = RR_SNAP_RECV_RING_SIZE;
			ring->rr_entry_size = sizeof(struct rr_descr);
		}
		ring->rr_prod_index = sc->sc_snap_recv.ec_producer = 0;
		ring->rr_mode = RR_RR_IP;

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma,
				(caddr_t) ring - (caddr_t) sc->sc_dma_addr,
				sizeof(ring),
				BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	} else {
		printf("%s:  snap receive ring already initialized!\n",
		       sc->sc_dev.dv_xname);
	}

	if (sc->sc_options & RR_OP_LONG_RX) {
		struct rr2_descr *d = sc->sc2_snap_recv_ring;

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma,
				(caddr_t) d - (caddr_t) sc->sc_dma_addr,
				sizeof(struct rr2_descr) * 
				RR2_SNAP_RECV_RING_SIZE,
				BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		bzero(d, sizeof(struct rr2_descr) * RR2_SNAP_RECV_RING_SIZE);
		for (i = 0; i < RR2_SNAP_RECV_RING_SIZE; i++)
			d[i].rd_dma_state = RR_DM_RX;

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma,
				(caddr_t) d - (caddr_t) sc->sc_dma_addr,
				sizeof(struct rr2_descr) * 
				RR2_SNAP_RECV_RING_SIZE,
				BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	}

	esh_send_cmd(sc, RR_CC_ENABLE_RING, HIPPI_ULP_802, 
		     sc->sc_snap_recv.ec_producer);
}

/* 
 * Fill in the snap ring with more mbuf buffers so that we can
 * receive traffic.
 */

static void
esh_fill_snap_ring(sc)
	struct esh_softc *sc;
{
	struct esh_ring_ctl *recv = &sc->sc_snap_recv;
	int count = 0;
	int start_producer = recv->ec_producer;
	int error;

	if (sc->sc_options & RR_OP_LONG_TX)
		esh_dma_sync(sc, recv->ec2_descr,
			     recv->ec_producer, recv->ec_consumer,
			     RR2_SNAP_RECV_RING_SIZE, 
			     sizeof(struct rr2_descr), 1, 
			     BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	else
		esh_dma_sync(sc, recv->ec_descr,
			     recv->ec_producer, recv->ec_consumer,
			     RR_SNAP_RECV_RING_SIZE, 
			     sizeof(struct rr_descr), 1, 
			     BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	while (((sc->sc_options & RR_OP_LONG_RX) ? 
		NEXT_RECV2(recv->ec_producer) :
		NEXT_RECV(recv->ec_producer)) != recv->ec_consumer) {
		int offset = recv->ec_producer;
		struct mbuf *m, *m0;

		count++;

		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (!m)
			break;
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			MFREE(m, m0);
			break;
		}

		error = bus_dmamap_load(sc->sc_dmat, recv->ec_dma[offset],
					mtod(m, void *), MCLBYTES,
					NULL, BUS_DMA_NOWAIT);
		if (error) {
			printf("%s:  esh_fill_recv_ring:  bus_dmamap_load "
			       "failed\toffset %x, error code %d\n", 
			       sc->sc_dev.dv_xname, offset, error);
			MFREE(m, m0);
			break;
		}

		/* 
		 * In this implementation, we should only see one segment 
		 * per DMA.
		 */

		assert(recv->ec_dma[offset]->dm_nsegs == 1);

		/* 
		 * Load into the descriptors.  
		 */

		if (sc->sc_options & RR_OP_LONG_RX) {
			recv->ec2_descr[offset].rd_buffer_addr = 
				recv->ec_dma[offset]->dm_segs->ds_addr;
			recv->ec2_descr[offset].rd_length =
				recv->ec_dma[offset]->dm_segs->ds_len;
			recv->ec2_descr[offset].rd_control = 0;
		} else {
			recv->ec_descr[offset].rd_ring = 
				(sc->sc_version == 1) ? HIPPI_ULP_802 : 0;
			recv->ec_descr[offset].rd_buffer_addr = 
				recv->ec_dma[offset]->dm_segs->ds_addr;
			recv->ec_descr[offset].rd_length =
				recv->ec_dma[offset]->dm_segs->ds_len;
			recv->ec_descr[offset].rd_control = 0;
		}

		bus_dmamap_sync(sc->sc_dmat, recv->ec_dma[offset], 0, MCLBYTES,
				BUS_DMASYNC_PREREAD);

		recv->ec_m[offset] = m;

		if (sc->sc_options & RR_OP_LONG_RX)
			recv->ec_producer = NEXT_RECV2(recv->ec_producer);
		else
			recv->ec_producer = NEXT_RECV(recv->ec_producer);
	}

	if (sc->sc_options & RR_OP_LONG_TX)
		esh_dma_sync(sc, recv->ec2_descr,
			     start_producer, recv->ec_consumer,
			     RR2_SNAP_RECV_RING_SIZE, 
			     sizeof(struct rr2_descr), 1, 
			     BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	else
		esh_dma_sync(sc, recv->ec_descr,
			     start_producer, recv->ec_consumer,
			     RR_SNAP_RECV_RING_SIZE, 
			     sizeof(struct rr_descr), 1, 
			     BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	if (sc->sc_version == 1)
		esh_send_cmd(sc, RR_CC_SET_RECV_PRODUCER, HIPPI_ULP_802, 
			     recv->ec_producer);
	else
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, 
				  RR_SNAP_RECV_PRODUCER, recv->ec_producer);
}


int
eshioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	int error = 0;
	struct esh_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifdrv *ifd = (struct ifdrv *) data;
	struct rr_eeprom rr_eeprom;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int32_t misc_host_ctl;
	u_int32_t misc_local_ctl;
	u_int32_t address;
	u_int32_t value;
	u_int32_t offset;
	u_int32_t length;
	u_long len;
	int s, i;

	s = splnet();

	while (sc->sc_flags & ESH_FL_EEPROM_BUSY) {
		error = tsleep((void *)&sc->sc_flags, PCATCH | PRIBIO,
		    "esheeprom", 0);
		if (error != 0)
			goto ioctl_done;
	}

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			/* The driver doesn't really care about IP addresses */
			break;
#endif
#ifdef NS
		case AF_NS:
			{
				register struct ns_addr *ina = 
					&IA_SNS(ifa)->sns_addr;

				if (ns_nullhost(*ina))
					ina->x_host = *(union ns_host *)
						LLADDR(ifp->if_sadl);
				else
					bcopy(ina->x_host.c_host,
					      LLADDR(ifp->if_sadl),
					      ifp->if_addrlen);
				/* Set new address. */
				eshinit(sc);
				break;
			}
#endif
		default:
			eshinit(sc);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			eshstop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
			   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.  But wait -- if the interface has
			 * just been reset, and is coming up again,
			 * don't reset it again!  There aren't yet
			 * any flags we care about.
			 * 
			 * The sc->sc_flags is set to INITIALIZED when
			 * the NIC has been reset;  the RUNCODE_UP
			 * flag gets added when it actually comes up.
			 * If it fails to come up in five seconds,
			 * it'll get shut down anyway.
			 */

			if (sc->sc_flags != ESH_FL_INITIALIZED)  {
				eshreset(sc);
			}
		}
		break;

	case SIOCSDRVSPEC: /* Driver-specific configuration calls */
	        cmd = ifd->ifd_cmd;
		len = ifd->ifd_len;
		data = ifd->ifd_data;

		/* XXX:  these functions need to check for suser! */
		
		switch (cmd) {
		case EIOCGTUNE:
			if (len != sizeof(struct rr_tuning))
				error = EMSGSIZE;
			else {
				error = copyout((caddr_t) &sc->sc_tune, data, 
						sizeof(struct rr_tuning));
			}
			break;

		case EIOCSTUNE:
			if ((ifp->if_flags & IFF_UP) == 0) {
				if (len != sizeof(struct rr_tuning)) {
					error = EMSGSIZE;
				} else {
					error = copyin(data, 
						    (caddr_t) &sc->sc_tune, 
						    sizeof(struct rr_tuning));
				}
			} else {
				error = EBUSY;
			}
			break;

		case EIOCGSTATS:
			if (len != sizeof(struct rr_stats))
				error = EMSGSIZE;
			else 
				error = copyout((caddr_t) &sc->sc_gen_info->ri_stats,
						data, sizeof(struct rr_stats));
			break;

		case EIOCGEEPROM:
		case EIOCSEEPROM:
			if ((ifp->if_flags & IFF_UP) != 0) {
				error = EBUSY;
				break;
			}

			if (len != sizeof(struct rr_eeprom)) {
				error = EMSGSIZE;
				break;
			}

			error = copyin(data, (caddr_t) &rr_eeprom, 
				       sizeof(rr_eeprom));
			if (error != 0)
				break;

			offset = rr_eeprom.ifr_offset;
			length = rr_eeprom.ifr_length;

			if (length > RR_EE_MAX_LEN * sizeof(u_int32_t)) {
				error = EFBIG;
				break;
			}
	    
			if (offset + length > 
			    RR_EE_MAX_LEN * sizeof(u_int32_t)) {
				error = EFAULT;
				break;
			}

			if (offset % 4 || length % 4) {
				error = EIO;
				break;
			}
	    
			/* Halt the processor (preserve NO_SWAP, if set) */

			misc_host_ctl = bus_space_read_4(iot, ioh, 
							 RR_MISC_HOST_CTL);
			bus_space_write_4(iot, ioh, RR_MISC_HOST_CTL, 
					  (misc_host_ctl & RR_MH_NO_SWAP) | 
					  RR_MH_HALT_PROC);

			/* Make the EEPROM accessable */

			misc_local_ctl = bus_space_read_4(iot, ioh, 
							  RR_MISC_LOCAL_CTL);
			value = misc_local_ctl & 
				~(RR_LC_FAST_PROM | RR_LC_ADD_SRAM | 
				  RR_LC_PARITY_ON);
			if (cmd == EIOCSEEPROM)   /* make writeable! */
				value |= RR_LC_WRITE_PROM;
			bus_space_write_4(iot, ioh, RR_MISC_LOCAL_CTL, value);
	    
			if (cmd == EIOCSEEPROM) {
				printf("%s:  writing EEPROM\n", 
				       sc->sc_dev.dv_xname);
				sc->sc_flags |= ESH_FL_EEPROM_BUSY;
				splx(s);
			}

			/* Do that EEPROM voodoo that you do so well... */

			address = offset * RR_EE_BYTE_LEN;
			for (i = 0; i < length; i += 4) {
				if (cmd == EIOCGEEPROM) {
					value = esh_read_eeprom(sc, address);
					address += RR_EE_WORD_LEN;
					if (copyout(&value, 
						    (caddr_t) rr_eeprom.ifr_buffer + i,
						    sizeof(u_int32_t)) != 0) {
						error = EFAULT;
						break;
					}
				} else {
					if (copyin((caddr_t) rr_eeprom.ifr_buffer + i,
						   &value, 
						   sizeof(u_int32_t)) != 0) {
						error = EFAULT;
						break;
					}
					if (esh_write_eeprom(sc, 
							     address, 
							     value) != 0) {
						error = EIO;
						break;
					}

					/* 
					 * Have to give up control now and 
					 * then, so sleep for a clock tick.
					 * Might be good to figure out how 
					 * long a tick is, so that we could 
					 * intelligently chose the frequency 
					 * of these pauses.
					 */

					if (i % 40 == 0) {
						tsleep((void *)&sc->sc_flags,
						    PRIBIO, "eshweeprom", 1);
					}
			
					address += RR_EE_WORD_LEN;
				}
			}		

			bus_space_write_4(iot, ioh, RR_MISC_LOCAL_CTL, 
					  misc_local_ctl);
			if (cmd == EIOCSEEPROM) {
				sc->sc_flags &= ~ESH_FL_EEPROM_BUSY;
				wakeup((void *)&sc->sc_flags);
				s = splnet();
				printf("%s:  done writing EEPROM\n", 
				       sc->sc_dev.dv_xname);
			}
			break;

		default:
			error = EINVAL;
			break;
		}
		break;
	default:
		error = EINVAL;
		break;
	}

ioctl_done:
	splx(s);
	return (error);
}


void
eshreset(sc)
	struct esh_softc *sc;
{
	int s;

	s = splnet();
	eshstop(sc);
	eshinit(sc);
	splx(s);
}

/*
 * The NIC expects a watchdog command every 10 seconds.  If it doesn't
 * get the watchdog, it figures the host is dead and stops.  When it does
 * get the command, it'll generate a watchdog event to let the host know
 * that it is still alive.  We watch for this.
 */

void
eshwatchdog(ifp)
	struct ifnet *ifp;
{
	struct esh_softc *sc = ifp->if_softc;

	if (!sc->sc_watchdog) {
		printf("%s:  watchdog timer expired.  "
		       "Should reset interface!\n",
		       sc->sc_dev.dv_xname);
		ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
		eshstatus(sc);
	} else {
		sc->sc_watchdog = 0;

		esh_send_cmd(sc, RR_CC_WATCHDOG, 0, 0);
		ifp->if_timer = 5;

		if (ifp->if_flags & IFF_LINK1) {
			esh_send_cmd(sc, RR_CC_UPDATE_STATS, 0, 0);
			eshstatus(sc);
		}
	}
}


/*
 * Stop the NIC and throw away packets that have started to be sent,
 * but didn't make it all the way.  Re-adjust the various queue
 * pointers to account for this.
 */

void
eshstop(sc)
	register struct esh_softc *sc;
{
	struct ifnet *ifp = &sc->sc_if;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct rr_ring_ctl *ring;
	struct esh_ring_ctl *ring_ctl;
	u_int32_t misc_host_ctl;

	if (!(sc->sc_flags & ESH_FL_INITIALIZED))
		return;

	/* Just shut it all down.  This isn't pretty, but it works */

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma, 0, sc->sc_dma_size,
			BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	misc_host_ctl = bus_space_read_4(iot, ioh, RR_MISC_HOST_CTL);
	bus_space_write_4(iot, ioh, RR_MISC_HOST_CTL, 
			  (misc_host_ctl & RR_MH_NO_SWAP) | RR_MH_HALT_PROC);

	ring = sc->sc_recv_ring_table + HIPPI_ULP_802;
	ring_ctl = &sc->sc_snap_recv;

	while (ring_ctl->ec_consumer != ring_ctl->ec_producer) {
		struct mbuf *m0;
		u_int16_t offset = ring_ctl->ec_consumer;
	
		bus_dmamap_unload(sc->sc_dmat, ring_ctl->ec_dma[offset]);
		MFREE(ring_ctl->ec_m[offset], m0);
		ring_ctl->ec_m[offset] = NULL;
		if (sc->sc_options & RR_OP_LONG_RX) 
			ring_ctl->ec_consumer = 
				NEXT_RECV2(ring_ctl->ec_consumer);
		else
			ring_ctl->ec_consumer =
				NEXT_RECV(ring_ctl->ec_consumer);
	}

	ring = &sc->sc_gen_info->ri_send_ring_ctl;
	ring_ctl = &sc->sc_send;

	while (ring_ctl->ec_consumer != ring_ctl->ec_producer) {
		struct mbuf *m0;
		u_int16_t offset = ring_ctl->ec_consumer;
	
		bus_dmamap_unload(sc->sc_dmat, ring_ctl->ec_dma[offset]);
		MFREE(ring_ctl->ec_m[offset], m0);
		ring_ctl->ec_m[offset] = NULL;
		if (sc->sc_options & RR_OP_LONG_TX)
			ring_ctl->ec_consumer = 
				NEXT_SEND2(ring_ctl->ec_consumer);
		else
			ring_ctl->ec_consumer =
				NEXT_SEND(ring_ctl->ec_consumer);
	}

	sc->sc_flags = 0;
	ifp->if_timer = 0;  /* turn off watchdog timer */
}


/*
 * Before reboots, reset card completely.
 */

static void
eshshutdown(arg)
	void *arg;
{
	register struct esh_softc *sc = arg;
	int i;

	esh_print_ring(sc, "Shutting Down:", &sc->sc_send, 0);
	esh_reset_runcode(sc);
	esh_init_snap_ring(sc);
	eshstop(sc);
	eshstatus(sc);
	eshshutdown(sc);
	for (i = 0; i < RR_SEND_RING_SIZE; i++)
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_send.ec_dma[i]);

	for (i = 0; i < RR_SNAP_RECV_RING_SIZE; i++)
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_snap_recv.ec_dma[i]);
}


/*
 * Read a value from the eeprom.  This expects that the NIC has already
 * been tweaked to put it into the right state for reading from the
 * EEPROM -- the HALT bit is set in the MISC_HOST_CTL register,
 * and the FAST_PROM, ADD_SRAM, and PARITY flags have been cleared
 * in the MISC_LOCAL_CTL register.
 *
 * The EEPROM layout is a little weird.  There is a valid byte every
 * eight bytes.  Words are then smeared out over 32 bytes.
 * All addresses listed here are the actual starting addresses.
 */

static u_int32_t 
esh_read_eeprom(sc, addr)
	struct esh_softc *sc;
	u_int32_t addr;
{
	int i;
	u_int32_t tmp;
	u_int32_t value = 0;

	/* If the offset hasn't been added, add it.  Otherwise pass through */

	if (!(addr & RR_EE_OFFSET))
		addr += RR_EE_OFFSET;

	for (i = 0; i < 4; i++, addr += RR_EE_BYTE_LEN) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, 
				  RR_WINDOW_BASE, addr);
		tmp = bus_space_read_4(sc->sc_iot, sc->sc_ioh, 
				       RR_WINDOW_DATA);
		value = (value << 8) | ((tmp >> 24) & 0xff);
	}
	return value;
}


/*
 * Write a value to the eeprom.  Just like esh_read_eeprom, this routine
 * expects that the NIC has already been tweaked to put it into the right 
 * state for reading from the EEPROM.  Things are further complicated
 * in that we need to read each byte after we write it to ensure that
 * the new value has been successfully written.  It can take as long
 * as 1ms (!) to write a byte.
 */

static int
esh_write_eeprom(sc, addr, value)
	struct esh_softc *sc;
	u_int32_t addr;
	u_int32_t value;
{
	int i, j;
	u_int32_t shifted_value, tmp;
 
	/* If the offset hasn't been added, add it.  Otherwise pass through */

	if (!(addr & RR_EE_OFFSET))
		addr += RR_EE_OFFSET;

	for (i = 0; i < 4; i++, addr += RR_EE_BYTE_LEN) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, 
				  RR_WINDOW_BASE, addr);

		/* 
		 * Get the byte out of value, starting with the top, and 
		 * put it into the top byte of the word to write.
		 */

		shifted_value = ((value >> ((3 - i) * 8)) & 0xff) << 24;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, RR_WINDOW_DATA, 
				  shifted_value);
		for (j = 0; j < 50; j++) {
			tmp = bus_space_read_4(sc->sc_iot, sc->sc_ioh, 
					       RR_WINDOW_DATA); 
			if (tmp == shifted_value)
				break;
			delay(500);  /* 50us break * 20 = 1ms */
		}
		if (tmp != shifted_value)
			return -1;
	}

	return 0;
}


/*
 * Send a command to the NIC.  If there is no room in the command ring,
 * panic.
 */

static void 
esh_send_cmd(sc, cmd, ring, index)
	struct esh_softc *sc;
	u_int8_t cmd;
	u_int8_t ring;
	u_int8_t index;
{
	union rr_cmd c;

#define NEXT_CMD(i) (((i) + 0x10 - 1) & 0x0f)

	if (ring && ring != 4)
		printf("esh_send_cmd:  bogus ring %x\n", ring);

	c.l = 0;

	c.b.rc_code = cmd;
	c.b.rc_ring = ring;
	c.b.rc_index = index;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, 
			  RR_COMMAND_RING + sizeof(c) * sc->sc_cmd_producer,
			  c.l);

#ifdef ESH_PRINTF
	/* avoid annoying messages when possible */
	if (cmd != RR_CC_WATCHDOG)
		printf("esh_send_cmd:  cmd %x ring %x index %x slot %x\n",
		       cmd, ring, index, sc->sc_cmd_producer);
#endif

	sc->sc_cmd_producer = NEXT_CMD(sc->sc_cmd_producer);
}


/*
 * Write an address to the device.
 * XXX:  This belongs in bus-dependent land!
 */

static void
esh_write_addr(iot, ioh, addr, value)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t addr;
	bus_addr_t value;
{
	bus_space_write_4(iot, ioh, addr, 0);
	bus_space_write_4(iot, ioh, addr + sizeof(u_int32_t), value);
}


/* Copy the RunCode from EEPROM to SRAM.  Ughly. */

static void
esh_reset_runcode(sc)
	struct esh_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int32_t value;
	u_int32_t len;
	u_int32_t i;
	u_int32_t segments;
	u_int32_t ee_addr;
	u_int32_t rc_addr;
	u_int32_t sram_addr;

	/* Zero the SRAM */

	for (i = 0; i < sc->sc_sram_size; i += 4) {
		bus_space_write_4(iot, ioh, RR_WINDOW_BASE, i);
		bus_space_write_4(iot, ioh, RR_WINDOW_DATA, 0);
	}

	/* Find the address of the segment description section */

	rc_addr = esh_read_eeprom(sc, RR_EE_RUNCODE_SEGMENTS);
	segments = esh_read_eeprom(sc, rc_addr);

	for (i = 0; i < segments; i++) {
		rc_addr += RR_EE_WORD_LEN;
		sram_addr = esh_read_eeprom(sc, rc_addr);
		rc_addr += RR_EE_WORD_LEN;
		len = esh_read_eeprom(sc, rc_addr);
		rc_addr += RR_EE_WORD_LEN;
		ee_addr = esh_read_eeprom(sc, rc_addr);
	
		while (len--) {
			value = esh_read_eeprom(sc, ee_addr);
			bus_space_write_4(iot, ioh, RR_WINDOW_BASE, sram_addr);
			bus_space_write_4(iot, ioh, RR_WINDOW_DATA, value);

			ee_addr += RR_EE_WORD_LEN;
			sram_addr += 4;
		}
	}
}


/*
 * Perform bus DMA syncing operations on various rings.
 * We have to worry about our relative position in the ring,
 * and whether the ring has wrapped.  All of this code should take
 * care of those worries.
 */

static void
esh_dma_sync(sc, mem, start, end, entries, size, do_equal, ops)
	struct esh_softc *sc;
	void *mem;
	int start;
	int end;
	int size;
	int do_equal;
	int ops;
{
	int offset = mem - (void *) sc->sc_dma_addr;

	if (start < end) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma,
				offset + start * size, 
				(end - start) * size, ops);
	} else if (do_equal || start != end) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma,
				offset, 
				end * size, ops);
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma,
				offset + start * size, 
				(entries - start) * size, ops);
	}
}

/* ------------------------- debugging functions --------------------------- */

/* 
 * Print out status information about the NIC and the driver.
 */

static int
eshstatus(sc)
	struct esh_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int i;

	/* XXX:   This looks pathetic, and should be improved! */

	printf("%s:  status -- fail1 %x fail2 %x\n",
	       sc->sc_dev.dv_xname, 
	       bus_space_read_4(iot, ioh, RR_RUNCODE_FAIL1),
	       bus_space_read_4(iot, ioh, RR_RUNCODE_FAIL2));
	printf("\tmisc host ctl %x  misc local ctl %x\n",
	       bus_space_read_4(iot, ioh, RR_MISC_HOST_CTL),
	       bus_space_read_4(iot, ioh, RR_MISC_LOCAL_CTL));
	printf("\toperating mode %x  event producer %x\n", 
	       bus_space_read_4(iot, ioh, RR_MODE_AND_STATUS),
	       bus_space_read_4(iot, ioh, RR_EVENT_PRODUCER));
	printf("\tPC %x  max rings %x\n", 
	       bus_space_read_4(iot, ioh, RR_PROC_PC),
	       bus_space_read_4(iot, ioh, RR_MAX_RECV_RINGS));
	printf("\tHIPPI tx state %x  rx state %x\n", 
	       bus_space_read_4(iot, ioh, RR_TX_STATE),
	       bus_space_read_4(iot, ioh, RR_RX_STATE));
	printf("\tDMA write state %x  read state %x\n", 
	       bus_space_read_4(iot, ioh, RR_DMA_WRITE_STATE),
	       bus_space_read_4(iot, ioh, RR_DMA_READ_STATE));
	printf("\tDMA write addr %x%x  read addr %x%x\n", 
	       bus_space_read_4(iot, ioh, RR_WRITE_HOST),
	       bus_space_read_4(iot, ioh, RR_WRITE_HOST + 4),
	       bus_space_read_4(iot, ioh, RR_READ_HOST),
	       bus_space_read_4(iot, ioh, RR_READ_HOST + 4));

	for (i = 0; i < 64; i++) 
		if (sc->sc_gen_info->ri_stats.rs_stats[i])
			printf("stat %x is %x\n", i * 4, 
			       sc->sc_gen_info->ri_stats.rs_stats[i]);

	return 0;
}

/*
 * Print out the contents of a ring
 */

static void
esh_print_ring(sc, name, ring, size)
	struct esh_softc *sc;
	char *name;
	struct esh_ring_ctl *ring;
	int size;
{
	int i;

	printf("%s ring:\n\n", name);

	for (i = 0; i < size; i++) {
		printf("%2x %c%c addr: %8x  len: %4x  ring: %2x  control %2x\n", 
		       i, 
		       (i == ring->ec_producer) ? 'P' : ' ',
		       (i == ring->ec_consumer) ? 'C' : ' ',
		       ring->ec_descr[i].rd_buffer_addr,
		       ring->ec_descr[i].rd_length,
		       ring->ec_descr[i].rd_ring,
		       ring->ec_descr[i].rd_control);
	}
}

#ifdef ESH_PRINTF

/* Check to make sure that the NIC is still running */

static int
esh_check(sc)
	struct esh_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	if (bus_space_read_4(iot, ioh, RR_MISC_HOST_CTL) & RR_MH_HALT_PROC) {
		printf("esh_check:  NIC stopped\n");
		eshstatus(sc);
		return 1;
	} else {
		return 0;
	}
}
#endif

