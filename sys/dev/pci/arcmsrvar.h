/*	$NetBSD: arcmsrvar.h,v 1.17 2026/11/23 06:05:21 jnemeth Exp $ */
/*	Derived from $OpenBSD: arc.c,v 1.68 2007/10/27 03:28:27 dlg Exp $ */

/*
 * Copyright (c) 2007 Juan Romero Pardines <xtraeme@netbsd.org>
 * Copyright (c) 2006 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _PCI_ARCMSRVAR_H_
#define _PCI_ARCMSRVAR_H_

#define ARCMSR_VERSION	"arcmsr v1.5.0.0-20200623" 

#define ARC_HBA_TYPE_A			0x00000000
#define ARC_HBA_TYPE_B			0x00000001
#define ARC_HBA_TYPE_C			0x00000002
#define ARC_HBA_TYPE_D			0x00000003
#define ARC_HBA_TYPE_E			0x00000004
#define ARC_HBA_TYPE_F			0x00000005

#define ARC_PCI_BAR			PCI_MAPREG_START
#define ARC_RB_DOORBELL_BAR		PCI_MAPREG_START
#define ARC_RB_RWBUFFER_BAR		PCI_MAPREG_PPB_END
#define ARC_RC_PCI_BAR			PCI_MAPREG_PCB_END
#define ARC_RD_PCI_BAR			PCI_MAPREG_START
#define ARC_RE_PCI_BAR			PCI_MAPREG_PCB_END
#define ARC_RF_PCI_BAR			PCI_MAPREG_START

#define ARCMSR_MAX_CCB_COUNT		264
#define ARCMSR_MAX_HBB_POSTQUEUE	264
#define ARCMSR_MAX_HBD_POSTQUEUE	256
#define ARCMSR_MAX_HBE_COMPLETION_Q	256
#define ARCMSR_MAX_HBF_COMPLETION_Q	256

/* Areca boards using the Intel IOP are Type A (RA) */
#define ARC_REG_INB_MSG0		0x0010
#define  ARC_REG_INB_MSG0_NOP			(0x00000000)
#define  ARC_REG_INB_MSG0_GET_CONFIG		(0x00000001)
#define  ARC_REG_INB_MSG0_SET_CONFIG		(0x00000002)
#define  ARC_REG_INB_MSG0_ABORT_CMD		(0x00000003)
#define  ARC_REG_INB_MSG0_STOP_BGRB		(0x00000004)
#define  ARC_REG_INB_MSG0_FLUSH_CACHE		(0x00000005)
#define  ARC_REG_INB_MSG0_START_BGRB		(0x00000006)
#define  ARC_REG_INB_MSG0_CHK331PENDING		(0x00000007)
#define  ARC_REG_INB_MSG0_SYNC_TIMER		(0x00000008)
#define ARC_REG_INB_MSG1		0x0014
#define ARC_REG_OUTB_ADDR0		0x0018
#define ARC_REG_OUTB_ADDR1		0x001c
#define  ARC_REG_OUTB_ADDR1_FIRMWARE_OK		(1<<31)
#define ARC_REG_INB_DOORBELL		0x0020
#define  ARC_REG_INB_DOORBELL_WRITE_OK		(1<<0)
#define  ARC_REG_INB_DOORBELL_READ_OK		(1<<1)
#define ARC_REG_OUTB_DOORBELL		0x002c
#define  ARC_REG_OUTB_DOORBELL_WRITE_OK		(1<<0)
#define  ARC_REG_OUTB_DOORBELL_READ_OK		(1<<1)
#define ARC_REG_INTRSTAT		0x0030
#define  ARC_REG_INTRSTAT_MSG0			(1<<0)
#define  ARC_REG_INTRSTAT_MSG1			(1<<1)
#define  ARC_REG_INTRSTAT_DOORBELL		(1<<2)
#define  ARC_REG_INTRSTAT_POSTQUEUE		(1<<3)
#define  ARC_REG_INTRSTAT_PCI			(1<<4)
#define ARC_REG_INTRMASK		0x0034
#define  ARC_REG_INTRMASK_MSG0			(1<<0)
#define  ARC_REG_INTRMASK_MSG1			(1<<1)
#define  ARC_REG_INTRMASK_DOORBELL		(1<<2)
#define  ARC_REG_INTRMASK_POSTQUEUE		(1<<3)
#define  ARC_REG_INTRMASK_PCI			(1<<4)
#define ARC_REG_POST_QUEUE		0x0040
#define  ARC_REG_POST_QUEUE_ADDR_SHIFT		5
#define  ARC_REG_POST_QUEUE_IAMBIOS		(1<<30)
#define  ARC_REG_POST_QUEUE_BIGFRAME		(1<<31)
#define ARC_REG_REPLY_QUEUE		0x0044
#define  ARC_REG_REPLY_QUEUE_ADDR_SHIFT		5
#define  ARC_REG_REPLY_QUEUE_ERR		(1<<28)
#define  ARC_REG_REPLY_QUEUE_IAMBIOS		(1<<30)
#define ARC_REG_MSGBUF			0x0a00
#define  ARC_REG_MSGBUF_LEN		1024
#define ARC_REG_IOC_WBUF_LEN		0x0e00
#define ARC_REG_IOC_WBUF		0x0e04
#define ARC_REG_IOC_RBUF_LEN		0x0f00
#define ARC_REG_IOC_RBUF		0x0f04
#define  ARC_REG_IOC_RWBUF_MAXLEN	124 /* for both RBUF and WBUF */

/* Areca boards using the Marvel IOP0 are Type B (RB) */

#define ARC_RB_INB_DOORBELL			0x00020400
#define ARC_RB_INB_DOORBELL_MASK		0x00020404
#define ARC_RB_OUTB_DOORBELL			0x00020408
#define ARC_RB_OUTB_DOORBELL_MASK		0x0002040c
#define ARC_RB_OUTB_DOORBELL_FIRMWARE_OK	(1<<31)

#define ARC_RB_OUTB_DOORBELL_1203		0x00021870
#define ARC_RB_OUTB_DOORBELL_MASK_1203		0x00021874
#define ARC_RB_INB_DOORBELL_1203		0x00021878
#define ARC_RB_INB_DOORBELL_MASK_1203		0x0002187C

#define ARC_RB_INB_MSG0_GET_CONFIG		0x00010008
#define ARC_RB_INB_MSG0_SET_CONFIG		0x00020008
#define ARC_RB_INB_MSG0_ABORT_CMD		0x00030008
#define ARC_RB_INB_MSG0_STOP_BGRB		0x00040008
#define ARC_RB_INB_MSG0_FLUSH_CACHE		0x00050008
#define ARC_RB_INB_MSG0_START_BGRB		0x00060008
#define ARC_RB_INB_MSG0_START_DRIVER_MODE	0x000E0008
#define ARC_RB_INB_MSG0_SET_POST_WINDOW		0x000F0008
#define ARC_RB_INB_MSG0_ACTIVE_EOI_MODE		0x00100008
#define ARC_RB_INB_MSG0_FIRMWARE_OK		0x80000000

#define ARC_RB_I2D_DATA_WRITE_OK		0x00000001
#define ARC_RB_I2D_DATA_READ_OK			0x00000002
#define ARC_RB_I2D_CDB_DONE			0x00000004
#define ARC_RB_I2D_MSG_CMD_DONE			0x00000008
#define ARC_RB_I2D_INTR_ENABLE_DOORBELL		0x0000000F
#define ARC_RB_I2D_DOORBELL_INT			0x0000000B
#define ARC_RB_I2D_POSTQ_INT			0x00000004

#define ARC_RB_D2I_DATA_WRITE_OK		0x00000001
#define ARC_RB_D2I_DATA_READ_OK			0x00000002
#define ARC_RB_D2I_CDB_POSTED			0x00000004
#define ARC_RB_D2I_MSG_CMD_POSTED		0x00000008
#define ARC_RB_D2I_END_OF_INTERRUPT		0x00000010

#define ARC_RB_DOORBELL_HANDLE_INT		0x0000000F
#define ARC_RB_DOORBELL_INT_CLEAR_PATTERN	0xFF00FFF0
#define ARC_RB_MESSAGE_INT_CLEAR_PATTERN	0xFF00FFF7

/* data tunnel buffer between user space and firmware */
#define ARC_RB_MSG_RWBUF			0x0000fa00
#define ARC_RB_MSG_WBUF				0x0000fe00
#define ARC_RB_MSG_RBUF				0x0000ff00

#define  ARC_RB_REPLY_QUEUE_ERR			(1<<28)

/* Areca boards using the LSI IOP are Type C (RC) */

#define ARC_RC_INB_DOORBELL	   	0x20
#define ARC_RC_INTR_STAT		0x30
#define	ARC_RC_INTR_MASK		0x34
#define	ARC_RC_OUTB_DOORBELL		0x9C
#define	ARC_RC_OUTB_DOORBELL_CLR	0xA0
#define	ARC_RC_D2I_MSG_CMD_DONE		0x08
#define	ARC_RC_I2D_MSG_CMD_DONE		0x08
#define	ARC_RC_I2D_MSG_CMD_DONE_CLR	0x08
#define ARC_RC_INB_MSGADDR0		0xB0
#define ARC_RC_INB_MSGADDR1		0xB4
#define ARC_RC_OUTB_MSGADDR0		0xB8
#define ARC_RC_OUTB_MSGADDR1		0xBC
#define ARC_RC_OUTB_MSG_FIRMWARE_OK	0x80000000
#define ARC_RC_INB_POSTQ_LOW		0xC0
#define ARC_RC_INB_POSTQ_HIGH		0xC4
#define	ARC_RC_OUTB_REPLYQ_LOW		0xC8
#define	ARC_RC_OUTB_REPLYQ_HIGH		0xCC
#define	ARC_RC_MSG_WBUF_LEN		0x2000
#define	ARC_RC_MSG_WBUF			0x2004
#define	ARC_RC_MSG_RBUF_LEN		0x2100
#define	ARC_RC_MSG_RBUF			0x2104
#define	ARC_RC_MSG_RWBUF		0x2200

#define ARC_RC_INB_MSG0_NOP		(0x00000000)
#define ARC_RC_INB_MSG0_GET_CONFIG	(0x00000001)
#define ARC_RC_INB_MSG0_SET_CONFIG	(0x00000002)
#define ARC_RC_INB_MSG0_ABORT_CMD	(0x00000003)
#define ARC_RC_INB_MSG0_STOP_BGRB	(0x00000004)
#define ARC_RC_INB_MSG0_FLUSH_CACHE	(0x00000005)
#define ARC_RC_INB_MSG0_START_BGRB	(0x00000006)
#define ARC_RC_INB_MSG0_CHK331PENDING	(0x00000007)
#define ARC_RC_INB_MSG0_SYNC_TIMER	(0x00000008)

#define ARC_RC_D2I_DATA_WRITE_OK	0x00000002
#define ARC_RC_D2I_DATA_READ_OK		0x00000004
#define ARC_RC_D2I_MESSAGE_CMD_DONE	0x00000008
#define ARC_RC_D2I_POSTQUEUE_THROTTLING	0x00000010
#define ARC_RC_I2D_DATA_WRITE_OK	0x00000002
#define ARC_RC_I2D_DATA_WRITE_OK_CLEAR	0x00000002
#define ARC_RC_I2D_DATA_READ_OK		0x00000004
#define ARC_RC_I2D_DATA_READ_OK_CLEAR	0x00000004
#define ARC_RC_I2D_MESSAGE_CMD_DONE	0x00000008
#define ARC_RC_I2D_MESSAGE_CMD_DONE_CLEAR 0x00000008
#define ARC_RC_MESSAGE_FIRMWARE_OK	0x80000000

#define ARC_RC_INTR_STAT_UTILITY_A	(1<<0)
#define ARC_RC_INTR_STAT_DOORBELL	(1<<2)
#define ARC_RC_INTR_STAT_POSTQUEUE	(1<<3)
#define ARC_RC_INTR_MASK_ALL		0x0000000D
#define ARC_RC_INTR_MASK_UTILITY_A	(1<<0)
#define ARC_RC_INTR_MASK_DOORBELL	(1<<2)
#define ARC_RC_INTR_MASK_POSTQUEUE	(1<<3)
#define ARC_RC_REPLY_QUEUE_ERR		1
#define	ARC_RC_THROTTLE			12

/* Areca boards using the Marvell IOP 9580 are Type D (RD) */

#define ARC_RD_INTR_STAT		0x200
#define ARC_RD_HOST_INT_ENABLE		0x204
#define	ARC_RD_INTR_ENABLE		0x20C
#define	ARC_RD_D2I_MSG_CMD_DONE		0x08
#define	ARC_RD_I2D_MSG_CMD_DONE		0x2000000
#define	ARC_RD_I2D_MSG_CMD_DONE_CLR	0x2000000
#define ARC_RD_INB_MSGADDR0		0x400
#define ARC_RD_INB_MSGADDR1		0x404
#define ARC_RD_OUTB_MSGADDR0		0x420
#define ARC_RD_OUTB_MSGADDR1		0x424
#define ARC_RD_INB_DOORBELL		0x460
#define	ARC_RD_OUTB_DOORBELL		0x480
#define	ARC_RD_OUTB_DOORBELL_CLR	0x480
#define	ARC_RD_OUTB_DOORBELL_ENABLE	0x484
#define ARC_RD_OUTB_MSG_FIRMWARE_OK	0x80000000
#define ARC_RD_INB_POSTQ_LOW		0x1000
#define ARC_RD_INB_POSTQ_HIGH		0x1004
#define	ARC_RD_OUTB_REPLYQ_LOW		0x1060
#define	ARC_RD_OUTB_REPLYQ_HIGH		0x1064

#define ARC_RD_INB_WRITE_PTR		0x1018
#define ARC_RD_INB_READ_PTR		0x101C
#define	ARC_RD_OUTB_COPY_PTR		0x106C
#define	ARC_RD_OUTB_READ_PTR		0x1070
#define	ARC_RD_OUTB_INTR_CAUSE		0x1088
#define	ARC_RD_OUTB_INT_ENABLE		0x108C
#define	ARC_RD_MSG_WBUF_LEN		0x2000
#define	ARC_RD_MSG_WBUF			0x2004
#define	ARC_RD_MSG_RBUF_LEN		0x2100
#define	ARC_RD_MSG_RBUF			0x2104
#define	ARC_RD_MSG_RWBUF		0x2200

#define ARC_RD_INB_MSG0_NOP		(0x00000000)
#define ARC_RD_INB_MSG0_GET_CONFIG	(0x00000001)
#define ARC_RD_INB_MSG0_SET_CONFIG	(0x00000002)
#define ARC_RD_INB_MSG0_ABORT_CMD	(0x00000003)
#define ARC_RD_INB_MSG0_STOP_BGRB	(0x00000004)
#define ARC_RD_INB_MSG0_FLUSH_CACHE	(0x00000005)
#define ARC_RD_INB_MSG0_START_BGRB	(0x00000006)
#define ARC_RD_INB_MSG0_CHK331PENDING	(0x00000007)
#define ARC_RD_INB_MSG0_SYNC_TIMER	(0x00000008)

#define ARC_RD_D2I_DATA_WRITE_OK	0x00000001
#define ARC_RD_D2I_DATA_READ_OK		0x00000002
#define ARC_RD_D2I_MESSAGE_CMD_DONE	0x02000000
#define ARC_RD_D2I_POSTQUEUE_THROTTLING	0x00000010
#define ARC_RD_I2D_DATA_WRITE_OK	0x00000001
#define ARC_RD_I2D_DATA_WRITE_CLEAR	0x00000001
#define ARC_RD_I2D_DATA_READ_OK		0x00000002
#define ARC_RD_I2D_DATA_READ_CLEAR	0x00000002
#define ARC_RD_I2D_MESSAGE_CMD_DONE	0x02000000
#define ARC_RD_I2D_MESSAGE_CMD_DONE_CLEAR 0x02000000
#define ARC_RD_MESSAGE_FIRMWARE_OK	0x80000000

#define ARC_RD_INTR_STAT_DOORBELL	0x00001000
#define ARC_RD_INTR_STAT_POSTQUEUE	0x00000010
#define ARC_RD_INTR_ENABLE_ALL		0x00001010
#define ARC_RD_INTR_DISABLE_ALL		0x00000000
#define ARC_RD_INTR_ENABLE_DOORBELL	0x00001000
#define ARC_RD_INTR_ENABLE_POSTQUEUE	0x00000010
#define ARC_RD_REPLY_QUEUE_ERR		1
#define	ARC_RD_OUTB_LIST_INT_CLR	1

/* Areca boards using the LSI IOP are Type E (RE) */
#define ARC_SIGNATURE_1884		0x188417D3

#define ARC_RE_INB_DOORBELL	   	0x00
#define	ARC_RE_OUTB_DOORBELL		0x00
#define ARC_RE_INTR_STAT		0x30
#define	ARC_RE_INTR_MASK		0x34
#define	ARC_RE_D2I_MSG_CMD_DONE		0x08
#define	ARC_RE_I2D_MSG_CMD_DONE		0x08
#define ARC_RE_REPLY_PRODUCER_INDEX	0x68
#define ARC_RE_REPLY_CONSUMER_INDEX	0x6C
#define ARC_RE_INB_MSGADDR0		0xB0
#define ARC_RE_INB_MSGADDR1		0xB4
#define ARC_RE_OUTB_MSGADDR0		0xB8
#define ARC_RE_OUTB_MSGADDR1		0xBC
#define ARC_RE_INB_POSTQ_LOW		0xC0
#define ARC_RE_INB_POSTQ_HIGH		0xC4
#define	ARC_RE_OUTB_REPLYQ_LOW		0xC8
#define	ARC_RE_OUTB_REPLYQ_HIGH		0xCC
#define	ARC_RE_MSG_WBUF_LEN		0x2000
#define	ARC_RE_MSG_WBUF			0x2004
#define	ARC_RE_MSG_RBUF_LEN		0x2100
#define	ARC_RE_MSG_RBUF			0x2104
#define	ARC_RE_MSG_RWBUF		0x2200

#define ARC_RE_INB_MSG0_NOP		(0x00000000)
#define ARC_RE_INB_MSG0_GET_CONFIG	(0x00000001)
#define ARC_RE_INB_MSG0_SET_CONFIG	(0x00000002)
#define ARC_RE_INB_MSG0_ABORT_CMD	(0x00000003)
#define ARC_RE_INB_MSG0_STOP_BGRB	(0x00000004)
#define ARC_RE_INB_MSG0_FLUSH_CACHE	(0x00000005)
#define ARC_RE_INB_MSG0_START_BGRB	(0x00000006)
#define ARC_RE_INB_MSG0_SYNC_TIMER	(0x00000008)
#define ARC_RE_DOORBELL_SYNC		(0x00000100)

#define ARC_RE_D2I_DATA_WRITE_OK	0x00000002
#define ARC_RE_D2I_DATA_READ_OK		0x00000004
#define ARC_RE_D2I_MESSAGE_CMD_DONE	0x00000008
#define ARC_RE_I2D_DATA_WRITE_OK	0x00000002
#define ARC_RE_I2D_DATA_READ_OK		0x00000004
#define ARC_RE_I2D_MESSAGE_CMD_DONE	0x00000008
#define ARC_RE_MESSAGE_FIRMWARE_OK	0x80000000
#define ARC_RE_OUTB_MSG_FIRMWARE_OK	0x80000000

#define ARC_RE_INTR_STAT_DOORBELL	(1<<0)
#define ARC_RE_INTR_STAT_POSTQUEUE	(1<<3)
#define ARC_RE_INTR_MASK_ALL		0x00000009
#define ARC_RE_INTR_MASK_DOORBELL	(1<<0)
#define ARC_RE_INTR_MASK_POSTQUEUE	(1<<3)
#define ARC_RE_REPLY_QUEUE_ERR		1

/* Areca boards using the LSI IOP are Type F (RF) */
// Type F register definition and offset are same as type E,
// except message R/W/RW buffer register(in system memory)
#define ARC_SIGNATURE_1886		0x188617D3
#define ARC_RF_DOORBELL_SYNC		0x00000100
//set host rw buffer physical address at inbound message 0, 1 (low,high)
#define ARC_RF_DOORBELL_SYNC1		0x00000300
#define ARC_RF_MESSAGE_FIRMWARE_OK	0x80000000
#define ARC_RF_MSGE_NO_VOLUME_CHANGE	0x20000000

struct arc_msg_firmware_info {
	uint32_t	signature;
#define ARC_FWINFO_SIGNATURE_GET_CONFIG		(0x87974060)
#define ARC_FWINFO_SIGNATURE_SET_CONFIG		(0x87974063)
	uint32_t	request_len;
	uint32_t	queue_len;
	uint32_t	sdram_size;
	uint32_t	sata_ports;
	uint8_t		vendor[40];
	uint8_t		model[8];
	uint8_t		fw_version[16];
	uint8_t		device_map[16];
} __packed;

struct arc_msg_scsicmd {
	uint8_t		bus;
	uint8_t		target;
	uint8_t		lun;
	uint8_t		function;

	uint8_t		cdb_len;
	uint8_t		sgl_len;
	uint8_t		flags;
#define ARC_MSG_SCSICMD_FLAG_SGL_BSIZE_512	(1<<0)
#define ARC_MSG_SCSICMD_FLAG_FROM_BIOS		(1<<1)
#define ARC_MSG_SCSICMD_FLAG_WRITE		(1<<2)
#define ARC_MSG_SCSICMD_FLAG_SIMPLEQ		(0x00)
#define ARC_MSG_SCSICMD_FLAG_HEADQ		(0x08)
#define ARC_MSG_SCSICMD_FLAG_ORDERQ		(0x10)
	uint8_t		msgPages;

	uint32_t	context;
	uint32_t	data_len;

#define ARC_MSG_CDBLEN				16
	uint8_t		cdb[ARC_MSG_CDBLEN];

	uint8_t		status;
#define ARC_MSG_STATUS_SELTIMEOUT		0xf0
#define ARC_MSG_STATUS_ABORTED			0xf1
#define ARC_MSG_STATUS_INIT_FAIL		0xf2
#define ARC_MSG_SENSELEN			15
	uint8_t		sense_data[ARC_MSG_SENSELEN];

	/* followed by an sgl */
} __packed;

struct arc_sge {
	uint32_t	sg_hdr;
#define ARC_SGE_64BIT				(1<<24)
	uint32_t	sg_lo_addr;
	uint32_t	sg_hi_addr;
} __packed;

#define ARC_MAX_TARGET		16
#define ARC_MAX_LUN		8
#define ARC_MAX_IOCMDLEN	512
#define ARC_BLOCKSIZE		512

/* 
 * the firmware deals with up to 256 or 512 byte command frames.
 */

/* 
 * sizeof(struct arc_msg_scsicmd) + (sizeof(struct arc_sge) * 38) == 504.
 */
#define ARC_SGL_MAXLEN		38
/* 
 * sizeof(struct arc_msg_scsicmd) + (sizeof(struct arc_sge) * 17) == 252.
 */
#define ARC_SGL_256LEN		17

struct arc_io_cmd {
	struct arc_msg_scsicmd	cmd;
	struct arc_sge		sgl[ARC_SGL_MAXLEN];
	uint32_t		reserved;
	uint32_t		ccb_id;
} __packed;

#define ARC_IOCMD_LEN		sizeof(struct arc_io_cmd)

/* 
 * stuff to manage a scsi command.
 */
struct arc_ccb {
	struct arc_softc	*ccb_sc;
	int			ccb_id;

	struct scsipi_xfer	*ccb_xs;

	bus_dmamap_t		ccb_dmamap;
	bus_addr_t		ccb_offset;
	struct arc_io_cmd	*ccb_cmd;
	uint32_t		ccb_cmd_post;

	TAILQ_ENTRY(arc_ccb)	ccb_link;
	uint32_t		arc_io_cmd_length;
	uint32_t		smid;
};

/* 
 * definitions of the firmware commands sent via the doorbells.
 */
struct arc_fw_hdr {
	uint8_t		byte1;
	uint8_t		byte2;
	uint8_t		byte3;
} __packed;

struct arc_fw_bufhdr {
	struct arc_fw_hdr	hdr;
	uint16_t		len;
} __packed;

/* Firmware command codes */
#define ARC_FW_CHECK_PASS	0x14	/* opcode + 1 byte length + password */
#define ARC_FW_GETEVENTS	0x1a	/* opcode + 1 byte for page 0/1/2/3 */
#define ARC_FW_GETHWMON		0x1b	/* opcode + arc_fw_hwmon */
#define ARC_FW_RAIDINFO		0x20	/* opcode + raid# */
#define ARC_FW_VOLINFO		0x21	/* opcode + vol# */
#define ARC_FW_DISKINFO		0x22	/* opcode + physdisk# */
#define ARC_FW_SYSINFO		0x23	/* opcode. reply is fw_sysinfo */
#define ARC_FW_CLEAREVENTS	0x24	/* opcode only */
#define ARC_FW_MUTE_ALARM	0x30	/* opcode only */
#define ARC_FW_SET_ALARM	0x31	/* opcode + 1 byte for setting */
#define  ARC_FW_SET_ALARM_DISABLE		0x00
#define  ARC_FW_SET_ALARM_ENABLE		0x01
#define ARC_FW_SET_PASS		0x32	/* opcode + 1 byte length + password */
#define ARC_FW_REBUILD_PRIO	0x34	/* Rebuild priority for disks */
#define  ARC_FW_REBUILD_PRIO_ULTRALOW		(1<<0)
#define  ARC_FW_REBUILD_PRIO_LOW		(1<<1)
#define  ARC_FW_REBUILD_PRIO_NORMAL		(1<<2)
#define  ARC_FW_REBUILD_PRIO_HIGH		(1<<3)
#define ARC_FW_SET_MAXATA_MODE	0x35	/* opcode + 1 byte mode */
#define  ARC_FW_SET_MAXATA_MODE_133		(1<<0)
#define  ARC_FW_SET_MAXATA_MODE_100		(1<<1)
#define  ARC_FW_SET_MAXATA_MODE_66		(1<<2)
#define  ARC_FW_SET_MAXATA_MODE_33		(1<<3)
#define ARC_FW_NOP		0x38	/* opcode only */
/*
 * Structure for ARC_FW_CREATE_PASSTHRU:
 *
 * byte 2	command code 0x40
 * byte 3	device #
 * byte 4	scsi channel (0/1)
 * byte 5	scsi id (0/15)
 * byte 6	scsi lun (0/7)
 * byte 7	tagged queue (1 enabled)
 * byte 8	cache mode (1 enabled)
 * byte 9	max speed ((0/1/2/3/4 -> 33/66/100/133/150)
 */
#define ARC_FW_CREATE_PASSTHRU	0x40
#define ARC_FW_DELETE_PASSTHRU	0x42	/* opcode + device# */

/*
 * Structure for ARC_FW_CREATE_RAIDSET:
 *
 * byte 2	command code 0x50
 * byte 3-6	device mask
 * byte 7-22	raidset name (byte 7 == 0 use default)
 */	
#define ARC_FW_CREATE_RAIDSET	0x50
#define ARC_FW_DELETE_RAIDSET	0x51	/* opcode + raidset# */
#define ARC_FW_CREATE_HOTSPARE	0x54	/* opcode + 4 bytes device mask */
#define ARC_FW_DELETE_HOTSPARE	0x55	/* opcode + 4 bytes device mask */

/*
 * Structure for ARC_FW_CREATE_VOLUME/ARC_FW_MODIFY_VOLUME:
 *
 * byte 2 	command code 0x60
 * byte 3 	raidset#
 * byte 4-19 	volume set name (byte 4 == 0 use default)
 * byte 20-27	volume capacity in blocks
 * byte 28	raid level
 * byte 29	stripe size
 * byte 30	channel
 * byte 31	ID
 * byte 32	LUN
 * byte 33	1 enable tag queuing
 * byte 33	1 enable cache
 * byte 35	speed 0/1/2/3/4 -> 33/66/100/133/150
 * byte 36	1 for quick init (only for CREATE_VOLUME)
 */
#define ARC_FW_CREATE_VOLUME	0x60
#define ARC_FW_MODIFY_VOLUME 	0x61
#define ARC_FW_DELETE_VOLUME	0x62	/* opcode + vol# */
#define ARC_FW_START_CHECKVOL	0x63	/* opcode + vol# */
#define ARC_FW_STOP_CHECKVOL	0x64	/* opcode only */

/* Status codes for the firmware command codes */
#define ARC_FW_CMD_OK		0x41
#define ARC_FW_CMD_RAIDINVAL	0x42
#define ARC_FW_CMD_VOLINVAL	0x43
#define ARC_FW_CMD_NORAID	0x44
#define ARC_FW_CMD_NOVOLUME	0x45
#define ARC_FW_CMD_NOPHYSDRV	0x46
#define ARC_FW_CMD_PARAM_ERR	0x47
#define ARC_FW_CMD_UNSUPPORTED	0x48
#define ARC_FW_CMD_DISKCFG_CHGD	0x49
#define ARC_FW_CMD_PASS_INVAL	0x4a
#define ARC_FW_CMD_NODISKSPACE	0x4b
#define ARC_FW_CMD_CHECKSUM_ERR	0x4c
#define ARC_FW_CMD_PASS_REQD	0x4d

struct arc_fw_hwmon {
	uint8_t 	nfans;
	uint8_t 	nvoltages;
	uint8_t 	ntemps;
	uint8_t 	npower;
	uint16_t 	fan0;		/* RPM */
	uint16_t 	fan1;		/* RPM */
	uint16_t 	voltage_orig0;	/* original value * 1000 */
	uint16_t 	voltage_val0;	/* value */
	uint16_t 	voltage_orig1;	/* original value * 1000 */
	uint16_t 	voltage_val1;	/* value */
	uint16_t 	voltage_orig2;
	uint16_t 	voltage_val2;
	uint8_t 	temp0;
	uint8_t 	temp1;
	uint8_t 	pwr_indicator;	/* (bit0 : power#0, bit1 : power#1) */
	uint8_t 	ups_indicator;
} __packed;

struct arc_fw_comminfo {
	uint8_t		baud_rate;
	uint8_t		data_bits;
	uint8_t		stop_bits;
	uint8_t		parity;
	uint8_t		flow_control;
} __packed;

struct arc_fw_scsiattr {
	uint8_t		channel;	/* channel for SCSI target (0/1) */
	uint8_t		target;
	uint8_t		lun;
	uint8_t		tagged;
	uint8_t		cache;
	uint8_t		speed;
} __packed;

struct arc_fw_raidinfo {
	uint8_t		set_name[16];
	uint32_t	capacity;
	uint32_t	capacity2;
	uint32_t	fail_mask;
	uint8_t		device_array[32];
	uint8_t		member_devices;
	uint8_t		new_member_devices;
	uint8_t		raid_state;
	uint8_t		volumes;
	uint8_t		volume_list[16];
	uint8_t		reserved1[3];
	uint8_t		free_segments;
	uint32_t	raw_stripes[8];
	uint8_t		reserved2[12];
	uint8_t		vol_ListX[112];
	uint8_t		devEncArray[32];
	uint8_t		reserved3[72];
} __packed;

struct arc_fw_volinfo {
	uint8_t		set_name[16];
	uint32_t	capacity;
	uint32_t	capacity2;
	uint32_t	fail_mask;
	uint32_t	stripe_size;	/* in blocks */
	uint32_t	new_fail_mask;
	uint32_t	new_stripe_size;
	uint32_t	volume_status;
#define ARC_FW_VOL_STATUS_NORMAL	0x00
#define ARC_FW_VOL_STATUS_INITTING	(1<<0)
#define ARC_FW_VOL_STATUS_FAILED	(1<<1)
#define ARC_FW_VOL_STATUS_MIGRATING	(1<<2)
#define ARC_FW_VOL_STATUS_REBUILDING	(1<<3)
#define ARC_FW_VOL_STATUS_NEED_INIT	(1<<4)
#define ARC_FW_VOL_STATUS_NEED_MIGRATE	(1<<5)
#define ARC_FW_VOL_STATUS_INIT_FLAG	(1<<6)
#define ARC_FW_VOL_STATUS_NEED_REGEN	(1<<7)
#define ARC_FW_VOL_STATUS_CHECKING	(1<<8)
#define ARC_FW_VOL_STATUS_NEED_CHECK	(1<<9)
	uint32_t	progress;
	struct arc_fw_scsiattr	scsi_attr;
	uint8_t		member_disks;
	uint8_t		raid_level;
#define ARC_FW_VOL_RAIDLEVEL_0		0x00
#define ARC_FW_VOL_RAIDLEVEL_1		0x01
#define ARC_FW_VOL_RAIDLEVEL_3		0x02
#define ARC_FW_VOL_RAIDLEVEL_5		0x03
#define ARC_FW_VOL_RAIDLEVEL_6		0x04
#define ARC_FW_VOL_RAIDLEVEL_PASSTHRU	0x05
	uint8_t		new_member_disks;
	uint8_t		new_raid_level;
	uint8_t		raid_set_number;
	uint8_t		vol_state0;
	uint32_t	host_speed;
	uint32_t	vol_state;
	uint8_t		vol_array[16];
	uint8_t		num_5060volumes;
	uint8_t		reserved[43];
} __packed;

struct arc_fw_diskinfo {
	uint8_t		model[40];
	uint8_t		serial[20];
	uint8_t		firmware_rev[8];
	uint32_t	capacity;
	uint32_t	capacity2;
	uint8_t		device_state;
#define ARC_FW_DISK_NORMAL	0x88	/* disk attached/initialized */
#define ARC_FW_DISK_PASSTHRU	0x8a	/* pass through disk in normal state */
#define ARC_FW_DISK_HOTSPARE	0xa8	/* hotspare disk in normal state */
#define ARC_FW_DISK_UNUSED	0xc8	/* free/unused disk in normal state */
#define ARC_FW_DISK_FAILED	0x10	/* disk in failed state */
	uint8_t		pio_mode;
	uint8_t		current_udma_mode;
	uint8_t		udma_mode;
	uint8_t		drive_select;
	uint8_t		raid_number;	/* 0xff unowned */
	struct arc_fw_scsiattr	scsi_attr;
	uint8_t		reserved[208];
} __packed;

struct arc_fw_sysinfo {
	uint8_t		vendor_name[40];
	uint8_t		serial_number[16];
	uint8_t		firmware_version[16];
	uint8_t		boot_version[16];
	uint8_t		mb_version[16];
	uint8_t		model_name[8];

	uint8_t		local_ip[4];
	uint8_t		current_ip[4];

	uint32_t	time_tick;
	uint32_t	cpu_speed;
	uint32_t	icache;
	uint32_t	dcache;
	uint32_t	scache;
	uint32_t	memory_size;
	uint32_t	memory_speed;
	uint32_t	events;

	uint8_t		gsiMacAddress[6];
	uint8_t		gsiDhcp;

	uint8_t		alarm;
	uint8_t		channel_usage;
	uint8_t		max_ata_mode;
	uint8_t		sdram_ecc;
	uint8_t		rebuild_priority;
	struct arc_fw_comminfo	comm_a;
	struct arc_fw_comminfo	comm_b;
	uint8_t		ide_channels;
	uint8_t		scsi_host_channels;
	uint8_t		ide_host_channels;
	uint8_t		max_volume_set;
	uint8_t		max_raid_set;
	uint8_t		ether_port;
	uint8_t		raid6_engine;
	uint8_t		reserved[75];
} __packed;

struct arc_HBB_Msgu {
	uint32_t post_qbuffer[ARCMSR_MAX_HBB_POSTQUEUE];
	uint32_t done_qbuffer[ARCMSR_MAX_HBB_POSTQUEUE];
	uint32_t postq_index;
	uint32_t doneq_index;
};

#define	ARC_HBB_CMDQ_PTR_LEN	sizeof(struct arc_HBB_Msgu)

struct InBound_SRB {
	uint32_t addressLow;	/* pointer to SRB block */
	uint32_t addressHigh;
	uint32_t length;	/* in DWORDs */
	uint32_t reserved0;
};

struct OutBound_SRB {
	uint32_t addressLow;	/* pointer to SRB block */
	uint32_t addressHigh;
};

struct arc_HBD_Msgu {
 	struct InBound_SRB post_qbuffer[ARCMSR_MAX_HBD_POSTQUEUE];
   	struct OutBound_SRB done_qbuffer[ARCMSR_MAX_HBD_POSTQUEUE+1];
	uint16_t postq_index;
	uint16_t doneq_index;
};

#define		ARC_HBD_CMDQ_PTR_LEN	sizeof(struct arc_HBD_Msgu)

struct arc_HBE_Msgu {
	uint16_t	cmdFlag;
	uint16_t	cmdSMID;
	uint16_t	cmdLMID;        // reserved (0)
	uint16_t	cmdFlag2;       // reserved (0)
};

#define		ARC_HBE_COMPLETION_Q_LEN	(sizeof(struct arc_HBE_Msgu) * ARCMSR_MAX_HBE_COMPLETION_Q + 128)
#define		ARC_HBF_COMPLETION_Q_LEN	ARC_HBE_COMPLETION_Q_LEN
#define		ARC_HBF_MESG_BUFFER_SIZE	(256 * 3)

/*
 * autoconf(9) glue.
 */
TAILQ_HEAD(arc_ccb_list, arc_ccb);

typedef struct arc_edata {
	envsys_data_t	arc_sensor;
	int		arc_diskid;
	int		arc_volid;
} arc_edata_t;

struct arc_softc {
	struct scsipi_channel	sc_chan;
	struct scsipi_adapter	sc_adapter;

	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_tag;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_size_t		sc_ios;
	bus_space_tag_t		sc_msgt;
	bus_space_handle_t	sc_msgh;
	bus_size_t		sc_msgs;
	bus_dma_tag_t		sc_dmat;

	void			*sc_ih;

	int			sc_req_count;

	struct arc_dmamem	*sc_requests;
	struct arc_ccb		*sc_ccbs;
	struct arc_ccb_list	sc_ccb_free;

	struct lwp		*sc_lwp;
	volatile int		sc_talking;
	kmutex_t		sc_mutex;
	kcondvar_t		sc_condvar;
	krwlock_t		sc_rwlock;

	struct sysmon_envsys	*sc_sme;
	arc_edata_t		*sc_arc_sensors;
	int			sc_nsensors;

	size_t			sc_maxraidset;	/* max raid sets */
	size_t 			sc_maxvolset;	/* max volume sets */
	size_t 			sc_cchans;	/* connected channels */

	device_t		sc_dev;		/* self */
	device_t		sc_scsibus_dv;

	uint16_t		sc_adp_type;
	uint16_t		ab_product;
	uint32_t		sc_ccb_phys_lo;
	uint32_t		sc_ccb_phys_hi;
	union {
		struct arc_HBB_Msgu	*pmuB;
		struct arc_HBD_Msgu	*pmuD;
		struct arc_HBE_Msgu	*pmuE;
		struct arc_HBE_Msgu	*pmuF;
	};
	bus_size_t		inb_doorbell;
	bus_size_t		inb_doorbell_mask;
	bus_size_t		outb_doorbell;
	bus_size_t		outb_doorbell_mask;
	uint32_t		sc_int_mask;
	uint32_t		postQ_buffer;
	uint32_t		doneQ_buffer;
	uint32_t		doneq_index;
	bus_addr_t		cmdQ_ptr_offset;
	uint32_t		doneq_size;
	uint32_t		doneq_entry;
	uint32_t		doneq_phys_lo;
	uint32_t		doneq_phys_hi;
	uint32_t		in_doorbell;
	uint32_t		out_doorbell;
	uint32_t		*message_wbuffer;
	uint32_t		*message_rbuffer;
	uint32_t		*msgcode_rwbuffer;
	uint32_t		msgBuf_phys_lo;
	uint32_t		msgBuf_phys_hi;
};

/* 
 * wrap up the bus_dma api.
 */
struct arc_dmamem {
	bus_dmamap_t		adm_map;
	bus_dma_segment_t	adm_seg;
	size_t			adm_size;
	void			*adm_kva;
};
#define ARC_DMA_MAP(_adm)	((_adm)->adm_map)
#define ARC_DMA_DVA(_adm)	((_adm)->adm_map->dm_segs[0].ds_addr)
#define ARC_DMA_KVA(_adm)	((void *)(_adm)->adm_kva)

struct arc_board {
	uint16_t		ab_vendor;
	uint16_t		ab_product;
	uint16_t		ab_io_type;
};

#endif /* ! _PCI_ARCMSRVAR_H_ */
