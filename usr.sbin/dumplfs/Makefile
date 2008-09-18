#	$NetBSD: Makefile,v 1.11.20.1 2008/09/18 04:30:03 wrstuden Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

.include <bsd.own.mk>

PROG=	dumplfs
SRCS=	dumplfs.c lfs_cksum.c misc.c
.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs
MAN=	dumplfs.8

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
COPTS+=	-fno-strict-aliasing
.endif

.include <bsd.prog.mk>
