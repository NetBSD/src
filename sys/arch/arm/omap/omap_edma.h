/* $NetBSD: omap_edma.h,v 1.1.20.2 2017/12/03 11:35:55 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _OMAP_EDMA_H
#define _OMAP_EDMA_H

#define EDMA_PID_REG			0x0000
#define EDMA_CCCFG_REG			0x0004
#define EDMA_CCCFG_MP_EXIST		__BIT(25)
#define EDMA_CCCFG_CHMAP_EXIST		__BIT(24)
#define EDMA_CCCFG_NUM_REGN		__BITS(21,20)
#define EDMA_CCCFG_NUM_EVQUEUE		__BITS(18,16)
#define EDMA_CCCFG_NUM_PAENTRY		__BITS(14,12)
#define EDMA_CCCFG_NUM_INTCH		__BITS(10,8)
#define EDMA_CCCFG_NUM_QDMACH		__BITS(6,4)
#define EDMA_CCCFG_NUM_DMACH		__BITS(2,0)
#define EDMA_SYSCONFIG_REG		0x0010
#define EDMA_DCHMAP_REG(n)		(0x0100 + 4 * (n))
#define EDMA_DCHMAP_PAENTRY		__BITS(13,5)
#define EDMA_QCHMAP_REG(n)		(0x0200 + 4 * (n))
#define EDMA_DMAQNUM_REG(n)		(0x0240 + 4 * (n))
#define EDMA_QDMAQNUM_REG		0x0260
#define EDMA_QUEPRI_REG			0x0284
#define EDMA_EMR_REG			0x0300
#define EDMA_EMRH_REG			0x0304
#define EDMA_EMCR_REG			0x0308
#define EDMA_EMCRH_REG			0x030c
#define EDMA_QEMR_REG			0x0310
#define EDMA_QEMRC_REG			0x0314
#define EDMA_CCERR_REG			0x0318
#define EDMA_CCERRCLR_REG		0x031c
#define EDMA_EEVAL_REG			0x0320
#define EDMA_DRAE_REG(n)		(0x0340 + 8 * (n))
#define EDMA_DRAEH_REG(n)		(0x0340 + 8 * (n) + 4)
#define EDMA_QRAE_REG(n)		(0x0380 + 4 * (n))
#define EDMA_QE_REG(q, e)		(0x0400 + 0x40 * (q) + 4 * (e))
#define EDMA_QSTAT_REG(n)		(0x0600 + 4 * (n))
#define EDMA_QWMTHRA_REG		0x0620
#define EDMA_CCSTAT_REG			0x0640
#define EDMA_MPFAR_REG			0x0800
#define EDMA_MPFSR_REG			0x0804
#define EDMA_MPFCR_REG			0x0808
#define EDMA_MPPAG_REG			0x080c
#define EDMA_MPPA_REG(n)		(0x0810 + 4 * (n))
#define EDMA_ER_REG			0x1000
#define EDMA_ERH_REG			0x1004
#define EDMA_ECR_REG			0x1008
#define EDMA_ECRH_REG			0x100c
#define EDMA_ESR_REG			0x1010
#define EDMA_ESRH_REG			0x1014
#define EDMA_CER_REG			0x1018
#define EDMA_CERH_REG			0x101c
#define EDMA_EER_REG			0x1020
#define EDMA_EERH_REG			0x1024
#define EDMA_EECR_REG			0x1028
#define EDMA_EECRH_REG			0x102c
#define EDMA_EESR_REG			0x1030
#define EDMA_EESRH_REG			0x1034
#define EDMA_SER_REG			0x1038
#define EDMA_SERH_REG			0x103c
#define EDMA_SECR_REG			0x1040
#define EDMA_SECRH_REG			0x1044
#define EDMA_IER_REG			0x1050
#define EDMA_IERH_REG			0x1054
#define EDMA_IECR_REG			0x1058
#define EDMA_IECRH_REG			0x105c
#define EDMA_IESR_REG			0x1060
#define EDMA_IESRH_REG			0x1064
#define EDMA_IPR_REG			0x1068
#define EDMA_IPRH_REG			0x106c
#define EDMA_ICR_REG			0x1070
#define EDMA_ICRH_REG			0x1074
#define EDMA_IEVAL_REG			0x1078
#define EDMA_IEVAL_EVAL			__BIT(0)
#define EDMA_QER_REG			0x1080
#define EDMA_QEER_REG			0x1084
#define EDMA_QEECR_REG			0x1088
#define EDMA_QEESR_REG			0x108c
#define EDMA_QSER_REG			0x1090
#define EDMA_QSECR_REG			0x1094

#define EDMA_PARAM_BASE(n)		(0x4000 + 0x20 * (n))
#define EDMA_PARAM_OPT_REG(n)		(EDMA_PARAM_BASE(n) + 0x00)
#define EDMA_PARAM_OPT_PRIV		__BIT(31)
#define EDMA_PARAM_OPT_PRIVID		__BITS(27,24)
#define EDMA_PARAM_OPT_ITCCHEN		__BIT(23)
#define EDMA_PARAM_OPT_TCCHEN		__BIT(22)
#define EDMA_PARAM_OPT_ITCINTEN		__BIT(21)
#define EDMA_PARAM_OPT_TCINTEN		__BIT(20)
#define EDMA_PARAM_OPT_TCC		__BITS(17,12)
#define EDMA_PARAM_OPT_TCCMODE		__BIT(11)
#define EDMA_PARAM_OPT_FWID		__BITS(10,8)
#define EDMA_PARAM_OPT_STATIC		__BIT(3)
#define EDMA_PARAM_OPT_SYNCDIM		__BIT(2)
#define EDMA_PARAM_OPT_DAM		__BIT(1)
#define EDMA_PARAM_OPT_SAM		__BIT(0)
#define EDMA_PARAM_SRC_REG(n)		(EDMA_PARAM_BASE(n) + 0x04)
#define EDMA_PARAM_CNT_REG(n)		(EDMA_PARAM_BASE(n) + 0x08)
#define EDMA_PARAM_CNT_BCNT		__BITS(31,16)
#define EDMA_PARAM_CNT_ACNT		__BITS(15,0)
#define EDMA_PARAM_DST_REG(n)		(EDMA_PARAM_BASE(n) + 0x0c)
#define EDMA_PARAM_BIDX_REG(n)		(EDMA_PARAM_BASE(n) + 0x10)
#define EDMA_PARAM_BIDX_DSTBIDX		__BITS(31,16)
#define EDMA_PARAM_BIDX_SRCBIDX		__BITS(15,0)
#define EDMA_PARAM_LNK_REG(n)		(EDMA_PARAM_BASE(n) + 0x14)
#define EDMA_PARAM_LNK_BCNTRLD		__BITS(31,16)
#define EDMA_PARAM_LNK_LINK		__BITS(15,0)
#define EDMA_PARAM_CIDX_REG(n)		(EDMA_PARAM_BASE(n) + 0x18)
#define EDMA_PARAM_CIDX_DSTCIDX		__BITS(31,16)
#define EDMA_PARAM_CIDX_SRCCIDX		__BITS(15,0)
#define EDMA_PARAM_CCNT_REG(n)		(EDMA_PARAM_BASE(n) + 0x1c)
#define EDMA_PARAM_CCNT_CCNT		__BITS(15,0)

enum edma_type {
	EDMA_TYPE_DMA,
	EDMA_TYPE_QDMA
};

struct edma_param {
	uint32_t	ep_opt;
	uint32_t	ep_src;
	uint32_t	ep_dst;
	uint16_t	ep_bcnt;
	uint16_t	ep_acnt;
	uint16_t	ep_dstbidx;
	uint16_t	ep_srcbidx;
	uint16_t	ep_bcntrld;
	uint16_t	ep_link;
	uint16_t	ep_dstcidx;
	uint16_t	ep_srccidx;
	uint16_t	ep_ccnt;
};

struct edma_channel;

struct edma_channel *edma_channel_alloc(enum edma_type, unsigned int,
					void (*)(void *), void *);
void edma_channel_free(struct edma_channel *);
uint16_t edma_param_alloc(struct edma_channel *);
void edma_param_free(struct edma_channel *, uint16_t);
void edma_set_param(struct edma_channel *, uint16_t, struct edma_param *);
int edma_transfer_enable(struct edma_channel *, uint16_t);
int edma_transfer_start(struct edma_channel *);
void edma_halt(struct edma_channel *);
uint8_t edma_channel_index(struct edma_channel *);
void edma_dump(struct edma_channel *);
void edma_dump_param(struct edma_channel *, uint16_t);

#endif /* !_OMAP_EDMA_H */
