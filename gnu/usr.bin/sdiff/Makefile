#	$NetBSD: Makefile,v 1.13 2002/09/19 03:09:42 lukem Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
DIFF=		${NETBSDSRCDIR}/gnu/usr.bin/diff

PROG=		sdiff
SRCS=		sdiff.c getopt.c getopt1.c version.c
CPPFLAGS+=	-I${DIFF} -I${DIST}/diffutils \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"

.PATH: ${DIFF} ${DIST}/diffutils

.include <bsd.prog.mk>
