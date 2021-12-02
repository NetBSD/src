/*        $NetBSD: hammer2.c,v 1.9 2021/12/02 14:26:42 christos Exp $      */

/*-
 * Copyright (c) 2017-2019 The DragonFly Project
 * Copyright (c) 2017-2019 Tomohiro Kusumi <tkusumi@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hammer2.c,v 1.9 2021/12/02 14:26:42 christos Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/disk.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <assert.h>
#include <uuid.h>

#include "fstyp.h"
#include "hammer2_disk.h"

static ssize_t
get_file_size(FILE *fp)
{
	ssize_t siz;
	struct dkwedge_info dkw;

	if (ioctl(fileno(fp), DIOCGWEDGEINFO, &dkw) != -1) {
		return (ssize_t)dkw.dkw_size * DEV_BSIZE;
	}

	if (fseek(fp, 0, SEEK_END) == -1) {
		warnx("hammer2: failed to seek media end");
		return -1;
	}

	siz = ftell(fp);
	if (siz == -1) {
		warnx("hammer2: failed to tell media end");
		return -1;
	}

	return siz;
}

static hammer2_volume_data_t *
read_voldata(FILE *fp, int i)
{
	if (i < 0 || i >= HAMMER2_NUM_VOLHDRS)
		return NULL;

	if ((hammer2_off_t)i * (hammer2_off_t)HAMMER2_ZONE_BYTES64 >= (hammer2_off_t)get_file_size(fp))
		return NULL;

	return read_buf(fp, (off_t)i * (off_t)HAMMER2_ZONE_BYTES64,
	    sizeof(hammer2_volume_data_t));
}

static int
test_voldata(FILE *fp)
{
	hammer2_volume_data_t *voldata;
	int i;
	static int count = 0;
	static uuid_t fsid, fstype;

	for (i = 0; i < HAMMER2_NUM_VOLHDRS; i++) {
		if ((hammer2_off_t)i * (hammer2_off_t)HAMMER2_ZONE_BYTES64 >= (hammer2_off_t)get_file_size(fp))
			break;
		voldata = read_voldata(fp, i);
		if (voldata == NULL) {
			warnx("hammer2: failed to read volume data");
			return 1;
		}
		if (voldata->magic != HAMMER2_VOLUME_ID_HBO &&
		    voldata->magic != HAMMER2_VOLUME_ID_ABO) {
			free(voldata);
			return 1;
		}
		if (voldata->volu_id > HAMMER2_MAX_VOLUMES - 1) {
			free(voldata);
			return 1;
		}
		if (voldata->nvolumes > HAMMER2_MAX_VOLUMES) {
			free(voldata);
			return 1;
		}

		if (count == 0) {
			count = voldata->nvolumes;
			memcpy(&fsid, &voldata->fsid, sizeof(fsid));
			memcpy(&fstype, &voldata->fstype, sizeof(fstype));
		} else {
			if (voldata->nvolumes != count) {
				free(voldata);
				return 1;
			}
			if (!uuid_equal(&fsid, &voldata->fsid, NULL)) {
				free(voldata);
				return 1;
			}
			if (!uuid_equal(&fstype, &voldata->fstype, NULL)) {
				free(voldata);
				return 1;
			}
		}
		free(voldata);
	}

	return 0;
}

static hammer2_media_data_t*
read_media(FILE *fp, const hammer2_blockref_t *bref, size_t *media_bytes)
{
	hammer2_media_data_t *media;
	hammer2_off_t io_off, io_base;
	size_t bytes, io_bytes, boff, fbytes;

	bytes = (bref->data_off & HAMMER2_OFF_MASK_RADIX);
	if (bytes)
		bytes = (size_t)1 << bytes;
	*media_bytes = bytes;

	if (!bytes) {
		warnx("hammer2: blockref has no data");
		return NULL;
	}

	io_off = bref->data_off & ~HAMMER2_OFF_MASK_RADIX;
	io_base = io_off & ~(hammer2_off_t)(HAMMER2_LBUFSIZE - 1);
	boff = (size_t)((hammer2_off_t)io_off - io_base);

	io_bytes = HAMMER2_LBUFSIZE;
	while (io_bytes + boff < bytes)
		io_bytes <<= 1;

	if (io_bytes > sizeof(hammer2_media_data_t)) {
		warnx("hammer2: invalid I/O bytes");
		return NULL;
	}

	/*
	 * XXX fp is currently always root volume, so read fails if io_base is
	 * beyond root volume limit. Fail with a message before read_buf() then.
	 */
	fbytes = (size_t)get_file_size(fp);
	if ((ssize_t)fbytes == -1) {
		warnx("hammer2: failed to get media size");
		return NULL;
	}
	if (io_base >= fbytes) {
		warnx("hammer2: XXX read beyond HAMMER2 root volume limit unsupported");
		return NULL;
	}

	if (fseeko(fp, (off_t)io_base, SEEK_SET) == -1) {
		warnx("hammer2: failed to seek media");
		return NULL;
	}
	media = read_buf(fp, (off_t)io_base, io_bytes);
	if (media == NULL) {
		warnx("hammer2: failed to read media");
		return NULL;
	}
	if (boff)
		memcpy(media, (char *)media + boff, bytes);

	return media;
}

static int
find_pfs(FILE *fp, const hammer2_blockref_t *bref, const char *pfs, bool *res)
{
	hammer2_media_data_t *media;
	hammer2_inode_data_t ipdata;
	hammer2_blockref_t *bscan;
	size_t bytes;
	int i, bcount;

	media = read_media(fp, bref, &bytes);
	if (media == NULL)
		return -1;

	switch (bref->type) {
	case HAMMER2_BREF_TYPE_INODE:
		ipdata = media->ipdata;
		if (ipdata.meta.pfs_type == HAMMER2_PFSTYPE_SUPROOT) {
			bscan = &ipdata.u.blockset.blockref[0];
			bcount = HAMMER2_SET_COUNT;
		} else {
			bscan = NULL;
			bcount = 0;
			if (ipdata.meta.op_flags & HAMMER2_OPFLAG_PFSROOT) {
				if (memchr(ipdata.filename, 0,
				    sizeof(ipdata.filename))) {
					if (!strcmp(
					    (const char*)ipdata.filename, pfs))
						*res = true;
				} else {
					if (strlen(pfs) > 0 &&
					    !memcmp(ipdata.filename, pfs,
					    strlen(pfs)))
						*res = true;
				}
			} else {
				free(media);
				return -1;
			}
		}
		break;
	case HAMMER2_BREF_TYPE_INDIRECT:
		bscan = &media->npdata[0];
		bcount = (int)(bytes / sizeof(hammer2_blockref_t));
		break;
	default:
		bscan = NULL;
		bcount = 0;
		break;
	}

	for (i = 0; i < bcount; ++i) {
		if (bscan[i].type != HAMMER2_BREF_TYPE_EMPTY) {
			if (find_pfs(fp, &bscan[i], pfs, res) == -1) {
				free(media);
				return -1;
			}
		}
	}
	free(media);

	return 0;
}

static char*
extract_device_name(const char *devpath)
{
	char *p, *head;

	if (!devpath)
		return NULL;

	p = strdup(devpath);
	head = p;

	p = strchr(p, '@');
	if (p)
		*p = 0;

	p = strrchr(head, '/');
	if (p) {
		p++;
		if (*p == 0) {
			free(head);
			return NULL;
		}
		p = strdup(p);
		free(head);
		return p;
	}

	return head;
}

static int
read_label(FILE *fp, char *label, size_t size, const char *devpath)
{
	hammer2_blockref_t broot, best, *bref;
	hammer2_media_data_t *vols[HAMMER2_NUM_VOLHDRS], *media;
	size_t bytes;
	bool res = false;
	int i, best_i, error = 1;
	const char *pfs;
	char *devname;

	best_i = -1;
	memset(vols, 0, sizeof(vols));
	memset(&best, 0, sizeof(best));

	for (i = 0; i < HAMMER2_NUM_VOLHDRS; i++) {
		if ((hammer2_off_t)i * (hammer2_off_t)HAMMER2_ZONE_BYTES64 >= (hammer2_off_t)get_file_size(fp))
			break;
		memset(&broot, 0, sizeof(broot));
		broot.type = HAMMER2_BREF_TYPE_VOLUME;
		broot.data_off = ((hammer2_off_t)i * (hammer2_off_t)HAMMER2_ZONE_BYTES64) | HAMMER2_PBUFRADIX;
		vols[i] = (void*)read_voldata(fp, i);
		if (vols[i] == NULL) {
			warnx("hammer2: failed to read volume data");
			goto fail;
		}
		broot.mirror_tid = vols[i]->voldata.mirror_tid;
		if (best_i < 0 || best.mirror_tid < broot.mirror_tid) {
			best_i = i;
			best = broot;
		}
	}

	bref = &vols[best_i]->voldata.sroot_blockset.blockref[0];
	if (bref->type != HAMMER2_BREF_TYPE_INODE) {
		/* Don't print error as devpath could be non-root volume. */
		goto fail;
	}

	media = read_media(fp, bref, &bytes);
	if (media == NULL) {
		goto fail;
	}

	/*
	 * fstyp_function in DragonFly takes an additional devpath argument
	 * which doesn't exist in FreeBSD and NetBSD.
	 */
#ifdef HAS_DEVPATH
	pfs = strchr(devpath, '@');
	if (!pfs) {
		assert(strlen(devpath));
		switch (devpath[strlen(devpath) - 1]) {
		case 'a':
			pfs = "BOOT";
			break;
		case 'd':
			pfs = "ROOT";
			break;
		default:
			pfs = "DATA";
			break;
		}
	} else
		pfs++;

	if (strlen(pfs) > HAMMER2_INODE_MAXNAME) {
		goto fail;
	}
	devname = extract_device_name(devpath);
#else
	pfs = "";
	devname = extract_device_name(NULL);
	assert(!devname);
#endif

	/* Add device name to help support multiple autofs -media mounts. */
	if (find_pfs(fp, bref, pfs, &res) == 0 && res) {
		if (devname)
			snprintf(label, size, "%s_%s", pfs, devname);
		else
			strlcpy(label, pfs, size);
	} else {
		memset(label, 0, size);
		memcpy(label, media->ipdata.filename,
		    sizeof(media->ipdata.filename));
		if (devname) {
			strlcat(label, "_", size);
			strlcat(label, devname, size);
		}
	}
	if (devname)
		free(devname);
	free(media);
	error = 0;
fail:
	for (i = 0; i < HAMMER2_NUM_VOLHDRS; i++)
		free(vols[i]);

	return error;
}

int
fstyp_hammer2(FILE *fp, char *label, size_t size)
{
	hammer2_volume_data_t *voldata = read_voldata(fp, 0);
	int error = 1;

	if (voldata == NULL)
		goto fail;
	if (voldata->volu_id != HAMMER2_ROOT_VOLUME)
		goto fail;
	if (voldata->nvolumes != 0)
		goto fail;
	if (test_voldata(fp))
		goto fail;

	error = read_label(fp, label, size, NULL);
fail:
	free(voldata);
	return error;
}

static int
__fsvtyp_hammer2(const char *blkdevs, char *label, size_t size, int partial)
{
	hammer2_volume_data_t *voldata = NULL;
	FILE *fp = NULL;
	char *dup = NULL, *target_label = NULL, *p, *volpath, *rootvolpath;
	char x[HAMMER2_MAX_VOLUMES];
	int i, volid, error = 1;

	if (!blkdevs)
		goto fail;

	memset(x, 0, sizeof(x));
	p = dup = strdup(blkdevs);
	if ((p = strchr(p, '@')) != NULL) {
		*p++ = '\0';
		target_label = p;
	}
	p = dup;

	volpath = NULL;
	rootvolpath = NULL;
	volid = -1;
	while (p) {
		volpath = p;
		if ((p = strchr(p, ':')) != NULL)
			*p++ = '\0';
		if ((fp = fopen(volpath, "r")) == NULL) {
			warnx("hammer2: failed to open %s", volpath);
			goto fail;
		}
		if (test_voldata(fp))
			break;
		voldata = read_voldata(fp, 0);
		fclose(fp);
		if (voldata == NULL) {
			warnx("hammer2: failed to read volume data");
			goto fail;
		}
		volid = voldata->volu_id;
		free(voldata);
		voldata = NULL;
		if (volid < 0 || volid >= HAMMER2_MAX_VOLUMES)
			goto fail;
		x[volid]++;
		if (volid == HAMMER2_ROOT_VOLUME)
			rootvolpath = volpath;
	}

	/* If no rootvolpath, proceed only if partial mode with volpath. */
	if (rootvolpath)
		volpath = rootvolpath;
	else if (!partial || !volpath)
		goto fail;
	if ((fp = fopen(volpath, "r")) == NULL) {
		warnx("hammer2: failed to open %s", volpath);
		goto fail;
	}
	voldata = read_voldata(fp, 0);
	if (voldata == NULL) {
		warnx("hammer2: failed to read volume data");
		goto fail;
	}

	if (volid == -1)
		goto fail;
	if (partial)
		goto success;

	for (i = 0; i < HAMMER2_MAX_VOLUMES; i++)
		if (x[i] > 1)
			goto fail;
	for (i = 0; i < HAMMER2_MAX_VOLUMES; i++)
		if (x[i] == 0)
			break;
	if (voldata->nvolumes != i)
		goto fail;
	for (; i < HAMMER2_MAX_VOLUMES; i++)
		if (x[i] != 0)
			goto fail;
success:
	/* Reconstruct @label format path using only root volume. */
	if (target_label) {
		size_t siz = strlen(volpath) + strlen(target_label) + 2;
		p = calloc(1, siz);
		snprintf(p, siz, "%s@%s", volpath, target_label);
		volpath = p;
	}
	error = read_label(fp, label, size, volpath);
	if (target_label)
		free(p);
	/* If in partial mode, read label but ignore error. */
	if (partial)
		error = 0;
fail:
	if (fp)
		fclose(fp);
	free(voldata);
	free(dup);
	return error;
}

int
fsvtyp_hammer2(const char *blkdevs, char *label, size_t size)
{
	return __fsvtyp_hammer2(blkdevs, label, size, 0);
}

int
fsvtyp_hammer2_partial(const char *blkdevs, char *label, size_t size)
{
	return __fsvtyp_hammer2(blkdevs, label, size, 1);
}
