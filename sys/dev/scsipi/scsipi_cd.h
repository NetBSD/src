/*	$NetBSD: scsipi_cd.h,v 1.9 2005/01/31 23:06:41 reinoud Exp $	*/

/*
 * Written by Julian Elischer (julian@tfs.com)
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
 *	Define two bits always in the same place in byte 2 (flag byte)
 */
#define	CD_RELADDR	0x01
#define	CD_MSF		0x02

/*
 * SCSI and SCSI-like command format
 */

#define	LOAD_UNLOAD	0xa6
struct scsipi_load_unload {
	uint8_t opcode;
	uint8_t unused1[3];
	uint8_t options;
	uint8_t unused2[3];
	uint8_t slot;
	uint8_t unused3[3];
} __attribute__((packed));

#define PAUSE			0x4b	/* cdrom pause in 'play audio' mode */
struct scsipi_pause {
	uint8_t opcode;
	uint8_t byte2;
	uint8_t unused[6];
	uint8_t resume;
	uint8_t control;
} __attribute__((packed));
#define	PA_PAUSE	0x00
#define PA_RESUME	0x01

#define PLAY_MSF		0x47	/* cdrom play Min,Sec,Frames mode */
struct scsipi_play_msf {
	uint8_t opcode;
	uint8_t byte2;
	uint8_t unused;
	uint8_t start_m;
	uint8_t start_s;
	uint8_t start_f;
	uint8_t end_m;
	uint8_t end_s;
	uint8_t end_f;
	uint8_t control;
} __attribute__((packed));

#define PLAY			0x45	/* cdrom play  'play audio' mode */
struct scsipi_play {
	uint8_t opcode;
	uint8_t byte2;
	uint8_t blk_addr[4];
	uint8_t unused;
	uint8_t xfer_len[2];
	uint8_t control;
} __attribute__((packed));

#define READ_HEADER		0x44	/* cdrom read header */
struct scsipi_read_header {
	uint8_t opcode;
	uint8_t byte2;
	uint8_t blk_addr[4];
	uint8_t unused;
	uint8_t data_len[2];
	uint8_t control;
} __attribute__((packed));

#define READ_SUBCHANNEL		0x42	/* cdrom read Subchannel */
struct scsipi_read_subchannel {
	uint8_t opcode;
	uint8_t byte2;
	uint8_t byte3;
#define	SRS_SUBQ	0x40
	uint8_t subchan_format;
	uint8_t unused[2];
	uint8_t track;
	uint8_t data_len[2];
	uint8_t control;
} __attribute__((packed));

#define READ_TOC		0x43	/* cdrom read TOC */
struct scsipi_read_toc {
	uint8_t opcode;
	uint8_t addr_mode;
	uint8_t resp_format;
	uint8_t unused[3];
	uint8_t from_track;
	uint8_t data_len[2];
	uint8_t control;
} __attribute__((packed));

#define READ_CD_CAPACITY	0x25	/* slightly different from disk */
struct scsipi_read_cd_capacity {
	uint8_t opcode;
	uint8_t byte2;
	uint8_t addr[4];
	uint8_t unused[3];
	uint8_t control;
} __attribute__((packed));

struct scsipi_read_cd_cap_data {
	uint8_t addr[4];
	uint8_t length[4];
} __attribute__((packed));

/* mod pages common to scsi and atapi */
struct cd_audio_page {
	uint8_t pg_code;
#define		AUDIO_PAGE	0x0e
	uint8_t pg_length;
	uint8_t flags;
#define		CD_PA_SOTC	0x02
#define		CD_PA_IMMED	0x04
	uint8_t unused[2];
	uint8_t format_lba; /* valid only for SCSI CDs */
#define		CD_PA_FORMAT_LBA 0x0F
#define		CD_PA_APR_VALID	0x80
	uint8_t lb_per_sec[2];
	struct port_control {
		uint8_t channels;
#define	CHANNEL 0x0F
#define	CHANNEL_0 1
#define	CHANNEL_1 2
#define	CHANNEL_2 4
#define	CHANNEL_3 8
#define		LEFT_CHANNEL	CHANNEL_0
#define		RIGHT_CHANNEL	CHANNEL_1
#define		MUTE_CHANNEL	0x0
#define		BOTH_CHANNEL	LEFT_CHANNEL | RIGHT_CHANNEL
		uint8_t volume;
	} port[4];
#define	LEFT_PORT	0
#define	RIGHT_PORT	1
};
