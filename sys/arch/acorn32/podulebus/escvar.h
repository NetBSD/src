/* $NetBSD: escvar.h,v 1.1 2001/10/05 22:27:55 reinoud Exp $ */

/*
 * Copyright (c) 1995 Daniel Widenfalk
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
 *      This product includes software developed by Daniel Widenfalk
 *      for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef _ESCVAR_H_
#define _ESCVAR_H_

#ifndef _ESCREG_H_
#include <acorn32/podulebus/escreg.h>
#endif

/*
 * MAXCHAIN is the anticipated maximum number of chain blocks needed. This
 * assumes that we are NEVER requested to transfer more than MAXPHYS bytes.
 */
#define MAXCHAIN	(MAXPHYS/NBPG+2)

/*
 * Maximum number of requests standing by. Could be anything, but I think 9
 * looks nice :-) NOTE: This does NOT include requests already started!
 */
#define MAXPENDING	9	/* 7 IDs + 2 extra */

/*
 * DMA chain block. If flg == ESC_CHAIN_PRG or flg == ESC_CHAIN_BUMP then
 * ptr is a VIRTUAL adress. If flg == ESC_CHAIN_DMA then ptr is a PHYSICAL
 * adress.
 */
struct	esc_dma_chain {
	vm_offset_t	ptr;
	u_short		len;
	short		flg;
};
#define ESC_CHAIN_DMA	0x00
#define ESC_CHAIN_BUMP	0x01
#define ESC_CHAIN_PRG	0x02


/*
 * This struct contains the necessary info for a pending request. Pointer to
 * a scsipi_xfer struct.
 */
struct	esc_pending {
	TAILQ_ENTRY(esc_pending) link;
	struct scsipi_xfer	 *xs;
};

/*
 * nexus contains all active data for one SCSI unit. Parts of the info in this
 * struct survives between scsi commands.
 */
struct nexus {
	struct	scsipi_xfer 	*xs;		/* Pointer to request */

	u_char			 ID;		/* ID message to be sent */
	u_char			 clen;		/* scsi command length + */
	u_char			 cbuf[14];	/* the actual command bytes */

	struct esc_dma_chain	 dma[MAXCHAIN];	/* DMA chain blocks */
	short			 max_link;	/* Maximum used of above */
	short			 cur_link;	/* Currently handled block */

	u_char			*buf;		/* Virtual adress of data */
	int			 len;		/* Bytes left to transfer */

	vm_offset_t		 dma_buf;	/* Current DMA adress */
	int			 dma_len;	/* Current DMA length */

	vm_offset_t		 dma_blk_ptr;	/* Current chain adress */
	int			 dma_blk_len;	/* Current chain length */
	u_char			 dma_blk_flg;	/* Current chain flags */

	u_char			 state;		/* Nexus state, see below */
	u_short			 flags;		/* Nexus flags, see below */

	short			 period;	/* Sync period to request */
	u_char			 offset;	/* Sync offset to request */

	u_char			 syncper;	/* FAS216 variable storage */
	u_char			 syncoff;	/* FAS216 variable storage */
	u_char			 config3;	/* FAS216 variable storage */

	u_char			 lun_unit;	/* (Lun<<4) | Unit of nexus */
	u_char			 status;	/* Status byte from unit*/

};

/* SCSI nexus_states */
#define ESC_NS_IDLE		0	/* Nexus idle */
#define ESC_NS_SELECTED	1	/* Last command was a SELECT command */
#define ESC_NS_DATA_IN		2	/* Last command was a TRANSFER_INFO */
					/* command during a data in phase */
#define ESC_NS_DATA_OUT	3	/* Last command was a TRANSFER_INFO */
					/* command during a data out phase */
#define ESC_NS_STATUS		4	/* We have send a COMMAND_COMPLETE */
					/* command and are awaiting status */
#define ESC_NS_MSG_IN		5	/* Last phase was MESSAGE IN */
#define ESC_NS_MSG_OUT		6	/* Last phase was MESSAGE OUT */
#define ESC_NS_SVC		7	/* We have sent the command */
#define ESC_NS_DISCONNECTING	8	/* We have received a disconnect msg */
#define ESC_NS_DISCONNECTED	9	/* We are disconnected */
#define ESC_NS_RESELECTED	10	/* We was reselected */
#define ESC_NS_DONE		11	/* Done. Prephsase to FINISHED */
#define ESC_NS_FINISHED		12	/* Realy done. Call scsi_done */
#define ESC_NS_RESET		13	/* We are reseting this unit */

/* SCSI nexus flags */
#define ESC_NF_UNIT_BUSY	0x0001	/* Unit is not available */

#define ESC_NF_SELECT_ME	0x0002	/* Nexus is set up, waiting for bus */

#define ESC_NF_HAS_MSG		0x0010	/* We have received a complete msg */

#define ESC_NF_DO_SDTR		0x0020	/* We should send a SDTR */
#define ESC_NF_SDTR_SENT	0x0040	/* We have sent a SDTR */
#define ESC_NF_SYNC_TESTED	0x0080	/* We have negotiated sync */

#define ESC_NF_RESET		0x0100	/* Reset this nexus */
#define ESC_NF_IMMEDIATE	0x0200	/* We are operating from escicmd */

#define ESC_NF_RETRY_SELECT	0x1000	/* Selection needs retrying */

#define ESC_NF_DEBUG		0x8000	/* As it says: DEBUG */

struct	esc_softc {
	struct	device		 sc_dev;	/* System required struct */
	struct	scsipi_channel	 sc_channel;	/* For sub devices */
	struct	scsipi_adapter	 sc_adapter;
	irqhandler_t		 sc_ih;		/* Interrupt chain struct */

	TAILQ_HEAD(,esc_pending) sc_xs_pending;
	TAILQ_HEAD(,esc_pending) sc_xs_free;
	struct	esc_pending 	 sc_xs_store[MAXPENDING];

	esc_regmap_p		 sc_esc;	/* FAS216 Address */
	void			*sc_spec;	/* Board-specific data */

	u_char			*sc_bump_va;	/* Bumpbuf virtual adr */
	vm_offset_t		 sc_bump_pa;	/* Bumpbuf physical adr */
	int			 sc_bump_sz;	/* Bumpbuf size */

/* Configuration registers, must be set BEFORE escinitialize */
	u_char			 sc_clock_freq;
	u_short			 sc_timeout;
	u_char			 sc_host_id;
	u_char			 sc_config_flags;

/* Generic DMA functions */
	int		       (*sc_setup_dma)();
	int		       (*sc_build_dma_chain)();
	int		       (*sc_need_bump)();

/* Generic Led data */
	int			 sc_led_status;
	void		       (*sc_led)();

/* Nexus list */
	struct nexus		 sc_nexus[8];
	struct nexus		*sc_cur_nexus;
	struct nexus		*sc_sel_nexus;

/* Current transfer data */
	u_char			*sc_buf;	/* va */
	int			 sc_len;

	vm_offset_t		 sc_dma_buf;	/* pa */
	int			 sc_dma_len;
	vm_offset_t		 sc_dma_blk_ptr;
	int			 sc_dma_blk_len;
	short			 sc_dma_blk_flg;

	struct esc_dma_chain	*sc_chain;	/* Current DMA chain */
	short			 sc_max_link;
	short			 sc_cur_link;

/* Interrupt registers */
	u_char			 sc_status;
	u_char			 sc_interrupt;
	u_char			 sc_resel[2];

	u_char			 sc_units_disconnected;

/* Storage for FAS216 config registers (current values) */
	u_char			 sc_config1;
	u_char			 sc_config2;
	u_char			 sc_config3;
	u_char			 sc_config4;
	u_char			 sc_clock_conv_fact;
	u_char			 sc_timeout_val;
	u_char			 sc_clock_period;

	u_char			 sc_msg_in[7];
	u_char			 sc_msg_in_len;

	u_char			 sc_msg_out[7];
	u_char			 sc_msg_out_len;

	u_char			 sc_unit;
	u_char			 sc_lun;
	u_char			 sc_flags;
};

#define ESC_DMA_READ	0
#define ESC_DMA_WRITE	1
#define ESC_DMA_CLEAR	2

/* sc_flags */
#define ESC_ACTIVE	 0x01
#define ESC_DONT_WAIT	 0x02

/* SCSI Selection modes */
#define ESC_SELECT	0x00	/* Normal selection: No sync, no resel */
#define ESC_SELECT_R	0x01	/* Reselection allowed */
#define ESC_SELECT_S	0x02	/* Synchronous transfer allowed */
#define ESC_SELECT_I	0x04	/* Selection for escicmd */
#define ESC_SELECT_K	0x08	/* Send a BUS DEVICE RESET message (Kill) */

/* Nice abbreviations of the above */
#define ESC_SELECT_RS	(ESC_SELECT_R|ESC_SELECT_S)
#define ESC_SELECT_RI	(ESC_SELECT_R|ESC_SELECT_I)
#define ESC_SELECT_SI	(ESC_SELECT_S|ESC_SELECT_I)
#define ESC_SELECT_RSI	(ESC_SELECT_R|ESC_SELECT_S|ESC_SELECT_I)

/* sc_config_flags */
#define ESC_NO_SYNCH	 0x01	/* Disable synchronous transfer */
#define ESC_NO_DMA	 0x02	/* Do not use DMA! EVER! */
#define ESC_NO_RESELECT 0x04	/* Do not allow relesection */
#define ESC_SLOW_CABLE	 0x08	/* Cable is "unsafe" for fast scsi-2 */
#define ESC_SLOW_START	 0x10	/* There are slow starters on the bus */

void	escinitialize __P((struct esc_softc *sc));
void	esc_minphys   __P((struct buf *bp));
void	esc_scsi_request __P((struct scsipi_channel *,
				scsipi_adapter_req_t, void *));
void	escintr       __P((struct esc_softc *dev));

#endif /* _ESCVAR_H_ */
