/*
 * Copyright © 2007 Alistair Crooks.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#define EXTERN

#include "scsi_cmd_codes.h"
#include "iscsi.h"
#include "initiator.h"
#include "tests.h"

#include "virtdir.h"

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
		iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
		return -1;
	}
	if (args.status) {
		iscsi_trace_error(__FILE__, __LINE__, "READ_CAPACITY failed (status %#x)\n", args.status);
		return -1;
	}
	*maxlba = ISCSI_NTOHL(*((uint32_t *) (data)));
	*blocklen = ISCSI_NTOHL(*((uint32_t *) (data + 4)));
	if (*maxlba == 0) {
		iscsi_trace_error(__FILE__, __LINE__, "Device returned Maximum LBA of zero\n");
		return -1;
	}
	if (*blocklen % 2) {
		iscsi_trace_error(__FILE__, __LINE__, "Device returned strange block len: %u\n", *blocklen);
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
		iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
		return -1;
	}
	if (args.status) {
		iscsi_trace_error(__FILE__, __LINE__, "INQUIRY failed (status %#x)\n", args.status);
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
		iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
		return -1;
	}

	if (args.status) {
		iscsi_trace_error(__FILE__, __LINE__, "scsi_command() failed (status %#x)\n", args.status);
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
		iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
		return -1;
	}
	if (args.status) {
		iscsi_trace_error(__FILE__, __LINE__, "scsi_command() failed (status %#x)\n", args.status);
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
			iscsi_trace_error(__FILE__, __LINE__, "out of memory\n");
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
			iscsi_trace_error(__FILE__, __LINE__, "read_10() failed\n");
			return -1;
		}
		iscsi_free(iov);
	} else {
		req_len = length / tv.v[t].blocksize;
		if ((req_len * tv.v[t].blocksize) < length)
			req_len++;
		if (blockop(tv.v[t].target, tv.v[t].lun, offset / tv.v[t].blocksize, 
				req_len, tv.v[t].blocksize, (uint8_t *) buf, writing) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "read_10() failed\n");
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
	char		       *user;
	int			address_family;
	char			devtype;
	int			port;
	int             	target = -1;
        int			digest_type;
	int			discover;
        int			mutual_auth;
        int			auth_type;
	int			cc;
	int			i;

	(void) memset(&tinfo, 0x0, sizeof(tinfo));
	user = NULL;
	(void) gethostname(host = hostname, sizeof(hostname));
	digest_type = DigestNone;
	auth_type = AuthNone;
	address_family = ISCSI_UNSPEC;
	port = ISCSI_PORT;
	mutual_auth = 0;
	discover = 0;
	(void) stat("/etc/hosts", &sti.st);
	devtype = 'f';
	while ((i = getopt(argc, argv, "46a:bcd:Dfh:p:t:u:vV")) != -1) {
		switch(i) {
		case '4':
			address_family = ISCSI_IPv4;
			break;
		case '6':
			address_family = ISCSI_IPv6;
			break;
		case 'a':
			if (strcasecmp(optarg, "chap") == 0) {
				auth_type = AuthCHAP;
			} else if (strcasecmp(optarg, "kerberos") == 0) {
				auth_type = AuthKerberos;
			} else if (strcasecmp(optarg, "srp") == 0) {
				auth_type = AuthSRP;
			}
			break;
		case 'b':
			devtype = 'b';
			break;
		case 'c':
			devtype = 'c';
			break;
		case 'd':
			if (strcasecmp(optarg, "header") == 0) {
				digest_type = DigestHeader;
			} else if (strcasecmp(optarg, "data") == 0) {
				digest_type = DigestData;
			} else if (strcasecmp(optarg, "both") == 0 || strcasecmp(optarg, "all") == 0) {
				digest_type = (DigestHeader | DigestData);
			}
			break;
		case 'D':
			discover = 1;
			break;
		case 'f':
			devtype = 'f';
			break;
		case 'h':
			host = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 't':
			target = atoi(optarg);
			break;
		case 'u':
			user = optarg;
			break;
		case 'V':
			(void) printf("\"%s\" %s\nPlease send all bug reports to %s\n", PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_BUGREPORT);
			exit(EXIT_SUCCESS);
			/* NOTREACHED */
		case 'v':
			verbose += 1;
			break;
		default:
			(void) fprintf(stderr, "%s: unknown option `%c'", *argv, i);
		}
	}
	if (user == NULL) {
		iscsi_trace_error(__FILE__, __LINE__, "user must be specified with -u");
		exit(EXIT_FAILURE);
	}

	if (initiator_init(host, port, address_family, user, auth_type, mutual_auth, digest_type) == -1) {
		iscsi_trace_error(__FILE__, __LINE__, "initiator_init() failed\n");
		exit(EXIT_FAILURE);
	}


        if (initiator_discover(host, 0, 0) < 0) {
                printf("initiator_discover() in discover failed\n");
		exit(EXIT_FAILURE);
	}

        if (initiator_get_targets(0,&all_targets) == -1) {
 		iscsi_trace_error(__FILE__, __LINE__, "initiator_get_targets() failed\n");
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

	if (all_targets.c/2 > CONFIG_INITIATOR_NUM_TARGETS) {
		(void) fprintf(stderr, "CONFIG_INITIATOR_NUM_TARGETS in initiator.h is too small.  %d targets available, only %d configurable.\n", all_targets.c/2, CONFIG_INITIATOR_NUM_TARGETS);
		(void) fprintf(stderr, "Truncating number of targets to %d.\n", CONFIG_INITIATOR_NUM_TARGETS);
		all_targets.c = CONFIG_INITIATOR_NUM_TARGETS;
	}


	sti.st.st_ino = 0x15c51;

	for (u = 0 ; u < all_targets.c / 2 ; u++) {
		ALLOC(targetinfo_t, tv.v, tv.size, tv.c, 10, 10, "iscsifs",
				exit(EXIT_FAILURE));

		initiator_set_target_name(u, all_targets.v[u * 2]);

		if (initiator_discover(host, u, 0) < 0) {
			printf("initiator_discover() failed\n");
			break;
		}

		get_target_info(u, &tinfo);
		if ((colon = strrchr(tinfo.TargetName, ':')) == NULL) {
			colon = tinfo.TargetName;
		} else {
			colon += 1;
		}

		/* stuff size into st.st_size */
		(void) read_capacity(u, 0, &lbac, &blocksize);
		sti.st.st_size = ((uint64_t)lbac + 1) * blocksize;
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
		(void) memcpy(tv.v[tv.c].product, &data[8 + VendorLen], ProductLen);
		(void) memcpy(tv.v[tv.c].version, &data[8 + VendorLen + ProductLen], VersionLen);
		(void) memset(data, 0x0, sizeof(data));
		inquiry(u, 0, INQUIRY_EVPD_BIT, INQUIRY_UNIT_SERIAL_NUMBER_VPD, data);
		tv.v[tv.c].serial = strdup((char *)&data[4]);

		cc = snprintf(name, sizeof(name), "/%s/%s", host, colon);
		virtdir_add(&iscsi, name, cc, 'd', name, cc);
		cc = snprintf(name, sizeof(name), "/%s/%s/storage", host, colon);
		virtdir_add(&iscsi, name, cc, devtype, (void *)&sti, sizeof(sti));
		cc = snprintf(name, sizeof(name), "/%s/%s/hostname", host, colon);
		virtdir_add(&iscsi, name, cc, 'l', tinfo.name, strlen(tinfo.name));
		cc = snprintf(name, sizeof(name), "/%s/%s/ip", host, colon);
		virtdir_add(&iscsi, name, cc, 'l', tinfo.ip, strlen(tinfo.ip));
		cc = snprintf(name, sizeof(name), "/%s/%s/targetname", host, colon);
		virtdir_add(&iscsi, name, cc, 'l', tinfo.TargetName, strlen(tinfo.TargetName));
		cc = snprintf(name, sizeof(name), "/%s/%s/vendor", host, colon);
		virtdir_add(&iscsi, name, cc, 'l', tv.v[tv.c].vendor, strlen(tv.v[tv.c].vendor));
		cc = snprintf(name, sizeof(name), "/%s/%s/product", host, colon);
		virtdir_add(&iscsi, name, cc, 'l', tv.v[tv.c].product, strlen(tv.v[tv.c].product));
		cc = snprintf(name, sizeof(name), "/%s/%s/version", host, colon);
		virtdir_add(&iscsi, name, cc, 'l', tv.v[tv.c].version, strlen(tv.v[tv.c].version));
		if (tv.v[tv.c].serial[0] && tv.v[tv.c].serial[0] != ' ') {
			cc = snprintf(name, sizeof(name), "/%s/%s/serial", host, colon);
			virtdir_add(&iscsi, name, cc, 'l', tv.v[tv.c].serial, strlen(tv.v[tv.c].serial));
		}


		tv.c += 1;
	}
	return fuse_main(argc - optind, argv + optind, &iscsiops, NULL);
}
