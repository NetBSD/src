/*-
 * Copyright (c) 2002 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * CRC32 code derived from work by Gary S. Brown.
 */

#include <sys/cdefs.h>
#ifdef __FBSDID
__FBSDID("$FreeBSD: src/sbin/gpt/gpt.c,v 1.16 2006/07/07 02:44:23 marcel Exp $");
#endif
#ifdef __RCSID
__RCSID("$NetBSD: gpt.c,v 1.15.4.1 2012/10/26 09:02:27 sborrill Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/disk.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __NetBSD__
#include <util.h>
#include <ctype.h>
#include <prop/proplib.h>
#include <sys/drvctlio.h>
#endif

#include "map.h"
#include "gpt.h"

char	device_path[MAXPATHLEN];
const char *device_arg;
char	*device_name;

off_t	mediasz;

u_int	parts;
u_int	secsz;

int	readonly, verbose;

static uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t
crc32(const void *buf, size_t size)
{
	const uint8_t *p;
	uint32_t crc;

	p = buf;
	crc = ~0U;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}

uint8_t *
utf16_to_utf8(uint16_t *s16)
{
	static uint8_t *s8 = NULL;
	static size_t s8len = 0;
	size_t s8idx, s16idx, s16len;
	uint32_t utfchar;
	unsigned int c;

	s16len = 0;
	while (s16[s16len++] != 0)
		;
	if (s8len < s16len * 3) {
		if (s8 != NULL)
			free(s8);
		s8len = s16len * 3;
		s8 = calloc(s16len, 3);
	}
	s8idx = s16idx = 0;
	while (s16idx < s16len) {
		utfchar = le16toh(s16[s16idx++]);
		if ((utfchar & 0xf800) == 0xd800) {
			c = le16toh(s16[s16idx]);
			if ((utfchar & 0x400) != 0 || (c & 0xfc00) != 0xdc00)
				utfchar = 0xfffd;
			else
				s16idx++;
		}
		if (utfchar < 0x80) {
			s8[s8idx++] = utfchar;
		} else if (utfchar < 0x800) {
			s8[s8idx++] = 0xc0 | (utfchar >> 6);
			s8[s8idx++] = 0x80 | (utfchar & 0x3f);
		} else if (utfchar < 0x10000) {
			s8[s8idx++] = 0xe0 | (utfchar >> 12);
			s8[s8idx++] = 0x80 | ((utfchar >> 6) & 0x3f);
			s8[s8idx++] = 0x80 | (utfchar & 0x3f);
		} else if (utfchar < 0x200000) {
			s8[s8idx++] = 0xf0 | (utfchar >> 18);
			s8[s8idx++] = 0x80 | ((utfchar >> 12) & 0x3f);
			s8[s8idx++] = 0x80 | ((utfchar >> 6) & 0x3f);
			s8[s8idx++] = 0x80 | (utfchar & 0x3f);
		}
	}
	return (s8);
}

void
utf8_to_utf16(const uint8_t *s8, uint16_t *s16, size_t s16len)
{
	size_t s16idx, s8idx, s8len;
	uint32_t utfchar = 0;
	unsigned int c, utfbytes;

	s8len = 0;
	while (s8[s8len++] != 0)
		;
	s8idx = s16idx = 0;
	utfbytes = 0;
	do {
		c = s8[s8idx++];
		if ((c & 0xc0) != 0x80) {
			/* Initial characters. */
			if (utfbytes != 0) {
				/* Incomplete encoding. */
				s16[s16idx++] = htole16(0xfffd);
				if (s16idx == s16len) {
					s16[--s16idx] = 0;
					return;
				}
			}
			if ((c & 0xf8) == 0xf0) {
				utfchar = c & 0x07;
				utfbytes = 3;
			} else if ((c & 0xf0) == 0xe0) {
				utfchar = c & 0x0f;
				utfbytes = 2;
			} else if ((c & 0xe0) == 0xc0) {
				utfchar = c & 0x1f;
				utfbytes = 1;
			} else {
				utfchar = c & 0x7f;
				utfbytes = 0;
			}
		} else {
			/* Followup characters. */
			if (utfbytes > 0) {
				utfchar = (utfchar << 6) + (c & 0x3f);
				utfbytes--;
			} else if (utfbytes == 0)
				utfbytes = -1;
		}
		if (utfbytes == 0) {
			if (utfchar >= 0x10000 && s16idx + 2 >= s16len)
				utfchar = 0xfffd;
			if (utfchar >= 0x10000) {
				s16[s16idx++] =
				    htole16(0xd800 | ((utfchar>>10)-0x40));
				s16[s16idx++] =
				    htole16(0xdc00 | (utfchar & 0x3ff));
			} else
				s16[s16idx++] = htole16(utfchar);
			if (s16idx == s16len) {
				s16[--s16idx] = 0;
				return;
			}
		}
	} while (c != 0);
}

void
le_uuid_dec(void const *buf, uuid_t *uuid)
{
	u_char const *p;
	int i;

	p = buf;
	uuid->time_low = le32dec(p);
	uuid->time_mid = le16dec(p + 4);
	uuid->time_hi_and_version = le16dec(p + 6);
	uuid->clock_seq_hi_and_reserved = p[8];
	uuid->clock_seq_low = p[9];
	for (i = 0; i < _UUID_NODE_LEN; i++)
		uuid->node[i] = p[10 + i];
}

void
le_uuid_enc(void *buf, uuid_t const *uuid)
{
	u_char *p;
	int i;

	p = buf;
	le32enc(p, uuid->time_low);
	le16enc(p + 4, uuid->time_mid);
	le16enc(p + 6, uuid->time_hi_and_version);
	p[8] = uuid->clock_seq_hi_and_reserved;
	p[9] = uuid->clock_seq_low;
	for (i = 0; i < _UUID_NODE_LEN; i++)
		p[10 + i] = uuid->node[i];
}

int
parse_uuid(const char *s, uuid_t *uuid)
{
	uint32_t status;

	uuid_from_string(s, uuid, &status);
	if (status == uuid_s_ok)
		return (0);

	switch (*s) {
	case 'b':
		if (strcmp(s, "bios") == 0) {
			uuid_t bios = GPT_ENT_TYPE_BIOS;
			*uuid = bios;
			return (0);
		}
		break;
	case 'c':
		if (strcmp(s, "ccd") == 0) {
			uuid_t ccd = GPT_ENT_TYPE_NETBSD_CCD;
			*uuid = ccd;
			return (0);
		} else if (strcmp(s, "cgd") == 0) {
			uuid_t cgd = GPT_ENT_TYPE_NETBSD_CGD;
			*uuid = cgd;
			return (0);
		}
		break;
	case 'e':
		if (strcmp(s, "efi") == 0) {
			uuid_t efi = GPT_ENT_TYPE_EFI;
			*uuid = efi;
			return (0);
		}
		break;
	case 'f':
		if (strcmp(s, "ffs") == 0) {
			uuid_t nb_ffs = GPT_ENT_TYPE_NETBSD_FFS;
			*uuid = nb_ffs;
			return (0);
		}
		break;
	case 'h':
		if (strcmp(s, "hfs") == 0) {
			uuid_t hfs = GPT_ENT_TYPE_APPLE_HFS;
			*uuid = hfs;
			return (0);
		}
		break;
	case 'l':
		if (strcmp(s, "lfs") == 0) {
			uuid_t lfs = GPT_ENT_TYPE_NETBSD_LFS;
			*uuid = lfs;
			return (0);
		} else if (strcmp(s, "linux") == 0) {
			uuid_t lnx = GPT_ENT_TYPE_MS_BASIC_DATA;
			*uuid = lnx;
			return (0);
		}
		break;
	case 'r':
		if (strcmp(s, "raid") == 0) {
			uuid_t raid = GPT_ENT_TYPE_NETBSD_RAIDFRAME;
			*uuid = raid;
			return (0);
		}
		break;
	case 's':
		if (strcmp(s, "swap") == 0) {
			uuid_t sw = GPT_ENT_TYPE_NETBSD_SWAP;
			*uuid = sw;
			return (0);
		}
		break;
	case 'u':
		if (strcmp(s, "ufs") == 0) {
			uuid_t ufs = GPT_ENT_TYPE_NETBSD_FFS;
			*uuid = ufs;
			return (0);
		}
		break;
	case 'w':
		if (strcmp(s, "windows") == 0) {
			uuid_t win = GPT_ENT_TYPE_MS_BASIC_DATA;
			*uuid = win;
			return (0);
		}
		break;
	}
	return (EINVAL);
}

void*
gpt_read(int fd, off_t lba, size_t count)
{
	off_t ofs;
	void *buf;

	count *= secsz;
	buf = malloc(count);
	if (buf == NULL)
		return (NULL);

	ofs = lba * secsz;
	if (lseek(fd, ofs, SEEK_SET) == ofs &&
	    read(fd, buf, count) == (ssize_t)count)
		return (buf);

	free(buf);
	return (NULL);
}

int
gpt_write(int fd, map_t *map)
{
	off_t ofs;
	size_t count;

	count = map->map_size * secsz;
	ofs = map->map_start * secsz;
	if (lseek(fd, ofs, SEEK_SET) == ofs &&
	    write(fd, map->map_data, count) == (ssize_t)count)
		return (0);
	return (-1);
}

static int
gpt_mbr(int fd, off_t lba)
{
	struct mbr *mbr;
	map_t *m, *p;
	off_t size, start;
	unsigned int i, pmbr;

	mbr = gpt_read(fd, lba, 1);
	if (mbr == NULL)
		return (-1);

	if (mbr->mbr_sig != htole16(MBR_SIG)) {
		if (verbose)
			warnx("%s: MBR not found at sector %llu", device_name,
			    (long long)lba);
		free(mbr);
		return (0);
	}

	/*
	 * Differentiate between a regular MBR and a PMBR. This is more
	 * convenient in general. A PMBR is one with a single partition
	 * of type 0xee.
	 */
	pmbr = 0;
	for (i = 0; i < 4; i++) {
		if (mbr->mbr_part[i].part_typ == 0)
			continue;
		if (mbr->mbr_part[i].part_typ == 0xee)
			pmbr++;
		else
			break;
	}
	if (pmbr && i == 4 && lba == 0) {
		if (pmbr != 1)
			warnx("%s: Suspicious PMBR at sector %llu",
			    device_name, (long long)lba);
		else if (verbose > 1)
			warnx("%s: PMBR at sector %llu", device_name,
			    (long long)lba);
		p = map_add(lba, 1LL, MAP_TYPE_PMBR, mbr);
		return ((p == NULL) ? -1 : 0);
	}
	if (pmbr)
		warnx("%s: Suspicious MBR at sector %llu", device_name,
		    (long long)lba);
	else if (verbose > 1)
		warnx("%s: MBR at sector %llu", device_name, (long long)lba);

	p = map_add(lba, 1LL, MAP_TYPE_MBR, mbr);
	if (p == NULL)
		return (-1);
	for (i = 0; i < 4; i++) {
		if (mbr->mbr_part[i].part_typ == 0 ||
		    mbr->mbr_part[i].part_typ == 0xee)
			continue;
		start = le16toh(mbr->mbr_part[i].part_start_hi);
		start = (start << 16) + le16toh(mbr->mbr_part[i].part_start_lo);
		size = le16toh(mbr->mbr_part[i].part_size_hi);
		size = (size << 16) + le16toh(mbr->mbr_part[i].part_size_lo);
		if (start == 0 && size == 0) {
			warnx("%s: Malformed MBR at sector %llu", device_name,
			    (long long)lba);
			continue;
		}
		/* start is relative to the offset of the MBR itself. */
		start += lba;
		if (verbose > 2)
			warnx("%s: MBR part: type=%d, start=%llu, size=%llu",
			    device_name, mbr->mbr_part[i].part_typ,
			    (long long)start, (long long)size);
		if (mbr->mbr_part[i].part_typ != 15) {
			m = map_add(start, size, MAP_TYPE_MBR_PART, p);
			if (m == NULL)
				return (-1);
			m->map_index = i + 1;
		} else {
			if (gpt_mbr(fd, start) == -1)
				return (-1);
		}
	}
	return (0);
}

#ifdef __NetBSD__
static int
drvctl(const char *name, u_int *sector_size, off_t *media_size)
{
	prop_dictionary_t command_dict, args_dict, results_dict, data_dict,
	    disk_info, geometry;
	prop_string_t string;
	prop_number_t number;
	int dfd, res;
	char *dname, *p;

	if ((dfd = open("/dev/drvctl", O_RDONLY)) == -1) {
		warn("%s: /dev/drvctl", __func__);
		return -1;
	}

	command_dict = prop_dictionary_create();
	args_dict = prop_dictionary_create();

	string = prop_string_create_cstring_nocopy("get-properties");
	prop_dictionary_set(command_dict, "drvctl-command", string);
	prop_object_release(string);

	if ((dname = strdup(name[0] == 'r' ? name + 1 : name)) == NULL) {
		(void)close(dfd);
		return -1;
	}
	for (p = dname; *p; p++)
		continue;
	for (--p; p >= dname && !isdigit((unsigned char)*p); *p-- = '\0')
		continue;

	string = prop_string_create_cstring(dname);
	free(dname);
	prop_dictionary_set(args_dict, "device-name", string);
	prop_object_release(string);

	prop_dictionary_set(command_dict, "drvctl-arguments", args_dict);
	prop_object_release(args_dict);

	res = prop_dictionary_sendrecv_ioctl(command_dict, dfd, DRVCTLCOMMAND,
	    &results_dict);
	(void)close(dfd);
	prop_object_release(command_dict);
	if (res) {
		warn("%s: prop_dictionary_sendrecv_ioctl", __func__);
		errno = res;
		return -1;
	}

	number = prop_dictionary_get(results_dict, "drvctl-error");
	if ((errno = prop_number_integer_value(number)) != 0)
		return -1;

	data_dict = prop_dictionary_get(results_dict, "drvctl-result-data");
	if (data_dict == NULL)
		goto out;

	disk_info = prop_dictionary_get(data_dict, "disk-info");
	if (disk_info == NULL)
		goto out;

	geometry = prop_dictionary_get(disk_info, "geometry");
	if (geometry == NULL)
		goto out;

	number = prop_dictionary_get(geometry, "sector-size");
	if (number == NULL)
		goto out;

	*sector_size = prop_number_integer_value(number);

	number = prop_dictionary_get(geometry, "sectors-per-unit");
	if (number == NULL)
		goto out;

	*media_size = prop_number_integer_value(number) * *sector_size;

	return 0;
out:
	errno = EINVAL;
	return -1;
}
#endif

static int
gpt_gpt(int fd, off_t lba, int found)
{
	uuid_t type;
	off_t size;
	struct gpt_ent *ent;
	struct gpt_hdr *hdr;
	char *p, *s;
	map_t *m;
	size_t blocks, tblsz;
	unsigned int i;
	uint32_t crc;

	hdr = gpt_read(fd, lba, 1);
	if (hdr == NULL)
		return (-1);

	if (memcmp(hdr->hdr_sig, GPT_HDR_SIG, sizeof(hdr->hdr_sig)))
		goto fail_hdr;

	crc = le32toh(hdr->hdr_crc_self);
	hdr->hdr_crc_self = 0;
	if (crc32(hdr, le32toh(hdr->hdr_size)) != crc) {
		if (verbose)
			warnx("%s: Bad CRC in GPT header at sector %llu",
			    device_name, (long long)lba);
		goto fail_hdr;
	}

	tblsz = le32toh(hdr->hdr_entries) * le32toh(hdr->hdr_entsz);
	blocks = tblsz / secsz + ((tblsz % secsz) ? 1 : 0);

	/* Use generic pointer to deal with hdr->hdr_entsz != sizeof(*ent). */
	p = gpt_read(fd, le64toh(hdr->hdr_lba_table), blocks);
	if (p == NULL) {
		if (found) {
			if (verbose)
				warn("%s: Cannot read LBA table at sector %llu",
				    device_name, (unsigned long long)
				    le64toh(hdr->hdr_lba_table));
			return (-1);
		}
		goto fail_hdr;
	}

	if (crc32(p, tblsz) != le32toh(hdr->hdr_crc_table)) {
		if (verbose)
			warnx("%s: Bad CRC in GPT table at sector %llu",
			    device_name,
			    (long long)le64toh(hdr->hdr_lba_table));
		goto fail_ent;
	}

	if (verbose > 1)
		warnx("%s: %s GPT at sector %llu", device_name,
		    (lba == 1) ? "Pri" : "Sec", (long long)lba);

	m = map_add(lba, 1, (lba == 1)
	    ? MAP_TYPE_PRI_GPT_HDR : MAP_TYPE_SEC_GPT_HDR, hdr);
	if (m == NULL)
		return (-1);

	m = map_add(le64toh(hdr->hdr_lba_table), blocks, (lba == 1)
	    ? MAP_TYPE_PRI_GPT_TBL : MAP_TYPE_SEC_GPT_TBL, p);
	if (m == NULL)
		return (-1);

	if (lba != 1)
		return (1);

	for (i = 0; i < le32toh(hdr->hdr_entries); i++) {
		ent = (void*)(p + i * le32toh(hdr->hdr_entsz));
		if (uuid_is_nil((uuid_t *)&ent->ent_type, NULL))
			continue;

		size = le64toh(ent->ent_lba_end) - le64toh(ent->ent_lba_start) +
		    1LL;
		if (verbose > 2) {
			le_uuid_dec(&ent->ent_type, &type);
			uuid_to_string(&type, &s, NULL);
			warnx(
	"%s: GPT partition: type=%s, start=%llu, size=%llu", device_name, s,
			    (long long)le64toh(ent->ent_lba_start),
			    (long long)size);
			free(s);
		}
		m = map_add(le64toh(ent->ent_lba_start), size,
		    MAP_TYPE_GPT_PART, ent);
		if (m == NULL)
			return (-1);
		m->map_index = i + 1;
	}
	return (1);

 fail_ent:
	free(p);

 fail_hdr:
	free(hdr);
	return (0);
}

int
gpt_open(const char *dev)
{
	struct stat sb;
	int fd, mode, found;

	mode = readonly ? O_RDONLY : O_RDWR|O_EXCL;

	device_arg = dev;
#ifdef __FreeBSD__
	strlcpy(device_path, dev, sizeof(device_path));
	if ((fd = open(device_path, mode)) != -1)
		goto found;

	snprintf(device_path, sizeof(device_path), "%s%s", _PATH_DEV, dev);
	device_name = device_path + strlen(_PATH_DEV);
	if ((fd = open(device_path, mode)) != -1)
		goto found;
	return (-1);
 found:
#endif
#ifdef __NetBSD__
	device_name = device_path + strlen(_PATH_DEV);
	fd = opendisk(dev, mode, device_path, sizeof(device_path), 0);
	if (fd == -1)
		return -1;
#endif

	if (fstat(fd, &sb) == -1)
		goto close;

	if ((sb.st_mode & S_IFMT) != S_IFREG) {
#ifdef DIOCGSECTORSIZE
		if (ioctl(fd, DIOCGSECTORSIZE, &secsz) == -1 ||
		    ioctl(fd, DIOCGMEDIASIZE, &mediasz) == -1)
			goto close;
#endif
#ifdef __NetBSD__
		if (drvctl(device_name, &secsz, &mediasz) == -1)
			goto close;
#endif
	} else {
		secsz = 512;	/* Fixed size for files. */
		if (sb.st_size % secsz) {
			errno = EINVAL;
			goto close;
		}
		mediasz = sb.st_size;
	}

	/*
	 * We require an absolute minimum of 6 sectors. One for the MBR,
	 * 2 for the GPT header, 2 for the GPT table and one to hold some
	 * user data. Let's catch this extreme border case here so that
	 * we don't have to worry about it later.
	 */
	if (mediasz / secsz < 6) {
		errno = ENODEV;
		goto close;
	}

	if (verbose)
		warnx("%s: mediasize=%llu; sectorsize=%u; blocks=%llu",
		    device_name, (long long)mediasz, secsz,
		    (long long)(mediasz / secsz));

	map_init(mediasz / secsz);

	if (gpt_mbr(fd, 0LL) == -1)
		goto close;
	if ((found = gpt_gpt(fd, 1LL, 1)) == -1)
		goto close;
	if (gpt_gpt(fd, mediasz / secsz - 1LL, found) == -1)
		goto close;

	return (fd);

 close:
	close(fd);
	return (-1);
}

void
gpt_close(int fd)
{
	/* XXX post processing? */
	close(fd);
}

static struct {
	int (*fptr)(int, char *[]);
	const char *name;
} cmdsw[] = {
	{ cmd_add, "add" },
	{ cmd_biosboot, "biosboot" },
	{ cmd_create, "create" },
	{ cmd_destroy, "destroy" },
	{ NULL, "help" },
	{ cmd_label, "label" },
	{ cmd_migrate, "migrate" },
	{ cmd_recover, "recover" },
	{ cmd_remove, "remove" },
	{ NULL, "rename" },
	{ cmd_show, "show" },
	{ NULL, "verify" },
	{ NULL, NULL }
};

__dead static void
usage(void)
{
	extern const char addmsg[], biosbootmsg[], createmsg[], destroymsg[];
	extern const char labelmsg1[], labelmsg2[], labelmsg3[];
	extern const char migratemsg[], recovermsg[], removemsg1[];
	extern const char removemsg2[], showmsg[];

	fprintf(stderr,
	    "usage: %s %s\n"
	    "       %s %s\n"
	    "       %s %s\n"
	    "       %s %s\n"
	    "       %s %s\n"
	    "       %s %s\n"
	    "       %*s %s\n"
	    "       %s %s\n"
	    "       %s %s\n"
	    "       %s %s\n"
	    "       %s %s\n"
	    "       %s %s\n",
	    getprogname(), addmsg,
	    getprogname(), biosbootmsg,
	    getprogname(), createmsg,
	    getprogname(), destroymsg,
	    getprogname(), labelmsg1,
	    getprogname(), labelmsg2,
	    (int)strlen(getprogname()), "", labelmsg3,
	    getprogname(), migratemsg,
	    getprogname(), recovermsg,
	    getprogname(), removemsg1,
	    getprogname(), removemsg2,
	    getprogname(), showmsg);
	exit(1);
}

static void
prefix(const char *cmd)
{
	char *pfx;
	const char *prg;

	prg = getprogname();
	pfx = malloc(strlen(prg) + strlen(cmd) + 2);
	/* Don't bother failing. It's not important */
	if (pfx == NULL)
		return;

	sprintf(pfx, "%s %s", prg, cmd);
	setprogname(pfx);
}

int
main(int argc, char *argv[])
{
	char *cmd, *p;
	int ch, i;

	/* Get the generic options */
	while ((ch = getopt(argc, argv, "p:rv")) != -1) {
		switch(ch) {
		case 'p':
			if (parts > 0)
				usage();
			parts = strtoul(optarg, &p, 10);
			if (*p != 0 || parts < 1)
				usage();
			break;
		case 'r':
			readonly = 1;
			break;
		case 'v':
			verbose++;
			break;
		default:
			usage();
		}
	}
	if (!parts)
		parts = 128;

	if (argc == optind)
		usage();

	cmd = argv[optind++];
	for (i = 0; cmdsw[i].name != NULL && strcmp(cmd, cmdsw[i].name); i++);

	if (cmdsw[i].fptr == NULL)
		errx(1, "unknown command: %s", cmd);

	prefix(cmd);
	return ((*cmdsw[i].fptr)(argc, argv));
}
