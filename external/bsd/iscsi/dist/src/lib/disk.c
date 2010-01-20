/* $NetBSD: disk.c,v 1.3 2010/01/20 00:50:09 yamt Exp $ */

/*-
 * Copyright (c) 2006, 2007, 2008, 2009 The NetBSD Foundation, Inc.
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
/*
 * IMPORTANT:  READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. 
 * By downloading, copying, installing or using the software you agree
 * to this license.  If you do not agree to this license, do not
 * download, install, copy or use the software.
 *
 * Intel License Agreement
 *
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * -Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 * -Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the
 *  distribution.
 *
 * -The name of Intel Corporation may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL INTEL
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include <sys/types.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <unistd.h>

#include "scsi_cmd_codes.h"

#include "iscsiprotocol.h"
#include "compat.h"
#include "iscsiutil.h"
#include "device.h"
#include "target.h"
#include "defs.h"
#include "storage.h"

#define iSCSI_DEFAULT_LUNS	1
#define iSCSI_DEFAULT_BLOCKLEN	512

/* End disk configuration */

/*
 * Globals
 */
enum {
	MAX_RESERVATIONS = 32,

	ISCSI_FS =	0x03,
	ISCSI_CONTROL = 0x04
};

#define MB(x)	((x) * 1024 * 1024)

/* this struct describes an iscsi LUN */
typedef struct iscsi_disk_t {
	int		 type;		/* type of disk - fs/mmap and fs */
	char		 filename[MAXPATHLEN];	/* filename for the disk */
	uint8_t		*buffer;	/* buffer for disk read/write ops */
	uint64_t	 blockc;	/* # of blocks */
	uint64_t	 blocklen;	/* block size */
	uint64_t	 luns;		/* # of luns */
	uint64_t	 size;		/* size of complete disk */
	nbuuid_t	 uuid;		/* disk's uuid */
	char		*uuid_string;	/* uuid string */
	targv_t		*lunv;		/* the component devices and extents */
	uint32_t	 resc;		/* # of reservation keys */
	uint64_t	 reskeys[MAX_RESERVATIONS];	/* reservation keys */
} iscsi_disk_t;

DEFINE_ARRAY(disks_t, iscsi_disk_t);

static disks_t		disks;
static iscsi_disk_t	defaults;

#ifndef FDATASYNC
/*
this means that we probably don't have the fsync_range(2) system call,
but no matter - define this here to preserve the abstraction for the
disk/extent code
*/
#define FDATASYNC	0x0010
#endif

/*
 * Private Interface
 */
static int      disk_read(target_session_t *, iscsi_scsi_cmd_args_t *,
				uint32_t, uint16_t, uint8_t);
static int      disk_write(target_session_t *, iscsi_scsi_cmd_args_t *,
				uint8_t, uint32_t, uint32_t);

/* return the de index and offset within the device for RAID0 */
static int
raid0_getoff(disc_device_t *dp, uint64_t off, uint32_t *d, uint64_t *de_off)
{
	uint64_t	o;

	for (o = 0, *d = 0 ; *d < dp->c ; o += dp->xv[*d].size, (*d)++) {
		if (off >= o && off < o + dp->xv[*d].size) {
			break;
		}
	}
	*de_off = off - o;
	return (*d < dp->c);
}

/* open the extent's device */
static int
extent_open(disc_extent_t *xp, int mode, int flags)
{
	return xp->fd = open(xp->dev, mode, flags);
}

/* (recursively) open the device's devices */
static int
device_open(disc_device_t *dp, int flags, int mode)
{
	int	fd;
	uint32_t i;

	for (fd = -1, i = 0 ; i < dp->c ; i++) {
		switch (dp->xv[i].type) {
		case DE_DEVICE:
			fd = device_open(dp->xv[i].u.dp, flags, mode);
			if (fd < 0) {
				return -1;
			}
			break;
		case DE_EXTENT:
			fd = extent_open(dp->xv[i].u.xp, flags, mode);
			if (fd < 0) {
				return -1;
			}
			break;
		default:
			break;
		}
	}
	return fd;
}

/* and for the undecided... */
static int
de_open(disc_de_t *dp, int flags, int mode)
{
	switch(dp->type) {
	case DE_DEVICE:
		return device_open(dp->u.dp, flags, mode);
	case DE_EXTENT:
		return extent_open(dp->u.xp, flags, mode);
	default:
		return -1;
	}
}

/* lseek on the extent */
static off_t
extent_lseek(disc_extent_t *xp, off_t off, int whence)
{
	return lseek(xp->fd, (long long)(xp->sacred + off), whence);
}

/* (recursively) lseek on the device's devices */
static off_t
device_lseek(disc_device_t *dp, off_t off, int whence)
{
	uint64_t	suboff;
	off_t		ret;
	uint32_t	d;

	ret = -1;
	switch(dp->raid) {
	case 0:
		if (raid0_getoff(dp, (uint64_t) off, &d, &suboff)) {
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				ret = device_lseek(dp->xv[d].u.dp,
					(off_t) suboff, whence);
				if (ret < 0) {
					return -1;
				}
				break;
			case DE_EXTENT:
				ret = extent_lseek(dp->xv[d].u.xp,
						(off_t) suboff, whence);
				if (ret < 0) {
					return -1;
				}
				break;
			default:
				break;
			}
		}
		break;
	case 1:
		for (d = 0 ; d < dp->c ; d++) {
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				ret = device_lseek(dp->xv[d].u.dp, (off_t)off,
							whence);
				if (ret < 0) {
					return -1;
				}
				break;
			case DE_EXTENT:
				ret = extent_lseek(dp->xv[d].u.xp, (off_t)off,
							whence);
				if (ret < 0) {
					return -1;
				}
				break;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}
	return dp->off = ret;
}

/* and for the undecided... */
static off_t
de_lseek(disc_de_t *dp, off_t off, int whence)
{
	switch(dp->type) {
	case DE_DEVICE:
		return device_lseek(dp->u.dp, off, whence);
	case DE_EXTENT:
		return extent_lseek(dp->u.xp, off, whence);
	default:
		return -1;
	}
}

/* fsync_range on the extent */
static int
extent_fsync_range(disc_extent_t *xp, int how, off_t from, off_t len)
{
#ifdef HAVE_FSYNC_RANGE
	return fsync_range(xp->fd, how, (off_t)(xp->sacred + from), len);
#else
	return fsync(xp->fd);
#endif
}

/* (recursively) fsync_range on the device's devices */
static int
device_fsync_range(disc_device_t *dp, int how, off_t from, off_t len)
{
	uint64_t	suboff;
	int		ret;
	uint32_t	d;

	ret = -1;
	switch(dp->raid) {
	case 0:
		if (raid0_getoff(dp, (uint64_t) from, &d, &suboff)) {
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				ret = device_fsync_range(dp->xv[d].u.dp, how,
						(off_t)suboff, len);
				if (ret < 0) {
					return -1;
				}
				break;
			case DE_EXTENT:
				ret = extent_fsync_range(dp->xv[d].u.xp, how,
						(off_t)suboff, len);
				if (ret < 0) {
					return -1;
				}
				break;
			default:
				break;
			}
		}
		break;
	case 1:
		for (d = 0 ; d < dp->c ; d++) {
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				ret = device_fsync_range(dp->xv[d].u.dp, how,
						from, len);
				if (ret < 0) {
					return -1;
				}
				break;
			case DE_EXTENT:
				ret = extent_fsync_range(dp->xv[d].u.xp, how,
						from, len);
				if (ret < 0) {
					return -1;
				}
				break;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}
	dp->off = (uint64_t) ret;
	return ret;
}

/* and for the undecided... */
static int
de_fsync_range(disc_de_t *dp, int how, off_t from, off_t len)
{
	switch(dp->type) {
	case DE_DEVICE:
		return device_fsync_range(dp->u.dp, how, from, len);
	case DE_EXTENT:
		return extent_fsync_range(dp->u.xp, how, from, len);
	default:
		return -1;
	}
}

/* read from the extent */
static ssize_t
extent_read(disc_extent_t *xp, void *buf, size_t cc)
{
	return read(xp->fd, buf, cc);
}

/* (recursively) read from the device's devices */
static ssize_t
device_read(disc_device_t *dp, void *buf, size_t cc)
{
	uint64_t	 suboff;
	uint64_t	 got;
	uint32_t	 d;
	ssize_t		 ret;
	size_t		 subcc;
	char		*cbuf;

	ret = -1;
	switch(dp->raid) {
	case 0:
		for (cbuf = (char *) buf, got = 0 ; got < cc ; got += ret) {
			if (!raid0_getoff(dp, dp->off, &d, &suboff)) {
				return -1;
			}
			if (device_lseek(dp, (off_t)dp->off, SEEK_SET) < 0) {
				return -1;
			}
			subcc = MIN(cc - (size_t)got,
					(size_t)(dp->len - (size_t)dp->off));
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				ret = device_read(dp->xv[d].u.dp,
						&cbuf[(int)got], subcc);
				if (ret < 0) {
					return -1;
				}
				break;
			case DE_EXTENT:
				ret = extent_read(dp->xv[d].u.xp,
						&cbuf[(int)got], subcc);
				if (ret < 0) {
					return -1;
				}
				break;
			default:
				break;
			}
			dp->off += ret;
		}
		ret = (ssize_t)got;
		break;
	case 1:
		for (d = 0 ; d < dp->c ; d++) {
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				ret = device_read(dp->xv[d].u.dp, buf, cc);
				if (ret < 0) {
					return -1;
				}
				break;
			case DE_EXTENT:
				ret = extent_read(dp->xv[d].u.xp, buf, cc);
				if (ret < 0) {
					return -1;
				}
				break;
			default:
				break;
			}
		}
		dp->off += ret;
		break;
	default:
		break;
	}
	return ret;
}

/* and for the undecided... */
static ssize_t
de_read(disc_de_t *dp, void *buf, size_t cc)
{
	switch(dp->type) {
	case DE_DEVICE:
		return device_read(dp->u.dp, buf, cc);
	case DE_EXTENT:
		return extent_read(dp->u.xp, buf, cc);
	default:
		return -1;
	}
}

/* write to the extent */
static ssize_t
extent_write(disc_extent_t *xp, void *buf, size_t cc)
{
	return write(xp->fd, buf, cc);
}

/* (recursively) write to the device's devices */
static ssize_t
device_write(disc_device_t *dp, void *buf, size_t cc)
{
	uint64_t	 suboff;
	uint64_t	 done;
	uint32_t	 d;
	ssize_t		 ret;
	size_t		 subcc;
	char		*cbuf;

	ret = -1;
	switch(dp->raid) {
	case 0:
		for (cbuf = (char *) buf, done = 0 ; done < cc ; done += ret) {
			if (!raid0_getoff(dp, dp->off, &d, &suboff)) {
				return -1;
			}
			subcc = (size_t)MIN(cc - (size_t)done,
					(size_t)(dp->len - dp->off));
			if (device_lseek(dp, (off_t)dp->off, SEEK_SET) < 0) {
				return -1;
			}
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				ret = device_write(dp->xv[d].u.dp,
					&cbuf[(int)done], subcc);
				if (ret < 0) {
					return -1;
				}
				break;
			case DE_EXTENT:
				ret = extent_write(dp->xv[d].u.xp,
						&cbuf[(int)done], subcc);
				if (ret < 0) {
					return -1;
				}
				break;
			default:
				break;
			}
			dp->off += ret;
		}
		ret = (ssize_t) done;
		break;
	case 1:
		for (d = 0 ; d < dp->c ; d++) {
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				ret = device_write(dp->xv[d].u.dp, buf, cc);
				if (ret < 0) {
					iscsi_err(__FILE__, __LINE__,
						"device_write RAID1 device "
						"write failure\n");
					return -1;
				}
				break;
			case DE_EXTENT:
				ret = extent_write(dp->xv[d].u.xp, buf, cc);
				if (ret < 0) {
					iscsi_err(__FILE__, __LINE__,
						"device_write RAID1 extent "
						"write failure\n");
					return -1;
				}
				break;
			default:
				break;
			}
		}
		dp->off += ret;
		break;
	default:
		break;
	}
	return ret;
}

/* and for the undecided... */
static ssize_t
de_write(disc_de_t *dp, void *buf, size_t cc)
{
	switch(dp->type) {
	case DE_DEVICE:
		return device_write(dp->u.dp, buf, cc);
	case DE_EXTENT:
		return extent_write(dp->u.xp, buf, cc);
	default:
		return -1;
	}
}

/* return non-zero if the target is writable */
static int
target_writable(disc_target_t *tp)
{
	return !(tp->flags & TARGET_READONLY);
}

/* return size of the extent */
static uint64_t
extent_getsize(disc_extent_t *xp)
{
	return xp->len;
}

/* (recursively) return the size of the device's devices */
static uint64_t
device_getsize(disc_device_t *dp)
{
	uint64_t	size;
	uint32_t	d;

	size = 0;
	switch(dp->raid) {
	case 0:
		for (d = 0 ; d < dp->c ; d++) {
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				size += device_getsize(dp->xv[d].u.dp);
				break;
			case DE_EXTENT:
				size += extent_getsize(dp->xv[d].u.xp);
				break;
			default:
				break;
			}
		}
		break;
	case 1:
		size = dp->len;
		break;
	default:
		break;
	}
	return size;
}

/* and for the undecided... */
static int64_t
de_getsize(disc_de_t *dp)
{
	switch(dp->type) {
	case DE_DEVICE:
		return device_getsize(dp->u.dp);
	case DE_EXTENT:
		return extent_getsize(dp->u.xp);
	default:
		return -1;
	}
}

/* return a filename for the device or extent */
static char *
disc_get_filename(disc_de_t *de)
{
	switch (de->type) {
	case DE_EXTENT:
		return de->u.xp->dev;
	case DE_DEVICE:
		return disc_get_filename(&de->u.dp->xv[0]);
	default:
		return NULL;
	}
}

/*
 * Public Interface (called by utarget and ktarket)
 */

 /* set various global variables */
void
device_set_var(const char *var, const char *arg)
{
	if (strcmp(var, "blocklen") == 0) {
		defaults.blocklen = strtoll(arg, (char **)NULL, 10);
	} else if (strcmp(var, "blocks") == 0) {
		defaults.blockc = strtoll(arg, (char **)NULL, 10);
	} else if (strcmp(var, "luns") == 0) {
		defaults.luns = strtoll(arg, (char **)NULL, 10);
	} else {
		(void) fprintf(stderr, "Unrecognised variable: `%s'\n", var);
	}
}

/* allocate some space for a disk/extent, using an lseek, read and
* write combination */
static int
de_allocate(disc_de_t *de, char *filename)
{
	off_t	size;
	char	block[DEFAULT_TARGET_BLOCK_LEN];

	size = de_getsize(de);
	if (de_lseek(de, size - sizeof(block), SEEK_SET) == -1) {
		iscsi_err(__FILE__, __LINE__,
				"error seeking \"%s\"\n", filename);
		return 0;
	}
	if (de_read(de, block, sizeof(block)) == -1) {
		iscsi_err(__FILE__, __LINE__,
				"error reading \"%s\"", filename);
		return 0;
	}
	if (de_write(de, block, sizeof(block)) == -1) {
		iscsi_err(__FILE__, __LINE__,
				"error writing \"%s\"", filename);
		return 0;
	}
	return 1;
}

/* allocate space as desired */
static int
allocate_space(disc_target_t *tp)
{
	uint32_t	i;

	/* Don't perform check for writability in the target here, as the
	following write() in de_allocate is non-destructive */
	switch(tp->de.type) {
	case DE_EXTENT:
		return de_allocate(&tp->de, tp->target);
	case DE_DEVICE:
		for (i = 0 ; i < tp->de.u.dp->c ; i++) {
			if (!de_allocate(&tp->de.u.dp->xv[i], tp->target)) {
				return 0;
			}
		}
		return 1;
	default:
		break;
	}
	return 0;
}

/* copy src to dst, of size `n' bytes, padding any extra with `pad' */
static void
strpadcpy(uint8_t *dst, size_t dstlen, const char *src, const size_t srclen,
		char pad)
{
	if (srclen < dstlen) {
		(void) memcpy(dst, src, srclen);
		(void) memset(&dst[srclen], pad, dstlen - srclen);
	} else {
		(void) memcpy(dst, src, dstlen);
	}
}	

/* handle REPORT LUNs SCSI command */
static int
report_luns(uint64_t *data, int64_t luns)
{
	uint64_t	i;
	int32_t		off;

	for (i = 0, off = 8 ; i < (uint64_t)luns ; i++, off += sizeof(i)) {
		data[(int)i] = ISCSI_HTONLL(i);
	}
	return off;
}

/* handle persistent reserve in command */
static int
persistent_reserve_in(uint8_t action, uint8_t *data)
{
	uint64_t	key;

	switch(action) {
	case PERSISTENT_RESERVE_IN_READ_KEYS:
		key = 0;	/* simulate "just powered on" */
		*((uint32_t *)(void *)data) =
				(uint32_t)ISCSI_HTONL((uint32_t) 0);
		*((uint32_t *) (void *)data + 4) =
				(uint32_t) ISCSI_HTONL((uint32_t) sizeof(key));
				/* length in bytes of list of keys */
		*((uint64_t *) (void *)data + 8) = (uint64_t) ISCSI_HTONLL(key);
		return 8 + sizeof(key);
	case PERSISTENT_RESERVE_IN_REPORT_CAPABILITIES:
		(void) memset(data, 0x0, 8);
		/* length is fixed at 8 bytes */
		*((uint16_t *)(void *)data) =
				(uint16_t)ISCSI_HTONS((uint16_t)8);
		data[2] = PERSISTENT_RESERVE_IN_CRH;
				/* also SIP_C, ATP_C and PTPL_C here */
		data[3] = 0;	/* also TMV and PTPL_A here */
		data[4] = 0;
			/* also WR_EX_AR, EX_AC_RD, WR_EX_RD, EX_AC, WR_EX */
		data[5] = 0;	/* also EX_AC_AR here */
		return 8;
	default:
		iscsi_err(__FILE__, __LINE__,
			"persistent_reserve_in: action %x unrecognised\n",
			action);
		return 0;
	}
}

/* initialise the device */
int 
device_init(iscsi_target_t *tgt, targv_t *tvp, disc_target_t *tp)
{
	iscsi_disk_t	*idisk;
	int	 	 mode;

	ALLOC(iscsi_disk_t, disks.v, disks.size, disks.c, 10, 10,
			"device_init", ;);
	idisk = &disks.v[disks.c];
	idisk->lunv = tvp;
	if ((idisk->luns = defaults.luns) == 0) {
		idisk->luns = iSCSI_DEFAULT_LUNS;
	}
	idisk->blocklen = atoi(iscsi_target_getvar(tgt, "blocklen"));
	switch(idisk->blocklen) {
	case 512:
	case 1024:
	case 2048:
	case 4096:
	case 8192:
		break;
	default:
		iscsi_err(__FILE__, __LINE__,
			"Invalid block len %" PRIu64
			". Choose one of 512, 1024, 2048, 4096, or 8192.\n",
			idisk->blocklen);
		return -1;
	}
	idisk->size = de_getsize(&tp->de);
	idisk->blockc = idisk->size / idisk->blocklen;
	NEWARRAY(uint8_t, idisk->buffer, MB(1), "buffer1", ;);
	idisk->type = ISCSI_FS;
	printf("DISK: %" PRIu64 " logical unit%s (%" PRIu64 " blocks, %"
			PRIu64 " bytes/block), type %s\n",
			idisk->luns,
			(idisk->luns == 1) ? "" : "s",
			idisk->blockc, idisk->blocklen,
			(idisk->type == ISCSI_FS) ? "iscsi fs" :
				"iscsi fs mmap");
	printf("DISK: LUN 0: ");
	(void) strlcpy(idisk->filename, disc_get_filename(&tp->de),
			sizeof(idisk->filename));
	mode = (tp->flags & TARGET_READONLY) ? O_RDONLY : (O_CREAT | O_RDWR);
	if (de_open(&tp->de, mode, 0666) == -1) {
		iscsi_err(__FILE__, __LINE__,
			"error opening \"%s\"\n", idisk->filename);
		return -1;
	}
	if (!(tp->flags & TARGET_READONLY) && !allocate_space(tp)) {
		iscsi_err(__FILE__, __LINE__,
			"error allocating space for \"%s\"", tp->target);
		return -1;
	}
	printf("%" PRIu64 " MB %sdisk storage for \"%s\"\n",
		(de_getsize(&tp->de) / MB(1)),
		(tp->flags & TARGET_READONLY) ? "readonly " : "",
		tp->target);
	return disks.c++;
}

static void
cdb2lba(uint32_t *lba, uint16_t *len, uint8_t *cdb)
{
	/* Some platforms (like strongarm) aligns on */
	/* word boundaries.  So HTONL and NTOHL won't */
	/* work here. */
	int	little_endian = 1;

	if (*(char *) (void *) &little_endian) {
		/* little endian */
		((uint8_t *) (void *) lba)[0] = cdb[5];
		((uint8_t *) (void *) lba)[1] = cdb[4];
		((uint8_t *) (void *) lba)[2] = cdb[3];
		((uint8_t *) (void *) lba)[3] = cdb[2];
		((uint8_t *) (void *) len)[0] = cdb[8];
		((uint8_t *) (void *) len)[1] = cdb[7];
	} else {
		((uint8_t *) (void *) lba)[0] = cdb[2];
		((uint8_t *) (void *) lba)[1] = cdb[3];
		((uint8_t *) (void *) lba)[2] = cdb[4];
		((uint8_t *) (void *) lba)[3] = cdb[5];
		((uint8_t *) (void *) len)[0] = cdb[7];
		((uint8_t *) (void *) len)[1] = cdb[8];
	}
}

/* handle MODE_SENSE_6 and MODE_SENSE_10 commands */
static int
mode_sense(const int bytes, target_cmd_t *cmd)
{
	iscsi_scsi_cmd_args_t	*args = cmd->scsi_cmd;
	uint16_t		 len;
	uint8_t			*cp;
	uint8_t			*cdb = args->cdb;
	size_t			 mode_data_len;

	switch(bytes) {
	case 6:
		cp = args->send_data;
		len = ISCSI_MODE_SENSE_LEN;
		mode_data_len = len + 3;

		iscsi_trace(TRACE_SCSI_CMD, "MODE_SENSE_6\n");
		(void) memset(cp, 0x0, mode_data_len);

		cp[0] = mode_data_len;
		cp[1] = 0;
		cp[2] = 0;
		cp[3] = 8;	/* block descriptor length */
		cp[10] = 2;	/* density code and block length */

		args->input = 1;
		args->length = (unsigned)len;
		args->status = SCSI_SUCCESS;
		return 1;
	case 10:
		cp = args->send_data;
		len = ISCSI_MODE_SENSE_LEN;
		mode_data_len = len + 3;

		iscsi_trace(TRACE_SCSI_CMD, "MODE_SENSE_10\n");
		(void) memset(cp, 0x0, mode_data_len);
		if (cdb[4] == 0) {
			/* zero length cdb means just return success */
			args->input = 1;
			args->length = (unsigned)(mode_data_len);
			args->status = SCSI_SUCCESS;
			return 1;
		}
		if ((cdb[2] & PAGE_CONTROL_MASK) ==
					PAGE_CONTROL_CHANGEABLE_VALUES) {
			/* just send back a CHECK CONDITION */
			args->input = 1;
			args->length = (unsigned)(len);
			args->status = SCSI_CHECK_CONDITION;
			cp[2] = SCSI_SKEY_ILLEGAL_REQUEST;
			cp[12] = ASC_LUN_UNSUPPORTED;
			cp[13] = ASCQ_LUN_UNSUPPORTED;
			return 1;
		}
		iscsi_trace(TRACE_SCSI_CMD, "PC %02x\n", cdb[2]);

		cp[0] = mode_data_len;
		cp[1] = 0;
		cp[2] = 0;
		cp[3] = 8;	/* block descriptor length */
		cp[10] = 2;	/* density code and block length */

		args->input = 1;
		args->length = (unsigned)(len);
		args->status = SCSI_SUCCESS;
		return 1;
	}
	return 0;
}

/* fill in the device serial number vital product data */
static uint8_t
serial_vpd(uint8_t *data)
{
	uint8_t	len;

	data[0] = DISK_PERIPHERAL_DEVICE;
	data[1] = INQUIRY_DEVICE_IDENTIFICATION_VPD;
	len = 16;
	/* add target device's Unit Serial Number */
	/* section 7.6.10 of SPC-3 says that if there is no serial number,
	 * use spaces */
	strpadcpy(&data[4], (size_t)len, " ", strlen(" "), ' ');
	return len;
}

/* fill in the device identification vital product data */
static void
device_vpd(iscsi_target_t *tgt, uint8_t *data, uint8_t *rspc,
		uint8_t *cdbsize, uint8_t lun, char *uuid)
{
	uint16_t	 len;
	uint8_t		*cp;

	data[0] = DISK_PERIPHERAL_DEVICE;
	data[1] = INQUIRY_DEVICE_IDENTIFICATION_VPD;
	*rspc = 0;
	cp = &data[4];
	/* add target device's IQN */
	cp[0] = (INQUIRY_DEVICE_ISCSI_PROTOCOL << 4) |
			INQUIRY_DEVICE_CODESET_UTF8;
	cp[1] = (INQUIRY_DEVICE_PIV << 7) |
			(INQUIRY_DEVICE_ASSOCIATION_TARGET_DEVICE << 4) |
			INQUIRY_DEVICE_IDENTIFIER_SCSI_NAME;
	len = (uint8_t) snprintf((char *)&cp[4],
			(unsigned)(*cdbsize - (int)(cp - &data[4])), "%s",
			iscsi_target_getvar(tgt, "iqn"));
	cp[3] = len;
	*rspc += len + 4;
	cp += len + 4;
	/* add target port's IQN + LUN */
	cp[0] = (INQUIRY_DEVICE_ISCSI_PROTOCOL << 4) |
		INQUIRY_DEVICE_CODESET_UTF8;
	cp[1] = (INQUIRY_DEVICE_PIV << 7) |
		(INQUIRY_DEVICE_ASSOCIATION_TARGET_PORT << 4) |
		INQUIRY_DEVICE_IDENTIFIER_SCSI_NAME;
	len = (uint8_t) snprintf((char *)&cp[4],
				(unsigned)(*cdbsize - (int)(cp - &data[4])),
				"%s,t,%#x",
				iscsi_target_getvar(tgt, "iqn"),
				lun);
	cp[3] = len;
	*rspc += len + 4;
	cp += len + 4;
	/* add target port's IQN + LUN extension */
	cp[0] = (INQUIRY_DEVICE_ISCSI_PROTOCOL << 4) |
		INQUIRY_DEVICE_CODESET_UTF8;
	cp[1] = (INQUIRY_DEVICE_PIV << 7) |
		(INQUIRY_DEVICE_ASSOCIATION_LOGICAL_UNIT << 4) |
		INQUIRY_DEVICE_IDENTIFIER_SCSI_NAME;
	len = (uint8_t) snprintf((char *)&cp[4],
				(unsigned) (*cdbsize - (int)(cp - &data[4])),
				"%s,L,0x%8.8s%4.4s%4.4s",
				iscsi_target_getvar(tgt, "iqn"),
				uuid, &uuid[9], &uuid[14]);
	cp[3] = len;
	*rspc += len + 4;
	cp += len + 4;
	/* add target's uuid as a T10 identifier */
	cp[0] = (INQUIRY_DEVICE_ISCSI_PROTOCOL << 4) |
		INQUIRY_DEVICE_CODESET_UTF8;
	cp[1] = (INQUIRY_DEVICE_PIV << 7) |
		(INQUIRY_DEVICE_ASSOCIATION_TARGET_DEVICE << 4) |
		INQUIRY_IDENTIFIER_TYPE_T10;
	strpadcpy(&cp[4], 8, ISCSI_VENDOR, strlen(ISCSI_VENDOR), ' ');
	len = 8;
	len += (uint8_t) snprintf((char *)&cp[8 + 4],
				(unsigned)(*cdbsize - (int)(cp - &data[4])),
				"0x%8.8s%4.4s%4.4s",
				uuid, &uuid[9], &uuid[14]);
	cp[3] = len;
	*rspc += len + 4;
}

static void
version_inquiry(uint8_t *data, uint8_t *cdbsize)
{
	char	versionstr[8];

	data[0] = DISK_PERIPHERAL_DEVICE;
	data[2] = SCSI_VERSION_SPC;
	data[4] = *cdbsize - 4;	/* Additional length  */
	data[7] |= (WIDE_BUS_32 | WIDE_BUS_16);
	strpadcpy(&data[8], 8, ISCSI_VENDOR, strlen(ISCSI_VENDOR), ' ');
	strpadcpy(&data[16], 16, ISCSI_PRODUCT, strlen(ISCSI_PRODUCT), ' ');
	(void) snprintf(versionstr, sizeof(versionstr), "%d", ISCSI_VERSION);
	strpadcpy(&data[32], 4, versionstr, strlen(versionstr), ' ');
}

int 
device_command(target_session_t *sess, target_cmd_t *cmd)
{
	iscsi_scsi_cmd_args_t  *args = cmd->scsi_cmd;
	uint32_t        	status;
	uint32_t		lba;
	uint16_t		len;
	uint8_t			*cdbsize;
	uint8_t			*rspc;
	uint8_t			*data;
	uint8_t			*cdb;
	uint8_t			lun;

	cdb = args->cdb;
	lun = (uint8_t) (args->lun >> 32);
	cdbsize = &cdb[4];

	/*
	 * added section to return no device equivalent for lun request
	 * beyond available lun
	 */
	if (lun >= disks.v[sess->d].luns) {
		data = args->send_data;
		(void) memset(data, 0x0, (size_t) *cdbsize);
		/*
		 * data[0] = 0x7F; means no device
		 */
		data[0] = 0x1F;	/* device type */
		data[0] |= 0x60;/* peripheral qualifier */
		args->input = 1;
		args->length = cdb[4] + 1;
		args->status = SCSI_SUCCESS;
		return 0;
	}

	lun = (uint8_t) sess->d;
	iscsi_trace(TRACE_SCSI_CMD, "SCSI op %#x (lun %d): \n", cdb[0], lun);

	switch (cdb[0]) {
	case TEST_UNIT_READY:
		iscsi_trace(TRACE_SCSI_CMD, "TEST_UNIT_READY\n");
		args->status = SCSI_SUCCESS;
		args->length = 0;
		break;

	case INQUIRY:
		iscsi_trace(TRACE_SCSI_CMD, "INQUIRY%s\n",
				(cdb[1] & INQUIRY_EVPD_BIT) ?
					" for Vital Product Data" : "");
		data = args->send_data;
		args->status = SCSI_SUCCESS;
		/* Clear allocated buffer */
		(void) memset(data, 0x0, (unsigned) *cdbsize);
		if (cdb[1] & INQUIRY_EVPD_BIT) {
			rspc = &data[3];
			switch(cdb[2]) {
			case INQUIRY_UNIT_SERIAL_NUMBER_VPD:
				*rspc = serial_vpd(data);
				args->length = 16;
				break;
			case INQUIRY_DEVICE_IDENTIFICATION_VPD:
				if (disks.v[sess->d].uuid_string == NULL) {
					nbuuid_create(&disks.v[sess->d].uuid,
						&status);
					nbuuid_to_string(&disks.v[sess->d].uuid,
						&disks.v[sess->d].uuid_string,
						&status);
				}
				device_vpd(sess->target, data, rspc, cdbsize,
					lun, disks.v[sess->d].uuid_string);
				args->length = *rspc + 6;
				break;
			case INQUIRY_SUPPORTED_VPD_PAGES:
				data[0] = DISK_PERIPHERAL_DEVICE;
				data[1] = INQUIRY_SUPPORTED_VPD_PAGES;
				*rspc = 3;	/* # of supported pages */
				data[4] = INQUIRY_SUPPORTED_VPD_PAGES;
				data[5] = INQUIRY_DEVICE_IDENTIFICATION_VPD;
				data[6] = EXTENDED_INQUIRY_DATA_VPD;
				args->length = *cdbsize + 1;
				break;
			case EXTENDED_INQUIRY_DATA_VPD:
				data[0] = DISK_PERIPHERAL_DEVICE;
				data[1] = EXTENDED_INQUIRY_DATA_VPD;
				data[3] = 0x3c;	/* length is defined to be 60 */
				data[4] = 0;
				data[5] = 0;
				args->length = 64;
				break;
			default:
				iscsi_err(__FILE__, __LINE__,
					"Unsupported INQUIRY VPD page %x\n",
					cdb[2]);
				args->status = SCSI_CHECK_CONDITION;
				break;
			}
		} else {
			version_inquiry(data, cdbsize);
			args->length = cdb[4] + 1;
		}
		if (args->status == SCSI_SUCCESS) {
			args->input = 1;
		}
		break;

	case MODE_SELECT_6:
		iscsi_trace(TRACE_SCSI_CMD, "MODE_SELECT_6\n");
		args->status = SCSI_SUCCESS;
		args->length = 0;
		break;

	case STOP_START_UNIT:
		iscsi_trace(TRACE_SCSI_CMD, "STOP_START_UNIT\n");
		args->status = SCSI_SUCCESS;
		args->length = 0;
		break;

	case READ_CAPACITY:
		iscsi_trace(TRACE_SCSI_CMD, "READ_CAPACITY\n");
		data = args->send_data;
		*((uint32_t *)(void *)data) = (uint32_t) ISCSI_HTONL(
				(uint32_t) disks.v[sess->d].blockc - 1);
				/* Max LBA */
		*((uint32_t *)(void *)(data + 4)) = (uint32_t) ISCSI_HTONL(
				(uint32_t) disks.v[sess->d].blocklen);
				/* Block len */
		args->input = 8;
		args->length = 8;
		args->status = SCSI_SUCCESS;
		break;

	case WRITE_6:
		lba = ISCSI_NTOHL(*((uint32_t *) (void *)cdb)) & 0x001fffff;
		if ((len = *cdbsize) == 0) {
			len = 256;
		}
		iscsi_trace(TRACE_SCSI_CMD, 
				"WRITE_6(lba %u, len %u blocks)\n", lba, len);
		if (disk_write(sess, args, lun, lba, (unsigned) len) != 0) {
			iscsi_err(__FILE__, __LINE__,
				"disk_write() failed\n");
			args->status = SCSI_CHECK_CONDITION;
		}
		args->length = 0;
		break;


	case READ_6:
		lba = ISCSI_NTOHL(*((uint32_t *)(void *)cdb)) & 0x001fffff;
		if ((len = *cdbsize) == 0) {
			len = 256;
		}
		iscsi_trace(TRACE_SCSI_CMD,
			"READ_6(lba %u, len %u blocks)\n", lba, len);
		if (disk_read(sess, args, lba, len, lun) != 0) {
			iscsi_err(__FILE__, __LINE__,
					"disk_read() failed\n");
			args->status = SCSI_CHECK_CONDITION;
		}
		args->input = 1;
		break;

	case MODE_SENSE_6:
		mode_sense(6, cmd);
		break;

	case WRITE_10:
	case WRITE_VERIFY:
		cdb2lba(&lba, &len, cdb);

		iscsi_trace(TRACE_SCSI_CMD,
			"WRITE_10 | WRITE_VERIFY(lba %u, len %u blocks)\n",
			lba, len);
		if (disk_write(sess, args, lun, lba, (unsigned) len) != 0) {
			iscsi_err(__FILE__, __LINE__,
					"disk_write() failed\n");
			args->status = SCSI_CHECK_CONDITION;
		}
		args->length = 0;
		break;

	case READ_10:
		cdb2lba(&lba, &len, cdb);
		iscsi_trace(TRACE_SCSI_CMD,
				"READ_10(lba %u, len %u blocks)\n", lba, len);
		if (disk_read(sess, args, lba, len, lun) != 0) {
			iscsi_err(__FILE__, __LINE__,
				"disk_read() failed\n");
			args->status = SCSI_CHECK_CONDITION;
		}
		args->input = 1;
		break;

	case VERIFY:
		/* For now just set the status to success. */
		args->status = SCSI_SUCCESS;
		break;

	case SYNC_CACHE:
		cdb2lba(&lba, &len, cdb);
		iscsi_trace(TRACE_SCSI_CMD,
			"SYNC_CACHE (lba %u, len %u blocks)\n", lba, len);
		if (de_fsync_range(&disks.v[sess->d].lunv->v[lun].de,
				FDATASYNC, lba,
				(off_t)(len * disks.v[sess->d].blocklen)) < 0) {
			iscsi_err(__FILE__, __LINE__,
					"disk_read() failed\n");
			args->status = SCSI_CHECK_CONDITION;
		} else {
			args->status = SCSI_SUCCESS;
			args->length = 0;
		}
		break;

	case LOG_SENSE:
		iscsi_trace(TRACE_SCSI_CMD, "LOG_SENSE\n");
		args->status = SCSI_SUCCESS;
		args->length = 0;
		break;

	case MODE_SENSE_10:
		mode_sense(10, cmd);
		break;

	case MODE_SELECT_10:
		/* XXX still to do */
		iscsi_trace(TRACE_SCSI_CMD, "MODE_SELECT_10\n");
		args->status = SCSI_SUCCESS;
		args->length = 0;
		break;

	case PERSISTENT_RESERVE_IN:
		iscsi_trace(TRACE_SCSI_CMD, "PERSISTENT_RESERVE_IN\n");
		args->length = persistent_reserve_in((cdb[1] &
				PERSISTENT_RESERVE_IN_SERVICE_ACTION_MASK),
				args->send_data);
		args->status = SCSI_SUCCESS;
		break;

	case REPORT_LUNS:
		iscsi_trace(TRACE_SCSI_CMD, "REPORT LUNS\n");
		args->length = report_luns(
				(uint64_t *)(void *)&args->send_data[8],
				(off_t)disks.v[sess->d].luns);
		*((uint32_t *)(void *)args->send_data) =
				ISCSI_HTONL(disks.v[sess->d].luns *
				sizeof(uint64_t));
		args->input = 8;
		args->status = SCSI_SUCCESS;
		break;

	case RESERVE_6:
		iscsi_trace(TRACE_SCSI_CMD, "RESERVE_6\n");
		args->status = SCSI_SUCCESS;
		args->length = 0;
		break;

	case RELEASE_6:
		iscsi_trace(TRACE_SCSI_CMD, "RELEASE_6\n");
		args->status = SCSI_SUCCESS;
		args->length = 0;
		break;

	case RESERVE_10:
		iscsi_trace(TRACE_SCSI_CMD, "RESERVE_10\n");
		args->status = SCSI_SUCCESS;
		args->length = 0;
		break;

	case RELEASE_10:
		iscsi_trace(TRACE_SCSI_CMD, "RELEASE_10\n");
		args->status = SCSI_SUCCESS;
		args->length = 0;
		break;

	default:
		iscsi_err(__FILE__, __LINE__,
			"UNKNOWN OPCODE %#x\n", cdb[0]);
		/* to not cause confusion with some initiators */
		args->status = SCSI_CHECK_CONDITION;
		break;
	}
	iscsi_trace(TRACE_SCSI_DEBUG,
		"SCSI op %#x: done (status %#x)\n", cdb[0], args->status);
	return 0;
}

int 
device_shutdown(target_session_t *sess)
{
	USE_ARG(sess);
	return 1;
}

/*
 * Private Interface
 */

static int 
disk_write(target_session_t *sess, iscsi_scsi_cmd_args_t *args, uint8_t lun,
		uint32_t lba, uint32_t len)
{
	struct iovec    sg;
	uint64_t        byte_offset;
	uint64_t        bytec;
	uint8_t        *ptr;

	byte_offset = lba * disks.v[sess->d].blocklen;
	bytec = len * disks.v[sess->d].blocklen;
	ptr = NULL;
	iscsi_trace(TRACE_SCSI_DATA,
		"writing %" PRIu64
		" bytes from socket into device at byte offset %" PRIu64 "\n",
		bytec, byte_offset);

	if ((unsigned) bytec > MB(1)) {
		iscsi_err(__FILE__, __LINE__, "bytec > %u\n", bytec);
		NO_CLEANUP;
		return -1;
	}

	/* Assign ptr for write data */
	ptr = disks.v[sess->d].buffer;

	/* Have target do data transfer */
	sg.iov_base = ptr;
	sg.iov_len = (unsigned)bytec;
	if (target_transfer_data(sess, args, &sg, 1) != 0) {
		iscsi_err(__FILE__, __LINE__,
			"target_transfer_data() failed\n");
		return -1;
	}
	/* Finish up write */
	if (de_lseek(&disks.v[sess->d].lunv->v[lun].de, (off_t)byte_offset,
				SEEK_SET) == -1) {
		iscsi_err(__FILE__, __LINE__,
			"lseek() to offset %" PRIu64 " failed\n",
			byte_offset);
		return -1;
	}
	if (!target_writable(&disks.v[sess->d].lunv->v[lun])) {
		iscsi_err(__FILE__, __LINE__,
			"write() of %" PRIu64 " bytes failed at offset %"
			PRIu64 ", size %" PRIu64 "[READONLY TARGET]\n",
			bytec, byte_offset,
			de_getsize(&disks.v[sess->d].lunv->v[lun].de));
		return -1;
	}
	if ((uint64_t)de_write(&disks.v[sess->d].lunv->v[lun].de, ptr,
			(unsigned) bytec) != bytec) {
		iscsi_err(__FILE__, __LINE__,
			"write() of %" PRIu64 " bytes failed at offset %"
			PRIu64 ", size %" PRIu64 "\n",
			bytec, byte_offset,
			de_getsize(&disks.v[sess->d].lunv->v[lun].de));
		return -1;
	}
	iscsi_trace(TRACE_SCSI_DATA, 
		"wrote %" PRIu64 " bytes to device OK\n", bytec);
	return 0;
}

static int 
disk_read(target_session_t *sess, iscsi_scsi_cmd_args_t *args, uint32_t lba,
		uint16_t len, uint8_t lun)
{
	uint64_t        byte_offset;
	uint64_t        bytec;
	uint64_t        extra;
	uint8_t        *ptr;
	uint32_t        n;
	int             rc;

	byte_offset = lba * disks.v[sess->d].blocklen;
	bytec = len * disks.v[sess->d].blocklen;
	extra = 0;
	ptr = NULL;
	if (len == 0) {
		iscsi_err(__FILE__, __LINE__, "Zero \"len\"\n");
		NO_CLEANUP;
		return -1;
	}
	if (lba > disks.v[sess->d].blockc - 1 ||
	    (lba + len) > disks.v[sess->d].blockc) {
		iscsi_err(__FILE__, __LINE__,
			"attempt to read beyond end of media\n"
			"max_lba = %" PRIu64 ", requested lba = %u, len = %u\n",
			disks.v[sess->d].blockc - 1, lba, len);
		return -1;
	}
	if ((unsigned) bytec > MB(1)) {
		iscsi_err(__FILE__, __LINE__, "bytec > %u\n", bytec);
		NO_CLEANUP;
		return -1;
	}
	ptr = disks.v[sess->d].buffer;
	n = 0;
	do {
		if (de_lseek(&disks.v[sess->d].lunv->v[lun].de,
				(off_t)(n + byte_offset), SEEK_SET) == -1) {
			iscsi_err(__FILE__, __LINE__, "lseek failed\n");
			return -1;
		}
		rc = de_read(&disks.v[sess->d].lunv->v[lun].de, ptr + n,
				(size_t)(bytec - n));
		if (rc <= 0) {
			iscsi_err(__FILE__, __LINE__,
				"read failed: rc %d errno %d\n", rc, errno);
			return -1;
		}
		n += rc;
		if (n < bytec) {
			iscsi_err(__FILE__, __LINE__,
				"Got partial file read: %d bytes of %" PRIu64
				"\n", rc, bytec - n + rc);
		}
	} while (n < bytec);
	((struct iovec *)(void *)args->send_data)[0].iov_base =
			ptr + (unsigned) extra;
	((struct iovec *)(void *)args->send_data)[0].iov_len =
			(unsigned) bytec;
	args->length = (unsigned) bytec;
	args->send_sg_len = 1;
	args->status = 0;
	return 0;
}
