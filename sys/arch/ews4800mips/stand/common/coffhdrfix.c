/*	$NetBSD: coffhdrfix.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

/* fixup GNU binutils file offset error. */

#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

typedef uint8_t Coff_Byte;
typedef uint8_t Coff_Half[2];
typedef uint8_t Coff_Word[4];

#define	ECOFF_OMAGIC		0407
#define	ECOFF_MAGIC_MIPSEB	0x0160

struct coff_filehdr {
	Coff_Half	f_magic;
	Coff_Half	f_nscns;
	Coff_Word	f_timdat;
	Coff_Word	f_symptr;
	Coff_Word	f_nsyms;
	Coff_Half	f_opthdr;
	Coff_Half	f_flags;
};

struct coff_aouthdr {
	Coff_Half	a_magic;
	Coff_Half	a_vstamp;
	Coff_Word	a_tsize;
	Coff_Word	a_dsize;
	Coff_Word	a_bsize;
	Coff_Word	a_entry;
	Coff_Word	a_tstart;
	Coff_Word	a_dstart;
};

#define	COFF_GET_HALF(w)	((w)[1] | ((w)[0] << 8))
#define	COFF_SET_HALF(w,v)	((w)[1] = (uint8_t)(v),			\
				(w)[0] = (uint8_t)((v) >> 8))
#define	COFF_GET_WORD(w)	((w)[3] | ((w)[2] << 8) | 		\
				((w)[1] << 16) | ((w)[0] << 24))
#define	COFF_SET_WORD(w,v)	((w)[3] = (uint8_t)(v),			\
				(w)[2] = (uint8_t)((v) >> 8),		\
				(w)[1] = (uint8_t)((v) >> 16),		\
				(w)[0] = (uint8_t)((v) >> 24))

#define	FILHSZ	sizeof(struct coff_filehdr)
#define	SCNHSZ	40

int
main(int argc, char *argp[])
{
	int fd, fdout, fileoff, i;
	struct coff_filehdr file;
	struct coff_aouthdr aout;
	char fname[256];
	char buf[1024];

	if (argc < 3)
		return 0;

	if ((fd = open(argp[1], O_RDWR)) < 0) {
		perror(0);
		return 0;
	}
	read(fd, &file, sizeof file);
	read(fd, &aout, sizeof aout);

	if (COFF_GET_HALF(file.f_magic) != ECOFF_MAGIC_MIPSEB) {
		fprintf (stderr, "not COFF file.\n");
		return 0;
	}
	if (COFF_GET_HALF(aout.a_magic) != ECOFF_OMAGIC) {
		fprintf (stderr, "not OMAGIC.\n");
		return 0;
	}
	fprintf(stderr, "File: magic: 0x%04x flags: 0x%04x\n",
	    COFF_GET_HALF(file.f_magic), COFF_GET_HALF(file.f_flags));
	fprintf(stderr, "Aout: magic: 0x%04x vstamp: %d\n",
	    COFF_GET_HALF(aout.a_magic), COFF_GET_HALF(aout.a_vstamp));

	fileoff = (FILHSZ +
	    COFF_GET_HALF(file.f_opthdr) +
	    COFF_GET_HALF(file.f_nscns) * SCNHSZ + 7) & ~7;
	fprintf(stderr, "File offset: 0x%x\n", fileoff);
	close(fd);

	if ((fdout = open(argp[2], O_CREAT | O_TRUNC | O_RDWR, 0644)) < 0) {
		perror (0);
		return 0;
	}
	fd = open(argp[1], O_RDWR);
	read(fd, buf, fileoff);
	write(fdout, buf, fileoff);
	lseek(fd, 8, SEEK_CUR);
	while ((i = read(fd, buf, 1024)) > 0)
		write(fdout, buf, i);
	close(fd);
	close(fdout);

	return 0;
}
