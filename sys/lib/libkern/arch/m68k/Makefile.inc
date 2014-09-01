#	$NetBSD: Makefile.inc,v 1.34 2014/09/01 07:32:35 matt Exp $

SRCS+=	bswap16.S bswap32.S bswap64.S
SRCS+=	memcmp.S memcpy.S memmove.S memset.S
SRCS+=	strcat.S strchr.S strcmp.S
SRCS+=	strcpy.S strlen.S strncmp.S strncpy.S strrchr.S
SRCS+=	scanc.S skpc.S
SRCS+=	htonl.S htons.S ntohl.S ntohs.S
SRCS+=	ffs.S

.if defined(MACHINE_ARCH) && (${MACHINE_ARCH} == "m68000")
SRCS+=	mulsi3.S divsi3.S udivsi3.S modsi3.S umodsi3.S
.endif
.if defined(MACHINE_ARCH) && (${MACHINE_ARCH} != "m68k")
random.S: .INVISIBLE
.endif
