/*	$NetBSD: siop.c,v 1.1 2000/04/21 17:56:58 bouyer Exp $	*/

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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* SYM53c7/8xx PCI-SCSI I/O Processors driver */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/kernel.h>

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

#ifndef SIOP_DEFAULT_TARGET
#define SIOP_DEFAULT_TARGET 7
#endif

#define MEM_SIZE 8192
#define SCRIPT_OFF 4096

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
	u_int8_t msg_out;	/* 0 */
	u_int8_t msg_in;	/* 1 */
	u_int8_t status;	/* 2 */
	u_int8_t pad2;		/* 3 */
	u_int32_t id;		/* 4 */
	u_int32_t pad1;		/* 8 */
	scr_table_t t_msgin;	/* 12 */
	scr_table_t t_msgout;	/* 20 */
	scr_table_t cmd;	/* 28 */
	scr_table_t t_status;	/* 36 */
	scr_table_t data[SIOP_NSG]; /* 44 */
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
	bus_dmamap_t	dmamap_cmd;
	bus_dmamap_t	dmamap_data;
	struct scsipi_sense rs_cmd; /* request sense command buffer */
	u_int16_t       status;
	u_int16_t       flags;
};

/* status defs */
#define CMDST_FREE	0 /* cmd slot is free */
#define CMDST_READY	1 /* cmd slot is waiting for processing */
#define CMDST_ACTIVE	2 /* cmd slot is being processed */
#define CMDST_SENSE	3 /* cmd slot is being requesting sense */
#define CMDST_SENSE_DONE 4 /* request sense done */
#define CMDST_DONE	5 /* cmd slot has been processed */
/* flags defs */
#define CMDFL_TIMEOUT	0x0001 /* cmd timed out */

/* initial number of cmd descriptors */
#define SIOP_NCMD 10

void	siop_reset __P((struct siop_softc *));
void	siop_start __P((struct siop_softc *));
void 	siop_timeout __P((void *));
void	siop_minphys __P((struct buf *));
int	siop_scsicmd __P((struct scsipi_xfer *));
void 	siop_sdp __P((struct siop_cmd *));
void 	siop_ssg __P((struct siop_cmd *));

struct scsipi_adapter siop_adapter = {
	0,
	siop_scsicmd,
	siop_minphys,
	NULL,
};

struct scsipi_device siop_dev = {
	NULL,
	NULL,
	NULL,
	NULL,
};

void
siop_attach(sc)
	struct siop_softc *sc;
{
	int error, i;
	bus_dma_segment_t seg;
	int rseg;
	struct siop_cmd *cmds;

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
	cmds = malloc(sizeof(struct siop_cmd) * SIOP_NCMD, M_DEVBUF, M_NOWAIT);
	if (cmds == NULL) {
		printf("%s: can't allocate memory for command descriptors\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	for (i = 0; i < SIOP_NCMD; i++) {
		error = bus_dmamap_create(sc->sc_dmat, MAXPHYS, SIOP_NSG,
		    MAXPHYS, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
		    &cmds[i].dmamap_data);
		if (error) {
			printf("%s: unable to create data DMA map for cbd %d\n",
			    sc->sc_dev.dv_xname, error);
			return;
		}
		error = bus_dmamap_create(sc->sc_dmat,
		    sizeof(struct scsipi_generic), 1,
		    sizeof(struct scsipi_generic), 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
		    &cmds[i].dmamap_cmd);
		if (error) {
			printf("%s: unable to create cmd DMA map for cbd %d\n",
			    sc->sc_dev.dv_xname, error);
			return;
		}
		cmds[i].siop_sc = sc;
		cmds[i].siop_table = &((struct siop_xfer *)sc->sc_script)[i];
		cmds[i].status = CMDST_FREE;
		cmds[i].siop_table->t_msgout.count= htole32(1);
		cmds[i].siop_table->t_msgout.addr =
		    htole32(sc->sc_scriptdma->dm_segs[0].ds_addr +
		    i * sizeof(struct siop_xfer));
		cmds[i].siop_table->t_msgin.count= htole32(1);
		cmds[i].siop_table->t_msgin.addr =
		    htole32(htole32(cmds[i].siop_table->t_msgout.addr) + 1);
		cmds[i].siop_table->t_status.count= htole32(1);
		cmds[i].siop_table->t_status.addr =
		    htole32(htole32(cmds[i].siop_table->t_msgin.addr) + 1);
		TAILQ_INSERT_TAIL(&sc->free_list, &cmds[i], next);
		printf("tables[%d]: out=0x%x in=0x%x status=0x%x\n", i,
		    cmds[i].siop_table->t_msgin.addr,
		    cmds[i].siop_table->t_msgout.addr,
		    cmds[i].siop_table->t_status.addr);
	}
#ifdef DEBUG
	printf("%s: script size = %d, PHY addr=0x%x, VIRT=%p\n",
	    sc->sc_dev.dv_xname, sizeof(siop_script),
	    (u_int)sc->sc_scriptdma->dm_segs[0].ds_addr, sc->sc_script);
#endif

	sc->sc_link.adapter_softc = sc;
	sc->sc_link.openings = 1;
	sc->sc_link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
	sc->sc_link.scsipi_scsi.max_target  =
	    (sc->features & SF_BUS_WIDE) ? 15 : 7;
	sc->sc_link.scsipi_scsi.max_lun = 7;
	sc->sc_link.scsipi_scsi.adapter_target = bus_space_read_1(sc->sc_rt,
	    sc->sc_rh, SIOP_SCID);
	if (sc->sc_link.scsipi_scsi.adapter_target >
	    sc->sc_link.scsipi_scsi.max_target)
		sc->sc_link.scsipi_scsi.adapter_target = SIOP_DEFAULT_TARGET;
	sc->sc_link.type = BUS_SCSI;
	sc->sc_link.adapter = &siop_adapter;
	sc->sc_link.device = &siop_dev;
	sc->sc_link.flags  = 0;

	siop_reset(sc);

	config_found((struct device*)sc, &sc->sc_link, scsiprint);
}

void
siop_reset(sc)
	struct siop_softc *sc;
{
	/* reset the chip */
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_ISTAT, ISTAT_SRST);
	delay(1000);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_ISTAT, 0);

	/* copy and patch the script */
	memcpy(&sc->sc_script[SCRIPT_OFF/4], siop_script, sizeof(siop_script));

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
}

#if 0
#define CALL_SCRIPT(ent) do {\
	printf ("start script DSA 0x%lx DSP 0x%lx\n", \
	htole32(sc->sc_scriptdma->dm_segs[0].ds_addr + \
	(((caddr_t)siop_cmd->siop_table) - ((caddr_t)sc->sc_script))),\
	htole32(sc->sc_scriptdma->dm_segs[0].ds_addr + SCRIPT_OFF + ent)); \
	bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSA, \
	    htole32(sc->sc_scriptdma->dm_segs[0].ds_addr + \
	    (((caddr_t)siop_cmd->siop_table) - ((caddr_t)sc->sc_script)))); \
bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSP, htole32(sc->sc_scriptdma->dm_segs[0].ds_addr + SCRIPT_OFF + ent)); \
} while (0)
#else
#define CALL_SCRIPT(ent) do {\
	bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSA, \
	    htole32(sc->sc_scriptdma->dm_segs[0].ds_addr + \
	    (((caddr_t)siop_cmd->siop_table) - ((caddr_t)sc->sc_script)))); \
bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSP, htole32(sc->sc_scriptdma->dm_segs[0].ds_addr + SCRIPT_OFF + ent)); \
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
	int offset;

	istat = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_ISTAT);
	if (istat & ISTAT_INTF) {
		printf("INTRF\n");
		bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_ISTAT, ISTAT_INTF);
	}
	if (istat & ISTAT_DIP) {
		u_int32_t *p;
		dstat = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_DSTAT);
		if (dstat & ~(DSTAT_SIR | DSTAT_DFE)) {
		printf("DMA IRQ:");
		if (dstat & DSTAT_IID)
			printf(" Illegal instruction");
		if (dstat & DSTAT_SSI)
			printf(" single step");
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
		p = &sc->sc_script[SCRIPT_OFF/4] +
		    (bus_space_read_4(sc->sc_rt, sc->sc_rh, SIOP_DSP) -
		    (sc->sc_scriptdma->dm_segs[0].ds_addr + SCRIPT_OFF)-8) / 4;
		printf("0x%x 0x%x 0x%x 0x%x\n", p[0], p[1], p[2], p[3]);
		if ((siop_cmd = sc->active_list[sc->current_target].tqh_first))
			printf("last msg_in=0x%x status=0x%x\n",
			    siop_cmd->siop_table->msg_in,
			    siop_cmd->siop_table->status);
		need_reset = 1;
		}
	}
	if (istat & ISTAT_SIP) {
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
		siop_cmd = sc->active_list[sc->current_target].tqh_first;
		if (siop_cmd)
			xs = siop_cmd->xs;
		if (sist0 & SIST0_RST) {
			/* scsi bus reset. reset the chip and restart
			 * the queue.
			 */
			printf("%s: scsi bus reset\n", sc->sc_dev.dv_xname);
			/* if we had a command running handle it */
			if (siop_cmd) {
				xs = siop_cmd->xs;
				if (siop_cmd->status == CMDST_ACTIVE ||
				    siop_cmd->status == CMDST_SENSE)
					xs->error =
					    (siop_cmd->flags & CMDFL_TIMEOUT) ?
					    XS_TIMEOUT : XS_RESET;
			}
			siop_reset(sc);
			if (siop_cmd)
				goto end;
			siop_start(sc);
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
				switch (sstat1 & SSTAT1_PHASE_MASK) {
				case SSTAT1_PHASE_STATUS:
				/*
				 * previous phase may be aborted for any reason
				 * ( for example, the target has less data to
				 * transfer than requested). Just go to status
				 * and the command should terminate.
				 */
					CALL_SCRIPT(Ent_status);
					return 1;
				case SSTAT1_PHASE_MSGIN:
					/*
					 * target may be ready to disconnect
					 * Save data pointers just in case.
					 */
					siop_sdp(siop_cmd);
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
			need_reset = 1;
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
		return 1;
	}


	siop_cmd = sc->active_list[sc->current_target].tqh_first;
	if (siop_cmd == NULL) {
		printf("Aie, no command !\n");
		return(1);
	}
	xs = siop_cmd->xs;
	if ((istat & ISTAT_DIP) && (dstat & DSTAT_SIR)) { /* script interrupt */
		irqcode = bus_space_read_4(sc->sc_rt, sc->sc_rh,
		    SIOP_DSPS);
		switch(irqcode) {
		case A_int_err:
			printf("error\n");
			xs->error = XS_SELTIMEOUT;
			goto end;
		case A_int_msgin:
			scsi_print_addr(xs->sc_link);
			printf("unhandled message %d\n",
			    siop_cmd->siop_table->msg_in);
			xs->error = XS_DRIVER_STUFFUP;
			goto end;
		case A_int_resel: /* reselected */
			if ((siop_cmd->siop_table->msg_in & 0x80) == 0) {
				printf("%s: reselect without identify (%d)\n",
				    sc->sc_dev.dv_xname,
				    siop_cmd->siop_table->msg_in);
				goto reset;
			}
			sc->current_target = bus_space_read_1(sc->sc_rt,
			    sc->sc_rh, SIOP_SCRATCHA);
			if ((sc->current_target & 0x80) == 0) {
				printf("reselect without id (%d)\n",
				    sc->current_target);
				goto reset;
			}
			sc->current_target &= 0x0f;
#ifdef DEBUG_DR
			printf("reselected by target %d lun %d\n",
			    sc->current_target,
			    siop_cmd->siop_table->msg_in & 0x07);
#endif
			siop_cmd =
			    sc->active_list[sc->current_target].tqh_first;
			if (siop_cmd == NULL) {
				printf("%s: reselected without cmd\n",
				    sc->sc_dev.dv_xname);
				goto reset;
			}
			bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSA,
			    htole32(sc->sc_scriptdma->dm_segs[0].ds_addr + 
				(((caddr_t)siop_cmd->siop_table) -
				((caddr_t)sc->sc_script))));
			CALL_SCRIPT(Ent_reselected);
			return 1;
		case A_int_disc:
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
			}
			CALL_SCRIPT(Ent_reselect);
			return 1;
		case A_int_resfail:
			printf("reselect failed\n");
			CALL_SCRIPT(Ent_reselect);
			return  1;
		case A_int_done:
#if 0
			printf("done, taget id 0x%x last msg in=0x%x "
			    "status=0x%x\n",
			    siop_cmd->siop_table->id,
			    siop_cmd->siop_table->msg_in,
			    siop_cmd->siop_table->status);
#endif
			if (siop_cmd->status == CMDST_SENSE)
				siop_cmd->status = CMDST_SENSE_DONE;
			else
				siop_cmd->status = CMDST_DONE;
			switch(siop_cmd->siop_table->status) {
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
					/* request sense on a request sense ? */					printf("request sense failed\n");
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
	siop_cmd->status = CMDST_DONE;
	xs->error = XS_BUSY;
	goto end;
end:
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
		siop_cmd->siop_table->status = 0xff; /* set invalid status */
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
		bus_dmamap_sync(sc->sc_dmat, sc->sc_scriptdma, 0,
		    sc->sc_scriptdma->dm_mapsize, BUS_DMASYNC_PREWRITE);
		siop_start(sc);
		return(1);
	} else if (siop_cmd->status == CMDST_SENSE_DONE) {
		bus_dmamap_sync(sc->sc_dmat, siop_cmd->dmamap_data, 0,
		    siop_cmd->dmamap_data->dm_mapsize, BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, siop_cmd->dmamap_data);
	}
out:
	callout_stop(&siop_cmd->xs->xs_callout);
	TAILQ_REMOVE(&sc->active_list[sc->current_target], siop_cmd, next);
	siop_cmd->status = CMDST_FREE;
	TAILQ_INSERT_TAIL(&sc->free_list, siop_cmd, next);
	xs->xs_status |= XS_STS_DONE;
	xs->resid = 0;
	scsipi_done (xs);
	/* restart any pending cmd */
	siop_start(sc);
	return 1;
}

void
siop_minphys(bp)
	struct buf *bp;
{
	minphys(bp);
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
	siop_cmd->siop_table->msg_out =
	    MSG_IDENTIFY(xs->sc_link->scsipi_scsi.lun, 1);
	siop_cmd->siop_table->status = 0xff; /* set invalid status */

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
	bus_dmamap_sync(sc->sc_dmat, sc->sc_scriptdma, 0,
	    sc->sc_scriptdma->dm_mapsize, BUS_DMASYNC_PREWRITE);

	siop_cmd->status = CMDST_READY;
	s = splbio();
	TAILQ_INSERT_TAIL(&sc->active_list[xs->sc_link->scsipi_scsi.target],
	    siop_cmd, next);
	if ((sc->sc_flags & SC_CTRL_ACTIVE) == 0) {
		siop_start(sc);
	}
	splx(s);
	return (SUCCESSFULLY_QUEUED);
}

void
siop_start(sc)
	struct siop_softc *sc;
{
	struct siop_cmd *siop_cmd;
	int timeout;
	int i;

	for (i = 0; i < 16; i++) {
		siop_cmd = sc->active_list[i].tqh_first;
		if (siop_cmd) {
			sc->current_target = i;
			break;
		}
	}
	if (siop_cmd == NULL) {
		sc->sc_flags &= ~SC_CTRL_ACTIVE;
		return;
	}

	sc->sc_flags |= SC_CTRL_ACTIVE;
	if (siop_cmd->status == CMDST_READY &&
	    (siop_cmd->xs->xs_control & XS_CTL_POLL) == 0) {
		/* start exire timer */
		timeout = siop_cmd->xs->timeout * hz / 1000;
		if (timeout == 0)
			timeout = 1; /* at last one tick */
		callout_reset(&siop_cmd->xs->xs_callout, timeout,
		    siop_timeout, siop_cmd);
	}
	/* load DSA */
	bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSA,
	    htole32(sc->sc_scriptdma->dm_segs[0].ds_addr +
	    (((caddr_t)siop_cmd->siop_table) - ((caddr_t)sc->sc_script))));
	/* mark command as active (if not reqsense) and start script */
	if (siop_cmd->status == CMDST_READY)
		siop_cmd->status = CMDST_ACTIVE;
#if 0
	printf("starting script, DSA 0x%lx\n", 
		htole32(sc->sc_scriptdma->dm_segs[0].ds_addr +
		   (((caddr_t)siop_cmd->siop_table) - ((caddr_t)sc->sc_script))));
#endif
	bus_space_write_4(sc->sc_rt, sc->sc_rh, SIOP_DSP,
	    htole32(sc->sc_scriptdma->dm_segs[0].ds_addr + SCRIPT_OFF + Ent_select));
	/* now wait for IRQ */
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
	    htole32(htole32(table->addr) + htole32(table->count) - dbc);
	table->count = htole32(dbc);
#ifdef DEBUG_DR
	printf("now count=%d addr=0x%x\n", table->count, table->addr);
#endif
}
