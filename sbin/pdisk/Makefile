#	$NetBSD: Makefile,v 1.3 2002/08/22 22:38:05 dbj Exp $	

.include <bsd.own.mk>

SRCDIR=	${NETBSDSRCDIR}/dist/pdisk
.PATH:	${SRCDIR}

.if (${MACHINE} == "macppc" || \
     ${MACHINE} == "mac68k")

PROG=	pdisk

SRCS=	bitfield.c convert.c deblock_media.c dump.c \
	file_media.c hfs_misc.c io.c layout_dump.c \
	media.c partition_map.c pathname.c pdisk.c util.c \
	validate.c errors.c
.endif

MAN=	pdisk.8 

CPPFLAGS+=	-I${SRCDIR}

CPPFLAGS+=	-D__unix__

WARNS=0

.include <bsd.prog.mk>
