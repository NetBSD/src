/*	$NetBSD: scsipi_all.h,v 1.16 2001/04/25 17:53:39 bouyer Exp $	*/

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

#ifndef _DEV_SCSIPI_SCSIPI_ALL_H_
#define	_DEV_SCSIPI_SCSIPI_ALL_H_

/*
 * SCSI-like command format and opcode
 */

/*
 * Some basic, common SCSI command group definitions.
 */

#define	CDB_GROUPID(cmd)        ((cmd >> 5) & 0x7)
#define	CDB_GROUPID_0	0
#define	CDB_GROUPID_1	1
#define	CDB_GROUPID_2	2
#define	CDB_GROUPID_3	3
#define	CDB_GROUPID_4	4
#define	CDB_GROUPID_5	5
#define	CDB_GROUPID_6	6
#define	CDB_GROUPID_7	7

#define	CDB_GROUP0	6       /*  6-byte cdb's */
#define	CDB_GROUP1	10      /* 10-byte cdb's */
#define	CDB_GROUP2	10      /* 10-byte cdb's */
#define	CDB_GROUP3	0       /* reserved */
#define	CDB_GROUP4	16      /* 16-byte cdb's */
#define	CDB_GROUP5	12      /* 12-byte cdb's */
#define	CDB_GROUP6	0       /* vendor specific */
#define	CDB_GROUP7	0       /* vendor specific */

/*
 * Some basic, common SCSI commands
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

#define	SKEY_NO_SENSE		0x00
#define	SKEY_RECOVERED_ERROR	0x01
#define	SKEY_NOT_READY		0x02
#define	SKEY_MEDIUM_ERROR	0x03
#define	SKEY_HARDWARE_ERROR	0x04
#define	SKEY_ILLEGAL_REQUEST	0x05
#define	SKEY_UNIT_ATTENTION	0x06
#define	SKEY_WRITE_PROTECT	0x07
#define	SKEY_BLANK_CHECK	0x08
#define	SKEY_VENDOR_UNIQUE	0x09
#define	SKEY_COPY_ABORTED	0x0A
#define	SKEY_ABORTED_COMMAND	0x0B
#define	SKEY_EQUAL		0x0C
#define	SKEY_VOLUME_OVERFLOW	0x0D
#define	SKEY_MISCOMPARE		0x0E
#define	SKEY_RESERVED		0x0F

/*
 * Sense bytes described by the extra_len tag start at cmd_spec_info,
 * and can only continue up to the end of the structure we've defined
 * (which is too short for some cases).
 */
#define	ADD_BYTES_LIM(sp)	\
	((((int)(sp)->extra_len) < (int) sizeof(struct scsipi_sense_data) - 8)? \
	((sp)->extra_len) : (sizeof (struct scsipi_sense_data) - 8))


struct scsipi_sense_data_unextended {
/* 1*/	u_int8_t error_code; 
/* 4*/	u_int8_t block[3];
}; 

#define	T_REMOV		1	/* device is removable */
#define	T_FIXED		0	/* device is not removable */

/*
 * According to SPC-2r16, in order to know if a U3W device support PPR,
 * Inquiry Data structure should be at least 57 Bytes
 */

struct scsipi_inquiry_data {
/* 1*/	u_int8_t device;
#define	SID_TYPE		0x1f	/* device type mask */
#define	SID_QUAL		0xe0	/* device qualifier mask */
#define	SID_QUAL_LU_PRESENT	0x00	/* logical unit present */
#define	SID_QUAL_LU_NOTPRESENT	0x20	/* logical unit not present */
#define	SID_QUAL_reserved	0x40
#define	SID_QUAL_LU_NOT_SUPP	0x60	/* logical unit not supported */

#define	T_DIRECT		0x00	/* direct access device */
#define	T_SEQUENTIAL		0x01	/* sequential access device */
#define	T_PRINTER		0x02	/* printer device */
#define	T_PROCESSOR		0x03	/* processor device */
#define	T_WORM			0x04	/* write once, read many device */
#define	T_CDROM			0x05	/* cd-rom device */
#define	T_SCANNER 		0x06	/* scanner device */
#define	T_OPTICAL 		0x07	/* optical memory device */
#define	T_CHANGER		0x08	/* medium changer device */
#define	T_COMM			0x09	/* communication device */
#define	T_IT8_1			0x0a	/* Defined by ASC IT8... */
#define	T_IT8_2			0x0b	/* ...(Graphic arts pre-press devices) */
#define	T_STORARRAY		0x0c	/* storage array device */
#define	T_ENCLOSURE		0x0d	/* enclosure services device */
#define	T_SIMPLE_DIRECT		0x0E	/* Simplified direct-access device */
#define	T_OPTIC_CARD_RW		0x0F	/* Optical card reader/writer device */
#define	T_OBJECT_STORED		0x11	/* Object-based Storage Device */
#define	T_NODEVICE		0x1f

	u_int8_t dev_qual2;
#define	SID_QUAL2		0x7F
#define	SID_REMOVABLE		0x80

/* 3*/	u_int8_t version;
#define	SID_ANSII	0x07
#define	SID_ECMA	0x38
#define	SID_ISO		0xC0

/* 4*/	u_int8_t response_format;
#define	SID_RespDataFmt	0x0F
#define	SID_FORMAT_SCSI1	0x00	/* SCSI-1 format */
#define	SID_FORMAT_CCS		0x01	/* SCSI CCS format */
#define	SID_FORMAT_ISO		0x02	/* ISO format */

/* 5*/	u_int8_t additional_length;	/* n-4 */
/* 6*/	u_int8_t flags1;
#define	SID_SCC		0x80
/* 7*/	u_int8_t flags2;
#define	SID_Addr16	0x01
#define SID_MChngr	0x08
#define	SID_MultiPort	0x10
#define	SID_EncServ	0x40
#define	SID_BasQue	0x80
/* 8*/	u_int8_t flags3;
#define	SID_SftRe	0x01
#define	SID_CmdQue	0x02
#define	SID_Linked	0x08
#define	SID_Sync	0x10
#define	SID_WBus16	0x20
#define	SID_WBus32	0x40
#define	SID_RelAdr	0x80
/* 9*/	char    vendor[8];
/*17*/	char    product[16];
/*33*/	char    revision[4];
/*37*/	u_int8_t vendor_specific[20];
/*57*/	u_int8_t flags4;
#define        SID_IUS         0x01
#define        SID_QAS         0x02
#define        SID_Clocking    0x0C
#define	SID_CLOCKING_ST_ONLY  0x00
#define	SID_CLOCKING_DT_ONLY  0x04
#define	SID_CLOCKING_SD_DT    0x0C
/*58*/	u_int8_t reserved;
/*59*/	char    version_descriptor[8][2];
}; /* 74 Bytes */

#endif /* _DEV_SCSIPI_SCSIPI_ALL_H_ */
