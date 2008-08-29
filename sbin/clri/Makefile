#	$NetBSD: Makefile,v 1.14 2008/08/29 00:02:23 gmcgarry Exp $
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
