/*	$NetBSD: siivar.h,v 1.8 2001/08/26 11:47:23 simonb Exp $	*/

#ifndef _SIIVAR_H
#define _SIIVAR_H

/*
 * This structure contains information that a SCSI interface controller
 * needs to execute a SCSI command.
 */
typedef struct ScsiCmd {
	int	unit;		/* unit number passed to device done routine */
	int	flags;		/* control flags for this command (see below) */
	int	buflen;		/* length of the data buffer in bytes */
	char	*buf;		/* pointer to data buffer for this command */
	int	cmdlen;		/* length of data in cmdbuf */
	u_char	*cmd;		/* buffer for the SCSI command */
	int	error;		/* compatibility hack for new scsi */
	int	lun;		/* LUN for MI SCSI */
	struct callout timo_ch;	/* timeout callout handle */
} ScsiCmd;

typedef struct scsi_state {
	int	statusByte;	/* status byte returned during STATUS_PHASE */
	int	dmaDataPhase;	/* which data phase to expect */
	int	dmaCurPhase;	/* SCSI phase if DMA is in progress */
	int	dmaPrevPhase;	/* SCSI phase of DMA suspended by disconnect */
	u_short	*dmaAddr[2];	/* DMA buffer memory address */
	int	dmaBufIndex;	/* which of the above is currently in use */
	int	dmalen;		/* amount to transfer in this chunk */
	int	cmdlen;		/* total remaining amount of cmd to transfer */
	u_char	*cmd;		/* current pointer within scsicmd->cmd */
	int	buflen;		/* total remaining amount of data to transfer */
	char	*buf;		/* current pointer within scsicmd->buf */
	u_short	flags;		/* see below */
	u_short	prevComm;	/* command reg before disconnect */
	u_short	dmaCtrl;	/* DMA control register if disconnect */
	u_short	dmaAddrL;	/* DMA address register if disconnect */
	u_short	dmaAddrH;	/* DMA address register if disconnect */
	u_short	dmaCnt;		/* DMA count if disconnect */
	u_short	dmaByte;	/* DMA byte if disconnect on odd boundary */
	u_short	dmaReqAck;	/* DMA synchronous xfer offset or 0 if async */
} State;

/* state flags */
#define FIRST_DMA	0x01	/* true if no data DMA started yet */
#define PARITY_ERR	0x02	/* true if parity error seen */

#define SII_NCMD	8
struct siisoftc {
	struct device sc_dev;		/* us as a device */
	struct scsipi_channel sc_channel;
	struct scsipi_adapter sc_adapter;	/* scsipi adapter glue */
	ScsiCmd sc_cmd_fake[SII_NCMD];		/* XXX - hack!!! */
	struct scsipi_xfer *sc_xs[SII_NCMD];	/* XXX - hack!!! */
	void *sc_buf;			/* DMA buffer (may be special mem) */
	SIIRegs	*sc_regs;		/* HW address of SII controller chip */
	int	sc_flags;
	int	sc_target;		/* target SCSI ID if connected */
	ScsiCmd	*sc_cmd[SII_NCMD];	/* active command indexed by ID */
 	void  (*sii_copytobuf) __P((u_short *src, volatile u_short *dst, int ln));
	void (*sii_copyfrombuf) __P((volatile u_short *src, char *dst, int len));

	State	sc_st[SII_NCMD];	/* state info for each active command */
};

int	siiintr __P((void *sc));

/* Machine-indepedent back-end attach entry point */

void	sii_scsi_request __P((struct scsipi_channel *,
				scsipi_adapter_req_t, void *));
void	siiattach __P((struct siisoftc *));

#endif	/* _SIIVAR_H */
