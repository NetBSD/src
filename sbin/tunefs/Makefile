#	$NetBSD: Makefile,v 1.15 2026/05/01 20:39:26 christos Exp $
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
