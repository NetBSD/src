#	$NetBSD: Makefile,v 1.5 1997/05/06 20:45:05 gwr Exp $

PROG=	fsck_msdos
MAN=	fsck_msdos.8
SRCS=	main.c check.c boot.c fat.c dir.c fsutil.c
CFLAGS+= -I${.CURDIR}/../fsck 
.PATH.c:   ${.CURDIR}/../fsck

.include <bsd.prog.mk>
