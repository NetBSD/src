#	$NetBSD: Makefile,v 1.11 2006/08/26 18:14:28 christos Exp $

.include <bsd.own.mk>

PROG=	fsck_msdos
MAN=	fsck_msdos.8
SRCS=	main.c check.c boot.c fat.c dir.c fsutil.c

FSCK=	${NETBSDSRCDIR}/sbin/fsck
CPPFLAGS+= -I${FSCK}
.PATH:	${FSCK}

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
