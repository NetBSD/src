#	$NetBSD: Makefile,v 1.14.100.1 2026/11/04 23:19:44 perseant Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	tunefs
MAN=	tunefs.8
SRCS=	tunefs.c ffs_bswap.c openspecial.c
LDADD+=-lutil

.PATH: ${.CURDIR}/../fsck
CPPFLAGS+=-I${.CURDIR}/../fsck

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs

.include <bsd.prog.mk>
