#	$NetBSD: Makefile,v 1.14.2.1 2012/10/30 19:00:32 yamt Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

WARNS?=	3	# XXX -Wsign-compare

.include <bsd.own.mk>

PROG=	dumplfs
SRCS=	dumplfs.c lfs_cksum.c misc.c
.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs
MAN=	dumplfs.8

.if defined(HAVE_GCC) || defined(HAVE_LLVM)
COPTS+=	-fno-strict-aliasing
.endif

.include <bsd.prog.mk>
