/*	$NetBSD: scsi_sense.c,v 1.4 2003/05/17 23:03:28 itojun Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * SCSI sense interpretation.  Lifted from src/sys/dev/scsipi/scsi_verbose.c,
 * and modified for userland.
 *
 * XXX THESE SHOULD BE IN A LIBRARY!
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/scsiio.h>
#include <stdio.h>
#include <string.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/scsiconf.h>

#include "extern.h"

static const char *sense_keys[16] = {
	"No Additional Sense",
	"Recovered Error",
	"Not Ready",
	"Media Error",
	"Hardware Error",
	"Illegal Request",
	"Unit Attention",
	"Write Protected",
	"Blank Check",
	"Vendor Unique",
	"Copy Aborted",
	"Aborted Command",
	"Equal Error",
	"Volume Overflow",
	"Miscompare Error",
	"Reserved"
};

static const struct {
	unsigned char asc;
	unsigned char ascq;
	const char *description;
} adesc[] = {
{ 0x00, 0x00, "No Additional Sense Information" },
{ 0x00, 0x01, "Filemark Detected" },
{ 0x00, 0x02, "End-Of-Partition/Medium Detected" },
{ 0x00, 0x03, "Setmark Detected" },
{ 0x00, 0x04, "Beginning-Of-Partition/Medium Detected" },
{ 0x00, 0x05, "End-Of-Data Detected" },
{ 0x00, 0x06, "I/O Process Terminated" },
{ 0x00, 0x11, "Audio Play Operation In Progress" },
{ 0x00, 0x12, "Audio Play Operation Paused" },
{ 0x00, 0x13, "Audio Play Operation Successfully Completed" },
{ 0x00, 0x14, "Audio Play Operation Stopped Due to Error" },
{ 0x00, 0x15, "No Current Audio Status To Return" },
{ 0x01, 0x00, "No Index/Sector Signal" },
{ 0x02, 0x00, "No Seek Complete" },
{ 0x03, 0x00, "Peripheral Device Write Fault" },
{ 0x03, 0x01, "No Write Current" },
{ 0x03, 0x02, "Excessive Write Errors" },
{ 0x04, 0x00, "Logical Unit Not Ready, Cause Not Reportable" },
{ 0x04, 0x01, "Logical Unit Is in Process Of Becoming Ready" },
{ 0x04, 0x02, "Logical Unit Not Ready, Initialization Command Required" },
{ 0x04, 0x03, "Logical Unit Not Ready, Manual Intervention Required" },
{ 0x04, 0x04, "Logical Unit Not Ready, Format In Progress" },
{ 0x05, 0x00, "Logical Unit Does Not Respond To Selection" },
{ 0x06, 0x00, "No Reference Position Found" },
{ 0x07, 0x00, "Multiple Peripheral Devices Selected" },
{ 0x08, 0x00, "Logical Unit Communication Failure" },
{ 0x08, 0x01, "Logical Unit Communication Timeout" },
{ 0x08, 0x02, "Logical Unit Communication Parity Error" },
{ 0x09, 0x00, "Track Following Error" },
{ 0x09, 0x01, "Tracking Servo Failure" },
{ 0x09, 0x02, "Focus Servo Failure" },
{ 0x09, 0x03, "Spindle Servo Failure" },
{ 0x0A, 0x00, "Error Log Overflow" },
{ 0x0C, 0x00, "Write Error" },
{ 0x0C, 0x01, "Write Error Recovered with Auto Reallocation" },
{ 0x0C, 0x02, "Write Error - Auto Reallocate Failed" },
{ 0x10, 0x00, "ID CRC Or ECC Error" },
{ 0x11, 0x00, "Unrecovered Read Error" },
{ 0x11, 0x01, "Read Retried Exhausted" },
{ 0x11, 0x02, "Error Too Long To Correct" },
{ 0x11, 0x03, "Multiple Read Errors" },
{ 0x11, 0x04, "Unrecovered Read Error - Auto Reallocate Failed" },
{ 0x11, 0x05, "L-EC Uncorrectable Error" },
{ 0x11, 0x06, "CIRC Unrecovered Error" },
{ 0x11, 0x07, "Data Resynchronization Error" },
{ 0x11, 0x08, "Incomplete Block Found" },
{ 0x11, 0x09, "No Gap Found" },
{ 0x11, 0x0A, "Miscorrected Error" },
{ 0x11, 0x0B, "Uncorrected Read Error - Recommend Reassignment" },
{ 0x11, 0x0C, "Uncorrected Read Error - Recommend Rewrite the Data" },
{ 0x12, 0x00, "Address Mark Not Found for ID Field" },
{ 0x13, 0x00, "Address Mark Not Found for Data Field" },
{ 0x14, 0x00, "Recorded Entity Not Found" },
{ 0x14, 0x01, "Record Not Found" },
{ 0x14, 0x02, "Filemark or Setmark Not Found" },
{ 0x14, 0x03, "End-Of-Data Not Found" },
{ 0x14, 0x04, "Block Sequence Error" },
{ 0x15, 0x00, "Random Positioning Error" },
{ 0x15, 0x01, "Mechanical Positioning Error" },
{ 0x15, 0x02, "Positioning Error Detected By Read of Medium" },
{ 0x16, 0x00, "Data Synchronization Mark Error" },
{ 0x17, 0x00, "Recovered Data With No Error Correction Applied" },
{ 0x17, 0x01, "Recovered Data With Retries" },
{ 0x17, 0x02, "Recovered Data With Positive Head Offset" },
{ 0x17, 0x03, "Recovered Data With Negative Head Offset" },
{ 0x17, 0x04, "Recovered Data With Retries and/or CIRC Applied" },
{ 0x17, 0x05, "Recovered Data Using Previous Sector ID" },
{ 0x17, 0x06, "Recovered Data Without ECC - Data Auto-Reallocated" },
{ 0x17, 0x07, "Recovered Data Without ECC - Recommend Reassignment" },
{ 0x17, 0x08, "Recovered Data Without ECC - Recommend Rewrite" },
{ 0x18, 0x00, "Recovered Data With Error Correction Applied" },
{ 0x18, 0x01, "Recovered Data With Error Correction & Retries Applied" },
{ 0x18, 0x02, "Recovered Data - Data Auto-Reallocated" },
{ 0x18, 0x03, "Recovered Data With CIRC" },
{ 0x18, 0x04, "Recovered Data With LEC" },
{ 0x18, 0x05, "Recovered Data - Recommend Reassignment" },
{ 0x18, 0x06, "Recovered Data - Recommend Rewrite" },
{ 0x19, 0x00, "Defect List Error" },
{ 0x19, 0x01, "Defect List Not Available" },
{ 0x19, 0x02, "Defect List Error in Primary List" },
{ 0x19, 0x03, "Defect List Error in Grown List" },
{ 0x1A, 0x00, "Parameter List Length Error" },
{ 0x1B, 0x00, "Synchronous Data Transfer Error" },
{ 0x1C, 0x00, "Defect List Not Found" },
{ 0x1C, 0x01, "Primary Defect List Not Found" },
{ 0x1C, 0x02, "Grown Defect List Not Found" },
{ 0x1D, 0x00, "Miscompare During Verify Operation" },
{ 0x1E, 0x00, "Recovered ID with ECC" },
{ 0x20, 0x00, "Invalid Command Operation Code" },
{ 0x21, 0x00, "Logical Block Address Out of Range" },
{ 0x21, 0x01, "Invalid Element Address" },
{ 0x22, 0x00, "Illegal Function (Should 20 00, 24 00, or 26 00)" },
{ 0x24, 0x00, "Illegal Field in CDB" },
{ 0x25, 0x00, "Logical Unit Not Supported" },
{ 0x26, 0x00, "Invalid Field In Parameter List" },
{ 0x26, 0x01, "Parameter Not Supported" },
{ 0x26, 0x02, "Parameter Value Invalid" },
{ 0x26, 0x03, "Threshold Parameters Not Supported" },
{ 0x27, 0x00, "Write Protected" },
{ 0x28, 0x00, "Not Ready To Ready Transition (Medium May Have Changed)" },
{ 0x28, 0x01, "Import Or Export Element Accessed" },
{ 0x29, 0x00, "Power On, Reset, or Bus Device Reset Occurred" },
{ 0x2A, 0x00, "Parameters Changed" },
{ 0x2A, 0x01, "Mode Parameters Changed" },
{ 0x2A, 0x02, "Log Parameters Changed" },
{ 0x2B, 0x00, "Copy Cannot Execute Since Host Cannot Disconnect" },
{ 0x2C, 0x00, "Command Sequence Error" },
{ 0x2C, 0x01, "Too Many Windows Specified" },
{ 0x2C, 0x02, "Invalid Combination of Windows Specified" },
{ 0x2D, 0x00, "Overwrite Error On Update In Place" },
{ 0x2F, 0x00, "Commands Cleared By Another Initiator" },
{ 0x30, 0x00, "Incompatible Medium Installed" },
{ 0x30, 0x01, "Cannot Read Medium - Unknown Format" },
{ 0x30, 0x02, "Cannot Read Medium - Incompatible Format" },
{ 0x30, 0x03, "Cleaning Cartridge Installed" },
{ 0x31, 0x00, "Medium Format Corrupted" },
{ 0x31, 0x01, "Format Command Failed" },
{ 0x32, 0x00, "No Defect Spare Location Available" },
{ 0x32, 0x01, "Defect List Update Failure" },
{ 0x33, 0x00, "Tape Length Error" },
{ 0x36, 0x00, "Ribbon, Ink, or Toner Failure" },
{ 0x37, 0x00, "Rounded Parameter" },
{ 0x39, 0x00, "Saving Parameters Not Supported" },
{ 0x3A, 0x00, "Medium Not Present" },
{ 0x3B, 0x00, "Positioning Error" },
{ 0x3B, 0x01, "Tape Position Error At Beginning-of-Medium" },
{ 0x3B, 0x02, "Tape Position Error At End-of-Medium" },
{ 0x3B, 0x03, "Tape or Electronic Vertical Forms Unit Not Ready" },
{ 0x3B, 0x04, "Slew Failure" },
{ 0x3B, 0x05, "Paper Jam" },
{ 0x3B, 0x06, "Failed To Sense Top-Of-Form" },
{ 0x3B, 0x07, "Failed To Sense Bottom-Of-Form" },
{ 0x3B, 0x08, "Reposition Error" },
{ 0x3B, 0x09, "Read Past End Of Medium" },
{ 0x3B, 0x0A, "Read Past Begining Of Medium" },
{ 0x3B, 0x0B, "Position Past End Of Medium" },
{ 0x3B, 0x0C, "Position Past Beginning Of Medium" },
{ 0x3B, 0x0D, "Medium Destination Element Full" },
{ 0x3B, 0x0E, "Medium Source Element Empty" },
{ 0x3D, 0x00, "Invalid Bits In IDENTIFY Message" },
{ 0x3E, 0x00, "Logical Unit Has Not Self-Configured Yet" },
{ 0x3F, 0x00, "Target Operating Conditions Have Changed" },
{ 0x3F, 0x01, "Microcode Has Changed" },
{ 0x3F, 0x02, "Changed Operating Definition" },
{ 0x3F, 0x03, "INQUIRY Data Has Changed" },
{ 0x40, 0x00, "RAM FAILURE (Should Use 40 NN)" },
{ 0x41, 0x00, "Data Path FAILURE (Should Use 40 NN)" },
{ 0x42, 0x00, "Power-On or Self-Test FAILURE (Should Use 40 NN)" },
{ 0x43, 0x00, "Message Error" },
{ 0x44, 0x00, "Internal Target Failure" },
{ 0x45, 0x00, "Select Or Reselect Failure" },
{ 0x46, 0x00, "Unsuccessful Soft Reset" },
{ 0x47, 0x00, "SCSI Parity Error" },
{ 0x48, 0x00, "INITIATOR DETECTED ERROR Message Received" },
{ 0x49, 0x00, "Invalid Message Error" },
{ 0x4A, 0x00, "Command Phase Error" },
{ 0x4B, 0x00, "Data Phase Error" },
{ 0x4C, 0x00, "Logical Unit Failed Self-Configuration" },
{ 0x4E, 0x00, "Overlapped Commands Attempted" },
{ 0x50, 0x00, "Write Append Error" },
{ 0x50, 0x01, "Write Append Position Error" },
{ 0x50, 0x01, "Write Append Position Error" },
{ 0x50, 0x02, "Position Error Related To Timing" },
{ 0x51, 0x00, "Erase Failure" },
{ 0x52, 0x00, "Cartridge Fault" },
{ 0x53, 0x00, "Media Load or Eject Failed" },
{ 0x53, 0x01, "Unload Tape Failure" },
{ 0x53, 0x02, "Medium Removal Prevented" },
{ 0x54, 0x00, "SCSI To Host System Interface Failure" },
{ 0x55, 0x00, "System Resource Failure" },
{ 0x57, 0x00, "Unable To Recover Table-Of-Contents" },
{ 0x58, 0x00, "Generation Does Not Exist" },
{ 0x59, 0x00, "Updated Block Read" },
{ 0x5A, 0x00, "Operator Request or State Change Input (Unspecified)" },
{ 0x5A, 0x01, "Operator Medium Removal Requested" },
{ 0x5A, 0x02, "Operator Selected Write Protect" },
{ 0x5A, 0x03, "Operator Selected Write Permit" },
{ 0x5B, 0x00, "Log Exception" },
{ 0x5B, 0x01, "Threshold Condition Met" },
{ 0x5B, 0x02, "Log Counter At Maximum" },
{ 0x5B, 0x03, "Log List Codes Exhausted" },
{ 0x5C, 0x00, "RPL Status Change" },
{ 0x5C, 0x01, "Spindles Synchronized" },
{ 0x5C, 0x02, "Spindles Not Synchronized" },
{ 0x60, 0x00, "Lamp Failure" },
{ 0x61, 0x00, "Video Acquisition Error" },
{ 0x61, 0x01, "Unable To Acquire Video" },
{ 0x61, 0x02, "Out Of Focus" },
{ 0x62, 0x00, "Scan Head Positioning Error" },
{ 0x63, 0x00, "End Of User Area Encountered On This Track" },
{ 0x64, 0x00, "Illegal Mode For This Track" },
{ 0x00, 0x00, NULL }
};

static void asc2ascii __P((unsigned char, unsigned char, char *, size_t));

static void
asc2ascii(asc, ascq, result, reslen)
	unsigned char asc, ascq;
	char *result;
	size_t reslen;
{
	int i = 0;

	while (adesc[i].description != NULL) {
		if (adesc[i].asc == asc && adesc[i].ascq == ascq)
			break;
		i++;
	}
	if (adesc[i].description == NULL) {
		if (asc == 0x40 && ascq != 0)
			(void) snprintf(result, reslen,
			    "Diagnostic Failure on Component 0x%02x",
			    ascq & 0xff);
		else
			(void) snprintf(result, reslen,
			    "ASC 0x%02x ASCQ 0x%02x",
			    asc & 0xff, ascq & 0xff);
	} else
		(void) strlcpy(result, adesc[i].description, reslen);
}

void
scsi_print_sense_data(s, slen, verbosity)
	const unsigned char *s;
	int slen, verbosity;
{
	int32_t info;
	int i, j, k;
	char sbs[132], *cp;

	/*
	 * Basics - print out SENSE KEY
	 */
	printf("    SENSE KEY: %s", scsi_decode_sense(s, 0, sbs, sizeof(sbs)));

	/*
	 * Print out, unqualified but aligned, FMK, EOM and ILI status.
	 */
	if (s[2] & 0xe0) {
		char pad;
	        printf("\n              ");
		pad = ' ';
		if (s[2] & SSD_FILEMARK) {
			printf("%c Filemark Detected", pad);
			pad = ',';
		}
		if (s[2] & SSD_EOM) {
			printf("%c EOM Detected", pad);
			pad = ',';
		}
		if (s[2] & SSD_ILI)
			printf("%c Incorrect Length Indicator Set", pad);
	}

	/*
	 * Now we should figure out, based upon device type, how
	 * to format the information field. Unfortunately, that's
	 * not convenient here, so we'll print it as a signed
	 * 32 bit integer.
	 */
	info = _4btol(&s[3]);
	if (info)
		printf("\n   INFO FIELD: %d", info);

	/*
	 * Now we check additional length to see whether there is
	 * more information to extract.
	 */

	/* enough for command specific information? */
	if (((unsigned int) s[7]) < 4) {
		printf("\n");
		return;
	}
	info = _4btol(&s[8]);
	if (info)
		printf("\n COMMAND INFO: %d (0x%x)", info, info);

	/*
	 * Decode ASC && ASCQ info, plus FRU, plus the rest...
	 */

	cp = scsi_decode_sense(s, 1, sbs, sizeof(sbs));
	if (cp)
		printf("\n     ASC/ASCQ: %s", cp);
	if (s[14] != 0)
		printf("\n     FRU CODE: 0x%x", s[14] & 0xff);
	cp = scsi_decode_sense(s, 3, sbs, sizeof(sbs));
	if (cp)
		printf("\n         SKSV: %s", cp);
	printf("\n");
	if (verbosity == 0) {
		printf("\n");
		return;
	}

	/*
	 * Now figure whether we should print any additional informtion.
	 *
	 * Where should we start from? If we had SKSV data,
	 * start from offset 18, else from offset 15.
	 *
	 * From that point until the end of the buffer, check for any
	 * nonzero data. If we have some, go back and print the lot,
	 * otherwise we're done.
	 */
	if (sbs)
		i = 18;
	else
		i = 15;
	for (j = i; j < slen; j++)
		if (s[j])
			break;
	if (j == slen)
		return;

	printf("\n Additional Sense Information (byte %d out...):\n", i);
	if (i == 15) {
		printf("\n\t%2d:", i);
		k = 7;
	} else {
		printf("\n\t%2d:", i);
		k = 2;
		j -= 2;
	}
	while (j > 0) {
		if (i >= slen)
			break;
		if (k == 8) {
			k = 0;
			printf("\n\t%2d:", i);
		}
		printf(" 0x%02x", s[i] & 0xff);
		k++;
		j--;
		i++;
	}
	printf("\n\n");
}

char *
scsi_decode_sense(snsbuf, flag, rqsbuf, rqsbuflen)
	const unsigned char *snsbuf;
	int flag;
	char *rqsbuf;
	size_t rqsbuflen;
{
	unsigned char skey;
	char localbuf[64];

	skey = 0;

	if (flag == 0 || flag == 2 || flag == 3)
		skey = snsbuf[2] & 0xf;
	if (flag == 0) {			/* Sense Key Only */
		(void) strlcpy(rqsbuf, sense_keys[skey], rqsbuflen);
		return (rqsbuf);
	} else if (flag == 1) {			/* ASC/ASCQ Only */
		asc2ascii(snsbuf[12], snsbuf[13], rqsbuf, rqsbuflen);
		return (rqsbuf);
	} else  if (flag == 2) {		/* Sense Key && ASC/ASCQ */
		asc2ascii(snsbuf[12], snsbuf[13], localbuf, sizeof(localbuf));
		(void) snprintf(rqsbuf, rqsbuflen, "%s, %s", sense_keys[skey],
		    localbuf);
		return (rqsbuf);
	} else if (flag == 3 && snsbuf[7] >= 9 && (snsbuf[15] & 0x80)) {
		/*
		 * SKSV Data
		 */
		switch (skey) {
		case SKEY_ILLEGAL_REQUEST:
			if (snsbuf[15] & 0x8)
				(void) snprintf(rqsbuf, rqsbuflen,
				    "Error in %s, Offset %d, bit %d",
				    (snsbuf[15] & 0x40)? "CDB" : "Parameters",
				    (snsbuf[16] & 0xff) << 8 |
				    (snsbuf[17] & 0xff), snsbuf[15] & 0x7);
			else
				(void) snprintf(rqsbuf, rqsbuflen,
				    "Error in %s, Offset %d",
				    (snsbuf[15] & 0x40)? "CDB" : "Parameters",
				    (snsbuf[16] & 0xff) << 8 |
				    (snsbuf[17] & 0xff));
			return (rqsbuf);

		case SKEY_RECOVERED_ERROR:
		case SKEY_MEDIUM_ERROR:
		case SKEY_HARDWARE_ERROR:
			(void) snprintf(rqsbuf, rqsbuflen,
			    "Actual Retry Count: %d",
			    (snsbuf[16] & 0xff) << 8 | (snsbuf[17] & 0xff));
			return (rqsbuf);

		case SKEY_NOT_READY:
			(void) snprintf(rqsbuf, rqsbuflen,
			    "Progress Indicator: %d",
			    (snsbuf[16] & 0xff) << 8 | (snsbuf[17] & 0xff));
			return (rqsbuf);

		default:
			break;
		}
	}
	return (NULL);
}

void
scsi_print_sense(name, req, verbosity)
	const char *name;
	const scsireq_t *req;
	int verbosity;
{
	int i;

 	printf("%s: Check Condition on CDB:", name);
 	for (i = 0; i < req->cmdlen; i++)
 		printf(" %02x", req->cmd[i]);
 	printf("\n");
	scsi_print_sense_data(req->sense, req->senselen_used, verbosity);
}
