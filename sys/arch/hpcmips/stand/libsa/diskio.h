/*	$NetBSD: diskio.h,v 1.1 2000/08/29 15:10:16 takemura Exp $	*/

/*-
 * Copyright (c) 2000 Tomohide Saito.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* "fake" diskio.h */

#define DISK_IOCTL_GETINFO		1
#define DISK_IOCTL_READ			2
#define DISK_INFO_FLAG_MBR		0x00000001
#define DISK_INFO_FLAG_CHS_UNCERTAIN	0x00000002
#define DISK_INFO_FLAG_UNFORMATTED	0x00000004
#define DISK_INFO_FLAG_PAGEABLE		0x00000008

typedef struct {
	long pad1;
	DWORD di_bytes_per_sect;/* offset 4 */
	DWORD di_cylinders;	/* offset 8 */
	DWORD di_heads;		/* offset 12 */
	DWORD di_sectors;	/* offset 16 */
	DWORD di_flags;		/* offset 20 */
} DISK_INFO;			/* sizeof(DISK_INFO) = 24 */

typedef struct _SG_REQ {
	DWORD sr_start;		/* offset 0 */
	DWORD sr_num_sec;	/* offset 4 */
	DWORD sr_num_sg;	/* offset 8 */
	DWORD sr_status;	/* offset 12 */
	DWORD (*sr_callback)(struct _SG_REQ *);	/* offset 16 */
	struct {		/* offset 20 */
		PUCHAR sb_buf;		/* offset 0 */
		DWORD sb_len;		/* offset 4 */
	} sr_sglist[1];			/* sizeof(sr_sglist[]) = 8 */
} SG_REQ;			/* sizeof(SG_REQ) = 28 */
