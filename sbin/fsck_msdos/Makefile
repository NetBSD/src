#	$NetBSD: Makefile,v 1.7 1997/10/10 19:48:30 christos Exp $

PROG=	fsck_msdos
MAN=	fsck_msdos.8
SRCS=	main.c check.c boot.c fat.c dir.c fsutil.c
FSCK=	${.CURDIR}/../fsck
CPPFLAGS+= -I${FSCK}
.PATH:	${FSCK}

.include <bsd.prog.mk>
