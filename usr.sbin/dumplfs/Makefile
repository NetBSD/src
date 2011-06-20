#	$NetBSD: Makefile,v 1.14 2011/06/20 07:44:02 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

WARNS?=	3	# XXX -Wsign-compare

.include <bsd.own.mk>

PROG=	dumplfs
SRCS=	dumplfs.c lfs_cksum.c misc.c
.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs
MAN=	dumplfs.8

.if defined(HAVE_GCC)
COPTS+=	-fno-strict-aliasing
.endif

.include <bsd.prog.mk>
