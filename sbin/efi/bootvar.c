/* $NetBSD: bootvar.c,v 1.3 2025/03/02 01:07:11 riastradh Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: bootvar.c,v 1.3 2025/03/02 01:07:11 riastradh Exp $");
#endif /* not lint */

#include <sys/queue.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <uuid.h>

#include "defs.h"
#include "efiio.h"
#include "bootvar.h"
#include "devpath.h"
#include "gptsubr.h"
#include "map.h"
#include "utils.h"

typedef SIMPLEQ_HEAD(boothead, boot_blk) boothead_t;

typedef struct boot_blk {
	size_t size;
	union {
		uint8_t    *bp;
		char       *cp;
		void       *vp;
		boot_var_t *body;	/* first element */
		devpath_t  *path;	/* remaining elements */
	} u;
	SIMPLEQ_ENTRY(boot_blk) entry;
} boot_blk_t;

static inline boot_blk_t *
new_blk(uint8_t type, uint8_t subtype, uint16_t length)
{
	boot_blk_t *bb;

	bb = ecalloc(sizeof(*bb), 1);

	if (length == 0)	/* alloc bb only */
		return bb;

	bb->u.vp = ecalloc(length, 1);
	bb->size = length;

	if (type == 0)		/* non-devpath */
		return bb;

	bb->u.path->Type = type;
	bb->u.path->SubType = subtype;
	bb->u.path->Length = length;

	return bb;
}

static void *
collapse_list(boothead_t *head, size_t datasize)
{
	boot_blk_t *bb;
	void *data;
	char *cp;

	data = ecalloc(datasize, 1);
	cp = data;
	SIMPLEQ_FOREACH(bb, head, entry) {
		memcpy(cp, bb->u.vp, bb->size);
		cp += bb->size;
	}
	return data;
}

static boot_blk_t *
create_bootbody(const char *label, uint32_t attrib)
{
	boot_blk_t *bb;
	size_t body_size, desc_size, size;

	desc_size = utf8_to_ucs2_size(label);
	body_size = sizeof(*bb->u.body) + desc_size;

	bb = new_blk(0, 0, (uint16_t)body_size);

	bb->u.body->Attributes = attrib;

	size = desc_size;
	utf8_to_ucs2(label, strlen(label) + 1, bb->u.body->Description, &size);
	assert(size == desc_size);

	return bb;
}

static boot_blk_t *
create_devpath_media_hd(const char *dev, uint partnum)
{
	struct {
		devpath_t	hdr;	/* Length 42 */
		uint32_t	PartitionNumber;
		uint64_t	PartitionStart;
		uint64_t	PartitionSize;
		struct uuid	PartitionSignature;
		uint8_t		PartitionFormat;
#define PARTITION_FORMAT_MBR	0x01
#define PARTITION_FORMAT_GPT	0x02

		uint8_t		SignatureType;
#define SIGNATURE_TYPE_NONE	0x00
#define SIGNATURE_TYPE_MBR	0x01
#define SIGNATURE_TYPE_GUID	0x02
	} __packed *pp;
	assert(sizeof(*pp) == 42);
	boot_blk_t *bb;
	struct gpt_ent *ent;
	map_t m;

	/* Get GPT info for device and partition */
	m = find_gpt_map(dev, partnum);
	if (m == NULL)
		errx(EXIT_FAILURE, "cannot find partition number %u on %s\n",
		    partnum, dev);

	ent = m->map_data;
	if (m->map_type != MAP_TYPE_GPT_PART)
		errx(EXIT_FAILURE, "not a MAP_TYPE_GPT_PART: %u\n",
		    m->map_type);

	/* Check that this is an EFI partition? */
	if (memcmp(ent->ent_type, (void *)&(uuid_t)GPT_ENT_TYPE_EFI,
		sizeof(ent->ent_type)) != 0)
		errx(EXIT_FAILURE, "not an EFI partition");

	bb = new_blk(DEVPATH_TYPE_MEDIA, 1, sizeof(*pp));

	pp = bb->u.vp;
	pp->PartitionNumber = m->map_index;
	pp->PartitionStart  = (uint64_t)m->map_start;
	pp->PartitionSize   = (uint64_t)m->map_size;
	memcpy(&pp->PartitionSignature, ent->ent_guid,
	    sizeof(pp->PartitionSignature));
	pp->PartitionFormat = PARTITION_FORMAT_GPT;
	pp->SignatureType   = SIGNATURE_TYPE_GUID;

	return bb;
}

static boot_blk_t *
create_devpath_media_pathname(const char *loader)
{
	struct {
		devpath_t	hdr;
		uint16_t	PathName[];
	} __packed *pn;
	size_t len, path_len;
	boot_blk_t *bb;

	path_len = utf8_to_ucs2_size(loader);
	len = sizeof(pn->hdr) + path_len;

	bb = new_blk(DEVPATH_TYPE_MEDIA, 4, (uint16_t)len);

	pn = bb->u.vp;
	(void)utf8_to_ucs2(loader, strlen(loader) + 1, pn->PathName,
	    &path_len);

	return bb;
}

static boot_blk_t *
create_devpath_end(void)
{
	boot_blk_t *bb;

	bb = new_blk(DEVPATH_TYPE_END, 0xff, sizeof(*bb->u.path));

	return bb;
}

static boot_blk_t *
create_optdata(const char *fname)
{
	boot_blk_t *bb;

	bb = new_blk(0, 0, 0);
	bb->u.vp = read_file(fname, &bb->size);
	return bb;
}

PUBLIC void *
make_bootvar_data(const char *dev, uint partnum, uint32_t attrib,
    const char *label, const char *loader, const char *fname,
    size_t *datasize)
{
	boothead_t head = SIMPLEQ_HEAD_INITIALIZER(head);
	boot_blk_t *bb;
	size_t FilePathListLength, OptDataLength;

	bb = create_bootbody(label, attrib);
	SIMPLEQ_INSERT_TAIL(&head, bb, entry);
	FilePathListLength = 0;

	bb = create_devpath_media_hd(dev, partnum);
	SIMPLEQ_INSERT_TAIL(&head, bb, entry);
	FilePathListLength += bb->size;

	bb = create_devpath_media_pathname(loader);
	SIMPLEQ_INSERT_TAIL(&head, bb, entry);
	FilePathListLength += bb->size;

	bb = create_devpath_end();
	SIMPLEQ_INSERT_TAIL(&head, bb, entry);
	FilePathListLength += bb->size;

	if (fname == NULL) {
		OptDataLength = 0;
	}
	else {
		bb = create_optdata(fname);
		SIMPLEQ_INSERT_TAIL(&head, bb, entry);
		OptDataLength = bb->size;
	}

	bb = SIMPLEQ_FIRST(&head);
	bb->u.body->FilePathListLength = (uint16_t)FilePathListLength;

	*datasize = bb->size + FilePathListLength + OptDataLength;
	return collapse_list(&head, *datasize);
}

PUBLIC int
find_new_bootvar(efi_var_t **var_array, size_t var_cnt, const char *target)
{
	int idx, lastidx;
	int rstatus;
	size_t n;

	assert(target != NULL);

	n = strlen(target);
	lastidx = -1;
	for (size_t i = 0; i < var_cnt; i++) {
		idx = (int)strtou(var_array[i]->name + n, NULL, 16, 0, 0xffff,
		    &rstatus);
		if (rstatus != 0)
			err(EXIT_FAILURE, "strtou: %s", var_array[i]->name);

//		printf("idx: %x\n", idx);
		assert(idx > lastidx);
		if (idx != lastidx + 1) {
			break;
		}
		lastidx = idx;
	}
	return lastidx + 1;
}
