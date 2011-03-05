/*	$NetBSD: flashctl.c,v 1.1.2.2 2011/03/05 15:11:02 bouyer Exp $	*/

/*-
 * Copyright (c) 2010 Department of Software Engineering,
 *		      University of Szeged, Hungary
 * Copyright (c) 2010 Adam Hoka <ahoka@NetBSD.org>
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by the Department of Software Engineering, University of Szeged, Hungary
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

#include <sys/ioctl.h>
#include <sys/flashio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <inttypes.h>
#include <ctype.h>
#include <errno.h>


void usage(void);
int to_intmax(intmax_t *, const char *);

int
main(int argc, char **argv)
{
	char *device, *command;
	int fd, error = 0;
	intmax_t n = -1;

	setprogname(argv[0]);

	if (argc < 3)
		usage();

	device = argv[1];
	command = argv[2];
	argc -= 3;
	argv += 3;

	fd = open(device, O_RDWR, 0);
	if (fd == -1)
		err(EXIT_FAILURE, "can't open flash device");

	if (!strcmp("erase", command)) {
		struct flash_info_params ip;
		struct flash_erase_params ep;

		error = ioctl(fd, FLASH_GET_INFO, &ip);
		if (error) {
			warn("ioctl: FLASH_GET_INFO");
			goto out;
		}

		if (argc == 2) {
			error = to_intmax(&n, argv[0]);
			if (error) {
				warnx(strerror(error));
				goto out;
			}
			ep.ep_addr = n;

			if (!strcmp("all", argv[1])) {
				ep.ep_len = ip.ip_flash_size;
			} else {
				error = to_intmax(&n, argv[1]);
				if (error) {
					warnx(strerror(error));
					goto out;
				}
				ep.ep_len = n;
			}
		} else {
			warnx("invalid number of arguments");
			error = 1;
			goto out;
		}
		
		printf("Erasing %jx bytes starting from %jx\n",
		    (uintmax_t )ep.ep_len, (uintmax_t )ep.ep_addr);
		
		error = ioctl(fd, FLASH_ERASE_BLOCK, &ep);
		if (error) {
			warn("ioctl: FLASH_ERASE_BLOCK");
			goto out;
		}
	} else if (!strcmp("identify", command)) {
		struct flash_info_params ip;
		
		error = ioctl(fd, FLASH_GET_INFO, &ip);
		if (error) {
			warn("ioctl: FLASH_GET_INFO");
			goto out;
		}

		printf("Device type: ");
		switch (ip.ip_flash_type) {
		case FLASH_TYPE_NOR:
			printf("NOR flash");
			break;
		case FLASH_TYPE_NAND:
			printf("NAND flash");
			break;
		default:
			printf("unknown (%d)", ip.ip_flash_type);
		}
		printf("\n");

		/* TODO: humanize */
		printf("Capacity %jd Mbytes, %jd pages, %zu bytes/page\n", 
		    (intmax_t )ip.ip_flash_size / 1024 / 1024,
		    (intmax_t )ip.ip_flash_size / ip.ip_page_size,
		    ip.ip_page_size);

		if (ip.ip_flash_type == FLASH_TYPE_NAND) {
			printf("Block size %jd Kbytes, %jd pages/block\n",
			    (intmax_t )ip.ip_erase_size / 1024,
			    (intmax_t )ip.ip_erase_size / ip.ip_page_size);
		}
	} else if (!strcmp("badblocks", command)) {
		struct flash_info_params ip;
		struct flash_badblock_params bbp;
		flash_addr_t addr;
		bool hasbad = false;

		error = ioctl(fd, FLASH_GET_INFO, &ip);
		if (error) {
			warn("ioctl: FLASH_GET_INFO");
			goto out;
		}

		printf("Scanning for bad blocks: ");

		addr = 0;
		while (addr < ip.ip_flash_size) {
			bbp.bbp_addr = addr;
			
			error = ioctl(fd, FLASH_BLOCK_ISBAD, &bbp);
			if (error) {
				warn("ioctl: FLASH_BLOCK_ISBAD");
				goto out;
			}

			if (bbp.bbp_isbad) {
				hasbad = true;
				printf("0x%jx ", addr);
			}

			addr += ip.ip_erase_size;
		}

		if (hasbad)
			printf("Done.\n");
		else
			printf("No bad blocks found.\n");
	} else if (!strcmp("markbad", command)) {
		flash_addr_t address;
		
		error = to_intmax(&n, argv[1]);
		if (error) {
			warnx(strerror(error));
			goto out;
		}

		address = n;
		
		printf("Marking block 0x%jx as bad.\n",
		    (intmax_t )address);

		error = ioctl(fd, FLASH_BLOCK_MARKBAD, &address);
		if (error) {
			warn("ioctl: FLASH_BLOCK_MARKBAD");
			goto out;
		}
	} else {
		warnx("Unknown command");
		error = 1;
		goto out;
	}

out:
	close(fd);
	return error;
}

int
to_intmax(intmax_t *num, const char *str)
{
	char *endptr;

	errno = 0;
	if (str[0] == '0' && tolower((int )str[1]) == 'x') {
		if (!isxdigit((int )str[0]))
			return EINVAL;
		*num = strtoimax(str, &endptr, 16);
	} else {
		if (!isdigit((int )str[0]))
			return EINVAL;
		*num = strtoimax(str, &endptr, 10);
	}

	if (errno == ERANGE && (*num == INTMAX_MIN || *num == INTMAX_MAX)) {
		return ERANGE;
	}

	return 0;
}

void
usage(void)
{
	fprintf(stderr, "usage: %s device identify\n",
	    getprogname());
	fprintf(stderr, "       %s device erase <start address> <size>|all\n",
	    getprogname());
	fprintf(stderr, "       %s device badblocks\n",
	    getprogname());
	fprintf(stderr, "       %s device markbad <address>\n",
	    getprogname());
	
	exit(1);
}
