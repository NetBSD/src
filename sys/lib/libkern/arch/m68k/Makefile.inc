#	$NetBSD: Makefile.inc,v 1.21.2.1 2002/06/23 17:49:48 jdolecek Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.S bswap32.S bswap64.S \
	bcmp.S bzero.S \
	memchr.c memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcasecmp.c \
	strcpy.S strlen.S strncmp.S strncasecmp.c strncpy.S strrchr.S \
	scanc.S skpc.S \
	htonl.S htons.S ntohl.S ntohs.S
.if defined(MACHINE_ARCH) && (${MACHINE_ARCH} == "m68000")
SRCS+=	mulsi3.S divsi3.S udivsi3.S modsi3.S umodsi3.S
SRCS+=	ffs.c random.c
.else
SRCS+=	ffs.S random.S
.endif
