/*	$NetBSD: atareg.h,v 1.35 2010/01/10 16:04:25 tsutsui Exp $	*/

/*
 * Copyright (c) 1998, 2001 Manuel Bouyer.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)wdreg.h	7.1 (Berkeley) 5/9/91
 */

#ifndef _DEV_ATA_ATAREG_H_
#define	_DEV_ATA_ATAREG_H_

/*
 * ATA Task File register definitions.
 */

/* Status bits. */
#define	WDCS_BSY		0x80    /* busy */
#define	WDCS_DRDY		0x40    /* drive ready */
#define	WDCS_DWF		0x20    /* drive write fault */
#define	WDCS_DSC		0x10    /* drive seek complete */
#define	WDCS_DRQ		0x08    /* data request */
#define	WDCS_CORR		0x04    /* corrected data */
#define	WDCS_IDX		0x02    /* index */
#define	WDCS_ERR		0x01    /* error */
#define	WDCS_BITS \
    "\020\010bsy\007drdy\006dwf\005dsc\004drq\003corr\002idx\001err"

/* Error bits. */
#define	WDCE_BBK		0x80	/* bad block detected */
#define	WDCE_CRC		0x80	/* CRC error (Ultra-DMA only) */
#define	WDCE_UNC		0x40	/* uncorrectable data error */
#define	WDCE_MC			0x20	/* media changed */
#define	WDCE_IDNF		0x10	/* id not found */
#define	WDCE_MCR		0x08	/* media change requested */
#define	WDCE_ABRT		0x04	/* aborted command */
#define	WDCE_TK0NF		0x02	/* track 0 not found */
#define	WDCE_AMNF		0x01	/* address mark not found */

/* Commands for Disk Controller. */
#define	WDCC_NOP		0x00	/* Always fail with "aborted command" */
#define	WDCC_RECAL		0x10	/* disk restore code -- resets cntlr */

#define	WDCC_READ		0x20	/* disk read code */
#define	WDCC_WRITE		0x30	/* disk write code */
#define	 WDCC__LONG		 0x02	/* modifier -- access ecc bytes */
#define	 WDCC__NORETRY		 0x01	/* modifier -- no retrys */

#define	WDCC_FORMAT		0x50	/* disk format code */
#define	WDCC_DIAGNOSE		0x90	/* controller diagnostic */
#define	WDCC_IDP		0x91	/* initialize drive parameters */

#define	WDCC_SMART		0xb0	/* Self Mon, Analysis, Reporting Tech */

#define	WDCC_READMULTI		0xc4	/* read multiple */
#define	WDCC_WRITEMULTI		0xc5	/* write multiple */
#define	WDCC_SETMULTI		0xc6	/* set multiple mode */

#define	WDCC_READDMA		0xc8	/* read with DMA */
#define	WDCC_WRITEDMA		0xca	/* write with DMA */

#define	WDCC_ACKMC		0xdb	/* acknowledge media change */
#define	WDCC_LOCK		0xde	/* lock drawer */
#define	WDCC_UNLOCK		0xdf	/* unlock drawer */

#define	WDCC_FLUSHCACHE		0xe7	/* Flush cache */
#define	WDCC_FLUSHCACHE_EXT	0xea	/* Flush cache ext */
#define	WDCC_IDENTIFY		0xec	/* read parameters from controller */
#define	SET_FEATURES		0xef	/* set features */

#define	WDCC_IDLE		0xe3	/* set idle timer & enter idle mode */
#define	WDCC_IDLE_IMMED		0xe1	/* enter idle mode */
#define	WDCC_SLEEP		0xe6	/* enter sleep mode */
#define	WDCC_STANDBY		0xe2	/* set standby timer & enter standby */
#define	WDCC_STANDBY_IMMED	0xe0	/* enter standby mode */
#define	WDCC_CHECK_PWR		0xe5	/* check power mode */

#define WDCC_SECURITY_FREEZE	0xf5	/* freeze locking state */

/* Big Drive support */
#define	WDCC_READ_EXT		0x24	/* read 48-bit addressing */
#define	WDCC_WRITE_EXT		0x34	/* write 48-bit addressing */

#define	WDCC_READMULTI_EXT	0x29	/* read multiple 48-bit addressing */
#define	WDCC_WRITEMULTI_EXT	0x39	/* write multiple 48-bit addressing */

#define	WDCC_READDMA_EXT	0x25	/* read 48-bit addressing with DMA */
#define	WDCC_WRITEDMA_EXT	0x35	/* write 48-bit addressing with DMA */

#if defined(_KERNEL) || defined(_STANDALONE)
#include <dev/ata/ataconf.h>

/* Convert a 32-bit command to a 48-bit command. */
static __inline int
atacmd_to48(int cmd32)
{
	switch (cmd32) {
	case WDCC_READ:
		return WDCC_READ_EXT;
	case WDCC_WRITE:
		return WDCC_WRITE_EXT;
	case WDCC_READMULTI:
		return WDCC_READMULTI_EXT;
	case WDCC_WRITEMULTI:
		return WDCC_WRITEMULTI_EXT;
#if NATA_DMA
	case WDCC_READDMA:
		return WDCC_READDMA_EXT;
	case WDCC_WRITEDMA:
		return WDCC_WRITEDMA_EXT;
#endif
	default:
		panic("atacmd_to48: illegal 32-bit command: %d", cmd32);
		/* NOTREACHED */
	}
}
#endif /* _KERNEL || _STANDALONE */

/* Native SATA command queueing */
#define	WDCC_READ_FPDMA_QUEUED	0x60	/* SATA native queued read (48bit) */
#define	WDCC_WRITE_FPDMA_QUEUED	0x61	/* SATA native queued write (48bit) */

#ifdef _KERNEL
/* Convert a 32-bit command to a Native SATA Queued command. */
static __inline int
atacmd_tostatq(int cmd32)
{
	switch (cmd32) {
	case WDCC_READDMA:
		return WDCC_READ_FPDMA_QUEUED;
	case WDCC_WRITEDMA:
		return WDCC_WRITE_FPDMA_QUEUED;
	default:
		panic("atacmd_tosataq: illegal 32-bit command: %d", cmd32);
		/* NOTREACHED */
	}
}
#endif /* _KERNEL */

/* Subcommands for SET_FEATURES (features register) */
#define	WDSF_8BIT_PIO_EN	0x01
#define	WDSF_WRITE_CACHE_EN	0x02
#define	WDSF_SET_MODE		0x03
#define	WDSF_REASSIGN_EN	0x04
#define	WDSF_APM_EN		0x05
#define	WDSF_RETRY_DS		0x33
#define	WDSF_SET_CACHE_SGMT	0x54
#define	WDSF_READAHEAD_DS	0x55
#define	WDSF_POD_DS		0x66
#define	WDSF_ECC_DS		0x77
#define	WDSF_WRITE_CACHE_DS	0x82
#define	WDSF_REASSIGN_DS	0x84
#define	WDSF_APM_DS		0x85
#define	WDSF_ECC_EN		0x88
#define	WDSF_RETRY_EN		0x99
#define	WDSF_SET_CURRENT	0x9a
#define	WDSF_READAHEAD_EN	0xaa
#define	WDSF_PREFETCH_SET	0xab
#define	WDSF_POD_EN		0xcc

/* Subcommands for SMART (features register) */
#define	WDSM_RD_DATA		0xd0
#define	WDSM_RD_THRESHOLDS	0xd1
#define	WDSM_ATTR_AUTOSAVE_EN	0xd2
#define	WDSM_SAVE_ATTR		0xd3
#define	WDSM_EXEC_OFFL_IMM	0xd4
#define	WDSM_RD_LOG		0xd5
#define	WDSM_ENABLE_OPS		0xd8
#define	WDSM_DISABLE_OPS	0xd9
#define	WDSM_STATUS		0xda

#define WDSMART_CYL		0xc24f

/* parameters uploaded to device/heads register */
#define	WDSD_IBM		0xa0	/* forced to 512 byte sector, ecc */
#define	WDSD_CHS		0x00	/* cylinder/head/sector addressing */
#define	WDSD_LBA		0x40	/* logical block addressing */

/* Commands for ATAPI devices */
#define	ATAPI_CHECK_POWER_MODE	0xe5
#define	ATAPI_EXEC_DRIVE_DIAGS	0x90
#define	ATAPI_IDLE_IMMEDIATE	0xe1
#define	ATAPI_NOP		0x00
#define	ATAPI_PKT_CMD		0xa0
#define	ATAPI_IDENTIFY_DEVICE	0xa1
#define	ATAPI_SOFT_RESET	0x08
#define	ATAPI_SLEEP		0xe6
#define	ATAPI_STANDBY_IMMEDIATE	0xe0

/* Bytes used by ATAPI_PACKET_COMMAND (feature register) */
#define	ATAPI_PKT_CMD_FTRE_DMA	0x01
#define	ATAPI_PKT_CMD_FTRE_OVL	0x02

/* ireason */
#define	WDCI_CMD		0x01	/* command(1) or data(0) */
#define	WDCI_IN			0x02	/* transfer to(1) or from(0) the host */
#define	WDCI_RELEASE		0x04	/* bus released until completion */

#define	PHASE_CMDOUT		(WDCS_DRQ | WDCI_CMD)
#define	PHASE_DATAIN		(WDCS_DRQ | WDCI_IN)
#define	PHASE_DATAOUT		(WDCS_DRQ)
#define	PHASE_COMPLETED		(WDCI_IN | WDCI_CMD)
#define	PHASE_ABORTED		(0)

/*
 * Drive parameter structure for ATA/ATAPI.
 * Bit fields: WDC_* : common to ATA/ATAPI
 *             ATA_* : ATA only
 *             ATAPI_* : ATAPI only.
 */
struct ataparams {
    /* drive info */
    u_int16_t	atap_config;		/* 0: general configuration */
#define WDC_CFG_ATAPI_MASK    	0xc000
#define WDC_CFG_ATAPI    	0x8000
#define	ATA_CFG_REMOVABLE	0x0080
#define	ATA_CFG_FIXED		0x0040
#define ATAPI_CFG_TYPE_MASK	0x1f00
#define ATAPI_CFG_TYPE(x) (((x) & ATAPI_CFG_TYPE_MASK) >> 8)
#define	ATAPI_CFG_REMOV		0x0080
#define ATAPI_CFG_DRQ_MASK	0x0060
#define ATAPI_CFG_STD_DRQ	0x0000
#define ATAPI_CFG_IRQ_DRQ	0x0020
#define ATAPI_CFG_ACCEL_DRQ	0x0040
#define ATAPI_CFG_CMD_MASK	0x0003
#define ATAPI_CFG_CMD_12	0x0000
#define ATAPI_CFG_CMD_16	0x0001
/* words 1-9 are ATA only */
    u_int16_t	atap_cylinders;		/* 1: # of non-removable cylinders */
    u_int16_t	__reserved1;
    u_int16_t	atap_heads;		/* 3: # of heads */
    u_int16_t	__retired1[2];		/* 4-5: # of unform. bytes/track */
    u_int16_t	atap_sectors;		/* 6: # of sectors */
    u_int16_t	__retired2[3];

    u_int8_t	atap_serial[20];	/* 10-19: serial number */
    u_int16_t	__retired3[2];
    u_int16_t	__obsolete1;
    u_int8_t	atap_revision[8];	/* 23-26: firmware revision */
    u_int8_t	atap_model[40];		/* 27-46: model number */
    u_int16_t	atap_multi;		/* 47: maximum sectors per irq (ATA) */
    u_int16_t	__reserved2;
    u_int16_t	atap_capabilities1;	/* 49: capability flags */
#define WDC_CAP_IORDY	0x0800
#define WDC_CAP_IORDY_DSBL 0x0400
#define	WDC_CAP_LBA	0x0200
#define	WDC_CAP_DMA	0x0100
#define ATA_CAP_STBY	0x2000
#define ATAPI_CAP_INTERL_DMA	0x8000
#define ATAPI_CAP_CMD_QUEUE	0x4000
#define	ATAPI_CAP_OVERLP	0X2000
#define ATAPI_CAP_ATA_RST	0x1000
    u_int16_t	atap_capabilities2;	/* 50: capability flags (ATA) */
#if BYTE_ORDER == LITTLE_ENDIAN
    u_int8_t	__junk2;
    u_int8_t	atap_oldpiotiming;	/* 51: old PIO timing mode */
    u_int8_t	__junk3;
    u_int8_t	atap_olddmatiming;	/* 52: old DMA timing mode (ATA) */
#else
    u_int8_t	atap_oldpiotiming;	/* 51: old PIO timing mode */
    u_int8_t	__junk2;
    u_int8_t	atap_olddmatiming;	/* 52: old DMA timing mode (ATA) */
    u_int8_t	__junk3;
#endif
    u_int16_t	atap_extensions;	/* 53: extensions supported */
#define WDC_EXT_UDMA_MODES	0x0004
#define WDC_EXT_MODES		0x0002
#define WDC_EXT_GEOM		0x0001
/* words 54-62 are ATA only */
    u_int16_t	atap_curcylinders;	/* 54: current logical cylinders */
    u_int16_t	atap_curheads;		/* 55: current logical heads */
    u_int16_t	atap_cursectors;	/* 56: current logical sectors/tracks */
    u_int16_t	atap_curcapacity[2];	/* 57-58: current capacity */
    u_int16_t	atap_curmulti;		/* 59: current multi-sector setting */
#define WDC_MULTI_VALID 0x0100
#define WDC_MULTI_MASK  0x00ff
    u_int16_t	atap_capacity[2];  	/* 60-61: total capacity (LBA only) */
    u_int16_t	__retired4;
#if BYTE_ORDER == LITTLE_ENDIAN
    u_int8_t	atap_dmamode_supp; 	/* 63: multiword DMA mode supported */
    u_int8_t	atap_dmamode_act; 	/*     multiword DMA mode active */
    u_int8_t	atap_piomode_supp;       /* 64: PIO mode supported */
    u_int8_t	__junk4;
#else
    u_int8_t	atap_dmamode_act; 	/*     multiword DMA mode active */
    u_int8_t	atap_dmamode_supp; 	/* 63: multiword DMA mode supported */
    u_int8_t	__junk4;
    u_int8_t	atap_piomode_supp;       /* 64: PIO mode supported */
#endif
    u_int16_t	atap_dmatiming_mimi;	/* 65: minimum DMA cycle time */
    u_int16_t	atap_dmatiming_recom;	/* 66: recommended DMA cycle time */
    u_int16_t	atap_piotiming;    	/* 67: mini PIO cycle time without FC */
    u_int16_t	atap_piotiming_iordy;	/* 68: mini PIO cycle time with IORDY FC */
    u_int16_t	__reserved3[2];
/* words 71-72 are ATAPI only */
    u_int16_t	atap_pkt_br;		/* 71: time (ns) to bus release */
    u_int16_t	atap_pkt_bsyclr;	/* 72: tme to clear BSY after service */
    u_int16_t	__reserved4[2];
    u_int16_t	atap_queuedepth;   	/* 75: */
#define WDC_QUEUE_DEPTH_MASK 0x1F
    u_int16_t   atap_sata_caps;/* 76: */
#define SATA_SIGNAL_GEN1	0x02
#define SATA_SIGNAL_GEN2	0x04
#define SATA_NATIVE_CMDQ	0x0100
#define SATA_HOST_PWR_MGMT	0x0200
#define SATA_PHY_EVNT_CNT	0x0400
    u_int16_t   atap_sata_reserved;    /* 77: */
    u_int16_t   atap_sata_features_supp;    /* 78: */
#define SATA_NONZERO_OFFSETS	0x02
#define SATA_DMA_SETUP_AUTO	0x04
#define SATA_DRIVE_PWR_MGMT	0x08
#define SATA_IN_ORDER_DATA	0x10
#define SATA_SW_STTNGS_PRS	0x40
    u_int16_t   atap_sata_features_en;    /* 79: */
    u_int16_t	atap_ata_major;  	/* 80: Major version number */
#define	WDC_VER_ATA1	0x0002
#define	WDC_VER_ATA2	0x0004
#define	WDC_VER_ATA3	0x0008
#define	WDC_VER_ATA4	0x0010
#define	WDC_VER_ATA5	0x0020
#define	WDC_VER_ATA6	0x0040
#define	WDC_VER_ATA7	0x0080
    u_int16_t   atap_ata_minor;  	/* 81: Minor version number */
    u_int16_t	atap_cmd_set1;    	/* 82: command set supported */
#define	WDC_CMD1_NOP	0x4000		/*	NOP */
#define	WDC_CMD1_RB	0x2000		/*	READ BUFFER */
#define	WDC_CMD1_WB	0x1000		/*	WRITE BUFFER */
/*			0x0800			Obsolete */
#define	WDC_CMD1_HPA	0x0400		/*	Host Protected Area */
#define	WDC_CMD1_DVRST	0x0200		/*	DEVICE RESET */
#define	WDC_CMD1_SRV	0x0100		/*	SERVICE */
#define	WDC_CMD1_RLSE	0x0080		/*	release interrupt */
#define	WDC_CMD1_AHEAD	0x0040		/*	look-ahead */
#define	WDC_CMD1_CACHE	0x0020		/*	write cache */
#define	WDC_CMD1_PKT	0x0010		/*	PACKET */
#define	WDC_CMD1_PM	0x0008		/*	Power Management */
#define	WDC_CMD1_REMOV	0x0004		/*	Removable Media */
#define	WDC_CMD1_SEC	0x0002		/*	Security Mode */
#define	WDC_CMD1_SMART	0x0001		/*	SMART */
    u_int16_t	atap_cmd_set2;    	/* 83: command set supported */
#define	ATA_CMD2_FCE	0x2000		/*	FLUSH CACHE EXT */
#define	WDC_CMD2_FC	0x1000		/*	FLUSH CACHE */
#define	WDC_CMD2_DCO	0x0800		/*	Device Configuration Overlay */
#define	ATA_CMD2_LBA48	0x0400		/*	48-bit Address */
#define	WDC_CMD2_AAM	0x0200		/*	Automatic Acoustic Management */
#define	WDC_CMD2_SM	0x0100		/*	SET MAX security extension */
#define	WDC_CMD2_SFREQ	0x0040		/*	SET FEATURE is required
						to spin-up after power-up */
#define	WDC_CMD2_PUIS	0x0020		/*	Power-Up In Standby */
#define	WDC_CMD2_RMSN	0x0010		/*	Removable Media Status Notify */
#define	ATA_CMD2_APM	0x0008		/*	Advanced Power Management */
#define	ATA_CMD2_CFA	0x0004		/*	CFA */
#define	ATA_CMD2_RWQ	0x0002		/*	READ/WRITE DMA QUEUED */
#define	WDC_CMD2_DM	0x0001		/*	DOWNLOAD MICROCODE */
    u_int16_t	atap_cmd_ext;		/* 84: command/features supp. ext. */
#define	ATA_CMDE_TLCONT	0x1000		/*	Time-limited R/W Continuous */
#define	ATA_CMDE_TL	0x0800		/*	Time-limited R/W */
#define	ATA_CMDE_URGW	0x0400		/*	URG for WRITE STREAM DMA/PIO */
#define	ATA_CMDE_URGR	0x0200		/*	URG for READ STREAM DMA/PIO */
#define	ATA_CMDE_WWN	0x0100		/*	World Wide name */
#define	ATA_CMDE_WQFE	0x0080		/*	WRITE DMA QUEUED FUA EXT */
#define	ATA_CMDE_WFE	0x0040		/*	WRITE DMA/MULTIPLE FUA EXT */
#define	ATA_CMDE_GPL	0x0020		/*	General Purpose Logging */
#define	ATA_CMDE_STREAM	0x0010		/*	Streaming */
#define	ATA_CMDE_MCPTC	0x0008		/*	Media Card Pass Through Cmd */
#define	ATA_CMDE_MS	0x0004		/*	Media serial number */
#define	ATA_CMDE_SST	0x0002		/*	SMART self-test */
#define	ATA_CMDE_SEL	0x0001		/*	SMART error logging */
    u_int16_t	atap_cmd1_en;		/* 85: cmd/features enabled */
/* bits are the same as atap_cmd_set1 */
    u_int16_t	atap_cmd2_en;		/* 86: cmd/features enabled */
/* bits are the same as atap_cmd_set2 */
    u_int16_t	atap_cmd_def;		/* 87: cmd/features default */
#if BYTE_ORDER == LITTLE_ENDIAN
    u_int8_t	atap_udmamode_supp; 	/* 88: Ultra-DMA mode supported */
    u_int8_t	atap_udmamode_act; 	/*     Ultra-DMA mode active */
#else
    u_int8_t	atap_udmamode_act; 	/*     Ultra-DMA mode active */
    u_int8_t	atap_udmamode_supp; 	/* 88: Ultra-DMA mode supported */
#endif
/* 89-92 are ATA-only */
    u_int16_t	atap_seu_time;		/* 89: Sec. Erase Unit compl. time */
    u_int16_t	atap_eseu_time;		/* 90: Enhanced SEU compl. time */
    u_int16_t	atap_apm_val;		/* 91: current APM value */
    u_int16_t	__reserved5[8];		/* 92-99: reserved */
    u_int16_t	atap_max_lba[4];	/* 100-103: Max. user LBA addr */
    u_int16_t	__reserved6[4];		/* 104-107: reserved */
    u_int16_t 	atap_wwn[4];		/* 108-111: World Wide Name */
    u_int16_t	__reserved7[15];	/* 112-126: reserved */
    u_int16_t	atap_rmsn_supp;		/* 127: remov. media status notif. */
#define WDC_RMSN_SUPP_MASK 0x0003
#define WDC_RMSN_SUPP 0x0001
    u_int16_t	atap_sec_st;		/* 128: security status */
#define WDC_SEC_LEV_MAX	0x0100
#define WDC_SEC_ESE_SUPP 0x0020
#define WDC_SEC_EXP	0x0010
#define WDC_SEC_FROZEN	0x0008
#define WDC_SEC_LOCKED	0x0004
#define WDC_SEC_EN	0x0002
#define WDC_SEC_SUPP	0x0001
};

/*
 * If WDSM_ATTR_ADVISORY, device exceeded intended design life period.
 * If not WDSM_ATTR_ADVISORY, imminent data loss predicted.
 */
#define WDSM_ATTR_ADVISORY	1

/*
 * If WDSM_ATTR_COLLECTIVE, attribute only updated in off-line testing.
 * If not WDSM_ATTR_COLLECTIVE, attribute updated also in on-line testing.
 */
#define WDSM_ATTR_COLLECTIVE	2

/*
 * ATA SMART attributes
 */

struct ata_smart_attr {
	u_int8_t		id;		/* attribute id number */
	u_int16_t		flags;
	u_int8_t		value;		/* attribute value */
	u_int8_t		worst;
	u_int8_t		raw[6];
	u_int8_t		reserved;
} __packed;

struct ata_smart_attributes {
	u_int16_t		data_structure_revision;
	struct ata_smart_attr	attributes[30];
	u_int8_t		offline_data_collection_status;
	u_int8_t		self_test_exec_status;
	u_int16_t		total_time_to_complete_off_line;
	u_int8_t		vendor_specific_366;
	u_int8_t		offline_data_collection_capability;
	u_int16_t		smart_capability;
	u_int8_t		errorlog_capability;
	u_int8_t		vendor_specific_371;
	u_int8_t		short_test_completion_time;
	u_int8_t		extend_test_completion_time;
	u_int8_t		reserved_374_385[12];
	u_int8_t		vendor_specific_386_509[125];
	int8_t			checksum;
} __packed;

struct ata_smart_thresh {
	u_int8_t		id;
	u_int8_t		value;
	u_int8_t		reserved[10];
} __packed;

struct ata_smart_thresholds {
	u_int16_t		data_structure_revision;
	struct ata_smart_thresh	thresholds[30];
	u_int8_t		reserved[18];
	u_int8_t		vendor_specific[131];
	int8_t			checksum;
} __packed;

struct ata_smart_selftest {
	u_int8_t		number;
	u_int8_t		status;
	uint16_t		time_stamp;
	u_int8_t		failure_check_point;
	u_int32_t		lba_first_error;
	u_int8_t		vendor_specific[15];
} __packed;

struct ata_smart_selftestlog {
	u_int16_t		data_structure_revision;
	struct ata_smart_selftest log_entries[21];
	u_int8_t		vendorspecific[2];
	u_int8_t		mostrecenttest;
	u_int8_t		reserved[2];
	u_int8_t		checksum;
} __packed;

#endif /* _DEV_ATA_ATAREG_H_ */
