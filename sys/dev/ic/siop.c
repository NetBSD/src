/*	$NetBSD: siop.c,v 1.5 2000/04/27 14:06:57 bouyer Exp $	*/

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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* SYM53c7/8xx PCI-SCSI I/O Processors driver */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/scsiio.h>

#include <machine/endian.h>
#include <machine/bus.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_kern.h>

#include <dev/microcode/siop/siop.out>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsipi_all.h>

#include <dev/scsipi/scsiconf.h>

#include <dev/ic/siopreg.h>
#include <dev/ic/siopvar.h>

#undef DEBUG
#undef DEBUG_INTR
#undef DEBUG_SHED
#undef DUMP_SCRIPT

#define SIOP_STATS

#ifndef SIOP_DEFAULT_TARGET
#define SIOP_DEFAULT_TARGET 7
#endif

#define MEM_SIZE 8192
#define CMD_OFF 4096

/* tables used by SCRIPT */
typedef struct scr_table {
	u_int32_t count;
	u_int32_t addr;
} scr_table_t ;

/* Number of scatter/gather entries */
#define SIOP_NSG	(MAXPHYS/NBPG + 1)

/*
 * This structure interfaces the SCRIPT with the driver; it describes a full
 * transfert. It lives in the same chunk of DMA-safe memory as the script.
 */
struct siop_xfer {
	u_int8_t msg_out[8];	/* 0 */
	u_int8_t msg_in[8];	/* 8 */
	int status;		/* 16 */
	u_int32_t id;		/* 20 */
	u_int32_t pad1;		/* 24 */
	scr_table_t t_msgin;	/* 28 */
	scr_table_t t_extmsgin;	/* 36 */
	scr_table_t t_extmsgdata; /* 44 */
	scr_table_t t_extmsgtag; /* 52 */
	scr_table_t t_msgout;	/* 60 */
	scr_table_t cmd;	/* 68 */
	scr_table_t t_status;	/* 76 */
	scr_table_t data[SIOP_NSG]; /* 84 */
};

/*
 * This decribes a command handled by the SCSI controller
 * These are chained in either a free list or a active list
 * We have one queue per target + (one at the adapter's target for probe)
 */
struct siop_cmd {
	TAILQ_ENTRY (siop_cmd) next;
	struct siop_softc *siop_sc; /* pointer to adapter */
	struct scsipi_xfer *xs; /* xfer from the upper level */
	struct siop_xfer *siop_table; /* tables dealing with this xfer */
	bus_addr_t	dsa; /* DSA value to load */
	bus_dmamap_t	dmamap_cmd;
	bus_dmamap_t	dmamap_data;
	struct scsipi_sense rs_cmd; /* request sense command buffer */
	int       status;
	int       flags;
};

/* status defs */
#define CMDST_FREE		0 /* cmd slot is free */
#define CMDST_READY		1 /* cmd slot is waiting for processing */
#define CMDST_ACTIVE		2 /* cmd slot is being processed */
#define CMDST_SENSE		3 /* cmd slot is being requesting sense */
#define CMDST_SENSE_ACTIVE	4 /* request sense active */
#define CMDST_SENSE_DONE 	5 /* request sense done */
#define CMDST_DONE		6 /* cmd slot has been processed */
/* flags defs */
#define CMDFL_TIMEOUT	0x0001 /* cmd timed out */

/* initial number of cmd descriptors */
#define SIOP_NCMD 10

void	siop_reset __P((struct siop_softc *));
void	siop_handle_reset __P((struct siop_softc *));
void	siop_scsicmd_end __P((struct siop_cmd *));
void	siop_start __P((struct siop_softc *));
void 	siop_timeout __P((void *));
void	siop_minphys __P((struct buf *));
int	siop_ioctl __P((struct scsipi_link *, u_long,
		caddr_t, int, struct proc *));
int	siop_scsicmd __P((struct scsipi_xfer *));
void 	siop_sdp __P((struct siop_cmd *));
void 	siop_ssg __P((struct siop_cmd *));
void	siop_dump_script __P((struct siop_softc *));

struct scsipi_adapter siop_adapter = {
	0,
	siop_scsicmd,
	siop_minphys,
	siop_ioctl,
	NULL,
};

struct scsipi_device siop_dev = {
	NULL,
	NULL,
	NULL,
	NULL,
};

#ifdef SIOP_STATS
static int siop_stat_intr = 0;
static int siop_stat_intr_shortxfer = 0;
static int siop_stat_intr_sdp = 0;
static int siop_stat_intr_done = 0;
static int siop_stat_intr_reselect = 0;
static int siop_stat_intr_xferdisc = 0;
void siop_printstats __P((void));
#define INCSTAT(x) x++
#else
#define INCSTAT(x) 
#endif

static __inline__ void siop_table_sync __P((struct siop_cmd *, int));
static __inline__ void
siop_table_sync(siop_cmd, ops)
	struct siop_cmd *siop_cmd;
	int ops;
{
	struct siop_softc *sc  = siop_cmd->siop_sc;
	bus_addr_t offset;
	
	offset = sc->sc_scriptdma->dm_segs[0].ds_addr - siop_cmd->dsa;
	bus_dmamap_sync(sc->sc_dmat, sc->sc_scriptdma, offset,
	    sizeof(struct siop_xfer), ops);
}

static __inline__ void siop_script_sync __P((struct siop_softc *, int));
static __inline__ void
siop_script_sync(sc, ops)
	struct siop_softc *sc;
	int ops;
{
	bus_dmamap_sync(sc->sc_dmat, sc->sc_scriptdma, 0, CMD_OFF, ops);
}

void
siop_attach(sc)
	struct siop_softc *sc;
{
	int error, i;
	bus_dma_segment_t seg;
	int rseg;

	/*
	 * Allocate DMA-safe memory for the script itself and internal
	 * variables and map it.
	 */
	error = bus_dmamem_alloc(sc->sc_dmat, MEM_SIZE, 
	    NBPG, 0, &seg, 1, &rseg, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: unable to allocate script DMA memory, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		return;
	}
	error = bus_dmamem_map(sc->sc_dmat, &seg, rseg, MEM_SIZE,
	    (caddr_t *)&sc->sc_script, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error) {
		printf("%s: unable to map script DMA memory, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		return;
	}
	error = bus_dmamap_create(sc->sc_dmat, MEM_SIZE, 1,
	    MEM_SIZE, 0, BUS_DMA_NOWAIT, &sc->sc_scriptdma);
	if (error) {
		printf("%s: unable to create script DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		return;
	}
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_scriptdma, sc->sc_script,
	    MEM_SIZE, NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: unable to load script DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		return;
	}
	TAILQ_INIT(&sc->free_list);
	for (i = 0; i < 16; i++) 
		TAILQ_INIT(&sc->active_list[i]);
	/* allocate cmd list */
	sc->cmds =
	    malloc(sizeof(struct siop_cmd) * SIOP_NCMD, M_DEVBUF, M_NOWAIT);
	if (sc->cmds == NULL) {
		printf("%s: can't allocate memory for command descriptors\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	for (i = 0; i < SIOP_NCMD; i++) {
		error = bus_dmamap_create(sc->sc_dmat, MAXPHYS, SIOP_NSG,
		    MAXPHYS, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
		    &sc->cmds[i].dmamap_data);
		if (error) {
			printf("%s: unable to create data DMA map for cbd %d\n",
			    sc->sc_dev.dv_xname, error);
			return;
		}
		error = bus_dmamap_create(sc->sc_dmat,
		    sizeof(struct scsipi_generic), 1,
		    sizeof(struct scsipi_generic), 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
		    &sc->cmds[i].dmamap_cmd);
		if (error) {
			printf("%s: unable to create cmd DMA map for cbd %d\n",
			    sc->sc_dev.dv_xname, error);
			return;
		}
		sc->cmds[i].siop_sc = sc;
		sc->cmds[i].siop_table =
		    &((struct siop_xfer *)(&sc->sc_script[CMD_OFF/4]))[i];
		sc->cmds[i].dsa = sc->sc_scriptdma->dm_segs[0].ds_addr +
		    CMD_OFF + i * sizeof(struct siop_xfer);
		sc->cmds[i].status = CMDST_FREE;
		sc->cmds[i].siop_table->t_msgout.count= htole32(1);
		sc->cmds[i].siop_table->t_msgout.addr =
		    htole32(sc->cmds[i].dsa);
		sc->cmds[i].siop_table->t_msgin.count= htole32(1);
		sc->cmds[i].siop_table->t_msgin.addr =
		    htole32(sc->cmds[i].dsa + 8);
		sc->cmds[i].siop_table->t_extmsgin.count= htole32(2);
		sc->cmds[i].siop_table->t_extmsgin.addr =
		    htole32(le32toh(sc->cmds[i].siop_table->t_msgin.addr) + 1);
		sc->cmds[i].siop_table->t_status.count= htole32(1);
		sc->cmds[i].siop_table->t_status.addr =
		    htole32(le32toh(sc->cmds[i].siop_table->t_msgin.addr) + 8);
		TAILQ_INSERT_TAIL(&sc->free_list, &sc->cmds[i], next);
#ifdef DEBUG
		printf("tables[%d]: out=0x%x in=0x%x status=0x%x\n", i,
		    le32toh(sc->cmds[i].siop_table->t_msgin.addr),
		    le32toh(sc->cmds[i].siop_table->t_msgout.addr),
		    le32toh(sc->cmds[i].siop_table->t_status.addr));
#endif
	}
	/* compute number of sheduler slots */
	sc->sc_nshedslots = (
	    CMD_OFF /* memory size allocated for scripts */
	    - sizeof(siop_script) /* memory for main script */
	    + 8		/* extra NOP at end of main script */
	    - sizeof(endslot_script) /* memory needed at end of sheduler */
	    ) / (sizeof(slot_script) - 8);
#ifdef DEBUG
	printf("%s: script size = %d, PHY addr=0x%x, VIRT=%p nslots %d\n",
	    sc->sc_dev.dv_xname, (int)sizeof(siop_script),
	    (u_int)sc->sc_scriptdma->dm_segs[0].ds_addr, sc->sc_script,
	    sc->sc_nshedslots);
#endif

	sc->sc_link.adapter_softc = sc;
	sc->sc_link.openings = 1;
	sc->sc_link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
	sc->sc_link.scsipi_scsi.max_target  =
	    (sc->features & SF_BUS_WIDE) ? 15 : 7;
	sc->sc_link.scsipi_scsi.max_lun = 7;
	sc->sc_link.scsipi_scsi.adapter_target = bus_space_read_1(sc->sc_rt,
	    sc->sc_rh, SIOP_SCID);
	if (sc->sc_link.scsipi_scsi.adapter_target == 0 ||
	    sc->sc_link.scsipi_scsi.adapter_target >
	    sc->sc_link.scsipi_scsi.max_target)
		sc->sc_link.scsipi_scsi.adapter_target = SIOP_DEFAULT_TARGET;
	sc->sc_link.type = BUS_SCSI;
	sc->sc_link.adapter = &siop_adapter;
	sc->sc_link.device = &siop_dev;
	sc->sc_link.flags  = 0;

	siop_reset(sc);
#ifdef DUMP_SCRIPT
	siop_dump_script(sc);
#endif

	config_found((struct device*)sc, &sc->sc_link, scsiprint);
}

void
siop_reset(sc)
	struct siop_softc *sc;
{
	int i, j;
	u_int32_t *scr;
	bus_addr_t physaddr;

	/* reset the chip */
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_ISTAT, ISTAT_SRST);
	delay(1000);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_ISTAT, 0);

	/* copy and patch the script */
	for (j = 0; j < (sizeof(siop_script) / sizeof(siop_script[0])); j++) {
		sc->sc_script[j] = htole32(siop_script[j]);
	}
	/* copy the sheduler slots script */
	for (i = 0; i < sc->sc_nshedslots; i++) {
		scr = &sc->sc_script[Ent_sheduler / 4 + (Ent_nextslot / 4) * i];
		physaddr = sc->sc_scriptdma->dm_segs[0].ds_addr + Ent_sheduler
		    + Ent_nextslot * i;
		for (j = 0; j < (sizeof(slot_script) / sizeof(slot_script[0]));
		    j++) {
			scr[j] = htole32(slot_script[j]);
		}
		/*
		 * save current jump offset and patch MOVE MEMORY operands
		 * to restore it.
		 */
		scr[Ent_slotdata/4 + 1] = scr[Ent_slot/4 + 1];
		scr[E_slot_nextp_Used[0]] = htole32(physaddr + Ent_slot + 4);
		scr[E_slot_shed_addrsrc_Used[0]] = htole32(physaddr +
		    Ent_slotdata + 4);
		/* JUMP selected, in main script */
		scr[E_slot_abs_selected_Used[0]] =
		   htole32(sc->sc_scriptdma->dm_segs[0].ds_addr + Ent_selected);
		/* JUMP addr if SELECT fail */
		scr[E_slot_abs_reselect_Used[0]] = 
		   htole32(sc->sc_scriptdma->dm_segs[0].ds_addr + Ent_reselect);
	}
	/* Now the final JUMP */
	scr = &sc->sc_script[Ent_sheduler / 4 +
	    (Ent_nextslot / 4) * sc->sc_nshedslots];
	for (j = 0; j < (sizeof(endslot_script) / sizeof(endslot_script[0]));
	    j++) {
		scr[j] = htole32(endslot_script[j]);
	}
	scr[E_endslot_abs_reselect_Used[0]] = 
	    htole32(sc->sc_scriptdma->dm_segs[0].ds_addr + Ent_reselect);

	/* init registers */
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL0,
	    SCNTL0_ARB_MASK | SCNTL0_EPC | SCNTL0_AAP);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL1, 0);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL3, 0x3);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_DIEN, 0xff);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SIEN0,
	    0xff & ~(SIEN0_CMP | SIEN0_SEL | SIEN0_RSL));
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SIEN1,
	    0xff & ~(SIEN1_HTH | SIEN1_GEN));
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_STEST2, STEST2_EXT);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_STEST3, STEST3_TE);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_STIME0,
	    (0xb << STIME0_SEL_SHIFT));
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCID,
	    sc->sc_link.scsipi_scsi.adapter_target | SCID_RRE);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_RESPID0,
	    1 << sc->sc_link.scsipi_scsi.adapter_target);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_DCNTL, 0);

	/* start script */
	siop_script_sync(sc, BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSP,
	    sc->sc_scriptdma->dm_segs[0].ds_addr + Ent_reselect);
}

#if 0
#define CALL_SCRIPT(ent) do {\
	printf ("start script DSA 0x%lx DSP 0x%lx\n", \
	    siop_cmd->dsa, \
	    sc->sc_scriptdma->dm_segs[0].ds_addr + ent); \
bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSP, sc->sc_scriptdma->dm_segs[0].ds_addr + ent); \
} while (0)
#else
#define CALL_SCRIPT(ent) do {\
bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSP, sc->sc_scriptdma->dm_segs[0].ds_addr + ent); \
} while (0)
#endif

int
siop_intr(v)
	void *v;
{
	struct siop_softc *sc = v;
	struct siop_cmd *siop_cmd;
	struct scsipi_xfer *xs;
	u_int8_t istat, sist0, sist1, sstat1, dstat, scntl1;
	u_int32_t irqcode;
	int need_reset = 0;
	int offset, target;
	bus_addr_t dsa;

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
	if (dsa >= sc->sc_scriptdma->dm_segs[0].ds_addr + CMD_OFF &&
	    dsa < sc->sc_scriptdma->dm_segs[0].ds_addr + CMD_OFF +
	       SIOP_NCMD * sizeof(struct siop_xfer)) {
		dsa -= sc->sc_scriptdma->dm_segs[0].ds_addr + CMD_OFF;
		siop_cmd = &sc->cmds[dsa / sizeof(struct siop_xfer)];
		siop_table_sync(siop_cmd,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	} else {
		printf("%s: current DSA invalid\n",
		    sc->sc_dev.dv_xname);
		siop_cmd = NULL;
	}
	if (istat & ISTAT_DIP) {
		u_int32_t *p;
		dstat = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_DSTAT);
		if (dstat & DSTAT_SSI) {
			printf("single step dsp 0x%08x dsa 0x08%x\n",
			    bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSP),
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
		    bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSP),
		    bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSA));
		p = sc->sc_script +
		    (bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSP) -
		    sc->sc_scriptdma->dm_segs[0].ds_addr - 8) / 4;
		printf("0x%x 0x%x 0x%x 0x%x\n", le32toh(p[0]), le32toh(p[1]),
		    le32toh(p[2]), le32toh(p[3]));
		if (siop_cmd)
			printf("last msg_in=0x%x status=0x%x\n",
			    siop_cmd->siop_table->msg_in[0],
			    htole32(siop_cmd->siop_table->status));
		need_reset = 1;
		}
	}
	if (istat & ISTAT_SIP) {
		/*
		 * SCSI interrupt. If current command is not active,
		 * we don't need siop_cmd
		 */
		if (siop_cmd->status != CMDST_ACTIVE &&
		    siop_cmd->status != CMDST_SENSE_ACTIVE) {
			siop_cmd = NULL;
		}
		if (istat & ISTAT_DIP)
			delay(1);
		sist0 = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SIST0);
			delay(1);
		sist1 = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SIST1);
		sstat1 = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SSTAT1);
#if 0
		printf("scsi interrupt, sist0=0x%x sist1=0x%x sstat1=0x%x "
		    "DSA=0x%x DSP=0x%x\n", sist0, sist1,
		    bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SSTAT1),
		    bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSA),
		    bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSP));
#endif
		if (siop_cmd)
			xs = siop_cmd->xs;
		if (sist0 & SIST0_RST) {
			siop_handle_reset(sc);
			siop_start(sc);
			/* no table to flush here */
			return 1;
		}
		if (sist0 & SIST0_SGE) {
			if (siop_cmd)
				scsi_print_addr(xs->sc_link);
			else
				printf("%s:", sc->sc_dev.dv_xname);
			printf("scsi gross error\n");
			goto reset;
		}
		if ((sist0 & SIST0_MA) && need_reset == 0) {
			if (siop_cmd) { 
				/*
				 * first restore DSA, in case we were in a S/G
				 * operation.
				 */
				bus_space_write_4(sc->sc_rt, sc->sc_rh,
				    SIOP_DSA, siop_cmd->dsa);
				switch (sstat1 & SSTAT1_PHASE_MASK) {
				case SSTAT1_PHASE_STATUS:
				/*
				 * previous phase may be aborted for any reason
				 * ( for example, the target has less data to
				 * transfer than requested). Just go to status
				 * and the command should terminate.
				 */
					INCSTAT(siop_stat_intr_shortxfer);
					CALL_SCRIPT(Ent_status);
					/* no table to flush here */
					return 1;
				case SSTAT1_PHASE_MSGIN:
					/*
					 * target may be ready to disconnect
					 * Save data pointers just in case.
					 */
					INCSTAT(siop_stat_intr_xferdisc);
					siop_sdp(siop_cmd);
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
		if (sist0 & SIST0_PAR) {
			/* parity error, reset */
			if (siop_cmd)
				scsi_print_addr(xs->sc_link);
			else
				printf("%s:", sc->sc_dev.dv_xname);
			printf("parity error\n");
			need_reset = 1;
		}
		if ((sist1 & SIST1_STO) && need_reset == 0) {
			/* selection time out, assume there's no device here */
			if (siop_cmd) {
				siop_cmd->status = CMDST_DONE;
				xs->error = XS_SELTIMEOUT;
				goto end;
			} else {
				printf("%s: selection timeout without "
				    "command\n", sc->sc_dev.dv_xname);
				need_reset = 1;
			}
		}
		if (sist0 & SIST0_UDC) {
			/*
			 * unexpected disconnect. Usually the target signals
			 * a fatal condition this way. Attempt to get sense.
			 */
			 if (siop_cmd)
			 	goto check_sense;
			printf("%s: unexpected disconnect without "
			    "command\n", sc->sc_dev.dv_xname);
			goto reset;
		}
		/* Else it's an unhandled exeption (for now). */
		printf("%s: unhandled scsi interrupt, sist0=0x%x sist1=0x%x "
		    "sstat1=0x%x DSA=0x%x DSP=0x%x\n", sc->sc_dev.dv_xname,
		    sist0, sist1,
		    bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SSTAT1),
		    bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSA),
		    bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSP));
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
		scntl1 = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL1);
		bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL1,
		    scntl1 | SCNTL1_RST);
		/* minimum 25 us, more time won't hurt */
		delay(100);
		bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL1, scntl1);
		/* no table to flush here */
		return 1;
	}


	if ((istat & ISTAT_DIP) && (dstat & DSTAT_SIR)) { /* script interrupt */
		irqcode = bus_space_read_4(sc->sc_rt, sc->sc_rh,
		    SIOP_DSPS);
#ifdef DEBUG_INTR
		printf("script interrupt 0x%x\n", irqcode);
#endif
		/*
		 * an inactive command is only valid if it's a reselect
		 * interrupt: we'll change siop_cmd to point to the rigth one
		 * just here
		 */
		if (irqcode != A_int_resel &&
		    siop_cmd->status != CMDST_ACTIVE &&
		    siop_cmd->status != CMDST_SENSE_ACTIVE) {
			printf("%s: Aie, no command (IRQ code 0x%x current "
			    "status %d) !\n", sc->sc_dev.dv_xname,
			    irqcode, siop_cmd->status);
			xs = NULL;
		} else
			xs = siop_cmd->xs;
		switch(irqcode) {
		case A_int_err:
			printf("error, DSP=0x%x\n",
			    bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSP));
			if (xs) {
				xs->error = XS_SELTIMEOUT;
				goto end;
			} else {
				goto reset;
			}
		case A_int_msgin:
			if (xs)
				scsi_print_addr(xs->sc_link);
			else
				printf("%s: ", sc->sc_dev.dv_xname);
			if (siop_cmd->siop_table->msg_in[0] ==
			    MSG_MESSAGE_REJECT) {
				printf("scsi message reject, message sent "
				    "was 0x%x\n",
				    siop_cmd->siop_table->msg_out[0]);
				if (siop_cmd->siop_table->msg_out[0] ==
				    MSG_MESSAGE_REJECT) {
					/* MSG_REJECT  for a MSG_REJECT  !*/
					goto reset;
				}
				/* no table to flush here */
				CALL_SCRIPT(Ent_msgin_ack);
				return 1;
			}
			printf("unhandled message 0x%x\n",
			    siop_cmd->siop_table->msg_in[0]);
			siop_cmd->siop_table->t_msgout.count= htole32(1);
			siop_cmd->siop_table->t_msgout.addr =
			    htole32(siop_cmd->dsa);
			siop_cmd->siop_table->msg_out[0] = MSG_MESSAGE_REJECT;
			siop_table_sync(siop_cmd,
			    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
			CALL_SCRIPT(Ent_send_msgout);
			return 1;
		case A_int_extmsgin:
#ifdef DEBUG_INTR
			printf("extended message: msg 0x%x len %d\n",
			    siop_cmd->siop_table->msg_in[2], 
			    siop_cmd->siop_table->msg_in[1]);
#endif
			siop_cmd->siop_table->t_extmsgdata.count =
			    htole32(siop_cmd->siop_table->msg_in[1] - 1);
			siop_cmd->siop_table->t_extmsgdata.addr = 
			    htole32(
			    le32toh(siop_cmd->siop_table->t_extmsgin.addr)
			    + 2);
			siop_table_sync(siop_cmd,
			    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
			CALL_SCRIPT(Ent_get_extmsgdata);
			return 1;
		case A_int_extmsgdata:
#ifdef DEBUG_INTR
			{
			int i;
			printf("extended message: 0x%x, data:",
			    siop_cmd->siop_table->msg_in[2]);
			for (i = 3; i < 2 + siop_cmd->siop_table->msg_in[1];
			    i++)
				printf(" 0x%x",
				    siop_cmd->siop_table->msg_in[i]);
			printf("\n");
			}
#endif
			if (siop_cmd->siop_table->msg_in[2] == MSG_EXT_SDTR) {
				/* anserw with async for now */
				siop_cmd->siop_table->msg_out[0] = MSG_EXTENDED;
				siop_cmd->siop_table->msg_out[1] =
				    MSG_EXT_SDTR_LEN;
				siop_cmd->siop_table->msg_out[2] = MSG_EXT_SDTR;
				siop_cmd->siop_table->msg_out[3] = 0;
				siop_cmd->siop_table->msg_out[4] = 0;
				siop_cmd->siop_table->t_msgout.count =
				    htole32(MSG_EXT_SDTR_LEN + 2);
				siop_cmd->siop_table->t_msgout.addr =
				    htole32(siop_cmd->dsa);
			} else {
				/* send a message reject */
				siop_cmd->siop_table->t_msgout.count =
				    htole32(1);
				siop_cmd->siop_table->t_msgout.addr =
				    htole32(siop_cmd->dsa);
				siop_cmd->siop_table->msg_out[0] =
				    MSG_MESSAGE_REJECT;
			}
			siop_table_sync(siop_cmd,
			    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
			CALL_SCRIPT(Ent_send_msgout);
			return 1;
		case A_int_resel: /* reselected */
			INCSTAT(siop_stat_intr_reselect);
			if ((siop_cmd->siop_table->msg_in[0] & 0x80) == 0) {
				printf("%s: reselect without identify (%d)\n",
				    sc->sc_dev.dv_xname,
				    siop_cmd->siop_table->msg_in[0]);
				goto reset;
			}
			target = bus_space_read_1(sc->sc_rt,
			    sc->sc_rh, SIOP_SCRATCHA);
			if ((target & 0x80) == 0) {
				printf("reselect without id (%d)\n", target);
				goto reset;
			}
			target &= 0x0f;
#ifdef DEBUG_DR
			printf("reselected by target %d lun %d\n",
			    target,
			    siop_cmd->siop_table->msg_in[0] & 0x07);
#endif
			siop_cmd =
			    sc->active_list[target].tqh_first;
			if (siop_cmd == NULL) {
				printf("%s: reselected without cmd\n",
				    sc->sc_dev.dv_xname);
				goto reset;
			}
			bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSA,
			    siop_cmd->dsa);
			/* no table to flush */
			CALL_SCRIPT(Ent_selected);
			return 1;
		case A_int_disc:
			INCSTAT(siop_stat_intr_sdp);
			offset = bus_space_read_1(sc->sc_rt, sc->sc_rh,
			    SIOP_SCRATCHA + 1);
#ifdef DEBUG_DR
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
				memmove(&siop_cmd->siop_table->data[0],
				    &siop_cmd->siop_table->data[offset],
				    (SIOP_NSG - offset) * sizeof(scr_table_t));
				siop_table_sync(siop_cmd,
				    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
			}
			CALL_SCRIPT(Ent_sheduler);
			return 1;
		case A_int_resfail:
			printf("reselect failed\n");
			CALL_SCRIPT(Ent_sheduler);
			return  1;
		case A_int_done:
			if (xs == NULL) {
				printf("%s: done without command\n",
				    sc->sc_dev.dv_xname);
				siop_cmd->status = 0;
				CALL_SCRIPT(Ent_sheduler);
				siop_start(sc);
				return 1;
			}
#if 0
			printf("done, taget id 0x%x last msg in=0x%x "
			    "status=0x%x\n",
			    le32toh(siop_cmd->siop_table->id),
			    siop_cmd->siop_table->msg_in[0],
			    le32toh(siop_cmd->siop_table->status));
#endif
			INCSTAT(siop_stat_intr_done);
			if (siop_cmd->status == CMDST_SENSE_ACTIVE)
				siop_cmd->status = CMDST_SENSE_DONE;
			else
				siop_cmd->status = CMDST_DONE;
			switch(le32toh(siop_cmd->siop_table->status)) {
			case SCSI_OK:
				xs->error = (siop_cmd->status == CMDST_DONE) ?
				    XS_NOERROR : XS_SENSE;
				break;
			case SCSI_BUSY:
				xs->error = XS_BUSY;
				break;
			case SCSI_CHECK:
check_sense:
				if (siop_cmd->status == CMDST_SENSE_DONE) {
					/* request sense on a request sense ? */
					printf("request sense failed\n");
					xs->error = XS_DRIVER_STUFFUP;
				} else {
					siop_cmd->status = CMDST_SENSE;
				}
				break;
			case 0xff:
				/*
				 * the status byte was not updated, cmd was
				 * aborted
				 */
				xs->error = XS_SELTIMEOUT;
				break;
			default:
				xs->error = XS_DRIVER_STUFFUP;
			}
			goto end;
		default:
			printf("unknown irqcode %x\n", irqcode);
			xs->error = XS_SELTIMEOUT;
			goto end;
		}
		return 1;
	}
	/* We just should't get there */
	panic("siop_intr: I shouldn't be there !");
	return 1;
end:
	siop_scsicmd_end(siop_cmd);
	if (siop_cmd->status == CMDST_FREE) {
		TAILQ_REMOVE(&sc->active_list[xs->sc_link->scsipi_scsi.target],
		    siop_cmd, next);
		TAILQ_INSERT_TAIL(&sc->free_list, siop_cmd, next);
	}
	bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSP,
	    sc->sc_scriptdma->dm_segs[0].ds_addr + Ent_reselect);
	siop_start(sc);
	return 1;
}

void
siop_scsicmd_end(siop_cmd)
	struct siop_cmd *siop_cmd;
{
	struct scsipi_xfer *xs = siop_cmd->xs;
	struct siop_softc *sc = siop_cmd->siop_sc;

	if (siop_cmd->status != CMDST_SENSE_DONE &&
	    xs->xs_control & (XS_CTL_DATA_IN | XS_CTL_DATA_OUT)) {
		bus_dmamap_sync(sc->sc_dmat, siop_cmd->dmamap_data, 0,
		    siop_cmd->dmamap_data->dm_mapsize,
		    (xs->xs_control & XS_CTL_DATA_IN) ?
		    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, siop_cmd->dmamap_data);
	}
	bus_dmamap_unload(sc->sc_dmat, siop_cmd->dmamap_cmd);
	if (siop_cmd->status == CMDST_SENSE) {
		/* issue a request sense for this target */
		int error, i;
		siop_cmd->rs_cmd.opcode = REQUEST_SENSE;
		siop_cmd->rs_cmd.byte2 = xs->sc_link->scsipi_scsi.lun << 5;
		siop_cmd->rs_cmd.unused[0] = siop_cmd->rs_cmd.unused[1] = 0;
		siop_cmd->rs_cmd.length = sizeof(struct scsipi_sense_data);
		siop_cmd->rs_cmd.control = 0;
		siop_cmd->siop_table->status = htole32(0xff); /*invalid status*/
		siop_cmd->siop_table->t_msgout.count= htole32(1);
		siop_cmd->siop_table->t_msgout.addr = htole32(siop_cmd->dsa);
		siop_cmd->siop_table->msg_out[0] =
		    MSG_IDENTIFY(xs->sc_link->scsipi_scsi.lun, 1);
		error = bus_dmamap_load(sc->sc_dmat, siop_cmd->dmamap_cmd,
		    &siop_cmd->rs_cmd, sizeof(struct scsipi_sense),
		    NULL, BUS_DMA_NOWAIT);
		if (error) {
			printf("%s: unable to load cmd DMA map: %d",
			    sc->sc_dev.dv_xname, error);
			xs->error = XS_DRIVER_STUFFUP;
			goto out;
		}
		siop_cmd->siop_table->cmd.count = 
		    htole32(siop_cmd->dmamap_cmd->dm_segs[0].ds_len);
		siop_cmd->siop_table->cmd.addr =
		    htole32(siop_cmd->dmamap_cmd->dm_segs[0].ds_addr);
		error = bus_dmamap_load(sc->sc_dmat, siop_cmd->dmamap_data,
		    &xs->sense.scsi_sense, sizeof(struct  scsipi_sense_data),
		    NULL, BUS_DMA_NOWAIT);
		if (error) {
			printf("%s: unable to load sense DMA map: %d",
			    sc->sc_dev.dv_xname, error);
			xs->error = XS_DRIVER_STUFFUP;
			bus_dmamap_unload(sc->sc_dmat, siop_cmd->dmamap_cmd);
			goto out;
		}
		for (i = 0; i < siop_cmd->dmamap_data->dm_nsegs; i++) {
			siop_cmd->siop_table->data[i].count =
			    htole32(siop_cmd->dmamap_data->dm_segs[i].ds_len);
			siop_cmd->siop_table->data[i].addr =
			    htole32(siop_cmd->dmamap_data->dm_segs[i].ds_addr);
		}
		bus_dmamap_sync(sc->sc_dmat, siop_cmd->dmamap_data, 0,
		    siop_cmd->dmamap_data->dm_mapsize, BUS_DMASYNC_PREREAD);
		bus_dmamap_sync(sc->sc_dmat, siop_cmd->dmamap_cmd, 0,
		    siop_cmd->dmamap_cmd->dm_mapsize, BUS_DMASYNC_PREWRITE);
		siop_table_sync(siop_cmd, BUS_DMASYNC_PREWRITE);
#if 0
		bus_dmamap_sync(sc->sc_dmat, sc->sc_scriptdma, CMD_OFF,
		    sc->sc_scriptdma->dm_mapsize - CMD_OFF,
		    BUS_DMASYNC_PREWRITE);
#endif
		return;
	} else if (siop_cmd->status == CMDST_SENSE_DONE) {
		bus_dmamap_sync(sc->sc_dmat, siop_cmd->dmamap_data, 0,
		    siop_cmd->dmamap_data->dm_mapsize, BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, siop_cmd->dmamap_data);
	}
out:
	callout_stop(&siop_cmd->xs->xs_callout);
	siop_cmd->status = CMDST_FREE;
	xs->xs_status |= XS_STS_DONE;
	xs->resid = 0;
	scsipi_done (xs);
}

/*
 * handle a bus reset: reset chip, unqueue all active commands and report
 * loosage to upper layer.
 * As the upper layer may requeue immediatly we have to first store
 * all active commands in a temporary queue.
 */
void
siop_handle_reset(sc)
	struct siop_softc *sc;
{
	struct cmd_list reset_list;
	struct siop_cmd *siop_cmd, *next_siop_cmd;
	int target;
	/*
	 * scsi bus reset. reset the chip and restart
	 * the queue. Need to clean up all active commands
	 */
	printf("%s: scsi bus reset\n", sc->sc_dev.dv_xname);
	/* stop, reset and restart the chip */
	siop_reset(sc);
	TAILQ_INIT(&reset_list);
	/* find all active commands */
	for (target = 0; target < 16; target++) {
		for (siop_cmd = TAILQ_FIRST(&sc->active_list[target]);
		    siop_cmd != NULL; siop_cmd = next_siop_cmd) {
			next_siop_cmd = TAILQ_NEXT(siop_cmd, next);
			if (siop_cmd->status < CMDST_ACTIVE)
				continue;
			printf("cmd %p (target %d) in reset list\n", siop_cmd,
			    target);
			TAILQ_REMOVE( &sc->active_list[target], siop_cmd, next);
			TAILQ_INSERT_TAIL(&reset_list, siop_cmd, next);
		}
	}
	for (siop_cmd = TAILQ_FIRST(&reset_list); siop_cmd != NULL;
	    siop_cmd = next_siop_cmd) {
		next_siop_cmd = TAILQ_NEXT(siop_cmd, next);
		siop_cmd->xs->error = (siop_cmd->flags & CMDFL_TIMEOUT) ?
		    XS_TIMEOUT : XS_RESET;
		printf("cmd %p about to be processed\n", siop_cmd);
		TAILQ_REMOVE(&reset_list, siop_cmd, next);
		siop_scsicmd_end(siop_cmd);
		TAILQ_INSERT_TAIL(&sc->free_list, siop_cmd, next);
	}
}

void
siop_minphys(bp)
	struct buf *bp;
{
	minphys(bp);
}

int
siop_ioctl(link, cmd, arg, flag, p)
	struct scsipi_link *link;
	u_long cmd;
	caddr_t arg;
	int flag;
	struct proc *p;
{
	struct siop_softc *sc = link->adapter_softc;
	u_int8_t scntl1;
	int s;

	switch (cmd) {
	case SCBUSIORESET:
		s = splbio();
		scntl1 = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL1);
		bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL1,
		    scntl1 | SCNTL1_RST);
		/* minimum 25 us, more time won't hurt */
		delay(100);
		bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL1, scntl1);
		splx(s);
		return (0);
	default:
		return (ENOTTY);
	}
}

int
siop_scsicmd(xs)
	struct scsipi_xfer *xs;
{
	struct siop_softc *sc = (struct siop_softc *)xs->sc_link->adapter_softc;
	struct siop_cmd *siop_cmd;
	int s, error, i;
	u_int32_t id;

	s = splbio();
#if 0
	printf("starting cmd for %d:%d\n", xs->sc_link->scsipi_scsi.target,xs->sc_link->scsipi_scsi.lun);
#endif
	siop_cmd = sc->free_list.tqh_first;
	if (siop_cmd) {
		TAILQ_REMOVE(&sc->free_list, siop_cmd, next);
	}
	splx(s);
	if (siop_cmd == NULL) {
		xs->error = XS_DRIVER_STUFFUP;
		return(TRY_AGAIN_LATER);
	}
#ifdef DIAGNOSTIC
	if (siop_cmd->status != CMDST_FREE)
		panic("siop_scsicmd: new cmd not free");
#endif
	siop_cmd->xs = xs;
	id = 0x3 << 24; /* scntl3 */
	id |=  xs->sc_link->scsipi_scsi.target << 16; /* id */
	id |= 0xe0 << 8; /* scxfer */
	siop_cmd->siop_table->id = htole32(id);
	siop_cmd->siop_table->t_msgout.count= htole32(1);
	siop_cmd->siop_table->t_msgout.addr = htole32(siop_cmd->dsa);
	siop_cmd->siop_table->msg_out[0] =
	    MSG_IDENTIFY(xs->sc_link->scsipi_scsi.lun, 1);
	siop_cmd->siop_table->status = htole32(0xff); /* set invalid status */

	/* load the DMA maps */
	error = bus_dmamap_load(sc->sc_dmat, siop_cmd->dmamap_cmd,
	    xs->cmd, xs->cmdlen, NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: unable to load cmd DMA map: %d",
		    sc->sc_dev.dv_xname, error);
		xs->error = XS_DRIVER_STUFFUP;
		return(TRY_AGAIN_LATER);
	}
	siop_cmd->siop_table->cmd.count =
	    htole32(siop_cmd->dmamap_cmd->dm_segs[0].ds_len);
	siop_cmd->siop_table->cmd.addr =
	    htole32(siop_cmd->dmamap_cmd->dm_segs[0].ds_addr);
	if (xs->xs_control & (XS_CTL_DATA_IN | XS_CTL_DATA_OUT)) {
		error = bus_dmamap_load(sc->sc_dmat, siop_cmd->dmamap_data,
		    xs->data, xs->datalen, NULL, BUS_DMA_NOWAIT);
		if (error) {
			printf("%s: unable to load cmd DMA map: %d",
			    sc->sc_dev.dv_xname, error);
			xs->error = XS_DRIVER_STUFFUP;
			return(TRY_AGAIN_LATER);
			bus_dmamap_unload(sc->sc_dmat, siop_cmd->dmamap_cmd);
		}
		for (i = 0; i < siop_cmd->dmamap_data->dm_nsegs; i++) {
			siop_cmd->siop_table->data[i].count =
			    htole32(siop_cmd->dmamap_data->dm_segs[i].ds_len);
			siop_cmd->siop_table->data[i].addr =
			    htole32(siop_cmd->dmamap_data->dm_segs[i].ds_addr);
		}
		bus_dmamap_sync(sc->sc_dmat, siop_cmd->dmamap_data, 0,
		    siop_cmd->dmamap_data->dm_mapsize,
		    (xs->xs_control & XS_CTL_DATA_IN) ?
		    BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
	}
	bus_dmamap_sync(sc->sc_dmat, siop_cmd->dmamap_cmd, 0,
	    siop_cmd->dmamap_cmd->dm_mapsize, BUS_DMASYNC_PREWRITE);
	siop_table_sync(siop_cmd, BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
#if 0
	bus_dmamap_sync(sc->sc_dmat, sc->sc_scriptdma, 0,
	    sc->sc_scriptdma->dm_mapsize, BUS_DMASYNC_PREWRITE);
#endif

	siop_cmd->status = CMDST_READY;
	s = splbio();
	TAILQ_INSERT_TAIL(&sc->active_list[xs->sc_link->scsipi_scsi.target],
	    siop_cmd, next);
	siop_start(sc);
	splx(s);
	return (SUCCESSFULLY_QUEUED);
}

void
siop_start(sc)
	struct siop_softc *sc;
{
	struct siop_cmd *siop_cmd;
	u_int32_t *scr;
	u_int32_t dsa;
	u_int8_t *dsap = (u_int8_t *)&dsa;
	int timeout;
	int target, slot;
	int newcmd = 0; 

	/*
	 * first make sure to read valid data
	 */
	siop_script_sync(sc, BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	/*
	 * no need to restart at slot 0 each time we're looking for a free
	 * slot; init slot before the target loop.
	 */
	slot = 0;
	for (target = 0; target < 16; target++) {
		siop_cmd = sc->active_list[target].tqh_first;
		if (siop_cmd == NULL)
			continue;
		if (siop_cmd->status != CMDST_READY &&
		    siop_cmd->status != CMDST_SENSE)
			continue;
		/* mark command as active (if not reqsense) and start script */
		if (siop_cmd->status == CMDST_READY)
			siop_cmd->status = CMDST_ACTIVE;
		else if (siop_cmd->status == CMDST_SENSE)
			siop_cmd->status = CMDST_SENSE_ACTIVE;
		else
			panic("siop_start: bad status");
		/* find a free sheduler slot and load it */
		for (; slot < sc->sc_nshedslots; slot++) {
			scr = &sc->sc_script[Ent_sheduler / 4 +
			    (Ent_nextslot / 4) * slot];
			/*
			 * if relative addr of first jump is 0 the slot isn't
			 * free
			 */
			if (scr[Ent_slot / 4 + 1] == 0)
				continue;
#ifdef DEBUG_SHED
			printf("using slot %d\n", slot);
#endif
			/* record that we started at last one new comand */
			newcmd = 1;
			/* ok, patch script with DSA addr */
			dsa = siop_cmd->dsa;
			/*
			 * 0x78000000 is a 'move data8 to reg'. data8 is the
			 * second octet, reg offset is the third.
			 */
			scr[Ent_idsa0 / 4] =
			    htole32(0x78100000 | (dsap[0] << 8));
			scr[Ent_idsa1 / 4] =
			    htole32(0x78110000 | (dsap[1] << 8));
			scr[Ent_idsa2 / 4] =
			    htole32(0x78120000 | (dsap[2] << 8));
			scr[Ent_idsa3 / 4] =
			    htole32(0x78130000 | (dsap[3] << 8));
			/* change status of cmd */
			if (siop_cmd->status == CMDST_ACTIVE) {
				if ((siop_cmd->xs->xs_control & XS_CTL_POLL)
				    == 0) {
					/* start exire timer */
					timeout =
					    siop_cmd->xs->timeout * hz / 1000;
					if (timeout == 0)
						timeout = 1;
					callout_reset(&siop_cmd->xs->xs_callout,
					    timeout, siop_timeout, siop_cmd);
				}
			}
			/*
			 * Change jump offset so that this slot will be
			 * handled
			 */
			scr[Ent_slot / 4 + 1] = 0;
			break;
		}
		/* if we didn't find any free slot no need to try next target */
		if (slot == sc->sc_nshedslots)
			break;
	}
	/* if nothing changed no need to flush cache and wakeup script */
	if (newcmd == 0)
		return;
	/* make sure SCRIPT processor will read valid data */
	siop_script_sync(sc, BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
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
	u_int8_t scntl1;

	scsi_print_addr(siop_cmd->xs->sc_link);
	printf("command timeout\n");

	s = splbio();
	/* reset the scsi bus */
	scntl1 = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL1);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL1,
	    scntl1 | SCNTL1_RST);
	/* minimum 25 us, more time won't hurt */
	delay(100);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL1, scntl1);

	/* desactivate callout */
	callout_stop(&siop_cmd->xs->xs_callout);
	/* mark command has being timed out; siop_intr will handle it */
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
siop_sdp(siop_cmd)
	struct siop_cmd *siop_cmd;
{
	/* save data pointer. Handle async only for now */
	int offset, dbc, sstat;
	struct siop_softc *sc = siop_cmd->siop_sc;
	scr_table_t *table; /* table to patch */

	if ((siop_cmd->xs->xs_control & (XS_CTL_DATA_OUT | XS_CTL_DATA_IN))
	    == 0)
	    return; /* no data pointers to save */
	offset = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SCRATCHA + 1);
	if (offset >= SIOP_NSG) {
		printf("%s: bad offset in siop_sdp (%d)\n",
		    sc->sc_dev.dv_xname, offset);
		return;
	}
	table = &siop_cmd->siop_table->data[offset];
#ifdef DEBUG_DR
	printf("sdp: offset %d count=%d addr=0x%x ", offset,
	    table->count, table->addr);
#endif
	dbc = bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DBC) & 0x00ffffff;
	if (siop_cmd->xs->xs_control & XS_CTL_DATA_OUT) {
		/* need to account stale data in FIFO */
		/* XXX check for large fifo */
		dbc += (bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_DFIFO) -
		    (dbc & 0x7f)) & 0x7f;
		sstat = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SSTAT0);
		if (sstat & SSTAT0_OLF)
			dbc++;
		if (sstat & SSTAT0_ORF)
			dbc++;
		/* XXX check sstat1 for wide adapters */
		/* Flush the FIFO */
		bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_CTEST3,
		    bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_CTEST3) |
			CTEST3_CLF);
	}
	table->addr =
	    htole32(le32toh(table->addr) + le32toh(table->count) - dbc);
	table->count = htole32(dbc);
#ifdef DEBUG_DR
	printf("now count=%d addr=0x%x\n", table->count, table->addr);
#endif
}

void
siop_dump_script(sc)
	struct siop_softc *sc;
{
	int i;
	siop_script_sync(sc, BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	for (i = 0; i < CMD_OFF / 4; i += 2) {
		printf("0x%04x: 0x%08x 0x%08x", i * 4,
		    le32toh(sc->sc_script[i]), le32toh(sc->sc_script[i+1]));
		if ((le32toh(sc->sc_script[i]) & 0xe0000000) == 0xc0000000) {
			i++;
			printf(" 0x%08x", le32toh(sc->sc_script[i+1]));
		}
		printf("\n");
	}
}

#ifdef SIOP_STATS
void
siop_printstats()
{
	printf("siop_stat_intr %d\n", siop_stat_intr);
	printf("siop_stat_intr_shortxfer %d\n", siop_stat_intr_shortxfer);
	printf("siop_stat_intr_xferdisc %d\n", siop_stat_intr_xferdisc);
	printf("siop_stat_intr_sdp %d\n", siop_stat_intr_sdp);
	printf("siop_stat_intr_reselect %d\n", siop_stat_intr_reselect);
	printf("siop_stat_intr_done %d\n", siop_stat_intr_done);
}
#endif
