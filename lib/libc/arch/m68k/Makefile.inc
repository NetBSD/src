#	$NetBSD: Makefile.inc,v 1.9 2003/09/22 14:42:02 cl Exp $

KMINCLUDES=	arch/m68k/DEFS.h arch/m68k/SYS.h
KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S index.S rindex.S \
	memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S strncmp.S strncpy.S \
	htonl.S htons.S ntohl.S ntohs.S \
	ashldi3.S ashrdi3.S lshrdi3.S \
	bswap16.S bswap32.S bswap64.S

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S __sigtramp2.S
