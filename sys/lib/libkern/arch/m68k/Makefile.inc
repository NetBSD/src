#	$NetBSD: Makefile.inc,v 1.21 2001/05/16 03:30:39 fredette Exp $

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
LIBGCC_OBJS=	_mulsi3.o _divsi3.o _udivsi3.o _modsi3.o _umodsi3.o
${LIBGCC_OBJS}:
	@${AR} xv `${CC} --print-libgcc-file-name` $@
OBJS+=	${LIBGCC_OBJS}
SRCS+=	ffs.c random.c
.else
SRCS+=	ffs.S random.S
.endif
