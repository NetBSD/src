#	$NetBSD: Makefile.inc,v 1.10 2004/09/26 21:13:27 jmmv Exp $

KMINCLUDES=	arch/m68k/DEFS.h arch/m68k/SYS.h
KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S index.S rindex.S \
	memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S strncmp.S strncpy.S \
	htonl.S htons.S ntohl.S ntohs.S \
	ashldi3.S ashrdi3.S lshrdi3.S \
	bswap16.S bswap32.S bswap64.S

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S __sigtramp2.S

.if ${MKSOFTFLOAT} != "no"
CPPFLAGS+= -DSOFTLOFLOAT_NEED_FIXUNS -DSOFTFLOAT -DSOFTFLOATM68K_FOR_GCC
.  include <softfloat/Makefile.inc>
.endif
