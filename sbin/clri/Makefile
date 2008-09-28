#	$NetBSD: Makefile,v 1.13.20.1 2008/09/28 11:17:10 mjf Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	clri
MAN=	clri.8
SRCS=	clri.c ffs_bswap.c
.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs

.include <bsd.prog.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
COPTS.clri.c+=	-fno-strict-aliasing
.endif
