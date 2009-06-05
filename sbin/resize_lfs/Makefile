#	$NetBSD: Makefile,v 1.5 2009/06/05 21:52:31 haad Exp $

WARNS?=	3	# XXX: sign-compare issues

.include <bsd.own.mk>

PROG=resize_lfs
SRCS=resize_lfs.c partutil.c
MAN=resize_lfs.8

FSCK=${NETBSDSRCDIR}/sbin/fsck
CPPFLAGS+=-I${FSCK}

DPADD+= ${LIBUTIL}
LDADD+= -lutil

LDADD+=-lprop
DPADD+=${LIBPROP}

.PATH: ${FSCK}

.include <bsd.prog.mk>
