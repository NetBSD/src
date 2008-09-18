#	$NetBSD: Makefile,v 1.13.24.1 2008/09/18 04:28:22 wrstuden Exp $
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
