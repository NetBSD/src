/* $NetBSD: uscsilib_machdep.h,v 1.1 2008/05/14 16:49:48 reinoud Exp $ */

/*
 * Copyright (c) 2003, 2004, 2005, 2006, 2008 Reinoud Zandijk
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#ifndef _USCSILIB_MACHDEP_H_
#define _USCSILIB_MACHDEP_H_

#ifndef _DEV_SCSIPI_SCSIPI_ALL_H_
#	define SSD_KEY         0x0F
#	define SSD_ILI         0x20
#	define SSD_EOM         0x40
#	define SSD_FILEMARK    0x80
#	define SKEY_NO_SENSE           0x00
#	define SKEY_RECOVERED_ERROR    0x01
#	define SKEY_NOT_READY          0x02
#	define SKEY_MEDIUM_ERROR       0x03
#	define SKEY_HARDWARE_ERROR     0x04
#	define SKEY_ILLEGAL_REQUEST    0x05
#	define SKEY_UNIT_ATTENTION     0x06
#	define SKEY_WRITE_PROTECT      0x07
#	define SKEY_BLANK_CHECK        0x08
#	define SKEY_VENDOR_UNIQUE      0x09
#	define SKEY_COPY_ABORTED       0x0A
#	define SKEY_ABORTED_COMMAND    0x0B
#	define SKEY_EQUAL              0x0C
#	define SKEY_VOLUME_OVERFLOW    0x0D
#	define SKEY_MISCOMPARE         0x0E
#	define SKEY_RESERVED           0x0F
#endif


#ifndef SCSI
#	define SCSI_READCMD  0
#	define SCSI_WRITECMD 0
struct scsi_addr;
int    scsilib_verbose;
#endif


#ifdef USCSI_SCSIPI
#	include <sys/scsiio.h>
#	define SCSI_READCMD   SCCMD_READ
#	define SCSI_WRITECMD  SCCMD_WRITE
#	define SCSI_NODATACMD SCCMD_WRITE
#endif


#ifdef USCSI_LINUX_SCSI
#	include <scsi/sg.h>
#	define SCSI_READCMD   SG_DXFER_FROM_DEV
#	define SCSI_WRITECMD  SG_DXFER_TO_DEV
#	define SCSI_NODATACMD SC_DXFER_NONE
#endif


#ifdef USCSI_FREEBSD_CAM
#	include <camlib.h>
#	include <cam/scsi/scsi_message.h>
#	define SCSI_READCMD	1
#	define SCSI_WRITECMD	2
#	define SCSI_NODATACMD	0
#endif


#endif /* _USCSILIB_MACHDEP_H_ */

