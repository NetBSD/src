#	$NetBSD: Makefile,v 1.15 2000/11/08 00:08:30 wiz Exp $

PROG=		diff
SRCS=		diff.c analyze.c cmpbuf.c io.c context.c ed.c normal.c util.c \
		dir.c regex.c side.c ifdef.c version.c getopt.c getopt1.c

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR} -I${DIST}/diffutils \
		-DPR_PROGRAM=\"/usr/bin/pr\"

TEXINFO=	diff.texi

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/diffutils

.include <bsd.info.mk>
.include <bsd.prog.mk>
