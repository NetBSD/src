/*	$NetBSD: scsi_cd.h,v 1.8 1997/10/01 01:18:58 enami Exp $	*/

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
#ifndef	_SCSI_SCSI_CD_H
#define _SCSI_SCSI_CD_H 1

/*
 * SCSI specific command format
 */

#define PLAY_TRACK		0x48	/* cdrom play track/index mode */
struct scsi_play_track {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused[2];
	u_int8_t start_track;
	u_int8_t start_index;
	u_int8_t unused1;
	u_int8_t end_track;
	u_int8_t end_index;
	u_int8_t control;
};

/* sense pages */
#define SCSI_CD_PAGE_CODE    0x3F
#define SCSI_AUDIO_PAGE  0x0e
#define SCSI_CD_PAGE_PS  0x80

union scsi_cd_pages {
	struct cd_audio_page audio;
};

struct scsi_cd_mode_data {
	struct scsi_mode_header header;
	struct scsi_blk_desc blk_desc;
	union scsi_cd_pages page;
};
#endif /*_SCSI_SCSI_CD_H*/

