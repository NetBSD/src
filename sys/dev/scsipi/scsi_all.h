/*	$NetBSD: scsi_all.h,v 1.21 2005/01/31 23:06:41 reinoud Exp $	*/

/*
 * SCSI-specific interface description.
 */

/*
 * Largely written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * Ported to run under 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 */

#ifndef _DEV_SCSIPI_SCSI_ALL_H_
#define _DEV_SCSIPI_SCSI_ALL_H_

/*
 * Define dome bits that are in ALL (or a lot of) scsi commands
 */
#define	SCSI_CTL_LINK		0x01
#define	SCSI_CTL_FLAG		0x02
#define	SCSI_CTL_VENDOR		0xC0


/*
 * Some old SCSI devices need the LUN to be set in the top 3 bits of the
 * second byte of the CDB.
 */
#define	SCSI_CMD_LUN_MASK	0xe0
#define	SCSI_CMD_LUN_SHIFT	5

/* XXX Is this a command ? What's its opcode ? */
struct scsi_send_diag {
	uint8_t opcode;
	uint8_t byte2;
#define	SSD_UOL		0x01
#define	SSD_DOL		0x02
#define	SSD_SELFTEST	0x04
#define	SSD_PF		0x10
	uint8_t unused[1];
	uint8_t paramlen[2];
	uint8_t control;
};

#define	SCSI_RESERVE      		0x16
struct scsi_reserve {
	uint8_t opcode;
	uint8_t byte2;
	uint8_t unused[2];
	uint8_t length;
	uint8_t control;
};

#define	SCSI_RELEASE      		0x17
struct scsi_release {
	uint8_t opcode;
	uint8_t byte2;
	uint8_t unused[2];
	uint8_t length;
	uint8_t control;
};

#define	SCSI_CHANGE_DEFINITION	0x40
struct scsi_changedef {
	uint8_t opcode;
	uint8_t byte2;
	uint8_t unused1;
	uint8_t how;
	uint8_t unused[4];
	uint8_t datalen;
	uint8_t control;
};
#define	SC_SCSI_1 0x01
#define	SC_SCSI_2 0x03

/* block descriptor, for mode sense/mode select */
struct scsi_blk_desc {
	uint8_t density;
	uint8_t nblocks[3];
	uint8_t reserved;
	uint8_t blklen[3];
};

/*
 * Status Byte
 */
#define	SCSI_OK			0x00
#define	SCSI_CHECK		0x02
#define	SCSI_BUSY		0x08
#define	SCSI_INTERM		0x10
#define	SCSI_RESV_CONFLICT	0x18
#define	SCSI_TERMINATED		0x22
#define	SCSI_QUEUE_FULL		0x28
#define	SCSI_ACA_ACTIVE		0x30

#endif /* _DEV_SCSIPI_SCSI_ALL_H_ */
