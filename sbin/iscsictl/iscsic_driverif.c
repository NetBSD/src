/*	$NetBSD: iscsic_driverif.c,v 1.4 2012/06/20 08:19:49 martin Exp $	*/

/*-
 * Copyright (c) 2005,2006,2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "iscsic_globals.h"

#include <ctype.h>
#include <inttypes.h>

typedef struct {
	uint8_t		 asc;
	uint8_t		 ascq;
	const char	*key;
} asc_tab_t;

asc_tab_t asctab[] = {
	{0x00, 0x01, "FILEMARK DETECTED"},
	{0x00, 0x02, "END-OF-PARTITION/MEDIUM DETECTED"},
	{0x00, 0x03, "SETMARK DETECTED"},
	{0x00, 0x04, "BEGINNING-OF-PARTITION/MEDIUM DETECTED"},
	{0x00, 0x05, "END-OF-DATA DETECTED"},
	{0x00, 0x06, "I/O PROCESS TERMINATED"},
	{0x00, 0x11, "AUDIO PLAY OPERATION IN PROGRESS"},
	{0x00, 0x12, "AUDIO PLAY OPERATION PAUSED"},
	{0x00, 0x13, "AUDIO PLAY OPERATION SUCCESSFULLY COMPLETED"},
	{0x00, 0x14, "AUDIO PLAY OPERATION STOPPED DUE TO ERROR"},
	{0x00, 0x15, "NO CURRENT AUDIO STATUS TO RETURN"},
	{0x01, 0x00, "NO INDEX/SECTOR SIGNAL"},
	{0x02, 0x00, "NO SEEK COMPLETE"},
	{0x03, 0x00, "PERIPHERAL DEVICE WRITE FAULT"},
	{0x03, 0x01, "NO WRITE CURRENT"},
	{0x03, 0x02, "EXCESSIVE WRITE ERRORS"},
	{0x04, 0x00, "LOGICAL UNIT NOT READY, CAUSE NOT REPORTABLE"},
	{0x04, 0x01, "LOGICAL UNIT IS IN PROCESS OF BECOMING READY"},
	{0x04, 0x02, "LOGICAL UNIT NOT READY, INITIALIZING COMMAND REQUIRED"},
	{0x04, 0x03, "LOGICAL UNIT NOT READY, MANUAL INTERVENTION REQUIRED"},
	{0x04, 0x04, "LOGICAL UNIT NOT READY, FORMAT IN PROGRESS"},
	{0x05, 0x00, "LOGICAL UNIT DOES NOT RESPOND TO SELECTION"},
	{0x06, 0x00, "NO REFERENCE POSITION FOUND"},
	{0x07, 0x00, "MULTIPLE PERIPHERAL DEVICES SELECTED"},
	{0x08, 0x00, "LOGICAL UNIT COMMUNICATION FAILURE"},
	{0x08, 0x01, "LOGICAL UNIT COMMUNICATION TIME-OUT"},
	{0x08, 0x02, "LOGICAL UNIT COMMUNICATION PARITY ERROR"},
	{0x09, 0x00, "TRACK FOLLOWING ERROR"},
	{0x09, 0x01, "TRACKING SERVO FAILURE"},
	{0x09, 0x02, "FOCUS SERVO FAILURE"},
	{0x09, 0x03, "SPINDLE SERVO FAILURE"},
	{0x0A, 0x00, "ERROR LOG OVERFLOW"},
	{0x0C, 0x00, "WRITE ERROR"},
	{0x0C, 0x01, "WRITE ERROR RECOVERED WITH AUTO REALLOCATION"},
	{0x0C, 0x02, "WRITE ERROR - AUTO REALLOCATION FAILED"},
	{0x10, 0x00, "ID CRC OR ECC ERROR"},
	{0x11, 0x00, "UNRECOVERED READ ERROR"},
	{0x11, 0x01, "READ RETRIES EXHAUSTED"},
	{0x11, 0x02, "ERROR TOO LONG TO CORRECT"},
	{0x11, 0x03, "MULTIPLE READ ERRORS"},
	{0x11, 0x04, "UNRECOVERED READ ERROR - AUTO REALLOCATE FAILED"},
	{0x11, 0x05, "L-EC UNCORRECTABLE ERROR"},
	{0x11, 0x06, "CIRC UNRECOVERED ERROR"},
	{0x11, 0x07, "DATA RESYNCHRONIZATION ERROR"},
	{0x11, 0x08, "INCOMPLETE BLOCK READ"},
	{0x11, 0x09, "NO GAP FOUND"},
	{0x11, 0x0A, "MISCORRECTED ERROR"},
	{0x11, 0x0B, "UNRECOVERED READ ERROR - RECOMMEND REASSIGNMENT"},
	{0x11, 0x0C, "UNRECOVERED READ ERROR - RECOMMEND REWRITE THE DATA"},
	{0x12, 0x00, "ADDRESS MARK NOT FOUND FOR ID FIELD"},
	{0x13, 0x00, "ADDRESS MARK NOT FOUND FOR DATA FIELD"},
	{0x14, 0x00, "RECORDED ENTITY NOT FOUND"},
	{0x14, 0x01, "RECORD NOT FOUND"},
	{0x14, 0x02, "FILEMARK OR SETMARK NOT FOUND"},
	{0x14, 0x03, "END-OF-DATA NOT FOUND"},
	{0x14, 0x04, "BLOCK SEQUENCE ERROR"},
	{0x15, 0x00, "RANDOM POSITIONING ERROR"},
	{0x15, 0x01, "MECHANICAL POSITIONING ERROR"},
	{0x15, 0x02, "POSITIONING ERROR DETECTED BY READ OF MEDIUM"},
	{0x16, 0x00, "DATA SYNCHRONIZATION MARK ERROR"},
	{0x17, 0x00, "RECOVERED DATA WITH NO ERROR CORRECTION APPLIED"},
	{0x17, 0x01, "RECOVERED DATA WITH RETRIES"},
	{0x17, 0x02, "RECOVERED DATA WITH POSITIVE HEAD OFFSET"},
	{0x17, 0x03, "RECOVERED DATA WITH NEGATIVE HEAD OFFSET"},
	{0x17, 0x04, "RECOVERED DATA WITH RETRIES AND/OR CIRC APPLIED"},
	{0x17, 0x05, "RECOVERED DATA USING PREVIOUS SECTOR ID"},
	{0x17, 0x06, "RECOVERED DATA WITHOUT ECC - DATA AUTO-REALLOCATED"},
	{0x17, 0x07, "RECOVERED DATA WITHOUT ECC - RECOMMEND REASSIGNMENT"},
	{0x17, 0x08, "RECOVERED DATA WITHOUT ECC - RECOMMEND REWRITE"},
	{0x18, 0x00, "RECOVERED DATA WITH ERROR CORRECTION APPLIED"},
	{0x18, 0x01, "RECOVERED DATA WITH ERROR CORRECTION & RETRIES APPLIED"},
	{0x18, 0x02, "RECOVERED DATA - DATA AUTO-REALLOCATED"},
	{0x18, 0x03, "RECOVERED DATA WITH CIRC"},
	{0x18, 0x04, "RECOVERED DATA WITH L-EC"},
	{0x18, 0x05, "RECOVERED DATA - RECOMMEND REASSIGNMENT"},
	{0x18, 0x06, "RECOVERED DATA - RECOMMEND REWRITE"},
	{0x19, 0x00, "DEFECT LIST ERROR"},
	{0x19, 0x01, "DEFECT LIST NOT AVAILABLE"},
	{0x19, 0x02, "DEFECT LIST ERROR IN PRIMARY LIST"},
	{0x19, 0x03, "DEFECT LIST ERROR IN GROWN LIST"},
	{0x1A, 0x00, "PARAMETER LIST LENGTH ERROR"},
	{0x1B, 0x00, "SYNCHRONOUS DATA TRANSFER ERROR"},
	{0x1C, 0x00, "DEFECT LIST NOT FOUND"},
	{0x1C, 0x01, "PRIMARY DEFECT LIST NOT FOUND"},
	{0x1C, 0x02, "GROWN DEFECT LIST NOT FOUND"},
	{0x1D, 0x00, "MISCOMPARE DURING VERIFY OPERATION"},
	{0x1E, 0x00, "RECOVERED ID WITH ECC CORRECTION"},
	{0x20, 0x00, "INVALID COMMAND OPERATION CODE"},
	{0x21, 0x00, "LOGICAL BLOCK ADDRESS OUT OF RANGE"},
	{0x21, 0x01, "INVALID ELEMENT ADDRESS"},
	{0x22, 0x00, "ILLEGAL FUNCTION"},
	{0x24, 0x00, "INVALID FIELD IN CDB"},
	{0x25, 0x00, "LOGICAL UNIT NOT SUPPORTED"},
	{0x26, 0x00, "INVALID FIELD IN PARAMETER LIST"},
	{0x26, 0x01, "PARAMETER NOT SUPPORTED"},
	{0x26, 0x02, "PARAMETER VALUE INVALID"},
	{0x26, 0x03, "THRESHOLD PARAMETERS NOT SUPPORTED"},
	{0x27, 0x00, "WRITE PROTECTED"},
	{0x28, 0x00, "NOT READY TO READY TRANSITION, MEDIUM MAY HAVE CHANGED"},
	{0x28, 0x01, "IMPORT OR EXPORT ELEMENT ACCESSED"},
	{0x29, 0x00, "POWER ON, RESET, OR BUS DEVICE RESET OCCURRED"},
	{0x2A, 0x00, "PARAMETERS CHANGED"},
	{0x2A, 0x01, "MODE PARAMETERS CHANGED"},
	{0x2A, 0x02, "LOG PARAMETERS CHANGED"},
	{0x2B, 0x00, "COPY CANNOT EXECUTE SINCE HOST CANNOT DISCONNECT"},
	{0x2C, 0x00, "COMMAND SEQUENCE ERROR"},
	{0x2C, 0x01, "TOO MANY WINDOWS SPECIFIED"},
	{0x2C, 0x02, "INVALID COMBINATION OF WINDOWS SPECIFIED"},
	{0x2D, 0x00, "OVERWRITE ERROR ON UPDATE IN PLACE"},
	{0x2F, 0x00, "COMMANDS CLEARED BY ANOTHER INITIATOR"},
	{0x30, 0x00, "INCOMPATIBLE MEDIUM INSTALLED"},
	{0x30, 0x01, "CANNOT READ MEDIUM - UNKNOWN FORMAT"},
	{0x30, 0x02, "CANNOT READ MEDIUM - INCOMPATIBLE FORMAT"},
	{0x30, 0x03, "CLEANING CARTRIDGE INSTALLED"},
	{0x31, 0x00, "MEDIUM FORMAT CORRUPTED"},
	{0x31, 0x01, "FORMAT COMMAND FAILED"},
	{0x32, 0x00, "NO DEFECT SPARE LOCATION AVAILABLE"},
	{0x32, 0x01, "DEFECT LIST UPDATE FAILURE"},
	{0x33, 0x00, "TAPE LENGTH ERROR"},
	{0x36, 0x00, "RIBBON, INK, OR TONER FAILURE"},
	{0x37, 0x00, "ROUNDED PARAMETER"},
	{0x39, 0x00, "SAVING PARAMETERS NOT SUPPORTED"},
	{0x3A, 0x00, "MEDIUM NOT PRESENT"},
	{0x3B, 0x00, "SEQUENTIAL POSITIONING ERROR"},
	{0x3B, 0x01, "TAPE POSITION ERROR AT BEGINNING-OF-MEDIUM"},
	{0x3B, 0x02, "TAPE POSITION ERROR AT END-OF-MEDIUM"},
	{0x3B, 0x03, "TAPE OR ELECTRONIC VERTICAL FORMS UNIT NOT READY"},
	{0x3B, 0x04, "SLEW FAILURE"},
	{0x3B, 0x05, "PAPER JAM"},
	{0x3B, 0x06, "FAILED TO SENSE TOP-OF-FORM"},
	{0x3B, 0x07, "FAILED TO SENSE BOTTOM-OF-FORM"},
	{0x3B, 0x08, "REPOSITION ERROR"},
	{0x3B, 0x09, "READ PAST END OF MEDIUM"},
	{0x3B, 0x0A, "READ PAST BEGINNING OF MEDIUM"},
	{0x3B, 0x0B, "POSITION PAST END OF MEDIUM"},
	{0x3B, 0x0C, "POSITION PAST BEGINNING OF MEDIUM"},
	{0x3B, 0x0D, "MEDIUM DESTINATION ELEMENT FULL"},
	{0x3B, 0x0E, "MEDIUM SOURCE ELEMENT EMPTY"},
	{0x3D, 0x00, "INVALID BITS IN IDENTIFY MESSAGE"},
	{0x3E, 0x00, "LOGICAL UNIT HAS NOT SELF-CONFIGURED YET"},
	{0x3F, 0x00, "TARGET OPERATING CONDITIONS HAVE CHANGED"},
	{0x3F, 0x01, "MICROCODE HAS BEEN CHANGED"},
	{0x3F, 0x02, "LPWRSOMC CHANGED OPERATING DEFINITION"},
	{0x3F, 0x03, "INQUIRY DATA HAS CHANGED"},
	{0x40, 0x00, "RAM FAILURE (SHOULD USE 40 NN)"},
	{0x41, 0x00, "DATA PATH FAILURE (SHOULD USE 40 NN)"},
	{0x42, 0x00, "POWER-ON OR SELF-TEST FAILURE (SHOULD USE 40 NN)"},
	{0x43, 0x00, "MESSAGE ERROR"},
	{0x44, 0x00, "INTERNAL TARGET FAILURE"},
	{0x45, 0x00, "SELECT OR RESELECT FAILURE"},
	{0x46, 0x00, "UNSUCCESSFUL SOFT RESET"},
	{0x47, 0x00, "SCSI PARITY ERROR"},
	{0x48, 0x00, "INITIATOR DETECTED ERROR MESSAGE RECEIVED"},
	{0x49, 0x00, "INVALID MESSAGE ERROR"},
	{0x4A, 0x00, "COMMAND PHASE ERROR"},
	{0x4B, 0x00, "DATA PHASE ERROR"},
	{0x4C, 0x00, "LOGICAL UNIT FAILED SELF-CONFIGURATION"},
	{0x4E, 0x00, "OVERLAPPED COMMANDS ATTEMPTED"},
	{0x50, 0x00, "WRITE APPEND ERROR"},
	{0x50, 0x01, "WRITE APPEND POSITION ERROR"},
	{0x50, 0x02, "POSITION ERROR RELATED TO TIMING"},
	{0x51, 0x00, "ERASE FAILURE"},
	{0x52, 0x00, "CARTRIDGE FAULT"},
	{0x53, 0x00, "MEDIA LOAD OR EJECT FAILED"},
	{0x53, 0x01, "UNLOAD TAPE FAILURE"},
	{0x53, 0x02, "MEDIUM REMOVAL PREVENTED"},
	{0x54, 0x00, "SCSI TO HOST SYSTEM INTERFACE FAILURE"},
	{0x55, 0x00, "SYSTEM RESOURCE FAILURE"},
	{0x57, 0x00, "UNABLE TO RECOVER TABLE-OF-CONTENTS"},
	{0x58, 0x00, "GENERATION DOES NOT EXIST"},
	{0x59, 0x00, "UPDATED BLOCK READ"},
	{0x5A, 0x00, "OPERATOR REQUEST OR STATE CHANGE INPUT (UNSPECIFIED)"},
	{0x5A, 0x01, "OPERATOR MEDIUM REMOVAL REQUEST"},
	{0x5A, 0x02, "OPERATOR SELECTED WRITE PROTECT"},
	{0x5A, 0x03, "OPERATOR SELECTED WRITE PERMIT"},
	{0x5B, 0x00, "LOG EXCEPTION"},
	{0x5B, 0x01, "THRESHOLD CONDITION MET"},
	{0x5B, 0x02, "LOG COUNTER AT MAXIMUM"},
	{0x5B, 0x03, "LOG LIST CODES EXHAUSTED"},
	{0x5C, 0x00, "RPL STATUS CHANGE"},
	{0x5C, 0x01, "SPINDLES SYNCHRONIZED"},
	{0x5C, 0x02, "SPINDLES NOT SYNCHRONIZED"},
	{0x60, 0x00, "LAMP FAILURE"},
	{0x61, 0x00, "VIDEO ACQUISITION ERROR"},
	{0x61, 0x01, "UNABLE TO ACQUIRE VIDEO"},
	{0x61, 0x02, "OUT OF FOCUS"},
	{0x62, 0x00, "SCAN HEAD POSITIONING ERROR"},
	{0x63, 0x00, "END OF USER AREA ENCOUNTERED ON THIS TRACK"},
	{0x64, 0x00, "ILLEGAL MODE FOR THIS TRACK"},
	{0x00, 0x00, NULL}
};

/*
 * get_sessid:
 *    Get session ID. Queries Daemon for numeric ID if the user specifies
 *    a symbolic session name.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    session ID if OK - else it doesn't return at all.
 */

uint32_t
get_sessid(int argc, char **argv, int optional)
{
	iscsid_sym_id_t sid;
	iscsid_search_list_req_t srch;
	iscsid_response_t *rsp;

	if (!cl_get_id('I', &sid, argc, argv)) {
		if (!optional) {
			arg_missing("Session ID");
		}
		return 0;
	}

	if (!sid.id) {
		srch.list_kind = SESSION_LIST;
		srch.search_kind = FIND_NAME;
		strlcpy((char *)srch.strval, (char *)sid.name, sizeof(srch.strval));
		srch.intval = 0;

		send_request(ISCSID_SEARCH_LIST, sizeof(srch), &srch);
		rsp = get_response(FALSE);
		if (rsp->status)
			status_error_slist(rsp->status);

		GET_SYM_ID(sid.id, rsp->parameter);
		free_response(rsp);
	}
	return sid.id;
}


/*
 * dump_data:
 *    Displays the returned data in hex and ASCII format.
 *
 *    Parameter:
 *       title    Title text for Dump.
 *       buf      Buffer to dump.
 *       len      Number of bytes in buffer.
 */

void
dump_data(const char *title, const void *buffer, size_t len)
{
	const uint8_t *bp = buffer;
	size_t i, nelem;

	printf("%s\n", title);

	while (len > 0) {
		nelem = min(16, len);
		printf("  ");

		for (i = 0; i < nelem; i++) {
			if (i >= len)
				printf("   ");
			else
				printf("%02x ", bp[i]);
		}
		for (i = nelem; i < 16; i++) {
			printf("   ");
		}
		printf(" '");
		for (i = 0; i < nelem; i++) {
			if (i >= len)
				break;
			printf("%c", isprint(bp[i]) ? bp[i] : ' ');
		}
		printf("'\n");
		if (len < 16)
			break;
		len -= 16;
		bp += 16;
	}
}


/*
 * add_asc_info:
 *    Examines additional sense code and qualifier, adds text to given string
 *    if it's in the table.
 *
 *    Parameter:
 *       str      The existing sense code string
 *       asc      Additional sense code
 *       ascq     Additional sense code qualifier
 *
 *    Returns: the result string.
 */

STATIC char *
add_asc_info(char *str, uint8_t asc, uint8_t ascq)
{
	asc_tab_t *pt;
	char *bp;

	for (pt = asctab; pt->key != NULL && asc >= pt->asc; pt++) {
		if (asc == pt->asc && ascq == pt->ascq) {
			bp = (char *)&buf[1024];
			snprintf(bp, sizeof(buf) - 1024, "%s: %s",
					str, pt->key);
			return bp;
		}
	}
	return str;
}


/*
 * do_ioctl:
 *    Executes the I/O command, evaluates the return code, and displays
 *    sense data if the command failed.
 *
 *    Parameter:
 *       io       The i/o command parameters.
 *
 *    Returns: 0 if OK, else an error code.
 */

int
do_ioctl(iscsi_iocommand_parameters_t * io, int rd)
{
	char	*esp;
	char	 es[64];
	int	 rc;

	io->req.databuf = buf;
	io->req.senselen = sizeof(io->req.sense);
	io->req.senselen_used = 0;
	io->req.flags = (rd) ? SCCMD_READ : SCCMD_WRITE;

	rc = ioctl(driver, ISCSI_IO_COMMAND, io);

	if (io->req.senselen_used) {
		switch (io->req.sense[2] & 0x0f) {
		case 0x01:
			snprintf(esp = es, sizeof(es), "Recovered Error");
			break;
		case 0x02:
			snprintf(esp = es, sizeof(es), "Not Ready");
			break;
		case 0x03:
			snprintf(esp = es, sizeof(es), "Medium Error");
			break;
		case 0x04:
			snprintf(esp = es, sizeof(es), "Hardware Error");
			break;
		case 0x05:
			snprintf(esp = es, sizeof(es), "Illegal Request");
			break;
		case 0x06:
			snprintf(esp = es, sizeof(es), "Unit Attention");
			break;
		case 0x07:
			snprintf(esp = es, sizeof(es), "Data Protect");
			break;
		case 0x08:
			snprintf(esp = es, sizeof(es), "Blank Check");
			break;
		default:
			snprintf(esp = (char *)&buf[256], sizeof(buf) - 256, "Sense key 0x%x",
				io->req.sense[2] & 0x0f);
			break;
		}
		if (io->req.senselen_used >= 14) {
			add_asc_info(esp, io->req.sense[12], io->req.sense[13]);
		}
		snprintf((char *)buf, sizeof(buf), "Sense Data (%s):", esp);
		dump_data((char *)buf, io->req.sense, io->req.senselen_used);
		return io->req.retsts;
	}

	if (io->status) {
		status_error(io->status);
	}
	if (rc) {
		io_error("I/O command");
	}
	return 0;
}


/*
 * inquiry:
 *    Handle the inquiry command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
inquiry(int argc, char **argv)
{
	iscsi_iocommand_parameters_t io;
	char opt;
	int pag, rc;

	(void) memset(&io, 0x0, sizeof(io));
	if ((io.session_id = get_sessid(argc, argv, FALSE)) == 0) {
		return 1;
	}
	io.lun = cl_get_longlong('l', argc, argv);

	opt = cl_get_char('d', argc, argv);
	switch (opt) {
	case 0:
	case '0':
		opt = 0;
		break;

	case 'p':
	case 'P':
		io.req.cmd[1] = 0x01;
		opt = 1;
		break;

	case 'c':
	case 'C':
		io.req.cmd[1] = 0x02;
		opt = 2;
		break;

	default:
		gen_error("Invalid detail option '%c'\n", opt);
	}

	pag = cl_get_int('p', argc, argv);

	check_extra_args(argc, argv);

	io.req.cmdlen = 6;
	io.req.cmd[0] = 0x12;
	io.req.cmd[2] = (uint8_t) pag;
	io.req.cmd[4] = 0xff;

	io.req.datalen = 0xff;

	if ((rc = do_ioctl(&io, TRUE)) != 0) {
		return rc;
	}
	if (!io.req.datalen_used) {
		printf("No Data!\n");
		return 1;
	}
	dump_data("Inquiry Data:", buf, io.req.datalen_used);
	return 0;
}


/*
 * read_capacity:
 *    Handle the read_capacity command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
read_capacity(int argc, char **argv)
{
	iscsi_iocommand_parameters_t io;
	int rc;
	uint32_t bsz;
	uint64_t lbn, cap;
	uint32_t n;

	(void) memset(&io, 0x0, sizeof(io));
	if ((io.session_id = get_sessid(argc, argv, FALSE)) == 0) {
		return 1;
	}
	io.lun = cl_get_longlong('l', argc, argv);
	check_extra_args(argc, argv);

	io.req.cmdlen = 10;
	io.req.cmd[0] = 0x25;

	io.req.datalen = 8;

	if ((rc = do_ioctl(&io, TRUE)) != 0) {
		return rc;
	}
	(void) memcpy(&n, buf, sizeof(n));
	lbn = (uint64_t)(n + 1);
	(void) memcpy(&n, &buf[4], sizeof(n));
	bsz = ntohl(n);
	cap = lbn * bsz;
	printf("Total Blocks: %" PRIu64 ", Block Size: %u, Capacity: %" PRIu64 " Bytes\n",
		lbn, bsz, cap);

	return 0;
}


/*
 * test_unit_ready:
 *    Handle the test_unit_ready command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
test_unit_ready(int argc, char **argv)
{
	iscsi_iocommand_parameters_t io;
	int rc;

	(void) memset(&io, 0x0, sizeof(io));
	if ((io.session_id = get_sessid(argc, argv, FALSE)) == 0) {
		return 1;
	}
	io.lun = cl_get_longlong('l', argc, argv);
	check_extra_args(argc, argv);

	io.req.cmdlen = 6;
	io.req.cmd[0] = 0x00;

	io.req.datalen = 0;

	if ((rc = do_ioctl(&io, TRUE)) != 0) {
		return rc;
	}
	printf("Unit is ready\n");
	return 0;
}


/*
 * report_luns:
 *    Handle the report_luns command.
 *
 *    Parameter:  argc, argv (shifted)
 *
 *    Returns:    0 if OK - else it doesn't return at all.
 */

int
report_luns(int argc, char **argv)
{
	iscsi_iocommand_parameters_t io;
	int rc;
	size_t llen;
	uint32_t n;
	uint16_t n2;
	uint64_t *lp;

	(void) memset(&io, 0x0, sizeof(io));
	if ((io.session_id = get_sessid(argc, argv, FALSE)) == 0) {
		return 1;
	}
	check_extra_args(argc, argv);

	io.req.cmdlen = 12;
	io.req.cmd[0] = 0xa0;
	n = htonl(sizeof(buf));
	(void) memcpy(&io.req.cmd[6], &n, sizeof(n));

	io.req.datalen = sizeof(buf);

	if ((rc = do_ioctl(&io, TRUE)) != 0) {
		return rc;
	}
	(void) memcpy(&n2, buf, sizeof(n2));
	llen = ntohs(n2);
	if (!llen) {
		printf("No LUNs!\n");
		return 1;
	}
	if (llen + 8 > sizeof(buf))
		printf("Partial ");
	printf("LUN List:\n");
	lp = (uint64_t *)(void *) &buf[8];

	for (llen = min(llen, sizeof(buf) - 8) / 8; llen; llen--) {
		printf("  0x%" PRIx64 "\n", ntohq(*lp));
		lp++;
	}
	return 0;
}
