/*	$NetBSD: iplsum.c,v 1.2.92.1 2009/05/13 17:17:44 jym Exp $	*/

/*
 * Calculate 32bit checksum of IPL and store in a certain location
 *
 * Written in 2003 by ITOH Yasufumi (itohy@NetBSD.org).
 * Public domain
 */

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#ifndef __BIT_TYPES_DEFINED__
typedef unsigned int	u_int32_t;
#endif

/* see README.ipl */
#define IPLOFF		(4*1024)	/* 4KB */
#define IPL1SIZE	(4*1024)	/* 4KB */
#define IPL2SIZE	(1*1024)	/* 1KB */
#define IPL2ONDISK	0x0400
#define IPL3SIZE	(3*512)		/* 1.5KB */
#define IPL3ONDISK	0x0A00
#define IPLSIZE		(IPL1SIZE + IPL2SIZE + IPL3SIZE)
#define BOOTSIZE	(IPLOFF + IPLSIZE)
#define BOOTBLOCKSIZE	8192

u_int32_t bootblk[BOOTSIZE / sizeof(u_int32_t) + 1];

#define SUMOFF		((IPLOFF + 4) / sizeof(u_int32_t))

#ifdef __STDC__
int main(int, char *[]);
#endif

int
main(int argc, char *argv[])
{
	FILE *fp;
	int len;
	u_int32_t sum, *p;
	int iploff, iplsumsize;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <input> <output>\n", argv[0]);
		return 1;
	}

	/* read file */
	if ((fp = fopen(argv[1], "rb")) == NULL) {
		perror(argv[1]);
		return 1;
	}
	if ((len = fread(bootblk, 1, sizeof bootblk, fp)) <= IPLOFF) {
		fprintf(stderr, "%s: too short\n", argv[1]);
		return 1;
	} else if (len > BOOTSIZE) {
		fprintf(stderr, "%s: too long\n", argv[1]);
		return 1;
	}
	(void) fclose(fp);

	/* sanity check */
	if ((ntohl(bootblk[0]) & 0xffff0000) != 0x80000000) {
		fprintf(stderr, "%s: bad LIF magic\n", argv[1]);
		return 1;
	}
	iploff = ntohl(bootblk[0xf0 / sizeof(u_int32_t)]);
	iplsumsize = ntohl(bootblk[0xf4 / sizeof(u_int32_t)]);
	printf("%d bytes free, ipl offset = %d, ipl sum size = %d\n",
	    BOOTSIZE - len, iploff, iplsumsize);
	if (iploff != IPLOFF || iplsumsize <= 0 || iplsumsize % 2048 ||
	    iploff + iplsumsize > BOOTBLOCKSIZE) {
		fprintf(stderr, "%s: bad ipl offset / size\n", argv[1]);
		return 1;
	}

	/* checksum */
	sum = 0;
	for (p = bootblk + IPLOFF / sizeof(u_int32_t);
	    p < bootblk + (IPLOFF + IPL1SIZE) / sizeof(u_int32_t); p++)
		sum += ntohl(*p);

	bootblk[SUMOFF] = htonl(ntohl(bootblk[SUMOFF]) - sum);

	/* transfer ipl part 2 */
	memcpy(bootblk + IPL2ONDISK / sizeof(u_int32_t),
	    bootblk + (IPLOFF + IPL1SIZE) / sizeof(u_int32_t),
	    IPL2SIZE);

	/* transfer ipl part 3 */
	memcpy(bootblk + IPL3ONDISK / sizeof(u_int32_t),
	    bootblk + (IPLOFF + IPL1SIZE + IPL2SIZE) / sizeof(u_int32_t),
	    IPL3SIZE);

	/* write file */
	if ((fp = fopen(argv[2], "wb")) == NULL) {
		perror(argv[2]);
		return 1;
	}
	if ((len = fwrite(bootblk, 1, BOOTBLOCKSIZE, fp)) != BOOTBLOCKSIZE) {
		if (len < 0)
			perror(argv[2]);
		else
			fprintf(stderr, "%s: short write\n", argv[2]);
		fclose(fp);
		(void) remove(argv[2]);
		return 1;
	}
	if (fclose(fp)) {
		perror(argv[2]);
		(void) remove(argv[2]);
		return 1;
	}

	return 0;
}
