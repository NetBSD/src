#	$NetBSD: Makefile,v 1.12 2010/04/11 08:23:52 hannken Exp $

.include <bsd.own.mk>

PROG=	fsck_msdos
MAN=	fsck_msdos.8
SRCS=	main.c check.c boot.c fat.c dir.c fsutil.c snapshot.c

FSCK=	${NETBSDSRCDIR}/sbin/fsck
DUMP=	${NETBSDSRCDIR}/sbin/dump
CPPFLAGS+= -I${FSCK} -I${DUMP}
.PATH:	${FSCK} ${DUMP}

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
