#	$NetBSD: Makefile,v 1.20 2020/09/07 03:19:05 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

WARNS?=	3	# XXX -Wsign-compare

.include <bsd.own.mk>

PROG=	dumplfs
SRCS=	dumplfs.c lfs_cksum.c misc.c
.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs
MAN=	dumplfs.8
LDADD+=	-lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>
