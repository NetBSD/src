/*	$NetBSD: xxboot.h,v 1.3 2020/08/14 03:34:22 isaki Exp $	*/

/*
 * Copyright (C) 2020 Tetsuya Isaki. All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define BINF_ISFD(pbinf)	(*((uint8_t *)(pbinf) + 1) == 0)

/* boot.S */
extern void RAW_READ(void *, uint32_t, size_t);
extern int badbaddr(volatile void *);
extern unsigned int BOOT_INFO;	/* result of IOCS(__BOOTINF) */
extern unsigned int ID;		/* target SCSI ID */
extern struct {
	struct fdfmt{
		uint8_t	N;	/* sector length 0: 128, ..., 3: 1K */
		uint8_t	C;	/* cylinder # */
		uint8_t	H;	/* head # */
		uint8_t	R;	/* sector # */
	} minsec, maxsec;
} FDSECMINMAX;			/* FD format type of the first track */

/* xx.c */
extern int xxopen(struct open_file *, ...);
extern int xxclose(struct open_file *);
extern int xxstrategy(void *, int, daddr_t, size_t, void *, size_t *);

/* vers.c */
extern const char bootprog_name[];
extern const char bootprog_rev[];
