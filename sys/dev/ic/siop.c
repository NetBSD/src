/*	$NetBSD: siop.c,v 1.37.2.12 2001/04/03 15:32:58 bouyer Exp $	*/

/*
 * Copyright (c) 2000 Manuel Bouyer.
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
 *	This product includes software developed by Manuel Bouyer
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,     
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* SYM53c7/8xx PCI-SCSI I/O Processors driver */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/endian.h>
#include <machine/bus.h>

#include <dev/microcode/siop/siop.out>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsipi_all.h>

#include <dev/scsipi/scsiconf.h>

#include <dev/ic/siopreg.h>
#include <dev/ic/siopvar.h>
#include <dev/ic/siopvar_common.h>

#ifndef DEBUG
#undef DEBUG
#endif
#undef SIOP_DEBUG
#undef SIOP_DEBUG_DR
#undef SIOP_DEBUG_INTR
#undef SIOP_DEBUG_SCHED
#undef DUMP_SCRIPT

#define SIOP_STATS

#ifndef SIOP_DEFAULT_TARGET
#define SIOP_DEFAULT_TARGET 7
#endif

/* number of cmd descriptors per block */
#define SIOP_NCMDPB (PAGE_SIZE / sizeof(struct siop_xfer))

/* Number of scheduler slot (needs to match script) */
#define SIOP_NSLOTS 40

void	siop_reset __P((struct siop_softc *));
void	siop_handle_reset __P((struct siop_softc *));
int	siop_handle_qtag_reject __P((struct siop_cmd *));
void	siop_scsicmd_end __P((struct siop_cmd *));
void	siop_unqueue __P((struct siop_softc *, int, int));
static void	siop_start __P((struct siop_softc *, struct siop_cmd *));
void 	siop_timeout __P((void *));
int	siop_scsicmd __P((struct scsipi_xfer *));
void	siop_scsipi_request __P((struct scsipi_channel *,
			scsipi_adapter_req_t, void *));
void	siop_dump_script __P((struct siop_softc *));
int	siop_morecbd __P((struct siop_softc *));
struct siop_lunsw *siop_get_lunsw __P((struct siop_softc *));
void	siop_add_reselsw __P((struct siop_softc *, int));
void	siop_update_scntl3 __P((struct siop_softc *, struct siop_target *));

#ifdef SIOP_STATS
static int siop_stat_intr = 0;
static int siop_stat_intr_shortxfer = 0;
static int siop_stat_intr_sdp = 0;
static int siop_stat_intr_done = 0;
static int siop_stat_intr_xferdisc = 0;
static int siop_stat_intr_lunresel = 0;
static int siop_stat_intr_qfull = 0;
void siop_printstats __P((void));
#define INCSTAT(x) x++
#else
#define INCSTAT(x) 
#endif

static __inline__ void siop_script_sync __P((struct siop_softc *, int));
static __inline__ void
siop_script_sync(sc, ops)
	struct siop_softc *sc;
	int ops;
{
	if ((sc->features & SF_CHIP_RAM) == 0)
		bus_dmamap_sync(sc->sc_dmat, sc->sc_scriptdma, 0,
		    PAGE_SIZE, ops);
}

static __inline__ u_int32_t siop_script_read __P((struct siop_softc *, u_int));
static __inline__ u_int32_t
siop_script_read(sc, offset)
	struct siop_softc *sc;
	u_int offset;
{
	if (sc->features & SF_CHIP_RAM) {
		return bus_space_read_4(sc->sc_ramt, sc->sc_ramh, offset * 4);
	} else {
		return le32toh(sc->sc_script[offset]);
	}
}

static __inline__ void siop_script_write __P((struct siop_softc *, u_int,
	u_int32_t));
static __inline__ void
siop_script_write(sc, offset, val)
	struct siop_softc *sc;
	u_int offset;
	u_int32_t val;
{
	if (sc->features & SF_CHIP_RAM) {
		bus_space_write_4(sc->sc_ramt, sc->sc_ramh, offset * 4, val);
	} else {
		sc->sc_script[offset] = htole32(val);
	}
}

void
siop_attach(sc)
	struct siop_softc *sc;
{
	int error, i;
	bus_dma_segment_t seg;
	int rseg;

	/*
	 * Allocate DMA-safe memory for the script and map it.
	 */
	if ((sc->features & SF_CHIP_RAM) == 0) {
		error = bus_dmamem_alloc(sc->sc_dmat, PAGE_SIZE, 
		    PAGE_SIZE, 0, &seg, 1, &rseg, BUS_DMA_NOWAIT);
		if (error) {
			printf("%s: unable to allocate script DMA memory, "
			    "error = %d\n", sc->sc_dev.dv_xname, error);
			return;
		}
		error = bus_dmamem_map(sc->sc_dmat, &seg, rseg, PAGE_SIZE,
		    (caddr_t *)&sc->sc_script, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
		if (error) {
			printf("%s: unable to map script DMA memory, "
			    "error = %d\n", sc->sc_dev.dv_xname, error);
			return;
		}
		error = bus_dmamap_create(sc->sc_dmat, PAGE_SIZE, 1,
		    PAGE_SIZE, 0, BUS_DMA_NOWAIT, &sc->sc_scriptdma);
		if (error) {
			printf("%s: unable to create script DMA map, "
			    "error = %d\n", sc->sc_dev.dv_xname, error);
			return;
		}
		error = bus_dmamap_load(sc->sc_dmat, sc->sc_scriptdma,
		    sc->sc_script, PAGE_SIZE, NULL, BUS_DMA_NOWAIT);
		if (error) {
			printf("%s: unable to load script DMA map, "
			    "error = %d\n", sc->sc_dev.dv_xname, error);
			return;
		}
		sc->sc_scriptaddr = sc->sc_scriptdma->dm_segs[0].ds_addr;
		sc->ram_size = PAGE_SIZE;
	}
	TAILQ_INIT(&sc->free_list);
	TAILQ_INIT(&sc->cmds);
	TAILQ_INIT(&sc->lunsw_list);
	sc->sc_currschedslot = 0;
#ifdef SIOP_DEBUG
	printf("%s: script size = %d, PHY addr=0x%x, VIRT=%p\n",
	    sc->sc_dev.dv_xname, (int)sizeof(siop_script),
	    (u_int32_t)sc->sc_scriptaddr, sc->sc_script);
#endif

	sc->sc_adapt.adapt_dev = &sc->sc_dev;
	sc->sc_adapt.adapt_nchannels = 1;
	sc->sc_adapt.adapt_openings = 225;
	sc->sc_adapt.adapt_max_periph = SIOP_NTAG - 1;
	sc->sc_adapt.adapt_ioctl = siop_ioctl;
	sc->sc_adapt.adapt_minphys = minphys;
	sc->sc_adapt.adapt_request = siop_scsipi_request;

	memset(&sc->sc_chan, 0, sizeof(sc->sc_chan));
	sc->sc_chan.chan_adapter = &sc->sc_adapt;
	sc->sc_chan.chan_bustype = &scsi_bustype;
	sc->sc_chan.chan_channel = 0;
	sc->sc_chan.chan_ntargets = (sc->features & SF_BUS_WIDE) ? 16 : 8;
	sc->sc_chan.chan_nluns = 8;
	sc->sc_chan.chan_id = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SCID);
	if (sc->sc_chan.chan_id == 0 ||
	    sc->sc_chan.chan_id >= sc->sc_chan.chan_ntargets)
		sc->sc_chan.chan_id = SIOP_DEFAULT_TARGET;

	for (i = 0; i < 16; i++)
		sc->targets[i] = NULL;

	/* find min/max sync period for this chip */
	sc->maxsync = 0;
	sc->minsync = 255;
	for (i = 0; i < sizeof(scf_period) / sizeof(scf_period[0]); i++) {
		if (sc->clock_period != scf_period[i].clock)
			continue;
		if (sc->maxsync < scf_period[i].period)
			sc->maxsync = scf_period[i].period;
		if (sc->minsync > scf_period[i].period)
			sc->minsync = scf_period[i].period;
	}
	if (sc->maxsync == 255 || sc->minsync == 0)
		panic("siop: can't find my sync parameters\n");
	/* Do a bus reset, so that devices fall back to narrow/async */
	siop_resetbus(sc);
	/*
	 * siop_reset() will reset the chip, thus clearing pending interrupts
	 */
	siop_reset(sc);
#ifdef DUMP_SCRIPT
	siop_dump_script(sc);
#endif

	config_found((struct device*)sc, &sc->sc_chan, scsiprint);
}

void
siop_reset(sc)
	struct siop_softc *sc;
{
	int i, j;
	struct siop_lunsw *lunsw;

	siop_common_reset(sc);

	/* copy and patch the script */
	if (sc->features & SF_CHIP_RAM) {
		bus_space_write_region_4(sc->sc_ramt, sc->sc_ramh, 0,
		    siop_script, sizeof(siop_script) / sizeof(siop_script[0]));
		for (j = 0; j <
		    (sizeof(E_abs_msgin_Used) / sizeof(E_abs_msgin_Used[0]));
		    j++) {
			bus_space_write_4(sc->sc_ramt, sc->sc_ramh,
			    E_abs_msgin_Used[j] * 4,
			    sc->sc_scriptaddr + Ent_msgin_space);
		}
	} else {
		for (j = 0;
		    j < (sizeof(siop_script) / sizeof(siop_script[0])); j++) {
			sc->sc_script[j] = htole32(siop_script[j]);
		}
		for (j = 0; j <
		    (sizeof(E_abs_msgin_Used) / sizeof(E_abs_msgin_Used[0]));
		    j++) {
			sc->sc_script[E_abs_msgin_Used[j]] =
			    htole32(sc->sc_scriptaddr + Ent_msgin_space);
		}
	}
	sc->script_free_lo = sizeof(siop_script) / sizeof(siop_script[0]);
	sc->script_free_hi = sc->ram_size / 4;

	/* free used and unused lun switches */
	while((lunsw = TAILQ_FIRST(&sc->lunsw_list)) != NULL) {
#ifdef SIOP_DEBUG
		printf("%s: free lunsw at offset %d\n",
				sc->sc_dev.dv_xname, lunsw->lunsw_off);
#endif
		TAILQ_REMOVE(&sc->lunsw_list, lunsw, next);
		free(lunsw, M_DEVBUF);
	}
	TAILQ_INIT(&sc->lunsw_list);
	/* restore reselect switch */
	for (i = 0; i < sc->sc_chan.chan_ntargets; i++) {
		if (sc->targets[i] == NULL)
			continue;
#ifdef SIOP_DEBUG
		printf("%s: restore sw for target %d\n",
				sc->sc_dev.dv_xname, i);
#endif
		free(sc->targets[i]->lunsw, M_DEVBUF);
		sc->targets[i]->lunsw = siop_get_lunsw(sc);
		if (sc->targets[i]->lunsw == NULL) {
			printf("%s: can't alloc lunsw for target %d\n",
			    sc->sc_dev.dv_xname, i);
			break;
		}
		siop_add_reselsw(sc, i);
	}

	/* start script */
	if ((sc->features & SF_CHIP_RAM) == 0) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_scriptdma, 0, PAGE_SIZE,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	}
	bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSP,
	    sc->sc_scriptaddr + Ent_reselect);
}

#if 0
#define CALL_SCRIPT(ent) do {\
	printf ("start script DSA 0x%lx DSP 0x%lx\n", \
	    siop_cmd->dsa, \
	    sc->sc_scriptaddr + ent); \
bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSP, sc->sc_scriptaddr + ent); \
} while (0)
#else
#define CALL_SCRIPT(ent) do {\
bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSP, sc->sc_scriptaddr + ent); \
} while (0)
#endif

int
siop_intr(v)
	void *v;
{
	struct siop_softc *sc = v;
	struct siop_target *siop_target;
	struct siop_cmd *siop_cmd;
	struct siop_lun *siop_lun;
	struct scsipi_xfer *xs;
	int istat, sist, sstat1, dstat;
	u_int32_t irqcode;
	int need_reset = 0;
	int offset, target, lun, tag;
	bus_addr_t dsa;
	struct siop_cbd *cbdp;
	int freetarget = 0;
	int restart = 0;

	istat = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_ISTAT);
	if ((istat & (ISTAT_INTF | ISTAT_DIP | ISTAT_SIP)) == 0)
		return 0;
	INCSTAT(siop_stat_intr);
	if (istat & ISTAT_INTF) {
		printf("INTRF\n");
		bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_ISTAT, ISTAT_INTF);
	}
	/* use DSA to find the current siop_cmd */
	dsa = bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSA);
	for (cbdp = TAILQ_FIRST(&sc->cmds); cbdp != NULL;
	    cbdp = TAILQ_NEXT(cbdp, next)) {
		if (dsa >= cbdp->xferdma->dm_segs[0].ds_addr &&
	    	    dsa < cbdp->xferdma->dm_segs[0].ds_addr + PAGE_SIZE) {
			dsa -= cbdp->xferdma->dm_segs[0].ds_addr;
			siop_cmd = &cbdp->cmds[dsa / sizeof(struct siop_xfer)];
			siop_table_sync(siop_cmd,
			    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
			break;
		}
	} 
	if (cbdp == NULL) {
		siop_cmd = NULL;
	}
	if (siop_cmd) {
		xs = siop_cmd->xs;
		siop_target = siop_cmd->siop_target;
		target = siop_cmd->xs->xs_periph->periph_target;
		lun = siop_cmd->xs->xs_periph->periph_lun;
		tag = siop_cmd->tag;
		siop_lun = siop_target->siop_lun[lun];
#ifdef DIAGNOSTIC
		if (siop_cmd->status != CMDST_ACTIVE) {
			printf("siop_cmd (lun %d) not active (%d)\n",
				lun, siop_cmd->status);
			xs = NULL;
			siop_target = NULL;
			target = -1;
			lun = -1;
			tag = -1;
			siop_lun = NULL;
			siop_cmd = NULL;
		} else if (siop_lun->siop_tag[tag].active != siop_cmd) {
			printf("siop_cmd (lun %d tag %d) not in siop_lun "
			    "active (%p != %p)\n", lun, tag, siop_cmd,
			    siop_lun->siop_tag[tag].active);
		}
#endif
	} else {
		xs = NULL;
		siop_target = NULL;
		target = -1;
		lun = -1;
		tag = -1;
		siop_lun = NULL;
	}
	if (istat & ISTAT_DIP) {
		dstat = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_DSTAT);
		if (dstat & DSTAT_SSI) {
			printf("single step dsp 0x%08x dsa 0x08%x\n",
			    (int)(bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSP) -
			    sc->sc_scriptaddr),
			    bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSA));
			if ((dstat & ~(DSTAT_DFE | DSTAT_SSI)) == 0 &&
			    (istat & ISTAT_SIP) == 0) {
				bus_space_write_1(sc->sc_rt, sc->sc_rh,
				    SIOP_DCNTL, bus_space_read_1(sc->sc_rt,
				    sc->sc_rh, SIOP_DCNTL) | DCNTL_STD);
			}
			return 1;
		}
		if (dstat & ~(DSTAT_SIR | DSTAT_DFE | DSTAT_SSI)) {
		printf("DMA IRQ:");
		if (dstat & DSTAT_IID)
			printf(" Illegal instruction");
		if (dstat & DSTAT_ABRT)
			printf(" abort");
		if (dstat & DSTAT_BF)
			printf(" bus fault");
		if (dstat & DSTAT_MDPE)
			printf(" parity");
		if (dstat & DSTAT_DFE)
			printf(" dma fifo empty");
		printf(", DSP=0x%x DSA=0x%x: ",
		    (int)(bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSP) -
		    sc->sc_scriptaddr),
		    bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSA));
		if (siop_cmd)
			printf("last msg_in=0x%x status=0x%x\n",
			    siop_cmd->siop_tables.msg_in[0],
			    le32toh(siop_cmd->siop_tables.status));
		else 
			printf("%s: current DSA invalid\n",
			    sc->sc_dev.dv_xname);
		need_reset = 1;
		}
	}
	if (istat & ISTAT_SIP) {
		if (istat & ISTAT_DIP)
			delay(10);
		/*
		 * Can't read sist0 & sist1 independantly, or we have to
		 * insert delay
		 */
		sist = bus_space_read_2(sc->sc_rt, sc->sc_rh, SIOP_SIST0);
		sstat1 = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SSTAT1);
#ifdef SIOP_DEBUG_INTR
		printf("scsi interrupt, sist=0x%x sstat1=0x%x "
		    "DSA=0x%x DSP=0x%lx\n", sist,
		    bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SSTAT1),
		    bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSA),
		    (u_long)(bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSP) -
		    sc->sc_scriptaddr));
#endif
		if (sist & SIST0_RST) {
			siop_handle_reset(sc);
			/* no table to flush here */
			return 1;
		}
		if (sist & SIST0_SGE) {
			if (siop_cmd)
				scsipi_printaddr(xs->xs_periph);
			else
				printf("%s:", sc->sc_dev.dv_xname);
			printf("scsi gross error\n");
			goto reset;
		}
		if ((sist & SIST0_MA) && need_reset == 0) {
			if (siop_cmd) { 
				int scratcha0;
				dstat = bus_space_read_1(sc->sc_rt, sc->sc_rh,
				    SIOP_DSTAT);
				/*
				 * first restore DSA, in case we were in a S/G
				 * operation.
				 */
				bus_space_write_4(sc->sc_rt, sc->sc_rh,
				    SIOP_DSA, siop_cmd->dsa);
				scratcha0 = bus_space_read_1(sc->sc_rt,
				    sc->sc_rh, SIOP_SCRATCHA);
				switch (sstat1 & SSTAT1_PHASE_MASK) {
				case SSTAT1_PHASE_STATUS:
				/*
				 * previous phase may be aborted for any reason
				 * ( for example, the target has less data to
				 * transfer than requested). Just go to status
				 * and the command should terminate.
				 */
					INCSTAT(siop_stat_intr_shortxfer);
					if ((dstat & DSTAT_DFE) == 0)
						siop_clearfifo(sc);
					/* no table to flush here */
					CALL_SCRIPT(Ent_status);
					return 1;
				case SSTAT1_PHASE_MSGIN:
					/*
					 * target may be ready to disconnect
					 * Save data pointers just in case.
					 */
					INCSTAT(siop_stat_intr_xferdisc);
					if (scratcha0 & A_flag_data)
						siop_sdp(siop_cmd);
					else if ((dstat & DSTAT_DFE) == 0)
						siop_clearfifo(sc);
					bus_space_write_1(sc->sc_rt, sc->sc_rh,
					    SIOP_SCRATCHA,
					    scratcha0 & ~A_flag_data);
					siop_table_sync(siop_cmd,
					    BUS_DMASYNC_PREREAD |
					    BUS_DMASYNC_PREWRITE);
					CALL_SCRIPT(Ent_msgin);
					return 1;
				}
				printf("%s: unexpected phase mismatch %d\n",
				    sc->sc_dev.dv_xname,
				    sstat1 & SSTAT1_PHASE_MASK);
			} else {
				printf("%s: phase mismatch without command\n",
				    sc->sc_dev.dv_xname);
			}
			need_reset = 1;
		}
		if (sist & SIST0_PAR) {
			/* parity error, reset */
			if (siop_cmd)
				scsipi_printaddr(xs->xs_periph);
			else
				printf("%s:", sc->sc_dev.dv_xname);
			printf("parity error\n");
			goto reset;
		}
		if ((sist & (SIST1_STO << 8)) && need_reset == 0) {
			/* selection time out, assume there's no device here */
			if (siop_cmd) {
				siop_cmd->status = CMDST_DONE;
				xs->error = XS_SELTIMEOUT;
				freetarget = 1;
				goto end;
			} else {
				printf("%s: selection timeout without "
				    "command\n", sc->sc_dev.dv_xname);
				need_reset = 1;
			}
		}
		if (sist & SIST0_UDC) {
			/*
			 * unexpected disconnect. Usually the target signals
			 * a fatal condition this way. Attempt to get sense.
			 */
			 if (siop_cmd) {
				siop_cmd->siop_tables.status =
				    htole32(SCSI_CHECK);
				goto end;
			}
			printf("%s: unexpected disconnect without "
			    "command\n", sc->sc_dev.dv_xname);
			goto reset;
		}
		if (sist & (SIST1_SBMC << 8)) {
			/* SCSI bus mode change */
			if (siop_modechange(sc) == 0 || need_reset == 1)
				goto reset;
			if ((istat & ISTAT_DIP) && (dstat & DSTAT_SIR)) {
				/*
				 * we have a script interrupt, it will
				 * restart the script.
				 */
				goto scintr;
			}
			/*
			 * else we have to restart it ourselve, at the
			 * interrupted instruction.
			 */
			bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSP,
			    bus_space_read_4(sc->sc_rt, sc->sc_rh,
			    SIOP_DSP) - 8);
			return 1;
		}
		/* Else it's an unhandled exeption (for now). */
		printf("%s: unhandled scsi interrupt, sist=0x%x sstat1=0x%x "
		    "DSA=0x%x DSP=0x%x\n", sc->sc_dev.dv_xname, sist,
		    bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SSTAT1),
		    bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSA),
		    (int)(bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSP) -
		    sc->sc_scriptaddr));
		if (siop_cmd) {
			siop_cmd->status = CMDST_DONE;
			xs->error = XS_SELTIMEOUT;
			goto end;
		}
		need_reset = 1;
	}
	if (need_reset) {
reset:
		/* fatal error, reset the bus */
		siop_resetbus(sc);
		/* no table to flush here */
		return 1;
	}

scintr:
	if ((istat & ISTAT_DIP) && (dstat & DSTAT_SIR)) { /* script interrupt */
		irqcode = bus_space_read_4(sc->sc_rt, sc->sc_rh,
		    SIOP_DSPS);
#ifdef SIOP_DEBUG_INTR
		printf("script interrupt 0x%x\n", irqcode);
#endif
		/*
		 * no command, or an inactive command is only valid for a
		 * reselect interrupt
		 */
		if ((irqcode & 0x80) == 0) {
			if (siop_cmd == NULL) {
				printf("%s: script interrupt (0x%x) with
				    invalid DSA !!!\n", sc->sc_dev.dv_xname,
				    irqcode);
				goto reset;
			}
			if (siop_cmd->status != CMDST_ACTIVE) {
				printf("%s: command with invalid status "
				    "(IRQ code 0x%x current status %d) !\n",
				    sc->sc_dev.dv_xname,
				    irqcode, siop_cmd->status);
				xs = NULL;
			}
		}
		switch(irqcode) {
		case A_int_err:
			printf("error, DSP=0x%x\n",
			    (int)(bus_space_read_4(sc->sc_rt, sc->sc_rh,
			    SIOP_DSP) - sc->sc_scriptaddr));
			if (xs) {
				xs->error = XS_SELTIMEOUT;
				goto end;
			} else {
				goto reset;
			}
		case A_int_reseltarg:
			printf("%s: reselect with invalid target\n",
				    sc->sc_dev.dv_xname);
			goto reset;
		case A_int_resellun: 
			INCSTAT(siop_stat_intr_lunresel);
			target = bus_space_read_1(sc->sc_rt, sc->sc_rh,
			    SIOP_SCRATCHA) & 0xf;
			lun = bus_space_read_1(sc->sc_rt, sc->sc_rh,
			    SIOP_SCRATCHA + 1);
			tag = bus_space_read_1(sc->sc_rt, sc->sc_rh,
			    SIOP_SCRATCHA + 2);
			siop_target = sc->targets[target];
			if (siop_target == NULL) {
				printf("%s: reselect with invalid "
				    "target %d\n", sc->sc_dev.dv_xname, target);
				goto reset;
			}
			siop_lun = siop_target->siop_lun[lun];
			if (siop_lun == NULL) {
				printf("%s: target %d reselect with invalid "
				    "lun %d\n", sc->sc_dev.dv_xname,
				    target, lun);
				goto reset;
			}
			if (siop_lun->siop_tag[tag].active == NULL) {
				printf("%s: target %d lun %d tag %d reselect "
				    "without command\n", sc->sc_dev.dv_xname,
				    target, lun, tag);
				goto reset;
			}
			siop_cmd = siop_lun->siop_tag[tag].active;
			bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSP,
			    siop_cmd->dsa + sizeof(struct siop_xfer_common) +
			    Ent_ldsa_reload_dsa);
			siop_table_sync(siop_cmd, BUS_DMASYNC_PREWRITE);
			return 1;
		case A_int_reseltag:
			printf("%s: reselect with invalid tag\n",
				    sc->sc_dev.dv_xname);
			goto reset;
		case A_int_msgin:
		{
			int msgin = bus_space_read_1(sc->sc_rt, sc->sc_rh,
			    SIOP_SFBR);
			if (msgin == MSG_MESSAGE_REJECT) {
				int msg, extmsg;
				if (siop_cmd->siop_tables.msg_out[0] & 0x80) {
					/*
					 * message was part of a identify +
					 * something else. Identify shoudl't
					 * have been rejected.
					 */
					msg = siop_cmd->siop_tables.msg_out[1];
					extmsg =
					    siop_cmd->siop_tables.msg_out[3];
				} else {
					msg = siop_cmd->siop_tables.msg_out[0];
					extmsg =
					    siop_cmd->siop_tables.msg_out[2];
				}
				if (msg == MSG_MESSAGE_REJECT) {
					/* MSG_REJECT  for a MSG_REJECT  !*/
					if (xs)
						scsipi_printaddr(xs->xs_periph);
					else
						printf("%s: ",
						   sc->sc_dev.dv_xname);
					printf("our reject message was "
					    "rejected\n");
					goto reset;
				}
				if (msg == MSG_EXTENDED &&
				    extmsg == MSG_EXT_WDTR) {
					/* WDTR rejected, initiate sync */
					if ((siop_target->flags & TARF_SYNC)
					    == 0) {
						siop_target->status = TARST_OK;
						siop_update_xfer_mode(sc,
						    target);
						/* no table to flush here */
						CALL_SCRIPT(Ent_msgin_ack);
						return 1;
					}
					siop_target->status = TARST_SYNC_NEG;
					siop_sdtr_msg(siop_cmd, 0,
					    sc->minsync, sc->maxoff);
					siop_table_sync(siop_cmd,
					    BUS_DMASYNC_PREREAD |
					    BUS_DMASYNC_PREWRITE);
					CALL_SCRIPT(Ent_send_msgout);
					return 1;
				} else if (msg == MSG_EXTENDED &&
				    extmsg == MSG_EXT_SDTR) {
					/* sync rejected */
					siop_target->offset = 0;
					siop_target->period = 0;
					siop_target->status = TARST_OK;
					siop_update_xfer_mode(sc, target);
					/* no table to flush here */
					CALL_SCRIPT(Ent_msgin_ack);
					return 1;
				} else if (msg == MSG_SIMPLE_Q_TAG || 
				    msg == MSG_HEAD_OF_Q_TAG ||
				    msg == MSG_ORDERED_Q_TAG) {
					if (siop_handle_qtag_reject(
					    siop_cmd) == -1)
						goto reset;
					CALL_SCRIPT(Ent_msgin_ack);
					return 1;
				}
				if (xs)
					scsipi_printaddr(xs->xs_periph);
				else
					printf("%s: ", sc->sc_dev.dv_xname);
				if (msg == MSG_EXTENDED) {
					printf("scsi message reject, extended "
					    "message sent was 0x%x\n", extmsg);
				} else {
					printf("scsi message reject, message "
					    "sent was 0x%x\n", msg);
				}
				/* no table to flush here */
				CALL_SCRIPT(Ent_msgin_ack);
				return 1;
			}
			if (xs)
				scsipi_printaddr(xs->xs_periph);
			else
				printf("%s: ", sc->sc_dev.dv_xname);
			printf("unhandled message 0x%x\n",
			    siop_cmd->siop_tables.msg_in[0]);
			siop_cmd->siop_tables.msg_out[0] = MSG_MESSAGE_REJECT;
			siop_cmd->siop_tables.t_msgout.count= htole32(1);
			siop_table_sync(siop_cmd,
			    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
			CALL_SCRIPT(Ent_send_msgout);
			return 1;
		}
		case A_int_extmsgin:
#ifdef SIOP_DEBUG_INTR
			printf("extended message: msg 0x%x len %d\n",
			    siop_cmd->siop_tables.msg_in[2], 
			    siop_cmd->siop_tables.msg_in[1]);
#endif
			if (siop_cmd->siop_tables.msg_in[1] > 6)
				printf("%s: extended message too big (%d)\n",
				    sc->sc_dev.dv_xname,
				    siop_cmd->siop_tables.msg_in[1]);
			siop_cmd->siop_tables.t_extmsgdata.count =
			    htole32(siop_cmd->siop_tables.msg_in[1] - 1);
			siop_table_sync(siop_cmd,
			    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
			CALL_SCRIPT(Ent_get_extmsgdata);
			return 1;
		case A_int_extmsgdata:
#ifdef SIOP_DEBUG_INTR
			{
			int i;
			printf("extended message: 0x%x, data:",
			    siop_cmd->siop_tables.msg_in[2]);
			for (i = 3; i < 2 + siop_cmd->siop_tables.msg_in[1];
			    i++)
				printf(" 0x%x",
				    siop_cmd->siop_tables.msg_in[i]);
			printf("\n");
			}
#endif
			if (siop_cmd->siop_tables.msg_in[2] == MSG_EXT_WDTR) {
				switch (siop_wdtr_neg(siop_cmd)) {
				case SIOP_NEG_MSGOUT:
					siop_update_scntl3(sc,
					    siop_cmd->siop_target);
					siop_table_sync(siop_cmd,
					    BUS_DMASYNC_PREREAD |
					    BUS_DMASYNC_PREWRITE);
					CALL_SCRIPT(Ent_send_msgout);
					return(1);
				case SIOP_NEG_ACK:
					siop_update_scntl3(sc,
					    siop_cmd->siop_target);
					CALL_SCRIPT(Ent_msgin_ack);
					return(1);
				default:
					panic("invalid retval from "
					    "siop_wdtr_neg()");
				}
				return(1);
			}
			if (siop_cmd->siop_tables.msg_in[2] == MSG_EXT_SDTR) {
				switch (siop_sdtr_neg(siop_cmd)) {
				case SIOP_NEG_MSGOUT:
					siop_update_scntl3(sc,
					    siop_cmd->siop_target);
					siop_table_sync(siop_cmd,
					    BUS_DMASYNC_PREREAD |
					    BUS_DMASYNC_PREWRITE);
					CALL_SCRIPT(Ent_send_msgout);
					return(1);
				case SIOP_NEG_ACK:
					siop_update_scntl3(sc,
					    siop_cmd->siop_target);
					CALL_SCRIPT(Ent_msgin_ack);
					return(1);
				default:
					panic("invalid retval from "
					    "siop_wdtr_neg()");
				}
				return(1);
			}
			/* send a message reject */
			siop_cmd->siop_tables.msg_out[0] = MSG_MESSAGE_REJECT;
			siop_cmd->siop_tables.t_msgout.count = htole32(1);
			siop_table_sync(siop_cmd,
			    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
			CALL_SCRIPT(Ent_send_msgout);
			return 1;
		case A_int_disc:
			INCSTAT(siop_stat_intr_sdp);
			offset = bus_space_read_1(sc->sc_rt, sc->sc_rh,
			    SIOP_SCRATCHA + 1);
#ifdef SIOP_DEBUG_DR
			printf("disconnect offset %d\n", offset);
#endif
			if (offset > SIOP_NSG) {
				printf("%s: bad offset for disconnect (%d)\n",
				    sc->sc_dev.dv_xname, offset);
				goto reset;
			}
			/* 
			 * offset == SIOP_NSG may be a valid condition if
			 * we get a sdp when the xfer is done.
			 * Don't call memmove in this case.
			 */
			if (offset < SIOP_NSG) {
				memmove(&siop_cmd->siop_tables.data[0],
				    &siop_cmd->siop_tables.data[offset],
				    (SIOP_NSG - offset) * sizeof(scr_table_t));
				siop_table_sync(siop_cmd,
				    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
			}
			CALL_SCRIPT(Ent_script_sched);
			return 1;
		case A_int_resfail:
			printf("reselect failed\n");
			CALL_SCRIPT(Ent_script_sched);
			return  1;
		case A_int_done:
			if (xs == NULL) {
				printf("%s: done without command, DSA=0x%lx\n",
				    sc->sc_dev.dv_xname, (u_long)siop_cmd->dsa);
				siop_cmd->status = CMDST_FREE;
				CALL_SCRIPT(Ent_script_sched);
				return 1;
			}
#ifdef SIOP_DEBUG_INTR
			printf("done, DSA=0x%lx target id 0x%x last msg "
			    "in=0x%x status=0x%x\n", (u_long)siop_cmd->dsa,
			    le32toh(siop_cmd->siop_tables.id),
			    siop_cmd->siop_tables.msg_in[0],
			    le32toh(siop_cmd->siop_tables.status));
#endif
			INCSTAT(siop_stat_intr_done);
			siop_cmd->status = CMDST_DONE;
			goto end;
		default:
			printf("unknown irqcode %x\n", irqcode);
			if (xs) {
				xs->error = XS_SELTIMEOUT;
				goto end;
			}
			goto reset;
		}
		return 1;
	}
	/* We just should't get there */
	panic("siop_intr: I shouldn't be there !");
	return 1;
end:
	/*
	 * restart the script now if command completed properly
	 * Otherwise wait for siop_scsicmd_end(), we may need to cleanup the
	 * queue
	 */
	xs->status = le32toh(siop_cmd->siop_tables.status);
	if (xs->status == SCSI_OK)
		CALL_SCRIPT(Ent_script_sched);
	else
		restart = 1;
	siop_lun->siop_tag[tag].active = NULL;
	if (sc->sc_flags & SCF_CHAN_NOSLOT) {
		/* a command terminated, so we have free slots now */
		sc->sc_flags &= ~SCF_CHAN_NOSLOT;
		scsipi_channel_thaw(&sc->sc_chan, 1);
	}
	siop_scsicmd_end(siop_cmd);
	if (freetarget && siop_target->status == TARST_PROBING)
		siop_del_dev(sc, target, lun);
	if (restart)
		CALL_SCRIPT(Ent_script_sched);
		
	return 1;
}

void
siop_scsicmd_end(siop_cmd)
	struct siop_cmd *siop_cmd;
{
	struct scsipi_xfer *xs = siop_cmd->xs;
	struct siop_softc *sc = siop_cmd->siop_sc;

	switch(xs->status) {
	case SCSI_OK:
		xs->error = XS_NOERROR;
		break;
	case SCSI_BUSY:
		xs->error = XS_BUSY;
		break;
	case SCSI_CHECK:
		xs->error = XS_BUSY;
		/* remove commands in the queue and scheduler */
		siop_unqueue(sc, xs->xs_periph->periph_target,
		    xs->xs_periph->periph_lun);
		break;
	case SCSI_QUEUE_FULL:
		INCSTAT(siop_stat_intr_qfull);
#ifdef SIOP_DEBUG
		printf("%s:%d:%d: queue full (tag %d)\n", sc->sc_dev.dv_xname,
		    xs->xs_periph->periph_target,
		    xs->xs_periph->periph_lun, siop_cmd->tag);
#endif
		xs->error = XS_BUSY;
		break;
	case SCSI_SIOP_NOCHECK:
		/*
		 * don't check status, xs->error is already valid
		 */
		break;
	case SCSI_SIOP_NOSTATUS:
		/*
		 * the status byte was not updated, cmd was
		 * aborted
		 */
		xs->error = XS_SELTIMEOUT;
		break;
	default:
		xs->error = XS_DRIVER_STUFFUP;
	}
	if (xs->xs_control & (XS_CTL_DATA_IN | XS_CTL_DATA_OUT)) {
		bus_dmamap_sync(sc->sc_dmat, siop_cmd->dmamap_data, 0,
		    siop_cmd->dmamap_data->dm_mapsize,
		    (xs->xs_control & XS_CTL_DATA_IN) ?
		    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, siop_cmd->dmamap_data);
	}
	bus_dmamap_unload(sc->sc_dmat, siop_cmd->dmamap_cmd);
	callout_stop(&siop_cmd->xs->xs_callout);
	siop_cmd->status = CMDST_FREE;
	TAILQ_INSERT_TAIL(&sc->free_list, siop_cmd, next);
	xs->resid = 0;
	scsipi_done (xs);
}

void
siop_unqueue(sc, target, lun)
	struct siop_softc *sc;
	int target;
	int lun;
{
 	int slot, tag;
	struct siop_cmd *siop_cmd;
	struct siop_lun *siop_lun = sc->targets[target]->siop_lun[lun];

	/* first make sure to read valid data */
	siop_script_sync(sc, BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	for (tag = 1; tag < SIOP_NTAG; tag++) {
		/* look for commands in the scheduler, not yet started */
		if (siop_lun->siop_tag[tag].active == NULL) 
			continue;
		siop_cmd = siop_lun->siop_tag[tag].active;
		for (slot = 0; slot <= sc->sc_currschedslot; slot++) {
			if (siop_script_read(sc,
			    (Ent_script_sched_slot0 / 4) + slot * 2 + 1) ==
			    siop_cmd->dsa + sizeof(struct siop_xfer_common) +
			    Ent_ldsa_select)
				break;
		}
		if (slot >  sc->sc_currschedslot)
			continue; /* didn't find it */
		if (siop_script_read(sc,
		    (Ent_script_sched_slot0 / 4) + slot * 2) == 0x80000000)
			continue; /* already started */
		/* clear the slot */
		siop_script_write(sc, (Ent_script_sched_slot0 / 4) + slot * 2,
		    0x80000000);
		/* ask to requeue */
		siop_cmd->xs->error = XS_REQUEUE;
		siop_cmd->xs->status = SCSI_SIOP_NOCHECK;
		siop_lun->siop_tag[tag].active = NULL;
		siop_scsicmd_end(siop_cmd);
	}
	/* update sc_currschedslot */
	sc->sc_currschedslot = 0;
	for (slot = SIOP_NSLOTS - 1; slot >= 0; slot--) {
		if (siop_script_read(sc,
		    (Ent_script_sched_slot0 / 4) + slot * 2) != 0x80000000)
			sc->sc_currschedslot = slot;
	}
}

/*
 * handle a rejected queue tag message: the command will run untagged,
 * has to adjust the reselect script.
 */
int
siop_handle_qtag_reject(siop_cmd)
	struct siop_cmd *siop_cmd;
{
	struct siop_softc *sc = siop_cmd->siop_sc;
	int target = siop_cmd->xs->xs_periph->periph_target;
	int lun = siop_cmd->xs->xs_periph->periph_lun;
	int tag = siop_cmd->siop_tables.msg_out[2];
	struct siop_lun *siop_lun = sc->targets[target]->siop_lun[lun];

#ifdef SIOP_DEBUG
	printf("%s:%d:%d: tag message %d (%d) rejected (status %d)\n",
	    sc->sc_dev.dv_xname, target, lun, tag, siop_cmd->tag,
	    siop_cmd->status);
#endif

	if (siop_lun->siop_tag[0].active != NULL) {
		printf("%s: untagged command already running for target %d "
		    "lun %d (status %d)\n", sc->sc_dev.dv_xname, target, lun,
		    siop_lun->siop_tag[0].active->status);
		return -1;
	}
	/* clear tag slot */
	siop_lun->siop_tag[tag].active = NULL;
	/* add command to non-tagged slot */
	siop_lun->siop_tag[0].active = siop_cmd;
	siop_cmd->tag = 0;
	/* adjust reselect script if there is one */
	if (siop_lun->siop_tag[0].reseloff > 0) {
		siop_script_write(sc,
		    siop_lun->siop_tag[0].reseloff + 1,
		    siop_cmd->dsa + sizeof(struct siop_xfer_common) +
		    Ent_ldsa_reload_dsa);
		siop_table_sync(siop_cmd, BUS_DMASYNC_PREWRITE);
	}
	return 0;
}

/*
 * handle a bus reset: reset chip, unqueue all active commands, free all
 * target struct and report loosage to upper layer.
 * As the upper layer may requeue immediatly we have to first store
 * all active commands in a temporary queue.
 */
void
siop_handle_reset(sc)
	struct siop_softc *sc;
{
	struct siop_cmd *siop_cmd;
	struct siop_lun *siop_lun;
	int target, lun, tag;
	/*
	 * scsi bus reset. reset the chip and restart
	 * the queue. Need to clean up all active commands
	 */
	printf("%s: scsi bus reset\n", sc->sc_dev.dv_xname);
	/* stop, reset and restart the chip */
	siop_reset(sc);
	if (sc->sc_flags & SCF_CHAN_NOSLOT) {
		/* chip has been reset, all slots are free now */
		sc->sc_flags &= ~SCF_CHAN_NOSLOT;
		scsipi_channel_thaw(&sc->sc_chan, 1);
	}
	/*
	 * Process all commands: first commmands being executed
	 */
	for (target = 0; target < sc->sc_chan.chan_ntargets;
	    target++) {
		if (sc->targets[target] == NULL)
			continue;
		for (lun = 0; lun < 8; lun++) {
			siop_lun = sc->targets[target]->siop_lun[lun];
			if (siop_lun == NULL)
				continue;
			for (tag = 0; tag <
			    ((sc->targets[target]->flags & TARF_TAG) ?
			    SIOP_NTAG : 1);
			    tag++) {
				siop_cmd = siop_lun->siop_tag[tag].active;
				if (siop_cmd == NULL)
					continue;
				scsipi_printaddr(siop_cmd->xs->xs_periph);
				printf("command with tag id %d reset\n", tag);
				siop_cmd->xs->error =
				    (siop_cmd->flags & CMDFL_TIMEOUT) ?
		    		    XS_TIMEOUT : XS_RESET;
				siop_cmd->xs->status = SCSI_SIOP_NOCHECK;
				siop_lun->siop_tag[tag].active = NULL;
				siop_cmd->status = CMDST_DONE;
				siop_scsicmd_end(siop_cmd);
			}
		}
		sc->targets[target]->status = TARST_ASYNC;
		sc->targets[target]->flags &= ~TARF_ISWIDE;
		sc->targets[target]->period = sc->targets[target]->offset = 0;
		siop_update_xfer_mode(sc, target);
	}

	scsipi_async_event(&sc->sc_chan, ASYNC_EVENT_RESET, NULL);
}

void
siop_scsipi_request(chan, req, arg)
	struct scsipi_channel *chan;
	scsipi_adapter_req_t req;
	void *arg;
{
	struct scsipi_xfer *xs;
	struct scsipi_periph *periph;
	struct siop_softc *sc = (void *)chan->chan_adapter->adapt_dev;
	struct siop_cmd *siop_cmd;
	int s, error, i;
	int target;
	int lun;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;
		periph = xs->xs_periph;
		target = periph->periph_target;
		lun = periph->periph_lun;

		s = splbio();
#ifdef SIOP_DEBUG_SCHED
		printf("starting cmd for %d:%d\n", target, lun);
#endif
		siop_cmd = TAILQ_FIRST(&sc->free_list);
		if (siop_cmd) {
			TAILQ_REMOVE(&sc->free_list, siop_cmd, next);
		} else {
			if (siop_morecbd(sc) == 0) {
				siop_cmd = TAILQ_FIRST(&sc->free_list);
#ifdef DIAGNOSTIC
				if (siop_cmd == NULL)
					panic("siop_morecbd succeed and does nothing");
#endif
				TAILQ_REMOVE(&sc->free_list, siop_cmd, next);
			}
		}
		if (siop_cmd == NULL) {
			xs->error = XS_RESOURCE_SHORTAGE;
			scsipi_done(xs);
			splx(s);
			return;
		}
#ifdef DIAGNOSTIC
		if (siop_cmd->status != CMDST_FREE)
			panic("siop_scsicmd: new cmd not free");
#endif
		if (sc->targets[target] == NULL) {
#ifdef SIOP_DEBUG
			printf("%s: alloc siop_target for target %d\n",
				sc->sc_dev.dv_xname, target);
#endif
			sc->targets[target] =
			    malloc(sizeof(struct siop_target),
				M_DEVBUF, M_NOWAIT);
			if (sc->targets[target] == NULL) {
				printf("%s: can't malloc memory for "
				    "target %d\n", sc->sc_dev.dv_xname, target);
				xs->error = XS_RESOURCE_SHORTAGE;
				scsipi_done(xs);
				splx(s);
				return;
			}
			sc->targets[target]->status = TARST_PROBING;
			sc->targets[target]->flags = 0;
			sc->targets[target]->id =
			    sc->clock_div << 24; /* scntl3 */
			sc->targets[target]->id |=  target << 16; /* id */
			/* sc->targets[target]->id |= 0x0 << 8; scxfer is 0 */

			/* get a lun switch script */
			sc->targets[target]->lunsw = siop_get_lunsw(sc);
			if (sc->targets[target]->lunsw == NULL) {
				printf("%s: can't alloc lunsw for target %d\n",
				    sc->sc_dev.dv_xname, target);
				xs->error = XS_RESOURCE_SHORTAGE;
				scsipi_done(xs);
				splx(s);
				return;
			}
			for (i=0; i < 8; i++)
				sc->targets[target]->siop_lun[i] = NULL;
			siop_add_reselsw(sc, target);
		}
		if (sc->targets[target]->siop_lun[lun] == NULL) {
			sc->targets[target]->siop_lun[lun] =
			    malloc(sizeof(struct siop_lun), M_DEVBUF, M_NOWAIT);
			if (sc->targets[target]->siop_lun[lun] == NULL) {
				printf("%s: can't alloc siop_lun for "
				    "target %d lun %d\n",
				    sc->sc_dev.dv_xname, target, lun);
				xs->error = XS_RESOURCE_SHORTAGE;
				scsipi_done(xs);
				splx(s);
				return;
			}
			memset(sc->targets[target]->siop_lun[lun], 0,
			    sizeof(struct siop_lun));
		}
		siop_cmd->siop_target = sc->targets[target];
		siop_cmd->xs = xs;
		siop_cmd->flags = 0;
		siop_cmd->status = CMDST_READY;

		/* load the DMA maps */
		error = bus_dmamap_load(sc->sc_dmat, siop_cmd->dmamap_cmd,
		    xs->cmd, xs->cmdlen, NULL, BUS_DMA_NOWAIT);
		if (error) {
			printf("%s: unable to load cmd DMA map: %d\n",
			    sc->sc_dev.dv_xname, error);
			xs->error = XS_DRIVER_STUFFUP;
			scsipi_done(xs);
			splx(s);
			return;
		}
		if (xs->xs_control & (XS_CTL_DATA_IN | XS_CTL_DATA_OUT)) {
			error = bus_dmamap_load(sc->sc_dmat,
			    siop_cmd->dmamap_data, xs->data, xs->datalen,
			    NULL, BUS_DMA_NOWAIT | BUS_DMA_STREAMING);
			if (error) {
				printf("%s: unable to load cmd DMA map: %d",
				    sc->sc_dev.dv_xname, error);
				xs->error = XS_DRIVER_STUFFUP;
				scsipi_done(xs);
				bus_dmamap_unload(sc->sc_dmat, siop_cmd->dmamap_cmd);
				splx(s);
				return;
			}
			bus_dmamap_sync(sc->sc_dmat, siop_cmd->dmamap_data, 0,
			    siop_cmd->dmamap_data->dm_mapsize,
			    (xs->xs_control & XS_CTL_DATA_IN) ?
			    BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
		}
		bus_dmamap_sync(sc->sc_dmat, siop_cmd->dmamap_cmd, 0,
		    siop_cmd->dmamap_cmd->dm_mapsize, BUS_DMASYNC_PREWRITE);

		siop_setuptables(siop_cmd);
		siop_start(sc, siop_cmd);
		if (xs->xs_control & XS_CTL_POLL) {
			/* poll for command completion */
			while ((xs->xs_status & XS_STS_DONE) == 0) {
				delay(1000);
				siop_intr(sc);
			}
		}
		splx(s);
		return;

	case ADAPTER_REQ_GROW_RESOURCES:
		/* XXX Not supported. */
		return;

	case ADAPTER_REQ_SET_XFER_MODE:
	{
		struct scsipi_xfer_mode *xm = arg;
		if (sc->targets[xm->xm_target] == NULL)
			return;
		s = splbio();
		if (xm->xm_mode & PERIPH_CAP_TQING)
			sc->targets[xm->xm_target]->flags |= TARF_TAG;
		if ((xm->xm_mode & PERIPH_CAP_WIDE16) &&
		    (sc->features & SF_BUS_WIDE))
			sc->targets[xm->xm_target]->flags |= TARF_WIDE;
		if (xm->xm_mode & PERIPH_CAP_SYNC)
			sc->targets[xm->xm_target]->flags |= TARF_SYNC;
		if ((xm->xm_mode & (PERIPH_CAP_SYNC | PERIPH_CAP_WIDE16)) ||
		    sc->targets[xm->xm_target]->status == TARST_PROBING)
			sc->targets[xm->xm_target]->status =
			    TARST_ASYNC;

		for (lun = 0; lun < sc->sc_chan.chan_nluns; lun++) {
			if (sc->sc_chan.chan_periphs[xm->xm_target][lun])
				/* allocate a lun sw entry for this device */
				siop_add_dev(sc, xm->xm_target, lun);
		}

		splx(s);
	}
	}
}

static void
siop_start(sc, siop_cmd)
	struct siop_softc *sc;
	struct siop_cmd *siop_cmd;
{
	struct siop_lun *siop_lun;
	u_int32_t dsa;
	int timeout;
	int target, lun, slot;

	/*
	 * first make sure to read valid data
	 */
	siop_script_sync(sc, BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	/*
	 * The queue management here is a bit tricky: the script always looks
	 * at the slot from first to last, so if we always use the first 
	 * free slot commands can stay at the tail of the queue ~forever.
	 * The algorithm used here is to restart from the head when we know
	 * that the queue is empty, and only add commands after the last one.
	 * When we're at the end of the queue wait for the script to clear it.
	 * The best thing to do here would be to implement a circular queue,
	 * but using only 53c720 features this can be "interesting".
	 * A mid-way solution could be to implement 2 queues and swap orders.
	 */
	slot = sc->sc_currschedslot;
	/*
	 * If the instruction is 0x80000000 (JUMP foo, IF FALSE) the slot is
	 * free. As this is the last used slot, all previous slots are free,
	 * we can restart from 0.
	 */
	if (siop_script_read(sc, (Ent_script_sched_slot0 / 4) + slot * 2) ==
	    0x80000000) {
		slot = sc->sc_currschedslot = 0;
	} else {
		slot++;
	}
	target = siop_cmd->xs->xs_periph->periph_target;
	lun = siop_cmd->xs->xs_periph->periph_lun;
	siop_lun = sc->targets[target]->siop_lun[lun];
	/* if non-tagged command active, panic: this shouldn't happen */
	if (siop_lun->siop_tag[0].active != NULL) {
		panic("siop_start: tagged cmd while untagged running");
	}
#ifdef DIAGNOSTIC
	/* sanity check the tag if needed */
	if (siop_cmd->flags & CMDFL_TAG) {
		if (siop_lun->siop_tag[siop_cmd->tag].active != NULL)
			panic("siop_start: tag not free");
		if (siop_cmd->tag >= SIOP_NTAG) {
			scsipi_printaddr(siop_cmd->xs->xs_periph);
			printf(": tag id %d\n", siop_cmd->tag);
			panic("siop_start: invalid tag id");
		}
	}
#endif
	/*
	 * find a free scheduler slot and load it.
	 */
	for (; slot < SIOP_NSLOTS; slot++) {
		/*
		 * If cmd if 0x80000000 the slot is free
		 */
		if (siop_script_read(sc,
		    (Ent_script_sched_slot0 / 4) + slot * 2) ==
		    0x80000000)
			break;
	}
	if (slot == SIOP_NSLOTS) {
		/*
		 * no more free slot, no need to continue. freeze the queue
		 * and requeue this command.
		 */
		scsipi_channel_freeze(&sc->sc_chan, 1);
		sc->sc_flags |= SCF_CHAN_NOSLOT;
		siop_cmd->xs->error = XS_REQUEUE;
		siop_cmd->xs->status = SCSI_SIOP_NOCHECK;
		siop_scsicmd_end(siop_cmd);
		return;
	}
#ifdef SIOP_DEBUG_SCHED
	printf("using slot %d for DSA 0x%lx\n", slot,
	    (u_long)siop_cmd->dsa);
#endif
	/* mark command as active */
	if (siop_cmd->status == CMDST_READY)
		siop_cmd->status = CMDST_ACTIVE;
	else
		panic("siop_start: bad status");
	siop_lun->siop_tag[siop_cmd->tag].active = siop_cmd;
	/* patch scripts with DSA addr */
	dsa = siop_cmd->dsa;
	/* first reselect switch, if we have an entry */
	if (siop_lun->siop_tag[siop_cmd->tag].reseloff > 0)
		siop_script_write(sc,
		    siop_lun->siop_tag[siop_cmd->tag].reseloff + 1,
		    dsa + sizeof(struct siop_xfer_common) +
		    Ent_ldsa_reload_dsa);
	/* CMD script: MOVE MEMORY addr */
	siop_cmd->siop_xfer->resel[E_ldsa_abs_slot_Used[0]] = 
	   htole32(sc->sc_scriptaddr + Ent_script_sched_slot0 + slot * 8);
		siop_table_sync(siop_cmd, BUS_DMASYNC_PREWRITE);
	/* scheduler slot: JUMP ldsa_select */
	siop_script_write(sc,
	    (Ent_script_sched_slot0 / 4) + slot * 2 + 1,
	    dsa + sizeof(struct siop_xfer_common) + Ent_ldsa_select);
	/* handle timeout */
	if ((siop_cmd->xs->xs_control & XS_CTL_POLL) == 0) {
		/* start exire timer */
		timeout =
		    (u_int64_t)siop_cmd->xs->timeout * (u_int64_t)hz / 1000;
		if (timeout == 0)
			timeout = 1;
		callout_reset( &siop_cmd->xs->xs_callout,
		    timeout, siop_timeout, siop_cmd);
	}
	/*
	 * Change JUMP cmd so that this slot will be handled
	 */
	siop_script_write(sc, (Ent_script_sched_slot0 / 4) + slot * 2,
	    0x80080000);
	sc->sc_currschedslot = slot;

	/* make sure SCRIPT processor will read valid data */
	siop_script_sync(sc,BUS_DMASYNC_PREREAD |  BUS_DMASYNC_PREWRITE);
	/* Signal script it has some work to do */
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_ISTAT, ISTAT_SIGP);
	/* and wait for IRQ */
	return;
}

void
siop_timeout(v)
	void *v;
{
	struct siop_cmd *siop_cmd = v;
	struct siop_softc *sc = siop_cmd->siop_sc;
	int s;

	scsipi_printaddr(siop_cmd->xs->xs_periph);
	printf("command timeout\n");

	s = splbio();
	/* reset the scsi bus */
	siop_resetbus(sc);

	/* deactivate callout */
	callout_stop(&siop_cmd->xs->xs_callout);
	/* mark command as being timed out; siop_intr will handle it */
	/*
	 * mark command has being timed out and just return;
	 * the bus reset will generate an interrupt,
	 * it will be handled in siop_intr()
	 */
	siop_cmd->flags |= CMDFL_TIMEOUT;
	splx(s);
	return;

}

void
siop_dump_script(sc)
	struct siop_softc *sc;
{
	int i;
	for (i = 0; i < PAGE_SIZE / 4; i += 2) {
		printf("0x%04x: 0x%08x 0x%08x", i * 4,
		    le32toh(sc->sc_script[i]), le32toh(sc->sc_script[i+1]));
		if ((le32toh(sc->sc_script[i]) & 0xe0000000) == 0xc0000000) {
			i++;
			printf(" 0x%08x", le32toh(sc->sc_script[i+1]));
		}
		printf("\n");
	}
}

int
siop_morecbd(sc)
	struct siop_softc *sc;
{
	int error, i, j;
	bus_dma_segment_t seg;
	int rseg;
	struct siop_cbd *newcbd;
	bus_addr_t dsa;
	u_int32_t *scr;

	/* allocate a new list head */
	newcbd = malloc(sizeof(struct siop_cbd), M_DEVBUF, M_NOWAIT);
	if (newcbd == NULL) {
		printf("%s: can't allocate memory for command descriptors "
		    "head\n", sc->sc_dev.dv_xname);
		return ENOMEM;
	}
	memset(newcbd, 0, sizeof(struct siop_cbd));

	/* allocate cmd list */
	newcbd->cmds =
	    malloc(sizeof(struct siop_cmd) * SIOP_NCMDPB, M_DEVBUF, M_NOWAIT);
	if (newcbd->cmds == NULL) {
		printf("%s: can't allocate memory for command descriptors\n",
		    sc->sc_dev.dv_xname);
		error = ENOMEM;
		goto bad3;
	}
	memset(newcbd->cmds, 0, sizeof(struct siop_cmd) * SIOP_NCMDPB);
	error = bus_dmamem_alloc(sc->sc_dmat, PAGE_SIZE, PAGE_SIZE, 0, &seg,
	    1, &rseg, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: unable to allocate cbd DMA memory, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto bad2;
	}
	error = bus_dmamem_map(sc->sc_dmat, &seg, rseg, PAGE_SIZE,
	    (caddr_t *)&newcbd->xfers, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error) {
		printf("%s: unable to map cbd DMA memory, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto bad2;
	}
	error = bus_dmamap_create(sc->sc_dmat, PAGE_SIZE, 1, PAGE_SIZE, 0,
	    BUS_DMA_NOWAIT, &newcbd->xferdma);
	if (error) {
		printf("%s: unable to create cbd DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto bad1;
	}
	error = bus_dmamap_load(sc->sc_dmat, newcbd->xferdma, newcbd->xfers,
	    PAGE_SIZE, NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: unable to load cbd DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto bad0;
	}
#ifdef DEBUG
	printf("%s: alloc newcdb at PHY addr 0x%lx\n", sc->sc_dev.dv_xname,
	    (unsigned long)newcbd->xferdma->dm_segs[0].ds_addr);
#endif
	
	for (i = 0; i < SIOP_NCMDPB; i++) {
		error = bus_dmamap_create(sc->sc_dmat, MAXPHYS, SIOP_NSG,
		    MAXPHYS, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
		    &newcbd->cmds[i].dmamap_data);
		if (error) {
			printf("%s: unable to create data DMA map for cbd: "
			    "error %d\n",
			    sc->sc_dev.dv_xname, error);
			goto bad0;
		}
		error = bus_dmamap_create(sc->sc_dmat,
		    sizeof(struct scsipi_generic), 1,
		    sizeof(struct scsipi_generic), 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
		    &newcbd->cmds[i].dmamap_cmd);
		if (error) {
			printf("%s: unable to create cmd DMA map for cbd %d\n",
			    sc->sc_dev.dv_xname, error);
			goto bad0;
		}
		newcbd->cmds[i].siop_sc = sc;
		newcbd->cmds[i].siop_cbdp = newcbd;
		newcbd->cmds[i].siop_xfer = &newcbd->xfers[i];
		memset(newcbd->cmds[i].siop_xfer, 0,
		    sizeof(struct siop_xfer));
		newcbd->cmds[i].dsa = newcbd->xferdma->dm_segs[0].ds_addr +
		    i * sizeof(struct siop_xfer);
		dsa = newcbd->cmds[i].dsa;
		newcbd->cmds[i].status = CMDST_FREE;
		newcbd->cmds[i].siop_tables.t_msgout.count= htole32(1);
		newcbd->cmds[i].siop_tables.t_msgout.addr = htole32(dsa);
		newcbd->cmds[i].siop_tables.t_msgin.count= htole32(1);
		newcbd->cmds[i].siop_tables.t_msgin.addr = htole32(dsa + 8);
		newcbd->cmds[i].siop_tables.t_extmsgin.count= htole32(2);
		newcbd->cmds[i].siop_tables.t_extmsgin.addr = htole32(dsa + 9);
		newcbd->cmds[i].siop_tables.t_extmsgdata.addr =
		    htole32(dsa + 11);
		newcbd->cmds[i].siop_tables.t_status.count= htole32(1);
		newcbd->cmds[i].siop_tables.t_status.addr = htole32(dsa + 16);

		/* The select/reselect script */
		scr = &newcbd->cmds[i].siop_xfer->resel[0];
		for (j = 0; j < sizeof(load_dsa) / sizeof(load_dsa[0]); j++)
			scr[j] = htole32(load_dsa[j]);
		/*
		 * 0x78000000 is a 'move data8 to reg'. data8 is the second
		 * octet, reg offset is the third.
		 */
		scr[Ent_rdsa0 / 4] =
		    htole32(0x78100000 | ((dsa & 0x000000ff) <<  8));
		scr[Ent_rdsa1 / 4] =
		    htole32(0x78110000 | ( dsa & 0x0000ff00       ));
		scr[Ent_rdsa2 / 4] =
		    htole32(0x78120000 | ((dsa & 0x00ff0000) >>  8));
		scr[Ent_rdsa3 / 4] =
		    htole32(0x78130000 | ((dsa & 0xff000000) >> 16));
		scr[E_ldsa_abs_reselected_Used[0]] =
		    htole32(sc->sc_scriptaddr + Ent_reselected);
		scr[E_ldsa_abs_reselect_Used[0]] =
		    htole32(sc->sc_scriptaddr + Ent_reselect);
		scr[E_ldsa_abs_selected_Used[0]] =
		    htole32(sc->sc_scriptaddr + Ent_selected);
		scr[E_ldsa_abs_data_Used[0]] =
		    htole32(dsa + sizeof(struct siop_xfer_common) +
		    Ent_ldsa_data);
		/* JUMP foo, IF FALSE - used by MOVE MEMORY to clear the slot */
		scr[Ent_ldsa_data / 4] = htole32(0x80000000);
		TAILQ_INSERT_TAIL(&sc->free_list, &newcbd->cmds[i], next);
#ifdef SIOP_DEBUG
		printf("tables[%d]: in=0x%x out=0x%x status=0x%x\n", i,
		    le32toh(newcbd->cmds[i].siop_tables.t_msgin.addr),
		    le32toh(newcbd->cmds[i].siop_tables.t_msgout.addr),
		    le32toh(newcbd->cmds[i].siop_tables.t_status.addr));
#endif
	}
	TAILQ_INSERT_TAIL(&sc->cmds, newcbd, next);
	return 0;
bad0:
	bus_dmamap_destroy(sc->sc_dmat, newcbd->xferdma);
bad1:
	bus_dmamem_free(sc->sc_dmat, &seg, rseg);
bad2:
	free(newcbd->cmds, M_DEVBUF);
bad3:
	free(newcbd, M_DEVBUF);
	return error;
}

struct siop_lunsw *
siop_get_lunsw(sc)
	struct siop_softc *sc;
{
	struct siop_lunsw *lunsw;
	int i;

	if (sc->script_free_lo + (sizeof(lun_switch) / sizeof(lun_switch[0])) >=
	    sc->script_free_hi)
		return NULL;
	lunsw = TAILQ_FIRST(&sc->lunsw_list);
	if (lunsw != NULL) {
#ifdef SIOP_DEBUG
		printf("siop_get_lunsw got lunsw at offset %d\n",
		    lunsw->lunsw_off);
#endif
		TAILQ_REMOVE(&sc->lunsw_list, lunsw, next);
		return lunsw;
	}
	lunsw = malloc(sizeof(struct siop_lunsw), M_DEVBUF, M_NOWAIT);
	if (lunsw == NULL)
		return NULL;
	memset(lunsw, 0, sizeof(struct siop_lunsw));
#ifdef SIOP_DEBUG
	printf("allocating lunsw at offset %d\n", sc->script_free_lo);
#endif
	if (sc->features & SF_CHIP_RAM) {
		bus_space_write_region_4(sc->sc_ramt, sc->sc_ramh,
		    sc->script_free_lo * 4, lun_switch,
		    sizeof(lun_switch) / sizeof(lun_switch[0]));
		bus_space_write_4(sc->sc_ramt, sc->sc_ramh,
		    (sc->script_free_lo + E_abs_lunsw_return_Used[0]) * 4,
		    sc->sc_scriptaddr + Ent_lunsw_return);
	} else {
		for (i = 0; i < sizeof(lun_switch) / sizeof(lun_switch[0]);
		    i++)
			sc->sc_script[sc->script_free_lo + i] =
			    htole32(lun_switch[i]);
		sc->sc_script[sc->script_free_lo + E_abs_lunsw_return_Used[0]] = 
		    htole32(sc->sc_scriptaddr + Ent_lunsw_return);
	}
	lunsw->lunsw_off = sc->script_free_lo;
	lunsw->lunsw_size = sizeof(lun_switch) / sizeof(lun_switch[0]);
	sc->script_free_lo += lunsw->lunsw_size;
	siop_script_sync(sc, BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	return lunsw;
}

void
siop_add_reselsw(sc, target)
	struct siop_softc *sc;
	int target;
{
	int i;
	struct siop_lun *siop_lun;
	/*
	 * add an entry to resel switch
	 */
	siop_script_sync(sc, BUS_DMASYNC_POSTWRITE);
	for (i = 0; i < 15; i++) {
		sc->targets[target]->reseloff = Ent_resel_targ0 / 4 + i * 2;
		if ((siop_script_read(sc, sc->targets[target]->reseloff) & 0xff)
		    == 0xff) { /* it's free */
#ifdef SIOP_DEBUG
			printf("siop: target %d slot %d offset %d\n",
			    target, i, sc->targets[target]->reseloff);
#endif
			/* JUMP abs_foo, IF target | 0x80; */
			siop_script_write(sc, sc->targets[target]->reseloff,
			    0x800c0080 | target);
			siop_script_write(sc, sc->targets[target]->reseloff + 1,
			    sc->sc_scriptaddr +
			    sc->targets[target]->lunsw->lunsw_off * 4 +
			    Ent_lun_switch_entry);
			break;
		}
	}
	if (i == 15) /* no free slot, shouldn't happen */
		panic("siop: resel switch full");

	sc->sc_ntargets++;
	for (i = 0; i < 8; i++) {
		siop_lun = sc->targets[target]->siop_lun[i];
		if (siop_lun == NULL)
			continue;
		if (siop_lun->reseloff > 0) {
			siop_lun->reseloff = 0;
			siop_add_dev(sc, target, i);
		}
	}
	siop_update_scntl3(sc, sc->targets[target]);
	siop_script_sync(sc, BUS_DMASYNC_PREWRITE);
}

void
siop_update_scntl3(sc, siop_target)
	struct siop_softc *sc;
	struct siop_target *siop_target;
{
	/* MOVE target->id >> 24 TO SCNTL3 */
	siop_script_write(sc,
	    siop_target->lunsw->lunsw_off + (Ent_restore_scntl3 / 4),
	    0x78030000 | ((siop_target->id >> 16) & 0x0000ff00));
	/* MOVE target->id >> 8 TO SXFER */
	siop_script_write(sc,
	    siop_target->lunsw->lunsw_off + (Ent_restore_scntl3 / 4) + 2,
	    0x78050000 | (siop_target->id & 0x0000ff00));
	siop_script_sync(sc, BUS_DMASYNC_PREWRITE);
}

void
siop_add_dev(sc, target, lun)
	struct siop_softc *sc;
	int target;
	int lun;
{
	struct siop_lunsw *lunsw;
	struct siop_lun *siop_lun = sc->targets[target]->siop_lun[lun];
	int i, ntargets;

	if (siop_lun->reseloff > 0)
		return;
	lunsw = sc->targets[target]->lunsw;
	if ((lunsw->lunsw_off + lunsw->lunsw_size) < sc->script_free_lo) {
		/*
		 * can't extend this slot. Probably not worth trying to deal
		 * with this case
		 */
#ifdef DEBUG
		printf("%s:%d:%d: can't allocate a lun sw slot\n",
		    sc->sc_dev.dv_xname, target, lun);
#endif
		return;
	}
	/* count how many free targets we still have to probe */
	ntargets =  sc->sc_chan.chan_ntargets - 1 - sc->sc_ntargets;

	/*
	 * we need 8 bytes for the lun sw additionnal entry, and
	 * eventually sizeof(tag_switch) for the tag switch entry.
	 * Keep enouth free space for the free targets that could be
	 * probed later.
	 */
	if (sc->script_free_lo + 2 +
	    (ntargets * sizeof(lun_switch) / sizeof(lun_switch[0])) >=
	    ((sc->targets[target]->flags & TARF_TAG) ?
	    sc->script_free_hi - (sizeof(tag_switch) / sizeof(tag_switch[0])) :
	    sc->script_free_hi)) {
		/*
		 * not enouth space, probably not worth dealing with it.
		 * We can hold 13 tagged-queuing capable devices in the 4k RAM.
		 */
#ifdef DEBUG
		printf("%s:%d:%d: not enouth memory for a lun sw slot\n",
		    sc->sc_dev.dv_xname, target, lun);
#endif
		return;
	}
#ifdef SIOP_DEBUG
	printf("%s:%d:%d: allocate lun sw entry\n",
	    sc->sc_dev.dv_xname, target, lun);
#endif
	/* INT int_resellun */
	siop_script_write(sc, sc->script_free_lo, 0x98080000);
	siop_script_write(sc, sc->script_free_lo + 1, A_int_resellun);
	/* Now the slot entry: JUMP abs_foo, IF lun */
	siop_script_write(sc, sc->script_free_lo - 2,
	    0x800c0000 | lun);
	siop_script_write(sc, sc->script_free_lo - 1, 0);
	siop_lun->reseloff = sc->script_free_lo - 2;
	lunsw->lunsw_size += 2;
	sc->script_free_lo += 2;
	if (sc->targets[target]->flags & TARF_TAG) {
		/* we need a tag switch */
		sc->script_free_hi -=
		    sizeof(tag_switch) / sizeof(tag_switch[0]);
		if (sc->features & SF_CHIP_RAM) {
			bus_space_write_region_4(sc->sc_ramt, sc->sc_ramh,
			    sc->script_free_hi * 4, tag_switch,
			    sizeof(tag_switch) / sizeof(tag_switch[0]));
		} else {
			for(i = 0;
			    i < sizeof(tag_switch) / sizeof(tag_switch[0]);
			    i++) {
				sc->sc_script[sc->script_free_hi + i] = 
				    htole32(tag_switch[i]);
			}
		}
		siop_script_write(sc, 
		    siop_lun->reseloff + 1,
		    sc->sc_scriptaddr + sc->script_free_hi * 4 +
		    Ent_tag_switch_entry);

		for (i = 0; i < SIOP_NTAG; i++) {
			siop_lun->siop_tag[i].reseloff =
			    sc->script_free_hi + (Ent_resel_tag0 / 4) + i * 2;
		}
	} else {
		/* non-tag case; just work with the lun switch */
		siop_lun->siop_tag[0].reseloff = 
		    sc->targets[target]->siop_lun[lun]->reseloff;
	}
	siop_script_sync(sc, BUS_DMASYNC_PREWRITE);
}

void
siop_del_dev(sc, target, lun)
	struct siop_softc *sc;
	int target;
	int lun;
{
	int i;
#ifdef SIOP_DEBUG
		printf("%s:%d:%d: free lun sw entry\n",
		    sc->sc_dev.dv_xname, target, lun);
#endif
	if (sc->targets[target] == NULL)
		return;
	free(sc->targets[target]->siop_lun[lun], M_DEVBUF);
	sc->targets[target]->siop_lun[lun] = NULL;
	/* XXX compact sw entry too ? */
	/* check if we can free the whole target */
	for (i = 0; i < 8; i++) {
		if (sc->targets[target]->siop_lun[i] != NULL)
			return;
	}
#ifdef SIOP_DEBUG
	printf("%s: free siop_target for target %d lun %d lunsw offset %d\n",
	    sc->sc_dev.dv_xname, target, lun,
	    sc->targets[target]->lunsw->lunsw_off);
#endif
	/*
	 * nothing here, free the target struct and resel
	 * switch entry
	 */
	siop_script_write(sc, sc->targets[target]->reseloff, 0x800c00ff);
	siop_script_sync(sc, BUS_DMASYNC_PREWRITE);
	TAILQ_INSERT_TAIL(&sc->lunsw_list, sc->targets[target]->lunsw, next);
	free(sc->targets[target], M_DEVBUF);
	sc->targets[target] = NULL;
	sc->sc_ntargets--;
}

void
siop_update_xfer_mode(sc, target)
	struct siop_softc *sc;
	int target;
{
	struct siop_target *siop_target = sc->targets[target];
	struct scsipi_xfer_mode xm;

	xm.xm_target = target;
	xm.xm_mode = 0;
	xm.xm_period = 0;
	xm.xm_offset = 0;

	if (siop_target->flags & TARF_ISWIDE)
		xm.xm_mode |= PERIPH_CAP_WIDE16;
	if (siop_target->period) {
		xm.xm_period = siop_target->period;
		xm.xm_offset = siop_target->offset;
		xm.xm_mode |= PERIPH_CAP_SYNC;
	}
	if (siop_target->flags & TARF_TAG)
		xm.xm_mode |= PERIPH_CAP_TQING;
	scsipi_async_event(&sc->sc_chan, ASYNC_EVENT_XFER_MODE, &xm);
}

#ifdef SIOP_STATS
void
siop_printstats()
{
	printf("siop_stat_intr %d\n", siop_stat_intr);
	printf("siop_stat_intr_shortxfer %d\n", siop_stat_intr_shortxfer);
	printf("siop_stat_intr_xferdisc %d\n", siop_stat_intr_xferdisc);
	printf("siop_stat_intr_sdp %d\n", siop_stat_intr_sdp);
	printf("siop_stat_intr_done %d\n", siop_stat_intr_done);
	printf("siop_stat_intr_lunresel %d\n", siop_stat_intr_lunresel);
	printf("siop_stat_intr_qfull %d\n", siop_stat_intr_qfull);
}
#endif
