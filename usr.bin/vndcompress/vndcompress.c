/* $Id: vndcompress.c,v 1.3 2005/07/27 09:29:02 he Exp $ */

/*
 * Copyright (c) 2005 by Florian Stoehr <netbsd@wolfnode.de>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Florian Stoehr
 * 4. The name of Florian Stoehr may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FLORIAN STOEHR ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
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
 * cloop2 - Compressed filesystem images
 * vndcompress program - Compress/decompress filesystem images to
 * the cloop2 format
 */
#include <arpa/inet.h>

#include <err.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#include "vndcompress.h"

enum opermodes {
	OM_COMPRESS,	/* Compress a fs       */
	OM_UNCOMPRESS,  /* Uncompress an image */
};

/*
 * This is the original header of the Linux files. It is useless
 * on NetBSD and integrated for compatibility issues only.
 */
static const char *cloop_sh = "#!/bin/sh\n" "#V2.0 Format\n" "insmod cloop.o file=$0 && mount -r -t iso9660 /dev/cloop $1\n" "exit $?\n";

int opmode;

/*
 * Print usage information, then exit program
 */
void
usage(void)
{
	if (opmode == OM_COMPRESS) {
		printf("usage: vndcompress [-cd] disk/fs-image compressed-image [blocksize]\n");
	} else {
		printf("usage: vnduncompress [-cd] compressed-image disk/fs-image\n");
	}
			   
	exit(EXIT_FAILURE);
}

/*
 * Compress a given file system
 */
void
vndcompress(const char *fs, const char *comp, uint32_t blocksize)
{
	int fd_in, fd_out;
	int total_blocks, offtable_size;
	int i;
	int read_blocksize;
	off_t fsize, diffatom, cursize;
	struct cloop_header clh;
	uint64_t *offtable;
	uint64_t curoff;
	unsigned long complen;
	unsigned char *cb, *ucb;
	
	fd_in = open(fs, O_RDONLY);
	
	if (fd_in < 0)
		err(EXIT_FAILURE, "Cannot open input file \"%s\"", fs);
		/* NOTREACHED */
		
	fd_out = open(comp, O_CREAT | O_TRUNC | O_WRONLY, 
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	
	if (fd_out < 0)
		err(EXIT_FAILURE, "Cannot create output file \"%s\"", comp);
		/* NOTREACHED */
		
	if ((blocksize % ATOMBLOCK) || (blocksize < ATOMBLOCK))
		errx(EXIT_FAILURE, "Invalid block size: %d (Block size must be "\
			"a multiple of %d Bytes)", blocksize, ATOMBLOCK);
		/* NOTREACHED */
		
	/*
	 * Init the compression
	 */
	 
	/* Determine number of total input blocks, round up to complete blocks */
	fsize = lseek(fd_in, 0, SEEK_END);
	lseek(fd_in, 0, SEEK_SET);	
	total_blocks = fsize / blocksize;

	printf("Using blocksize: %d ", blocksize);		

	if (fsize % blocksize) {
		printf("(%d complete and 1 zero-padded blocks)\n", total_blocks);
		total_blocks++;
	} else {
		printf("(%d complete blocks)\n", total_blocks);
	}
		
	/* Struct fillup */
	memset(&clh, 0, sizeof(struct cloop_header));
	memcpy(clh.sh, cloop_sh, strlen(cloop_sh));

	/* Remember the header is also in network format! */
	clh.block_size = htonl(blocksize);
	clh.num_blocks = htonl(total_blocks);
	
	/* Prepare the offset table (unsigned 64-bit big endian offsets) */
	offtable_size = (total_blocks + 1) * sizeof(uint64_t);
	offtable = (uint64_t *)malloc(offtable_size);
	
	/* 
	 * Setup block buffers.
	 * Since compression may actually stretch a block in bad cases,
	 * make the "compressed" space large enough here.
	 */
	ucb = (unsigned char *)malloc(blocksize);
	cb = (unsigned char *)malloc(blocksize * 2);
	
	/*
	 * Compression
	 * 
	 * We'll leave file caching to the operating system and write
	 * first the (fixed-size) header with dummy-data, then the compressed
	 * blocks block-by-block to disk. After that, we overwrite the offset
	 * table in the image file with the real offset table.
	 */
	if (write(fd_out, &clh, sizeof(struct cloop_header)) 
		< sizeof(struct cloop_header))
		err(EXIT_FAILURE, "Cannot write to output file \"%s\"", comp);
		/* NOTREACHED */
		
	if (write(fd_out, offtable, offtable_size) < offtable_size)
		err(EXIT_FAILURE, "Cannot write to output file \"%s\"", comp);
		/* NOTREACHED */
		
	/* 
	 * Offsets are relative to the beginning of the file, not
	 * relative to offset table start
	 */
	curoff = sizeof(struct cloop_header) + offtable_size; 
	
	/* Compression loop */
	for (i = 0; i < total_blocks; i++) {

		/* By default, assume to read blocksize bytes */
		read_blocksize = blocksize;
		
		/* 
		 * However, the last block may be smaller than block size.
		 * If this is the case, pad uncompressed buffer with zeros
		 * (by zero-filling before the read() call)
		 */
		if (i == total_blocks - 1) {
			if (fsize % blocksize) {		
				read_blocksize = fsize % blocksize;
				memset(ucb, 0x00, blocksize);
			}
		}
	
		if (read(fd_in, ucb, read_blocksize) < read_blocksize)
			err(EXIT_FAILURE, "Cannot read input file \"%s\"", fs);
			/* NOTREACHED */		
		
		complen = blocksize * 2;
			
		if (compress2(cb, &complen, ucb, blocksize, Z_BEST_COMPRESSION) != Z_OK)
			errx(EXIT_FAILURE, "Compression failed in block %d", i);
			/* NOTREACHED */

		if (write(fd_out, cb, complen) < complen)
			err(EXIT_FAILURE, "Cannot write to output file \"%s\"", comp);
			/* NOTREACHED */
		
		*(offtable + i) = SWAPPER(curoff);
		curoff += complen;
	}
	
	/* Always write +1 block to determine (compressed) block size */
	*(offtable + total_blocks) = SWAPPER(curoff);
	
	/* Fixup compression table */
	lseek(fd_out, sizeof(struct cloop_header), SEEK_SET);

	if (write(fd_out, offtable, offtable_size) < offtable_size)
		err(EXIT_FAILURE, "Cannot write to output file \"%s\"", comp);
		/* NOTREACHED */
		
	/* Finally, align the image size to be a multiple of ATOMBLOCK bytes */
	cursize = lseek(fd_out, 0, SEEK_END);
	
	if (cursize % ATOMBLOCK) {
		/*
		 * Reusing the cb buffer here. It *IS* large enough since
		 * blocksize may not be smaller than ATOMBLOCK
		 */
		diffatom = (((cursize / ATOMBLOCK) + 1) * ATOMBLOCK) - cursize;
		memset(cb, 0, blocksize * 2);
		write(fd_out, cb, diffatom);
	}
	
	free(cb);
	free(ucb);
	free(offtable);
		
	close(fd_in);
	close(fd_out);	
}

/*
 * Read in header and offset table from compressed image
 */
uint64_t *
readheader(int fd, struct cloop_header *clh, off_t *dstart)
{
	uint32_t offtable_size;
	uint64_t *offt;

	if (read(fd, clh, sizeof(struct cloop_header)) 
		< sizeof(struct cloop_header))
		return NULL;
		
	/* Convert endianness */
	clh->block_size = ntohl(clh->block_size);
	clh->num_blocks = ntohl(clh->num_blocks);
		
	offtable_size = (clh->num_blocks + 1) * sizeof(uint64_t);
	offt = (uint64_t *)malloc(offtable_size);
	
	if (read(fd, offt, offtable_size) < offtable_size) {
		free(offt);
		return NULL;
	}
		
	*dstart = offtable_size + sizeof(struct cloop_header);
		
	return offt;
}

/*
 * Decompress a given file system image
 */
void
vnduncompress(const char *comp, const char *fs)
{
	int fd_in, fd_out;
	int i;
	struct cloop_header clh;
	uint64_t *offtable;
	off_t imgofs, datastart;
	unsigned long complen, uncomplen;
	unsigned char *cb, *ucb;
	
	/*
	 * Setup decompression, read in header tables
	 */
	fd_in = open(comp, O_RDONLY);
	
	if (fd_in < 0)
		err(EXIT_FAILURE, "Cannot open input file \"%s\"", comp);
		/* NOTREACHED */
		
	fd_out = open(fs, O_CREAT | O_TRUNC | O_WRONLY, 
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	
	if (fd_out < 0)
		err(EXIT_FAILURE, "Cannot create output file \"%s\"", fs);
		/* NOTREACHED */
		
	offtable = readheader(fd_in, &clh, &datastart);
	
	if (offtable == NULL)
		errx(EXIT_FAILURE, "Input file \"%s\": Size mismatch, too small to be a valid image", comp);
		/* NOTREACHED */
		
	/* As for the compression, alloc the compressed block bigger */
	ucb = (unsigned char *)malloc(clh.block_size);
	cb = (unsigned char *)malloc(clh.block_size * 2);
	
	/*
	 * Perform the actual decompression
	 */		
	for (i = 0; i < clh.num_blocks; i++) {
		int rc;
		
		imgofs = SWAPPER(*(offtable + i));
		lseek(fd_in, imgofs, SEEK_SET);
		
		complen = SWAPPER(*(offtable + i + 1))
		           - SWAPPER(*(offtable + i));
		
		if (read(fd_in, cb, complen) < complen)
			err(EXIT_FAILURE, "Cannot read compressed block %d from \"%s\"", i, comp);
			/* NOTREACHED */

		uncomplen = clh.block_size;
		rc = uncompress(ucb, &uncomplen, cb, complen);
		if (rc != Z_OK)
			errx(EXIT_FAILURE, "Cannot decompress block %d from \"%s\" (rc=%d)",
			    i, comp, rc);
			/* NOTREACHED */
			
		write(fd_out, ucb, clh.block_size);		
	}

	free(cb);
	free(ucb);
	free(offtable);	
		
	close(fd_in);
	close(fd_out);	
}

/*
 * vndcompress: Handle cloop2-compatible compressed file systems; This is the
 *              user-level configuration program, to be used in conjunction
 *              with the vnd(4) driver.
 */
int
main(int argc, char **argv)
{
	char *ep, *p;
	int ch;
	
	setprogname(argv[0]);
	
	if ((p = strrchr(argv[0], '/')) == NULL)
		p = argv[0];
	else
		++p;
	if (strcmp(p, "vnduncompress") == 0)
		opmode = OM_UNCOMPRESS;
        else if (strcmp(p, "vndcompress") == 0)
                opmode = OM_COMPRESS;
        else
		warnx("unknown program name '%s'", p);
	
	/* Process command-line options */	
	while ((ch = getopt(argc, argv, "cd")) != -1) {
		switch (ch) {
		case 'c':
			opmode = OM_COMPRESS;
			break;				
			
		case 'd':
			opmode = OM_UNCOMPRESS;
			break;
			
		default:
			usage();
			/* NOTREACHED */
		}
	}
	
	argc -= optind;
	argv += optind;

	if (argc < 2) {
		usage();
		/* NOTREACHED */
	}
		
	switch (opmode) {
	case OM_COMPRESS:
		if (argc > 2) {
			vndcompress(argv[0], argv[1], strtoul(argv[2], &ep, 10));
		} else {
			vndcompress(argv[0], argv[1], DEF_BLOCKSIZE);
		}
		break;
		
	case OM_UNCOMPRESS:
		vnduncompress(argv[0], argv[1]);
		break;
	}

	exit(EXIT_SUCCESS);
}
