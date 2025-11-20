/* $NetBSD: sdmmc.c,v 1.1.4.2 2025/11/20 19:17:40 martin Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <sys/bootblock.h>

#define FSTYPENAMES
#include <sys/disklabel.h>

#include "sdmmc.h"
#include "miniipc.h"
#include "gpio.h"

#if LABELSECTOR != 1
#error bad LABELSECTOR
#endif

#define DEFAULT_DEVICE	"ld0"
#define DEFAULT_PART	0

#define	SDMMC_STATE_NEW_CARD		2

static uint8_t sdmmc_buf[SDMMC_BLOCK_SIZE];
static struct disklabel sdmmc_disklabel;

static void
sdmmc_print_label(void)
{
#ifdef SDMMC_DEBUG
	struct disklabel *d = &sdmmc_disklabel;
	int part;

	for (part = 0; part < le16toh(d->d_npartitions); part++) {
		struct partition *p = &d->d_partitions[part];

		printf("partition %s%c ", DEFAULT_DEVICE, part + 'a');

		if (p->p_fstype < __arraycount(fstypenames)) {
			printf("(%s)", fstypenames[p->p_fstype]);
		} else {
			printf("(type %u)", p->p_fstype);
		}

		printf(" - %u %u\n", le32toh(p->p_offset), le32toh(p->p_size));
	}
#endif
}

static int
sdmmc_read_label(daddr_t dblk, size_t size)
{
	struct disklabel d;
	int error;

	error = miniipc_sdmmc_read(dblk + LABELSECTOR, 1, sdmmc_buf);
	if (error != 0) {
		printf("SDMMC: failed to read disklabel: %d\n", error);
		return error;
	}
	memcpy(&d, sdmmc_buf, sizeof(d));

	if (le32toh(d.d_magic) != DISKMAGIC ||
	    le32toh(d.d_magic2) != DISKMAGIC) {
#ifdef SDMMC_DEBUG
		printf("SDMMC: bad diskmagic 0x%x 0x%x\n", le32toh(d.d_magic),
		    le32toh(d.d_magic2));
#endif
		return EINVAL;
	}
	if (le16toh(d.d_npartitions) > MAXPARTITIONS) {
		printf("SDMMC: bad npartitions %u\n", le16toh(d.d_npartitions));
		return EINVAL;
	}

	memcpy(&sdmmc_disklabel, sdmmc_buf, sizeof(sdmmc_disklabel));
	return 0;
}

int
sdmmc_init(void)
{
	struct mbr_sector mbr;
	struct mbr_partition *mbr_part;
	int error, n;
	bool has_label = false;
	uint32_t state, ack;

	miniipc_sdmmc_state(&state);
	if (state == SDMMC_STATE_NEW_CARD) {
		miniipc_sdmmc_ack(&ack);
	}

	error = miniipc_sdmmc_read(0, 1, sdmmc_buf);
	if (error != 0) {
		printf("SDMMC: failed to read MBR: %d\n", error);
		return error;
	}
	memcpy(&mbr, sdmmc_buf, sizeof(mbr));
	if (le16toh(mbr.mbr_magic) != MBR_MAGIC) {
		printf("SDMMC: bad MBR magic: 0x%x\n", le32toh(mbr.mbr_magic));
		return ENXIO;
	}

	for (n = 0; n < MBR_PART_COUNT; n++) {
		uint32_t start, size;

		mbr_part = &mbr.mbr_parts[n];
		size = le32toh(mbr_part->mbrp_size);
		if (le32toh(mbr_part->mbrp_size) == 0) {
			continue;
		}
		start = le32toh(mbr_part->mbrp_start);
#ifdef SDMMC_DEBUG
		printf("MBR part %u type 0x%x start %u size %u\n",
		    n, mbr_part->mbrp_type, start, size);
#endif

		if (mbr_part->mbrp_type == MBR_PTYPE_NETBSD && !has_label) {
			error = sdmmc_read_label(start, size);
			if (error != 0) {
				struct partition *p =
				    &sdmmc_disklabel.d_partitions[0];

				/* No label on disk, fake one. */
				p->p_fstype = FS_BSDFFS;
				p->p_size = htole32(size);
				p->p_offset = htole32(start);
				sdmmc_disklabel.d_npartitions = htole16(1);
			}
			has_label = true;
		}
	}

	sdmmc_print_label();

	return 0;
}

static int
sdmmc_parse(const char *fname, int *part, char **pfile)
{
	const char *full_path;
	char pathbuf[PATH_MAX];

	if (strchr(fname, ':') == NULL) {
		snprintf(pathbuf, sizeof(pathbuf), "%s%c:%s",
		    DEFAULT_DEVICE, DEFAULT_PART + 'a', fname);
		full_path = pathbuf;
		*pfile = __UNCONST(fname);
	} else {
		full_path = fname;
		*pfile = strchr(fname, ':') + 1;
	}
	if (*pfile[0] == '\0') {
		*pfile = __UNCONST("/");
	}

	if (strncmp(full_path, DEFAULT_DEVICE, 3) != 0) {
		return EINVAL;
	}
	if (full_path[3] < 'a' || full_path[3] >= 'a' + MAXPARTITIONS ||
	    full_path[4] != ':') {
		return EINVAL;
	}
	*part = full_path[3] - 'a';

	return 0;
}

int
sdmmc_open(struct open_file *f, ...)
{
	int error, n, part;
	const char *fname;
	va_list ap;
	char **file;
	char *path;

	va_start(ap, f);
	fname = va_arg(ap, const char *);
	file = va_arg(ap, char **);
	va_end(ap);

	error = sdmmc_parse(fname, &part, &path);
	if (error != 0) {
		return error;
	}

	for (n = 0; n < ndevs; n++) {
		if (strcmp(DEV_NAME(&devsw[n]), "sdmmc") == 0) {
			f->f_dev = &devsw[n];
			break;
		}
	}
	if (n == ndevs) {
		return ENXIO;
	}

	f->f_devdata = &sdmmc_disklabel.d_partitions[part];
	*file = path;

	return 0;
}

int
sdmmc_close(struct open_file *f)
{
	return 0;
}

int
sdmmc_strategy(void *devdata, int rw, daddr_t dblk, size_t size, void *buf,
    size_t *rsize)
{
	struct partition *p = devdata;
	int error;

	if (rw != F_READ) {
		return EROFS;
	}
	if ((size % SDMMC_BLOCK_SIZE) != 0) {
		printf("I/O must be multiple of SDMMC_BLOCK_SIZE\n");
		return EIO;
	}

	gpio_clear(GPIO_SLOT_LED);
	error = miniipc_sdmmc_read(le32toh(p->p_offset) + dblk,
	    size / SDMMC_BLOCK_SIZE, buf);
	gpio_set(GPIO_SLOT_LED);
	if (error == 0) {
		*rsize = size;
	}

	return error;
}
