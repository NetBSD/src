/*	$NetBSD: scsipi_all.h,v 1.5 1997/10/03 02:04:19 thorpej Exp $	*/

/*
 * SCSI and SCSI-like general interface description
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

#ifndef	_SCSI_PI_ALL_H
#define	_SCSI_PI_ALL_H 1

/*
 * SCSI-like command format and opcode
 */

#define	TEST_UNIT_READY		0x00
struct scsipi_test_unit_ready {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused[3];
	u_int8_t control;
};

#define	REQUEST_SENSE		0x03
struct scsipi_sense {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused[2];
	u_int8_t length;
	u_int8_t control;
};

#define	INQUIRY			0x12
struct scsipi_inquiry {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused[2];
	u_int8_t length;
	u_int8_t control;
};

#define	PREVENT_ALLOW		0x1e
struct scsipi_prevent {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused[2];
	u_int8_t how;
	u_int8_t control;
};
#define	PR_PREVENT 0x01
#define	PR_ALLOW   0x00

/*
 * inquiry and sense data format
 */

struct scsipi_sense_data {
/* 1*/	u_int8_t error_code;
#define	SSD_ERRCODE	0x7F
#define	SSD_ERRCODE_VALID 0x80
/* 2*/	u_int8_t segment;
/* 3*/	u_int8_t flags;
#define	SSD_KEY		0x0F
#define	SSD_ILI		0x20
#define	SSD_EOM		0x40
#define	SSD_FILEMARK	0x80
/* 7*/	u_int8_t info[4];
/* 8*/	u_int8_t extra_len;
/*12*/	u_int8_t cmd_spec_info[4];
/*13*/	u_int8_t add_sense_code;
/*14*/	u_int8_t add_sense_code_qual;
/*15*/	u_int8_t fru;
/*16*/	u_int8_t sense_key_spec_1;
#define	SSD_SCS_VALID	0x80
/*17*/	u_int8_t sense_key_spec_2;
/*18*/	u_int8_t sense_key_spec_3;
/*32*/	u_int8_t extra_bytes[14];
};
/*
 * Sense bytes described by the extra_len tag start at cmd_spec_info,
 * and can only continue up to the end of the structure we've defined
 * (which is too short for some cases).
 */
#define	ADD_BYTES_LIM(sp)	\
	(((int)(sp)->extra_len) < (int) sizeof(struct scsipi_sense_data) - 8)? \
	((sp)->extra_len) : (sizeof (struct scsipi_sense_data) - 8)


struct scsipi_sense_data_unextended {
/* 1*/	u_int8_t error_code; 
/* 4*/	u_int8_t block[3];
}; 

#define	T_DIRECT	0x00	/* direct access device */
#define	T_SEQUENTIAL	0x01	/* sequential access device */
#define	T_PRINTER	0x02	/* printer device */
#define	T_PROCESSOR	0x03	/* processor device */
#define	T_WORM		0x04	/* write once, read many device */
#define	T_CDROM		0x05	/* cd-rom device */
#define	T_SCANNER 	0x06	/* scanner device */
#define	T_OPTICAL 	0x07	/* optical memory device */
#define	T_CHANGER	0x08	/* medium changer device */
#define	T_COMM		0x09	/* communication device */
#define	T_IT8_1		0x0a	/* ??? */
#define	T_IT8_2		0x0b	/* ??? */
#define	T_STORARRAY	0x0c	/* storage array device */
#define	T_ENCLOSURE	0x0d	/* enclosure services device */
#define	T_NODEVICE	0x1F

#define	T_REMOV		1	/* device is removable */
#define	T_FIXED		0	/* device is not removable */

/*
 * XXX
 * Actually I think some SCSI driver expects this structure to be 32 bytes, so
 * don't change it unless you really know what you are doing
 */

struct scsipi_inquiry_data {
	u_int8_t device;
#define	SID_TYPE	0x1F
#define	SID_QUAL	0xE0
#define	SID_QUAL_LU_OK	0x00
#define	SID_QUAL_LU_OFFLINE	0x20
#define	SID_QUAL_RSVD	0x40
#define	SID_QUAL_BAD_LU	0x60
	u_int8_t dev_qual2;
#define	SID_QUAL2	0x7F
#define	SID_REMOVABLE	0x80
	u_int8_t version;
#define	SID_ANSII	0x07
#define	SID_ECMA	0x38
#define	SID_ISO		0xC0
	u_int8_t response_format;
	u_int8_t additional_length;
	u_int8_t unused[2];
	u_int8_t flags;
#define	SID_SftRe	0x01
#define	SID_CmdQue	0x02
#define	SID_Linked	0x08
#define	SID_Sync	0x10
#define	SID_WBus16	0x20
#define	SID_WBus32	0x40
#define	SID_RelAdr	0x80
	char	vendor[8];
	char	product[16];
	char	revision[4];
	u_int8_t extra[8];
};

#endif /* _SCSI_PI_ALL_H */
