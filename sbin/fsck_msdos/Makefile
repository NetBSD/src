#	$NetBSD: Makefile,v 1.9 2002/08/19 10:16:54 lukem Exp $

.include <bsd.own.mk>

PROG=	fsck_msdos
MAN=	fsck_msdos.8
SRCS=	main.c check.c boot.c fat.c dir.c fsutil.c

WARNS=	2

FSCK=	${NETBSDSRCDIR}/sbin/fsck
CPPFLAGS+= -I${FSCK}
.PATH:	${FSCK}

.include <bsd.prog.mk>
