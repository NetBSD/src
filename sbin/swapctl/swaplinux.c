/*-
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "swapctl.h"

/*
 * Definitions and structure taken from
 * https://github.com/util-linux/util-linux/blob/master/include/swapheader.h
 */

#define SWAP_VERSION 1
#define SWAP_UUID_LENGTH 16
#define SWAP_LABEL_LENGTH 16
#define SWAP_SIGNATURE "SWAPSPACE2"
#define SWAP_SIGNATURE_SZ (sizeof(SWAP_SIGNATURE) - 1)

struct swap_header_v1_2 {
	char	      bootbits[1024];    /* Space for disklabel etc. */
	uint32_t      version;
	uint32_t      last_page;
	uint32_t      nr_badpages;
	unsigned char uuid[SWAP_UUID_LENGTH];
	char	      volume_name[SWAP_LABEL_LENGTH];
	uint32_t      padding[117];
	uint32_t      badpages[1];
};

typedef union {
	struct swap_header_v1_2 header;
	struct {
		uint8_t	reserved[4096 - SWAP_SIGNATURE_SZ];
		char	signature[SWAP_SIGNATURE_SZ];
	} tail;
} swhdr_t;

#define sw_version	header.version
#define sw_volume_name	header.volume_name
#define sw_signature	tail.signature

int
is_linux_swap(const char *name)
{
	uint8_t buf[4096];
	swhdr_t *hdr = (swhdr_t *) buf;
	int fd;

	fd = open(name, O_RDONLY);
	if (fd == -1)
		return (-1);

	if (read(fd, buf, 4096) != 4096) {
		close(fd);
		return (-1);
	}
	close(fd);

	return (hdr->sw_version == SWAP_VERSION &&
	    !memcmp(hdr->sw_signature, SWAP_SIGNATURE, SWAP_SIGNATURE_SZ));
}

char *
add_wedge_linux_swap(const char *dev)
{
	size_t len = strlen(dev);
	struct dkwedge_info dkw;
	struct disklabel label;
	char disk[PATH_MAX];
	char ch;
	int fd;

	strlcpy(disk, dev, sizeof(disk));

	ch = disk[len-1];
	disk[len-1] = '\0';

	fd = open(disk, O_RDWR);
	if (fd == -1) {
		warn("%s: open", disk);
		return (NULL);
	}

	if (ioctl(fd, DIOCGDINFO, &label) == -1) {
		warn("%s: ioctl DIOCGDINFO", dev);
		goto bad;
	}

	struct partition *part = &label.d_partitions[ch - 'a'];

	if (part->p_fstype != FS_SWAP) {
		errno = EINVAL;
		goto bad;
	}

	(void) strlcpy((char *)dkw.dkw_wname, dev, sizeof(dkw.dkw_wname));
	(void) strlcpy(dkw.dkw_ptype, "swap", sizeof(dkw.dkw_ptype));
	/* Skip 8*512 bytes of header */
	dkw.dkw_offset = part->p_offset + 8;
	dkw.dkw_size = part->p_size - 8;

	if (ioctl(fd, DIOCAWEDGE, &dkw) == -1) {
		warn("%s: ioctl DIOCAWEDGE", dev);
		goto bad;
	}

	(void) close(fd);
	(void) snprintf(disk, sizeof(disk), "%s%s", _PATH_DEV, dkw.dkw_devname);
	return (strdup(disk));

bad:
	(void) close(fd);
	return (NULL);
}

int
del_wedge_linux_swap(const char *dev) {
	struct dkwedge_info dkw;
	char disk[PATH_MAX];
	int fd;

	if (strncmp(dev, "/dev/dk", strlen("/dev/dk")))
		return (0);

	fd = open(dev, O_RDWR);
	if (fd == -1) {
		warn("%s: open", dev);
		return (-1);
	}

	if (ioctl(fd, DIOCGWEDGEINFO, &dkw) == -1) {
		warn("%s: ioctl DIOCGWEDGEINFO,", dev);
		goto bad2;
	}
	(void) close(fd);

	(void) snprintf(disk, sizeof(disk), "%sr%s", _PATH_DEV, dkw.dkw_parent);
	fd = open(disk, O_RDWR);
	if (fd == -1) {
		warn("%s: open", disk);
		return (-1);
	}

	if (ioctl(fd, DIOCDWEDGE, &dkw) == -1) {
		warn("%s: ioctl DIOCDWEDGE", dev);
		goto bad2;
	}

	(void) close(fd);
	return (0);

bad2:
	(void) close(fd);
	return (-1);
}
