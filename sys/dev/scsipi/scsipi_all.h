/*	$NetBSD: scsipi_all.h,v 1.34 2024/11/09 12:06:22 mlelstv Exp $	*/

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

#define	INQUIRY			0x12
struct scsipi_inquiry {
	u_int8_t opcode;
	u_int8_t byte2;
#define SINQ_EVPD		0x01
	u_int8_t pagecode;
	u_int8_t length_hi;	/* upper byte of length */
	u_int8_t length;
	u_int8_t control;
} __packed;

#define START_STOP		0x1b
struct scsipi_start_stop {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused[2];
	u_int8_t how;
#define SSS_STOP		0x00
#define SSS_START		0x01
#define SSS_LOEJ		0x02
	u_int8_t control;
};

/*
 * inquiry data format
 */

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
#define	SCSIPI_INQUIRY_LENGTH_SCSI2	36
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
#define	SCSIPI_INQUIRY_LENGTH_SCSI3	74
} __packed; /* 74 Bytes */

/* Vital product data when SINQ_EVPD is set */

struct scsipi_inquiry_evpd_header {
/* 1*/	u_int8_t device;
/* 2*/	u_int8_t pagecode;
/* 3*/	u_int8_t length[2];
};

#define SINQ_VPD_PAGES		0x00
#define SINQ_VPD_UNIT_SERIAL	0x80
#define SINQ_VPD_DEVICE_ID	0x83
#define SINQ_VPD_SOFTWARE_ID	0x84
#define SINQ_VPD_MN_ADDRESS	0x85
#define SINQ_VPD_INQUIRY	0x86
#define SINQ_VPD_MP_POLICY	0x87
#define SINQ_VPD_PORTS		0x88
#define SINQ_VPD_POWER_COND	0x8a
#define SINQ_VPD_CONSTITUENTS	0x8b
#define SINQ_VPD_CFA_PROFILE	0x8c
#define SINQ_VPD_CONSUMPTION	0x8d
#define SINQ_VPD_BLOCK_LIMITS	0xb0
#define SINQ_VPD_BLOCK_CHARS	0xb1
#define SINQ_VPD_LOGICAL_PROV	0xb2
#define SINQ_VPD_REFERRASLS	0xb3
#define SINQ_VPD_SUPPORTED	0xb4
#define SINQ_VPD_BLOCK_CHARSX	0xb5
#define SINQ_VPD_BLOCK_ZONED	0xb6
#define SINQ_VPD_BLOCK_LIMITSX	0xb7
#define SINQ_VPD_FIRMWARE_NUM	0xc0
#define SINQ_VPD_JUMPERS	0xc2
#define SINQ_VPD_BEHAVIOUR	0xc3

#define SINQ_VPD_DATE_CODE	0xc1
struct scsipi_inquiry_evpd_date_code {
/* 1*/	u_int8_t etf_log_date[8]; /* MMDDYYYY */
/* 9*/	u_int8_t compile_date[8]; /* MMDDYYYY */
/*17*/	u_int8_t spindown_count[2];
/*19*/	u_int8_t spindown_time[6]; /* HHMMSS */
} __packed;

#define SINQ_VPD_SERIAL		0x80
struct scsipi_inquiry_evpd_serial {
/* 1*/	u_int8_t serial_number[251];
} __packed;

#define SINQ_VPD_DEVICE_ID	0x83
struct scsipi_inquiry_evpd_device_id {
/* 1*/	u_int8_t pc;
#define SINQ_DEVICE_ID_PROTOCOL			0xf0
#define SINQ_DEVICE_ID_PROTOCOL_FC		0x00
#define SINQ_DEVICE_ID_PROTOCOL_SSA		0x20
#define SINQ_DEVICE_ID_PROTOCOL_IEEE1394	0x30
#define SINQ_DEVICE_ID_PROTOCOL_RDMA		0x40
#define SINQ_DEVICE_ID_PROTOCOL_ISCSI		0x50
#define SINQ_DEVICE_ID_PROTOCOL_SAS		0x60
#define SINQ_DEVICE_ID_CODESET			0x0f
#define SINQ_DEVICE_ID_CODESET_BINARY		0x01
#define SINQ_DEVICE_ID_CODESET_ASCII		0x02
#define SINQ_DEVICE_ID_CODESET_UTF8		0x03
/* 2*/	u_int8_t flags;
#define SINQ_DEVICE_ID_PIV			0x80
#define SINQ_DEVICE_ID_ASSOCIATION		0x30
#define SINQ_DEVICE_ID_ASSOCIATION_DEVICE	0x00
#define SINQ_DEVICE_ID_ASSOCIATION_PORT		0x10
#define SINQ_DEVICE_ID_ASSOCIATION_TARGET	0x20
#define SINQ_DEVICE_ID_TYPE			0x0f
#define SINQ_DEVICE_ID_TYPE_UNASSIGNED		0x00
#define SINQ_DEVICE_ID_TYPE_VENDOR		0x01
#define SINQ_DEVICE_ID_TYPE_EUI64		0x02
#define SINQ_DEVICE_ID_TYPE_FC			0x03
#define SINQ_DEVICE_ID_TYPE_PORTNUMBER1		0x04
#define SINQ_DEVICE_ID_TYPE_PORTNUMBER2		0x05
#define SINQ_DEVICE_ID_TYPE_PORTNUMBER3		0x06
#define SINQ_DEVICE_ID_TYPE_MD5			0x07
/* 3*/	u_int8_t reserved;
/* 4*/	u_int8_t designator_length;
/* 5*/	u_int8_t designator[1];
} __packed;

#endif /* _DEV_SCSIPI_SCSIPI_ALL_H_ */
