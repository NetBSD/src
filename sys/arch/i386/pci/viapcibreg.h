/* $NetBSD: viapcibreg.h,v 1.1.8.2 2006/04/22 11:37:34 simonb Exp $ */

/*-
 * Copyright (c) 2005, 2006 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS_ARCH_I386_PCI_VIAPCIBREG_H
#define _SYS_ARCH_I386_PCI_VIAPCIBREG_H

/* PCI configuration registers for SMB base address; chip dependent */
#define	SMB_BASE1		0x90
#define	SMB_BASE2		0x80
#define	SMB_BASE3		0xd0

#define	SMB_HOST_CONFIG		0xd2
#define	SMB_REVISION		(SMB_HOST_CONFIG + 4)

/* SMBus register offsets; from Linux and FreeBSD */
#define	SMBHSTSTS		0x00
#define		SMBHSTSTS_BUSY		0x01
#define		SMBHSTSTS_INTR		0x02
#define		SMBHSTSTS_ERROR		0x04
#define		SMBHSTSTS_COLLISION	0x08
#define		SMBHSTSTS_FAILED	0x10
#define	SMBHSLVSTS		0x01
#define	SMBHSTCNT		0x02
#define		SMBHSTCNT_QUICK		0x00
#define		SMBHSTCNT_KILL		0x02
#define		SMBHSTCNT_SENDRECV	0x04
#define		SMBHSTCNT_BYTE		0x08
#define		SMBHSTCNT_WORD		0x0c
#define		SMBHSTCNT_BLOCK		0x14
#define		SMBHSTCNT_START		0x40
#define	SMBHSTCMD		0x03
#define	SMBHSTADD		0x04
#define	SMBHSTDAT0		0x05
#define	SMBHSTDAT1		0x06
#define	SMBBLKDAT		0x07
#define	SMBSLVCNT		0x08
#define	SMBSHDWCMD		0x09
#define	SMBSLVEVT		0x0a
#define	SMBSLVDAT		0x0c

#endif /* !_SYS_ARCH_I386_PCI_VIAPCIBREG_H */
