/*	$NetBSD: trm.c,v 1.1 2001/11/03 17:01:17 tsutsui Exp $	*/
/*
 * Device Driver for Tekram DC395U/UW/F, DC315/U
 * PCI SCSI Bus Master Host Adapter
 * (SCSI chip set used Tekram ASIC TRM-S1040)
 *
 * Copyright (c) 2001 Rui-Xiang Guo
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
 * 3. The name of the author may not be used to endorse or promote products
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
 */
/*
 * Ported from
 *   dc395x_trm.c
 *
 * Written for NetBSD 1.4.x by
 *   Erich Chen     (erich@tekram.com.tw)
 *
 * Provided by
 *   (C)Copyright 1995-1999 Tekram Technology Co., Ltd. All rights reserved.
 */

#undef TRM_DEBUG

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <uvm/uvm_extern.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/trmreg.h>

/*
 * feature of chip set MAX value
 */
#define TRM_MAX_TARGETS		16
#define TRM_MAX_SG_ENTRIES	(MAXPHYS / PAGE_SIZE + 1)
#define TRM_MAX_SRB		32

/*
 * Segment Entry
 */
struct trm_sg_entry {
	u_int32_t address;
	u_int32_t length;
};

#define TRM_SG_SIZE	(sizeof(struct trm_sg_entry) * TRM_MAX_SG_ENTRIES)

/*
 **********************************************************************
 * The SEEPROM structure for TRM_S1040
 **********************************************************************
 */
struct nvram_target {
	u_int8_t config0;		/* Target configuration byte 0 */
#define NTC_DO_WIDE_NEGO	0x20	/* Wide negotiate	     */
#define NTC_DO_TAG_QUEUING	0x10	/* Enable SCSI tag queuing   */
#define NTC_DO_SEND_START	0x08	/* Send start command SPINUP */
#define NTC_DO_DISCONNECT	0x04	/* Enable SCSI disconnect    */
#define NTC_DO_SYNC_NEGO	0x02	/* Sync negotiation	     */
#define NTC_DO_PARITY_CHK	0x01	/* (it should define at NAC) Parity check enable */
	u_int8_t period;		/* Target period	       */
	u_int8_t config2;		/* Target configuration byte 2 */
	u_int8_t config3;		/* Target configuration byte 3 */
};

struct trm_nvram {
	u_int8_t subvendor_id[2];		/* 0,1 Sub Vendor ID */
	u_int8_t subsys_id[2];			/* 2,3 Sub System ID */
	u_int8_t subclass;			/* 4   Sub Class */
	u_int8_t vendor_id[2];			/* 5,6 Vendor ID */
	u_int8_t device_id[2];			/* 7,8 Device ID */
	u_int8_t reserved0;			/* 9   Reserved */
	struct nvram_target target[TRM_MAX_TARGETS];
						/* 10,11,12,13
						 * 14,15,16,17
						 * ....
						 * 70,71,72,73 */
	u_int8_t scsi_id;			/* 74 Host Adapter SCSI ID */
	u_int8_t channel_cfg;			/* 75 Channel configuration */
#define NAC_SCANLUN		0x20	/* Include LUN as BIOS device */
#define NAC_DO_PARITY_CHK	0x08    /* Parity check enable */
#define NAC_POWERON_SCSI_RESET	0x04	/* Power on reset enable      */
#define NAC_GREATER_1G		0x02	/* > 1G support enable	      */
#define NAC_GT2DRIVES		0x01	/* Support more than 2 drives */
	u_int8_t delay_time;			/* 76 Power on delay time */
	u_int8_t max_tag;			/* 77 Maximum tags */
	u_int8_t reserved1;			/* 78 */
	u_int8_t boot_target;			/* 79 */
	u_int8_t boot_lun;			/* 80 */
	u_int8_t reserved2;			/* 81 */
	u_int8_t reserved3[44];			/* 82,..125 */
	u_int8_t checksum0;			/* 126 */
	u_int8_t checksum1;			/* 127 */
#define TRM_NVRAM_CKSUM	0x1234
};

/* Nvram Initiater bits definition */
#define MORE2_DRV		0x00000001
#define GREATER_1G		0x00000002
#define RST_SCSI_BUS		0x00000004
#define ACTIVE_NEGATION		0x00000008
#define NO_SEEK			0x00000010
#define LUN_CHECK		0x00000020

#define trm_wait_30us()	DELAY(30)

/*
 *-----------------------------------------------------------------------
 *			SCSI Request Block
 *-----------------------------------------------------------------------
 */
struct trm_srb {
	struct trm_srb *next;
	struct trm_dcb *dcb;

	struct trm_sg_entry *sgentry;
	struct trm_sg_entry tempsg;	/* Temp sgentry when Request Sense */
	/*
	 * the scsipi_xfer for this cmd
	 */
	struct scsipi_xfer *xs;
	bus_dmamap_t dmap;
	bus_size_t sgoffset;		/* Xfer buf offset */

	u_int32_t buflen;		/* Total xfer length */
	u_int32_t templen;		/* Temp buflen when Request Sense */
	u_int32_t sgaddr;		/* SGList physical starting address */

	u_int state;			/* SRB State */
#define SRB_FREE		0x0000
#define SRB_WAIT		0x0001
#define SRB_READY		0x0002
#define SRB_MSGOUT		0x0004	/* arbitration+msg_out 1st byte */
#define SRB_MSGIN		0x0008
#define SRB_EXTEND_MSGIN	0x0010
#define SRB_COMMAND		0x0020
#define SRB_START_		0x0040	/* arbitration+msg_out+command_out */
#define SRB_DISCONNECT		0x0080
#define SRB_DATA_XFER		0x0100
#define SRB_XFERPAD		0x0200
#define SRB_STATUS		0x0400
#define SRB_COMPLETED		0x0800
#define SRB_ABORT_SENT		0x1000
#define SRB_DO_SYNC_NEGO	0x2000
#define SRB_DO_WIDE_NEGO	0x4000
#define SRB_UNEXPECT_RESEL	0x8000
	u_int8_t *msg;

	int sgcnt;
	int sgindex;

	int phase;			/* SCSI phase */
	int hastat;			/* Host Adapter Status */
#define H_STATUS_GOOD		0x00
#define H_SEL_TIMEOUT		0x11
#define H_OVER_UNDER_RUN	0x12
#define H_UNEXP_BUS_FREE	0x13
#define H_TARGET_PHASE_F	0x14
#define H_INVALID_CCB_OP	0x16
#define H_LINK_CCB_BAD		0x17
#define H_BAD_TARGET_DIR	0x18
#define H_DUPLICATE_CCB		0x19
#define H_BAD_CCB_OR_SG		0x1A
#define H_ABORT			0xFF
	int tastat;			/* Target SCSI Status Byte */
	int flag;			/* SRBFlag */
#define DATAOUT			0x0080
#define DATAIN			0x0040
#define RESIDUAL_VALID		0x0020
#define ENABLE_TIMER		0x0010
#define RESET_DEV0		0x0004
#define ABORT_DEV		0x0002
#define AUTO_REQSENSE		0x0001
	int srbstat;			/* SRB Status */
#define SRB_OK			0x01
#define ABORTION		0x02
#define OVER_RUN		0x04
#define UNDER_RUN		0x08
#define PARITY_ERROR		0x10
#define SRB_ERROR		0x20
	int tagnum;			/* Tag number */
	int retry;			/* Retry Count */
	int msgcnt;

	int cmdlen;			/* SCSI command length */
	u_int8_t cmd[12];       	/* SCSI command */
	u_int8_t tempcmd[6];		/* Temp cmd when Request Sense */

	u_int8_t msgin[6];
	u_int8_t msgout[6];
};

/*
 *-----------------------------------------------------------------------
 *			Device Control Block
 *-----------------------------------------------------------------------
 */
struct trm_dcb {
	struct trm_dcb *next;

	struct trm_srb *waitsrb;
	struct trm_srb *last_waitsrb;

	struct trm_srb *gosrb;
	struct trm_srb *last_gosrb;

	struct trm_srb *actsrb;

	int gosrb_cnt;
	u_int maxcmd;		/* Max command */

	int id;			/* SCSI Target ID  (SCSI Only) */
	int lun;		/* SCSI Log.  Unit (SCSI Only) */

	u_int8_t tagmask;	/* Tag mask */

	u_int8_t tacfg;		/* Target Config */
	u_int8_t idmsg;		/* Identify Msg */
	u_int8_t period;	/* Max Period for nego. */

	u_int8_t synctl;	/* Sync control for reg. */
	u_int8_t offset;	/* Sync offset for reg. and nego.(low nibble) */
	u_int8_t mode;		/* Sync mode ? (1 sync):(0 async)  */
#define SYNC_NEGO_ENABLE	0x01
#define SYNC_NEGO_DONE		0x02
#define WIDE_NEGO_ENABLE	0x04
#define WIDE_NEGO_DONE		0x08
#define EN_TAG_QUEUING		0x10
#define EN_ATN_STOP		0x20
#define SYNC_NEGO_OFFSET	15
	u_int8_t flag;
#define ABORT_DEV_		0x01
#define SHOW_MESSAGE_		0x02
	u_int8_t type;		/* Device Type */
};

/*
 *-----------------------------------------------------------------------
 *			Adapter Control Block
 *-----------------------------------------------------------------------
 */
struct trm_softc {
	struct device sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_dma_tag_t sc_dmat;
	bus_dmamap_t sc_dmamap;	/* Map the control structures */

	struct trm_dcb *sc_linkdcb;
	struct trm_dcb *sc_roundcb;

	struct trm_dcb *sc_actdcb;
	struct trm_dcb *sc_dcb[TRM_MAX_TARGETS][8];

	struct trm_srb *sc_freesrb;
	struct trm_srb *sc_tempsrb;
	struct trm_srb *sc_srb;	/* SRB array */

	struct trm_sg_entry *sc_sglist;

	int maxid;
	int maxtag;		/* Max Tag number */
	/*
	 * Link to the generic SCSI driver
	 */
	struct scsipi_channel sc_channel;
	struct scsipi_adapter sc_adapter;

	int sc_id;		/* Adapter SCSI Target ID */

	int devcnt;		/* Device Count */

	int devflag[TRM_MAX_TARGETS][8]; /* flag of initDCB for device */

	int devscan[TRM_MAX_TARGETS][8];
	int devscan_end;
	int cur_offset;		/* Current Sync offset */

	struct trm_nvram sc_eeprom;
	int sc_config;
#define HCC_WIDE_CARD		0x20
#define HCC_SCSI_RESET		0x10
#define HCC_PARITY		0x08
#define HCC_AUTOTERM		0x04
#define HCC_LOW8TERM		0x02
#define HCC_UP8TERM		0x01
	int sc_flag;
#define RESET_DEV		0x01
#define RESET_DETECT		0x02
#define RESET_DONE		0x04
};

/*
 * SCSI Status codes not defined in scsi_all.h
 */
#define SCSI_COND_MET		0x04	/* Condition Met              */
#define SCSI_INTERM_COND_MET	0x14	/* Intermediate condition met */
#define SCSI_UNEXP_BUS_FREE	0xFD	/* Unexpect Bus Free          */
#define SCSI_BUS_RST_DETECT	0xFE	/* Scsi Bus Reset detected    */
#define SCSI_SEL_TIMEOUT	0xFF	/* Selection Time out         */

static void trm_rewait_srb(struct trm_dcb *, struct trm_srb *);
static void trm_wait_srb(struct trm_softc *);
static void trm_reset_device(struct trm_softc *);
static void trm_recover_srb(struct trm_softc *);
static int  trm_start_scsi(struct trm_softc *, struct trm_dcb *,
    struct trm_srb *);
static int  trm_intr(void *);

static void trm_dataout_phase0(struct trm_softc *, struct trm_srb *, int *);
static void trm_datain_phase0(struct trm_softc *, struct trm_srb *, int *);
static void trm_command_phase0(struct trm_softc *, struct trm_srb *, int *);
static void trm_status_phase0(struct trm_softc *, struct trm_srb *, int *);
static void trm_msgout_phase0(struct trm_softc *, struct trm_srb *, int *);
static void trm_msgin_phase0(struct trm_softc *, struct trm_srb *, int *);
static void trm_dataout_phase1(struct trm_softc *, struct trm_srb *, int *);
static void trm_datain_phase1(struct trm_softc *, struct trm_srb *, int *);
static void trm_command_phase1(struct trm_softc *, struct trm_srb *, int *);
static void trm_status_phase1(struct trm_softc *, struct trm_srb *, int *);
static void trm_msgout_phase1(struct trm_softc *, struct trm_srb *, int *);
static void trm_msgin_phase1(struct trm_softc *, struct trm_srb *, int *);
static void trm_nop0(struct trm_softc *, struct trm_srb *, int *);
static void trm_nop1(struct trm_softc *, struct trm_srb *, int *);

static void trm_set_xfer_rate(struct trm_softc *, struct trm_srb *,
    struct trm_dcb *);
static void trm_dataio_xfer(struct trm_softc *, struct trm_srb *, int);
static void trm_disconnect(struct trm_softc *);
static void trm_reselect(struct trm_softc *);
static void trm_srb_done(struct trm_softc *, struct trm_dcb *,
    struct trm_srb *);
static void trm_doing_srb_done(struct trm_softc *);
static void trm_scsi_reset_detect(struct trm_softc *);
static void trm_reset_scsi_bus(struct trm_softc *);
static void trm_request_sense(struct trm_softc *, struct trm_dcb *,
    struct trm_srb *);
static void trm_msgout_abort(struct trm_softc *, struct trm_srb *);
static void trm_timeout(void *);
static void trm_reset(struct trm_softc *);
static void trm_send_srb(struct scsipi_xfer *, struct trm_softc *,
    struct trm_srb *);
static int  trm_init(struct trm_softc *);
static void trm_init_adapter(struct trm_softc *);
static void trm_init_dcb(struct trm_softc *, struct trm_dcb *,
    struct scsipi_xfer *);
static void trm_link_srb(struct trm_softc *);
static void trm_init_sc(struct trm_softc *);
static void trm_check_eeprom(struct trm_softc *, struct trm_nvram *);
static void trm_release_srb(struct trm_softc *, struct trm_dcb *,
    struct trm_srb *);
void trm_scsipi_request(struct scsipi_channel *, scsipi_adapter_req_t, void *);

static void trm_eeprom_read_all(struct trm_softc *, struct trm_nvram *);
static void trm_eeprom_write_all(struct trm_softc *, struct trm_nvram *);
static void trm_eeprom_set_data(struct trm_softc *, u_int8_t, u_int8_t);
static void trm_eeprom_write_cmd(struct trm_softc *, u_int8_t, u_int8_t);
static u_int8_t trm_eeprom_get_data(struct trm_softc *, u_int8_t);

static int  trm_probe(struct device *, struct cfdata *, void *);
static void trm_attach(struct device *, struct device *, void *);

struct cfattach trm_ca = {
	sizeof(struct trm_softc), trm_probe, trm_attach
};


/*
 * state_v = (void *) trm_scsi_phase0[phase]
 */
static void *trm_scsi_phase0[] = {
	trm_dataout_phase0,	/* phase:0 */
	trm_datain_phase0,	/* phase:1 */
	trm_command_phase0,	/* phase:2 */
	trm_status_phase0,	/* phase:3 */
	trm_nop0,		/* phase:4 */
	trm_nop1,		/* phase:5 */
	trm_msgout_phase0,	/* phase:6 */
	trm_msgin_phase0,	/* phase:7 */
};

/*
 * state_v = (void *) trm_scsi_phase1[phase]
 */
static void *trm_scsi_phase1[] = {
	trm_dataout_phase1,	/* phase:0 */
	trm_datain_phase1,	/* phase:1 */
	trm_command_phase1,	/* phase:2 */
	trm_status_phase1,	/* phase:3 */
	trm_nop0,		/* phase:4 */
	trm_nop1,		/* phase:5 */
	trm_msgout_phase1,	/* phase:6 */
	trm_msgin_phase1,	/* phase:7 */
};

/* real period: */
static const u_int8_t trm_clock_period[] = {
	13,	/*  52  ns 20.0 MB/sec */
	18,	/*  72  ns 13.3 MB/sec */
	25,	/* 100  ns 10.0 MB/sec */
	31,	/* 124  ns  8.0 MB/sec */
	37,	/* 148  ns  6.6 MB/sec */
	43,	/* 172  ns  5.7 MB/sec */
	50,	/* 200  ns  5.0 MB/sec */
	62	/* 248  ns  4.0 MB/sec */
};

/*
 * Q back to pending Q
 */
static void
trm_rewait_srb(dcb, srb)
	struct trm_dcb *dcb;
	struct trm_srb *srb;
{
	struct trm_srb *psrb1;
	int s;

	s = splbio();

	dcb->gosrb_cnt--;
	psrb1 = dcb->gosrb;
	if (srb == psrb1)
		dcb->gosrb = psrb1->next;
	else {
		while (srb != psrb1->next)
			psrb1 = psrb1->next;

		psrb1->next = srb->next;
		if (srb == dcb->last_gosrb)
			dcb->last_gosrb = psrb1;
	}
	if (dcb->waitsrb) {
		srb->next = dcb->waitsrb;
		dcb->waitsrb = srb;
	} else {
		srb->next = NULL;
		dcb->waitsrb = srb;
		dcb->last_waitsrb = srb;
	}
	dcb->tagmask &= ~(1 << srb->tagnum); /* Free TAG number */

	splx(s);
}

static void
trm_wait_srb(sc)
	struct trm_softc *sc;
{
	struct trm_dcb *ptr, *ptr1;
	struct trm_srb *srb;
	int s;

	s = splbio();

	if (sc->sc_actdcb == NULL &&
	    (sc->sc_flag & (RESET_DETECT | RESET_DONE | RESET_DEV)) == 0) {
		ptr = sc->sc_roundcb;
		if (ptr == NULL) {
			ptr = sc->sc_linkdcb;
			sc->sc_roundcb = ptr;
		}
		for (ptr1 = ptr; ptr1 != NULL;) {
			sc->sc_roundcb = ptr1->next;
			if (ptr1->maxcmd <= ptr1->gosrb_cnt ||
			    (srb = ptr1->waitsrb) == NULL) {
				if (sc->sc_roundcb == ptr)
					break;
				ptr1 = ptr1->next;
			} else {
				if (trm_start_scsi(sc, ptr1, srb) == 0) {
					/*
					 * If trm_start_scsi return 0 :
					 * current interrupt status is
					 * interrupt enable.  It's said that
					 * SCSI processor is unoccupied
					 */
					ptr1->gosrb_cnt++;
					if (ptr1->last_waitsrb == srb) {
						ptr1->waitsrb = NULL;
						ptr1->last_waitsrb = NULL;
					} else
						ptr1->waitsrb = srb->next;

					srb->next = NULL;
					if (ptr1->gosrb != NULL)
						ptr1->last_gosrb->next = srb;
					else
						ptr1->gosrb = srb;

					ptr1->last_gosrb = srb;
				}
				break;
			}
		}
	}
	splx(s);
}

static void
trm_send_srb(xs, sc, srb)
	struct scsipi_xfer *xs;
	struct trm_softc *sc;
	struct trm_srb *srb;
{
	struct trm_dcb *dcb;
	int s;

#ifdef TRM_DEBUG
	printf("trm_send_srb..........\n");
#endif
	s = splbio();

	/*
	 *  now get the DCB from upper layer( OS )
	 */
	dcb = srb->dcb;

	if (dcb->maxcmd <= dcb->gosrb_cnt ||
	    sc->sc_actdcb != NULL ||
	    (sc->sc_flag & (RESET_DETECT | RESET_DONE | RESET_DEV))) {
		if (dcb->waitsrb != NULL) {
			dcb->last_waitsrb->next = srb;
			dcb->last_waitsrb = srb;
			srb->next = NULL;
		} else {
			dcb->waitsrb = srb;
			dcb->last_waitsrb = srb;
		}
		splx(s);
		return;
	}
	if (dcb->waitsrb != NULL) {
		dcb->last_waitsrb->next = srb;
		dcb->last_waitsrb = srb;
		srb->next = NULL;
		/* srb = GetWaitingSRB(dcb); */
		srb = dcb->waitsrb;
		dcb->waitsrb = srb->next;
		srb->next = NULL;
	}
	if (trm_start_scsi(sc, dcb, srb) == 0) {
		/*
		 * If trm_start_scsi return 0: current interrupt status
		 * is interrupt enable.  It's said that SCSI processor is
		 * unoccupied.
		 */
		dcb->gosrb_cnt++;	/* stack waiting SRB */
		if (dcb->gosrb != NULL) {
			dcb->last_gosrb->next = srb;
			dcb->last_gosrb = srb;
		} else {
			dcb->gosrb = srb;
			dcb->last_gosrb = srb;
		}
	} else {
		/*
		 * If trm_start_scsi return 1: current interrupt status
		 * is interrupt disreenable.  It's said that SCSI processor
		 * has more one SRB need to do we need reQ back SRB.
		 */
		if (dcb->waitsrb != NULL) {
			srb->next = dcb->waitsrb;
			dcb->waitsrb = srb;
		} else {
			srb->next = NULL;
			dcb->waitsrb = srb;
			dcb->last_waitsrb = srb;
		}
	}

	splx(s);
}

/*
 * Called by GENERIC SCSI driver
 * enqueues a SCSI command
 */
void
trm_scsipi_request(chan, req, arg)
	struct scsipi_channel *chan;
	scsipi_adapter_req_t req;
	void *arg;
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct trm_softc *sc;
	struct trm_dcb *dcb = NULL;
	struct trm_srb *srb;
	struct scsipi_xfer *xs;
	int error, i, id, lun, s;

	sc = (struct trm_softc *)chan->chan_adapter->adapt_dev;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;
		id = xs->xs_periph->periph_target;
		lun = xs->xs_periph->periph_lun;
#ifdef TRM_DEBUG
		printf("trm_scsipi_request.....\n");
		printf("%s: id= %d lun= %d\n", sc->sc_dev.dv_xname, id, lun);
		printf("sc->devscan[id][lun]= %d\n", sc->devscan[id][lun]);
#endif
		if ((id > sc->maxid) || (lun > 7)) {
			xs->error = XS_DRIVER_STUFFUP;
			return;
		}
		dcb = sc->sc_dcb[id][lun];
		if (sc->devscan[id][lun] != 0 && sc->devflag[id][lun] == 0) {
			/*
			 * Scan SCSI BUS => trm_init_dcb
			 */
			if (sc->devcnt < TRM_MAX_TARGETS) {
#ifdef TRM_DEBUG
			  	printf("trm_init_dcb: dcb=%8x, ", (int) dcb);
				printf("ID=%2x, LUN=%2x\n", id, lun);
#endif
				sc->devflag[id][lun] = 1;
				trm_init_dcb(sc, dcb, xs);
			} else {
			  	printf("%s: ", sc->sc_dev.dv_xname);
				printf("sc->devcnt >= TRM_MAX_TARGETS\n");
				xs->error = XS_DRIVER_STUFFUP;
				return;
			}
		}

		if (xs->xs_control & XS_CTL_RESET) {
			trm_reset(sc);
			xs->error = XS_NOERROR | XS_RESET;
			return;
		}
		if (xs->xs_status & XS_STS_DONE) {
			printf("%s: Is it done?\n", sc->sc_dev.dv_xname);
			xs->xs_status &= ~XS_STS_DONE;
		}
		xs->error = 0;
		xs->status = 0;
		xs->resid = 0;

		s = splbio();

		/* Get SRB */
		srb = sc->sc_freesrb;
		if (srb != NULL) {
			sc->sc_freesrb = srb->next;
			srb->next = NULL;
#ifdef TRM_DEBUG
			printf("srb = %8p sc->sc_freesrb= %8p\n",
			    srb, sc->sc_freesrb);
#endif
		} else {
			xs->error = XS_RESOURCE_SHORTAGE;
			scsipi_done(xs);
			splx(s);
			return;
		}
		/*
		 * XXX BuildSRB(srb ,dcb); XXX
		 */
		srb->dcb = dcb;
		srb->xs = xs;
		srb->cmdlen = xs->cmdlen;
		/*
		 * Move layer of CAM command block to layer of SCSI
		 * Request Block for SCSI processor command doing.
		 */
		memcpy(srb->cmd, xs->cmd, xs->cmdlen);
		if (xs->datalen > 0) {
#ifdef TRM_DEBUG
			printf("xs->datalen...\n");
			printf("sc->sc_dmat=%x\n", (int) sc->sc_dmat);
			printf("srb->dmap=%x\n", (int) srb->dmap);
			printf("xs->data=%x\n", (int) xs->data);
			printf("xs->datalen=%x\n", (int) xs->datalen);
#endif
			if ((error = bus_dmamap_load(sc->sc_dmat, srb->dmap,
			    xs->data, xs->datalen, NULL,
			    (xs->xs_control & XS_CTL_NOSLEEP) ?
			    BUS_DMA_NOWAIT : BUS_DMA_WAITOK)) != 0) {
				printf("%s: DMA transfer map unable to load, "
				    "error = %d\n", sc->sc_dev.dv_xname, error);
				xs->error = XS_DRIVER_STUFFUP;
				/*
				 * free SRB
				 */
				srb->next = sc->sc_freesrb;
				sc->sc_freesrb = srb;
				return;
			}
			bus_dmamap_sync(sc->sc_dmat, srb->dmap, 0,
			    srb->dmap->dm_mapsize,
			    (xs->xs_control & XS_CTL_DATA_IN) ?
			    BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

			/* Set up the scatter gather list */
			for (i = 0; i < srb->dmap->dm_nsegs; i++) {
				srb->sgentry[i].address =
				    htole32(srb->dmap->dm_segs[i].ds_addr);
				srb->sgentry[i].length =
				    htole32(srb->dmap->dm_segs[i].ds_len);
			}
			srb->buflen = xs->datalen;
			srb->sgcnt = srb->dmap->dm_nsegs;
		} else {
			srb->sgentry[0].address = 0;
			srb->sgentry[0].length = 0;
			srb->buflen = 0;
			srb->sgcnt = 0;
		}
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap,
		    srb->sgoffset, TRM_SG_SIZE, BUS_DMASYNC_PREWRITE);

		if (dcb->type != T_SEQUENTIAL)
			srb->retry = 1;
		else
			srb->retry = 0;

		srb->sgindex = 0;
		srb->hastat = 0;
		srb->tastat = 0;
		srb->msgcnt = 0;
		srb->srbstat = 0;
		srb->flag = 0;
		srb->state = 0;
		srb->phase = PH_BUS_FREE;	/* SCSI bus free Phase */

		trm_send_srb(xs, sc, srb);
		splx(s);

		if ((xs->xs_control & XS_CTL_POLL) == 0) {
			int timeout = xs->timeout;
			timeout = (timeout > 100000) ?
			    timeout / 1000 * hz : timeout * hz / 1000;
			callout_reset(&xs->xs_callout, timeout,
			    trm_timeout, srb);
		} else {
			s = splbio();
			do {
				while (--xs->timeout) {
					DELAY(1000);
					if (bus_space_read_2(iot, ioh,
					    TRM_SCSI_STATUS) & SCSIINTERRUPT)
						break;
				}
				if (xs->timeout == 0) {
					trm_timeout(srb);
					break;
				} else
					trm_intr(sc);
			} while ((xs->xs_status & XS_STS_DONE) == 0);
			splx(s);
		}
		return;

	case ADAPTER_REQ_GROW_RESOURCES:
		/* XXX Not supported. */
		return;

	case ADAPTER_REQ_SET_XFER_MODE:
		/* XXX XXX XXX */
		return;
	}
}

static void
trm_reset_device(sc)
	struct trm_softc *sc;
{
	struct trm_dcb *dcb, *pdcb;
	struct trm_nvram *eeprom;
	int index;

	dcb = sc->sc_linkdcb;
	if (dcb == NULL)
		return;

	pdcb = dcb;
	do {
		dcb->mode &= ~(SYNC_NEGO_DONE | WIDE_NEGO_DONE);
		dcb->synctl = 0;
		dcb->offset = 0;
		eeprom = &sc->sc_eeprom;
		dcb->tacfg = eeprom->target[dcb->id].config0;
		index = eeprom->target[dcb->id].period & 0x07;
		dcb->period = trm_clock_period[index];
		if ((dcb->tacfg & NTC_DO_WIDE_NEGO) &&
		    (sc->sc_config & HCC_WIDE_CARD))
			dcb->mode |= WIDE_NEGO_ENABLE;

		dcb = dcb->next;
	}
	while (pdcb != dcb);
}

static void
trm_recover_srb(sc)
	struct trm_softc *sc;
{
	struct trm_dcb *dcb, *pdcb;
	struct trm_srb *psrb, *psrb2;
	int i;

	dcb = sc->sc_linkdcb;
	if (dcb == NULL)
		return;

	pdcb = dcb;
	do {
		psrb = pdcb->gosrb;
		for (i = 0; i < pdcb->gosrb_cnt; i++) {
			psrb2 = psrb;
			psrb = psrb->next;
			if (pdcb->waitsrb) {
				psrb2->next = pdcb->waitsrb;
				pdcb->waitsrb = psrb2;
			} else {
				pdcb->waitsrb = psrb2;
				pdcb->last_waitsrb = psrb2;
				psrb2->next = NULL;
			}
		}
		pdcb->gosrb_cnt = 0;
		pdcb->gosrb = NULL;
		pdcb->tagmask = 0;
		pdcb = pdcb->next;
	}
	while (pdcb != dcb);
}

/*
 * perform a hard reset on the SCSI bus (and TRM_S1040 chip).
 */
static void
trm_reset(sc)
	struct trm_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s;

#ifdef TRM_DEBUG
	printf("%s: SCSI RESET.........", sc->sc_dev.dv_xname);
#endif
	s = splbio();

	/* disable SCSI and DMA interrupt */
	bus_space_write_1(iot, ioh, TRM_DMA_INTEN, 0);
	bus_space_write_1(iot, ioh, TRM_SCSI_INTEN, 0);

	trm_reset_scsi_bus(sc);
	DELAY(500000);

	/* Enable SCSI interrupt */
	bus_space_write_1(iot, ioh, TRM_SCSI_INTEN,
	    EN_SELECT | EN_SELTIMEOUT | EN_DISCONNECT | EN_RESELECTED |
	    EN_SCSIRESET | EN_BUSSERVICE | EN_CMDDONE);

	/* Enable DMA interrupt */
	bus_space_write_1(iot, ioh, TRM_DMA_INTEN, EN_SCSIINTR);

	/* Clear DMA FIFO */
	bus_space_write_1(iot, ioh, TRM_DMA_CONTROL, CLRXFIFO);

	/* Clear SCSI FIFO */
	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_CLRFIFO);

	trm_reset_device(sc);
	trm_doing_srb_done(sc);
	sc->sc_actdcb = NULL;
	sc->sc_flag = 0;	/* RESET_DETECT, RESET_DONE, RESET_DEV */
	trm_wait_srb(sc);

	splx(s);
}

static void
trm_timeout(arg)
	void *arg;
{
	struct trm_srb *srb = (struct trm_srb *)arg;
	struct scsipi_xfer *xs = srb->xs;
	struct scsipi_periph *periph = xs->xs_periph;
	struct trm_softc *sc;
	int s;

	if (xs == NULL)
		printf("trm_timeout called with xs == NULL\n");

	else {
		scsipi_printaddr(xs->xs_periph);
		printf("SCSI OpCode 0x%02x timed out\n", xs->cmd->opcode);
	}

	sc = (void *)periph->periph_channel->chan_adapter->adapt_dev;

	s = splbio();
	trm_reset_scsi_bus(sc);
	callout_stop(&xs->xs_callout);
	splx(s);
}

static int
trm_start_scsi(sc, dcb, srb)
	struct trm_softc *sc;
	struct trm_dcb *dcb;
	struct trm_srb *srb;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int tagnum;
	u_int32_t tagmask;
	u_int8_t scsicmd, idmsg;

	srb->tagnum = 31;

	bus_space_write_1(iot, ioh, TRM_SCSI_HOSTID, sc->sc_id);
	bus_space_write_1(iot, ioh, TRM_SCSI_TARGETID, dcb->id);
	bus_space_write_1(iot, ioh, TRM_SCSI_SYNC, dcb->synctl);
	bus_space_write_1(iot, ioh, TRM_SCSI_OFFSET, dcb->offset);
	srb->phase = PH_BUS_FREE;	/* initial phase */
	/* Flush FIFO */
	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_CLRFIFO);

	idmsg = dcb->idmsg;

	if ((srb->cmd[0] == INQUIRY) ||
	    (srb->cmd[0] == REQUEST_SENSE) ||
	    (srb->flag & AUTO_REQSENSE)) {
		if (((dcb->mode & WIDE_NEGO_ENABLE) &&
		     (dcb->mode & WIDE_NEGO_DONE) == 0) ||
		    ((dcb->mode & SYNC_NEGO_ENABLE) &&
		     (dcb->mode & SYNC_NEGO_DONE) == 0)) {
			if ((dcb->idmsg & 7) == 0 || srb->cmd[0] != INQUIRY) {
				scsicmd = SCMD_SEL_ATNSTOP;
				srb->state = SRB_MSGOUT;
				goto polling;
			}
		}
		/* Send identify message */
		bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
		    idmsg & ~MSG_IDENTIFY_DISCFLAG);
		scsicmd = SCMD_SEL_ATN;
		srb->state = SRB_START_;
	} else { /* not inquiry,request sense,auto request sense */
		/* Send identify message */
		bus_space_write_1(iot, ioh, TRM_SCSI_FIFO, idmsg);
		DELAY(30);
		scsicmd = SCMD_SEL_ATN;
		srb->state = SRB_START_;
		if (dcb->mode & EN_TAG_QUEUING) {
			/* Send Tag message, get tag id */
			tagmask = 1;
			tagnum = 0;
			while (tagmask & dcb->tagmask) {
				tagmask = tagmask << 1;
				tagnum++;
			}
			/* Send Tag id */
			bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
			    MSG_SIMPLE_Q_TAG);
			bus_space_write_1(iot, ioh, TRM_SCSI_FIFO, tagnum);

			dcb->tagmask |= tagmask;
			srb->tagnum = tagnum;

			scsicmd = SCMD_SEL_ATN3;
			srb->state = SRB_START_;
		}
	}
polling:
	/*
	 * Send CDB ..command block...
	 */
	if (srb->flag & AUTO_REQSENSE) {
		bus_space_write_1(iot, ioh, TRM_SCSI_FIFO, REQUEST_SENSE);
		bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
		    dcb->idmsg << SCSI_CMD_LUN_SHIFT);
		bus_space_write_1(iot, ioh, TRM_SCSI_FIFO, 0);
		bus_space_write_1(iot, ioh, TRM_SCSI_FIFO, 0);
		bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
		    sizeof(struct scsipi_sense_data));
		bus_space_write_1(iot, ioh, TRM_SCSI_FIFO, 0);
	} else
		bus_space_write_multi_1(iot, ioh, TRM_SCSI_FIFO,
		    srb->cmd, srb->cmdlen);

	if (bus_space_read_2(iot, ioh, TRM_SCSI_STATUS) & SCSIINTERRUPT) {
		/*
		 * If trm_start_scsi return 1: current interrupt status
		 * is interrupt disreenable.  It's said that SCSI processor
		 * has more one SRB need to do, SCSI processor has been
		 * occupied by one SRB.
		 */
		srb->state = SRB_READY;
		dcb->tagmask &= ~(1 << srb->tagnum);
		return (1);
	} else {
		/*
		 * If trm_start_scsi return 0: current interrupt status
		 * is interrupt enable.  It's said that SCSI processor is
		 * unoccupied.
		 */
		srb->phase = PH_BUS_FREE;	/* SCSI bus free Phase */
		sc->sc_actdcb = dcb;
		dcb->actsrb = srb;
		bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL,
		    DO_DATALATCH | DO_HWRESELECT);
		/* it's important for atn stop */
		/*
		 * SCSI command
		 */
		bus_space_write_1(iot, ioh, TRM_SCSI_COMMAND, scsicmd);
		return (0);
	}
}

/*
 * Catch an interrupt from the adapter
 * Process pending device interrupts.
 */
static int
trm_intr(vsc)
	void *vsc;
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct trm_softc *sc;
	struct trm_dcb *dcb;
	struct trm_srb *srb;
	void (*state_v) (struct trm_softc *, struct trm_srb *, int *);
	int phase, intstat, stat = 0;

#ifdef TRM_DEBUG
	printf("trm_intr......\n");
#endif
	sc = (struct trm_softc *)vsc;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	if (sc == NULL)
		return (0);

	stat = bus_space_read_2(iot, ioh, TRM_SCSI_STATUS);
	if ((stat & SCSIINTERRUPT) == 0)
		return (0);

#ifdef TRM_DEBUG
	printf("stat=%2x,", stat);
#endif
	intstat = bus_space_read_1(iot, ioh, TRM_SCSI_INTSTATUS);

#ifdef TRM_DEBUG
	printf("intstat=%2x,", intstat);
#endif
	if (intstat & (INT_SELTIMEOUT | INT_DISCONNECT)) {
		trm_disconnect(sc);
		return (1);
	}
	if (intstat & INT_RESELECTED) {
		trm_reselect(sc);
		return (1);
	}
	if (intstat & INT_SCSIRESET) {
		trm_scsi_reset_detect(sc);
		return (1);
	}
	if (intstat & (INT_BUSSERVICE | INT_CMDDONE)) {
		dcb = sc->sc_actdcb;
		srb = dcb->actsrb;
		if (dcb != NULL)
			if (dcb->flag & ABORT_DEV_) {
				srb->msgout[0] = MSG_ABORT;
				trm_msgout_abort(sc, srb);
			}
		/*
		 * software sequential machine
		 */
		phase = srb->phase;	/* phase: */

		/*
		 * 62037 or 62137 call  trm_scsi_phase0[]... "phase
		 * entry" handle every phase before start transfer
		 */
		state_v = (void *)trm_scsi_phase0[phase];
		state_v(sc, srb, &stat);

		/*
		 *        if there were any exception occured
		 * stat will be modify to bus free phase new
		 * stat transfer out from ... prvious state_v
		 * 
		 */
		/* phase:0,1,2,3,4,5,6,7 */
		srb->phase = stat & PHASEMASK;
		phase = stat & PHASEMASK;

		/*
		 * call  trm_scsi_phase1[]... "phase entry" handle every
		 * phase do transfer
		 */
		state_v = (void *)trm_scsi_phase1[phase];
		state_v(sc, srb, &stat);
		return (1);
	}
	return (0);
}

static void
trm_msgout_phase0(sc, srb, pstat)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int *pstat;
{

	if (srb->state & (SRB_UNEXPECT_RESEL | SRB_ABORT_SENT))
		*pstat = PH_BUS_FREE;	/* .. initial phase */
}

static void
trm_msgout_phase1(sc, srb, pstat)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int *pstat;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct trm_dcb *dcb;

	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_CLRFIFO);
	dcb = sc->sc_actdcb;
	if ((srb->state & SRB_MSGOUT) == 0) {
		if (srb->msgcnt > 0) {
			bus_space_write_multi_1(iot, ioh, TRM_SCSI_FIFO,
			    srb->msgout, srb->msgcnt);
			srb->msgcnt = 0;
			if ((dcb->flag & ABORT_DEV_) &&
			    (srb->msgout[0] == MSG_ABORT))
				srb->state = SRB_ABORT_SENT;
		} else {
			if ((srb->cmd[0] == INQUIRY) ||
			    (srb->cmd[0] == REQUEST_SENSE) ||
			    (srb->flag & AUTO_REQSENSE))
				if (dcb->mode & SYNC_NEGO_ENABLE)
					goto mop1;

			bus_space_write_1(iot, ioh, TRM_SCSI_FIFO, MSG_ABORT);
		}
	} else {
mop1:		/* message out phase */
		if ((srb->state & SRB_DO_WIDE_NEGO) == 0 &&
		    (dcb->mode & WIDE_NEGO_ENABLE)) {
			/*
			 * WIDE DATA TRANSFER REQUEST code (03h)
			 */
			dcb->mode &= ~(SYNC_NEGO_DONE | EN_ATN_STOP);
			bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
			    dcb->idmsg & ~MSG_IDENTIFY_DISCFLAG);
			bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
			    MSG_EXTENDED); /* (01h) */

			/* Message length (02h) */
			bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
			    MSG_EXT_WDTR_LEN);

			/* wide data xfer (03h) */
			bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
			    MSG_EXT_WDTR);

			/* width: 0(8bit), 1(16bit) ,2(32bit) */
			bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
			    MSG_EXT_WDTR_BUS_16_BIT);

			srb->state |= SRB_DO_WIDE_NEGO;
		} else if ((srb->state & SRB_DO_SYNC_NEGO) == 0 &&
			   (dcb->mode & SYNC_NEGO_ENABLE)) {
			/*
			 * SYNCHRONOUS DATA TRANSFER REQUEST code (01h)
			 */
			if ((dcb->mode & WIDE_NEGO_DONE) == 0)
				bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
				    dcb->idmsg & ~MSG_IDENTIFY_DISCFLAG);

			bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
			    MSG_EXTENDED); /* (01h) */

			/* Message length (03h) */
			bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
			    MSG_EXT_SDTR_LEN);

			/* SYNCHRONOUS DATA TRANSFER REQUEST code (01h) */
			bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
			    MSG_EXT_SDTR);

			/* Transfer peeriod factor */
			bus_space_write_1(iot, ioh, TRM_SCSI_FIFO, dcb->period);

			/* REQ/ACK offset */
			bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
			    SYNC_NEGO_OFFSET);
			srb->state |= SRB_DO_SYNC_NEGO;
		}
	}
	/* it's important for atn stop */
	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_DATALATCH); 

	/*
	 * SCSI cammand
	 */
	bus_space_write_1(iot, ioh, TRM_SCSI_COMMAND, SCMD_FIFO_OUT);
}

static void
trm_command_phase0(sc, srb, pstat)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int *pstat;
{

}

static void
trm_command_phase1(sc, srb, pstat)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int *pstat;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct trm_dcb *dcb;

	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_CLRATN | DO_CLRFIFO);
	if (srb->flag & AUTO_REQSENSE) {
		bus_space_write_1(iot, ioh, TRM_SCSI_FIFO, REQUEST_SENSE);
		dcb = sc->sc_actdcb;
		/* target id */
		bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
		    dcb->idmsg << SCSI_CMD_LUN_SHIFT);
		bus_space_write_1(iot, ioh, TRM_SCSI_FIFO, 0);
		bus_space_write_1(iot, ioh, TRM_SCSI_FIFO, 0);
		/* sizeof(struct scsi_sense_data) */
		bus_space_write_1(iot, ioh, TRM_SCSI_FIFO,
		    sizeof(struct scsipi_sense_data));
		bus_space_write_1(iot, ioh, TRM_SCSI_FIFO, 0);
	} else
		bus_space_write_multi_1(iot, ioh, TRM_SCSI_FIFO,
		    srb->cmd, srb->cmdlen);

	srb->state = SRB_COMMAND;
	/* it's important for atn stop */
	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_DATALATCH); 

	/*
	 * SCSI cammand
	 */
	bus_space_write_1(iot, ioh, TRM_SCSI_COMMAND, SCMD_FIFO_OUT);
}

static void
trm_dataout_phase0(sc, srb, pstat)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int *pstat;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct trm_dcb *dcb;
	struct trm_sg_entry *sg;
	int sgindex;
	u_int32_t xferlen, leftcnt = 0;

	dcb = srb->dcb;

	if ((srb->state & SRB_XFERPAD) == 0) {
		if (*pstat & PARITYERROR)
			srb->srbstat |= PARITY_ERROR;

		if ((*pstat & SCSIXFERDONE) == 0) {
			/*
			 * when data transfer from DMA FIFO to SCSI FIFO
			 * if there was some data left in SCSI FIFO
			 */
			leftcnt = bus_space_read_1(iot, ioh, TRM_SCSI_FIFOCNT) &
			    SCSI_FIFOCNT_MASK;
			if (dcb->synctl & WIDE_SYNC)
				/*
				 * if WIDE scsi SCSI FIFOCNT unit is word
				 * so need to * 2
				 */
				leftcnt <<= 1;
		}
		/*
		 * caculate all the residue data that not yet tranfered
		 * SCSI transfer counter + left in SCSI FIFO data
		 *
		 * .....TRM_SCSI_XCNT (24bits)
		 * The counter always decrement by one for every SCSI
		 * byte transfer.
		 * .....TRM_SCSI_FIFOCNT ( 5bits)
		 * The counter is SCSI FIFO offset counter
		 */
		leftcnt += bus_space_read_4(iot, ioh, TRM_SCSI_XCNT);
		if (leftcnt == 1) {
			leftcnt = 0;
			bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL,
			    DO_CLRFIFO);
		}
		if ((leftcnt == 0) || (*pstat & SCSIXFERCNT_2_ZERO)) {
			while ((bus_space_read_1(iot, ioh, TRM_DMA_STATUS) &
			    DMAXFERCOMP) == 0)
				;

			srb->buflen = 0;
		} else {	/* Update SG list */
			/*
			 * if transfer not yet complete
			 * there were some data residue in SCSI FIFO or
			 * SCSI transfer counter not empty
			 */
			if (srb->buflen != leftcnt) {
				/* data that had transferred length */
				xferlen = srb->buflen - leftcnt;

				/* next time to be transferred length */
				srb->buflen = leftcnt;

				/*
				 * parsing from last time disconnect sgindex
				 */
				sg = srb->sgentry + srb->sgindex;
				for (sgindex = srb->sgindex;
				     sgindex < srb->sgcnt;
				     sgindex++, sg++) {
					/*
					 * find last time which SG transfer
					 * be disconnect
					 */
					if (xferlen >= le32toh(sg->length))
						xferlen -= le32toh(sg->length);
					else {
						/*
						 * update last time
						 * disconnected SG list
						 */
					        /* residue data length  */
						sg->length = htole32(
						    le32toh(sg->length)
						    - xferlen);
						/* residue data pointer */
						sg->address = htole32(
						    le32toh(sg->address)
						    + xferlen);
						srb->sgindex = sgindex;
						break;
					}
				}
				bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap,
				    srb->sgoffset, TRM_SG_SIZE,
				    BUS_DMASYNC_PREWRITE);
			}
		}
	}
	bus_space_write_1(iot, ioh, TRM_DMA_CONTROL, STOPDMAXFER);
}

static void
trm_dataout_phase1(sc, srb, pstat)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int *pstat;
{

	/*
	 * do prepare befor transfer when data out phase
	 */
	trm_dataio_xfer(sc, srb, XFERDATAOUT);
}

static void
trm_datain_phase0(sc, srb, pstat)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int *pstat;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct trm_sg_entry *sg;
	int sgindex;
	u_int32_t xferlen, leftcnt = 0;

	if ((srb->state & SRB_XFERPAD) == 0) {
		if (*pstat & PARITYERROR)
			srb->srbstat |= PARITY_ERROR;

		leftcnt += bus_space_read_4(iot, ioh, TRM_SCSI_XCNT);
		if ((leftcnt == 0) || (*pstat & SCSIXFERCNT_2_ZERO)) {
			while ((bus_space_read_1(iot, ioh, TRM_DMA_STATUS) &
			    DMAXFERCOMP) == 0)
				;

			srb->buflen = 0;
		} else {	/* phase changed */
			/*
			 * parsing the case:
			 * when a transfer not yet complete
			 * but be disconnected by uper layer
			 * if transfer not yet complete
			 * there were some data residue in SCSI FIFO or
			 * SCSI transfer counter not empty
			 */
			if (srb->buflen != leftcnt) {
				/*
				 * data that had transferred length
				 */
				xferlen = srb->buflen - leftcnt;

				/*
				 * next time to be transferred length
				 */
				srb->buflen = leftcnt;

				/*
				 * parsing from last time disconnect sgindex
				 */
				sg = srb->sgentry + srb->sgindex;
				for (sgindex = srb->sgindex;
				     sgindex < srb->sgcnt;
				     sgindex++, sg++) {
					/*
					 * find last time which SG transfer
					 * be disconnect
					 */
					if (xferlen >= le32toh(sg->length))
						xferlen -= le32toh(sg->length);
					else {
						/*
						 * update last time
						 * disconnected SG list
						 */
						/* residue data length  */
						sg->length = htole32(
						    le32toh(sg->length)
						    - xferlen);
						/* residue data pointer */
						sg->address = htole32(
						    le32toh(sg->address)
						    + xferlen);
						srb->sgindex = sgindex;
						break;
					}
				}
				bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap,
				    srb->sgoffset, TRM_SG_SIZE,
				    BUS_DMASYNC_PREWRITE);
			}
		}
	}
}

static void
trm_datain_phase1(sc, srb, pstat)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int *pstat;
{

	/*
	 * do prepare befor transfer when data in phase
	 */
	trm_dataio_xfer(sc, srb, XFERDATAIN);
}

static void
trm_dataio_xfer(sc, srb, iodir)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int iodir;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct trm_dcb *dcb = srb->dcb;

	if (srb->sgindex < srb->sgcnt) {
		if (srb->buflen > 0) {
			/*
			 * load what physical address of Scatter/Gather
			 * list table want to be transfer
			 */
			srb->state = SRB_DATA_XFER;
			bus_space_write_4(iot, ioh, TRM_DMA_XHIGHADDR, 0);
			bus_space_write_4(iot, ioh, TRM_DMA_XLOWADDR,
			    srb->sgaddr +
			    srb->sgindex * sizeof(struct trm_sg_entry));
			/*
			 * load how many bytes in the Scatter/Gather list table
			 */
			bus_space_write_4(iot, ioh, TRM_DMA_XCNT,
			    (srb->sgcnt - srb->sgindex)
			    * sizeof(struct trm_sg_entry));
			/*
			 * load total xfer length (24bits) max value 16Mbyte
			 */
			bus_space_write_4(iot, ioh, TRM_SCSI_XCNT, srb->buflen);
			/* Start DMA transfer */
			bus_space_write_1(iot, ioh, TRM_DMA_COMMAND,
			    iodir | SGXFER);
			bus_space_write_1(iot, ioh, TRM_DMA_CONTROL,
			    STARTDMAXFER);

			/* Start SCSI transfer */
			/* it's important for atn stop */
			bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL,
			    DO_DATALATCH);
									
			/*
			 * SCSI cammand
			 */
			bus_space_write_1(iot, ioh, TRM_SCSI_COMMAND,
			    (iodir == XFERDATAOUT) ?
			    SCMD_DMA_OUT : SCMD_DMA_IN);
		} else {	/* xfer pad */
			if (srb->sgcnt) {
				srb->hastat = H_OVER_UNDER_RUN;
				srb->srbstat |= OVER_RUN;
			}
			bus_space_write_4(iot, ioh, TRM_SCSI_XCNT,
			    (dcb->synctl & WIDE_SYNC) ? 2 : 1);

			if (iodir == XFERDATAOUT)
				bus_space_write_2(iot, ioh, TRM_SCSI_FIFO, 0);
			else
				bus_space_read_2(iot, ioh, TRM_SCSI_FIFO);

			srb->state |= SRB_XFERPAD;
			/* it's important for atn stop */
			bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL,
			    DO_DATALATCH);
			
			/*
			 * SCSI cammand
			 */
			bus_space_write_1(iot, ioh, TRM_SCSI_COMMAND,
			    (iodir == XFERDATAOUT) ?
			    SCMD_FIFO_OUT : SCMD_FIFO_IN);
		}
	}
}

static void
trm_status_phase0(sc, srb, pstat)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int *pstat;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	srb->tastat = bus_space_read_1(iot, ioh, TRM_SCSI_FIFO);
	srb->state = SRB_COMPLETED;
	*pstat = PH_BUS_FREE;	/* .. initial phase */
	/* it's important for atn stop */
	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_DATALATCH);

	/*
	 * SCSI cammand
	 */
	bus_space_write_1(iot, ioh, TRM_SCSI_COMMAND, SCMD_MSGACCEPT);
}

static void
trm_status_phase1(sc, srb, pstat)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int *pstat;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	if (bus_space_read_1(iot, ioh, TRM_DMA_COMMAND) & XFERDATAIN) {
		if ((bus_space_read_1(iot, ioh, TRM_SCSI_FIFOCNT)
		    & SCSI_FIFO_EMPTY) == 0)
			bus_space_write_2(iot, ioh,
			    TRM_SCSI_CONTROL, DO_CLRFIFO);
		if ((bus_space_read_1(iot, ioh, TRM_DMA_FIFOSTATUS)
		    & DMA_FIFO_EMPTY) == 0)
			bus_space_write_1(iot, ioh, TRM_DMA_CONTROL, CLRXFIFO);
	} else {
		if ((bus_space_read_1(iot, ioh, TRM_DMA_FIFOSTATUS)
		    & DMA_FIFO_EMPTY) == 0)
			bus_space_write_1(iot, ioh, TRM_DMA_CONTROL, CLRXFIFO);
		if ((bus_space_read_1(iot, ioh, TRM_SCSI_FIFOCNT)
		    & SCSI_FIFO_EMPTY) == 0)
			bus_space_write_2(iot, ioh,
			    TRM_SCSI_CONTROL, DO_CLRFIFO);
	}
	srb->state = SRB_STATUS;
	/* it's important for atn stop */
	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_DATALATCH);

	/*
	 * SCSI cammand
	 */
	bus_space_write_1(iot, ioh, TRM_SCSI_COMMAND, SCMD_COMP);
}

static void
trm_msgin_phase0(sc, srb, pstat)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int *pstat;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct trm_dcb *dcb = sc->sc_actdcb;
	struct trm_srb *tempsrb;
	int syncxfer, tagid, index;
	u_int8_t msgin_code;

	msgin_code = bus_space_read_1(iot, ioh, TRM_SCSI_FIFO);
	if ((srb->state & SRB_EXTEND_MSGIN) == 0) {
		if (msgin_code == MSG_DISCONNECT) {
			srb->state = SRB_DISCONNECT;
			goto min6;
		} else if (msgin_code == MSG_SAVEDATAPOINTER) {
			goto min6;
		} else if ((msgin_code == MSG_EXTENDED) ||
			   ((msgin_code >= MSG_SIMPLE_Q_TAG) &&
			    (msgin_code <= MSG_ORDERED_Q_TAG))) {
			srb->state |= SRB_EXTEND_MSGIN;
			/* extended message (01h) */
			srb->msgin[0] = msgin_code;

			srb->msgcnt = 1;
			/* extended message length (n) */
			srb->msg = &srb->msgin[1];

			goto min6;
		} else if (msgin_code == MSG_MESSAGE_REJECT) {
			/* Reject message */
			/* do wide nego reject */
			if (dcb->mode & WIDE_NEGO_ENABLE) {
				dcb = srb->dcb;
				dcb->mode |= WIDE_NEGO_DONE;
				dcb->mode &= ~(SYNC_NEGO_DONE | EN_ATN_STOP |
				    WIDE_NEGO_ENABLE);
				srb->state &= ~(SRB_DO_WIDE_NEGO | SRB_MSGIN);
				if ((dcb->mode & SYNC_NEGO_ENABLE) &&
				    (dcb->mode & SYNC_NEGO_DONE) == 0) {
					/* Set ATN, in case ATN was clear */
					srb->state |= SRB_MSGOUT;
					bus_space_write_2(iot, ioh,
					    TRM_SCSI_CONTROL, DO_SETATN);
				} else
					/* Clear ATN */
					bus_space_write_2(iot, ioh,
					    TRM_SCSI_CONTROL, DO_CLRATN);
			} else if (dcb->mode & SYNC_NEGO_ENABLE) {
				/* do sync nego reject */
				bus_space_write_2(iot, ioh,
				    TRM_SCSI_CONTROL, DO_CLRATN);
				if (srb->state & SRB_DO_SYNC_NEGO) {
					dcb = srb->dcb;
					dcb->mode &= ~(SYNC_NEGO_ENABLE |
					    SYNC_NEGO_DONE);
					dcb->synctl = 0;
					dcb->offset = 0;
					goto re_prog;
				}
			}
			goto min6;
		} else if (msgin_code == MSG_IGN_WIDE_RESIDUE) {
			bus_space_write_4(iot, ioh, TRM_SCSI_XCNT, 1);
			bus_space_read_1(iot, ioh, TRM_SCSI_FIFO);
			goto min6;
		} else {
			/*
			 * Restore data pointer message
			 * Save data pointer message
			 * Completion message
			 * NOP message
			 */
			goto min6;
		}
	} else {
		/*
		 * when extend message in:srb->state = SRB_EXTEND_MSGIN
		 * Parsing incomming extented messages
		 */
		*srb->msg = msgin_code;
		srb->msgcnt++;
		srb->msg++;
#ifdef TRM_DEBUG
		printf("srb->msgin[0]=%2x\n", srb->msgin[0]);
		printf("srb->msgin[1]=%2x\n", srb->msgin[1]);
		printf("srb->msgin[2]=%2x\n", srb->msgin[2]);
		printf("srb->msgin[3]=%2x\n", srb->msgin[3]);
		printf("srb->msgin[4]=%2x\n", srb->msgin[4]);
#endif
		if ((srb->msgin[0] >= MSG_SIMPLE_Q_TAG) &&
		    (srb->msgin[0] <= MSG_ORDERED_Q_TAG)) {
			/*
			 * is QUEUE tag message :
			 *
			 * byte 0:
			 *        HEAD    QUEUE TAG (20h)
			 *        ORDERED QUEUE TAG (21h)
			 *        SIMPLE  QUEUE TAG (22h)
			 * byte 1:
			 *        Queue tag (00h - FFh)
			 */
			if (srb->msgcnt == 2) {
				srb->state = 0;
				tagid = srb->msgin[1];
				srb = dcb->gosrb;
				tempsrb = dcb->last_gosrb;
				if (srb) {
					for (;;) {
						if (srb->tagnum != tagid) {
							if (srb == tempsrb)
								goto mingx0;

							srb = srb->next;
						} else
							break;
					}
					if (dcb->flag & ABORT_DEV_) {
						srb->state = SRB_ABORT_SENT;
						srb->msgout[0] = MSG_ABORT;
						trm_msgout_abort(sc, srb);
					}
					if ((srb->state & SRB_DISCONNECT) == 0)
						goto mingx0;

					dcb->actsrb = srb;
					srb->state = SRB_DATA_XFER;
				} else {
			mingx0:
					srb = sc->sc_tempsrb;
					srb->state = SRB_UNEXPECT_RESEL;
					dcb->actsrb = srb;
					srb->msgout[0] = MSG_ABORT_TAG;
					trm_msgout_abort(sc, srb);
				}
			}
		} else if ((srb->msgin[0] == MSG_EXTENDED) &&
			   (srb->msgin[2] == MSG_EXT_WDTR) &&
			   (srb->msgcnt == 4)) {
			/*
			 * is Wide data xfer Extended message :
			 * ======================================
			 * WIDE DATA TRANSFER REQUEST
			 * ======================================
			 * byte 0 :  Extended message (01h)
			 * byte 1 :  Extended message length (02h)
			 * byte 2 :  WIDE DATA TRANSFER code (03h)
			 * byte 3 :  Transfer width exponent
			 */
			dcb = srb->dcb;
			srb->state &= ~(SRB_EXTEND_MSGIN | SRB_DO_WIDE_NEGO);
			if ((srb->msgin[1] != MSG_EXT_WDTR_LEN)) {
				/* Length is wrong, reject it */
				dcb->mode &=
				    ~(WIDE_NEGO_ENABLE | WIDE_NEGO_DONE);
				srb->msgcnt = 1;
				srb->msgin[0] = MSG_MESSAGE_REJECT;
				bus_space_write_2(iot, ioh,
				    TRM_SCSI_CONTROL, DO_SETATN);
				goto min6;
			}
			if (dcb->mode & WIDE_NEGO_ENABLE) {
				/* Do wide negoniation */
				if (srb->msgin[3] > MSG_EXT_WDTR_BUS_32_BIT) {
					/* reject_msg: */
					dcb->mode &= ~(WIDE_NEGO_ENABLE |
					    WIDE_NEGO_DONE);
					srb->msgcnt = 1;
					srb->msgin[0] = MSG_MESSAGE_REJECT;
					bus_space_write_2(iot, ioh,
					    TRM_SCSI_CONTROL, DO_SETATN);
					goto min6;
				}
				if (srb->msgin[3] == MSG_EXT_WDTR_BUS_32_BIT)
					/* do 16 bits */
					srb->msgin[3] = MSG_EXT_WDTR_BUS_16_BIT;
				else {
					if ((dcb->mode & WIDE_NEGO_DONE) == 0) {
						srb->state &=
						    ~(SRB_DO_WIDE_NEGO |
						    SRB_MSGIN);
						dcb->mode |= WIDE_NEGO_DONE;
						dcb->mode &=
						    ~(SYNC_NEGO_DONE |
						    EN_ATN_STOP |
						    WIDE_NEGO_ENABLE);
						if (srb->msgin[3] !=
						    MSG_EXT_WDTR_BUS_8_BIT)
							/* is Wide data xfer */
							dcb->synctl |=
							    WIDE_SYNC;
					}
				}
			} else
				srb->msgin[3] = MSG_EXT_WDTR_BUS_8_BIT;

			srb->state |= SRB_MSGOUT;
			bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL,
			    DO_SETATN);
			goto min6;
		} else if ((srb->msgin[0] == MSG_EXTENDED) &&
			   (srb->msgin[2] == MSG_EXT_SDTR) &&
			   (srb->msgcnt == 5)) {
			/*
			 * is 8bit transfer Extended message :
			 * =================================
			 * SYNCHRONOUS DATA TRANSFER REQUEST
			 * =================================
			 * byte 0 :  Extended message (01h)
			 * byte 1 :  Extended message length (03)
			 * byte 2 :  SYNCHRONOUS DATA TRANSFER code (01h)
			 * byte 3 :  Transfer period factor
			 * byte 4 :  REQ/ACK offset
			 */
			srb->state &= ~(SRB_EXTEND_MSGIN | SRB_DO_SYNC_NEGO);
			if (srb->msgin[1] != MSG_EXT_SDTR_LEN) {
				/* reject_msg: */
				srb->msgcnt = 1;
				srb->msgin[0] = MSG_MESSAGE_REJECT;
				bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL,
				    DO_SETATN);
			} else if (srb->msgin[3] == 0 || srb->msgin[4] == 0) {
				/* set async */
				dcb = srb->dcb;
				/* disable sync & sync nego */
				dcb->mode &=
				    ~(SYNC_NEGO_ENABLE | SYNC_NEGO_DONE);
				dcb->synctl = 0;
				dcb->offset = 0;
				if (((dcb->flag & SHOW_MESSAGE_) == 0) &&
				    (dcb->lun == 0)) {
					printf("%s: target %d, Sync period=0 "
					    "or Sync offset=0 to be "
					    "asynchronous transfer\n",
					    sc->sc_dev.dv_xname, dcb->id);
					dcb->flag |= SHOW_MESSAGE_;
				}
				goto re_prog;
			} else { /* set sync */
				dcb = srb->dcb;
				dcb->mode |= SYNC_NEGO_ENABLE | SYNC_NEGO_DONE;

				/* Transfer period factor */
				dcb->period = srb->msgin[3];

				/* REQ/ACK offset */
				dcb->offset = srb->msgin[4];
				for (index = 0; index < 7; index++)
					if (srb->msgin[3] <=
					    trm_clock_period[index])
						break;

				dcb->synctl |= (index | ALT_SYNC);
				/*
				 * show negotiation message
				 */
				if (((dcb->flag & SHOW_MESSAGE_) == 0) &&
				    (dcb->lun == 0)) {
					syncxfer = 100000 /
					    (trm_clock_period[index] * 4);
					if (dcb->synctl & WIDE_SYNC) {
						printf("%s: target %d, "
						    "16bits Wide transfer\n",
						    sc->sc_dev.dv_xname,
						    dcb->id);
						syncxfer = syncxfer * 2;
					} else
						printf("%s: target %d, "
						    "8bits Narrow transfer\n",
						    sc->sc_dev.dv_xname,
						    dcb->id);

					printf("%s: target %d, "
					    "Sync transfer %d.%01d MB/sec, "
					    "Offset %d\n", sc->sc_dev.dv_xname,
					    dcb->id, syncxfer / 100,
					    syncxfer % 100, dcb->offset);
					dcb->flag |= SHOW_MESSAGE_;
				}
		re_prog:
				/*
				 * program SCSI control register
				 */
				bus_space_write_1(iot, ioh, TRM_SCSI_SYNC,
				    dcb->synctl);
				bus_space_write_1(iot, ioh, TRM_SCSI_OFFSET,
				    dcb->offset);
				trm_set_xfer_rate(sc, srb, dcb);
			}
		}
	}
min6:
	*pstat = PH_BUS_FREE; /* .. initial phase */
	/* it's important for atn stop */
	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_DATALATCH);

	/*
	 * SCSI cammand
	 */
	bus_space_write_1(iot, ioh, TRM_SCSI_COMMAND, SCMD_MSGACCEPT);
}

static void
trm_msgin_phase1(sc, srb, pstat)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int *pstat;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_CLRFIFO);
	bus_space_write_4(iot, ioh, TRM_SCSI_XCNT, 1);
	if ((srb->state & SRB_MSGIN) == 0) {
		srb->state &= SRB_DISCONNECT;
		srb->state |= SRB_MSGIN;
	}
	/* it's important for atn stop */
	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_DATALATCH); 

	/*
	 * SCSI cammand
	 */
	bus_space_write_1(iot, ioh, TRM_SCSI_COMMAND, SCMD_FIFO_IN);
}

static void
trm_nop0(sc, srb, pstat)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int *pstat;
{

}

static void
trm_nop1(sc, srb, pstat)
	struct trm_softc *sc;
	struct trm_srb *srb;
	int *pstat;
{

}

static void
trm_set_xfer_rate(sc, srb, dcb)
	struct trm_softc *sc;
	struct trm_srb *srb;
	struct trm_dcb *dcb;
{
	struct trm_dcb *tempdcb;
	int i;

	/*
	 * set all lun device's (period, offset)
	 */
#ifdef TRM_DEBUG
	printf("trm_set_xfer_rate............\n");
#endif
	if ((dcb->idmsg & 0x07) == 0) {
		if (sc->devscan_end == 0)
			sc->cur_offset = dcb->offset;
		else {
			tempdcb = sc->sc_linkdcb;
			for (i = 0; i < sc->devcnt; i++) {
				/*
				 * different LUN but had same target ID
				 */
				if (tempdcb->id == dcb->id) {
					tempdcb->synctl = dcb->synctl;
					tempdcb->offset = dcb->offset;
					tempdcb->mode = dcb->mode;
				}
				tempdcb = tempdcb->next;
			}
		}
	}
}

static void
trm_disconnect(sc)
	struct trm_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct trm_dcb *dcb;
	struct trm_srb *srb, *psrb;
	int i, s;

#ifdef TRM_DEBUG
	printf("trm_disconnect...............\n");
#endif
	s = splbio();

	dcb = sc->sc_actdcb;
	if (dcb == NULL) {
		DELAY(1000);	/* 1 msec */

		bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL,
		    DO_CLRFIFO | DO_HWRESELECT);
		return;
	}
	srb = dcb->actsrb;
	sc->sc_actdcb = 0;
	srb->phase = PH_BUS_FREE;	/* SCSI bus free Phase */
	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL,
	    DO_CLRFIFO | DO_HWRESELECT);
	DELAY(100);
	if (srb->state & SRB_UNEXPECT_RESEL) {
		srb->state = 0;
		trm_wait_srb(sc);
	} else if (srb->state & SRB_ABORT_SENT) {
		dcb->tagmask = 0;
		dcb->flag &= ~ABORT_DEV_;
		srb = dcb->gosrb;
		for (i = 0; i < dcb->gosrb_cnt; i++) {
			psrb = srb->next;
			srb->next = sc->sc_freesrb;
			sc->sc_freesrb = srb;
			srb = psrb;
		}
		dcb->gosrb_cnt = 0;
		dcb->gosrb = 0;
		trm_wait_srb(sc);
	} else {
		if ((srb->state & (SRB_START_ | SRB_MSGOUT)) ||
		    (srb->state & (SRB_DISCONNECT | SRB_COMPLETED)) == 0) {
				/* Selection time out */
			if (sc->devscan_end) {
				srb->state = SRB_READY;
				trm_rewait_srb(dcb, srb);
			} else {
				srb->tastat = SCSI_SEL_TIMEOUT;
				goto disc1;
			}
		} else if (srb->state & SRB_DISCONNECT) {
			/*
			 * SRB_DISCONNECT
			 */
			trm_wait_srb(sc);
		} else if (srb->state & SRB_COMPLETED) {
	disc1:
			/*
			 * SRB_COMPLETED
			 */
			if (dcb->maxcmd > 1) {
				/* free tag mask */
				dcb->tagmask &= ~(1 << srb->tagnum);
			}
			dcb->actsrb = 0;
			srb->state = SRB_FREE;
			trm_srb_done(sc, dcb, srb);
		}
	}
	splx(s);
}

static void
trm_reselect(sc)
	struct trm_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct trm_dcb *dcb;
	struct trm_srb *srb;
	int id, lun;

#ifdef TRM_DEBUG
	printf("trm_reselect.................\n");
#endif
	dcb = sc->sc_actdcb;
	if (dcb != NULL) {	/* Arbitration lost but Reselection win */
		srb = dcb->actsrb;
		srb->state = SRB_READY;
		trm_rewait_srb(dcb, srb);
	}
	/* Read Reselected Target Id and LUN */
	id = bus_space_read_1(iot, ioh, TRM_SCSI_TARGETID);
	lun = bus_space_read_1(iot, ioh, TRM_SCSI_IDMSG) & 0x07;
	dcb = sc->sc_linkdcb;
	while (id != dcb->id && lun != dcb->lun)
		/* get dcb of the reselect id */
		dcb = dcb->next;

	sc->sc_actdcb = dcb;
	if (dcb->mode & EN_TAG_QUEUING) {
		srb = sc->sc_tempsrb;
		dcb->actsrb = srb;
	} else {
		srb = dcb->actsrb;
		if (srb == NULL || (srb->state & SRB_DISCONNECT) == 0) {
			/*
			 * abort command
			 */
			srb = sc->sc_tempsrb;
			srb->state = SRB_UNEXPECT_RESEL;
			dcb->actsrb = srb;
			srb->msgout[0] = MSG_ABORT;
			trm_msgout_abort(sc, srb);
		} else {
			if (dcb->flag & ABORT_DEV_) {
				srb->state = SRB_ABORT_SENT;
				srb->msgout[0] = MSG_ABORT;
				trm_msgout_abort(sc, srb);
			} else
				srb->state = SRB_DATA_XFER;
		}
	}
	srb->phase = PH_BUS_FREE;	/* SCSI bus free Phase */
	/*
	 * Program HA ID, target ID, period and offset
	 */
	/* target ID */
	bus_space_write_1(iot, ioh, TRM_SCSI_TARGETID, id);

	/* host ID */
	bus_space_write_1(iot, ioh, TRM_SCSI_HOSTID, sc->sc_id);

	/* period */
	bus_space_write_1(iot, ioh, TRM_SCSI_SYNC, dcb->synctl);

	/* offset */
	bus_space_write_1(iot, ioh, TRM_SCSI_OFFSET, dcb->offset);

	/* it's important for atn stop */
	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_DATALATCH);
	DELAY(30);
	/*
	 * SCSI cammand
	 */
	/* to rls the /ACK signal */
	bus_space_write_1(iot, ioh, TRM_SCSI_COMMAND, SCMD_MSGACCEPT);
}

/*
 * Complete execution of a SCSI command
 * Signal completion to the generic SCSI driver
 */
static void
trm_srb_done(sc, dcb, srb)
	struct trm_softc *sc;
	struct trm_dcb *dcb;
	struct trm_srb *srb;
{
	struct scsipi_xfer *xs = srb->xs;
	struct scsipi_inquiry_data *ptr;
	struct trm_dcb *tempdcb;
	int i, j, id, lun, s, tastat;
	u_int8_t bval;

#ifdef	TRM_DEBUG
	printf("trm_srb_done..................\n");
#endif
	if ((xs->xs_control & XS_CTL_POLL) == 0)
		callout_stop(&xs->xs_callout);

	if (xs == NULL)
		return;

	/*
	 * target status
	 */
	tastat = srb->tastat;

	if (srb->flag & AUTO_REQSENSE) {
		/*
		 * status of auto request sense
		 */
		srb->flag &= ~AUTO_REQSENSE;
		srb->hastat = 0;
		srb->tastat = SCSI_CHECK;
		if (tastat == SCSI_CHECK) {
			xs->error = XS_TIMEOUT;
			goto ckc_e;
		}
		memcpy(srb->cmd, srb->tempcmd, sizeof(srb->tempcmd));

		srb->buflen = srb->templen;
		srb->sgentry[0].address = srb->tempsg.address;
		srb->sgentry[0].length = srb->tempsg.length;
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, srb->sgoffset,
		    TRM_SG_SIZE, BUS_DMASYNC_PREWRITE);
		xs->status = SCSI_CHECK;
		goto ckc_e;
	}
	/*
	 * target status
	 */
	if (tastat)
		switch (tastat) {
		case SCSI_CHECK:
			trm_request_sense(sc, dcb, srb);
			return;
		case SCSI_QUEUE_FULL:
			dcb->maxcmd = dcb->gosrb_cnt - 1;
			trm_rewait_srb(dcb, srb);
			srb->hastat = 0;
			srb->tastat = 0;
			goto ckc_e;
		case SCSI_SEL_TIMEOUT:
			srb->hastat = H_SEL_TIMEOUT;
			srb->tastat = 0;
			xs->error = XS_TIMEOUT;
			break;
		case SCSI_BUSY:
			xs->error = XS_BUSY;
			break;
		case SCSI_RESV_CONFLICT:
#ifdef TRM_DEBUG
			printf("%s: target reserved at ", sc->sc_dev.dv_xname);
			printf("%s %d\n", __FILE__, __LINE__);
#endif
			xs->error = XS_BUSY;
			break;
		default:
			srb->hastat = 0;
			if (srb->retry) {
				srb->retry--;
				srb->tastat = 0;
				srb->sgindex = 0;
				if (trm_start_scsi(sc, dcb, srb))
					/*
					 * If trm_start_scsi return 1 :
					 * current interrupt status is
					 * interrupt disreenable.  It's said
					 * that SCSI processor has more one
					 * SRB need to do.
					 */
					trm_rewait_srb(dcb, srb);
				return;
			} else {
#ifdef TRM_DEBUG
				printf("%s: driver stuffup at %s %d\n",
				    sc->sc_dev.dv_xname, __FILE__, __LINE__);
#endif
				xs->error = XS_DRIVER_STUFFUP;
				break;
			}
		}
	else {
		/*
		 * process initiator status......
		 * Adapter (initiator) status
		 */
		if (srb->hastat & H_OVER_UNDER_RUN) {
			srb->tastat = 0;
			/* Illegal length (over/under run) */
			xs->error = XS_DRIVER_STUFFUP;
		} else if (srb->srbstat & PARITY_ERROR) {
#ifdef TRM_DEBUG
			printf("%s: driver stuffup at %s %d\n",
			    sc->sc_dev.dv_xname, __FILE__, __LINE__);
#endif
			/* Driver failed to perform operation */
			xs->error = XS_DRIVER_STUFFUP;
		} else {	/* No error */
			srb->hastat = 0;
			srb->tastat = 0;
			xs->error = XS_NOERROR;
			/* there is no error, (sense is invalid) */
		}
	}
ckc_e:
	id = srb->xs->xs_periph->periph_target;
	lun = srb->xs->xs_periph->periph_lun;
	if (sc->devscan[id][lun]) {
		/*
		 * if SCSI command in "scan devices" duty
		 */
		if (srb->cmd[0] == TEST_UNIT_READY) {
			/* SCSI command phase: test unit ready */
#ifdef TRM_DEBUG
			printf("srb->cmd[0] == TEST_UNIT_READY....\n");
#endif
		} else if (srb->cmd[0] == INQUIRY) {
			/*
			 * SCSI command phase: inquiry scsi device data
			 * (type,capacity,manufacture....
			 */
			if (xs->error == XS_TIMEOUT)
				goto NO_DEV;

			ptr = (struct scsipi_inquiry_data *)xs->data;
			bval = ptr->device & SID_TYPE;
			/*
			 * #define T_NODEVICE 0x1f   Unknown or no device type
			 */
			if (bval == T_NODEVICE) {
		NO_DEV:
#ifdef TRM_DEBUG
				printf("trm_srb_done NO Device: ");
				printf("id= %d ,lun= %d\n", id, lun);
#endif
				s = splbio();
				/*
				 * dcb Q link
				 * move the head of DCB to temdcb
				 */
				tempdcb = sc->sc_linkdcb;

				/*
				 * search current DCB for pass link
				 */
				while (tempdcb->next != dcb)
					tempdcb = tempdcb->next;

				/*
				 * when the current DCB been found
				 * than connect current DCB tail
				 * to the DCB tail that before current DCB
				 */
				tempdcb->next = dcb->next;

				/*
				 * if there was only one DCB ,connect his
				 * tail to his head
				 */
				if (sc->sc_linkdcb == dcb)
					sc->sc_linkdcb = tempdcb->next;

				if (sc->sc_roundcb == dcb)
					sc->sc_roundcb = tempdcb->next;

				/*
				 * if no device than free this device DCB
				 * free( dcb, M_DEVBUF);
				 */
				sc->devcnt--;
#ifdef TRM_DEBUG
				printf("sc->devcnt=%d\n", sc->devcnt);
#endif
				if (sc->devcnt == 0) {
					sc->sc_linkdcb = NULL;
					sc->sc_roundcb = NULL;
				}
				/* no device set scan device flag=0 */
				sc->devscan[id][lun] = 0;
				i = 0;
				j = 0;
				while (i <= sc->maxid) {
					while (j < 8) {
						if (sc->devscan[i][j] == 1) {
							sc->devscan_end = 0;
							splx(s);
							goto exit;
						} else
							sc->devscan_end = 1;

						j++;
					}
					j = 0;
					i++;
				}
				splx(s);
			} else {
				dcb->type = bval;
				if (bval == T_DIRECT || bval == T_OPTICAL) {
					if ((((ptr->version & 0x07) >= 2) ||
					     ((ptr->response_format & 0x0F)
					      == 2)) &&
					    (ptr->flags3 & SID_CmdQue) &&
					    (dcb->tacfg & NTC_DO_TAG_QUEUING) &&
					    (dcb->tacfg & NTC_DO_DISCONNECT)) {
						dcb->maxcmd = sc->maxtag;
						dcb->mode |= EN_TAG_QUEUING;
						dcb->tagmask = 0;
					} else
						dcb->mode |= EN_ATN_STOP;
				}
			}
			/* srb->cmd[0] == INQUIRY */
		}
		/* sc->devscan[id][lun] */
	}
exit:
	if (xs->datalen > 0) {
		bus_dmamap_sync(sc->sc_dmat, srb->dmap, 0,
		    srb->dmap->dm_mapsize, (xs->xs_control & XS_CTL_DATA_IN) ?
		    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, srb->dmap);
	}
	trm_release_srb(sc, dcb, srb);
	trm_wait_srb(sc);
	xs->xs_status |= XS_STS_DONE;
	/* Notify cmd done */
	scsipi_done(xs);
}

static void
trm_release_srb(sc, dcb, srb)
	struct trm_softc *sc;
	struct trm_dcb *dcb;
	struct trm_srb *srb;
{
	struct trm_srb *psrb;
	int s;

	s = splbio();
	if (srb == dcb->gosrb)
		dcb->gosrb = srb->next;
	else {
		psrb = dcb->gosrb;
		while (psrb->next != srb)
			psrb = psrb->next;

		psrb->next = srb->next;
		if (srb == dcb->last_gosrb)
			dcb->last_gosrb = psrb;
	}
	srb->next = sc->sc_freesrb;
	sc->sc_freesrb = srb;
	dcb->gosrb_cnt--;
	splx(s);
	return;
}

static void
trm_doing_srb_done(sc)
	struct trm_softc *sc;
{
	struct trm_dcb *dcb, *pdcb;
	struct trm_srb *psrb, *psrb2;
	struct scsipi_xfer *xs;
	int i;

	dcb = sc->sc_linkdcb;
	if (dcb == NULL)
		return;

	pdcb = dcb;
	do {
		psrb = pdcb->gosrb;
		for (i = 0; i < pdcb->gosrb_cnt; i++) {
			psrb2 = psrb->next;
			xs = psrb->xs;
			xs->error = XS_TIMEOUT;
			/* ReleaseSRB( dcb, srb ); */
			psrb->next = sc->sc_freesrb;
			sc->sc_freesrb = psrb;
			scsipi_done(xs);
			psrb = psrb2;
		}
		pdcb->gosrb_cnt = 0;;
		pdcb->gosrb = NULL;
		pdcb->tagmask = 0;
		pdcb = pdcb->next;
	}
	while (pdcb != dcb);
}

static void
trm_reset_scsi_bus(sc)
	struct trm_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s;

	s = splbio();

	sc->sc_flag |= RESET_DEV;
	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_RSTSCSI);
	while ((bus_space_read_2(iot, ioh, TRM_SCSI_INTSTATUS) &
	    INT_SCSIRESET) == 0)
		;

	splx(s);
}

static void
trm_scsi_reset_detect(sc)
	struct trm_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s;

#ifdef TRM_DEBUG
	printf("trm_scsi_reset_detect...............\n");
#endif
	DELAY(1000000);		/* delay 1 sec */

	s = splbio();

	bus_space_write_1(iot, ioh, TRM_DMA_CONTROL, STOPDMAXFER);
	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_CLRFIFO);

	if (sc->sc_flag & RESET_DEV) {
		sc->sc_flag |= RESET_DONE;
	} else {
		sc->sc_flag |= RESET_DETECT;
		trm_reset_device(sc);
		/* trm_doing_srb_done( sc ); ???? */
		trm_recover_srb(sc);
		sc->sc_actdcb = NULL;
		sc->sc_flag = 0;
		trm_wait_srb(sc);
	}
	splx(s);
}

static void
trm_request_sense(sc, dcb, srb)
	struct trm_softc *sc;
	struct trm_dcb *dcb;
	struct trm_srb *srb;
{
	struct scsipi_xfer *xs = srb->xs;
	struct scsipi_sense *ss;
	int error, lun = xs->xs_periph->periph_lun;

	srb->flag |= AUTO_REQSENSE;
	memcpy(srb->tempcmd, srb->cmd, sizeof(srb->tempcmd));

	srb->templen = srb->buflen;
	srb->tempsg.address = srb->sgentry[0].address;
	srb->tempsg.length = srb->sgentry[0].length;

	/* Status of initiator/target */
	srb->hastat = 0;
	srb->tastat = 0;

	ss = (struct scsipi_sense *)srb->cmd;
	ss->opcode = REQUEST_SENSE;
	ss->byte2 = lun << SCSI_CMD_LUN_SHIFT;
	ss->unused[0] = ss->unused[1] = 0;
	ss->length = sizeof(struct scsipi_sense_data);
	ss->control = 0;

	srb->buflen = sizeof(struct scsipi_sense_data);
	srb->sgcnt = 1;
	srb->sgindex = 0;
	srb->cmdlen = sizeof(struct scsipi_sense);

	if ((error = bus_dmamap_load(sc->sc_dmat, srb->dmap,
	    &xs->sense.scsi_sense, srb->buflen, NULL,
	    BUS_DMA_READ|BUS_DMA_NOWAIT)) != 0) {
		printf("trm_request_sense: can not bus_dmamap_load()\n");
		xs->error = XS_DRIVER_STUFFUP;
		return;
	}
	bus_dmamap_sync(sc->sc_dmat, srb->dmap, 0,
	    srb->buflen, BUS_DMASYNC_PREREAD);

	srb->sgentry[0].address = htole32(srb->dmap->dm_segs[0].ds_addr);
	srb->sgentry[0].length = htole32(sizeof(struct scsipi_sense_data));
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, srb->sgoffset,
	    TRM_SG_SIZE, BUS_DMASYNC_PREWRITE);

	if (trm_start_scsi(sc, dcb, srb))
		/*
		 * If trm_start_scsi return 1: current interrupt status
		 * is interrupt disreenable.  It's said that SCSI processor
		 * has more one SRB need to do.
		 */
		trm_rewait_srb(dcb, srb);
}

static void
trm_msgout_abort(sc, srb)
	struct trm_softc *sc;
	struct trm_srb *srb;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	srb->msgcnt = 1;
	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_SETATN);
	srb->dcb->flag &= ~ABORT_DEV_;
}

/*
 * initialize the internal structures for a given DCB
 */
static void
trm_init_dcb(sc, dcb, xs)
	struct trm_softc *sc;
	struct trm_dcb *dcb;
	struct scsipi_xfer *xs;
{
	struct trm_nvram *eeprom;
	struct trm_dcb *tempdcb;
	int index, id, lun, s;

	id = xs->xs_periph->periph_target;
	lun = xs->xs_periph->periph_lun;

	s = splbio();
	if (sc->sc_linkdcb == 0) {
		sc->sc_linkdcb = dcb;
		/*
		 * RunRobin impersonate the role that let each device had
		 * good proportion about SCSI command proceeding.
		 */
		sc->sc_roundcb = dcb;
		dcb->next = dcb;
	} else {
		tempdcb = sc->sc_linkdcb;
		/* search the last nod of DCB link */
		while (tempdcb->next != sc->sc_linkdcb)
			tempdcb = tempdcb->next;

		/* connect current DCB with last DCB tail */
		tempdcb->next = dcb;
		/* connect current DCB tail to this DCB Q head */
		dcb->next = sc->sc_linkdcb;
	}
	splx(s);

	sc->devcnt++;
	dcb->id = id;
	dcb->lun = lun;
	dcb->waitsrb = NULL;
	dcb->gosrb = NULL;
	dcb->gosrb_cnt = 0;
	dcb->actsrb = NULL;
	dcb->tagmask = 0;
	dcb->maxcmd = 1;
	dcb->flag = 0;

	eeprom = &sc->sc_eeprom;
	dcb->tacfg = eeprom->target[id].config0;
	/*
	 * disconnect enable?
	 */
	dcb->idmsg = MSG_IDENTIFY(lun, dcb->tacfg & NTC_DO_DISCONNECT);
	/*
	 * tag Qing enable?
	 * wide nego, sync nego enable?
	 */
	dcb->synctl = 0;
	dcb->offset = 0;
	index = eeprom->target[id].period & 0x07;
	dcb->period = trm_clock_period[index];
	dcb->mode = 0;
	if ((dcb->tacfg & NTC_DO_WIDE_NEGO) && (sc->sc_config & HCC_WIDE_CARD))
		/* enable wide nego */
		dcb->mode |= WIDE_NEGO_ENABLE;

	if ((dcb->tacfg & NTC_DO_SYNC_NEGO) && (lun == 0 || sc->cur_offset > 0))
		/* enable sync nego */
		dcb->mode |= SYNC_NEGO_ENABLE;
}

static void
trm_link_srb(sc)
	struct trm_softc *sc;
{
	struct trm_srb *srb;
	int i;

	sc->sc_srb = malloc(sizeof(struct trm_srb) * TRM_MAX_SRB,
	    M_DEVBUF, M_NOWAIT);
	if (sc->sc_srb == NULL) {
		printf("%s: can not allocate SRB\n", sc->sc_dev.dv_xname);
		return;
	}
	memset(sc->sc_srb, 0, sizeof(struct trm_srb) * TRM_MAX_SRB);

	for (i = 0, srb = sc->sc_srb; i < TRM_MAX_SRB; i++, srb++) {
		srb->sgentry = sc->sc_sglist + TRM_MAX_SG_ENTRIES * i;
		srb->sgoffset = TRM_SG_SIZE * i;
		srb->sgaddr = sc->sc_dmamap->dm_segs[0].ds_addr + srb->sgoffset;
		/*
		 * map all SRB space to SRB_array
		 */
		if (bus_dmamap_create(sc->sc_dmat,
		    MAXPHYS, TRM_MAX_SG_ENTRIES, MAXPHYS, 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &srb->dmap)) {
			printf("%s: unable to create DMA transfer map...\n",
			    sc->sc_dev.dv_xname);
			free(sc->sc_srb, M_DEVBUF);
			return;
		}
		if (i != TRM_MAX_SRB - 1) {
			/*
			 * link all SRB
			 */
			srb->next = sc->sc_srb + 1;
#ifdef TRM_DEBUG
			printf("srb->next = %8x ", (int) (sc->sc_srb + 1));
#endif
		} else {
			/*
			 * load NULL to NextSRB of the last SRB
			 */
			srb->next = NULL;
		}
#ifdef TRM_DEBUG
		printf("srb = %8x\n", (int) sc->sc_srb);
#endif
	}
	return;
}

/*
 * initialize the internal structures for a given SCSI host
 */
static void
trm_init_sc(sc)
	struct trm_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct trm_nvram *eeprom;
	struct trm_srb tempsrb;
	int i, j;

	eeprom = &sc->sc_eeprom;
	sc->maxid = 7;
	sc->sc_config = HCC_AUTOTERM | HCC_PARITY;
	if (bus_space_read_1(iot, ioh, TRM_GEN_STATUS) & WIDESCSI) {
		sc->sc_config |= HCC_WIDE_CARD;
		sc->maxid = 15;
	}
	if (eeprom->channel_cfg & NAC_POWERON_SCSI_RESET)
		sc->sc_config |= HCC_SCSI_RESET;

	sc->sc_linkdcb = NULL;
	sc->sc_roundcb = NULL;
	sc->sc_actdcb = NULL;
	sc->sc_id = eeprom->scsi_id;
	sc->devcnt = 0;
	sc->maxtag = 2 << eeprom->max_tag;
	sc->sc_flag = 0;
	sc->devscan_end = 0;
	/*
	 * link all device's SRB Q of this adapter
	 */
	trm_link_srb(sc);
	sc->sc_freesrb = sc->sc_srb;
	/*
	 * temp SRB for Q tag used or abord command used
	 */
	sc->sc_tempsrb = &tempsrb;
	/* allocate DCB array for scan device */
	for (i = 0; i <= sc->maxid; i++)
		if (sc->sc_id != i)
			for (j = 0; j < 8; j++) {
				sc->devscan[i][j] = 1;
				sc->devflag[i][j] = 0;
				sc->sc_dcb[i][j] =
				    malloc(sizeof(struct trm_dcb),
				    M_DEVBUF, M_WAITOK);
			}

#ifdef TRM_DEBUG
	printf("sizeof(struct trm_dcb)= %8x\n", sizeof(struct trm_dcb));
	printf("sizeof(struct trm_softc)= %8x\n", sizeof(struct trm_softc));
	printf("sizeof(struct trm_srb)= %8x\n", sizeof(struct trm_srb));
#endif
	sc->sc_adapter.adapt_dev = &sc->sc_dev;
	sc->sc_adapter.adapt_nchannels = 1;
	sc->sc_adapter.adapt_openings = TRM_MAX_SRB;
	sc->sc_adapter.adapt_max_periph = TRM_MAX_SRB;
	sc->sc_adapter.adapt_request = trm_scsipi_request;
	sc->sc_adapter.adapt_minphys = minphys;

	sc->sc_channel.chan_adapter = &sc->sc_adapter;
	sc->sc_channel.chan_bustype = &scsi_bustype;
	sc->sc_channel.chan_channel = 0;
	sc->sc_channel.chan_ntargets = sc->maxid + 1;
	sc->sc_channel.chan_nluns = 8;
	sc->sc_channel.chan_id = sc->sc_id;
}

/*
 * write sc_eeprom 128 bytes to seeprom
 */
static void
trm_eeprom_write_all(sc, eeprom)
	struct trm_softc *sc;
	struct trm_nvram *eeprom;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int8_t *buf = (u_int8_t *)eeprom;
	u_int8_t addr;

	/* Enable SEEPROM */
	bus_space_write_1(iot, ioh, TRM_GEN_CONTROL,
	    bus_space_read_1(iot, ioh, TRM_GEN_CONTROL) | EN_EEPROM);

	/*
	 * Write enable
	 */
	trm_eeprom_write_cmd(sc, 0x04, 0xFF);
	bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, 0);
	trm_wait_30us();

	for (addr = 0; addr < 128; addr++, buf++)
		trm_eeprom_set_data(sc, addr, *buf);

	/*
	 * Write disable
	 */
	trm_eeprom_write_cmd(sc, 0x04, 0x00);
	bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, 0);
	trm_wait_30us();

	/* Disable SEEPROM */
	bus_space_write_1(iot, ioh, TRM_GEN_CONTROL,
	    bus_space_read_1(iot, ioh, TRM_GEN_CONTROL) & ~EN_EEPROM);
}

/*
 * write one byte to seeprom
 */
static void
trm_eeprom_set_data(sc, addr, data)
	struct trm_softc *sc;
	u_int8_t addr;
	u_int8_t data;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int i;
	u_int8_t send;

	/*
	 * Send write command & address
	 */
	trm_eeprom_write_cmd(sc, 0x05, addr);
	/*
	 * Write data
	 */
	for (i = 0; i < 8; i++, data <<= 1) {
		send = NVR_SELECT;
		if (data & 0x80)	/* Start from bit 7 */
			send |= NVR_BITOUT;

		bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, send);
		trm_wait_30us();
		bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, send | NVR_CLOCK);
		trm_wait_30us();
	}
	bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, NVR_SELECT);
	trm_wait_30us();
	/*
	 * Disable chip select
	 */
	bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, 0);
	trm_wait_30us();
	bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, NVR_SELECT);
	trm_wait_30us();
	/*
	 * Wait for write ready
	 */
	for (;;) {
		bus_space_write_1(iot, ioh, TRM_GEN_NVRAM,
		    NVR_SELECT | NVR_CLOCK);
		trm_wait_30us();
		bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, NVR_SELECT);
		trm_wait_30us();
		if (bus_space_read_1(iot, ioh, TRM_GEN_NVRAM) & NVR_BITIN)
			break;
	}
	/*
	 * Disable chip select
	 */
	bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, 0);
}

/*
 * read seeprom 128 bytes to sc_eeprom
 */
static void
trm_eeprom_read_all(sc, eeprom)
	struct trm_softc *sc;
	struct trm_nvram *eeprom;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int8_t *buf = (u_int8_t *)eeprom;
	u_int8_t addr;

	/*
	 * Enable SEEPROM
	 */
	bus_space_write_1(iot, ioh, TRM_GEN_CONTROL,
	    bus_space_read_1(iot, ioh, TRM_GEN_CONTROL) | EN_EEPROM);

	for (addr = 0; addr < 128; addr++, buf++)
		*buf = trm_eeprom_get_data(sc, addr);

	/*
	 * Disable SEEPROM
	 */
	bus_space_write_1(iot, ioh, TRM_GEN_CONTROL,
	    bus_space_read_1(iot, ioh, TRM_GEN_CONTROL) & ~EN_EEPROM);
}

/*
 * read one byte from seeprom
 */
static u_int8_t
trm_eeprom_get_data(sc, addr)
	struct trm_softc *sc;
	u_int8_t addr;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int i;
	u_int8_t read, data = 0;

	/*
	 * Send read command & address
	 */
	trm_eeprom_write_cmd(sc, 0x06, addr);

	for (i = 0; i < 8; i++) { /* Read data */
		bus_space_write_1(iot, ioh, TRM_GEN_NVRAM,
		    NVR_SELECT | NVR_CLOCK);
		trm_wait_30us();
		bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, NVR_SELECT);
		/*
		 * Get data bit while falling edge
		 */
		read = bus_space_read_1(iot, ioh, TRM_GEN_NVRAM);
		data <<= 1;
		if (read & NVR_BITIN)
			data |= 1;

		trm_wait_30us();
	}
	/*
	 * Disable chip select
	 */
	bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, 0);
	return (data);
}

/*
 * write SB and Op Code into seeprom
 */
static void
trm_eeprom_write_cmd(sc, cmd, addr)
	struct trm_softc *sc;
	u_int8_t cmd;
	u_int8_t addr;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int i;
	u_int8_t send;

	/* Program SB+OP code */
	for (i = 0; i < 3; i++, cmd <<= 1) {
		send = NVR_SELECT;
		if (cmd & 0x04)	/* Start from bit 2 */
			send |= NVR_BITOUT;

		bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, send);
		trm_wait_30us();
		bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, send | NVR_CLOCK);
		trm_wait_30us();
	}

	/* Program address */
	for (i = 0; i < 7; i++, addr <<= 1) {
		send = NVR_SELECT;
		if (addr & 0x40)	/* Start from bit 6 */
			send |= NVR_BITOUT;

		bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, send);
		trm_wait_30us();
		bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, send | NVR_CLOCK);
		trm_wait_30us();
	}
	bus_space_write_1(iot, ioh, TRM_GEN_NVRAM, NVR_SELECT);
	trm_wait_30us();
}

/*
 * read seeprom 128 bytes to sc_eeprom and check checksum.
 * If it is wrong, updated with default value.
 */
static void
trm_check_eeprom(sc, eeprom)
	struct trm_softc *sc;
	struct trm_nvram *eeprom;
{
	struct nvram_target *target;
	u_int16_t *ep;
	u_int16_t chksum;
	int i;

#ifdef TRM_DEBUG
	printf("\n trm_check_eeprom......\n");
#endif
	trm_eeprom_read_all(sc, eeprom);
	ep = (u_int16_t *)eeprom;
	chksum = 0;
	for (i = 0; i < 64; i++)
		chksum += le16toh(*ep++);

	if (chksum != TRM_NVRAM_CKSUM) {
#ifdef TRM_DEBUG
		printf("TRM_S1040 EEPROM Check Sum ERROR (load default).\n");
#endif
		/*
		 * Checksum error, load default
		 */
		eeprom->subvendor_id[0] = PCI_VENDOR_TEKRAM2 & 0xFF;
		eeprom->subvendor_id[1] = PCI_VENDOR_TEKRAM2 >> 8;
		eeprom->subsys_id[0] = PCI_PRODUCT_TEKRAM2_DC315 & 0xFF;
		eeprom->subsys_id[1] = PCI_PRODUCT_TEKRAM2_DC315 >> 8;
		eeprom->subclass = 0x00;
		eeprom->vendor_id[0] = PCI_VENDOR_TEKRAM2 & 0xFF;
		eeprom->vendor_id[1] = PCI_VENDOR_TEKRAM2 >> 8;
		eeprom->device_id[0] = PCI_PRODUCT_TEKRAM2_DC315 & 0xFF;
		eeprom->device_id[1] = PCI_PRODUCT_TEKRAM2_DC315 >> 8;
		eeprom->reserved0 = 0x00;

		for (i = 0, target = eeprom->target;
		     i < TRM_MAX_TARGETS;
		     i++, target++) {
			target->config0 = 0x77;
			target->period = 0x00;
			target->config2 = 0x00;
			target->config3 = 0x00;
		}

		eeprom->scsi_id = 7;
		eeprom->channel_cfg = 0x0F;
		eeprom->delay_time = 0;
		eeprom->max_tag = 4;
		eeprom->reserved1 = 0x15;
		eeprom->boot_target = 0;
		eeprom->boot_lun = 0;
		eeprom->reserved2 = 0;
		memset(eeprom->reserved3, 0, sizeof(eeprom->reserved3));

		chksum = 0;
		ep = (u_int16_t *)eeprom;
		for (i = 0; i < 63; i++)
			chksum += le16toh(*ep++);

		chksum = TRM_NVRAM_CKSUM - chksum;
		eeprom->checksum0 = chksum & 0xFF;
		eeprom->checksum1 = chksum >> 8;

		trm_eeprom_write_all(sc, eeprom);
	}
}

/*
 * initialize the SCSI chip ctrl registers
 */
static void
trm_init_adapter(sc)
	struct trm_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int8_t bval;

	/* program configuration 0 */
	bval = PHASELATCH | INITIATOR | BLOCKRST;
	if (sc->sc_config & HCC_PARITY)
		bval |= PARITYCHECK;
	bus_space_write_1(iot, ioh, TRM_SCSI_CONFIG0, bval);

	/* program configuration 1 */
	bus_space_write_1(iot, ioh, TRM_SCSI_CONFIG1,
	    ACTIVE_NEG | ACTIVE_NEGPLUS);

	/* 250ms selection timeout */
	bus_space_write_1(iot, ioh, TRM_SCSI_TIMEOUT, SEL_TIMEOUT);

	/* Mask all the interrupt */
	bus_space_write_1(iot, ioh, TRM_DMA_INTEN, 0);
	bus_space_write_1(iot, ioh, TRM_SCSI_INTEN, 0);

	/* Reset SCSI module */
	bus_space_write_2(iot, ioh, TRM_SCSI_CONTROL, DO_RSTMODULE);

	/* program Host ID */
	bus_space_write_1(iot, ioh, TRM_SCSI_HOSTID, sc->sc_id);

	/* set ansynchronous transfer */
	bus_space_write_1(iot, ioh, TRM_SCSI_OFFSET, 0);

	/* Trun LED control off */
	bus_space_write_2(iot, ioh, TRM_GEN_CONTROL,
	    bus_space_read_2(iot, ioh, TRM_GEN_CONTROL) & ~EN_LED);

	/* DMA config */
	bus_space_write_2(iot, ioh, TRM_DMA_CONFIG,
	    bus_space_read_2(iot, ioh, TRM_DMA_CONFIG) | DMA_ENHANCE);

	/* Clear pending interrupt status */
	bus_space_read_1(iot, ioh, TRM_SCSI_INTSTATUS);

	/* Enable SCSI interrupt */
	bus_space_write_1(iot, ioh, TRM_SCSI_INTEN,
	    EN_SELECT | EN_SELTIMEOUT | EN_DISCONNECT | EN_RESELECTED |
	    EN_SCSIRESET | EN_BUSSERVICE | EN_CMDDONE);
	bus_space_write_1(iot, ioh, TRM_DMA_INTEN, EN_SCSIINTR);
}

/*
 * initialize the internal structures for a given SCSI host
 */
static int
trm_init(sc)
	struct trm_softc *sc;
{
	bus_dma_segment_t seg;
	int error, rseg, all_sgsize;

	/*
	 * EEPROM CHECKSUM
	 */
	trm_check_eeprom(sc, &sc->sc_eeprom);
	/*
	 * MEMORY ALLOCATE FOR ADAPTER CONTROL BLOCK
	 * allocate the space for all SCSI control blocks (SRB) for DMA memory
	 */
	all_sgsize = TRM_MAX_SRB * TRM_SG_SIZE;
	if ((error = bus_dmamem_alloc(sc->sc_dmat, all_sgsize, PAGE_SIZE,
	    0, &seg, 1, &rseg, BUS_DMA_NOWAIT)) != 0) {
		printf("%s: unable to allocate SCSI REQUEST BLOCKS, "
		    "error = %d\n", sc->sc_dev.dv_xname, error);
		return (-1);
	}
	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, rseg,
	    all_sgsize, (caddr_t *) &sc->sc_sglist,
	    BUS_DMA_NOWAIT | BUS_DMA_COHERENT)) != 0) {
		printf("%s: unable to map SCSI REQUEST BLOCKS, "
		    "error = %d\n", sc->sc_dev.dv_xname, error);
		return (-1);
	}
	if ((error = bus_dmamap_create(sc->sc_dmat, all_sgsize, 1,
	    all_sgsize, 0, BUS_DMA_NOWAIT, &sc->sc_dmamap)) != 0) {
		printf("%s: unable to create SRB DMA maps, "
		    "error = %d\n", sc->sc_dev.dv_xname, error);
		return (-1);
	}
	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap,
	    sc->sc_sglist, all_sgsize, NULL, BUS_DMA_NOWAIT)) != 0) {
		printf("%s: unable to load SRB DMA maps, "
		    "error = %d\n", sc->sc_dev.dv_xname, error);
		return (-1);
	}
#ifdef	TRM_DEBUG
	printf("\n\n%s: all_sgsize=%x\n", sc->sc_dev.dv_xname, all_sgsize);
#endif
	memset(sc->sc_sglist, 0, all_sgsize);
	trm_init_sc(sc);
	trm_init_adapter(sc);
	trm_reset(sc);
	return (0);
}

/*
 * attach and init a host adapter
 */
static void
trm_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct pci_attach_args *const pa = aux;
	struct trm_softc *sc = (void *) self;
	bus_space_tag_t iot;	/* bus space tag */
	bus_space_handle_t ioh; /* bus space handle */
	pci_intr_handle_t ih;
	pcireg_t command;
	const char *intrstr;

	/*
	 * These cards do not allow memory mapped accesses
	 * pa_pc:  chipset tag
	 * pa_tag: pci tag
	 */
	command = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	if ((command & (PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MASTER_ENABLE)) !=
	    (PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MASTER_ENABLE)) {
		command |= PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MASTER_ENABLE;
		pci_conf_write(pa->pa_pc, pa->pa_tag,
		    PCI_COMMAND_STATUS_REG, command);
	}
	/*
	 * mask for get correct base address of pci IO port
	 */
	if (pci_mapreg_map(pa, PCI_MAPREG_START, PCI_MAPREG_TYPE_IO, 0,
	    &iot, &ioh, NULL, NULL)) {
		printf("%s: unable to map registers\n", sc->sc_dev.dv_xname);
		return;
	}
	/*
	 * test checksum of eeprom..& initial "ACB" adapter control block...
	 */
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_dmat = pa->pa_dmat;
	if (trm_init(sc)) {
		/*
		 * Error during initialization!
		 */
		printf(": Error during initialization\n");
		return;
	}
	/*
	 * Now try to attach all the sub-devices
	 */
	if (sc->sc_config & HCC_WIDE_CARD)
		printf(": Tekram DC395UW/F (TRM-S1040) Fast40 "
		    "Ultra Wide SCSI Adapter\n");
	else
		printf(": Tekram DC395U, DC315/U (TRM-S1040) Fast20 "
		    "Ultra SCSI Adapter\n");

	printf("%s: Adapter ID=%d, Max tag number=%d, %d SCBs\n",
	    sc->sc_dev.dv_xname, sc->sc_id, sc->maxtag, TRM_MAX_SRB);
	/*
	 * Now tell the generic SCSI layer about our bus.
	 * map and establish interrupt
	 */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);

	if (pci_intr_establish(pa->pa_pc, ih, IPL_BIO, trm_intr, sc) == NULL) {
		printf("%s: couldn't establish interrupt", sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	if (intrstr != NULL)
		printf("%s: interrupting at %s\n",
		    sc->sc_dev.dv_xname, intrstr);
	config_found(&sc->sc_dev, &sc->sc_channel, scsiprint);
}

/*
 * match pci device
 */
static int
trm_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_TEKRAM2)
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_TEKRAM2_DC315:
			return (1);
		}
	return (0);
}
