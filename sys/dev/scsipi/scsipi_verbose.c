/*	$NetBSD: scsipi_verbose.c,v 1.32.18.1 2017/12/03 11:37:32 jdolecek Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: scsipi_verbose.c,v 1.32.18.1 2017/12/03 11:37:32 jdolecek Exp $");

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/device.h>

#ifdef _KERNEL
#include <sys/systm.h>
#include <sys/module.h>
#else
#include <stdio.h>
#endif
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/scsiconf.h>

int		scsipi_print_sense_real(struct scsipi_xfer *, int);
void		scsipi_print_sense_data_real(struct scsi_sense_data *, int);
static char    *scsipi_decode_sense(void *, int);

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

/*
 * The current version of this list can be obtained from
 * <ftp://ftp.t10.org/t10/drafts/spc3/asc-num.txt>
 */

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
{ 0x00, 0x07, "Programmable Early Warning Detected" },
{ 0x00, 0x11, "Audio Play Operation In Progress" },
{ 0x00, 0x12, "Audio Play Operation Paused" },
{ 0x00, 0x13, "Audio Play Operation Successfully Completed" },
{ 0x00, 0x14, "Audio Play Operation Stopped Due To Error" },
{ 0x00, 0x15, "No Current Audio Status To Return" },
{ 0x00, 0x16, "Operation In Progress" },
{ 0x00, 0x17, "Cleaning Requested" },
{ 0x00, 0x18, "Erase Operation In Progress" },
{ 0x00, 0x19, "Locate Operation In Progress" },
{ 0x00, 0x1A, "Rewind Operation In Progress" },
{ 0x00, 0x1B, "Set Capacity Operation In Progress" },
{ 0x00, 0x1C, "Verify Operation In Progress" },
{ 0x00, 0x1D, "ATA Pass Through Information Available" },
{ 0x00, 0x1E, "Conflicting SA Creation Request" },
{ 0x00, 0x1F, "Logical Unit Transitioning To Another Power Condition" },
{ 0x00, 0x20, "Extended Copy Information Available" },
{ 0x00, 0x21, "Atomic Command Aborted Due To ACA" },
{ 0x01, 0x00, "No Index/Sector Signal" },
{ 0x02, 0x00, "No Seek Complete" },
{ 0x03, 0x00, "Peripheral Device Write Fault" },
{ 0x03, 0x01, "No Write Current" },
{ 0x03, 0x02, "Excessive Write Errors" },
{ 0x04, 0x00, "Logical Unit Not Ready, Cause Not Reportable" },
{ 0x04, 0x01, "Logical Unit Is In Process Of Becoming Ready" },
{ 0x04, 0x02, "Logical Unit Not Ready, Initializing Command Required" },
{ 0x04, 0x03, "Logical Unit Not Ready, Manual Intervention Required" },
{ 0x04, 0x04, "Logical Unit Not Ready, Format In Progress" },
{ 0x04, 0x05, "Logical Unit Not Ready, Rebuild In Progress" },
{ 0x04, 0x06, "Logical Unit Not Ready, Recalculation In Progress" },
{ 0x04, 0x07, "Logical Unit Not Ready, Operation In Progress" },
{ 0x04, 0x08, "Logical Unit Not Ready, Long Write In Progress" },
{ 0x04, 0x09, "Logical Unit Not Ready, Self-Test In Progress" },
{ 0x04, 0x0A, "Logical Unit Not Accessible, Asymmetric Access State "
								"Transition" },
{ 0x04, 0x0B, "Logical Unit Not Accessible, Target Port In Standby State" },
{ 0x04, 0x0C, "Logical Unit Not Accessible, Target Port In Unavailable State" },
{ 0x04, 0x0D, "Logical Unit Not Ready, Structure Check Required" },
{ 0x04, 0x0E, "Logical Unit Not Ready, Security Session In Progress" },
{ 0x04, 0x10, "Logical Unit Not Ready, Auxiliary Memory Not Accessible" },
{ 0x04, 0x11, "Logical Unit Not Ready, Notify (Enable Spinup) Required" },
{ 0x04, 0x12, "Logical Unit Not Ready, Offline" },
{ 0x04, 0x13, "Logical Unit Not Ready, SA Creation In Progress" },
{ 0x04, 0x14, "Logical Unit Not Ready, Space Allocation In Progress" },
{ 0x04, 0x15, "Logical Unit Not Ready, Robotics Disabled" },
{ 0x04, 0x16, "Logical Unit Not Ready, Configuration Required" },
{ 0x04, 0x17, "Logical Unit Not Ready, Calibration Required" },
{ 0x04, 0x18, "Logical Unit Not Ready, A Door Is Open" },
{ 0x04, 0x19, "Logical Unit Not Ready, Operating In Sequential Mode" },
{ 0x04, 0x1A, "Logical Unit Not Ready, Start Stop Unit Command In Progress" },
{ 0x04, 0x1B, "Logical Unit Not Ready, Sanitize In Progress" },
{ 0x04, 0x1C, "Logical Unit Not Ready, Additional Power Use Not Yet Granted" },
{ 0x04, 0x1D, "Logical Unit Not Ready, Configuration In Progress" },
{ 0x04, 0x1E, "Logical Unit Not Ready, Microcode Activation Required" },
{ 0x04, 0x1F, "Logical Unit Not Ready, Microcode Download Required" },
{ 0x04, 0x20, "Logical Unit Not Ready, Logical Unit Reset Required" },
{ 0x04, 0x21, "Logical Unit Not Ready, Hard Reset Required" },
{ 0x04, 0x22, "Logical Unit Not Ready, Power Cycle Required" },
{ 0x04, 0x23, "Logical Unit Not Ready, Affiliation Required" },
{ 0x05, 0x00, "Logical Unit Does Not Respond To Selection" },
{ 0x06, 0x00, "No Reference Position Found" },
{ 0x07, 0x00, "Multiple Peripheral Devices Selected" },
{ 0x08, 0x00, "Logical Unit Communication Failure" },
{ 0x08, 0x01, "Logical Unit Communication Timeout" },
{ 0x08, 0x02, "Logical Unit Communication Parity Error" },
{ 0x08, 0x03, "Logical Unit Communication CRC Error (Ultra-Dma/32)" },
{ 0x08, 0x04, "Unreachable Copy Target" },
{ 0x09, 0x00, "Track Following Error" },
{ 0x09, 0x01, "Tracking Servo Failure" },
{ 0x09, 0x02, "Focus Servo Failure" },
{ 0x09, 0x03, "Spindle Servo Failure" },
{ 0x09, 0x04, "Head Select Fault" },
{ 0x09, 0x05, "Vibration Induced Tracking Error" },
{ 0x0A, 0x00, "Error Log Overflow" },
{ 0x0B, 0x00, "Warning" },
{ 0x0B, 0x01, "Warning - Specified Temperature Exceeded" },
{ 0x0B, 0x02, "Warning - Enclosure Degraded" },
{ 0x0B, 0x03, "Warning - Background Self-Test Failed" },
{ 0x0B, 0x04, "Warning - Background Pre-Scan Detected Medium Error" },
{ 0x0B, 0x05, "Warning - Background Medium Scan Detected Medium Error" },
{ 0x0B, 0x06, "Warning - Non-Volatile Cache Now Volatile" },
{ 0x0B, 0x07, "Warning - Degraded Power To Non-Volatile Cache" },
{ 0x0B, 0x08, "Warning - Power Loss Expected" },
{ 0x0B, 0x09, "Warning - Device Statistics Notification Active" },
{ 0x0B, 0x0A, "Warning - High Critical Temperature Limit Exceeded" },
{ 0x0B, 0x0B, "Warning - Low Critical Temperature Limit Exceeded" },
{ 0x0B, 0x0C, "Warning - High Operating Temperature Limit Exceeded" },
{ 0x0B, 0x0D, "Warning - Low Operating Temperature Limit Exceeded" },
{ 0x0B, 0x0E, "Warning - High Critical Humidity Limit Exceeded" },
{ 0x0B, 0x0F, "Warning - Low Critical Humidity Limit Exceeded" },
{ 0x0B, 0x10, "Warning - High Operating Humidity Limit Exceeded" },
{ 0x0B, 0x11, "Warning - Low Operating Humidity Limit Exceeded" },
{ 0x0C, 0x00, "Write Error" },
{ 0x0C, 0x01, "Write Error - Recovered With Auto Reallocation" },
{ 0x0C, 0x02, "Write Error - Auto Reallocation Failed" },
{ 0x0C, 0x03, "Write Error - Recommend Reassignment" },
{ 0x0C, 0x04, "Compression Check Miscompare Error" },
{ 0x0C, 0x05, "Data Expansion Occurred During Compression" },
{ 0x0C, 0x06, "Block Not Compressible" },
{ 0x0C, 0x07, "Write Error - Recovery Needed" },
{ 0x0C, 0x08, "Write Error - Recovery Failed" },
{ 0x0C, 0x09, "Write Error - Loss Of Streaming" },
{ 0x0C, 0x0A, "Write Error - Padding Blocks Added" },
{ 0x0C, 0x0B, "Auxiliary Memory Write Error" },
{ 0x0C, 0x0C, "Write Error - Unexpected Unsolicited Data" },
{ 0x0C, 0x0D, "Write Error - Not Enough Unsolicited Data" },
{ 0x0C, 0x0E, "Multiple Write Errors" },
{ 0x0C, 0x0F, "Defects In Error Window" },
{ 0x0C, 0x10, "Incomplete Multiple Atomic Write Operations" },
{ 0x0C, 0x11, "Write Error - Recovery Scan Needed" },
{ 0x0C, 0x12, "Write Error - Insufficient Zone Resources" },
{ 0x0D, 0x00, "Error Detected By Third Party Temporary Initiator" },
{ 0x0D, 0x01, "Third Party Device Failure" },
{ 0x0D, 0x02, "Copy Target Device Not Reachable" },
{ 0x0D, 0x03, "Incorrect Copy Target Device Type" },
{ 0x0D, 0x04, "Copy Target Device Data Underrun" },
{ 0x0D, 0x05, "Copy Target Device Data Overrun" },
{ 0x0E, 0x00, "Invalid Information Unit" },
{ 0x0E, 0x01, "Information Unit Too Short" },
{ 0x0E, 0x02, "Information Unit Too Long" },
{ 0x0E, 0x03, "Invalid Field In Command Information Unit" },
{ 0x10, 0x00, "Id CRC Or ECC Error" },
{ 0x10, 0x01, "Logical Block Guard Check Failed" },
{ 0x10, 0x02, "Logical Block Application Tag Check Failed" },
{ 0x10, 0x03, "Logical Block Reference Tag Check Failed" },
{ 0x10, 0x04, "Logical Block Protection Error On Recover Buffered Data" },
{ 0x10, 0x05, "Logical Block Protection Method Error" },
{ 0x11, 0x00, "Unrecovered Read Error" },
{ 0x11, 0x01, "Read Retries Exhausted" },
{ 0x11, 0x02, "Error Too Long To Correct" },
{ 0x11, 0x03, "Multiple Read Errors" },
{ 0x11, 0x04, "Unrecovered Read Error - Auto Reallocate Failed" },
{ 0x11, 0x05, "L-EC Uncorrectable Error" },
{ 0x11, 0x06, "CIRC Unrecovered Error" },
{ 0x11, 0x07, "Data Re-synchronization Error" },
{ 0x11, 0x08, "Incomplete Block Read" },
{ 0x11, 0x09, "No Gap Found" },
{ 0x11, 0x0A, "Miscorrected Error" },
{ 0x11, 0x0B, "Unrecovered Read Error - Recommend Reassignment" },
{ 0x11, 0x0C, "Unrecovered Read Error - Recommend Rewrite The Data" },
{ 0x11, 0x0D, "De-Compression CRC Error" },
{ 0x11, 0x0E, "Cannot Decompress Using Declared Algorithm" },
{ 0x11, 0x0F, "Error Reading UPC/EAN Number" },
{ 0x11, 0x10, "Error Reading ISRC Number" },
{ 0x11, 0x11, "Read Error - Loss Of Streaming" },
{ 0x11, 0x12, "Auxiliary Memory Read Error" },
{ 0x11, 0x13, "Read Error - Failed Retransmission Request" },
{ 0x11, 0x14, "Read Error - Lba Marked Bad By Application Client" },
{ 0x11, 0x15, "Write After Sanitize Required" },
{ 0x12, 0x00, "Address Mark Not Found For ID Field" },
{ 0x13, 0x00, "Address Mark Not Found For DATA Field" },
{ 0x14, 0x00, "Recorded Entity Not Found" },
{ 0x14, 0x01, "Record Not Found" },
{ 0x14, 0x02, "Filemark or Setmark Not Found" },
{ 0x14, 0x03, "End-Of-Data Not Found" },
{ 0x14, 0x04, "Block Sequence Error" },
{ 0x14, 0x05, "Record Not Found - Recommend Reassignment" },
{ 0x14, 0x06, "Record Not Found - Data Auto-Reallocated" },
{ 0x14, 0x07, "Locate Operation Failure" },
{ 0x15, 0x00, "Random Positioning Error" },
{ 0x15, 0x01, "Mechanical Positioning Error" },
{ 0x15, 0x02, "Positioning Error Detected By Read of Medium" },
{ 0x16, 0x00, "Data Synchronization Mark Error" },
{ 0x16, 0x01, "Data Sync Error - Data Rewritten" },
{ 0x16, 0x02, "Data Sync Error - Recommend Rewrite" },
{ 0x16, 0x03, "Data Sync Error - Data Auto-Reallocated" },
{ 0x16, 0x04, "Data Sync Error - Recommend Reassignment" },
{ 0x17, 0x00, "Recovered Data With No Error Correction Applied" },
{ 0x17, 0x01, "Recovered Data With Retries" },
{ 0x17, 0x02, "Recovered Data With Positive Head Offset" },
{ 0x17, 0x03, "Recovered Data With Negative Head Offset" },
{ 0x17, 0x04, "Recovered Data With Retries and/or CIRC Applied" },
{ 0x17, 0x05, "Recovered Data Using Previous Sector ID" },
{ 0x17, 0x06, "Recovered Data Without ECC - Data Auto-Reallocated" },
{ 0x17, 0x07, "Recovered Data Without ECC - Recommend Reassignment" },
{ 0x17, 0x08, "Recovered Data Without ECC - Recommend Rewrite" },
{ 0x17, 0x09, "Recovered Data Without ECC - Data Rewritten" },
{ 0x18, 0x00, "Recovered Data With Error Correction Applied" },
{ 0x18, 0x01, "Recovered Data With Error Corr. & Retries Applied" },
{ 0x18, 0x02, "Recovered Data - Data Auto-Reallocated" },
{ 0x18, 0x03, "Recovered Data With CIRC" },
{ 0x18, 0x04, "Recovered Data With LEC" },
{ 0x18, 0x05, "Recovered Data - Recommend Reassignment" },
{ 0x18, 0x06, "Recovered Data - Recommend Rewrite" },
{ 0x18, 0x07, "Recovered Data With ECC - Data Rewritten" },
{ 0x18, 0x08, "Recovered Data With Linking" },
{ 0x19, 0x00, "Defect List Error" },
{ 0x19, 0x01, "Defect List Not Available" },
{ 0x19, 0x02, "Defect List Error In Primary List" },
{ 0x19, 0x03, "Defect List Error In Grown List" },
{ 0x1A, 0x00, "Parameter List Length Error" },
{ 0x1B, 0x00, "Synchronous Data Transfer Error" },
{ 0x1C, 0x00, "Defect List Not Found" },
{ 0x1C, 0x01, "Primary Defect List Not Found" },
{ 0x1C, 0x02, "Grown Defect List Not Found" },
{ 0x1D, 0x00, "Miscompare During Verify Operation" },
{ 0x1D, 0x01, "Miscompare Verify Of Unmapped Lba" },
{ 0x1E, 0x00, "Recovered ID With ECC Correction" },
{ 0x1F, 0x00, "Partial Defect List Transfer" },
{ 0x20, 0x00, "Invalid Command Operation Code" },
{ 0x20, 0x01, "Access Denied - Initiator Pending-Enrolled" },
{ 0x20, 0x02, "Access Denied - No Access Rights" },
{ 0x20, 0x03, "Access Denied - Invalid Mgmt ID Key" },
{ 0x20, 0x04, "Illegal Command While In Write Capable State" },
{ 0x20, 0x05, "Obsolete" },
{ 0x20, 0x06, "Illegal Command While In Explicit Address Mode" },
{ 0x20, 0x07, "Illegal Command While In Implicit Address Mode" },
{ 0x20, 0x08, "Access Denied - Enrollment Conflict" },
{ 0x20, 0x09, "Access Denied - Invalid LU Identifer" },
{ 0x20, 0x0A, "Access Denied - Invalid Proxy Token" },
{ 0x20, 0x0B, "Access Denied - ACL LUN Conflict" },
{ 0x20, 0x0C, "Illegal Command When Not In Append-Only Mode" },
{ 0x20, 0x0D, "Not An Administrative Logical Unit" },
{ 0x20, 0x0E, "Not A Subsidiary Logical Unit" },
{ 0x20, 0x0F, "Not A Conglomerate Logical Unit" },
{ 0x21, 0x00, "Logical Block Address Out Of Range" },
{ 0x21, 0x01, "Invalid Element Address" },
{ 0x21, 0x02, "Invalid Address For Write" },
{ 0x21, 0x03, "Invalid Write Crossing Layer Jump" },
{ 0x21, 0x04, "Unaligned Write Command" },
{ 0x21, 0x05, "Write Boundary Violation" },
{ 0x21, 0x06, "Attempt To Read Invalid Data" },
{ 0x21, 0x07, "Read Boundary Violation" },
{ 0x21, 0x08, "Misaligned Write Command" },
{ 0x22, 0x00, "Illegal Function (Use 20 00, 24 00, Or 26 00)" },
{ 0x23, 0x00, "Invalid Token Operation, Cause Not Reportable" },
{ 0x23, 0x01, "Invalid Token Operation, Unsupported Token Type" },
{ 0x23, 0x02, "Invalid Token Operation, Remote Token Usage Not Supported" },
{ 0x23, 0x03, "Invalid Token Operation, Remote Rod Token Creation Not Supported" },
{ 0x23, 0x04, "Invalid Token Operation, Token Unknown" },
{ 0x23, 0x05, "Invalid Token Operation, Token Corrupt" },
{ 0x23, 0x06, "Invalid Token Operation, Token Revoked" },
{ 0x23, 0x07, "Invalid Token Operation, Token Expired" },
{ 0x23, 0x08, "Invalid Token Operation, Token Cancelled" },
{ 0x23, 0x09, "Invalid Token Operation, Token Deleted" },
{ 0x23, 0x0A, "Invalid Token Operation, Invalid Token Length" },
{ 0x24, 0x00, "Invalid Field In CDB" },
{ 0x24, 0x01, "CDB Decryption Error" },
{ 0x24, 0x02, "Obsolete" },
{ 0x24, 0x03, "Obsolete" },
{ 0x24, 0x04, "Security Audit Value Frozen" },
{ 0x24, 0x05, "Security Working Key Frozen" },
{ 0x24, 0x06, "Nonce Not Unique" },
{ 0x24, 0x07, "Nonce Timestamp Out Of Range" },
{ 0x24, 0x08, "Invalid XCDB" },
{ 0x24, 0x09, "Invalid Fast Format" },
{ 0x25, 0x00, "Logical Unit Not Supported" },
{ 0x26, 0x00, "Invalid Field In Parameter List" },
{ 0x26, 0x01, "Parameter Not Supported" },
{ 0x26, 0x02, "Parameter Value Invalid" },
{ 0x26, 0x03, "Threshold Parameters Not Supported" },
{ 0x26, 0x04, "Invalid Release Of Persistent Reservation" },
{ 0x26, 0x05, "Data Decryption Error" },
{ 0x26, 0x06, "Too Many Target Descriptors" },
{ 0x26, 0x07, "Unsupported Target Descriptor Type Code" },
{ 0x26, 0x08, "Too Many Segment Descriptors" },
{ 0x26, 0x09, "Unsupported Segment Descriptor Type Code" },
{ 0x26, 0x0A, "Unexpected Inexact Segment" },
{ 0x26, 0x0B, "Inline Data Length Exceeded" },
{ 0x26, 0x0C, "Invalid Operation For Copy Source Or Destination" },
{ 0x26, 0x0D, "Copy Segment Granularity Violation" },
{ 0x26, 0x0E, "Invalid Parameter While Port Is Enabled" },
{ 0x26, 0x0F, "Invalid Data-Out Buffer Integrity Check Value" },
{ 0x26, 0x10, "Data Decryption Key Fail Limit Reached" },
{ 0x26, 0x11, "Incomplete Key-Associated Data Set" },
{ 0x26, 0x12, "Vendor Specific Key Reference Not Found" },
{ 0x26, 0x13, "Application Tag Mode Page Is Invalid" },
{ 0x26, 0x14, "Tape Stream Mirroring Prevented" },
{ 0x26, 0x15, "Copy Source Or Copy Destination Not Authorized" },
{ 0x27, 0x00, "Write Protected" },
{ 0x27, 0x01, "Hardware Write Protected" },
{ 0x27, 0x02, "Logical Unit Software Write Protected" },
{ 0x27, 0x03, "Associated Write Protect" },
{ 0x27, 0x04, "Persistent Write Protect" },
{ 0x27, 0x05, "Permanent Write Protect" },
{ 0x27, 0x06, "Conditional Write Protect" },
{ 0x27, 0x07, "Space Allocation Failed Write Protect" },
{ 0x27, 0x08, "Zone Is Read Only" },
{ 0x28, 0x00, "Not Ready To Ready Change, Medium May Have Changed" },
{ 0x28, 0x01, "Import Or Export Element Accessed" },
{ 0x28, 0x02, "Format-Layer May Have Changed" },
{ 0x28, 0x03, "Import/Export Element Accessed, Medium Changed" },
{ 0x29, 0x00, "Power On, Reset, Or Bus Device Reset Occurred" },
{ 0x29, 0x01, "Power On Occurred" },
{ 0x29, 0x02, "SCSI Bus Reset Occurred" },
{ 0x29, 0x03, "Bus Device Reset Function Occurred" },
{ 0x29, 0x04, "Device Internal Reset" },
{ 0x29, 0x05, "Transceiver Mode Changed To Single-Ended" },
{ 0x29, 0x06, "Transceiver Mode Changed To LVD" },
{ 0x29, 0x07, "I_T Nexus Loss Occurred" },
{ 0x2A, 0x00, "Parameters Changed" },
{ 0x2A, 0x01, "Mode Parameters Changed" },
{ 0x2A, 0x02, "Log Parameters Changed" },
{ 0x2A, 0x03, "Reservations Preempted" },
{ 0x2A, 0x04, "Reservations Released" },
{ 0x2A, 0x05, "Registrations Preempted" },
{ 0x2A, 0x06, "Asymmetric Access State Changed" },
{ 0x2A, 0x07, "Implicit Asymmetric Access State Transition Failed" },
{ 0x2A, 0x08, "Priority Changed" },
{ 0x2A, 0x09, "Capacity Data Has Changed" },
{ 0x2A, 0x0A, "Error History I_t Nexus Cleared" },
{ 0x2A, 0x0B, "Error History Snapshot Released" },
{ 0x2A, 0x0C, "Error Recovery Attributes Have Changed" },
{ 0x2A, 0x0D, "Data Encryption Capabilities Changed" },
{ 0x2A, 0x10, "Timestamp Changed" },
{ 0x2A, 0x11, "Data Encryption Parameters Changed By Another I_t Nexus" },
{ 0x2A, 0x12, "Data Encryption Parameters Changed By Vendor Specific Event" },
{ 0x2A, 0x13, "Data Encryption Key Instance Counter Has Changed" },
{ 0x2A, 0x14, "SA Creation Capabilities Data Has Changed" },
{ 0x2A, 0x15, "Medium Removal Prevention Preempted" },
{ 0x2A, 0x16, "Zone Reset Write Pointer Recommended" },
{ 0x2B, 0x00, "Copy Cannot Execute Since Host Cannot Disconnect" },
{ 0x2C, 0x00, "Command Sequence Error" },
{ 0x2C, 0x01, "Too Many Windows Specified" },
{ 0x2C, 0x02, "Invalid Combination Of Windows Specified" },
{ 0x2C, 0x03, "Current Program Area Is Not Empty" },
{ 0x2C, 0x04, "Current Program Area Is Empty" },
{ 0x2C, 0x05, "Illegal Power Condition Request" },
{ 0x2C, 0x06, "Persistent Prevent Conflict" },
{ 0x2C, 0x07, "Previous Busy Status" },
{ 0x2C, 0x08, "Previous Task Set Full Status" },
{ 0x2C, 0x09, "Previous Reservation Conflict Status" },
{ 0x2C, 0x0A, "Partition Or Collection Contains User Objects" },
{ 0x2C, 0x0B, "Not Reserved" },
{ 0x2C, 0x0C, "Orwrite Generation Does Not Match" },
{ 0x2C, 0x0D, "Reset Write Pointer Not Allowed" },
{ 0x2C, 0x0E, "Zone Is Offline" },
{ 0x2C, 0x0F, "Stream Not Open" },
{ 0x2C, 0x10, "Unwritten Data In Zone" },
{ 0x2C, 0x11, "Descriptor Format Sense Data Required" },
{ 0x2D, 0x00, "Overwrite Error On Update In Place" },
{ 0x2E, 0x00, "Insufficient Time For Operation" },
{ 0x2E, 0x01, "Command Timeout Before Processing" },
{ 0x2E, 0x02, "Command Timeout During Processing" },
{ 0x2E, 0x03, "Command Timeout During Processing Due To Error Recovery" },
{ 0x2F, 0x00, "Commands Cleared By Another Initiator" },
{ 0x2F, 0x01, "Commands Cleared By Power Loss Notification" },
{ 0x2F, 0x02, "Commands Cleared By Device Server" },
{ 0x2F, 0x03, "Some Commands Cleared By Queuing Layer Event" },
{ 0x30, 0x00, "Incompatible Medium Installed" },
{ 0x30, 0x01, "Cannot Read Medium - Unknown Format" },
{ 0x30, 0x02, "Cannot Read Medium - Incompatible Format" },
{ 0x30, 0x03, "Cleaning Cartridge Installed" },
{ 0x30, 0x04, "Cannot Write Medium - Unknown Format" },
{ 0x30, 0x05, "Cannot Write Medium - Incompatible Format" },
{ 0x30, 0x06, "Cannot Format Medium - Incompatible Medium" },
{ 0x30, 0x07, "Cleaning Failure" },
{ 0x30, 0x08, "Cannot Write - Application Code Mismatch" },
{ 0x30, 0x09, "Current Session Not Fixated For Append" },
{ 0x30, 0x0A, "Cleaning Request Rejected" },
{ 0x30, 0x0C, "Worm Medium - Overwrite Attempted" },
{ 0x30, 0x0D, "Worm Medium - Integrity Check" },
{ 0x30, 0x10, "Medium Not Formatted" },
{ 0x30, 0x11, "Incompatible Volume Type" },
{ 0x30, 0x12, "Incompatible Volume Qualifier" },
{ 0x30, 0x13, "Cleaning Volume Expired" },
{ 0x31, 0x00, "Medium Format Corrupted" },
{ 0x31, 0x01, "Format Command Failed" },
{ 0x31, 0x02, "Zoned Formatting Failed Due To Spare Linking" },
{ 0x31, 0x03, "Sanitize Command Failed" },
{ 0x32, 0x00, "No Defect Spare Location Available" },
{ 0x32, 0x01, "Defect List Update Failure" },
{ 0x33, 0x00, "Tape Length Error" },
{ 0x34, 0x00, "Enclosure Failure" },
{ 0x35, 0x00, "Enclosure Services Failure" },
{ 0x35, 0x01, "Unsupported Enclosure Function" },
{ 0x35, 0x02, "Enclosure Services Unavailable" },
{ 0x35, 0x03, "Enclosure Services Transfer Failure" },
{ 0x35, 0x04, "Enclosure Services Transfer Refused" },
{ 0x35, 0x05, "Enclosure Services Checksum Error" },
{ 0x36, 0x00, "Ribbon, Ink, Or Toner Failure" },
{ 0x37, 0x00, "Rounded Parameter" },
{ 0x38, 0x00, "Event Status Notification" },
{ 0x38, 0x02, "ESN - Power Management Class Event" },
{ 0x38, 0x04, "ESN - Media Class Event" },
{ 0x38, 0x06, "ESN - Device Busy Class Event" },
{ 0x38, 0x07, "Thin Provisioning Soft Threshold Reached" },
{ 0x39, 0x00, "Saving Parameters Not Supported" },
{ 0x3A, 0x00, "Medium Not Present" },
{ 0x3A, 0x01, "Medium Not Present - Tray Closed" },
{ 0x3A, 0x02, "Medium Not Present - Tray Open" },
{ 0x3A, 0x03, "Medium Not Present - Loadable" },
{ 0x3A, 0x04, "Medium Not Present - Medium Auxiliary Memory Accessible" },
{ 0x3B, 0x00, "Sequential Positioning Error" },
{ 0x3B, 0x01, "Tape Position Error At Beginning-Of-Medium" },
{ 0x3B, 0x02, "Tape Position Error At End-Of-Medium" },
{ 0x3B, 0x03, "Tape Or Electronic Vertical Forms Unit Not Ready" },
{ 0x3B, 0x04, "Slew Failure" },
{ 0x3B, 0x05, "Paper Jam" },
{ 0x3B, 0x06, "Failed To Sense Top-Of-Form" },
{ 0x3B, 0x07, "Failed To Sense Bottom-Of-Form" },
{ 0x3B, 0x08, "Reposition Error" },
{ 0x3B, 0x09, "Read Past End Of Medium" },
{ 0x3B, 0x0A, "Read Past Beginning Of Medium" },
{ 0x3B, 0x0B, "Position Past End Of Medium" },
{ 0x3B, 0x0C, "Position Past Beginning Of Medium" },
{ 0x3B, 0x0D, "Medium Destination Element Full" },
{ 0x3B, 0x0E, "Medium Source Element Empty" },
{ 0x3B, 0x0F, "End Of Medium Reached" },
{ 0x3B, 0x11, "Medium Magazine Not Accessible" },
{ 0x3B, 0x12, "Medium Magazine Removed" },
{ 0x3B, 0x13, "Medium Magazine Inserted" },
{ 0x3B, 0x14, "Medium Magazine Locked" },
{ 0x3B, 0x15, "Medium Magazine Unlocked" },
{ 0x3B, 0x16, "Mechanical Positioning Or Changer Error" },
{ 0x3B, 0x17, "Read Past End Of User Object" },
{ 0x3B, 0x18, "Element Disabled" },
{ 0x3B, 0x19, "Element Enabled" },
{ 0x3B, 0x1A, "Data Transfer Device Removed" },
{ 0x3B, 0x1B, "Data Transfer Device Inserted" },
{ 0x3B, 0x1C, "Too Many Logical Objects On Partition To Support Operation" },
{ 0x3D, 0x00, "Invalid Bits In IDENTIFY Message" },
{ 0x3E, 0x00, "Logical Unit Has Not Self-Configured Yet" },
{ 0x3E, 0x01, "Logical Unit Failure" },
{ 0x3E, 0x02, "Timeout On Logical Unit" },
{ 0x3E, 0x03, "Logical Unit Failed Self-Test" },
{ 0x3E, 0x04, "Logical Unit Unable To Update Self-Test Log" },
{ 0x3F, 0x00, "Target Operating Conditions Have Changed" },
{ 0x3F, 0x01, "Microcode Has Been Changed" },
{ 0x3F, 0x02, "Changed Operating Definition" },
{ 0x3F, 0x03, "INQUIRY Data Has Changed" },
{ 0x3F, 0x04, "Component Device Attached" },
{ 0x3F, 0x05, "Device Identifier Changed" },
{ 0x3F, 0x06, "Redundancy Group Created Or Modified" },
{ 0x3F, 0x07, "Redundancy Group Deleted" },
{ 0x3F, 0x08, "Spare Created Or Modified" },
{ 0x3F, 0x09, "Spare Deleted" },
{ 0x3F, 0x0A, "Volume Set Created Or Modified" },
{ 0x3F, 0x0B, "Volume Set Deleted" },
{ 0x3F, 0x0C, "Volume Set Deassigned" },
{ 0x3F, 0x0D, "Volume Set Reassigned" },
{ 0x3F, 0x0E, "Reported LUNs Data Has Changed" },
{ 0x3F, 0x0F, "Echo Buffer Overwritten" },
{ 0x3F, 0x10, "Medium Loadable" },
{ 0x3F, 0x11, "Medium Auxiliary Memory Accessible" },
{ 0x3F, 0x12, "Iscsi Ip Address Added" },
{ 0x3F, 0x13, "Iscsi Ip Address Removed" },
{ 0x3F, 0x14, "Iscsi Ip Address Changed" },
{ 0x3F, 0x15, "Inspect Referrals Sense Descriptors" },
{ 0x3F, 0x16, "Microcode Has Been Changed Without Reset" },
{ 0x3F, 0x17, "Zone Transition To Full" },
{ 0x3F, 0x18, "Bind Completed" },
{ 0x3F, 0x19, "Bind Redirected" },
{ 0x3F, 0x1A, "Subsidiary Binding Changed" },
{ 0x40, 0x00, "Ram Failure (Should Use 40 NN)" },
{ 0x40, 0x00, "Diagnostic Failure On Component NN (80h-FFh)" },
{ 0x41, 0x00, "Data Path Failure (Should Use 40 NN)" },
{ 0x42, 0x00, "Power-On Or Self-Test Failure (Should Use 40 NN)" },
{ 0x43, 0x00, "Message Error" },
{ 0x44, 0x00, "Internal Target Failure" },
{ 0x44, 0x01, "Persistent Reservation Information Lost" },
{ 0x44, 0x71, "Ata Device Failed Set Features" },
{ 0x45, 0x00, "Select Or Reselect Failure" },
{ 0x46, 0x00, "Unsuccessful Soft Reset" },
{ 0x47, 0x00, "SCSI Parity Error" },
{ 0x47, 0x01, "Data Phase CRC Error Detected" },
{ 0x47, 0x02, "SCSI Parity Error Detected During ST Data Phase" },
{ 0x47, 0x03, "Information Unit iuCRC Error Detected" },
{ 0x47, 0x04, "Asynchronous Information Protection Error Detected" },
{ 0x47, 0x05, "Protocol Service Crc Error" },
{ 0x47, 0x06, "PHY Test Function In Progress" },
{ 0x47, 0x7F, "Some Commands Cleared By Iscsi Protocol Event" },
{ 0x48, 0x00, "Initiator Detected Error Message Received" },
{ 0x49, 0x00, "Invalid Message Error" },
{ 0x4A, 0x00, "Command Phase Error" },
{ 0x4B, 0x00, "Data Phase Error" },
{ 0x4B, 0x01, "Illegal Target Port Transfer Tag Received" },
{ 0x4B, 0x02, "Too Much Write Data" },
{ 0x4B, 0x03, "ACK/NAK Timeout" },
{ 0x4B, 0x04, "NAK Reveived" },
{ 0x4B, 0x05, "Data Offset Error" },
{ 0x4B, 0x06, "Initiator Response Timeout" },
{ 0x4B, 0x07, "Connection Lost" },
{ 0x4B, 0x08, "Data-In Buffer Overflow - Data Buffer Size" },
{ 0x4B, 0x09, "Data-In Buffer Overflow - Data Buffer Descriptor Area" },
{ 0x4B, 0x0A, "Data-In Buffer Error" },
{ 0x4B, 0x0B, "Data-Out Buffer Overflow - Data Buffer Size" },
{ 0x4B, 0x0C, "Data-Out Buffer Overflow - Data Buffer Descriptor Area" },
{ 0x4B, 0x0D, "Data-Out Buffer Error" },
{ 0x4B, 0x0E, "PCIe Fabric Error" },
{ 0x4B, 0x0F, "PCIe Completion Timeout" },
{ 0x4B, 0x10, "PCIe Completer Abort" },
{ 0x4B, 0x11, "PCIe Poisoned Tlp Received" },
{ 0x4B, 0x12, "PCIe Ecrc Check Failed" },
{ 0x4B, 0x13, "PCIe Unsupported Request" },
{ 0x4B, 0x14, "PCIe ACS Violation" },
{ 0x4B, 0x15, "PCIe TLP Prefix Blocked" },
{ 0x4C, 0x00, "Logical Unit Failed Self-Configuration" },
{ 0x4D, 0x00, "Tagged Overlapped Commands (NN = Task Tag)" },
{ 0x4E, 0x00, "Overlapped Commands Attempted" },
{ 0x50, 0x00, "Write Append Error" },
{ 0x50, 0x01, "Write Append Position Error" },
{ 0x50, 0x02, "Position Error Related To Timing" },
{ 0x51, 0x00, "Erase Failure" },
{ 0x51, 0x01, "Erase Failure - Incomplete Erase Operation Detected" },
{ 0x52, 0x00, "Cartridge Fault" },
{ 0x53, 0x00, "Media Load or Eject Failed" },
{ 0x53, 0x01, "Unload Tape Failure" },
{ 0x53, 0x02, "Medium Removal Prevented" },
{ 0x53, 0x03, "Medium Removal Prevented By Data Transfer Element" },
{ 0x53, 0x04, "Medium Thread Or Unthread Failure" },
{ 0x53, 0x05, "Volume Identifier Invalid" },
{ 0x53, 0x06, "Volume Identifier Missing" },
{ 0x53, 0x07, "Duplicate Volume Identifier" },
{ 0x53, 0x08, "Element Status Unknown" },
{ 0x53, 0x09, "Data Transfer Device Error - Load Failed" },
{ 0x53, 0x0A, "Data Transfer Device Error - Unload Failed" },
{ 0x53, 0x0B, "Data Transfer Device Error - Unload Missing" },
{ 0x53, 0x0C, "Data Transfer Device Error - Eject Failed" },
{ 0x53, 0x0D, "Data Transfer Device Error - Library Communication Failed" },
{ 0x54, 0x00, "SCSI To Host System Interface Failure" },
{ 0x55, 0x00, "System Resource Failure" },
{ 0x55, 0x01, "System Buffer Full" },
{ 0x55, 0x02, "Insufficient Reservation Resources" },
{ 0x55, 0x03, "Insufficient Resources" },
{ 0x55, 0x04, "Insufficient Registration Resources" },
{ 0x55, 0x05, "Insufficient Access Control Resources" },
{ 0x55, 0x06, "Auxiliary Memory Out Of Space" },
{ 0x55, 0x07, "Quota Error" },
{ 0x55, 0x08, "Maximum Number Of Supplemental Decryption Keys Exceeded" },
{ 0x55, 0x09, "Medium Auxiliary Memory Not Accessible" },
{ 0x55, 0x0A, "Data Currently Unavailable" },
{ 0x55, 0x0B, "Insufficient Power For Operation" },
{ 0x55, 0x0C, "Insufficient Resources To Create Rod" },
{ 0x55, 0x0D, "Insufficient Resources To Create Rod Token" },
{ 0x55, 0x0E, "Insufficient Zone Resources" },
{ 0x55, 0x0F, "Insufficient Zone Resources To Complete Write" },
{ 0x55, 0x10, "Maximum Number Of Streams Open" },
{ 0x55, 0x11, "Insufficient Resources To Bind" },
{ 0x57, 0x00, "Unable To Recover Table-Of-Contents" },
{ 0x58, 0x00, "Generation Does Not Exist" },
{ 0x59, 0x00, "Updated Block Read" },
{ 0x5A, 0x00, "Operator Request Or State Change Input" },
{ 0x5A, 0x01, "Operator Medium Removal Request" },
{ 0x5A, 0x02, "Operator Selected Write Protect" },
{ 0x5A, 0x03, "Operator Selected Write Permit" },
{ 0x5B, 0x00, "Log Exception" },
{ 0x5B, 0x01, "Threshold Condition Met" },
{ 0x5B, 0x02, "Log Counter At Maximum" },
{ 0x5B, 0x03, "Log List Codes Exhausted" },
{ 0x5C, 0x00, "RPL Status Change" },
{ 0x5C, 0x01, "Spindles Synchronized" },
{ 0x5C, 0x02, "Spindles Not Synchronized" },
{ 0x5D, 0x00, "Failure Prediction Threshold Exceeded" },
{ 0x5D, 0x01, "Media Failure Prediction Threshold Exceeded" },
{ 0x5D, 0x02, "Logical Unit Failure Prediction Threshold Exceeded" },
{ 0x5D, 0x03, "Spare Area Exhaustion Prediction Threshold Exceeded" },
{ 0x5D, 0x10, "Hardware Impending Failure General Hard Drive Failure" },
{ 0x5D, 0x11, "Hardware Impending Failure Drive Error Rate Too High" },
{ 0x5D, 0x12, "Hardware Impending Failure Data Error Rate Too High" },
{ 0x5D, 0x13, "Hardware Impending Failure Seek Error Rate Too High" },
{ 0x5D, 0x14, "Hardware Impending Failure Too Many Block Reassigns" },
{ 0x5D, 0x15, "Hardware Impending Failure Access Times Too High" },
{ 0x5D, 0x16, "Hardware Impending Failure Start Unit Times Too High" },
{ 0x5D, 0x17, "Hardware Impending Failure Channel Parametrics" },
{ 0x5D, 0x18, "Hardware Impending Failure Controller Detected" },
{ 0x5D, 0x19, "Hardware Impending Failure Throughput Performance" },
{ 0x5D, 0x1A, "Hardware Impending Failure Seek Time Performance" },
{ 0x5D, 0x1B, "Hardware Impending Failure Spin-Up Retry Count" },
{ 0x5D, 0x1C, "Hardware Impending Failure Drive Calibration Retry Count" },
{ 0x5D, 0x20, "Controller Impending Failure General Hard Drive Failure" },
{ 0x5D, 0x21, "Controller Impending Failure Drive Error Rate Too High" },
{ 0x5D, 0x22, "Controller Impending Failure Data Error Rate Too High" },
{ 0x5D, 0x23, "Controller Impending Failure Seek Error Rate Too High" },
{ 0x5D, 0x24, "Controller Impending Failure Too Many Block Reassigns" },
{ 0x5D, 0x25, "Controller Impending Failure Access Times Too High" },
{ 0x5D, 0x26, "Controller Impending Failure Start Unit Times Too High" },
{ 0x5D, 0x27, "Controller Impending Failure Channel Parametrics" },
{ 0x5D, 0x28, "Controller Impending Failure Controller Detected" },
{ 0x5D, 0x29, "Controller Impending Failure Throughput Performance" },
{ 0x5D, 0x2A, "Controller Impending Failure Seek Time Performance" },
{ 0x5D, 0x2B, "Controller Impending Failure Spin-Up Retry Count" },
{ 0x5D, 0x2C, "Controller Impending Failure Drive Calibration Retry Count" },
{ 0x5D, 0x30, "Data Channel Impending Failure General Hard Drive Failure" },
{ 0x5D, 0x31, "Data Channel Impending Failure Drive Error Rate Too High" },
{ 0x5D, 0x32, "Data Channel Impending Failure Data Error Rate Too High" },
{ 0x5D, 0x33, "Data Channel Impending Failure Seek Error Rate Too High" },
{ 0x5D, 0x34, "Data Channel Impending Failure Too Many Block Reassigns" },
{ 0x5D, 0x35, "Data Channel Impending Failure Access Times Too High" },
{ 0x5D, 0x36, "Data Channel Impending Failure Start Unit Times Too High" },
{ 0x5D, 0x37, "Data Channel Impending Failure Channel Parametrics" },
{ 0x5D, 0x38, "Data Channel Impending Failure Controller Detected" },
{ 0x5D, 0x39, "Data Channel Impending Failure Throughput Performance" },
{ 0x5D, 0x3A, "Data Channel Impending Failure Seek Time Performance" },
{ 0x5D, 0x3B, "Data Channel Impending Failure Spin-Up Retry Count" },
{ 0x5D, 0x3C, "Data Channel Impending Failure Drive Calibration Retry Count" },
{ 0x5D, 0x40, "Servo Impending Failure General Hard Drive Failure" },
{ 0x5D, 0x41, "Servo Impending Failure Drive Error Rate Too High" },
{ 0x5D, 0x42, "Servo Impending Failure Data Error Rate Too High" },
{ 0x5D, 0x43, "Servo Impending Failure Seek Error Rate Too High" },
{ 0x5D, 0x44, "Servo Impending Failure Too Many Block Reassigns" },
{ 0x5D, 0x45, "Servo Impending Failure Access Times Too High" },
{ 0x5D, 0x46, "Servo Impending Failure Start Unit Times Too High" },
{ 0x5D, 0x47, "Servo Impending Failure Channel Parametrics" },
{ 0x5D, 0x48, "Servo Impending Failure Controller Detected" },
{ 0x5D, 0x49, "Servo Impending Failure Throughput Performance" },
{ 0x5D, 0x4A, "Servo Impending Failure Seek Time Performance" },
{ 0x5D, 0x4B, "Servo Impending Failure Spin-Up Retry Count" },
{ 0x5D, 0x4C, "Servo Impending Failure Drive Calibration Retry Count" },
{ 0x5D, 0x50, "Spindle Impending Failure General Hard Drive Failure" },
{ 0x5D, 0x51, "Spindle Impending Failure Drive Error Rate Too High" },
{ 0x5D, 0x52, "Spindle Impending Failure Data Error Rate Too High" },
{ 0x5D, 0x53, "Spindle Impending Failure Seek Error Rate Too High" },
{ 0x5D, 0x54, "Spindle Impending Failure Too Many Block Reassigns" },
{ 0x5D, 0x55, "Spindle Impending Failure Access Times Too High" },
{ 0x5D, 0x56, "Spindle Impending Failure Start Unit Times Too High" },
{ 0x5D, 0x57, "Spindle Impending Failure Channel Parametrics" },
{ 0x5D, 0x58, "Spindle Impending Failure Controller Detected" },
{ 0x5D, 0x59, "Spindle Impending Failure Throughput Performance" },
{ 0x5D, 0x5A, "Spindle Impending Failure Seek Time Performance" },
{ 0x5D, 0x5B, "Spindle Impending Failure Spin-Up Retry Count" },
{ 0x5D, 0x5C, "Spindle Impending Failure Drive Calibration Retry Count" },
{ 0x5D, 0x60, "Firmware Impending Failure General Hard Drive Failure" },
{ 0x5D, 0x61, "Firmware Impending Failure Drive Error Rate Too High" },
{ 0x5D, 0x62, "Firmware Impending Failure Data Error Rate Too High" },
{ 0x5D, 0x63, "Firmware Impending Failure Seek Error Rate Too High" },
{ 0x5D, 0x64, "Firmware Impending Failure Too Many Block Reassigns" },
{ 0x5D, 0x65, "Firmware Impending Failure Access Times Too High" },
{ 0x5D, 0x66, "Firmware Impending Failure Start Unit Times Too High" },
{ 0x5D, 0x67, "Firmware Impending Failure Channel Parametrics" },
{ 0x5D, 0x68, "Firmware Impending Failure Controller Detected" },
{ 0x5D, 0x69, "Firmware Impending Failure Throughput Performance" },
{ 0x5D, 0x6A, "Firmware Impending Failure Seek Time Performance" },
{ 0x5D, 0x6B, "Firmware Impending Failure Spin-Up Retry Count" },
{ 0x5D, 0x6C, "Firmware Impending Failure Drive Calibration Retry Count" },
{ 0x5D, 0xFF, "Failure Prediction Threshold Exceeded (False)" },
{ 0x5E, 0x00, "Low Power Condition On" },
{ 0x5E, 0x01, "Idle Condition Activated By Timer" },
{ 0x5E, 0x02, "Standby Condition Activated By Timer" },
{ 0x5E, 0x03, "Idle Condition Activated By Command" },
{ 0x5E, 0x04, "Standby Condition Activated By Command" },
{ 0x5E, 0x05, "IDLE_B Condition Activated By Timer" },
{ 0x5E, 0x06, "IDLE_B Condition Activated By Command" },
{ 0x5E, 0x07, "IDLE_C Condition Activated By Timer" },
{ 0x5E, 0x08, "IDLE_C Condition Activated By Command" },
{ 0x5E, 0x09, "STANDBY_Y Condition Activated By Timer" },
{ 0x5E, 0x0A, "STANDBY_Y Condition Activated By Command" },
{ 0x5E, 0x41, "Power State Change To Active" },
{ 0x5E, 0x42, "Power State Change To Idle" },
{ 0x5E, 0x43, "Power State Change To Standby" },
{ 0x5E, 0x45, "Power State Change To Sleep" },
{ 0x5E, 0x47, "Power State Change To Device Control" },
{ 0x60, 0x00, "Lamp Failure" },
{ 0x61, 0x00, "Video Acquisition Error" },
{ 0x61, 0x01, "Unable To Acquire Video" },
{ 0x61, 0x02, "Out Of Focus" },
{ 0x62, 0x00, "Scan Head Positioning Error" },
{ 0x63, 0x00, "End Of User Area Encountered On This Track" },
{ 0x63, 0x01, "Packet Does Not Fit In Available Space" },
{ 0x64, 0x00, "Illegal Mode For This Track" },
{ 0x64, 0x01, "Invalid Packet Size" },
{ 0x65, 0x00, "Voltage Fault" },
{ 0x66, 0x00, "Automatic Document Feeder Cover Up" },
{ 0x66, 0x01, "Automatic Document Feeder Lift Up" },
{ 0x66, 0x02, "Document Jam In Automatic Document Feeder" },
{ 0x66, 0x03, "Document Miss Feed Automatic In Document Feeder" },
{ 0x67, 0x00, "Configuration Failure" },
{ 0x67, 0x01, "Configuration Of Incapable Logical Units Failed" },
{ 0x67, 0x02, "Add Logical Unit Failed" },
{ 0x67, 0x03, "Modification Of Logical Unit Failed" },
{ 0x67, 0x04, "Exchange Of Logical Unit Failed" },
{ 0x67, 0x05, "Remove Of Logical Unit Failed" },
{ 0x67, 0x06, "Attachment Of Logical Unit Failed" },
{ 0x67, 0x07, "Creation of Logical Unit Failed" },
{ 0x67, 0x08, "Assign Failure Occurred" },
{ 0x67, 0x09, "Multiply Assigned Logical Unit" },
{ 0x67, 0x0A, "Set Target Port Groups Command Failed" },
{ 0x67, 0x0B, "ATA Device Feature Not Enabled" },
{ 0x67, 0x0C, "Command Rejected" },
{ 0x67, 0x0D, "Explicit Bind Not Allowed" },
{ 0x68, 0x00, "Logical Unit Not Configured" },
{ 0x68, 0x01, "Subsidiary Logical Unit Not Configured" },
{ 0x69, 0x00, "Data Loss On Logical Unit" },
{ 0x69, 0x01, "Multiple Logical Unit Failures" },
{ 0x69, 0x02, "Parity/Data Mismatch" },
{ 0x6A, 0x00, "Informational, Refer To Log" },
{ 0x6B, 0x00, "State Change Has Occurred" },
{ 0x6B, 0x01, "Redundancy Level Got Better" },
{ 0x6B, 0x02, "Redundancy Level Got Worse" },
{ 0x6C, 0x00, "Rebuild Failure Occurred" },
{ 0x6D, 0x00, "Recalculate Failure Occurred" },
{ 0x6E, 0x00, "Command To Logical Unit Failed" },
{ 0x6F, 0x00, "Copy Protection Key Exchange Failure - Authentication Failure" },
{ 0x6F, 0x01, "Copy Protection Key Exchange Failure - Key Not Present" },
{ 0x6F, 0x02, "Copy Protection Key Exchange Failure - Key Not Established" },
{ 0x6F, 0x03, "Read Of Scrambled Sector Without Authentication" },
{ 0x6F, 0x04, "Media Region Code Is Mismatched To Logical Unit Region" },
{ 0x6F, 0x05, "Drive Region Must Be Permanent/Region Reset Count Error" },
{ 0x6F, 0x06, "Insufficient Block Count For Binding Nonce Recording" },
{ 0x6F, 0x07, "Conflict In Binding Nonce Recording" },
{ 0x6F, 0x08, "Insufficient Permission" },
{ 0x6F, 0x09, "Invalid Drive-Host Pairing Server" },
{ 0x6F, 0x0A, "Drive-Host Pairing Suspended" },
{ 0x70, 0x00, "Decompression Exception Short Algorithm Id Of NN" },
{ 0x71, 0x00, "Decompression Exception Long Algorithm Id" },
{ 0x72, 0x00, "Session Fixation Error" },
{ 0x72, 0x01, "Session Fixation Error Writing Lead-In" },
{ 0x72, 0x02, "Session Fixation Error Writing Lead-Out" },
{ 0x72, 0x03, "Session Fixation Error - Incomplete Track In Session" },
{ 0x72, 0x04, "Empty Or Partially Written Reserved Track" },
{ 0x72, 0x05, "No More Track Reservations Allowed" },
{ 0x72, 0x06, "RMZ Extension Is Not Allowed" },
{ 0x72, 0x07, "No More Test Zone Extensions Are Allowed" },
{ 0x73, 0x00, "CD Control Error" },
{ 0x73, 0x01, "Power Calibration Area Almost Full" },
{ 0x73, 0x02, "Power Calibration Area Is Full" },
{ 0x73, 0x03, "Power Calibration Area Error" },
{ 0x73, 0x04, "Program Memory Area Update Failure" },
{ 0x73, 0x05, "Program Memory Area Is Full" },
{ 0x73, 0x06, "RMA/PMA Is Almost Full" },
{ 0x73, 0x10, "Current Power Calibration Area Almost Full" },
{ 0x73, 0x11, "Current Power Calibration Area Is Full" },
{ 0x73, 0x17, "RDZ Is Full" },
{ 0x74, 0x00, "Security Error" },
{ 0x74, 0x01, "Unable To Decrypt Data" },
{ 0x74, 0x02, "Unencrypted Data Encountered While Decrypting" },
{ 0x74, 0x03, "Incorrect Data Encryption Key" },
{ 0x74, 0x04, "Cryptographic Integrity Validation Failed" },
{ 0x74, 0x05, "Error Decrypting Data" },
{ 0x74, 0x06, "Unknown Signature Verification Key" },
{ 0x74, 0x07, "Encryption Parameters Not Useable" },
{ 0x74, 0x08, "Digital Signature Validation Failure" },
{ 0x74, 0x09, "Encryption Mode Mismatch On Read" },
{ 0x74, 0x0A, "Encrypted Block Not Raw Read Enabled" },
{ 0x74, 0x0B, "Incorrect Encryption Parameters" },
{ 0x74, 0x0C, "Unable To Decrypt Parameter List" },
{ 0x74, 0x0D, "Encryption Algorithm Disabled" },
{ 0x74, 0x10, "SA Creation Parameter Value Invalid" },
{ 0x74, 0x11, "SA Creation Parameter Value Rejected" },
{ 0x74, 0x12, "Invalid Sa Usage" },
{ 0x74, 0x21, "Data Encryption Configuration Prevented" },
{ 0x74, 0x30, "SA Creation Parameter Not Supported" },
{ 0x74, 0x40, "Authentication Failed" },
{ 0x74, 0x61, "External Data Encryption Key Manager Access Error" },
{ 0x74, 0x62, "External Data Encryption Key Manager Error" },
{ 0x74, 0x63, "External Data Encryption Key Not Found" },
{ 0x74, 0x64, "External Data Encryption Request Not Authorized" },
{ 0x74, 0x6E, "External Data Encryption Control Timeout" },
{ 0x74, 0x6F, "External Data Encryption Control Error" },
{ 0x74, 0x71, "Logical Unit Access Not Authorized" },
{ 0x74, 0x79, "Security Conflict In Translated Device" },
{ 0x00, 0x00, NULL }
};

#ifdef _KERNEL
MODULE(MODULE_CLASS_MISC, scsiverbose, NULL);

static int
scsiverbose_modcmd(modcmd_t cmd, void *arg)
{
	static int   (*saved_print_sense)(struct scsipi_xfer *, int);
	static void  (*saved_print_sense_data)(struct scsi_sense_data *, int);

	switch (cmd) {
	case MODULE_CMD_INIT:
		saved_print_sense = scsipi_print_sense;
		saved_print_sense_data = scsipi_print_sense_data;
		scsipi_print_sense = scsipi_print_sense_real;
		scsipi_print_sense_data = scsipi_print_sense_data_real;
		scsi_verbose_loaded = 1;
		return 0;
	case MODULE_CMD_FINI:
		scsipi_print_sense = saved_print_sense;
		scsipi_print_sense_data = saved_print_sense_data;
		scsi_verbose_loaded = 0;
		return 0;
	default:
		return ENOTTY;
	}
}
#else
int	(*scsipi_print_sense)(struct scsipi_xfer *, int) =
		scsipi_print_sense_real;
void	(*scsipi_print_sense_data)(struct scsi_sense_data *, int) =
		scsipi_print_sense_data_real; 
#endif

static void
asc2ascii(u_char asc, u_char ascq, char *result, size_t l)
{
	int i = 0;

	while (adesc[i].description != NULL) {
		if (adesc[i].asc == asc && adesc[i].ascq == ascq)
			break;
		i++;
	}
	if (adesc[i].description == NULL) {
		if (asc == 0x40 && ascq != 0)
			(void)snprintf(result, l,
			    "Diagnostic Failure on Component 0x%02x",
			    ascq & 0xff);
		else
			(void)snprintf(result, l, "ASC 0x%02x ASCQ 0x%02x",
			    asc & 0xff, ascq & 0xff);
	} else
		(void)strlcpy(result, adesc[i].description, l);
}

void
scsipi_print_sense_data_real(struct scsi_sense_data *sense, int verbosity)
{
	int32_t info;
	int i, j, k;
	char *sbs, *s = (char *) sense;

	/*
	 * Basics- print out SENSE KEY
	 */
	printf("    SENSE KEY:  %s", scsipi_decode_sense(s, 0));

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
		printf("\n   INFO FIELD:  %d", info);

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
		printf("\n COMMAND INFO:  %d (0x%x)", info, info);

	/*
	 * Decode ASC && ASCQ info, plus FRU, plus the rest...
	 */

	sbs = scsipi_decode_sense(s, 1);
	if (sbs)
		printf("\n     ASC/ASCQ:  %s", sbs);
	if (s[14] != 0)
		printf("\n     FRU CODE:  0x%x", s[14] & 0xff);
	sbs = scsipi_decode_sense(s, 3);
	if (sbs)
		printf("\n         SKSV:  %s", sbs);
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
	for (j = i; j < sizeof (*sense); j++)
		if (s[j])
			break;
	if (j == sizeof (*sense))
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
		if (i >= sizeof (*sense))
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

static char *
scsipi_decode_sense(void *sinfo, int flag)
{
	unsigned char *snsbuf;
	unsigned char skey;
	static char rqsbuf[132];

	skey = 0;

	snsbuf = (unsigned char *) sinfo;
	if (flag == 0 || flag == 2 || flag == 3)
		skey = snsbuf[2] & 0xf;
	if (flag == 0) {			/* Sense Key Only */
		(void) strlcpy(rqsbuf, sense_keys[skey], sizeof(rqsbuf));
		return (rqsbuf);
	} else if (flag == 1) {			/* ASC/ASCQ Only */
		asc2ascii(snsbuf[12], snsbuf[13], rqsbuf, sizeof(rqsbuf));
		return (rqsbuf);
	} else  if (flag == 2) {		/* Sense Key && ASC/ASCQ */
		auto char localbuf[64];
		asc2ascii(snsbuf[12], snsbuf[13], localbuf, sizeof(localbuf));
		(void) snprintf(rqsbuf, sizeof(rqsbuf), "%s, %s",
		    sense_keys[skey], localbuf);
		return (rqsbuf);
	} else if (flag == 3 && snsbuf[7] >= 9 && (snsbuf[15] & 0x80)) {
		/*
		 * SKSV Data
		 */
		switch (skey) {
		case SKEY_ILLEGAL_REQUEST:
			if (snsbuf[15] & 0x8)
				(void)snprintf(rqsbuf, sizeof(rqsbuf),
				    "Error in %s, Offset %d, bit %d",
				    (snsbuf[15] & 0x40)? "CDB" : "Parameters",
				    (snsbuf[16] & 0xff) << 8 |
				    (snsbuf[17] & 0xff), snsbuf[15] & 0x7);
			else
				(void)snprintf(rqsbuf, sizeof(rqsbuf),
				    "Error in %s, Offset %d",
				    (snsbuf[15] & 0x40)? "CDB" : "Parameters",
				    (snsbuf[16] & 0xff) << 8 |
				    (snsbuf[17] & 0xff));
			return (rqsbuf);
		case SKEY_RECOVERED_ERROR:
		case SKEY_MEDIUM_ERROR:
		case SKEY_HARDWARE_ERROR:
			(void)snprintf(rqsbuf, sizeof(rqsbuf),
			    "Actual Retry Count: %d",
			    (snsbuf[16] & 0xff) << 8 | (snsbuf[17] & 0xff));
			return (rqsbuf);
		case SKEY_NOT_READY:
			(void)snprintf(rqsbuf, sizeof(rqsbuf),
			    "Progress Indicator: %d",
			    (snsbuf[16] & 0xff) << 8 | (snsbuf[17] & 0xff));
			return (rqsbuf);
		default:
			break;
		}
	}
	return (NULL);
}

int
scsipi_print_sense_real(struct scsipi_xfer *xs, int verbosity)
{
	scsipi_printaddr(xs->xs_periph);
 	printf(" Check Condition on CDB: ");
	scsipi_print_cdb(xs->cmd);
 	printf("\n");
	scsipi_print_sense_data(&xs->sense.scsi_sense, verbosity);
	return 1;
}
