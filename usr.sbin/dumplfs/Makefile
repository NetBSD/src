#	$NetBSD: Makefile,v 1.11 2006/05/11 23:16:30 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

.include <bsd.own.mk>

PROG=	dumplfs
SRCS=	dumplfs.c lfs_cksum.c misc.c
.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs
MAN=	dumplfs.8

.if ${HAVE_GCC} == 4
COPTS+=	-fno-strict-aliasing
.endif

.include <bsd.prog.mk>
