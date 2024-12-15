/* $NetBSD: mkubootimage.c,v 1.30.8.1 2024/12/15 14:38:14 martin Exp $ */

/*-
 * Copyright (c) 2010 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
__RCSID("$NetBSD: mkubootimage.c,v 1.30.8.1 2024/12/15 14:38:14 martin Exp $");

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/endian.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "uboot.h"
#include "arm64.h"
#include "crc32.h"

#ifndef __arraycount
#define __arraycount(__x)	(sizeof(__x) / sizeof(__x[0]))
#endif

enum image_format {
	FMT_UNKNOWN,
	FMT_UIMG,	/* Legacy U-Boot image */
	FMT_ARM64,	/* Linux ARM64 image (booti) */
};

static enum uboot_image_os image_os = IH_OS_NETBSD;
static enum uboot_image_arch image_arch = IH_ARCH_UNKNOWN;
static enum uboot_image_type image_type = IH_TYPE_UNKNOWN;
static enum uboot_image_comp image_comp = IH_COMP_NONE;
static uint32_t image_loadaddr = 0;
static uint32_t image_entrypoint = 0;
static char *image_name;
static uint32_t image_magic = IH_MAGIC;
static enum image_format image_format = FMT_UIMG;
static int update_image = 0;

static const struct uboot_image_format {
	enum image_format format;
	const char *name;
} uboot_image_format[] = {
	{ FMT_UIMG,		"uimg" },
	{ FMT_ARM64,		"arm64" },
};

static enum image_format
get_image_format(const char *name)
{
	unsigned int i;

	for (i = 0; i < __arraycount(uboot_image_format); i++) {
		if (strcmp(uboot_image_format[i].name, name) == 0)
			return uboot_image_format[i].format;
	}

	return FMT_UNKNOWN;
}

static const char *
get_image_format_name(enum image_format format)
{
	unsigned int i;

	for (i = 0; i < __arraycount(uboot_image_format); i++) {
		if (uboot_image_format[i].format == format)
			return uboot_image_format[i].name;
	}

	return "Unknown";
}

static const struct uboot_os {
	enum uboot_image_os os;
	const char *name;
} uboot_os[] = {
	{ IH_OS_OPENBSD,	"openbsd" },
	{ IH_OS_NETBSD,		"netbsd" },
	{ IH_OS_FREEBSD,	"freebsd" },
	{ IH_OS_LINUX,		"linux" },
};

static enum uboot_image_os
get_os(const char *name)
{
	unsigned int i;

	for (i = 0; i < __arraycount(uboot_os); i++) {
		if (strcmp(uboot_os[i].name, name) == 0)
			return uboot_os[i].os;
	}

	return IH_OS_UNKNOWN;
}

static const char *
get_os_name(enum uboot_image_os os)
{
	unsigned int i;

	for (i = 0; i < __arraycount(uboot_os); i++) {
		if (uboot_os[i].os == os)
			return uboot_os[i].name;
	}

	return "Unknown";
}

static const struct uboot_arch {
	enum uboot_image_arch arch;
	const char *name;
} uboot_arch[] = {
	{ IH_ARCH_ARM,		"arm" },
	{ IH_ARCH_ARM64,	"arm64" },
	{ IH_ARCH_I386,		"i386" },
	{ IH_ARCH_MIPS,		"mips" },
	{ IH_ARCH_MIPS64,	"mips64" },
	{ IH_ARCH_PPC,		"powerpc" },
	{ IH_ARCH_OPENRISC,	"or1k" },
	{ IH_ARCH_SH,		"sh" },
};

static enum uboot_image_arch
get_arch(const char *name)
{
	unsigned int i;

	for (i = 0; i < __arraycount(uboot_arch); i++) {
		if (strcmp(uboot_arch[i].name, name) == 0)
			return uboot_arch[i].arch;
	}

	return IH_ARCH_UNKNOWN;
}

static const char * 
get_arch_name(enum uboot_image_arch arch)
{
	unsigned int i;

	for (i = 0; i < __arraycount(uboot_arch); i++) {
		if (uboot_arch[i].arch == arch)
			return uboot_arch[i].name;
	}

	return "Unknown";
}

static const struct uboot_type {
	enum uboot_image_type type;
	const char *name;
} uboot_type[] = {
	{ IH_TYPE_STANDALONE,		"standalone" },
	{ IH_TYPE_KERNEL,		"kernel" },
	{ IH_TYPE_KERNEL_NOLOAD,	"kernel_noload" },
	{ IH_TYPE_RAMDISK,		"ramdisk" },
	{ IH_TYPE_FILESYSTEM,		"fs" },
	{ IH_TYPE_SCRIPT,		"script" },
};

static enum uboot_image_type
get_type(const char *name)
{
	unsigned int i;

	for (i = 0; i < __arraycount(uboot_type); i++) {
		if (strcmp(uboot_type[i].name, name) == 0)
			return uboot_type[i].type;
	}

	return IH_TYPE_UNKNOWN;
}

static const char *
get_type_name(enum uboot_image_type type)
{
	unsigned int i;

	for (i = 0; i < __arraycount(uboot_type); i++) {
		if (uboot_type[i].type == type)
			return uboot_type[i].name;
	}

	return "Unknown";
}

static const struct uboot_comp {
	enum uboot_image_comp comp;
	const char *name;
} uboot_comp[] = {
	{ IH_COMP_NONE,		"none" },
	{ IH_COMP_GZIP,		"gz" },
	{ IH_COMP_BZIP2,	"bz2" },
	{ IH_COMP_LZMA,		"lzma" },
	{ IH_COMP_LZO,		"lzo" },
};

static enum uboot_image_comp
get_comp(const char *name)
{
	unsigned int i;

	for (i = 0; i < __arraycount(uboot_comp); i++) {
		if (strcmp(uboot_comp[i].name, name) == 0)
			return uboot_comp[i].comp;
	}

	return IH_COMP_NONE;
}

static const char *
get_comp_name(enum uboot_image_comp comp)
{
	unsigned int i;

	for (i = 0; i < __arraycount(uboot_comp); i++) {
		if (uboot_comp[i].comp == comp)
			return uboot_comp[i].name;
	}

	return "Unknown";
}

__dead static void
usage(void)
{
	fprintf(stderr,
"Usage: %s [-hu] -A <arm|arm64|i386|mips|mips64|or1k|powerpc|sh> -a address\n"
"\t-C <bz2|gz|lzma|lzo|none> [-E address] [-e address] [-t timestamp]\n"
"\t[-f <arm64|uimg>] [-m magic] -n image -O <freebsd|linux|netbsd|openbsd>\n"
"\t-T <fs|kernel|kernel_noload|ramdisk|script|standalone>\n"
"\tsource destination\n", getprogname());

	exit(EXIT_FAILURE);
}

static void
dump_header_uimg(struct uboot_image_header *hdr)
{
	time_t tm = be32toh(hdr->ih_time);

	printf(" magic:       0x%08x\n", be32toh(hdr->ih_magic));
	printf(" time:        %s", ctime(&tm));
	printf(" size:        %u\n", be32toh(hdr->ih_size));
	printf(" load addr:   0x%08x\n", be32toh(hdr->ih_load));
	printf(" entry point: 0x%08x\n", be32toh(hdr->ih_ep));
	printf(" data crc:    0x%08x\n", be32toh(hdr->ih_dcrc));
	printf(" os:          %d (%s)\n", hdr->ih_os,
	    get_os_name(hdr->ih_os));
	printf(" arch:        %d (%s)\n", hdr->ih_arch,
	    get_arch_name(hdr->ih_arch));
	printf(" type:        %d (%s)\n", hdr->ih_type,
	    get_type_name(hdr->ih_type));
	printf(" comp:        %d (%s)\n", hdr->ih_comp,
	    get_comp_name(hdr->ih_comp));
	printf(" name:        %s\n", hdr->ih_name);
	printf(" header crc:  0x%08x\n", hdr->ih_hcrc);
}

static int
generate_header_uimg(struct uboot_image_header *hdr, time_t repro_time,
    int kernel_fd)
{
	uint8_t *p;
	struct stat st;
	uint32_t crc, dsize, size_buf[2];
	int error;

	error = fstat(kernel_fd, &st);
	if (error == -1) {
		perror("stat");
		return errno;
	}

	if (st.st_size + sizeof(*hdr) > UINT32_MAX) {
		fprintf(stderr, "fatal: kernel too big\n");
		return EINVAL;
	}

	p = mmap(0, st.st_size, PROT_READ, MAP_FILE|MAP_SHARED, kernel_fd, 0);
	if (p == MAP_FAILED) {
		perror("mmap kernel");
		return EINVAL;
	}
	if (image_type == IH_TYPE_SCRIPT) {
		struct iovec iov[3];
		dsize = (uint32_t)(st.st_size + (sizeof(uint32_t) * 2));
		size_buf[0] = htobe32(st.st_size);
		size_buf[1] = htobe32(0);
		iov[0].iov_base = &size_buf[0];
		iov[0].iov_len = sizeof(size_buf[0]);
		iov[1].iov_base = &size_buf[1];
		iov[1].iov_len = sizeof(size_buf[1]);
		iov[2].iov_base = p;
		iov[2].iov_len = st.st_size;
		crc = crc32v(iov, 3);
	} else {
		dsize = update_image ? (uint32_t)(st.st_size - sizeof(*hdr)) :
		    (uint32_t)st.st_size;
		crc = crc32(p, st.st_size);
	}
	munmap(p, st.st_size);

	memset(hdr, 0, sizeof(*hdr));
	hdr->ih_magic = htobe32(image_magic);
	hdr->ih_time = htobe32(repro_time ? repro_time : st.st_mtime);
	hdr->ih_size = htobe32(dsize);
	hdr->ih_load = htobe32(image_loadaddr);
	hdr->ih_ep = htobe32(image_entrypoint);
	hdr->ih_dcrc = htobe32(crc);
	hdr->ih_os = image_os;
	hdr->ih_arch = image_arch;
	hdr->ih_type = image_type;
	hdr->ih_comp = image_comp;
	strlcpy((char *)hdr->ih_name, image_name, sizeof(hdr->ih_name));
	crc = crc32((void *)hdr, sizeof(*hdr));
	hdr->ih_hcrc = htobe32(crc);

	dump_header_uimg(hdr);

	return 0;
}

static void
dump_header_arm64(struct arm64_image_header *hdr)
{
	printf(" magic:       0x%" PRIx32 "\n", le32toh(hdr->magic));
	printf(" text offset: 0x%" PRIx64 "\n", le64toh(hdr->text_offset));
	printf(" image size:  %" PRIu64 "\n", le64toh(hdr->image_size));
	printf(" flags:       0x%" PRIx64 "\n", le64toh(hdr->flags));
}

static int
generate_header_arm64(struct arm64_image_header *hdr, int kernel_fd)
{
	struct stat st;
	uint32_t flags;
	int error;

	error = fstat(kernel_fd, &st);
	if (error == -1) {
		perror("stat");
		return errno;
	}

	flags = 0;
	flags |= ARM64_FLAGS_PAGE_SIZE_4K;
#if 0
	flags |= ARM64_FLAGS_PHYS_PLACEMENT_ANY;
#endif

	const uint64_t dsize = update_image ?
	    (uint64_t)st.st_size : (uint64_t)st.st_size + sizeof(*hdr);

	memset(hdr, 0, sizeof(*hdr));
	hdr->code0 = htole32(ARM64_CODE0);
	hdr->text_offset = htole64(image_entrypoint);
	hdr->image_size = htole64(dsize);
	hdr->flags = htole32(flags);
	hdr->magic = htole32(ARM64_MAGIC);

	dump_header_arm64(hdr);

	return 0;
}

static int
write_image(void *hdr, size_t hdrlen, int kernel_fd, int image_fd)
{
	uint8_t buf[4096];
	ssize_t rlen, wlen;
	struct stat st;
	uint32_t size_buf[2];
	int error;

	error = fstat(kernel_fd, &st);
	if (error == -1) {
		perror("stat");
		return errno;
	}

	wlen = write(image_fd, hdr, hdrlen);
	if (wlen != (ssize_t)hdrlen) {
		perror("short write");
		return errno;
	}

	if (image_type == IH_TYPE_SCRIPT) {
		size_buf[0] = htobe32(st.st_size);
		size_buf[1] = htobe32(0);
		wlen = write(image_fd, &size_buf, sizeof(size_buf));
		if (wlen != sizeof(size_buf)) {
			perror("short write");
			return errno;
		}
	}

	if (update_image) {
		if (lseek(kernel_fd, hdrlen, SEEK_SET) != (off_t)hdrlen) {
			perror("seek failed");
			return errno;
		}
	}

	while ((rlen = read(kernel_fd, buf, sizeof(buf))) > 0) {
		wlen = write(image_fd, buf, rlen);
		if (wlen != rlen) {
			perror("short write");
			return errno;
		}
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	struct uboot_image_header hdr_uimg;
	struct arm64_image_header hdr_arm64;
	const char *src, *dest;
	char *ep;
	int kernel_fd, image_fd;
	int ch;
	unsigned long long num;
	time_t repro_time = 0;

	while ((ch = getopt(argc, argv, "A:C:E:O:T:a:e:f:hm:n:t:u")) != -1) {
		switch (ch) {
		case 'A':	/* arch */
			image_arch = get_arch(optarg);
			break;
		case 'C':	/* comp */
			image_comp = get_comp(optarg);
			break;
		case 'O':	/* os */
			image_os = get_os(optarg);
			break;
		case 'T':	/* type */
			image_type = get_type(optarg);
			break;
		case 'a':	/* addr */
			errno = 0;
			num = strtoull(optarg, &ep, 0);
			if (*ep != '\0' || (errno == ERANGE &&
			    (num == ULLONG_MAX || num == 0)) ||
			    ((signed long long)num != (int32_t)num &&
			     num != (uint32_t)num))
				errx(1, "illegal number -- %s", optarg);
			image_loadaddr = (uint32_t)num;
			break;
		case 'E':	/* ep (byte swapped) */
		case 'e':	/* ep */
			errno = 0;
			num = strtoull(optarg, &ep, 0);
			if (*ep != '\0' || (errno == ERANGE &&
			    (num == ULLONG_MAX || num == 0)) ||
			    ((signed long long)num != (int32_t)num &&
			     num != (uint32_t)num))
				errx(1, "illegal number -- %s", optarg);
			image_entrypoint = (uint32_t)num;
			if (ch == 'E')
				image_entrypoint = bswap32(image_entrypoint);
			break;
		case 'f':	/* image format */
			image_format = get_image_format(optarg);
			break;
		case 'm':	/* magic */
			errno = 0;
			num = strtoul(optarg, &ep, 0);
			if (*ep != '\0' || (errno == ERANGE &&
			    (num == ULONG_MAX || num == 0)))
				errx(1, "illegal number -- %s", optarg);
			image_magic = (uint32_t)num;
			break;
		case 'n':	/* name */
			image_name = strdup(optarg);
			break;
		case 't':       /* FS timestamp */
			repro_time = atoll(optarg);
			break;
		case 'u':	/* update image */
			update_image = 1;
			break;
		case 'h':
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	if (image_entrypoint == 0)
		image_entrypoint = image_loadaddr;

	switch (image_format) {
	case FMT_UIMG:
		if (image_arch == IH_ARCH_UNKNOWN ||
		    image_type == IH_TYPE_UNKNOWN ||
		    image_name == NULL)
			usage();
			/* NOTREACHED */

		switch (image_type) {
		case IH_TYPE_SCRIPT:
		case IH_TYPE_RAMDISK:
		case IH_TYPE_KERNEL_NOLOAD:
			break;
		default:
			if (image_loadaddr == 0)
				usage();
				/* NOTREACHED */
			break;
		}
		break;

	case FMT_ARM64:
		if (image_arch != IH_ARCH_UNKNOWN &&
		    image_arch != IH_ARCH_ARM64)
			usage();
			/* NOTREACHED */

		break;

	default:
		usage();
		/* NOTREACHED */
	}

	src = argv[0];
	dest = argv[1];

	kernel_fd = open(src, O_RDONLY);
	if (kernel_fd == -1) {
		perror("open kernel");
		return EXIT_FAILURE;
	}
	image_fd = open(dest, O_WRONLY|O_CREAT|O_TRUNC, 0666);
	if (image_fd == -1) {
		perror("open image");
		return EXIT_FAILURE;
	}

	printf(" image type:  %s\n", get_image_format_name(image_format));

	switch (image_format) {
	case FMT_UIMG:
		if (generate_header_uimg(&hdr_uimg, repro_time, kernel_fd) != 0)
			return EXIT_FAILURE;

		if (write_image(&hdr_uimg, sizeof(hdr_uimg),
		    kernel_fd, image_fd) != 0)
			return EXIT_FAILURE;

		break;
	case FMT_ARM64:
		if (generate_header_arm64(&hdr_arm64, kernel_fd) != 0)
			return EXIT_FAILURE;

		if (write_image(&hdr_arm64, sizeof(hdr_arm64),
		    kernel_fd, image_fd) != 0)
			return EXIT_FAILURE;

		break;
	default:
		break;
	}

	close(image_fd);
	close(kernel_fd);

	return EXIT_SUCCESS;
}
