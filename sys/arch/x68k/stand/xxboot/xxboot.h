/*	$NetBSD: xxboot.h,v 1.4 2020/08/16 06:43:43 isaki Exp $	*/

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
extern int raw_read(uint32_t, uint32_t, void *);
extern int badbaddr(volatile void *);
extern void BOOT_ERROR(const char *) __dead;
extern uint32_t BOOT_INFO;	/* result of IOCS(__BOOTINF) */
extern uint32_t SCSI_ID;	/* target SCSI ID */
extern uint32_t SCSI_PARTTOP;	/* top sector # of this partition */
extern uint32_t SCSI_BLKLEN;	/* sector length index */
extern struct {
	uint32_t total_blocks;
	uint32_t blocksize;
} SCSI_CAP;
extern struct {
	struct fdfmt{
		uint8_t	N;	/* sector length 1:256, 2:512, 3:1024 */
		uint8_t	C;	/* cylinder # */
		uint8_t	H;	/* head # */
		uint8_t	R;	/* sector # */
	} minsec, maxsec;
} FDSEC;			/* FD format type of the first track */

/* bootmain.c */
extern void print_hex(u_int, int);

/* consio1.c */
extern int getchar(void);
extern void putchar(int);

/* xx.c */
extern int xxopen(struct open_file *, ...);
extern int xxclose(struct open_file *);
extern int xxstrategy(void *, int, daddr_t, size_t, void *, size_t *);

/* xxboot.ldscript */
extern uint32_t startregs[16];

/* vers.c */
extern const char bootprog_name[];
extern const char bootprog_rev[];
