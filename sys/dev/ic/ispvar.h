/*	$NetBSD: ispvar.h,v 1.5.4.1 1997/08/23 07:12:59 thorpej Exp $	*/

/*
 * Soft Definitions for for Qlogic ISP SCSI adapters.
 *
 * Copyright (c) 1997 by Matthew Jacob
 * NASA/Ames Research Center
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_ISPVAR_H
#define	_ISPVAR_H

#include <dev/ic/ispmbox.h>

/*
 * Vector for MD code to provide specific services.
 */
struct ispsoftc;
struct ispmdvec {
	u_int16_t	(*dv_rd_reg) __P((struct ispsoftc *, int));
	void		(*dv_wr_reg) __P((struct ispsoftc *, int, u_int16_t));
	int		(*dv_mbxdma) __P((struct ispsoftc *));
	int		(*dv_dmaset) __P((struct ispsoftc *,
		struct scsi_xfer *, ispreq_t *, u_int8_t *, u_int8_t));
	void		(*dv_dmaclr)
		__P((struct ispsoftc *, struct scsi_xfer *, u_int32_t));
	void		(*dv_reset0) __P((struct ispsoftc *));
	void		(*dv_reset1) __P((struct ispsoftc *));
	void		(*dv_dregs) __P((struct ispsoftc *));
	const u_int16_t *dv_ispfw;	/* ptr to f/w */
	u_int16_t 	dv_fwlen;	/* length of f/w */
	u_int16_t	dv_codeorg;	/* code ORG for f/w */
	u_int16_t	dv_fwrev;	/* f/w revision */
	/*
	 * Initial values for conf1 register
	 */
	u_int16_t	dv_conf1;
	u_int16_t	dv_clock;	/* clock frequency */
};

#define	MAX_TARGETS	16
#define	MAX_LUNS	8
#define	MAX_FC_TARG	126

#define	MAXISPREQUEST	256
#define	RQUEST_QUEUE_LEN(isp)	MAXISPREQUEST
#define	RESULT_QUEUE_LEN(isp)	(RQUEST_QUEUE_LEN(isp) >> 2)
#define	QENTRY_LEN		64

#define	ISP_QUEUE_ENTRY(q, idx)	((q) + ((idx) * QENTRY_LEN))
#define	ISP_QUEUE_SIZE(n)	((n) * QENTRY_LEN)

/*
 * SCSI (as opposed to FC-PH) Specific Host Adapter Parameters
 */

typedef struct {
        u_int16_t	isp_adapter_enabled	: 1,
        		isp_req_ack_active_neg	: 1,	
	        	isp_data_line_active_neg: 1,
			isp_cmd_dma_burst_enable: 1,
			isp_data_dma_burst_enabl: 1,
			isp_fifo_threshold	: 2,
			isp_diffmode		: 1,
			isp_initiator_id	: 4,
        		isp_async_data_setup	: 4;
        u_int16_t	isp_selection_timeout;
        u_int16_t	isp_max_queue_depth;
	u_int16_t	isp_clock;
	u_int8_t	isp_tag_aging;
       	u_int8_t	isp_bus_reset_delay;
        u_int8_t	isp_retry_count;
        u_int8_t	isp_retry_delay;
	struct {
		u_int8_t	dev_flags;	/* Device Flags - see below */
		u_int8_t	exc_throttle;
		u_int8_t	sync_period;
		u_int8_t	sync_offset	: 4,
				dev_enable	: 1;
	} isp_devparam[MAX_TARGETS];
} sdparam;	/* scsi device parameters */

/*
 * Device Flags
 */
#define	DPARM_DISC	0x80
#define	DPARM_PARITY	0x40
#define	DPARM_WIDE	0x20
#define	DPARM_SYNC	0x10
#define	DPARM_TQING	0x08
#define	DPARM_ARQ	0x04
#define	DPARM_QFRZ	0x02
#define	DPARM_RENEG	0x01
#define	DPARM_DEFAULT	(0xff & ~DPARM_QFRZ)

#define ISP_20M_SYNCPARMS	0x080c
#define ISP_10M_SYNCPARMS	0x0c19
#define ISP_08M_SYNCPARMS	0x0c25
#define ISP_05M_SYNCPARMS	0x0c32
#define ISP_04M_SYNCPARMS	0x0c41

/*
 * Fibre Channel Specifics
 */
typedef struct {
	u_int64_t		isp_wwn;	/* WWN of adapter */
	u_int8_t		isp_loopid;	/* FCAL of this adapter inst */
        u_int8_t		isp_retry_count;
        u_int8_t		isp_retry_delay;
	u_int8_t		isp_fwstate;	/* ISP F/W state */

	/*
	 * Scratch DMA mapped in area to fetch Port Database stuff, etc.
	 */
	volatile caddr_t	isp_scratch;
	u_int32_t		isp_scdma;
} fcparam;

#define	ISP2100_SCRLEN		0x100

#define	FW_CONFIG_WAIT		0x0000
#define	FW_WAIT_AL_PA		0x0001
#define	FW_WAIT_LOGIN		0x0002
#define	FW_READY		0x0003
#define	FW_LOSS_OF_SYNC		0x0004
#define	FW_ERROR		0x0005
#define	FW_REINIT		0x0006
#define	FW_NON_PART		0x0007

/*
 * Soft Structure per host adapter
 */
struct ispsoftc {
	struct device		isp_dev;
	struct ispmdvec *	isp_mdvec;
#define	isp_name	isp_dev.dv_xname
	struct scsi_link	isp_link;
	u_int8_t		isp_state;
	int8_t			isp_dblev;
	u_int16_t		isp_fwrev;

	/*
	 * Host Adapter Type and Parameters.
	 * Some parameters nominally stored in NVRAM on card.
	 */
	u_int8_t		isp_type;
	void * 			isp_param;

	/*
	 * Result and Request Queues.
	 */
	volatile u_int8_t	isp_reqidx;	/* index of next request */
	volatile u_int8_t	isp_residx;	/* index of next result */
	volatile u_int8_t	isp_sendmarker;
	volatile u_int8_t	isp_seqno;

	/*
	 * Sheer laziness, but it gets us around the problem
	 * where we don't have a clean way of remembering
	 * which scsi_xfer is bound to which ISP queue entry.
	 *
	 * There are other more clever ways to do this, but,
	 * jeez, so I blow a couple of KB per host adapter...
	 * and it *is* faster.
	 */
	volatile struct scsi_xfer *isp_xflist[MAXISPREQUEST];

	/*
	 * request/result queues
	 */
	volatile caddr_t	isp_rquest;
	volatile caddr_t	isp_result;
	u_int32_t		isp_rquest_dma;
	u_int32_t		isp_result_dma;
};

/*
 * Adapter Variants
 */
#define	ISP_HA_SCSI		0xf
#define	ISP_HA_SCSI_UNKNOWN	0x0
#define	ISP_HA_SCSI_1020	0x1
#define	ISP_HA_SCSI_1040A	0x2
#define	ISP_HA_SCSI_1040B	0x3
#define	ISP_HA_FC		0xf0
#define	ISP_HA_FC_2100		0x10

/*
 * ISP States
 */
#define	ISP_NILSTATE	0
#define	ISP_RESETSTATE	1
#define	ISP_INITSTATE	2
#define	ISP_RUNSTATE	3




/*
 * Macros to read, write ISP registers through MD code
 */

#define	ISP_READ(isp, reg)	\
	(*(isp)->isp_mdvec->dv_rd_reg)((isp), (reg))

#define	ISP_WRITE(isp, reg, val)	\
	(*(isp)->isp_mdvec->dv_wr_reg)((isp), (reg), (val))

#define	ISP_MBOXDMASETUP(isp)	\
	(*(isp)->isp_mdvec->dv_mbxdma)((isp))

#define	ISP_DMASETUP(isp, xs, req, iptrp, optr)	\
	(*(isp)->isp_mdvec->dv_dmaset)((isp), (xs), (req), (iptrp), (optr))

#define	ISP_DMAFREE(isp, xs, seqno)	\
	if ((isp)->isp_mdvec->dv_dmaclr) \
		 (*(isp)->isp_mdvec->dv_dmaclr)((isp), (xs), (seqno))

#define	ISP_RESET0(isp)	\
	if ((isp)->isp_mdvec->dv_reset0) (*(isp)->isp_mdvec->dv_reset0)((isp))
#define	ISP_RESET1(isp)	\
	if ((isp)->isp_mdvec->dv_reset1) (*(isp)->isp_mdvec->dv_reset1)((isp))
#define	ISP_DUMPREGS(isp)	\
	if ((isp)->isp_mdvec->dv_dregs) (*(isp)->isp_mdvec->dv_dregs)((isp))

#define	ISP_SETBITS(isp, reg, val)	\
 (*(isp)->isp_mdvec->dv_wr_reg)((isp), (reg), ISP_READ((isp), (reg)) | (val))

#define	ISP_CLRBITS(isp, reg, val)	\
 (*(isp)->isp_mdvec->dv_wr_reg)((isp), (reg), ISP_READ((isp), (reg)) & ~(val))

/*
 * Function Prototypes
 */
/*
 * Reset Hardware.
 *
 * Only looks at sc_dev.dv_xname, sc_iot and sc_ioh fields.
 */
void isp_reset __P((struct ispsoftc *));

/*
 * Initialize Hardware to known state
 */
void isp_init __P((struct ispsoftc *));

/*
 * Complete attachment of Hardware
 */
void isp_attach __P((struct ispsoftc *));

/*
 * Free any associated resources prior to decommissioning.
 */
void isp_uninit __P((struct ispsoftc *));

/*
 * Interrupt Service Routine
 */
int isp_intr __P((void *));




#endif	/* _ISPVAR_H */
