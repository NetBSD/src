#	$NetBSD: Makefile,v 1.1 2013/03/23 15:42:40 christos Exp $	

.include <bsd.own.mk>
WARNS=5
BINDIR=/sbin

SRCDIR=	${.CURDIR}/../dist
.PATH:	${SRCDIR}

PROG=	pdisk

SRCS=	bitfield.c convert.c deblock_media.c dump.c \
	file_media.c hfs_misc.c io.c layout_dump.c \
	media.c partition_map.c pathname.c pdisk.c util.c \
	validate.c errors.c

MAN=	pdisk.8 

CPPFLAGS+=	-I${SRCDIR}

CPPFLAGS+=	-D__unix__

.include <bsd.prog.mk>
