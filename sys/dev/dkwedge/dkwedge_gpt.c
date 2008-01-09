/*	$NetBSD: dkwedge_gpt.c,v 1.6.16.1 2008/01/09 01:52:32 matt Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * EFI GUID Partition Table support for disk wedges
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dkwedge_gpt.c,v 1.6.16.1 2008/01/09 01:52:32 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/disk.h>
#include <sys/vnode.h>
#include <sys/malloc.h>

#include <sys/disklabel_gpt.h>
#include <sys/uuid.h>

static const struct {
	struct uuid ptype_guid;
	const char *ptype_str;
} gpt_ptype_guid_to_str_tab[] = {
	{ GPT_ENT_TYPE_EFI,		"msdos" },	/* XXX yes? */
#if 0
	{ GPT_ENT_TYPE_FREEBSD,		??? },
#endif
	{ GPT_ENT_TYPE_NETBSD_SWAP,		DKW_PTYPE_SWAP },
	{ GPT_ENT_TYPE_FREEBSD_SWAP,		DKW_PTYPE_SWAP },
	{ GPT_ENT_TYPE_NETBSD_FFS,		DKW_PTYPE_FFS },
	{ GPT_ENT_TYPE_FREEBSD_UFS,		DKW_PTYPE_FFS },
	{ GPT_ENT_TYPE_NETBSD_LFS,		DKW_PTYPE_LFS },
	{ GPT_ENT_TYPE_NETBSD_RAIDFRAME,	DKW_PTYPE_RAIDFRAME },
	{ GPT_ENT_TYPE_NETBSD_CCD,		DKW_PTYPE_CCD },
	{ GPT_ENT_TYPE_NETBSD_CGD,		DKW_PTYPE_CGD },

	/* XXX What about the MS and Linux types? */

	{ { .time_low = 0 },		NULL },
};

static const char *
gpt_ptype_guid_to_str(const struct uuid *guid)
{
	int i;

	for (i = 0; gpt_ptype_guid_to_str_tab[i].ptype_str != NULL; i++) {
		if (memcmp(&gpt_ptype_guid_to_str_tab[i].ptype_guid,
			   guid, sizeof(*guid)) == 0)
			return (gpt_ptype_guid_to_str_tab[i].ptype_str);
	}

	return (NULL);
}

static const uint32_t gpt_crc_tab[16] = {
	0x00000000U, 0x1db71064U, 0x3b6e20c8U, 0x26d930acU,
	0x76dc4190U, 0x6b6b51f4U, 0x4db26158U, 0x5005713cU,
	0xedb88320U, 0xf00f9344U, 0xd6d6a3e8U, 0xcb61b38cU,
	0x9b64c2b0U, 0x86d3d2d4U, 0xa00ae278U, 0xbdbdf21cU
};

static uint32_t
gpt_crc32(const void *vbuf, size_t len)
{
	const uint8_t *buf = vbuf;
	uint32_t crc;

	crc = 0xffffffffU;
	while (len--) {
		crc ^= *buf++;
		crc = (crc >> 4) ^ gpt_crc_tab[crc & 0xf];
		crc = (crc >> 4) ^ gpt_crc_tab[crc & 0xf];
	}

	return (crc ^ 0xffffffffU);
}

static int
gpt_verify_header_crc(struct gpt_hdr *hdr)
{
	uint32_t crc;
	int rv;

	crc = hdr->hdr_crc_self;
	hdr->hdr_crc_self = 0;
	rv = le32toh(crc) == gpt_crc32(hdr, le32toh(hdr->hdr_size));
	hdr->hdr_crc_self = crc;

	return (rv);
}

static int
dkwedge_discover_gpt(struct disk *pdk, struct vnode *vp)
{
	static const struct uuid ent_type_unused = GPT_ENT_TYPE_UNUSED;
	static const char gpt_hdr_sig[] = GPT_HDR_SIG;
	struct dkwedge_info dkw;
	void *buf;
	struct gpt_hdr *hdr;
	struct gpt_ent *ent;
	uint32_t entries, entsz;
	daddr_t lba_start, lba_end, lba_table;
	uint32_t gpe_crc;
	int error;
	u_int i;

	buf = malloc(DEV_BSIZE, M_DEVBUF, M_WAITOK);

	/*
	 * Note: We don't bother with a Legacy or Protective MBR
	 * here.  If a GPT is found, then the search stops, and
	 * the GPT is authoritative.
	 */

	/* Read in the GPT Header. */
	error = dkwedge_read(pdk, vp, GPT_HDR_BLKNO, buf, DEV_BSIZE);
	if (error)
		goto out;
	hdr = buf;

	/* Validate it. */
	if (memcmp(gpt_hdr_sig, hdr->hdr_sig, sizeof(hdr->hdr_sig)) != 0) {
		/* XXX Should check at end-of-disk. */
		error = ESRCH;
		goto out;
	}
	if (hdr->hdr_revision != htole32(GPT_HDR_REVISION)) {
		/* XXX Should check at end-of-disk. */
		error = ESRCH;
		goto out;
	}
	if (le32toh(hdr->hdr_size) > DEV_BSIZE) {
		/* XXX Should check at end-of-disk. */
		error = ESRCH;
		goto out;
	}
	if (gpt_verify_header_crc(hdr) == 0) {
		/* XXX Should check at end-of-disk. */
		error = ESRCH;
		goto out;
	}

	/* XXX Now that we found it, should we validate the backup? */

	{
		struct uuid disk_guid;
		char guid_str[UUID_STR_LEN];
		uuid_dec_le(hdr->hdr_guid, &disk_guid);
		uuid_snprintf(guid_str, sizeof(guid_str), &disk_guid);
		aprint_verbose("%s: GPT GUID: %s\n", pdk->dk_name, guid_str);
	}

	entries = le32toh(hdr->hdr_entries);
	entsz = roundup(le32toh(hdr->hdr_entsz), 8);
	if (entsz > roundup(sizeof(struct gpt_ent), 8)) {
		aprint_error("%s: bogus GPT entry size: %u\n",
		    pdk->dk_name, le32toh(hdr->hdr_entsz));
		error = EINVAL;
		goto out;
	}
	gpe_crc = le32toh(hdr->hdr_crc_table);

	/* XXX Clamp entries at 128 for now. */
	if (entries > 128) {
		aprint_error("%s: WARNING: clamping number of GPT entries to "
		    "128 (was %u)\n", pdk->dk_name, entries);
		entries = 128;
	}

	lba_start = le64toh(hdr->hdr_lba_start);
	lba_end = le64toh(hdr->hdr_lba_end);
	lba_table = le64toh(hdr->hdr_lba_table);
	if (lba_start < 0 || lba_end < 0 || lba_table < 0) {
		aprint_error("%s: GPT block numbers out of range\n",
		    pdk->dk_name);
		error = EINVAL;
		goto out;
	}

	free(buf, M_DEVBUF);
	buf = malloc(roundup(entries * entsz, DEV_BSIZE), M_DEVBUF, M_WAITOK);
	error = dkwedge_read(pdk, vp, lba_table, buf,
			     roundup(entries * entsz, DEV_BSIZE));
	if (error) {
		/* XXX Should check alternate location. */
		aprint_error("%s: unable to read GPT partition array, "
		    "error = %d\n", pdk->dk_name, error);
		goto out;
	}

	if (gpt_crc32(buf, entries * entsz) != gpe_crc) {
		/* XXX Should check alternate location. */
		aprint_error("%s: bad GPT partition array CRC\n",
		    pdk->dk_name);
		error = EINVAL;
		goto out;
	}

	/*
	 * Walk the partitions, adding a wedge for each type we know about.
	 */
	for (i = 0; i < entries; i++) {
		struct uuid ptype_guid, ent_guid;
		const char *ptype;
		int j;
		char ptype_guid_str[UUID_STR_LEN], ent_guid_str[UUID_STR_LEN];

		ent = (struct gpt_ent *)((char *)buf + (i * entsz));

		uuid_dec_le(ent->ent_type, &ptype_guid);
		if (memcmp(&ptype_guid, &ent_type_unused,
			   sizeof(ptype_guid)) == 0)
			continue;

		uuid_dec_le(ent->ent_guid, &ent_guid);

		uuid_snprintf(ptype_guid_str, sizeof(ptype_guid_str),
		    &ptype_guid);
		uuid_snprintf(ent_guid_str, sizeof(ent_guid_str),
		    &ent_guid);

		/* Skip it if we don't grok this ptype. */
		if ((ptype = gpt_ptype_guid_to_str(&ptype_guid)) == NULL) {
			/*
			 * XXX Should probably just add these... maybe
			 * XXX just have an empty ptype?
			 */
			aprint_verbose("%s: skipping entry %u (%s), type %s\n",
			    pdk->dk_name, i, ent_guid_str, ptype_guid_str);
			continue;
		}
		strcpy(dkw.dkw_ptype, ptype);

		strcpy(dkw.dkw_parent, pdk->dk_name);
		dkw.dkw_offset = le64toh(ent->ent_lba_start);
		dkw.dkw_size = le64toh(ent->ent_lba_end) - dkw.dkw_offset + 1;

		/* XXX Make sure it falls within the disk's data area. */

		if (ent->ent_name[0] == 0x0000)
			strcpy(dkw.dkw_wname, ent_guid_str);
		else {
			for (j = 0; ent->ent_name[j] != 0x0000; j++) {
				/* XXX UTF-16 -> UTF-8 */
				dkw.dkw_wname[j] =
				    le16toh(ent->ent_name[j]) & 0xff;
			}
			dkw.dkw_wname[j] = '\0';
		}

		/*
		 * Try with the partition name first.  If that fails,
		 * use the GUID string.  If that fails, punt.
		 */
		if ((error = dkwedge_add(&dkw)) == EEXIST) {
			aprint_error("%s: wedge named '%s' already exists, "
			    "trying '%s'\n", pdk->dk_name,
			    dkw.dkw_wname, /* XXX Unicode */
			    ent_guid_str);
			strcpy(dkw.dkw_wname, ent_guid_str);
			error = dkwedge_add(&dkw);
		}
		if (error == EEXIST)
			aprint_error("%s: wedge named '%s' already exists, "
			    "manual intervention required\n", pdk->dk_name,
			    dkw.dkw_wname);
		else if (error)
			aprint_error("%s: error %d adding entry %u (%s), "
			    "type %s\n", pdk->dk_name, error, i, ent_guid_str,
			    ptype_guid_str);
	}
	error = 0;

 out:
	free(buf, M_DEVBUF);
	return (error);
}

DKWEDGE_DISCOVERY_METHOD_DECL(GPT, 0, dkwedge_discover_gpt);
