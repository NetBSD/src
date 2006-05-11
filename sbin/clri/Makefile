#	$NetBSD: Makefile,v 1.13 2006/05/11 23:16:29 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	clri
MAN=	clri.8
SRCS=	clri.c ffs_bswap.c
.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS.clri.c+=	-fno-strict-aliasing
.endif
