/*	$NetBSD: icpvar.h,v 1.1 2002/04/22 21:05:21 ad Exp $	*/

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

#ifndef _IC_ICPVAR_H_
#define _IC_ICPVAR_H_

#include "locators.h"

/*
 * Miscellaneous constants.
 */
#define ICP_RETRIES		6
#define	ICP_WATCHDOG_FREQ	5
#define	ICP_BUSY_WAIT_MS	2500
#define	ICP_MAX_XFER		65536		
#define ICP_SCRATCH_SIZE	8192
#define	ICP_SCRATCH_SENSE \
    (ICP_SCRATCH_SIZE - sizeof(struct scsipi_sense_data) * ICP_NCCBS)

#define	ICP_NCCBS		ICP_MAX_CMDS
#define	ICP_NCCB_RESERVE	4

/*
 * Context structure for interrupt service.
 */
struct icp_intr_ctx {
	u_int32_t	info;
	u_int32_t	info2;
	u_int16_t	cmd_status;
	u_int16_t	service;
	u_int8_t	istatus;
};

/*
 * Command control block.
 */
struct icp_ccb {
	SIMPLEQ_ENTRY(icp_ccb) ic_chain;
	u_int		ic_service;
	u_int		ic_flags;
	u_int		ic_status;
	u_int		ic_ident;
	u_int		ic_nsgent;
	u_int		ic_cmdlen;
	u_int		ic_xfer_size;
	bus_dmamap_t	ic_xfer_map;
	struct icp_sg	*ic_sg;
	struct device	*ic_dv;
	void		*ic_context;
	void		(*ic_intr)(struct icp_ccb *);
	struct icp_cmd	ic_cmd;
};
#define	IC_XFER_IN	0x01	/* Map describes inbound xfer */
#define	IC_XFER_OUT	0x02	/* Map describes outbound xfer */
#define	IC_WAITING	0x04	/* We have waiters */
#define	IC_COMPLETE	0x08	/* Command completed */
#define	IC_ALLOCED	0x10	/* CCB allocated */

/*
 * Logical drive information.
 */
struct icp_cachedrv {
	u_int		cd_size;
	u_int		cd_type;
};

/*
 * Per-controller context.
 */
struct icp_softc {
	struct device		icp_dv;
	void			*icp_ih;
	bus_dma_tag_t		icp_dmat;
	bus_space_tag_t		icp_dpmemt;
	bus_space_handle_t	icp_dpmemh;
	bus_addr_t		icp_dpmembase;
	bus_space_tag_t		icp_iot;
	bus_space_handle_t	icp_ioh;
	bus_addr_t		icp_iobase;

	int			icp_class;
	u_int16_t		icp_ic_all_size;
	u_int8_t		icp_bus_cnt;
	u_int8_t		icp_bus_id[ICP_MAXBUS];
	struct icp_cachedrv	icp_cdr[ICP_MAX_HDRIVES];
	int			icp_ndevs;
	int			icp_openings;

	u_int32_t		icp_info;
	u_int32_t		icp_info2;
	u_int16_t		icp_status;

	bus_dmamap_t		icp_scr_dmamap;
	bus_dma_segment_t	icp_scr_seg[1];
	caddr_t			icp_scr;

	struct icp_ccb		*icp_ccbs;
	u_int			icp_nccbs;
	SIMPLEQ_HEAD(,icp_ccb)	icp_ccb_freelist;
	SIMPLEQ_HEAD(,icp_ccb)	icp_ccb_queue;
	struct callout		icp_wdog_callout;

	void		(*icp_copy_cmd)(struct icp_softc *, struct icp_ccb *);
	u_int8_t	(*icp_get_status)(struct icp_softc *);
	void		(*icp_intr)(struct icp_softc *, struct icp_intr_ctx *);
	void		(*icp_release_event)(struct icp_softc *,
					     struct icp_ccb *);
	void		(*icp_set_sema0)(struct icp_softc *);
	int		(*icp_test_busy)(struct icp_softc *);
};

#define ICP_ISA		0x01
#define ICP_EISA	0x02
#define ICP_PCI		0x03
#define ICP_PCINEW	0x04
#define ICP_MPR		0x05
#define ICP_CLASS_MASK	0x07
#define ICP_FC		0x10
#define ICP_CLASS(icp)	((icp)->icp_class & ICP_CLASS_MASK)

int	icp_init(struct icp_softc *, const char *);
int	icp_intr(void *);

/*
 * Consumer interface.
 */
struct icp_attach_args {
	int		icpa_unit;
};

#define	icpacf_unit	cf_loc[ICPCF_UNIT]

#define	ICPA_UNIT_SCSI	100

struct icp_ccb	*icp_ccb_alloc(struct icp_softc *);
void	icp_ccb_enqueue(struct icp_softc *, struct icp_ccb *);
void	icp_ccb_free(struct icp_softc *, struct icp_ccb *);
int	icp_ccb_map(struct icp_softc *, struct icp_ccb *, void *, int, int);
int	icp_ccb_poll(struct icp_softc *, struct icp_ccb *, int);
void	icp_ccb_unmap(struct icp_softc *, struct icp_ccb *);
int	icp_ccb_wait(struct icp_softc *, struct icp_ccb *, int);
int	icp_cmd(struct icp_softc *, u_int8_t, u_int16_t, u_int32_t, u_int32_t,
		u_int32_t);

#endif	/* !_IC_ICPVAR_H_ */
