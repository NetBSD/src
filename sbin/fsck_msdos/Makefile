#	$NetBSD: Makefile,v 1.10 2005/01/20 16:39:23 xtraeme Exp $

.include <bsd.own.mk>

PROG=	fsck_msdos
MAN=	fsck_msdos.8
SRCS=	main.c check.c boot.c fat.c dir.c fsutil.c

FSCK=	${NETBSDSRCDIR}/sbin/fsck
CPPFLAGS+= -I${FSCK}
.PATH:	${FSCK}

.include <bsd.prog.mk>
