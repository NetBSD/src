#	$NetBSD: Makefile,v 1.2 2019/02/04 04:05:16 mrg Exp $	

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

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.file_media.c+=	-Wno-error=implicit-fallthrough
COPTS.pathname.c+=	-Wno-error=implicit-fallthrough
COPTS.pdisk.c+=		-Wno-error=implicit-fallthrough
.endif

.include <bsd.prog.mk>
