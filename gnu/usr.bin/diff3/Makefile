#	$NetBSD: Makefile,v 1.14 2002/09/19 03:09:35 lukem Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
DIFF=		${NETBSDSRCDIR}/gnu/usr.bin/diff

PROG=		diff3
SRCS=		diff3.c getopt.c getopt1.c version.c
CPPFLAGS+=	-I${DIFF} -I${DIST}/diffutils \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"

.PATH: ${DIFF} ${DIST}/diffutils

.include <bsd.prog.mk>
