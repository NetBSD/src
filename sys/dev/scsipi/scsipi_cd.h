/*	$NetBSD: scsipi_cd.h,v 1.1.2.2 1997/07/01 23:32:48 thorpej Exp $	*/

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
#ifndef	_SCSI_PI_CD_H
#define _SCSI_PI_CD_H 1

/*
 *	Define two bits always in the same place in byte 2 (flag byte)
 */
#define	CD_RELADDR	0x01
#define	CD_MSF		0x02

/*
 * SCSI and SCSI-like command format
 */

#define PAUSE			0x4b	/* cdrom pause in 'play audio' mode */
struct scsipi_pause {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused[6];
	u_int8_t resume;
	u_int8_t control;
};
#define	PA_PAUSE	0x00
#define PA_RESUME	0x01

#define PLAY_MSF		0x47	/* cdrom play Min,Sec,Frames mode */
struct scsipi_play_msf {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused;
	u_int8_t start_m;
	u_int8_t start_s;
	u_int8_t start_f;
	u_int8_t end_m;
	u_int8_t end_s;
	u_int8_t end_f;
	u_int8_t control;
};

#define PLAY			0x45	/* cdrom play  'play audio' mode */
struct scsipi_play {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t blk_addr[4];
	u_int8_t unused;
	u_int8_t xfer_len[2];
	u_int8_t control;
};

#define READ_HEADER		0x44	/* cdrom read header */
struct scsipi_read_header {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t blk_addr[4];
	u_int8_t unused;
	u_int8_t data_len[2];
	u_int8_t control;
};

#define READ_SUBCHANNEL		0x42	/* cdrom read Subchannel */
struct scsipi_read_subchannel {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t byte3;
#define	SRS_SUBQ	0x40
	u_int8_t subchan_format;
	u_int8_t unused[2];
	u_int8_t track;
	u_int8_t data_len[2];
	u_int8_t control;
};

#define READ_TOC		0x43	/* cdrom read TOC */
struct scsipi_read_toc {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused[4];
	u_int8_t from_track;
	u_int8_t data_len[2];
	u_int8_t control;
};
;

#define READ_CD_CAPACITY	0x25	/* slightly different from disk */
struct scsipi_read_cd_capacity {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t addr[4];
	u_int8_t unused[3];
	u_int8_t control;
};

struct scsipi_read_cd_cap_data {
	u_int8_t addr[4];
	u_int8_t length[4];
};

/* mod pages common to scsi and atapi */
struct cd_audio_page {
	u_int8_t page_code;
#define	CD_PAGE_CODE	0x3F
#define	AUDIO_PAGE	0x0e
#define	CD_PAGE_PS	0x80
	u_int8_t param_len;
	u_int8_t flags;
#define		CD_PA_SOTC	0x02
#define		CD_PA_IMMED	0x04
	u_int8_t unused[2];
	u_int8_t format_lba; /* valid only for SCSI CDs */
#define		CD_PA_FORMAT_LBA	0x0F
#define		CD_PA_APR_VALID	0x80
	u_int8_t lb_per_sec[2];
	struct	port_control {
		u_int8_t channels; 
#define	CHANNEL 0x0F
#define	CHANNEL_0 1
#define	CHANNEL_1 2
#define	CHANNEL_2 4
#define	CHANNEL_3 8
#define	LEFT_CHANNEL	CHANNEL_0
#define	RIGHT_CHANNEL	CHANNEL_1
#define MUTE_CHANNEL  0x0
#define BOTH_CHANNEL  LEFT_CHANNEL | RIGHT_CHANNEL
		u_int8_t volume;
	} port[4];
#define	LEFT_PORT	0
#define	RIGHT_PORT	1
};


#endif /* _SCSI_PI_CD_H */
