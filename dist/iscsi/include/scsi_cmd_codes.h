/* taken from http://www.arkeia.com/resources/scsi_rsc.html */

#ifndef SCSI_CMD_CODES_H_
#define SCSI_CMD_CODES_H_

enum {
	TEST_UNIT_READY = 0x00,
	WRITE_6 = 0x06,
	READ_6 = 0x08,
	INQUIRY = 0x12,
	MODE_SENSE_6 = 0x1a,
	STOP_START_UNIT = 0x1b,
	READ_CAPACITY = 0x25,
	READ_10 = 0x28,
	WRITE_10 = 0x2a,
	VERIFY = 0x2f,
	SYNC_CACHE = 0x35,
	LOG_SENSE = 0x4d,
	MODE_SENSE_10 = 0x5a,
	PERSISTENT_RESERVE_IN = 0x5e,
	REPORT_LUNS = 0xa0
};

#define SIX_BYTE_COMMAND(op)	((op) <= 0x1f)
#define TEN_BYTE_COMMAND(op)	((op) > 0x1f && (op) <= 0x5f)

#define ISCSI_MODE_SENSE_LEN	11

#endif /* !SCSI_CMD_CODES_H_ */
