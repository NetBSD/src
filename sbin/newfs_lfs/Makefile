#	$NetBSD: Makefile,v 1.2 2002/08/19 10:17:00 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

.include <bsd.own.mk>

PROG=	newfs_lfs
SRCS=	dkcksum.c lfs.c lfs_cksum.c misc.c newfs.c
MAN=	newfs_lfs.8
.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs ${NETBSDSRCDIR}/sbin/disklabel

DPADD=  ${LIBUTIL}
LDADD=  -lutil

.include <bsd.prog.mk>
