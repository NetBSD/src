#	$NetBSD: Makefile,v 1.16 2002/09/19 03:09:35 lukem Exp $

.include <bsd.own.mk>

PROG=		diff
SRCS=		diff.c analyze.c cmpbuf.c io.c context.c ed.c normal.c util.c \
		dir.c regex.c side.c ifdef.c version.c getopt.c getopt1.c

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR} -I${DIST}/diffutils \
		-DPR_PROGRAM=\"/usr/bin/pr\"

TEXINFO=	diff.texi

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/diffutils

.include <bsd.info.mk>
.include <bsd.prog.mk>
