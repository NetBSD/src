/*	$NetBSD: siop.c,v 1.10 2022/05/23 19:21:30 andvar Exp $	*/
/*
 * Copyright (c) 2010 KIYOHARA Takashi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <dev/microcode/siop/siop.out>

#include "boot.h"
#include "sdvar.h"

#ifdef DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)
#endif

#define ALLOC(T, A)	\
		(T *)(((uint32_t)alloc(sizeof(T) + (A)) + (A)) & ~((A) - 1))
#define VTOPHYS(va)	(uint32_t)(va)
#define DEVTOV(pa)	(uint32_t)(pa)
#define wbinv(adr, siz)	_wbinv(VTOPHYS(adr), (uint32_t)(siz))
#define inv(adr, siz)	_inv(VTOPHYS(adr), (uint32_t)(siz))

/* 53c810 supports little endian */
#define htoc32(x)	htole32(x)
#define ctoh32(x)	le32toh(x)

static void siop_pci_reset(int);

static void siop_setuptables(struct siop_adapter *, struct siop_xfer *,
			     struct scsi_xfer *);
static void siop_ma(struct siop_adapter *, struct scsi_xfer *);
static void siop_sdp(struct siop_adapter *, struct siop_xfer *,
		     struct scsi_xfer *, int);
static void siop_update_resid(struct siop_adapter *, struct siop_xfer *,
			      struct scsi_xfer *, int);

static int siop_intr(struct siop_adapter *);
static void siop_scsicmd_end(struct siop_adapter *, struct scsi_xfer *);
static int siop_scsi_request(struct siop_adapter *, struct scsi_xfer *);
static void siop_start(struct siop_adapter *, struct scsi_xfer *);
static void siop_xfer_setup(struct siop_xfer *, void *);

static int siop_add_reselsw(struct siop_adapter *, int, int);
static void siop_update_scntl3(struct siop_adapter *, int, int);

static int _scsi_inquire(struct siop_adapter *, int, int, int, char *);
static void scsi_request_sense(struct siop_adapter *, struct scsi_xfer *);
static int scsi_interpret_sense(struct siop_adapter *, struct scsi_xfer *);
static int scsi_probe(struct siop_adapter *);

static struct siop_adapter adapt;


static void
siop_pci_reset(int addr)
{
	int dmode, ctest5;
	const int maxburst = 4;			/* 53c810 */

	dmode = readb(addr + SIOP_DMODE);

	ctest5 = readb(addr + SIOP_CTEST5);
	writeb(addr + SIOP_CTEST4, readb(addr + SIOP_CTEST4) & ~CTEST4_BDIS);
	ctest5 &= ~CTEST5_BBCK;
	ctest5 |= (maxburst - 1) & CTEST5_BBCK;
	writeb(addr + SIOP_CTEST5, ctest5);

	dmode |= DMODE_ERL;
	dmode &= ~DMODE_BL_MASK;
	dmode |= ((maxburst - 1) << DMODE_BL_SHIFT) & DMODE_BL_MASK;
	writeb(addr + SIOP_DMODE, dmode);
}


static void
siop_setuptables(struct siop_adapter *adp, struct siop_xfer *xfer,
		 struct scsi_xfer *xs)
{
	int msgoffset = 1;

	xfer->siop_tables.id =
	    htoc32((adp->clock_div << 24) | (xs->target << 16));
	memset(xfer->siop_tables.msg_out, 0, sizeof(xfer->siop_tables.msg_out));
	/* request sense doesn't disconnect */
	if (xs->cmd->opcode == SCSI_REQUEST_SENSE)
		xfer->siop_tables.msg_out[0] = MSG_IDENTIFY(xs->lun, 0);
	else
		xfer->siop_tables.msg_out[0] = MSG_IDENTIFY(xs->lun, 1);

	xfer->siop_tables.t_msgout.count = htoc32(msgoffset);
	xfer->siop_tables.status =
	    htoc32(SCSI_SIOP_NOSTATUS); /* set invalid status */

	xfer->siop_tables.cmd.count = htoc32(xs->cmdlen);
	xfer->siop_tables.cmd.addr = htoc32(local_to_PCI((u_long)xs->cmd));
	if (xs->datalen != 0) {
		xfer->siop_tables.data[0].count = htoc32(xs->datalen);
		xfer->siop_tables.data[0].addr =
		    htoc32(local_to_PCI((u_long)xs->data));
	}
}

static void
siop_ma(struct siop_adapter *adp, struct scsi_xfer *xs)
{
	int offset, dbc;

	/*
	 * compute how much of the current table didn't get handled when
	 * a phase mismatch occurs
	 */
	if (xs->datalen == 0)
	    return; /* no valid data transfer */

	offset = readb(adp->addr + SIOP_SCRATCHA + 1);
	if (offset >= SIOP_NSG) {
		printf("bad offset in siop_sdp (%d)\n", offset);
		return;
	}
	dbc = readl(adp->addr + SIOP_DBC) & 0x00ffffff;
	xs->resid = dbc;
}

static void
siop_clearfifo(struct siop_adapter *adp)
{
	int timo = 0;
	uint8_t ctest3 = readb(adp->addr + SIOP_CTEST3);

	DPRINTF(("DMA FIFO not empty!\n"));
	writeb(adp->addr + SIOP_CTEST3, ctest3 | CTEST3_CLF);
	while ((readb(adp->addr + SIOP_CTEST3) & CTEST3_CLF) != 0) {
		delay(1);
		if (++timo > 1000) {
			printf("Clear FIFO failed!\n");
			writeb(adp->addr + SIOP_CTEST3,
			    readb(adp->addr + SIOP_CTEST3) & ~CTEST3_CLF);
			return;
		}
	}
}

static void
siop_sdp(struct siop_adapter *adp, struct siop_xfer *xfer, struct scsi_xfer *xs,
	 int offset)
{

	if (xs->datalen == 0)
	    return; /* no data pointers to save */

	/*
	 * offset == SIOP_NSG may be a valid condition if we get a Save data
	 * pointer when the xfer is done. Just ignore the Save data pointer
	 * in this case
	 */
	if (offset == SIOP_NSG)
		return;
	/*
	 * Save data pointer. We do this by adjusting the tables to point
	 * at the begginning of the data not yet transferred.
	 * offset points to the first table with untransferred data.
	 */

	/*
	 * before doing that we decrease resid from the amount of data which
	 * has been transferred.
	 */
	siop_update_resid(adp, xfer, xs, offset);

#if 0
	/*
	 * First let see if we have a resid from a phase mismatch. If so,
	 * we have to adjst the table at offset to remove transferred data.
	 */
	if (siop_cmd->flags & CMDFL_RESID) {
		scr_table_t *table;

		siop_cmd->flags &= ~CMDFL_RESID;
		table = &xfer->siop_tables.data[offset];
		/* "cut" already transferred data from this table */
		table->addr =
		    htoc32(ctoh32(table->addr) + ctoh32(table->count) -
							siop_cmd->resid);
		table->count = htoc32(siop_cmd->resid);
	}
#endif

	/*
	 * now we can remove entries which have been transferred.
	 * We just move the entries with data left at the beginning of the
	 * tables
	 */
	memmove(xfer->siop_tables.data, &xfer->siop_tables.data[offset],
	    (SIOP_NSG - offset) * sizeof(scr_table_t));
}

static void
siop_update_resid(struct siop_adapter *adp, struct siop_xfer *xfer,
		  struct scsi_xfer *xs, int offset)
{
	int i;

	if (xs->datalen == 0)
	    return; /* no data to transfer */

	/*
	 * update resid. First account for the table entries which have
	 * been fully completed.
	 */
	for (i = 0; i < offset; i++)
		xs->resid -= ctoh32(xfer->siop_tables.data[i].count);
#if 0
	/*
	 * if CMDFL_RESID is set, the last table (pointed by offset) is a
	 * partial transfers. If not, offset points to the entry folloing
	 * the last full transfer.
	 */
	if (siop_cmd->flags & CMDFL_RESID) {
		scr_table_t *table = &xfer->siop_tables.data[offset];

		xs->resid -= ctoh32(table->count) - xs->resid;
	}
#endif
}


#define CALL_SCRIPT(ent)	writel(adp->addr + SIOP_DSP, scriptaddr + ent);

static int
siop_intr(struct siop_adapter *adp)
{
	struct siop_xfer *siop_xfer = NULL;
	struct scsi_xfer *xs = NULL;
	u_long scriptaddr = local_to_PCI((u_long)adp->script);
	int offset, target, lun, tag, restart = 0, need_reset = 0;
	uint32_t dsa, irqcode;
	uint16_t sist;
	uint8_t dstat, sstat1, istat;

	istat = readb(adp->addr + SIOP_ISTAT);
	if ((istat & (ISTAT_INTF | ISTAT_DIP | ISTAT_SIP)) == 0)
		return 0;
	if (istat & ISTAT_INTF) {
		printf("INTRF\n");
		writeb(adp->addr + SIOP_ISTAT, ISTAT_INTF);
	}
	if ((istat & (ISTAT_DIP | ISTAT_SIP | ISTAT_ABRT)) ==
	    (ISTAT_DIP | ISTAT_ABRT))
		/* clear abort */
		writeb(adp->addr + SIOP_ISTAT, 0);
	/* use DSA to find the current siop_cmd */
	dsa = readl(adp->addr + SIOP_DSA);
	if (dsa >= local_to_PCI((u_long)adp->xfer) &&
	    dsa < local_to_PCI((u_long)adp->xfer) + SIOP_TABLE_SIZE) {
		dsa -= local_to_PCI((u_long)adp->xfer);
		siop_xfer = adp->xfer;
		_inv((u_long)siop_xfer, sizeof(*siop_xfer));

		xs = adp->xs;
	}

	if (istat & ISTAT_DIP)
		dstat = readb(adp->addr + SIOP_DSTAT);
	if (istat & ISTAT_SIP) {
		if (istat & ISTAT_DIP)
			delay(10);
		/*
		 * Can't read sist0 & sist1 independently, or we have to
		 * insert delay
		 */
		sist = readw(adp->addr + SIOP_SIST0);
		sstat1 = readb(adp->addr + SIOP_SSTAT1);

		if ((sist & SIST0_MA) && need_reset == 0) {
			if (siop_xfer) {
				int scratcha0;

				dstat = readb(adp->addr + SIOP_DSTAT);
				/*
				 * first restore DSA, in case we were in a S/G
				 * operation.
				 */
				writel(adp->addr + SIOP_DSA,
				    local_to_PCI((u_long)siop_xfer));
				scratcha0 = readb(adp->addr + SIOP_SCRATCHA);
				switch (sstat1 & SSTAT1_PHASE_MASK) {
				case SSTAT1_PHASE_STATUS:
				/*
				 * previous phase may be aborted for any reason
				 * ( for example, the target has less data to
				 * transfer than requested). Compute resid and
				 * just go to status, the command should
				 * terminate.
				 */
					if (scratcha0 & A_flag_data)
						siop_ma(adp, xs);
					else if ((dstat & DSTAT_DFE) == 0)
						siop_clearfifo(adp);
					CALL_SCRIPT(Ent_status);
					return 1;
				case SSTAT1_PHASE_MSGIN:
				/*
				 * target may be ready to disconnect
				 * Compute resid which would be used later
				 * if a save data pointer is needed.
				 */
					if (scratcha0 & A_flag_data)
						siop_ma(adp, xs);
					else if ((dstat & DSTAT_DFE) == 0)
						siop_clearfifo(adp);
					writeb(adp->addr + SIOP_SCRATCHA,
					    scratcha0 & ~A_flag_data);
					CALL_SCRIPT(Ent_msgin);
					return 1;
				}
				printf("unexpected phase mismatch %d\n",
				    sstat1 & SSTAT1_PHASE_MASK);
			} else
				printf("phase mismatch without command\n");
			need_reset = 1;
		}
		if (sist & (SIST1_STO << 8)) {
			/* selection time out, assume there's no device here */
			if (siop_xfer) {
				xs->error = XS_SELTIMEOUT;
				goto end;
			} else
				printf("selection timeout without command\n");
		}

		/* Else it's an unhandled exception (for now). */
		printf("unhandled scsi interrupt,"
		    " sist=0x%x sstat1=0x%x DSA=0x%x DSP=0x%lx\n",
		    sist, sstat1, dsa, 
		    readl(adp->addr + SIOP_DSP) - scriptaddr);
		if (siop_xfer) {
			xs->error = XS_SELTIMEOUT;
			goto end;
		}
		need_reset = 1;
	}
	if (need_reset) {
reset:
		printf("XXXXX: fatal error, need reset the bus...\n");
		return 1;
	}

//scintr:
	if ((istat & ISTAT_DIP) && (dstat & DSTAT_SIR)) { /* script interrupt */
		irqcode = readl(adp->addr + SIOP_DSPS);
		/*
		 * no command, or an inactive command is only valid for a
		 * reselect interrupt
		 */
		if ((irqcode & 0x80) == 0) {
			if (siop_xfer == NULL) {
				printf(
				    "script interrupt 0x%x with invalid DSA\n",
				    irqcode);
				goto reset;
			}
		}
		switch(irqcode) {
		case A_int_err:
			printf("error, DSP=0x%lx\n",
			    readl(adp->addr + SIOP_DSP) - scriptaddr);
			if (xs) {
				xs->error = XS_SELTIMEOUT;
				goto end;
			} else {
				goto reset;
			}
		case A_int_reseltarg:
			printf("reselect with invalid target\n");
			goto reset;
		case A_int_resellun:
			target = readb(adp->addr + SIOP_SCRATCHA) & 0xf;
			lun = readb(adp->addr + SIOP_SCRATCHA + 1);
			tag = readb(adp->addr + SIOP_SCRATCHA + 2);
			if (target != adp->xs->target ||
			    lun != adp->xs->lun ||
			    tag != 0) {
				printf("unknwon resellun:"
				    " target %d lun %d tag %d\n",
				    target, lun, tag);
				goto reset;
			}
			siop_xfer = adp->xfer;
			dsa = local_to_PCI((u_long)siop_xfer);
			writel(adp->addr + SIOP_DSP,
			    dsa + sizeof(struct siop_common_xfer) +
			    Ent_ldsa_reload_dsa);
			_wbinv((u_long)siop_xfer, sizeof(*siop_xfer));
			return 1;
		case A_int_reseltag:
			printf("reselect with invalid tag\n");
			goto reset;
		case A_int_disc:
			offset = readb(adp->addr + SIOP_SCRATCHA + 1);
			siop_sdp(adp, siop_xfer, xs, offset);
#if 0
			/* we start again with no offset */
			siop_cmd->saved_offset = SIOP_NOOFFSET;
#endif
			_wbinv((u_long)siop_xfer, sizeof(*siop_xfer));
			CALL_SCRIPT(Ent_script_sched);
			return 1;
		case A_int_resfail:
			printf("reselect failed\n");
			return  1;
		case A_int_done:
			if (xs == NULL) {
				printf("done without command, DSA=0x%lx\n",
				    local_to_PCI((u_long)adp->xfer));
				return 1;
			}
			/* update resid.  */
			offset = readb(adp->addr + SIOP_SCRATCHA + 1);
#if 0
			/*
			 * if we got a disconnect between the last data phase
			 * and the status phase, offset will be 0. In this
			 * case, siop_cmd->saved_offset will have the proper
			 * value if it got updated by the controller
			 */
			if (offset == 0 &&
			    siop_cmd->saved_offset != SIOP_NOOFFSET)
				offset = siop_cmd->saved_offset;
#endif
			siop_update_resid(adp, siop_xfer, xs, offset);
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
	xs->status = ctoh32(siop_xfer->siop_tables.status);
	if (xs->status == SCSI_OK)
		writel(adp->addr + SIOP_DSP, scriptaddr + Ent_script_sched);
	else
		restart = 1;
	siop_scsicmd_end(adp, xs);
	if (restart)
		writel(adp->addr + SIOP_DSP, scriptaddr + Ent_script_sched);

	return 1;
}

static void
siop_scsicmd_end(struct siop_adapter *adp, struct scsi_xfer *xs)
{

	switch(xs->status) {
	case SCSI_OK:
		xs->error = XS_NOERROR;
		break;
	case SCSI_BUSY:
	case SCSI_CHECK:
	case SCSI_QUEUE_FULL:
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
		printf("invalid status code %d\n", xs->status);
		xs->error = XS_DRIVER_STUFFUP;
	}
	_inv((u_long)xs->cmd, xs->cmdlen);
	if (xs->datalen != 0)
		_inv((u_long)xs->data, xs->datalen);
	xs->xs_status = XS_STS_DONE;
}

static int
siop_scsi_request(struct siop_adapter *adp, struct scsi_xfer *xs)
{
	void *xfer = adp->xfer;
	int timo, error;

	if (adp->sel_t != xs->target) {
		const int free_lo = __arraycount(siop_script);
		int i;
		void *scriptaddr = (void *)local_to_PCI((u_long)adp->script);

		if (adp->sel_t != -1)
			adp->script[Ent_resel_targ0 / 4 + adp->sel_t * 2] =
			    htoc32(0x800c00ff);

		for (i = 0; i < __arraycount(lun_switch); i++)
			adp->script[free_lo + i] = htoc32(lun_switch[i]);
		adp->script[free_lo + E_abs_lunsw_return_Used[0]] =
		    htoc32(scriptaddr + Ent_lunsw_return);

		siop_add_reselsw(adp, xs->target, free_lo);

		adp->sel_t = xs->target;
	}

restart:

	siop_setuptables(adp, xfer, xs);

	/* load the DMA maps */
	if (xs->datalen != 0)
		_inv((u_long)xs->data, xs->datalen);
	_wbinv((u_long)xs->cmd, xs->cmdlen);

	_wbinv((u_long)xfer, sizeof(struct siop_xfer));
	siop_start(adp, xs);

	adp->xs = xs;
	timo = 0;
	while (!(xs->xs_status & XS_STS_DONE)) {
		delay(1000);
		siop_intr(adp);

		if (timo++ > 3000) {		/* XXXX: 3sec */
			printf("%s: timeout\n", __func__);
			return ETIMEDOUT;
		}
	}

	if (xs->error != XS_NOERROR) {
		if (xs->error == XS_BUSY || xs->status == SCSI_CHECK)
			scsi_request_sense(adp, xs);

		switch (xs->error) {
		case XS_SENSE:
		case XS_SHORTSENSE:
			error = scsi_interpret_sense(adp, xs);
			break;
		case XS_RESOURCE_SHORTAGE:
			printf("adapter resource shortage\n");

			/* FALLTHROUGH */
		case XS_BUSY:
			error = EBUSY;
			break;
		case XS_REQUEUE:
			printf("XXXX: requeue...\n");
			error = ERESTART;
			break;
		case XS_SELTIMEOUT:
		case XS_TIMEOUT:
			error = EIO;
			break;
		case XS_RESET:
			error = EIO;
			break;
		case XS_DRIVER_STUFFUP:
			printf("generic HBA error\n");
			error = EIO;
			break;
		default:
			printf("invalid return code from adapter: %d\n",
			    xs->error);
			error = EIO;
			break;
		}
		if (error == ERESTART) {
			xs->error = XS_NOERROR;
			xs->status = SCSI_OK;
			xs->xs_status &= ~XS_STS_DONE;
			goto restart;
		}
		return error;
	}
	return 0;
}

static void
siop_start(struct siop_adapter *adp, struct scsi_xfer *xs)
{
	struct siop_xfer *siop_xfer = adp->xfer;
	uint32_t dsa, *script = adp->script;
	int slot;
	void *scriptaddr = (void *)local_to_PCI((u_long)script);
	const int siop_common_xfer_size = sizeof(struct siop_common_xfer);

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
	slot = adp->currschedslot;
	/*
	 * If the instruction is 0x80000000 (JUMP foo, IF FALSE) the slot is
	 * free. As this is the last used slot, all previous slots are free,
	 * we can restart from 0.
	 */
	if (ctoh32(script[(Ent_script_sched_slot0 / 4) + slot * 2]) ==
	    0x80000000) {
		slot = adp->currschedslot = 0;
	} else {
		slot++;
	}
	/*
	 * find a free scheduler slot and load it.
	 */
#define SIOP_NSLOTS	0x40
	for (; slot < SIOP_NSLOTS; slot++) {
		/*
		 * If cmd if 0x80000000 the slot is free
		 */
		if (ctoh32(script[(Ent_script_sched_slot0 / 4) + slot * 2]) ==
		    0x80000000)
			break;
	}
	if (slot == SIOP_NSLOTS) {
		/*
		 * no more free slot, no need to continue. freeze the queue
		 * and requeue this command.
		 */
		printf("no mode free slot\n");
		return;
	}

	/* patch scripts with DSA addr */
	dsa = local_to_PCI((u_long)siop_xfer);

	/* CMD script: MOVE MEMORY addr */
	siop_xfer->resel[E_ldsa_abs_slot_Used[0]] =
	    htoc32(scriptaddr + Ent_script_sched_slot0 + slot * 8);
	_wbinv((u_long)siop_xfer, sizeof(*siop_xfer));
	/* scheduler slot: JUMP ldsa_select */
	script[(Ent_script_sched_slot0 / 4) + slot * 2 + 1] =
	    htoc32(dsa + siop_common_xfer_size + Ent_ldsa_select);
	/*
	 * Change JUMP cmd so that this slot will be handled
	 */
	script[(Ent_script_sched_slot0 / 4) + slot * 2] = htoc32(0x80080000);
	adp->currschedslot = slot;

	/* make sure SCRIPT processor will read valid data */
	_wbinv((u_long)script, SIOP_SCRIPT_SIZE);
	/* Signal script it has some work to do */
	writeb(adp->addr + SIOP_ISTAT, ISTAT_SIGP);
	/* and wait for IRQ */
}

static void
siop_xfer_setup(struct siop_xfer *xfer, void *scriptaddr)
{
	const int off_msg_in = offsetof(struct siop_common_xfer, msg_in);
	const int off_status = offsetof(struct siop_common_xfer, status);
	uint32_t dsa, *scr;
	int i;

	memset(xfer, 0, sizeof(*xfer));
	dsa = local_to_PCI((u_long)xfer);
	xfer->siop_tables.t_msgout.count = htoc32(1);
	xfer->siop_tables.t_msgout.addr = htoc32(dsa);
	xfer->siop_tables.t_msgin.count = htoc32(1);
	xfer->siop_tables.t_msgin.addr = htoc32(dsa + off_msg_in);
	xfer->siop_tables.t_extmsgin.count = htoc32(2);
	xfer->siop_tables.t_extmsgin.addr = htoc32(dsa + off_msg_in + 1);
	xfer->siop_tables.t_extmsgdata.addr = htoc32(dsa + off_msg_in + 3);
	xfer->siop_tables.t_status.count = htoc32(1);
	xfer->siop_tables.t_status.addr = htoc32(dsa + off_status);

	/* The select/reselect script */
	scr = xfer->resel;
	for (i = 0; i < __arraycount(load_dsa); i++)
		scr[i] = htoc32(load_dsa[i]);

	/*
	 * 0x78000000 is a 'move data8 to reg'. data8 is the second
	 * octet, reg offset is the third.
	 */
	scr[Ent_rdsa0 / 4] = htoc32(0x78100000 | ((dsa & 0x000000ff) <<  8));
	scr[Ent_rdsa1 / 4] = htoc32(0x78110000 | ( dsa & 0x0000ff00       ));
	scr[Ent_rdsa2 / 4] = htoc32(0x78120000 | ((dsa & 0x00ff0000) >>  8));
	scr[Ent_rdsa3 / 4] = htoc32(0x78130000 | ((dsa & 0xff000000) >> 16));
	scr[E_ldsa_abs_reselected_Used[0]] =
	    htoc32(scriptaddr + Ent_reselected);
	scr[E_ldsa_abs_reselect_Used[0]] = htoc32(scriptaddr + Ent_reselect);
	scr[E_ldsa_abs_selected_Used[0]] = htoc32(scriptaddr + Ent_selected);
	scr[E_ldsa_abs_data_Used[0]] =
	    htoc32(dsa + sizeof(struct siop_common_xfer) + Ent_ldsa_data);
	/* JUMP foo, IF FALSE - used by MOVE MEMORY to clear the slot */
	scr[Ent_ldsa_data / 4] = htoc32(0x80000000);
}

static int
siop_add_reselsw(struct siop_adapter *adp, int target, int lunsw_off)
{
	uint32_t *script = adp->script;
	int reseloff;
	void *scriptaddr = (void *)local_to_PCI((u_long)adp->script);

	/*
	 * add an entry to resel switch
	 */
	reseloff = Ent_resel_targ0 / 4 + target * 2;
	if ((ctoh32(script[reseloff]) & 0xff) != 0xff) {
		/* it's not free */
		printf("siop: resel switch full\n");
		return EBUSY;
	}

	/* JUMP abs_foo, IF target | 0x80; */
	script[reseloff + 0] = htoc32(0x800c0080 | target);
	script[reseloff + 1] =
	    htoc32(scriptaddr + lunsw_off * 4 + Ent_lun_switch_entry);

	siop_update_scntl3(adp, target, lunsw_off);
	return 0;
}

static void
siop_update_scntl3(struct siop_adapter *adp, int target, int lunsw_off)
{
	uint32_t *script = adp->script;

	/* MOVE target->id >> 24 TO SCNTL3 */
	script[lunsw_off + (Ent_restore_scntl3 / 4)] =
	    htoc32(0x78030000 | ((adp->clock_div >> 16) & 0x0000ff00));
	/* MOVE target->id >> 8 TO SXFER */
	script[lunsw_off + (Ent_restore_scntl3 / 4) + 2] =
	    htoc32(0x78050000 | (0x000000000 & 0x0000ff00));
	_wbinv((u_long)script, SIOP_SCRIPT_SIZE);
}


/*
 * SCSI functions
 */

static int
_scsi_inquire(struct siop_adapter *adp, int t, int l, int buflen, char *buf)
{
	struct scsipi_inquiry *cmd = (struct scsipi_inquiry *)adp->cmd;
	struct scsipi_inquiry_data *inqbuf =
	    (struct scsipi_inquiry_data *)adp->data;
	struct scsi_xfer xs;
	int error;

	memset(cmd, 0, sizeof(*cmd));
	cmd->opcode = INQUIRY;
	cmd->length = SCSIPI_INQUIRY_LENGTH_SCSI2;
	memset(inqbuf, 0, sizeof(*inqbuf));

	memset(&xs, 0, sizeof(xs));
	xs.target = t;
	xs.lun = l;
	xs.cmdlen = sizeof(*cmd);
	xs.cmd = (void *)cmd;
	xs.datalen = SCSIPI_INQUIRY_LENGTH_SCSI2;
	xs.data = (void *)inqbuf;

	xs.error = XS_NOERROR;
	xs.resid = xs.datalen;
	xs.status = SCSI_OK;

	error = siop_scsi_request(adp, &xs);
	if (error != 0)
		return error;

	memcpy(buf, inqbuf, buflen);
	return 0;
}

static void
scsi_request_sense(struct siop_adapter *adp, struct scsi_xfer *xs)
{
	struct scsi_request_sense *cmd = adp->sense;
	struct scsi_sense_data *data = (struct scsi_sense_data *)adp->data;
	struct scsi_xfer sense;
	int error;

	memset(cmd, 0, sizeof(struct scsi_request_sense));
	cmd->opcode = SCSI_REQUEST_SENSE;
	cmd->length = sizeof(struct scsi_sense_data);
	memset(data, 0, sizeof(struct scsi_sense_data));

	memset(&sense, 0, sizeof(sense));
	sense.target = xs->target;
	sense.lun = xs->lun;
	sense.cmdlen = sizeof(struct scsi_request_sense);
	sense.cmd = (void *)cmd;
	sense.datalen = sizeof(struct scsi_sense_data);
	sense.data = (void *)data;

	sense.error = XS_NOERROR;
	sense.resid = sense.datalen;
	sense.status = SCSI_OK;

	error = siop_scsi_request(adp, &sense);
	switch (error) {
	case 0:
		/* we have a valid sense */
		xs->error = XS_SENSE;
		return;
	case EINTR:
		/* REQUEST_SENSE interrupted by bus reset. */
		xs->error = XS_RESET;
		return;
	case EIO:
		 /* request sense couldn't be performed */
		/*
		 * XXX this isn't quite right but we don't have anything
		 * better for now
		 */
		xs->error = XS_DRIVER_STUFFUP;
		return;
	default:
		 /* Notify that request sense failed. */
		xs->error = XS_DRIVER_STUFFUP;
		printf("request sense failed with error %d\n", error);
		return;
	}
}

/*
 * scsi_interpret_sense:
 *
 *	Look at the returned sense and act on the error, determining
 *	the unix error number to pass back.  (0 = report no error)
 *
 *	NOTE: If we return ERESTART, we are expected to haved
 *	thawed the device!
 *
 *	THIS IS THE DEFAULT ERROR HANDLER FOR SCSI DEVICES.
 */
static int
scsi_interpret_sense(struct siop_adapter *adp, struct scsi_xfer *xs)
{
	struct scsi_sense_data *sense;
	u_int8_t key;
	int error;
	uint32_t info;
	static const char *error_mes[] = {
		"soft error (corrected)",
		"not ready", "medium error",
		"non-media hardware failure", "illegal request",
		"unit attention", "readonly device",
		"no data found", "vendor unique",
		"copy aborted", "command aborted",
		"search returned equal", "volume overflow",
		"verify miscompare", "unknown error key"
	};

	sense = (struct scsi_sense_data *)xs->data;

	DPRINTF((" sense debug information:\n"));
	DPRINTF(("\tcode 0x%x valid %d\n",
		SSD_RCODE(sense->response_code),
		sense->response_code & SSD_RCODE_VALID ? 1 : 0));
	DPRINTF(("\tseg 0x%x key 0x%x ili 0x%x eom 0x%x fmark 0x%x\n",
		sense->segment,
		SSD_SENSE_KEY(sense->flags),
		sense->flags & SSD_ILI ? 1 : 0,
		sense->flags & SSD_EOM ? 1 : 0,
		sense->flags & SSD_FILEMARK ? 1 : 0));
	DPRINTF(("\ninfo: 0x%x 0x%x 0x%x 0x%x followed by %d "
		"extra bytes\n",
		sense->info[0],
		sense->info[1],
		sense->info[2],
		sense->info[3],
		sense->extra_len));

	switch (SSD_RCODE(sense->response_code)) {

		/*
		 * Old SCSI-1 and SASI devices respond with
		 * codes other than 70.
		 */
	case 0x00:		/* no error (command completed OK) */
		return 0;
	case 0x04:		/* drive not ready after it was selected */
		if (adp->sd->sc_flags & FLAGS_REMOVABLE)
			adp->sd->sc_flags &= ~FLAGS_MEDIA_LOADED;
		/* XXX - display some sort of error here? */
		return EIO;
	case 0x20:		/* invalid command */
		return EINVAL;
	case 0x25:		/* invalid LUN (Adaptec ACB-4000) */
		return EACCES;

		/*
		 * If it's code 70, use the extended stuff and
		 * interpret the key
		 */
	case 0x71:		/* delayed error */
		key = SSD_SENSE_KEY(sense->flags);
		printf(" DEFERRED ERROR, key = 0x%x\n", key);
		/* FALLTHROUGH */
	case 0x70:
		if ((sense->response_code & SSD_RCODE_VALID) != 0)
			info = _4btol(sense->info);
		else
			info = 0;
		key = SSD_SENSE_KEY(sense->flags);

		switch (key) {
		case SKEY_NO_SENSE:
		case SKEY_RECOVERED_ERROR:
			if (xs->resid == xs->datalen && xs->datalen) {
				/*
				 * Why is this here?
				 */
				xs->resid = 0;	/* not short read */
			}
		case SKEY_EQUAL:
			error = 0;
			break;
		case SKEY_NOT_READY:
			if (adp->sd->sc_flags & FLAGS_REMOVABLE)
				adp->sd->sc_flags &= ~FLAGS_MEDIA_LOADED;
			if (sense->asc == 0x3A) {
				error = ENODEV; /* Medium not present */
			} else
				error = EIO;
			break;
		case SKEY_ILLEGAL_REQUEST:
			error = EINVAL;
			break;
		case SKEY_UNIT_ATTENTION:
			if (sense->asc == 0x29 &&
			    sense->ascq == 0x00) {
				/* device or bus reset */
				return ERESTART;
			}
			if (adp->sd->sc_flags & FLAGS_REMOVABLE)
				adp->sd->sc_flags &= ~FLAGS_MEDIA_LOADED;
			if (!(adp->sd->sc_flags & FLAGS_REMOVABLE))
				return ERESTART;
			error = EIO;
			break;
		case SKEY_DATA_PROTECT:
			error = EROFS;
			break;
		case SKEY_BLANK_CHECK:
			error = 0;
			break;
		case SKEY_ABORTED_COMMAND:
			/* XXX XXX initialize 'error' */
			break;
		case SKEY_VOLUME_OVERFLOW:
			error = ENOSPC;
			break;
		default:
			error = EIO;
			break;
		}

		/* Print brief(er) sense information */
		printf("%s", error_mes[key - 1]);
		if ((sense->response_code & SSD_RCODE_VALID) != 0) {
			switch (key) {
			case SKEY_NOT_READY:
			case SKEY_ILLEGAL_REQUEST:
			case SKEY_UNIT_ATTENTION:
			case SKEY_DATA_PROTECT:
				break;
			case SKEY_BLANK_CHECK:
				printf(", requested size: %d (decimal)",
				    info);
				break;
			case SKEY_ABORTED_COMMAND:
				printf(", cmd 0x%x, info 0x%x",
				    xs->cmd->opcode, info);
				break;
			default:
				printf(", info = %d (decimal)", info);
			}
		}
		if (sense->extra_len != 0) {
			int n;
			printf(", data =");
			for (n = 0; n < sense->extra_len; n++)
				printf(" %x", sense->csi[n]);
		}
		printf("\n");
		return error;

	/*
	 * Some other code, just report it
	 */
	default:
		printf("Sense Error Code 0x%x",
			SSD_RCODE(sense->response_code));
		if ((sense->response_code & SSD_RCODE_VALID) != 0) {
			struct scsi_sense_data_unextended *usense =
			    (struct scsi_sense_data_unextended *)sense;
			printf(" at block no. %d (decimal)",
			    _3btol(usense->block));
		}
		printf("\n");
		return EIO;
	}
}

static int
scsi_probe(struct siop_adapter *adp)
{
	struct scsipi_inquiry_data *inqbuf;
	int found, t, l;
	uint8_t device;
	char buf[SCSIPI_INQUIRY_LENGTH_SCSI2],
	    product[sizeof(inqbuf->product) + 1];

	found = 0;
	for (t = 0; t < 8; t++) {
		if (t == adp->id)
			continue;
		for (l = 0; l < 8; l++) {
			if (_scsi_inquire(adp, t, l, sizeof(buf), buf) != 0)
				continue;

			inqbuf = (struct scsipi_inquiry_data *)buf;
			device = inqbuf->device & SID_TYPE;
			if (device == T_NODEVICE)
				continue;
			if (device != T_DIRECT &&
			    device != T_OPTICAL &&
			    device != T_SIMPLE_DIRECT)
				continue;

			memset(product, 0, sizeof(product));
			strncpy(product, inqbuf->product, sizeof(product) - 1);
			printf("/dev/disk/scsi/0%d%d: <%s>\n", t, l, product);
			found++;
		}
	}
	return found;
}

int
scsi_inquire(struct sd_softc *sd, int buflen, void *buf)
{
	struct siop_adapter *adp;
	int error;

	if (sd->sc_bus != 0)
		return ENOTSUP;
	if (adapt.addr == 0)
		return ENOENT;
	adp = &adapt;

	adp->sd = sd;
	error = _scsi_inquire(adp, sd->sc_target, sd->sc_lun, buflen, buf);
	adp->sd = NULL;

	return error;
}

/*
 * scsi_mode_sense
 *	get a sense page from a device
 */

int
scsi_mode_sense(struct sd_softc *sd, int byte2, int page,
		  struct scsi_mode_parameter_header_6 *data, int len)
{
	struct scsi_mode_sense_6 cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SCSI_MODE_SENSE_6;
	cmd.byte2 = byte2;
	cmd.page = page;
	cmd.length = len & 0xff;

	return scsi_command(sd, (void *)&cmd, sizeof(cmd), (void *)data, len);
}

int
scsi_command(struct sd_softc *sd, void *cmd, int cmdlen, void *data,
	     int datalen)
{
	struct siop_adapter *adp;
	struct scsi_xfer xs;
	int error;

	if (sd->sc_bus != 0)
		return ENOTSUP;
	if (adapt.addr == 0)
		return ENOENT;
	adp = &adapt;

	memcpy(adp->cmd, cmd, cmdlen);
	adp->sd = sd;

	memset(&xs, 0, sizeof(xs));
	xs.target = sd->sc_target;
	xs.lun = sd->sc_lun;
	xs.cmdlen = cmdlen;
	xs.cmd = adp->cmd;
	xs.datalen = datalen;
	xs.data = adp->data;

	xs.error = XS_NOERROR;
	xs.resid = datalen;
	xs.status = SCSI_OK;

	error = siop_scsi_request(adp, &xs);
	adp->sd = NULL;
	if (error != 0)
		return error;

	if (datalen > 0)
		memcpy(data, adp->data, datalen);
	return 0;
}

/*
 * Initialize the device.
 */
int
siop_init(int bus, int dev, int func)
{
	struct siop_adapter tmp;
	struct siop_xfer *xfer;
	struct scsipi_generic *cmd;
	struct scsi_request_sense *sense;
	uint32_t reg;
	u_long addr;
	uint32_t *script;
	int slot, id, i;
	void *scriptaddr;
	u_char *data;
	const int clock_div = 3;		/* 53c810 */

	slot = PCISlotnum(bus, dev, func);
	if (slot == -1)
		return ENOENT;

	addr = PCIAddress(slot, 1, PCI_MAPREG_TYPE_MEM);
	if (addr == 0xffffffff)
		return EINVAL;
	enablePCI(slot, 0, 1, 1);

	script = ALLOC(uint32_t, SIOP_SCRIPT_SIZE);
	if (script == NULL)
		return ENOMEM;
	scriptaddr = (void *)local_to_PCI((u_long)script);
	cmd = ALLOC(struct scsipi_generic, SIOP_SCSI_COMMAND_SIZE);
	if (cmd == NULL)
		return ENOMEM;
	sense = ALLOC(struct scsi_request_sense, SIOP_SCSI_COMMAND_SIZE);
	if (sense == NULL)
		return ENOMEM;
	data = ALLOC(u_char, SIOP_SCSI_DATA_SIZE);
	if (data == NULL)
		return ENOMEM;
	xfer = ALLOC(struct siop_xfer, sizeof(struct siop_xfer));
	if (xfer == NULL)
		return ENOMEM;
	siop_xfer_setup(xfer, scriptaddr);

	id = readb(addr + SIOP_SCID) & SCID_ENCID_MASK;

	/* reset bus */
	reg = readb(addr + SIOP_SCNTL1);
	writeb(addr + SIOP_SCNTL1, reg | SCNTL1_RST);
	delay(100);
	writeb(addr + SIOP_SCNTL1, reg);

	/* reset the chip */
	writeb(addr + SIOP_ISTAT, ISTAT_SRST);
	delay(1000);
	writeb(addr + SIOP_ISTAT, 0);

	/* init registers */
	writeb(addr + SIOP_SCNTL0, SCNTL0_ARB_MASK | SCNTL0_EPC | SCNTL0_AAP);
	writeb(addr + SIOP_SCNTL1, 0);
	writeb(addr + SIOP_SCNTL3, clock_div);
	writeb(addr + SIOP_SXFER, 0);
	writeb(addr + SIOP_DIEN, 0xff);
	writeb(addr + SIOP_SIEN0, 0xff & ~(SIEN0_CMP | SIEN0_SEL | SIEN0_RSL));
	writeb(addr + SIOP_SIEN1, 0xff & ~(SIEN1_HTH | SIEN1_GEN));
	writeb(addr + SIOP_STEST2, 0);
	writeb(addr + SIOP_STEST3, STEST3_TE);
	writeb(addr + SIOP_STIME0, (0xb << STIME0_SEL_SHIFT));
	writeb(addr + SIOP_SCID, id | SCID_RRE);
	writeb(addr + SIOP_RESPID0, 1 << id);
	writeb(addr + SIOP_DCNTL, DCNTL_COM);

	/* BeBox uses PCIC */
	writeb(addr + SIOP_STEST1, STEST1_SCLK);

	siop_pci_reset(addr);

	/* copy and patch the script */
	for (i = 0; i < __arraycount(siop_script); i++)
		script[i] = htoc32(siop_script[i]);
	for (i = 0; i < __arraycount(E_abs_msgin_Used); i++)
		script[E_abs_msgin_Used[i]] =
		    htoc32(scriptaddr + Ent_msgin_space);

	/* start script */
	_wbinv((u_long)script, SIOP_SCRIPT_SIZE);
	writel(addr + SIOP_DSP, (int)scriptaddr + Ent_reselect);

	memset(&tmp, 0, sizeof(tmp));
	tmp.id = id;
	tmp.clock_div = clock_div;
	tmp.addr = addr;
	tmp.script = script;
	tmp.xfer = xfer;
	tmp.cmd = cmd;
	tmp.sense = sense;
	tmp.data = data;
	tmp.currschedslot = 0;
	tmp.sel_t = -1;

	if (scsi_probe(&tmp) == 0)
		return ENXIO;
	adapt = tmp;
	return 0;
}
