/*	$NetBSD: scsi_all.h,v 1.14.14.3 2001/04/21 17:49:48 bouyer Exp $	*/

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

/*
 * SCSI command format
 */

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

/*
 * XXX
 * Actually some SCSI driver expects this structure to be 12 bytes, so
 * don't change it unless you really know what you are doing
 */

struct scsi_generic {
	u_int8_t opcode;
	u_int8_t bytes[11];
};

/* XXX Is this a command ? What's its opcode ? */
struct scsi_send_diag {
	u_int8_t opcode;
	u_int8_t byte2;
#define	SSD_UOL		0x01
#define	SSD_DOL		0x02
#define	SSD_SELFTEST	0x04
#define	SSD_PF		0x10
	u_int8_t unused[1];
	u_int8_t paramlen[2];
	u_int8_t control;
};

#define	SCSI_MODE_SENSE		0x1a
struct scsi_mode_sense {
	u_int8_t opcode;
	u_int8_t byte2;
#define	SMS_DBD				0x08
	u_int8_t page;
#define	SMS_PAGE_CODE 			0x3F
#define	SMS_PAGE_CTRL 			0xC0
#define	SMS_PAGE_CTRL_CURRENT 		0x00
#define	SMS_PAGE_CTRL_CHANGEABLE 	0x40
#define	SMS_PAGE_CTRL_DEFAULT 		0x80
#define	SMS_PAGE_CTRL_SAVED 		0xC0
	u_int8_t unused;
	u_int8_t length;
	u_int8_t control;
};

#define	SCSI_MODE_SENSE_BIG		0x5A
struct scsi_mode_sense_big {
	u_int8_t opcode;
	u_int8_t byte2;		/* same bits as small version */
	u_int8_t page; 		/* same bits as small version */
	u_int8_t unused[4];
	u_int8_t length[2];
	u_int8_t control;
};

#define	SCSI_MODE_SELECT		0x15
struct scsi_mode_select {
	u_int8_t opcode;
	u_int8_t byte2;
#define	SMS_SP	0x01
#define	SMS_PF	0x10
	u_int8_t unused[2];
	u_int8_t length;
	u_int8_t control;
};

#define	SCSI_MODE_SELECT_BIG		0x55
struct scsi_mode_select_big {
	u_int8_t opcode;
	u_int8_t byte2;		/* same bits as small version */
	u_int8_t unused[5];
	u_int8_t length[2];
	u_int8_t control;
};

#define	SCSI_RESERVE      		0x16
struct scsi_reserve {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused[2];
	u_int8_t length;
	u_int8_t control;
};

#define	SCSI_RELEASE      		0x17
struct scsi_release {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused[2];
	u_int8_t length;
	u_int8_t control;
};

#define	SCSI_CHANGE_DEFINITION	0x40
struct scsi_changedef {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused1;
	u_int8_t how;
	u_int8_t unused[4];
	u_int8_t datalen;
	u_int8_t control;
};
#define	SC_SCSI_1 0x01
#define	SC_SCSI_2 0x03

struct scsi_blk_desc {
	u_int8_t density;
	u_int8_t nblocks[3];
	u_int8_t reserved;
	u_int8_t blklen[3];
};

struct scsi_mode_header {
	u_int8_t data_length;	/* Sense data length */
	u_int8_t medium_type;
	u_int8_t dev_spec;
	u_int8_t blk_desc_len;
};

struct scsi_mode_header_big {
	u_int8_t data_length[2];	/* Sense data length */
	u_int8_t medium_type;
	u_int8_t dev_spec;
	u_int8_t unused[2];
	u_int8_t blk_desc_len[2];
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
