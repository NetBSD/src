/*	$NetBSD: icp.c,v 1.2.2.2 2002/06/20 03:44:40 nathanw Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/*
 * Copyright (c) 1999, 2000 Niklas Hallqvist.  All rights reserved.
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
 *	This product includes software developed by Niklas Hallqvist.
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
 * from OpenBSD: gdt_common.c,v 1.12 2001/07/04 06:43:18 niklas Exp
 */

/*
 * This driver would not have written if it was not for the hardware donations
 * from both ICP-Vortex and Öko.neT.  I want to thank them for their support.
 *
 * Re-worked for NetBSD by Andrew Doran.  Test hardware kindly supplied by
 * Intel.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: icp.c,v 1.2.2.2 2002/06/20 03:44:40 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/endian.h>
#include <sys/malloc.h>
#include <sys/disk.h>

#include <uvm/uvm_extern.h>

#include <machine/bswap.h>
#include <machine/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/icpreg.h>
#include <dev/ic/icpvar.h>

int	icp_async_event(struct icp_softc *, int);
void	icp_ccb_submit(struct icp_softc *icp, struct icp_ccb *ic);
void	icp_chain(struct icp_softc *);
int	icp_print(void *, const char *);
int	icp_submatch(struct device *, struct cfdata *, void *);
void	icp_watchdog(void *);

int
icp_init(struct icp_softc *icp, const char *intrstr)
{
	struct icp_attach_args icpa;
	struct icp_binfo binfo;
	struct icp_ccb *ic;
	u_int16_t cdev_cnt;
	int i, j, state, feat, nsegs, rv, noscsi, nocache;

	noscsi = 0;
	nocache = 0;

	if (intrstr != NULL)
		printf("%s: interrupting at %s\n", icp->icp_dv.dv_xname,
		    intrstr);

	SIMPLEQ_INIT(&icp->icp_ccb_queue);
	SIMPLEQ_INIT(&icp->icp_ccb_freelist);
	callout_init(&icp->icp_wdog_callout);

	/*
	 * Allocate a scratch area.
	 */
	if (bus_dmamap_create(icp->icp_dmat, ICP_SCRATCH_SIZE, 1,
	    ICP_SCRATCH_SIZE, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
	    &icp->icp_scr_dmamap) != 0) {
		printf("%s: cannot create scratch dmamap\n",
		    icp->icp_dv.dv_xname);
		return (1);
	}
	state++;

	if (bus_dmamem_alloc(icp->icp_dmat, ICP_SCRATCH_SIZE, PAGE_SIZE, 0,
	    icp->icp_scr_seg, 1, &nsegs, BUS_DMA_NOWAIT) != 0) {
		printf("%s: cannot alloc scratch dmamem\n",
		    icp->icp_dv.dv_xname);
		goto bail_out;
	}
	state++;

	if (bus_dmamem_map(icp->icp_dmat, icp->icp_scr_seg, nsegs,
	    ICP_SCRATCH_SIZE, &icp->icp_scr, 0)) {
		printf("%s: cannot map scratch dmamem\n", icp->icp_dv.dv_xname);
		goto bail_out;
	}
	state++;

	if (bus_dmamap_load(icp->icp_dmat, icp->icp_scr_dmamap, icp->icp_scr,
	    ICP_SCRATCH_SIZE, NULL, BUS_DMA_NOWAIT)) {
		printf("%s: cannot load scratch dmamap\n", icp->icp_dv.dv_xname);
		goto bail_out;
	}
	state++;

	/*
	 * Allocate and initialize the command control blocks.
	 */
	ic = malloc(sizeof(*ic) * ICP_NCCBS, M_DEVBUF, M_NOWAIT | M_ZERO);
	if ((icp->icp_ccbs = ic) == NULL) {
		printf("%s: malloc() failed\n", icp->icp_dv.dv_xname);
		goto bail_out;
	}
	state++;

	for (i = 0; i < ICP_NCCBS; i++, ic++) {
		/*
		 * The first two command indexes have special meanings, so
		 * we can't use them.
		 */
		ic->ic_ident = i + 2;
		rv = bus_dmamap_create(icp->icp_dmat, ICP_MAX_XFER,
		    ICP_MAXSG, ICP_MAX_XFER, 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
		    &ic->ic_xfer_map);
		if (rv != 0)
			break;
		icp->icp_nccbs++;
		icp_ccb_free(icp, ic);
	}
#ifdef DIAGNOSTIC
	if (icp->icp_nccbs != ICP_NCCBS)
		printf("%s: %d/%d CCBs usable\n", icp->icp_dv.dv_xname,
		    icp->icp_nccbs, ICP_NCCBS);
#endif

	/*
	 * Initalize the controller.
	 */
	if (!icp_cmd(icp, ICP_SCREENSERVICE, ICP_INIT, 0, 0, 0)) {
		printf("%s: screen service init error %d\n",
		    icp->icp_dv.dv_xname, icp->icp_status);
		goto bail_out;
	}

	if (!icp_cmd(icp, ICP_CACHESERVICE, ICP_INIT, ICP_LINUX_OS, 0, 0)) {
		printf("%s: cache service init error %d\n",
		    icp->icp_dv.dv_xname, icp->icp_status);
		goto bail_out;
	}

	icp_cmd(icp, ICP_CACHESERVICE, ICP_UNFREEZE_IO, 0, 0, 0);

	if (!icp_cmd(icp, ICP_CACHESERVICE, ICP_MOUNT, 0xffff, 1, 0)) {
		printf("%s: cache service mount error %d\n",
		    icp->icp_dv.dv_xname, icp->icp_status);
		goto bail_out;
	}

	if (!icp_cmd(icp, ICP_CACHESERVICE, ICP_INIT, ICP_LINUX_OS, 0, 0)) {
		printf("%s: cache service post-mount init error %d\n",
		    icp->icp_dv.dv_xname, icp->icp_status);
		goto bail_out;
	}
	cdev_cnt = (u_int16_t)icp->icp_info;

	if (!icp_cmd(icp, ICP_SCSIRAWSERVICE, ICP_INIT, 0, 0, 0)) {
		printf("%s: raw service init error %d\n",
		    icp->icp_dv.dv_xname, icp->icp_status);
		goto bail_out;
	}

	/*
	 * Set/get raw service features (scatter/gather).
	 */
	feat = 0;
	if (icp_cmd(icp, ICP_SCSIRAWSERVICE, ICP_SET_FEAT, ICP_SCATTER_GATHER,
	    0, 0))
		if (icp_cmd(icp, ICP_SCSIRAWSERVICE, ICP_GET_FEAT, 0, 0, 0))
			feat = icp->icp_info;

	if ((feat & ICP_SCATTER_GATHER) == 0) {
#ifdef DIAGNOSTIC
		printf("%s: scatter/gather not supported (raw service)\n",
		    icp->icp_dv.dv_xname);
#endif
		noscsi = 1;
	}

	/*
	 * Set/get cache service features (scatter/gather).
	 */
	feat = 0;
	if (icp_cmd(icp, ICP_CACHESERVICE, ICP_SET_FEAT, 0,
	    ICP_SCATTER_GATHER, 0))
		if (icp_cmd(icp, ICP_CACHESERVICE, ICP_GET_FEAT, 0, 0, 0))
			feat = icp->icp_info;

	if ((feat & ICP_SCATTER_GATHER) == 0) {
#ifdef DIAGNOSTIC
		printf("%s: scatter/gather not supported (cache service)\n",
		    icp->icp_dv.dv_xname);
#endif
		nocache = 1;
	}

	/*
	 * Pull some information from the board and dump.
	 */
	if (!icp_cmd(icp, ICP_CACHESERVICE, ICP_IOCTL, ICP_BOARD_INFO,
	    ICP_INVALID_CHANNEL, sizeof(struct icp_binfo))) {
		printf("%s: unable to retrive board info\n",
		    icp->icp_dv.dv_xname);
		goto bail_out;
	}
	memcpy(&binfo, icp->icp_scr, sizeof(binfo));

	printf("%s: model <%s>, firmware <%s>, %d channel(s), %dMB memory\n",
	    icp->icp_dv.dv_xname, binfo.bi_type_string, binfo.bi_raid_string,
	    binfo.bi_chan_count, le32toh(binfo.bi_memsize) >> 20);

	/*
	 * Determine the number of devices, and number of openings per
	 * device.
	 */
	if (!nocache) {
		for (j = 0; j < cdev_cnt && j < ICP_MAX_HDRIVES; j++) {
			if (!icp_cmd(icp, ICP_CACHESERVICE, ICP_INFO, j, 0,
			    0))
				continue;

			icp->icp_cdr[j].cd_size = icp->icp_info;
			icp->icp_ndevs++;

			if (icp_cmd(icp, ICP_CACHESERVICE, ICP_DEVTYPE, j, 0,
			    0))
				icp->icp_cdr[j].cd_type = icp->icp_info;
		}
	}

	if (!noscsi)
		icp->icp_ndevs += binfo.bi_chan_count;

	if (icp->icp_ndevs != 0)
		icp->icp_openings =
		    (icp->icp_nccbs - ICP_NCCB_RESERVE) / icp->icp_ndevs;
#ifdef ICP_DEBUG
	printf("%s: %d openings per device\n", icp->icp_dv.dv_xname,
	    icp->icp_openings);
#endif

	/*
	 * Attach SCSI channels.
	 */
	if (!noscsi) {
		struct icp_ioc_version *iv;
		struct icp_rawioc *ri;
		struct icp_getch *gc;

		iv = (struct icp_ioc_version *)icp->icp_scr;
		iv->iv_version = htole32(ICP_IOC_NEWEST);
		iv->iv_listents = ICP_MAXBUS;
		iv->iv_firstchan = 0;
		iv->iv_lastchan = ICP_MAXBUS - 1;
		iv->iv_listoffset = htole32(sizeof(*iv));

		if (icp_cmd(icp, ICP_CACHESERVICE, ICP_IOCTL,
		    ICP_IOCHAN_RAW_DESC, ICP_INVALID_CHANNEL,
		    sizeof(*iv) + ICP_MAXBUS * sizeof(*ri))) {
			ri = (struct icp_rawioc *)(iv + 1);
			for (j = 0; j < binfo.bi_chan_count; j++, ri++)
				icp->icp_bus_id[j] = ri->ri_procid;
		} else {
			/*
			 * Fall back to the old method.
			 */
			gc = (struct icp_getch *)icp->icp_scr;

			for (i = 0; j < binfo.bi_chan_count; j++) {
				if (!icp_cmd(icp, ICP_CACHESERVICE, ICP_IOCTL,
				    ICP_SCSI_CHAN_CNT | ICP_L_CTRL_PATTERN,
				    ICP_IO_CHANNEL | ICP_INVALID_CHANNEL,
				    sizeof(*gc))) {
				    	printf("%s: unable to get chan info",
				    	    icp->icp_dv.dv_xname);
					goto bail_out;
				}
				icp->icp_bus_id[j] = gc->gc_scsiid;
			}
		}

		for (j = 0; j < binfo.bi_chan_count; j++) {
			if (icp->icp_bus_id[j] > ICP_MAXID_FC)
				icp->icp_bus_id[j] = ICP_MAXID_FC;

			icpa.icpa_unit = j + ICPA_UNIT_SCSI;
			config_found_sm(&icp->icp_dv, &icpa, icp_print,
			    icp_submatch);
		}
	}

	/*
	 * Attach cache devices.
	 */
	if (!nocache) {
		for (j = 0; j < cdev_cnt && j < ICP_MAX_HDRIVES; j++) {
			if (icp->icp_cdr[j].cd_size == 0)
				continue;
		
			icpa.icpa_unit = j;
			config_found_sm(&icp->icp_dv, &icpa, icp_print,
			    icp_submatch);
		}
	}

	/*
	 * Start the watchdog.
	 */
	icp_watchdog(icp);
	return (0);

 bail_out:
	if (state > 4)
		for (j = 0; j < i; j++)
			bus_dmamap_destroy(icp->icp_dmat,
			    icp->icp_ccbs[j].ic_xfer_map);
 	if (state > 3)
		free(icp->icp_ccbs, M_DEVBUF);
	if (state > 2)
		bus_dmamap_unload(icp->icp_dmat, icp->icp_scr_dmamap);
	if (state > 1)
		bus_dmamem_unmap(icp->icp_dmat, icp->icp_scr,
		    ICP_SCRATCH_SIZE);
	if (state > 0)
		bus_dmamem_free(icp->icp_dmat, icp->icp_scr_seg, nsegs);
	bus_dmamap_destroy(icp->icp_dmat, icp->icp_scr_dmamap);

	return (1);
}

void
icp_watchdog(void *cookie)
{
	struct icp_softc *icp;
	int s;

	icp = cookie;

	s = splbio();
	icp_intr(icp);
	if (! SIMPLEQ_EMPTY(&icp->icp_ccb_queue))
		icp_ccb_enqueue(icp, NULL);
	splx(s);

	callout_reset(&icp->icp_wdog_callout, hz * ICP_WATCHDOG_FREQ,
	    icp_watchdog, icp);
}

int
icp_print(void *aux, const char *pnp)
{
	struct icp_attach_args *icpa;
	const char *str;

	icpa = (struct icp_attach_args *)aux;

	if (pnp != NULL) {
		if (icpa->icpa_unit < ICPA_UNIT_SCSI)
			str = "block device";
		else
			str = "SCSI channel";
		printf("%s at %s", str, pnp);
	}
	printf(" unit %d", icpa->icpa_unit);

	return (UNCONF);
}

int
icp_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct icp_attach_args *icpa;

	icpa = (struct icp_attach_args *)aux;

	if (cf->icpacf_unit != ICPCF_UNIT_DEFAULT &&
	    cf->icpacf_unit != icpa->icpa_unit)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

int
icp_async_event(struct icp_softc *icp, int val)
{

	/* XXX */
	return (1);
}

int
icp_intr(void *cookie)
{
	struct icp_softc *icp;
	struct icp_intr_ctx ctx;
	struct icp_ccb *ic;

	icp = cookie;

	ctx.istatus = (*icp->icp_get_status)(icp);
	if (!ctx.istatus) {
		icp->icp_status = ICP_S_NO_STATUS;
		return (0);
	}

	(*icp->icp_intr)(icp, &ctx);

	icp->icp_status = ctx.cmd_status;
	icp->icp_info = ctx.info;
	icp->icp_info2 = ctx.info2;

	switch (ctx.istatus) {
	case ICP_ASYNCINDEX:
		icp_async_event(icp, ctx.service);
		return (1);

	case ICP_SPEZINDEX:
		printf("%s: uninitialized or unknown service (%d/%d)\n",
		    icp->icp_dv.dv_xname, ctx.info, ctx.info2);
		return (1);
	}

	if ((ctx.istatus - 2) > icp->icp_nccbs)
		panic("icp_intr: bad command index returned");

	ic = &icp->icp_ccbs[ctx.istatus - 2];
	ic->ic_status = icp->icp_status;

	if ((ic->ic_flags & IC_ALLOCED) == 0)
		panic("icp_intr: inactive CCB identified");

	switch (icp->icp_status) {
	case ICP_S_BSY:
#ifdef ICP_DEBUG
		printf("%s: ICP_S_BSY received\n", icp->icp_dv.dv_xname);
#endif
		SIMPLEQ_INSERT_HEAD(&icp->icp_ccb_queue, ic, ic_chain);
		break;

	default:
		ic->ic_flags |= IC_COMPLETE;

		if ((ic->ic_flags & IC_WAITING) != 0)
			wakeup(ic);
		else if (ic->ic_intr != NULL)
			(*ic->ic_intr)(ic);

		if (! SIMPLEQ_EMPTY(&icp->icp_ccb_queue))
			icp_ccb_enqueue(icp, NULL);

		break;
	}

	return (1);
}

int
icp_cmd(struct icp_softc *icp, u_int8_t service, u_int16_t opcode,
	u_int32_t arg1, u_int32_t arg2, u_int32_t arg3)
{
	struct icp_ioctlcmd *icmd;
	struct icp_cachecmd *cc;
	struct icp_rawcmd *rc;
	int retries, rv;
	struct icp_ccb *ic;

	retries = ICP_RETRIES;

	do {
		ic = icp_ccb_alloc(icp);
		memset(&ic->ic_cmd, 0, sizeof(ic->ic_cmd));
		ic->ic_cmd.cmd_opcode = htole16(opcode);

		switch (service) {
		case ICP_CACHESERVICE:
			if (opcode == ICP_IOCTL) {
				icmd = &ic->ic_cmd.cmd_packet.ic;
				icmd->ic_subfunc = htole16(arg1);
				icmd->ic_channel = htole32(arg2);
				icmd->ic_bufsize = htole32(arg3);
				icmd->ic_addr =
				    htole32(icp->icp_scr_seg[0].ds_addr);

				bus_dmamap_sync(icp->icp_dmat,
				    icp->icp_scr_dmamap, 0, arg3,
				    BUS_DMASYNC_PREWRITE |
				    BUS_DMASYNC_PREREAD);
			} else {
				cc = &ic->ic_cmd.cmd_packet.cc;
				cc->cc_deviceno = htole16(arg1);
				cc->cc_blockno = htole32(arg2);
			}
			break;

		case ICP_SCSIRAWSERVICE:
			rc = &ic->ic_cmd.cmd_packet.rc;
			rc->rc_direction = htole32(arg1);
			rc->rc_bus = arg2;
			rc->rc_target = arg3;
			rc->rc_lun = arg3 >> 8;
			break;
		}

		ic->ic_service = service;
		ic->ic_cmdlen = sizeof(ic->ic_cmd);
		rv = icp_ccb_poll(icp, ic, 10000);

		switch (service) {
		case ICP_CACHESERVICE:
			if (opcode == ICP_IOCTL) {
				bus_dmamap_sync(icp->icp_dmat,
				    icp->icp_scr_dmamap, 0, arg3,
				    BUS_DMASYNC_POSTWRITE |
				    BUS_DMASYNC_POSTREAD);
			}
			break;
		}

		icp_ccb_free(icp, ic);
	} while (rv != 0 && --retries > 0);

	return (icp->icp_status == ICP_S_OK);
}

struct icp_ccb *
icp_ccb_alloc(struct icp_softc *icp)
{
	struct icp_ccb *ic;
	int s;

	s = splbio();
	ic = SIMPLEQ_FIRST(&icp->icp_ccb_freelist);
	SIMPLEQ_REMOVE_HEAD(&icp->icp_ccb_freelist, ic_chain);
	splx(s);

	ic->ic_flags = IC_ALLOCED;
	return (ic);
}

void
icp_ccb_free(struct icp_softc *icp, struct icp_ccb *ic)
{
	int s;

	s = splbio();
	ic->ic_flags = 0;
	ic->ic_intr = NULL;
	SIMPLEQ_INSERT_HEAD(&icp->icp_ccb_freelist, ic, ic_chain);
	splx(s);
}

void
icp_ccb_enqueue(struct icp_softc *icp, struct icp_ccb *ic)
{
	int s;

	s = splbio();

	if (ic != NULL)
		SIMPLEQ_INSERT_TAIL(&icp->icp_ccb_queue, ic, ic_chain);

	while ((ic = SIMPLEQ_FIRST(&icp->icp_ccb_queue)) != NULL) {
		if ((*icp->icp_test_busy)(icp))
			break;
		icp_ccb_submit(icp, ic);
		SIMPLEQ_REMOVE_HEAD(&icp->icp_ccb_queue, ic_chain);
	}

	splx(s);
}

int
icp_ccb_map(struct icp_softc *icp, struct icp_ccb *ic, void *data, int size,
	    int dir)
{
	struct icp_sg *sg;
	int nsegs, i, rv;
	bus_dmamap_t xfer;

	xfer = ic->ic_xfer_map;

	rv = bus_dmamap_load(icp->icp_dmat, xfer, data, size, NULL,
	    BUS_DMA_NOWAIT | BUS_DMA_STREAMING |
	    ((dir & IC_XFER_IN) ? BUS_DMA_READ : BUS_DMA_WRITE));
	if (rv != 0)
		return (rv);

	nsegs = xfer->dm_nsegs;
	ic->ic_xfer_size = size;
	ic->ic_nsgent = nsegs;
	ic->ic_flags |= dir;
	sg = ic->ic_sg;

	if (sg != NULL) {
		for (i = 0; i < nsegs; i++, sg++) {
			sg->sg_addr = htole32(xfer->dm_segs[i].ds_addr);
			sg->sg_len = htole32(xfer->dm_segs[i].ds_len);
		}
	} else if (nsegs > 1)
		panic("icp_ccb_map: no SG list specified, but nsegs > 1\n");

	if ((dir & IC_XFER_OUT) != 0)
		i = BUS_DMASYNC_PREWRITE;
	else /* if ((dir & IC_XFER_IN) != 0) */
		i = BUS_DMASYNC_PREREAD;

	bus_dmamap_sync(icp->icp_dmat, xfer, 0, ic->ic_xfer_size, i);
	return (0);
}

void
icp_ccb_unmap(struct icp_softc *icp, struct icp_ccb *ic)
{
	int i;

	if ((ic->ic_flags & IC_XFER_OUT) != 0)
		i = BUS_DMASYNC_POSTWRITE;
	else /* if ((ic->ic_flags & IC_XFER_IN) != 0) */
		i = BUS_DMASYNC_POSTREAD;

	bus_dmamap_sync(icp->icp_dmat, ic->ic_xfer_map, 0, ic->ic_xfer_size, i);
	bus_dmamap_unload(icp->icp_dmat, ic->ic_xfer_map);
}

int
icp_ccb_poll(struct icp_softc *icp, struct icp_ccb *ic, int timo)
{
	int rv;

	for (timo = ICP_BUSY_WAIT_MS * 100; timo != 0; timo--) {
		if (!(*icp->icp_test_busy)(icp))
			break;
		DELAY(10);
	}
	if (timo == 0) {
		printf("%s: submit: busy\n", icp->icp_dv.dv_xname);
		return (EAGAIN);
	}

	icp_ccb_submit(icp, ic);

	for (timo *= 10; timo != 0; timo--) {
		DELAY(100);
		icp_intr(icp);
		if ((ic->ic_flags & IC_COMPLETE) != 0)
			break;
	}

	if (timo != 0) {
		if (ic->ic_status != ICP_S_OK) {
#ifdef ICP_DEBUG
			printf("%s: request failed; status=0x%04x\n",
			    icp->icp_dv.dv_xname, ic->ic_status);
#endif
			rv = EIO;
		} else
			rv = 0;
	} else {
		printf("%s: command timed out\n", icp->icp_dv.dv_xname);
		rv = EIO;
	}

	while ((*icp->icp_test_busy)(icp) != 0)
		DELAY(10);

	return (rv);
}

int
icp_ccb_wait(struct icp_softc *icp, struct icp_ccb *ic, int timo)
{
	int s, rv;

	ic->ic_flags |= IC_WAITING;

	s = splbio();
	icp_ccb_enqueue(icp, ic);
	if ((rv = tsleep(ic, PRIBIO, "icpwccb", mstohz(timo))) != 0) {
		splx(s);
		return (rv);
	}
	splx(s);

	if (ic->ic_status != ICP_S_OK) {
		printf("%s: command failed; status=%x\n", icp->icp_dv.dv_xname,
		    ic->ic_status);
		return (EIO);
	}

	return (0);
}

void
icp_ccb_submit(struct icp_softc *icp, struct icp_ccb *ic)
{

	ic->ic_cmdlen = (ic->ic_cmdlen + 3) & ~3;

	(*icp->icp_set_sema0)(icp);
	DELAY(10);

	ic->ic_cmd.cmd_boardnode = htole32(ICP_LOCALBOARD);
	ic->ic_cmd.cmd_cmdindex = htole32(ic->ic_ident);

	(*icp->icp_copy_cmd)(icp, ic);
	(*icp->icp_release_event)(icp, ic);
}
