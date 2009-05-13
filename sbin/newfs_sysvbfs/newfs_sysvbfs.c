/*	$NetBSD: newfs_sysvbfs.c,v 1.2.6.1 2009/05/13 19:19:04 jym Exp $	*/

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

#include <sys/types.h>
#include <sys/param.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <assert.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>

#include <fs/sysvbfs/bfs.h>

static void usage(void);
static int bfs_newfs(int, uint32_t);

int
main(int argc, char **argv)
{
	const char *device;
	struct disklabel d;
	struct partition *p;
	struct stat st;
	uint32_t partsize;
	int Fflag, Zflag;
	int part;
	int fd, ch;

	if (argc < 2)
		usage();

	Fflag = Zflag = partsize = 0;
	while ((ch = getopt(argc, argv, "Fs:Z")) != -1) {
		switch (ch) {
		case 'F':
			Fflag = 1;
			break;
		case 's':
			partsize = atoi(optarg);
			break;
		case 'Z':
			Zflag = 1;
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();
	device = argv[0];

	if (!Fflag) {
		if ((fd = open(device, O_RDWR)) == -1) {
			perror("open device");
			exit(EXIT_FAILURE);
		}
		if (fstat(fd, &st) != 0) {
			perror("device stat");
			goto err_exit;
		}
		if (!S_ISCHR(st.st_mode)) {
			fprintf(stderr, "WARNING: not a raw device.\n");
		}

		part = DISKPART(st.st_rdev);

		if (ioctl(fd, DIOCGDINFO, &d) == -1) {
			perror("disklabel");
			goto err_exit;
		}
		p = &d.d_partitions[part];
		printf("partition = %d\n", part);
		printf("size=%d offset=%d fstype=%d secsize=%d\n",
		    p->p_size, p->p_offset, p->p_fstype, d.d_secsize);

		if (p->p_fstype != FS_SYSVBFS) {
			fprintf(stderr, "not a SysVBFS partition.\n");
			goto err_exit;
		}
		partsize = p->p_size;
	} else {
		off_t filesize;
		uint8_t zbuf[8192] = {0, };

		if (partsize == 0) {
			warnx("-F requires -s");
			exit(EXIT_FAILURE);
		}

		filesize = partsize << BFS_BSHIFT;

		fd = open(device, O_RDWR|O_CREAT|O_TRUNC, 0666);
		if (fd == -1) {
			perror("open file");
			exit(EXIT_FAILURE);
		}

		if (Zflag) {
			while (filesize > 0) {
				size_t writenow = MIN(filesize, (off_t)sizeof(zbuf));

				if ((size_t)write(fd, zbuf, writenow) != writenow) {
					perror("zwrite");
					exit(EXIT_FAILURE);
				}
				filesize -= writenow;
			}
		} else {
			if (lseek(fd, filesize-1, SEEK_SET) == -1) {
				perror("lseek");
				exit(EXIT_FAILURE);
			}
			if (write(fd, zbuf, 1) != 1) {
				perror("write");
				exit(EXIT_FAILURE);
			}
			if (lseek(fd, 0, SEEK_SET) == -1) {
				perror("lseek 2");
				exit(EXIT_FAILURE);
			}
		}
	}

	if (bfs_newfs(fd, partsize) != 0)
		goto err_exit;

	close(fd);

	return 0;
 err_exit:
	close(fd);
	exit(EXIT_FAILURE);
}

static int
bfs_newfs(int fd, uint32_t nsectors)
{
	uint8_t buf[DEV_BSIZE];
	struct bfs_super_block *bfs = (void *)buf;
	struct bfs_inode *inode = (void *)buf;
	struct bfs_dirent *dirent = (void *)buf;
	time_t t = time(0);
	int error;

	/* Super block */
	memset(buf, 0, DEV_BSIZE);
	bfs->header.magic = BFS_MAGIC;
	bfs->header.data_start_byte = DEV_BSIZE * 2; /* super block + inode */
	bfs->header.data_end_byte = nsectors * BFS_BSIZE - 1;
	bfs->compaction.from = 0xffffffff;
	bfs->compaction.to = 0xffffffff;
	bfs->compaction.from_backup = 0xffffffff;
	bfs->compaction.to_backup = 0xffffffff;

	if ((error = lseek(fd, 0, SEEK_SET)) == -1) {
		perror("seek super block");
		return -1;
	}
	if (write(fd, buf, BFS_BSIZE) < 0) {
		perror("write super block");
		return -1;
	}

	/* i-node table */
	memset(buf, 0, BFS_BSIZE);
	inode->number = BFS_ROOT_INODE;
	inode->start_sector = 2;
	inode->end_sector = 2;
	inode->eof_offset_byte = sizeof(struct bfs_dirent) +
	    inode->start_sector * BFS_BSIZE;
	inode->attr.atime = t;
	inode->attr.mtime = t;
	inode->attr.ctime = t;
	inode->attr.mode = 0755;
	inode->attr.type = 2;	/* DIR */
	inode->attr.nlink = 2;	/* . + .. */
	if (write(fd, buf, BFS_BSIZE) < 0) {
		perror("write i-node");
		return -1;
	}

	/* dirent table */
	memset(buf, 0, BFS_BSIZE);
	dirent->inode = BFS_ROOT_INODE;
	sprintf(dirent->name, ".");
	dirent++;
	dirent->inode = BFS_ROOT_INODE;
	sprintf(dirent->name, "..");
	if (write(fd, buf, BFS_BSIZE) < 0) {
		perror("write dirent");
		return -1;
	}

	return 0;
}

static void
usage(void)
{

	(void)fprintf(stderr, "usage: %s [-FZ] [-s sectors] special-device\n",
	    getprogname());
	exit(EXIT_FAILURE);
}
