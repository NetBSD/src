/* $NetBSD: disk.c,v 1.18 2006/05/26 16:34:43 agc Exp $ */

/*
 * Copyright © 2006 Alistair Crooks.  All rights reserved.
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
 *	This product includes software developed by Alistair Crooks
 *	for the NetBSD project.
 * 4. The name of the author may not be used to endorse or promote
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
/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By downloading, copying, installing or
 * using the software you agree to this license. If you do not agree to this license, do not download, install,
 * copy or use the software.
 *
 * Intel License Agreement
 *
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *
 * -Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *  following disclaimer.
 *
 * -Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *  following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * -The name of Intel Corporation may not be used to endorse or promote products derived from this software
 *  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

#ifdef HAVE_UUID_H
#include <uuid.h>
#endif

#include "scsi_cmd_codes.h"

#include "iscsi.h"
#include "compat.h"
#include "util.h"
#include "device.h"
#include "target.h"
#include "defs.h"
#include "storage.h"

/*
 * Begin disk configuration. If we're operating in filesystem mode, then the
 * file /tmp/<targetname>_<iscsi port>_iscsi_disk_lun_x is created for each
 * lun x. You can create symbolic links in /tmp to point to devices in /dev.
 * For example, "ln -s /dev/sda /tmp/mytarget_3260_iscsi_disk_lun_0." If
 * we're operating in ramdisk mode, then each lun is its own ramdisk.
 */

#define CONFIG_DISK_NUM_LUNS_DFLT           1
#define CONFIG_DISK_BLOCK_LEN_DFLT          512
#define CONFIG_DISK_NUM_BLOCKS_DFLT         204800
#define CONFIG_DISK_INITIAL_CHECK_CONDITION 0
#define CONFIG_DISK_MAX_LUNS                8

/* End disk configuration */

/*
 * Globals
 */
enum {
 	ISCSI_RAMDISK =	0x01,
	ISCSI_FS_MMAP = 0x02,
	ISCSI_FS =	0x03
};

#define MB(x)	((x) * 1024 * 1024)

typedef struct iscsi_disk_t {
	int		 type;
	uint8_t		*ramdisk[CONFIG_DISK_MAX_LUNS];
	char		 filename[MAXPATHLEN];
	uint8_t		 buffer[CONFIG_DISK_MAX_LUNS][MB(1)];
	uint64_t	 blockc;
	uint64_t	 blocklen;
	uint64_t	 luns;
	uint64_t	 size;
	uuid_t		 uuid;
	char		*uuid_string;
	targv_t		*tv;
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

static int      disk_read(target_session_t * , iscsi_scsi_cmd_args_t * , uint32_t , uint16_t , uint8_t );
static int      disk_write(target_session_t * , iscsi_scsi_cmd_args_t * , uint8_t , uint32_t , uint32_t );

/* return the de index and offset within the device for RAID0 */
static int
raid0_getoff(disc_device_t *dp, uint64_t off, int *d, uint64_t *de_off)
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
	int	i;

	for (fd = -1, i = 0 ; i < dp->c ; i++) {
		switch (dp->xv[i].type) {
		case DE_DEVICE:
			if ((fd = device_open(dp->xv[i].u.dp, flags, mode)) < 0) {
				return -1;
			}
			break;
		case DE_EXTENT:
			if ((fd = extent_open(dp->xv[i].u.xp, flags, mode)) < 0) {
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
	return lseek(xp->fd, xp->sacred + off, whence);
}

/* (recursively) lseek on the device's devices */
static off_t
device_lseek(disc_device_t *dp, off_t off, int whence)
{
	uint64_t	suboff;
	off_t		ret;
	int		d;

	ret = -1;
	switch(dp->raid) {
	case 0:
		if (raid0_getoff(dp, off, &d, &suboff)) {
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				if ((ret = device_lseek(dp->xv[d].u.dp, suboff, whence)) < 0) {
					return -1;
				}
				break;
			case DE_EXTENT:
				if ((ret = extent_lseek(dp->xv[d].u.xp, suboff, whence)) < 0) {
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
				if ((ret = device_lseek(dp->xv[d].u.dp, off, whence)) < 0) {
					return -1;
				}
				break;
			case DE_EXTENT:
				if ((ret = extent_lseek(dp->xv[d].u.xp, off, whence)) < 0) {
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
	return fsync_range(xp->fd, how, xp->sacred + from, len);
#else
	return fsync(xp->fd);
#endif
}

/* (recursively) lseek on the device's devices */
static int
device_fsync_range(disc_device_t *dp, int how, off_t from, off_t len)
{
	uint64_t	suboff;
	int		ret;
	int		d;

	ret = -1;
	switch(dp->raid) {
	case 0:
		if (raid0_getoff(dp, from, &d, &suboff)) {
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				if ((ret = device_fsync_range(dp->xv[d].u.dp, how, suboff, len)) < 0) {
					return -1;
				}
				break;
			case DE_EXTENT:
				if ((ret = extent_fsync_range(dp->xv[d].u.xp, how, suboff, len)) < 0) {
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
				if ((ret = device_fsync_range(dp->xv[d].u.dp, how, from, len)) < 0) {
					return -1;
				}
				break;
			case DE_EXTENT:
				if ((ret = extent_fsync_range(dp->xv[d].u.xp, how, from, len)) < 0) {
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
	uint64_t	suboff;
	uint64_t	got;
	ssize_t		ret;
	size_t		subcc;
	char		*cbuf;
	int		d;

	ret = -1;
	switch(dp->raid) {
	case 0:
		for (cbuf = (char *) buf, got = 0 ; got < cc ; got += ret) {
			if (!raid0_getoff(dp, dp->off, &d, &suboff)) {
				return -1;
			}
			if (device_lseek(dp, dp->off, SEEK_SET) < 0) {
				return -1;
			}
			subcc = MIN(cc - got, dp->len - dp->off);
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				if ((ret = device_read(dp->xv[d].u.dp, &cbuf[got], subcc)) < 0) {
					return -1;
				}
				break;
			case DE_EXTENT:
				if ((ret = extent_read(dp->xv[d].u.xp, &cbuf[got], subcc)) < 0) {
					return -1;
				}
				break;
			default:
				break;
			}
			dp->off += ret;
		}
		ret = got;
		break;
	case 1:
		for (d = 0 ; d < dp->c ; d++) {
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				if ((ret = device_read(dp->xv[d].u.dp, buf, cc)) < 0) {
					return -1;
				}
				break;
			case DE_EXTENT:
				if ((ret = extent_read(dp->xv[d].u.xp, buf, cc)) < 0) {
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
	uint64_t	suboff;
	uint64_t	done;
	ssize_t		ret;
	size_t		subcc;
	char		*cbuf;
	int		d;

	ret = -1;
	switch(dp->raid) {
	case 0:
		for (cbuf = (char *) buf, done = 0 ; done < cc ; done += ret) {
			if (!raid0_getoff(dp, dp->off, &d, &suboff)) {
				return -1;
			}
			subcc = MIN(cc - done, dp->len - dp->off);
			if (device_lseek(dp, dp->off, SEEK_SET) < 0) {
				return -1;
			}
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				if ((ret = device_write(dp->xv[d].u.dp, &cbuf[done], subcc)) < 0) {
					return -1;
				}
				break;
			case DE_EXTENT:
				if ((ret = extent_write(dp->xv[d].u.xp, &cbuf[done], subcc)) < 0) {
					return -1;
				}
				break;
			default:
				break;
			}
			dp->off += ret;
		}
		ret = done;
		break;
	case 1:
		for (d = 0 ; d < dp->c ; d++) {
			switch (dp->xv[d].type) {
			case DE_DEVICE:
				if ((ret = device_write(dp->xv[d].u.dp, buf, cc)) < 0) {
					iscsi_trace_error(__FILE__, __LINE__, "device_write RAID1 device write failure\n");
					return -1;
				}
				break;
			case DE_EXTENT:
				if ((ret = extent_write(dp->xv[d].u.xp, buf, cc)) < 0) {
					iscsi_trace_error(__FILE__, __LINE__, "device_write RAID1 extent write failure\n");
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
	int		d;

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
static uint64_t
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

/* mmap on the extent */
static void *
extent_mmap(void *addr, size_t len, int prot, int flags, disc_extent_t *xp, off_t offset)
{
	return mmap(addr, len, prot, flags, xp->fd, xp->sacred + offset);
}

/* (recursively) mmap on the device's devices */
static void *
device_mmap(void *addr, size_t len, int prot, int flags, disc_device_t *dp, off_t offset)
{
	void	*ret;
	int	i;

	for (ret = MAP_FAILED, i = 0 ; i < dp->c ; i++) {
		switch (dp->xv[i].type) {
		case DE_DEVICE:
			if ((ret = device_mmap(addr, len, prot, flags, dp->xv[i].u.dp, offset)) == MAP_FAILED) {
				return MAP_FAILED;
			}
			break;
		case DE_EXTENT:
			if ((ret = extent_mmap(addr, len, prot, flags, dp->xv[i].u.xp, offset)) == MAP_FAILED) {
				return MAP_FAILED;
			}
			break;
		default:
			break;
		}
	}
	return ret;
}

/* and for the undecided... */
static void *
de_mmap(void *addr, size_t len, int prot, int flags, disc_de_t *dp, off_t offset)
{
	switch(dp->type) {
	case DE_DEVICE:
		return device_mmap(addr, len, prot, flags, dp->u.dp, offset);
	case DE_EXTENT:
		return extent_mmap(addr, len, prot, flags, dp->u.xp, offset);
	default:
		return MAP_FAILED;
	}
}

/* munmap the extent's device */
/* ARGSUSED */
static int
extent_munmap(disc_extent_t *xp, void *addr, size_t len)
{
	return munmap(addr, len);
}

/* (recursively) munmap the device's devices */
static int
device_munmap(disc_device_t *dp, void *addr, size_t len)
{
	int	ret;
	int	i;

	for (ret = -1, i = 0 ; i < dp->c ; i++) {
		switch (dp->xv[i].type) {
		case DE_DEVICE:
			if ((ret = device_munmap(dp->xv[i].u.dp, addr, len)) < 0) {
				return -1;
			}
			break;
		case DE_EXTENT:
			if ((ret = extent_munmap(dp->xv[i].u.xp, addr, len)) < 0) {
				return -1;
			}
			break;
		default:
			break;
		}
	}
	return ret;
}

/* and for the undecided... */
static int
de_munmap(disc_de_t *dp, void *addr, size_t len)
{
	switch(dp->type) {
	case DE_DEVICE:
		return device_munmap(dp->u.dp, addr, len);
	case DE_EXTENT:
		return extent_munmap(dp->u.xp, addr, len);
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
device_set_var(const char *var, char *arg)
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

/* allocate some space for a disk/extent, using an lseek, read and write combination */
static int
de_allocate(disc_de_t *de, char *filename)
{
	uint64_t	size;
	char		ch;

	size = de_getsize(de);
	if (de_lseek(de, size - 1, SEEK_SET) == -1) {
		iscsi_trace_error(__FILE__, __LINE__, "error seeking \"%s\"\n", filename);
		return 0;
	}
	if (de_read(de, &ch, 1) == -1) {
		iscsi_trace_error(__FILE__, __LINE__, "error reading \"%s\"", filename);
		return 0;
	}
	if (de_write(de, &ch, 1) == -1) {
		iscsi_trace_error(__FILE__, __LINE__, "error writing \"%s\"", filename);
		return 0;
	}
	return 1;
}

/* allocate space as desired */
static int
allocate_space(disc_target_t *tp)
{
	int	i;

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
strpadcpy(uint8_t *dst, size_t dstlen, const char *src, const size_t srclen, char pad)
{
	int	i;

	if (srclen < dstlen) {
		(void) memcpy(dst, src, srclen);
		for (i = srclen ; i < dstlen ; i++) {
			dst[i] = pad;
		}
	} else {
		(void) memcpy(dst, src, dstlen);
	}
}	

/* handle REPORT LUNs SCSI command */
static int
report_luns(uint64_t *data, int64_t luns)
{
	uint64_t	lun;
	int32_t		off;

	for (lun = 0, off = 8 ; lun < luns ; lun++, off += sizeof(lun)) {
		data[lun] = ISCSI_HTONLL(lun);
	}
	return off;
}

/* initialise the device */
int 
device_init(globals_t *gp, targv_t *tvp, disc_target_t *tp)
{
	int   	i;

	ALLOC(iscsi_disk_t, disks.v, disks.size, disks.c, 10, 10, "device_init", ;);
	disks.v[disks.c].tv = tvp;
	if ((disks.v[disks.c].luns = defaults.luns) == 0) {
		disks.v[disks.c].luns = CONFIG_DISK_NUM_LUNS_DFLT;
	}
	if ((disks.v[disks.c].blocklen = defaults.blocklen) == 0) {
		disks.v[disks.c].blocklen = CONFIG_DISK_BLOCK_LEN_DFLT;
	}
	disks.v[disks.c].size = de_getsize(&tp->de);
	disks.v[disks.c].blockc = disks.v[disks.c].size / disks.v[disks.c].blocklen;
	switch(disks.v[disks.c].blocklen) {
	case 512:
	case 1024:
	case 2048:
	case 4096:
		break;
	default:
		iscsi_trace_error(__FILE__, __LINE__, "Invalid block len %" PRIu64 ". Choose one of 512, 1024, 2048, 4096.\n", disks.v[disks.c].blocklen);
		return -1;
	}
#if 0
	disks.v[disks.c].type = (disks.v[disks.c].size > MB(100)) ?
		ISCSI_FS : (disks.v[disks.c].size > MB(50)) ?
		ISCSI_FS_MMAP : ISCSI_RAMDISK;
#else
	disks.v[disks.c].type = ISCSI_FS;
#endif
	printf("DISK: %" PRIu64 " logical units (%" PRIu64 " blocks, %" PRIu64 " bytes/block), type %s\n",
	      disks.v[disks.c].luns, disks.v[disks.c].blockc, disks.v[disks.c].blocklen,
	      (disks.v[disks.c].type == ISCSI_FS) ? "iscsi fs" :
	      (disks.v[disks.c].type == ISCSI_FS_MMAP) ? "iscsi fs mmap" : "iscsi ramdisk");
	for (i = 0; i < disks.v[disks.c].luns; i++) {
		printf("DISK: LU %i: ", i);

		if (disks.v[disks.c].type == ISCSI_RAMDISK) {
			if ((disks.v[disks.c].ramdisk[i] = iscsi_malloc((unsigned)disks.v[disks.c].size)) == NULL) {
				iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc() failed\n");
				return -1;
			}
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "allocated %" PRIu64 " bytes at %p\n", disks.v[disks.c].size, disks.v[disks.c].ramdisk[i]);
			printf("%" PRIu64 " MB ramdisk\n", disks.v[disks.c].size / MB(1));
		} else {
			(void) strlcpy(disks.v[disks.c].filename, disc_get_filename(&tp->de), sizeof(disks.v[disks.c].filename));
			if (de_open(&tp->de, O_CREAT | O_RDWR, 0666) == -1) {
				iscsi_trace_error(__FILE__, __LINE__, "error opening \"%s\"\n", disks.v[disks.c].filename);
				return -1;
			}
			if (!allocate_space(tp)) {
				iscsi_trace_error(__FILE__, __LINE__, "error allocating space for \"%s\"", tp->target);
				return -1;
			}
			printf("%" PRIu64 " MB disk storage for \"%s\"\n", (de_getsize(&tp->de) / MB(1)), tp->target);
		}
	}
	return disks.c++;
}

int 
device_command(target_session_t * sess, target_cmd_t * cmd)
{
	iscsi_scsi_cmd_args_t	*args = cmd->scsi_cmd;
	uint32_t        	status;
	uint32_t		lba;
	uint16_t		len;
	uint8_t			*cp;
	uint8_t			*data;
	uint8_t			*cdb = args->cdb;
	uint8_t			lun = (uint8_t) (args->lun >> 32);
	int			mode_data_len;

#if (CONFIG_DISK_INITIAL_CHECK_CONDITION==1)
	static int      initialized = 0;
	static int      flag[disks.v[sess->d].luns];
	int             i;

	if (!initialized) {
		for (i = 0; i < disks.v[sess->d].luns; i++) {
			flag[i] = 0;
		}
		initialized = 1;
	}
	if (!flag[lun]) {
		printf("DISK: Simulating CHECK CONDITION with sense data (cdb 0x%x, lun %i)\n", cdb[0], lun);
		flag[lun]++;
		args->status = 0x02;
		args->length = 1024;
		return 0;
	}
#endif

	/*
	 * added section to return no device equivalent for lun request
	 * beyond available lun
	 */
	if (lun >= disks.v[sess->d].luns) {
		data = args->send_data;
		(void) memset(data, 0x0, (size_t) cdb[4]);
		/*
		 * data[0] = 0x7F;
		 * / no device
		 */
		data[0] = 0x1F;	/* device type */
		data[0] |= 0x60;/* peripheral qualifier */
		args->input = 1;
		args->length = cdb[4] + 1;
		args->status = 0;
		return 0;
	}
	iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "SCSI op 0x%x (lun %i): \n", cdb[0], lun);

	switch (cdb[0]) {

	case TEST_UNIT_READY:
		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "TEST_UNIT_READY\n");
		args->status = 0;
		args->length = 0;
		break;

	case INQUIRY:
		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "INQUIRY%s\n", (cdb[1] & INQUIRY_EVPD_BIT) ? " for Vital Product Data" : "");
		data = args->send_data;
		args->status = 0;
		(void) memset(data, 0x0, (unsigned) cdb[4]);	/* Clear allocated buffer */
		if (cdb[1] & INQUIRY_EVPD_BIT) {
			switch(cdb[2]) {
			case INQUIRY_DEVICE_IDENTIFICATION_VPD:
				data[0] = DISK_PERIPHERAL_DEVICE;
				data[1] = INQUIRY_DEVICE_IDENTIFICATION_VPD;
				data[3] = 0;
				cp = &data[4];
				/* add target device's IQN */
				cp[0] = (INQUIRY_DEVICE_ISCSI_PROTOCOL << 4) | INQUIRY_DEVICE_CODESET_UTF8;
				cp[1] = (INQUIRY_DEVICE_PIV << 7) | (INQUIRY_DEVICE_ASSOCIATION_TARGET_DEVICE << 4) | INQUIRY_DEVICE_IDENTIFIER_SCSI_NAME;
				len = (uint8_t) snprintf((char *)&cp[4], (int)(cdb[4] - 7), "%s", sess->globals->targetname);
				cp[3] = len;
				data[3] += len + 4;
				cp += len + 4;
				/* add target port's IQN + LUN */
				cp[0] = (INQUIRY_DEVICE_ISCSI_PROTOCOL << 4) | INQUIRY_DEVICE_CODESET_UTF8;
				cp[1] = (INQUIRY_DEVICE_PIV << 7) | (INQUIRY_DEVICE_ASSOCIATION_TARGET_PORT << 4) | INQUIRY_DEVICE_IDENTIFIER_SCSI_NAME;
				len = (uint8_t) snprintf((char *)&cp[4], (int)(cdb[4] - 7), "%s,t,0x%x", sess->globals->targetname, lun);
				cp[3] = len;
				data[3] += len + 4;
				cp += len + 4;
				/* add target port's IQN + LUN extension */
				cp[0] = (INQUIRY_DEVICE_ISCSI_PROTOCOL << 4) | INQUIRY_DEVICE_CODESET_UTF8;
				cp[1] = (INQUIRY_DEVICE_PIV << 7) | (INQUIRY_DEVICE_ASSOCIATION_LOGICAL_UNIT << 4) | INQUIRY_DEVICE_IDENTIFIER_SCSI_NAME;
				if (disks.v[sess->d].uuid_string == NULL) {
					uuid_create(&disks.v[sess->d].uuid, &status);
					uuid_to_string(&disks.v[sess->d].uuid, &disks.v[sess->d].uuid_string, &status);
				}
				len = (uint8_t) snprintf((char *)&cp[4], (int)(cdb[4] - 7), "%s,L,0x%8.8s%4.4s%4.4s",
							sess->globals->targetname,
							disks.v[sess->d].uuid_string,
							&disks.v[sess->d].uuid_string[9],
							&disks.v[sess->d].uuid_string[14]);
				cp[3] = len;
				data[3] += len + 4;
				cp += len + 4;
				/* add target's uuid as a T10 identifier */
				cp[0] = (INQUIRY_DEVICE_ISCSI_PROTOCOL << 4) | INQUIRY_DEVICE_CODESET_UTF8;
				cp[1] = (INQUIRY_DEVICE_PIV << 7) | (INQUIRY_DEVICE_ASSOCIATION_TARGET_DEVICE << 4) | INQUIRY_IDENTIFIER_TYPE_T10;
				strpadcpy(&cp[4], 8, ISCSI_VENDOR, strlen(ISCSI_VENDOR), ' ');
				len = (uint8_t) snprintf((char *)&cp[8 + 4], (int)(cdb[8 + 4] - 7), "0x%8.8s%4.4s%4.4s",
								disks.v[sess->d].uuid_string,
								&disks.v[sess->d].uuid_string[9],
								&disks.v[sess->d].uuid_string[14]);
				cp[3] = len;
				data[3] += len + 4;
				args->length = data[3] + 6;
				break;
			case INQUIRY_SUPPORTED_VPD_PAGES:
				data[0] = DISK_PERIPHERAL_DEVICE;
				data[1] = INQUIRY_SUPPORTED_VPD_PAGES;
				data[3] = 2;	/* # of supported pages */
				data[4] = INQUIRY_SUPPORTED_VPD_PAGES;
				data[5] = INQUIRY_DEVICE_IDENTIFICATION_VPD;
				args->length = cdb[4] + 1;
				break;
			default:
				iscsi_trace_error(__FILE__, __LINE__, "Unsupported INQUIRY VPD page %x\n", cdb[2]);
				args->status = 0x01;
				break;
			}
		} else {
			char	versionstr[8];

			data[0] = DISK_PERIPHERAL_DEVICE;
			data[2] = SCSI_VERSION_SPC;
			data[4] = cdb[4] - 4;	/* Additional length  */
			data[7] |= (WIDE_BUS_32 | WIDE_BUS_16);
			strpadcpy(&data[8], 8, ISCSI_VENDOR, strlen(ISCSI_VENDOR), ' ');
			strpadcpy(&data[16], 16, ISCSI_PRODUCT, strlen(ISCSI_PRODUCT), ' ');
			(void) snprintf(versionstr, sizeof(versionstr), "%d", ISCSI_VERSION);
			strpadcpy(&data[32], 4, versionstr, strlen(versionstr), ' ');
			args->length = cdb[4] + 1;
		}
		if (args->status == 0) {
			args->input = 1;
		}
		break;

	case STOP_START_UNIT:
		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "STOP_START_UNIT\n");
		args->status = 0;
		args->length = 0;
		break;

	case READ_CAPACITY:
		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "READ_CAPACITY\n");
		data = args->send_data;
		*((uint32_t *) (void *)data) = (uint32_t) ISCSI_HTONL((uint32_t) disks.v[sess->d].blockc - 1);	/* Max LBA */
		*((uint32_t *) (void *)(data + 4)) = (uint32_t) ISCSI_HTONL((uint32_t) disks.v[sess->d].blocklen);	/* Block len */
		args->input = 8;
		args->length = 8;
		args->status = 0;
		break;

	case WRITE_6:
		lba = ISCSI_NTOHL(*((uint32_t *) (void *)cdb)) & 0x001fffff;
		if ((len = cdb[4]) == 0) {
			len = 256;
		}
		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "WRITE_6(lba %u, len %u blocks)\n", lba, len);
		if (disk_write(sess, args, lun, lba, (unsigned) len) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "disk_write() failed\n");
			args->status = 0x01;
		}
		args->length = 0;
		break;


	case READ_6:
		lba = ISCSI_NTOHL(*((uint32_t *) (void *)cdb)) & 0x001fffff;
		if ((len = cdb[4]) == 0) {
			len = 256;
		}
		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "READ_6(lba %u, len %u blocks)\n", lba, len);
		if (disk_read(sess, args, lba, len, lun) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "disk_read() failed\n");
			args->status = 0x01;
		}
		args->input = 1;
		break;

	case MODE_SENSE_6:

		cp = data = args->send_data;
		len = ISCSI_MODE_SENSE_LEN;
		mode_data_len = len + 3;

		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "MODE_SENSE_6(len %u blocks)\n", len);
		(void) memset(cp, 0x0, (size_t) mode_data_len);
		/* magic constants courtesy of some values in the Lunix UNH iSCSI target */
		cp[0] = mode_data_len;
		cp[1] = 0;
		cp[2] = 0;
		cp[3] = 8;	/* block descriptor length */
		cp[10] = 2;	/* density code and block length */

		args->input = 1;
		args->length = (unsigned)(len);
		args->status = 0;
		break;

	case WRITE_10:

		cdb2lba(&lba, &len, cdb);

		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "WRITE_10(lba %u, len %u blocks)\n", lba, len);
		if (disk_write(sess, args, lun, lba, (unsigned) len) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "disk_write() failed\n");
			args->status = 0x01;
		}
		args->length = 0;
		break;

	case READ_10:

		cdb2lba(&lba, &len, cdb);

		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "READ_10(lba %u, len %u blocks)\n", lba, len);
		if (disk_read(sess, args, lba, len, lun) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "disk_read() failed\n");
			args->status = 0x01;
		}
		args->input = 1;
		break;

	case VERIFY:
		/* For now just set the status to success. */
		args->status = 0;
		break;

	case SYNC_CACHE:

		cdb2lba(&lba, &len, cdb);

		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "SYNC_CACHE (lba %u, len %u blocks)\n", lba, len);
		if (de_fsync_range(&disks.v[sess->d].tv->v[lun].de, FDATASYNC, lba, len * disks.v[sess->d].blocklen) < 0) {
			iscsi_trace_error(__FILE__, __LINE__, "disk_read() failed\n");
			args->status = 0x01;
		} else {
			args->status = 0;
			args->length = 0;
		}
		break;

	case LOG_SENSE:

		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "LOG_SENSE\n");
		args->status = 0;
		args->length = 0;
		break;

	case PERSISTENT_RESERVE_IN:

		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "PERSISTENT_RESERVE_IN\n");
		args->status = 0;
		args->length = 0;
		break;

	case REPORT_LUNS:
		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "REPORT LUNS\n");
		args->length = report_luns((uint64_t *) &args->send_data[8], disks.v[sess->d].luns);
		*((uint32_t *) (void *)args->send_data) = ISCSI_HTONL(disks.v[sess->d].luns * sizeof(uint64_t));
		args->input = 8;
		args->status = 0;
		break;

	default:
		iscsi_trace_error(__FILE__, __LINE__, "UNKNOWN OPCODE 0x%x\n", cdb[0]);
		/* to not cause confusion with some initiators */
		args->status = 0x02;
		break;
	}
	iscsi_trace(TRACE_SCSI_DEBUG, __FILE__, __LINE__, "SCSI op 0x%x: done (status 0x%x)\n", cdb[0], args->status);
	return 0;
}

int 
device_shutdown(target_session_t *sess)
{
	int             i;

	if (disks.v[sess->d].type == ISCSI_RAMDISK) {
		for (i = 0; i < disks.v[sess->d].luns; i++) {
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "freeing ramdisk[%i] (%p)\n", i, disks.v[sess->d].ramdisk[i]);
			iscsi_free(disks.v[sess->d].ramdisk[i]);
		}
	}
	return 1;
}

/*
 * Private Interface
 */

static int 
disk_write(target_session_t *sess, iscsi_scsi_cmd_args_t *args, uint8_t lun, uint32_t lba, uint32_t len)
{
	uint64_t        byte_offset = lba * disks.v[sess->d].blocklen;
	uint64_t        num_bytes = len * disks.v[sess->d].blocklen;
	uint8_t        *ptr = NULL;
	struct iovec    sg;
	uint64_t        extra = 0;

	iscsi_trace(TRACE_SCSI_DATA, __FILE__, __LINE__, "writing %" PRIu64 " bytes from socket into device at byte offset %" PRIu64 "\n", num_bytes, byte_offset);

	/* Assign ptr for write data */

	switch(disks.v[sess->d].type) {
	case ISCSI_RAMDISK:
		ptr = disks.v[sess->d].ramdisk[lun] + (int) byte_offset;
		break;
	case ISCSI_FS:
		RETURN_GREATER("num_bytes (FIX ME)", (unsigned) num_bytes, MB(1), NO_CLEANUP, -1);
		ptr = disks.v[sess->d].buffer[lun];
		break;
	case ISCSI_FS_MMAP:
		extra = byte_offset % 4096;
		if ((ptr = de_mmap(0, (size_t) (num_bytes + extra), PROT_WRITE, MAP_SHARED, &disks.v[sess->d].tv->v[lun].de, (off_t)(byte_offset - extra))) == NULL) {
			iscsi_trace_error(__FILE__, __LINE__, "mmap() failed\n");
			return -1;
		} else {
			ptr += (uint32_t) extra;
		}
	}

	/* Have target do data transfer */

	sg.iov_base = ptr;
	sg.iov_len = (unsigned)num_bytes;
	if (target_transfer_data(sess, args, &sg, 1) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "target_transfer_data() failed\n");
	}
	/* Finish up write */
	switch(disks.v[sess->d].type) {
	case ISCSI_FS:
		if (de_lseek(&disks.v[sess->d].tv->v[lun].de, (off_t) byte_offset, SEEK_SET) == -1) {
			iscsi_trace_error(__FILE__, __LINE__, "lseek() to offset %" PRIu64 " failed\n", byte_offset);
			return -1;
		}
		if (!target_writable(&disks.v[sess->d].tv->v[lun])) {
			iscsi_trace_error(__FILE__, __LINE__, "write() of %" PRIu64 " bytes failed at offset %" PRIu64 ", size %" PRIu64 "[READONLY TARGET]\n", num_bytes, byte_offset, de_getsize(&disks.v[sess->d].tv->v[lun].de));
			return -1;
		}
		if (de_write(&disks.v[sess->d].tv->v[lun].de, ptr, (unsigned) num_bytes) != num_bytes) {
			iscsi_trace_error(__FILE__, __LINE__, "write() of %" PRIu64 " bytes failed at offset %" PRIu64 ", size %" PRIu64 "\n", num_bytes, byte_offset, de_getsize(&disks.v[sess->d].tv->v[lun].de));
			return -1;
		}
		break;
	case ISCSI_FS_MMAP:
		ptr -= (uint32_t) extra;
		if (de_munmap(&disks.v[sess->d].tv->v[lun].de, ptr, (size_t)(num_bytes + extra)) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "munmap() failed\n");
			return -1;
		}
	}
	iscsi_trace(TRACE_SCSI_DATA, __FILE__, __LINE__, "wrote %" PRIu64 " bytes to device OK\n", num_bytes);
	return 0;
}

static int 
disk_read(target_session_t * sess, iscsi_scsi_cmd_args_t * args, uint32_t lba, uint16_t len, uint8_t lun)
{
	uint64_t        byte_offset = lba * disks.v[sess->d].blocklen;
	uint64_t        num_bytes = len * disks.v[sess->d].blocklen;
	uint64_t        extra = 0;
	uint8_t        *ptr = NULL;
	uint32_t        n;
	int             rc;
	static void    *last_ptr[CONFIG_DISK_MAX_LUNS];
	static uint64_t last_extra[CONFIG_DISK_MAX_LUNS];
	static uint64_t last_num_bytes[CONFIG_DISK_MAX_LUNS];
	static int      initialized = 0;

	/* Need to replace this with a callback (when the iSCSI read is done)  */
	/* that munmaps the ptrs for us. */

	if (disks.v[sess->d].type == ISCSI_FS_MMAP && !initialized) {
		int             i;
		for (i = 0; i < disks.v[sess->d].luns; i++) {
			last_ptr[i] = NULL;
			last_extra[i] = 0;
			last_num_bytes[i] = 0;
		}
		initialized++;
	}

	RETURN_EQUAL("len", len, 0, NO_CLEANUP, -1);
	if ((lba > (disks.v[sess->d].blockc - 1)) || ((lba + len) > disks.v[sess->d].blockc)) {
		iscsi_trace_error(__FILE__, __LINE__, "attempt to read beyond end of media\n");
		iscsi_trace_error(__FILE__, __LINE__, "max_lba = %" PRIu64 ", requested lba = %u, len = %u\n", disks.v[sess->d].blockc - 1, lba, len);
		return -1;
	}
	switch (disks.v[sess->d].type) {
	case ISCSI_RAMDISK:
		ptr = disks.v[sess->d].ramdisk[lun] + (int)byte_offset;
		break;
	case ISCSI_FS:
		RETURN_GREATER("num_bytes (FIX ME)", (unsigned) num_bytes, MB(1), NO_CLEANUP, -1);
		ptr = disks.v[sess->d].buffer[lun];
		n = 0;
		do {
			if (de_lseek(&disks.v[sess->d].tv->v[lun].de, (off_t)(n + byte_offset), SEEK_SET) == -1) {
				iscsi_trace_error(__FILE__, __LINE__, "lseek() failed\n");
				return -1;
			}
			rc = de_read(&disks.v[sess->d].tv->v[lun].de, ptr + n, (size_t)(num_bytes - n));
			if (rc <= 0) {
				iscsi_trace_error(__FILE__, __LINE__, "read() failed: rc %i errno %i\n", rc, errno);
				return -1;
			}
			n += rc;
			if (n < num_bytes) {
				iscsi_trace_error(__FILE__, __LINE__, "Got partial file read: %i bytes of %" PRIu64 "\n", rc, num_bytes - n + rc);
			}
		} while (n < num_bytes);
		break;
	case ISCSI_FS_MMAP:
		if (last_ptr[lun]) {
			if (de_munmap(&disks.v[sess->d].tv->v[lun].de, last_ptr[lun], (unsigned)(last_extra[lun] + last_num_bytes[lun])) != 0) {
				iscsi_trace_error(__FILE__, __LINE__, "munmap() failed\n");
				return -1;
			}
			last_ptr[lun] = NULL;
			last_num_bytes[lun] = 0;
			last_extra[lun] = 0;
		}
		extra = byte_offset % 4096;
		if ((ptr = de_mmap(0, (size_t)(num_bytes + extra), PROT_READ, MAP_SHARED, &disks.v[sess->d].tv->v[lun].de, (off_t)(byte_offset - extra))) == NULL) {
			iscsi_trace_error(__FILE__, __LINE__, "mmap() failed\n");
			return -1;
		}
		/* Need to replace this with a callback */

		last_ptr[lun] = ptr;
		last_num_bytes[lun] = num_bytes;
		last_extra[lun] = extra;
		break;
	}


	((struct iovec *) (void *)args->send_data)[0].iov_base = ptr + (unsigned) extra;
	((struct iovec *) (void *)args->send_data)[0].iov_len = (unsigned) num_bytes;
	args->length = (unsigned) num_bytes;
	args->send_sg_len = 1;
	args->status = 0;

	return 0;
}
