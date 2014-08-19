/*-
 * Copyright (c) 2007,2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alistair Crooks (agc@netbsd.org)
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

#define FUSE_USE_VERSION	26

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "scsi_cmd_codes.h"
#include "iscsi.h"
#include "initiator.h"
#include "tests.h"

#include "virtdir.h"

#if defined(__NetBSD__) && defined(USE_LIBKMOD)
#include "libkmod.h"
#endif

#include "defs.h"

static int		 verbose; /* how chatty are we? */

static virtdir_t	 iscsi;

enum {
	VendorLen = 8,
	ProductLen = 16,
	VersionLen = 4,

	SGsize = 131072
};


/* this struct keeps information on the target */
typedef struct targetinfo_t {
	 char			*host;		/* resolvable host name */
	 char			*ip;		/* textual IP address */
	 char			*targetname;	/* name of iSCSI target prog */
	 char			*stargetname;   /* short name of the target */ 
	 uint64_t		 target;	/* target number */
	 uint32_t		 lun;		/* LUN number */
	 uint32_t		 lbac;		/* number of LBAs */
	 uint32_t		 blocksize;	/* size of device blocks */
	 uint32_t		 devicetype;	/* SCSI device type */
	 char			 vendor[VendorLen + 1];
	 					/* device vendor information */
	 char			 product[ProductLen + 1];
	 					/* device product information */
	 char			 version[VersionLen + 1];
	 					/* device version information */
	 char			*serial;	/* unit serial number */
} targetinfo_t;

DEFINE_ARRAY(targetv_t, targetinfo_t);

static targetv_t	tv;	/* target vector of targetinfo_t structs */

/* iqns and target addresses are returned as pairs in this dynamic array */
static strv_t		all_targets;

/* Small Target Info... */
typedef struct sti_t {
	struct stat             st;		/* normal stat info */
	uint64_t                target;		/* cached target number, so we don't have an expensive pathname-based lookup */
} sti_t;

#ifndef __UNCONST
#define __UNCONST(x)	(x)
#endif

static void
lba2cdb(uint8_t *cdb, uint32_t *lba, uint16_t *len)
{
	/* Some platforms (like strongarm) aligns on */
	/* word boundaries.  So HTONL and NTOHL won't */
	/* work here. */
	int	little_endian = 1;

	if (*(char *) (void *) &little_endian) {
		/* little endian */
		cdb[2] = ((uint8_t *) (void *)lba)[3];
		cdb[3] = ((uint8_t *) (void *)lba)[2];
		cdb[4] = ((uint8_t *) (void *)lba)[1];
		cdb[5] = ((uint8_t *) (void *)lba)[0];
		cdb[7] = ((uint8_t *) (void *)len)[1];
		cdb[8] = ((uint8_t *) (void *)len)[0];
	} else {
		/* big endian */
		cdb[2] = ((uint8_t *) (void *)lba)[2];
		cdb[3] = ((uint8_t *) (void *)lba)[3];
		cdb[4] = ((uint8_t *) (void *)lba)[0];
		cdb[5] = ((uint8_t *) (void *)lba)[1];
		cdb[7] = ((uint8_t *) (void *)len)[0];
		cdb[8] = ((uint8_t *) (void *)len)[1];
	}
}

/* read the capacity (maximum LBA and blocksize) from the target */
int 
read_capacity(uint64_t target, uint32_t lun, uint32_t *maxlba, uint32_t *blocklen)
{
	iscsi_scsi_cmd_args_t	args;
	initiator_cmd_t		cmd;
	uint8_t			data[8];
	uint8_t			cdb[16];

	(void) memset(cdb, 0x0, sizeof(cdb));
	cdb[0] = READ_CAPACITY;
	cdb[1] = lun << 5;

	(void) memset(&args, 0x0, sizeof(args));
	args.recv_data = data;
	args.input = 1;
	args.lun = lun;
	args.trans_len = 8;
	args.cdb = cdb;

	(void) memset(&cmd, 0, sizeof(initiator_cmd_t));

	cmd.isid = target;
	cmd.type = ISCSI_SCSI_CMD;
	cmd.ptr = &args;

	if (initiator_command(&cmd) != 0) {
		iscsi_err(__FILE__, __LINE__, "initiator_command() failed\n");
		return -1;
	}
	if (args.status) {
		iscsi_err(__FILE__, __LINE__, "READ_CAPACITY failed (status %#x)\n", args.status);
		return -1;
	}
	memcpy(maxlba, data, sizeof(*maxlba));
	*maxlba = ISCSI_NTOHL(*maxlba);
	if (*maxlba == 0) {
		iscsi_err(__FILE__, __LINE__, "Device returned Maximum LBA of zero\n");
		return -1;
	}
	memcpy(blocklen, data + 4, sizeof(*blocklen));
	*blocklen = ISCSI_NTOHL(*blocklen);
	if (*blocklen % 2) {
		iscsi_err(__FILE__, __LINE__, "Device returned strange block len: %u\n", *blocklen);
		return -1;
	}
	return 0;
}

/* send inquiry command to the target, to get it to identify itself */
static int 
inquiry(uint64_t target, uint32_t lun, uint8_t type, uint8_t inquire, uint8_t *data)
{
	iscsi_scsi_cmd_args_t	args;
	initiator_cmd_t		cmd;
	uint8_t			cdb[16];

	(void) memset(cdb, 0x0, sizeof(cdb));
	cdb[0] = INQUIRY;
	cdb[1] = type | (lun << 5);
	cdb[2] = inquire;
	cdb[4] = 256 - 1;

	(void) memset(&args, 0x0, sizeof(args));
	args.input = 1;
	args.trans_len = 256;
	args.cdb = cdb;
	args.lun = lun;
	args.recv_data = data;
	(void) memset(&cmd, 0x0, sizeof(cmd));
	cmd.isid = target;
	cmd.type = ISCSI_SCSI_CMD;
	cmd.ptr = &args;

	if (initiator_command(&cmd) != 0) {
		iscsi_err(__FILE__, __LINE__, "initiator_command() failed\n");
		return -1;
	}
	if (args.status) {
		iscsi_err(__FILE__, __LINE__, "INQUIRY failed (status %#x)\n", args.status);
		return -1;
	}

	return 0;
}

/* read or write a single block of information */
static int 
blockop(uint64_t target, uint32_t lun, uint32_t lba, uint32_t len,
	      uint32_t blocklen, uint8_t *data, int writing)
{
	iscsi_scsi_cmd_args_t	args;
	initiator_cmd_t		cmd;
	uint16_t   		readlen;
	uint8_t   		cdb[16];

	/* Build CDB */
	(void) memset(cdb, 0, 16);
	cdb[0] = (writing) ? WRITE_10 : READ_10;
	cdb[1] = lun << 5;
	readlen = (uint16_t) len;
	lba2cdb(cdb, &lba, &readlen);

	/* Build SCSI command */
	(void) memset(&args, 0x0, sizeof(args));
	if (writing) {
		args.send_data = data;
		args.output = 1;
	} else {
		args.recv_data = data;
		args.input = 1;
	}
	args.lun = lun;
	args.trans_len = len*blocklen;
	args.length = len*blocklen;
	args.cdb = cdb;
	(void) memset(&cmd, 0, sizeof(initiator_cmd_t));
	cmd.isid = target;
	cmd.type = ISCSI_SCSI_CMD;
	cmd.ptr = &args;
	/* Execute iSCSI command */
	if (initiator_command(&cmd) != 0) {
		iscsi_err(__FILE__, __LINE__, "initiator_command() failed\n");
		return -1;
	}

	if (args.status) {
		iscsi_err(__FILE__, __LINE__, "scsi_command() failed (status %#x)\n", args.status);
		return -1;
	}
	return 0;
}

/* perform a scatter/gather block operation */
static int 
sgblockop(uint64_t target, uint32_t lun, uint32_t lba, uint32_t len,
	      uint32_t blocklen, uint8_t *data, int sglen, int writing)
{
	iscsi_scsi_cmd_args_t	args;
	initiator_cmd_t		cmd;
	uint16_t		readlen;
	uint8_t			cdb[16];

	/* Build CDB */

	(void) memset(cdb, 0, 16);
	cdb[0] = (writing) ? WRITE_10 : READ_10;
	cdb[1] = lun << 5;
	readlen = (uint16_t) len;
	lba2cdb(cdb, &lba, &readlen);

	/* Build iSCSI command */
	(void) memset(&args, 0x0, sizeof(args));
	args.lun = lun;
	args.output = (writing) ? 1 : 0;
	args.input = (writing) ? 0 : 1;
	args.trans_len = len * blocklen;
	args.length = len * blocklen;
	args.send_data = (writing) ? data : NULL;
	args.send_sg_len = (writing) ? sglen : 0;
	args.recv_data = (writing) ? NULL : data;
	args.recv_sg_len = (writing) ? 0 : sglen;
	args.cdb = cdb;
	memset(&cmd, 0, sizeof(initiator_cmd_t));
	cmd.isid = target;
	cmd.ptr = &args;
	cmd.type = ISCSI_SCSI_CMD;

	/* Execute iSCSI command */

	if (initiator_command(&cmd) != 0) {
		iscsi_err(__FILE__, __LINE__, "initiator_command() failed\n");
		return -1;
	}
	if (args.status) {
		iscsi_err(__FILE__, __LINE__, "scsi_command() failed (status %#x)\n", args.status);
		return -1;
	}
	return 0;
}

/* read info from the target - method depends on size of data being read */
static int 
targetop(uint32_t t, uint64_t offset, uint32_t length, uint32_t request, char *buf, int writing)
{
	struct iovec 	*iov;
	uint32_t	 ioc;
	uint32_t	 i;
	int		 req_len;

	if (request > SGsize) {
		/* split up request into blocksize chunks */
		ioc = request / SGsize;
		if ((ioc * SGsize) < request)
			ioc++;
		if ((iov = iscsi_malloc(ioc * sizeof(*iov))) == NULL) {
			iscsi_err(__FILE__, __LINE__, "out of memory\n");
			return -1;
		}

		for (i = 0 ; i < ioc ; i++) {
			iov[i].iov_base = &buf[i * SGsize];
			if (i == (ioc - 1)) { /* last one */
				iov[i].iov_len = request - (i * SGsize);
			} else {
				iov[i].iov_len = SGsize;
			}
		}

		if (sgblockop(tv.v[t].target, tv.v[t].lun, offset / tv.v[t].blocksize, (length / tv.v[t].blocksize), tv.v[t].blocksize, (uint8_t *) iov, ioc, writing) != 0) {
			iscsi_free(iov);
			iscsi_err(__FILE__, __LINE__, "read_10() failed\n");
			return -1;
		}
		iscsi_free(iov);
	} else {
		req_len = length / tv.v[t].blocksize;
		if ((req_len * tv.v[t].blocksize) < length)
			req_len++;
		if (blockop(tv.v[t].target, tv.v[t].lun, offset / tv.v[t].blocksize, 
				req_len, tv.v[t].blocksize, (uint8_t *) buf, writing) != 0) {
			iscsi_err(__FILE__, __LINE__, "read_10() failed\n");
			return -1;
		}
	}
	return 0;
}


/****************************************************************************/

/* perform the stat operation */
/* if this is the root, then just synthesise the data */
/* otherwise, retrieve the data, and be sure to fill in the size */
static int 
iscsifs_getattr(const char *path, struct stat *st)
{
	virt_dirent_t	*ep;
	sti_t		*p;

	if (strcmp(path, "/") == 0) {
		(void) memset(st, 0x0, sizeof(*st));
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
		return 0;
	}
	if ((ep = virtdir_find(&iscsi, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	switch(ep->type) {
	case 'b':
		(void) memcpy(st, &iscsi.file, sizeof(*st));
		st->st_mode = (S_IFBLK | 0644);
		break;
	case 'c':
		(void) memcpy(st, &iscsi.file, sizeof(*st));
		st->st_mode = (S_IFCHR | 0644);
		break;
	case 'd':
		(void) memcpy(st, &iscsi.dir, sizeof(*st));
		break;
	case 'f':
		(void) memcpy(st, &iscsi.file, sizeof(*st));
		p = (sti_t *) ep->tgt;
		st->st_size = p->st.st_size;
		break;
	case 'l':
		(void) memcpy(st, &iscsi.lnk, sizeof(*st));
		st->st_size = ep->tgtlen;
		break;
	default:
		warn("unknown directory type `%c'", ep->type);
		return -ENOENT;
	}
	st->st_ino = ep->ino;
	return 0;
}

/* readdir operation */
static int 
iscsifs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	      off_t offset, struct fuse_file_info * fi)
{
	virt_dirent_t	*dp;
	VIRTDIR		*dirp;

	if ((dirp = openvirtdir(&iscsi, path)) == NULL) {
		return 0;
	}
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	while ((dp = readvirtdir(dirp)) != NULL) {
		filler(buf, dp->d_name, NULL, 0);
	}
	closevirtdir(dirp);
	return 0;
}

/* open the file in the file system */
static int 
iscsifs_open(const char *path, struct fuse_file_info *fi)
{
	virt_dirent_t	*ep;
	const char	*slash;

	if ((ep = virtdir_find(&iscsi, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	/* check path is the correct one */
	if ((slash = strrchr(path, '/')) == NULL) {
		slash = path;
	} else {
		slash += 1;
	}
	if (strcmp(slash, "storage") != 0) {
		return -ENOENT;
	}
	return 0;
}

/* read the storage from the iSCSI target */
static int 
iscsifs_read(const char *path, char *buf, size_t size, off_t offset,
	   struct fuse_file_info * fi)
{
	virt_dirent_t	*ep;
	uint64_t target;
	sti_t *p;

	if ((ep = virtdir_find(&iscsi, path, strlen(path))) == NULL) {
		return -ENOENT;
	}

	p = (sti_t *)ep->tgt;
	target = p->target;

	if (targetop(target, offset, size, size, buf, 0) < 0) {
		return -EPERM;
	}
	return size;
}

/* write the file's contents to the file system */
static int 
iscsifs_write(const char *path, const char *buf, size_t size, off_t offset,
	   struct fuse_file_info * fi)
{
	virt_dirent_t	*ep;
        uint64_t	 target;   
        sti_t		*p;

	if ((ep = virtdir_find(&iscsi, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	
	p = (sti_t *)ep->tgt;
	target = p->target;

	if (targetop(target, offset, size, size, __UNCONST(buf), 1) < 0) {
		return -EPERM;
	}
	return size;
}

/* fill in the statvfs struct */
static int
iscsifs_statfs(const char *path, struct statvfs *st)
{
	(void) memset(st, 0x0, sizeof(*st));
	return 0;
}

/* read the symbolic link */
static int
iscsifs_readlink(const char *path, char *buf, size_t size)
{
	virt_dirent_t	*ep;

	if ((ep = virtdir_find(&iscsi, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	if (ep->tgt == NULL) {
		return -ENOENT;
	}
	(void) strlcpy(buf, ep->tgt, size);
	return 0;
}

/* operations struct */
static struct fuse_operations iscsiops = {
	.getattr = iscsifs_getattr,
	.readlink = iscsifs_readlink,
	.readdir = iscsifs_readdir,
	.open = iscsifs_open,
	.read = iscsifs_read,
	.write = iscsifs_write,
	.statfs = iscsifs_statfs
};

int 
main(int argc, char **argv)
{
	iscsi_initiator_t	ini;
	initiator_target_t	tinfo;
	unsigned		u;
	uint32_t		lbac;
	uint32_t		blocksize;
	uint8_t			data[256];
	sti_t			sti;
	char			hostname[1024];
	char			name[1024];
	char			*colon;
	char		       *host;
	char			buf[32];
	char			devtype;
	int			discover;
	int			cc;
	int			i;
	uint32_t		max_targets;

	(void) memset(&tinfo, 0x0, sizeof(tinfo));
	iscsi_initiator_set_defaults(&ini);
	(void) gethostname(host = hostname, sizeof(hostname));
	discover = 0;
	(void) stat("/etc/hosts", &sti.st);
	devtype = 'f';
	iscsi_initiator_setvar(&ini, "address family", "4");
	max_targets = iscsi_initiator_get_max_targets();
	
	while ((i = getopt(argc, argv, "46a:bcd:Dfh:p:t:u:v:V")) != -1) {
		switch(i) {
		case '4':
		case '6':
			buf[0] = i;
			buf[1] = 0x0;
			iscsi_initiator_setvar(&ini, "address family", buf);
			break;
		case 'a':
			iscsi_initiator_setvar(&ini, "auth type", optarg);
			break;
		case 'b':
			devtype = 'b';
			break;
		case 'c':
			devtype = 'c';
			break;
		case 'd':
			iscsi_initiator_setvar(&ini, "digest type", optarg);
			break;
		case 'D':
			discover = 1;
			break;
		case 'f':
			devtype = 'f';
			break;
		case 'h':
			iscsi_initiator_setvar(&ini, "target hostname", optarg);
			break;
		case 'p':
			iscsi_initiator_setvar(&ini, "target port", optarg);
			break;
		case 't':
			iscsi_initiator_setvar(&ini, "target instance", optarg);
			break;
		case 'u':
			iscsi_initiator_setvar(&ini, "user", optarg);
			break;
		case 'V':
			(void) printf("\"%s\" %s\nPlease send all bug reports "
					"to %s\n",
					PACKAGE_NAME,
					PACKAGE_VERSION,
					PACKAGE_BUGREPORT);
			exit(EXIT_SUCCESS);
			/* NOTREACHED */
		case 'v':
			verbose += 1;
			if (strcmp(optarg, "net") == 0) {
				iscsi_initiator_setvar(&ini, "debug", "net");
			} else if (strcmp(optarg, "iscsi") == 0) {
				iscsi_initiator_setvar(&ini, "debug", "iscsi");
			} else if (strcmp(optarg, "scsi") == 0) {
				iscsi_initiator_setvar(&ini, "debug", "scsi");
			} else if (strcmp(optarg, "all") == 0) {
				iscsi_initiator_setvar(&ini, "debug", "all");
			}
			break;
		default:
			(void) fprintf(stderr, "%s: unknown option `%c'",
					*argv, i);
		}
	}
	if (!strcmp(iscsi_initiator_getvar(&ini, "auth type"), "chap") &&
	    iscsi_initiator_getvar(&ini, "user") == NULL) {
		iscsi_err(__FILE__, __LINE__, "user must be specified with "
		    "-u if using CHAP authentication\n");
		exit(EXIT_FAILURE);
	}

	if (strcmp(iscsi_initiator_getvar(&ini, "auth type"), "none") &&
	    iscsi_initiator_getvar(&ini, "user") != NULL) {
		/* 
		 * For backwards compatibility, default to using CHAP
		 * if username given
		 */
		iscsi_initiator_setvar(&ini, "auth type", "chap");
	}

	if (iscsi_initiator_start(&ini) == -1) {
		iscsi_err(__FILE__, __LINE__, "initiator_init() failed\n");
		exit(EXIT_FAILURE);
	}

        if (iscsi_initiator_discover(host, 0, 0) < 0) {
                printf("initiator_discover() in discover failed\n");
		exit(EXIT_FAILURE);
	}

        if (iscsi_initiator_get_targets(0,&all_targets) == -1) {
 		iscsi_err(__FILE__, __LINE__,
			"initiator_get_targets() failed\n");
               	exit(EXIT_FAILURE);
	}


        if (discover) {
		printf("Targets available from host %s:\n",host);
		for (u = 0; u < all_targets.c ; u += 2) {
			printf("%s at %s\n", all_targets.v[u],
				all_targets.v[u + 1]);
		}

                exit(EXIT_SUCCESS);
        }

	if (all_targets.c/2 > max_targets) {
		(void) fprintf(stderr,
			"CONFIG_INITIATOR_NUM_TARGETS in initiator.h "
			"is too small.  %d targets available, "
			"only %d configurable.\n",
			all_targets.c/2, max_targets);
		(void) fprintf(stderr,
			"To increase this value, libiscsi will have be "
			"recompiled.\n");
		(void) fprintf(stderr,
			"Truncating number of targets to %d.\n",
			max_targets);
		all_targets.c = 2 * max_targets;
	}

	sti.st.st_ino = 0x15c51;

#if defined(__NetBSD__) && defined(USE_LIBKMOD)
	/* check that the puffs module is loaded on NetBSD */
	if (kmodstat("puffs", NULL) == 0 && !kmodload("puffs")) {
		(void) fprintf(stderr, "initiator: can't load puffs module\n");
	}
#endif

	for (u = 0 ; u < all_targets.c / 2 ; u++) {
		ALLOC(targetinfo_t, tv.v, tv.size, tv.c, 10, 10, "iscsifs",
				exit(EXIT_FAILURE));

		initiator_set_target_name(u, all_targets.v[u * 2]);

		if (iscsi_initiator_discover(host, u, 0) < 0) {
			printf("iscsi_initiator_discover() failed\n");
			break;
		}

		get_target_info(u, &tinfo);
		if ((colon = strrchr(tinfo.TargetName, ':')) == NULL) {
			colon = tinfo.TargetName;
		} else {
			colon += 1;
		}

		/* stuff size into st.st_size */
		{
			int retry = 5;
			while (retry > 0) {
				if (read_capacity(u, 0, &lbac, &blocksize) == 0)
					break;
				retry--;
				iscsi_warn(__FILE__, __LINE__,
				    "read_capacity failed - retrying %d\n", retry);
				sleep(1);
			}
			if (retry == 0) {
				iscsi_err(__FILE__, __LINE__, "read_capacity failed - giving up\n");
				break;
			}
		}
		sti.st.st_size = (off_t)(((uint64_t)lbac + 1) * blocksize);
		sti.target = u;

		tv.v[tv.c].host = strdup(tinfo.name);
		tv.v[tv.c].ip = strdup(tinfo.ip);
		tv.v[tv.c].targetname = strdup(tinfo.TargetName);
		tv.v[tv.c].stargetname = strdup(colon);
		tv.v[tv.c].target = u;
		tv.v[tv.c].lun = 0;
		tv.v[tv.c].lbac = lbac;
		tv.v[tv.c].blocksize = blocksize;

		/* get iSCSI target information */
		(void) memset(data, 0x0, sizeof(data));
		inquiry(u, 0, 0, 0, data);
		tv.v[tv.c].devicetype = (data[0] & 0x1f);
		(void) memcpy(tv.v[tv.c].vendor, &data[8], VendorLen);
		(void) memcpy(tv.v[tv.c].product, &data[8 + VendorLen],
				ProductLen);
		(void) memcpy(tv.v[tv.c].version,
				&data[8 + VendorLen + ProductLen], VersionLen);
		(void) memset(data, 0x0, sizeof(data));
		inquiry(u, 0, INQUIRY_EVPD_BIT, INQUIRY_UNIT_SERIAL_NUMBER_VPD,
				data);
		tv.v[tv.c].serial = strdup((char *)&data[4]);

		/* create the tree using virtdir routines */
		cc = snprintf(name, sizeof(name), "/%s", colon);
		virtdir_add(&iscsi, name, cc, 'd', name, cc);
		cc = snprintf(name, sizeof(name), "/%s/storage", colon);
		virtdir_add(&iscsi, name, cc, devtype, (void *)&sti,
				sizeof(sti));
		cc = snprintf(name, sizeof(name), "/%s/hostname", colon);
		virtdir_add(&iscsi, name, cc, 'l', tinfo.name,
				strlen(tinfo.name));
		cc = snprintf(name, sizeof(name), "/%s/ip", colon);
		virtdir_add(&iscsi, name, cc, 'l', tinfo.ip, strlen(tinfo.ip));
		cc = snprintf(name, sizeof(name), "/%s/targetname", colon);
		virtdir_add(&iscsi, name, cc, 'l', tinfo.TargetName,
				strlen(tinfo.TargetName));
		cc = snprintf(name, sizeof(name), "/%s/vendor", colon);
		virtdir_add(&iscsi, name, cc, 'l', tv.v[tv.c].vendor,
				strlen(tv.v[tv.c].vendor));
		cc = snprintf(name, sizeof(name), "/%s/product", colon);
		virtdir_add(&iscsi, name, cc, 'l', tv.v[tv.c].product,
				strlen(tv.v[tv.c].product));
		cc = snprintf(name, sizeof(name), "/%s/version", colon);
		virtdir_add(&iscsi, name, cc, 'l', tv.v[tv.c].version,
				strlen(tv.v[tv.c].version));
		if (tv.v[tv.c].serial[0] && tv.v[tv.c].serial[0] != ' ') {
			cc = snprintf(name, sizeof(name), "/%s/serial",
				colon);
			virtdir_add(&iscsi, name, cc, 'l', tv.v[tv.c].serial,
				strlen(tv.v[tv.c].serial));
		}
		tv.c += 1;
	}
	return fuse_main(argc - optind, argv + optind, &iscsiops, NULL);
}
