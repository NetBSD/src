/*        $NetBSD: hammer.c,v 1.4 2021/01/10 12:38:40 tkusumi Exp $      */

/*-
 * Copyright (c) 2016-2019 The DragonFly Project
 * Copyright (c) 2016-2019 Tomohiro Kusumi <tkusumi@netbsd.org>
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
__KERNEL_RCSID(0, "$NetBSD: hammer.c,v 1.4 2021/01/10 12:38:40 tkusumi Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <uuid.h>

#include "fstyp.h"
#include "hammer_disk.h"

static hammer_volume_ondisk_t
read_ondisk(FILE *fp)
{
	return (read_buf(fp, 0, sizeof(struct hammer_volume_ondisk)));
}

static int
test_ondisk(const hammer_volume_ondisk_t ondisk)
{
	static int count = 0;
	static hammer_uuid_t fsid, fstype;
	static char label[64];

	if (ondisk->vol_signature != HAMMER_FSBUF_VOLUME &&
	    ondisk->vol_signature != HAMMER_FSBUF_VOLUME_REV)
		return (1);
	if (ondisk->vol_rootvol != HAMMER_ROOT_VOLNO)
		return (1);
	if (ondisk->vol_no < 0 || ondisk->vol_no > HAMMER_MAX_VOLUMES - 1)
		return (1);
	if (ondisk->vol_count < 1 || ondisk->vol_count > HAMMER_MAX_VOLUMES)
		return (1);

	if (count == 0) {
		count = ondisk->vol_count;
		if (count == 0)
			return (1);
		memcpy(&fsid, &ondisk->vol_fsid, sizeof(fsid));
		memcpy(&fstype, &ondisk->vol_fstype, sizeof(fstype));
		strlcpy(label, ondisk->vol_label, sizeof(label));
	} else {
		if (ondisk->vol_count != count)
			return (1);
		if (!uuid_equal(&ondisk->vol_fsid, &fsid, NULL))
			return (1);
		if (!uuid_equal(&ondisk->vol_fstype, &fstype, NULL))
			return (1);
		if (strcmp(ondisk->vol_label, label))
			return (1);
	}

	return (0);
}

static const char*
extract_device_name(const char *devpath)
{
	const char *p;

	p = strrchr(devpath, '/');
	if (p) {
		p++;
		if (*p == 0)
			p = NULL;
	} else {
		p = devpath;
	}
	return (p);
}

int
fstyp_hammer(FILE *fp, char *label, size_t size)
{
	hammer_volume_ondisk_t ondisk;
	int error = 1;
#ifdef HAS_DEVPATH
	const char *p;
#endif
	ondisk = read_ondisk(fp);
	if (!ondisk)
		goto fail;
	if (ondisk->vol_no != HAMMER_ROOT_VOLNO)
		goto fail;
	if (ondisk->vol_count != 1)
		goto fail;
	if (test_ondisk(ondisk))
		goto fail;

	/*
	 * fstyp_function in DragonFly takes an additional devpath argument
	 * which doesn't exist in FreeBSD and NetBSD.
	 */
#ifdef HAS_DEVPATH
	/* Add device name to help support multiple autofs -media mounts. */
	p = extract_device_name(devpath);
	if (p)
		snprintf(label, size, "%s_%s", ondisk->vol_label, p);
	else
		strlcpy(label, ondisk->vol_label, size);
#else
	strlcpy(label, ondisk->vol_label, size);
#endif
	error = 0;
fail:
	free(ondisk);
	return (error);
}

static int
test_volume(const char *volpath)
{
	hammer_volume_ondisk_t ondisk = NULL;
	FILE *fp;
	int volno = -1;

	if ((fp = fopen(volpath, "r")) == NULL)
		goto fail;

	ondisk = read_ondisk(fp);
	if (!ondisk)
		goto fail;
	if (test_ondisk(ondisk))
		goto fail;

	volno = ondisk->vol_no;
fail:
	if (fp)
		fclose(fp);
	free(ondisk);
	return (volno);
}

static int
__fsvtyp_hammer(const char *blkdevs, char *label, size_t size, int partial)
{
	hammer_volume_ondisk_t ondisk = NULL;
	FILE *fp = NULL;
	char *dup = NULL, *p, *volpath, *rootvolpath, x[HAMMER_MAX_VOLUMES];
	int i, volno, error = 1;

	if (!blkdevs)
		goto fail;

	memset(x, 0, sizeof(x));
	dup = strdup(blkdevs);
	p = dup;

	volpath = NULL;
	rootvolpath = NULL;
	volno = -1;
	while (p) {
		volpath = p;
		if ((p = strchr(p, ':')) != NULL)
			*p++ = '\0';
		if ((volno = test_volume(volpath)) == -1)
			break;
		if (volno < 0 || volno >= HAMMER_MAX_VOLUMES)
			goto fail;
		x[volno]++;
		if (volno == HAMMER_ROOT_VOLNO)
			rootvolpath = volpath;
	}

	/* If no rootvolpath, proceed only if partial mode with volpath. */
	if (rootvolpath)
		volpath = rootvolpath;
	else if (!partial || !volpath)
		goto fail;
	if ((fp = fopen(volpath, "r")) == NULL)
		goto fail;
	ondisk = read_ondisk(fp);
	if (!ondisk)
		goto fail;

	if (volno == -1)
		goto fail;
	if (partial)
		goto success;

	for (i = 0; i < HAMMER_MAX_VOLUMES; i++)
		if (x[i] > 1)
			goto fail;
	for (i = 0; i < HAMMER_MAX_VOLUMES; i++)
		if (x[i] == 0)
			break;
	if (ondisk->vol_count != i)
		goto fail;
	for (; i < HAMMER_MAX_VOLUMES; i++)
		if (x[i] != 0)
			goto fail;
success:
	/* Add device name to help support multiple autofs -media mounts. */
	p = __UNCONST(extract_device_name(volpath));
	if (p)
		snprintf(label, size, "%s_%s", ondisk->vol_label, p);
	else
		strlcpy(label, ondisk->vol_label, size);
	error = 0;
fail:
	if (fp)
		fclose(fp);
	free(ondisk);
	free(dup);
	return (error);
}

int
fsvtyp_hammer(const char *blkdevs, char *label, size_t size)
{
	return (__fsvtyp_hammer(blkdevs, label, size, 0));
}

int
fsvtyp_hammer_partial(const char *blkdevs, char *label, size_t size)
{
	return (__fsvtyp_hammer(blkdevs, label, size, 1));
}
