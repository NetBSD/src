#	$NetBSD: Makefile,v 1.2 1996/09/23 16:27:56 christos Exp $

PROG=	fsck_msdos
MAN=	fsck_msdos.8
SRCS=	main.c check.c boot.c fat.c dir.c util.c
CFLAGS+= -I${.CURDIR}/../fsck 
.PATH: ${.CURDIR}/../fsck


.include <bsd.prog.mk>
