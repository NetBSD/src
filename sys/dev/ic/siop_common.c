/*	$NetBSD: siop_common.c,v 1.2 2000/05/15 15:16:59 bouyer Exp $	*/

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
#include <sys/scsiio.h>

#include <machine/endian.h>
#include <machine/bus.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_kern.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsipi_all.h>

#include <dev/scsipi/scsiconf.h>

#include <dev/ic/siopreg.h>
#include <dev/ic/siopvar.h>
#include <dev/ic/siopvar_common.h>

#undef DEBUG
#undef DEBUG_DR

void
siop_common_reset(sc)
	struct siop_softc *sc;
{
	u_int32_t stest3;

	/* reset the chip */
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_ISTAT, ISTAT_SRST);
	delay(1000);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_ISTAT, 0);

	/* init registers */
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL0,
	    SCNTL0_ARB_MASK | SCNTL0_EPC | SCNTL0_AAP);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL1, 0);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL3, sc->clock_div);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCXFER, 0);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_DIEN, 0xff);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SIEN0,
	    0xff & ~(SIEN0_CMP | SIEN0_SEL | SIEN0_RSL));
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SIEN1,
	    0xff & ~(SIEN1_HTH | SIEN1_GEN));
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_STEST2, 0);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_STEST3, STEST3_TE);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_STIME0,
	    (0xb << STIME0_SEL_SHIFT));
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCID,
	    sc->sc_link.scsipi_scsi.adapter_target | SCID_RRE);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_RESPID0,
	    1 << sc->sc_link.scsipi_scsi.adapter_target);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_DCNTL,
	    (sc->features & SF_CHIP_PF) ? DCNTL_COM | DCNTL_PFEN : DCNTL_COM);

	/* enable clock doubler or quadruler if appropriate */
	if (sc->features & (SF_CHIP_DBLR | SF_CHIP_QUAD)) {
		stest3 = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_STEST3);
		bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_STEST1,
		    STEST1_DBLEN);
		if (sc->features & SF_CHIP_QUAD) {
			/* wait for PPL to lock */
			while ((bus_space_read_1(sc->sc_rt, sc->sc_rh,
			    SIOP_STEST4) & STEST4_LOCK) == 0)
				delay(10);
		} else {
			/* data sheet says 20us - more won't hurt */
			delay(100);
		}
		/* halt scsi clock, select doubler/quad, restart clock */
		bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_STEST3,
		    stest3 | STEST3_HSC);
		bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_STEST1,
		    STEST1_DBLEN | STEST1_DBLSEL);
		bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_STEST3, stest3);
	} else {
		bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_STEST1, 0);
	}
	if (sc->features & SF_CHIP_FIFO)
		bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_CTEST5,
		    bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_CTEST5) |
		    CTEST5_DFS);
		
	sc->sc_reset(sc);
}

int
siop_wdtr_neg(siop_cmd)
	struct siop_cmd *siop_cmd;
{
	struct siop_softc *sc = siop_cmd->siop_target->siop_sc;
	struct siop_target *siop_target = siop_cmd->siop_target;
	int target = siop_cmd->xs->sc_link->scsipi_scsi.target;

	if (siop_target->status == TARST_WIDE_NEG) {
		/* we initiated wide negotiation */
		switch (siop_cmd->siop_table->msg_in[3]) {
		case MSG_EXT_WDTR_BUS_8_BIT:
			printf("%s: target %d using 8bit transfers\n",
			    sc->sc_dev.dv_xname, target);
			siop_target->flags &= ~SF_BUS_WIDE;
			sc->targets[target]->id &= ~(SCNTL3_EWS << 24);
			break;
		case MSG_EXT_WDTR_BUS_16_BIT:
			if (sc->features & SF_BUS_WIDE) {
				printf("%s: target %d using 16bit transfers\n",
				    sc->sc_dev.dv_xname, target);
				siop_target->flags |= TARF_WIDE;
				sc->targets[target]->id |= (SCNTL3_EWS << 24);
				break;
			}
		/* FALLTHROUH */
		default:
			/*
 			 * hum, we got more than what we can handle, shoudn't
			 * happen. Reject, and stay async
			 */
			siop_target->flags &= ~TARF_WIDE;
			siop_target->status = TARST_OK;
			printf("%s: rejecting invalid wide negotiation from "
			    "target %d (%d)\n", sc->sc_dev.dv_xname, target,
			    siop_cmd->siop_table->msg_in[3]);
			siop_cmd->siop_table->t_msgout.count= htole32(1);
			siop_cmd->siop_table->t_msgout.addr =
			    htole32(siop_cmd->dsa);
			siop_cmd->siop_table->msg_out[0] = MSG_MESSAGE_REJECT;
			return SIOP_NEG_MSGOUT;
		}
		siop_cmd->siop_table->id =
		    htole32(sc->targets[target]->id);
		bus_space_write_1(sc->sc_rt, sc->sc_rh,
		    SIOP_SCNTL3,
		    (sc->targets[target]->id >> 24) & 0xff);
		/* we now need to do sync */
		siop_target->status = TARST_SYNC_NEG;
		siop_cmd->siop_table->msg_out[0] = MSG_EXTENDED;
		siop_cmd->siop_table->msg_out[1] = MSG_EXT_SDTR_LEN;
		siop_cmd->siop_table->msg_out[2] = MSG_EXT_SDTR;
		siop_cmd->siop_table->msg_out[3] = sc->minsync;
		siop_cmd->siop_table->msg_out[4] = sc->maxoff;
		siop_cmd->siop_table->t_msgout.count =
		    htole32(MSG_EXT_SDTR_LEN + 2);
		siop_cmd->siop_table->t_msgout.addr = htole32(siop_cmd->dsa);
		return SIOP_NEG_MSGOUT;
	} else {
		/* target initiated wide negotiation */
		if (siop_cmd->siop_table->msg_in[3] >= MSG_EXT_WDTR_BUS_16_BIT
		    && (sc->features & SF_BUS_WIDE)) {
			printf("%s: target %d using 16bit transfers\n",
			    sc->sc_dev.dv_xname, target);
			siop_target->flags |= TARF_WIDE;
			sc->targets[target]->id |= SCNTL3_EWS << 24;
			siop_cmd->siop_table->msg_out[3] =
			    MSG_EXT_WDTR_BUS_16_BIT;
		} else {
			printf("%s: target %d using 8bit transfers\n",
			    sc->sc_dev.dv_xname, target);
			siop_target->flags &= ~SF_BUS_WIDE;
			sc->targets[target]->id &= ~(SCNTL3_EWS << 24);
			siop_cmd->siop_table->msg_out[3] =
			    MSG_EXT_WDTR_BUS_8_BIT;
		}
		siop_cmd->siop_table->id =
		    htole32(sc->targets[target]->id);
		bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL3,
		    (sc->targets[target]->id >> 24) & 0xff);
		/*
		 * we did reset wide parameters, so fall back to async,
		 * but don't shedule a sync neg, target should initiate it
		 */
		siop_target->status = TARST_OK;
		siop_cmd->siop_table->msg_out[0] = MSG_EXTENDED;
		siop_cmd->siop_table->msg_out[1] = MSG_EXT_WDTR_LEN;
		siop_cmd->siop_table->msg_out[2] = MSG_EXT_WDTR;
		siop_cmd->siop_table->t_msgout.count=
		    htole32(MSG_EXT_WDTR_LEN + 2);
		siop_cmd->siop_table->t_msgout.addr = htole32(siop_cmd->dsa);
		return SIOP_NEG_MSGOUT;
	}
}

int
siop_sdtr_neg(siop_cmd)
	struct siop_cmd *siop_cmd;
{
	struct siop_softc *sc = siop_cmd->siop_target->siop_sc;
	struct siop_target *siop_target = siop_cmd->siop_target;
	int target = siop_cmd->xs->sc_link->scsipi_scsi.target;
	int sync, offset, i;
	int send_msgout = 0;

	sync = siop_cmd->siop_table->msg_in[3];
	offset = siop_cmd->siop_table->msg_in[4];

	if (siop_target->status == TARST_SYNC_NEG) {
		/* we initiated sync negotiation */
		siop_target->status = TARST_OK;
#ifdef DEBUG
		printf("sdtr: sync %d offset %d\n", sync, offset);
#endif
		if (offset > sc->maxoff || sync < sc->minsync ||
			sync > sc->maxsync)
			goto reject;
		for (i = 0; i < sizeof(scf_period) / sizeof(scf_period[0]);
		    i++) {
			if (sc->clock_period != scf_period[i].clock)
				continue;
			if (scf_period[i].period == sync) {
				/* ok, found it. we now are sync. */
				printf("%s: target %d now synchronous at "
				    "%sMhz, offset %d\n", sc->sc_dev.dv_xname,
				    target, scf_period[i].rate, offset);
				sc->targets[target]->id &=
				    ~(SCNTL3_SCF_MASK << 24);
				sc->targets[target]->id |= scf_period[i].scf
				    << (24 + SCNTL3_SCF_SHIFT);
				if (sync < 25) /* Ultra */
					sc->targets[target]->id |=
					    SCNTL3_ULTRA << 24;
				else
					sc->targets[target]->id &=
					    ~(SCNTL3_ULTRA << 24);
				sc->targets[target]->id &=
				    ~(SCXFER_MO_MASK << 8);
				sc->targets[target]->id |=
				    (offset & SCXFER_MO_MASK) << 8;
				goto end;
			}
		}
		/*
		 * we didn't find it in our table, do async and send reject
		 * msg
		 */
reject:
		send_msgout = 1;
		siop_cmd->siop_table->t_msgout.count= htole32(1);
		siop_cmd->siop_table->msg_out[0] = MSG_MESSAGE_REJECT;
		printf("%s: target %d asynchronous\n", sc->sc_dev.dv_xname,
		    target);
		sc->targets[target]->id &= ~(SCNTL3_SCF_MASK << 24);
		sc->targets[target]->id &= ~(SCNTL3_ULTRA << 24);
		sc->targets[target]->id &= ~(SCXFER_MO_MASK << 8);
	} else { /* target initiated sync neg */
#ifdef DEBUG
		printf("sdtr (target): sync %d offset %d\n", sync, offset);
#endif
		if (offset == 0 || sync > sc->maxsync) { /* async */
			goto async;
		}
		if (offset > sc->maxoff)
			offset = sc->maxoff;
		if (sync < sc->minsync)
			sync = sc->minsync;
		/* look for sync period */
		for (i = 0; i < sizeof(scf_period) / sizeof(scf_period[0]);
		    i++) {
			if (sc->clock_period != scf_period[i].clock)
				continue;
			if (scf_period[i].period == sync) {
				/* ok, found it. we now are sync. */
				printf("%s: target %d now synchronous at "
				    "%sMhz, offset %d\n", sc->sc_dev.dv_xname,
				    target, scf_period[i].rate, offset);
				sc->targets[target]->id &=
				    ~(SCNTL3_SCF_MASK << 24);
				sc->targets[target]->id |= scf_period[i].scf
				    << (24 + SCNTL3_SCF_SHIFT);
				if (sync < 25) /* Ultra */
					sc->targets[target]->id |=
					    SCNTL3_ULTRA << 24;
				else
					sc->targets[target]->id &=
					    ~(SCNTL3_ULTRA << 24);
				sc->targets[target]->id &=
				    ~(SCXFER_MO_MASK << 8);
				sc->targets[target]->id |=
				    (offset & SCXFER_MO_MASK) << 8;
				siop_cmd->siop_table->msg_out[0] = MSG_EXTENDED;
				siop_cmd->siop_table->msg_out[1] =
				    MSG_EXT_SDTR_LEN;
				siop_cmd->siop_table->msg_out[2] = MSG_EXT_SDTR;
				siop_cmd->siop_table->msg_out[3] = sync;
				siop_cmd->siop_table->msg_out[4] = offset;
				siop_cmd->siop_table->t_msgout.count=
				    htole32(MSG_EXT_SDTR_LEN + 2);
				send_msgout = 1;
				goto end;
			}
		}
async:
		printf("%s: target %d asynchronous\n",
		    sc->sc_dev.dv_xname, target);
		sc->targets[target]->id &= ~(SCNTL3_SCF_MASK << 24);
		sc->targets[target]->id &= ~(SCNTL3_ULTRA << 24);
		sc->targets[target]->id &= ~(SCXFER_MO_MASK << 8);
		siop_cmd->siop_table->msg_out[0] = MSG_EXTENDED;
		siop_cmd->siop_table->msg_out[1] = MSG_EXT_SDTR_LEN;
		siop_cmd->siop_table->msg_out[2] = MSG_EXT_SDTR;
		siop_cmd->siop_table->msg_out[3] = 0;
		siop_cmd->siop_table->msg_out[4] = 0;
		siop_cmd->siop_table->t_msgout.count=
		    htole32(MSG_EXT_SDTR_LEN + 2);
		send_msgout = 1;
	}
end:
#ifdef DEBUG
	printf("id now 0x%x\n", sc->targets[target]->id);
#endif
	siop_cmd->siop_table->id = htole32(sc->targets[target]->id);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCNTL3,
	    (sc->targets[target]->id >> 24) & 0xff);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_SCXFER,
	    (sc->targets[target]->id >> 8) & 0xff);
	if (send_msgout) {
		siop_cmd->siop_table->t_msgout.addr = htole32(siop_cmd->dsa);
		return SIOP_NEG_MSGOUT;
	} else {
		return SIOP_NEG_ACK;
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

void
siop_sdp(siop_cmd)
	struct siop_cmd *siop_cmd;
{
	/* save data pointer. Handle async only for now */
	int offset, dbc, sstat;
	struct siop_softc *sc = siop_cmd->siop_target->siop_sc;
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
		int dfifo = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_DFIFO);
		if (sc->features & SF_CHIP_FIFO) {
			dfifo |= (bus_space_read_1(sc->sc_rt, sc->sc_rh,
			    SIOP_CTEST5) & CTEST5_BOMASK) << 8;
			dbc += (dfifo - (dbc & 0x3ff)) & 0x3ff;
		} else {
			dbc += (dfifo - (dbc & 0x7f)) & 0x7f;
		}
		sstat = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_SSTAT0);
		if (sstat & SSTAT0_OLF)
			dbc++;
		if (sstat & SSTAT0_ORF)
			dbc++;
		if (siop_cmd->siop_target->flags & TARF_WIDE) {
			sstat = bus_space_read_1(sc->sc_rt, sc->sc_rh,
			    SIOP_SSTAT2);
			if (sstat & SSTAT2_OLF1)
				dbc++;
			if (sstat & SSTAT2_ORF1)
				dbc++;
		}
		/* clear the FIFO */
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
siop_clearfifo(sc)
	struct siop_softc *sc;
{
	int timeout = 0;
	int ctest3 = bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_CTEST3);

#ifdef DEBUG_INTR
	printf("DMA fifo not empty !\n");
#endif
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_CTEST3,
	    ctest3 | CTEST3_CLF);
	while ((bus_space_read_1(sc->sc_rt, sc->sc_rh, SIOP_CTEST3) &
	    CTEST3_CLF) != 0) {
		delay(1);
		if (++timeout > 1000) {
			printf("clear fifo failed\n");
			bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_CTEST3,
			    bus_space_read_1(sc->sc_rt, sc->sc_rh,
			    SIOP_CTEST3) & ~CTEST3_CLF);
			return;
		}
	}
}
