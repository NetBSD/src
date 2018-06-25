#	$NetBSD: Makefile,v 1.17.14.1 2018/06/25 07:26:11 pgoyette Exp $
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
