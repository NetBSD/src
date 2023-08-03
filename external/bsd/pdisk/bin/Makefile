#	$NetBSD: Makefile,v 1.7 2023/08/03 13:36:08 rin Exp $	

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

COPTS.file_media.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}
COPTS.io.c+=		${CC_WNO_USE_AFTER_FREE}
COPTS.partition_map.c+=	${CC_WNO_STRINGOP_TRUNCATION}
COPTS.pathname.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}
COPTS.pdisk.c+=		${CC_WNO_IMPLICIT_FALLTHROUGH} ${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
