#	$NetBSD: Makefile,v 1.8 2001/04/06 16:52:41 wiz Exp $

PROG=	fsck_msdos
MAN=	fsck_msdos.8
SRCS=	main.c check.c boot.c fat.c dir.c fsutil.c

WARNS=	2

FSCK=	${.CURDIR}/../fsck
CPPFLAGS+= -I${FSCK}
.PATH:	${FSCK}

.include <bsd.prog.mk>
